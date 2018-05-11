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
 * @file
 *
 * AVS Client internal definitions.
 */

#pragma once

#include "wiced_defaults.h"
#include "wiced_rtos.h"
#include "wiced_tcpip.h"

#include "http2.h"

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AVSC_TAG_VALID                      0x0FF1C1A1
#define AVSC_TAG_INVALID                    0xDEADBEEF

#define AVSC_THREAD_PRIORITY                WICED_DEFAULT_LIBRARY_PRIORITY
#define AVSC_THREAD_STACK_SIZE              (2 * 4096)

#define AVSC_HOSTNAME_LEN                   (40)
#define AVSC_PATH_LEN                       (128)

#define AVSC_HTTP_BUF_SIZE                  (8 * 1024)
#define AVSC_DIRECTIVE_BUF_SIZE             (4 * 1024)
#define AVSC_TMP_BUF_SIZE                   (4 * 1024)

#define AVSC_VOICE_BUF_SIZE                 (320 * 2)       /* 320 16-bit mono samples */

#define AVSC_MSG_QUEUE_SIZE                 (10)

#define MSGQ_PUSH_TIMEOUT_MS                (100)

#define AVSC_NOT_SCANNED                    (0xFFFFFFFF)

#define HTTP_STATUS_OK                      (200)
#define HTTP_STATUS_NO_CONTENT              (204)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AVSC_STATE_IDLE                  = 0,
    AVSC_STATE_CONNECTED,
    AVSC_STATE_DOWNCHANNEL,
    AVSC_STATE_SYNCHRONIZE,
    AVSC_STATE_READY,
    AVSC_STATE_SPEECH_REQUEST,
    AVSC_STATE_SPEECH_RESPONSE,

    AVSC_STATE_MAX,                       /* needs to remain last entry in this enumeration; not a valid state ! */
} AVSC_STATE_T;

typedef enum
{
    AVSC_EVENT_SHUTDOWN                 = (1 <<  0),
    AVSC_EVENT_CONNECT                  = (1 <<  1),
    AVSC_EVENT_DISCONNECT               = (1 <<  2),
    AVSC_EVENT_DISCONNECTED             = (1 <<  3),
    AVSC_EVENT_MSG_QUEUE                = (1 <<  4),
    AVSC_EVENT_TIMER                    = (1 <<  5),
    AVSC_EVENT_SPEECH_REQUEST           = (1 <<  6),
    AVSC_EVENT_END_SPEECH_REQUEST       = (1 <<  7),
    AVSC_EVENT_GET_VOICE_DATA           = (1 <<  8),
    AVSC_EVENT_DIRECTIVE_DATA           = (1 <<  9),
    AVSC_EVENT_RESPONSE_DATA            = (1 << 10),
    AVSC_EVENT_SPEECH_TIMED_OUT         = (1 << 11),
    AVSC_EVENT_PLAYBACK_QUEUE_CLEARED   = (1 << 12),
} AVSC_EVENTS_T;

#define AVSC_ALL_EVENTS             (-1)

typedef enum
{
    AVSC_MSG_TYPE_HTTP_RESPONSE,
    AVSC_MSG_TYPE_SPEECH_EVENT,
    AVSC_MSG_TYPE_PLAYBACK_EVENT,
    AVSC_MSG_TYPE_ALERT_EVENT,
    AVSC_MSG_TYPE_INACTIVITY_REPORT,
    AVSC_MSG_TYPE_PLAYBACK_CONTROLLER,
    AVSC_MSG_TYPE_SPEAKER_EVENT,
    AVSC_MSG_TYPE_SETTINGS_UPDATED,
    AVSC_MSG_TYPE_EXCEPTION_ENCOUNTERED,

    AVSC_MSG_TYPE_MAX
} AVSC_MSG_TYPE_T;


/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    AVSC_MSG_TYPE_T type;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
} avsc_msg_t;


typedef struct
{
    uint32_t offset_ms;
    uint32_t duration_ms;
    AVS_PLAYBACK_STATE_T state;
    AVS_PLAYBACK_ERROR_T error;
    char* token;
    char* error_msg;
    char data[0];
} avsc_playback_event_t;


typedef struct
{
    char*               directive;
    AVS_ERROR_TYPE_T    error_type;
    char*               error_message;
    char                data[0];
} avsc_exception_event_t;


typedef struct avs_client_s
{
    uint32_t                    tag;
    AVSC_STATE_T                state;

    avs_client_params_t         params;
    char*                       access_token;

    wiced_event_flags_t         events;
    wiced_timer_t               timer;
    wiced_queue_t               msgq;
    wiced_mutex_t               buffer_mutex;

    wiced_tcp_socket_t          socket;
    wiced_tcp_socket_t*         socket_ptr;

    char                        hostname[AVSC_HOSTNAME_LEN];
    char                        path[AVSC_PATH_LEN];
    int                         port;
    wiced_bool_t                tls;

    char                        http_buf[AVSC_HTTP_BUF_SIZE];
    char                        tmp_buf[AVSC_TMP_BUF_SIZE];

    char                        directive_buf[AVSC_DIRECTIVE_BUF_SIZE]; /* Used for HTTP1.1 token processing and directive coalescing. */
    uint32_t                    directive_widx;
    wiced_bool_t                directive_first_boundary;

    char*                       response_buffer;
    uint32_t                    response_ridx;
    uint32_t                    response_widx;
    uint32_t                    response_audio_begin_idx;
    uint32_t                    response_audio_end_idx;
    uint32_t                    response_total_attachments;
    uint32_t                    response_cur_attachment;
    wiced_bool_t                response_binary;
    wiced_bool_t                response_complete;
    wiced_bool_t                response_discard;
    wiced_bool_t                response_first_boundary;
    wiced_bool_t                response_scan_for_next;

    /*
     * HTTP2 stuff.
     */

    http_connection_t           http2_connect;
    http_security_info_t        http2_security;
    http_request_id_t           http2_stream_id;
    http_request_id_t           http2_downchannel_stream_id;
    http_request_id_t           http2_voice_stream_id;                          /* Stream that are sending captured voice data to AVS       */
    http_request_id_t           http2_response_stream_id;                       /* Stream that we want to capture the response for parsing  */
    http_request_id_t           http2_binary_data_stream_id;                    /* Flag to disable debug printing of binary stream data     */
    uint8_t                     http2_scratch[HTTP2_FRAME_SCRATCH_BUFF_SIZE];

    /*
     * AVS stuff we want to keep track of.
     */

    char*                       msg_boundary_downchannel;
    char*                       msg_boundary_response;
    int                         boundary_len;

    uint32_t                    message_id;
    uint32_t                    dialog_id;
    uint32_t                    active_dialog_id;

    avs_client_alert_t*         alert_list;
    avs_client_playback_state_t playback_state;
    avs_client_playback_state_t speech_state;
    avs_client_volume_state_t   volume_state;

    uint8_t                     vbuf[AVSC_VOICE_BUF_SIZE];
    uint32_t                    vbuflen;
    wiced_bool_t                voice_active;

    wiced_time_t                active_time;
    wiced_time_t                expect_timeout;
    wiced_time_t                speech_start;
    uint32_t                    speech_time_ms;

    /*
     * Thread management.
     */

    wiced_thread_t              client_thread;
    wiced_thread_t*             client_thread_ptr;

    uint8_t                     client_thread_stack_buffer[AVSC_THREAD_STACK_SIZE];

} avs_client_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

extern char avs_audio_content_str[];
extern char avs_HTTP_body_separator[];

/******************************************************
 *               Function Declarations
 ******************************************************/

void avsc_reset_response_tracking(avs_client_t* client);

int avsc_urlencode(char* dst, const char* src);
wiced_result_t avsc_uri_split(avs_client_t* client, const char* uri);
wiced_result_t avsc_parse_uri_and_lookup_hostname(avs_client_t* client, const char* uri, wiced_ip_address_t* ip_address);
wiced_result_t avsc_scan_for_response_message_boundary(avs_client_t* client, uint32_t ridx, uint32_t widx);
wiced_result_t avsc_scan_for_audio_content(avs_client_t* client, uint32_t ridx, uint32_t widx);
char* avsc_find_JSON_start(char* buf);
cJSON* avsc_create_context_json(avs_client_t* client);
cJSON* avsc_create_synchronize_state_json(avs_client_t* client);
cJSON* avsc_create_recognize_json(avs_client_t* client);
char* avsc_create_speech_event_json_text(avs_client_t* client, char* name, char* token);
char* avsc_create_speech_timed_out_event_json_text(avs_client_t* client);
char* avsc_create_playback_event_json_text(avs_client_t* client, AVSC_IOCTL_T type, avsc_playback_event_t* pb_event);
char* avsc_create_alert_event_json_text(avs_client_t* client, AVSC_IOCTL_T type, char* token);
char* avsc_create_inactivity_report_event_json_text(avs_client_t* client, uint32_t seconds);
char* avsc_create_playback_controller_event_json_text(avs_client_t* client, AVSC_IOCTL_T type);
char* avsc_create_playback_queue_cleared_event_json_text(avs_client_t* client);
char* avsc_create_speaker_event_json_text(avs_client_t* client, AVSC_IOCTL_T type, uint32_t volume, wiced_bool_t muted);
char* avsc_create_settings_updated_event_json_text(avs_client_t* client, AVS_CLIENT_LOCALE_T locale);
char* avsc_create_exception_encountered_event_json_text(avs_client_t* client, avsc_exception_event_t* exception);

wiced_result_t avsc_get_access_token(avs_client_t* client);

wiced_result_t avsc_http2_initialize(avs_client_t* client);
wiced_result_t avsc_http2_shutdown(avs_client_t* client);

wiced_result_t avsc_connect_to_avs(avs_client_t* client);
wiced_result_t avsc_send_downchannel_message(avs_client_t* client);
wiced_result_t avsc_send_synchronize_state_message(avs_client_t* client);
wiced_result_t avsc_send_recognize_message(avs_client_t* client);
wiced_result_t avsc_finish_recognize_message(avs_client_t* client);
wiced_result_t avsc_send_speech_event(avs_client_t* client, char* token, wiced_bool_t speech_started);
wiced_result_t avsc_send_speech_timed_out_event(avs_client_t* client);
wiced_result_t avsc_send_playback_event(avs_client_t* client, AVSC_IOCTL_T type, avsc_playback_event_t* pb_event);
wiced_result_t avsc_send_playback_queue_cleared_event(avs_client_t* client);
wiced_result_t avsc_send_alert_event(avs_client_t* client, AVSC_IOCTL_T type, char* token);
wiced_result_t avsc_send_inactivity_report_event(avs_client_t* client, uint32_t seconds);
wiced_result_t avsc_send_playback_controller_event(avs_client_t* client, AVSC_IOCTL_T type);
wiced_result_t avsc_send_speaker_event(avs_client_t* client, AVSC_IOCTL_T type, uint32_t volume, wiced_bool_t muted);
wiced_result_t avsc_send_settings_updated_event(avs_client_t* client, AVS_CLIENT_LOCALE_T locale);
wiced_result_t avsc_send_exception_encountered_event(avs_client_t* client, avsc_exception_event_t* exception);

void avsc_process_directive(avs_client_t* client);
void avsc_process_speech_response(avs_client_t* client);

#ifdef __cplusplus
} /*extern "C" */
#endif
