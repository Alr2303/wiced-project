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
 * HomeKit Air Quality Sensor Accessory Snippet
 *
 * This snippet demonstrates how to setup a Air Quality Sensor service
 *
 * The snippet has 2 build options:
 *    - MFi mode
 *    - Non-MFi mode
 *
 *     MFi Mode
 *        This uses WAC to start a soft AP and bring the Wiced HomeKit Air Quality Sensor Accessory onto the network
 *        It also advertises MFi support, and uses MFi certificate during the pairing process
 *        to prove it is an MFi device.
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.air_quality_sensor_service-BCM9WCDPLUS114 USE_MFI=1
 *
 *     Non-MFi Mode
 *        This uses the wifi credentials stored in wifi_config_dct.h to bring up the Wiced HomeKit Air Quality Sensor Accessory
 *        This advertises pin only support, and does not use MFi certificate during pairing
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.air_quality_sensor_service-BCM9WCDPLUS114
 *
 *
 * Once the Wiced HomeKit Air Quality Sensor Accessory is on the network, it can be discovered by the HAT application or iPhone/iPad app.
 * From here you should be able to perform a pair setup and perform a discovery of all the services
 * and characteristics.
 *
 * Features demonstrated
 *  - Adding of multiple services and characteristics
 *  - MFi/Non MFi pairing support
 *  - Read characteristics
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
 *      If running MFi mode, use the AirPort utility on the MAC (or  iPhone/iPad ), to bring up the Wiced HomeKit Air Quality Accessory
 *
 * HomeKit Accessory Demonstration
 *   To demonstrate the Wiced HomeKit Air Quality Sensor Accessory, start the HAT application once the Wiced HomeKit Air Quality Sensor Accessory has been added to
 *   your local network.
 *   - In the HAT application, click on the "Start Pairing" button, and wait for the "Accessory Password" to be shown
 *     on the serial output.
 *   - Enter the password shown on the display ( including the dashes )
 *   - Once the pairing is complete, click on the "Discover" button to perform a 'Pair-Verify' and get a listing of all
 *     services and characteristics for the accessory.
 *   - You can now read the required characteristic value which has read permissions
 *   - To add multiple controllers, perform the following steps:
 *     1. In the HAT application, click on the bottom right '+' sign and add another IP device
 *     2. For this controller, press "Start" to start listening for the Wiced HomeKit Air Quality Sensor Accessory
 *     3. Go to the paired admin controller, and select the newly created IP device, then select "Add Pairing"
 *     4. Once the pairing has been added, you can return to the new IP device, and begin discovery.
 *   - To demonstrate the air quality characteristic press SW1 button on evel board, it will display console massage
 *     and it will also send the event back to the controller
 *   - The modified characteristic values can be seen in the controller
 *
 * Notes.
 *   The HomeKit specification imposes some restrictions on the usage of the accessory, which may not be intuitive.
 *   Please keep these points in mind when exploring the Wiced HomeKit Air Quality Accessory snippet.
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

#define COMPARE_MATCH               ( 0 )
#define MAIN_ACCESSORY_NAME         "Wiced IP Air Quality Sensor-"
#define MAX_NAME_LENGTH             64
#define VALID_AIR_QUALITY_VALUES    6
#define MAC_OCTET4_OFFSET           ( 9 )
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

static wiced_result_t wiced_homekit_display_password( uint8_t* srp_pairing_password );
static wiced_result_t identify_action( uint16_t accessory_id, uint16_t characteristic_id );

/******************************************************
 *               Variable Definitions
 ******************************************************/
char accessory_name[MAX_NAME_LENGTH] = { MAIN_ACCESSORY_NAME };

char* airquality_values[ VALID_AIR_QUALITY_VALUES ] =
{
    "Unknown",
    "Excellent",
    "Good",
    "Fair",
    "Inferior",
    "Poor"
};

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
    .mdns_desired_hostname  = "homekit-air-quality-sensor-test",
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
    .accessory_category_identifier  = SENSOR

};


/* Accessory definitions */
wiced_homekit_accessories_t air_quality_sensor_accessory;

/* Accessory Information Service and Characteristics */
wiced_homekit_services_t          accessory_information_service;
wiced_homekit_characteristics_t   accessory_identify_characteristic;
wiced_homekit_characteristics_t   accessory_manufacturer_characteristic;
wiced_homekit_characteristics_t   accessory_model_characteristic;
wiced_homekit_characteristics_t   accessory_name_characteristic;
wiced_homekit_characteristics_t   accessory_serial_number_characteristic;

/* Air Quality services and characteristics */
wiced_homekit_services_t          air_quality_sensor_service;
wiced_homekit_characteristics_t   air_quality_characteristic;
wiced_homekit_characteristics_t   air_quality_name_characteristic;
wiced_homekit_characteristics_t   air_particulate_size_characteristic;
wiced_homekit_characteristics_t   air_particulate_density_characteristic;
wiced_homekit_characteristics_t   air_quality_status_active_characteristic;
wiced_homekit_characteristics_t   air_quality_status_fault_characteristic;
wiced_homekit_characteristics_t   air_quality_status_tampered_characteristic;
wiced_homekit_characteristics_t   air_quality_status_low_battery_characteristic;

/* Accessory information service characteristic values */
char accessory_name_value[MAX_NAME_LENGTH] = "\""MAIN_ACCESSORY_NAME"\"";
char accessory_manufacturer_value[]        = "\"Cypress\"";
char accessory_model_value[]               = "\"Controller 1.0\"";
char accessory_serial_number_value[]       = "\"12345678\"";

/* Air Quality characteristics values */
char air_quality_value[]                   = "0";
char air_quality_name_value[]              = "\"Air Quality Sensor\"";
char air_quality_particulate_size[]        = "0";
char air_quality_particulate_density[]     = "0";
char air_quality_status_active[]           = "0";
char air_quality_status_fault[]            = "0";
char air_quality_status_tampered[]         = "0";
char air_quality_status_low_battery[]      = "0";

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_bool_t    button1_pressed;
    wiced_result_t  result;
    uint32_t        wiced_heap_allocated_for_database;
    int             button_press_count;
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

        /* If we were assigned a new name, update with new name */
        if ( apple_wac_info.out_new_name != NULL )
        {
            snprintf( accessory_name, sizeof(accessory_name), "%s", apple_wac_info.out_new_name );
            snprintf( accessory_name_value, sizeof(accessory_name_value), "\"%s\"", apple_wac_info.out_new_name );
            free( apple_wac_info.out_new_name );
        }
    }

#endif

    /* Initialise Air Quality Accessory information service */
    wiced_homekit_initialise_accessory_information_service( &accessory_information_service, iid++ );

    /* Initialise Air Quality service */
    wiced_homekit_initialise_air_quality_sensor_service( &air_quality_sensor_service, iid++ );

    /******************************
     * Initialise characteristics
     ******************************/

    /* Initialize Accessory Information Service Characteristics */
    wiced_homekit_initialise_identify_characteristic        ( &accessory_identify_characteristic,      identify_action, iid++ );
    wiced_homekit_initialise_name_characteristic            ( &accessory_name_characteristic,          accessory_name_value,          strlen( accessory_name_value ),          NULL, NULL, iid++ );
    wiced_homekit_initialise_manufacturer_characteristic    ( &accessory_manufacturer_characteristic,  accessory_manufacturer_value,  strlen( accessory_manufacturer_value ),  NULL, NULL, iid++ );
    wiced_homekit_initialise_model_characteristic           ( &accessory_model_characteristic,         accessory_model_value,         strlen( accessory_model_value ),         NULL, NULL, iid++ );
    wiced_homekit_initialise_serial_number_characteristic   ( &accessory_serial_number_characteristic, accessory_serial_number_value, strlen( accessory_serial_number_value ), NULL, NULL, iid++ );

    /* Initialise Air Quality Sensor Service characteristics */
    wiced_homekit_initialise_air_quality_characteristic                  ( &air_quality_characteristic,                    air_quality_value,                strlen( air_quality_value ),           NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic                         ( &air_quality_name_characteristic,               air_quality_name_value,           strlen( air_quality_name_value ),          NULL, NULL, iid++ );
    wiced_homekit_initialise_air_particulate_size_characteristic         ( &air_particulate_size_characteristic,           air_quality_particulate_size,     strlen( air_quality_particulate_size ),    NULL, NULL, iid++ );
    wiced_homekit_initialise_air_particulate_density_characteristic      ( &air_particulate_density_characteristic,        air_quality_particulate_density,  strlen( air_quality_particulate_density ), NULL, NULL, iid++ );
    wiced_homekit_initialise_status_active_characteristic                ( &air_quality_status_active_characteristic,      air_quality_status_active,        strlen( air_quality_status_active ),       NULL, NULL, iid++ );
    wiced_homekit_initialise_status_fault_characteristic                 ( &air_quality_status_fault_characteristic,       air_quality_status_fault,         strlen( air_quality_status_fault ),        NULL, NULL, iid++ );
    wiced_homekit_initialise_status_tampered_characteristic              ( &air_quality_status_tampered_characteristic,    air_quality_status_tampered,      strlen( air_quality_status_tampered ),     NULL, NULL, iid++ );
    wiced_homekit_initialise_status_low_battery_characteristic           ( &air_quality_status_low_battery_characteristic, air_quality_status_low_battery,   strlen( air_quality_status_low_battery ),  NULL, NULL, iid++ );

    /* Add Air Quality Sensor Accessory*/
    wiced_homekit_add_accessory( &air_quality_sensor_accessory, 1 );

    /* Add Air Quality Sensor Accessory Information Service */
    wiced_homekit_add_service( &air_quality_sensor_accessory, &accessory_information_service );

    wiced_homekit_add_characteristic( &accessory_information_service, &accessory_identify_characteristic      );
    wiced_homekit_add_characteristic( &accessory_information_service, &accessory_manufacturer_characteristic  );
    wiced_homekit_add_characteristic( &accessory_information_service, &accessory_model_characteristic         );
    wiced_homekit_add_characteristic( &accessory_information_service, &accessory_name_characteristic          );
    wiced_homekit_add_characteristic( &accessory_information_service, &accessory_serial_number_characteristic );

    /* Add Air Quality Sensor service to accessory */
    wiced_homekit_add_service( &air_quality_sensor_accessory, &air_quality_sensor_service );

    /* Add the required characteristics to the Air Quality Sensor Service */
    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_quality_characteristic);
    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_quality_name_characteristic );
    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_particulate_size_characteristic);
    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_particulate_density_characteristic );
    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_quality_status_active_characteristic );
    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_quality_status_fault_characteristic );
    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_quality_status_tampered_characteristic );
    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_quality_status_low_battery_characteristic );

    /* Register a callback to receive Pair-Setup code to be displayed to the user */
    wiced_configure_accessory_password_for_device_with_display( wiced_homekit_display_password );

    /* Use the API below if your device has no display */
    /* wiced_configure_accessory_password_for_device_with_no_display( "888-77-888" ); */

    wiced_register_url_identify_callback( identify_action );

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

    button_press_count = 0;
    while( 1 )
    {
        button1_pressed = wiced_gpio_input_get( WICED_BUTTON1 ) ? WICED_FALSE : WICED_TRUE;  /* The button has inverse logic */
        if ( button1_pressed )
        {
            button_press_count++;
            button_press_count = button_press_count % VALID_AIR_QUALITY_VALUES;

            snprintf( air_quality_value, sizeof(air_quality_value), "%d", button_press_count );
            WPRINT_APP_INFO(("\n Air Quality value is [%s]\n", airquality_values[button_press_count]));

             /**
             * Update the current value length( value.current.value_length), and place this characteristic value
             * into a list of events to be sent to the controller (if controller enabled events for this characteristic)
             **/
            wiced_homekit_register_characteristic_value_update( &air_quality_sensor_accessory, &air_quality_characteristic, air_quality_value, strlen(air_quality_value) );
        }

        /**
         * Send send all characteristic values which have been updated and had event notifications enabled to controller
         * Send the list created by calling wiced_homekit_register_characteristic_value_update
         **/
        wiced_homekit_send_all_updates_for_accessory( );

        /* Based on HAP spec, all the notification should be coalesced using a delay of no less than 1 sec */
        wiced_rtos_delay_milliseconds( 1000 );
    }
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

    /* Check if this is the Air Quality Accessory */
    if ( air_quality_sensor_accessory.instance_id == accessory_id )
    {
        WPRINT_APP_INFO(("\r\n AIR QUALITY ACCESSORY IDENTIFY \r\n"));

        /* Flash the LED representing the Air Quality */
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