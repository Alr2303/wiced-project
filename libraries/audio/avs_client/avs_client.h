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
 * AVS Client Public API.
 *
 * The AVS Client library provides Alexa Voice Service functionality for applications.
 *
 * Information and documentation of the Alexa APIs are available online at
 * http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/content/avs-api-overview
 */

#pragma once

#include "wiced_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AVS_API_VERSION     "v20160207"

#define AVS_BASE_URL_NA     "https://avs-alexa-na.amazon.com"
#define AVS_BASE_URL_EU     "https://avs-alexa-eu.amazon.com"

#define AVS_MAX_TOKEN_LEN   (200)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AVS_ASR_PROFILE_CLOSE = 0,
    AVS_ASR_PROFILE_NEAR,
    AVS_ASR_PROFILE_FAR
} AVS_ASR_PROFILE_T;

typedef enum
{
    AVS_PLAYBACK_STATE_IDLE = 0,        /* Nothing playing, no enqueued items.                      */
    AVS_PLAYBACK_STATE_PLAYING,         /* Stream playing.                                          */
    AVS_PLAYBACK_STATE_PAUSED,          /* Stream paused.                                           */
    AVS_PLAYBACK_STATE_BUFFER_UNDERRUN, /* Buffer underrun.                                         */
    AVS_PLAYBACK_STATE_FINISHED,        /* Stream finished playing.                                 */
    AVS_PLAYBACK_STATE_STOPPED,         /* Stream interrupted.                                      */

    AVS_PLAYBACK_STATE_MAX              /* Must be last.                                            */
} AVS_PLAYBACK_STATE_T;

typedef enum
{
    AVS_PLAY_BEHAVIOR_REPLACE_ALL = 0,
    AVS_PLAY_BEHAVIOR_ENQUEUE,
    AVS_PLAY_BEHAVIOR_REPLACE_ENQUEUED
} AVS_PLAY_BEHAVIOR_T;

typedef enum
{
    AVS_PLAYBACK_ERROR_UNKNOWN = 0,             /* An unknown error occurred.                                               */
    AVS_PLAYBACK_ERROR_INVALID_REQUEST,         /* The server recognized the request as being malformed.                    */
    AVS_PLAYBACK_ERROR_SERVICE_UNAVAILABLE,     /* The client was unable to reach the service.                              */
    AVS_PLAYBACK_ERROR_INTERNAL_SERVER_ERROR,   /* The server accepted the request but was unable to process as expected.   */
    AVS_PLAYBACK_ERROR_INTERNAL_DEVICE_ERROR,   /* There was an internal error on the client                                */

    AVS_PLAYBACK_ERROR_MAX
} AVS_PLAYBACK_ERROR_T;

typedef enum
{
    AVS_CLEAR_BEHAVIOR_CLEAR_ALL = 0,
    AVS_CLEAR_BEHAVIOR_CLEAR_ENQUEUED,
} AVS_CLEAR_BEHAVIOR_T;

typedef enum
{
    AVS_ALERT_TYPE_ALARM = 0,
    AVS_ALERT_TYPE_TIMER
} AVS_ALERT_TYPE_T;

typedef enum
{
    AVS_CLIENT_STATE_IDLE = 0,          /* AVS Client is initialized, not connected to the network  */
    AVS_CLIENT_STATE_CONNECTED,         /* AVS Client connected to AVS server                       */
} AVS_CLIENT_STATE_T;

typedef enum
{
    AVS_CLIENT_LOCALE_EN_US = 0,
    AVS_CLIENT_LOCALE_EN_GB,
    AVS_CLIENT_LOCALE_DE_DE,
} AVS_CLIENT_LOCALE_T;

typedef enum
{
    AVS_ERROR_UNEXPECTED_INFORMATION,
    AVS_ERROR_UNSUPPORTED,
    AVS_ERROR_INTERNAL
} AVS_ERROR_TYPE_T;

/**
 * Events sent from the AVS Client library to the application.
 */

typedef enum
{
    AVSC_APP_EVENT_CONNECTED = 0,           /* AVS Client has connected to AVS                                      */
    AVSC_APP_EVENT_DISCONNECTED,            /* AVS Client has disconnected from AVS                                 */
    AVSC_APP_EVENT_GET_ALERT_LIST,          /* AVS Client is requesting Alerts alert state/list                     */
    AVSC_APP_EVENT_GET_PLAYBACK_STATE,      /* AVS Client is requesting AudioPlayer playback state                  */
    AVSC_APP_EVENT_GET_VOLUME_STATE,        /* AVS Client is requesting Speaker volume state                        */
    AVSC_APP_EVENT_GET_SPEECH_STATE,        /* AVS Client is requesting SpeechSynthesize speech state               */
    AVSC_APP_EVENT_VOICE_START_RECORDING,   /* AVS Client is requesting voice recording to start                    */
    AVSC_APP_EVENT_VOICE_GET_DATA,          /* AVS Client is requesting voice data                                  */
    AVSC_APP_EVENT_VOICE_STOP_RECORDING,    /* AVS Client is requesting voice recording to stop                     */

    AVSC_APP_EVENT_EXPECT_SPEECH,           /* ExpectSpeech Directive - Arg is timeout in milliseconds (uint32_t)   */
    AVSC_APP_EVENT_SPEAK,                   /* Speak Directive - Arg is pointer to token string                     */
    AVSC_APP_EVENT_PLAY,                    /* Play Directive - Arg is pointer to avs_client_play_t                 */
    AVSC_APP_EVENT_STOP,                    /* Stop Directive - Arg is NULL                                         */
    AVSC_APP_EVENT_CLEAR_QUEUE,             /* ClearQueue Directive - Arg is AVS_CLEAR_BEHAVIOR_T                   */
    AVSC_APP_EVENT_SET_ALERT,               /* SetAlert Directive - Arg is pointer to avs_client_set_alert_t        */
    AVSC_APP_EVENT_DELETE_ALERT,            /* DeleteAlert Directive - Arg is pointer to token string               */
    AVSC_APP_EVENT_SET_VOLUME,              /* SetVolume Directive - Arg is volume level, 0-100 (uint32_t)          */
    AVSC_APP_EVENT_ADJUST_VOLUME,           /* AdjustVolume Directive - Arg is volume adjustment -100-100 (int32_t) */
    AVSC_APP_EVENT_SET_MUTE,                /* SetMute Directive - Arg is nonzero (mute) and zero (unmuted)         */
    AVSC_APP_EVENT_RESET_USER_INACTIVITY,   /* ResetUserInactivity Directive - Arg is NULL                          */
    AVSC_APP_EVENT_SET_END_POINT,           /* SetEndPoint Directive - Arg is pointer to endpoint URL string        */

} AVSC_APP_EVENT_T;

/**
 * Event Notes:
 *
 * AVSC_APP_EVENT_SET_END_POINT:
 * The library will automatically disconnect from the current endpoint and
 * connect to the new endpoint specified by this directive.
 *
 * State information required for Context container:
 * The Context container is included automatically with certain events as required by AVS.
 * The state information required for the Context container (speech, volume, alert, and playback)
 * is maintained automatically within the AVS client library by monitoring the events
 * sent by the application with the exception of the playback state. When playback is active,
 * the AVS client library will query the application as needed using the the AVSC_APP_EVENT_GET_PLAYBACK_STATE
 * event to ensure that the library has accurate information.
 */

/**
 * Commands sent from the application to the AVS Client library.
 */
typedef enum
{
    AVSC_IOCTL_STATE = 0,                   /* Arg is pointer to AVS_CLIENT_STATE_T                                             */
    AVSC_IOCTL_CONNECT,                     /* Arg is NULL - Connect to AVS                                                     */
    AVSC_IOCTL_DISCONNECT,                  /* Arg is NULL - Disconnect from AVS                                                */
    AVSC_IOCTL_EXPECT_SPEECH_TIMED_OUT,     /* Arg is NULL                                                                      */
    AVSC_IOCTL_SPEECH_TRIGGER,              /* Arg is zero/nonzero - Zero to start a speech recognize event and non-zero to finish (used when ASR profile is CLOSE) */
    AVSC_IOCTL_SPEECH_STARTED,              /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_SPEECH_FINISHED,             /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_PLAYBACK_STARTED,            /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PLAYBACK_NEARLY_FINISHED,    /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PROGRESS_DELAY_ELAPSED,      /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PROGRESS_INTERVAL_ELAPSED,   /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PLAYBACK_STUTTER_STARTED,    /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PLAYBACK_STUTTER_FINISHED,   /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PLAYBACK_FINISHED,           /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PLAYBACK_FAILED,             /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PLAYBACK_STOPPED,            /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PLAYBACK_PAUSED,             /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PLAYBACK_RESUMED,            /* Arg is pointer to avs_client_playback_event_t                                    */
    AVSC_IOCTL_PLAYBACK_QUEUE_CLEARED,      /* Arg is NULL                                                                      */
    AVSC_IOCTL_SET_ALERT_SUCCEEDED,         /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_SET_ALERT_FAILED,            /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_DELETE_ALERT_SUCCEEDED,      /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_DELETE_ALERT_FAILED,         /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_ALERT_STARTED,               /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_ALERT_STOPPED,               /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_ALERT_FOREGROUND,            /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_ALERT_BACKGROUND,            /* Arg is pointer to token string                                                   */
    AVSC_IOCTL_PLAY_COMMAND_ISSUED,         /* Arg is NULL                                                                      */
    AVSC_IOCTL_PAUSE_COMMAND_ISSUED,        /* Arg is NULL                                                                      */
    AVSC_IOCTL_NEXT_COMMAND_ISSUED,         /* Arg is NULL                                                                      */
    AVSC_IOCTL_PREVIOUS_COMMAND_ISSUED,     /* Arg is NULL                                                                      */
    AVSC_IOCTL_VOLUME_CHANGED,              /* Arg is pointer to avs_client_volume_state_t                                      */
    AVSC_IOCTL_MUTE_CHANGED,                /* Arg is pointer to avs_client_volume_state_t                                      */
    AVSC_IOCTL_SETTINGS_UPDATED,            /* Arg is AVS_CLIENT_LOCALE_T                                                       */
    AVSC_IOCTL_USER_INACTIVITY_REPORT,      /* Arg is time in seconds since last user interaction (uint32_t)                    */
    AVSC_IOCTL_EXCEPTION_ENCOUNTERED,       /* Arg is pointer to avs_client_exception_event_t                                   */

    AVSC_IOCTL_GET_AUDIO_DATA,              /* Arg is pointer to avs_client_audio_buf_t  */

    AVSC_IOCTL_MAX
} AVSC_IOCTL_T;


/**
 * IOCTL Notes:
 *
 * AVSC_IOCTL_EXPECT_SPEECH_TIMED_OUT:
 * The library will automatically generate an ExpectSpeechTimedOut event
 * if a speech trigger is not received within the timeout period after an
 * ExpectSpeech directive is received. The application may also manually
 * initiate the timeout event using this IOCTL.
 *
 */


/******************************************************
 *                 Type Definitions
 ******************************************************/

/**
 * Alert structure passed with AVSC_APP_EVENT_GET_ALERT_LIST event.
 * AVS Client library will pass address of an avs_alert_t pointer in arg.
 * Application should dynamically allocate a linked list of alerts
 * and set *arg = list of alerts. The AVS Client library will free the list
 * when done.
 */
typedef struct avs_client_alert_s
{
    AVS_ALERT_TYPE_T            type;
    char                        token[AVS_MAX_TOKEN_LEN];
    uint64_t                    utc_ms;
    wiced_bool_t                active;
    struct avs_client_alert_s*  next;
} avs_client_alert_t;

/**
 * Playback state structure passed with AVSC_APP_EVENT_GET_PLAYBACK_STATE and
 * AVSC_APP_EVENT_GET_SPEECH_STATE events.
 */
typedef struct
{
    AVS_PLAYBACK_STATE_T    state;
    char                    token[AVS_MAX_TOKEN_LEN];
    uint32_t                offset_ms;
} avs_client_playback_state_t;

/**
 * Volume state structure passed with AVSC_APP_EVENT_GET_VOLUME_STATE event
 * and with AVSC_IOCTL_VOLUME_CHANGED and AVSC_IOCTL_MUTE_CHANGED ioctls.
 */
typedef struct
{
    uint32_t            volume;
    wiced_bool_t        muted;
} avs_client_volume_state_t;

/**
 * Buffer structure passed with AVSC_APP_EVENT_VOICE_GET_DATA event.
 */

typedef struct
{
    uint8_t*        buf;        /* Pointer to the data buffer.                  */
    int             buflen;     /* Length of the data buffer in bytes.          */
    int             len;        /* Length of valid data in the buffer in bytes. */
    wiced_bool_t    eos;
} avs_client_audio_buf_t;

/**
 * Structure passed with AVSC_APP_EVENT_PLAY event.
 * Note that strings passed in structure are not persistent.
 */

typedef struct
{
    AVS_PLAY_BEHAVIOR_T behavior;           /* Playback behavior.                                                                           */
    char*               url;                /* Location of audio content. URLs starting with "cid:" are binary audio attachments.           */
    uint32_t            offset_ms;          /* Starting playback offset in milliseconds.                                                    */
    uint32_t            report_delay_ms;    /* Time in milliseconds to send ProgressReportDelayElapsed event to AVS - 0 means no report     */
    uint32_t            report_interval_ms; /* Time in milliseconds to send ProgressReportIntervalElapsed event to AVS - 0 means no report  */
    char*               token;              /* Token associated with stream.                                                                */
    char*               previous_token;     /* Token associated with previous stream.                                                       */
} avs_client_play_t;


/**
 * Structure passed with AVSC_APP_EVENT_SET_ALERT event.
 * Note that strings passed in structure are not persistent.
 */

typedef struct
{
    AVS_ALERT_TYPE_T    type;   /* Alert type                                       */
    char*               token;  /* Token associated with the alert                  */
    char*               time;   /* Schedule time for the alert in ISO 8601 format   */
} avs_client_set_alert_t;

/**
 * Structure passed with AVSC_IOCTL_PLAYBACK_xxx and AVSC_IOCTL_PROGRESS_xxx
 * events.
 */

typedef struct
{
    char*                   token;          /* Token provided by Play directive                         */
    uint32_t                offset_ms;      /* Track offset in milliseconds                             */
    uint32_t                duration_ms;    /* Duration in ms - only used for stutter finished event    */
    AVS_PLAYBACK_STATE_T    state;          /* Player state  - only used for playback failed event      */
    AVS_PLAYBACK_ERROR_T    error;          /* Error type    - only used for playback failed event      */
    char*                   error_msg;      /* Error message - only used for playback failed event      */
} avs_client_playback_event_t;

/**
 * Structure passed with AVSC_IOCTL_EXCEPTION_ENCOUNTERED events.
 */

typedef struct
{
    char*               directive;          /* The directive that was unable to be executed             */
    AVS_ERROR_TYPE_T    error_type;         /* Error type encountered                                   */
    char*               error_message;      /* Additional error details for logging and debugging       */
} avs_client_exception_event_t;


typedef struct avs_client_s *avs_client_ref;

/**
 * AVS Client event callback. This routine is invoked by the AVS Client library.
 *
 * Note that this routine may be called by different threads.
 *
 * @param handle    AVS Client instance.
 * @param userdata  Opaque userdata passed in initialization parameters.
 * @param event     AVS Client library -> application event
 * @param arg       Event argument (context deoends upon event)
 *
 * @return status of the operation.
 */
typedef wiced_result_t (*event_callback_t)(avs_client_ref handle, void* userdata, AVSC_APP_EVENT_T event, void* arg);


/**
 * Initialization parameters for AVS Client library initialization.
 */
typedef struct
{
    void*                           userdata;           /* Opaque userdata passed back in callbacks                 */

    wiced_interface_t               interface;          /* Network interface to use                                 */

    char*                           refresh_token;
    char*                           client_id;
    char*                           client_secret;

    char*                           base_url;

    AVS_ASR_PROFILE_T               ASR_profile;
    uint32_t                        response_buffer_size;   /* Minimum size should be at least 32K since AVS will send 16K data frames */

    event_callback_t                event_callback;

} avs_client_params_t;


/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Initialize the AVS client library.
 *
 * @param[in] params      : Pointer to the configuration parameters.
 *
 * @return Pointer to the avs client instance or NULL
 */
avs_client_ref avs_client_init(avs_client_params_t* params);

/** Deinitialize the AVS client library.
 *
 * @param[in] avs_client : Pointer to the avs client instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t avs_client_deinit(avs_client_ref avs_client);

/** Send an IOCTL to the AVS client session.
 *
 * @param[in] avs_client    : Pointer to the avs client instance.
 * @param[in] cmd           : IOCTL command to process.
 * @param[inout] arg        : Pointer to argument for IOTCL.
 *
 * @return    Status of the operation.
 */
wiced_result_t avs_client_ioctl(avs_client_ref avs_client, AVSC_IOCTL_T cmd, void* arg);

/** Convert an AVS ISO8601 time string to UTC time in milliseconds.
 *
 * NOTE: This routine will not handle general ISO 8601 formatted strings. It will
 * parse and convert the ISO 8601 string sent by AVS.
 *
 * @param[in] time_string   : Pointer to the ISO 8601 time string.
 * @param[inout] utc_ms     : Pointer to the variable for the UTC value.
 */
wiced_result_t avs_client_parse_iso8601_time_string(char* time_string, uint64_t* utc_ms);

#ifdef __cplusplus
} /*extern "C" */
#endif
