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

/** @ws_erver.c
 *
 * Reference application for Websocket server application
 *
 */

#include "websocket.h"
#include "wiced_resource.h"
#include "resources.h"
#include "wiced_tls.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define BUFFER_LENGTH       (1024)
#define FINAL_FRAME         WICED_TRUE

/* comment-out below macro to make this server listen for non-secure client connections */
#define USE_WEBSOCKET_SECURE_SERVER

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

static wiced_result_t websocket_server_on_open_callback     ( void* websocket );
static wiced_result_t websocket_server_on_close_callback    ( void* websocket );
static wiced_result_t websocket_server_on_error_callback    ( void* websocket );
static wiced_result_t websocket_server_on_message_callback  ( void* websocket );

/******************************************************
 *               Variable Definitions
 ******************************************************/
#define MAX_NUM_OF_SUB_PROTOCOLS 2
const char myserver_protocol_list[MAX_NUM_OF_SUB_PROTOCOLS][10] = { "soap", "wamp" };

static wiced_websocket_url_protocol_entry_t  myserver_url_protocol_list[] =
{
    [0] =
    {
     .url = "ws://192.168.1.2:80/",
     .protocols = NULL,
    },
    [1] =
    {
     .url = "ws://192.168.1.2:80/chat",
     .protocols = (const char**)myserver_protocol_list,
    },
};

static wiced_websocket_url_protocol_table_t myserver_table =
{
    .count = 2,
    .entries = myserver_url_protocol_list,
};

static wiced_websocket_callbacks_t myserver_callbacks =
{
    .on_open    = websocket_server_on_open_callback,
    .on_close   = websocket_server_on_close_callback,
    .on_error   = websocket_server_on_error_callback,
    .on_message = websocket_server_on_message_callback,
};

static wiced_websocket_server_config_t myserver_config =
{
    .max_connections            = 4,
    .heartbeat_duration         = 0,
    .url_protocol_table         = &myserver_table,
};

static wiced_websocket_server_t myserver;

static wiced_websocket_frame_t rx_frame;
static wiced_websocket_frame_t tx_frame;

static char tx_buffer[ BUFFER_LENGTH ] = { 'H','e','l','l','o',' ','C','l','i','e','n','t','!','\0' };
static uint8_t tx_bin_buffer[BUFFER_LENGTH] = { 0x01, 0x02, 0xAA, 0xFE, 0x22, 0x34, 0x56 };

static char rx_buffer[ BUFFER_LENGTH ];

wiced_tls_identity_t myserver_tls_identity;

/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_result_t websocket_server_on_open_callback ( void* socket )
{
    wiced_websocket_t* websocket = ( wiced_websocket_t* )socket;
    UNUSED_VARIABLE(websocket);
    WPRINT_APP_INFO(("[App] Connection established @websocket:%p\n", socket ) );
    wiced_websocket_initialise_tx_frame( &tx_frame, FINAL_FRAME, WEBSOCKET_TEXT_FRAME, strlen(tx_buffer), tx_buffer, BUFFER_LENGTH );
    /* Send sample data to client when connection is established */
    WICED_VERIFY( wiced_websocket_send( websocket, &tx_frame ) );
    return WICED_SUCCESS;
}

static wiced_result_t websocket_server_on_error_callback ( void* socket )
{
    wiced_websocket_t* websocket = ( wiced_websocket_t* )socket;
    WPRINT_APP_INFO(("[App] Error[%d] @websocket:%p\n", websocket->error_type, socket ) );
    return WICED_SUCCESS;
}

static wiced_result_t websocket_server_on_close_callback( void* socket )
{
    wiced_websocket_t* websocket = ( wiced_websocket_t* )socket;
    UNUSED_VARIABLE(websocket);
    WPRINT_APP_INFO(("[App] Connection Closed @websocket:%p\n", socket ) );
    return WICED_SUCCESS;
}

static wiced_result_t websocket_server_on_message_callback( void* socket )
{
    wiced_websocket_t* websocket = ( wiced_websocket_t* )socket;
    WPRINT_APP_INFO( ( "[App] Message received @websocket:%p\n", socket) );

    wiced_websocket_receive( websocket, &rx_frame );

    switch( rx_frame.payload_type )
    {
        case WEBSOCKET_TEXT_FRAME :
        {
            uint8_t nr_bytes = 0;
            char* echo_payload = NULL;
            WPRINT_APP_INFO( ( "\tFrame Type: TEXT FRAME\r\n" ) );
            WPRINT_APP_INFO( ( "\tFrame Data: %s\r\n", (char*)rx_frame.payload ) );
            nr_bytes = strlen(rx_frame.payload) + 1;
            echo_payload = malloc(nr_bytes);
            if(echo_payload == NULL)
            {
                WPRINT_APP_INFO(("[App] Error sending echo!\n"));
                break;
            }

            memcpy( echo_payload, rx_frame.payload, nr_bytes );
            wiced_websocket_initialise_tx_frame( &tx_frame, FINAL_FRAME, WEBSOCKET_TEXT_FRAME, (nr_bytes- 1), echo_payload, nr_bytes );
            WICED_VERIFY( wiced_websocket_send(websocket, &tx_frame) );
            WPRINT_APP_INFO( ( "\tEchoed Text-frame back\r\n" ) );
            free(echo_payload);

            wiced_websocket_initialise_tx_frame( &tx_frame, FINAL_FRAME, WEBSOCKET_BINARY_FRAME, 7, tx_bin_buffer, 7 );
            WICED_VERIFY( wiced_websocket_send(websocket, &tx_frame) );
            WPRINT_APP_INFO(("\tSent some binary data\r\n") );
            break;
        }

        case WEBSOCKET_PONG:
        {
            WPRINT_APP_INFO( ( "\tFrame Type: PONG\r\n" ) );
            WPRINT_APP_INFO( ( "\tFrame Data: %s\r\n", (char*)rx_frame.payload ) );
            break;
        }

        case WEBSOCKET_PING:
        {
            char* pong_payload = NULL;
            uint8_t nr_bytes = 0;
            WPRINT_APP_INFO( ( "\tFrame Type: PING\r\n" ) );
            WPRINT_APP_INFO( ( "\tFrame Data: %s\r\n", (char*)rx_frame.payload ) );
            nr_bytes = strlen(rx_frame.payload) + 1;
            pong_payload = malloc(nr_bytes);
            if(pong_payload == NULL)
            {
                WPRINT_APP_INFO(("[App] Error sending PONG\n"));
                break;
            }

            memcpy( pong_payload, rx_frame.payload, nr_bytes );
            wiced_websocket_initialise_tx_frame( &tx_frame, FINAL_FRAME, WEBSOCKET_PONG, (nr_bytes- 1), pong_payload, nr_bytes );
            WICED_VERIFY( wiced_websocket_send(websocket, &tx_frame) );
            WPRINT_APP_INFO((" \tSent PONG\r\n"));
            free(pong_payload);
            break;
        }

        case WEBSOCKET_CONNECTION_CLOSE:
        {
            wiced_result_t result;
            WPRINT_APP_INFO( ( "\tFrame Type: CONNECTION CLOSE\r\n" ) );
            result = wiced_websocket_close( websocket, WEBSOCKET_CLOSE_STATUS_CODE_NORMAL, NULL );
            WPRINT_APP_INFO(("\tSending CLOSE frame back result:%d\n", result) );
            break;
        }

        case WEBSOCKET_BINARY_FRAME:
        {
            WPRINT_APP_INFO( ( "\tFrame Type: BINARY\r\n" ) );
            break;
        }

        case WEBSOCKET_CONTINUATION_FRAME:
        {
            WPRINT_APP_INFO( ( "\tFrame Type: CONTINUATION\r\n" ) );
            break;
        }

        default:
            WPRINT_APP_INFO( ( "\tFrame Type: RESERVED\r\n" ) );
            break;
    }

    printf("\r\n");

    return WICED_SUCCESS;
}

void application_start( )
{
    wiced_tls_identity_t* tls_identity = NULL;
    wiced_init( );

    if ( wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ( "\r\n[App] Failed to bring up network\r\n" ) );
        return;
    }

    WPRINT_APP_INFO(( "[App] Network initialized\n") );

#ifdef USE_WEBSOCKET_SECURE_SERVER
    wiced_result_t result = WICED_ERROR;
    platform_dct_security_t* dct_security = NULL;
    /* Set-up TLS identity structure with some server certificate etc. */

    /* Lock the DCT to allow us to access the certificate and key */
    WPRINT_APP_INFO( ( "[App] Read the certificate Key from DCT\n" ) );
    result = wiced_dct_read_lock( (void**) &dct_security, WICED_FALSE, DCT_SECURITY_SECTION, 0, sizeof( *dct_security ) );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("[App] Unable to lock DCT to read certificate\n"));
        return;
    }

    /* Setup TLS identity */
    result = wiced_tls_init_identity( &myserver_tls_identity, dct_security->private_key, strlen( dct_security->private_key ), (uint8_t*) dct_security->certificate, strlen( dct_security->certificate ) );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(( "[App] Unable to initialize TLS identity. Error = [%d]\n", result ));
        return;
    }

    tls_identity = &myserver_tls_identity;

#endif // End of USE_WEBSOCKET_SECURE_SERVER

    if ( wiced_websocket_server_start( &myserver, &myserver_config, &myserver_callbacks, tls_identity ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ( "[App] Failed to start Websocket Server\n" ) );
        return;
    }

#ifdef USE_WEBSOCKET_SECURE_SERVER
    /* Finished accessing the certificates */
    result = wiced_dct_read_unlock( dct_security, WICED_FALSE );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(( "[App] DCT Read Unlock Failed. Error = [%d]\n", result ));
        return;
    }
#endif

    WPRINT_APP_INFO( ("[App] WebSocket Server running(listening...)\n") );
    wiced_websocket_initialise_rx_frame( &rx_frame, rx_buffer, BUFFER_LENGTH );
}
