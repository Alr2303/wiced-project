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
 * HomeKit Tunnel Accessory Snippet
 *
 * This snippet demonstrates how to setup a tunnel for Bluetooth LE accessories
 *
 * The snippet has 2 build options:
 *    - MFi mode
 *    - Non-MFi mode
 *
 *     MFi Mode
 *        This uses WAC to start a soft AP and bring the Wiced HomeKit Tunnel Accessory onto the network
 *        It also advertises MFi support, and uses MFi certificate during the pairing process
 *        to prove it is an MFi device.
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.tunnel-BCM9WCDPLUS114 USE_MFI=1
 *
 *     Non-MFi Mode
 *        This uses the wifi credentials stored in wifi_config_dct.h to bring up the Wiced HomeKit Tunnel Accessory
 *        This advertises pin only support, and does not use MFi certificate during pairing
 *
 *        Build string in this mode:
 *
 *        snip.apple_homekit.tunnel-BCM9WCDPLUS114
 *
 *
 * Once the Wiced HomeKit Tunnel Accessory is on the network, it can be discovered by the HAT application or iPhone/iPad app.
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
 *  - Tunneling between controller and BLE accessories
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
 *   4. Wiced HomeKit Tunnel Accessory starts discovering BLE accessories.
 *
 *   5. From controller, connect to Tunnel Accessory and try to access tunneled
 *      BLE accessory characteristics
 *
 *      If running MFi mode, use the AirPort utility on the MAC (or  iPhone/iPad ), to bring up the Wiced HomeKit Tunnel Accessory
 *
 * HomeKit Accessory Demonstration
 *   To demonstrate the Wiced HomeKit Tunnel Accessory, start the HAT application once the Wiced HomeKit Tunnel Accessory has been added to
 *   your local network.
 *   - In the HAT application, click on the "Start Pairing" button, and wait for the "Accessory Password" to be shown
 *     on the serial output.
 *   - Enter the password shown on the display ( including the dashes )
 *   - Once the pairing is complete, click on the "Discover" button to perform a 'Pair-Verify' and get a listing of all
 *     services and characteristics for the accessory and tunneled BLE accessories.
 *   - You can now modify a characteristic value on tunneled BLE accessory which has read/write permissions
 *   - To add multiple controllers, perform the following steps:
 *     1. In the HAT application, click on the bottom right '+' sign and add another IP device
 *     2. For this controller, press "Start" to start listening for the Wiced HomeKit Tunnel Accessory
 *     3. Go to the paired admin controller, and select the newly created IP device, then select "Add Pairing"
 *     4. Once the pairing has been added, you can return to the new IP device, and begin discovery.
 *   - To enable events, go to any characteristic that support event and press the 'enable' button
 *   - Send an event from a tunneled BLE accessory and the tunnel will send it to controller
 *
 * Notes.
 *   The HomeKit specification imposes some restrictions on the usage of the accessory, which may not be intuitive.
 *   Please keep these points in mind when exploring the Wiced HomeKit Tunnel Accessory snippet.
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
#include "apple_homekit_app_helper.h"

#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI
#include "apple_wac.h"
#endif

#include "bt_target.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_cfg.h"

#include "base64.h"
#include "tunnel.h"
#include "tunnel_ble.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAIN_ACCESSORY_NAME                 "Wiced IP Tunnel Accessory-"
#define MAX_NAME_LENGTH                     ( 64 )
#define MAC_OCTET4_OFFSET                   ( 9 )


#define TUNNEL_WORKER_THREAD_STACK_SIZE     2048
#define TUNNEL_QUEUE_MAX_ENTRIES            10

#define TUNNEL_BLE_ACC_ADV_TIMEOUT          (TUNNEL_BLE_SCAN_DURATION + TUNNEL_BLE_NO_SCAN_DURATION)
#define TUNNEL_BLE_ACC_ASSOC_TIMEOUT        (TUNNEL_BLE_ACC_ADV_TIMEOUT * 2)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct
{
    TIMER_LIST_ENT          timer_entry;
    uint16_t                max_inst_id;

    BUFFER_Q                rsp_queue;

    tunneled_accessory_t*   acc_list_front;
    tunneled_accessory_t*   acc_list_rear;
} tunnel_cb_t;

typedef struct
{
    uint16_t                aid;
    uint16_t                iid;
    int                     status;
    uint8_t                 data_length;
    uint8_t                 data[1];
} response_data_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static void tunnel_app_init();
static wiced_result_t tunnel_on_characteristic_read(wiced_homekit_characteristic_read_parameters_t* read_params, wiced_http_response_stream_t* stream);
static wiced_result_t tunnel_on_characteristic_write(wiced_homekit_characteristic_value_update_list_t* update_list, wiced_http_response_stream_t* stream);
static wiced_result_t tunnel_on_characteristic_config(wiced_homekit_characteristic_value_update_list_t* event_list, wiced_http_response_stream_t* stream);
static wiced_result_t tunnel_local_value_read(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char);
static wiced_result_t tunnel_local_value_write(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char, char* p_value, uint8_t value_length);
static wiced_result_t wiced_homekit_display_password( uint8_t* srp_pairing_password );
static wiced_result_t identify_action( uint16_t accessory_id, uint16_t characteristic_id );
static void tunnel_timer_handler(TIMER_LIST_ENT* p_tle);
static void tunnel_ble_advertisement_state_update(tunneled_accessory_t* p_acc, wiced_bool_t advertising);
static void tunnel_create_tunnel_accessory();
static void tunnel_add_accessory(tunneled_accessory_t* p_accessory);
static void tunnel_delete_accessory(tunneled_accessory_t* p_accessory);
static void tunnel_add_accessory_service(tunneled_accessory_t* p_accessory, tunneled_service_t* p_service);
static void tunnel_add_service_characteristic(tunneled_service_t* p_service, tunneled_characteristic_t* p_characteristic);
static void tunnel_configure_accessory(tunneled_accessory_t* p_accessory, ble_discovery_data_t* p_data);
static void tunnel_configure_service(tunneled_service_t* p_service, ble_discovery_service_t* p_data);
static void tunnel_configure_characteristic(tunneled_characteristic_t* p_char, ble_discovery_char_t* p_data);
static void tunnel_add_tunneled_accessory_service(tunneled_accessory_t* p_accessory, tunneled_service_t* p_service, tunneled_characteristic_t* p_char);
static void tunnel_accessory_database_changed();
static void tunnel_debug_dump_configuration(tunneled_accessory_t* p_accessory);

static wiced_result_t tunnel_save_event_session(uint16_t accessory_id, uint16_t characteristic_id, wiced_http_response_stream_t* stream);
static wiced_result_t tunnel_remove_event_session(uint16_t accessory_id, uint16_t characteristic_id, wiced_http_response_stream_t* stream);
static wiced_result_t tunnel_event_sessions_remove_accessory(uint16_t accessory_id);
static wiced_result_t tunnel_event_sessions_remove_controller(wiced_http_response_stream_t* stream);
static wiced_result_t tunnel_event_sessions_remove_all();
static wiced_http_response_stream_t* tunnel_find_first_event_session(uint16_t accessory_id, uint16_t characteristic_id);
static wiced_http_response_stream_t* tunnel_find_next_event_session(uint16_t accessory_id, uint16_t characteristic_id);

static int tunnel_base64_to_raw_data(uint8_t* p_src, uint16_t src_len, uint8_t* p_dst, uint16_t dst_len);

/* Use BTU timer */
#define BTU_TTYPE_USER_FUNC         13
extern void btu_start_timer (TIMER_LIST_ENT *p_tle, uint16_t type, uint32_t timeout);

extern uint32_t tunnel_write_accessory_database(wiced_homekit_accessories_t* accessory, uint8_t* send_buffer, uint32_t offset);

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
    .mdns_desired_hostname  = "homekit-tunnel-test",
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


/* Accessory definitions */
wiced_homekit_accessories_t tunnel_accessory;

/* Accessory Information Service and Characteristics */
wiced_homekit_services_t        tunnel_accessory_information_service;
wiced_homekit_characteristics_t tunnel_accessory_identify_characteristic;
wiced_homekit_characteristics_t tunnel_accessory_manufacturer_characteristic;
wiced_homekit_characteristics_t tunnel_accessory_model_characteristic;
wiced_homekit_characteristics_t tunnel_accessory_name_characteristic;
wiced_homekit_characteristics_t tunnel_accessory_serial_number_characteristic;
wiced_homekit_characteristics_t tunnel_accessory_firmware_revision_characteristic;

/* Accessory information service characteristic values */
char tunnel_accessory_name_value[MAX_NAME_LENGTH] = "\""MAIN_ACCESSORY_NAME"\"";
char tunnel_accessory_manufacturer_value[]        = "\"Lights Co.\"";
char tunnel_accessory_model_value[]               = "\"Controller 1.0\"";
char tunnel_accessory_serial_number_value[]       = "\"12345678\"";
char tunnel_accessory_firmware_revision_value[]   = "\"10.1.1\"";

static tunnel_cb_t          tcb;
static uint16_t tunnel_aid = 0;
/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_result_t          result;
    uint32_t                wiced_heap_allocated_for_database;
    wiced_mac_t             mac;
    char                    mac_string[STRING_MAX_MAC];
    uint8_t                 offset;
    uint8_t                 mac_offset;

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

    WPRINT_APP_INFO(("HomeKit Tunnel Start\n"));

    tunnel_app_init();

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
            snprintf( tunnel_accessory_name_value, sizeof(tunnel_accessory_name_value), "\"%s\"", apple_wac_info.out_new_name );
            free( apple_wac_info.out_new_name );
        }
    }

#endif

    /* Create tunnel accessory and its information service */
    tunnel_create_tunnel_accessory();

    WPRINT_APP_INFO(("+---------------------------+\r\n"));
    WPRINT_APP_INFO(("+ Loading Bluetooth stack...+\r\n"));
    WPRINT_APP_INFO(("+---------------------------+\r\n"));

    tunnel_ble_init();

    /* Start main timer */
    tcb.timer_entry.param = (uint32_t)tunnel_timer_handler;
    btu_start_timer(&tcb.timer_entry, BTU_TTYPE_USER_FUNC, TIMER_INTERVAL);

    wiced_configure_accessory_password_for_device_with_display( wiced_homekit_display_password );

    /* Use the API below if your device has no display */
    /* wiced_configure_accessory_password_for_device_with_no_display( "888-77-888" ); */

    wiced_register_url_identify_callback( identify_action );

    wiced_register_tunneled_accessory_callbacks(tunnel_write_accessory_database, tunnel_on_characteristic_read,
            tunnel_on_characteristic_write, tunnel_on_characteristic_config);

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

    tunnel_ble_start();

    while( 1 )
    {
        /**
         * Send all characteristic values which have been updated and had event notifications enabled to controller
         * Send the list created by calling wiced_homekit_register_characteristic_value_update
         **/
        wiced_homekit_send_all_updates_for_accessory( );

        wiced_rtos_delay_milliseconds( 250 );
    }

}

void tunnel_app_init()
{
    memset(&tcb, 0, sizeof(tunnel_cb_t));
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

    /* Check if this is the Tunnel Accessory */
    if ( tunnel_accessory.instance_id == accessory_id )
    {
        WPRINT_APP_INFO(("\r\n TUNNEL ACCESSORY IDENTIFY \r\n"));

        /* Flash the LED representing the tunnel */
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

void tunnel_timer_handler(TIMER_LIST_ENT* p_tle)
{
    tunneled_accessory_t* p_acc = tcb.acc_list_front;

    while (p_acc)
    {
        tunneled_accessory_t* p_next = p_acc->next;

        if (p_acc->connection_timer_counter > 0)
        {
            if (--p_acc->connection_timer_counter == 0)
                tunnel_ble_disconnect_accessory(p_acc);
        }

        p_acc->advertisement_timer_counter++;
        if (p_acc->advertisement_timer_counter > TUNNEL_BLE_ACC_ASSOC_TIMEOUT * TICKS_PER_SECOND)
        {
            tunnel_ble_advertisement_state_update(p_acc, WICED_FALSE);

            WPRINT_APP_DEBUG(("Remove BLE accessory due to timeout\n"));
            tunnel_remove_accessory(p_acc);
        }
        p_acc = p_next;
    }

    tunnel_ble_timeout();

    btu_start_timer(&tcb.timer_entry, BTU_TTYPE_USER_FUNC, TIMER_INTERVAL);
}

void tunnel_set_connection_timer(tunneled_accessory_t* p_acc, wiced_bool_t start_timer)
{
    if (start_timer)
    {
        p_acc->connection_timer_counter = p_acc->tunneled_acc_connection_timeout * TICKS_PER_SECOND;
        p_acc->advertisement_timer_counter = 0;
    }
    else
    {
        p_acc->connection_timer_counter = 0;
    }
}

void tunnel_ble_discovery_callback(ble_discovery_data_t* p_data)
{
    size_t alloc_size;
    tunneled_accessory_t* p_accessory;
    tunneled_service_t* p_service;
    tunneled_characteristic_t* p_char;
    ble_discovery_service_t* p_disc_svc;
    ble_discovery_char_t* p_disc_char;
    uint8_t i, j;

    alloc_size = sizeof(tunneled_accessory_t);
    for (i = 0; i < p_data->num_of_svcs; i++)
    {
        alloc_size += sizeof(tunneled_service_t);
        for (j = 0; j < p_data->services[i].num_of_chars; j++)
        {
            p_disc_char = &p_data->services[i].characteristics[j];
            if (p_disc_char->optional_params)
                alloc_size += FULL_CHARACTERISTIC_SIZE;
            else
                alloc_size += THIN_CHARACTERISTIC_SIZE;
        }
    }
    alloc_size += sizeof(tunneled_service_t) + THIN_CHARACTERISTIC_SIZE * TUNNELED_ACCESSORY_CHAR_NUM;

    WPRINT_APP_DEBUG(("%d bytes allocated for device %s\n", alloc_size, p_data->local_name));
    p_accessory = (tunneled_accessory_t*)malloc(alloc_size);
    if (p_accessory)
    {
        memset(p_accessory, 0, alloc_size);
        tcb.max_inst_id = 0;

        tunnel_add_accessory(p_accessory);
        tunnel_configure_accessory(p_accessory, p_data);

        p_service = (tunneled_service_t*)(p_accessory + 1);
        p_char = (tunneled_characteristic_t*)(p_service + p_data->num_of_svcs + 1);
        for (i = 0; i < p_data->num_of_svcs; i++)
        {
            p_disc_svc = &p_data->services[i];
            tunnel_add_accessory_service(p_accessory, p_service);
            tunnel_configure_service(p_service, p_disc_svc);

            for (j = 0; j < p_disc_svc->num_of_chars; j++)
            {
                p_disc_char = &p_disc_svc->characteristics[j];
                tunnel_add_service_characteristic(p_service, p_char);
                tunnel_configure_characteristic(p_char, p_disc_char);

                if (p_char->full_size)
                    p_char = (tunneled_characteristic_t*)(((uint8_t*)p_char) + FULL_CHARACTERISTIC_SIZE);
                else
                    p_char = (tunneled_characteristic_t*)(((uint8_t*)p_char) + THIN_CHARACTERISTIC_SIZE);
            }
            p_service++;
        }

        p_accessory->max_ble_inst_id = tcb.max_inst_id;
        strncpy(p_accessory->tunneled_acc_name, p_data->local_name, TUNNEL_ACCESSORY_MAX_NAME_LEN - 1);
        memcpy(p_accessory->tunneled_acc_identifier, p_data->adv_manu_data.device_id,
                TUNNEL_ACCESSORY_ID_LEN);
        p_accessory->tunneled_acc_state_number = p_data->adv_manu_data.global_state_num;
        p_accessory->tunneled_acc_connected = WICED_TRUE;
        p_accessory->tunneled_acc_advertising = WICED_TRUE;
        p_accessory->tunneled_acc_connection_timeout = 8;
        tunnel_add_tunneled_accessory_service(p_accessory, p_service, p_char);

        tunnel_debug_dump_configuration(p_accessory);

        tunnel_accessory_database_changed();
    }
}

void tunnel_trace_data(uint8_t* p_data, uint16_t data_len)
{
    for (int i = 0; i < data_len; i++)
    {
        WPRINT_APP_DEBUG(("%02X ", p_data[i]));
        if ((i + 1) % 16 == 0)
            WPRINT_APP_DEBUG(("\n"));
    }
    WPRINT_APP_DEBUG(("\n"));
}

static void tunnel_enqueue_homekit_response(uint16_t aid, uint16_t iid, int status,
        char* p_data, uint16_t data_len)
{
    response_data_t* p_rsp;

    p_rsp = (response_data_t*)GKI_getbuf(sizeof(response_data_t) + data_len);
    if (p_rsp)
    {
        memset(p_rsp, 0, sizeof(response_data_t) + data_len),
        p_rsp->aid = aid;
        p_rsp->iid = iid;
        p_rsp->status = status;
        if (p_data && data_len > 0)
        {
            memcpy(p_rsp->data, p_data, data_len);
            p_rsp->data_length = data_len;
        }
        GKI_enqueue(&tcb.rsp_queue, p_rsp);
    }
}

static wiced_result_t tunnel_send_homekit_response(wiced_http_response_stream_t* stream,
        wiced_homekit_response_type_t response_type)
{
    wiced_homekit_response_data_t* response_data;
    int num_of_rsp = tcb.rsp_queue.count;
    response_data_t* p_rsp;
    int i = 0;
    wiced_result_t result = WICED_SUCCESS;

    WPRINT_APP_DEBUG(("Send HAP response type:%d, number:%d\n", response_type, num_of_rsp));

    if (num_of_rsp == 0)
        return WICED_SUCCESS;

    response_data = GKI_getbuf(sizeof(wiced_homekit_response_data_t) * num_of_rsp);

    if (response_data)
    {
        p_rsp = GKI_getfirst(&tcb.rsp_queue);
        while (p_rsp)
        {
            response_data[i].accessory_id = p_rsp->aid;
            response_data[i].instance_id = p_rsp->iid;
            response_data[i].status = p_rsp->status;
            response_data[i].data_length = p_rsp->data_length;
            response_data[i].data = p_rsp->data;
            i++;

            p_rsp = GKI_getnext(p_rsp);
        }

        wiced_homekit_send_responses(stream, response_type, response_data, num_of_rsp);

        GKI_freebuf(response_data);
    }
    else
    {
        result = WICED_OUT_OF_HEAP_SPACE;
    }

    while ((p_rsp = GKI_dequeue(&tcb.rsp_queue)) != NULL)
        GKI_freebuf(p_rsp);

    return result;
}

static wiced_result_t tunnel_send_homekit_event(tunneled_accessory_t* p_acc,
        tunneled_characteristic_t* p_char, char* data, uint8_t data_length)
{
    wiced_homekit_response_data_t rsp_data;
    wiced_http_response_stream_t* stream;
    wiced_socket_state_t socket_state;

    /* Find inactive socket and remove it */
    stream = tunnel_find_first_event_session(p_acc->ip_acc.instance_id, p_char->inst_id);
    while (stream)
    {
        wiced_tcp_get_socket_state(stream->tcp_stream.socket, &socket_state);
        if(socket_state == WICED_SOCKET_CONNECTED)
        {
            stream = tunnel_find_next_event_session(p_acc->ip_acc.instance_id, p_char->inst_id);
        }
        else
        {
            tunnel_event_sessions_remove_controller(stream);
            stream = tunnel_find_first_event_session(p_acc->ip_acc.instance_id, p_char->inst_id);
        }
    }

    /* Prepare event data */
    rsp_data.accessory_id = p_acc->ip_acc.instance_id;
    rsp_data.instance_id = p_char->inst_id;
    rsp_data.status = HOMEKIT_STATUS_SUCCESS;
    rsp_data.data_length = data_length;
    rsp_data.data = (uint8_t*)data;

    /* Send event to every configured controller */
    stream = tunnel_find_first_event_session(p_acc->ip_acc.instance_id, p_char->inst_id);
    while (stream)
    {
        WPRINT_APP_DEBUG(("Send HAP event aid:%d, iid:%d, value:%s\n", rsp_data.accessory_id, rsp_data.instance_id, data));
        wiced_homekit_send_responses(stream, HOMEKIT_RESPONSE_TYPE_EVENT, &rsp_data, 1);
        stream = tunnel_find_next_event_session(p_acc->ip_acc.instance_id, p_char->inst_id);
    }

    return WICED_SUCCESS;
}

static void tunnel_send_tunneled_accessory_service_event(tunneled_accessory_t* p_acc, uint16_t char_uuid)
{
    tunneled_characteristic_t* p_char;
    char value_string[20];

    p_char = tunnel_get_service_characteristic_by_uuid(p_acc, UUID_SVC_TUNNELED_BLE_ACCESSORY, char_uuid);
    if (p_char)
    {
        tunnel_get_tunneled_accessory_value(p_acc, p_char, value_string, 20);
        tunnel_send_homekit_event(p_acc, p_char, value_string, strlen(value_string));
    }
}

void tunnel_ble_connected_callback(tunneled_accessory_t* p_acc, wiced_bool_t connected)
{
    if (p_acc->tunneled_acc_connected != connected)
    {
        p_acc->tunneled_acc_connected = connected;

        tunnel_send_tunneled_accessory_service_event(p_acc, UUID_CHAR_TUNNELED_ACCESSORY_CONNECTED);
    }

    tunnel_set_connection_timer(p_acc, p_acc->tunneled_acc_connected);
}

void tunnel_ble_advertisement_update(tunneled_accessory_t* p_acc, uint16_t state_number)
{
    if (p_acc->tunneled_acc_state_number != state_number)
    {
        p_acc->tunneled_acc_state_number = state_number;

        tunnel_send_tunneled_accessory_service_event(p_acc, UUID_CHAR_TUNNELED_ACCESSORY_STATE_NUMBER);
    }

    p_acc->advertisement_timer_counter = 0;
    tunnel_ble_advertisement_state_update(p_acc, WICED_TRUE);
}

void tunnel_ble_advertisement_state_update(tunneled_accessory_t* p_acc, wiced_bool_t advertising)
{
    if (p_acc->tunneled_acc_advertising != advertising)
    {
        p_acc->tunneled_acc_advertising = advertising;

        tunnel_send_tunneled_accessory_service_event(p_acc, UUID_CHAR_TUNNELED_ACCESSORY_ADVERTISING);
    }
}

static int gatt_status_to_homekit_status(wiced_bt_gatt_status_t status)
{
    int homekit_status;

    switch (status)
    {
    case WICED_BT_GATT_SUCCESS:
        homekit_status = HOMEKIT_STATUS_SUCCESS;
        break;
    case WICED_BT_GATT_INVALID_HANDLE:
        homekit_status = HOMEKIT_STATUS_NOTIFICATION_NOT_SUPPORTED;
        break;
    case WICED_BT_GATT_INSUF_AUTHENTICATION:
    case WICED_BT_GATT_INSUF_AUTHORIZATION:
        homekit_status = HOMEKIT_STATUS_INSUFFICIENT_PRIVILEGES;
        break;
    default:
        homekit_status = HOMEKIT_STATUS_UNABLE_TO_COMMUNICATE;
    }

    return homekit_status;
}

void tunnel_ble_read_callback(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char,
        wiced_bt_gatt_status_t status, uint8_t* p_data, uint16_t data_len)
{
    response_data_t* p_rsp;
    uint16_t buf_len;

    if (status == WICED_BT_GATT_SUCCESS)
    {
        WPRINT_APP_DEBUG(("BLE data received:\n"));
        tunnel_trace_data(p_data, data_len);

        buf_len = data_len * 2;
    }
    else
    {
        buf_len = 0;
    }

    p_rsp = (response_data_t*)GKI_getbuf(buf_len + sizeof(response_data_t));
    if (p_rsp)
    {
        p_rsp->aid = p_acc->ip_acc.instance_id;
        p_rsp->iid = p_char->inst_id;
        p_rsp->status = gatt_status_to_homekit_status(status);
        if (status == WICED_BT_GATT_SUCCESS)
        {
            int encode_len;

            p_rsp->data[0] = '\"';
            encode_len = base64_encode(p_data, data_len, &p_rsp->data[1], buf_len - 1, BASE64_STANDARD);
            if (encode_len < 0)
            {
                WPRINT_APP_DEBUG(("BASE64 encoding failed\n"));
                encode_len = 0;
            }
            p_rsp->data_length = encode_len + 1;
            p_rsp->data[p_rsp->data_length++] = '\"';
        }
        GKI_enqueue(&tcb.rsp_queue, p_rsp);
    }

    tunnel_set_connection_timer(p_acc, WICED_TRUE);
}

void tunnel_ble_write_callback(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char,
        wiced_bt_gatt_status_t status)
{
    tunnel_enqueue_homekit_response(p_acc->ip_acc.instance_id, p_char->inst_id,
            gatt_status_to_homekit_status(status), NULL, 0);

    tunnel_set_connection_timer(p_acc, WICED_TRUE);
}

void tunnel_ble_indication_callback(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char,
        wiced_bt_gatt_status_t status)
{
    char value_string[8];

    tunnel_get_null_value(p_char->format, value_string, 8);

    tunnel_send_homekit_event(p_acc, p_char, value_string, strlen(value_string));

    tunnel_set_connection_timer(p_acc, WICED_TRUE);
}

wiced_result_t tunnel_on_characteristic_read(wiced_homekit_characteristic_read_parameters_t* read_params,
        wiced_http_response_stream_t* stream)
{
    tunneled_accessory_t* p_acc;
    tunneled_characteristic_t* p_char;
    uint16_t* p_aid = read_params->accessory_instance_id;
    uint16_t* p_iid = read_params->characteristic_instance_id;

    /* read value data from BLE accessory */
    while (*p_aid)
    {
        WPRINT_APP_DEBUG(("Remote read: Accessory ID %d, Characteristic ID %d\n", *p_aid, *p_iid));

        p_acc = tunnel_get_accessory_by_aid(*p_aid);
        if (p_acc)
        {
            p_char = tunnel_get_characteristic_by_iid(p_acc, *p_iid);
            if (p_char)
            {
                if (p_char->inst_id <= p_acc->max_ble_inst_id)
                {
                    tunnel_set_connection_timer(p_acc, WICED_TRUE);
                    tunnel_ble_read_characteristic(p_acc, p_char);
                }
                else
                {
                    tunnel_local_value_read(p_acc, p_char);
                }
            }
        }

        p_aid++;
        p_iid++;
    }

    tunnel_send_homekit_response(stream, HOMEKIT_RESPONSE_TYPE_READ);

    return WICED_SUCCESS;
}

wiced_result_t tunnel_on_characteristic_write(wiced_homekit_characteristic_value_update_list_t* update_list, wiced_http_response_stream_t* stream)
{
    tunneled_accessory_t* p_acc;
    tunneled_characteristic_t* p_char;
    uint16_t aid;
    uint16_t iid;
    char* value;
    uint8_t value_length;

    for (uint8_t i = 0; i < update_list->number_of_updates; i++)
    {
        aid = update_list->characteristic_descriptor[i].accessory_instance_id;
        iid = update_list->characteristic_descriptor[i].characteristic_instance_id;
        value = update_list->characteristic_descriptor[i].value;
        value_length = update_list->characteristic_descriptor[i].value_length;

        WPRINT_APP_DEBUG(("Remote write: Accessory ID %d, Characteristic ID %d\n", aid, iid));

        p_acc = tunnel_get_accessory_by_aid(aid);
        if (p_acc)
        {
            p_char = tunnel_get_characteristic_by_iid(p_acc, iid);
            if (p_char)
            {
                if (p_char->inst_id <= p_acc->max_ble_inst_id)
                {
                    uint8_t* p_data;
                    int data_len;

                    p_data = GKI_getbuf(value_length);
                    if (p_data)
                    {
                        data_len = tunnel_base64_to_raw_data((uint8_t*)value, value_length, p_data, value_length);
                        if (data_len > 0)
                        {
                            tunnel_trace_data(p_data, (uint16_t)data_len);
                            tunnel_set_connection_timer(p_acc, WICED_TRUE);
                            tunnel_ble_write_characteristic(p_acc, p_char, p_data, (uint16_t)data_len);
                        }
                        GKI_freebuf(p_data);
                    }
                }
                else
                {
                    tunnel_local_value_write(p_acc, p_char, value, value_length);
                }
            }
        }
    }

    tunnel_send_homekit_response(stream, HOMEKIT_RESPONSE_TYPE_WRITE);

    return WICED_SUCCESS;
}

wiced_result_t tunnel_on_characteristic_config(wiced_homekit_characteristic_value_update_list_t* event_list, wiced_http_response_stream_t* stream)
{
    tunneled_accessory_t* p_acc;
    tunneled_characteristic_t* p_char;
    uint16_t aid;
    uint16_t iid;
    wiced_bool_t config_sent;
    int status;

    for (uint8_t i = 0; i < event_list->number_of_updates; i++)
    {
        aid = event_list->characteristic_descriptor[i].accessory_instance_id;
        iid = event_list->characteristic_descriptor[i].characteristic_instance_id;

        WPRINT_APP_DEBUG(("Remote config: Accessory ID %d, Characteristic ID %d\n", aid, iid));

        config_sent = WICED_FALSE;
        status = HOMEKIT_STATUS_NOTIFICATION_NOT_SUPPORTED;
        p_acc = tunnel_get_accessory_by_aid(aid);
        if (p_acc)
        {
            p_char = tunnel_get_characteristic_by_iid(p_acc, iid);
            if (p_char)
            {
                if ((p_char->hap_properties & HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_CONNECTED) ||
                    (p_char->hap_properties & HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_DISCONNECTED))
                {
                    if (memcmp(event_list->characteristic_descriptor[i].value, "true", event_list->characteristic_descriptor[i].value_length ) == 0)
                    {
                        if (!tunnel_find_first_event_session(aid, iid))
                        {
                            if (p_char->inst_id <= p_acc->max_ble_inst_id)
                            {
                                tunnel_set_connection_timer(p_acc, WICED_TRUE);
                                tunnel_ble_configure_characteristic(p_acc, p_char, 0x02);
                                config_sent = WICED_TRUE;
                            }
                        }
                        /* save configured event with connected HTTP stream */
                        tunnel_save_event_session(aid, iid, stream);
                    }
                    else
                    {
                        tunnel_remove_event_session(aid, iid, stream);
                        if (!tunnel_find_first_event_session(aid, iid))
                        {
                            if (p_char->inst_id <= p_acc->max_ble_inst_id)
                            {
                                tunnel_set_connection_timer(p_acc, WICED_TRUE);
                                tunnel_ble_configure_characteristic(p_acc, p_char, 0x00);
                                config_sent = WICED_TRUE;
                            }
                        }
                    }

                    status = HOMEKIT_STATUS_SUCCESS;
                }
            }
        }

        if (!config_sent)
        {
            tunnel_enqueue_homekit_response(aid, iid, status, NULL, 0);
        }
    }

    tunnel_send_homekit_response(stream, HOMEKIT_RESPONSE_TYPE_CONFIG);

    return WICED_SUCCESS;
}

int get_int_value(char* p_data, uint8_t data_len)
{
    char str[16];
    int i;

    if (data_len > 15)
        data_len = 15;
    memcpy(str, p_data, data_len);
    str[data_len] = 0;
    sscanf(str, "%d", &i);

    return i;
}

wiced_result_t tunnel_get_null_value(uint8_t format, char* value_string, uint32_t string_length)
{
    if (format == HAP_CHARACTERISTIC_FORMAT_STRING)
    {
        strncpy(value_string, "\"\"", string_length);
    }
    else
    {
        strncpy(value_string, "null", string_length);
    }

    return WICED_SUCCESS;
}

static void tunnel_accessory_id_to_string(uint8_t* p_data, char* p_str)
{
    *p_str++ = '\"';

    for (int i = 0; i < TUNNEL_ACCESSORY_ID_LEN; i++)
    {
        sprintf(p_str, "%02X:", *p_data++);
        p_str += 3;
    }

    *(--p_str) = '\"';
}

wiced_result_t tunnel_get_tunneled_accessory_value(tunneled_accessory_t* p_acc,
        tunneled_characteristic_t* p_char, char* value_string, uint32_t string_length)
{
    wiced_result_t result = WICED_SUCCESS;

    switch (p_char->uuid)
    {
    case UUID_CHAR_NAME:
        snprintf(value_string, string_length, "\"%s\"", p_acc->tunneled_acc_name);
        break;
    case UUID_CHAR_TUNNELED_ACCESSORY_IDENTIFIER:
        tunnel_accessory_id_to_string(p_acc->tunneled_acc_identifier, value_string);
        break;
    case UUID_CHAR_TUNNELED_ACCESSORY_STATE_NUMBER:
        snprintf(value_string, string_length, "%d", (int)p_acc->tunneled_acc_state_number);
        break;
    case UUID_CHAR_TUNNELED_ACCESSORY_CONNECTED:
        snprintf(value_string, string_length, "%s", p_acc->tunneled_acc_connected ? "true" : "false");
        break;
    case UUID_CHAR_TUNNELED_ACCESSORY_ADVERTISING:
        snprintf(value_string, string_length, "%s", p_acc->tunneled_acc_advertising ? "true" : "false");
        break;
    case UUID_CHAR_TUNNELED_ACCESSORY_CONNECTION_TIMEOUT:
        snprintf(value_string, string_length, "%d", p_acc->tunneled_acc_connection_timeout);
        break;
    default:
        result = WICED_UNSUPPORTED;
        break;
    }

    return result;
}

wiced_result_t tunnel_local_value_read(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char)
{
    char value_string[20];

    tunnel_get_tunneled_accessory_value(p_acc, p_char, value_string, 20);

    tunnel_enqueue_homekit_response(p_acc->ip_acc.instance_id, p_char->inst_id, HOMEKIT_STATUS_SUCCESS,
            value_string, strlen(value_string));

    return WICED_SUCCESS;
}

wiced_result_t tunnel_local_value_write(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char,
        char* p_value, uint8_t value_length)
{
    int status = HOMEKIT_STATUS_SUCCESS;

    if (p_char->uuid == UUID_CHAR_TUNNELED_ACCESSORY_CONNECTED)
    {
        int value = get_int_value(p_value, value_length);
        if (value)
            tunnel_ble_connect_accessory(p_acc);
        else
            tunnel_ble_disconnect_accessory(p_acc);
    }
    else if (p_char->uuid == UUID_CHAR_TUNNELED_ACCESSORY_CONNECTION_TIMEOUT)
    {
        p_acc->tunneled_acc_connection_timeout = get_int_value(p_value, value_length);
    }
    else
    {
        status = HOMEKIT_STATUS_CAN_NOT_WRITE;
    }

    tunnel_enqueue_homekit_response(p_acc->ip_acc.instance_id, p_char->inst_id, status, NULL, 0);

    return WICED_SUCCESS;
}

void tunnel_create_tunnel_accessory()
{
    uint16_t iid = 1;

    /* Initialize Tunnel Accessory information service */
    wiced_homekit_initialise_accessory_information_service( &tunnel_accessory_information_service, iid++ );

    /* Initialize Tunnel Information Service Characteristics */
    wiced_homekit_initialise_identify_characteristic     ( &tunnel_accessory_identify_characteristic,      identify_action, iid++ );
    wiced_homekit_initialise_name_characteristic         ( &tunnel_accessory_name_characteristic,          tunnel_accessory_name_value,          strlen( tunnel_accessory_name_value ),          NULL, NULL, iid++ );
    wiced_homekit_initialise_manufacturer_characteristic ( &tunnel_accessory_manufacturer_characteristic,  tunnel_accessory_manufacturer_value,  strlen( tunnel_accessory_manufacturer_value ),  NULL, NULL, iid++ );
    wiced_homekit_initialise_model_characteristic        ( &tunnel_accessory_model_characteristic,         tunnel_accessory_model_value,         strlen( tunnel_accessory_model_value ),         NULL, NULL, iid++ );
    wiced_homekit_initialise_serial_number_characteristic( &tunnel_accessory_serial_number_characteristic, tunnel_accessory_serial_number_value, strlen( tunnel_accessory_serial_number_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_firmwar_revision_characteristic( &tunnel_accessory_firmware_revision_characteristic, tunnel_accessory_firmware_revision_value, strlen( tunnel_accessory_firmware_revision_value ), NULL, NULL, iid++ );

    /* Add Tunnel Accessory*/
    wiced_homekit_add_accessory( &tunnel_accessory, tunnel_aid++ );

    /* Add Tunnel Accessory Information Service */
    wiced_homekit_add_service( &tunnel_accessory, &tunnel_accessory_information_service );

    /* Add Tunnel Information Service Characteristics */
    wiced_homekit_add_characteristic( &tunnel_accessory_information_service, &tunnel_accessory_identify_characteristic         );
    wiced_homekit_add_characteristic( &tunnel_accessory_information_service, &tunnel_accessory_manufacturer_characteristic     );
    wiced_homekit_add_characteristic( &tunnel_accessory_information_service, &tunnel_accessory_model_characteristic            );
    wiced_homekit_add_characteristic( &tunnel_accessory_information_service, &tunnel_accessory_name_characteristic             );
    wiced_homekit_add_characteristic( &tunnel_accessory_information_service, &tunnel_accessory_serial_number_characteristic    );
    wiced_homekit_add_characteristic( &tunnel_accessory_information_service, &tunnel_accessory_firmware_revision_characteristic );
}

void tunnel_add_accessory(tunneled_accessory_t* p_accessory)
{
    if (tcb.acc_list_front == NULL)
    {
        tcb.acc_list_front = tcb.acc_list_rear = p_accessory;
    }
    else
    {
        tcb.acc_list_rear->next = p_accessory;
        tcb.acc_list_rear = p_accessory;
    }
}

void tunnel_delete_accessory(tunneled_accessory_t* p_accessory)
{
    if (tcb.acc_list_front == p_accessory)
    {
        tcb.acc_list_front = p_accessory->next;

        if (tcb.acc_list_front == NULL)
            tcb.acc_list_rear = NULL;
    }
    else
    {
        tunneled_accessory_t* p = tcb.acc_list_front;

        while (p)
        {
            if (p->next == p_accessory)
            {
                if (p->next == tcb.acc_list_rear)
                    tcb.acc_list_rear = p;

                p->next = p_accessory->next;
                break;
            }
            p = p->next;
        }
    }
}

void tunnel_add_accessory_service(tunneled_accessory_t* p_accessory, tunneled_service_t* p_service)
{
    if (p_accessory->svc_list_front == NULL)
    {
        p_accessory->svc_list_front = p_accessory->svc_list_rear = p_service;
    }
    else
    {
        p_accessory->svc_list_rear->next = p_service;
        p_accessory->svc_list_rear = p_service;
    }
}

void tunnel_add_service_characteristic(tunneled_service_t* p_service,
        tunneled_characteristic_t* p_characteristic)
{
    if (p_service->char_list_front == NULL)
    {
        p_service->char_list_front = p_service->char_list_rear = p_characteristic;
    }
    else
    {
        p_service->char_list_rear->next = p_characteristic;
        p_service->char_list_rear = p_characteristic;
    }
}

void tunnel_configure_accessory(tunneled_accessory_t* p_accessory, ble_discovery_data_t* p_data)
{
    WPRINT_APP_DEBUG(("Configure accessory %s\n", p_data->local_name));

    /* Configure BLE parameters */
    memcpy(p_accessory->bd_addr, p_data->bd_addr, BD_ADDR_LEN);
    p_accessory->addr_type = p_data->addr_type;

    /* Configure IP parameters */
    p_accessory->ip_acc.tunneled = WICED_TRUE;
    wiced_homekit_add_accessory(&p_accessory->ip_acc, tunnel_aid++);
}

void tunnel_configure_service(tunneled_service_t* p_service, ble_discovery_service_t* p_data)
{
    WPRINT_APP_DEBUG(("Configure service 0x%x instance ID %d\n", p_data->uuid, p_data->inst_id));

    /* Configure BLE parameters */
    p_service->uuid = p_data->uuid;
    p_service->inst_id = p_data->inst_id;

    if (p_service->inst_id > tcb.max_inst_id)
        tcb.max_inst_id = p_service->inst_id;
}

void tunnel_configure_characteristic(tunneled_characteristic_t* p_char, ble_discovery_char_t* p_data)
{
    WPRINT_APP_DEBUG(("Configure char 0x%x instance ID %d\n", p_data->uuid, p_data->inst_id));

    p_char->uuid = p_data->uuid;
    p_char->inst_id = p_data->inst_id;
    p_char->handle = p_data->handle;
    p_char->ccc_handle = p_data->ccc_handle;
    p_char->ble_properties = p_data->ble_properties;
    p_char->format = p_data->format;
    p_char->unit = p_data->unit;
    p_char->hap_properties = p_data->hap_properties;

    if (p_data->optional_params)
    {
        p_char->full_size = 1;
        memcpy(&p_char->minimum, p_data->minimum, HAP_PARAM_MAX_RANGE_LEN);
        memcpy(&p_char->maximum, p_data->maximum, HAP_PARAM_MAX_RANGE_LEN);
        memcpy(&p_char->step, p_data->step, HAP_PARAM_MAX_RANGE_LEN);
    }
    if (p_char->inst_id > tcb.max_inst_id)
        tcb.max_inst_id = p_char->inst_id;
}

void tunnel_add_tunneled_accessory_service(tunneled_accessory_t* p_accessory,
        tunneled_service_t* p_service, tunneled_characteristic_t* p_char)
{
    uint16_t i = 1;

    /* Add tunneled BLE accessory service */
    tunnel_add_accessory_service(p_accessory, p_service);
    p_service->uuid = UUID_SVC_TUNNELED_BLE_ACCESSORY;
    p_service->inst_id = p_accessory->max_ble_inst_id + i++;

    /* Add tunneled accessory name characteristic */
    tunnel_add_service_characteristic(p_service, p_char);
    p_char->uuid = UUID_CHAR_NAME;
    p_char->inst_id = p_accessory->max_ble_inst_id + i++;
    p_char->hap_properties = HAP_CHARACTERISTIC_PROPERTY_SECURE_READ;
    p_char->format = HAP_CHARACTERISTIC_FORMAT_STRING;
    p_char = (tunneled_characteristic_t*)(((uint8_t*)p_char) + THIN_CHARACTERISTIC_SIZE);

    /* Add tunneled accessory identifier characteristic */
    tunnel_add_service_characteristic(p_service, p_char);
    p_char->uuid = UUID_CHAR_TUNNELED_ACCESSORY_IDENTIFIER;
    p_char->inst_id = p_accessory->max_ble_inst_id + i++;
    p_char->hap_properties = HAP_CHARACTERISTIC_PROPERTY_SECURE_READ;
    p_char->format = HAP_CHARACTERISTIC_FORMAT_STRING;
    p_char = (tunneled_characteristic_t*)(((uint8_t*)p_char) + THIN_CHARACTERISTIC_SIZE);

    /* Add tunneled accessory state number characteristic */
    tunnel_add_service_characteristic(p_service, p_char);
    p_char->uuid = UUID_CHAR_TUNNELED_ACCESSORY_STATE_NUMBER;
    p_char->inst_id = p_accessory->max_ble_inst_id + i++;
    p_char->hap_properties = HAP_CHARACTERISTIC_PROPERTY_SECURE_READ | HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_CONNECTED;
    p_char->format = HAP_CHARACTERISTIC_FORMAT_INT32;
    p_char = (tunneled_characteristic_t*)(((uint8_t*)p_char) + THIN_CHARACTERISTIC_SIZE);

    /* Add tunneled accessory connected characteristic */
    tunnel_add_service_characteristic(p_service, p_char);
    p_char->uuid = UUID_CHAR_TUNNELED_ACCESSORY_CONNECTED;
    p_char->inst_id = p_accessory->max_ble_inst_id + i++;
    p_char->hap_properties = HAP_CHARACTERISTIC_PROPERTY_SECURE_READ | HAP_CHARACTERISTIC_PROPERTY_SECURE_WRITE | HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_CONNECTED;
    p_char->format = HAP_CHARACTERISTIC_FORMAT_BOOL;
    p_char = (tunneled_characteristic_t*)(((uint8_t*)p_char) + THIN_CHARACTERISTIC_SIZE);

    /* Add tunneled accessory advertising characteristic */
    tunnel_add_service_characteristic(p_service, p_char);
    p_char->uuid = UUID_CHAR_TUNNELED_ACCESSORY_ADVERTISING;
    p_char->inst_id = p_accessory->max_ble_inst_id + i++;
    p_char->hap_properties = HAP_CHARACTERISTIC_PROPERTY_SECURE_READ | HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_CONNECTED;
    p_char->format = HAP_CHARACTERISTIC_FORMAT_BOOL;
    p_char = (tunneled_characteristic_t*)(((uint8_t*)p_char) + THIN_CHARACTERISTIC_SIZE);

    /* Add tunneled accessory connection timeout characteristic */
    tunnel_add_service_characteristic(p_service, p_char);
    p_char->uuid = UUID_CHAR_TUNNELED_ACCESSORY_CONNECTION_TIMEOUT;
    p_char->inst_id = p_accessory->max_ble_inst_id + i++;
    p_char->hap_properties = HAP_CHARACTERISTIC_PROPERTY_SECURE_READ | HAP_CHARACTERISTIC_PROPERTY_SECURE_WRITE | HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_CONNECTED;
    p_char->format = HAP_CHARACTERISTIC_FORMAT_INT32;
    p_char = (tunneled_characteristic_t*)(((uint8_t*)p_char) + THIN_CHARACTERISTIC_SIZE);
}

void tunnel_remove_accessory(tunneled_accessory_t* p_accessory)
{
    uint8_t* bd_addr = &p_accessory->bd_addr[0];
    WPRINT_APP_DEBUG(("Removing accessory %s(%02x:%02x:%02x:%02x:%02x:%02x)\n",
            p_accessory->tunneled_acc_name, bd_addr[0], bd_addr[1], bd_addr[2],
            bd_addr[3], bd_addr[4], bd_addr[5]));

    tunnel_event_sessions_remove_accessory(p_accessory->ip_acc.instance_id);

    wiced_homekit_remove_accessory(&p_accessory->ip_acc);

    tunnel_accessory_database_changed();

    tunnel_delete_accessory(p_accessory);
    free(p_accessory);
}

void tunnel_accessory_database_changed()
{
    uint32_t        config_number = 0;
    wiced_result_t  res = WICED_SUCCESS;

    tunnel_event_sessions_remove_all();

    wiced_homekit_disconnect_all_controllers();

    wiced_homekit_recalculate_accessory_database();

    res = wiced_homekit_get_configuration_number( &config_number );
    if (res != WICED_SUCCESS)
    {
        WPRINT_APP_DEBUG(("Failed to get config number. Error = [%d]\n", res));
        return;
    }
    config_number++;
    res = wiced_homekit_set_configuration_number( config_number );
    if (res != WICED_SUCCESS)
    {
        WPRINT_APP_DEBUG(("Failed to set config number. Error = [%d]\n", res));
        return;
    }
}

void tunnel_debug_dump_configuration(tunneled_accessory_t* p_accessory)
{
#if 0
    tunneled_service_t* p_service;
    tunneled_characteristic_t* p_char;

    WPRINT_APP_DEBUG(("Device %s configuration\n", p_accessory->tunneled_acc_name));

    p_service = p_accessory->svc_list_front;
    while (p_service)
    {
        WPRINT_APP_DEBUG(("  Service UUID 0x%04X IID %d\n", p_service->uuid, p_service->inst_id));

        p_char = p_service->char_list_front;
        while (p_char)
        {
            WPRINT_APP_DEBUG(("    Characteristic UUID 0x%04X IID %d\n", p_char->uuid, p_char->inst_id));
            WPRINT_APP_DEBUG(("        handle 0x%04X ccc_handle 0x%04X\n", p_char->handle, p_char->ccc_handle));
            WPRINT_APP_DEBUG(("        BLE prop 0x%04X HAP prop 0x%04X unit 0x%04X\n", p_char->ble_properties, p_char->hap_properties, p_char->unit));
            switch (p_char->format)
            {
            case HAP_CHARACTERISTIC_FORMAT_BOOL:
                WPRINT_APP_DEBUG(("        format BOOL\n"));
                break;

            case HAP_CHARACTERISTIC_FORMAT_UINT8:
                if (p_char->full_size)
                    WPRINT_APP_DEBUG(("        format UINT8, min %d, max %d, step %d\n", p_char->minimum.value_u8, p_char->maximum.value_u8, p_char->step.value_u8));
                else
                    WPRINT_APP_DEBUG(("        format UINT8\n"));
                break;

            case HAP_CHARACTERISTIC_FORMAT_UINT16:
                if (p_char->full_size)
                    WPRINT_APP_DEBUG(("        format UINT16, min %d, max %d, step %d\n", p_char->minimum.value_u16, p_char->maximum.value_u16, p_char->step.value_u16));
                else
                    WPRINT_APP_DEBUG(("        format UINT16\n"));
                break;

            case HAP_CHARACTERISTIC_FORMAT_UINT32:
                if (p_char->full_size)
                    WPRINT_APP_DEBUG(("        format UINT32, min %d, max %d, step %d\n", (int)p_char->minimum.value_u32, (int)p_char->maximum.value_u32, (int)p_char->step.value_u32));
                else
                    WPRINT_APP_DEBUG(("        format UINT32\n"));
                break;

            case HAP_CHARACTERISTIC_FORMAT_INT32:
                if (p_char->full_size)
                    WPRINT_APP_DEBUG(("        format INT, min %d, max %d, step %d\n", (int)p_char->minimum.value_i32, (int)p_char->maximum.value_i32, (int)p_char->step.value_i32));
                else
                    WPRINT_APP_DEBUG(("        format INT\n"));
                break;

            case HAP_CHARACTERISTIC_FORMAT_FLOAT:
                if (p_char->full_size)
                    WPRINT_APP_DEBUG(("        format FLOAT, min %f, max %f, step %f\n", p_char->minimum.value_f, p_char->maximum.value_f, p_char->step.value_f));
                else
                    WPRINT_APP_DEBUG(("        format FLOAT\n"));
                break;

            case HAP_CHARACTERISTIC_FORMAT_STRING:
                WPRINT_APP_DEBUG(("        format STRING\n"));
                break;

            case HAP_CHARACTERISTIC_FORMAT_TLV8:
                WPRINT_APP_DEBUG(("        format TLV8\n"));
                break;
            }

            p_char = p_char->next;
        }

        p_service = p_service->next;
    }
#endif
}

tunneled_accessory_t* tunnel_get_accessory_by_aid(uint16_t aid)
{
    tunneled_accessory_t* p_acc = tcb.acc_list_front;

    while (p_acc)
    {
        if (p_acc->ip_acc.instance_id == aid)
        {
            return p_acc;
        }
        p_acc = p_acc->next;
    }

    return NULL;
}

tunneled_accessory_t* tunnel_get_accessory_by_conn_id(uint16_t conn_id)
{
    tunneled_accessory_t* p_acc = tcb.acc_list_front;

    while (p_acc)
    {
        if (p_acc->conn_id == conn_id)
        {
            return p_acc;
        }
        p_acc = p_acc->next;
    }

    return NULL;
}

tunneled_accessory_t* tunnel_get_accessory_by_bdaddr(uint8_t* bd_addr)
{
    tunneled_accessory_t* p_acc = tcb.acc_list_front;

    while (p_acc)
    {
        if (memcmp(p_acc->bd_addr, bd_addr, BD_ADDR_LEN) == 0)
        {
            return p_acc;
        }
        p_acc = p_acc->next;
    }

    return NULL;
}

tunneled_characteristic_t* tunnel_get_characteristic_by_iid(tunneled_accessory_t* p_acc,
        uint16_t iid)
{
    tunneled_service_t* p_service = p_acc->svc_list_front;

    while (p_service)
    {
        tunneled_characteristic_t* p_char = p_service->char_list_front;

        while (p_char)
        {
            if (p_char->inst_id == iid)
                return p_char;
            p_char = p_char->next;
        }
        p_service = p_service->next;
    }
    return NULL;
}

tunneled_characteristic_t* tunnel_get_characteristic_by_ble_handle(tunneled_accessory_t* p_acc,
        uint16_t handle)
{
    tunneled_service_t* p_service = p_acc->svc_list_front;

    while (p_service)
    {
        tunneled_characteristic_t* p_char = p_service->char_list_front;

        while (p_char)
        {
            if (p_char->handle == handle || p_char->ccc_handle == handle)
                return p_char;
            p_char = p_char->next;
        }
        p_service = p_service->next;
    }
    return NULL;
}

tunneled_characteristic_t* tunnel_get_service_characteristic_by_uuid(tunneled_accessory_t* p_acc,
        uint16_t svc_uuid, uint16_t char_uuid)
{
    tunneled_service_t* p_service = p_acc->svc_list_front;

    while (p_service)
    {
        if (p_service->uuid == svc_uuid)
        {
            tunneled_characteristic_t* p_char = p_service->char_list_front;

            while (p_char)
            {
                if (p_char->uuid == char_uuid)
                    return p_char;
                p_char = p_char->next;
            }
            break;
        }
        p_service = p_service->next;
    }
    return NULL;
}

int tunnel_base64_to_raw_data(uint8_t* p_src, uint16_t src_len, uint8_t* p_dst, uint16_t dst_len)
{
    uint8_t* p_buf = GKI_getbuf(src_len + 1);
    uint8_t* p1 = p_src;
    uint8_t* p2 = p_buf;
    int ret;

    if (!p_buf)
        return -1;

    /* Remove backslash '\' */
    for (int i = 0; i < src_len; i++)
    {
        if (*p1 == '\\')
            p1++;
        else
            *p2++ = *p1++;
    }
    *p2 = 0;

    ret = base64_decode(p_buf, -1, p_dst, dst_len, BASE64_STANDARD);
    GKI_freebuf(p_buf);

    return ret;
}

//
// Tunnel notification event session management
//
typedef struct
{
    uint16_t                        accessory_id;
    uint16_t                        characteristic_id;
    wiced_http_response_stream_t*   stream;
} tunnel_event_session_t;

#define TUNNEL_MAX_EVENT_SESSIONS   100
typedef struct
{
    int                             num_of_sessions;
    int                             search_index;
    tunnel_event_session_t          sessions[TUNNEL_MAX_EVENT_SESSIONS];
} tunnel_event_control_t;

tunnel_event_control_t ev_ctrl = { 0 };

static void tunnel_delete_event_session(int index)
{
    if (index >= 0 && ev_ctrl.num_of_sessions > 0)
    {
        ev_ctrl.sessions[index] = ev_ctrl.sessions[--ev_ctrl.num_of_sessions];
    }
}

static wiced_result_t tunnel_save_event_session(uint16_t accessory_id, uint16_t characteristic_id, wiced_http_response_stream_t* stream)
{
    wiced_http_response_stream_t* temp = tunnel_find_first_event_session(accessory_id, characteristic_id);

    while (temp)
    {
        if (temp == stream)
            return WICED_SUCCESS;
        temp = tunnel_find_next_event_session(accessory_id, characteristic_id);
    }

    if (ev_ctrl.num_of_sessions >= TUNNEL_MAX_EVENT_SESSIONS)
        return WICED_OUT_OF_HEAP_SPACE;

    ev_ctrl.sessions[ev_ctrl.num_of_sessions].accessory_id = accessory_id;
    ev_ctrl.sessions[ev_ctrl.num_of_sessions].characteristic_id = characteristic_id;
    ev_ctrl.sessions[ev_ctrl.num_of_sessions].stream = stream;
    ev_ctrl.num_of_sessions++;

    return WICED_SUCCESS;
}

static wiced_result_t tunnel_remove_event_session(uint16_t accessory_id, uint16_t characteristic_id, wiced_http_response_stream_t* stream)
{
    int i;

    for (i = 0; i < ev_ctrl.num_of_sessions; i++)
    {
        if (ev_ctrl.sessions[i].accessory_id == accessory_id &&
            ev_ctrl.sessions[i].characteristic_id == characteristic_id &&
            ev_ctrl.sessions[i].stream == stream)
        {
            tunnel_delete_event_session(i);
            return WICED_SUCCESS;
        }
    }

    return WICED_NOT_FOUND;
}

static wiced_result_t tunnel_event_sessions_remove_accessory(uint16_t accessory_id)
{
    int i;

    for (i = 0; i < ev_ctrl.num_of_sessions; )
    {
        if (ev_ctrl.sessions[i].accessory_id == accessory_id)
        {
            tunnel_delete_event_session(i);
        }
        else
        {
            i++;
        }
    }

    return WICED_SUCCESS;
}

static wiced_result_t tunnel_event_sessions_remove_controller(wiced_http_response_stream_t* stream)
{
    int i;

    for (i = 0; i < ev_ctrl.num_of_sessions; )
    {
        if (ev_ctrl.sessions[i].stream == stream)
        {
            tunnel_delete_event_session(i);
        }
        else
        {
            i++;
        }
    }

    return WICED_SUCCESS;
}

static wiced_result_t tunnel_event_sessions_remove_all()
{
    ev_ctrl.num_of_sessions = 0;

    return WICED_SUCCESS;
}

static wiced_http_response_stream_t* tunnel_find_first_event_session(uint16_t accessory_id, uint16_t characteristic_id)
{
    ev_ctrl.search_index = 0;

    return tunnel_find_next_event_session(accessory_id, characteristic_id);
}

static wiced_http_response_stream_t* tunnel_find_next_event_session(uint16_t accessory_id, uint16_t characteristic_id)
{
    int i;

    for (i = ev_ctrl.search_index; i < ev_ctrl.num_of_sessions; i++)
    {
        if (ev_ctrl.sessions[i].accessory_id == accessory_id &&
            ev_ctrl.sessions[i].characteristic_id == characteristic_id)
        {
            ev_ctrl.search_index = i + 1;
            return ev_ctrl.sessions[i].stream;
        }
    }

    return NULL;
}

// Tunnel notification event session management ends
