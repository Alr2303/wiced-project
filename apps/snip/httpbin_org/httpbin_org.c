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
 * www.httpbin.org Client Application
 *
 * This application snippet demonstrates how to use the WICED HTTP Client API
 * to connect to https://www.httpbin.org
 *
 * Features demonstrated
 *  - Wi-Fi client mode
 *  - DNS lookup
 *  - Secure HTTPS client connection
 *
 * Application Instructions
 * 1. Modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials
 *    in the wifi_config_dct.h header file to match your Wi-Fi access point
 * 2. Connect a PC terminal to the serial port of the WICED Eval board,
 *    then build and download the application as described in the WICED
 *    Quick Start Guide
 *
 * After the download completes, the application :
 *  - Connects to the Wi-Fi network specified
 *  - Resolves the www.httpbin.org IP address using a DNS lookup
 *  - Sends multiple GET requests to https://www.httpbin.org
 *  - Prints the results to the UART
 *
 */

#include <stdlib.h>
#include "wiced.h"
#include "wiced_tls.h"
#include "http_client.h"
#include "JSON.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define SERVER_HOST        "www.httpbin.org"

#define SERVER_PORT        ( 443 )
#define DNS_TIMEOUT_MS     ( 10000 )
#define CONNECT_TIMEOUT_MS ( 3000 )
#define TOTAL_REQUESTS     ( 2 )

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

static void  event_handler( http_client_t* client, http_event_t event, http_response_t* response );
static void  print_data   ( char* data, uint32_t length );
static void  print_content( char* data, uint32_t length );
static void  print_header ( http_header_field_t* header, uint32_t number_of_fields );
static wiced_result_t parse_http_response_info(wiced_json_object_t * json_object );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static const char httpbin_root_ca_certificate[] =
        "-----BEGIN CERTIFICATE-----\r\n"                                        \
        "MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\r\n"   \
        "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\r\n"   \
        "DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\r\n"   \
        "PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\r\n"   \
        "Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n"   \
        "AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\r\n"   \
        "rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\r\n"   \
        "OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\r\n"   \
        "xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\r\n"   \
        "7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\r\n"   \
        "aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\r\n"   \
        "HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\r\n"   \
        "SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\r\n"   \
        "ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\r\n"   \
        "AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\r\n"   \
        "R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\r\n"   \
        "JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\r\n"   \
        "Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\r\n"                           \
        "-----END CERTIFICATE-----\n";


/******************************************************
 *               Function Definitions
 ******************************************************/

static http_client_t  client;
static http_request_t requests[TOTAL_REQUESTS];
static http_client_configuration_info_t client_configuration;

static const char* request_uris[] =
{
    [0] = "/get",
    [1] = "/html",
};

void application_start( void )
{
    wiced_ip_address_t  ip_address;
    wiced_result_t      result;
    http_header_field_t header[2];

    wiced_init( );
    wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);

    WPRINT_APP_INFO( ( "Resolving IP address of %s\n", SERVER_HOST ) );
    wiced_hostname_lookup( SERVER_HOST, &ip_address, DNS_TIMEOUT_MS, WICED_STA_INTERFACE );
    WPRINT_APP_INFO( ( "%s is at %u.%u.%u.%u\n", SERVER_HOST,
                                                 (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 24),
                                                 (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 16),
                                                 (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 8),
                                                 (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 0) ) );

    /* Initialize the root CA certificate */
    result = wiced_tls_init_root_ca_certificates( httpbin_root_ca_certificate, strlen(httpbin_root_ca_certificate) );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ( "Error: Root CA certificate failed to initialize: %u\n", result) );
        return;
    }

    http_client_init( &client, WICED_STA_INTERFACE, event_handler, NULL );
    WPRINT_APP_INFO( ( "Connecting to %s\n", SERVER_HOST ) );

    /* configure HTTP client parameters */
    client_configuration.flag = (http_client_configuration_flags_t)(HTTP_CLIENT_CONFIG_FLAG_SERVER_NAME | HTTP_CLIENT_CONFIG_FLAG_MAX_FRAGMENT_LEN);
    client_configuration.server_name = (uint8_t*)"www.httpbin.org";
    client_configuration.max_fragment_length = TLS_FRAGMENT_LENGTH_1024;
    http_client_configure(&client, &client_configuration);

    /* if you set hostname, library will make sure subject name in the server certificate is matching with host name you are trying to connect. pass NULL if you don't want to enable this check */
    client.peer_cn = NULL;

    if ( ( result = http_client_connect( &client, (const wiced_ip_address_t*)&ip_address, SERVER_PORT, HTTP_USE_TLS, CONNECT_TIMEOUT_MS ) ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ( "Error: failed to connect to server: %u\n", result) );
        return;
    }

    WPRINT_APP_INFO( ( "Connected\n" ) );

    wiced_JSON_parser_register_callback(parse_http_response_info);

    header[0].field        = HTTP_HEADER_HOST;
    header[0].field_length = sizeof( HTTP_HEADER_HOST ) - 1;
    header[0].value        = SERVER_HOST;
    header[0].value_length = sizeof( SERVER_HOST ) - 1;

    http_request_init( &requests[0], &client, HTTP_GET, request_uris[0], HTTP_1_1 );
    http_request_write_header( &requests[0], &header[0], 1 );
    http_request_write_end_header( &requests[0] );
    http_request_flush( &requests[0] );

    header[1].field        = HTTP_HEADER_HOST;
    header[1].field_length = sizeof( HTTP_HEADER_HOST ) - 1;
    header[1].value        = SERVER_HOST;
    header[1].value_length = sizeof( SERVER_HOST ) - 1;

    http_request_init( &requests[1], &client, HTTP_GET, request_uris[1], HTTP_1_1 );
    http_request_write_header( &requests[1], &header[1], 1 );
    http_request_write_end_header( &requests[1] );
    http_request_flush( &requests[1] );
}

static void event_handler( http_client_t* client, http_event_t event, http_response_t* response )
{
    switch( event )
    {
        case HTTP_CONNECTED:
            WPRINT_APP_INFO(( "Connected to %s\n", SERVER_HOST ));
            break;

        case HTTP_DISCONNECTED:
        {
            WPRINT_APP_INFO(( "Disconnected from %s\n", SERVER_HOST ));
            http_request_deinit( &requests[0] );
            http_request_deinit( &requests[1] );
            break;
        }

        case HTTP_DATA_RECEIVED:
        {
            if ( response->request == &requests[0] )
            {
                /* Response to first request. Simply print the result */
                WPRINT_APP_INFO( ( "\nRecieved response for request #1. Content received:\n" ) );

                /* print only HTTP header */
                if(response->response_hdr != NULL)
                {
                    WPRINT_APP_INFO( ( "\n HTTP Header Information for response1 : \n" ) );
                    print_content( (char*) response->response_hdr, response->response_hdr_length );
                }

                /* print payload information comes as HTTP response body */
                WPRINT_APP_INFO( ( "\n Payload Information for response1 : \n" ) );
                print_content( (char*) response->payload, response->payload_data_length );
                if(wiced_JSON_parser( (const char*)response->payload , response->payload_data_length ) != WICED_SUCCESS)
                {
                    WPRINT_APP_INFO( ( "\n JSON parsing Error: \n" ) );
                }

                if(response->remaining_length == 0)
                {
                   WPRINT_APP_INFO( ( "Received total payload data for response1 \n" ) );
                }
            }
            else if ( response->request == &requests[1] )
            {
                /* Response to 2nd request. Simply print the result */
                WPRINT_APP_INFO( ( "\nRecieved response for request #2. Content received:\n" ) );

                /* Response to second request. Parse header for "Date" and "Content-Length" */
                http_header_field_t header_fields[2];
                uint32_t size = sizeof( header_fields ) / sizeof(http_header_field_t);

                /* only process HTTP header when response contains it */
                if(response->response_hdr != NULL)
                {
                    WPRINT_APP_INFO( ( "\n HTTP Header Information for response2 : \n" ) );
                    print_content( (char*) response->response_hdr, response->response_hdr_length );

                    header_fields[ 0 ].field        = HTTP_HEADER_DATE;
                    header_fields[ 0 ].field_length = sizeof( HTTP_HEADER_DATE ) - 1;
                    header_fields[ 0 ].value        = NULL;
                    header_fields[ 0 ].value_length = 0;
                    header_fields[ 1 ].field        = HTTP_HEADER_CONTENT_LENGTH;
                    header_fields[ 1 ].field_length = sizeof( HTTP_HEADER_CONTENT_LENGTH ) - 1;
                    header_fields[ 1 ].value        = NULL;
                    header_fields[ 1 ].value_length = 0;

                    if ( http_parse_header( response->response_hdr, response->response_hdr_length, header_fields, size ) == WICED_SUCCESS )
                    {
                        WPRINT_APP_INFO( ( "\nParsing response of request #2 for \"Date\" and \"Content-Length\". Fields found:\n" ) );
                        print_header( header_fields, size );
                    }
                }

                /* Print payload information that comes as response body */
                WPRINT_APP_INFO( ( "Payload Information for response2 : \n" ) );
                print_content( (char*) response->payload, response->payload_data_length );

                if(response->remaining_length == 0)
                {
                    WPRINT_APP_INFO( ( "Received total payload data for response2 \n" ) );
                }
            }
        break;
        }
        default:
        break;
    }
}

static void print_data( char* data, uint32_t length )
{
    uint32_t a;

    for ( a = 0; a < length; a++ )
    {
        WPRINT_APP_INFO( ( "%c", data[a] ) );
    }
}

static void print_content( char* data, uint32_t length )
{
    WPRINT_APP_INFO(( "==============================================\n" ));
    print_data( (char*)data, length );
    WPRINT_APP_INFO(( "\n==============================================\n" ));
}

static void print_header( http_header_field_t* header_fields, uint32_t number_of_fields )
{
    uint32_t a;

    WPRINT_APP_INFO(( "==============================================\n" ));
    for ( a = 0; a < 2; a++ )
    {
        print_data( header_fields[a].field, header_fields[a].field_length );
        WPRINT_APP_INFO(( " : " ));
        print_data( header_fields[a].value, header_fields[a].value_length );
        WPRINT_APP_INFO(( "\n" ));
    }
    WPRINT_APP_INFO(( "==============================================\n" ));
}

static wiced_result_t parse_http_response_info(wiced_json_object_t * json_object )
{
    if(strncmp(json_object->object_string, "url", sizeof("url")-1) == 0)
    {
        WPRINT_APP_INFO (("Requested URL : %.*s\n",json_object->value_length, json_object->value));
    }

    return WICED_SUCCESS;
}
