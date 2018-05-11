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

/** @file
 *
 * Airplay Audio Application
 *
 * This application demonstrates that Airplay can be streamed from an iOS device capable of the Airplay streaming.
 *
 * Features Supported:
 *   - Playback of airplay audio
 *   - PLAY/PAUSE/FWD/BACKWARD etc. from the DUT.
 *   - Display the song details on the screen in the case of boards which support display (e.g. 43907WAE_1)
 *
 * This application needs WiFi to connect to the access point, and hardware that supports I2S and a speaker.
 *
 * The network to be used can be changed by the #define WICED_NETWORK_INTERFACE in wifi_config_dct.h
 * In the case of using AP or STA mode, change the AP_SSID and AP_PASSPHRASE accordingly.
 *
 * To enable display, ensure that
 * GLOBAL_DEFINES     += USE_AUDIO_DISPLAY
 * is uncommented in airplay_audio.mk
 *
 */

#include "wiced.h"
#include "wiced_airplay.h"
#include "platform_audio.h"

#include "airplay_audio_dct.h"

#if defined(USE_DACP_CONSOLE)
#include "command_console.h"
#if defined(USE_WIFI_CONSOLE_COMMANDS)
#include "command_console_wifi.h"
#endif
#endif

#if defined(USE_APPLE_WAC)
#include "apple_wac.h"
#include "button_manager.h"
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

#if defined (USE_DACP_CONSOLE)

#define AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(command_name) \
    int dacp_command_handler_ ##command_name ( int argc, char *argv[] );


#define AIRPLAY_ADD_DACP_CONSOLE_COMMAND(command_name) \
    DEFAULT_0_ARGUMENT_COMMAND_ENTRY(command_name, dacp_command_handler_ ##command_name )


#define AIRPLAY_DACP_COMMAND_HANDLER(command_name) \
    int dacp_command_handler_ ##command_name ( int argc, char *argv[] ) \
    { \
        int rc = ERR_UNKNOWN; \
        wiced_result_t result; \
        wiced_jump_when_not_true( ( app_ctx.dacp_is_connected == WICED_TRUE ), _exit ); \
        result = wiced_airplay_dacp_schedule_command(#command_name ); \
        wiced_assert("Can't schedule DACP command", result == WICED_SUCCESS); \
        wiced_jump_when_not_true( ( result == WICED_SUCCESS ), _exit ); \
        rc = ERR_CMD_OK; \
     _exit: \
        return rc; \
    }

#define AIRPLAY_DACP_CONSOLE_START() \
    do \
    { \
        wiced_result_t result; \
        REFERENCE_DEBUG_ONLY_VARIABLE(result); \
        result = command_console_init( STDIO_UART, sizeof(airplay_command_buffer), airplay_command_buffer, AIRPLAY_CONSOLE_COMMAND_HISTORY_LENGTH, airplay_command_history_buffer, " " ); \
        wiced_assert("Can't START command console !", result == WICED_SUCCESS); \
        console_add_cmd_table( &dacp_command_table[0] ); \
    } while(0)

#define AIRPLAY_DACP_CONSOLE_STOP() \
    do \
    { \
        wiced_result_t result; \
        REFERENCE_DEBUG_ONLY_VARIABLE(result); \
        result = command_console_deinit(); \
        wiced_assert("Can't STOP command console !", result == WICED_SUCCESS); \
    } while(0)

#else

#define AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(command_name)

#define AIRPLAY_DACP_COMMAND_HANDLER(command_name)

#define AIRPLAY_DACP_CONSOLE_START() \
    do \
    { \
    } while(0)

#define AIRPLAY_DACP_CONSOLE_STOP() \
    do \
    { \
    } while(0)

#endif /* USE_DACP_CONSOLE */

#if defined(USE_APPLE_WAC)

#define kHexDigitsUppercase         "0123456789ABCDEF"

#define AIRPLAY_SET_WIFI_DEVICE_UNCONFIGURED_AND_REBOOT() \
    do \
    { \
        do_wac_on_next_reboot(); \
    } while(0)

#define AIRPLAY_START_WAC_IF_WIFI_DEVICE_IS_UNCONFIGURED() \
    do \
    { \
        if ( app_ctx.dct_wifi->device_configured == WICED_FALSE ) \
        { \
            do_wac(); \
        } \
    } while(0)

#define AIRPLAY_BREAK_OUT_OF_LOOP() \
    break

#else

#define AIRPLAY_SET_WIFI_DEVICE_UNCONFIGURED_AND_REBOOT() \
    do \
    { \
    } while(0)

#define AIRPLAY_START_WAC_IF_WIFI_DEVICE_IS_UNCONFIGURED() \
    do \
    { \
    } while(0)

#define AIRPLAY_BREAK_OUT_OF_LOOP() \
    do \
    { \
    } while(0)

#endif /* USE_APPLE_WAC */

/******************************************************
 *                    Constants
 ******************************************************/

#define MY_DEVICE_NAME                           "wiced_airplay_rocks"
#define MY_DEVICE_MODEL                          "1.0"
#define MY_DEVICE_MANUFACTURER                   "Cypress"
#define MAX_AIRPLAY_COMMAND_LENGTH               (20)
#define AIRPLAY_CONSOLE_COMMAND_HISTORY_LENGTH   (10)

#define FIRMWARE_VERSION                         "wiced_1_0"

#define AIRPLAY_PROGRESS_WATCH /* If defined enables printing of the current song progress during the airplay session */
#define AIRPLAY_TEXT_METADATA  /* If defined enables printing metadata of the current song during the airplay session */
#define AIRPLAY_PROGRESS_WATCH_INTERVAL_SECS     (5)

#ifndef WICED_AIRPLAY_RESTART_MILLISECOND_VALUE
#define WICED_AIRPLAY_RESTART_MILLISECOND_VALUE  WICED_WAIT_FOREVER
#endif

#define WICED_AIRPLAY_CONFIG_AP_CHANNEL          (6)

#define WICED_AIRPLAY_REBOOT_DELAY_MSECS         (500)

#define BUTTON_WORKER_STACK_SIZE                 ( 1024 )
#define BUTTON_WORKER_QUEUE_SIZE                 ( 4 )


/******************************************************
 *                   Enumerations
 ******************************************************/

#if defined(USE_APPLE_WAC)
typedef enum
{
    /* Launch Apple Wireless Accessory Configuration process */
    ACTION_AIRPLAY_WAC_LAUNCH
} app_action_t;

typedef enum
{
    AIRPLAY_WAC_BUTTON
} application_button_t;

#endif /* USE_APPLE_WAC */

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_interface_t airplay_interface;
    airplay_dct_t *dct_app;
    platform_dct_wifi_config_t* dct_wifi;
    wiced_semaphore_t button_wac_semaphore;
    airplay_server_state_t current_airplay_server_state;
#if defined(USE_DACP_CONSOLE)
    wiced_bool_t dacp_is_connected;
#endif /* USE_DACP_CONSOLE */
#if defined(USE_APPLE_WAC)
    wiced_bool_t button_action_start_wac;
#endif /* USE_APPLE_WAC */
#if defined(USE_AUDIO_DISPLAY)
    wiced_thread_t display_thread;
    char ssid_str[SSID_NAME_SIZE + 1];
#endif /* USE_AUDIO_DISPLAY */
} airplay_audio_context_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(beginff)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(beginrew)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(getproperty)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(mutetoggle)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(nextchapter)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(nextcontaine)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(nextgroup)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(nextitem)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(pause)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(play)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(playpause)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(playresume)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(playspec)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(prevchapter)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(prevcontainer)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(prevgroup)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(previtem)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(repeatadv)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(restartitem)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(shufflesongs)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(shuffletoggle)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(stop)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(volumedown)
AIRPLAY_DACP_COMMAND_HANDLER_DECLARATION(volumeup)

static wiced_result_t airplay_state_change_callback ( airplay_server_state_t state );
static wiced_result_t app_update_playback_property_callback( void* property_value, airplay_playback_property_t property_type );
static wiced_result_t do_airplay_audio(void);

#if defined(USE_APPLE_WAC)
static void app_button_event_handler( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state );
#endif

/******************************************************
 *               Variables Definitions
 ******************************************************/

static airplay_audio_context_t app_ctx;

#if defined(USE_DACP_CONSOLE)
static char airplay_command_buffer[MAX_AIRPLAY_COMMAND_LENGTH];
static char airplay_command_history_buffer[ MAX_AIRPLAY_COMMAND_LENGTH * AIRPLAY_CONSOLE_COMMAND_HISTORY_LENGTH ];

static const command_t dacp_command_table[] =
{
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(beginff)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(beginrew)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(getproperty)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(mutetoggle)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(nextchapter)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(nextcontaine)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(nextgroup)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(nextitem)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(pause)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(play)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(playpause)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(playresume)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(playspec)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(prevchapter)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(prevcontainer)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(prevgroup)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(previtem)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(repeatadv)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(restartitem)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(shufflesongs)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(shuffletoggle)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(stop)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(volumedown)
    AIRPLAY_ADD_DACP_CONSOLE_COMMAND(volumeup)
#if defined(USE_WIFI_CONSOLE_COMMANDS)
    WIFI_COMMANDS
#endif
    COMMAND_TABLE_ENDING()
};
#endif

static const wiced_ip_setting_t ap_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};

static const char* firmware_version = FIRMWARE_VERSION;

#if defined(USE_APPLE_WAC)

static const wiced_button_manager_configuration_t button_manager_configuration =
{
    .short_hold_duration     = 500  * MILLISECONDS,
    .debounce_duration       = 150  * MILLISECONDS,

    .event_handler           = app_button_event_handler,
};

/* Static button configuration */
static const wiced_button_configuration_t button_configurations[] =
{
#if (WICED_PLATFORM_BUTTON_COUNT > 0)
    [ AIRPLAY_WAC_BUTTON ]     = { PLATFORM_BUTTON_1, BUTTON_CLICK_EVENT , ACTION_AIRPLAY_WAC_LAUNCH },
#endif
};

/* Button objects for the button manager */
button_manager_button_t buttons[] =
{
#if (WICED_PLATFORM_BUTTON_COUNT > 0)
    [ AIRPLAY_WAC_BUTTON ]             = { &button_configurations[ AIRPLAY_WAC_BUTTON ]        },
#endif
};

static button_manager_t button_manager;
wiced_worker_thread_t   button_worker_thread;

static const uint8_t oui_array[] = {0x00, 0xA0, 0x40};

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
        .oui                    = (uint8_t *)oui_array,
        .mfi_protocols          = NULL,
        .num_mfi_protocols      = 0,
        .bundle_seed_id         = NULL,
        .soft_ap_channel        = 0,
        .out_new_name           = NULL,
        .out_play_password      = NULL,
        .device_random_id       = "XX:XX:XX:XX:XX:XX",
};
#endif /* USE_APPLE_WAC */

#if defined(USE_WEBSERVER_CONFIG)
static const configuration_entry_t const app_config[] =
{
    {"Airplay Speaker Name", DCT_OFFSET(airplay_dct_t, speaker_name),  AIRPLAY_SPEAKER_NAME_STRING_LENGTH  + 1, CONFIG_STRING_DATA },
    {"Airplay Password",     DCT_OFFSET(airplay_dct_t, play_password), AIRPLAY_PLAY_PASSWORD_STRING_LENGTH + 1, CONFIG_STRING_DATA },
    {0,0,0,0}
};
#endif /* USE_WEBSERVER_CONFIG */

/******************************************************
 *               Function Definitions
 ******************************************************/

AIRPLAY_DACP_COMMAND_HANDLER(beginff)
AIRPLAY_DACP_COMMAND_HANDLER(beginrew)
AIRPLAY_DACP_COMMAND_HANDLER(getproperty)
AIRPLAY_DACP_COMMAND_HANDLER(mutetoggle)
AIRPLAY_DACP_COMMAND_HANDLER(nextchapter)
AIRPLAY_DACP_COMMAND_HANDLER(nextcontaine)
AIRPLAY_DACP_COMMAND_HANDLER(nextgroup)
AIRPLAY_DACP_COMMAND_HANDLER(nextitem)
AIRPLAY_DACP_COMMAND_HANDLER(pause)
AIRPLAY_DACP_COMMAND_HANDLER(play)
AIRPLAY_DACP_COMMAND_HANDLER(playpause)
AIRPLAY_DACP_COMMAND_HANDLER(playresume)
AIRPLAY_DACP_COMMAND_HANDLER(playspec)
AIRPLAY_DACP_COMMAND_HANDLER(prevchapter)
AIRPLAY_DACP_COMMAND_HANDLER(prevcontainer)
AIRPLAY_DACP_COMMAND_HANDLER(prevgroup)
AIRPLAY_DACP_COMMAND_HANDLER(previtem)
AIRPLAY_DACP_COMMAND_HANDLER(repeatadv)
AIRPLAY_DACP_COMMAND_HANDLER(restartitem)
AIRPLAY_DACP_COMMAND_HANDLER(shufflesongs)
AIRPLAY_DACP_COMMAND_HANDLER(shuffletoggle)
AIRPLAY_DACP_COMMAND_HANDLER(stop)
AIRPLAY_DACP_COMMAND_HANDLER(volumedown)
AIRPLAY_DACP_COMMAND_HANDLER(volumeup)

void host_get_firmware_version(const char** firmware_string)
{
    *firmware_string = firmware_version;
}


void host_dacp_connection_state_changes( airplay_dacp_connection_state_t state )
{
#if defined(USE_DACP_CONSOLE)
    if( state == DACP_CONNECTED )
    {
        app_ctx.dacp_is_connected = WICED_TRUE;
        WPRINT_APP_INFO(("DACP Console is now ENABLED. To see DACP in action type one of the airplay commands and press enter \r\n"));
    } else if ( state == DACP_DISCONNECTED )
    {
        app_ctx.dacp_is_connected = WICED_FALSE;
        WPRINT_APP_INFO(("DACP Console is DISABLED.\r\n"));
    }
#else
    UNUSED_PARAMETER(state);
#endif
}


wiced_result_t app_update_playback_property_callback( void* property_value, airplay_playback_property_t property_type )
{
    switch( property_type )
    {

        case PLAYBACK_PROGRESS:
        {
#ifdef USE_AUDIO_DISPLAY
            airplay_playback_progress_t* progress_ptr = (airplay_playback_progress_t*)property_value;
            audio_display_footer_update_time_info(progress_ptr->elapsed_time, progress_ptr->duration);
#else /* !USE_AUDIO_DISPLAY */
#ifdef AIRPLAY_PROGRESS_WATCH
            airplay_playback_progress_t* progress_ptr = (airplay_playback_progress_t*)property_value;
            if ( !(progress_ptr->elapsed_time%AIRPLAY_PROGRESS_WATCH_INTERVAL_SECS) )
            {
                WPRINT_APP_INFO(("Current time = %02lu:%02lu, Song duration = %02lu:%02lu \r\n", progress_ptr->elapsed_time / 60,
                                 progress_ptr->elapsed_time % 60 , progress_ptr->duration / 60, progress_ptr->duration % 60 ) );
            }
#endif /* AIRPLAY_PROGRESS_WATCH */
#endif /* USE_AUDIO_DISPLAY */
        }
        break;

        case PLAYBACK_METADATA:
        {
#ifdef USE_AUDIO_DISPLAY
            airplay_playback_metadata_t* metadata_ptr = (airplay_playback_metadata_t*) property_value;
            audio_display_footer_update_song_info(metadata_ptr->song_name, metadata_ptr->artist_name);
            audio_display_footer_update_options(FOOTER_IS_VISIBLE | FOOTER_CENTER_ALIGN);
#else /* !USE_AUDIO_DISPLAY */
#ifdef AIRPLAY_TEXT_METADATA
            airplay_playback_metadata_t* metadata_ptr = (airplay_playback_metadata_t*)property_value;
            WPRINT_APP_INFO(("%s - %s - %s \r\n", metadata_ptr->song_name, metadata_ptr->artist_name, metadata_ptr->album_name ));
#endif /* AIRPLAY_TEXT_METADATA */
#endif /* USE_AUDIO_DISPLAY */
        }
        break;

        default:
            break;

    }
    return WICED_SUCCESS;
}


wiced_result_t airplay_state_change_callback ( airplay_server_state_t state )
{
    switch(state)
    {
        case AIRPLAY_SERVER_IDLE:
            WPRINT_APP_INFO(("Airplay server idle \r\n"));
            app_ctx.current_airplay_server_state = AIRPLAY_SERVER_IDLE;

#ifdef USE_AUDIO_DISPLAY
            audio_display_update_footer("Airplay Mode", app_ctx.ssid_str, 0, 0, FOOTER_IS_VISIBLE | FOOTER_CENTER_ALIGN | FOOTER_HIDE_SONG_DURATION);
#endif /* USE_AUDIO_DISPLAY */

            break;

        case AIRPLAY_SERVER_CONNECTED:
            WPRINT_APP_INFO(("Airplay server connected \r\n"));
            app_ctx.current_airplay_server_state = AIRPLAY_SERVER_CONNECTED;
            break;

        case AIRPLAY_SERVER_STREAMING:
            WPRINT_APP_INFO(("Airplay server is streaming \r\n"));
            app_ctx.current_airplay_server_state = AIRPLAY_SERVER_STREAMING;
            break;

        case AIRPLAY_SERVER_PAUSED:
            WPRINT_APP_INFO(("Airplay server is paused \r\n"));
            app_ctx.current_airplay_server_state = AIRPLAY_SERVER_PAUSED;
            break;

    }

    return WICED_SUCCESS;
}

wiced_result_t prevent_allow_playback( void* arg )
{
    static wiced_bool_t prevented = WICED_FALSE;
    UNUSED_PARAMETER(arg);
    if( prevented == WICED_FALSE )
    {
        WPRINT_APP_INFO(("Prevent playback \r\n"));
        wiced_airplay_prevent_playback();
        prevented = WICED_TRUE;
    }
    else
    {
        WPRINT_APP_INFO(("Allow playback \r\n"));
        wiced_airplay_allow_playback();
        prevented = WICED_FALSE;
    }

    return WICED_SUCCESS;
}

static wiced_result_t do_airplay_audio(void)
{
    wiced_result_t result;
    uint16_t       monitored_properties;

    monitored_properties = 0;

#ifdef AIRPLAY_PROGRESS_WATCH
    monitored_properties |= ( 1 << PLAYBACK_PROGRESS );
#endif /* AIRPLAY_PROGRESS_WATCH */

#ifdef AIRPLAY_TEXT_METADATA
    monitored_properties |= ( 1 << PLAYBACK_METADATA );
#endif /* AIRPLAY_TEXT_METADATA */

    for( ;; )
    {
        AIRPLAY_DACP_CONSOLE_START();

#ifdef USE_AUDIO_DISPLAY
        audio_display_header_update_options(BATTERY_ICON_IS_VISIBLE | BATTERY_ICON_SHOW_PERCENT | SIGNAL_STRENGTH_IS_VISIBLE);
        if ( app_ctx.dct_wifi != NULL )
        {
            memcpy(app_ctx.ssid_str, app_ctx.dct_wifi->stored_ap_list->details.SSID.value, app_ctx.dct_wifi->stored_ap_list->details.SSID.length);
            app_ctx.ssid_str[app_ctx.dct_wifi->stored_ap_list->details.SSID.length] = '\0';
            audio_display_update_footer("Airplay Mode", app_ctx.ssid_str, 0, 0, FOOTER_IS_VISIBLE | FOOTER_CENTER_ALIGN | FOOTER_HIDE_SONG_DURATION);
        }
#endif /* USE_AUDIO_DISPLAY */

        WPRINT_APP_INFO((">> Starting airplay server...\r\n"));
        if ( app_ctx.dct_app->play_password[0] != '\0' )
        {
            WPRINT_APP_INFO(("with speaker name [%s] and play password [%s].\r\n",
                             app_ctx.dct_app->speaker_name,
                             app_ctx.dct_app->play_password));
        }
        else
        {
            WPRINT_APP_INFO(("with speaker name [%s] and no play password.\r\n",
                             app_ctx.dct_app->speaker_name));
        }
        result = wiced_airplay_start_server( app_ctx.airplay_interface, app_ctx.dct_app->speaker_name, MY_DEVICE_MODEL,
                                             app_ctx.dct_app->play_password[0] ? app_ctx.dct_app->play_password : NULL,
                                             airplay_state_change_callback, app_update_playback_property_callback, monitored_properties,
                                             PLATFORM_DEFAULT_AUDIO_OUTPUT );
        if ( result != WICED_SUCCESS )
        {
            PRINT_ERR("Airplay server failed !\r\n");
#ifdef USE_AUDIO_DISPLAY
            audio_display_header_update_options(BATTERY_ICON_IS_VISIBLE | BATTERY_ICON_SHOW_PERCENT);
            audio_display_footer_update_options(0x00);
#endif /* USE_AUDIO_DISPLAY */
            break;
        }
        else
        {
            WPRINT_APP_INFO(("Started\r\n"));
        }

        /* Wait until timeout. */
        wiced_rtos_get_semaphore(&app_ctx.button_wac_semaphore, WICED_AIRPLAY_RESTART_MILLISECOND_VALUE);

        /* Perform a manual stop. */
        WPRINT_APP_INFO((">> Stopping airplay server...\r\n"));
        wiced_airplay_stop_server( app_ctx.airplay_interface );
        WPRINT_APP_INFO(("Stopped \r\n"));

        AIRPLAY_DACP_CONSOLE_STOP();

        AIRPLAY_BREAK_OUT_OF_LOOP();
    }
    return result;
}

#if defined(USE_APPLE_WAC)
static void do_wac_cleanup_and_reboot(void)
{
    wiced_dct_read_unlock( app_ctx.dct_app, WICED_TRUE );
    wiced_dct_read_unlock( app_ctx.dct_wifi, WICED_TRUE );
    app_ctx.dct_app = NULL;
    app_ctx.dct_wifi = NULL;
    wiced_deinit();
    wiced_rtos_delay_milliseconds(WICED_AIRPLAY_REBOOT_DELAY_MSECS);
    wiced_framework_reboot();
}

static void do_wac_on_next_reboot(void)
{
    if ( app_ctx.button_action_start_wac == WICED_TRUE )
    {
        wiced_result_t result;
        app_ctx.dct_wifi->device_configured = WICED_FALSE;
        result = wiced_dct_write( app_ctx.dct_wifi, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
        if ( result != WICED_SUCCESS )
        {
            PRINT_ERR("Writing DCT app section failed !\r\n");
        }
        else
        {
            WPRINT_APP_INFO((">> Device status was set to UNconfigured !\r\n"));
        }
        do_wac_cleanup_and_reboot();
    }
}

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

    apple_wac_info.name = app_ctx.dct_app->speaker_name;
    apple_wac_info.mdns_desired_hostname = app_ctx.dct_app->speaker_name;
    apple_wac_info.mdns_nice_name = app_ctx.dct_app->speaker_name;
    result = apple_wac_configure( &apple_wac_info );

    if ( result != WICED_SUCCESS )
    {
#ifdef USE_AUDIO_DISPLAY
        audio_display_footer_update_song_info("WAC Mode", "WAC Mode Failure");
#endif /* USE_AUDIO_DISPLAY */

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

        /* get new name and new play password */
        if ( ( apple_wac_info.out_new_name != NULL ) &&  ( apple_wac_info.out_new_name[0] != '\0' ) )
        {
            snprintf( app_ctx.dct_app->speaker_name, sizeof(app_ctx.dct_app->speaker_name), "%s", apple_wac_info.out_new_name );
            free(apple_wac_info.out_new_name);
            apple_wac_info.out_new_name = NULL;
            WPRINT_APP_INFO(("New speaker name: [%s]\r\n", app_ctx.dct_app->speaker_name ));
        }
        if ( apple_wac_info.out_play_password != NULL )
        {
            /* we got a new password */
            snprintf( app_ctx.dct_app->play_password, sizeof(app_ctx.dct_app->play_password), "%s", apple_wac_info.out_play_password );
            free(apple_wac_info.out_play_password);
            apple_wac_info.out_play_password = NULL;
            WPRINT_APP_INFO(("Play password: [%s]\r\n", app_ctx.dct_app->play_password ));
        }
        else
        {
            /* we don't have a new password: clear the old one */
            memset( app_ctx.dct_app->play_password, 0, sizeof(app_ctx.dct_app->play_password));
        }
        result = wiced_dct_write( app_ctx.dct_app, DCT_APP_SECTION, 0, sizeof(*app_ctx.dct_app) );
        if ( result != WICED_SUCCESS )
        {
            PRINT_ERR("Writing DCT app section failed !\r\n");
        }
        WPRINT_APP_INFO(("WAC process was a success. Rebooting...\r\n"));
        /* TODO: investigate why Airplay session fails after WAC process */
    }

    do_wac_cleanup_and_reboot();

_time_out:
    return result;
}

/*  Button events thread callback */
static void app_button_event_handler( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state )
{
    /* increase worker thread stack size to at least 4096 bytes before re-enabling the following print statement */
    //WPRINT_APP_DEBUG(("Button Event: %d for button-%d\r\n", (int)button->event, (int)button->button ));

    if ( ( app_ctx.button_action_start_wac != WICED_TRUE ) &&
         ( button->configuration->application_event == ACTION_AIRPLAY_WAC_LAUNCH ) &&
         ( event == BUTTON_CLICK_EVENT ) )
    {
        app_ctx.button_action_start_wac = WICED_TRUE;
        wiced_rtos_set_semaphore(&app_ctx.button_wac_semaphore);
    }

    return;
}
#endif /* USE_APPLE_WAC */

#if defined(USE_WEBSERVER_CONFIG)
static wiced_result_t do_wiced_device_config(void)
{
    wiced_result_t result;
    platform_dct_wifi_config_t* ap_dct;

    result = wiced_dct_read_lock( (void**)&ap_dct, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0,
                                  sizeof(platform_dct_wifi_config_t) );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("DCT read failed !\r\n");
        goto _dct_read_failed;
    }

    // Using device speaker name as SSID
    strncpy( (char *)ap_dct->config_ap_settings.SSID.val, app_ctx.dct_app->speaker_name, sizeof(app_ctx.dct_app->speaker_name) );
    ap_dct->config_ap_settings.SSID.len = strnlen( app_ctx.dct_app->speaker_name, sizeof(app_ctx.dct_app->speaker_name) );
    ap_dct->config_ap_settings.security = WICED_SECURITY_OPEN;
    ap_dct->config_ap_settings.channel = WICED_AIRPLAY_CONFIG_AP_CHANNEL;
    ap_dct->config_ap_settings.security_key_length = 0;
    ap_dct->config_ap_settings.details_valid = CONFIG_VALIDITY_VALUE;

    result = wiced_dct_write( (const void*) ap_dct, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
    wiced_dct_read_unlock( (void*)ap_dct, WICED_TRUE );
    if ( result != WICED_SUCCESS )
    {
            PRINT_ERR("DCT write failed !\r\n");
    }

_dct_read_failed:
    result = wiced_configure_device( app_config );

    return result;
}
#endif

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

static void app_context_init(void)
{
    app_ctx.dct_app = NULL;
    app_ctx.dct_wifi = NULL;
    wiced_rtos_init_semaphore(&app_ctx.button_wac_semaphore);
    app_ctx.current_airplay_server_state = AIRPLAY_SERVER_IDLE;
#if defined(USE_DACP_CONSOLE)
    app_ctx.dacp_is_connected = WICED_FALSE;
#endif /* USE_DACP_CONSOLE */
#if defined(USE_APPLE_WAC)
    wiced_result_t ret = WICED_ERROR;
    app_ctx.airplay_interface = WICED_STA_INTERFACE;
    app_ctx.button_action_start_wac = WICED_FALSE;

    wiced_rtos_create_worker_thread( &button_worker_thread, WICED_DEFAULT_WORKER_PRIORITY, BUTTON_WORKER_STACK_SIZE, BUTTON_WORKER_QUEUE_SIZE );

    ret = button_manager_init( &button_manager, &button_manager_configuration, &button_worker_thread, buttons, ARRAY_SIZE( buttons ) );
    if ( ret != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("[App] Failed to register Buttons!!\n"));
    }
#endif /* USE_APPLE_WAC */
#ifdef USE_AUDIO_DISPLAY
    app_ctx.ssid_str[0] = '\0';
    audio_display_create_management_thread(&app_ctx.display_thread);
    audio_display_header_update_options(BATTERY_ICON_IS_VISIBLE | BATTERY_ICON_SHOW_PERCENT);
#endif /*USE_AUDIO_DISPLAY */
}

static void app_context_deinit(void)
{
#if defined(USE_APPLE_WAC)
    /* disable input event handling */
   app_ctx.button_action_start_wac = WICED_TRUE;
   button_manager_deinit( &button_manager );
#endif /* USE_APPLE_WAC */
    wiced_rtos_deinit_semaphore(&app_ctx.button_wac_semaphore);
}

void application_start(void)
{
    wiced_result_t result;

    /* Initialise the device */
    result = wiced_init( );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("WICED framework init failed !\r\n");
        goto _short_exit;
    }

    app_context_init();

    /* Find out if device is configured with WiFi credentials */
    result = wiced_dct_read_lock( (void**) &app_ctx.dct_wifi, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0,
                                  sizeof(platform_dct_wifi_config_t) );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("Can't get device configuration status !\r\n");
        goto _clean_exit;
    }

    /* Read speaker name string from app DCT */
    result = wiced_dct_read_lock( (void**) &app_ctx.dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof(airplay_dct_t) );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("Can't get app data !\r\n");
        goto _clean_exit;
    }

    /* Generate a unique speaker name if string from app DCT is empty */
    if ( app_ctx.dct_app->speaker_name[0] == '\0' )
    {
        result = set_unique_device_name(app_ctx.dct_app->speaker_name, sizeof(app_ctx.dct_app->speaker_name));
        if ( result != WICED_SUCCESS )
        {
            PRINT_ERR("Can't set unique device name !\r\n");
            goto _clean_exit;
        }
    }

    /* Set hostname for DHCP client */
    result = wiced_network_set_hostname( app_ctx.dct_app->speaker_name );
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("wiced_network_set_hostname() failed !\r\n");
    }

    /* Start WAC process if device is UNconfigured */
    AIRPLAY_START_WAC_IF_WIFI_DEVICE_IS_UNCONFIGURED();
    /* If BACK button has been pressed then flag device as UNconfigured and reboot */
    AIRPLAY_SET_WIFI_DEVICE_UNCONFIGURED_AND_REBOOT();

    /* Bring up the interface */
    result = wiced_network_up_default( &app_ctx.airplay_interface, &ap_ip_settings );

    /* If BACK button has been pressed then flag device as UNconfigured and reboot */
    AIRPLAY_SET_WIFI_DEVICE_UNCONFIGURED_AND_REBOOT();

    if( result != WICED_SUCCESS )
    {
        PRINT_ERR("Bringing up network interface failed !\r\n");
        goto _clean_exit;
    }

    /* Initialize platform audio */
    result = platform_init_audio();
    if ( result != WICED_SUCCESS )
    {
        PRINT_ERR("Bringing up audio platform !\r\n");
        goto _clean_exit;
    }
    do_airplay_audio();

    /* If BACK button has been pressed then flag device as UNconfigured and reboot */
    AIRPLAY_SET_WIFI_DEVICE_UNCONFIGURED_AND_REBOOT();

_clean_exit:
    /* Clean-up */
    if ( app_ctx.dct_app != NULL )
    {
        wiced_dct_read_unlock( app_ctx.dct_app, WICED_TRUE );
    }
    if ( app_ctx.dct_wifi != NULL )
    {
        wiced_dct_read_unlock( app_ctx.dct_wifi, WICED_TRUE );
    }
    app_context_deinit();
    wiced_deinit();
_short_exit:
    PRINT_ERR("Exiting !\r\n");
}
