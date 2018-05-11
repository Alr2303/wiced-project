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

#include "bufmgr.h"
#include "audio_client.h"
#include "avs_client.h"
#include "button_manager.h"

#include "avs_app_dct.h"
#include "avs_app_config.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AVS_APP_TAG_VALID                   (0x0C0FFEE0)
#define AVS_APP_TAG_INVALID                 (0xDEADBEEF)

#define AVS_APP_RX_THREAD_PRIORITY          (4)
#define AVS_APP_RX_THREAD_STACK_SIZE        (2 * 4096)

#define AVS_APP_SAMPLES_PER_PERIOD          (320)
#define AVS_APP_PCM_BUF_SIZE                (2 * AVS_APP_SAMPLES_PER_PERIOD)
#define AVS_APP_NUM_PCM_BUFS                (32 * 10)       /* High network latency environments can't send the TCP traffic */
                                                            /* as fast as we capture the audio data from the mic. For now   */
                                                            /* just throw a lot of buffers at the problem.                  */

#define AVS_APP_AUDIO_BUF_SIZE              (1500)

//#define AVS_BUFMGR_BUF_SIZE                 (1500)
//#define AVS_BUFMSG_NUM_BUFS                 (50)

#define AVS_APP_MSG_QUEUE_SIZE              (10)

#define MSGQ_PUSH_TIMEOUT_MS                (100)
#define BUF_ALLOC_TIMEOUT_MS                (10)

#define AVS_APP_VOLUME_STEP                 (10)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AVS_APP_MSG_SPEAK_DIRECTIVE,
    AVS_APP_MSG_PLAY_DIRECTIVE,
    AVS_APP_MSG_PLAYLIST_LOADED,
    AVS_APP_MSG_EXPECT_SPEECH,
    AVS_APP_MSG_SET_ALERT,
    AVS_APP_MSG_DELETE_ALERT,
    AVS_APP_MSG_SET_VOLUME,
    AVS_APP_MSG_ADJUST_VOLUME,
    AVS_APP_MSG_SET_LOCALE,
    AVS_APP_MSG_SET_MUTE,
    AVS_APP_MSG_CLEAR_QUEUE,

    AVS_APP_MSG_MAX
} AVS_APP_MSG_TYPE_T;

typedef enum
{
    AVS_APP_EVENT_SHUTDOWN                  = (1 <<  0),
    AVS_APP_EVENT_MSG_QUEUE                 = (1 <<  1),
    AVS_APP_EVENT_CONNECT                   = (1 <<  2),
    AVS_APP_EVENT_DISCONNECT                = (1 <<  3),
    AVS_APP_EVENT_CAPTURE                   = (1 <<  4),
    AVS_APP_EVENT_VOICE_PLAYBACK            = (1 <<  5),
    AVS_APP_EVENT_PLAYBACK_COMPLETE         = (1 <<  6),
    AVS_APP_EVENT_PLAYBACK_ERROR            = (1 <<  7),
    AVS_APP_EVENT_PLAYBACK_AVAILABLE        = (1 <<  8),
    AVS_APP_EVENT_PLAYLIST_ENTRY_AVAILABLE  = (1 <<  9),
    AVS_APP_EVENT_STOP                      = (1 << 10),
    AVS_APP_EVENT_TIMER                     = (1 << 11),
    AVS_APP_EVENT_REPORT                    = (1 << 12),
    AVS_APP_EVENT_PLAY                      = (1 << 13),
    AVS_APP_EVENT_PAUSE                     = (1 << 14),
    AVS_APP_EVENT_NEXT                      = (1 << 15),
    AVS_APP_EVENT_PREVIOUS                  = (1 << 16),

    AVS_APP_EVENT_VOLUME_UP                 = (1 << 28),
    AVS_APP_EVENT_VOLUME_DOWN               = (1 << 29),
    AVS_APP_EVENT_RELOAD_DCT_WIFI           = (1 << 30),
    AVS_APP_EVENT_RELOAD_DCT_NETWORK        = (1 << 31),
} AVS_APP_EVENTS_T;

#define AVS_APP_ALL_EVENTS        (-1)

typedef enum
{
    AVS_APP_AUDIO_NODE_TYPE_SPEAK,
    AVS_APP_AUDIO_NODE_TYPE_PLAY,
} AVS_APP_AUDIO_NODE_TYPE_T;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    int inuse;
    int buflen;
    int bufused;
    uint8_t buf[AVS_APP_PCM_BUF_SIZE];
} pcm_buf_t;

typedef struct
{
    AVS_APP_MSG_TYPE_T type;
    uint32_t arg1;
    uint32_t arg2;
} avs_app_msg_t;

typedef struct avs_app_audio_node_s
{
    /* Fields valid for all audio types */
    struct avs_app_audio_node_s*    next;
    AVS_APP_AUDIO_NODE_TYPE_T       type;
    wiced_bool_t                    embedded_audio;     /* Audio sent as an embedded attachment in the HTTP response    */
    char*                           token;

    /* Fields valid for Play audio types */
    AVS_PLAY_BEHAVIOR_T             behavior;
    char*                           url;
    uint32_t                        start_ms;
    uint32_t                        offset_ms;
    uint32_t                        report_delay_ms;
    uint32_t                        report_interval_ms;
    uint32_t                        next_report_ms;
    char*                           previous_token;
    char*                           playlist_data;      /* Allocated separately */
    AUDIO_CLIENT_PLAYLIST_T         playlist_type;
    int                             num_playlist_entries;
    int                             cur_playlist_entry;

    char                            data[0];            /* Must be last! */
} avs_app_audio_node_t;

typedef struct avs_app_alert_node_s
{
    struct avs_app_alert_node_s*    next;
    AVS_ALERT_TYPE_T                type;
    uint64_t                        utc_ms;
    char                            token[0];           /* Must be last! */
} avs_app_alert_node_t;

typedef struct avs_s
{
    uint32_t                        tag;

    wiced_event_flags_t             events;
    wiced_queue_t                   msgq;
    wiced_timer_t                   timer;

    wiced_bool_t                    network_logging_active;

    avs_app_dct_collection_t        dct_tables;

    /*
     * AVS Client.
     */

    avs_client_ref                  avs_client_handle;
    uint32_t                        expect_speech;

    avs_app_audio_node_t*           speak_list;
    avs_app_audio_node_t*           play_list;
    avs_app_audio_node_t*           current_playback;
    avs_app_audio_node_t*           spoken_list;

    AVS_PLAYBACK_STATE_T            playback_state;

    avs_app_alert_node_t*           alert_list;
    avs_app_alert_node_t*           active_alert;

    wiced_bool_t                    configure_audio_codec;
    wiced_bool_t                    wait_for_audio_complete;
    wiced_bool_t                    recording_initiated;
    wiced_bool_t                    recording_active;
    uint8_t                         audio_buf[AVS_APP_AUDIO_BUF_SIZE];

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

    pcm_buf_t                       pcm_bufs[AVS_APP_NUM_PCM_BUFS];
    int                             pcm_write_idx;
    int                             pcm_read_idx;

    uint8_t                         rx_thread_stack_buffer[AVS_APP_RX_THREAD_STACK_SIZE];

    /*
     * In-memory logging support.
     */

    char*                           logbuf;
    uint32_t                        logbuf_size;
    uint32_t                        logbuf_idx;

    /*
     * Audio playback
     */

    audio_client_ref                audio_client_handle;
    int32_t                         audio_client_volume;
    AUDIO_CLIENT_PLAYLIST_T         playlist_type;
    wiced_bool_t                    audio_muted;

    /*
     * Keypad / buttons
     */

    button_manager_t                button_manager;
    wiced_worker_thread_t           button_worker_thread;

} avs_app_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/*
 * That's only an extern variable because of the console
 * and button handling manager not allowing API users to
 * provide their own context variable
 */
extern avs_app_t* g_avs_app;

/******************************************************
 *               Function Declarations
 ******************************************************/

int avs_app_log_output_handler(WICED_LOG_LEVEL_T level, char *logmsg);
void avs_app_dump_memory_log(avs_app_t* app);

wiced_result_t avs_app_console_start(avs_app_t *app);
wiced_result_t avs_app_console_stop(avs_app_t *app);

wiced_result_t avs_app_keypad_init(avs_app_t* app);
wiced_result_t avs_app_keypad_deinit(avs_app_t* app);

wiced_result_t avs_app_audio_rx_thread_start(avs_app_t* app);
wiced_result_t avs_app_audio_rx_thread_stop(avs_app_t* app);
wiced_result_t avs_app_audio_rx_capture_enable(avs_app_t* app, wiced_bool_t enable);

wiced_result_t avs_app_audio_playback_init(avs_app_t* app);
wiced_result_t avs_app_audio_playback_deinit(avs_app_t* app);

wiced_result_t avs_app_parse_playlist(avs_app_audio_node_t* play);

#ifdef __cplusplus
} /* extern "C" */
#endif
