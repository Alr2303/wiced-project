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
#pragma once

#include "wiced.h"
#include "websocket_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NOTE:
 *
 * Current Limitations:
 *  - This implementation can only support receiving single frames on packet boundaries
 */

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define SUB_PROTOCOL_STRING_LENGTH  10
#define SUB_EXTENSION_STRING_LENGTH 10

#define WEBSOCKET_CLOSE_FRAME_STATUS_CODE_LENGTH    2
#define WEBSOCKET_CLOSE_FRAME_MAX_REASON_LENGTH     123
#define WEBSOCKET_CLOSE_FRAME_BODY_MAX_LENGTH       ( WEBSOCKET_CLOSE_FRAME_STATUS_CODE_LENGTH + WEBSOCKET_CLOSE_FRAME_MAX_REASON_LENGTH )

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WEBSOCKET_UNINITIALISED = 0, /* The WebSocket in uninitialised */
    WEBSOCKET_INITIALISED,       /* The WebSocket in initialised */
    WEBSOCKET_CONNECTING,        /* The connection has not yet been established */
    WEBSOCKET_OPEN,              /* The WebSocket connection is established and communication is possible */
    WEBSOCKET_CLOSING,           /* The connection is going through the closing handshake */
    WEBSOCKET_CLOSED,            /* The connection has been closed or could not be opened */
} wiced_websocket_state_t;

typedef enum
{
    WEBSOCKET_CONTINUATION_FRAME = 0,
    WEBSOCKET_TEXT_FRAME,
    WEBSOCKET_BINARY_FRAME,
    WEBSOCKET_RESERVED_3,
    WEBSOCKET_RESERVED_4,
    WEBSOCKET_RESERVED_5,
    WEBSOCKET_RESERVED_6,
    WEBSOCKET_RESERVED_7,
    WEBSOCKET_CONNECTION_CLOSE,
    WEBSOCKET_PING,
    WEBSOCKET_PONG,
    WEBSOCKET_RESERVED_B,
    WEBSOCKET_RESERVED_C,
    WEBSOCKET_RESERVED_D,
    WEBSOCKET_RESERVED_E,
    WEBSOCKET_RESERVED_F
} wiced_websocket_payload_type_t;

typedef enum
{
    WEBSOCKET_NO_ERROR                              = 0,
    WEBSOCKET_CLIENT_CONNECT_ERROR                  = 1,
    WEBSOCKET_NO_AVAILABLE_SOCKET                   = 2,
    WEBSOCKET_SERVER_HANDSHAKE_RESPONSE_INVALID     = 3,
    WEBSOCKET_CREATE_SOCKET_ERROR                   = 4,
    WEBSOCKET_FRAME_SEND_ERROR                      = 5,
    WEBSOCKET_HANDSHAKE_SEND_ERROR                  = 6,
    WEBSOCKET_PONG_SEND_ERROR                       = 7,
    WEBSOCKET_RECEIVE_ERROR                         = 8,
    WEBSOCKET_DNS_RESOLVE_ERROR                     = 9,
    WEBSOCKET_SUBPROTOCOL_NOT_SUPPORTED             = 10,
    WEBSOCKET_ACCEPT_ERROR                          = 11
} wiced_websocket_error_t;

typedef enum
{
    WEBSOCKET_CLOSE_STATUS_NO_CODE                      = 0,

    WEBSOCKET_CLOSE_STATUS_CODE_NORMAL                  = 1000,
    WEBSOCKET_CLOSE_STATUS_CODE_GOING_AWAY,
    WEBSOCKET_CLOSE_STATUS_CODE_PROTOCOL_ERROR,
    WEBSOCKET_CLOSE_STATUS_CODE_DATA_NOT_ACCEPTED,
    WEBSOCKET_CLOSE_STATUS_CODE_RESERVED,
    WEBSOCKET_CLOSE_STATUS_CODE_RESERVED_NO_STATUS_CODE, // MUST NOT be set as a status code in Close Control frame
    WEBSOCKET_CLOSE_STATUS_CODE_RESERVED_ABNORMAL_CLOSE, // MUST NOT be set as a status code in Close Control frame
    WEBSOCKET_CLOSE_STATUS_CODE_DATA_INCONSISTENT,
    WEBSOCKET_CLOSE_STATUS_CODE_POLICY_VIOLATION,
    WEBSOCKET_CLOSE_STATUS_CODE_DATA_TOO_BIG,
    WEBSOCKET_CLOSE_STATUS_CODE_EXTENSION_EXPECTED,
    WEBSOCKET_CLOSE_STATUS_CODE_UNEXPECTED_CONDITION,
} wiced_websocket_close_status_code_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef wiced_result_t (*wiced_websocket_callback_t)( void* websocket );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_websocket_callback_t on_open;
    wiced_websocket_callback_t on_error;
    wiced_websocket_callback_t on_close;
    wiced_websocket_callback_t on_message;
} wiced_websocket_callbacks_t;

typedef struct
{
    wiced_bool_t                    final_frame;
    wiced_websocket_payload_type_t  payload_type;
    uint16_t                        payload_length;
    void*                           payload;
    uint16_t                        payload_buffer_size;
} wiced_websocket_frame_t;

typedef struct
{
    const char*   url;
    const char**  protocols;
} wiced_websocket_url_protocol_entry_t;

/**
 *  Mostly in-line with W3C APIs
 *  https://html.spec.whatwg.org/multipage/comms.html#websocket
 */
typedef struct wiced_websocket
{
    wiced_websocket_core_t                      core;                                      //!< Mainly an abstracted tcp_socket
    wiced_websocket_error_t                     error_type;                                //!< Error-type reported on websocket
    wiced_websocket_state_t                     state;                                     //!< Websocket State
    char                                        subprotocol[SUB_PROTOCOL_STRING_LENGTH];   //!< 'subprotocol in use' when websocket connection is established
    wiced_websocket_callbacks_t                 callbacks;                                 //!< callbacks for websocket
    wiced_websocket_url_protocol_entry_t*       url_protocol_ptr;                          //!< 'url'-'protocols' entry for this websocket
} wiced_websocket_t;

/**
 * Keeping this structure for backward compatibility.
 */
typedef struct
{
    char*       request_uri;
    char*       host;
    char*       origin;
    char*       sec_websocket_protocol;
} wiced_websocket_client_url_protocol_t;

typedef struct wiced_websocket_server wiced_websocket_server_t;

typedef struct
{
    uint8_t                                 count;
    wiced_websocket_url_protocol_entry_t*   entries;
} wiced_websocket_url_protocol_table_t;

typedef struct wiced_websocket_server_config
{
    uint8_t                                 max_connections;                //!< Maximum simultaneous websocket connections
    uint16_t                                heartbeat_duration;             //!< Duration for heart-beat; if 0, 'heart-beat' is disabled
    wiced_websocket_url_protocol_table_t*   url_protocol_table;             //!< Table of all url-protocol pairs supported by this server
} wiced_websocket_server_config_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/*****************************************************************************/
/**
 *
 *  @defgroup websocket          WebSocket
 *  @ingroup  ipcoms
 *
 * Communication functions for WebSocket protocol(both Client & Server)
 *
 * The WebSocket Protocol enables two-way communication between a client running untrusted code in
 * a controlled environment to a remote host that has opted-in to communication from that code.
 * Websocket is designed to supersede existing bidirectional communication technologies that use
 * HTTP as a transport layer to benefit from existing infrastructure. Refer to RFC #6455 for more
 * details on Websockets.
 *
 * Wiced Websockets APIs , can be broadly classified into the following:
 * - Websocket Server APIs to configure, start and stop Websocket server
 * - Websocket Client APIs to create a Websocket and connect/disconnect to the server
 * - Common APIs both for Server & Clients to send & receive frames on a Websocket handle.
 *
 *  @{
 */
/*****************************************************************************/

/** Perform opening handshake on port 80 with server and establish a connection. Called by Client only.
 *
 * @param[in] websocket            Websocket object
 * @param[in] url                  Server URL to be used for connection
 *
 * @return @ref wiced_result_t
 *
 * @note                           For additional error information, check the wiced_websocket_error_t field
 *                                 of the wiced_websocket_t structure
 */
wiced_result_t wiced_websocket_connect( wiced_websocket_t* websocket, const wiced_websocket_client_url_protocol_t* url );

/** Perform opening handshake on port 443 with server and establish a connection. Called by Client only.
 *
 * @param[in] websocket            Websocket object
 * @param[in] config               Server URL to be used for connection
 * @param[in] tls_identity         TLS identity object
 *
 *
 * @return @ref wiced_result_t
 *
 * @note                           For additional error information, check the wiced_websocket_error_t field
 *                                 of the  wiced_websocket_t structure
 */
wiced_result_t wiced_websocket_secure_connect( wiced_websocket_t* websocket, const wiced_websocket_client_url_protocol_t* url, wiced_tls_identity_t* tls_identity );

/** Send data to websocket end-point. Called by Server & Client both.
 *
 * @param[in] websocket            Websocket object
 * @param[in] tx_frame             Data frame to be send.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_websocket_send ( wiced_websocket_t* websocket, wiced_websocket_frame_t* tx_frame );

/** Receive data from Websocket end-point. This is a blocking call. Called by Server & Client both.
 *
 * @param[in] websocket                 Websocket object on data to be received.
 * @param[in] rx_frame                  Pointer to rx_frame where data from peer will be copied.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_websocket_receive ( wiced_websocket_t* websocket, wiced_websocket_frame_t* rx_frame );

/** Close and clean up websocket, and send close message to websocket server. Called by Server & Client both.
 *
 * @param[in] websocket             Websocket to close
 * @param[in] code                  Closing status code
 * @param[in] reason                Closing reason
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_websocket_close( wiced_websocket_t* websocket, const uint16_t code, const char* reason );

/** Register the on_open, on_close, on_message and on_error callbacks. Called by Client.
 *
 * @param[in] websocket                 websocket on which to register the callbacks
 * @param[in] on_open_callback          called on open websocket  connection
 * @param[in] on_close_callback         called on close websocket connection
 * @param[in] on_message_callback       called on websocket receive data
 * @param[in] on_error_callback         called on websocket error
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_websocket_register_callbacks ( wiced_websocket_t* websocket, wiced_websocket_callback_t on_open_callback, wiced_websocket_callback_t on_close_callback, wiced_websocket_callback_t on_message_callback, wiced_websocket_callback_t on_error ) ;

/** Un-Register the on_open, on_close, on_message and on_error callbacks for a given websocket
 *  Called by Client.
 *
 * @param[in] websocket                 websocket on which to unregister the callbacks
 *
 */
void wiced_websocket_unregister_callbacks ( wiced_websocket_t* websocket );

/** Initialise the websocket transmit frame. Called by Server & Client both.
 *
 * @param[in] websocket            tx frame we are initialising
 * @param[in] final_frame          Indicates if this is the final frame of the message
 * @param[in] payload_type         Type of payload (ping frame, binary frame, text frame etc).
 * @param[in] payload_length       Length of payload being sent
 *
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_websocket_initialise_tx_frame( wiced_websocket_frame_t* tx_frame, wiced_bool_t final_frame, wiced_websocket_payload_type_t payload_type, uint16_t payload_length, void* payload_buffer, uint16_t payload_buffer_size );


/** Initialise the websocket receive frame. Called by Server & Client both.
 *
 * @param[in] rx_frame             rx frame we are initialising
 * @param[in] payload_buffer       Pointer to buffer containing the payload
 * @param[in] payload_buffer_size  Length of buffer containing payload
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_websocket_initialise_rx_frame( wiced_websocket_frame_t* rx_frame, void* payload_buffer, uint16_t payload_buffer_size );

/** Initialise the websocket. Called by Client only.
 *
 * @param[in] websocket            websocket we are initialising
 *
 */
void wiced_websocket_initialise( wiced_websocket_t* websocket );

/** Un-initialise the websocket and free memory allocated in creating sending buffers. Called by Client only.
 *
 * @param[in] websocket            websocket we are un-initialising
 *
 */
void wiced_websocket_uninitialise( wiced_websocket_t* websocket );

/** Initialise and start a Websocket server. Called by Server.
 *
 * @param[in] server               Websocket server we are starting
 * @param[in] config               Configuration like max-connections, heart-beat, URL, list-of-protocols
 * @param[in] callbacks            Callbacks for events received on the websockets(under this webserver)
 * @param[in] tls_identity         TLS identity object;if NULL, TLS is disabled
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_websocket_server_start( wiced_websocket_server_t* server, wiced_websocket_server_config_t* config,
                                    wiced_websocket_callbacks_t* callbacks, wiced_tls_identity_t* tls_identity );

/** Stop and uninitialise Websocket server. Called by Server.
 *
 * @param[in] server               Websocket server we are stopping
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_websocket_server_stop( wiced_websocket_server_t* server );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
