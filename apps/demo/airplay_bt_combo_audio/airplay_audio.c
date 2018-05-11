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

/** @airplay_audio.c
 *
 * File Description
 *
 */

#include "wiced.h"
#include "airplay_bt_combo_audio.h"
#include "hashtable.h"
#include "airplay_audio.h"
#include "platform_audio.h"
#include "app_dct.h"
#if defined(USE_APPLE_WAC)
#include "apple_wac.h"
#endif

#ifdef USE_AUDIO_DISPLAY
#include "audio_display.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define PRINT_ERR(x,...) \
    do \
    { \
        fprintf(stderr, "[ERROR] %s: ", __PRETTY_FUNCTION__); \
        fprintf(stderr, x, ##__VA_ARGS__); \
    } while(0)

#if defined(USE_APPLE_WAC)

#define AIRPLAY_START_WAC_IF_WIFI_DEVICE_IS_UNCONFIGURED() \
    do \
    { \
        if ( dct_wifi->device_configured == WICED_FALSE ) \
        { \
            do_wac(); \
        } \
    } while(0)

#define kHexDigitsUppercase         "0123456789ABCDEF"

#else

#define AIRPLAY_START_WAC_IF_WIFI_DEVICE_IS_UNCONFIGURED() \
    do \
    { \
    } while(0)
#endif /* USE_APPLE_WAC */

/******************************************************
 *                    Constants
 ******************************************************/

#define MY_DEVICE_NAME                           "wiced_airplay_bt_audio"
#define MY_DEVICE_MODEL                          "1.0"
#define MY_DEVICE_MANUFACTURER                   "Cypress"

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/


static void wiced_airplay_init_service(void);
static wiced_result_t app_update_playback_property_callback( void* property_value, airplay_playback_property_t property_type );
static wiced_result_t airplay_state_change_callback ( airplay_server_state_t state );

static wiced_result_t wiced_airplay_button_event_handler( app_service_action_t action );

airplay_dct_t*          dct_app;
platform_dct_wifi_config_t* dct_wifi;

/******************************************************
 *               Variable Definitions
 ******************************************************/

static airplay_audio_context_t airplay_ctx;

static const wiced_ip_setting_t ap_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};

static airplay_dacp_connection_state_t airplay_dacp_state = DACP_DISCONNECTED;

#if defined(USE_APPLE_WAC)
static uint8_t oui_array[] = {0x00, 0xA0, 0x40};

static apple_wac_info_t apple_wac_info =
{
        .supports_airplay       = WICED_TRUE,
        .supports_airprint      = WICED_FALSE,
        .supports_5ghz_networks = WICED_TRUE,
        .has_app                = WICED_FALSE,
        .auth_chip_location     = AUTH_CHIP_LOCATION_INTERNAL,
        .mdns_desired_hostname  = NULL,
        .mdns_nice_name         = NULL,
        .firmware_revision      = FIRMWARE_VERSION,
        .hardware_revision      = MY_DEVICE_MODEL,
        .serial_number          = NULL,
        .name                   = NULL,
        .model                  = MY_DEVICE_MODEL,
        .manufacturer           = MY_DEVICE_MANUFACTURER,
        .oui                    = oui_array,
        .mfi_protocols          = NULL,
        .num_mfi_protocols      = 0,
        .bundle_seed_id         = NULL,
        .soft_ap_channel        = 0,
        .out_new_name           = NULL,
        .out_play_password      = NULL,
        .device_random_id       = "XX:XX:XX:XX:XX:XX",
};
#endif /* USE_APPLE_WAC */

/******************************************************
 *               Function Definitions
 ******************************************************/

static void wiced_airplay_ctx_deinit(void)
{
    if(dct_app != NULL)
    {
        wiced_dct_read_unlock( dct_app, WICED_TRUE );
    }
    if(dct_wifi != NULL)
    {
        wiced_dct_read_unlock( dct_wifi, WICED_TRUE );
    }
    dct_app = NULL;
    dct_wifi = NULL;
    airplay_ctx.is_airplay_server_up = WICED_FALSE;
}

static void wiced_airplay_ctx_init(void)
{
    dct_app = NULL;
    dct_wifi = NULL;
    airplay_ctx.airplay_interface = WICED_STA_INTERFACE;
    airplay_ctx.is_airplay_server_up = WICED_FALSE;
#ifdef USE_AUDIO_DISPLAY
    airplay_ctx.ssid_str[0] = '\0';
#endif /* USE_AUDIO_DISPLAY */
}



#if defined(USE_APPLE_WAC)
static char* mac_address_to_string( const void *mac_address, char *mac_address_string_format )
{
    const uint8_t *     src;
    const uint8_t *     end;
    char *              dst;
    uint8_t             b;

    src = (const uint8_t *) mac_address;
    end = src + 6; // size of MAC address in bytes
    dst = mac_address_string_format;
    while( src < end )
    {
       if( dst != mac_address_string_format ) *dst++ = ':';
       b = *src++;
       *dst++ = kHexDigitsUppercase[ b >> 4 ];
       *dst++ = kHexDigitsUppercase[ b & 0xF ];
    }
    *dst = '\0';
    return( mac_address_string_format );
}

static wiced_result_t do_wac(void)
{
    wiced_result_t result;
    wiced_mac_t mac;

    WPRINT_APP_INFO((">> Starting WAC process...\r\n"));
#ifdef USE_AUDIO_DISPLAY
    audio_display_header_update_options(BATTERY_ICON_IS_VISIBLE | BATTERY_ICON_SHOW_PERCENT);
    audio_display_update_footer("WAC Mode", "", 0, 0, FOOTER_IS_VISIBLE | FOOTER_CENTER_ALIGN | FOOTER_HIDE_SONG_DURATION);
#endif /* USE_AUDIO_DISPLAY */

    result = wiced_wifi_get_mac_address( &mac );
    mac_address_to_string( mac.octet, apple_wac_info.device_random_id );

    apple_wac_info.name = dct_app->speaker_name;
    apple_wac_info.mdns_desired_hostname = dct_app->speaker_name;
    apple_wac_info.mdns_nice_name = dct_app->speaker_name;
    result = apple_wac_configure( &apple_wac_info );

    if ( result != WICED_SUCCESS )
    {
#ifdef USE_AUDIO_DISPLAY
        audio_display_footer_update_song_info("WAC Mode", "WAC Mode Failure");
#endif /* USE_AUDIO_DISPLAY */

        WPRINT_APP_INFO((">> Starting apple_wac_configure process failed...\r\n"));
        if ( apple_wac_info.out_new_name != NULL )
        {
            free(apple_wac_info.out_new_name);
            apple_wac_info.out_new_name = NULL;
        }
        if ( apple_wac_info.out_play_password != NULL )
        {
            free(apple_wac_info.out_play_password);
            apple_wac_info.out_play_password = NULL;
        }
        if ( result == WICED_WWD_TIMEOUT )
        {
            PRINT_ERR("WAC process timed out !\r\n");
            goto _time_out;
        }
        PRINT_ERR("WAC process FAILED ! Rebooting...\r\n");
    }
    else
    {
#ifdef USE_AUDIO_DISPLAY
        audio_display_footer_update_song_info("WAC Mode", "WAC Mode Success");
#endif /* USE_AUDIO_DISPLAY */

        WPRINT_APP_INFO((">> Starting apple_wac_configure passed...\r\n"));
        /* get new name and new play password */
        if ( ( apple_wac_info.out_new_name != NULL ) &&  ( apple_wac_info.out_new_name[0] != '\0' ) )
        {
            snprintf( dct_app->speaker_name, sizeof(dct_app->speaker_name), "%s", apple_wac_info.out_new_name );
            free(apple_wac_info.out_new_name);
            apple_wac_info.out_new_name = NULL;
            WPRINT_APP_INFO(("New speaker name: [%s]\r\n", dct_app->speaker_name ));
        }
        if ( apple_wac_info.out_play_password != NULL )
        {
            /* we got a new password */
            snprintf( dct_app->play_password, sizeof(dct_app->play_password), "%s", apple_wac_info.out_play_password );
            free(apple_wac_info.out_play_password);
            apple_wac_info.out_play_password = NULL;
            WPRINT_APP_INFO(("Play password: [%s]\r\n", dct_app->play_password ));
        }
        else
        {
            /* we don't have a new password: clear the old one */
            memset( dct_app->play_password, 0, sizeof(dct_app->play_password));
        }
        result = wiced_dct_write( dct_app, DCT_APP_SECTION, app_dct_get_offset(APP_DCT_AIRPLAY), sizeof(*dct_app) );
        if ( result != WICED_SUCCESS )
        {
            PRINT_ERR("Writing DCT app section failed !\r\n");
        }

        WPRINT_APP_INFO(("WAC process was a success. Rebooting...\r\n"));
    }

    wiced_airplay_ctx_deinit();
    wiced_app_system_reboot();

_time_out:
    return result;
}
#endif /* USE_APPLE_WAC */

void host_dacp_connection_state_changes ( airplay_dacp_connection_state_t new_state )
{
    airplay_dacp_state = new_state;
    if ( new_state == DACP_CONNECTED )
    {
        WPRINT_APP_INFO( ("[Airplay] DACP service is connected. \r\n") );
    }
    else if ( new_state == DACP_DISCONNECTED )
    {
        WPRINT_APP_INFO( ("[Airplay] DACP service is DISCONNECTED !\r\n") );
    }
}

static void wiced_app_airplay_prevent_playback( wiced_app_service_t *service )
{
    WPRINT_APP_INFO( ("[Airplay] Prevent Service \r\n") );
    if( service == NULL || service->type != SERVICE_AIRPLAY )
    {
        WPRINT_APP_ERROR(("[Airplay] Invalid argument\n"))
        return;
    }

    if( service->state == SERVICE_PLAYING_AUDIO )
    {
        service->state = SERVICE_PREEMPTED;
    }
    else
    {
        service->state = SERVICE_PREVENTED;
    }

    /* Call library specific prevent* routine */
    wiced_airplay_prevent_playback();
}

static void wiced_app_airplay_allow_playback( wiced_app_service_t *service )
{
    WPRINT_APP_INFO( ("[Airplay] Allow Service \r\n") );
    if( service == NULL || service->type != SERVICE_AIRPLAY )
    {
        WPRINT_APP_ERROR(("[Airplay] Invalid argument\n"))
        return;
    }

    service->state= SERVICE_IDLE;

    /* Call library specific allow* routine */
    wiced_airplay_allow_playback();

}

static wiced_result_t set_unique_device_name( char *out_unique_device_name, uint8_t device_name_size )
{
    wiced_result_t  result;
    wiced_mac_t mac;
    int rc_snprintf;

    result = wiced_wifi_get_mac_address( &mac );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("Failed to get MAC address\r\n" );
        goto _exit;
    }
    WPRINT_LIB_INFO(("Setting unique name\n"));
    memset( out_unique_device_name, 0, device_name_size );
    rc_snprintf = snprintf( out_unique_device_name, device_name_size, "%s_%02x%02x%02x",
                            MY_DEVICE_NAME, mac.octet[3], mac.octet[4], mac.octet[5] );
    if ( rc_snprintf < ( strlen(MY_DEVICE_NAME) + strlen("_XXXXXX") ) )
    {
        result = WICED_ERROR;
        PRINT_ERR("snprintf failed\r\n");
        goto _exit;
    }

_exit:
    return result;
}

static wiced_result_t wiced_airplay_read_dct_config(void)
{
    wiced_result_t result = WICED_ERROR;
    /* Find out if device is configured with WiFi credentials */
    result = wiced_dct_read_lock( (void**) &dct_wifi, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0,
                                  sizeof(platform_dct_wifi_config_t) );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("Can't get device configuration status !\r\n");
        return WICED_ERROR;
    }
    /* Read speaker name string from app DCT */
    result = wiced_dct_read_lock( (void**) &dct_app, WICED_TRUE, DCT_APP_SECTION, app_dct_get_offset(APP_DCT_AIRPLAY), sizeof(airplay_dct_t) );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("Can't get app data !\r\n");
        return WICED_ERROR;
    }
    /* Generate a unique speaker name if string from app DCT is empty */
    if ( dct_app->speaker_name[0] == '\0' )
    {
        result = set_unique_device_name(dct_app->speaker_name, sizeof(dct_app->speaker_name));
        if ( result != WICED_SUCCESS )
        {
            PRINT_ERR("Can't set unique device name !\r\n");
            return WICED_ERROR;
        }
    }
    return result;
}

/*******************************************************************************
 **
 ** Function         wiced_add_airplay_audio_service
 **
 ** Description      This function adds the bluetooth service to the service table.
 **
 ** Returns          void
 *******************************************************************************/

void wiced_add_airplay_audio_service(void)
{
    wiced_result_t result = WICED_ERROR;
    wiced_app_service_t cell;

    cell.priority                   = SERVICE_AIRPLAY_PRIORITY;
    cell.type                       = SERVICE_AIRPLAY;
    cell.state                      = SERVICE_DISABLED;


    cell.init_service               = wiced_airplay_init_service;
    cell.deinit_service             = NULL;
    cell.prevent_service            = wiced_app_airplay_prevent_playback;
    cell.allow_service              = wiced_app_airplay_allow_playback;
    cell.button_handler             = wiced_airplay_button_event_handler;
    result = wiced_add_entry(&cell);

    if (result != WICED_SUCCESS)
    {
        WPRINT_APP_ERROR(("Failed to add Service entry [Error:%d]\n", result));
        /* TODO. May be we wish to send the result to the app_worker_thread as well */
        return;
    }

    cell.init_service();
}

static void wiced_airplay_init_service(void)
{
    wiced_result_t result;
    /* Right Now, not interested in monitoring properties,so keep it zero */
    uint16_t monitored_properties;

    WPRINT_APP_INFO( ("[Airplay] entered init service \r\n") );
    monitored_properties = ( 1 << PLAYBACK_PROGRESS ) | ( 1 << PLAYBACK_METADATA );

    wiced_airplay_ctx_init();

    result = wiced_airplay_read_dct_config();
    if(result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("[Airplay] Error reading DCT information\n" ));
        goto _dct_error;
    }
    /* Set hostname for DHCP client */
    result = wiced_network_set_hostname( dct_app->speaker_name );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("[Airplay] Setting Host-name failed !\r\n");
    }

    /* Start WAC process if device is UNconfigured */
    AIRPLAY_START_WAC_IF_WIFI_DEVICE_IS_UNCONFIGURED();

    result = wiced_network_up_default( &airplay_ctx.airplay_interface, &ap_ip_settings );
    if( result != WICED_SUCCESS )
    {
        PRINT_ERR("[Airplay] Bringing up network interface failed !\r\n");
        goto _error_network_up;
    }

    if ( dct_app->play_password[0] != '\0' )
    {
        WPRINT_APP_INFO(("with speaker name [%s] and play password [%s].\r\n", dct_app->speaker_name, dct_app->play_password));
    }
    else
    {
        WPRINT_APP_INFO(("with speaker name [%s] and no play password.\r\n", dct_app->speaker_name));
    }

#ifdef USE_AUDIO_DISPLAY
        audio_display_header_update_options(BATTERY_ICON_IS_VISIBLE | BATTERY_ICON_SHOW_PERCENT | SIGNAL_STRENGTH_IS_VISIBLE);
        if( (dct_wifi != NULL))
        {
            memcpy(airplay_ctx.ssid_str, dct_wifi->stored_ap_list->details.SSID.value, dct_wifi->stored_ap_list->details.SSID.length);
            airplay_ctx.ssid_str[dct_wifi->stored_ap_list->details.SSID.length] = '\0';
            audio_display_update_footer("Airplay Mode", airplay_ctx.ssid_str, 0, 0, FOOTER_IS_VISIBLE | FOOTER_CENTER_ALIGN | FOOTER_HIDE_SONG_DURATION);
        }
#endif /* USE_AUDIO_DISPLAY */

    result = wiced_airplay_start_server( airplay_ctx.airplay_interface, dct_app->speaker_name, MY_DEVICE_MODEL,
                                             dct_app->play_password[0] ? dct_app->play_password : NULL,
                                             airplay_state_change_callback, app_update_playback_property_callback, monitored_properties,
                                             PLATFORM_DEFAULT_AUDIO_OUTPUT );
    if( result != WICED_SUCCESS )
    {
#ifdef USE_AUDIO_DISPLAY
            audio_display_header_update_options(BATTERY_ICON_IS_VISIBLE | BATTERY_ICON_SHOW_PERCENT);
            audio_display_footer_update_options(0x00);
#endif /* USE_AUDIO_DISPLAY */
        WPRINT_APP_INFO( ("[Airplay] Airplay server starting failed ! \r\n") );
        goto _error_airplay_start;
    }

    airplay_ctx.is_airplay_server_up = WICED_TRUE;
    WPRINT_APP_INFO( ("[Airplay] Started...\r\n") );
    return;

_error_airplay_start:
    airplay_ctx.is_airplay_server_up = WICED_FALSE;
_error_network_up:
_dct_error:
    wiced_airplay_ctx_deinit();
    return;
}

static wiced_result_t airplay_state_change_callback ( airplay_server_state_t state )
{
    wiced_result_t result = WICED_SUCCESS;
    static wiced_bool_t is_idle_done = WICED_FALSE;
    switch(state)
    {
        case AIRPLAY_SERVER_IDLE:
        {

            if(is_idle_done == WICED_TRUE)
            {
                return WICED_SUCCESS;
            }
            app_reset_current_service(SERVICE_AIRPLAY);
            is_idle_done = WICED_TRUE;
            break;
        }

        case AIRPLAY_SERVER_CONNECTED:
        {
            WPRINT_APP_INFO( ("[Airplay] Server connected\r\n"));
            app_set_current_service(SERVICE_AIRPLAY);

#ifdef USE_AUDIO_DISPLAY
            if( (dct_wifi != NULL) )
            {
                audio_display_update_footer("Airplay Mode", airplay_ctx.ssid_str, 0, 0, FOOTER_IS_VISIBLE | FOOTER_CENTER_ALIGN | FOOTER_HIDE_SONG_DURATION);
                audio_display_header_update_options(BATTERY_ICON_IS_VISIBLE | BATTERY_ICON_SHOW_PERCENT | SIGNAL_STRENGTH_IS_VISIBLE);
                audio_display_footer_update_options(FOOTER_IS_VISIBLE | FOOTER_CENTER_ALIGN);
            }
#endif

            /* Delay a bit for other libraries to cope with the "prevent" */
            wiced_rtos_delay_milliseconds( 200 );
            is_idle_done = WICED_FALSE;
            break;
        }

        case AIRPLAY_SERVER_STREAMING:
        {
            WPRINT_APP_INFO( ("[Airplay] Server is streaming \r\n"));
            app_set_service_state(SERVICE_AIRPLAY, SERVICE_PLAYING_AUDIO);
            break;
        }

        case AIRPLAY_SERVER_PAUSED:
        {
            /* TODO: Not sure about the PAUSED use-case right now. */
            WPRINT_APP_INFO( ("[Airplay] Server is paused \r\n") );
            app_set_service_state(SERVICE_AIRPLAY, SERVICE_PAUSED_AUDIO);
            break;
        }

        default:
            WPRINT_APP_INFO( ("[Airplay] Invalid State %d\n",state) );
            return WICED_ERROR;
    }

    return result;
}

/* Keeping it for completion of the airplay server initialization. */
static wiced_result_t app_update_playback_property_callback( void* property_value, airplay_playback_property_t property_type )
{
    WPRINT_APP_DEBUG(("[app_update_playback_property_callback] entered\n"));
    switch( property_type )
    {

        case PLAYBACK_PROGRESS:
        {
            airplay_playback_progress_t* progress_ptr = (airplay_playback_progress_t*)property_value;
#ifdef USE_AUDIO_DISPLAY
            audio_display_footer_update_time_info(progress_ptr->elapsed_time, progress_ptr->duration);
#else /* !USE_AUDIO_DISPLAY */
            WPRINT_APP_INFO(("[Airplay] Current time = %02lu:%02lu, Song duration = %02lu:%02lu \r\n", progress_ptr->elapsed_time / 60,
                            progress_ptr->elapsed_time % 60 , progress_ptr->duration / 60, progress_ptr->duration % 60 ) );
#endif /* USE_AUDIO_DISPLAY */
        }
        break;

        case PLAYBACK_METADATA:
        {
            airplay_playback_metadata_t* metadata_ptr = (airplay_playback_metadata_t*)property_value;
#ifdef USE_AUDIO_DISPLAY
            audio_display_footer_update_song_info(metadata_ptr->song_name, metadata_ptr->artist_name);
            audio_display_footer_update_options(FOOTER_IS_VISIBLE | FOOTER_CENTER_ALIGN);
#else /* !USE_AUDIO_DISPLAY */
            WPRINT_APP_INFO(("[Airplay] %s - %s - %s \r\n", metadata_ptr->song_name, metadata_ptr->artist_name, metadata_ptr->album_name ));
#endif /* USE_AUDIO_DISPLAY */
        }
        break;

        default:
            WPRINT_APP_ERROR( ("[Airplay] Unhandled property type\n") );
            break;
    }
    return WICED_SUCCESS;
}

static wiced_result_t wiced_airplay_button_event_handler(app_service_action_t action)
{
    wiced_result_t ret = WICED_ERROR;
    char *command_str = NULL;
    switch( action )
    {
        case ACTION_PAUSE_PLAY:
            command_str = "playpause";
            break;
        case ACTION_FORWARD:
            command_str = "nextitem";
            break;
        case ACTION_BACKWARD:
            command_str = "previtem";
            break;
        case ACTION_VOLUME_UP:
            command_str = "volumeup";
            break;
        case ACTION_VOLUME_DOWN:
            command_str = "volumedown";
            break;
        case ACTION_STOP:
        case ACTION_FACTORY_RESET:
        case ACTION_MULTI_FUNCTION_SHORT_RELEASE:
        case ACTION_MULTI_FUNCTION_LONG_RELEASE:
        case NO_ACTION:
        default:
            break;
    }

    if(!command_str)
    {
        return ret;
    }

    ret = wiced_airplay_dacp_schedule_command(command_str);
    if(ret != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("[Airplay] %s DACP command failed\n", command_str));
    }
    return ret;
}

void wiced_airplay_cleanup(void)
{
    if( airplay_ctx.is_airplay_server_up == WICED_TRUE )
    {
        wiced_airplay_stop_server(airplay_ctx.airplay_interface);
    }
    return;
}

#if defined (USE_APPLE_WAC)
void wiced_airplay_do_wac_on_reboot(void)
{
    wiced_result_t result;
    dct_wifi->device_configured = WICED_FALSE;
    result = wiced_dct_write( dct_wifi, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("Writing DCT app section failed !\r\n");
    }
    else
    {
        WPRINT_APP_INFO(("[Airplay] Device status was set to UNconfigured !\r\n"));
    }

    wiced_airplay_ctx_deinit();
}
#endif
