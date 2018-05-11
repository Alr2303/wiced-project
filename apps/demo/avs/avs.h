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

#ifdef __cplusplus
extern "C" {
#endif

#include "wiced.h"
#include "wiced_log.h"
#include "wiced_audio.h"
#include "platform_audio.h"

#include "http2.h"
#include "bufmgr.h"
#include "audio_client.h"
#include "button_manager.h"

#include "avs_dct.h"
#include "avs_config.h"

/*
 * Turn on debug capture to save the audio response in a capture buffer.
 * Can be used with the debugger to dump the received data to a PC.
 */

#define AVS_DEBUG_CAPTURE           0

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AVS_TAG_VALID                       (0x0BAFF1ED)
#define AVS_TAG_INVALID                     (0xDEADBEEF)

#define AVS_HTTP_BUF_SIZE                   (8 * 1024)
#define AVS_TMP_BUF_SIZE                    (4 * 1024)

#define AVS_CAPTURE_BUF_SIZE                (100 * 1024)

#define AVS_HOSTNAME_LEN                    (40)
#define AVS_PATH_LEN                        (256)

#define AVS_RX_THREAD_PRIORITY              (4)
#define AVS_RX_THREAD_STACK_SIZE            (2 * 4096)

#define AVS_SAMPLES_PER_PERIOD              (320)
#define AVS_PCM_BUF_SIZE                    (2 * AVS_SAMPLES_PER_PERIOD)
#define AVS_NUM_PCM_BUFS                    (32 * 10)       /* High network latency environments can't send the TCP traffic */
                                                            /* as fast as we capture the audio data from the mic. For now   */
                                                            /* just throw a lot of buffers at the problem.                  */

#define AVS_BUFMGR_BUF_SIZE                 (1500)
#define AVS_BUFMSG_NUM_BUFS                 (50)

#define AVS_MSG_QUEUE_SIZE                  (AVS_BUFMSG_NUM_BUFS)

#define MSGQ_PUSH_TIMEOUT_MS                (100)
#define BUF_ALLOC_TIMEOUT_MS                (10)

#define AVS_TOKEN_ACCESS                    "access_token"
#define AVS_TOKEN_DIRECTIVE                 "directive"
#define AVS_TOKEN_EVENT                     "event"
#define AVS_TOKEN_HEADER                    "header"
#define AVS_TOKEN_PAYLOAD                   "payload"
#define AVS_TOKEN_MESSAGEID                 "messageId"
#define AVS_TOKEN_NAMESPACE                 "namespace"
#define AVS_TOKEN_NAME                      "name"
#define AVS_TOKEN_TOKEN                     "token"
#define AVS_TOKEN_FORMAT                    "format"
#define AVS_TOKEN_AUDIO_MPEG                "AUDIO_MPEG"
#define AVS_TOKEN_SPEECHRECOGNIZER          "SpeechRecognizer"
#define AVS_TOKEN_SPEECHSYNTHESIZER         "SpeechSynthesizer"
#define AVS_TOKEN_SPEECHSTARTED             "SpeechStarted"
#define AVS_TOKEN_SPEECHFINISHED            "SpeechFinished"
#define AVS_TOKEN_STOPCAPTURE               "StopCapture"
#define AVS_TOKEN_SPEAK                     "Speak"
#define AVS_TOKEN_SYSTEM                    "System"
#define AVS_TOKEN_SETENDPOINT               "SetEndpoint"
#define AVS_TOKEN_ENDPOINT                  "endpoint"

#define AVS_FORMAT_STRING_MID               "msgId-%08lx"
#define AVS_FORMAT_STRING_DID               "dlgId-%08lx"

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AVS_STATE_IDLE                  = 0,
    AVS_STATE_CONNECTED,
    AVS_STATE_DOWNCHANNEL,
    AVS_STATE_SYNCHRONIZE,
    AVS_STATE_READY,
    AVS_STATE_SPEECH_REQUEST,
    AVS_STATE_PROCESSING_RESPONSE,
    AVS_STATE_PLAYING_RESPONSE,
    AVS_STATE_AWAITING_RESPONSE_EOS,

    AVS_STATE_MAX,                       /* needs to remain last entry in this enumeration; not a valid state ! */
} AVS_STATE_T;

typedef enum
{
    AVS_MSG_TYPE_HTTP_RESPONSE,
    AVS_MSG_TYPE_AVS_DIRECTIVE,
    AVS_MSG_TYPE_SPEECH_SYNTHESIZER,

    AVS_MSG_TYPE_MAX
} AVS_MSG_TYPE_T;

typedef enum
{
    AVS_EVENT_SHUTDOWN              = (1 <<  0),
    AVS_EVENT_AVS                   = (1 <<  1),
    AVS_EVENT_SPEECH_REQUEST        = (1 <<  2),
    AVS_EVENT_TIMER                 = (1 <<  3),
    AVS_EVENT_DISCONNECT            = (1 <<  4),
    AVS_EVENT_CAPTURE               = (1 <<  5),
    AVS_EVENT_PCM_AUDIO_DATA        = (1 <<  6),
    AVS_EVENT_MSG_QUEUE             = (1 <<  7),

    AVS_EVENT_VOLUME_UP             = (1 << 28),
    AVS_EVENT_VOLUME_DOWN           = (1 << 29),
    AVS_EVENT_RELOAD_DCT_WIFI       = (1 << 30),
    AVS_EVENT_RELOAD_DCT_NETWORK    = (1 << 31),
} AVS_EVENTS_T;

#define AVS_ALL_EVENTS        (-1)

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    int inuse;
    int buflen;
    int bufused;
    uint8_t buf[AVS_PCM_BUF_SIZE];
} pcm_buf_t;


typedef struct
{
    AVS_MSG_TYPE_T type;
    uint32_t arg1;
    uint32_t arg2;
} avs_msg_t;

typedef struct avs_s
{
    uint32_t                        tag;
    AVS_STATE_T                     state;

    wiced_event_flags_t             events;
    wiced_timer_t                   timer;
    wiced_queue_t                   msgq;

    avs_dct_collection_t            dct_tables;

    char*                           test_uri;
    char                            hostname[AVS_HOSTNAME_LEN];
    char                            path[AVS_PATH_LEN];
    int                             port;
    wiced_bool_t                    tls;

    char                            http_buf[AVS_HTTP_BUF_SIZE];
    char                            encode_buf[AVS_TMP_BUF_SIZE];
    char                            tmp_buf[AVS_TMP_BUF_SIZE];
    int                             http_idx;

    char*                           avs_access_token;

    uint32_t                        message_id;
    uint32_t                        dialog_id;

    wiced_tcp_socket_t              socket;
    wiced_tcp_socket_t*             socket_ptr;

    /*
     * HTTP2 stuff.
     */

    http_connection_t               http2_connect;
    http_security_info_t            http2_security;
    http_request_id_t               http2_stream_id;
    http_request_id_t               http2_downchannel_stream_id;
    http_request_id_t               http2_capture_stream_id;
    http_request_id_t               http2_binary_data_stream;
    uint8_t                         http2_scratch[HTTP2_FRAME_SCRATCH_BUFF_SIZE];

    char*                           msg_boundary_downchannel;
    char*                           msg_boundary_speech;

    char*                           token_speak;

    wiced_bool_t                    parse_speech_response;
    wiced_bool_t                    discard_capture_stream;

    /*
     * Audio RX support.
     */

    wiced_thread_t                  rx_thread;
    wiced_thread_t*                 rx_thread_ptr;
    wiced_event_flags_t             rx_events;

    wiced_audio_session_ref         rx_session;
    wiced_bool_t                    rx_run;
    wiced_bool_t                    rx_started;
    wiced_bool_t                    rx_configured;

    pcm_buf_t                       pcm_bufs[AVS_NUM_PCM_BUFS];
    int                             pcm_write_idx;
    int                             pcm_read_idx;
    wiced_bool_t                    pcm_process_bufs;

    uint8_t                         rx_thread_stack_buffer[AVS_RX_THREAD_STACK_SIZE];

    /*
     * Buffer pool.
     */

    objhandle_t                     buf_pool;
    objhandle_t                     current_handle;

    /*
     * Capture buffer - This is for debugging purposes.
     */

#if AVS_DEBUG_CAPTURE
    uint8_t                         capture[AVS_CAPTURE_BUF_SIZE];
    uint32_t                        capture_idx;
#endif

    /*
     * Audio playback
     */

    audio_client_ref                audio_client_handle;
    uint32_t                        audio_client_volume;

    /*
     * Keypad / buttons
     */

    button_manager_t                button_manager;
    wiced_worker_thread_t           button_worker_thread;

} avs_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/*
 * That's only an extern variable because of the button handling manager
 * not allowing API users to provide their own context variable
 */
extern avs_t* g_avs;

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t avs_audio_rx_thread_start(avs_t* avs);
wiced_result_t avs_audio_rx_thread_stop(avs_t* avs);
wiced_result_t avs_audio_rx_capture_enable(avs_t* avs, wiced_bool_t enable);

wiced_result_t avs_audio_playback_init(avs_t* avs);
wiced_result_t avs_audio_playback_deinit(avs_t* avs);
wiced_result_t avs_audio_playback_volume_up(avs_t* avs);
wiced_result_t avs_audio_playback_volume_down(avs_t* avs);

wiced_result_t avs_keypad_init(avs_t* avs);
wiced_result_t avs_keypad_deinit(avs_t* avs);

#ifdef __cplusplus
} /* extern "C" */
#endif
