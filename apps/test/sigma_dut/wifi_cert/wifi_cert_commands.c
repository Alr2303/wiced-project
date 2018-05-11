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
/**
 * Console commands for Wi-Fi certification
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <wiced_wifi.h>
#include <string.h>
#include "WWD/internal/wwd_sdpcm.h"
#include "wifi_cert_ping.h"
#include "command_console.h"
#include "wiced_network.h"
#include "wiced_management.h"
#include "wwd_management.h"
#include "wwd_buffer_interface.h"
#include "wifi/wifi.h"
#include "wifi_cert_traffic_stream.h"
#include "compat.h"
#ifdef WICED_USE_WIFI_P2P_INTERFACE
#include "wiced_p2p.h"
#include "wiced_wps.h"
#include "platform_dct.h"
#include "wiced_framework.h"
#include "p2p_frame_writer.h"
#include "wiced_utilities.h"
#endif

// Global defines
#define VENDOR "Cypress"
#define MODEL PLATFORM
#define PLATFORM_VERSION "1"
#define SOFTWARE_VERSION WICED_VERSION
#define TEST_TX_POWER 80
#define MAX_SSID_LEN 32


// Global variables
static char test_passphrase[64]   = "12345678";
static char test_encptype[16]     = "none";
static char test_keymgmttype[16]  = "wpa2";
static char test_using_dhcp[4]    = "1";
static char dut_ip_addr[16]       = "0.0.0.0";
static char dut_netmask[16]       = "0.0.0.0";
static char dut_gateway[16]       = "0.0.0.0";
static char dut_primary_dns[16]   = "0.0.0.0";
static char dut_secondary_dns[16] = "0.0.0.0";
static char test_ssid[33]         = "Cypress";
static wiced_wep_key_t wep_key;
static uint32_t stream_id         = 0; // stream_id is used by Sigma to track which streams to start/stop etc

traffic_stream_t stream_table[NUM_STREAM_TABLE_ENTRIES] = { { 0 }, { 0 }, { 0 }, { 0 } };
static edcf_acparam_t ac_params[AC_COUNT];
int ac_priority[AC_COUNT];
int tx_ac_priority_num[AC_COUNT];
int rx_ac_priority_num[AC_COUNT];
static wiced_worker_thread_t          ping_worker_thread;
static wiced_security_t auth_type;

#ifndef WICED_USE_WIFI_P2P_INTERFACE
static wiced_semaphore_t    scan_complete_semaphore;
#endif

#ifdef WICED_USE_WIFI_P2P_INTERFACE
static p2p_workspace_t                p2p_workspace;
static besl_p2p_device_detail_t device_details =
{
    .wps_device_details =
    {
        .device_name     = "WICED_P2P",
        .manufacturer    = "Cypress",
        .model_name      = "BCM943362",
        .model_number    = "Wiced",
        .serial_number   = "12345670",
        .device_category = WICED_WPS_DEVICE_COMPUTER,
        .sub_category    = 7,
        .config_methods  = WPS_CONFIG_PUSH_BUTTON | WPS_CONFIG_VIRTUAL_PUSH_BUTTON | WPS_CONFIG_DISPLAY | WPS_CONFIG_VIRTUAL_DISPLAY_PIN | WPS_CONFIG_KEYPAD,
        .authentication_type_flags = WPS_OPEN_AUTHENTICATION | WPS_WPA_PSK_AUTHENTICATION | WPS_WPA2_PSK_AUTHENTICATION,
        .encryption_type_flags     = WPS_NO_ENCRYPTION | WPS_MIXED_ENCRYPTION,
    },
    .listen_channel =
    {
        .country_string  = "XX\x04",
        .operating_class = 81,
        .channel         = 1,
    },
    .operating_channel =
    {
        .country_string  = "XX\x04",
        .operating_class = 81,
        .channel         = 6,
    },
    .channel_list =
    {
        .country_string  = "XX\x04",
        .operating_class = 81,
        .number_of_channels = 11,
        .channel_list    = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
    },
    .group_owner_intent = 1,
    .go_configuration_timeout = 100,    /* 1000 milliseconds (Units of 10 milliseconds) */
    .client_configuration_timeout = 50, /* 500 milliseconds */
    .device_password_id = WPS_DEFAULT_DEVICEPWDID,
    .peer_device_timeout = 60000,
    .group_formation_timeout = 30000,
    .p2p_capability = 0x0820,           /* Intra BSS Distribution, Invitation Procedure */
};
#endif


/* Forward declarations */
wiced_result_t wiced_wifi_get_bssid( wiced_mac_t * mac );
wiced_security_t get_authtype( char* encptype, char* keymgmttype );
void init_security( void );
void init_ipconfig( void );
void init_stream_table( void );
int find_unallocated_stream_table_entry ( void );
int find_stream_table_entry ( int id );
uint8_t a_to_x(uint8_t value);
wiced_bool_t is_hex_digit(char c);
void dump_bytes(const uint8_t* bptr, uint32_t len);
wiced_result_t association_scan_result_handler( wiced_scan_handler_result_t* malloced_scan_result );

#ifdef WICED_USE_WIFI_P2P_INTERFACE
wiced_result_t my_set_data_rate( uint32_t rate, wwd_interface_t interface );
static void p2p_group_formation_result_handler( void* ws );
static void p2p_connection_result_handler( void* ws );
static void p2p_go_start_result_handler( void* ws );
void p2p_connection_request_callback( p2p_discovered_device_t* device );
#endif

// External declarations
extern int show_ip_config( uint8_t interface, char* test_using_dhcp );
extern int spawn_ts_thread( void *ts_function, traffic_stream_t *ts );


int show_ip_config( uint8_t interface, char* test_using_dhcp )
{
    wiced_result_t result;
    wiced_ip_address_t ipv4_address;
    wiced_ip_address_t ipv4_netmask;

    printf( "status,COMPLETE,dhcp,%s,", test_using_dhcp );

    result = wiced_ip_get_ipv4_address( interface, &ipv4_address );
    if ( result != WICED_SUCCESS )
    {
        return -1;
    }

    result = wiced_ip_get_netmask( interface, &ipv4_netmask );
    if ( result != WICED_SUCCESS )
    {
        return -2;
    }


    if ( wiced_network_is_up( interface ) == WICED_TRUE )
    {
        printf( "ip,%u.%u.%u.%u,",   (unsigned char) ( ( ipv4_address.ip.v4 >> 24 ) & 0xff ), (unsigned char) ( ( ipv4_address.ip.v4 >> 16 ) & 0xff ), (unsigned char) ( ( ipv4_address.ip.v4 >> 8 ) & 0xff ), ( unsigned char ) ( ( ipv4_address.ip.v4 >> 0 ) & 0xff ) );
        printf( "mask,%u.%u.%u.%u,", (unsigned char) ( ( ipv4_netmask.ip.v4 >> 24 ) & 0xff ), (unsigned char) ( ( ipv4_netmask.ip.v4 >> 16 ) & 0xff ), (unsigned char) ( ( ipv4_netmask.ip.v4 >> 8 ) & 0xff ), ( unsigned char ) ( ( ipv4_netmask.ip.v4 >> 0 ) & 0xff ) );
        printf( "primary-dns,%u.%u.%u.%u,", 0, 0, 0, 0 );
        if ( interface == WICED_STA_INTERFACE )
        {
            printf("secondary-dns,%u.%u.%u.%u\n", 0, 0, 0, 0 );
        }
        if ( interface == WICED_P2P_INTERFACE )
        {
            wiced_mac_t mac;
            wwd_wifi_get_mac_address( &mac, WICED_P2P_INTERFACE );
            printf("p2pinterfaceaddress,%.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]);
        }
        else
        {
            printf("\n");
        }

    }
    else
    {
        printf( "ip,%s,",   dut_ip_addr );
        printf( "mask,%s,", dut_netmask );
        printf( "primary-dns,%s,", dut_primary_dns );
        if ( interface == WICED_STA_INTERFACE )
        {
             printf("secondary-dns,%s\n", dut_secondary_dns );
        }
        if ( interface == WICED_P2P_INTERFACE )
        {
             wiced_mac_t mac;
             memset( &mac, 0, sizeof(mac));
             printf("p2pinterfaceaddress,%.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]);
        }
        else
        {
             printf("\n");
        }
    }
    return 0;
}

int sta_get_ip_config( int argc, char *argv[] )
{
    uint8_t interface = 0; // STA interface

    show_ip_config( interface, test_using_dhcp );

    return 0;
}

int sta_set_ip_config( int argc, char *argv[] )
{
    int i = 1;

    init_ipconfig();

    while (i < (argc - 1))
    {
        if ( strcmp( argv[i], "dhcp" ) == 0 )
        {
            strncpy(test_using_dhcp, argv[i+1], sizeof( test_using_dhcp ) - 1 );
        }
        else if (strcmp( argv[i], "ip" ) == 0 )
        {
            strncpy(dut_ip_addr, argv[i+1], sizeof( dut_ip_addr ) - 1 );
        }
        else if (strcmp( argv[i], "mask" ) == 0 )
        {
            strncpy(dut_netmask, argv[i+1], sizeof( dut_netmask ) - 1 );
        }
        else if ( ( strcmp( argv[i], "defaultgateway" ) == 0 ) || ( strcmp( argv[i], "defaultGateway" ) == 0 ) )
        {
            strncpy(dut_gateway, argv[i+1], sizeof( dut_gateway ) - 1 );
        }
        else if (strcmp( argv[i], "primary-dns" ) == 0 )
        {
            strncpy(dut_primary_dns, argv[i+1], sizeof( dut_primary_dns ) - 1 );
        }
        else if (strcmp( argv[i], "secondary-dns" ) == 0 )
        {
            strncpy(dut_secondary_dns, argv[i+1], sizeof( dut_secondary_dns ) - 1 );
        }

        i = i + 2;
    }

    printf("status,COMPLETE\n");

    return 0;
}

int sta_get_info( int argc, char *argv[] )
{
    wiced_mac_t mac;
    wiced_wifi_get_mac_address( &mac );

    printf("status,COMPLETE,vendor,%s,model,%s,version,%s,firmware,%s,mac,%02X:%02X:%02X:%02X:%02X:%02X\n",
            VENDOR, PLATFORM, PLATFORM_VERSION, SOFTWARE_VERSION, mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5] );
    return 0;
}

// Note that this only returns the STA interface MAC currently
int sta_get_mac_address( int argc, char *argv[] )
{
    wiced_mac_t mac;
    wiced_wifi_get_mac_address( &mac );

    printf("status,COMPLETE,mac,%02X:%02X:%02X:%02X:%02X:%02X\n", mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5] );

    return 0;
}


// Note that this only tests association
int sta_is_connected( int argc, char *argv[] )
{
    uint8_t interface;

    interface = 0; // Station interface

    if ( ( wwd_wifi_is_ready_to_transceive( interface ) == WWD_SUCCESS ) )
    {
        printf("status,COMPLETE,connected,1\n");
    }
    else
    {
        printf("status,COMPLETE,connected,0\n");
    }
    return 0;
}

int sta_verify_ip_connection( int argc, char *argv[] )
{
    return 0;
}

// Get the MAC address of the current AP or return all zeroes if not associated
int sta_get_bssid( int argc, char *argv[] )
{
    wiced_mac_t bssid;

    wiced_result_t res = wiced_wifi_get_bssid( &bssid );
    if (res == WICED_SUCCESS)
    {
        printf("status,COMPLETE,bssid,%02X:%02X:%02X:%02X:%02X:%02X\n", bssid.octet[0],bssid.octet[1],bssid.octet[2],bssid.octet[3],bssid.octet[4],bssid.octet[5]);
    }
    else
    {
        printf("status,COMPLETE,bssid,00:00:00:00:00:00\n");
    }

    return 0;
}


int device_get_info( int argc, char *argv[] )
{
    printf("status,COMPLETE,vendor,%s,model,%s,version,%s\n", VENDOR, PLATFORM, PLATFORM_VERSION);

    stream_id = 0;

    return 0;
}

int device_list_interfaces( int argc, char *argv[] )
{
    printf("status,COMPLETE,interfaceType,802.11,interfaceID,wlan0\n");
    return 0;
}

int sta_set_encryption( int argc, char *argv[] )
{
    int i = 1;

    memset(test_ssid, 0, sizeof( test_ssid ));
    init_security();

    while (i < (argc - 1))
    {
        if ( strcmp( argv[i], "ssid" ) == 0 )
        {
            strncpy(test_ssid, argv[i+1], sizeof( test_ssid ) - 1 );
        }
        else if ( ( strcmp( argv[i], "encptype" ) == 0 ) || ( strcmp( argv[i], "encpType" ) == 0 ) )
        {
            strncpy(test_encptype, argv[i+1], sizeof( test_encptype ) - 1 );
        }
        else if ( ( strcmp( argv[i], "key1" ) == 0 ) ||
                  ( strcmp( argv[i], "key2" ) == 0 ) ||
                  ( strcmp( argv[i], "key3" ) == 0 ) ||
                  ( strcmp( argv[i], "key4" ) == 0 ) )
        {
            int j = 0;
            wep_key.index = argv[i][3] - 0x31; // Index for key1 is 0, for key2 is 1, etc
            wep_key.length = ( strlen( argv[i+1] ) ) / 2;
            // Convert key from hex characters to hex value
            while ( j < ( wep_key.length * 2 ) )
            {
                wep_key.data[j/2] = a_to_x(argv[i+1][j]) << 4;
                wep_key.data[j/2] = wep_key.data[j/2] | a_to_x(argv[i+1][j+1]);
                j = j + 2;
            }
            //dump_bytes( &wep_key.data[0], wep_key.length );
        }

        i = i + 2;
    }

    printf("status,COMPLETE\n");

    return 0;
}

int sta_set_psk( int argc, char *argv[] )
{
    int i = 1;

    init_security();
    memset(test_ssid, 0, sizeof( test_ssid ));

    while (i < (argc - 1))
    {
        if ( strcmp( argv[i], "ssid" ) == 0 )
        {
            strncpy(test_ssid, argv[i+1], sizeof( test_ssid ) - 1 );
        }
        else if (strcmp( argv[i], "passphrase" ) == 0 )
        {
            strncpy(test_passphrase, argv[i+1], sizeof( test_passphrase ) - 1 );
        }
        else if ( ( strcmp( argv[i], "encptype" ) == 0 ) || ( strcmp( argv[i], "encpType" ) == 0 ) )
        {
            strncpy(test_encptype, argv[i+1], sizeof( test_encptype ) - 1 );
        }
        else if (strcmp( argv[i], "keymgmttype" ) == 0 )
        {
            strncpy(test_keymgmttype, argv[i+1], sizeof( test_keymgmttype ) - 1 );
        }
        i = i + 2;
    }

    printf("status,COMPLETE\n");

    return 0;
}

int sta_associate( int argc, char *argv[] )
{
    /* Attempt to join the Wi-Fi network */
    uint8_t*         security_key;
    uint8_t          key_length;
    uint8_t          wep_key_buffer[64] = { 0 };

    if ( ( wwd_wifi_is_ready_to_transceive( 0 ) == WWD_SUCCESS ) )
    {
        wiced_network_down( WICED_STA_INTERFACE );
    }

    // Set the tx power
    wwd_result_t ret = wwd_wifi_set_tx_power( TEST_TX_POWER );
    if (ret != WWD_SUCCESS)
    {
        printf( "problem setting tx power\n" );
    }

    auth_type = get_authtype( test_encptype, test_keymgmttype );

    if ( auth_type == WICED_SECURITY_WEP_PSK )
    {
        wiced_wep_key_t* temp_wep_key = (wiced_wep_key_t*)wep_key_buffer;
        key_length = wep_key.length;

        /* Setup WEP key 0 */
        temp_wep_key[0].index = 0;
        temp_wep_key[0].length = wep_key.length;
        memcpy(temp_wep_key[0].data, wep_key.data, wep_key.length);

        /* Setup WEP keys 1 to 3 */
        memcpy(wep_key_buffer + 1*(2 + key_length), temp_wep_key, (2 + key_length));
        memcpy(wep_key_buffer + 2*(2 + key_length), temp_wep_key, (2 + key_length));
        memcpy(wep_key_buffer + 3*(2 + key_length), temp_wep_key, (2 + key_length));
        wep_key_buffer[1*(2 + key_length)] = 1;
        wep_key_buffer[2*(2 + key_length)] = 2;
        wep_key_buffer[3*(2 + key_length)] = 3;

        security_key = wep_key_buffer;
        key_length = 4*(2 + key_length);
    }
    else if ( ( auth_type != WICED_SECURITY_OPEN ) && ( argc < 4 ) )
    {
        printf("Error: Missing security key\n" );
        return ERR_UNKNOWN;
    }
    else
    {
        security_key = (uint8_t*)test_passphrase;
        key_length = strlen((char*)security_key);

#ifndef WICED_USE_WIFI_P2P_INTERFACE
        /* Initialise scan complete semaphore */
        if ( wiced_rtos_init_semaphore( &scan_complete_semaphore ) != WICED_SUCCESS )
        {
             printf( "Problem initialising scan complete semaphore\n" );
        }
        /* Scan for test SSID and possibly modify authentication type in cases of WPA2 versus WPA2/WPA mixed mode versus WPA */
        wiced_wifi_scan_networks(association_scan_result_handler, NULL );
        /* Block until scan is complete */
        if ( wiced_rtos_get_semaphore( &scan_complete_semaphore, 5000 ) != WICED_SUCCESS )
        {
             printf( "Problem getting scan complete semaphore\n" );
        }
        wiced_rtos_deinit_semaphore( &scan_complete_semaphore );
#endif /* WICED_USE_WIFI_P2P_INTERFACE */
    }

    if ( strcmp( test_using_dhcp, "1" ) == 0 )
    {
        if (WICED_SUCCESS != wifi_join(test_ssid, strlen(test_ssid), auth_type, (uint8_t*) security_key, key_length, NULL, NULL, NULL ))
        {
            printf( "status,COMPLETE\n" );
            return 0;

        }
    }
    else
    {
        if (WICED_SUCCESS != wifi_join(test_ssid, strlen(test_ssid), auth_type, (uint8_t*) security_key, key_length, dut_ip_addr, dut_netmask, dut_gateway ))
        {
            printf( "status,COMPLETE\n" );
            return 0;
        }
    }

    printf( "status,COMPLETE\n" );

    if ( wwd_wifi_get_acparams_sta( ac_params ) != WWD_SUCCESS )
    {
        printf( "Error reading EDCF AC Parameters\n");
    }

    wwd_wifi_prioritize_acparams( ac_params, ac_priority ); // Re-prioritize access categories to match AP configuration
//    print_ac_params( ac_params, ac_priority );

    return 0;
}

/*
 * Callback function to handle scan results
 */
wiced_result_t association_scan_result_handler( wiced_scan_handler_result_t* malloced_scan_result )
{
    if ( malloced_scan_result != NULL )
    {
        malloc_transfer_to_curr_thread( malloced_scan_result );

        if ( malloced_scan_result->status == WICED_SCAN_INCOMPLETE )
        {
            wiced_scan_result_t* record = &malloced_scan_result->ap_details;

            if ( memcmp( record->SSID.value, test_ssid, strlen( test_ssid ) ) == 0 )
            {
                //print_scan_result(record);
                /* This is required to pass test 5.2.53 */
                if ( ( auth_type == WICED_SECURITY_WPA2_MIXED_PSK ) && ( record->security == WICED_SECURITY_WPA_TKIP_PSK ) )
                {
                    auth_type = record->security;
                }
            }
        }
#ifndef WICED_USE_WIFI_P2P_INTERFACE
        else
        {
            if ( wiced_rtos_set_semaphore( &scan_complete_semaphore ) != WICED_SUCCESS )
            {
                printf( "Problem setting scan complete semaphore\n");
            }
        }
#endif /* WICED_USE_WIFI_P2P_INTERFACE */
        free( malloced_scan_result );
        malloced_scan_result = NULL;
    }

    return WICED_SUCCESS;
}

#ifdef WICED_USE_WIFI_P2P_INTERFACE
int sta_p2p_reset( int argc, char *argv[] )
{
    wiced_network_down( WICED_STA_INTERFACE ); // The STA interface will be up after some tests have run
    wiced_network_down( WICED_P2P_INTERFACE );
    if ( p2p_workspace.p2p_initialised )
    {
        if ( besl_p2p_deinit( &p2p_workspace ) != 0 )
        {
            printf(("Error when disabling P2P discovery\n"));
        }
    }

    memset( &p2p_workspace, 0, sizeof(p2p_workspace_t) );

    printf( "status,COMPLETE\n" );

    return 0;
}

int sta_p2p_dissolve( int argc, char* argv[] )
{
    if ( besl_p2p_deinit( &p2p_workspace ) != 0 )
    {
        printf("Error when stopping P2P group owner\n");
    }

    wiced_rtos_delay_milliseconds( 100 );

    printf( "status,COMPLETE\n" );

    return 0;
}

int sta_get_p2p_dev_address( int argc, char *argv[] )
{
    wiced_mac_t mac;

    /* Create P2P MAC address */
    wiced_wifi_get_mac_address( &mac );
    mac.octet[0] |= 0x2;

    printf( "status,COMPLETE,DevID,%02X:%02X:%02X:%02X:%02X:%02X\n", mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5] );

    return 0;
}

int sta_set_p2p( int argc, char *argv[] )
{
    int i = 1;
    wiced_bool_t initialize = WICED_FALSE;
    uint8_t      form_persistent_group = 0;

    while (i < (argc - 1))
    {
        if ( strcmp( argv[i], "p2p_mode" ) == 0 )
        {
            if ( ( strcmp( argv[i+1], "discover" ) == 0 ) || ( strcmp( argv[i+1], "Listen" ) == 0 ) || ( strcmp( argv[i+1], "listen" ) == 0 ) )
            {
                initialize = WICED_TRUE;
            }
        }
        else if ( strcmp( argv[i], "listen_chn" ) == 0 )
        {
            if ( p2p_workspace.p2p_initialised !=  1 )
            {
                device_details.listen_channel.channel = (uint8_t)atoi(argv[i+1]);
            }
            else
            {
                p2p_workspace.listen_channel.channel =  (uint8_t)atoi(argv[i+1]);
            }
        }
        else if ( strcmp( argv[i], "persistent" ) == 0 )
        {
            form_persistent_group = (uint8_t)atoi(argv[i+1]);
        }
        i += 2;
    }

    if ( ( initialize == WICED_TRUE ) && ( p2p_workspace.p2p_initialised != 1 ) )
    {
        besl_p2p_init( &p2p_workspace, &device_details );
        if ( form_persistent_group == 1 )
        {
            p2p_workspace.form_persistent_group = 1;
            p2p_workspace.p2p_capability |= P2P_GROUP_CAPABILITY_P2P_PERSISTENT_GROUP;
        }
        besl_p2p_register_p2p_device_connection_callback( &p2p_workspace, p2p_connection_request_callback);
        besl_p2p_start( &p2p_workspace );
    }

    printf( "status,COMPLETE\n" );

    return 0;
}

int sta_p2p_start_group_formation( int argc, char *argv[] )
{
    int i = 1;
    uint8_t operating_channel = 1;
    uint8_t group_owner_intent = 1;
    uint8_t initiate_negotiation = 0;
    wiced_mac_t mac;
    char mac_string[20] = { 0 };

    while (i < (argc - 1))
    {
        if ( strcmp( argv[i], "intent_val" ) == 0 )
        {
            group_owner_intent = (uint8_t)atoi(argv[i+1]);
        }
        else if ( strcmp( argv[i], "oper_chn" ) == 0 )
        {
            operating_channel = (uint8_t)atoi(argv[i+1]);
        }
        else if ( strcmp( argv[i], "p2pdevid" ) == 0 )
        {
            strncpy(mac_string, argv[i+1], sizeof(mac_string));
        }
        else if ( strcmp( argv[i], "init_go_neg" ) == 0 )
        {
            initiate_negotiation = (uint8_t)atoi(argv[i+1]);
        }
        i += 2;
    }

    i = 0;
    int j = 0;
    while ( i < 17 ) // MAC string format is "00:11:22:33:44:55" which becomes 0x001122334455
    {
        mac.octet[j] = a_to_x(mac_string[i]) << 4;
        mac.octet[j] = mac.octet[j] | a_to_x(mac_string[i+1]);
        j ++;
        i += 3;
    }

    p2p_workspace.group_owner_intent = group_owner_intent;
    p2p_workspace.operating_channel.channel = operating_channel;
    memcpy( &p2p_workspace.group_candidate.p2p_device_address, &mac, sizeof( wiced_mac_t ) );
    p2p_workspace.initiate_negotiation = initiate_negotiation;
    p2p_workspace.ok_to_accept_negotiation = 1;
    besl_p2p_register_group_formation_result_callback( &p2p_workspace, p2p_group_formation_result_handler);

    if ( p2p_workspace.initiate_negotiation == 0 )
    {
        printf( "status,COMPLETE\n" );
    }

    if ( besl_p2p_start_negotiation( &p2p_workspace ) != 0 )
    {
        printf("besl_p2p_start_negotiation failed\n");
        return 0;
    }

    return 0;
}

int sta_start_autonomous_go( int argc, char *argv[] )
{
    int i = 1;
    int ssid_suffix_length = 0;

    if ( p2p_workspace.p2p_initialised )
    {
        besl_p2p_deinit( &p2p_workspace );
    }

    memset(&p2p_workspace, 0, sizeof(p2p_workspace_t));
    p2p_workspace.i_am_group_owner = 1;
    besl_p2p_init_common( &p2p_workspace, &device_details );

    while (i < (argc - 1))
    {
        if ( strcmp( argv[i], "oper_chn" ) == 0 )
        {
            p2p_workspace.operating_channel.channel = (uint8_t)atoi(argv[i+1]);
        }
        if ( strcmp( argv[i], "SSID" ) == 0 )
        {
            ssid_suffix_length = strlen( argv[i+1] );
            if ( ( p2p_workspace.group_candidate.ssid_length + ssid_suffix_length ) <= MAX_SSID_LEN )
            {
                memcpy( &p2p_workspace.group_candidate.ssid[p2p_workspace.group_candidate.ssid_length], argv[i+1], ssid_suffix_length );
                p2p_workspace.group_candidate.ssid_length += ssid_suffix_length;
            }
        }
        i = i + 2;
    }

    /* Set the device details */
    p2p_workspace.wps_device_details = &device_details.wps_device_details;
    p2p_workspace.device_name_length = strlen(device_details.wps_device_details.device_name);
    memcpy(&p2p_workspace.device_name, device_details.wps_device_details.device_name, p2p_workspace.device_name_length);
    besl_p2p_register_group_formation_result_callback( &p2p_workspace, p2p_go_start_result_handler);

    if ( ( besl_p2p_group_owner_start(&p2p_workspace) ) != BESL_SUCCESS )
    {
        printf( ( "Error starting group owner\n" ) );
        besl_p2p_deinit( &p2p_workspace );
    }

    return 0;
}

int sta_get_psk( int argc, char *argv[] )
{
    if ( p2p_workspace.p2p_passphrase_length != 64 )
    {
        printf( "status,COMPLETE,passPhrase,%s,ssid,%s\n", p2p_workspace.p2p_passphrase, p2p_workspace.group_candidate.ssid );
    }
    else
    {
        printf( "status,COMPLETE,passPhrase,%.64s,ssid,%s\n", p2p_workspace.p2p_passphrase, p2p_workspace.group_candidate.ssid );
    }

    return 0;
}


int sta_p2p_connect( int argc, char *argv[] )
{
    int i = 1;
    char p2p_device_address[18] = { 0 };
    char p2p_interface_address[18] = { 0 };
    char group_ssid[33] = { 0 };

    while (i < (argc - 1))
    {
        if ( strcmp( argv[i], "p2pdevid" ) == 0 )
        {
            memcpy(p2p_device_address, argv[i+1], 17);
        }
        else if ( strcmp( argv[i], "groupid" ) == 0 )
        {
            memcpy(p2p_interface_address, argv[i+1], 17);
            strncpy(group_ssid, &argv[i+1][18], sizeof(group_ssid));
        }
        i += 2;
    }

    i = 0;
    int j = 0;
    while ( i < 17 ) // MAC string format is "00:11:22:33:44:55" which becomes 0x001122334455
    {
        p2p_workspace.group_candidate.p2p_device_address.octet[j] = a_to_x(p2p_device_address[i]) << 4;
        p2p_workspace.group_candidate.p2p_device_address.octet[j] = p2p_workspace.group_candidate.p2p_device_address.octet[j] | a_to_x(p2p_device_address[i+1]);
        j ++;
        i += 3;
    }

    p2p_workspace.group_candidate.ssid_length = strlen( group_ssid );
    memcpy( p2p_workspace.group_candidate.ssid, group_ssid, p2p_workspace.group_candidate.ssid_length );

    if ( p2p_workspace.p2p_wps_device_password_id == WPS_PUSH_BTN_DEVICEPWDID )
    {
        p2p_workspace.provisioning_config_method = PUSH_BUTTON;
    }
    else if ( p2p_workspace.p2p_wps_device_password_id == WPS_DEVICEPWDID_REG_SPEC )
    {
        p2p_workspace.provisioning_config_method = KEYPAD;
    }
    else if ( p2p_workspace.p2p_wps_device_password_id == WPS_USER_SPEC_DEVICEPWDID )
    {
        p2p_workspace.provisioning_config_method = DISPLAY;
    }

    besl_p2p_register_group_formation_result_callback( &p2p_workspace, p2p_connection_result_handler);
    besl_p2p_find_group_owner( &p2p_workspace );

    return 0;
}


int sta_send_p2p_invitation_req( int argc, char *argv[] )
{
    int i = 1;
    int j = 0;
    int reinvoke_persistent_group = 0;
    char p2p_device_address[18] = { 0 };
    char p2p_interface_address[18] = { 0 };
    char group_ssid[33] = { 0 };
    besl_mac_t invitee_device_address;
    platform_dct_wifi_config_t* dct_wifi_config;
    platform_dct_p2p_config_t* dct_p2p_config;

    while (i < (argc - 1))
    {
        if ( strcmp( argv[i], "p2pdevid" ) == 0 )
        {
            memcpy(p2p_device_address, argv[i+1], 17);
        }
        else if ( strcmp( argv[i], "groupid" ) == 0 )
        {
            memcpy(p2p_interface_address, argv[i+1], 17);
            strncpy(group_ssid, &argv[i+1][18], sizeof(group_ssid));
        }
        else if ( strcmp( argv[i], "reinvoke" ) == 0 )
        {
            reinvoke_persistent_group = (uint8_t)atoi(argv[i+1]);
        }
        i += 2;
    }

    i = 0;
    j = 0;
    while ( i < 17 ) // MAC string format is "00:11:22:33:44:55" which becomes 001122334455
    {
        invitee_device_address.octet[j] = a_to_x(p2p_device_address[i]) << 4;
        invitee_device_address.octet[j] = invitee_device_address.octet[j] | a_to_x(p2p_device_address[i+1]);
        j ++;
        i += 3;
    }

    p2p_workspace.candidate_device = besl_p2p_host_find_device(&p2p_workspace, &invitee_device_address );

    if ( p2p_workspace.candidate_device != NULL )
    {
        if ( reinvoke_persistent_group )
        {
            p2p_workspace.reinvoking_group = 1;
            p2p_workspace.invitation_flags |= P2P_INVITATION_REINVOKE_PERSISTENT_GROUP;
            i = 0;
            j = 0;
            while ( i < 17 ) // MAC string format is "00:11:22:33:44:55" which becomes 001122334455
            {
                p2p_workspace.group_candidate.p2p_device_address.octet[j] = a_to_x(p2p_device_address[i]) << 4;
                p2p_workspace.group_candidate.p2p_device_address.octet[j] = p2p_workspace.group_candidate.p2p_device_address.octet[j] | a_to_x(p2p_device_address[i+1]);
                j ++;
                i += 3;
            }
            i = 0;
            j = 0;
            while ( i < 17 ) // MAC string format is "00:11:22:33:44:55" which becomes 001122334455
            {
                p2p_workspace.group_candidate.bssid.octet[j] = a_to_x(p2p_interface_address[i]) << 4;
                p2p_workspace.group_candidate.bssid.octet[j] = p2p_workspace.group_candidate.bssid.octet[j] | a_to_x(p2p_interface_address[i+1]);
                j ++;
                i += 3;
            }

            if ( memcmp( &p2p_workspace.group_candidate.bssid, &p2p_workspace.p2p_interface_address, sizeof( besl_mac_t ) ) == 0 )
            {
                /* Reinvoke our group */
                wiced_dct_read_lock( (void**) &dct_p2p_config, WICED_TRUE, DCT_P2P_CONFIG_SECTION, 0, sizeof(platform_dct_p2p_config_t) );
                p2p_workspace.i_am_group_owner = 1;
                p2p_workspace.form_persistent_group = 1;
                memcpy( &p2p_workspace.persistent_group, &dct_p2p_config->p2p_group_owner_settings, sizeof( wiced_config_soft_ap_t ) );
                p2p_workspace.group_candidate.ssid_length = p2p_workspace.persistent_group.SSID.length;
                memcpy( &p2p_workspace.group_candidate.ssid, p2p_workspace.persistent_group.SSID.value, p2p_workspace.group_candidate.ssid_length );
                p2p_workspace.p2p_passphrase_length = p2p_workspace.persistent_group.security_key_length;
                memcpy( p2p_workspace.p2p_passphrase, p2p_workspace.persistent_group.security_key, p2p_workspace.p2p_passphrase_length );
                p2p_workspace.operating_channel.channel = p2p_workspace.persistent_group.channel;
                memcpy( &p2p_workspace.group_candidate.operating_channel, &p2p_workspace.operating_channel, sizeof( p2p_channel_info_t ));
                besl_p2p_register_group_formation_result_callback( &p2p_workspace, p2p_go_start_result_handler);
                wiced_dct_read_unlock( (void*) dct_p2p_config, WICED_TRUE );
            }
            else
            {
                /* Reinvoke their group XXX should check against workspace invitation candidate */
                wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
                p2p_workspace.group_candidate.ssid_length = dct_wifi_config->stored_ap_list[0].details.SSID.length;
                memcpy( p2p_workspace.group_candidate.ssid, dct_wifi_config->stored_ap_list[0].details.SSID.value, p2p_workspace.group_candidate.ssid_length );
                memcpy( &p2p_workspace.group_candidate.bssid, &dct_wifi_config->stored_ap_list[0].details.BSSID, sizeof( besl_mac_t ) );
                p2p_workspace.p2p_passphrase_length = dct_wifi_config->stored_ap_list[0].security_key_length;
                memcpy( p2p_workspace.p2p_passphrase, dct_wifi_config->stored_ap_list[0].security_key, p2p_workspace.p2p_passphrase_length);
                memcpy( &p2p_workspace.group_candidate.operating_channel, &p2p_workspace.candidate_device->operating_channel, sizeof( p2p_channel_info_t ));
                besl_p2p_register_group_formation_result_callback( &p2p_workspace, p2p_connection_result_handler);
                wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );
            }
        }
        besl_p2p_send_action_frame( &p2p_workspace, p2p_workspace.candidate_device, p2p_write_invitation_request, p2p_workspace.current_channel, 2 );
    }

    wiced_rtos_delay_milliseconds( 100 ); // Delay to allow frame to be sent
    printf( "status,COMPLETE\n");
//    printf( "status,COMPLETE, %02X:%02X:%02X:%02X:%02X:%02X %02X:%02X:%02X:%02X:%02X:%02X\n", p2p_workspace.p2p_interface_address.octet[0],
//                                                                     p2p_workspace.p2p_interface_address.octet[1],
//                                                                     p2p_workspace.p2p_interface_address.octet[2],
//                                                                     p2p_workspace.p2p_interface_address.octet[3],
//                                                                     p2p_workspace.p2p_interface_address.octet[4],
//                                                                     p2p_workspace.p2p_interface_address.octet[5],
//                                                                     p2p_workspace.group_candidate.bssid.octet[0],
//                                                                     p2p_workspace.group_candidate.bssid.octet[1],
//                                                                     p2p_workspace.group_candidate.bssid.octet[2],
//                                                                     p2p_workspace.group_candidate.bssid.octet[3],
//                                                                     p2p_workspace.group_candidate.bssid.octet[4],
//                                                                     p2p_workspace.group_candidate.bssid.octet[5]);

    return 0;
}


int sta_get_p2p_ip_config( int argc, char *argv[] )
{
    show_ip_config( WICED_P2P_INTERFACE, "1" );

    return 0;
}

int sta_wps_read_pin( int argc, char *argv[] )
{
    p2p_workspace.p2p_wps_mode = WPS_PIN_MODE;
    if (p2p_workspace.i_am_group_owner != 1)
    {
        p2p_workspace.p2p_wps_device_password_id = WPS_DEVICEPWDID_REG_SPEC;
    }

    besl_wps_generate_pin( p2p_workspace.p2p_wps_pin );

    printf( "status,COMPLETE,pin,%s\n", p2p_workspace.p2p_wps_pin );

    if ( ( p2p_workspace.i_am_group_owner == 1 ) && ( wwd_wifi_is_ready_to_transceive( WWD_P2P_INTERFACE ) == WWD_SUCCESS ) )
    {
        besl_p2p_start_registrar();
    }

    return 0;
}

int sta_wps_enter_pin( int argc, char *argv[] )
{
    p2p_workspace.p2p_wps_mode = WPS_PIN_MODE;
    if (p2p_workspace.i_am_group_owner != 1)
    {
        p2p_workspace.p2p_wps_device_password_id = WPS_USER_SPEC_DEVICEPWDID;
    }

    if ( besl_wps_validate_pin_checksum( argv[4]) )
    {
        memcpy(&p2p_workspace.p2p_wps_pin, argv[4], 9);
    }

    if ( ( p2p_workspace.i_am_group_owner == 1 ) && ( wwd_wifi_is_ready_to_transceive( WWD_P2P_INTERFACE ) == WWD_SUCCESS ) )
    {
        besl_p2p_start_registrar();
        wiced_rtos_delay_milliseconds( 1000 ); // Delay to allow registrar to start before we get the next command
    }

    printf( "status,COMPLETE,pin,%s\n", p2p_workspace.p2p_wps_pin );

    return 0;
}

int sta_set_wps_pbc( int argc, char *argv[] )
{
    char pin[] = "00000000";

    p2p_workspace.p2p_wps_mode = WPS_PBC_MODE;
    p2p_workspace.p2p_wps_device_password_id = WPS_PUSH_BTN_DEVICEPWDID;
    memcpy(&p2p_workspace.p2p_wps_pin, pin, 9);

    if ( ( p2p_workspace.i_am_group_owner == 1 ) && ( wwd_wifi_is_ready_to_transceive( WWD_P2P_INTERFACE ) == WWD_SUCCESS ) )
    {
        besl_p2p_start_registrar();
        wiced_rtos_delay_milliseconds( 1000 ); // Delay to allow registrar to start before we get the next command
    }

    printf( "status,COMPLETE\n" );

    return 0;
}

int sta_set_sleep( int argc, char *argv[] )
{
    besl_result_t result;

    result = besl_p2p_client_enable_powersave( &p2p_workspace, (uint32_t) PM1_POWERSAVE_MODE ); // Set legacy power save for this particular command, i.e. use ps-polls

    if ( result != BESL_SUCCESS )
    {
        printf( "Unable to set legacy power save mode\n");
    }

    printf( "status,COMPLETE\n" );

    return 0;
}

int sta_accept_p2p_invitation_req( int argc, char *argv[] )
{
    printf( "status,COMPLETE\n" );
    return 0;
}

wiced_result_t my_set_data_rate( uint32_t rate, wwd_interface_t interface )
{
    wiced_buffer_t buffer;
    uint32_t*      data;
    wwd_result_t   result;

    data = wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, "bg_rate" );
    if ( data == NULL )
    {
        return WICED_ERROR;
    }

    *data = 2 * rate;

    if ( ( result = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, interface ) ) != WWD_SUCCESS )
    {
        if ( result == WWD_WLAN_UNSUPPORTED )
        {
            data = wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, "2g_rate" );
            if ( data == NULL )
            {
                return WICED_ERROR;
            }
            *data = 2 * rate;
            result = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, interface );
        }
    }

    if ( result != WWD_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


/*!
 ******************************************************************************
 * P2P group formation result handler
 */

static void p2p_group_formation_result_handler( void* ws )
{
    p2p_workspace_t* workspace = (p2p_workspace_t*)ws;
    platform_dct_wifi_config_t* dct_wifi_config;
    platform_dct_p2p_config_t* dct_p2p_config;

    if ( wiced_network_is_ip_up( WICED_P2P_INTERFACE ) == WICED_TRUE )
    {
        if ( workspace->i_am_group_owner != 1 )
        {
            if ( workspace->group_candidate.p2p_capability & P2P_GROUP_CAPABILITY_P2P_PERSISTENT_GROUP )
            {
                wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
                memcpy(dct_wifi_config->stored_ap_list[0].details.SSID.value, workspace->group_candidate.ssid, MIN( workspace->group_candidate.ssid_length, MAX_SSID_LEN ) );
                dct_wifi_config->stored_ap_list[0].details.SSID.length = workspace->group_candidate.ssid_length;
                memcpy(&dct_wifi_config->stored_ap_list[0].details.BSSID, &workspace->group_candidate.bssid, sizeof( besl_mac_t ) );
                dct_wifi_config->stored_ap_list[0].details.security = WICED_SECURITY_WPA2_AES_PSK;
                memcpy(dct_wifi_config->stored_ap_list[0].security_key, (char*)workspace->p2p_passphrase, 64);
                dct_wifi_config->stored_ap_list[0].security_key_length = workspace->p2p_passphrase_length;
                wiced_dct_write( (const void*)dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof( platform_dct_wifi_config_t));
                wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );
            }
            /* Print the device interface, not BSSID, and SSID */
            printf( "status,COMPLETE,result,CLIENT,groupid,%02X:%02X:%02X:%02X:%02X:%02X %s\n", workspace->group_candidate.p2p_device_address.octet[0],
                                                                                                  workspace->group_candidate.p2p_device_address.octet[1],
                                                                                                  workspace->group_candidate.p2p_device_address.octet[2],
                                                                                                  workspace->group_candidate.p2p_device_address.octet[3],
                                                                                                  workspace->group_candidate.p2p_device_address.octet[4],
                                                                                                  workspace->group_candidate.p2p_device_address.octet[5],
                                                                                                  workspace->group_candidate.ssid);
        }
        else if ( p2p_workspace.initiate_negotiation == 1 )
        {
            /* Print the device interface and SSID */
            printf( "status,COMPLETE,result,GO,groupid,%02X:%02X:%02X:%02X:%02X:%02X %s\n", workspace->p2p_device_address.octet[0],
                                                                                              workspace->p2p_device_address.octet[1],
                                                                                              workspace->p2p_device_address.octet[2],
                                                                                              workspace->p2p_device_address.octet[3],
                                                                                              workspace->p2p_device_address.octet[4],
                                                                                              workspace->p2p_device_address.octet[5],
                                                                                              workspace->group_candidate.ssid);
        }
        else
        {
            printf( "status,COMPLETE\n");
        }
        if ( ( workspace->i_am_group_owner == 1 ) && ( workspace->form_persistent_group == 1 ) )
        {
            wiced_dct_read_lock( (void**) &dct_p2p_config, WICED_TRUE, DCT_P2P_CONFIG_SECTION, 0, sizeof(platform_dct_p2p_config_t) );
            workspace->persistent_group.details_valid = CONFIG_VALIDITY_VALUE;
            memcpy( &dct_p2p_config->p2p_group_owner_settings, &workspace->persistent_group, sizeof( wiced_config_soft_ap_t ) );
            wiced_dct_write( (const void*)dct_p2p_config, DCT_P2P_CONFIG_SECTION, 0, sizeof( platform_dct_p2p_config_t));
            wiced_dct_read_unlock( (void*) dct_p2p_config, WICED_TRUE );
        }
    }
    else
    {
        printf( "status,COMPLETE,result,FAIL,group_formation, interface %u\n", (unsigned int)workspace->p2p_interface );
    }
}




/*!
 ******************************************************************************
 * P2P connection result handler
 */

static void p2p_connection_result_handler( void* ws )
{
    p2p_workspace_t* workspace = (p2p_workspace_t*)ws;

    if ( ( wiced_network_is_ip_up( WICED_P2P_INTERFACE ) == WICED_TRUE ) && ( workspace->i_am_group_owner != WICED_TRUE ) )
    {
        printf( "status,COMPLETE\n");
    }
    else
    {
        printf( "status,COMPLETE,result,FAIL,connection_result, interface %u\n", (unsigned int)p2p_workspace.p2p_interface );
    }
}


/*!
 ******************************************************************************
 * P2P group owner start result handler
 */

static void p2p_go_start_result_handler( void* ws )
{
    wiced_mac_t mac;
    p2p_workspace_t* workspace = (p2p_workspace_t*)ws;

    /* Read P2P MAC address */
    wwd_wifi_get_mac_address( &mac, workspace->p2p_interface );

    if ( ( wiced_network_is_ip_up( WICED_P2P_INTERFACE ) == WICED_TRUE ) && ( workspace->group_owner_is_up == 1 ) )
    {
        printf( "status,COMPLETE,groupid,%02X:%02X:%02X:%02X:%02X:%02X %s\n", mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5], workspace->group_candidate.ssid );
    }
    else
    {
        printf( "status,COMPLETE,result,FAIL,go_start\n" );
    }
}


/*!
 ******************************************************************************
 * P2P device connection request callback
 */

void p2p_connection_request_callback( p2p_discovered_device_t* device )
{
    platform_dct_wifi_config_t* dct_wifi_config;
    platform_dct_p2p_config_t* dct_p2p_config;

    if ( ( p2p_workspace.p2p_current_state == P2P_STATE_DISCOVERY ) && ( device->status == P2P_DEVICE_INVITATION_REQUEST ) )
    {
        if ( device->invitation_flags == 0 )
        {
            p2p_workspace.group_candidate.ssid_length = p2p_workspace.invitation_candidate.ssid_length;
            memcpy( p2p_workspace.group_candidate.ssid, p2p_workspace.invitation_candidate.ssid, p2p_workspace.group_candidate.ssid_length );
            memcpy( &p2p_workspace.group_candidate.p2p_device_address, &device->p2p_device_address, sizeof( besl_mac_t ) );
            besl_p2p_register_group_formation_result_callback( &p2p_workspace, p2p_connection_result_handler);
            besl_p2p_find_group_owner( &p2p_workspace );
        }
        else if ( device->invitation_flags | P2P_INVITATION_REINVOKE_PERSISTENT_GROUP )
        {
            p2p_workspace.reinvoking_group = 1;
            if ( memcmp( &device->group_owner_device_address, &p2p_workspace.p2p_device_address, sizeof( besl_mac_t ) ) == 0 )
            {
                /* Reinvoke our group */
                wiced_dct_read_lock( (void**) &dct_p2p_config, WICED_TRUE, DCT_P2P_CONFIG_SECTION, 0, sizeof(platform_dct_p2p_config_t) );
                p2p_workspace.i_am_group_owner = 1;
                p2p_workspace.form_persistent_group = 1;
                memcpy( &p2p_workspace.persistent_group, &dct_p2p_config->p2p_group_owner_settings, sizeof( wiced_config_soft_ap_t ) );
                p2p_workspace.group_candidate.ssid_length = p2p_workspace.persistent_group.SSID.length;
                memcpy( &p2p_workspace.group_candidate.ssid, p2p_workspace.persistent_group.SSID.value, p2p_workspace.group_candidate.ssid_length );
                p2p_workspace.p2p_passphrase_length = p2p_workspace.persistent_group.security_key_length;
                memcpy( p2p_workspace.p2p_passphrase, p2p_workspace.persistent_group.security_key, p2p_workspace.p2p_passphrase_length );
                p2p_workspace.operating_channel.channel = p2p_workspace.persistent_group.channel;
                p2p_workspace.group_candidate.operating_channel.channel = p2p_workspace.operating_channel.channel;
                besl_p2p_register_group_formation_result_callback( &p2p_workspace, p2p_go_start_result_handler);
                wiced_dct_read_unlock( (void*) dct_p2p_config, WICED_TRUE );
            }
            else
            {
                /* Reinvoke their group XXX should check against workspace invitation candidate */
                wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
                p2p_workspace.group_candidate.ssid_length = dct_wifi_config->stored_ap_list[0].details.SSID.length;
                memcpy( p2p_workspace.group_candidate.ssid, dct_wifi_config->stored_ap_list[0].details.SSID.value, p2p_workspace.group_candidate.ssid_length );
                memcpy( &p2p_workspace.group_candidate.bssid, (char *)&dct_wifi_config->stored_ap_list[0].details.BSSID, sizeof( besl_mac_t ) );
                p2p_workspace.p2p_passphrase_length = dct_wifi_config->stored_ap_list[0].security_key_length;
                memcpy( p2p_workspace.p2p_passphrase, dct_wifi_config->stored_ap_list[0].security_key, p2p_workspace.p2p_passphrase_length);
                memcpy( &p2p_workspace.group_candidate.operating_channel, &p2p_workspace.invitation_candidate.operating_channel, sizeof( p2p_channel_info_t ) );
                besl_p2p_register_group_formation_result_callback( &p2p_workspace, p2p_connection_result_handler);
                wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );
            }
            besl_p2p_host_negotiation_complete(&p2p_workspace);
        }
    }
}

#endif /* WICED_USE_WIFI_P2P_INTERFACE */

int sta_preset_testparameters( int argc, char *argv[] )
{
    printf("status,COMPLETE\n");
    return 0;
}

int traffic_send_ping( int argc, char *argv[] )
{
    ++stream_id;

    wiced_rtos_send_asynchronous_event( &ping_worker_thread, wifi_traffic_send_ping, argv );

    wiced_rtos_delay_milliseconds( 100 ); /* Delay so the arguments don't get wiped out otherwise we need to copy argv */

    printf("status,COMPLETE,streamID,%u\n", ( unsigned int )stream_id);

    return 0;
}

int traffic_stop_ping( int argc, char *argv[] )
{
    wifi_traffic_stop_ping();

    return 0;
}

int traffic_agent_config( int argc, char *argv[] )
{
    int i = 1;

    ++stream_id;
    int stream_index = find_unallocated_stream_table_entry();
    if ( stream_index == -1 )
    {
        printf("status,ERROR\n");
        return 0;
    }

    stream_table[stream_index].stream_id = stream_id;

    while (i < (argc - 1))
    {
        if ( strcmp( argv[i], "profile" ) == 0 )
        {
            if ( strcmp( argv[i+1], "IPTV" ) == 0 )
            {
                stream_table[stream_index].profile = PROFILE_IPTV;
            }
            else if ( strcmp( argv[i+1], "Transaction" ) == 0 )
            {
                stream_table[stream_index].profile = PROFILE_TRANSACTION;
            }
            else if ( strcmp( argv[i+1], "Multicast" ) == 0 )
            {
                stream_table[stream_index].profile = PROFILE_MULTICAST;
            }
//            printf("profile %u\n", stream_table[stream_index].profile);
        }
        else if (strcmp( argv[i], "direction" ) == 0 )
        {
            if ( strcmp( argv[i+1], "send" ) == 0 )
            {
                stream_table[stream_index].direction = TRAFFIC_SEND;
            }
            else if ( strcmp( argv[i+1], "receive" ) == 0 )
            {
                stream_table[stream_index].direction = TRAFFIC_RECV;
            }
//            printf("direction %u\n", stream_table[stream_index].direction);
        }
        else if (strcmp( argv[i], "source" ) == 0 )
        {
            strncpy(stream_table[stream_index].src_ipaddr, argv[i+1], 15 );
//            printf("source %s\n", stream_table[stream_index].src_ipaddr);
        }
        else if ( strcmp( argv[i], "sourcePort" ) == 0 )
        {
            stream_table[stream_index].src_port = (uint16_t)atoi(argv[i+1]);
//            printf("source port %u\n", stream_table[stream_index].src_port);
        }
        else if ( strcmp( argv[i], "destination" ) == 0 )
        {
            strncpy(stream_table[stream_index].dest_ipaddr, argv[i+1], 15 );
//            printf("destination %s\n", stream_table[stream_index].dest_ipaddr);
        }
        else if ( strcmp( argv[i], "destinationPort" ) == 0 )
        {
            stream_table[stream_index].dest_port = (uint16_t)atoi(argv[i+1]);
//            printf("destination port %u\n", stream_table[stream_index].dest_port);
        }
        else if ( strcmp( argv[i], "trafficClass" ) == 0 )
        {
            if ( strcmp( argv[i+1], "BestEffort" ) == 0 )
            {
                stream_table[stream_index].ac = WMM_AC_BE;
            }
            if ( strcmp( argv[i+1], "Background" ) == 0 )
            {
                stream_table[stream_index].ac = WMM_AC_BK;
            }
            else if ( strcmp( argv[i+1], "Video" ) == 0 )
            {
                stream_table[stream_index].ac = WMM_AC_VI;
            }
            else if ( strcmp( argv[i+1], "Voice" ) == 0 )
            {
                stream_table[stream_index].ac = WMM_AC_VO;
            }
//            printf("traffic class %u\n", (unsigned int) stream_table[stream_index].traffic_class);
        }
        else if ( strcmp( argv[i], "payloadSize" ) == 0 )
        {
            stream_table[stream_index].payload_size = atoi(argv[i+1]);
//            printf("payload size %d\n", stream_table[stream_index].payload_size);
        }
        else if ( strcmp( argv[i], "duration" ) == 0 )
        {
            stream_table[stream_index].duration = atoi(argv[i+1]);
//            printf("duration %d\n", stream_table[stream_index].duration);
        }
        else if ( strcmp( argv[i], "frameRate" ) == 0 )
        {
            stream_table[stream_index].frame_rate = atoi(argv[i+1]);
//            printf("frame rate %d\n", stream_table[stream_index].frame_rate);
        }

        i = i + 2;
    }

    printf("status,COMPLETE,streamID,%u\n", (unsigned int)stream_id );
    return 0;
}

int traffic_agent_reset( int argc, char *argv[] )
{
    // Stop and reinit all streams
    wiced_rtos_delay_milliseconds( 10 ); // Delay in case IP stack messages have not been delivered
    init_stream_table();
    printf("status,COMPLETE\n" );
    return 0;
}

int traffic_agent_send( int argc, char *argv[] )
{
    char* p;
    int idx = 0;

    wiced_time_t current_time = host_rtos_get_time( ); // Common time in case of multiple transmit streams

    // Select which streams are to be started
    p = strtok( argv[2], " " ); // StreamIDs are delimited by spaces, not commas
    while ( p )
    {
        idx = find_stream_table_entry( atoi( p ) ); // Find stream table entry

        if ( idx == -1 )
        {
            printf("status,ERROR\n");
            return 0;
        }

        stream_table[idx].enabled = 1;
        stream_table[idx].stop_time = current_time + ( stream_table[idx].duration * 1000 );

        if ( spawn_ts_thread( udp_tx, &stream_table[idx] ) != 0 )
        {
            printf( "status,ERROR\n" );
        }
        p = strtok( '\0', " "); // Get the next StreamID
    }

    //printf("End of traffic_agent_send\n" );
//    wiced_rtos_delay_milliseconds( 10 );

    return 0;
}

int traffic_agent_receive_start( int argc, char *argv[] )
{
    int idx = find_stream_table_entry( atoi(argv[2])); // Allocate a stream table entry

    if ( idx == -1 )
    {
        printf("status,ERROR\n");
        return 0;
    }

    traffic_stream_t *ts = &stream_table[idx];

    ts->enabled = 1;

    if ( ts->profile == PROFILE_IPTV )
    {
        spawn_ts_thread( udp_rx, ts );
    }
    else if ( ts->profile == PROFILE_TRANSACTION )
    {
        //printf("transactional\n");
        spawn_ts_thread ( udp_transactional, ts );
    }
    else if ( ts->profile == PROFILE_MULTICAST )
    {
        //printf("transactional\n");
        spawn_ts_thread ( udp_rx, ts );
    }

//    wiced_rtos_delay_milliseconds( 10 );

    return 0;
}

int traffic_agent_receive_stop( int argc, char *argv[] )
{
    int idx = 0;
    int num_rx_streams = 0;
    int idx_table[NUM_STREAM_TABLE_ENTRIES]; // Need an index table to record the order in which to report results
    char* p = NULL;
    //wiced_thread_t* tmp_hnd;

    //printf("highest rx tos %u\n", (unsigned int)sdpcm_highest_rx_tos);

    printf( "status,COMPLETE,streamID,%s,txFrames,", argv[2] ); // Print the first part of the return message

    // Select which stream results are to be printed and which streams are to be stopped
    p = strtok( argv[2], " " ); // StreamIDs are delimited by spaces, not commas
    while ( p && ( num_rx_streams < NUM_STREAM_TABLE_ENTRIES ) )
    {
        idx = find_stream_table_entry( atoi( p ) ); // Find stream table entry
        if ( idx == -1 ) // Error check just for the first while loop
        {
            printf("status,ERROR\n");
            return 0;
        }
        idx_table[num_rx_streams] = idx;
        //stream_table[idx].stop_flag = 1;
        p = strtok( '\0', " "); // Get the next StreamID
        num_rx_streams++;
    }

    int i = 0;
    while ( i < num_rx_streams ) // Disable the streams
    {
        stream_table[idx_table[i]].enabled = 0;
        i++;
    }

    host_rtos_delay_milliseconds( 100 );

    // Print the txFrames value for each stream
    i = 0;
    while ( i <  num_rx_streams )
    {
        printf( "%d ", stream_table[idx_table[i]].frames_sent );
        i++;
    }

    // Print the rxFrames value for each stream
    printf( ",rxFrames," );
    i = 0;
    while ( i < num_rx_streams )
    {
        printf( "%d ", stream_table[idx_table[i]].frames_received );
        i++;
    }

    // Print the txPayloadBytes value for each stream
    printf( ",txPayloadBytes," );
    i = 0;
    while ( i < num_rx_streams )
    {
        printf( "%d ", stream_table[idx_table[i]].bytes_sent );
        i++;
    }

    // Print the rxPayloadBytes value for each stream
    printf( ",rxPayloadBytes," );
    i = 0;
    while ( i < num_rx_streams )
    {
        printf( "%d ", stream_table[idx_table[i]].bytes_received );
        i++;
    }

    // Print the outOfSequenceFrames value for each stream
    printf( ",outOfSequenceFrames," );
    i = 0;
    while ( i < num_rx_streams )
    {
        printf( "%d ", stream_table[idx_table[i]].out_of_sequence_frames );
        i++;
    }
    printf( "\n" );

    return 0;
}

int sta_set_11n( int argc, char *argv[] )
{
    return 0;
}

int sta_disconnect( int argc, char *argv[] )
{
    wiced_network_down( WICED_STA_INTERFACE );
    printf("status,COMPLETE\n");
    return 0;
}

int sta_reassoc( int argc, char *argv[] )
{
    return 0;
}


/** Retrieves the current AP BSSID (MAC address)
 * @param mac Storage which will receive the current AP BSSID
 * @return    WICED_SUCCESS or WICED_ERROR
 */

wiced_result_t wiced_wifi_get_bssid( wiced_mac_t * mac )
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wiced_result_t result;

    wwd_sdpcm_get_ioctl_buffer( &buffer, sizeof( wiced_mac_t ) );
    result = wwd_sdpcm_send_ioctl( SDPCM_GET, WLC_GET_BSSID, buffer, &response, WWD_STA_INTERFACE );
    if ( result == WICED_SUCCESS )
    {
        memcpy( mac, host_buffer_get_current_piece_data_pointer( response ), sizeof( wiced_mac_t ) );
        host_buffer_release( response, WWD_NETWORK_RX );
    }
    return result;
}


/*!
 ******************************************************************************
 * Convert a security authentication type string to a wiced_security_t.
 *
 * @param[in] arg  The string containing the value.
 *
 * @return    The value represented by the string.
 */
wiced_security_t get_authtype( char* encptype, char* keymgmttype )
{
   // printf("encptype: %s, keymgmttype: %s\n", encptype, keymgmttype);
    if ( strcmp( encptype, "none" ) == 0 )
    {
        return WICED_SECURITY_OPEN;
    }
    else if ( ( strcmp( encptype, "aes-ccmp" ) == 0 ) && ( strcmp( keymgmttype, "wpa2" ) == 0 ) )
    {
//        printf("WICED_SECURITY_WPA2_AES_PSK\n");
        return WICED_SECURITY_WPA2_AES_PSK;
    }
    else if ( ( strcmp( encptype, "aes-ccmp-tkip" ) == 0 ) && ( strcmp( keymgmttype, "wpa2-wpa-psk" ) == 0 ) )
    {
//        printf("WICED_SECURITY_WPA2_MIXED_PSK\n");
        return WICED_SECURITY_WPA2_MIXED_PSK;
    }
    else if ( ( strcmp( encptype, "tkip" ) == 0 ) && ( strcmp( keymgmttype, "wpa" ) == 0 ) )
    {
//        printf("WICED_SECURITY_WPA_TKIP_PSK\n");
        return WICED_SECURITY_WPA_TKIP_PSK;
    }
    else if ( strcmp( encptype, "wep" ) == 0 )
    {
//        printf("WICED_SECURITY_WEP_PSK\n");
        return WICED_SECURITY_WEP_PSK;
    }
    else
    {
         return WICED_SECURITY_UNKNOWN;
    }
}


void init_security( void )
{
    strncpy(test_passphrase, "12345678", sizeof( test_passphrase ) - 1 );
    strncpy(test_encptype, "none", sizeof( test_encptype ) - 1 );
    strncpy(test_keymgmttype, "wpa2", sizeof( test_keymgmttype ) - 1 );
    strncpy(test_ssid, "Cypress", sizeof( test_ssid ) - 1 );
}

void init_ipconfig( void )
{
    strncpy(test_using_dhcp, "1", sizeof( test_using_dhcp ) - 1 );
    strncpy(dut_ip_addr, "0.0.0.0", sizeof( dut_ip_addr ) - 1 );
    strncpy(dut_netmask, "0.0.0.0", sizeof( dut_netmask ) - 1 );
    strncpy(dut_gateway, "0.0.0.0", sizeof( dut_gateway ) - 1 );
    strncpy(dut_primary_dns, "0.0.0.0", sizeof( dut_primary_dns ) - 1 );
    strncpy(dut_secondary_dns, "0.0.0.0", sizeof( dut_secondary_dns ) - 1 );
}

void init_stream_table( void )
{
    int i, result;

    for( i = 0; i < NUM_STREAM_TABLE_ENTRIES; i++ )
    {
        if( stream_table[i].thread_ptr != NULL )
        {
//            printf("non null thread pointer at init\n\n");
//            if ( wiced_rtos_is_current_thread( stream_table[i].thread_ptr ) != WICED_SUCCESS )
//            {
//                wiced_rtos_thread_force_awake( stream_table[i].thread_ptr );
//                wiced_rtos_thread_join( stream_table[i].thread_ptr );
//                wiced_rtos_delete_thread( stream_table[i].thread_ptr );
//            }


            result = wiced_rtos_delete_thread( &(((thread_details_t*) stream_table[i].thread_ptr)->thread_handle) );
            if ( result != WICED_SUCCESS )
            {
                printf( "Error in Delete thread.\n" );
            }
            free( stream_table[i].thread_ptr );
        }
    }

    memset( stream_table, 0x00, sizeof( stream_table ) );
}

int find_unallocated_stream_table_entry ( void )
{
    int i;

    for( i = 0; i < NUM_STREAM_TABLE_ENTRIES; i++ )
    {
        if( stream_table[i].allocated == 0 )
        {
            memset(&stream_table[i], 0, sizeof(traffic_stream_t));
            stream_table[i].allocated = 1;
            return( i );
        }
    }
    return -1;
}

int find_stream_table_entry ( int id )
{
    int i;

//    printf("id %d\n", id);
    for( i = 0; i < NUM_STREAM_TABLE_ENTRIES; i++ )
    {
        if( ( stream_table[i].stream_id == id ) && ( stream_table[i].allocated == 1 ) )
        {
            return( i );
        }
    }
    return -1;
}


/*!
******************************************************************************
Convert an ascii hex representation of 0..F (or f) to a hex number in the range 0x00 to 0x0f.

\return hex value of the ascii hex representation

\param[in] value  ascii value of a hexadecimal digit
*/

uint8_t a_to_x(uint8_t value)
{
    if ((value >= 0x30) && (value <= 0x39)) // Hex number between 0 and 9
    {
        return (value - 0x30);
    }

    if ((value >= 0x41) && (value <= 0x46)) // Hex number between A and F
    {
        return (value - 0x37);
    }

    if ((value >= 0x61) && (value <= 0x66))
    {
        return (value - 0x57);
    }
    return 0;
}

/*!
******************************************************************************
Determine if an ascii character is a hex character

*/

wiced_bool_t is_hex_digit(char c)
{
    if ((c >= 0x30) && (c <= 0x39)) // Hex number between 0 and 9
    {
        return WICED_TRUE;
    }

    if ((c >= 0x41) && (c <= 0x46)) // Hex number between A and F
    {
        return WICED_TRUE;
    }

    if ((c >= 0x61) && (c <= 0x66))
    {
        return WICED_TRUE;
    }
    return WICED_FALSE;
}

void dump_bytes(const uint8_t* bptr, uint32_t len)
{
    int i = 0;

    for (i = 0; i < len; )
    {
        if ((i & 0x0f) == 0)
        {
            printf( "\n" );
        }
        else if ((i & 0x07) == 0)
        {
            printf( " " );
        }
        printf( "%02x ", bptr[i++] );
    }
    printf( "\n" );
}


wiced_result_t create_ping_worker_thread( void )
{
    memset( &ping_worker_thread, 0, sizeof( wiced_worker_thread_t) );
    if ( wiced_rtos_create_worker_thread( &ping_worker_thread, WICED_NETWORK_WORKER_PRIORITY, 4096, 2 ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


