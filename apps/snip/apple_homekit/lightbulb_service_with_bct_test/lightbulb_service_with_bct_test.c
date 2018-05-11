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
 * HomeKit Lightbulb Accessory + Bonjour conformance test ( BCT ) Snippet
 *
 * This snippet by default run as Lightbulb Accessory application and can perform BCT test also if required.
 *
 * The snippet has 2 build options:
 *    - MFi mode
 *    - Non-MFi mode
 *
 *     MFi Mode
 *        This uses WAC to start a soft AP and bring the Wiced HomeKit Lightbulb Accessory + BCT Test onto the network
 *        It also advertises MFi support, and uses MFi certificate during the pairing process to prove it is an MFi device.
 *        Build string in this mode:
 *
 *        snip.apple_homekit.lightbulb_service_with_bct_tse-BCM9WCDPLUS114 USE_MFI=1 download
 *
 *     Non-MFi Mode
 *        This uses the wifi credentials stored in wifi_config_dct.h to bring up the Wiced HomeKit Lightbulb Accessory + BCT Test
 *        This advertises pin only support, and does not use MFi certificate during pairing
 *        Build string in this mode:
 *
 *        snip.apple_homekit.lightbulb_service_with_bct_test-BCM9WCDPLUS114 download
 *
 *
 * Once the Wiced HomeKit Lightbulb Accessory is on the network, it can be discovered by the HAT application or iPhone/iPad app.
 * From here you should be able to perform a pair setup and perform a discovery of all the services
 * and characteristics and also able to perform Bonjour conformance test ( BCT ).
 *
 * * Application Instructions
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
 *      If running MFi mode, use the AirPort utility on the MacBook (or  iPhone/iPad ), to connect the AP.
 *
 * HomeKit Accessory Demonstration
 *   To demonstrate the Wiced HomeKit Lightbulb Accessory, start the HAT application once the Wiced HomeKit Lightbulb Accessory has been added to
 *   your local network.
 *   1. In the HAT application, click on the "Start Pairing" button, and wait for the "Accessory Password" to be shown
 *      on the serial output.
 *
 *   2. Enter the password shown on the display ( including the dashes )
 *
 *   3. Once the pairing is complete, click on the "Discover" button to perform a 'Pair-Verify' and get a listing of all
 *      services and characteristics for the accessory.
 *
 *   4. You can now modify a characteristic value which has read/write permissions
 *
 *   5. To add multiple controllers, perform the following steps:
 *      - In the HAT application, click on the bottom right '+' sign and add another IP device
 *      - For this controller, press "Start" to start listening for the Wiced HomeKit Lightbulb Accessory
 *      - Go to the paired admin controller, and select the newly created IP device, then select "Add Pairing"
 *      - Once the pairing has been added, you can return to the new IP device, and begin discovery.
 *
 *   6. To enable events, go to the 'on' characteristic and press the 'enable' button next to "Event Notifications"
 *
 *   7. To send an event back to the controller, toggle D1 LED on the eval board by pressing SW1. This should send
 *      back either an on or off event
 *
 * How to run bonjour conformance test with this application
 *
 *   1. BCT test can be start after successful connection to the AP. For more information on how to setup a BCT
 *      isolated network please refer to the latest version of "Bonjour Conformance Test for HomeKit" document provided by Apple.
 *
 *   2. Open the terminal on Macbook in super user mode and Start BonjourConformanceTest executable.
 *
 *   3. Follow the instructions of the BonjourConformanceTest application in MacBook
 *      a) you must read the license terms and type yes if you are agree with them
 *      b) you must enter a valid IP address of the access point to which mac PC and wiced
 *      board are connected to. Please refer to [1] section 4 "Setup" for more information
 *
 *   4. MacBook console will print "starting: INITIAL PROBING". At this point you should type "start_bct_test" command on Wiced-board.
 *      And wait for ">" prompt on Wiced-board.
 *
 *   5. Issue "hot_unplug_plug" command on Wiced-board and wait until Wiced-board joins the network.
 *      Wait for further instructions displayed on MacBook and follow those instructions.
 *
 *   6. As soon as it asks to unplug the device network cable or disassociate from the AP issue a command : "hot_unplug_plug" again.
 *
 *   7. Let MDNS functionality testing running as soon as MacBook asks you to manually change the service name issue a command : "change_name".
 *
 *   8. wait until HOT-PLUGGING test is in action type command: "hot_unplug_plug" on a serial terminal of Wiced-board.
 *
 *   9. If BonjourConformanceTest starts LINK-LOCAL TO ROUTABLE COMMUNICATION test you will be asked to enter link-local
 *      IP address of the wiced_board. Use the most recently assigned address which can be found on the output of the serial console.
 *      For Example: "IPv4  AUTO_IP network ready IP: 169.254.203.142"
 *
 *  10. Then you will be prompted to set MacBook IP address to 17.2.3.4 and subnetmask to 255.255.255.0 manually.
 *
 *  11. In the next test will ask to set the Mac PC test machine to the link-local address. Use Network utility to assign link local
 *      IP address to (169.254/16) and subnet mask to 255.255.0.0. Press enter when complete.
 *
 *  12. Issue a command "set_routable" to a wiced-board via serial terminal to assign routable IP address : 17.1.1.1
 *
 *  13. Again you will be ask to change routable to link-local IP issue command "hot_unplug_plug"
 *
 *  14. Wait until a link-local IP address is assigned. The assigned IP address can be found on the output of the serial console.
 *      Use the assigned IP address for CHATTINESS test. CHATTINESS test may take more than 4 hours to complete.
 *
 *  15. To run BCT test for IPv6, type below command on MacBook:
 *      /.BonjourConformanceTest <interface name> -6 -M
 *      NOTE: 1) IPv6 test is currently supported only for MulticastDNS test cases
 *            2) After completion of the IPv6 mDNS test, you may see summary of the test result as "failed".
 *               You can ignore that message, if all the individual mDNS test cases PASSED.
 *
 *  16. If you want to go back to the Lightbulb accessory application use "stop_bct_test" command at any point.
 *
 * Notes.
 *   The HomeKit specification imposes some restrictions on the usage of the accessory.
 *   Please keep these points in mind when exploring the HomeKit Lightbulb Accessory + Bonjour conformance test ( BCT ) Snippet.
 *
 *   1. By default Lightbulb application will run but either the Lightbulb application or bct test can run at a time,
 *      when you start bct test HomeKit accessory automatically stop.
 *
 *   2. To resume the HomeKit accessory issue the command "stop_bct_test" on wiced terminal.
 *      For more understanding of the BCT test procedure one can refer the instruction given in bonjour_conformance_test.c file.
 *
 *   3. Only controllers which are admin controllers can perform add/remove pairings ( the first controller to pair automatically
 *      becomes an admin controller )
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
#include "command_console.h"
#include "gedday.h"
#include "http_server.h"
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

#define COMPARE_MATCH                       ( 0 )
#define MAIN_ACCESSORY_NAME                 "Wiced IP Lightbulb With BCT-"
#define MAX_NAME_LENGTH                     ( 64 )
#define BUTTON_WORKER_STACK_SIZE            ( 1024 )
#define BUTTON_WORKER_QUEUE_SIZE            ( 4 )
#define LIGHTBULB_ON_KEY_CODE               ( 1 )
#define MAC_OCTET4_OFFSET                   ( 9 )

#define TEST_COMMAND_LENGTH                 ( 50 )
#define TEST_COMMAND_HISTORY_LENGTH         ( 10 )
#define INITIAL_TEST_HOSTNAME               "WicedBoard"
#define INITIAL_TEST_SERVICE_NAME           "TestService"
#define TEST_SERVICE_TYPE                   "_http._tcp.local"
#define TEST_SERVICE_PORT                   ( 80 )
#define TEST_REQUIRED_NEW_SERVICE_NAME      "New - Bonjour Service Name"

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
static void    lightbulb_control_button_handler( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state );

static int start_bct_test_handler                   ( int argc, char *argv[] );
static int stop_bct_test_handler                    ( int argc, char *argv[] );
static int change_service_name_handler              ( int argc, char *argv[] );
static int hot_unplug_plug_handler                  ( int argc, char *argv[] );
static int routable_address_start_handler           ( int argc, char* argv[] );
static wiced_result_t bonjour_test_start_gedday     ( void );
static wiced_result_t bonjour_test_add_test_service ( const char* name );
static void bct_test_network_reset                  ( wiced_network_config_t, const wiced_ip_setting_t* );

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
    .mdns_desired_hostname  = "homekit-lightbulb-bct-test",
    .mdns_nice_name         = MAIN_ACCESSORY_NAME,
    .device_random_id       = "XX:XX:XX:XX:XX:XX",
};
#endif

apple_homekit_accessory_config_t apple_homekit_discovery_settings =
{
#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI
    .mfi_config_features            = MFI_CONFIG_FEATURES_SUPPORTS_MFI_AND_PIN,
#else
    .mfi_config_features            = MFI_CONFIG_FEATURES_SUPPORTS_PIN_ONLY,
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

/* Lightublb services and characteristics */
wiced_homekit_services_t        lightbulb_service;
wiced_homekit_characteristics_t lightbulb_on_characteristic;
wiced_homekit_characteristics_t lightbulb_hue_characteristic;
wiced_homekit_characteristics_t lightbulb_brightness_characteristic;
wiced_homekit_characteristics_t lightbulb_saturation_characteristic;
wiced_homekit_characteristics_t lightbulb_name_characteristic;

/* Accessory information service characteristic values */
char lightbulb_accessory_name_value[MAX_NAME_LENGTH] = "\""MAIN_ACCESSORY_NAME"\"";
char lightbulb_accessory_manufacturer_value[]        = "\"Lights Co.\"";
char lightbulb_accessory_model_value[]               = "\"Controller 1.0\"";
char lightbulb_accessory_serial_number_value[]       = "\"12345678\"";

/* Lightbulb characteristics values */
char lightbulb_name_value[]        = "\"LED lightbulb \"";
char lightbulb_on_value[]          = "false";
char lightbulb_hue_value[]         = "360";
char lightbulb_brightness_value[]  = "100";
char lightbulb_saturation_value[]  = "100";

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

static char text_record_buffer         [ 10 ];
static char test_command_buffer        [ TEST_COMMAND_LENGTH ];
static char test_command_history_buffer[ TEST_COMMAND_LENGTH * TEST_COMMAND_HISTORY_LENGTH ];

static const wiced_ip_setting_t routable_static_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS(17,1, 1, 1) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS(255,255,255,  0) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS(17,1,  1,  1) ),
};

static gedday_text_record_t  text_record;
static wiced_http_server_t   http_server;
static uint32_t              wiced_heap_allocated_for_database;
static wiced_worker_thread_t button_worker_thread;
static button_manager_t      button_manager;


const command_t test_command_table[ ] =
{
    DEFAULT_0_ARGUMENT_COMMAND_ENTRY( start_bct_test,  start_bct_test_handler )
    DEFAULT_0_ARGUMENT_COMMAND_ENTRY( stop_bct_test,   stop_bct_test_handler )
    DEFAULT_0_ARGUMENT_COMMAND_ENTRY( change_name,     change_service_name_handler )
    DEFAULT_0_ARGUMENT_COMMAND_ENTRY( hot_unplug_plug, hot_unplug_plug_handler )
    DEFAULT_0_ARGUMENT_COMMAND_ENTRY( set_routable,    routable_address_start_handler )
    COMMAND_TABLE_ENDING()
};


/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_result_t    result;
    wiced_mac_t       mac;
    char              mac_string[STRING_MAX_MAC];
    uint8_t           offset;
    uint8_t           mac_offset;
    uint16_t          iid = 1;

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
            snprintf( lightbulb_accessory_name_value, sizeof(lightbulb_accessory_name_value), "\"%s\"", apple_wac_info.out_new_name );
            free( apple_wac_info.out_new_name );
        }
    }

#endif

    /* Initialise Lightbulb Accessory information service */
    wiced_homekit_initialise_accessory_information_service( &lightbulb_accessory_information_service, iid++ );

    /* Initialise lightbulb service */
    wiced_homekit_initialise_lightbulb_service( &lightbulb_service, iid++ );

    /******************************
     * Initialise characteristics
     ******************************/

    /* Lightbulb Information Service Characteristics */
    wiced_homekit_initialise_identify_characteristic     ( &lightbulb_accessory_identify_characteristic,      identify_action, iid++ );
    wiced_homekit_initialise_name_characteristic         ( &lightbulb_accessory_name_characteristic,          lightbulb_accessory_name_value,          strlen( lightbulb_accessory_name_value ),          NULL, NULL, iid++ );
    wiced_homekit_initialise_manufacturer_characteristic ( &lightbulb_accessory_manufacturer_characteristic,  lightbulb_accessory_manufacturer_value,  strlen( lightbulb_accessory_manufacturer_value ),  NULL, NULL, iid++ );
    wiced_homekit_initialise_model_characteristic        ( &lightbulb_accessory_model_characteristic,         lightbulb_accessory_model_value,         strlen( lightbulb_accessory_model_value ),         NULL, NULL, iid++ );
    wiced_homekit_initialise_serial_number_characteristic( &lightbulb_accessory_serial_number_characteristic, lightbulb_accessory_serial_number_value, strlen( lightbulb_accessory_serial_number_value ), NULL, NULL, iid++ );

    /* Initialise Lightbulb characteristics */
    wiced_homekit_initialise_on_characteristic           ( &lightbulb_on_characteristic,         lightbulb_on_value,         strlen( lightbulb_on_value ),         NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic         ( &lightbulb_name_characteristic,       lightbulb_name_value,       strlen( lightbulb_name_value ),       NULL, NULL, iid++ );
    wiced_homekit_initialise_hue_characteristic          ( &lightbulb_hue_characteristic,        lightbulb_hue_value,        strlen( lightbulb_hue_value ),        NULL, NULL, iid++ );
    wiced_homekit_initialise_brightness_characteristic   ( &lightbulb_brightness_characteristic, lightbulb_brightness_value, strlen( lightbulb_brightness_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_saturation_characteristic   ( &lightbulb_saturation_characteristic, lightbulb_saturation_value, strlen( lightbulb_saturation_value ), NULL, NULL, iid++ );

    /* Add Lightbulb Accessory*/
    wiced_homekit_add_accessory( &lightbulb_accessory, 1 );

    /* Add Lightbulb Accessory Information Service */
    wiced_homekit_add_service( &lightbulb_accessory, &lightbulb_accessory_information_service );

    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_identify_characteristic      );
    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_manufacturer_characteristic  );
    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_model_characteristic         );
    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_name_characteristic          );
    wiced_homekit_add_characteristic( &lightbulb_accessory_information_service, &lightbulb_accessory_serial_number_characteristic );

    /* Add Lightbulb service to accessory */
    wiced_homekit_add_service( &lightbulb_accessory, &lightbulb_service );

    /* Add the required characteristics to the Lightbulb service */
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_on_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_hue_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_brightness_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_saturation_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_name_characteristic );

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

    command_console_init( STDIO_UART, sizeof(test_command_buffer), test_command_buffer, TEST_COMMAND_HISTORY_LENGTH, test_command_history_buffer, " ");
    console_add_cmd_table( test_command_table );

    while ( 1 )
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
    uint8_t light_characteristic[10];
    wiced_homekit_characteristics_t* updated_characteristic = NULL;

    /* Cycle through list of all updated characteristics */
    for ( i = 0; i < update_list->number_of_updates; i++ )
    {
        updated_characteristic = NULL;

        /* If Lightbulb Accessory, then check to see what services are being activated */
        if ( lightbulb_accessory.instance_id == update_list->characteristic_descriptor[ i ].accessory_instance_id )
        {
            WPRINT_APP_INFO(("\r\n LIGHTBULB ACCESSORY \r\n"));

            if ( lightbulb_on_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                if ( ( lightbulb_on_characteristic.value.new.value[ 0 ] == '1' ) ||
                     ( lightbulb_on_characteristic.value.new.value[0] == 't' ) )
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
            }
            else if ( lightbulb_hue_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                updated_characteristic = &lightbulb_hue_characteristic;

                memcpy( light_characteristic, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                light_characteristic[updated_characteristic->value.new.value_length] = '\0';

                WPRINT_APP_INFO(("\r\n\t Received hue value: %s \r\n", light_characteristic ));
            }
            else if ( lightbulb_brightness_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                updated_characteristic = &lightbulb_brightness_characteristic;

                memcpy( light_characteristic, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                light_characteristic[updated_characteristic->value.new.value_length] = '\0';

                WPRINT_APP_INFO(("\r\n\t Received brightness value: %s \r\n", light_characteristic ));
            }
            else if ( lightbulb_saturation_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                updated_characteristic = &lightbulb_saturation_characteristic;

                memcpy( light_characteristic, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                light_characteristic[updated_characteristic->value.new.value_length] = '\0';

                WPRINT_APP_INFO(("\r\n\t Received saturation value: %s \r\n", light_characteristic ));
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

static wiced_result_t bonjour_test_add_test_service( const char* service_instance_name )
{
    wiced_result_t result;
    gedday_text_record_create(&text_record, sizeof(text_record_buffer), text_record_buffer );
    gedday_text_record_set_key_value_pair( &text_record, "a", "1");

    do
    {
        result = gedday_add_service( service_instance_name , TEST_SERVICE_TYPE, TEST_SERVICE_PORT, 300, (const char*)gedday_text_record_get_string( &text_record ) );
        if ( result != WICED_SUCCESS )
        {
            wiced_rtos_delay_milliseconds( 1000 );
        }
    }
    while ( result != WICED_SUCCESS );
    return WICED_SUCCESS;
}

static wiced_result_t bonjour_test_start_gedday( void )
{
    wiced_result_t result;
    /* Init gedday and add test service */
    WPRINT_APP_INFO(("Initializing gedday...\r\n"));
    do
    {
        WPRINT_APP_INFO(("Init gedday\r\n"));
        result = gedday_init( apple_homekit_discovery_settings.interface, INITIAL_TEST_HOSTNAME );
        if ( result != WICED_SUCCESS )
        {
            wiced_rtos_delay_milliseconds( 1000 );
        }
    }
    while ( result != WICED_SUCCESS );

    WPRINT_APP_INFO(("Registering a testing MDNS service...\r\n"));
    /* Add text record to the test service a=1 */
    bonjour_test_add_test_service( INITIAL_TEST_SERVICE_NAME );
    return WICED_SUCCESS;
}

static void bct_test_network_reset ( wiced_network_config_t config, const wiced_ip_setting_t* ip_settings )
{
    gedday_text_record_delete( &text_record );
    if ( gedday_remove_service( INITIAL_TEST_SERVICE_NAME, TEST_SERVICE_TYPE ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(( "Gedday service not removed properly..\r\n" ));
    }

    /* Deinitialize current instance of gedday, then stop http server and then bring the network down */
    WPRINT_APP_INFO(("Deinitializing gedday...\r\n"));
    gedday_deinit();
    WPRINT_APP_INFO(("Stopping HTTP server...\r\n"));
    wiced_http_server_stop( &http_server );
    WPRINT_APP_INFO(("Bringing network down!\r\n"));
    wiced_network_down( apple_homekit_discovery_settings.interface );

    /* Bring up the network again and start gedday and HTTP server. */
    WPRINT_APP_INFO(("Bringing network up!\r\n"));
    wiced_network_up( apple_homekit_discovery_settings.interface, config, ip_settings );
    bonjour_test_start_gedday();
    WPRINT_APP_INFO(("Starting test HTTP server...\r\n"));
    wiced_http_server_start( &http_server, TEST_SERVICE_PORT, 1, NULL, apple_homekit_discovery_settings.interface, DEFAULT_URL_PROCESSOR_STACK_SIZE );
}

static int routable_address_start_handler( int argc, char* argv[] )
{
    bct_test_network_reset( WICED_USE_STATIC_IP, &routable_static_ip_settings );
    WPRINT_APP_INFO(("Ready for a new command\r\n"));
    return 0;
}

static int hot_unplug_plug_handler( int argc, char *argv[])
{
    bct_test_network_reset( WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    WPRINT_APP_INFO(("Ready for a new command\r\n"));
    return 0;
}

static int change_service_name_handler ( int argc, char *argv[] )
{
    gedday_text_record_delete( &text_record );
    if ( gedday_remove_service( INITIAL_TEST_SERVICE_NAME, TEST_SERVICE_TYPE ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(( "Gedday service not removed properly..\r\n" ));
    }

    /* Add text record to the test service a=1 */
    bonjour_test_add_test_service( TEST_REQUIRED_NEW_SERVICE_NAME );
    return 0;
}

static int start_bct_test_handler ( int argc, char *argv[] )
{
    /* Stop HomeKit accessory when BCT test Start */
    if ( wiced_homekit_stop() != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Failed to stop HomeKit Lightbulb accessory.\r\n"));
        return 0;
    }
    bonjour_test_start_gedday();
    wiced_http_server_start( &http_server, 80, 1, NULL, apple_homekit_discovery_settings.interface, DEFAULT_URL_PROCESSOR_STACK_SIZE );
    return 0;
}

static int stop_bct_test_handler ( int argc, char *argv[] )
{
    gedday_text_record_delete( &text_record );
    if ( gedday_remove_service( INITIAL_TEST_SERVICE_NAME, TEST_SERVICE_TYPE ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(( "Gedday service not removed properly..\r\n" ));
    }

    /* Deinitialize current instance of gedday, then stop http server and then bring the network down */
    WPRINT_APP_INFO(("Deinitializing gedday...\r\n"));
    gedday_deinit();
    WPRINT_APP_INFO(("Stopping HTTP server...\r\n"));
    wiced_http_server_stop( &http_server );

    /* Starting the HomeKit accessory again */
    if ( wiced_homekit_start( &apple_homekit_discovery_settings, &wiced_heap_allocated_for_database ) != WICED_SUCCESS )
    {
        return 0;
    }

    WPRINT_APP_INFO(("+-------------------+\r\n"));
    WPRINT_APP_INFO(("+ HomeKit Ready!    +\r\n"));
    WPRINT_APP_INFO(("+-------------------+\r\n"));
    return 0;
}
