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

/** @websocket_server.c
 *
 * Implementation of Websocket server APIs declared in websocket_server.h
 *
 */

#include "websocket.h"
#include "websocket_internal.h"
#include "websocket_handshake.h"
#include "wiced_rtos.h"
#include "internal/wiced_internal_api.h"
#include "wiced_tls.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_WEBSOCKET_SERVER_PORT                 (80)
#define WICED_WEBSOCKET_SECURE_SERVER_PORT          (443)
#define WEBSOCKET_SERVER_HELPER_THREAD_STACK_SIZE  (4000)
#define WEBSOCKET_SERVER_HELPER_THREAD_PRIORITY    (WICED_DEFAULT_LIBRARY_PRIORITY)
#define WEBSOCKET_SERVER_HELPER_THREAD_QUEUE_SIZE  (10)
#define WEBSOCKET_SERVER_DAEMON_THREAD_PRIORITY     (WICED_DEFAULT_LIBRARY_PRIORITY)
#define WEBSOCKET_SERVER_DAEMON_THREAD_STACK_SIZE   (5000)
#define WEBSOCKET_SERVER_QUEUE_SIZE                 (10)
/* If running persistent multiple socket connections, we can not queue on a socket */
#define PERSISTANT_TCP_SERVER_TCP_LISTEN_QUEUE_SIZE 0

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef enum
{
    TCP_ACCEPTED,
    HANDSHAKE_TIMED_OUT,
    DATA_PACKET_RECEIVED,
    HANDSHAKE_PACKET_RECEIVED,
} wiced_websocket_internal_event_t;

typedef struct
{
    wiced_websocket_internal_event_t    websocket_event;
    wiced_websocket_t*                  websocket;
} wiced_websocket_server_queue_element_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

wiced_result_t websocket_post_event( wiced_websocket_server_queue_element_t new_entry );

/******************************************************
 *               Variable Definitions
 ******************************************************/

wiced_worker_thread_t   helper_thread;
wiced_thread_t          websocketd;
wiced_queue_t           websocket_server_queue;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t websocket_post_event( wiced_websocket_server_queue_element_t new_entry )
{
    wiced_result_t result = WICED_ERROR;
    result = wiced_rtos_push_to_queue( &websocket_server_queue, (void *)&new_entry, WICED_NO_WAIT );
    if( result != WICED_SUCCESS )
    {
        WPRINT_LIB_INFO( ( "[%s] Failed to push an element to Server-Queue\n", __func__ ) );
    }
    return result;
}

static void do_websocket_loop( uint32_t arg )
{
    wiced_result_t result;
    wiced_websocket_server_t* server = (wiced_websocket_server_t* )arg;
    UNUSED_PARAMETER(arg);

    while( WICED_TRUE )
    {
        // if there is an event to be processed
        // -- fetch the event
        // -- take action on the event
        // Go back to get an event
        wiced_websocket_server_queue_element_t message;
        result = wiced_rtos_pop_from_queue( &websocket_server_queue, &message, WICED_NEVER_TIMEOUT );
        if( result != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO( ("[%s] Error receiving from queue (err: %d)\n", __func__, result) );
            continue;
        }

        switch ( message.websocket_event )
        {
            case TCP_ACCEPTED:
            {
                WPRINT_LIB_INFO( ("[%s] TCP accepted\n", __func__) );
                break;
            }

            case HANDSHAKE_PACKET_RECEIVED:
            {
                wiced_tcp_socket_t* socket = NULL;
                GET_TCP_SOCKET_FROM_WEBSOCKET(socket, message.websocket)
                WPRINT_LIB_INFO(("[%s] HANDSHAKE packet received @sock: %p\n", __func__, (void *)socket) );
                result = wiced_establish_websocket_handshake( message.websocket, NULL );

                if( result != WICED_SUCCESS )
                {
                    /* Disconnect socket if client handshake failed */
                    result = wiced_tcp_server_disconnect_socket( &server->tcp_server, socket );
                    WPRINT_LIB_INFO(("[%s] TCP server @sock: %p disconnecting(%d)\n", __func__, (void *)socket, result) );
                }
                else if( message.websocket->callbacks.on_open )
                {
                    message.websocket->callbacks.on_open(message.websocket);
                    message.websocket->state = WEBSOCKET_OPEN;
                }
                break;
            }

            case DATA_PACKET_RECEIVED:
            {
                wiced_tcp_socket_t* socket = NULL;
                GET_TCP_SOCKET_FROM_WEBSOCKET(socket, message.websocket)
                WPRINT_LIB_INFO(("[%s] DATA_PACKET received @socket:%p\n", __func__, (void *)socket) );
                if( message.websocket->callbacks.on_message )
                {
                    message.websocket->callbacks.on_message(message.websocket);
                }
                break;
            }

            case HANDSHAKE_TIMED_OUT:
            default:
                break;
        }
    };

    return;
}

static wiced_result_t websocket_server_deferred_connect_callback( void *arg )
{
    wiced_websocket_server_queue_element_t message;
    wiced_tls_context_t* context;
    wiced_tcp_socket_t* socket          = (wiced_tcp_socket_t*)arg;
    wiced_websocket_t*  websocket       = (wiced_websocket_t*)socket->callback_arg;
    wiced_websocket_server_t* server    = NULL;
    wiced_tcp_server_t* tcp_server      = NULL;
    wiced_result_t result = WICED_ERROR;

    GET_SERVER_FROM_WEBSOCKET(server, websocket)
    if( server == NULL )
    {
        return WICED_BADARG;
    }

    GET_TCP_SERVER_FROM_WEBSOCKET_SERVER(tcp_server, server)

#if 1
    if ( server->tcp_server.tls_identity != NULL )
    {
        if ( socket->tls_context == NULL )
        {
            context = malloc_named("wss", sizeof(wiced_tls_context_t));
            if (context == NULL)
            {
                WPRINT_LIB_INFO(("[%s] failed to get memory for tls-context\n", __func__));
                return WICED_OUT_OF_HEAP_SPACE;
            }
            socket->context_malloced = WICED_TRUE;
        }
        else
        {
            context = socket->tls_context;
        }
        wiced_tls_init_context( context, server->tcp_server.tls_identity, NULL );
        wiced_tcp_enable_tls( socket, context );
    }
#endif
    result = wiced_tcp_server_accept( tcp_server, socket );
    if( result != WICED_SUCCESS )
    {
        WPRINT_LIB_INFO( ( "[%s] Failed to accept socket - res:%d @websock: %p @sock: %p\n", __func__, result, (void*)websocket, (void*)socket ) );
        wiced_tcp_server_disconnect_socket( tcp_server, socket );
        websocket->error_type = WEBSOCKET_ACCEPT_ERROR;
        websocket->callbacks.on_error( websocket );
        return result;
    }

    message.websocket           = websocket;
    message.websocket_event     = TCP_ACCEPTED;

    WICED_VERIFY( websocket_post_event( message) );

    return WICED_SUCCESS;
}

static wiced_result_t websocket_server_deferred_receive_callback( void *arg )
{
    wiced_tcp_socket_t* socket       = (wiced_tcp_socket_t*)arg;
    wiced_websocket_t*  websocket    = (wiced_websocket_t*)socket->callback_arg;
    wiced_websocket_server_queue_element_t message;

    switch(websocket->state)
    {
        case WEBSOCKET_INITIALISED:
        case WEBSOCKET_CONNECTING:
            message.websocket           = websocket;
            message.websocket_event     = HANDSHAKE_PACKET_RECEIVED;
            WICED_VERIFY( websocket_post_event( message ) );
            break;
        case WEBSOCKET_OPEN:
            message.websocket           = websocket;
            message.websocket_event     = DATA_PACKET_RECEIVED;
            WICED_VERIFY( websocket_post_event( message ) );
            break;
        case WEBSOCKET_CLOSED:
        case WEBSOCKET_CLOSING:
        case WEBSOCKET_UNINITIALISED:
        default:
            WPRINT_LIB_INFO( ("receive_callback:%d\n", websocket->state) );
            break;
    }

    return WICED_SUCCESS;
}

static wiced_result_t websocket_server_deferred_disconnect_callback( void* arg )
{
    wiced_tcp_socket_t* socket      = (wiced_tcp_socket_t*)arg;
    wiced_websocket_t*  websocket   = (wiced_websocket_t*)socket->callback_arg;
    WPRINT_LIB_INFO( ("disconnect callback @websocket: %p @sock: %p\n",(void *)websocket, (void*)socket ) );
    switch(websocket->state)
    {
        case WEBSOCKET_INITIALISED:
        case WEBSOCKET_OPEN:
        case WEBSOCKET_CLOSED:
            break;
        case WEBSOCKET_CONNECTING:
        case WEBSOCKET_CLOSING:
        case WEBSOCKET_UNINITIALISED:
        default:
            break;
        break;
    }
    return WICED_SUCCESS;
}

static wiced_result_t server_socket_connect_callback( wiced_tcp_socket_t* socket, void* arg )
{
    /* Got a TCP connection request */
    UNUSED_PARAMETER(arg);
    return wiced_rtos_send_asynchronous_event( &helper_thread, websocket_server_deferred_connect_callback, (void*)socket );
}

static wiced_result_t server_socket_disconnect_callback( wiced_tcp_socket_t* socket, void* args)
{
    /* TCP socket disconnection */
    UNUSED_PARAMETER(args);
    return wiced_rtos_send_asynchronous_event( &helper_thread, websocket_server_deferred_disconnect_callback, (void*)socket );
}

static wiced_result_t server_socket_receive_callback( wiced_tcp_socket_t* socket, void* arg)
{
    UNUSED_PARAMETER(arg);
    return wiced_rtos_send_asynchronous_event( &helper_thread, websocket_server_deferred_receive_callback, (void*)socket );
}

static wiced_result_t set_up_internal_server( wiced_websocket_server_t* server, wiced_tls_identity_t* tls_identity )
{
    memset( &server->tcp_server, 0, sizeof(wiced_tcp_server_t) );

    if( tls_identity != NULL )
    {
        wiced_tcp_server_enable_tls( &server->tcp_server, tls_identity );
    }

    server->config  = NULL;
    server->sockets = NULL;

    return WICED_SUCCESS;
}

static wiced_result_t clean_up_websockets_server( wiced_websocket_server_t* server)
{
    linked_list_deinit( &server->socket_list );

    free(server->sockets);
    return WICED_SUCCESS;
}

static wiced_result_t set_up_websockets_server( wiced_websocket_server_t* server, wiced_websocket_callbacks_t* callbacks )
{
    int i;
    uint8_t max_websockets = server->config->max_connections;
    wiced_websocket_t* server_socket_list = NULL;

    if ( max_websockets > WICED_MAXIMUM_NUMBER_OF_SERVER_WEBSOCKETS )
    {
        return WICED_BADARG;
    }

    linked_list_init( &server->socket_list );

    server_socket_list = server->sockets;

    if( server_socket_list != NULL )
    {
        /* Expect websockets-pointer to be NULL when we set-up the server */
        return WICED_BADARG;
    }

    server_socket_list = malloc( sizeof(wiced_websocket_t) * max_websockets );
    if( server_socket_list == NULL )
    {
        return WICED_OUT_OF_HEAP_SPACE;
    }

    memset( server_socket_list, 0, sizeof(wiced_websocket_t) * max_websockets );

    for ( i = 0; i < max_websockets; i++ )
    {
        server_socket_list[i].core.role                         = WEBSOCKET_ROLE_SERVER;
        server_socket_list[i].core.socket                       = NULL;
        server_socket_list[i].core.args                         = server;
        server_socket_list[i].state                             = WEBSOCKET_INITIALISED;
        server_socket_list[i].core.formatted_websocket_frame         = NULL;
        server_socket_list[i].core.formatted_websocket_frame_length  = 0;

        server_socket_list[i].callbacks.on_open     = callbacks->on_open;
        server_socket_list[i].callbacks.on_error    = callbacks->on_error;
        server_socket_list[i].callbacks.on_message  = callbacks->on_message;
        server_socket_list[i].callbacks.on_close    = callbacks->on_close;
        // TODO: rest of websocket_t initialization follows
    }

    server->sockets = server_socket_list;

    return WICED_SUCCESS;
}


static void internal_websocket_nx_tcp_listen_callback( NX_TCP_SOCKET* socket_ptr, UINT port )
{
    wiced_tcp_socket_t* socket = (wiced_tcp_socket_t*)socket_ptr;
    UNUSED_PARAMETER( port );

    internal_defer_tcp_callback_to_wiced_network_thread( socket, socket->callbacks.connect );
}

/* Nothing changes here as compared to wiced_tcp_server_start except that for each initialized tcp_socket, we will tie
 * up a websocket as callback_arg. Will give us a direct mapping between a tcp_socket_t and websocket_t
 */
static wiced_result_t internal_websocket_tcp_server_start( wiced_tcp_server_t* tcp_server,  uint16_t port, uint16_t max_sockets, wiced_tcp_socket_callback_t connect_callback, wiced_tcp_socket_callback_t receive_callback, wiced_tcp_socket_callback_t disconnect_callback, void* arg )
{
    int                        i;
    wiced_tcp_server_socket_t* tcp_socket = NULL;
    wiced_result_t             status;
    wiced_websocket_server_t*  server = NULL;
    wiced_websocket_t*         websocket = NULL;
    uint32_t                   netx_status;

    tcp_server->interface = WICED_STA_INTERFACE;
    tcp_server->port      = port;

    server = (wiced_websocket_server_t* )arg;

    if( arg == NULL )
    {
        return WICED_BADARG;
    }

    /* first websocket in the list */
    websocket = server->sockets;
    if( websocket == NULL )
    {
        return WICED_BADARG;
    }

    linked_list_init( &tcp_server->socket_list );

    if ( max_sockets > WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS )
    {
        return WICED_BADARG;
    }

    tcp_socket = malloc( sizeof(wiced_tcp_server_socket_t) * max_sockets );
    if ( tcp_socket == NULL )
    {
        return WICED_OUT_OF_HEAP_SPACE;
    }

    memset( tcp_socket, 0, sizeof(wiced_tcp_server_socket_t) * max_sockets );

    for ( i = 0; i < max_sockets; i++ )
    {
        /* create tcp socket */
        status = wiced_tcp_create_socket ( &tcp_socket[i].socket, tcp_server->interface );
        if( status != WICED_SUCCESS )
        {
            goto clean_up;
        }

        /* get websocket */
        arg = &(websocket[i]);
        if( arg == NULL )
        {
            goto clean_up;
        }

        /* First fill socket-pointer of websocket with this newly created tcp-socket */
        websocket[i].core.socket = &tcp_socket[i].socket;

        /* Then pass this websocket object as callback_arg to tcp-socket
         * This way, websocket can be retrieved direclty when tcp-callbacks happen
         */
        status = wiced_tcp_register_callbacks( &tcp_socket[i].socket, connect_callback, receive_callback, disconnect_callback, arg );
        if( status != WICED_SUCCESS )
        {
            goto clean_up;
        }

        linked_list_set_node_data( &tcp_socket[i].socket_node, &tcp_socket[i] );
        linked_list_insert_node_at_rear( &tcp_server->socket_list, &tcp_socket[i].socket_node );
    }

    tcp_server->max_tcp_connections     = max_sockets;

    tcp_server->active_tcp_connections  = 0;
    /* start server listen */
    netx_status = nx_tcp_server_socket_listen( tcp_socket[0].socket.socket.nx_tcp_socket_ip_ptr,
                            tcp_server->port, &tcp_socket[0].socket.socket, PERSISTANT_TCP_SERVER_TCP_LISTEN_QUEUE_SIZE,
                            internal_websocket_nx_tcp_listen_callback );

    if( netx_status != NX_SUCCESS )
    {
        goto clean_up;
    }

    return WICED_SUCCESS;

clean_up:
    free( tcp_socket );

    return status;
}

/* Start the server; open the sockets and listen on them */
wiced_result_t wiced_websocket_server_start ( wiced_websocket_server_t* server, wiced_websocket_server_config_t* config,
                                                    wiced_websocket_callbacks_t* callbacks, wiced_tls_identity_t* tls_identity )
{
    uint16_t port = WICED_WEBSOCKET_SERVER_PORT;

    if( server == NULL || config == NULL || callbacks == NULL )
    {
        return WICED_BADARG;
    }

    //tls_identity = NULL;

    WICED_VERIFY( set_up_internal_server(server, tls_identity) );


    /* FIXME: It is possible for a websocket server to handle non-secure requests even when tls_identity is provided by application
     * For now, we are not handling those cases */
    if( tls_identity != NULL )
    {
        port = WICED_WEBSOCKET_SECURE_SERVER_PORT;
    }

    server->config = config;

    WICED_VERIFY( set_up_websockets_server( server, callbacks ) );

    WICED_VERIFY( wiced_rtos_init_queue(&websocket_server_queue, "websocketd_queue",
                                sizeof(wiced_websocket_server_queue_element_t), WEBSOCKET_SERVER_QUEUE_SIZE ) );

    /* Start helper-thread for processing tcp events */
    WICED_VERIFY( wiced_rtos_create_worker_thread( &helper_thread, WEBSOCKET_SERVER_HELPER_THREAD_PRIORITY,
                                WEBSOCKET_SERVER_HELPER_THREAD_STACK_SIZE, WEBSOCKET_SERVER_HELPER_THREAD_QUEUE_SIZE ) );

    /* Start TCP server - it will start listening on first available socket */
    WICED_VERIFY( internal_websocket_tcp_server_start(&(server->tcp_server), port, config->max_connections, server_socket_connect_callback,
                            server_socket_receive_callback, server_socket_disconnect_callback, server) );

    /* Start Main thread for processing websocket events */
    WICED_VERIFY( wiced_rtos_create_thread(&websocketd, WEBSOCKET_SERVER_DAEMON_THREAD_PRIORITY, "websocketd",
                                do_websocket_loop, WEBSOCKET_SERVER_DAEMON_THREAD_STACK_SIZE, server ) );

    return WICED_SUCCESS;
}


static wiced_result_t do_server_shutdown( wiced_websocket_server_t* server )
{
    int i = 0;
    wiced_result_t result = WICED_ERROR;
    wiced_websocket_t*  wbs_ptr = NULL;
    for( ; i < server->config->max_connections; i++ )
    {
        wbs_ptr = server->sockets;
        if( !wbs_ptr && wbs_ptr->state == WEBSOCKET_OPEN )
        {
            result = wiced_websocket_close( wbs_ptr, WEBSOCKET_CLOSE_STATUS_CODE_GOING_AWAY, NULL );
            if( result != WICED_SUCCESS )
            {
                WPRINT_LIB_INFO( ( "[%s] Failed to send close-frame @websocket :%p\n", __func__, (void *)wbs_ptr ) );
                continue;
            }
        }
        wbs_ptr++;
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_websocket_server_stop ( wiced_websocket_server_t* server )
{

    /* Send a CLOSE frame for all active connections */
    do_server_shutdown(server);

    /* free up websocket-server resources etc. */
    WICED_VERIFY( wiced_tcp_server_stop(&server->tcp_server) );

    WICED_VERIFY( wiced_rtos_delete_thread(&websocketd) );

    WICED_VERIFY( wiced_rtos_delete_worker_thread(&helper_thread) );

    WICED_VERIFY( wiced_rtos_deinit_queue(&websocket_server_queue) );

    clean_up_websockets_server(server);

    return WICED_SUCCESS;
}

