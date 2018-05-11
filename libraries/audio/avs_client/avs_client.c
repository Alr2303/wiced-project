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
 * AVS Client Routines
 *
 */

#include "wiced_tcpip.h"
#include "wiced_log.h"

#include "dns.h"
#include "cJSON.h"

#include "http2.h"
#include "sntp.h"

#include "avs_client.h"
#include "avs_client_private.h"
#include "avs_client_tokens.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HEARTBEAT_TIMER_PERIOD_MSECS            (1000)
#define PING_TIMER_PERIOD_MSECS                 (5 * 60 * 1000)

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

/******************************************************
 *               Function Definitions
 ******************************************************/

static void timer_callback(void *arg)
{
    avs_client_t* client = (avs_client_t *)arg;

    if (client && client->tag == AVSC_TAG_VALID)
    {
        wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_TIMER);
    }
}


void avsc_reset_response_tracking(avs_client_t* client)
{
    /*
     * Reset all our housekeeping variables.
     */

    client->response_first_boundary     = WICED_TRUE;
    client->response_complete           = WICED_FALSE;
    client->response_binary             = WICED_FALSE;
    client->response_discard            = WICED_FALSE;
    client->response_scan_for_next      = WICED_FALSE;
    client->response_total_attachments  = 0;
    client->response_cur_attachment     = 0;
    client->response_audio_begin_idx    = AVSC_NOT_SCANNED;
    client->response_audio_end_idx      = AVSC_NOT_SCANNED;
    client->response_widx               = 0;
    client->response_ridx               = 0;
    client->http2_response_stream_id    = 0;
}


static wiced_result_t avsc_prepare_synchronize_state(avs_client_t* client)
{
    avs_client_playback_state_t playback_state;
    avs_client_alert_t* alerts;
    wiced_result_t result;

    if (client->alert_list == NULL)
    {
        /*
         * Query the app for the current alert/timer list.
         */

        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_GET_ALERT_LIST, (void*)&alerts);
        if (result == WICED_SUCCESS)
        {
            client->alert_list = alerts;
        }
        else
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error returned from get alert list event (%d)\n", result);
        }
    }

    result  = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_GET_PLAYBACK_STATE, (void*)&playback_state);
    result |= client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_GET_VOLUME_STATE,   (void*)&client->volume_state);
    result |= client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_GET_SPEECH_STATE,   (void*)&client->speech_state);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error retrieving current state (%d)\n", result);
    }

    if (playback_state.state == AVS_PLAYBACK_STATE_STOPPED && playback_state.token[0] == '\0')
    {
        /*
         * Special case. AVS wants us to keep returning the last stream id (token)
         * in the stopped state. If we don't, they'll return an error to the
         * SynchronizeState message. Just make sure that we report the state
         * properly...we should already have the token stored from the earlier event messages.
         */

        client->playback_state.state = AVS_PLAYBACK_STATE_STOPPED;
    }
    else
    {
        memcpy(&client->playback_state, &playback_state, sizeof(avs_client_playback_state_t));
    }

    /*
     * Reset all our housekeeping variables since the SynchronizeState message
     * can send us a directive in the response.
     */

    avsc_reset_response_tracking(client);

    result = avsc_send_synchronize_state_message(client);

    return result;
}


static wiced_result_t process_bad_http_response(avs_client_t* client, uint32_t stream_id, uint32_t response_status)
{
    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Bad response (%lu) for stream %lu\n", response_status, stream_id);

    switch (client->state)
    {
        case AVSC_STATE_DOWNCHANNEL:
        case AVSC_STATE_SYNCHRONIZE:
            /*
             * Something went wrong with connecting to AVS. Send a disconnect event so that we clean up.
             */
            wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_DISCONNECT);
            break;

        case AVSC_STATE_SPEECH_REQUEST:
            /*
             * Tell the app to shut off the mic and go back to ready state.
             */
            if (client->voice_active)
            {
                client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_VOICE_STOP_RECORDING, NULL);
                client->voice_active = WICED_FALSE;
            }
            client->state = AVSC_STATE_READY;
            break;

        default:
            break;
    }
    return WICED_SUCCESS;
}


static wiced_result_t process_http_response(avs_client_t* client, uint32_t stream_id, uint32_t response_status)
{
    wiced_result_t result = WICED_SUCCESS;

    if (response_status != HTTP_STATUS_OK && response_status != HTTP_STATUS_NO_CONTENT)
    {
        /*
         * Something went wrong.
         */

        process_bad_http_response(client, stream_id, response_status);
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Stream %lu received a response status of %lu\n", stream_id, response_status);

    switch (client->state)
    {
        case AVSC_STATE_DOWNCHANNEL:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Sending initial synchronize state.\n");
            result = avsc_prepare_synchronize_state(client);
            break;

        case AVSC_STATE_SYNCHRONIZE:
            client->state = AVSC_STATE_READY;

            /*
             * Start the timer for our heartbeat timer requests.
             */

            if (wiced_rtos_is_timer_running(&client->timer) != WICED_SUCCESS)
            {
                wiced_rtos_start_timer(&client->timer);
            }

            /*
             * And let the app know that we've connected.
             */

            client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_CONNECTED, NULL);
            break;

        case AVSC_STATE_SPEECH_REQUEST:
            if (response_status != HTTP_STATUS_OK)
            {
                /*
                 * Alexa had nothing to say to our speech request.
                 */

                client->state = AVSC_STATE_READY;
            }
            break;

        case AVSC_STATE_READY:
        default:
            break;
    }

    return result;
}


static void process_get_voice_data(avs_client_t* client)
{
    avs_client_audio_buf_t audio_buf;
    wiced_result_t result;

    if (client->http2_voice_stream_id == 0 || !client->voice_active)
    {
        /*
         * We aren't in the right state for getting voice data...bail.
         */

        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Bail on voice data - voice_active %d, voice_sid %lu\n", client->voice_active, client->http2_voice_stream_id);
        return;
    }

    /*
     * Get the next chunk of voice audio data from the app.
     */

    audio_buf.buf    = &client->vbuf[client->vbuflen];
    audio_buf.buflen = sizeof(client->vbuf) - client->vbuflen;
    audio_buf.len    = 0;

    result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_VOICE_GET_DATA, (void*)&audio_buf);
    if (result != WICED_SUCCESS)
    {
        /*
         * Something went wrong getting the voice data so shut down the recording. The AVS processing will time out
         * and the response code processing will reset the state for us.
         */

        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error returned from voice get data\n");
        goto _badthing;
    }

    client->vbuflen += audio_buf.len;
    if (client->vbuflen >= AVSC_VOICE_BUF_SIZE)
    {
        /*
         * Send this data off.
         */

        result = http_request_put_data(&client->http2_connect, client->http2_voice_stream_id, client->vbuf, client->vbuflen, 0);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WLF_AVS_CLIENT, "Error writing audio data\n");
            goto _badthing;
        }
        client->vbuflen = 0;
   }

    /*
     * Fire off an event to read the voice data again.
     */

    wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_GET_VOICE_DATA);

    return;

  _badthing:
    client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_VOICE_STOP_RECORDING, NULL);
    client->voice_active = WICED_FALSE;
    avsc_finish_recognize_message(client);
}


static wiced_result_t avsc_finish_speech_request(avs_client_t* client)
{
    wiced_result_t result;

    if (client->voice_active)
    {
        client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_VOICE_STOP_RECORDING, NULL);
        client->voice_active = WICED_FALSE;
    }

    result = avsc_finish_recognize_message(client);

    return result;
}


static wiced_result_t avsc_start_speech_request(avs_client_t* client)
{
    wiced_result_t result;

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG1, "AVS client start speech request\n");

    if (client->voice_active)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "AVS client is already processing voice request\n");
        return WICED_ERROR;
    }

    client->expect_timeout = 0;

    if (client->response_discard)
    {
        /*
         * Reset all our housekeeping variables since the last response encountered an error.
         */

        avsc_reset_response_tracking(client);
    }

    /*
     * Make sure the overlord is ready to record voice data.
     */

    result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_VOICE_START_RECORDING, NULL);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error returned from voice start recording event\n");
        return WICED_ERROR;
    }
    client->voice_active = WICED_TRUE;

    /*
     * Send off the recognize event.
     */

    result = avsc_send_recognize_message(client);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error sending recognize event\n");
        client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_VOICE_STOP_RECORDING, NULL);
        client->voice_active = WICED_FALSE;
    }

    /*
     * Fire off an event to start reading the voice data.
     */

    wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_GET_VOICE_DATA);

    return WICED_SUCCESS;
}


static void avsc_handle_disconnect(avs_client_t* client)
{
    avs_client_alert_t* alert;
    avs_client_alert_t* next;
    wiced_result_t result;

    /*
     * Are we already idle?
     */

    if (client->state == AVSC_STATE_IDLE)
    {
        return;
    }

    /*
     * Stop the timer.
     */

    wiced_rtos_stop_timer(&client->timer);

    result = http_disconnect(&client->http2_connect);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error returned from http_disconnect: %d\n", result);
    }

    if (client->socket_ptr)
    {
        wiced_tcp_disconnect(client->socket_ptr);
        wiced_tcp_delete_socket(client->socket_ptr);
        client->socket_ptr = NULL;
    }

    /*
     * Free the access token and any alert entries.
     */

    if (client->access_token != NULL)
    {
        free(client->access_token);
        client->access_token = NULL;
    }

    for (alert = client->alert_list; alert != NULL; alert = next)
    {
        next = alert->next;
        free(alert);
    }
    client->alert_list = NULL;

    if (client->msg_boundary_downchannel)
    {
        free(client->msg_boundary_downchannel);
        client->msg_boundary_downchannel = NULL;
    }

    if (client->msg_boundary_response != NULL)
    {
        free(client->msg_boundary_response);
        client->msg_boundary_response = NULL;
    }

    client->active_dialog_id = 0;

    client->state = AVSC_STATE_IDLE;

    client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_DISCONNECTED, NULL);
}


static void avsc_handle_connect(avs_client_t* client)
{
    wiced_result_t result;

    /*
     * Are we already connected?
     */

    if (client->state != AVSC_STATE_IDLE)
    {
        return;
    }

    /*
     * Make sure that we have an access token.
     */

    if (client->access_token == NULL)
    {
        result = avsc_get_access_token(client);

        if (result != WICED_SUCCESS || client->access_token == NULL)
        {
            return;
        }
    }

    client->directive_widx  = 0;
    client->directive_first_boundary = WICED_TRUE;

    /*
     * Establish the initial HTTP2 connection to AVS.
     */

    if (avsc_connect_to_avs(client) != WICED_SUCCESS)
    {
        return;
    }

    /*
     * Send the message to establish the downchannel stream.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Establishing downchannel stream...\n");

    avsc_send_downchannel_message(client);
}


static void avsc_process_expect_speech_timed_out(avs_client_t* client)
{
    client->expect_timeout = 0;
    avsc_send_speech_timed_out_event(client);
}


static void process_timer_event(avs_client_t* client)
{
    uint8_t ping_data[HTTP2_PING_DATA_LENGTH];
    wiced_time_t time;
    wiced_result_t result;

    if (client->state < AVSC_STATE_READY)
    {
        /*
         * The connection is not established. Nothing to do.
         */

        if (wiced_rtos_is_timer_running(&client->timer) == WICED_SUCCESS)
        {
            wiced_rtos_stop_timer(&client->timer);
        }
        return;
    }

    /*
     * Get the current time.
     */

    wiced_time_get_time(&time);

    /*
     * Do we have an expect speech timeout?
     */

    if (client->expect_timeout && time > client->expect_timeout)
    {
        avsc_process_expect_speech_timed_out(client);
    }

    /*
     * Is it time to send a ping to let AVS know that we're alive?
     */

    if (time > client->active_time + PING_TIMER_PERIOD_MSECS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Send ping message.\n");
        memset(ping_data, 0, HTTP2_PING_DATA_LENGTH);
        result = http_ping_request(&client->http2_connect, ping_data, HTTP_REQUEST_FLAGS_PING_REQUEST);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error sending PING request (%d)\n", result);
        }
        client->active_time = time;
    }
}


static void process_msg_queue(avs_client_t* client)
{
    avsc_msg_t msg;
    wiced_result_t result;

    result = wiced_rtos_pop_from_queue(&client->msgq, &msg, WICED_NO_WAIT);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error reading message from queue\n");
        return;
    }

    switch (msg.type)
    {
        case AVSC_MSG_TYPE_HTTP_RESPONSE:
            process_http_response(client, msg.arg1, msg.arg2);
            break;

        case AVSC_MSG_TYPE_SPEECH_EVENT:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "AVS Client received speech event: %lu\n", msg.arg1);
            avsc_send_speech_event(client, (char*)msg.arg2, msg.arg1 == AVSC_IOCTL_SPEECH_STARTED ? WICED_TRUE : WICED_FALSE);
            free((char*)msg.arg2);
            break;

        case AVSC_MSG_TYPE_PLAYBACK_EVENT:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "AVS Client received playback event: %lu\n", msg.arg1);
            avsc_send_playback_event(client, msg.arg1, (avsc_playback_event_t*)msg.arg2);
            free((void*)msg.arg2);
            break;

        case AVSC_MSG_TYPE_ALERT_EVENT:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "AVS Client received alert event %d\n", msg.arg1);
            avsc_send_alert_event(client, msg.arg1, (char*)msg.arg2);
            free((void*)msg.arg2);
            break;

        case AVSC_MSG_TYPE_INACTIVITY_REPORT:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "AVS Client received inactivity report: %lu seconds\n", msg.arg1);
            avsc_send_inactivity_report_event(client, msg.arg1);
            break;

        case AVSC_MSG_TYPE_PLAYBACK_CONTROLLER:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "AVS Client received playback controller event: %lu\n", msg.arg1);
            avsc_send_playback_controller_event(client, msg.arg1);
            break;

        case AVSC_MSG_TYPE_SPEAKER_EVENT:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "AVS Client received speaker event: %lu\n", msg.arg1);
            avsc_send_speaker_event(client, msg.arg1, msg.arg2, msg.arg3);
            break;

        case AVSC_MSG_TYPE_SETTINGS_UPDATED:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "AVS Client received settings updated event: %lu\n", msg.arg1);
            avsc_send_settings_updated_event(client, msg.arg1);
            break;

        case AVSC_MSG_TYPE_EXCEPTION_ENCOUNTERED:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "AVS Client received exception encountered event\n");
            avsc_send_exception_encountered_event(client, (avsc_exception_event_t*)msg.arg1);
            free((void*)msg.arg1);
            break;

        default:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unexpected msg %d\n", msg.type);
            break;
    }

    /*
     * If there's another message waiting, make sure that the proper event flag is set.
     */

    if (wiced_rtos_is_queue_empty(&client->msgq) != WICED_SUCCESS)
    {
        wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_MSG_QUEUE);
    }
}


static void avsc_thread_main(uint32_t context)
{
    avs_client_t*   client = (avs_client_t*)context;
    wiced_result_t  result;
    uint32_t        events;
    wiced_bool_t    connect_again;

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_NOTICE, "AVS client thread begin\n");

    if (client == NULL)
    {
        return;
    }

    client->state = AVS_CLIENT_STATE_IDLE;

    while (client->tag == AVSC_TAG_VALID)
    {
        events = 0;

        result = wiced_rtos_wait_for_event_flags(&client->events, AVSC_ALL_EVENTS, &events, WICED_TRUE, WAIT_FOR_ANY_EVENT, WICED_WAIT_FOREVER);
        if (result != WICED_SUCCESS)
        {
            continue;
        }

        if (events & AVSC_EVENT_SHUTDOWN)
        {
            break;
        }

        if (events & AVSC_EVENT_CONNECT)
        {
            avsc_handle_connect(client);
        }

        if (events & AVSC_EVENT_DISCONNECT)
        {
            avsc_handle_disconnect(client);
        }

        if (events & AVSC_EVENT_DISCONNECTED)
        {
            connect_again = client->state > AVSC_STATE_IDLE ? WICED_TRUE : WICED_FALSE;
            avsc_handle_disconnect(client);
            if (connect_again)
            {
                avsc_handle_connect(client);
            }
        }

        if (events & AVSC_EVENT_MSG_QUEUE)
        {
            process_msg_queue(client);
        }

        if (events & AVSC_EVENT_TIMER)
        {
            process_timer_event(client);
        }

        if (events & AVSC_EVENT_SPEECH_REQUEST)
        {
            avsc_start_speech_request(client);
        }

        if (events & AVSC_EVENT_END_SPEECH_REQUEST)
        {
            avsc_finish_speech_request(client);
        }

        if (events & AVSC_EVENT_GET_VOICE_DATA)
        {
            process_get_voice_data(client);
        }

        if (events & AVSC_EVENT_DIRECTIVE_DATA)
        {
            avsc_process_directive(client);
        }

        if (events & AVSC_EVENT_RESPONSE_DATA)
        {
            avsc_process_speech_response(client);
        }

        if (events & AVSC_EVENT_SPEECH_TIMED_OUT)
        {
            avsc_process_expect_speech_timed_out(client);
        }

        if (events & AVSC_EVENT_PLAYBACK_QUEUE_CLEARED)
        {
            avsc_send_playback_queue_cleared_event(client);
        }
    }   /* while */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_NOTICE, "AVS client thread end\n");

    WICED_END_OF_CURRENT_THREAD();
}


static wiced_result_t avs_client_get_audio_data(avs_client_t* client, avs_client_audio_buf_t* abuf)
{
    wiced_bool_t copy_all;
    uint32_t to_copy;
    uint32_t avail;
    uint32_t widx;
    uint32_t ridx;
    uint32_t bytes;
    uint32_t len;
    wiced_result_t result;

    if (abuf == NULL)
    {
        return WICED_ERROR;
    }
    abuf->len = 0;
    abuf->eos = WICED_FALSE;

    if (client->response_total_attachments == 0 || client->response_discard)
    {
        abuf->eos = WICED_TRUE;
        return WICED_SUCCESS;
    }

    if (!client->response_binary)
    {
        /*
         * We haven't finished processing the directives yet.
         */

        return WICED_SUCCESS;
    }

    /*
     * Get the current buffer indices...
     */

    result = wiced_rtos_lock_mutex(&client->buffer_mutex);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to grab response buffer mutex\n");
        return WICED_ERROR;
    }
    ridx = client->response_ridx;
    widx = client->response_widx;
    wiced_rtos_unlock_mutex(&client->buffer_mutex);

    if (ridx == widx)
    {
        /*
         * No data available.
         */

        return WICED_SUCCESS;
    }

    if (client->response_scan_for_next)
    {
        /*
         * We need to scan for the beginning of the next track.
         */

        avsc_scan_for_audio_content(client, ridx, widx);
        if (client->response_audio_begin_idx == AVSC_NOT_SCANNED)
        {
            /*
             * Didn't find it yet...same as no data available.
             */

            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Start of audio not found: ridx %lu, widx: %lu\n", ridx, widx);
            return WICED_SUCCESS;
        }

        /*
         * Set the current read index for the start of the audio data.
         */

        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Found start of audio at location %lu\n", client->response_audio_begin_idx);

        client->response_scan_for_next = WICED_FALSE;
        ridx = client->response_audio_begin_idx;
        client->response_audio_begin_idx = AVSC_NOT_SCANNED;
        client->response_audio_end_idx   = AVSC_NOT_SCANNED;
    }

    if (client->response_audio_end_idx == AVSC_NOT_SCANNED)
    {
        /*
         * We need to scan for the end of this attachment.
         */

        avsc_scan_for_response_message_boundary(client, ridx, widx);
    }

    /*
     * Figure out how much data we can copy.
     */

    if (client->response_audio_end_idx != AVSC_NOT_SCANNED)
    {
        widx = client->response_audio_end_idx;
    }

    if (widx > ridx)
    {
        avail = widx - ridx;
    }
    else
    {
        avail = client->params.response_buffer_size - ridx + widx;
    }

    copy_all = WICED_FALSE;
    if (avail > abuf->buflen)
    {
        to_copy  = abuf->buflen;
    }
    else
    {
        to_copy = avail;
        if (client->response_audio_end_idx != AVSC_NOT_SCANNED && (ridx + to_copy) % client->params.response_buffer_size == client->response_audio_end_idx)
        {
            copy_all = WICED_TRUE;
        }
    }

    /*
     * Now copy the data over.
     */

    abuf->len = to_copy;
    bytes     = 0;
    while (to_copy > 0)
    {
        if (ridx + to_copy > client->params.response_buffer_size)
        {
            len = client->params.response_buffer_size - ridx;
        }
        else
        {
            len = to_copy;
        }

        memcpy(&abuf->buf[bytes], &client->response_buffer[ridx], len);
        bytes   += len;
        to_copy -= len;
        ridx     = (ridx + len) % client->params.response_buffer_size;
    }

    if (client->response_audio_end_idx != AVSC_NOT_SCANNED)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG1, "After copy: ridx %lu, audio end %lu, copy_all %d, cur %d, total %d\n",
                      ridx, client->response_audio_end_idx, copy_all, client->response_cur_attachment, client->response_total_attachments);
    }
    if (copy_all)
    {
        abuf->eos = WICED_TRUE;
        client->response_cur_attachment++;
        if (client->response_cur_attachment == client->response_total_attachments)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "End of last attachment...reset to response ready state\n");
            client->state = AVSC_STATE_READY;
            avsc_reset_response_tracking(client);
            ridx = 0;
        }
        else
        {
            client->response_audio_end_idx = AVSC_NOT_SCANNED;
            client->response_scan_for_next = WICED_TRUE;
        }
    }

    /*
     * And finally, update the read index.
     * No need to lock the mutex since the reader thread won't touch the read index.
     */

    client->response_ridx = ridx;

    return WICED_SUCCESS;
}


static void avs_client_shutdown(avs_client_t* client)
{
    if (client == NULL)
    {
        return;
    }

    /*
     * Shutdown any current network connections.
     */

    avsc_handle_disconnect(client);
    avsc_http2_shutdown(client);

    /*
     * Stop the main thread if it is running.
     */

    client->tag = AVSC_TAG_INVALID;
    if (client->client_thread_ptr != NULL)
    {
        wiced_rtos_thread_force_awake(&client->client_thread);
        wiced_rtos_thread_join(&client->client_thread);
        wiced_rtos_delete_thread(&client->client_thread);
        client->client_thread_ptr = NULL;
    }

    wiced_rtos_deinit_event_flags(&client->events);
    wiced_rtos_deinit_timer(&client->timer);
    wiced_rtos_deinit_queue(&client->msgq);
    wiced_rtos_deinit_mutex(&client->buffer_mutex);

    /*
     * Free all the dynamically allocated entries.
     * The alert list and access token were freed in the disconnect.
     */

    if (client->params.refresh_token != NULL)
    {
        free(client->params.refresh_token);
        client->params.refresh_token = NULL;
    }

    if (client->params.client_id != NULL)
    {
        free(client->params.client_id);
        client->params.client_id = NULL;
    }

    if (client->params.client_secret != NULL)
    {
        free(client->params.client_secret);
        client->params.client_secret = NULL;
    }

    if (client->params.base_url != NULL)
    {
        free(client->params.base_url);
        client->params.base_url = NULL;
    }

    if (client->response_buffer)
    {
        free(client->response_buffer);
        client->response_buffer = NULL;
    }

    free(client);
}


static avs_client_t* avs_client_initialize(avs_client_params_t* params)
{
    avs_client_t*   client;
    wiced_result_t  result;

    /*
     * Allocate the main avs client structure.
     */

    if ((client = calloc(1, sizeof(avs_client_t))) == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to allocate avs client structure\n");
        return NULL;
    }

    /*
     * Copy the configuration parameters.
     */

    memcpy(&client->params, params, sizeof(avs_client_params_t));

    /*
     * Make our own copies of the parameter string data. We don't want to keep
     * references to somebody else's data.
     */

    client->params.refresh_token  = strdup(params->refresh_token);
    client->params.client_id      = strdup(params->client_id);
    client->params.base_url       = strdup(params->base_url);

    if (params->client_secret != NULL)
    {
        client->params.client_secret = strdup(params->client_secret);
    }

    if (client->params.refresh_token == NULL || client->params.client_id == NULL ||
        (params->client_secret != NULL && client->params.client_secret == NULL) || client->params.base_url == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error allocating configuration params\n");
        goto _bail;
    }

    /*
     * Create the event flags.
     */

    result  = wiced_rtos_init_event_flags(&client->events);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error initializing avs client event flags\n");
        goto _bail;
    }

    result = wiced_rtos_init_timer(&client->timer, HEARTBEAT_TIMER_PERIOD_MSECS, timer_callback, client);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error initializing timer\n");
        goto _bail;
    }

    result = wiced_rtos_init_queue(&client->msgq, NULL, sizeof(avsc_msg_t), AVSC_MSG_QUEUE_SIZE);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error initializing message queue\n");
        goto _bail;
    }

    result = wiced_rtos_init_mutex(&client->buffer_mutex);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error initializing mutex\n");
        goto _bail;
    }

    /*
     * Allocate the speech response buffer.
     */

    client->response_buffer = malloc(params->response_buffer_size);
    if (client->response_buffer == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to allocate response buffer\n");
        goto _bail;
    }

    /*
     * Initialize the HTTP2 library.
     */

    if (avsc_http2_initialize(client) != WICED_SUCCESS)
    {
        goto _bail;
    }

    client->message_id = rand();
    client->dialog_id  = rand();

    return client;

  _bail:
    avs_client_shutdown(client);
    return NULL;
}


/** Initialize the avs client library.
 *
 * @param[in] params      : Pointer to the configuration parameters.
 *
 * @return Pointer to the avs client instance or NULL
 */
avs_client_ref avs_client_init(avs_client_params_t* params)
{
    avs_client_t* client;
    wiced_result_t result = WICED_SUCCESS;

    /*
     * Sanity checking.
     */

    if (params == NULL || params->event_callback == NULL)
    {
        return NULL;
    }

    if ((client = avs_client_initialize(params)) == NULL)
    {
        return NULL;
    }

    /*
     * Now create the main avs client thread.
     */

    client->tag = AVSC_TAG_VALID;

    result = wiced_rtos_create_thread_with_stack(&client->client_thread, AVSC_THREAD_PRIORITY, "AVS client thread", avsc_thread_main,
                                                 client->client_thread_stack_buffer, AVSC_THREAD_STACK_SIZE, client);
    if (result == WICED_SUCCESS)
    {
        client->client_thread_ptr = &client->client_thread;
    }
    else
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to create avs client thread\n");
        avs_client_shutdown(client);
        client = NULL;
    }

    return client;
}


/** Deinitialize the avs client library.
 *
 * @param[in] avs_client : Pointer to the avs client instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t avs_client_deinit(avs_client_ref avs_client)
{
    if (avs_client == NULL || avs_client->tag != AVSC_TAG_VALID)
    {
        return WICED_BADARG;
    }

    avs_client_shutdown(avs_client);

    return WICED_SUCCESS;
}


/** Send an IOCTL to the AVS client session.
 *
 * @param[in] avs_client    : Pointer to the avs client instance.
 * @param[in] cmd           : IOCTL command to process.
 * @param[inout] arg        : Pointer to argument for IOTCL.
 *
 * @return    Status of the operation.
 */
wiced_result_t avs_client_ioctl(avs_client_ref avs_client, AVSC_IOCTL_T cmd, void* arg)
{
    avs_client_playback_event_t* client_pb_event;
    avs_client_exception_event_t* client_exception_event;
    avs_client_volume_state_t* volume_state;
    avsc_exception_event_t* exception_event;
    avsc_playback_event_t* pb_event;
    AVS_CLIENT_STATE_T* state;
    avsc_msg_t msg;
    char* token;
    int len;

    if (avs_client == NULL || avs_client->tag != AVSC_TAG_VALID)
    {
        return WICED_BADARG;
    }

    wiced_time_get_time(&avs_client->active_time);

    switch (cmd)
    {
        case AVSC_IOCTL_STATE:
            state = (AVS_CLIENT_STATE_T*)arg;
            if (state != NULL)
            {
                *state = avs_client->state < AVSC_STATE_READY ? AVS_CLIENT_STATE_IDLE : AVS_CLIENT_STATE_CONNECTED;
            }
            break;

        case AVSC_IOCTL_CONNECT:
            wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_CONNECT);
            break;

        case AVSC_IOCTL_DISCONNECT:
            wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_DISCONNECT);
            break;

        case AVSC_IOCTL_EXPECT_SPEECH_TIMED_OUT:
            wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_SPEECH_TIMED_OUT);
            break;

        case AVSC_IOCTL_SPEECH_TRIGGER:
            if (arg == NULL)
            {
                if (avs_client->voice_active)
                {
                    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Speech request already in progress\n");
                    return WICED_ERROR;
                }
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_SPEECH_REQUEST);
            }
            else
            {
                if (!avs_client->voice_active)
                {
                    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Speech request not active\n");
                    return WICED_ERROR;
                }
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_END_SPEECH_REQUEST);
            }
            break;

        case AVSC_IOCTL_SPEECH_STARTED:
        case AVSC_IOCTL_SPEECH_FINISHED:
            if (arg == NULL)
            {
                return WICED_ERROR;
            }
            token = strdup((char*)arg);
            if (token == NULL)
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to duplicate token string\n");
                return WICED_ERROR;
            }
            msg.type = AVSC_MSG_TYPE_SPEECH_EVENT;
            msg.arg1 = cmd;
            msg.arg2 = (uint32_t)token;
            if (wiced_rtos_push_to_queue(&avs_client->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error pushing speech event message\n");
                free(token);
                return WICED_ERROR;
            }
            break;

        case AVSC_IOCTL_PLAYBACK_STARTED:
        case AVSC_IOCTL_PLAYBACK_NEARLY_FINISHED:
        case AVSC_IOCTL_PROGRESS_DELAY_ELAPSED:
        case AVSC_IOCTL_PROGRESS_INTERVAL_ELAPSED:
        case AVSC_IOCTL_PLAYBACK_STUTTER_STARTED:
        case AVSC_IOCTL_PLAYBACK_STUTTER_FINISHED:
        case AVSC_IOCTL_PLAYBACK_FINISHED:
        case AVSC_IOCTL_PLAYBACK_FAILED:
        case AVSC_IOCTL_PLAYBACK_STOPPED:
        case AVSC_IOCTL_PLAYBACK_PAUSED:
        case AVSC_IOCTL_PLAYBACK_RESUMED:
            client_pb_event = (avs_client_playback_event_t*)arg;
            if (client_pb_event == NULL || client_pb_event->token == NULL)
            {
                return WICED_ERROR;
            }
            len = sizeof(avsc_playback_event_t) + strlen(client_pb_event->token) + 1;
            if (cmd == AVSC_IOCTL_PLAYBACK_FAILED)
            {
                len += strlen(client_pb_event->error_msg) + 1;
            }
            pb_event = calloc(1, len);
            if (pb_event == NULL)
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to allocate playback event\n");
                return WICED_ERROR;
            }
            pb_event->token = pb_event->data;
            strcpy(pb_event->token, client_pb_event->token);
            pb_event->offset_ms   = client_pb_event->offset_ms;
            pb_event->duration_ms = client_pb_event->duration_ms;
            if (cmd == AVSC_IOCTL_PLAYBACK_FAILED)
            {
                pb_event->state = client_pb_event->state;
                pb_event->error = client_pb_event->error;
                pb_event->error_msg = &pb_event->data[strlen(pb_event->token) + 1];
                strcpy(pb_event->error_msg, client_pb_event->error_msg);
            }

            msg.type = AVSC_MSG_TYPE_PLAYBACK_EVENT;
            msg.arg1 = cmd;
            msg.arg2 = (uint32_t)pb_event;
            if (wiced_rtos_push_to_queue(&avs_client->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error pushing playback event message\n");
                free(pb_event);
                return WICED_ERROR;
            }
            break;

        case AVSC_IOCTL_PLAYBACK_QUEUE_CLEARED:
            /*
             * The application wants us to send a playback queue cleared event.
             * Ask the main loop to do so.
             */
            wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_PLAYBACK_QUEUE_CLEARED);
            break;

        case AVSC_IOCTL_SET_ALERT_SUCCEEDED:
        case AVSC_IOCTL_SET_ALERT_FAILED:
        case AVSC_IOCTL_DELETE_ALERT_SUCCEEDED:
        case AVSC_IOCTL_DELETE_ALERT_FAILED:
        case AVSC_IOCTL_ALERT_STARTED:
        case AVSC_IOCTL_ALERT_STOPPED:
        case AVSC_IOCTL_ALERT_FOREGROUND:
        case AVSC_IOCTL_ALERT_BACKGROUND:
            if (arg == NULL)
            {
                return WICED_ERROR;
            }
            token = strdup((char*)arg);
            if (token == NULL)
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to duplicate token string\n");
                return WICED_ERROR;
            }
            msg.type = AVSC_MSG_TYPE_ALERT_EVENT;
            msg.arg1 = cmd;
            msg.arg2 = (uint32_t)token;
            if (wiced_rtos_push_to_queue(&avs_client->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error pushing alert event message\n");
                free(token);
                return WICED_ERROR;
            }
            break;

        case AVSC_IOCTL_USER_INACTIVITY_REPORT:
            if (arg == NULL)
            {
                return WICED_ERROR;
            }
            msg.type = AVSC_MSG_TYPE_INACTIVITY_REPORT;
            msg.arg1 = (uint32_t)arg;
            if (wiced_rtos_push_to_queue(&avs_client->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error pushing user inactivity report event message\n");
                return WICED_ERROR;
            }
            break;

        case AVSC_IOCTL_GET_AUDIO_DATA:
            return avs_client_get_audio_data(avs_client, (avs_client_audio_buf_t*)arg);

        case AVSC_IOCTL_PLAY_COMMAND_ISSUED:
        case AVSC_IOCTL_PAUSE_COMMAND_ISSUED:
        case AVSC_IOCTL_NEXT_COMMAND_ISSUED:
        case AVSC_IOCTL_PREVIOUS_COMMAND_ISSUED:
            msg.type = AVSC_MSG_TYPE_PLAYBACK_CONTROLLER;
            msg.arg1 = cmd;
            if (wiced_rtos_push_to_queue(&avs_client->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error pushing user playback controller event message\n");
                return WICED_ERROR;
            }
            break;

        case AVSC_IOCTL_VOLUME_CHANGED:
        case AVSC_IOCTL_MUTE_CHANGED:
            if (arg == NULL)
            {
                return WICED_ERROR;
            }
            volume_state = (avs_client_volume_state_t*)arg;
            msg.type = AVSC_MSG_TYPE_SPEAKER_EVENT;
            msg.arg1 = cmd;
            msg.arg2 = (uint32_t)volume_state->volume;
            msg.arg3 = (uint32_t)volume_state->muted;
            if (wiced_rtos_push_to_queue(&avs_client->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error pushing user speaker event message\n");
                return WICED_ERROR;
            }
            break;

        case AVSC_IOCTL_SETTINGS_UPDATED:
            msg.type = AVSC_MSG_TYPE_SETTINGS_UPDATED;
            msg.arg1 = (uint32_t)arg;
            if (wiced_rtos_push_to_queue(&avs_client->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error pushing user playback controller event message\n");
                return WICED_ERROR;
            }
            break;

        case AVSC_IOCTL_EXCEPTION_ENCOUNTERED:
            client_exception_event = (avs_client_exception_event_t*)arg;
            if (client_exception_event == NULL || client_exception_event->directive == NULL || client_exception_event->error_message == NULL)
            {
                return WICED_ERROR;
            }
            len = sizeof(avsc_exception_event_t) + strlen(client_exception_event->directive) + 1 + strlen(client_exception_event->error_message) + 1;
            exception_event = calloc(1, sizeof(avsc_exception_event_t));
            if (exception_event == NULL)
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to allocate exception event\n");
                return WICED_ERROR;
            }
            exception_event->directive = exception_event->data;
            strcpy(exception_event->directive, client_exception_event->directive);

            exception_event->error_message = &exception_event->data[strlen(exception_event->directive) + 1];
            strcpy(exception_event->error_message, client_exception_event->error_message);

            exception_event->error_type = client_exception_event->error_type;

            msg.type = AVSC_MSG_TYPE_EXCEPTION_ENCOUNTERED;
            msg.arg1 = (uint32_t)exception_event;
            if (wiced_rtos_push_to_queue(&avs_client->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&avs_client->events, AVSC_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error pushing user exception encountered event message\n");
                free(exception_event);
                return WICED_ERROR;
            }
            break;

        default:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "AVS client unrecognized ioctl: %d\n", cmd);
            return WICED_ERROR;
    }

    return WICED_SUCCESS;
}
