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
 * HomeKit Door Accessory Snippet
 *
 * This snippet demonstrates how to setup a Door service
 *
 * The snippet has 2 build options:
 *    - MFi mode
 *    - Non-MFi mode
 *
 *     MFi Mode
 *        This uses WAC to start a soft AP and bring the Wiced HomeKit Door Accessory onto the network
 *        It also advertises MFi support, and uses MFi certificate during the pairing process
 *        to prove it is an MFi device.
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.door_service-BCM9WCDPLUS114 USE_MFI=1
 *
 *     Non-MFi Mode
 *        This uses the wifi credentials stored in wifi_config_dct.h to bring up the Wiced HomeKit Door Accessory
 *        This advertises pin only support, and does not use MFi certificate during pairing
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.door_service-BCM9WCDPLUS114
 *
 *
 * Once the Wiced HomeKit Door Accessory is on the network, it can be discovered by the HAT application or iPhone/iPad app.
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
 *      If running MFi mode, use the AirPort utility on the MAC (or  iPhone/iPad ), to bring up the Wiced HomeKit Door Accessory
 *
 * HomeKit Accessory Demonstration
 *   To demonstrate the Wiced HomeKit Door Accessory, start the HAT application once the Wiced HomeKit Door Accessory has been added to
 *   your local network.
 *   - In the HAT application, click on the "Start Pairing" button, and wait for the "Accessory Password" to be shown
 *     on the serial output.
 *   - Enter the password shown on the display ( including the dashes )
 *   - Once the pairing is complete, click on the "Discover" button to perform a 'Pair-Verify' and get a listing of all
 *     services and characteristics for the accessory.
 *   - You can now modify a characteristic value which has read/write permissions
 *   - To add multiple controllers, perform the following steps:
 *     1. In the HAT application, click on the bottom right '+' sign and add another IP device
 *     2. For this controller, press "Start" to start listening for the Wiced HomeKit Door Accessory
 *     3. Go to the paired admin controller, and select the newly created IP device, then select "Add Pairing"
 *     4. Once the pairing has been added, you can return to the new IP device, and begin discovery.
 *   - To enable events, go to the 'on' characteristic and press the 'enable' button next to "Event Notifications"
 *   - To send an event back to the controller, change the Door current Position value by pressing SW1 button
 *     if accessory support more then one button then SW2 button can be use to change the value of Door position state characteristic
 *     and SW3 button can be use to change the value of Door Obstruction Detected Characteristic
 *
 * Notes.
 *   The HomeKit specification imposes some restrictions on the usage of the accessory, which may not be intuitive.
 *   Please keep these points in mind when exploring the Wiced HomeKit Door Accessory snippet.
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

/******************************************************
 *                    Constants
 ******************************************************/

#define COMPARE_MATCH                        ( 0 )
#define MAIN_ACCESSORY_NAME                  "Wiced IP Door Accessory-"
#define MAX_NAME_LENGTH                      ( 64 )
#define BUTTON_WORKER_STACK_SIZE             ( 1024 )
#define BUTTON_WORKER_QUEUE_SIZE             ( 4 )
#define DOOR_CURRENT_POSITION_KEY_CODE       ( 1 )
#define DOOR_POSITION_STATE_KEY_CODE         ( 2 )
#define DOOR_OBSTRUCTION_DETECTED_KEY_CODE   ( 3 )
#define VALID_MAX_CURRENT_POSITION_VALUE     ( 100 )
#define MAC_OCTET4_OFFSET                    ( 9 )

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
static void    door_accessory_button_handler( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static const wiced_button_manager_configuration_t button_manager_configuration =
{
    .short_hold_duration     = 500  * MILLISECONDS,
    .debounce_duration       = 100  * MILLISECONDS,

    .event_handler           = door_accessory_button_handler,
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
    .mdns_desired_hostname  = "homekit-door-test",
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
    .accessory_category_identifier  = DOOR

};


/* Accessory definitions */
wiced_homekit_accessories_t door_accessory;

/* Accessory Information Service and Characteristics */
wiced_homekit_services_t        door_information_service;
wiced_homekit_characteristics_t door_identify_characteristic;
wiced_homekit_characteristics_t door_manufacturer_characteristic;
wiced_homekit_characteristics_t door_model_characteristic;
wiced_homekit_characteristics_t door_name_characteristic;
wiced_homekit_characteristics_t door_serial_number_characteristic;

/* Door services and characteristics */
wiced_homekit_services_t        door_service;
wiced_homekit_characteristics_t door_current_position_characteristic;
wiced_homekit_characteristics_t door_target_position_characteristic;
wiced_homekit_characteristics_t door_position_state_characteristic;
wiced_homekit_characteristics_t door_hold_position_characteristic;
wiced_homekit_characteristics_t door_obstruction_detected_characteristic;

/* Accessory information service characteristic values */
char door_name_value[MAX_NAME_LENGTH] = "\""MAIN_ACCESSORY_NAME"\"";
char door_manufacturer_value[]        = "\"Door Accessories Co.\"";
char door_model_value[]               = "\"Controller 1.0\"";
char door_serial_number_value[]       = "\"12345678\"";

/* Door characteristics values */
char door_current_position_value[]       = "100";
char door_target_position_value[]        = "100";
char door_position_state_value[]         = "255";
char door_hold_position_value[]          = "false";
char door_obstruction_detected_value[]   = "false";

/* Static button configuration */
static const wiced_button_configuration_t button_configurations[] =
{
#if ( WICED_PLATFORM_BUTTON_COUNT > 0 )
    [ 0 ]  = { PLATFORM_BUTTON_1, BUTTON_CLICK_EVENT , DOOR_CURRENT_POSITION_KEY_CODE },
#endif
#if ( WICED_PLATFORM_BUTTON_COUNT > 1 )     /* If Platform supports more then one button */
    [ 1 ]  = { PLATFORM_BUTTON_2, BUTTON_CLICK_EVENT , DOOR_POSITION_STATE_KEY_CODE },
#endif
#if ( WICED_PLATFORM_BUTTON_COUNT > 2 )
    [ 2 ]  = { PLATFORM_BUTTON_3, BUTTON_CLICK_EVENT , DOOR_OBSTRUCTION_DETECTED_KEY_CODE },
#endif
};

/* Button objects for the button manager */
button_manager_button_t buttons[] =
{
#if ( WICED_PLATFORM_BUTTON_COUNT > 0 )
    [ 0 ]  = { &button_configurations[ 0 ] },
#endif
#if ( WICED_PLATFORM_BUTTON_COUNT > 1 )     /* If Platform supports more then one button */
    [ 1 ]  = { &button_configurations[ 1 ] },
#endif
#if ( WICED_PLATFORM_BUTTON_COUNT > 2 )
    [ 2 ]  = { &button_configurations[ 2 ] },
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
            snprintf( door_name_value, sizeof(door_name_value), "\"%s\"", apple_wac_info.out_new_name );
            free( apple_wac_info.out_new_name );
        }
    }
#endif

    /* Initialise Door Accessory information service */
    wiced_homekit_initialise_accessory_information_service( &door_information_service, iid++ );

    /* Initialise Door service */
    wiced_homekit_initialise_door_service( &door_service, iid++ );

    /******************************
     * Initialise characteristics
     ******************************/

    /* Door Information Service Characteristics */
    wiced_homekit_initialise_identify_characteristic     ( &door_identify_characteristic,      identify_action, iid++ );
    wiced_homekit_initialise_name_characteristic         ( &door_name_characteristic,          door_name_value,          strlen( door_name_value ),          NULL, NULL, iid++ );
    wiced_homekit_initialise_manufacturer_characteristic ( &door_manufacturer_characteristic,  door_manufacturer_value,  strlen( door_manufacturer_value ),  NULL, NULL, iid++ );
    wiced_homekit_initialise_model_characteristic        ( &door_model_characteristic,         door_model_value,         strlen( door_model_value ),         NULL, NULL, iid++ );
    wiced_homekit_initialise_serial_number_characteristic( &door_serial_number_characteristic, door_serial_number_value, strlen( door_serial_number_value ), NULL, NULL, iid++ );

    /* Initialise Door characteristics */
    wiced_homekit_initialise_current_position_characteristic        ( &door_current_position_characteristic,     door_current_position_value,     strlen( door_current_position_value ),     NULL, NULL, iid++ );
    wiced_homekit_initialise_target_position_characteristic         ( &door_target_position_characteristic,      door_target_position_value,      strlen( door_target_position_value ),      NULL, NULL, iid++ );
    wiced_homekit_initialise_position_state_characteristic          ( &door_position_state_characteristic,       door_position_state_value,       strlen( door_position_state_value ),       NULL, NULL, iid++ );
    wiced_homekit_initialise_hold_position_characteristic           ( &door_hold_position_characteristic,        door_hold_position_value,        strlen( door_hold_position_value ),        NULL, NULL, iid++ );
    wiced_homekit_initialise_obstruction_detected_characteristic    ( &door_obstruction_detected_characteristic, door_obstruction_detected_value, strlen( door_obstruction_detected_value ), NULL, NULL, iid++ );

    /* Add Door Accessory*/
    wiced_homekit_add_accessory( &door_accessory, 1 );

    /* Add Door Accessory Information Service */
    wiced_homekit_add_service( &door_accessory, &door_information_service );

    wiced_homekit_add_characteristic( &door_information_service, &door_identify_characteristic );
    wiced_homekit_add_characteristic( &door_information_service, &door_name_characteristic );
    wiced_homekit_add_characteristic( &door_information_service, &door_manufacturer_characteristic );
    wiced_homekit_add_characteristic( &door_information_service, &door_model_characteristic );
    wiced_homekit_add_characteristic( &door_information_service, &door_serial_number_characteristic );

    /* Add Door service to accessory */
    wiced_homekit_add_service( &door_accessory, &door_service );

    /* Add the required characteristics to the Door service */
    wiced_homekit_add_characteristic( &door_service, &door_current_position_characteristic );
    wiced_homekit_add_characteristic( &door_service, &door_target_position_characteristic );
    wiced_homekit_add_characteristic( &door_service, &door_position_state_characteristic );
    wiced_homekit_add_characteristic( &door_service, &door_hold_position_characteristic );
    wiced_homekit_add_characteristic( &door_service, &door_obstruction_detected_characteristic );

    wiced_configure_accessory_password_for_device_with_display( wiced_homekit_display_password );

    /* Use the API below if your device has no display */
    /* wiced_configure_accessory_password_for_device_with_no_display( "888-77-888" ); */

    wiced_register_url_identify_callback( identify_action );

    wiced_register_value_update_callback( wiced_homekit_actions_on_remote_value_update );

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
        wiced_homekit_send_all_updates_for_accessory( );

        /* Based on HAP spec, all the notification should be coalesced using a delay of no less than 1 sec */
        wiced_rtos_delay_milliseconds( 1000 );
    }
}

wiced_result_t wiced_homekit_actions_on_remote_value_update( wiced_homekit_characteristic_value_update_list_t* update_list )
{
    int     i;

    /* Cycle through list of all updated characteristics */
    for ( i = 0; i < update_list->number_of_updates; i++ )
    {
        /* If Door accessory, then check to see what services are being activated */
        if ( door_accessory.instance_id == update_list->characteristic_descriptor[ i ].accessory_instance_id )
        {
            WPRINT_APP_INFO(("\r\n DOOR ACCESSORY \r\n"));

            /* Receive door target position value from controller and place it into local buffer */
            if ( door_target_position_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                WPRINT_APP_INFO(("\r\n\t Received Door target position value is: %.*s. \r\n", door_target_position_characteristic.value.new.value_length, door_target_position_characteristic.value.new.value ));

                /* Copies the new value requested by controller (value.new) to the current value (memory pointed to by value.current) */
                wiced_homekit_accept_controller_value( &door_target_position_characteristic );


                WPRINT_APP_INFO(("\r\n\t Updated current door position value with requested value of %.*s.\r\n", door_target_position_characteristic.value.new.value_length, door_target_position_characteristic.value.new.value ));

                /**
                 * Update the current value length( value.current.value_length), and place this characteristic value
                 * into a list of events to be sent to the controller (if controller enabled events for this characteristic)
                 **/
                wiced_homekit_register_characteristic_value_update( &door_accessory, &door_target_position_characteristic, door_target_position_characteristic.value.new.value, door_target_position_characteristic.value.new.value_length );
            }
            else
            {
                WPRINT_APP_INFO(("\r\n\t No actions for characteristic: %d specified \r\n", update_list->characteristic_descriptor[i].characteristic_instance_id ));
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
    /* Check if this is the Door Accessory */
    if ( door_accessory.instance_id == accessory_id )
    {
        WPRINT_APP_INFO(("\r\n DOOR ACCESSORY IDENTIFY \r\n"));

        /* Flash the LED representing the Door accessory */
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
static void door_accessory_button_handler( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state )
{
    static uint8_t  button1_press_count = 0;
           uint8_t  value_length;
    if( event == BUTTON_CLICK_EVENT )
    {
        /* Change the Current Door Position Characteristic value by pressing the SW1 button. */
       if( button->configuration->application_event == DOOR_CURRENT_POSITION_KEY_CODE )
       {
           if ( button1_press_count > VALID_MAX_CURRENT_POSITION_VALUE )
           {
               button1_press_count = 0;
           }
           snprintf( door_current_position_value, sizeof(door_current_position_value), "%d", button1_press_count );
           WPRINT_APP_INFO(("\n Door Current position value is : %s.\n", door_current_position_value));
           value_length = ( button1_press_count == VALID_MAX_CURRENT_POSITION_VALUE ) ? 3 : 2;
           /**
            * Update the current value length( value.current.value_length), and place this characteristic value
            * into a list of events to be sent to the controller (if controller enabled events for this characteristic)
            **/
           wiced_homekit_register_characteristic_value_update( &door_accessory, &door_current_position_characteristic, door_current_position_value, value_length );
           button1_press_count = button1_press_count + 10;
           return;
       }

       /* If Platform supports more the one button then SW2 button can be use to change the value of Door Position State characteristic. */
       if( button->configuration->application_event == DOOR_POSITION_STATE_KEY_CODE )
       {
           char value = *door_position_state_characteristic.value.current.value;
           switch( value )
           {
               case '0':
                   WPRINT_APP_INFO(("\n Door is going to the maximum value specified in metadata.\n"));
                   value = '1';
                   break;

               case '1':
                   WPRINT_APP_INFO(("\n Door is Stopped.\n"));
                   value = '2';
                   break;

               case '2':
                   WPRINT_APP_INFO(("\n Door is going to the minimum value specified in metadata.\n"));
                   value = '0';
                   break;
           }
            /**
            * Update the current value length( value.current.value_length), and place this characteristic value
            * into a list of events to be sent to the controller (if controller enabled events for this characteristic)
            **/
           wiced_homekit_register_characteristic_value_update( &door_accessory, &door_position_state_characteristic, &value, 1);
           return;
       }
       /* If Platform supports more the one button then SW3 button can be use to change the value of Obstruction Detected characteristic.*/
       else if( button->configuration->application_event == DOOR_OBSTRUCTION_DETECTED_KEY_CODE )
       {
           char value = *door_obstruction_detected_characteristic.value.current.value;
           if ( value == '0' || value == 'f' )
           {
               WPRINT_APP_INFO(("\n Obstruction is Detected in Door service.\n"));
               value = '1';
           }
           else
           {
               WPRINT_APP_INFO(("\n Obstruction is Removed in Door service.\n"));
               value = '0';
           }
           /**
            * Update the current value length( value.current.value_length), and place this characteristic value
            * into a list of events to be sent to the controller (if controller enabled events for this characteristic)
            **/
           wiced_homekit_register_characteristic_value_update( &door_accessory, &door_obstruction_detected_characteristic, &value, 1);
           return;
       }
    }
    return;
}
