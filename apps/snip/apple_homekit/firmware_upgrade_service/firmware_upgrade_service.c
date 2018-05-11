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
 * HomeKit Firmware Upgrade Snippet
 *
 * This snippet demonstrates how to setup a firmware upgrade accessory using a custom service
 *
 * The snippet has 2 build options:
 *    - MFi mode
 *    - Non-MFi mode
 *
 *     MFi Mode
 *        This uses WAC to start a soft AP and bring the Wiced HomeKit Accessory onto the network
 *        It also advertises MFi support, and uses MFi certificate during the pairing process
 *        to prove it is an MFi device.
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.firmware_upgrade_service-BCM9WCDPLUS114 download download_apps USE_MFI=1
 *
 *     Non-MFi Mode
 *        This uses the wifi credentials stored in wifi_config_dct.h to bring up the Wiced HomeKit Accessory
 *        This advertises pin only support, and does not use MFi certificate during pairing
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.firmware_upgrade_service-BCM9WCDPLUS114 download download_apps
 *
 * Once the Wiced HomeKit Accessory is on the network, it can be discovered by the HAT application or iPhone/iPad app.
 * From here you should be able to perform a pair setup and perform a discovery of all the services
 * and characteristics.
 *
 * Features demonstrated
 *  - Adding of multiple services and characteristics
 *  - MFi/Non MFi pairing support
 *  - Read/Write to characteristics
 *  - Support of adding multiple controllers (maximum of 16), through the 'add pairing' request from controller
 *  - Support of removing multiple controller through the 'remove pairing' request from controller
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
 *      If running MFi mode, use the AirPort utility on the MAC (or  iPhone/iPad ), to bring up the Wiced HomeKit Accessory
 *
 * HomeKit Accessory Demonstration
 *   To demonstrate the Wiced HomeKit Firmware Upgrade, start the HAT application once the Wiced HomeKit Firmware Upgrade has been added to
 *   your local network.
 *   - In the HAT application, click on the "Start Pairing" button, and wait for the "Accessory Password" to be shown
 *     on the serial output.
 *   - Enter the password shown on the display ( including the dashes )
 *   - Once the pairing is complete, click on the "Discover" button to perform a 'Pair-Verify' and get a listing of all
 *     services and characteristics for the accessory.
 *   - You can now modify a characteristic value which has read/write permissions
 *   - To add multiple controllers, perform the following steps:
 *     1. In the HAT application, click on the bottom right '+' sign and add another IP device
 *     2. For this controller, press "Start" to start listening for the Wiced HomeKit Accessory
 *     3. Go to the paired admin controller, and select the newly created IP device, then select "Add Pairing"
 *     4. Once the pairing has been added, you can return to the new IP device, and begin discovery.
 *   - To enable events, go to the 'on' characteristic and press the 'enable' button next to "Event Notifications"
 *   - To send an event back to the controller, toggle D1 LED on the eval board by pressing SW1. This should send
 *     back either an on or off event
 *
 * Notes.
 *   The HomeKit specification imposes some restrictions on the usage of the accessory, which may not be intuitive.
 *   Please keep these points in mind when exploring the snippet.
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
#include "wiced_framework.h"
#include "apple_homekit.h"
#include "apple_homekit_characteristics.h"
#include "apple_homekit_services.h"
#include "apple_homekit_developer.h"
#include "wiced_ota_server.h"
#include "apple_homekit_app_helper.h"

#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI
#include "apple_wac.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAIN_ACCESSORY_NAME     "Wiced IP Firmware Upgrade-"
#define MAX_NAME_LENGTH         64
#define MAC_OCTET4_OFFSET       ( 9 )

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
wiced_result_t wiced_homekit_display_password( uint8_t* srp_pairing_password );
wiced_result_t identify_action( uint16_t accessory_id, uint16_t characteristic_id );

/******************************************************
 *               Variable Definitions
 ******************************************************/
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
    .mdns_desired_hostname  = "homekit-firmware-upgrade-test",
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
    .accessory_category_identifier  = OTHER
};

static const wiced_ip_setting_t device_init_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS(192, 168, 10,  1) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS(255, 255, 255, 0) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS(192, 168, 10,  1) ),
};

wiced_bool_t firmware_update_request = WICED_FALSE;

/* Accessory definitions */
wiced_homekit_accessories_t firmware_upgrade_accessory;

/* Firmware upgrade Service and Characteristics */
wiced_homekit_services_t        firmware_upgrade_service;
wiced_homekit_characteristics_t firmware_upgrade_request_characteristic;
wiced_homekit_characteristics_t firmware_upgrade_request_name_characteristic;

wiced_homekit_services_t        firmware_upgrade_accessory_information_service;
wiced_homekit_characteristics_t firmware_upgrade_accessory_identify_characteristic;
wiced_homekit_characteristics_t firmware_upgrade_accessory_manufacturer_characteristic;
wiced_homekit_characteristics_t firmware_upgrade_accessory_model_characteristic;
wiced_homekit_characteristics_t firmware_upgrade_accessory_name_characteristic;
wiced_homekit_characteristics_t firmware_upgrade_accessory_serial_number_characteristic;


/* Firmware upgrade characteristic value */
char firmware_upgrade_request[]             = "false";
char firmware_upgrade_request_name_value[]  = "\"Request for firmware upgrade  \"";

/* Firmware upgrade information service characteristic values */
char firmware_upgrade_service_name_value[MAX_NAME_LENGTH]  = "\""MAIN_ACCESSORY_NAME"\"";
char firmware_upgrade_manufacturer_value[]                 = "\"OTA Co.\"";
char firmware_upgrade_model_value[]                        = "\"Firmware version 1.0\"";
char firmware_upgrade_serial_number_value[]                = "\"none\"";

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_result_t  result;
    uint32_t        wiced_heap_allocated_for_database;
    wiced_mac_t     mac;
    char            mac_string[STRING_MAX_MAC];
    uint8_t         offset;
    uint8_t         mac_offset;
    uint16_t        iid = 1;

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
    }

    /* If we were assigned a new name, update with new name */
    if ( apple_wac_info.out_new_name != NULL )
    {
        snprintf( accessory_name, sizeof(accessory_name), "%s", apple_wac_info.out_new_name );
        snprintf( firmware_upgrade_service_name_value, sizeof(firmware_upgrade_service_name_value), "\"%s\"", apple_wac_info.out_new_name );
        free( apple_wac_info.out_new_name );
    }

#endif


    /* Initialise firmware-upgrade information service */
    wiced_homekit_initialise_accessory_information_service( &firmware_upgrade_accessory_information_service, iid++ );

    /* Initialise firmware-upgrade service */
    wiced_homekit_initialise_firmware_upgrade_service( &firmware_upgrade_service, iid++ );

    /******************************
     * Initialise characteristics
     ******************************/

    /* Initialise Accessory Information characteristics */
    wiced_homekit_initialise_identify_characteristic        ( &firmware_upgrade_accessory_identify_characteristic, identify_action, iid++ );
    wiced_homekit_initialise_name_characteristic            ( &firmware_upgrade_accessory_name_characteristic,         firmware_upgrade_service_name_value,  strlen( firmware_upgrade_service_name_value ),  NULL, NULL, iid++ );
    wiced_homekit_initialise_manufacturer_characteristic    ( &firmware_upgrade_accessory_manufacturer_characteristic, firmware_upgrade_manufacturer_value,  strlen( firmware_upgrade_manufacturer_value ),  NULL, NULL, iid++ );
    wiced_homekit_initialise_model_characteristic           ( &firmware_upgrade_accessory_model_characteristic,        firmware_upgrade_model_value,         strlen( firmware_upgrade_model_value ),         NULL, NULL, iid++ );
    wiced_homekit_initialise_serial_number_characteristic   ( &firmware_upgrade_accessory_serial_number_characteristic,firmware_upgrade_serial_number_value, strlen( firmware_upgrade_serial_number_value ), NULL, NULL, iid++ );

    /* Initialise custom firmware upgrade characteristic */
    wiced_homekit_initialise_name_characteristic            ( &firmware_upgrade_request_name_characteristic,           firmware_upgrade_request_name_value,  strlen( firmware_upgrade_request_name_value ),  NULL, NULL, iid++ );
    wiced_homekit_initialise_on_characteristic              ( &firmware_upgrade_request_characteristic,                firmware_upgrade_request,             strlen( firmware_upgrade_request ),             NULL, NULL, iid++ );


    /* Create firmware upgrade accessory */
    wiced_homekit_add_accessory( &firmware_upgrade_accessory, 1 );

    /* Add accessory information service to accessory */
    wiced_homekit_add_service( &firmware_upgrade_accessory, &firmware_upgrade_accessory_information_service );

    /* Add the required characteristics to the accessory information service */
    wiced_homekit_add_characteristic( &firmware_upgrade_accessory_information_service, &firmware_upgrade_accessory_name_characteristic );
    wiced_homekit_add_characteristic( &firmware_upgrade_accessory_information_service, &firmware_upgrade_accessory_identify_characteristic );
    wiced_homekit_add_characteristic( &firmware_upgrade_accessory_information_service, &firmware_upgrade_accessory_manufacturer_characteristic );
    wiced_homekit_add_characteristic( &firmware_upgrade_accessory_information_service, &firmware_upgrade_accessory_model_characteristic );
    wiced_homekit_add_characteristic( &firmware_upgrade_accessory_information_service, &firmware_upgrade_accessory_serial_number_characteristic );

    /* Add  service to accessory */
    wiced_homekit_add_service( &firmware_upgrade_accessory, &firmware_upgrade_service );

    /* Give a name to the  characteristic */
    wiced_homekit_add_characteristic( &firmware_upgrade_service, &firmware_upgrade_request_name_characteristic );

    /* Add the required characteristics to the  service */
    wiced_homekit_add_characteristic( &firmware_upgrade_service, &firmware_upgrade_request_characteristic );

    wiced_configure_accessory_password_for_device_with_display( wiced_homekit_display_password );

    /* Use the API below if your device has no display */
    /* wiced_configure_accessory_password_for_device_with_no_display( "888-77-888" ); */

    wiced_register_url_identify_callback( identify_action );

    wiced_register_value_update_callback( wiced_homekit_actions_on_remote_value_update );

    WPRINT_APP_INFO(("+-------------------+\r\n"));
    WPRINT_APP_INFO(("+ HomeKit Loading...+\r\n"));
    WPRINT_APP_INFO(("+-------------------+\r\n"));

    if ( wiced_homekit_start( &apple_homekit_discovery_settings, &wiced_heap_allocated_for_database ) != WICED_SUCCESS )
    {
        return;
    }

    WPRINT_APP_INFO(("+-------------------+\r\n"));
    WPRINT_APP_INFO(("+ HomeKit Ready!    +\r\n"));
    WPRINT_APP_INFO(("+-------------------+\r\n"));

    WPRINT_APP_INFO(("\r\nBytes in heap allocated for Accessory Database = %lu\r\n", wiced_heap_allocated_for_database ));

    while( 1 )
    {
        if ( firmware_update_request == WICED_TRUE )
        {
            firmware_update_request = WICED_FALSE;

            /* Stop the HomeKit server and Gedday */
            wiced_homekit_stop();

#ifdef WICED_HOMEKIT_DO_NOT_USE_MFI
            wiced_network_down(apple_homekit_discovery_settings.interface);
#else
            wiced_network_down(WICED_AP_INTERFACE);
#endif
            wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &device_init_ip_settings );

            /* Start OTA server. Reference the snip.ota_fr snippet */
            wiced_ota_server_start( WICED_AP_INTERFACE );
        }

        wiced_rtos_delay_milliseconds( 250 );
    }
}

wiced_result_t wiced_homekit_actions_on_remote_value_update( wiced_homekit_characteristic_value_update_list_t* update_list )
{
    int   i;
    char* value;

    /* Cycle through list of all updated characteristics */
    for ( i = 0; i < update_list->number_of_updates; i++ )
    {
        if ( firmware_upgrade_accessory.instance_id == update_list->characteristic_descriptor[ i ].accessory_instance_id )
        {
            WPRINT_APP_INFO(("\r\n FIRMWARE UPGRADE ACCESSORY \r\n"));

            if ( firmware_upgrade_request_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                value = firmware_upgrade_request_characteristic.value.new.value;

                if ( ( *value == '1' ) ||
                     ( *value == 't' ) )
                {
                    WPRINT_APP_INFO(("\r\n\t Received firmware update request \r\n"));
                    WPRINT_APP_INFO(("\r\n\t Starting OTA server... \r\n\r\n"));

                    /**
                     *  Copies the new value requested by controller (value.new) to the current value
                     *  (memory pointed to by value.current)
                     **/
                    wiced_homekit_accept_controller_value( &firmware_upgrade_request_characteristic );

                    firmware_update_request = WICED_TRUE;
                }
                else
                {
                    WPRINT_APP_INFO(("\r\n\t No firmware update request received \r\n"));
                }
            }
        }
        else
        {
            WPRINT_APP_INFO(("\r\n\t No actions for accessory: %d\r\n", update_list->characteristic_descriptor[i].accessory_instance_id ));
        }
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

    /* Check if this is the firmware upgrade accessory */
    if ( firmware_upgrade_accessory.instance_id == accessory_id )
    {
        WPRINT_APP_INFO(("\r\n FIRMWARE UPGRADE ACCESSORY IDENTIFY \r\n"));

        /* Flash the LED representing the firmware upgrade */
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
