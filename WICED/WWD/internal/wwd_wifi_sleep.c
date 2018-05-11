/*
 * Copyright 2017, Cypress Semiconductor Corporation or a subsidiary of 
 * Cypress Semiconductor Corporation. All Rights Reserved.
 * 
 * This software, associated documentation and materials ("Software"),
 * is owned by Cypress Semiconductor Corporation
 * or one of its subsidiaries ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products. Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

#include <string.h> /* For strlen, stricmp, memcpy. memset */
#include <stddef.h>
#include "wwd_management.h"
#include "wwd_wifi.h"
#include "wwd_assert.h"
#include "wwd_wlioctl.h"
#include "wwd_debug.h"
#include "platform/wwd_platform_interface.h"
#include "network/wwd_buffer_interface.h"
#include "network/wwd_network_constants.h"
#include "RTOS/wwd_rtos_interface.h"
#include "internal/wwd_sdpcm.h"
#include "internal/wwd_bcmendian.h"
#include "internal/wwd_ap.h"
#include "internal/wwd_internal.h"
#include "wwd_bus_protocol.h"
#include "wiced_utilities.h"
#include "wiced_deep_sleep.h"
#include "wwd_events.h"
#include "wwd_wifi_sleep.h"
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"
#include "wwd_wifi_chip_common.h"

#define CHECK_RETURN( expr )  { wwd_result_t check_res = (expr); if ( check_res != WWD_SUCCESS ) { wiced_assert("Command failed\n", 0 == 1); return check_res; } }

/******************************************************
 *             Local Structures
 ******************************************************/

/******************************************************
 *             Static Variables
 ******************************************************/
static const wwd_event_num_t ulp_events[] = { WLC_E_ULP, WLC_E_NONE };
static volatile wwd_ds_state_t WICED_DEEP_SLEEP_SAVED_VAR( wwd_wifi_ds1_state ) = STATE_DS_DISABLED;
static void *WICED_DEEP_SLEEP_SAVED_VAR( ds_enabling_timer ) = NULL;

static volatile wwd_ds1_state_change_callback_t WICED_DEEP_SLEEP_SAVED_VAR( ds_notify_callback ) = NULL;
static volatile void* WICED_DEEP_SLEEP_SAVED_VAR( ds_notify_callback_user_parameter ) = NULL;


/******************************************************
 *            Function prototypes
 ******************************************************/
static void ds_entry_delay_done( void *param );
static void ds_state_set( wwd_ds_state_t new_state );

void* wwd_wifi_event_ulp_handler( const wwd_event_header_t* event_header, const uint8_t* event_data, /*@returned@*/ void* handler_user_data );

/* enable and set up wowl patterns */
wwd_result_t wwd_wifi_wowl_enable( wwd_interface_t interface, uint32_t wowl_caps, uint32_t wowl_os,
     wl_mkeep_alive_pkt_t *wowl_keepalive_data, uint8_t *pattern_data, uint32_t pattern_data_size, uint32_t *arp_host_ip_v4_address )
{
    wwd_result_t wwd_result = wwd_wifi_set_iovar_value( IOVAR_STR_WOWL, wowl_caps, interface );
    WPRINT_WWD_DEBUG(("wowl\n"));

    if ( WWD_SUCCESS != wwd_result )
    {
        WPRINT_WWD_ERROR(("Error on wowl set %d\n", wwd_result));
        return wwd_result;
    }

    /* Only bother setting wowl_os to a non-zero value */
    if ( 0 != wowl_os )
    {
        WPRINT_WWD_DEBUG(("wowl-os\n"));
        CHECK_RETURN( wwd_wifi_set_iovar_value( IOVAR_STR_WOWL_OS, wowl_os, interface ) );
    }


    /* Set up any type of wowl requested */
    if ( NULL != wowl_keepalive_data )
    {
        CHECK_RETURN( wwd_wifi_set_iovar_buffer( IOVAR_STR_WOWL_KEEP_ALIVE, wowl_keepalive_data, (uint16_t) ( wowl_keepalive_data->length + wowl_keepalive_data->len_bytes ), interface ) );
    }

    if ( NULL != pattern_data )
    {
        WPRINT_WWD_DEBUG(("pre-pattern add\n"));
        CHECK_RETURN( wwd_wifi_set_iovar_buffer( IOVAR_STR_WOWL_PATTERN, pattern_data, (uint16_t) pattern_data_size, interface ) );
        WPRINT_WWD_DEBUG(("post-pattern add\n"));
    }

    if ( NULL != arp_host_ip_v4_address )
    {
        WPRINT_WWD_DEBUG(("add ip"));
        CHECK_RETURN( wwd_wifi_set_iovar_buffer( IOVAR_STR_WOWL_ARP_HOST_IP, arp_host_ip_v4_address, sizeof( *arp_host_ip_v4_address ), interface ) );
    }

    WPRINT_WWD_DEBUG(("End wowl enable\n"));
    return WWD_SUCCESS;
}

/* called when state is changed */
static void ds_state_set( wwd_ds_state_t new_state )
{
    wwd_wifi_ds1_state = new_state;

    if ( NULL != ds_notify_callback )
    {
        ds_notify_callback( (void*)ds_notify_callback_user_parameter );
    }
}

/* Register a callback for deep sleep state changes; current state can then be queried */
wwd_result_t wwd_wifi_ds1_set_state_change_callback( wwd_ds1_state_change_callback_t callback, void *user_parameter )
{
    ds_notify_callback                = callback;
    ds_notify_callback_user_parameter = user_parameter;

    return WWD_SUCCESS;
}

/* Get current DS1 state */
extern wwd_ds_state_t wwd_wifi_ds1_get_state( void )
{
    return wwd_wifi_ds1_state;
}

void* wwd_wifi_event_ulp_handler( const wwd_event_header_t* event_header, const uint8_t* event_data, /*@returned@*/ void* handler_user_data )
{
    wl_ulp_event_t*  ulp_evt = (wl_ulp_event_t *)event_data;

    UNUSED_PARAMETER(event_header);
    UNUSED_PARAMETER(event_data);
    UNUSED_PARAMETER(handler_user_data);

    WPRINT_WWD_DEBUG(("%s : ULP event handler triggered [%d]\n", __FUNCTION__, ulp_evt->ulp_dongle_action));

    switch ( ulp_evt->ulp_dongle_action )
    {
        case WL_ULP_ENTRY:
          /* Just mark DS1 enabled flag to TRUE so that DS1 exit interrupt will be processed by WWD */
            break;
        default:
            break;
    }
    ds_state_set( STATE_DS_ENABLED );
    return NULL;
}

static void ds_entry_delay_done( void *param )
{
    UNUSED_PARAMETER(param);

    /* Now safe to say ds1 is enabled; deinit wait timer */
    ds_state_set( STATE_DS_ENABLED );

    HOST_RTOS_TIMER_IFC_CALL( host_rtos_stop_timer )( ds_enabling_timer );

    HOST_RTOS_TIMER_IFC_CALL( host_rtos_deinit_timer )( ds_enabling_timer );

    HOST_RTOS_TIMER_IFC_CALL( host_rtos_free_timer )( &ds_enabling_timer );

}

/* have device go into ds1 after given wait period */
wwd_result_t wwd_wifi_enter_ds1( wwd_interface_t interface, uint32_t ulp_wait_milliseconds )
{
    uint32_t ulp_enable         = 1;
    int rtos_result = 0;

    rtos_result = HOST_RTOS_TIMER_IFC_CALL( host_rtos_alloc_timer )( &ds_enabling_timer );

    if ( 0 != rtos_result )
    {
        wiced_assert("Timer failure on alloc", 0 != 0);
        goto error;
    }

    rtos_result = HOST_RTOS_TIMER_IFC_CALL( host_rtos_init_timer  )( ds_enabling_timer, ulp_wait_milliseconds, ds_entry_delay_done, NULL  );
    if ( 0 != rtos_result )
    {
        wiced_assert("Timer failure on init", 0 != 0);
        goto error;
    }

    WPRINT_WWD_DEBUG(("Setting ulp evt hdlr\n"));

    /* listen for ULP ready: avoid race condition by doing this first */
    CHECK_RETURN( wwd_management_set_event_handler( ulp_events, wwd_wifi_event_ulp_handler, NULL, WWD_STA_INTERFACE ) );

    WPRINT_WWD_DEBUG(("ulp wait\n"));

    /* then trigger ULP */
    CHECK_RETURN( wwd_wifi_set_iovar_value( IOVAR_STR_ULP_WAIT, ulp_wait_milliseconds, interface ) );
    WPRINT_WWD_DEBUG(("ulp\n"));

    CHECK_RETURN( wwd_wifi_set_iovar_value( IOVAR_STR_ULP, ulp_enable, interface ) );

    WPRINT_WWD_DEBUG(("ulp set to enable\n"));

    /* wait for number of ulp wait milliseconds prior to setting enabled, to allow FW to settle */
    ds_state_set( STATE_DS_ENABLING );

    rtos_result = HOST_RTOS_TIMER_IFC_CALL( host_rtos_start_timer )( ds_enabling_timer );

    if ( 0 != rtos_result )
    {
        wiced_assert("Timer failure on start", 0 != 0);
        goto error;
    }

    return WWD_SUCCESS;
error:
    return WWD_RTOS_ERROR;
}

/* turn off wowl and clear any previously added patterns */
wwd_result_t wwd_wifi_wowl_disable( wwd_interface_t interface )
{
    uint32_t val_disable = 0;
    CHECK_RETURN( wwd_wifi_set_iovar_buffer( IOVAR_STR_WOWL, &val_disable, sizeof( val_disable ), interface) );
    CHECK_RETURN( wwd_wifi_set_iovar_void( IOVAR_STR_WOWL_PATTERN_CLR, interface ) );
    return WWD_SUCCESS;
}

wiced_bool_t wwd_wifi_ds1_needs_wake( void )
{
    /* Is WWD in ds1? */
    if ( STATE_DS_ENABLED == wwd_wifi_ds1_state )
    {
        /* If so, is there tx packet to send or is there a wake up interrupt from the host? If so we do need to wake up */
        /* WICED_TRUE == wwd_sdpcm_has_tx_packet( ) ?? is that needed */
        if ( WICED_TRUE == wwd_bus_wake_interrupt_present( ) )
        {
            WPRINT_WWD_DEBUG(("wake need detected\n"));
            return WICED_TRUE;
        }
    }
    return WICED_FALSE;
}

wwd_result_t wwd_wifi_ds1_finish_wake( void )
{
    wwd_result_t wwd_result = WWD_SUCCESS;

    if ( STATE_DS_ENABLED != wwd_wifi_ds1_state )
    {
        WPRINT_WWD_DEBUG(("ds1 not yet enabled\n"));
        return WWD_SUCCESS;
    }

    /* reinit tx and rx counters FIRST, so code is in sync */
    wwd_sdpcm_bus_vars_init( );
    wwd_result = wwd_wlan_bus_complete_ds_wake( );

    if ( WWD_SUCCESS != wwd_result )
    {
        WPRINT_WWD_ERROR(("Could not complete bus ds wake\n"));
        /*@-unreachable@*/ /* Reachable after hitting assert */
        return wwd_result;
        /*@+unreachable@*/
    }

    wwd_result = wwd_bus_reinit();
    if ( wwd_result != WWD_SUCCESS )
    {
       WPRINT_WWD_ERROR(("SDIO Reinit failed with error \n"));
    }

    /* clear */
    ds_state_set( STATE_DS_DISABLED );

    return WWD_SUCCESS;
}
