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
 * AVS Client Directive Routines
 *
 */

#include "wiced_tcpip.h"
#include "wiced_log.h"

#include "cJSON.h"

#include "avs_client.h"
#include "avs_client_private.h"
#include "avs_client_tokens.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef wiced_result_t (*directive_handler_t)(avs_client_t* client, cJSON* root, cJSON* header);

typedef struct
{
    char*               namespace;
    directive_handler_t handler;
} avsc_directive_entry_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t process_system_directive(avs_client_t* client, cJSON* root, cJSON* header);
static wiced_result_t process_speaker_directive(avs_client_t* client, cJSON* root, cJSON* header);
static wiced_result_t process_audio_player_directive(avs_client_t* client, cJSON* root, cJSON* header);
static wiced_result_t process_alerts_directive(avs_client_t* client, cJSON* root, cJSON* header);
static wiced_result_t process_speech_synthesizer_directive(avs_client_t* client, cJSON* root, cJSON* header);
static wiced_result_t process_speech_recognizer_directive(avs_client_t* client, cJSON* root, cJSON* header);

/******************************************************
 *               Variable Definitions
 ******************************************************/

static avsc_directive_entry_t directive_table[] =
{
    { AVS_TOKEN_SPEECHRECOGNIZER,   process_speech_recognizer_directive     },
    { AVS_TOKEN_SPEECHSYNTHESIZER,  process_speech_synthesizer_directive    },
    { AVS_TOKEN_ALERTS,             process_alerts_directive                },
    { AVS_TOKEN_AUDIOPLAYER,        process_audio_player_directive          },
    { AVS_TOKEN_SPEAKER,            process_speaker_directive               },
    { AVS_TOKEN_SYSTEM,             process_system_directive                },
    { NULL,                         NULL                                    }
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_result_t process_system_directive(avs_client_t* client, cJSON* root, cJSON* header)
{
    wiced_result_t result = WICED_SUCCESS;
    cJSON* endpoint;
    cJSON* payload;
    cJSON* name;
    char* new_endpoint;

    /*
     * Get the directive name.
     */

    name    = cJSON_GetObjectItem(header, AVS_TOKEN_NAME);
    payload = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_PAYLOAD);
    if (name == NULL || payload == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Not a valid System directive\n");
        return WICED_ERROR;
    }

    if (strcmp(name->valuestring, AVS_TOKEN_RESETUSERINACTIVITY) == 0)
    {
        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_RESET_USER_INACTIVITY, NULL);
        wiced_time_get_time(&client->active_time);
    }
    else if (strcmp(name->valuestring, AVS_TOKEN_SETENDPOINT) == 0)
    {
        endpoint = cJSON_GetObjectItem(payload, AVS_TOKEN_ENDPOINT);
        if (endpoint == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Endpoint not found in SetEndpoint directive\n");
            return WICED_ERROR;
        }
        new_endpoint = strdup(endpoint->valuestring);
        if (new_endpoint == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to allocate endpoint URL\n");
            return WICED_ERROR;
        }
        if (client->params.base_url)
        {
            free(client->params.base_url);
        }
        client->params.base_url = new_endpoint;
        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_SET_END_POINT, new_endpoint);

        /*
         * Send a disconnected event to the main loop. That will trigger a disconnect, cleanup, and reconnect.
         */

        wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_DISCONNECTED);
    }
    else
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown System directive: %s\n", name->valuestring);
        return WICED_ERROR;
    }

    return result;
}


static wiced_result_t process_speaker_directive(avs_client_t* client, cJSON* root, cJSON* header)
{
    wiced_result_t result = WICED_SUCCESS;
    cJSON* payload;
    cJSON* volume;
    cJSON* mute;
    cJSON* name;

    /*
     * Get the directive name.
     */

    name    = cJSON_GetObjectItem(header, AVS_TOKEN_NAME);
    payload = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_PAYLOAD);
    if (name == NULL || payload == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Not a valid Speaker directive\n");
        return WICED_ERROR;
    }

    if (strcmp(name->valuestring, AVS_TOKEN_SETVOLUME) == 0)
    {
        volume = cJSON_GetObjectItem(payload, AVS_TOKEN_VOLUME);
        if (volume == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Missing volume in SetVolume directive\n");
            return WICED_ERROR;
        }
        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_SET_VOLUME, (void*)volume->valueint);
    }
    else if (strcmp(name->valuestring, AVS_TOKEN_ADJUSTVOLUME) == 0)
    {
        volume = cJSON_GetObjectItem(payload, AVS_TOKEN_VOLUME);
        if (volume == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Missing volume in AdjustVolume directive\n");
            return WICED_ERROR;
        }
        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_ADJUST_VOLUME, (void*)volume->valueint);
    }
    else if (strcmp(name->valuestring, AVS_TOKEN_SETMUTE) == 0)
    {
        mute = cJSON_GetObjectItem(payload, AVS_TOKEN_MUTE);
        if (mute == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Missing mute in SetMute directive\n");
            return WICED_ERROR;
        }
        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_SET_MUTE, (void*)(mute->type == cJSON_True ? 1 : 0));
    }
    else
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown Speaker directive: %s\n", name->valuestring);
        return WICED_ERROR;
    }

    return result;
}


static wiced_result_t process_audio_player_directive(avs_client_t* client, cJSON* root, cJSON* header)
{
    avs_client_play_t play;
    cJSON* payload;
    cJSON* audio_item;
    cJSON* progress_report;
    cJSON* stream;
    cJSON* format;
    cJSON* previous_token;
    cJSON* token;
    cJSON* url;
    AVS_CLEAR_BEHAVIOR_T clear_behavior;
    wiced_result_t result = WICED_SUCCESS;

    /*
     * Get the directive name.
     */

    token   = cJSON_GetObjectItem(header, AVS_TOKEN_NAME);
    payload = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_PAYLOAD);
    if (token == NULL || payload == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Not a valid AudioPlayer directive\n");
        return WICED_ERROR;
    }

    if (strcmp(token->valuestring, AVS_TOKEN_PLAY) == 0)
    {
        /*
         * We have an AudioPlayer Play directive. Now we need to pull out all the
         * JSON fields that we are interested in.
         */

        memset(&play, 0, sizeof(avs_client_play_t));
        play.behavior = AVS_PLAY_BEHAVIOR_ENQUEUE;

        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "AudioPlayer: Play Directive\n");

        if ((audio_item = cJSON_GetObjectItem(payload, AVS_TOKEN_AUDIOITEM)) == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Missing audioItem field\n");
            return WICED_ERROR;
        }

        token = cJSON_GetObjectItem(payload, AVS_TOKEN_PLAYBEHAVIOR);
        if (token != NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "   Behavior: %s\n", token->valuestring);
            if (strcmp(token->valuestring, AVS_TOKEN_REPLACEALL) == 0)
            {
                play.behavior = AVS_PLAY_BEHAVIOR_REPLACE_ALL;
            }
            else if (strcmp(token->valuestring, AVS_TOKEN_REPLACEENQUEUED) == 0)
            {
                play.behavior = AVS_PLAY_BEHAVIOR_REPLACE_ENQUEUED;
            }
            else
            {
                play.behavior = AVS_PLAY_BEHAVIOR_ENQUEUE;
            }
        }

        if ((stream = cJSON_GetObjectItem(audio_item, AVS_TOKEN_STREAM)) == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Missing stream field\n");
            return WICED_ERROR;
        }

        url = cJSON_GetObjectItem(stream, AVS_TOKEN_URL);
        if (strncmp(url->valuestring, AVS_TOKEN_EMBEDDED_AUDIO, sizeof(AVS_TOKEN_EMBEDDED_AUDIO) - 1) == 0)
        {
            /*
             * Verify the audio format.
             * Note: Even though the Amazon API docs say this field will always be present for
             * binary audio attachments, experience shows that this is not always the case.
             */

            format = cJSON_GetObjectItem(stream, AVS_TOKEN_STREAMFORMAT);
            if (format != NULL && strncmp(format->valuestring, AVS_TOKEN_AUDIO_MPEG, sizeof(AVS_TOKEN_AUDIO_MPEG) - 1) != 0)
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown audio format: %s\n", format->valuestring);
                return WICED_ERROR;
            }

            client->response_total_attachments++;
        }
        play.url = url->valuestring;
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "   URL: %s\n", url->valuestring);

        if ((token = cJSON_GetObjectItem(stream, AVS_TOKEN_OFFSET_IN_MS)) != NULL)
        {
            play.offset_ms = token->valueint;
        }

        if ((progress_report = cJSON_GetObjectItem(stream, AVS_TOKEN_PROGRESSREPORT)) != NULL)
        {
            if ((token = cJSON_GetObjectItem(progress_report, AVS_TOKEN_PROGRESSREPORT_DIM)) != NULL)
            {
                play.report_delay_ms = token->valueint;
            }
            if ((token = cJSON_GetObjectItem(progress_report, AVS_TOKEN_PROGRESSREPORT_IIM)) != NULL)
            {
                play.report_interval_ms = token->valueint;
            }
        }

        if ((token = cJSON_GetObjectItem(stream, AVS_TOKEN_TOKEN)) != NULL)
        {
            play.token = token->valuestring;
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "   Token: %s\n", token->valuestring);
        }

        if ((previous_token = cJSON_GetObjectItem(stream, AVS_TOKEN_EXPECTEDPREVIOUSTOKEN)) != NULL)
        {
            play.previous_token = previous_token->valuestring;
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "   Previous token: %s\n", previous_token->valuestring);
        }

        /*
         * Tell the app about this directive.
         */

        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_PLAY, (void*)&play);
    }
    else if (strcmp(token->valuestring, AVS_TOKEN_STOP) == 0)
    {
        /*
         * Nothing to parse, just tell the app about this directive.
         */

        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_STOP, NULL);
    }
    else if (strcmp(token->valuestring, AVS_TOKEN_CLEARQUEUE) == 0)
    {
        token = cJSON_GetObjectItem(payload, AVS_TOKEN_CLEARBEHAVIOR);
        if (token != NULL)
        {
            if (strcmp(token->valuestring, AVS_TOKEN_CLEAREALL) == 0)
            {
                clear_behavior = AVS_CLEAR_BEHAVIOR_CLEAR_ALL;
            }
            else if (strcmp(token->valuestring, AVS_TOKEN_CLEARENQUEUED) == 0)
            {
                clear_behavior = AVS_CLEAR_BEHAVIOR_CLEAR_ENQUEUED;
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown clearBehavior in ClearQueue directive: %s\n", token->valuestring);
                return WICED_ERROR;
            }

            result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_CLEAR_QUEUE, (void*)clear_behavior);
        }
        else
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "No clearBehavior in ClearQueue directive\n");
        }
    }
    else
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown AudioPlayer directive: %s\n", token->valuestring);
        return WICED_ERROR;
    }

    return result;
}


static wiced_result_t process_alerts_directive(avs_client_t* client, cJSON* root, cJSON* header)
{
    avs_client_alert_t* new_alert;
    avs_client_set_alert_t alert;
    cJSON* payload;
    cJSON* name;
    cJSON* token;
    cJSON* type;
    cJSON* time;
    wiced_result_t result = WICED_SUCCESS;

    /*
     * Get the directive name.
     */

    name    = cJSON_GetObjectItem(header, AVS_TOKEN_NAME);
    payload = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_PAYLOAD);
    if (name == NULL || payload == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Not a valid Alerts directive\n");
        return WICED_ERROR;
    }

    if ((token = cJSON_GetObjectItem(payload, AVS_TOKEN_TOKEN)) == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Missing token in Alerts directive\n");
        return WICED_ERROR;
    }

    if (strcmp(name->valuestring, AVS_TOKEN_SETALERT) == 0)
    {
        type = cJSON_GetObjectItem(payload, AVS_TOKEN_TYPE);
        time = cJSON_GetObjectItem(payload, AVS_TOKEN_SCHEDULEDTIME);
        if (type == NULL || time == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Missing field in SetAlert directive\n");
            return WICED_ERROR;
        }
        if (strcmp(type->valuestring, AVS_TOKEN_TIMER) == 0)
        {
            alert.type = AVS_ALERT_TYPE_TIMER;
        }
        else if (strcmp(type->valuestring, AVS_TOKEN_ALARM) == 0)
        {
            alert.type = AVS_ALERT_TYPE_ALARM;
        }
        else
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown alert type in SetAlert directive: %s\n", type->valuestring);
            return WICED_ERROR;
        }

        alert.token = token->valuestring;
        alert.time  = time->valuestring;

        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_SET_ALERT, (void*)&alert);

        if (result == WICED_SUCCESS)
        {
            /*
             * Add this new alert to our list. We keep the alert list updated by monitoring
             * the alert events sent by the app.
             */

            new_alert = (avs_client_alert_t*)calloc(1, sizeof(avs_client_alert_t));
            if (new_alert)
            {
                new_alert->type = alert.type;
                strlcpy(new_alert->token, alert.token, AVS_MAX_TOKEN_LEN);
                avs_client_parse_iso8601_time_string(alert.time, &new_alert->utc_ms);
                new_alert->next    = client->alert_list;
                client->alert_list = new_alert;
            }
        }
    }
    else if (strcmp(name->valuestring, AVS_TOKEN_DELETEALERT) == 0)
    {
        /*
         * We already have all the information we need. Send the event to the app.
         */

        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_DELETE_ALERT, (void*)token->valuestring);
    }
    else
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown Alerts directive: %s\n", name->valuestring);
        return WICED_ERROR;
    }

    return result;
}


static wiced_result_t process_speech_synthesizer_directive(avs_client_t* client, cJSON* root, cJSON* header)
{
    cJSON* payload;
    cJSON* format;
    cJSON* token;
    cJSON* url;
    wiced_result_t result = WICED_SUCCESS;

    /*
     * Get the directive name.
     */

    token = cJSON_GetObjectItem(header, AVS_TOKEN_NAME);
    if (token == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to retrieve SpeechSynthesizer name token\n");
        return WICED_ERROR;
    }

    if (strcmp(token->valuestring, AVS_TOKEN_SPEAK) == 0)
    {
        payload = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_PAYLOAD);
        url     = cJSON_GetObjectItem(payload, AVS_TOKEN_URL);
        format  = cJSON_GetObjectItem(payload, AVS_TOKEN_FORMAT);
        token   = cJSON_GetObjectItem(payload, AVS_TOKEN_TOKEN);
        if (payload == NULL || url == NULL || format == NULL || token == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Not a valid Speak directive\n");
            return WICED_ERROR;
        }

        /*
         * Verify the audio format.
         */

        if (strncmp(format->valuestring, AVS_TOKEN_AUDIO_MPEG, sizeof(AVS_TOKEN_AUDIO_MPEG) - 1) != 0)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown audio format: %s\n", format->valuestring);
            return WICED_ERROR;
        }

        client->response_total_attachments++;

        /*
         * Tell the app about this directive.
         */

        result = client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_SPEAK, (void*)token->valuestring);
    }
    else
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown SpeechSynthesizer name token: %s\n", token->valuestring);
        return WICED_ERROR;
    }

    return result;
}


static wiced_result_t process_speech_recognizer_directive(avs_client_t* client, cJSON* root, cJSON* header)
{
    cJSON* node;
    cJSON* payload;
    wiced_result_t result = WICED_SUCCESS;

    /*
     * Get the directive name.
     */

    node = cJSON_GetObjectItem(header, AVS_TOKEN_NAME);
    if (node == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to retrieve SpeechRecognizer name token\n");
        return WICED_ERROR;
    }

    if (strcmp(node->valuestring, AVS_TOKEN_STOPCAPTURE) == 0)
    {
        wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_END_SPEECH_REQUEST);
    }
    else if (strcmp(node->valuestring, AVS_TOKEN_EXPECTSPEECH) == 0)
    {
        payload = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_PAYLOAD);
        node    = cJSON_GetObjectItem(payload, AVS_TOKEN_TIMEOUT_IN_MS);
        if (payload == NULL || node == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Not a valid ExpectSpeech directive\n");
            return WICED_ERROR;
        }

        /*
         * Tell the app about this directive.
         */

        client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_EXPECT_SPEECH, (void*)node->valueint);
        wiced_time_get_time(&client->expect_timeout);
        client->expect_timeout += node->valueint;
    }
    else
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unknown SpeechRecognizer name token: %s\n", node->valuestring);
        return WICED_ERROR;
    }

    return result;
}


static wiced_result_t process_directive_json(avs_client_t* client, cJSON* root)
{
    wiced_result_t result = WICED_SUCCESS;
    cJSON* header;
    cJSON* node;
    char* json_text;
    int i;

    if (wiced_log_get_facility_level(WLF_AVS_CLIENT) >= WICED_LOG_DEBUG0)
    {
        json_text = cJSON_Print(root);
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Directive JSON:\n%s\n", json_text);
        free(json_text);
    }

    header = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_HEADER);
    node   = cJSON_GetObjectItem(header, AVS_TOKEN_NAMESPACE);
    if (header == NULL || node == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Not a valid directive\n");
        return WICED_ERROR;
    }

    /*
     * Find the handler for this directive.
     */

    for (i = 0; directive_table[i].namespace != NULL; i++)
    {
        if (strcmp(node->valuestring, directive_table[i].namespace) == 0)
        {
            result = directive_table[i].handler(client, root, header);
            break;
        }
    }

    if (directive_table[i].namespace == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unhandled directive namespace: %s\n", node->valuestring);
    }

    return result;
}


void avsc_process_directive(avs_client_t* client)
{
    uint32_t widx;
    uint32_t idx;
    cJSON* root;
    char* json_text;
    char* ptr;
    int len;

    /*
     * How much data do we have in the buffer? Note that we only need to protect
     * getting the index since the HTTP2 callback will only modify the buffer
     * from the index on.
     */

    wiced_rtos_lock_mutex(&client->buffer_mutex);
    widx = client->directive_widx;
    wiced_rtos_unlock_mutex(&client->buffer_mutex);

    /*
     * Do we have a complete directive?
     */

    len = strlen(client->msg_boundary_downchannel);
    ptr = strnstr(client->directive_buf, widx, client->msg_boundary_downchannel, len);
    if (ptr && client->directive_first_boundary)
    {
        /*
         * The first directive will start with the message boundary so we need
         * to search for the second one.
         */

        ptr += len;
        ptr  = strnstr(ptr, widx - (uint32_t)(ptr - client->directive_buf), client->msg_boundary_downchannel, len);
    }
    if (ptr == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Directive incomplete\n");
        return;
    }
    client->directive_first_boundary = WICED_FALSE;

    *ptr = '\0';
    ptr += len + 2;             /* Add in the trailing '\r\n' */

    /*
     * Find the start of the JSON text.
     */

    json_text = avsc_find_JSON_start(client->directive_buf);
    if (json_text)
    {
        root = cJSON_Parse(json_text);
        if (root)
        {
            process_directive_json(client, root);
            cJSON_Delete(root);
        }
    }
    else
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to find directive json start: %s\n", client->directive_buf);
    }

    /*
     * Now adjust what's left in the buffer.
     */

    idx = (uint32_t)(ptr - client->directive_buf);
    wiced_rtos_lock_mutex(&client->buffer_mutex);
    if (idx >= client->directive_widx)
    {
        /*
         * The calculated index could be greater if the trailing '\r\n' haven't been received yet.
         */

        client->directive_widx = 0;
    }
    else
    {
        /*
         * Move it down.
         */

        memmove(client->directive_buf, ptr, client->directive_widx - idx);
        client->directive_widx = client->directive_widx - idx;
    }
    wiced_rtos_unlock_mutex(&client->buffer_mutex);
}


static wiced_result_t delve_speech_response_buffer(avs_client_t* client)
{
    wiced_result_t result;
    uint32_t audio_ridx = 0;
    uint32_t ridx;
    uint32_t widx;
    uint32_t avail;
    uint32_t idx;
    cJSON* root;
    char* json_text;
    char* start;
    char* end;
    char* ptr;
    char save;

    /*
     * We're still processing the directive part of the response.
     * Start by getting the current read/write indices into the buffer.
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
         * Somehow we were triggered with no data to be processed.
         * Nothing to see here...move along.
         */

        return WICED_SUCCESS;
    }

    if (widx > ridx)
    {
        avail = widx - ridx;
    }
    else
    {
        /*
         * Make our available data only the amount up to the buffer wrap. Our current
         * processing does not handle parsing the text data across the wrap boundary.
         */

        avail = client->params.response_buffer_size - ridx;
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "WARNING: Parsing response buffer after wrap (%lu,%lu)\n", ridx, widx);
    }

    end = &client->response_buffer[ridx + avail];

    /*
     * We are going to take advantage of the fact that AVS usually sends everything
     * we need in the first HTTP2 data frame. See if we already have the audio header
     * lurking in our buffer.
     */

    ptr = strnstr(&client->response_buffer[ridx], avail, avs_audio_content_str, strlen(avs_audio_content_str));
    if (ptr != NULL)
    {
        /*
         * We've found the MP3 data part of the response. Find the starting point if we can.
         */

        idx = 0;
        while (ptr < end)
        {
            if (*ptr == avs_HTTP_body_separator[idx])
            {
                if (avs_HTTP_body_separator[++idx] == '\0')
                {
                    ptr++;
                    break;
                }
            }
            else
            {
                idx = 0;
            }
            ptr++;
        }

        if (ptr < end)
        {
            /*
             * We've found it.
             */

            audio_ridx = (uint32_t)(ptr - client->response_buffer);
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Found audio data at index %lu\n", audio_ridx);
        }
    }

    if (client->response_first_boundary)
    {
        /*
         * The response will start with the message boundary;
         */

        if (avail < client->boundary_len + 2)
        {
            /*
             * Not enough data yet.
             */

            return WICED_SUCCESS;
        }

        if (strncmp(&client->response_buffer[ridx + 2], client->msg_boundary_response, client->boundary_len) != 0)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Response buffer does not start with message boundary\n");
            return WICED_ERROR;
        }
        client->response_first_boundary = WICED_FALSE;
        ridx  += client->boundary_len + 2;
        avail -= client->boundary_len + 2;
    }

    /*
     * Search for directives in our buffer.
     */

    start = &client->response_buffer[ridx];
    while (start < end)
    {
        /*
         * Do we have a complete directive?
         */

        ptr = strnstr(start, avail, client->msg_boundary_response, client->boundary_len);
        if (ptr == NULL)
        {
            break;
        }

        save = *ptr;
        *ptr = '\0';

        /*
         * Find the start of the JSON text.
         */

        json_text = avsc_find_JSON_start(start);
        if (json_text)
        {
            root = cJSON_Parse(json_text);
            if (root)
            {
                result = process_directive_json(client, root);
                cJSON_Delete(root);
                if (result != WICED_SUCCESS)
                {
                    return result;
                }
            }
        }
        else
        {
            /*
             * We need to restore the character we wiped out or the boundary scanning when
             * we process the audio data will fail.
             */

            *ptr = save;
            if (audio_ridx == 0 || ptr < &client->response_buffer[audio_ridx])
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to find directive json start: %s\n", start);
            }
            break;
        }

        ptr += client->boundary_len;
        ridx = (uint32_t)(ptr - client->response_buffer);
        start = &client->response_buffer[ridx];
    }

    /*
     * Done with processing directives. Update the current read index.
     */

    result = wiced_rtos_lock_mutex(&client->buffer_mutex);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to grab response buffer mutex for update\n");
        return WICED_ERROR;
    }

    if (audio_ridx)
    {
        client->response_ridx   = audio_ridx;
        client->response_binary = WICED_TRUE;
    }
    else
    {
        client->response_ridx = ridx;
    }
    wiced_rtos_unlock_mutex(&client->buffer_mutex);

    if (client->response_binary)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "At end of response parsing we have %lu audio attachments (%d)\n",
                      client->response_total_attachments, audio_ridx);
    }

    return WICED_SUCCESS;
}


void avsc_process_speech_response(avs_client_t* client)
{
    wiced_result_t result;

    if (client->response_discard)
    {
        /*
         * We encountered a processing error and just need to discard response data.
         */

        return;
    }

    if (!client->response_binary)
    {
        result = delve_speech_response_buffer(client);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error processing speech response\n");
            client->response_discard = WICED_TRUE;
            client->state            = AVSC_STATE_READY;
        }
    }

    if (client->response_complete)
    {
        if (client->state != AVSC_STATE_SPEECH_RESPONSE)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Entire response received: %lu attachments\n", client->response_total_attachments);
            if (client->response_total_attachments > 0)
            {
                client->state = AVSC_STATE_SPEECH_RESPONSE;
            }
            else
            {
                /*
                 * No embedded audio attachment to process.
                 */

                client->state = AVSC_STATE_READY;
                avsc_reset_response_tracking(client);
            }
        }
    }
}
