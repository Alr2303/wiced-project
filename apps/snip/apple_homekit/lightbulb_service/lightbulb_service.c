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
 * HomeKit Lightbulb Accessory Snippet
 *
 * This snippet demonstrates how to setup a lightbulb service
 *
 * The snippet has 2 build options:
 *    - MFi mode
 *    - Non-MFi mode
 *
 *     MFi Mode
 *        This uses WAC to start a soft AP and bring the Wiced HomeKit Lightbulb Accessory onto the network
 *        It also advertises MFi support, and uses MFi certificate during the pairing process
 *        to prove it is an MFi device.
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.lightbulb_service-BCM9WCDPLUS114 USE_MFI=1
 *
 *     Non-MFi Mode
 *        This uses the wifi credentials stored in wifi_config_dct.h to bring up the Wiced HomeKit Lightbulb Accessory
 *        This advertises pin only support, and does not use MFi certificate during pairing
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.lightbulb_service-BCM9WCDPLUS114
 *
 *
 * Once the Wiced HomeKit Lightbulb Accessory is on the network, it can be discovered by the HAT application or iPhone/iPad app.
 * From here you should be able to perform a pair setup and perform a discovery of all the services
 * and characteristics.
 *
 * Features demonstrated
 *  - Adding of multiple services and characteristics
 *  - MFi/Non MFi pairing support
 *  - Read/Write to characteristics
 *  - Support of adding multiple controllers (maximum of 16), through the 'add pairing' request from controller
 *  - Support of removing multiple controller through the 'remove pairing' request from controller
 *  - Event handling ( request for events and sending events to all registered controllers )
 *  - Execution of the identify routine ( both from url and the Accessory Information Service characteristic )
 *
 * Application Instructions
 *   1. If running non-MFi mode, modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials
 *      in the wifi_config_dct.h header file to match your Wi-Fi access point
 *
 *   2. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *
 *   3. After the download completes, the terminal displays WICED startup
 *      information and starts WICED Configuration Mode.
 *
 *      If running MFi mode, use the AirPort utility on the MAC (or  iPhone/iPad ), to bring up the Wiced HomeKit Lightbulb Accessory
 *
 * HomeKit Accessory Demonstration
 *   To demonstrate the Wiced HomeKit Lightbulb Accessory, start the HAT application once the Wiced HomeKit Lightbulb Accessory has been added to
 *   your local network.
 *   - In the HAT application, click on the "Start Pairing" button, and wait for the "Accessory Password" to be shown
 *     on the serial output.
 *   - Enter the password shown on the display ( including the dashes )
 *   - Once the pairing is complete, click on the "Discover" button to perform a 'Pair-Verify' and get a listing of all
 *     services and characteristics for the accessory.
 *   - You can now modify a characteristic value which has read/write permissions
 *   - To add multiple controllers, perform the following steps:
 *     1. In the HAT application, click on the bottom right '+' sign and add another IP device
 *     2. For this controller, press "Start" to start listening for the Wiced HomeKit Lightbulb Accessory
 *     3. Go to the paired admin controller, and select the newly created IP device, then select "Add Pairing"
 *     4. Once the pairing has been added, you can return to the new IP device, and begin discovery.
 *   - To enable events, go to the 'on' characteristic and press the 'enable' button next to "Event Notifications"
 *   - To send an event back to the controller, toggle D1 LED on the eval board by pressing SW1. This should send
 *     back either an on or off event
 *
 * Notes.
 *   The HomeKit specification imposes some restrictions on the usage of the accessory, which may not be intuitive.
 *   Please keep these points in mind when exploring the Wiced HomeKit Lightbulb Accessory snippet.
 *
 *   1. One can not remove a pairing unless the device has gone through "Pair-Verify", which is part of the Discover
 *      process. This means once 'Pair Setup' is complete, the pairing can not be removed until the Services of the accessory have
 *      been discovered.
 *
 *   2. Only controllers which are admin controllers can perform add/remove pairings ( the first controller to pair automatically
 *      becomes an admin controller )
 *
 *   3. Once a controller is paired, no other controllers can go through pair setup. The only way to bring add another controller
 *      will be through Add/Remove pairing from the admin controller.
 *
 *   4. Controllers are persistently stored. This means if you reset HAT application, without removing the pairing beforehand,
 *      the appliance will still think it is paired to the controller. In this case you must perform a factory reset by downloading
 *      the firmware again.
 *
 */

#include "wiced.h"
#include "apple_homekit.h"
#include "apple_homekit_characteristics.h"
#include "apple_homekit_services.h"
#include "apple_homekit_developer.h"
#include "button_manager.h"
#include "apple_homekit_app_helper.h"

#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI
#include "apple_wac.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#ifdef HOMEKIT_ICLOUD_ENABLE
#define APPLE_CA_CERTIFICATE_FOR_ICLOUD_SERVER      ""    // Root CA certificate issued by Apple should used here
#define MAIN_ACCESSORY_NAME                         "Wiced Cloud Lightbulb-"
#else
#define MAIN_ACCESSORY_NAME                         "Wiced IP Lightbulb-"
#endif

/******************************************************
 *                    Constants
 ******************************************************/

#define COMPARE_MATCH                       ( 0 )
#define MAX_NAME_LENGTH                     ( 64 )
#define BUTTON_WORKER_STACK_SIZE            ( 1024 )
#define BUTTON_WORKER_QUEUE_SIZE            ( 4 )
#define LIGHTBULB_ON_KEY_CODE               ( 1 )
#define MAC_OCTET4_OFFSET                   ( 9 )

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

wiced_result_t wiced_homekit_actions_on_remote_value_update( wiced_homekit_characteristic_value_update_list_t* update_list );
wiced_result_t wicket_homekit_action_on_generic_event_callback( wiced_homekit_generic_event_info_t generic_event_info );
wiced_result_t wiced_homekit_display_password( uint8_t* srp_pairing_password );
wiced_result_t identify_action( uint16_t accessory_id, uint16_t characteristic_id );
static wiced_result_t read_homekit_persistent_data( void* data_ptr, uint32_t offset, uint32_t data_length );
static wiced_result_t write_homekit_persistent_data( void* data_ptr, uint32_t offset, uint32_t data_length );
static void    lightbulb_control_button_handler( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state );

/******************************************************
 *               Variable Definitions
 ******************************************************/
static const wiced_button_manager_configuration_t button_manager_configuration =
{
    .short_hold_duration     = 300  * MILLISECONDS,
    .debounce_duration       = 100  * MILLISECONDS,

    .event_handler           = lightbulb_control_button_handler,
};
char accessory_name[MAX_NAME_LENGTH] = { MAIN_ACCESSORY_NAME };

#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI

char* protocols[] =
{
    [ 0 ] = "com.apple.one",
};


/* info structure for WAC, see apple_wac.h for more details */
apple_wac_info_t apple_wac_info =
{
    .supports_homekit       = WICED_TRUE,
    .supports_homekit_v2    = WICED_FALSE, /*TODO: this needs to be add in other snip homekit applications R8*/
    .supports_airplay       = WICED_FALSE,
    .supports_airprint      = WICED_FALSE,
    .supports_5ghz_networks = WICED_FALSE,
    .has_app                = WICED_TRUE,
    .firmware_revision      = "1.0.0",
    .hardware_revision      = "2.0.0",
    .serial_number          = "001122334455",
    .model                  = "A6969",
    .manufacturer           = "Test",
    .oui                    = (uint8_t *)"\xAA\xBB\xCC",
    .mfi_protocols          = &protocols[0],
    .num_mfi_protocols      = 1,
    .bundle_seed_id         = "ABCA66M49V",
    .mdns_desired_hostname  = "homekit-lightbulb-test",
    .mdns_nice_name         = MAIN_ACCESSORY_NAME,
    .device_random_id       = "XX:XX:XX:XX:XX:XX",
};
#endif

apple_homekit_accessory_config_t apple_homekit_discovery_settings =
{
#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI
    .mfi_config_features = MFI_CONFIG_FEATURES_SUPPORTS_MFI_AND_PIN,
#else
    .mfi_config_features = MFI_CONFIG_FEATURES_SUPPORTS_PIN_ONLY,
#endif
    .mfi_protocol_version_string    = "1.0",
    .accessory_category_identifier  = LIGHTBULB

};


/* Accessory definitions */
wiced_homekit_accessories_t lightbulb_accessory;

/* Accessory Information Service and Characteristics */
wiced_homekit_services_t        lightbulb_accessory_information_service;
wiced_homekit_characteristics_t lightbulb_accessory_identify_characteristic;
wiced_homekit_characteristics_t lightbulb_accessory_manufacturer_characteristic;
wiced_homekit_characteristics_t lightbulb_accessory_model_characteristic;
wiced_homekit_characteristics_t lightbulb_accessory_name_characteristic;
wiced_homekit_characteristics_t lightbulb_accessory_serial_number_characteristic;
wiced_homekit_characteristics_t lightbulb_accessory_firmwar_revision_characteristic;


/* Lightublb services and characteristics */
wiced_homekit_services_t        lightbulb_service;
wiced_homekit_characteristics_t lightbulb_on_characteristic;
wiced_homekit_characteristics_t lightbulb_hue_characteristic;
wiced_homekit_characteristics_t lightbulb_brightness_characteristic;
wiced_homekit_characteristics_t lightbulb_saturation_characteristic;
wiced_homekit_characteristics_t lightbulb_name_characteristic;
wiced_homekit_characteristics_t lightbulb_color_custom_characteristic;        /* Custom characteristic */
wiced_homekit_characteristics_t lightbulb_power_custom_characteristic;        /* Custom characteristic */

/* Firmware upgrade service and characteristics */
wiced_homekit_services_t        lightbulb_firmware_upgrade_service;           /* Custom service  */
wiced_homekit_characteristics_t lightbulb_firmware_upgrade_characteristic;     /* Custom characteristic  */
wiced_homekit_characteristics_t lightbulb_firmware_name_characteristic;

/* Accessory information service characteristic values */
char lightbulb_accessory_name_value[MAX_NAME_LENGTH] = "\""MAIN_ACCESSORY_NAME"\"";
char lightbulb_accessory_manufacturer_value[]        = "\"Lights Co.\"";
char lightbulb_accessory_model_value[]               = "\"Controller 1.0\"";
char lightbulb_accessory_serial_number_value[]       = "\"12345678\"";
char lightbulb_accessory_firmware_revision_value[]   = "\"1.0.0\"";

/* Lightbulb characteristics values */
char lightbulb_name_value[]        = "\"LED lightbulb \"";
char lightbulb_on_value[]          = "false";
char lightbulb_hue_value[]         = "360";
char lightbulb_brightness_value[]  = "100";
char lightbulb_saturation_value[]  = "100";
char lightbulb_color_value[]       = "360";
char lightbulb_power_value[]       = "100";

/* Lightbulb firmware upgrade characteristics values */
char lightbulb_firmware_upgrade_value[1024];
char lightbulb_firmware_name_value[]    = "\"Lightbulb firmware upgrade custom service\"";

/* Static button configuration */
static const wiced_button_configuration_t button_configurations[] =
{
#if ( WICED_PLATFORM_BUTTON_COUNT > 0 )
    [ 0 ]  = { PLATFORM_BUTTON_1, BUTTON_CLICK_EVENT , LIGHTBULB_ON_KEY_CODE },
#endif
};

/* Button objects for the button manager */
button_manager_button_t buttons[] =
{
#if ( WICED_PLATFORM_BUTTON_COUNT > 0 )
    [ 0 ]  = { &button_configurations[ 0 ] },
#endif
};


/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_result_t          result;
    uint32_t                wiced_heap_allocated_for_database;
    wiced_worker_thread_t   button_worker_thread;
    static button_manager_t button_manager;
    wiced_mac_t             mac;
    char                    mac_string[STRING_MAX_MAC];
    uint8_t                 offset;
    uint8_t                 mac_offset;
    uint16_t                iid = 1;

    wiced_init();

    /* Generate unique name for accessory */
    wiced_wifi_get_mac_address( &mac );
    mac_address_to_string( mac.octet, mac_string );
    offset = sizeof(MAIN_ACCESSORY_NAME) - 1;
    mac_offset = MAC_OCTET4_OFFSET;
    memcpy((accessory_name + offset), &mac_string[mac_offset], 2);
    offset = (uint8_t)(offset + 2);
    mac_offset = (uint8_t)(mac_offset + 3);
    memcpy((accessory_name + offset), &mac_string[mac_offset], 2);
    offset = (uint8_t)(offset + 2);
    mac_offset = (uint8_t)(mac_offset + 3);
    memcpy((accessory_name + offset), &mac_string[mac_offset], 2);
    offset = (uint8_t)(offset + 2);
    *(accessory_name + offset) = '\0';

    WPRINT_APP_INFO(("Accessory Mac Addr = [%s]\n", mac_string));
    WPRINT_APP_INFO(("Unique Accessory Name = [%s]\n", accessory_name));

    apple_homekit_discovery_settings.name = accessory_name;
#ifdef RUN_ON_ETHERNET_INTERFACE
    apple_homekit_discovery_settings.interface = WICED_ETHERNET_INTERFACE;
#else
    apple_homekit_discovery_settings.interface = WICED_STA_INTERFACE;
#endif

#ifdef WICED_HOMEKIT_DO_NOT_USE_MFI
    /* Bring up the network interface */
    result = wiced_network_up( apple_homekit_discovery_settings.interface, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Unable to bring up network connection\n"));
        return;
    }
    homekit_get_random_device_id( apple_homekit_discovery_settings.device_id );
#else

    apple_wac_info.name = apple_homekit_discovery_settings.name;

    homekit_get_random_device_id( apple_homekit_discovery_settings.device_id );
    memcpy( apple_wac_info.device_random_id, apple_homekit_discovery_settings.device_id, sizeof(apple_homekit_discovery_settings.device_id) );

    /* Check if the device is factory reset */
    if ( is_device_factory_reset() == WICED_FALSE )
    {
        /* Device is not factory reset. Reload the characteristics values from DCT */
        homekit_application_dct_structure_t *app_dct;

        /* Reading the characteristics values from persistent storage */
        wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(is_factoryrest), sizeof( *app_dct ) );
        snprintf( lightbulb_on_value, sizeof(lightbulb_on_value), "%s", app_dct->lightbulb_on_value );
        snprintf( lightbulb_brightness_value, sizeof(lightbulb_brightness_value), "%s", app_dct->lightbulb_brightness_value );
        snprintf( lightbulb_hue_value, sizeof(lightbulb_hue_value), "%s", app_dct->lightbulb_hue_value );
        snprintf( lightbulb_saturation_value, sizeof(lightbulb_saturation_value), "%s", app_dct->lightbulb_saturation_value );
        snprintf( lightbulb_color_value, sizeof(lightbulb_color_value), "%s", app_dct->lightbulb_color_value );
        wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );
    }

    /* Bring up the network interface */
    result = wiced_network_up( apple_homekit_discovery_settings.interface, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Invalid Entry in DCT (or) the configured WiFi network is not UP at present. Starting WAC\n"));
        result = apple_wac_configure( &apple_wac_info );

        /* Clear once WAC is complete, and no longer used */
        if ( apple_wac_info.out_play_password != NULL )
        {
            free( apple_wac_info.out_play_password );
        }

        if ( result != WICED_SUCCESS )
        {
            /* Clear once WAC is complete, and no longer used */
            if ( apple_wac_info.out_new_name != NULL )
            {
                free( apple_wac_info.out_new_name );
            }

            WPRINT_APP_INFO( ("WAC failure\n") );
            return;
        }

        /* If we were assigned a new name, update with new name */
        if ( apple_wac_info.out_new_name != NULL )
        {
            snprintf( accessory_name, sizeof(accessory_name), "%s", apple_wac_info.out_new_name );
            snprintf( lightbulb_accessory_name_value, sizeof(lightbulb_accessory_name_value), "\"%s\"", apple_wac_info.out_new_name );
            free( apple_wac_info.out_new_name );
        }
    }

#endif

    /* Initialise Lightbulb Accessory information service */
    wiced_homekit_initialise_accessory_information_service( &lightbulb_accessory_information_service, iid++);

    /* Initialise lightbulb service */
    wiced_homekit_initialise_lightbulb_service( &lightbulb_service, iid++);

    /* Initialise lightbulb firmware upgrade service */
    wiced_homekit_initialise_firmware_upgrade_service( &lightbulb_firmware_upgrade_service, iid++);

    /******************************
     * Initialise characteristics
     ******************************/

    /* Lightbulb Information Service Characteristics */
    wiced_homekit_initialise_identify_characteristic        ( &lightbulb_accessory_identify_characteristic,      identify_action, iid++ );
    wiced_homekit_initialise_name_characteristic            ( &lightbulb_accessory_name_characteristic,          lightbulb_accessory_name_value,          strlen( lightbulb_accessory_name_value ),          NULL, NULL, iid++ );
    wiced_homekit_initialise_manufacturer_characteristic    ( &lightbulb_accessory_manufacturer_characteristic,  lightbulb_accessory_manufacturer_value,  strlen( lightbulb_accessory_manufacturer_value ),  NULL, NULL, iid++ );
    wiced_homekit_initialise_model_characteristic           ( &lightbulb_accessory_model_characteristic,         lightbulb_accessory_model_value,         strlen( lightbulb_accessory_model_value ),         NULL, NULL, iid++ );
    wiced_homekit_initialise_serial_number_characteristic   ( &lightbulb_accessory_serial_number_characteristic, lightbulb_accessory_serial_number_value, strlen( lightbulb_accessory_serial_number_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_firmwar_revision_characteristic( &lightbulb_accessory_firmwar_revision_characteristic, lightbulb_accessory_firmware_revision_value, strlen( lightbulb_accessory_firmware_revision_value ), NULL, NULL, iid++ );

    /* Initialise Lightbulb characteristics */
    wiced_homekit_initialise_on_characteristic           ( &lightbulb_on_characteristic,         lightbulb_on_value,         strlen( lightbulb_on_value ),         NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic         ( &lightbulb_name_characteristic,       lightbulb_name_value,       strlen( lightbulb_name_value ),       NULL, NULL, iid++ );
    wiced_homekit_initialise_hue_characteristic          ( &lightbulb_hue_characteristic,        lightbulb_hue_value,        strlen( lightbulb_hue_value ),        NULL, NULL, iid++ );
    wiced_homekit_initialise_brightness_characteristic   ( &lightbulb_brightness_characteristic, lightbulb_brightness_value, strlen( lightbulb_brightness_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_saturation_characteristic   ( &lightbulb_saturation_characteristic, lightbulb_saturation_value, strlen( lightbulb_saturation_value ), NULL, NULL, iid++ );

    /* Initialize custom characteristics */
    wiced_homekit_initialise_color_characteristic   ( &lightbulb_color_custom_characteristic,  lightbulb_color_value, strlen( lightbulb_color_value ), NULL, "\"Lightbulb Color Characteristic\"", iid++ );
    wiced_homekit_initialise_power_characteristic   ( &lightbulb_power_custom_characteristic,  lightbulb_power_value, strlen( lightbulb_power_value ), NULL, "\"Lightbulb Power Characteristic\"", iid++ );

    /* Initialize lightbulb firmware upgrade custom characteristics*/
    wiced_homekit_initialise_system_upgrade_characteristic  ( &lightbulb_firmware_upgrade_characteristic, lightbulb_firmware_upgrade_value, strlen( lightbulb_firmware_upgrade_value ), NULL, "\"Lightbulb Firmware Upgrade Characteristic\"", iid++ );
    wiced_homekit_initialise_name_characteristic            ( &lightbulb_firmware_name_characteristic, lightbulb_firmware_name_value, strlen( lightbulb_firmware_name_value ), NULL, NULL, iid++ );


    /* Add Lightbulb Accessory*/
    wiced_homekit_add_accessory( &lightbulb_accessory, 1 );

    /* Add Lightbulb Accessory Information Service */
    wiced_homekit_add_service( &lightbulb_accessory, &lightbulb_accessory_information_service );


    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_identify_characteristic      );
    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_manufacturer_characteristic  );
    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_model_characteristic         );
    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_name_characteristic          );
    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_serial_number_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_firmwar_revision_characteristic );


    /* Add Lightbulb service to accessory */
    wiced_homekit_add_service( &lightbulb_accessory, &lightbulb_service );

    /* set lightbulb service as primary service */
     wiced_homekit_service_set_primary( &lightbulb_accessory,&lightbulb_service);


    /* Add the required characteristics to the Lightbulb service */
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_on_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_hue_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_brightness_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_saturation_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_name_characteristic );

    /* Add Custom Characteristics */
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_color_custom_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_power_custom_characteristic );

    /* Add Lightbulb firmware upgrade custom service to accessory */
    wiced_homekit_add_service( &lightbulb_accessory, &lightbulb_firmware_upgrade_service );
    wiced_homekit_add_characteristic( &lightbulb_firmware_upgrade_service, &lightbulb_firmware_upgrade_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_firmware_upgrade_service, &lightbulb_firmware_name_characteristic );

#ifdef HOMEKIT_ICLOUD_ENABLE
    if (strlen(APPLE_CA_CERTIFICATE_FOR_ICLOUD_SERVER) == 0)
    {
        WPRINT_APP_INFO((" === INFO === : iCloud Root CA certificate is not loaded\n"));
        /* Add accessory Relay Service & it's characteristics */
        wiced_homekit_add_relay_service( &lightbulb_accessory, iid, NULL, 0 );
    }
    else
    {
        wiced_homekit_add_relay_service( &lightbulb_accessory, iid, APPLE_CA_CERTIFICATE_FOR_ICLOUD_SERVER, strlen(APPLE_CA_CERTIFICATE_FOR_ICLOUD_SERVER) );
    }
#endif

    wiced_configure_accessory_password_for_device_with_display( wiced_homekit_display_password );

    /* Use the API below if your device has no display */
    /* wiced_configure_accessory_password_for_device_with_no_display( "888-77-888" ); */

    wiced_register_url_identify_callback( identify_action );

    wiced_register_value_update_callback( wiced_homekit_actions_on_remote_value_update );

    /* HomeKit generic event register callback */
    wiced_homekit_register_generic_event_callback( wicket_homekit_action_on_generic_event_callback );

    /* Register callback to read and write HomeKit persistent data */
    wiced_homekit_register_persistent_data_handling_callback( read_homekit_persistent_data, write_homekit_persistent_data );

    WPRINT_APP_INFO(("+-------------------+\r\n"));
    WPRINT_APP_INFO(("+ HomeKit Loading...+\r\n"));
    WPRINT_APP_INFO(("+-------------------+\r\n"));

    result = wiced_homekit_start( &apple_homekit_discovery_settings, &wiced_heap_allocated_for_database );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to start HomeKit service. Error = [%d].\n" ,result) );
        return;
    }

    WPRINT_APP_INFO(("+-------------------+\r\n"));
    WPRINT_APP_INFO(("+ HomeKit Ready!    +\r\n"));
    WPRINT_APP_INFO(("+-------------------+\r\n"));

    WPRINT_APP_INFO(("\r\nBytes in heap allocated for Accessory Database = %lu\r\n", wiced_heap_allocated_for_database ));

    result = wiced_rtos_create_worker_thread( &button_worker_thread, WICED_DEFAULT_WORKER_PRIORITY, BUTTON_WORKER_STACK_SIZE, BUTTON_WORKER_QUEUE_SIZE );
    if ( result != WICED_SUCCESS  )
    {
        WPRINT_APP_INFO( ("Failed to create Button worker thread.\n") );
        return;
    }
    result = button_manager_init( &button_manager, &button_manager_configuration, &button_worker_thread, buttons, ARRAY_SIZE( buttons ) );
    if ( result !=  WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to initialize button interface.\n") );
        return;
    }
    while( 1 )
    {
        /**
         * Send send all characteristic values which have been updated and had event notifications enabled to controller
         * Send the list created by calling wiced_homekit_register_characteristic_value_update
         **/

        /* Send updates to controllers */
        wiced_homekit_send_all_updates_for_accessory( );

        /* Based on HAP spec, all the notification should be coalesced using a delay of no less than 1 sec */
        wiced_rtos_delay_milliseconds( 1000 * 2 );
    }
}

wiced_result_t wiced_homekit_actions_on_remote_value_update( wiced_homekit_characteristic_value_update_list_t* update_list )
{
    int                                 i;
    uint16_t                            offset;
    uint8_t                             data_length;
    char                                updated_value[6]; /* Maximum size of characteristics value stored in persistent storage is 6 bytes */
    char*                               dct_update_value;
    wiced_homekit_characteristics_t*    updated_characteristic = NULL;

    WPRINT_APP_INFO(("--------- No. of characteristics updates received = [%d] ---------\n", update_list->number_of_updates ));
    /* Cycle through list of all updated characteristics */
    for ( i = 0; i < update_list->number_of_updates; i++ )
    {
        updated_characteristic = NULL;

        /* If Lightbulb Accessory, then check to see what services are being activated */
        if ( lightbulb_accessory.instance_id == update_list->characteristic_descriptor[ i ].accessory_instance_id )
        {
//            WPRINT_APP_INFO(("\r\n LIGHTBULB ACCESSORY \r\n"));
            offset = 0;

            if ( lightbulb_on_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                if ( ( lightbulb_on_characteristic.value.new.value[ 0 ] == '1' ) ||
                     ( lightbulb_on_characteristic.value.new.value[ 0 ] == 't' ) )
                {
                    WPRINT_APP_INFO(("\r\n\t Received light ON request \r\n" ));
                    wiced_gpio_output_high( WICED_LED1 );
                }
                else
                {
                    WPRINT_APP_INFO(("\r\n\t Received light OFF request \r\n" ));
                    wiced_gpio_output_low( WICED_LED1 );
                }

                updated_characteristic = &lightbulb_on_characteristic;
                memcpy( updated_value, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
                data_length = updated_characteristic->value.new.value_length + 1;
                offset = HOMEKIT_APP_DCT_OFFSET(lightbulb_on_value);
            }
            else if ( lightbulb_hue_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                updated_characteristic = &lightbulb_hue_characteristic;
                WPRINT_APP_INFO(("\r\n\t Received hue value: %.*s \r\n", updated_characteristic->value.new.value_length, updated_characteristic->value.new.value ));
                memcpy( updated_value, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
                data_length = updated_characteristic->value.new.value_length + 1;
                offset = HOMEKIT_APP_DCT_OFFSET(lightbulb_hue_value);
            }
            else if ( lightbulb_brightness_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                updated_characteristic = &lightbulb_brightness_characteristic;
                WPRINT_APP_INFO(("\r\n\t Received brightness value: %.*s \r\n", updated_characteristic->value.new.value_length, updated_characteristic->value.new.value ));
                memcpy( updated_value, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
                data_length = updated_characteristic->value.new.value_length + 1;
                offset = HOMEKIT_APP_DCT_OFFSET(lightbulb_brightness_value);
            }
            else if ( lightbulb_saturation_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                updated_characteristic = &lightbulb_saturation_characteristic;
                WPRINT_APP_INFO(("\r\n\t Received saturation value: %.*s \r\n", updated_characteristic->value.new.value_length, updated_characteristic->value.new.value ));
                memcpy( updated_value, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
                data_length = updated_characteristic->value.new.value_length + 1;
                offset = HOMEKIT_APP_DCT_OFFSET(lightbulb_saturation_value);
            }
            else if ( lightbulb_color_custom_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                updated_characteristic = &lightbulb_color_custom_characteristic;
                WPRINT_APP_INFO(("\r\n\t Received light color value: %.*s \r\n", updated_characteristic->value.new.value_length, updated_characteristic->value.new.value ));
                memcpy( updated_value, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
                data_length = updated_characteristic->value.new.value_length + 1;
                offset = HOMEKIT_APP_DCT_OFFSET(lightbulb_color_value);
            }

            if ( updated_characteristic != NULL )
            {
                /* If the requested value is different to the current value/state update to new value */
                if ( memcmp( updated_characteristic->value.new.value, updated_characteristic->value.current.value, updated_characteristic->value.current.value_length ) != COMPARE_MATCH )
                {
                    /* Copies the new value requested by controller (value.new) to the current value (memory pointed to by value.current) */
                    wiced_homekit_accept_controller_value( updated_characteristic );

                    /**
                      * Update the current value length( value.current.value_length), and place this characteristic value
                      * into a list of events to be sent to the controller (if controller enabled events for this characteristic)
                      **/
                     wiced_homekit_register_characteristic_value_update( &lightbulb_accessory, updated_characteristic, updated_characteristic->value.current.value, updated_characteristic->value.current.value_length );

                     /* Preserve the updated characteristic value to the application persistent storage area */
                     wiced_dct_read_lock( (void**)&dct_update_value, WICED_TRUE, DCT_HOMEKIT_APP_SECTION, offset, data_length );
                     memcpy ( (void*)dct_update_value, (const void*)updated_value, data_length );
                     wiced_dct_write( (const void *)dct_update_value, DCT_HOMEKIT_APP_SECTION, offset, data_length  );
                     wiced_dct_read_unlock( (void*)dct_update_value, WICED_TRUE );
                }
            }
        }
        else
        {
            WPRINT_APP_INFO(("\r\n\t No actions for accessory: %d\r\n", update_list->characteristic_descriptor[i].accessory_instance_id ));
        }
    }
    WPRINT_APP_INFO(("--------- All received characteristics are updated ---------\n"));

    return WICED_SUCCESS;
}

wiced_result_t wicket_homekit_action_on_generic_event_callback( wiced_homekit_generic_event_info_t generic_event_info )
{
    switch( generic_event_info.type )
    {
        case HOMEKIT_GENERIC_EVENT_PAIRING_REQUESTED:
            WPRINT_APP_INFO(("Received new pairing request form controller.\n"));
            break;

        case HOMEKIT_GENERIC_EVENT_PAIRING_FAILED:
            WPRINT_APP_INFO(("New pairing request failed with ERROR [%d]\n", generic_event_info.info.error_code));
            break;

        case HOMEKIT_GENERIC_EVENT_PAIRED:
            WPRINT_APP_INFO(("Controller [%d] PAIRED.\n", generic_event_info.info.controller_id));
            break;

        case HOMEKIT_GENERIC_EVENT_REMOVE_PAIRING_REQUESTED:
            WPRINT_APP_INFO(("Received remove controller request.\n"));
            break;

        case HOMEKIT_GENERIC_EVENT_REMOVE_PAIRING_FAILED:
            WPRINT_APP_INFO(("Remove controller request failed with ERROR [%d]\n", generic_event_info.info.error_code));
            break;

        case HOMEKIT_GENERIC_EVENT_PAIRING_REMOVED:
            WPRINT_APP_INFO(("Controller [%d] REMOVED.\n", generic_event_info.info.controller_id));
            break;

        case HOMEKIT_GENERIC_EVENT_CONTROLLER_CONNECTED:
            WPRINT_APP_INFO(("Controller [%d] CONNECTED.\n", generic_event_info.info.controller_id));
            break;

        case HOMEKIT_GENERIC_EVENT_CONTROLLER_DISCONNECTED:
            WPRINT_APP_INFO(("Controller [%d] DISCONNECTED.\n", generic_event_info.info.controller_id));
            break;

        default:
            break;
    }

    return WICED_SUCCESS;
}
wiced_result_t wiced_homekit_display_password( uint8_t* srp_pairing_password )
{
    WPRINT_APP_INFO(("\r\n"));
    WPRINT_APP_INFO(("+------------------------------+\r\n"));
    WPRINT_APP_INFO(("+ Pairing Password: %s +\r\n", (char*)srp_pairing_password ));
    WPRINT_APP_INFO(("+------------------------------+\r\n"));
    WPRINT_APP_INFO(("\r\n"));

    return WICED_SUCCESS;
}

/*
 * NOTE:
 *  Accessory id = 0 is used URL identify
 */
wiced_result_t identify_action( uint16_t accessory_id, uint16_t characteristic_id )
{
    int i;

    /* Check if this is the Lightbulb Accessory */
    if ( lightbulb_accessory.instance_id == accessory_id )
    {
        WPRINT_APP_INFO(("\r\n LIGHTBULB ACCESSORY IDENTIFY \r\n"));

        /* Flash the LED representing the lightbulb */
        for ( i = 0; i < 5; i++ )
        {
            wiced_gpio_output_low( WICED_LED2 );

            wiced_rtos_delay_milliseconds( 200 );

            wiced_gpio_output_high( WICED_LED2 );

            wiced_rtos_delay_milliseconds( 200 );
        }

        wiced_gpio_output_low( WICED_LED2 );
    }
    else if ( accessory_id == 0)
    {
        WPRINT_APP_INFO(("\r\n URL IDENTIFY \r\n"));

        /* Flash the LED representing the URL identify */
        for ( i = 0; i < 3; i++ )
        {
            wiced_gpio_output_low( WICED_LED2 );

            wiced_rtos_delay_milliseconds( 300 );

            wiced_gpio_output_high( WICED_LED2 );

            wiced_rtos_delay_milliseconds( 300 );
        }

        wiced_gpio_output_low( WICED_LED2 );
    }
    return WICED_SUCCESS;
}

/* Handles Button events */
static void lightbulb_control_button_handler( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state )
{
    char*  value = lightbulb_on_characteristic.value.current.value;
    if( event == BUTTON_CLICK_EVENT )
    {
        if ( ( *value == '1' ) || ( *value == 't' ) )
        {
            lightbulb_on_value[0]= '0';
            WPRINT_APP_INFO(("\n Received light OFF request \n"));
            wiced_gpio_output_low( WICED_LED1 );
        }
        else
        {
            lightbulb_on_value[0]= '1';
            WPRINT_APP_INFO(("\n Received light ON request \n"));
            wiced_gpio_output_high( WICED_LED1 );
        }
        /**
         * Update the current value length( value.current.value_length), and place this characteristic value
         * into a list of events to be sent to the controller (if controller enabled events for this characteristic)
         **/
        wiced_homekit_register_characteristic_value_update( &lightbulb_accessory, &lightbulb_on_characteristic, lightbulb_on_value, 1);
    }
    return;
}


static wiced_result_t read_homekit_persistent_data( void* data_ptr, uint32_t offset, uint32_t data_length )
{
    void* local_dct_ptr;

    wiced_dct_read_lock( &local_dct_ptr, WICED_FALSE, DCT_APP_SECTION,  offset, data_length );

    memcpy( data_ptr, local_dct_ptr, data_length );

    wiced_dct_read_unlock( local_dct_ptr, WICED_FALSE );

    return WICED_TRUE;
}

static wiced_result_t write_homekit_persistent_data( void* data_ptr, uint32_t offset, uint32_t data_length )
{
    void* local_dct_ptr;

    wiced_dct_read_lock( &local_dct_ptr, WICED_TRUE, DCT_APP_SECTION, offset, data_length );

    memcpy( local_dct_ptr, data_ptr, data_length );

    wiced_dct_write( local_dct_ptr, DCT_APP_SECTION, offset, data_length );

    wiced_dct_read_unlock( local_dct_ptr, WICED_TRUE );

    return WICED_TRUE;
}
