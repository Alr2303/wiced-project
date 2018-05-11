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
#include "http2.h"
#include "http2_pvt.h"
#include "wiced_tcpip.h"
#include "wiced_tls.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HTTP2_CONNECTION_CHECK_INTERVAL_IN_MSEC     (100)

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
static wiced_result_t http_send_goaway_frame_with_error( http_connection_t* connect, http2_error_codes_t error, wiced_bool_t close_connection );

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t http_connection_init( http_connection_t* connect, http_security_info_t* security, http_connection_callbacks_t* callbacks, void* scratch_buf, uint32_t scratch_buf_len, void* user_data )
{
    wiced_result_t result;

    /* Validate input arguments */
    wiced_assert("Bad argument",  (connect != NULL) && (security != NULL) && (callbacks != NULL) && (scratch_buf != NULL) && (scratch_buf_len != 0));

    /*
     * Make sure that the connection structure is clear to start.
     */

    memset(connect, 0, sizeof(http_connection_t));

    /* Set the remote settings to default */
    connect->local_settings.header_table_size      = HTTP2_SETTINGS_HEADER_TABLE_SIZE;
    connect->local_settings.enable_push            = HTTP2_SETTINGS_ENABLE_PUSH;
    connect->local_settings.initial_window_size    = HTTP2_SETTINGS_INITIAL_WINDOW_SIZE;
    connect->local_settings.max_concurrent_streams = HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS;
    connect->local_settings.max_frame_size         = HTTP2_SETTINGS_MAX_FRAME_SIZE;
    connect->local_settings.max_header_list_size   = HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE;

    /* Set the remote settings to default */
    connect->remote_settings.header_table_size      = HTTP2_SETTINGS_HEADER_TABLE_SIZE_DEFAULT;
    connect->remote_settings.enable_push            = HTTP2_SETTINGS_ENABLE_PUSH_DEFAULT;
    connect->remote_settings.initial_window_size    = HTTP2_SETTINGS_INITIAL_WINDOW_SIZE_DEFAULT;
    connect->remote_settings.max_concurrent_streams = HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS_DEFAULT;
    connect->remote_settings.max_frame_size         = HTTP2_SETTINGS_MAX_FRAME_SIZE_DEFAULT;
    connect->remote_settings.max_header_list_size   = HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE_DEFAULT;

    if (callbacks != NULL)
    {
        connect->callbacks = *callbacks;
    }

    /* Validate if the scratch_buf buffer size is sufficient */
    if (scratch_buf_len < HTTP2_FRAME_SCRATCH_BUFF_SIZE)
    {
        HTTP_DEBUG(( " ### ERR : [%s()] : [%d] : Insufficient scratch buffer size. Given scratch buffer size [%u], Minimum size should be [%d]\n", __FUNCTION__, __LINE__, (unsigned int)scratch_buf_len, HTTP2_FRAME_SCRATCH_BUFF_SIZE ));
        return WICED_BADARG;
    }

    connect->header_table   = (uint8_t*)scratch_buf;
    connect->frame_scratch_buf = (uint8_t*)scratch_buf + connect->local_settings.header_table_size;
    connect->socket_ptr     = &connect->socket;
    connect->security       = security;
    connect->user_data      = user_data;

    /* Initialize state */
    connect->state.connection  = HTTP_CONNECTION_NOT_CONNECTED;
    HTTP_DEBUG(( "[%s()] : [%d] : Connection state set to HTTP_CONNECTION_NOT_CONNECTED\n", __FUNCTION__, __LINE__ ));

    if ( ( result = wiced_rtos_init_semaphore( &connect->semaphore ) ) != WICED_SUCCESS )
    {
        HTTP_DEBUG(( "[%s()] : [%d] : Failed in wiced_rtos_init_semaphore(). Error = [%d]\n", __FUNCTION__, __LINE__, result ));
        return result;
    }
    wiced_rtos_set_semaphore( &connect->semaphore );
    return http2_network_init(connect);
}

wiced_result_t http_connection_deinit( http_connection_t* connect )
{
    wiced_result_t result;
    if ( ( result = wiced_rtos_get_semaphore( &connect->semaphore, HTTP_CLIENT_CONNECTION_TIMEOUT ) ) != WICED_SUCCESS )
    {
        HTTP_DEBUG(( "[%s()] : [%d] : Failed in wiced_rtos_get_semaphore(). Error = [%d]\n", __FUNCTION__, __LINE__, result ));
        return result;
    }

    HTTP_DEBUG(( "[%s()] : [%d] : DeInitialize network\n", __FUNCTION__, __LINE__ ));
    result = http2_network_deinit(connect);

    wiced_rtos_deinit_semaphore( &connect->semaphore );

    return result;
}

wiced_result_t http_connect( http_connection_t* connect, wiced_ip_address_t* address, uint16_t portnumber, wiced_interface_t interface )
{
    wiced_result_t  result;
    http_packet_t   packet;
    uint32_t        length;
    uint16_t        size_available = 0;
    int32_t         i;
    uint16_t        packet_size = HTTP2_PREFACE_LEN          /* size of magic */
                                  + ( HTTP2_SETTINGS_FRAME_MAX_SIZE + HTTP2_FRAME_FIXED_HEADER_LEN ); /* maximum size of a settings frame */

    if ( ( result = wiced_rtos_get_semaphore( &connect->semaphore, HTTP_CLIENT_CONNECTION_TIMEOUT ) ) != WICED_SUCCESS )
    {
        HTTP_DEBUG(( "[%s()] : [%d] : Failed in wiced_rtos_get_semaphore(). Error = [%d]\n", __FUNCTION__, __LINE__, result ));
        return result;
    }

    if ( connect->state.connection  == HTTP_CONNECTION_NOT_CONNECTED )
    {
        connect->state.frame_fragment    = 0;
        connect->state.last_valid_stream = 0;
        connect->state.goaway_stream     = 0;
        connect->state.goaway_error      = 0;
#ifdef HTTP_SERVER
        connect->state.stream_id         = HTTP2_SERVER_INITIAL_STREAM_ID;
#else
        connect->state.stream_id         = HTTP2_CLIENT_INITIAL_STREAM_ID;
#endif
        result = http_network_tls_init(connect->socket_ptr, connect->security);
        if (result != WICED_SUCCESS)
        {
            HTTP_DEBUG(( "[%s()] : [%d] : Failed in http_network_tls_init(). Error = [%d]\n", __FUNCTION__, __LINE__, result ));
            goto http_connect_END;
        }

        /* Establish connection with HTTP/2.0 server */
        result = http_network_connect( connect->socket_ptr, address, portnumber, interface );
        if ( result != WICED_SUCCESS )
        {
            HTTP_DEBUG(( "[%s()] : [%d] : Failed in http_network_connect(). Error = [%d]\n", __FUNCTION__, __LINE__, result ));
            goto http_connect_END;
        }
        connect->state.connection = HTTP_CONNECTION_CONNECTED;
        HTTP_DEBUG(( "[%s()] : [%d] : Connection state set to HTTP_CONNECTION_CONNECTED\n", __FUNCTION__, __LINE__ ));

        /* Send SETTINGS frame to HTTP/2.0 server */
        result = http_network_create_packet( connect->socket_ptr, &packet, packet_size, &size_available );
        if (result != WICED_SUCCESS)
        {
            HTTP_DEBUG(( "[%s()] : [%d] : Failed in http_network_create_packet(). Error = [%d]\n", __FUNCTION__, __LINE__, result ));
            goto http_connect_END;
        }

        /* Add the preface to the beginning of settings frame */
        http_frame_put_preface( packet.data );
        packet.data += HTTP2_PREFACE_LEN;

        /* Add settings information */
        http_frame_put_settings( (http2_frame_t*)packet.data, &connect->local_settings, 0 );
        length = READ_BE24( packet.data ) + HTTP2_FRAME_FIXED_HEADER_LEN;
        packet.data += length;

        HTTP_DEBUG(( "[%s()] : [%d] : Send SETTINGS Frame\n", __FUNCTION__, __LINE__ ));
        http_network_send_packet( connect->socket_ptr, &packet );
        connect->state.connection = HTTP_CONNECTION_READY_LOCAL;

        wiced_rtos_set_semaphore( &connect->semaphore );

        /* Wait for the SETTINGS frames to be exchanged from the remote */
        for (i = 0; i < HTTP2_CONNECTION_TIMEOUT_IN_MSEC / HTTP2_CONNECTION_CHECK_INTERVAL_IN_MSEC; i++)
        {
            if (connect->state.connection == HTTP_CONNECTION_READY)
            {
                HTTP_DEBUG(( "[%s()] : [%d] : Connection READY!\n", __FUNCTION__, __LINE__));
                break;
            }

            HTTP_DEBUG(( "[%s()] : [%d] : Waiting for connection to be established. Current State = [%d]\n", __FUNCTION__, __LINE__, connect->state.connection ));
            wiced_rtos_delay_milliseconds(HTTP2_CONNECTION_CHECK_INTERVAL_IN_MSEC);
        }

        if (connect->state.connection != HTTP_CONNECTION_READY)
        {
            /* SETTINGS exchange not completed with in timeout. Treat it as connection error with SETTINGS_TIMEOUT */
            HTTP_DEBUG(( "[%s()] : [%d] : === ERR : SETTINGS exchange with peer didn't complete before [%d msec]. Treating it as connection error\n", __FUNCTION__, __LINE__, HTTP2_CONNECTION_TIMEOUT_IN_MSEC));
            result = WICED_TIMEOUT;
        }

        return result;
    }
    else
    {
        result = WICED_ALREADY_CONNECTED;
    }

http_connect_END:
    wiced_rtos_set_semaphore( &connect->semaphore );

    return result;
}

wiced_result_t http_disconnect( http_connection_t* connect )
{
    wiced_result_t result;

    if ( ( result = wiced_rtos_get_semaphore( &connect->semaphore, HTTP_CLIENT_CONNECTION_TIMEOUT ) ) != WICED_SUCCESS )
    {
        return result;
    }
    HTTP_DEBUG(( "[%s()] : [%d] : Connection State = [%d]\n", __FUNCTION__,__LINE__, connect->state.connection ));
    switch ( connect->state.connection )
    {
        case HTTP_CONNECTION_NOT_CONNECTED:
            result = WICED_NOT_CONNECTED;
            break;
        case HTTP_CONNECTION_READY:
        case HTTP_CONNECTION_READY_LOCAL:
        case HTTP_CONNECTION_READY_REMOTE:
        {
            /* Ok, preface has gone out, let's be nice and send a GOAWAY message */
            uint16_t size = (uint16_t) ( 9 /* frame header */
                                       + 8 /* Sream ID and Error code */
                                       );
            http_packet_t packet;
            uint16_t      size_available = 0;

            if ( ( result = http_network_create_packet( connect->socket_ptr, &packet, size, &size_available ) ) != WICED_SUCCESS )
            {
                result = WICED_OUT_OF_HEAP_SPACE;
            }
            else
            {
                if ( ( result = http_frame_put_goaway( (http2_frame_t*)packet.data, connect->state.last_valid_stream,  HTTP2_ERROR_CODE_NO_ERROR ) != WICED_SUCCESS ) )
                {
                    http_network_delete_packet( &packet );
                }
                else
                {
                    connect->state.connection = HTTP_CONNECTION_GOAWAY;
                    HTTP_DEBUG(( "[%s()] : [%d] : Connection state set to HTTP_CONNECTION_GOAWAY\n", __FUNCTION__,__LINE__ ));

                    packet.data += READ_BE24( packet.data ) + 9;
                    http_network_send_packet( connect->socket_ptr, &packet );
                }
            }
        }
        /* Fall through to close the connection */
        case HTTP_CONNECTION_GOAWAY:
        case HTTP_CONNECTION_CONNECTED:
            /* We haven't sent the preface, let's close silently */
            result = http_network_disconnect( connect->socket_ptr );
            connect->state.connection = HTTP_CONNECTION_NOT_CONNECTED;
            HTTP_DEBUG(( "[%s()] : [%d] : Connection state set to HTTP_CONNECTION_NOT_CONNECTED\n", __FUNCTION__, __LINE__ ));
            break;

        default:
            result = WICED_ERROR;
    }

    http_network_tls_deinit(connect->socket_ptr);

    wiced_rtos_set_semaphore( &connect->semaphore );

    return result;
}

wiced_result_t http_request_create( http_connection_t* connect, http_request_id_t* request )
{
    wiced_result_t result;

    if ( ( result = wiced_rtos_get_semaphore( &connect->semaphore, HTTP_CLIENT_CONNECTION_TIMEOUT ) ) != WICED_SUCCESS )
    {
        return result;
    }

    if ( connect->state.connection != HTTP_CONNECTION_READY )
    {
        result = WICED_NOTUP;
    }
    else
    {
        uint32_t        stream_id = connect->state.stream_id + 2;
        uint16_t        size = HTTP2_FRAME_FIXED_HEADER_LEN;
        http_packet_t   packet;
        uint16_t        size_available = 0;

        /* Send the header frame to create the stream */
        HTTP_DEBUG(( "[%s()] : [%d] : Create packet \n", __FUNCTION__,__LINE__ ));
        /* TODO: Handle the request size which is grater than network packet MTU size */
        if ( ( result = http_network_create_packet( connect->socket_ptr, &packet, size, &size_available ) ) != WICED_SUCCESS )
        {
            HTTP_DEBUG(( "[%s()] : [%d] : ERR : Network packets not available. Error = [%d]\n", __FUNCTION__,__LINE__, result ));
            result = WICED_OUT_OF_HEAP_SPACE;
            goto http_request_create_END;
        }
        else
        {
            size = MIN(size, size_available);
            if ( ( result = http_frame_put_headers( (http2_frame_t*)packet.data, NULL,  0x0, stream_id, size, HTTP2_FRAME_TYPE_HEADERS ) != WICED_SUCCESS ) )
            {
                http_network_delete_packet( &packet );
            }
            else
            {
                packet.data += READ_BE24( packet.data ) + HTTP2_FRAME_FIXED_HEADER_LEN;
                HTTP_DEBUG(( "[%s()] : Send the packet [%d] : \n", __FUNCTION__,__LINE__ ));
                http_network_send_packet( connect->socket_ptr, &packet );
            }
        }

        HTTP_DEBUG(( "[%s()] : [%d] : Request [%u] created successfully\n", __FUNCTION__, __LINE__, (unsigned int)stream_id ));
        connect->state.stream_id = stream_id;
        *request = stream_id;
    }

http_request_create_END:
    wiced_rtos_set_semaphore( &connect->semaphore );

    return result;
}

wiced_result_t http_request_put_headers ( http_connection_t* connect, http_request_id_t request, http_header_info_t* headers, uint8_t flags )
{
    wiced_result_t result;

    /* Validate input arguments */
    if ((connect == NULL) || ((uint32_t)request == 0) || (headers == NULL) || (flags & ~(HTTP_REQUEST_FLAGS_FINISH_REQUEST | HTTP_REQUEST_FLAGS_FINISH_HEADERS)))
    {
        return WICED_BADARG;
    }

    if ( ( result = wiced_rtos_get_semaphore( &connect->semaphore, HTTP_CLIENT_CONNECTION_TIMEOUT ) ) != WICED_SUCCESS )
    {
        return result;
    }

    if ( connect->state.connection != HTTP_CONNECTION_READY )
    {
        result = WICED_NOTUP;
        goto http_request_put_headers_END;
    }
    else
    {
        uint16_t        size = (uint16_t)connect->local_settings.max_frame_size;
        http_packet_t   packet;
        uint16_t        size_available = 0;
        wiced_bool_t    is_finishreq = WICED_FALSE;

        HTTP_DEBUG(( "[%s()] : [%d] : Create packet \n", __FUNCTION__,__LINE__ ));
        /* TODO: Handle the request size which is grater than network packet MTU size */
        if ( ( result = http_network_create_packet( connect->socket_ptr, &packet, size, &size_available ) ) != WICED_SUCCESS )
        {
            HTTP_DEBUG(( "[%s()] : [%d] : ERR : Network packets not available. Error = [%d]\n", __FUNCTION__,__LINE__, result ));
            result = WICED_OUT_OF_HEAP_SPACE;
            goto http_request_put_headers_END;
        }
        else
        {
            HTTP_DEBUG(( "[%s()] : [%d] : Requested packet size = [%d], Given packet size = [%d]\n", __FUNCTION__,__LINE__, size, size_available));

            /* Check if Padding (or) Priority frame is set. If yes, return an error, since it's not supported */
            if ((flags & HTTP2_FRAME_HEADER_FRAME_FLAG_PADDED) || (flags & HTTP2_FRAME_HEADER_FRAME_FLAG_PRIORITY))
            {
                HTTP_DEBUG(( "[%s()] : [%d] : Incorrect flags = [%d]\n", __FUNCTION__,__LINE__, flags ));
                result = WICED_BADARG;
                goto http_request_put_headers_END;
            }

            if (flags & HTTP_REQUEST_FLAGS_FINISH_REQUEST)
            {
                is_finishreq = WICED_TRUE;
                flags = (flags & ((uint8_t)~HTTP_REQUEST_FLAGS_FINISH_REQUEST));
            }

            HTTP_DEBUG(( "[%s()] : [%d] : Put HEADER Frame for stream ID =[%u] with flags = [0x%02X]\n", __FUNCTION__,__LINE__, (unsigned int)request, (int)flags));
            size = MIN(size, size_available);
            if ( ( result = http_frame_put_headers( (http2_frame_t*)packet.data, headers,  flags, (uint32_t)request, size, HTTP2_FRAME_TYPE_CONTINUATION ) != WICED_SUCCESS ) )
            {
                http_network_delete_packet( &packet );
            }
            else
            {
                packet.data += READ_BE24( packet.data ) + HTTP2_FRAME_FIXED_HEADER_LEN;
                HTTP_DEBUG(( "[%s()] : Send the packet [%d] : \n", __FUNCTION__,__LINE__ ));
                http_network_send_packet( connect->socket_ptr, &packet );
            }

            if (is_finishreq)
            {
                size = HTTP2_FRAME_FIXED_HEADER_LEN;

                if ( ( result = http_network_create_packet( connect->socket_ptr, &packet, size, &size_available ) ) != WICED_SUCCESS )
                {
                    HTTP_DEBUG(( "[%s()] : [%d] : No more packets available\n", __FUNCTION__,__LINE__ ));
                    result = WICED_OUT_OF_HEAP_SPACE;
                    goto http_request_put_headers_END;
                }

                HTTP_DEBUG(( "[%s()] : [%d] : Put DATA Frame for stream ID =[%u] with flags = [0x%02X]\n", __FUNCTION__,__LINE__, (unsigned int)request, (int)HTTP_REQUEST_FLAGS_FINISH_REQUEST));
                http_frame_put_data( ( http2_frame_t* )packet.data, NULL, 0, HTTP_REQUEST_FLAGS_FINISH_REQUEST, (uint32_t)request );

                packet.data += READ_BE24( packet.data ) + HTTP2_FRAME_FIXED_HEADER_LEN;
                http_network_send_packet( connect->socket_ptr, &packet );
            }
        }
    }

http_request_put_headers_END:
    wiced_rtos_set_semaphore( &connect->semaphore );

    return result;
}

wiced_result_t http_request_put_data ( http_connection_t* connect, http_request_id_t request, uint8_t* data, uint32_t length, uint8_t flags )
{
    wiced_result_t  result;
    uint32_t        data_remaining;
    uint32_t        data_tobe_sent;
    uint8_t         send_flag;
    uint32_t        data_offset;

    if ( ( result = wiced_rtos_get_semaphore( &connect->semaphore, HTTP_CLIENT_CONNECTION_TIMEOUT ) ) != WICED_SUCCESS )
    {
        return result;
    }

    if ( connect->state.connection != HTTP_CONNECTION_READY )
    {
        result = WICED_NOTUP;
    }
    else
    {
        data_remaining = length;
        data_offset = 0;
        do
        {
            uint16_t        size = (uint16_t)( data_remaining + sizeof( http2_frame_t ) - 1 );
            http_packet_t   packet;
            uint16_t        size_available = 0;

            if ( ( result = http_network_create_packet( connect->socket_ptr, &packet, size, &size_available ) ) != WICED_SUCCESS )
            {
                HTTP_DEBUG(( "[%s()] : [%d] : No more packets available\n", __FUNCTION__,__LINE__ ));
                result = WICED_OUT_OF_HEAP_SPACE;
                break;
            }

            /* Check if it's a last chunk to be sent. Else set the flag to 0 */
            send_flag = (size_available >= size) ? flags : 0;

            data_tobe_sent = MIN(size, size_available) - (sizeof( http2_frame_t ) - 1);
            http_frame_put_data( ( http2_frame_t* )packet.data, data + data_offset, data_tobe_sent, send_flag, (uint32_t)request );
            data_offset += data_tobe_sent;

            packet.data += READ_BE24( packet.data ) + (sizeof( http2_frame_t ) - 1);
            http_network_send_packet( connect->socket_ptr, &packet );

            data_remaining -= data_tobe_sent;
            HTTP_DEBUG(( "[%s()] : [%d] : Remaining Data to be sent = [%lu]\n", __FUNCTION__,__LINE__, data_remaining ));
        } while ( data_remaining > 0 );
    }

    HTTP_DEBUG(( "[%s()] : [%d] : Set the semaphore\n", __FUNCTION__,__LINE__ ));
    wiced_rtos_set_semaphore( &connect->semaphore );
    HTTP_DEBUG(( "[%s()] : [%d] : Semaphore set\n", __FUNCTION__,__LINE__ ));

    return result;

}

wiced_result_t http_ping_request ( http_connection_t* connect, uint8_t data[HTTP2_PING_DATA_LENGTH], uint8_t flags )
{
    wiced_result_t  result;
    http_packet_t   packet;
    uint16_t        size = (uint16_t)( HTTP2_PING_DATA_LENGTH + sizeof( http2_frame_t ) - 1 );
    uint16_t        size_available = 0;

    if ( ( result = wiced_rtos_get_semaphore( &connect->semaphore, HTTP_CLIENT_CONNECTION_TIMEOUT ) ) != WICED_SUCCESS )
    {
        return result;
    }

    if ( connect->state.connection != HTTP_CONNECTION_READY )
    {
        result = WICED_NOTUP;
    }
    else
    {
        if ( ( result = http_network_create_packet( connect->socket_ptr, &packet, size, &size_available ) ) != WICED_SUCCESS )
        {
            HTTP_DEBUG(( "[%s()] : [%d] : No more packets available\n", __FUNCTION__,__LINE__ ));
            result = WICED_OUT_OF_HEAP_SPACE;
            wiced_rtos_set_semaphore( &connect->semaphore );
            return result;
        }

        http_frame_put_ping( ( http2_frame_t* )packet.data, data, flags );
        packet.data += READ_BE24( packet.data ) + (sizeof( http2_frame_t ) - 1);;

        if ( ( result = http_network_send_packet( connect->socket_ptr, &packet ) ) != WICED_SUCCESS )
        {
            HTTP_DEBUG(( "[%s()] : [%d] : HTTP2 Packet sent failed\n", __FUNCTION__,__LINE__ ));
        }

        HTTP_DEBUG(( "[%s()] : [%d] : Ping request sent.\n", __FUNCTION__,__LINE__ ));
    }

    HTTP_DEBUG(( "[%s()] : [%d] : Set the semaphore\n", __FUNCTION__,__LINE__ ));
    wiced_rtos_set_semaphore( &connect->semaphore );
    HTTP_DEBUG(( "[%s()] : [%d] : Semaphore set\n", __FUNCTION__,__LINE__ ));

    return result;

}

static wiced_result_t http_send_goaway_frame_with_error( http_connection_t* connect, http2_error_codes_t error, wiced_bool_t close_connection )
{
    wiced_result_t      result = WICED_SUCCESS;
    http_packet_t       packet;
    uint16_t            size_available = 0;
    uint16_t            packet_size = (uint16_t) ( HTTP2_FRAME_FIXED_HEADER_LEN /* frame header */
                               + HTTP2_GOAWAY_FRAME_SIZE );

    HTTP_DEBUG(( "[%s()] : [%d] : Send GOAWAY Frame with error = [0x%X]\n", __FUNCTION__,__LINE__, error ));

    /* Send our GOAWAY frame to indicate error */
    if ( connect->state.connection != HTTP_CONNECTION_GOAWAY )
    {
        connect->state.connection = HTTP_CONNECTION_GOAWAY;
        HTTP_DEBUG(( "[%s()] : [%d] : Connection state set to HTTP_CONNECTION_GOAWAY\n", __FUNCTION__, __LINE__ ));
        if ( ( result = http_network_create_packet( connect->socket_ptr, &packet, packet_size, &size_available ) ) != WICED_SUCCESS )
        {
            result = WICED_OUT_OF_HEAP_SPACE;
        }
        else
        {
            if ( ( result = http_frame_put_goaway( (http2_frame_t*)packet.data, connect->state.last_valid_stream, error ) != WICED_SUCCESS ) )
            {
                http_network_delete_packet( &packet );
            }
            else
            {
                packet.data += packet_size;
                HTTP_DEBUG(( "[%s()] : [%d] : Sending GOAWAY Frame with error = [0x%X]\n", __FUNCTION__,__LINE__, error ));
                http_network_send_packet( connect->socket_ptr, &packet );
            }
        }
    }

    if ( close_connection )
    {
        /* Close TCP connection */
        HTTP_DEBUG(( "[%s()] : [%d] : Connection state set to HTTP_CONNECTION_NOT_CONNECTED\n", __FUNCTION__, __LINE__ ));
        connect->state.connection = HTTP_CONNECTION_NOT_CONNECTED;

        HTTP_DEBUG(( "[%s()] : [%d] : Close the TCP connection\n", __FUNCTION__,__LINE__ ));
        result = http_network_disconnect( connect->socket_ptr );
        if ( connect->callbacks.http_disconnect_callback )
        {
            /* Should be pushed out of semaphore */
            connect->callbacks.http_disconnect_callback( connect, connect->state.last_valid_stream, error, connect->user_data );
        }
    }

    return result;
}

static wiced_result_t http_process_frame( http_connection_t* connect, http2_frame_t* frame )
{
    wiced_result_t      result = WICED_SUCCESS;
    http_packet_t       packet;
    uint16_t            size_available = 0;
    uint32_t            frame_size = READ_BE24( frame->length );
    wiced_bool_t        close_connection = WICED_FALSE;

    HTTP_DEBUG(( "Received frame size:%u, type:0x%x, stream ID:%d\n", (unsigned int)frame_size, (unsigned int)frame->type, (unsigned int)htobe32( frame->stream_id ) ));
    HTTP_DEBUG(( "[%s()] : [%d] : Frame type = [%d]\n", __FUNCTION__,__LINE__, (unsigned int)(unsigned int)frame->type ));

    /* Check if the frame-size is bigger than frame-size agreed */
    if (frame_size > (HTTP2_SETTINGS_MAX_FRAME_SIZE + HTTP2_FRAME_FIXED_HEADER_SIZE))
    {
        /* Send GOAWAY frame to indicate the error */
        HTTP_DEBUG((" ===== WARN : Received Frame size exceeded by [%u] bytes. It's a FRAME_SIZE_ERROR\n", (unsigned int)(frame_size - (HTTP2_SETTINGS_MAX_FRAME_SIZE + HTTP2_FRAME_FIXED_HEADER_SIZE))));

        /* Check for connection error */
        if ( ( frame->stream_id == 0 ) || (frame->type == HTTP2_FRAME_TYPE_HEADERS) || (frame->type == HTTP2_FRAME_TYPE_SETTINGS) || (frame->type == HTTP2_FRAME_TYPE_CONTINUATION) || (frame->type == HTTP2_FRAME_TYPE_PUSH_PROMISE) )
        {
            close_connection = WICED_TRUE;
        }

        /* Send GOAWAY frame with error HTTP2_ERROR_CODE_FRAME_SIZE_ERROR */
        return http_send_goaway_frame_with_error(connect, HTTP2_ERROR_CODE_FRAME_SIZE_ERROR, close_connection);
    }

    switch ( frame->type )
    {
        case HTTP2_FRAME_TYPE_SETTINGS:
        {
            uint32_t length;
            uint8_t  flags = 0;

            HTTP_DEBUG(( "[%s()] : [%d] : SETTINGS Frame received from remote\n", __FUNCTION__,__LINE__ ));

            /* Ensure the frame size is multiple of 6 octet */
            if ( (frame_size % HTTP2_SETTINGS_FRAME_PARAMETER_LEN) != 0 )
            {
                /* Invalid SETTINGS Frame size. Treat it as FRAME_SIZE_ERROR and close the connection */
                return http_send_goaway_frame_with_error(connect, HTTP2_ERROR_CODE_FRAME_SIZE_ERROR, close_connection);
            }

            http_frame_get_settings( frame, &connect->remote_settings, &flags );
            HTTP_DEBUG(( "[%s()] : [%d] : Remote SETTINGS Frame processed\n", __FUNCTION__,__LINE__ ));

            /* Check if the settings frames are exchanged as part of initial connection handshake */
            if ( connect->state.connection != HTTP_CONNECTION_READY )
            {
                if ( ( flags & HTTP2_FRAME_FLAGS_SETTINGS_ACK ) == 0 )
                {
                    /*  Remote SETTINGS received. Send the ACK */
                    uint16_t    packet_size = HTTP2_FRAME_FIXED_HEADER_LEN;

                    HTTP_DEBUG(( "[%s()] : [%d] : Remote SETTINGS received for the initial connection handshake\n", __FUNCTION__,__LINE__ ));
                    connect->state.connection = HTTP_CONNECTION_READY_REMOTE;

                    /*  Send settings ACK frame. */
                    HTTP_DEBUG(( "[%s()] : [%d] : Send settings ACK \n", __FUNCTION__,__LINE__ ));
                    if ((result = http_network_create_packet( connect->socket_ptr, &packet, packet_size, &size_available )) != WICED_SUCCESS)
                    {
                        HTTP_DEBUG(( "[%s()] : [%d] : Error failed to create packet for sending SETTINGS Frame. Error = [%d]\n", __FUNCTION__,__LINE__, result ));
                        goto END_http_process_frame;
                    }
                    http_frame_put_ack( (http2_frame_t*)packet.data, WICED_TRUE );
                    length = READ_BE24( packet.data ) + HTTP2_FRAME_FIXED_HEADER_LEN;
                    packet.data += length;
                    http_network_send_packet( connect->socket_ptr, &packet );
                }
                else
                {
                    /* Check if SETTINGS ACK from remote is received after SETTINGS frame */
                    HTTP_DEBUG(( "[%s()] : [%d] : Current connection state = [%d]\n", __FUNCTION__,__LINE__, connect->state.connection ));
//                    if (connect->state.connection == HTTP_CONNECTION_READY_REMOTE)
                    {
                        /*  Received SETTINGS ACK from remote */
                        HTTP_DEBUG(( "[%s()] : [%d] : Received SETTINGS ACK from remote \n", __FUNCTION__,__LINE__ ));
                        connect->state.connection = HTTP_CONNECTION_READY;
                    }
//                    else
//                    {
//                        /*  Received SETTINGS ACK from remote without sending SETTINGS frame. Treat it as error condition */
//                        HTTP_DEBUG(( "[%s()] : [%d] : === ERR : Received SETTINGS ACK from remote before SETTINGS frame. Treating this as error\n", __FUNCTION__,__LINE__ ));
//                        /* TODO: Error handling to be added */
//                    }
                }
            }
            break;
        }
        case HTTP2_FRAME_TYPE_HEADERS:
        {
            http_header_info_t head;
            uint32_t      header_size = frame_size;
            uint8_t*      headers     = frame->data;
            uint8_t*      headers_buffer = connect->frame_scratch_buf;
            uint32_t      headers_buffer_size = connect->local_settings.max_header_list_size;
            HTTP_DEBUG(( "[%s()] : [%d] : HEADER Frame\n", __FUNCTION__,__LINE__ ));
            if ( frame->flags & HTTP2_FRAME_FLAGS_DATA_PADDED )
            {
                uint8_t pad_length = frame->data[0];
                headers++;
                header_size -= (uint32_t)( pad_length + 1 );
            }
            if ( frame->flags & HTTP2_FRAME_FLAGS_DATA_PRIORITY )
            {
                uint32_t    parent_stream = *((uint32_t*)headers);
                uint8_t     weight;

                parent_stream = htobe32( parent_stream & 0x7FFFFFFF );
                headers += 4;
                weight = *headers++;
                header_size -= 5;
                (void) weight;
                (void) parent_stream;
            }
            if ( ( result = http_hpack_decode_headers( &head, &headers_buffer, &headers_buffer_size, &headers, &header_size ) ) != WICED_SUCCESS )
            {
                /* COMPRESSION_ERROR - Send GOAWAY Frame and close the connection */
                HTTP_DEBUG((" ===== WARN : It's a COMPRESSION_ERROR\n"));
                return http_send_goaway_frame_with_error(connect, HTTP2_ERROR_CODE_COMPRESSION_ERROR, WICED_TRUE);
            }
            else
            {
                connect->state.last_valid_stream = htobe32( frame->stream_id );
                if ( connect->callbacks.http_request_recv_header_callback )
                {
                    HTTP_DEBUG(( "[%s()] : [%d] : Invoke frame header callback\n", __FUNCTION__,__LINE__ ));
                    /* Should be pushed out of semaphore */
                    connect->callbacks.http_request_recv_header_callback( connect, (http_request_id_t)htobe32(frame->stream_id), head.next, HTTP_FRAME_TYPE_HEADER,
                            frame->flags & ( HTTP_REQUEST_FLAGS_FINISH_REQUEST | HTTP_REQUEST_FLAGS_FINISH_HEADERS ), connect->user_data );
                }
            }
            break;
        }
        case HTTP2_FRAME_TYPE_DATA:
        {
            uint8_t*      frame_data     = frame->data;
            uint32_t      data_size = frame_size;
            HTTP_DEBUG(( "[%s()] : [%d] : DATA Frame\n", __FUNCTION__,__LINE__ ));
            if ( frame->flags & HTTP2_FRAME_FLAGS_DATA_PADDED )
            {
                uint8_t pad_length = frame->data[0];
                frame_data += ( pad_length + 1 );
                data_size -= (uint32_t)( pad_length + 1 );
            }
            connect->state.last_valid_stream = htobe32( frame->stream_id );
            if ( connect->callbacks.http_request_recv_data_callback )
            {
                HTTP_DEBUG(( "[%s()] : [%d] : Invoke Data callback. Size of the data = [%u]\n", __FUNCTION__,__LINE__, (unsigned int)data_size ));
                /* Should be pushed out of semaphore */
                connect->callbacks.http_request_recv_data_callback( connect, (http_request_id_t)htobe32(frame->stream_id), frame_data, data_size, frame->flags, connect->user_data );
            }
            /* Send Window update frame. */
            HTTP_DEBUG(( "[%s()] : [%d] : Send window update frame\n", __FUNCTION__,__LINE__ ));
            if ((result = http_network_create_packet( connect->socket_ptr, &packet, 13, &size_available /* frame header + window update size */ )) != WICED_SUCCESS)
            {
                HTTP_DEBUG(( "[%s()] : [%d] : Error failed to create packet for sending WINDOW_UPDATE Frame. Error = [%d]\n", __FUNCTION__,__LINE__, result ));
                goto END_http_process_frame;
            }
            http_frame_put_window_update( (http2_frame_t*)packet.data, htobe32(frame->stream_id), frame_size + 9 );
            packet.data += READ_BE24( packet.data ) + 9;
            http_network_send_packet( connect->socket_ptr, &packet );

            if ((result = http_network_create_packet( connect->socket_ptr, &packet, 13, &size_available /* frame header + window update size */ )) != WICED_SUCCESS)
            {
                HTTP_DEBUG(( "[%s()] : [%d] : Error failed to create packet for sending WINDOW_UPDATE Frame. Error = [%d]\n", __FUNCTION__,__LINE__, result ));
                goto END_http_process_frame;
            }
            http_frame_put_window_update( (http2_frame_t*)packet.data, 0, frame_size + 9 );
            packet.data += READ_BE24( packet.data ) + 9;
            http_network_send_packet( connect->socket_ptr, &packet );

            break;
        }
        case HTTP2_FRAME_TYPE_PUSH_PROMISE:
        {
            http_header_info_t head;
            uint32_t      header_size = frame_size;
            uint8_t*      headers     = frame->data;
            uint32_t      new_stream;
            uint8_t*      headers_buffer = connect->frame_scratch_buf;
            uint32_t      headers_buffer_size = connect->local_settings.max_header_list_size;
            HTTP_DEBUG(( "[%s()] : [%d] : PUSH Frame \n", __FUNCTION__,__LINE__ ));
            if ( frame->flags & HTTP2_FRAME_FLAGS_DATA_PADDED )
            {
                uint8_t pad_length = frame->data[0];
                headers++;
                header_size -= (uint32_t)( pad_length + 1 );
            }
            new_stream = *( (uint32_t*)headers );
            new_stream = htobe32( new_stream );
            headers += 4;
            header_size -= 4;
            if ( ( result = http_hpack_decode_headers( &head, &headers_buffer, &headers_buffer_size, &headers, &header_size ) ) != WICED_SUCCESS )
            {
                /* Should be treated as error */
            }
            else
            {
                if ( connect->callbacks.http_request_recv_header_callback )
                {
                    /* Should be pushed out of semaphore */
                    connect->callbacks.http_request_recv_header_callback( connect, (http_request_id_t)(new_stream), head.next, HTTP_FRAME_TYPE_PUSH_PROMISE, HTTP_REQUEST_FLAGS_PUSH_PROMISE, connect->user_data );
                }
            }
            break;
        }
        case HTTP2_FRAME_TYPE_GOAWAY:
        {
            http2_goaway_frame_t* goaway = ( http2_goaway_frame_t* )frame->data;

            connect->state.goaway_stream    = htobe32( goaway->last_stream_id );
            connect->state.goaway_error     = htobe32( goaway->error_code );
            HTTP_DEBUG(( "Received GOAWAY message error: %u\n", (unsigned int)connect->state.goaway_error ));
            if ( connect->state.connection != HTTP_CONNECTION_GOAWAY )
            {
                /* Server is closing the connection, Lets reply  with our GOAWAY */
                uint16_t packet_size = (uint16_t) ( HTTP2_FRAME_FIXED_HEADER_LEN /* frame header */
                                           + HTTP2_GOAWAY_FRAME_SIZE );
                connect->state.connection = HTTP_CONNECTION_GOAWAY;
                HTTP_DEBUG(( "[%s()] : [%d] : Connection state set to HTTP_CONNECTION_GOAWAY\n", __FUNCTION__, __LINE__ ));
                if ( ( result = http_network_create_packet( connect->socket_ptr, &packet, packet_size, &size_available ) ) != WICED_SUCCESS )
                {
                    result = WICED_OUT_OF_HEAP_SPACE;
                }
                else
                {
                    if ( ( result = http_frame_put_goaway( (http2_frame_t*)packet.data, connect->state.last_valid_stream,  HTTP2_ERROR_CODE_NO_ERROR ) != WICED_SUCCESS ) )
                    {
                        http_network_delete_packet( &packet );
                    }
                    else
                    {
                        packet.data += packet_size;
                        HTTP_DEBUG(( "[%s()] : [%d] : Send GOAWAY Frame\n", __FUNCTION__,__LINE__ ));
                        http_network_send_packet( connect->socket_ptr, &packet );
                    }
                }
            }
            break;
        }
        case HTTP2_FRAME_TYPE_WINDOW_UPDATE:
        {
            uint32_t* window_size_be = ( uint32_t* )frame->data;
            uint32_t  window_size = htobe32( *window_size_be );
            UNUSED_VARIABLE(window_size);
            HTTP_DEBUG( ( "Received window update:%u\n", (unsigned int)window_size) );

            break;
        }
        case HTTP2_FRAME_TYPE_PING:
        {
            uint8_t data[HTTP2_PING_DATA_LENGTH];
            uint8_t flags = HTTP2_FRAME_FLAGS_PING_ACK;

            HTTP_DEBUG(( "Received PING message, flags = [%d]\n", (unsigned int)frame->flags ));
            if ( (frame->stream_id != 0x00) || (frame_size != HTTP2_PING_DATA_LENGTH) )
            {
                HTTP_DEBUG(( "/* Connection error = PROTOCOL_ERROR */ \n" ));
                /* Connection error = PROTOCOL_ERROR */
            }

            if ( frame->flags & HTTP2_FRAME_FLAGS_PING_ACK )
            {
                HTTP_DEBUG(( "/* PONG flag already set */ \n" ));
                /* PONG flag already set */

                if ( connect->callbacks.http_request_recv_header_callback )
                {
                    HTTP_DEBUG(( "[%s()] : [%d] : Invoke frame header callback\n", __FUNCTION__,__LINE__ ));
                    /* Should be pushed out of semaphore */
                    connect->callbacks.http_request_recv_header_callback( connect, (http_request_id_t)htobe32(frame->stream_id), NULL, HTTP_FRAME_TYPE_PING, HTTP2_FRAME_FLAGS_PING_ACK, connect->user_data );
                }
            }
            else
            {
                HTTP_DEBUG(( "Sending PING ack, flags = [%d]\n", flags ));
                memcpy( &data, frame->data, HTTP2_PING_DATA_LENGTH );
                http_ping_request ( connect, data, flags );
            }
            break;
        }
        default:
            break;
    }

END_http_process_frame:
    return result;
}

wiced_result_t http_receive_callback( uint8_t* data, uint32_t* size, void* arg )
{
    wiced_result_t     result = WICED_SUCCESS;
    http2_frame_t*     frame;
    http_connection_t* connect = (http_connection_t*)arg;
    uint8_t            frame_header_size = sizeof( http2_frame_t ) - 1;
    uint32_t           recv_size = (*size);
    uint32_t           frame_size;
//    uint32_t           i = 0;

    if ( ( result = wiced_rtos_get_semaphore( &connect->semaphore, HTTP_CLIENT_CONNECTION_TIMEOUT ) ) != WICED_SUCCESS )
    {
        return result;
    }

    HTTP_DEBUG(( "[%s()] : [%d] : Received data size = [%u]\n", __FUNCTION__,__LINE__, (unsigned int)*size));
    if ( connect->state.frame_fragment == 0 )
    {
        /* We are not in the middle of packet fragmentation. Should be a start of frame data */
        /* Check if entire frame header is received */
        if ( recv_size >= frame_header_size )
        {
            /* got full frame header */
            frame      = ( http2_frame_t* ) data;
            frame_size = READ_BE24( frame->length ) + frame_header_size;
            if ( recv_size >= frame_size )
            {
                /* Received full frame, no fragmentation */
                /* http process frame */
                HTTP_DEBUG(( "[%s()] : [%d] : Received full frame. Process the frame\n", __FUNCTION__, __LINE__ ));
#if 0
                HTTP_DEBUG(( "[%s()] : [%d] : Dumping the packet data[\n", __FUNCTION__, __LINE__ ));
                for (i = 0; i < recv_size; i++)
                {
                    printf("0x%02X ", data[i]);
                    if ((i + 1) % 16 == 0)
                    {
                        printf("\n");
                    }
                }
                printf("]\n");
                HTTP_DEBUG(( "[%s()] : [%d] : Dumping the packet data[\n", __FUNCTION__, __LINE__ ));
                for (i = 0; i < recv_size; i++)
                {
                    printf("%c", data[i]);
                    if ((i + 1) % 16 == 0)
                    {
                        printf("\n");
                    }
                }
                printf("]\n");
#endif
                result = http_process_frame( connect, frame );
                recv_size -= frame_size;
            }
            else
            {
                /* Frame not fully received
                 * Copy current data to header scratch buffer for now.
                 * PS: We can't receive any other data except the remaining of the
                 * fragment.
                 */
                HTTP_DEBUG(( "[%s()] : [%d] : Received partial frame. Copying [%u] bytes of data\n", __FUNCTION__, __LINE__, (unsigned int)recv_size ));
                memcpy( connect->frame_scratch_buf, frame, recv_size );
                connect->state.frame_fragment = recv_size;

                HTTP_DEBUG(("Total frame fragment size = [%u]\n", (unsigned int)connect->state.frame_fragment));

                recv_size = 0;
            }
        }
        else
        {
            /* Frame header is not completely received. Store partial frame header in frame scratch buffer */
            memcpy( connect->frame_scratch_buf, data, recv_size );
            connect->state.frame_fragment = recv_size;
            recv_size = 0;
        }
    }
    else
    {
        /* We are in the middle of packet fragmentation */
        uint32_t remaining_size;
        HTTP_DEBUG(( "[%s()] : [%d] : In the middle of packet fragmentation\n", __FUNCTION__, __LINE__ ));
        if ( connect->state.frame_fragment < frame_header_size )
        {
            /*
             * Copy the remaining part of the header.
             * It is safe to assume that we have enough size for the remaining
             * of the frame header, as it is only 9 bytes.
             */
            remaining_size = frame_header_size - connect->state.frame_fragment;
            memcpy( connect->frame_scratch_buf + connect->state.frame_fragment, data, remaining_size );
            connect->state.frame_fragment = frame_header_size;
            recv_size -= remaining_size;
            data += remaining_size;
        }
        /* At least we have received full header, let's check what size we need. */
        frame      = ( http2_frame_t* ) connect->frame_scratch_buf;
        frame_size = READ_BE24( frame->length ) + frame_header_size;
        remaining_size = frame_size - connect->state.frame_fragment;
        if ( recv_size >= remaining_size )
        {
            /* Received full frame, no fragmentation */
            /* http process frame from headers scratch */
            HTTP_DEBUG ( ( "Copying remaining %u bytes\r\n", (unsigned int)remaining_size ) );
            memcpy( connect->frame_scratch_buf + connect->state.frame_fragment, data, remaining_size );
            recv_size -= remaining_size;
            connect->state.frame_fragment = 0;
            if ( frame->type == HTTP2_FRAME_TYPE_HEADERS )
            {
                /* We can't support in place decompression for now */
                result = WICED_ERROR;
            }
            else
            {
                /* call frame process for frame in headers */
                HTTP_DEBUG ( ( "Full frame received, processing now..\r\n" ) );
                result = http_process_frame( connect, frame );
            }
        }
        else
        {
            /* Frame not fully received
             * Copy current data to header scratch buffer for now.
             * PS: We can't receive any other data except the remaining of the
             * fragment.
             */
            memcpy( connect->frame_scratch_buf + connect->state.frame_fragment, data, recv_size );
            connect->state.frame_fragment += recv_size;
            HTTP_DEBUG(("Total frame fragment size = [%u]\n", (unsigned int)connect->state.frame_fragment));
            recv_size = 0;
        }
    }

    HTTP_DEBUG(( "[%s()] : [%d] : Size of data which is not processed = [%u]\n", __FUNCTION__,__LINE__, (unsigned int)recv_size));
    *size = recv_size;
    wiced_rtos_set_semaphore( &connect->semaphore );

    return result;
}

wiced_result_t http_disconnect_callback( void* arg )
{
    wiced_result_t result;
    http_connection_t* connect = (http_connection_t*)arg;
    if ( ( result = wiced_rtos_get_semaphore( &connect->semaphore, HTTP_CLIENT_CONNECTION_TIMEOUT ) ) != WICED_SUCCESS )
    {
        return result;
    }
    connect->state.connection = HTTP_CONNECTION_NOT_CONNECTED;
    HTTP_DEBUG(( "[%s()] : [%d] : Connection state set to HTTP_CONNECTION_NOT_CONNECTED\n", __FUNCTION__, __LINE__ ));

    /* We already sent a previous GOAWAY, this is probably the response */
    HTTP_DEBUG(( "[%s()] : [%d] : Disconnect Network\n", __FUNCTION__, __LINE__ ));
    result = http_network_disconnect( connect->socket_ptr );
    if ( connect->callbacks.http_disconnect_callback )
    {
        /* Should be pushed out of semaphore */
        connect->callbacks.http_disconnect_callback( connect, connect->state.goaway_stream, connect->state.goaway_error, connect->user_data );
    }
    wiced_rtos_set_semaphore( &connect->semaphore );
    return result;
}
