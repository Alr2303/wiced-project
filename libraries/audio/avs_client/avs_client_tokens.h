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
 * AVS Client Token Definitions
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/*
 * Object name strings.
 */

#define AVS_TOKEN_CONTEXT                       "context"
#define AVS_TOKEN_DIRECTIVE                     "directive"
#define AVS_TOKEN_EVENT                         "event"
#define AVS_TOKEN_HEADER                        "header"
#define AVS_TOKEN_PAYLOAD                       "payload"
#define AVS_TOKEN_AUDIOITEM                     "audioItem"
#define AVS_TOKEN_PROGRESSREPORT                "progressReport"
#define AVS_TOKEN_STREAM                        "stream"
#define AVS_TOKEN_CURRENTPLAYBACKSTATE          "currentPlaybackState"
#define AVS_TOKEN_METADATA                      "metadata"
#define AVS_TOKEN_SETTINGSARRAY                 "settings"
#define AVS_TOKEN_ERROR                         "error"
#define AVS_TOKEN_ALLALERTS                     "allAlerts"
#define AVS_TOKEN_ACTIVEALERTS                  "activeAlerts"

/*
 * Namespace strings.
 */

#define AVS_TOKEN_SPEECHRECOGNIZER              "SpeechRecognizer"
#define AVS_TOKEN_SPEECHSYNTHESIZER             "SpeechSynthesizer"
#define AVS_TOKEN_ALERTS                        "Alerts"
#define AVS_TOKEN_AUDIOPLAYER                   "AudioPlayer"
#define AVS_TOKEN_PLAYBACKCONTROLLER            "PlaybackController"
#define AVS_TOKEN_SPEAKER                       "Speaker"
#define AVS_TOKEN_SETTINGS                      "Settings"
#define AVS_TOKEN_SYSTEM                        "System"

/*
 * Directive name strings.
 */

#define AVS_TOKEN_STOPCAPTURE                   "StopCapture"
#define AVS_TOKEN_EXPECTSPEECH                  "ExpectSpeech"
#define AVS_TOKEN_SPEAK                         "Speak"
#define AVS_TOKEN_SETALERT                      "SetAlert"
#define AVS_TOKEN_DELETEALERT                   "DeleteAlert"
#define AVS_TOKEN_PLAY                          "Play"
#define AVS_TOKEN_STOP                          "Stop"
#define AVS_TOKEN_CLEARQUEUE                    "ClearQueue"
#define AVS_TOKEN_SETVOLUME                     "SetVolume"
#define AVS_TOKEN_ADJUSTVOLUME                  "AdjustVolume"
#define AVS_TOKEN_SETMUTE                       "SetMute"
#define AVS_TOKEN_RESETUSERINACTIVITY           "ResetUserInactivity"
#define AVS_TOKEN_SETENDPOINT                   "SetEndpoint"

/*
 * Event name strings.
 */

#define AVS_TOKEN_ROCOGNIZE                     "Recognize"
#define AVS_TOKEN_EXPECTSPEECHTIMEDOUT          "ExpectSpeechTimedOut"
#define AVS_TOKEN_SPEECHSTARTED                 "SpeechStarted"
#define AVS_TOKEN_SPEECHFINISHED                "SpeechFinished"
#define AVS_TOKEN_SETALERTSUCCEEDED             "SetAlertSucceeded"
#define AVS_TOKEN_SETALERTFAILED                "SetAlertFailed"
#define AVS_TOKEN_DELETEALERTSUCCEEDED          "DeleteAlertSucceeded"
#define AVS_TOKEN_DELETEALERTFAILED             "DeleteAlertFailed"
#define AVS_TOKEN_ALERTSTARTED                  "AlertStarted"
#define AVS_TOKEN_ALERTSTOPPED                  "AlertStopped"
#define AVS_TOKEN_ALERTENTEREDFOREGROUND        "AlertEnteredForeground"
#define AVS_TOKEN_ALERTENTEREDBACKGROUND        "AlertEnteredBackground"
#define AVS_TOKEN_PLAYBACKSTARTED               "PlaybackStarted"
#define AVS_TOKEN_PLAYBACKNEARLYFINISHED        "PlaybackNearlyFinished"
#define AVS_TOKEN_PROGRESSREPORTDELAYELAPSED    "ProgressReportDelayElapsed"
#define AVS_TOKEN_PROGRESSREPORTINTERVALELAPSED "ProgressReportIntervalElapsed"
#define AVS_TOKEN_PLAYBACKSTUTTERSTARTED        "PlaybackStutterStarted"
#define AVS_TOKEN_PLAYBACKSTUTTERFINISHED       "PlaybackStutterFinished"
#define AVS_TOKEN_PLAYBACKFINISHED              "PlaybackFinished"
#define AVS_TOKEN_PLAYBACKFAILED                "PlaybackFailed"
#define AVS_TOKEN_PLAYBACKSTOPPED               "PlaybackStopped"
#define AVS_TOKEN_PLAYBACKPAUSED                "PlaybackPaused"
#define AVS_TOKEN_PLAYBACKRESUMED               "PlaybackResumed"
#define AVS_TOKEN_PLAYBACKQUEUECLEARED          "PlaybackQueueCleared"
#define AVS_TOKEN_STREAMMETADATAEXTRACTED       "StreamMetadataExtracted"
#define AVS_TOKEN_PLAYCOMMANDISSUED             "PlayCommandIssued"
#define AVS_TOKEN_PAUSECOMMANDISSUED            "PauseCommandIssued"
#define AVS_TOKEN_NEXTCOMMANDISSUED             "NextCommandIssued"
#define AVS_TOKEN_PREVIOUSCOMMANDISSUED         "PreviousCommandIssued"
#define AVS_TOKEN_VOLUMECHANGED                 "VolumeChanged"
#define AVS_TOKEN_MUTECHANGED                   "MuteChanged"
#define AVS_TOKEN_SETTINGSUPDATED               "SettingsUpdated"
#define AVS_TOKEN_SYNCHRONIZESTATE              "SynchronizeState"
#define AVS_TOKEN_USERINACTIVITYREPORT          "UserInactivityReport"
#define AVS_TOKEN_EXCEPTIONENCOUNTERED          "ExceptionEncountered"

/*
 * Context name strings.
 */

#define AVS_TOKEN_PLAYBACKSTATE                 "PlaybackState"
#define AVS_TOKEN_ALERTSTATE                    "AlertState"
#define AVS_TOKEN_VOLUMESTATE                   "VolumeState"
#define AVS_TOKEN_SPEECHSTATE                   "SpeechState"

/*
 * Header parameter strings.
 */

#define AVS_TOKEN_NAMESPACE                     "namespace"
#define AVS_TOKEN_NAME                          "name"
#define AVS_TOKEN_MESSAGEID                     "messageId"
#define AVS_TOKEN_DIALOGID                      "dialogRequestId"

/*
 * Payload parameter strings.
 */

#define AVS_TOKEN_PROFILE                       "profile"
#define AVS_TOKEN_FORMAT                        "format"
#define AVS_TOKEN_TIMEOUT_IN_MS                 "timeoutInMilliseconds"
#define AVS_TOKEN_URL                           "url"
#define AVS_TOKEN_TOKEN                         "token"
#define AVS_TOKEN_TYPE                          "type"
#define AVS_TOKEN_SCHEDULEDTIME                 "scheduledTime"
#define AVS_TOKEN_PLAYBEHAVIOR                  "playBehavior"
#define AVS_TOKEN_AUDIOITEMID                   "audioItemId"
#define AVS_TOKEN_STREAMFORMAT                  "streamFormat"
#define AVS_TOKEN_OFFSET_IN_MS                  "offsetInMilliseconds"
#define AVS_TOKEN_PROGRESSREPORT_DIM            "progressReportDelayInMilliseconds"
#define AVS_TOKEN_PROGRESSREPORT_IIM            "progressReportIntervalInMilliseconds"
#define AVS_TOKEN_EXPECTEDPREVIOUSTOKEN         "expectedPreviousToken"
#define AVS_TOKEN_STUTTERDURATION_IN_MS         "stutterDurationInMilliseconds"
#define AVS_TOKEN_PLAYERACTIVITY                "playerActivity"
#define AVS_TOKEN_MESSAGE                       "message"
#define AVS_TOKEN_CLEARBEHAVIOR                 "clearBehavior"
#define AVS_TOKEN_VOLUME                        "volume"
#define AVS_TOKEN_MUTED                         "muted"
#define AVS_TOKEN_MUTE                          "mute"
#define AVS_TOKEN_KEY                           "key"
#define AVS_TOKEN_VALUE                         "value"
#define AVS_TOKEN_INACTIVETIME                  "inactiveTimeInSeconds"
#define AVS_TOKEN_ENDPOINT                      "endpoint"
#define AVS_TOKEN_UNPARSEDDIRECTIVE             "unparsedDirective"
#define AVS_TOKEN_WAKEWORD                      "wakeword"
#define AVS_TOKEN_CODE                          "code"
#define AVS_TOKEN_DESCRIPTION                   "description"

/*
 * Miscellaneous and parameter value strings.
 */

#define AVS_FORMAT_STRING_MID                   "msgId-%08lx"
#define AVS_FORMAT_STRING_DID                   "dlgId-%08lx"

#define AVS_TOKEN_ACCESS                        "access_token"
#define AVS_TOKEN_AUDIO_MPEG                    "AUDIO_MPEG"
#define AVS_TOKEN_TIMER                         "TIMER"
#define AVS_TOKEN_ALARM                         "ALARM"
#define AVS_TOKEN_REPLACEALL                    "REPLACE_ALL"
#define AVS_TOKEN_ENQUEUE                       "ENQUEUE"
#define AVS_TOKEN_REPLACEENQUEUED               "REPLACE_ENQUEUED"
#define AVS_TOKEN_CLEAREALL                     "CLEAR_ALL"
#define AVS_TOKEN_CLEARENQUEUED                 "CLEAR_ENQUEUED"
#define AVS_TOKEN_EMBEDDED_AUDIO                "cid:"
#define AVS_TOKEN_LOCALE                        "locale"
#define AVS_TOKEN_EN_US                         "en-US"
#define AVS_TOKEN_EN_GB                         "en-GB"
#define AVS_TOKEN_DE_DE                         "de-DE"
#define AVS_TOKEN_UNEXPECTED_INFORMATION        "UNEXPECTED_INFORMATION_RECEIVED"
#define AVS_TOKEN_UNSUPPORTED_OPERATION         "UNSUPPORTED_OPERATION"
#define AVS_TOKEN_INTERNAL_ERROR                "INTERNAL_ERROR"

#define AVS_AUDIO_PROFILE_CLOSE                 "CLOSE_TALK"
#define AVS_AUDIO_PROFILE_NEAR                  "NEAR_FIELD"
#define AVS_AUDIO_PROFILE_FAR                   "FAR_FIELD"
#define AVS_AUDIO_FORMAT                        "AUDIO_L16_RATE_16000_CHANNELS_1"

#define AVS_JSON_HEADERS        "Content-Disposition: form-data; name=\"metadata\"\r\nContent-Type: application/json; charset=UTF-08\r\n"
#define AVS_AUDIO_HEADERS       "Content-Disposition: form-data; name=\"audio\"\r\nContent-Type: application/octet-stream\r\n"


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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
