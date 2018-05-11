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
 */
#include "websocket.h"
#include "websocket_handshake.h"
#include "wwd_debug.h"
#include "wiced_crypto.h"
#include "wiced_utilities.h"
#include "wwd_assert.h"
#include "wiced_tls.h"

/*  Websocket packet format
 *
 *   (MSB)
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-------+-+-------------+-------------------------------+
 *   |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *   |I|S|S|S|  (4)  |A|     (7)     |             (16/63)           |
 *   |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *   | |1|2|3|       |K|             |                               |
 *   +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *   |     Extended payload length continued, if payload len == 127  |
 *   + - - - - - - - - - - - - - - - +-------------------------------+
 *   |                               |         Masking-key           |
 *   +-------------------------------+ - - - - - - - - - - - - - - - +<---------
 *   :    Masking-key (cont.)        |         Payload Data          :
 *   +---------------------------------------------------------------+ AREA TO
 *   :                          Payload data                         : MASK
 *   +---------------------------------------------------------------+<---------
 */

/******************************************************
 *                      Macros
 ******************************************************/

#define IF_ERROR_RECORD_AND_EXIT( GENERIC_ERROR, WEBSOCKET_ERROR )            \
    {                                                                         \
        if( GENERIC_ERROR != WICED_SUCCESS )                                  \
        {                                                                     \
            websocket->error_type=WEBSOCKET_ERROR;                            \
            if( websocket->callbacks.on_error != NULL )                       \
            {                                                                 \
                websocket->callbacks.on_error(websocket);                     \
            }                                                                 \
            unlock_websocket_mutex();                                         \
            return GENERIC_ERROR;                                             \
        }                                                                     \
    }

/******************************************************
 *                    Constants
 ******************************************************/
#define MAXIMUM_FRAME_OVERHEAD  8
#define MASKING_KEY_BYTES       4
#define MAX_PAYLOAD_FIELD_BYTES 2
#define CONTROL_FIELD_BYTES     2
#define MAX_HEADER_SIZE         (MASKING_KEY_BYTES + MAX_PAYLOAD_FIELD_BYTES + CONTROL_FIELD_BYTES)
#define MAX_CONTROL_FRAME       125

/* Websocket frame control byte mask */
#define FIN_MASK                         0x80
#define RSV1_MASK                        0x40
#define RSV2_MASK                        0x20
#define RSV3_MASK                        0x10
#define OPCODE_MASK                      0x0F

/* Websocket frame payloag byte mask */
#define PAYLOAD_MASK                     0x80
#define PAYLOAD_LENGTH_1_BYTE_MASK       0x7F

#define RSV_ERR_SHUTDOWN_MESSAGE              "rsv bits/extension negotiation is not supported"
#define CLOSE_REQUEST_SHUTDOWN_MESSAGE        "close requested from Server"
#define LENGTH_SHUTDOWN_MESSAGE               "max Frame length supported is 1024"
#define PAYLOAD_TYPE_ERROR_SHUTDOWN_MESSAGE   "unsupported payload type"

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum
{
    MUTEX_UNINITIALISED,
    MUTEX_LOCKED,
    MUTEX_UNLOCKED
}websocket_mutex_state_t;
/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef enum
{
    READ_CONTROL_BYTES,
    READ_LENGTH,
    READ_PAYLOAD,
    READ_COMPLETED_SUCCESSFULLY,
    READ_FRAME_ERROR
} wiced_websocket_read_frame_state_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_mutex_t           handshake_lock;
    websocket_mutex_state_t mutex_state;
} wiced_websocket_mutex_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t websocket_common_connect     ( wiced_websocket_t* websocket, const wiced_websocket_client_url_protocol_t* url, wiced_tls_identity_t* tls_identity );
static wiced_result_t websocket_connect            ( wiced_websocket_t* websocket, wiced_ip_address_t* address );
static wiced_result_t websocket_tls_connect        ( wiced_websocket_t* websocket, wiced_ip_address_t* address, wiced_tls_identity_t* tls_identity );
static wiced_result_t mask_unmask_frame_data       ( uint8_t* data_in, uint8_t* data_out, uint16_t data_length, uint8_t* mask );
static wiced_result_t update_socket_state          ( wiced_websocket_t* websocket, wiced_websocket_state_t state);
static wiced_result_t on_websocket_close_callback  ( wiced_tcp_socket_t* socket, void* websocket );
static wiced_result_t on_websocket_message_callback( wiced_tcp_socket_t* socket, void* websocket );
static void           lock_websocket_mutex         ( void );
static void           unlock_websocket_mutex       ( void );
static void           handle_frame_error           ( wiced_websocket_t* websocket, const char* close_string );
static void           handle_ping_frame            ( wiced_websocket_t* websocket, wiced_websocket_frame_t* rx_frame );
static void           handle_pong_frame            ( wiced_websocket_t* websocket, wiced_websocket_frame_t* rx_frame );
static void           handle_close_connection_frame( wiced_websocket_t* websocket, wiced_websocket_frame_t* rx_frame );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_websocket_mutex_t     websocket_mutex =
{
    .mutex_state = MUTEX_UNINITIALISED
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_bool_t verify_close_code( const uint16_t code )
{
    if( (code <= WEBSOCKET_CLOSE_STATUS_CODE_UNEXPECTED_CONDITION && code >= WEBSOCKET_CLOSE_STATUS_CODE_NORMAL)
        && (code != WEBSOCKET_CLOSE_STATUS_CODE_RESERVED_NO_STATUS_CODE)
        && (code != WEBSOCKET_CLOSE_STATUS_CODE_RESERVED_ABNORMAL_CLOSE) )
    {
        return WICED_TRUE;
    }

    /* code == 0 is a special value when applications just wants to send a connection close frame
     * without any body ==> 'reason' string will be discarded too
     */
    if( code == WEBSOCKET_CLOSE_STATUS_NO_CODE )
    {
        return WICED_TRUE;
    }

    return WICED_FALSE;
}

static wiced_bool_t verify_close_reason( const char* reason )
{
    if( strlen(reason ) > WEBSOCKET_CLOSE_FRAME_MAX_REASON_LENGTH )
    {
        return WICED_FALSE;
    }

    return WICED_TRUE;
}

static wiced_result_t mask_unmask_frame_data( uint8_t* data_in, uint8_t* data_out, uint16_t data_length, uint8_t* mask )
{
    wiced_result_t result = WICED_SUCCESS;
    uint32_t       i;

    for ( i = 0; i < data_length; i++ )
    {
        data_out[ i ] = data_in[ i ] ^ mask[ i % 4 ];
    }

    return result;
}

//static initialize_handshake_fields()
static void websocket_core_initialise( wiced_websocket_core_t* websocket, wiced_websocket_role_t role )
{
    websocket->socket                              = NULL;
    websocket->role                                = role;
    websocket->socket                              = malloc(sizeof(wiced_tcp_socket_t));
    websocket->formatted_websocket_frame           = NULL;
    websocket->formatted_websocket_frame_length    = 0;
}

static void websocket_core_uninitialise( wiced_websocket_core_t* websocket )
{
    if( websocket->formatted_websocket_frame != NULL )
    {
        free( websocket->formatted_websocket_frame );
    }
    websocket->formatted_websocket_frame = NULL;

    if( websocket->socket != NULL )
    {
        free(websocket->socket);
    }

    websocket->socket = NULL;
}

void wiced_websocket_initialise( wiced_websocket_t* websocket )
{

    websocket->state            = WEBSOCKET_INITIALISED;
    websocket->url_protocol_ptr = NULL;
   // websocket->subprotocol      = { 0x0 };

    websocket_core_initialise(&websocket->core, WEBSOCKET_ROLE_CLIENT );
}

void wiced_websocket_uninitialise( wiced_websocket_t* websocket )
{
    websocket->state = WEBSOCKET_UNINITIALISED;

    websocket_core_uninitialise(&websocket->core);
}

wiced_result_t wiced_websocket_connect( wiced_websocket_t* websocket, const wiced_websocket_client_url_protocol_t* url_entry )
{

    if ( websocket_common_connect( websocket, url_entry, NULL ) != WICED_SUCCESS )
    {
        wiced_websocket_close( websocket, WEBSOCKET_CLOSE_STATUS_NO_CODE, NULL );
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_websocket_secure_connect( wiced_websocket_t* websocket, const wiced_websocket_client_url_protocol_t* url_entry, wiced_tls_identity_t* tls_identity )
{
    if ( websocket_common_connect( websocket, url_entry, tls_identity ) != WICED_SUCCESS )
    {
        wiced_websocket_close( websocket, WEBSOCKET_CLOSE_STATUS_NO_CODE, NULL );
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

#if 0
static wiced_result_t parse_url( wiced_websocket_url_protocol_entry_t* url_entry, wiced_websocket_header_fields_t* header, wiced_websocket_role_t role )
{
    char* url_start_ptr;

    wiced_bool_t secure_uri = WICED_FALSE;
    uint8_t uri_header_length;
    uint8_t uri_host_length;
    uint8_t uri_port_length;
    uint8_t uri_path_length;
    uint8_t uri_query_length;

    char* host_start_ptr;
    char* port_start_ptr;
    char* path_start_ptr;
    char* query_start_ptr;


    /* FIXME: make it case-insensitive */
    char ws_uri[] = "ws://";
    char wss_uri[] = "wss://";

    if( !url_entry || !header || !(url_entry->url) )
    {
        return WICED_ERROR;
    }

    url_start_ptr = url_entry->url;

    if( memcmp( url_start_ptr, ws_uri, sizeof(ws_uri) ) != 0 )
    {
        if( memcmp( url_start_ptr, wss_uri, sizeof(wss_uri) ) != 0 )
        {
            return WICED_BADARG;
        }
        else
        {
            secure_uri = WICED_TRUE;
        }
    }

    if( secure_uri )
    {
        uri_header_length = sizeof(wss_uri);
    }
    else
    {
        uri_header_length = sizeof(ws_uri);
    }

    host_start_ptr = url_start_ptr + uri_header_length;

    /* first look for [":" port ] field */
    port_start_ptr = strstr( host_start_ptr, ':')

    if( port_start_ptr == NULL )
    {
        /* port is not provided; take default values for port and look for first '/' as end of 'host' */
        first_slash_ptr = strstr(host_start_ptr, '/');
        if( first_slash_ptr == NULL )
        {
            /* only |host| available, no path or query is provided */
            uri_host_length = strlen (url_start_ptr) - uri_header_length;

            header->host = malloc( uri_host_length + 1 );
            memset( &header->host, 0, uri_host_length + 1 );

            memcpy( &header->host, host_start_ptr, uri_host_length );

            uri_path_length = 2;
            /* Assumption here is that URI won't have query component if 'path' is empty */
            header->request_uri = malloc( uri_path_length );
            strncpy(&header->request_uri, "/", uri_path_length );
        }
        else
        {
            uri_host_length = first_slash_ptr - host_start_ptr;
            header->host = malloc( uri_host_length + 1 );
            memset( &header->host, 0, uri_host_length + 1 );

            memcpy( &header->host, host_start_ptr, uri_host_length );
        }
    }
    else
    {
        uri_host_length = port_start_ptr - host_start_ptr;
        /* port is provided; see if it matches default ports or not */
        first_slash_ptr = strstr(port_start_ptr, '/');
        if( first_slash_ptr == NULL )
        {
            /* 'path' is empty */
        }

    }

    if( url_entry->sec_websocket_protocols != NULL )
    {

    }

    if( role == WEBSOCKET_ROLE_CLIENT )
    {
        WPRINT_LIB_INFO( ("==== Websocket Header fields ====\n") );
        WPRINT_LIB_INFO( ("\tHost: %s\r\nRequest_uri: %s\r\n"))

    }

}

static void set_local_header_fields( wiced_websocket_t* websocket, const wiced_websocket_url_protocol_entry_t* url_entry )
{
    wiced_websocket_header_fields_t* local = &websocket->core.local;

    //parse_url( url_entry.url, local, websocket->core.role );
#if 1 // Hard-coding
    if( websocket->core.role == WEBSOCKET_ROLE_CLIENT )
    {
        local->host                     = "192.168.1.2";
        local->request_uri              = "?encoding=text";
        local->origin                   = client->origin;
        local->sec_websocket_protocols  = client->sec_websocket_protocols;
    }
    else
    {
        wiced_websocket_server_config_t* server = (wiced_websocket_server_config_t* )config;
        local->sec_websocket_protocols = server->sec_websocket_protocols;
        local->host                    = server->host;
    }
#endif
}
#endif

static void reset_client_handshake_fields( wiced_websocket_handshake_fields_t* client_handshake )
{
    websocket_client_handshake_fields_t* client = NULL;

    if( client_handshake == NULL )
    {
        WPRINT_LIB_INFO(("Handshake fields already NULL?\n"));
        return;
    }

    client = &client_handshake->client;

    if( client->host )
    {
        free(client->host);
    }

    if( client-> port )
    {
        free(client->port);
    }

    if( client->resource_name )
    {
        free(client->resource_name);
    }

    if( client->origin )
    {
        free(client->origin);
    }

    if( client->protocols )
    {
        free(client->protocols);
    }

    if( client->sec_websocket_key )
    {
        free(client->sec_websocket_key);
    }

    if( client->sec_websocket_version )
    {
        free(client->sec_websocket_version);
    }
    return;
}

static wiced_result_t set_client_handshake_fields( wiced_websocket_handshake_fields_t* client_handshake, const wiced_websocket_client_url_protocol_t* url_entry )
{
    uint8_t nr_bytes = 0;

    websocket_client_handshake_fields_t* client = &client_handshake->client;

    client->host                    = NULL;
    client->port                    = NULL;
    client->resource_name           = NULL;
    client->origin                  = NULL;
    client->protocols               = NULL;
    client->sec_websocket_key       = NULL;
    client->sec_websocket_version   = NULL;

    if( url_entry->host == NULL || url_entry->request_uri == NULL )
    {
        return WICED_BADARG;
    }

    /* extract 'host' field */
    nr_bytes = (uint8_t)(strlen(url_entry->host) + 1 );

    client->host = malloc( nr_bytes );

    if(client->host == NULL )
    {
        return WICED_OUT_OF_HEAP_SPACE;
    }
    memcpy(client->host, url_entry->host, nr_bytes );

    /* extract 'request_uri' field */
    nr_bytes = (uint8_t)(strlen(url_entry->request_uri) + 1 );

    client->resource_name = malloc( nr_bytes );

    if(client->resource_name == NULL )
    {
        return WICED_OUT_OF_HEAP_SPACE;
    }

    memcpy(client->resource_name, url_entry->request_uri, nr_bytes );

    /* extract 'origin' field if available */
    if( url_entry->origin != NULL )
    {
        nr_bytes = (uint8_t)(strlen(url_entry->origin) + 1);

        client->origin = malloc( nr_bytes );
        if(client->origin == NULL )
        {
            return WICED_OUT_OF_HEAP_SPACE;
        }

        memcpy(client->origin, url_entry->origin, nr_bytes );
    }
    /* extract 'sec_websocket_protocol' field if available */
    if( url_entry->sec_websocket_protocol != NULL )
    {
        nr_bytes = (uint8_t)(strlen(url_entry->sec_websocket_protocol) + 1);

        client->protocols = malloc( nr_bytes );
        if(client->protocols == NULL )
        {
            return WICED_OUT_OF_HEAP_SPACE;
        }

        memcpy(client->protocols, url_entry->sec_websocket_protocol, nr_bytes );
    }

    return WICED_SUCCESS;
}

static wiced_result_t websocket_common_connect( wiced_websocket_t* websocket, const wiced_websocket_client_url_protocol_t* url_entry, wiced_tls_identity_t* tls_identity )
{
    wiced_result_t     result = WICED_SUCCESS;
    wiced_ip_address_t address;
    wiced_tcp_socket_t* tcp_socket = NULL;
    wiced_websocket_handshake_fields_t handshake;

    if( websocket == NULL )
    {
        return result;
    }

    /* as per rfc 6455 specification, there can not be more then one socket in a connecting state*/
    lock_websocket_mutex();

    /* FIXME: First expect a Server IP address. If it fails, try hostname_lookup.
     * If none works, bail out. This logic need to be improved but as of now collision of a valid
     * str_to_ip(ip-address) and a valid hostname is minimal. And this approach
     * avoids adding another flag.
     */
    if( str_to_ip(url_entry->host, &address) != 0 )
    {
        WPRINT_LIB_INFO( ("Not a valid IP address?? Trying hostname lookup...\n") );
        result = wiced_hostname_lookup( url_entry->host, &address, 10000, WICED_STA_INTERFACE );
        if( result != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO( ("Host-name lookup failed as well ! bailing out...\n") );
        }
    }

    IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_DNS_RESOLVE_ERROR);

    update_socket_state( websocket, WEBSOCKET_CONNECTING );

    /* Establish secure or non secure tcp connection to server */
    if ( tls_identity != NULL )
    {
        result = websocket_tls_connect( websocket, &address, tls_identity );
    }
    else
    {
        result = websocket_connect( websocket, &address );
    }

    IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_CLIENT_CONNECT_ERROR );

    /* FIXME: Extract Client hadnshake fields from url_entry earlier as part of parse_url().
     * This need to be made better.
     */
    result = set_client_handshake_fields( &handshake, url_entry );

    IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_CLIENT_CONNECT_ERROR );

    result = wiced_establish_websocket_handshake( websocket, &handshake );

    IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_SERVER_HANDSHAKE_RESPONSE_INVALID );

    reset_client_handshake_fields(&handshake);

    /* If a sub protocol was requested, extract it from the handshake */
    if ( url_entry->sec_websocket_protocol != NULL )
    {
        result = wiced_get_websocket_subprotocol( websocket->subprotocol );
        IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_SUBPROTOCOL_NOT_SUPPORTED );
    }

    update_socket_state( websocket, WEBSOCKET_OPEN );
    if( websocket->callbacks.on_open != NULL )
    {
        websocket->callbacks.on_open( websocket );
    }

    GET_TCP_SOCKET_FROM_WEBSOCKET((tcp_socket),(websocket))
    /*Once the communication with server is established, register for the message callback and close callback */
    wiced_tcp_register_callbacks( tcp_socket, NULL, on_websocket_message_callback, on_websocket_close_callback, websocket );

    unlock_websocket_mutex();

    return result;
}

static wiced_result_t websocket_tls_connect( wiced_websocket_t* websocket, wiced_ip_address_t* address, wiced_tls_identity_t*  tls_identity )
{
    wiced_result_t result;
    wiced_tcp_socket_t* tcp_socket = NULL;
    wiced_tls_context_t* context = NULL;

    GET_TCP_SOCKET_FROM_WEBSOCKET((tcp_socket), (websocket))

    if( tls_identity != NULL )
    {
        context = malloc_named("wss", sizeof(wiced_tls_context_t));

        if( context == NULL )
        {
            return WICED_OUT_OF_HEAP_SPACE;
        }

        wiced_tls_init_context( context, tls_identity, NULL );
    }

    WICED_VERIFY( wiced_tcp_create_socket( tcp_socket, WICED_STA_INTERFACE ) );

    if( context != NULL )
    {
        wiced_tcp_enable_tls( tcp_socket, context );
    }

    result = wiced_tcp_connect( tcp_socket, address, 443, 10000 );
    if ( result != WICED_SUCCESS )
    {
        websocket->error_type = WEBSOCKET_CLIENT_CONNECT_ERROR;
        if( websocket->callbacks.on_error != NULL )
        {
            websocket->callbacks.on_error( websocket );
        }
        WPRINT_LIB_INFO(("[%s] Error[%d] connecting @websocket: %p\n", __func__, result, (void *)websocket) );
        wiced_tcp_delete_socket( tcp_socket );
    }

    return result;
}

static wiced_result_t websocket_connect( wiced_websocket_t* websocket, wiced_ip_address_t* address )
{
    wiced_result_t result;
    wiced_tcp_socket_t* tcp_socket = NULL;

    GET_TCP_SOCKET_FROM_WEBSOCKET((tcp_socket), (websocket))

    if( tcp_socket == NULL || websocket->core.role == WEBSOCKET_ROLE_SERVER )
    {
        return WICED_ERROR;
    }

    WICED_VERIFY( wiced_tcp_create_socket( tcp_socket, WICED_STA_INTERFACE ) );

    result = wiced_tcp_connect( tcp_socket, address, 80, 10000 );
    if ( result != WICED_SUCCESS )
    {
        websocket->error_type = WEBSOCKET_CLIENT_CONNECT_ERROR;
        if( websocket->callbacks.on_error != NULL )
        {
            websocket->callbacks.on_error( websocket );
        }
        WPRINT_LIB_INFO(("[%s] Error[%d] connecting @websocket: %p\n", __func__, result, (void *)websocket) );
        wiced_tcp_delete_socket( tcp_socket );
        return result;
    }
    return result;
}

static wiced_result_t websocket_server_send( wiced_websocket_t* websocket, wiced_websocket_frame_t* tx_frame )
{

    wiced_result_t result = WICED_ERROR;
    uint8_t additional_bytes_to_represent_length;
    uint8_t offset;
    uint8_t* payload_ptr;
    int i = 0;
    uint16_t max_websocket_payload;

    wiced_tcp_socket_t* tcp_socket = NULL;

    GET_TCP_SOCKET_FROM_WEBSOCKET( (tcp_socket), (websocket))

    max_websocket_payload = (uint16_t)(tx_frame->payload_buffer_size + MAXIMUM_FRAME_OVERHEAD);
    if( websocket->core.formatted_websocket_frame_length < max_websocket_payload )
    {
        if( websocket->core.formatted_websocket_frame != NULL )
        {
            free( websocket->core.formatted_websocket_frame );
            websocket->core.formatted_websocket_frame = NULL;
        }
        websocket->core.formatted_websocket_frame_length = max_websocket_payload;
        websocket->core.formatted_websocket_frame = malloc( websocket->core.formatted_websocket_frame_length *sizeof(uint8_t) );
    }

    if( !websocket->core.formatted_websocket_frame )
    {
        WPRINT_LIB_INFO(("Error mallocing websocket formatted frame %d\n",websocket->core.formatted_websocket_frame_length ));
        return WICED_ERROR;
    }

    /* Clear frame */
    memset( websocket->core.formatted_websocket_frame, 0, (int)websocket->core.formatted_websocket_frame_length );

    /* Set FIN, RSV and opcode fields of Byte 0 */
    websocket->core.formatted_websocket_frame[ 0 ] = (uint8_t) ( (tx_frame->final_frame ? FIN_MASK : 0x00) | ( (int) tx_frame->payload_type & OPCODE_MASK ) );
    /* Before setting payload, need to find out if extended bits of frame are required */
    if ( tx_frame->payload_length < 126 )
    {
        /* In this case, payload length is represented with 1 byte, and bytes extended payload is not used*/
        websocket->core.formatted_websocket_frame[ 1 ] = (uint8_t) ( tx_frame->payload_length & PAYLOAD_LENGTH_1_BYTE_MASK );

        additional_bytes_to_represent_length = 0;
    }
    else
    {
        /*
         * Check we can buffer the data for sending (needed due to masking). Also ensure its not a control frame,
         * as they have a maximum size limit of 125
         */
        if ( tx_frame->payload_type < WEBSOCKET_CONNECTION_CLOSE )
        {
            websocket->core.formatted_websocket_frame[ 1 ] = (uint8_t) ( 126 );
            websocket->core.formatted_websocket_frame[ 2 ] = (uint8_t) ( tx_frame->payload_length >> 8 );
            websocket->core.formatted_websocket_frame[ 3 ] = (uint8_t) ( tx_frame->payload_length );
            additional_bytes_to_represent_length = 2;
        }
        else /* payload > MAX_PAYLOAD */
        {
            /* unsupported frame size*/
            return WICED_UNSUPPORTED;
        }
    }

    payload_ptr = (uint8_t*) tx_frame->payload;

    offset = (uint8_t)( additional_bytes_to_represent_length + CONTROL_FIELD_BYTES );

    for( i = 0; i < tx_frame->payload_length; i++ )
    {
        websocket->core.formatted_websocket_frame [offset + i ] = payload_ptr[i];

    }
    /* Send frame to server */
    result = wiced_tcp_send_buffer( tcp_socket, websocket->core.formatted_websocket_frame, (uint16_t) ( tx_frame->payload_length + offset ) );

    IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_FRAME_SEND_ERROR );
    return result;
}

static wiced_result_t websocket_client_send( wiced_websocket_t* websocket, wiced_websocket_frame_t* tx_frame )
{
    wiced_result_t result = WICED_ERROR;
    uint8_t additional_bytes_to_represent_length;
    uint8_t masking_key_byte_offset;
    uint16_t max_websocket_payload = 0;

    wiced_tcp_socket_t* tcp_socket = NULL;

    GET_TCP_SOCKET_FROM_WEBSOCKET( (tcp_socket), (websocket))

    max_websocket_payload = (uint16_t)(tx_frame->payload_buffer_size + MAXIMUM_FRAME_OVERHEAD);
    if( websocket->core.formatted_websocket_frame_length < max_websocket_payload )
    {
        if( websocket->core.formatted_websocket_frame != NULL )
        {
            free( websocket->core.formatted_websocket_frame );
            websocket->core.formatted_websocket_frame = NULL;
        }
        websocket->core.formatted_websocket_frame_length = max_websocket_payload;
        websocket->core.formatted_websocket_frame = calloc( websocket->core.formatted_websocket_frame_length, sizeof(uint8_t) );
    }

    /* Clear frame */
    memset( websocket->core.formatted_websocket_frame, 0, (int)websocket->core.formatted_websocket_frame_length );

    /* Set FIN, RSV and opcode fields of Byte 0 */
    websocket->core.formatted_websocket_frame[ 0 ] = (uint8_t) ( (tx_frame->final_frame ? FIN_MASK : 0x00) | ( (int) tx_frame->payload_type & OPCODE_MASK ) );

    /* Before setting payload, need to find out if extended bits of frame are required */
    if ( tx_frame->payload_length < 126 )
    {
        /* In this case, payload length is represented with 1 byte, and bytes extended payload is not used*/
        websocket->core.formatted_websocket_frame[ 1 ] = (uint8_t) ( PAYLOAD_MASK | tx_frame->payload_length );

        additional_bytes_to_represent_length = 0;
    }
    else
    {
        /*
         * Check we can buffer the data for sending (needed due to masking). Also ensure its not a control frame,
         * as they have a maximum size limit of 125
         */
        if ( tx_frame->payload_type < WEBSOCKET_CONNECTION_CLOSE )
        {
            websocket->core.formatted_websocket_frame[ 1 ] = (uint8_t) ( PAYLOAD_MASK | 126 );
            websocket->core.formatted_websocket_frame[ 2 ] = (uint8_t) ( tx_frame->payload_length >> 8 );
            websocket->core.formatted_websocket_frame[ 3 ] = (uint8_t) ( tx_frame->payload_length );
            additional_bytes_to_represent_length = 2;
        }
        else /* payload > MAX_PAYLOAD */
        {
            /* unsupported frame size*/
            return WICED_UNSUPPORTED;
        }
    }

    /* Note: masking key starts after the payload. Two bytes of the control info of frame also need to be considered */
    masking_key_byte_offset = (uint8_t) ( additional_bytes_to_represent_length + CONTROL_FIELD_BYTES );

    /* Generate random mask to be used in masking the frame data*/
    wiced_crypto_get_random( &websocket->core.formatted_websocket_frame[ masking_key_byte_offset ], 4 );

    /* use algorithm of RFC6455, 5.3 to mask the payload only, not the header*/
    mask_unmask_frame_data( tx_frame->payload, &websocket->core.formatted_websocket_frame[ masking_key_byte_offset + MASKING_KEY_BYTES ], tx_frame->payload_length, &websocket->core.formatted_websocket_frame[ masking_key_byte_offset ] );

    /* Send frame to server */
    result = wiced_tcp_send_buffer( tcp_socket, websocket->core.formatted_websocket_frame, (uint16_t) ( tx_frame->payload_length + masking_key_byte_offset + MASKING_KEY_BYTES ) );
    IF_ERROR_RECORD_AND_EXIT( result, WEBSOCKET_FRAME_SEND_ERROR );

    return result;
}

wiced_result_t wiced_websocket_send( wiced_websocket_t* websocket, wiced_websocket_frame_t* tx_frame )
{
    wiced_result_t result = WICED_ERROR;

    if( tx_frame == NULL || websocket == NULL )
    {
        return WICED_BADARG;
    }

    if(websocket->core.role == WEBSOCKET_ROLE_CLIENT)
    {
        result = websocket_client_send(websocket, tx_frame);
    }
    else
    {
        result = websocket_server_send(websocket, tx_frame);
    }

    return result;
}

wiced_result_t wiced_websocket_receive( wiced_websocket_t* websocket, wiced_websocket_frame_t* rx_frame )
{
    wiced_result_t                      result                  = WICED_SUCCESS;
    uint16_t                            payload_length          = 0;
    wiced_websocket_read_frame_state_t  read_state              = READ_CONTROL_BYTES;
    uint32_t                            bytes_read              = 0;
    const char*                         connection_close_string = NULL;
    uint16_t                            total_received_bytes    = 0;
    wiced_packet_t*                     tcp_reply_packet;
    uint16_t                            tcp_data_available;
    uint8_t*                            received_data;
    wiced_tcp_socket_t* tcp_socket = NULL;

    GET_TCP_SOCKET_FROM_WEBSOCKET((tcp_socket), (websocket))

    /*Clear rx buffer */
    memset( rx_frame->payload, 0x0, rx_frame->payload_buffer_size );
    rx_frame->payload_length = 0;

    /* Unpack websocket frame */
    while ( read_state != ( READ_COMPLETED_SUCCESSFULLY ) || ( read_state == READ_FRAME_ERROR ) )
    {
        result = wiced_tcp_receive( tcp_socket, &tcp_reply_packet, WICED_NO_WAIT );
        if ( result != WICED_SUCCESS )
        {
            read_state = READ_FRAME_ERROR;
            break;
        }
        result = wiced_packet_get_data( tcp_reply_packet, 0, (uint8_t**) &received_data, &total_received_bytes, &tcp_data_available );
        if ( result != WICED_SUCCESS )
        {
            read_state = READ_FRAME_ERROR;
            break;
        }

        switch ( read_state )
        {
            case READ_CONTROL_BYTES:
                rx_frame->final_frame = ( received_data[ 0 ] & FIN_MASK ) ? WICED_TRUE : WICED_FALSE;

                /* read RSV bits. Note RSV and extension negotiations are not supported*/
                if ( received_data[ 0 ] & ( RSV1_MASK | RSV2_MASK | RSV3_MASK ) )
                {
                    connection_close_string = RSV_ERR_SHUTDOWN_MESSAGE;
                    read_state = READ_FRAME_ERROR;
                    break;
                }

                /* Read the payload type and pass to the application. Note any ping/pong or close action if required*/
                rx_frame->payload_type = (wiced_websocket_payload_type_t) ( received_data[ 0 ] & OPCODE_MASK );
                if ( ( ( rx_frame->payload_type >= WEBSOCKET_RESERVED_3 ) && ( rx_frame->payload_type <= WEBSOCKET_RESERVED_7 ) ) || ( ( rx_frame->payload_type >= WEBSOCKET_RESERVED_B ) ) )
                {
                    connection_close_string = PAYLOAD_TYPE_ERROR_SHUTDOWN_MESSAGE;
                    read_state = READ_FRAME_ERROR;
                    break;
                }
                read_state++;
                bytes_read++;
                /* Fall through */

            case READ_LENGTH:
                /* Extract the length of the payload*/
                payload_length = received_data[ 1 ] & PAYLOAD_LENGTH_1_BYTE_MASK;

                /* This indicates bytes 2 and 3 of frame are used to store actual payload length*/
                if ( payload_length == 126 )
                {
                    /* Extract Frame bytes 2 and 3*/
                    if ( total_received_bytes < ( CONTROL_FIELD_BYTES + MAX_PAYLOAD_FIELD_BYTES ) )
                    {
                        /* break out of switch and wait for get more data*/
                        break;
                    }
                    payload_length = (uint16_t) ( received_data[ 3 ] | ( received_data[ 2 ] << 8 ) );
                    if ( payload_length > rx_frame->payload_buffer_size )
                    {
                        connection_close_string = LENGTH_SHUTDOWN_MESSAGE;
                        read_state = READ_FRAME_ERROR;
                        break;
                    }
                    bytes_read += 3;
                }
                else if ( payload_length == 127 )
                {
                    /* unsupported payload length*/
                    connection_close_string = LENGTH_SHUTDOWN_MESSAGE;
                    read_state = READ_FRAME_ERROR;
                    break;
                }
                else
                {
                    /* payload length already extracted in second byte*/
                    bytes_read++;
                }
                read_state++;
                /* Fall through */

            case READ_PAYLOAD:
                /* Check if payload needs to be unmasked, and maksing key extracted along with data*/
                if ( ( received_data[ 1 ] & PAYLOAD_MASK ) != 0 )
                {
                    if ( ( total_received_bytes - bytes_read ) < (uint16_t) ( MASKING_KEY_BYTES + payload_length ) )
                    {
                        /* break out of switch and wait for get more data*/
                        break;
                    }
                    mask_unmask_frame_data( &received_data[ bytes_read + MASKING_KEY_BYTES ], rx_frame->payload, payload_length, &received_data[ bytes_read ] );
                }
                else if ( total_received_bytes - bytes_read < (uint16_t) payload_length )
                {
                    /* break out of switch and wait for get more data*/
                    break;
                }
                else
                {
                    if(  rx_frame->payload_buffer_size < payload_length )
                    {
                        memcpy( rx_frame->payload, &received_data[ bytes_read ], rx_frame->payload_buffer_size );
                        wiced_assert("RX Buffer is too small !", rx_frame->payload_buffer_size >= payload_length );
                    }
                    else
                    {
                        memcpy( rx_frame->payload, &received_data[ bytes_read ], payload_length );
                    }
                }

                /*Check if more frames are present in packet and restart state machine */
                read_state++;
                /* Fall through */

            case READ_COMPLETED_SUCCESSFULLY:
                break;

            case READ_FRAME_ERROR:
                /* fall through */

            default:
                read_state = READ_FRAME_ERROR;
                break;
        }

    }

    wiced_packet_delete( tcp_reply_packet );

    /* Check if unpack failed, close connection*/
    if ( read_state == READ_FRAME_ERROR )
    {
        handle_frame_error( websocket, connection_close_string );
        result = WICED_ERROR;
    }
    else if ( rx_frame->payload_type == WEBSOCKET_PING )
    {
        handle_ping_frame( websocket, rx_frame );
    }
    else if ( rx_frame->payload_type == WEBSOCKET_CONNECTION_CLOSE )
    {
        handle_close_connection_frame( websocket, rx_frame );
        /* Did I send a connection close string*/
        //wiced_websocket_close( websocket, NULL );
    }
    else if( rx_frame->payload_type == WEBSOCKET_PONG )
    {
        handle_pong_frame( websocket, rx_frame );
    }

    rx_frame->payload_length = payload_length;
    return result;
}

static void handle_frame_error( wiced_websocket_t* websocket, const char* close_string )
{
    /* If frame error, end-point should initiate Websocket-close connection */
    wiced_websocket_close( websocket, WEBSOCKET_CLOSE_STATUS_CODE_PROTOCOL_ERROR, close_string );
}

static void handle_ping_frame( wiced_websocket_t* websocket , wiced_websocket_frame_t* rx_frame )
{
    wiced_websocket_frame_t local_tx_frame;
    /* pong back with the data received*/
    wiced_websocket_initialise_tx_frame( &local_tx_frame, WICED_TRUE, WEBSOCKET_PONG, rx_frame->payload_length, rx_frame->payload, rx_frame->payload_buffer_size );
    wiced_websocket_send( websocket, &local_tx_frame );
}

static void handle_pong_frame( wiced_websocket_t* websocket, wiced_websocket_frame_t* rx_frame )
{
    UNUSED_PARAMETER(websocket);
    UNUSED_PARAMETER(rx_frame);
    /* Do Nothing */
}

static void handle_close_connection_frame( wiced_websocket_t* websocket, wiced_websocket_frame_t* rx_frame )
{
    /* Do Nothing */
    UNUSED_PARAMETER(websocket);
    UNUSED_PARAMETER(rx_frame);
}

wiced_result_t wiced_websocket_close( wiced_websocket_t* websocket, const uint16_t code, const char* reason )
{
    wiced_result_t          result = WICED_SUCCESS;
    wiced_websocket_frame_t local_tx_frame;
    wiced_tcp_socket_t* tcp_socket = NULL;
    uint8_t payload_length = 0;
    uint8_t body_message[WEBSOCKET_CLOSE_FRAME_BODY_MAX_LENGTH] = { 0 };
    GET_TCP_SOCKET_FROM_WEBSOCKET((tcp_socket), (websocket))

    if( websocket->state == WEBSOCKET_CLOSED || websocket->state == WEBSOCKET_CLOSING )
    {
        /* Nothing to DO */
        return WICED_SUCCESS;
    }

    if( verify_close_code( code ) == WICED_FALSE )
    {
        WPRINT_LIB_ERROR(("[%s] Invalid or Non-standard CLOSE status-code:%d\n", __func__, code ) );
        return WICED_ERROR;
    }

    if( verify_close_reason( reason ) == WICED_FALSE )
    {
        WPRINT_LIB_ERROR(("[%s] Invalid CLOSE reason string\n", __func__) );
        return WICED_ERROR;
    }

    if( code == WEBSOCKET_CLOSE_STATUS_NO_CODE )
    {
        /* there is no body, just send a CLOSE frame */
        wiced_websocket_initialise_tx_frame( &local_tx_frame, WICED_TRUE, WEBSOCKET_CONNECTION_CLOSE, 0,  NULL, 0 );
        result = wiced_websocket_send( websocket, &local_tx_frame );
        if( result != WICED_SUCCESS )
        {
            WPRINT_LIB_ERROR(("Error sending CLOSE frame\n"));
        }
        return result;
    }

    update_socket_state( websocket, WEBSOCKET_CLOSING );

    body_message[0] = (uint8_t) (code >> 8);
    body_message[1] = (uint8_t) (code & 0xFF);
    payload_length = 2;

    if( !reason )
    {
        uint8_t reason_length = (uint8_t)strlen(reason);
        memcpy( &body_message[2], reason, reason_length );
        payload_length = (uint8_t)(payload_length + reason_length );
    }

    wiced_websocket_initialise_tx_frame( &local_tx_frame, WICED_TRUE, WEBSOCKET_CONNECTION_CLOSE, payload_length, (char*) body_message, (uint16_t)sizeof( body_message ) );
    WICED_VERIFY( wiced_websocket_send( websocket, &local_tx_frame ) );

    if( websocket->core.formatted_websocket_frame != NULL )
    {
        free( websocket->core.formatted_websocket_frame );
        websocket->core.formatted_websocket_frame = NULL;
        websocket->core.formatted_websocket_frame_length = 0;
    }

    if( websocket->core.role == WEBSOCKET_ROLE_CLIENT )
    {
        wiced_tcp_disconnect( tcp_socket );

        update_socket_state( websocket, WEBSOCKET_CLOSED );

        wiced_tcp_delete_socket( tcp_socket );
    }
    else
    {
        wiced_websocket_server_t* websocket_server = NULL;
        GET_SERVER_FROM_WEBSOCKET(websocket_server, websocket)
        if( websocket_server == NULL )
        {
            return WICED_BADARG;
        }

        update_socket_state(websocket, WEBSOCKET_CLOSED);
        result = wiced_tcp_server_disconnect_socket(&websocket_server->tcp_server, tcp_socket);
        if( result != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO((" Server disconnecting socket result:%d\n", result));
            return result;
        }
        update_socket_state(websocket, WEBSOCKET_INITIALISED);
    }

    return result;
}

static wiced_result_t update_socket_state( wiced_websocket_t* websocket, wiced_websocket_state_t state )
{
    websocket->state = state;
    return WICED_SUCCESS;
}

static wiced_result_t on_websocket_close_callback(  wiced_tcp_socket_t* socket, void* websocket )
{
    wiced_websocket_t* websocket_ptr = websocket;

    UNUSED_PARAMETER(socket);

    if( websocket_ptr->callbacks.on_close != NULL )
    {
        websocket_ptr->callbacks.on_close( websocket_ptr );
    }

    return WICED_SUCCESS;
}

static wiced_result_t on_websocket_message_callback( wiced_tcp_socket_t* socket, void* websocket )
{
    wiced_websocket_t* websocket_ptr = websocket;

    UNUSED_PARAMETER(socket);

    if( websocket_ptr->callbacks.on_message != NULL )
    {
        websocket_ptr->callbacks.on_message( websocket_ptr );
    }

    return WICED_SUCCESS;
}

static void lock_websocket_mutex( void )
{
    if ( websocket_mutex.mutex_state == MUTEX_UNINITIALISED )
    {
        /* create handhsake mutex. There can only be one connection in a connecting state*/
        wiced_rtos_init_mutex( &websocket_mutex.handshake_lock );
        websocket_mutex.mutex_state = MUTEX_UNLOCKED;
    }

    if ( websocket_mutex.mutex_state == MUTEX_UNLOCKED )
    {
        wiced_rtos_lock_mutex( &websocket_mutex.handshake_lock );
        websocket_mutex.mutex_state = MUTEX_LOCKED;
    }

}

static void unlock_websocket_mutex( void )
{
    if ( websocket_mutex.mutex_state == MUTEX_LOCKED )
    {
        wiced_rtos_unlock_mutex( &websocket_mutex.handshake_lock );
        websocket_mutex.mutex_state = MUTEX_UNLOCKED;
    }
}

wiced_result_t wiced_websocket_register_callbacks ( wiced_websocket_t* websocket, wiced_websocket_callback_t on_open_callback, wiced_websocket_callback_t on_close_callback, wiced_websocket_callback_t on_message_callback, wiced_websocket_callback_t on_error_callback )
{
    websocket->callbacks.on_open    = on_open_callback;
    websocket->callbacks.on_close   = on_close_callback;
    websocket->callbacks.on_error   = on_error_callback;
    websocket->callbacks.on_message = on_message_callback;

    return WICED_SUCCESS;
}

void wiced_websocket_unregister_callbacks ( wiced_websocket_t* websocket )
{
    websocket->callbacks.on_open    = NULL;
    websocket->callbacks.on_close   = NULL;
    websocket->callbacks.on_error   = NULL;
    websocket->callbacks.on_message = NULL;
}

wiced_result_t wiced_websocket_initialise_tx_frame( wiced_websocket_frame_t* tx_frame, wiced_bool_t final_frame, wiced_websocket_payload_type_t payload_type, uint16_t payload_length, void* payload_buffer, uint16_t payload_buffer_size )
{
    if( payload_buffer == NULL )
    {
        return WICED_BADARG;
    }

    tx_frame->final_frame         = final_frame;
    tx_frame->payload_type        = payload_type;
    tx_frame->payload_length      = payload_length;
    tx_frame->payload             = payload_buffer;
    tx_frame->payload_buffer_size = payload_buffer_size;

    return WICED_SUCCESS;
}

wiced_result_t wiced_websocket_initialise_rx_frame( wiced_websocket_frame_t* rx_frame, void* payload_buffer, uint16_t payload_buffer_size )
{
    if( payload_buffer == NULL )
    {
        return WICED_BADARG;
    }

    memset( payload_buffer, 0x0, payload_buffer_size );
    rx_frame->payload             = payload_buffer;
    rx_frame->payload_buffer_size = payload_buffer_size;

    return WICED_SUCCESS;
}
