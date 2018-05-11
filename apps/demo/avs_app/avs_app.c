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

/** @file avs_app
 *
 * Reference AVS application to demonstrate how to interface with AVS Client service library.
 *
 * Information and documentation of the Alexa APIs are available online at
 * http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/content/avs-api-overview
 *
 * Before using this application for the first time, you should have followed Amazon's
 * Getting Started Guide (http://amzn.to/1Uui0QW) and created a developer account as well
 * as a new Alexa Voice Service device with a security profile.
 *
 * Once the security profile has been created you should have the following values:
 *
 * Device Type ID
 * Client ID
 * Client Secret
 *
 * With these values, use the apps/snip/avs_authorization program to generate a refresh token.
 * Fill in the defines in avs_app_dct.h with the strings for the refresh token, client ID, and client secret:
 *
 * #define AVS_APP_DEFAULT_REFRESH_TOKEN    ""
 * #define AVS_APP_DEFAULT_CLIENT_ID        ""
 * #define AVS_APP_DEFAULT_CLIENT_SECRET    ""
 *
 * Once the client device information has been entered, proceed with the following steps:
 *  1. Modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials in the
 *     wifi_config_dct.h header file to match your Wi-Fi access point
 *  2. Plug the WICED eval board into your computer
 *  3. Open a terminal application and connect to the WICED eval board
 *  4. Build and download the application (to the WICED board)
 *
 * After the download completes, the terminal displays WICED startup information and the
 * software will join the specified Wi-Fi network. On the console, issue the "connect" command
 * to establish the connection to AVS. Once the initial connection to AVS is complete, press
 * the Play/Pause button to start capturing audio with the digital microphone. The audio data
 * is sent to AVS for processing and the audio response will be played out through the default
 * output device.
 * Playback volume level can be adjusted by pressing Volume Down and Volume Up buttons.
 */

#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "wiced.h"
#include "wiced_log.h"
#include "command_console.h"
#include "wifi/command_console_wifi.h"
#include "dct/command_console_dct.h"
#include "resources.h"

#include "sntp.h"

#include "avs_app.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define PLAYBACK_START_HOUSEKEEPING(node)                   \
    do                                                      \
    {                                                       \
        wiced_time_get_time(&node->start_ms);               \
        node->next_report_ms = node->report_interval_ms;    \
    } while (0)

#define FREE_AUDIO_NODE(node)                               \
    do                                                      \
    {                                                       \
        if (node->playlist_data)                            \
        {                                                   \
            free(node->playlist_data);                      \
            node->playlist_data = NULL;                     \
        }                                                   \
        free(node);                                         \
    } while (0)

/******************************************************
 *                    Constants
 ******************************************************/

#define MS_PER_DAY                              (24 * 60 * 60 * 1000)

#define ALERT_DURATION_MS                       (5 * 1000)

#define HEARTBEAT_DURATION_MS                   (1 * 1000)

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

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_time_t avs_app_start_time;

avs_app_t* g_avs_app;

/******************************************************
 *               Function Definitions
 ******************************************************/

int avs_app_log_output_handler(WICED_LOG_LEVEL_T level, char *logmsg)
{
    uint32_t len;

    len = strlen(logmsg);

    if (g_avs_app && g_avs_app->logbuf && len + g_avs_app->logbuf_idx < g_avs_app->logbuf_size)
    {
        strcpy(&g_avs_app->logbuf[g_avs_app->logbuf_idx], logmsg);
        g_avs_app->logbuf_idx += len;
    }
    else
    {
        write(STDOUT_FILENO, logmsg, len);
    }

    return 0;
}


void avs_app_dump_memory_log(avs_app_t* app)
{
    if (app && app->logbuf && app->logbuf_idx > 0)
    {
        write(STDOUT_FILENO, app->logbuf, app->logbuf_idx);
        app->logbuf_idx = 0;
    }
}


static wiced_result_t avs_app_log_get_time(wiced_time_t* time)
{
    wiced_utc_time_ms_t utc_time_ms;
    wiced_time_t now;
    wiced_result_t result;

    /*
     * Get the current time.
     */

    result = wiced_time_get_utc_time_ms(&utc_time_ms);

    if (result == WICED_SUCCESS)
    {
        if (g_avs_app)
        {
            /*
             * Add in our timezone offset.
             */

            utc_time_ms = (uint64_t)((int64_t)utc_time_ms + (int64_t)g_avs_app->dct_tables.dct_app->utc_offset_ms);
        }
        *time = utc_time_ms % MS_PER_DAY;
    }
    else
    {
        result = wiced_time_get_time(&now);
        *time  = now - avs_app_start_time;
    }

    return result;
}


static void timer_callback(void *arg)
{
    avs_app_t* app = (avs_app_t *)arg;

    if (app && app->tag == AVS_APP_TAG_VALID)
    {
        wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_TIMER);
    }
}


static wiced_result_t update_timer_processing(avs_app_t* app)
{
    wiced_bool_t need_timer;

    /*
     * Do we need to have a timer running for playback reporting?
     */

    need_timer = WICED_FALSE;
    if (app->current_playback)
    {
        if (app->current_playback->report_delay_ms || app->current_playback->report_interval_ms)
        {
            need_timer = WICED_TRUE;
        }
    }

    /*
     * Do we need to have a timer running for alert processing?
     */

    if (app->alert_list != NULL)
    {
        need_timer = WICED_TRUE;
    }

    /*
     * Make sure that the timer is running or stopped as appropriate.
     */

    if (need_timer)
    {
        if (wiced_rtos_is_timer_running(&app->timer) != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Starting app heartbeat timer\n");
            wiced_rtos_start_timer(&app->timer);
        }
    }
    else
    {
        if (wiced_rtos_is_timer_running(&app->timer) == WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Stopping app heartbeat timer\n");
            wiced_rtos_stop_timer(&app->timer);
        }
    }

    return WICED_SUCCESS;
}


static void avs_app_process_timer(avs_app_t* app)
{
    avs_app_alert_node_t* alert;
    avs_client_playback_event_t playback_event;
    wiced_utc_time_ms_t utc_time_ms;
    wiced_bool_t recheck_timer;
    uint32_t cur_time;
    uint32_t diff_ms;

    recheck_timer = WICED_FALSE;

    /*
     * Any alerts that we need to check?
     */

    if (app->alert_list)
    {
        wiced_time_get_utc_time_ms(&utc_time_ms);

        /*
         * Check for an active alert. Note that the current implementation
         * only supports one active alert at a time.
         */

        if (app->active_alert)
        {
            diff_ms = (uint32_t)(utc_time_ms - app->active_alert->utc_ms);
            if (diff_ms >= ALERT_DURATION_MS)
            {
                /*
                 * Tell AVS that the alert has stopped and remove the alert from our list.
                 */

                avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_ALERT_STOPPED, app->active_alert->token);
                app->active_alert = NULL;

                alert           = app->alert_list;
                app->alert_list = app->alert_list->next;
                free(alert);

                recheck_timer = WICED_TRUE;
            }
        }

        if (app->alert_list && app->active_alert == NULL)
        {
            if (utc_time_ms >= app->alert_list->utc_ms)
            {
                /*
                 * Time for this alert to go off.
                 */

                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Alert started %lu ms ago\n", (uint32_t)(utc_time_ms - app->alert_list->utc_ms));

                avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_ALERT_STARTED, app->alert_list->token);
                app->active_alert = app->alert_list;
            }
        }
    }

    /*
     * Are we currently playing?
     */

    if (app->current_playback)
    {
        wiced_time_get_time(&cur_time);
        diff_ms = cur_time - app->current_playback->start_ms + app->current_playback->offset_ms;

        /*
         * We don't need to worry about whether it's a speech or audio player track
         * since speech tracks will have report values of 0.
         */

        if (app->current_playback->report_delay_ms)
        {
            if (diff_ms >= app->current_playback->report_delay_ms)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Delay playback report at %lu ms\n", diff_ms);

                playback_event.token     = app->current_playback->token;
                playback_event.offset_ms = diff_ms;
                avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PROGRESS_DELAY_ELAPSED, &playback_event);

                /*
                 * Set the report delay to 0 so we don't send this event again.
                 */

                app->current_playback->report_delay_ms = 0;
                recheck_timer = WICED_TRUE;
            }
        }

        if (app->current_playback->next_report_ms)
        {
            if (diff_ms >= app->current_playback->next_report_ms)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Interval playback report at %lu ms\n", diff_ms);

                playback_event.token     = app->current_playback->token;
                playback_event.offset_ms = diff_ms;
                avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PROGRESS_INTERVAL_ELAPSED, &playback_event);

                app->current_playback->next_report_ms += app->current_playback->report_interval_ms;
            }
        }
    }

    if (recheck_timer)
    {
        /*
         * Turn the timer off if it's no longer needed.
         */

        update_timer_processing(app);
    }
}


static avs_client_alert_t* generate_alert_list(avs_app_t* app)
{
    avs_app_alert_node_t* app_alert;
    avs_client_alert_t* alert_list;
    avs_client_alert_t* alert;
    avs_client_alert_t* ptr;

    alert_list = NULL;
    ptr        = NULL;
    for (app_alert = app->alert_list; app_alert != NULL; app_alert = app_alert->next)
    {
        /*
         * Allocate a new alert node.
         */

        alert = malloc(sizeof(avs_client_alert_t));
        if (alert == NULL)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Failed to allocate alert node\n");
            break;
        }

        /*
         * And copy the information over. Note that our current alert implementation
         * only allows us to have one active alert at a time.
         */

        alert->type   = app_alert->type;
        alert->utc_ms = app_alert->utc_ms;
        alert->next   = NULL;
        alert->active = app->active_alert == app_alert ? WICED_TRUE : WICED_FALSE;
        strlcpy(alert->token, app_alert->token, AVS_MAX_TOKEN_LEN);

        /*
         * And add it to the list.
         */

        if (ptr == NULL)
        {
            alert_list = alert;
        }
        else
        {
            ptr->next = alert;
        }
        ptr = alert;
    }

    return alert_list;
}


/*
 * Check to see if there is an embedded audio item next on our hit parade.
 */
static wiced_bool_t embedded_audio_item_pending_next(avs_app_t* app)
{
    wiced_bool_t pending;

    pending = WICED_FALSE;

    if (app->current_playback != NULL)
    {
        /*
         * There's a current playback item. It will be either the head of the speak_list
         * or the play_list so we need to account for that in our check.
         */

        if (app->current_playback == app->speak_list && (app->speak_list->next || (app->play_list && app->play_list->embedded_audio)))
        {
            pending = WICED_TRUE;
        }
        else if (app->current_playback == app->play_list && (app->speak_list || (app->play_list->next && app->play_list->next->embedded_audio)))
        {
            pending = WICED_TRUE;
        }
    }
    else
    {
        if (app->speak_list || (app->play_list && app->play_list->embedded_audio))
        {
            pending = WICED_TRUE;
        }
    }

    return pending;
}


/*
 * Note: this routine is only used to begin playing an embedded audio item before
 * the previous one has completed playing (after the last buffer of the previous
 * item has been push to audio_client).
 */
static wiced_bool_t start_next_embedded_audio(avs_app_t* app)
{
    avs_app_audio_node_t* ptr;

    if (app->current_playback == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "start next embedded called with no current playback item!\n");
        return WICED_FALSE;
    }

    if (app->expect_speech)
    {
        /*
         * If we have an expect speech event waiting then we need to take care of that.
         */

        return WICED_FALSE;
    }

    /*
     * We need to check to see if we have an embedded audio item to start.
     */

    if (!embedded_audio_item_pending_next(app))
    {
        return WICED_FALSE;
    }

    /*
     * Remove the current playback item from its list and move it to the spoken list.
     * We still need to notify AVS when it completes playing.
     */

    if (app->current_playback == app->speak_list)
    {
        app->speak_list = app->speak_list->next;
    }
    else
    {
        app->play_list = app->play_list->next;
    }

    if (app->spoken_list == NULL)
    {
        app->spoken_list = app->current_playback;
    }
    else
    {
        for (ptr = app->spoken_list; ptr->next != NULL; ptr = ptr->next)
        {
            /* Empty */
        }
        ptr->next = app->current_playback;
    }
    app->current_playback->next = NULL;
    app->current_playback       = NULL;

    /*
     * Set up the next item to start.
     */

    if (app->speak_list)
    {
        app->current_playback = app->speak_list;
    }
    else
    {
        app->current_playback = app->play_list;
    }

    /*
     * Note that we need to configure the codec and let 'er rip!
     */

    app->configure_audio_codec = WICED_TRUE;
    wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_VOICE_PLAYBACK);

    return WICED_TRUE;
}


static wiced_result_t process_get_voice_data(avs_app_t* app, avs_client_audio_buf_t* abuf)
{
    pcm_buf_t* pcmbuf;
    int len;

    if (app == NULL || abuf == NULL)
    {
        return WICED_ERROR;
    }

    if (!app->rx_run)
    {
        /*
         * Hmm, capture isn't running...something is not right.
         */

        return WICED_ERROR;
    }

    while (abuf->len < abuf->buflen)
    {
        /*
         * Grab the next buffer of data.
         */

        pcmbuf = &app->pcm_bufs[app->pcm_read_idx];
        if (!pcmbuf->inuse)
        {
            break;
        }

        len = pcmbuf->buflen - pcmbuf->bufused;
        if (len > abuf->buflen)
        {
            len = abuf->buflen;
        }
        memcpy(abuf->buf, &pcmbuf->buf[pcmbuf->bufused], len);

        pcmbuf->bufused += len;
        abuf->len       += len;

        if (pcmbuf->bufused >= pcmbuf->buflen)
        {
            pcmbuf->inuse   = 0;
            pcmbuf->buflen  = 0;
            pcmbuf->bufused = 0;
            app->pcm_read_idx = (app->pcm_read_idx + 1) % AVS_APP_NUM_PCM_BUFS;
        }
    }

    return WICED_SUCCESS;
}


static wiced_result_t avsc_event_callback(avs_client_ref handle, void* userdata, AVSC_APP_EVENT_T event, void* arg)
{
    avs_app_t* app = (avs_app_t*)userdata;
    avs_app_msg_t msg;
    avs_client_alert_t** alert;
    avs_client_playback_state_t* playback_state;
    avs_client_volume_state_t* volume_state;
    avs_client_play_t* client_play;
    avs_client_set_alert_t* set_alert;
    avs_app_alert_node_t* app_alert;
    avs_app_audio_node_t* anode;
    wiced_bool_t free_arg1_on_failure;
    wiced_bool_t push_msg;
    uint32_t cur_time;
    char* token;
    wiced_result_t result = WICED_SUCCESS;
    int len;

    if (event != AVSC_APP_EVENT_VOICE_GET_DATA)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "AVSC callback event: %d\n", event);
    }

    push_msg             = WICED_FALSE;
    free_arg1_on_failure = WICED_FALSE;
    switch (event)
    {
        case AVSC_APP_EVENT_CONNECTED:
            wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "Connection to AVS established\n");
            break;

        case AVSC_APP_EVENT_DISCONNECTED:
            wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "Connection to AVS disconnected\n");
            break;

        case AVSC_APP_EVENT_GET_ALERT_LIST:
            alert = (avs_client_alert_t**)arg;
            if (alert)
            {
                /*
                 * Get the current list of alerts.
                 */

                *alert = generate_alert_list(app);
            }
            break;

        case AVSC_APP_EVENT_GET_PLAYBACK_STATE:
            playback_state = (avs_client_playback_state_t*)arg;
            if (playback_state)
            {
                /*
                 * Report our playback state.
                 */
                playback_state->state = app->playback_state;

                if (app->current_playback &&
                    (app->playback_state == AVS_PLAYBACK_STATE_PLAYING || app->playback_state == AVS_PLAYBACK_STATE_PAUSED ||
                     app->playback_state == AVS_PLAYBACK_STATE_BUFFER_UNDERRUN))
                {
                    wiced_time_get_time(&cur_time);
                    playback_state->offset_ms = cur_time - app->current_playback->start_ms + app->current_playback->offset_ms;
                    strlcpy(playback_state->token, app->current_playback->token, AVS_MAX_TOKEN_LEN);
                }
                else
                {
                    playback_state->token[0]  = '\0';
                    playback_state->offset_ms = 0;
                }
            }
            break;

        case AVSC_APP_EVENT_GET_VOLUME_STATE:
            volume_state = (avs_client_volume_state_t*)arg;
            if (volume_state)
            {
                /*
                 * Return the current volume.
                 */
                volume_state->volume = (uint32_t)app->audio_client_volume;
                volume_state->muted  = app->audio_muted;
            }
            break;

        case AVSC_APP_EVENT_GET_SPEECH_STATE:
            playback_state = (avs_client_playback_state_t*)arg;
            if (playback_state)
            {
                /*
                 * AVS Client library only asks for the speech state in the beginning
                 * when we aren't processing any speech. After that it keeps track
                 * of the speech state internally.
                 */
                playback_state->state     = AVS_PLAYBACK_STATE_FINISHED;
                playback_state->token[0]  = '\0';
                playback_state->offset_ms = 0;
            }
            break;

        case AVSC_APP_EVENT_VOICE_START_RECORDING:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Crank up the mic!\n");
            result = avs_app_audio_rx_capture_enable(app, WICED_TRUE);
            if (result == WICED_SUCCESS)
            {
                app->recording_active = WICED_TRUE;
            }
            app->recording_initiated = WICED_FALSE;
            break;

        case AVSC_APP_EVENT_VOICE_GET_DATA:
            result = process_get_voice_data(app, (avs_client_audio_buf_t*)arg);
            break;

        case AVSC_APP_EVENT_VOICE_STOP_RECORDING:
            result = avs_app_audio_rx_capture_enable(app, WICED_FALSE);
            app->recording_active = WICED_FALSE;
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Stop talking!\n");
            if (app->play_list != NULL)
            {
                /*
                 * There might be pending playback waiting. Make sure the main loop
                 * takes a look at it.
                 */
                wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_PLAYBACK_AVAILABLE);
            }
            break;

        case AVSC_APP_EVENT_SPEAK:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Alexa has something to say\n");
            anode = calloc(1, sizeof(avs_app_audio_node_t) + strlen((char*)arg) + 1);
            if (anode == NULL)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to allocate speak node\n");
                result = WICED_ERROR;
                break;
            }
            strcpy(anode->data, (char*)arg);
            anode->token          = anode->data;
            anode->type           = AVS_APP_AUDIO_NODE_TYPE_SPEAK;
            anode->embedded_audio = WICED_TRUE;
            msg.type = AVS_APP_MSG_SPEAK_DIRECTIVE;
            msg.arg1 = (uint32_t)anode;
            push_msg = WICED_TRUE;
            free_arg1_on_failure = WICED_TRUE;
            break;

        case AVSC_APP_EVENT_PLAY:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Play directive reported to app\n");
            client_play = (avs_client_play_t*)arg;
            if (client_play == NULL || client_play->url == NULL || client_play->token == NULL)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Invalid play event\n");
                result = WICED_ERROR;
                break;
            }
            len = sizeof(avs_app_audio_node_t) + strlen(client_play->url) + 1 + strlen(client_play->token) + 1;
            if (client_play->previous_token != NULL)
            {
                len += strlen(client_play->previous_token) + 1;
            }
            anode = calloc(1, len);
            if (anode == NULL)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to allocate play node\n");
                result = WICED_ERROR;
                break;
            }

            /*
             * Copy over the information.
             */

            anode->type               = AVS_APP_AUDIO_NODE_TYPE_PLAY;
            anode->behavior           = client_play->behavior;
            anode->offset_ms          = client_play->offset_ms;
            anode->report_delay_ms    = client_play->report_delay_ms;
            anode->report_interval_ms = client_play->report_interval_ms;
            strcpy(anode->data, client_play->url);
            anode->url   = anode->data;
            anode->token = &anode->data[strlen(anode->url) + 1];
            strcpy(anode->token, client_play->token);
            if (client_play->previous_token != NULL)
            {
                anode->previous_token = &anode->token[strlen(anode->token) + 1];
                strcpy(anode->previous_token, client_play->previous_token);
            }

            /*
             * Is it a local (embedded audio) request?
             */

            if (anode->url[0] == 'c' && anode->url[1] == 'i' && anode->url[2] == 'd' && anode->url[3] == ':')
            {
                anode->embedded_audio = WICED_TRUE;
            }

            /*
             * And send it to the main loop.
             */

            msg.type = AVS_APP_MSG_PLAY_DIRECTIVE;
            msg.arg1 = (uint32_t)anode;
            push_msg = WICED_TRUE;
            free_arg1_on_failure = WICED_TRUE;
            break;

        case AVSC_APP_EVENT_STOP:
            wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_STOP);
            break;

        case AVSC_APP_EVENT_SET_ALERT:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "SetAlert directive reported to app\n");
            set_alert = (avs_client_set_alert_t*)arg;
            if (set_alert == NULL || set_alert->time == NULL || set_alert->token == NULL)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Invalid set alert event\n");
                result = WICED_ERROR;
                break;
            }
            app_alert = calloc(1, sizeof(avs_app_alert_node_t) + strlen(set_alert->token) + 1);
            if (app_alert == NULL)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Unable to allocate alert event\n");
                result = WICED_ERROR;
                break;
            }
            app_alert->type = set_alert->type;
            strcpy(app_alert->token, set_alert->token);
            if (avs_client_parse_iso8601_time_string(set_alert->time, &app_alert->utc_ms) != WICED_SUCCESS)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Unable to convert alert time\n");
                result = WICED_ERROR;
                free(app_alert);
                break;
            }
            msg.type = AVS_APP_MSG_SET_ALERT;
            msg.arg1 = (uint32_t)app_alert;
            push_msg = WICED_TRUE;
            free_arg1_on_failure = WICED_TRUE;
            break;

        case AVSC_APP_EVENT_DELETE_ALERT:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "DeleteAlert directive reported to app\n");
            if (arg == NULL)
            {
                result = WICED_ERROR;
                break;
            }
            if ((token = strdup((char*)arg)) == NULL)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error allocating delete alert token\n");
                result = WICED_ERROR;
                break;
            }

            msg.type = AVS_APP_MSG_DELETE_ALERT;
            msg.arg1 = (uint32_t)token;
            push_msg = WICED_TRUE;
            free_arg1_on_failure = WICED_TRUE;
            break;

        case AVSC_APP_EVENT_EXPECT_SPEECH:
            app->expect_speech = (uint32_t)arg;
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "ExpectSpeech directive reported to app - timeout %lu\n", (uint32_t)app->expect_speech);

            /*
             * We're going to push a message rather than an event flag so we don't have to worry about the main
             * loop getting the ExpectSpeech before any associated Speak directive.
             */

            msg.type = AVS_APP_MSG_EXPECT_SPEECH;
            push_msg = WICED_TRUE;
            break;

        case AVSC_APP_EVENT_SET_VOLUME:
            msg.type = AVS_APP_MSG_SET_VOLUME;
            msg.arg1 = (uint32_t)arg;
            push_msg = WICED_TRUE;
            break;

        case AVSC_APP_EVENT_ADJUST_VOLUME:
            msg.type = AVS_APP_MSG_ADJUST_VOLUME;
            msg.arg1 = (uint32_t)((int32_t)arg);
            push_msg = WICED_TRUE;
            break;

        case AVSC_APP_EVENT_SET_MUTE:
            msg.type = AVS_APP_MSG_SET_MUTE;
            msg.arg1 = (uint32_t)arg;
            push_msg = WICED_TRUE;
            break;

        case AVSC_APP_EVENT_RESET_USER_INACTIVITY:
            /*
             * We don't do anything with this.
             */
            break;

        case AVSC_APP_EVENT_CLEAR_QUEUE:
            msg.type = AVS_APP_MSG_CLEAR_QUEUE;
            msg.arg1 = (AVS_CLEAR_BEHAVIOR_T)arg;
            push_msg = WICED_TRUE;
            break;

        default:
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unknown callback event %d\n", event);
            break;
    }

    /*
     * Do we have a message to send to the main loop?
     */

    if (push_msg)
    {
        if (wiced_rtos_push_to_queue(&app->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
        {
            wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_MSG_QUEUE);
        }
        else
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error pushing msg %d\n", msg.type);
            if (free_arg1_on_failure)
            {
                free((void*)msg.arg1);
            }
        }
    }

    return result;
}


static void avs_app_set_locale(avs_app_t* app, AVS_CLIENT_LOCALE_T locale)
{
    /*
     * Send an event to AVS updating our locale.
     */

    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_SETTINGS_UPDATED, (void*)((uint32_t)locale));
}


static void avs_app_set_new_volume(avs_app_t* app, int32_t new_volume)
{
    avs_client_volume_state_t volume_state;

    /*
     * Max sure that the new volume is in range.
     */

    new_volume = MAX(new_volume, AVS_APP_VOLUME_MIN);
    new_volume = MIN(new_volume, AVS_APP_VOLUME_MAX);

    if (new_volume == app->audio_client_volume)
    {
        /*
         * New volume is the same as the old volume...nothing to do.
         */

        return;
    }

    app->audio_client_volume = new_volume;

    /*
     * Tell the audio client about the new volume.
     */

    audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_SET_VOLUME, (void *)((uint32_t)app->audio_client_volume));

    /*
     * And lastly tell AVS.
     */

    volume_state.volume = (uint32_t)app->audio_client_volume;
    volume_state.muted  = app->audio_muted;
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_VOLUME_CHANGED, &volume_state);
}


static void avs_app_set_new_mute(avs_app_t* app, uint32_t mute)
{
    avs_client_volume_state_t volume_state;

    if (mute)
    {
        app->audio_muted = WICED_TRUE;
    }
    else
    {
        app->audio_muted = WICED_FALSE;
    }

    /*
     * Now tell AVS.
     */

    volume_state.volume = (uint32_t)app->audio_client_volume;
    volume_state.muted  = app->audio_muted;
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_MUTE_CHANGED, &volume_state);
}


static void avs_app_process_expect_speech(avs_app_t* app)
{
    /*
     * If there's a speak directive currently being processed than we need
     * to wait for it to complete.
     */

    if (app->speak_list)
    {
        return;
    }

    if (app->current_playback != NULL)
    {
        /*
         * Ideally we'd put any content channel into the background. We can't handle recording
         * and playing at the same time. The real solution would be to pause playback here but
         * that's not supported yet. So just log the issue and move along.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "ExpectSpeech event while playing\n");
        return;
    }

    /*
     * Start capturing speech.
     */

    app->expect_speech       = 0;
    app->recording_initiated = WICED_TRUE;
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_SPEECH_TRIGGER, NULL);
}


static void avs_app_stop_playback(avs_app_t* app)
{
    avs_client_playback_event_t event;
    uint32_t cur_time;

    if (app->current_playback != NULL && app->current_playback->type == AVS_APP_AUDIO_NODE_TYPE_PLAY)
    {
        if (app->current_playback->embedded_audio)
        {
            /*
             * TODO: We need a way to reset AVS Client library here.
             */

            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "STOP when playing embedded audio\n");
        }

        /*
         * Stop the playback.
         */

        audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_STOP, NULL);
        app->wait_for_audio_complete = WICED_TRUE;

        /*
         * Send playback stopped event.
         */

        wiced_time_get_time(&cur_time);
        event.offset_ms = cur_time - app->current_playback->start_ms + app->current_playback->offset_ms;
        event.token     = app->current_playback->token;
        avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PLAYBACK_STOPPED, &event);

        app->current_playback = NULL;
        app->playback_state   = AVS_PLAYBACK_STATE_STOPPED;

        update_timer_processing(app);
    }
}


static void avs_app_process_playback_stop(avs_app_t* app)
{
    avs_app_audio_node_t* node;

    /*
     * Stop any current playback.
     */

    avs_app_stop_playback(app);

    /*
     * Free the play list.
     */

    while (app->play_list)
    {
        node = app->play_list;
        app->play_list = node->next;
        FREE_AUDIO_NODE(node);
    }
}


static void avs_app_clear_playback_queue(avs_app_t* app, AVS_CLEAR_BEHAVIOR_T behavior)
{
    avs_app_audio_node_t* node;
    avs_app_audio_node_t* next;

    if (behavior == AVS_CLEAR_BEHAVIOR_CLEAR_ALL)
    {
        /*
         * We need to replace everything in the play list.
         */

        if (app->current_playback != NULL)
        {
            if (app->current_playback->type == AVS_APP_AUDIO_NODE_TYPE_SPEAK)
            {
                /*
                 * We don't touch speak requests.
                 */
            }
            else
            {
                /*
                 * We're dealing with a play request.
                 */

                avs_app_stop_playback(app);
            }
        }

        while (app->play_list != NULL)
        {
            next = app->play_list->next;
            FREE_AUDIO_NODE(app->play_list);
            app->play_list = next;
        }
    }
    else
    {
        /*
         * We need to replace all but the currently playing stream.
         */

        if (app->current_playback != NULL && app->current_playback == app->play_list)
        {
            node = app->play_list->next;
            app->play_list->next = NULL;
        }
        else
        {
            node = app->play_list;
            app->play_list = NULL;
        }
        while (node != NULL)
        {
            next = node->next;
            FREE_AUDIO_NODE(node);
            node = next;
        }
    }
}


static void avs_app_clear_queue(avs_app_t* app, AVS_CLEAR_BEHAVIOR_T behavior)
{
    /*
     * Clear the current playback queue according to instructions.
     */

    avs_app_clear_playback_queue(app, behavior);

    /*
     * And tell AVS that we've done so.
     */

    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PLAYBACK_QUEUE_CLEARED, NULL);
}


static void avs_app_process_playlist_entry_available(avs_app_t* app)
{
    audio_client_play_offset_t play_offset;
    avs_client_playback_event_t event;
    wiced_result_t result;
    char* ptr;
    int i;

    if (app->current_playback == NULL || app->current_playback->playlist_data == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "No playlist data for playback\n");
        return;
    }

    if (app->current_playback->cur_playlist_entry >= app->current_playback->num_playlist_entries)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "No available playlist entries (%d,%d)\n",
                      app->current_playback->cur_playlist_entry, app->current_playback->num_playlist_entries);
        return;
    }

    ptr = app->current_playback->playlist_data;
    for (i = 0; i < app->current_playback->cur_playlist_entry; i++)
    {
        ptr += strlen(ptr) + 1;
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Play playlist track: %s\n", ptr);

    if (app->current_playback->offset_ms)
    {

        play_offset.uri       = ptr;
        play_offset.offset_ms = app->current_playback->offset_ms;

        result = audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_PLAY_OFFSET, &play_offset);
    }
    else
    {
        result = audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_PLAY, ptr);
    }
    app->current_playback->cur_playlist_entry++;

    if (result == WICED_SUCCESS)
    {
        event.offset_ms = app->current_playback->offset_ms;
        event.token     = app->current_playback->token;
        avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PLAYBACK_STARTED, &event);

        PLAYBACK_START_HOUSEKEEPING(app->current_playback);
        app->playback_state = AVS_PLAYBACK_STATE_PLAYING;
    }
    else
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to start playback (%d)\n", result);
    }

    update_timer_processing(app);
}


static void avs_app_process_playback_available(avs_app_t* app)
{
    audio_client_play_offset_t play_offset;
    avs_client_playback_event_t event;
    wiced_bool_t embedded_audio;
    wiced_result_t result;

    if (app->recording_active || app->recording_initiated)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Playback available but recording is active\n");
        return;
    }

    if (app->wait_for_audio_complete)
    {
        /*
         * Waiting on a completion event from audio client before starting the next item.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Waiting on playback completion\n");
        return;
    }

    if (app->current_playback != NULL)
    {
        /*
         * Playback is already active so there's nothing to do now.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Playback already active in process_playback_available\n");
        return;
    }

    /*
     * Do we have a pending expect speech directive to process?
     */

    if (!app->speak_list && app->expect_speech)
    {
        avs_app_process_expect_speech(app);
        if (app->recording_initiated)
        {
            return;
        }
    }

    if (app->speak_list != NULL || (app->play_list && app->play_list->embedded_audio))
    {
        embedded_audio = WICED_TRUE;
    }
    else
    {
        embedded_audio = WICED_FALSE;
    }

    if (embedded_audio)
    {
        /*
         * Start the next request going.
         */

        app->current_playback      = app->speak_list != NULL ? app->speak_list : app->play_list;
        app->configure_audio_codec = WICED_TRUE;
        wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_VOICE_PLAYBACK);
    }
    else if (app->play_list != NULL)
    {
        app->current_playback = app->play_list;

        /*
         * And ask the audio client to play it for us.
         */

        if (app->current_playback->offset_ms)
        {

            play_offset.uri       = app->current_playback->url;
            play_offset.offset_ms = app->current_playback->offset_ms;

            result = audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_PLAY_OFFSET, &play_offset);
        }
        else
        {
            result = audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_PLAY, app->current_playback->url);
        }

        if (result == WICED_SUCCESS)
        {
            event.offset_ms = app->current_playback->offset_ms;
            event.token     = app->current_playback->token;
            avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PLAYBACK_STARTED, &event);

            PLAYBACK_START_HOUSEKEEPING(app->current_playback);
            app->playback_state = AVS_PLAYBACK_STATE_PLAYING;
        }
        else
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to start playback (%d)\n", result);
        }

        /*
         * Make sure the timer is going if we need to report progress events.
         */

        update_timer_processing(app);
    }
}


static void avs_app_send_playback_finished_event(avs_app_t* app, avs_app_audio_node_t* audio)
{
    avs_client_playback_event_t event;
    uint32_t cur_time;

    wiced_time_get_time(&cur_time);

    /*
     * First send the playback nearly finished event. Sometimes AVS won't send the next
     * item unless the nearly finished event is sent.
     * We don't send the nearly finished event early since we don't have the memory
     * to start buffering the next track early.
     */

    event.offset_ms = cur_time - audio->start_ms + audio->offset_ms;
    event.token     = audio->token;
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PLAYBACK_NEARLY_FINISHED, &event);

    /*
     * And now send the playback finished event.
     * Note that we have sometimes seen out of order responses from AVS when sending
     * the NearlyFinished and PlaybackFinished events back to back. For now
     * add a slight delay between the messages.
     */

    wiced_rtos_delay_milliseconds(10);
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PLAYBACK_FINISHED, &event);

    app->playback_state = AVS_PLAYBACK_STATE_FINISHED;
}


static void avs_app_process_spoken_list_playback_complete(avs_app_t* app, wiced_bool_t playback_error)
{
    avs_client_playback_event_t event;
    avs_app_audio_node_t* ptr;

    /*
     * Send the finished event.
     */

    if (app->spoken_list->type == AVS_APP_AUDIO_NODE_TYPE_SPEAK)
    {
        avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_SPEECH_FINISHED, app->spoken_list->token);
    }
    else
    {
        /*
         * Send playback finished event.
         */

        avs_app_send_playback_finished_event(app, app->spoken_list);
    }

    /*
     * Remove the item from our list and free it.
     */

    if (app->spoken_list)
    {
        ptr = app->spoken_list;
        app->spoken_list = ptr->next;
        FREE_AUDIO_NODE(ptr);
    }

    /*
     * And send the playback started event for the next item.
     */

    if (app->spoken_list)
    {
        ptr = app->spoken_list;
    }
    else
    {
        ptr = app->current_playback;
    }

    if (ptr != NULL)
    {
        /*
         * Send a speech started or playback started event here.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "avs_app_process_spoken_list_playback_complete: sending started event...\n");
        if (ptr->type == AVS_APP_AUDIO_NODE_TYPE_SPEAK)
        {
            avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_SPEECH_STARTED, ptr->token);
        }
        else
        {
            event.offset_ms = 0;
            event.token     = ptr->token;
            avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PLAYBACK_STARTED, &event);

            PLAYBACK_START_HOUSEKEEPING(ptr);
            app->playback_state = AVS_PLAYBACK_STATE_PLAYING;
        }
    }
    else
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "No node for sending playback start event\n");
    }
}


static void avs_app_process_playback_complete(avs_app_t* app, wiced_bool_t playback_error)
{
    avs_app_audio_node_t* anode;

    if (app->spoken_list != NULL)
    {
        avs_app_process_spoken_list_playback_complete(app, playback_error);
        return;
    }

    app->wait_for_audio_complete = WICED_FALSE;

    if (app->current_playback != NULL)
    {
        if (app->current_playback->type == AVS_APP_AUDIO_NODE_TYPE_SPEAK)
        {
            /*
             * Send speech finished event to AVS and remove it from the list.
             */

            avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_SPEECH_FINISHED, app->current_playback->token);

            if (app->speak_list)
            {
                anode = app->speak_list;
                app->speak_list = anode->next;
                FREE_AUDIO_NODE(anode);
            }
        }
        else if (app->current_playback->type == AVS_APP_AUDIO_NODE_TYPE_PLAY)
        {
            if (app->current_playback->playlist_data && app->current_playback->cur_playlist_entry < app->current_playback->num_playlist_entries)
            {
                avs_app_process_playlist_entry_available(app);
                return;
            }

            /*
             * Send playback finished event.
             */

            avs_app_send_playback_finished_event(app, app->current_playback);

            if (app->play_list)
            {
                anode = app->play_list;
                app->play_list = anode->next;
                FREE_AUDIO_NODE(anode);
            }
        }
        app->current_playback = NULL;

        /*
         * Make sure that the timer processing is updated if necessary.
         */

        update_timer_processing(app);
    }

    /*
     * Kick off the next request if there is one.
     */

    avs_app_process_playback_available(app);
}


static void avs_app_process_voice_playback(avs_app_t* app)
{
    audio_client_push_buf_t ac_buf;
    avs_client_audio_buf_t avsc_buf;
    avs_client_playback_event_t event;
    wiced_result_t result;

    if (app->current_playback == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Current playback node not set!\n");
        return;
    }

    /*
     * Are we just starting up? If so we need to configure the codec.
     */

    if (app->configure_audio_codec)
    {
        if (app->wait_for_audio_complete)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "No startie while waitie!!!\n");
            return;
        }

        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Start voice playback for type %d\n", app->current_playback->type);
        result = audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_SET_CODEC, (uint32_t*)AUDIO_CLIENT_CODEC_MP3);
        if (result != WICED_SUCCESS)
        {
            /*
             * TODO: Need a way to reset avs client here...
             */

            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error configuring audio client\n");
            return;
        }
        app->configure_audio_codec = WICED_FALSE;

        if (app->spoken_list == NULL)
        {
            /*
             * Send a speech started or playback started event here.
             */

            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Spoken list is NULL - sending normal started event...\n");
            if (app->current_playback->type == AVS_APP_AUDIO_NODE_TYPE_SPEAK)
            {
                avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_SPEECH_STARTED, app->current_playback->token);
            }
            else
            {
                event.offset_ms = 0;
                event.token     = app->current_playback->token;
                avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PLAYBACK_STARTED, &event);

                PLAYBACK_START_HOUSEKEEPING(app->current_playback);
                app->playback_state = AVS_PLAYBACK_STATE_PLAYING;
            }
        }
    }

    /*
     * Get the next chunk of data from the AVS Client.
     */

    avsc_buf.buf    = app->audio_buf;
    avsc_buf.buflen = AVS_APP_AUDIO_BUF_SIZE;
    avsc_buf.len    = 0;
    avsc_buf.eos    = WICED_FALSE;

    result = avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_GET_AUDIO_DATA, &avsc_buf);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error returned from GET_AUDIO_DATA\n");
        result = audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_STOP, NULL);

        /*
         * TODO: Send a speech finished (or error) event here.
         */

        return;
    }

    if (avsc_buf.len > 0)
    {
        ac_buf.buf       = avsc_buf.buf;
        ac_buf.buflen    = avsc_buf.len;
        ac_buf.pushedlen = 0;

        do
        {
            result = audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_PUSH_BUFFER, &ac_buf);
            if (result != WICED_SUCCESS)
            {
                if (result == WICED_PENDING)
                {
                    wiced_rtos_delay_milliseconds(2);
                }
                else
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "audio_client_ioctl(PUSH_BUFFER) failed with %d\n", result);
                    break;
                }
            }
        } while (result != WICED_SUCCESS);
    }
    else if (!avsc_buf.eos)
    {
        /*
         * Not EOS but no data available. Sleep a tiny bit to wait for more data.
         */

        wiced_rtos_delay_milliseconds(2);
    }

    if (avsc_buf.eos)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Sending NULL buffer to audio client\n");
        ac_buf.buf    = NULL;
        ac_buf.buflen = 0;
        result = audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_PUSH_BUFFER, &ac_buf);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "push empty buffer failed with %d\n", result);
            audio_client_ioctl(app->audio_client_handle, AUDIO_CLIENT_IOCTL_STOP, NULL);
        }
        else
        {
            if (start_next_embedded_audio(app))
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Embedded audio item up next\n");
            }
            else
            {
                app->wait_for_audio_complete = WICED_TRUE;
            }
        }
    }
    else
    {
        /*
         * More to do so set the flag to trigger us again.
         */

        wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_VOICE_PLAYBACK);
    }
}


static void avs_app_process_capture_trigger(avs_app_t* app)
{
    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Capture button pressed\n");
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_SPEECH_TRIGGER, NULL);
}


static void avs_app_process_disconnect(avs_app_t* app)
{
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_DISCONNECT, NULL);
}


static void avs_app_process_connect(avs_app_t* app)
{
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_CONNECT, NULL);
}


static void avs_app_process_play(avs_app_t* app)
{
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PLAY_COMMAND_ISSUED, NULL);
}


static void avs_app_process_pause(avs_app_t* app)
{
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PAUSE_COMMAND_ISSUED, NULL);
}


static void avs_app_process_next(avs_app_t* app)
{
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_NEXT_COMMAND_ISSUED, NULL);
}


static void avs_app_process_previous(avs_app_t* app)
{
    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_PREVIOUS_COMMAND_ISSUED, NULL);
}


static void avs_app_process_inactivity_report(avs_app_t* app)
{
    wiced_time_t now;
    uint32_t seconds;

    wiced_time_get_time(&now);
    seconds = (now - avs_app_start_time) / 1000;

    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_USER_INACTIVITY_REPORT, (void*)seconds);
}


static wiced_result_t avs_app_process_delete_alert(avs_app_t* app, char* token)
{
    avs_app_alert_node_t* prev;
    avs_app_alert_node_t* alert;

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Processing DeleteAlert Directive\n");

    /*
     * Let's see if we can find this alert in our list.
     */

    prev = NULL;
    for (alert = app->alert_list; alert != NULL; alert = alert->next)
    {
        if (strcmp(alert->token, token) == 0)
        {
            break;
        }
        prev = alert;
    }

    if (alert)
    {
        /*
         * Found it. We need to remove it from our list.
         */

        if (app->active_alert && app->active_alert == alert)
        {
            avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_ALERT_STOPPED, token);
            app->active_alert = NULL;
        }

        if (prev)
        {
            prev->next = alert->next;
        }
        else
        {
            app->alert_list = alert->next;
        }
        free(alert);

        avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_DELETE_ALERT_SUCCEEDED, token);
    }
    else
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "DeleteAlert Directive failed for token %s\n", token);
        for (alert = app->alert_list; alert != NULL; alert = alert->next)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "   Alert %s, token %s\n", alert->type == AVS_ALERT_TYPE_TIMER ? "timer" : "alarm", alert->token);
        }

        /*
         * We didn't find it. Tell AVS that the DeleteAlert failed.
         */

        avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_DELETE_ALERT_FAILED, token);
    }

    free(token);

    /*
     * And make sure that the timer is updated for the current state of alerts.
     */

    update_timer_processing(app);

    return WICED_SUCCESS;
}


static wiced_result_t avs_app_process_set_alert(avs_app_t* app, avs_app_alert_node_t* new_alert)
{
    avs_app_alert_node_t* prev;
    avs_app_alert_node_t* alert;

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Processing SetAlert Directive\n");

    /*
     * Insert this alert into the list. Keep the list ordered by time.
     * NOTE: The current implementation will not handle multiple alerts
     * set to go off at the same time.
     */

    alert = app->alert_list;
    prev  = NULL;

    while (alert != NULL && new_alert->utc_ms > alert->utc_ms)
    {
        prev  = alert;
        alert = alert->next;
    }

    if (prev == NULL)
    {
        app->alert_list = new_alert;
        new_alert->next = NULL;
    }
    else
    {
        prev->next      = new_alert;
        new_alert->next = alert;
    }

    /*
     * Ideally the alerts should be stored in flash so that they are persistent
     * across reboots. We don't do that in this app (at least not yet).
     */

    /*
     * Let AVS know that we have successfully set this alert.
     */

    avs_client_ioctl(app->avs_client_handle, AVSC_IOCTL_SET_ALERT_SUCCEEDED, new_alert->token);

    /*
     * Update the timer.
     */

    update_timer_processing(app);

    return WICED_SUCCESS;
}


static wiced_result_t avs_app_process_playlist_loaded(avs_app_t* app, char* playlist)
{
    avs_app_audio_node_t* play;
    WICED_LOG_LEVEL_T level;
    char* ptr;

    if (app->current_playback == NULL || app->current_playback->type != AVS_APP_AUDIO_NODE_TYPE_PLAY || app->current_playback->embedded_audio)
    {
        /*
         * We received a playlist but don't know where it belongs. Free it and move on.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Playlist received with no current play item\n");
        free(playlist);
        return WICED_ERROR;
    }

    level = wiced_log_get_facility_level(WLF_DEF);

    play = app->current_playback;
    play->playlist_type = app->playlist_type;

    if (level >= WICED_LOG_DEBUG0)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "\nReceived playlist:\n");
        for (ptr = playlist; *ptr != '\0'; ptr++)
        {
            wiced_log_printf("%c", *ptr);
        }
        wiced_log_printf("\n");
    }

    if (play->playlist_data != NULL)
    {
        /*
         * A playlist has loaded another playlist. Free the first one.
         * Ultimately for a better implementation, we'd allow playlists
         * to reference other playlists.
         */

        free(play->playlist_data);
    }

    play->playlist_data = playlist;

    /*
     * Now we need to parse the playlist.
     */

    avs_app_parse_playlist(play);

    if (level >= WICED_LOG_DEBUG0)
    {
        int i;
        ptr = play->playlist_data;

        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Parsed %d playlist entries\n", play->num_playlist_entries);
        for (i = 0; i < play->num_playlist_entries; i++)
        {
            wiced_log_printf("%d) %s\n", i, ptr);
            ptr += strlen(ptr) + 1;
        }
    }

    if (play->num_playlist_entries > 0)
    {
        wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_PLAYLIST_ENTRY_AVAILABLE);
    }
    else
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "No entries found in playlist\n");
        wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_PLAYBACK_COMPLETE);
    }

    return WICED_SUCCESS;
}


static wiced_result_t avs_app_process_play_directive(avs_app_t* app, avs_app_audio_node_t* play)
{
    avs_app_audio_node_t* node;

    if (play == NULL)
    {
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Play directive received: %s\n", play->token);
    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "   URL: %s\n", play->url);
    play->next = NULL;

    /*
     * What's the behavior for this directive?
     */

    if (play->behavior == AVS_PLAY_BEHAVIOR_REPLACE_ALL)
    {
        avs_app_clear_playback_queue(app, AVS_CLEAR_BEHAVIOR_CLEAR_ALL);
    }
    else if (play->behavior == AVS_PLAY_BEHAVIOR_REPLACE_ENQUEUED)
    {
        avs_app_clear_playback_queue(app, AVS_CLEAR_BEHAVIOR_CLEAR_ENQUEUED);
    }

    /*
     * And add this play directive to our list.
     * TODO: Add previous token checking.
     */

    if (app->play_list == NULL)
    {
        app->play_list = play;
    }
    else
    {
        for (node = app->play_list; node->next != NULL; node = node->next)
        {
            /* Empty */
        }
        node->next = play;
    }

    wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_PLAYBACK_AVAILABLE);

    return WICED_SUCCESS;
}


static void avs_app_process_msg_queue(avs_app_t* app)
{
    avs_app_msg_t msg;
    avs_app_audio_node_t* speak;
    avs_app_audio_node_t* node;
    wiced_result_t result;

    result = wiced_rtos_pop_from_queue(&app->msgq, &msg, WICED_NO_WAIT);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error reading message from queue\n");
        return;
    }

    switch (msg.type)
    {
        case AVS_APP_MSG_SPEAK_DIRECTIVE:
            speak = (avs_app_audio_node_t*)msg.arg1;
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Main loop received speak directive: %s\n", speak->token);
            speak->next = NULL;
            if (app->speak_list == NULL)
            {
                app->speak_list = speak;
            }
            else
            {
                for (node = app->speak_list; node->next != NULL; node = node->next)
                {
                    /* Empty */
                }
                node->next = speak;
            }
            wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_PLAYBACK_AVAILABLE);
            break;

        case AVS_APP_MSG_PLAY_DIRECTIVE:
            avs_app_process_play_directive(app, (avs_app_audio_node_t*)msg.arg1);
            break;

        case AVS_APP_MSG_PLAYLIST_LOADED:
            avs_app_process_playlist_loaded(app, (char*)msg.arg1);
            break;

        case AVS_APP_MSG_EXPECT_SPEECH:
            avs_app_process_expect_speech(app);
            break;

        case AVS_APP_MSG_SET_ALERT:
            avs_app_process_set_alert(app, (avs_app_alert_node_t*)msg.arg1);
            break;

        case AVS_APP_MSG_DELETE_ALERT:
            avs_app_process_delete_alert(app, (char*)msg.arg1);
            break;

        case AVS_APP_MSG_SET_VOLUME:
            avs_app_set_new_volume(app, (int32_t)msg.arg1);
            break;

        case AVS_APP_MSG_ADJUST_VOLUME:
            avs_app_set_new_volume(app, app->audio_client_volume + (int32_t)msg.arg1);
            break;

        case AVS_APP_MSG_SET_MUTE:
            avs_app_set_new_mute(app, msg.arg1);
            break;

        case AVS_APP_MSG_SET_LOCALE:
            avs_app_set_locale(app, msg.arg1);
            break;

        case AVS_APP_MSG_CLEAR_QUEUE:
            avs_app_clear_queue(app, msg.arg1);
            break;

        default:
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unexpected msg %d\n", msg.type);
            break;
    }

    /*
     * If there's another message waiting, make sure that the proper event flag is set.
     */

    if (wiced_rtos_is_queue_empty(&app->msgq) != WICED_SUCCESS)
    {
        wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_MSG_QUEUE);
    }
}


static void avs_app_mainloop(avs_app_t* app)
{
    wiced_result_t      result;
    uint32_t            events;

    wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "Begin app mainloop\n");

    while (app->tag == AVS_APP_TAG_VALID)
    {
        events = 0;

        result = wiced_rtos_wait_for_event_flags(&app->events, AVS_APP_ALL_EVENTS, &events, WICED_TRUE, WAIT_FOR_ANY_EVENT, WICED_WAIT_FOREVER);
        if (result != WICED_SUCCESS)
        {
            continue;
        }

        if (events & AVS_APP_EVENT_SHUTDOWN)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "mainloop received EVENT_SHUTDOWN\n");
            break;
        }

        if (events & AVS_APP_EVENT_MSG_QUEUE)
        {
            avs_app_process_msg_queue(app);
        }

        if (events & AVS_APP_EVENT_CONNECT)
        {
            avs_app_process_connect(app);
        }

        if (events & AVS_APP_EVENT_DISCONNECT)
        {
            avs_app_process_disconnect(app);
        }

        if (events & AVS_APP_EVENT_STOP)
        {
            avs_app_process_playback_stop(app);
        }

        if (events & AVS_APP_EVENT_CAPTURE)
        {
            avs_app_process_capture_trigger(app);
        }

        if (events & AVS_APP_EVENT_VOICE_PLAYBACK)
        {
            avs_app_process_voice_playback(app);
        }

        if (events & AVS_APP_EVENT_PLAYBACK_COMPLETE)
        {
            avs_app_process_playback_complete(app, WICED_FALSE);
        }

        if (events & AVS_APP_EVENT_PLAYBACK_ERROR)
        {
            avs_app_process_playback_complete(app, WICED_TRUE);
        }

        if (events & AVS_APP_EVENT_PLAYLIST_ENTRY_AVAILABLE)
        {
            avs_app_process_playlist_entry_available(app);
        }

        if (events & AVS_APP_EVENT_PLAYBACK_AVAILABLE)
        {
            avs_app_process_playback_available(app);
        }

        if (events & AVS_APP_EVENT_TIMER)
        {
            avs_app_process_timer(app);
        }

        if (events & AVS_APP_EVENT_REPORT)
        {
            avs_app_process_inactivity_report(app);
        }

        if (events & AVS_APP_EVENT_PLAY)
        {
            avs_app_process_play(app);
        }

        if (events & AVS_APP_EVENT_PAUSE)
        {
            avs_app_process_pause(app);
        }

        if (events & AVS_APP_EVENT_NEXT)
        {
            avs_app_process_next(app);
        }

        if (events & AVS_APP_EVENT_PREVIOUS)
        {
            avs_app_process_previous(app);
        }

        if (events & AVS_APP_EVENT_VOLUME_UP)
        {
            avs_app_set_new_volume(app, app->audio_client_volume + AVS_APP_VOLUME_STEP);
        }

        if (events & AVS_APP_EVENT_VOLUME_DOWN)
        {
            avs_app_set_new_volume(app, app->audio_client_volume - AVS_APP_VOLUME_STEP);
        }

        if (events & AVS_APP_EVENT_RELOAD_DCT_WIFI)
        {
            avs_app_config_reload_dct_wifi(&app->dct_tables);
        }

        if (events & AVS_APP_EVENT_RELOAD_DCT_NETWORK)
        {
            avs_app_config_reload_dct_network(&app->dct_tables);
        }
    }   /* while */

    wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "End app mainloop\n");
}


static void avs_app_shutdown(avs_app_t* app)
{
    avs_app_audio_node_t* anode;
    avs_app_audio_node_t* next;
    avs_app_alert_node_t* alert;

    avs_app_keypad_deinit(app);

    /*
     * Shutdown the console.
     */

    avs_app_console_stop(app);

    app->tag = AVS_APP_TAG_INVALID;

    avs_app_audio_rx_thread_stop(app);
    avs_app_audio_playback_deinit(app);

    wiced_rtos_deinit_event_flags(&app->events);
    wiced_rtos_deinit_queue(&app->msgq);
    wiced_rtos_deinit_timer(&app->timer);

    avs_app_config_deinit(&app->dct_tables);

    sntp_stop_auto_time_sync();

    if (app->audio_client_handle != NULL)
    {
        audio_client_deinit(app->audio_client_handle);
        app->audio_client_handle = NULL;
    }

    if (app->avs_client_handle != NULL)
    {
        avs_client_deinit(app->avs_client_handle);
        app->avs_client_handle = NULL;
    }

    if (app->avs_client_handle != NULL)
    {
        avs_client_deinit(app->avs_client_handle);
        app->avs_client_handle = NULL;
    }

    for (anode = app->speak_list; anode != NULL; anode = next)
    {
        next = anode->next;
        FREE_AUDIO_NODE(anode);
    }
    app->speak_list = NULL;

    for (anode = app->play_list; anode != NULL; anode = next)
    {
        next = anode->next;
        FREE_AUDIO_NODE(anode);
    }
    app->play_list = NULL;

    for (anode = app->spoken_list; anode != NULL; anode = next)
    {
        next = anode->next;
        FREE_AUDIO_NODE(anode);
    }
    app->spoken_list = NULL;

    /*
     * Free up any alert list.
     */

    while (app->alert_list != NULL)
    {
        alert = app->alert_list->next;
        free(app->alert_list);
        app->alert_list = alert;
    }

    /*
     * And finally free the main application structure.
     */

    free(app);
}


static avs_app_t* avs_app_init(void)
{
    avs_app_t*          app;
    avs_client_params_t avs_params;
    wiced_time_t        time;
    wiced_result_t      result;
    uint32_t            tag;

    tag = AVS_APP_TAG_VALID;

    /* Initialize the device */
    result = wiced_init();
    if (result != WICED_SUCCESS)
    {
        return NULL;
    }

    /*
     * Note what time we started up.
     */

    result = wiced_time_get_time(&avs_app_start_time);

     /* initialize audio */
    platform_init_audio();

    /*
     * Initialize the logging subsystem.
     */

    wiced_log_init(WICED_LOG_INFO, avs_app_log_output_handler, avs_app_log_get_time);

    /*
     * Allocate the main application structure.
     */

    app = calloc_named("avs_app", 1, sizeof(avs_app_t));
    if (app == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to allocate app structure\n");
        return NULL;
    }

    /*
     * Create the command console.
     */

    avs_app_console_start(app);

    /*
     * Initialize keypad / button manager
     */

    result = avs_app_keypad_init(app);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing keypad\n");
    }

    /*
     * Create our event flags, timer, and message queue.
     */

    result = wiced_rtos_init_event_flags(&app->events);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing event flags\n");
        tag = AVS_APP_TAG_INVALID;
    }

    result = wiced_rtos_init_timer(&app->timer, HEARTBEAT_DURATION_MS, timer_callback, app);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing timer\n");
        tag = AVS_APP_TAG_INVALID;
    }

    result = wiced_rtos_init_queue(&app->msgq, NULL, sizeof(avs_app_msg_t), AVS_APP_MSG_QUEUE_SIZE);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing message queue\n");
        tag = AVS_APP_TAG_INVALID;
    }

    /* read in our configurations */
    avs_app_config_init(&app->dct_tables);

    /* print out our current configuration */
    avs_app_config_print_info(&app->dct_tables);

    wiced_log_set_all_levels(app->dct_tables.dct_app->log_level);

    /*
     * Create the audio capture thread.
     */

    result = avs_app_audio_rx_thread_start(app);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing audio rx\n");
        tag = AVS_APP_TAG_INVALID;
    }

    /* Bring up the network interface */
    result = wiced_network_up_default(&app->dct_tables.dct_network->interface, NULL);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Bringing up network interface failed\n");
        tag = AVS_APP_TAG_INVALID;
    }
    else
    {
        /*
         * Start SNTP.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Initializing SNTP...\n");
        if ((result = sntp_start_auto_time_sync(1 * HOURS)) != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to start SNTP (%d)\n", result);
        }
        g_avs_app = app;
    }

    /*
     * Seed the random number generator here. We're relying on the fact that the
     * network initialization takes a (slightly) variable amount of time.
     */

    wiced_time_get_time(&time);
    srand(time);

    /*
     * Fire off the AVS Client.
     */

    avs_params.userdata             = app;
    avs_params.interface            = app->dct_tables.dct_network->interface;
    avs_params.ASR_profile          = AVS_ASR_PROFILE_NEAR;
    avs_params.response_buffer_size = (64 * 1024);
    avs_params.refresh_token        = app->dct_tables.dct_app->refresh_token;
    avs_params.client_id            = app->dct_tables.dct_app->client_id;
    avs_params.client_secret        = app->dct_tables.dct_app->client_secret;

    avs_params.base_url       = AVS_BASE_URL_NA;

    avs_params.event_callback = avsc_event_callback;

    if ((app->avs_client_handle = avs_client_init(&avs_params)) == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to initialize AVS client\n");
        tag = AVS_APP_TAG_INVALID;
    }

    /*
     * And spin up the audio playback pipeline.
     */

    result = avs_app_audio_playback_init(app);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing audio tx\n");
        tag = AVS_APP_TAG_INVALID;
    }

    /* set our valid tag */
    app->tag = tag;

    return app;
}


void application_start(void)
{
    avs_app_t* app;

    /*
     * Main initialization.
     */

    if ((app = avs_app_init()) == NULL)
    {
        return;
    }
    g_avs_app = app;

    if (app->tag != AVS_APP_TAG_VALID)
    {
        /*
         * We didn't initialize successfully. Bail out here so that the console
         * will remain active. This lets the user correct any invalid configuration parameters.
         * Mark the structure as valid so the console command processing will work and
         * note that we are intentionally leaking the app structure memory here.
         */

        app->tag = AVS_APP_TAG_VALID;
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Main application thread exiting...command console still active\n");
        return;
    }

    /*
     * Drop into our main loop.
     */

    avs_app_mainloop(app);

    /*
     * Cleanup and exit.
     */

    g_avs_app = NULL;
    avs_app_shutdown(app);
    app = NULL;
}
