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
 * Bonjour conformance test ( BCT ) application
 *
 * How to run bonjour conformance test with Wiced:
 *
 * 1. Change the AP credentials in default_wifi_config_dct.h. or use WAC to configure (add USE_MFI=1 in your make target)
 *    Both Wiced-Board and mac PC must be connected to the same AP during the BCT.
 *    For more information on how to setup a BCT isolated network
 *    please refer to the latest version of "Bonjour Conformance Test for HomeKit" document provided by Apple.
 *
 * 2. Build the application for your platform:
 *    make test.bonjour_conformance_test-YOUR_PLATFORM_NAME download
 *
 * 3. Open the terminal on Macbook in super user mode and Start BonjourConformanceTest executable.
 *
 * 4. Follow the instructions of the BonjourConformanceTest application
 *    a) you must read the license terms and type yes if you are agree with them
 *    b) you must enter a valid IP address of the access point to which mac PC and wiced
 *    board are connected to. Please refer to [1] section 4 "Setup" for more information
 *
 * 5. Upon successful completion of the previous step BonjourConformanceTest application will
 *    print "starting: INITIAL PROBING". At this point you should reset a Wiced-board and wait
 *    until it joins the AP and assigns Link-local IP address.
 *
 *    Please make sure you have connected board to a serial console such as putty or terra term
 *    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *    Please double check that it actually prints the following messages on a serial console:
 *    Unable to obtain IP address via DHCP. Perform AUTO_IP
 *    IPv4  AUTO_IP network ready IP
 *
 * 6. Follow the instructions from the BonjourConformanceTest application
 *    As soon as it asks to unplug the device network cable or disassociate from the AP issue
 *    a command from the serial terminal: "hot_unplug_plug".
 *    Wait until Wiced board joins the network and assigns a link-local IP address
 *
 *    After a link local IP address is assigned the BonjourConformanceAppplication will move on to
 *    the next category of BCT : MDNS functionality testing.
 *    The last sub-test in this category is REPLY AGGREGATION. After this test is done the
 *    BonjourConformanceTest application will ask a user to press enter in order to continue
 *    the test.
 *
 * 7. When BonjourConformanceTest application asks you to manually change the service name
 *
 *    This message output will look as follows:
 *
 *    Please manually change the service name
 *    "TestService-1._http._tcp.local." to "New - Bonjour Service
 *    Name._http._tcp.local."
 *
 *    issue a command on a serial terminal on wiced board : change_name
 *    After a name of the service is changed successfully BonjourConformanceTest application
 *    should print: PASSED( MANUAL NAME CHANGE ). Press enter and wait until HOT-PLUGGING test
 *    is in action.
 *
 * 8. On a serial terminal of Wiced-board type command: "hot_unplug_plug"
 *    Wait until Hot-plugging tests are finished.
 *
 * 9. If BonjourConformanceTest starts, LINK-LOCAL TO ROUTABLE COMMUNICATION test
 *    you will be asked to enter link-local IP address of the wiced_board. Use the
 *    most recently assigned address which can be found on the output of the serial console.
 *    For Example: "IPv4  AUTO_IP network ready IP: 169.254.203.142"
 *
 *    Then you will be prompted to set MacBook IP address to 17.2.3.4 and subnetmask to 255.255.255.0 manually.
 *    Start a new terminal app in Mac PC and enter command "ifconfig <Interface Name> inet". This is to confirm
 *    if IP address change is applied. Then press Enter to continue in the test terminal.
 *
 * 10.If BonjourConformanceTest starts, ROUTABLE TO LINK-LOCAL COMMUNICATION test, BonjourConformanceTest application
 *    will ask to set the Mac PC test machine to the link-local address.Use Network utility to
 *    to assign link local IP address to (169.254/16) and subnet mask to 255.255.0.0. Press enter when complete.
 *    Issue a command "set_routable" to a wiced-board via serial terminal to assign routable IP address:
 *    Device will disassociate from the network and then join it again and will assign new IP 17.1.1.1
 *
 * 11.In the last test: CHATTINESS you will be asked to manually change the IP address of the
 *    Wiced board from routable to link-local. This can be done by issuing a command
 *    "hot_unplug_plug" on a serial terminal. Wait until a link-local IP address is assigned.
 *    The assigned IP address can be found on the output of the serial console.
 *    Use the assigned IP address for CHATTINESS test.
 *
 * 12.Wait until CHATTINESS test is in progress. It may take more than 4 hours.
 *    At the end of the test specify a file name where the test results will be saved.
 *    Enjoy the results.
 *
 * 13.To run BCT test for IPv6, type below command on MacBook:
 *    /.BonjourConformanceTest <interface name> -6 -M
 *    NOTE: 1) IPv6 test is currently supported only for MulticastDNS test cases
 *          2) After completion of the IPv6 mDNS test, you may see summary of the test result as "failed".
 *             You can ignore that message, if all the individual mDNS test cases PASSED.
 */

#include "wiced.h"
#include "command_console.h"
#include "gedday.h"
#include "http_server.h"
#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI
#include "apple_wac.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define TEST_COMMAND_LENGTH             (50)
#define TEST_COMMAND_HISTORY_LENGTH     (10)
#define INITIAL_TEST_HOSTNAME           "WicedBoard"
#define INITIAL_TEST_SERVICE_NAME       "TestService"
#define TEST_SERVICE_TYPE               "_http._tcp.local"
#define TEST_SERVICE_PORT               (80)
#define TEST_REQUIRED_NEW_SERVICE_NAME  "New - Bonjour Service Name"
#define MAIN_ACCESSORY_NAME             "Wiced BCT-"
#define MAC_OCTET4_OFFSET                (9)
#define STR_MAX_MAC                      (18)
#define HEX_DIGITS_UPPERCASE             "0123456789ABCDEF"
#define MAX_NAME_LENGTH                     ( 64 )
#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI
#include "apple_wac.h"
#endif

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
static int change_service_name_handler              ( int argc, char *argv[] );
static int hot_unplug_plug_handler                  ( int argc, char *argv[] );
static int routable_address_start_handler           ( int argc, char* argv[] );
static wiced_result_t bonjour_test_start_gedday     ( void );
static wiced_result_t bonjour_test_add_test_service ( const char* name );



/******************************************************
 *               Variables Definitions
 ******************************************************/
static char text_record_buffer         [ 10 ];
static char test_command_buffer        [ TEST_COMMAND_LENGTH ];
static char test_command_history_buffer[ TEST_COMMAND_LENGTH * TEST_COMMAND_HISTORY_LENGTH ];

static const wiced_ip_setting_t routable_static_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS(17,1, 1, 1) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS(255,255,255,  0) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS(17,1,  1,  1) ),
};

static gedday_text_record_t text_record;
static wiced_http_server_t  http_server;

const command_t test_command_table[ ] =
{
    DEFAULT_0_ARGUMENT_COMMAND_ENTRY(change_name,     change_service_name_handler )
    DEFAULT_0_ARGUMENT_COMMAND_ENTRY(hot_unplug_plug, hot_unplug_plug_handler )
    DEFAULT_0_ARGUMENT_COMMAND_ENTRY(set_routable,    routable_address_start_handler )
    COMMAND_TABLE_ENDING()
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
    .device_random_id       = "AA:BB:CC:11:22:AA",
};

#endif



/******************************************************
 *               Function Definitions
 ******************************************************/

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
        result = gedday_init( WICED_STA_INTERFACE, INITIAL_TEST_HOSTNAME );
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
    wiced_result_t result;
    UNUSED_PARAMETER(result);

    gedday_text_record_delete( &text_record );
    gedday_remove_service( INITIAL_TEST_SERVICE_NAME, TEST_SERVICE_TYPE );

    /* Deinitialize current instance of gedday, then stop http server and then bring the network down */
    WPRINT_APP_INFO(("Deinitializing gedday...\r\n"));
    gedday_deinit();
    WPRINT_APP_INFO(("Stopping HTTP server...\r\n"));
    wiced_http_server_stop( &http_server );
    WPRINT_APP_INFO(("Bringing network down!\r\n"));
    wiced_network_down( WICED_STA_INTERFACE );

    WPRINT_APP_INFO(("Bringing network up!\r\n"));
    result = wiced_network_up( WICED_STA_INTERFACE, config, ip_settings );
    bonjour_test_start_gedday();
    WPRINT_APP_INFO(("Starting test HTTP server...\r\n"));
    wiced_http_server_start( &http_server, TEST_SERVICE_PORT, 1, NULL, WICED_STA_INTERFACE, DEFAULT_URL_PROCESSOR_STACK_SIZE );

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
    gedday_remove_service( INITIAL_TEST_SERVICE_NAME, TEST_SERVICE_TYPE );

    /* Add text record to the test service a=1 */
    bonjour_test_add_test_service( TEST_REQUIRED_NEW_SERVICE_NAME );
    WPRINT_APP_INFO(("Ready for a command\r\n"));
    return 0;
}

char* mac_addr_to_string( const void *mac_address, char *mac_address_string_format )
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
        *dst++ = HEX_DIGITS_UPPERCASE[ b >> 4 ];
        *dst++ = HEX_DIGITS_UPPERCASE[ b & 0xF ];
    }
    *dst = '\0';
    return( mac_address_string_format );
}

void application_start()
{
    wiced_result_t result;
    wiced_mac_t             mac;
    char                    mac_string[STR_MAX_MAC];
    uint8_t                 offset;
    uint8_t                 mac_offset;

    /* Generate unique name for accessory */
    wiced_wifi_get_mac_address( &mac );
    mac_addr_to_string( mac.octet, mac_string );
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

    /* Initialise the device */
    result = wiced_init( );
    if ( result != WICED_SUCCESS )
    {
        return;
    }
#ifdef WICED_HOMEKIT_DO_NOT_USE_MFI
    /* Bring up the network interface */
    result = wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Unable to bring up network connection\n"));
        return;
    }
#else

    apple_wac_info.name = accessory_name;

    result = wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );

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
            free( apple_wac_info.out_new_name );
        }
   }
#endif
    bonjour_test_start_gedday();
    result = wiced_http_server_start( &http_server, 80, 1, NULL, WICED_STA_INTERFACE, DEFAULT_URL_PROCESSOR_STACK_SIZE );

    command_console_init( STDIO_UART, sizeof(test_command_buffer), test_command_buffer, TEST_COMMAND_HISTORY_LENGTH, test_command_history_buffer, " ");
    console_add_cmd_table( test_command_table );

    WPRINT_APP_INFO(("Ready for a command\r\n"));
    /* 2000 seconds wait until test is done */
    wiced_rtos_delay_milliseconds( 2000000 );
}




