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
 * AVS Client Utility Routines
 *
 */

#include <stdio.h>
#include <ctype.h>

#include "wiced_log.h"

#include "dns.h"

#include "avs_client.h"
#include "avs_client_private.h"
#include "avs_client_tokens.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define DEFAULT_HTTP_PORT                       (80)
#define DEFAULT_HTTPS_PORT                      (443)

#define DNS_TIMEOUT_MS                          (10 * 1000)

#define AVS_MIME_TYPE_JSON                      "application/json"

#define BASE_UTC_YEAR (1970)
#define IS_LEAP_YEAR( year ) ( ( ( year ) % 400 == 0 ) || \
                             ( ( ( year ) % 100 != 0 ) && ( ( year ) % 4 == 0 ) ) )
#define SECONDS_IN_365_DAY_YEAR  (31536000)
#define SECONDS_IN_A_DAY         (86400)
#define SECONDS_IN_A_HOUR        (3600)
#define SECONDS_IN_A_MINUTE      (60)

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

static char* playback_state_names[AVS_PLAYBACK_STATE_MAX] =
{
    "IDLE",
    "PLAYING",
    "PAUSED",
    "BUFFER_UNDERRUN",
    "FINISHED",
    "STOPPED"
};

static char* error_type_names[AVS_PLAYBACK_ERROR_MAX] =
{
    "MEDIA_ERROR_UNKNOWN",
    "MEDIA_ERROR_INVALID_REQUEST",
    "MEDIA_ERROR_SERVICE_UNAVAILABLE",
    "MEDIA_ERROR_INTERNAL_SERVER_ERROR",
    "MEDIA_ERROR_INTERNAL_DEVICE_ERROR"
};

char avs_HTTP_body_separator[] =
{
    "\r\n\r\n"
};

char avs_audio_content_str[] =
{
    "Content-Type: application/octet-stream"
};

static const uint32_t seconds_per_month[12] =
{
    31*SECONDS_IN_A_DAY,
    28*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t avs_client_parse_iso8601_time_string(char* time_string, uint64_t* utc_ms)
{
    uint16_t number_of_leap_years;
    int matches;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int offset;
    int i;

    if (time_string == NULL || utc_ms == NULL)
    {
        return WICED_ERROR;
    }

    *utc_ms = 0;

    /*
     * AVS sends time values in ISO 8601 UTC. We take advantage of the fact that they send us the data
     * in a known format rather than try and parse all possible 8601 formats.
     */

    matches = sscanf(time_string, "%d-%d-%dT%d:%d:%d+%d", &year, &month, &day, &hour, &minute, &second, &offset);

    /*
     * Make sure the string matches our assumptions.
     */

    if (matches != 7 || offset != 0 || year < BASE_UTC_YEAR || month > 12 || hour > 23 || minute > 59 || second > 59)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Bad time format string: %s\n", time_string);
        return WICED_ERROR;
    }

    /*
     * How many leap years are we dealing with?
     */

    number_of_leap_years = ( uint16_t )( ( ( year - ( BASE_UTC_YEAR - ( BASE_UTC_YEAR % 4 ) + 1 ) ) / 4 ) -
                           ( ( year - ( BASE_UTC_YEAR - ( BASE_UTC_YEAR % 100 ) + 1 ) ) / 100 ) +
                           ( ( year - ( BASE_UTC_YEAR - ( BASE_UTC_YEAR % 400 ) + 1 ) ) / 400 ) );

    *utc_ms = (year - BASE_UTC_YEAR) * SECONDS_IN_365_DAY_YEAR + (number_of_leap_years * SECONDS_IN_A_DAY);

    /*
     * Figure out the number of seconds for the months.
     */

    for (i = 1; i < month; i++)
    {
        *utc_ms += seconds_per_month[i-1];
    }

    /*
     * Is it a leap year?
     */

    if (IS_LEAP_YEAR(year) && month > 2)
    {
        *utc_ms += SECONDS_IN_A_DAY;
    }

    /*
     * Add in the seconds for the time of day.
     */

    *utc_ms += (day - 1) * SECONDS_IN_A_DAY;
    *utc_ms += hour * SECONDS_IN_A_HOUR;
    *utc_ms += minute * SECONDS_IN_A_MINUTE;
    *utc_ms += second;

    /*
     * And finally convert to milliseconds.
     */

    *utc_ms *= 1000;

    return WICED_SUCCESS;
}


/**
 * urlencode all non-alphanumeric characters in the C-string 'src'
 * store result in the C-string 'dst'
 *
 * NOTE: this routine assumes that dst is large enough and does not check length.
 *
 * return the length of the url encoded C-string
 */

int avsc_urlencode(char* dst, const char* src)
{
    char* d;
    int i;

    for (i = 0, d = dst; src[i]; i++)
    {
        if (isalnum((int)src[i]))
        {
            *(d++) = src[i];
        }
        else
        {
            sprintf(d, "%%%02X", src[i]);
            d += 3;
        }
    }

    *d = 0;

    return (int)(d - dst);
}


wiced_result_t avsc_uri_split(avs_client_t* client, const char* uri)
{
    const char* uri_start;
    int copy_len;
    int len;

    client->port = DEFAULT_HTTP_PORT;
    client->tls  = WICED_FALSE;

    /*
     * Skip over http:// or https://
     */

    uri_start = uri;
    if ((uri_start[0] == 'h' || uri_start[0] == 'H') && (uri_start[1] == 't' || uri_start[1] == 'T') &&
        (uri_start[2] == 't' || uri_start[2] == 'T') && (uri_start[3] == 'p' || uri_start[3] == 'P'))
    {
        uri_start += 4;
        if (uri_start[0] == 's' || uri_start[0] == 'S')
        {
            client->tls = WICED_TRUE;
            uri_start++;
        }
        if (uri_start[0] != ':' || uri_start[1] != '/' || uri_start[2] != '/')
        {
            return WICED_BADARG;
        }
        uri_start += 3;
    }

    /*
     * Isolate the host part of the URI.
     */

    for (len = 0; uri_start[len] != ':' && uri_start[len] != '/' && uri_start[len] != '\0'; )
    {
        len++;
    }

    if (uri_start[len] == ':')
    {
        client->port = atoi(&uri_start[len + 1]);
    }
    else if (client->tls == WICED_TRUE)
    {
        client->port = DEFAULT_HTTPS_PORT;
    }

    copy_len = len;
    if (copy_len > AVSC_HOSTNAME_LEN - 2)
    {
        copy_len = AVSC_HOSTNAME_LEN - 2;
    }
    memcpy(client->hostname, uri_start, copy_len);
    client->hostname[copy_len] = '\0';

    /*
     * Now pull out the path part.
     */

    uri_start += len;
    if (*uri_start == ':')
    {
        while (*uri_start != '/' && *uri_start != '\0')
        {
            uri_start++;
        }
    }

    if (*uri_start != '\0')
    {
        copy_len = strlen(uri_start);
        if (copy_len > AVSC_PATH_LEN - 2)
        {
            copy_len = AVSC_PATH_LEN - 2;
        }
        memcpy(client->path, uri_start, copy_len);
        client->path[copy_len] = '\0';
    }
    else
    {
        client->path[0] = '/';
        client->path[1] = '\0';
    }

    return WICED_SUCCESS;
}


wiced_result_t avsc_parse_uri_and_lookup_hostname(avs_client_t* client, const char* uri, wiced_ip_address_t* ip_address)
{
    wiced_result_t result;

    /*
     * Who are we connecting to?
     */

    if (avsc_uri_split(client, uri) != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to parse URL %s\n", uri);
        return WICED_ERROR;
    }

    /*
     * Are we dealing with an IP address or a hostname?
     */

    if (str_to_ip(client->hostname, ip_address) != 0)
    {
        /*
         * Try a hostname lookup.
         */

        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Attemping DNS lookup for %s\n", client->hostname);
        result = dns_client_hostname_lookup(client->hostname, ip_address, DNS_TIMEOUT_MS, client->params.interface);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "DNS lookup failed (%d)\n", result);
            return WICED_ERROR;
        }
    }

    return WICED_SUCCESS;
}


wiced_result_t avsc_scan_for_response_message_boundary(avs_client_t* client, uint32_t ridx, uint32_t widx)
{
    uint32_t save_idx;
    uint32_t end_idx;
    uint32_t bidx;
    uint32_t sidx;
    uint32_t idx;

    idx     = ridx;
    end_idx = widx;

    while (idx != end_idx)
    {
        if (client->response_buffer[idx] == 0x0D)
        {
            save_idx = idx;
            idx = (idx + 1) % client->params.response_buffer_size;

            if (idx == end_idx || client->response_buffer[idx] != 0x0A)
            {
                continue;
            }

            /*
             * Possible start of a message boundary. Now check for the initial
             * -- characters.
             */

            sidx = (idx + 1) % client->params.response_buffer_size;
            if (sidx == end_idx || client->response_buffer[sidx] != '-')
            {
                continue;
            }

            sidx = (sidx + 1) % client->params.response_buffer_size;
            if (sidx == end_idx || client->response_buffer[sidx] != '-')
            {
                continue;
            }

            /*
             * Looks good so far. Now we need to check against the message boundary string.
             */

            sidx = (sidx + 1) % client->params.response_buffer_size;
            bidx = 0;
            while (sidx != end_idx && bidx < client->boundary_len)
            {
                if (client->response_buffer[sidx] != client->msg_boundary_response[bidx])
                {
                    break;
                }
                bidx++;
                sidx = (sidx + 1) % client->params.response_buffer_size;
            }

            if (bidx == client->boundary_len)
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Message boundary at location %lu (%lu-%lu)\n", save_idx, ridx, end_idx);
                client->response_audio_end_idx = save_idx;
                break;
            }
        }

        idx = (idx + 1) % client->params.response_buffer_size;
    }

    return WICED_SUCCESS;
}


wiced_result_t avsc_scan_for_audio_content(avs_client_t* client, uint32_t ridx, uint32_t widx)
{
    uint32_t end_idx;
    uint32_t bidx;
    uint32_t sidx;
    uint32_t idx;

    idx     = ridx;
    end_idx = widx;
    bidx    = 0;

    while (idx != end_idx)
    {
        if (client->response_buffer[idx] == avs_audio_content_str[bidx])
        {
            idx  = (idx + 1) % client->params.response_buffer_size;
            sidx = idx;
            bidx++;

            while (sidx != end_idx)
            {
                if (avs_audio_content_str[bidx] == '\0' || client->response_buffer[sidx] != avs_audio_content_str[bidx])
                {
                    break;
                }
                bidx++;
                sidx = (sidx + 1) % client->params.response_buffer_size;
            }

            if (avs_audio_content_str[bidx] != '\0')
            {
                bidx = 0;
                continue;
            }

            /*
             * We found the field denoting audio content. Now we have to advance to the
             * beginning of the audio.
             */

            bidx = 0;
            while (sidx != end_idx)
            {
                if (avs_HTTP_body_separator[bidx] == '\0')
                {
                    break;
                }

                if (client->response_buffer[sidx] == avs_HTTP_body_separator[bidx])
                {
                    bidx++;
                }
                else
                {
                    bidx = 0;
                }
                sidx = (sidx + 1) % client->params.response_buffer_size;
            }

            if (avs_HTTP_body_separator[bidx] != '\0')
            {
                bidx = 0;
                continue;
            }

            /*
             * We found it. Woohoo!
             */

            client->response_audio_begin_idx = sidx;
            break;
        }
        else
        {
            bidx = 0;
        }

        idx = (idx + 1) % client->params.response_buffer_size;
    }

    return WICED_SUCCESS;
}


/**
 * Find the start of the JSON text in the buffer.
 *
 * @param buf : Pointer to the nul terminated buffer data
 *
 * @return Pointer to the start of the JSON text or NULL
 */

char* avsc_find_JSON_start(char* buf)
{
    char* json;
    int len;

    /*
     * Isolate the JSON portion of the message.
     */

    len  = strlen(AVS_MIME_TYPE_JSON);
    json = strstr(buf, AVS_MIME_TYPE_JSON);
    if (json == NULL)
    {
        return NULL;
    }
    json += len;

    /*
     * We stop searching at the message boundary if it follows the JSON mime type or at the end of the data.
     */

    while (*json != '{' && *json != '\0')
    {
        json++;
    }

    if (*json != '{')
    {
        return NULL;
    }

    return json;
}


static cJSON* avsc_create_header_and_payload_object(avs_client_t* client, char* namespace, char* name, cJSON** header, cJSON** payload, wiced_bool_t include_msgid)
{
    cJSON* root;
    char buf[20];

    *header  = NULL;
    *payload = NULL;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Create the header and payload sections.
     */

    cJSON_AddItemToObject(root, AVS_TOKEN_HEADER,  *header  = cJSON_CreateObject());
    cJSON_AddItemToObject(root, AVS_TOKEN_PAYLOAD, *payload = cJSON_CreateObject());
    if (header == NULL || payload == NULL)
    {
        goto _bad_object;
    }

    /*
     * Now add the header strings.
     */

    cJSON_AddStringToObject(*header, AVS_TOKEN_NAMESPACE, namespace);
    cJSON_AddStringToObject(*header, AVS_TOKEN_NAME,      name);

    if (client != NULL && include_msgid)
    {
        snprintf(buf, sizeof(buf), AVS_FORMAT_STRING_MID, client->message_id++);
        cJSON_AddStringToObject(*header, AVS_TOKEN_MESSAGEID, buf);
    }

    return root;

  _bad_object:
    if (root)
    {
        cJSON_Delete(root);
    }

    return NULL;
}


cJSON* avsc_create_alert_state_json(avs_client_alert_t* alert)
{
    cJSON* root;
    wiced_iso8601_time_t iso8601_time;
    char buf[sizeof(wiced_iso8601_time_t) + 1];

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Get the ISO8601 time for the alert.
     */

    wiced_time_convert_utc_ms_to_iso8601(alert->utc_ms, &iso8601_time);
    snprintf(buf, sizeof(buf), "%.*s", sizeof(wiced_iso8601_time_t), (char*)&iso8601_time);

    cJSON_AddStringToObject(root, AVS_TOKEN_TOKEN,         alert->token);
    cJSON_AddStringToObject(root, AVS_TOKEN_TYPE,          alert->type == AVS_ALERT_TYPE_ALARM ? AVS_TOKEN_ALARM : AVS_TOKEN_TIMER);
    cJSON_AddStringToObject(root, AVS_TOKEN_SCHEDULEDTIME, buf);

    return root;
}


cJSON* avsc_create_all_alerts_state_json(avs_client_alert_t* alert_list)
{
    cJSON* root;
    cJSON* header;
    cJSON* payload;
    cJSON* all_alerts;
    cJSON* active_alerts;
    cJSON* alert_object;
    avs_client_alert_t* alert;

    /*
     * Create the basic header and payload objects.
     */

    root = avsc_create_header_and_payload_object(NULL, AVS_TOKEN_ALERTS, AVS_TOKEN_ALERTSTATE, &header, &payload, WICED_FALSE);
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * And now fill in the payload.
     */

    cJSON_AddItemToObject(payload, AVS_TOKEN_ALLALERTS,    all_alerts    = cJSON_CreateArray());
    cJSON_AddItemToObject(payload, AVS_TOKEN_ACTIVEALERTS, active_alerts = cJSON_CreateArray());
    if (all_alerts == NULL || active_alerts == NULL)
    {
        goto _alert_state_bail;
    }

    /*
     * First we need to list all the alerts.
     */

    for (alert = alert_list; alert != NULL; alert = alert->next)
    {
        alert_object = avsc_create_alert_state_json(alert);
        if (alert_object == NULL)
        {
            goto _alert_state_bail;
        }
        cJSON_AddItemToArray(all_alerts, alert_object);
    }

    /*
     * Now we need the active alerts.
     */

    for (alert = alert_list; alert != NULL; alert = alert->next)
    {
        if (alert->active)
        {
            alert_object = avsc_create_alert_state_json(alert);
            if (alert_object == NULL)
            {
                goto _alert_state_bail;
            }
            cJSON_AddItemToArray(active_alerts, alert_object);
        }
    }

    return root;

  _alert_state_bail:
    if (root)
    {
        cJSON_Delete(root);
    }

    return NULL;
}


cJSON* avsc_create_volume_state_json(avs_client_volume_state_t *state)
{
    cJSON* root;
    cJSON* header;
    cJSON* payload;

    /*
     * Create the basic header and payload objects.
     */

    root = avsc_create_header_and_payload_object(NULL, AVS_TOKEN_SPEAKER, AVS_TOKEN_VOLUMESTATE, &header, &payload, WICED_FALSE);
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * And fill in the payload.
     */

    cJSON_AddNumberToObject(payload, AVS_TOKEN_VOLUME, state->volume);
    cJSON_AddBoolToObject(payload,   AVS_TOKEN_MUTED,  state->muted);

    return root;
}


cJSON* avsc_create_play_state_json(avs_client_t* client, wiced_bool_t playback)
{
    cJSON* root;
    cJSON* header;
    cJSON* payload;
    wiced_time_t now;
    uint32_t offset_ms;
    char* token;
    AVS_PLAYBACK_STATE_T state;

    /*
     * Create the basic header and payload objects.
     */

    root = avsc_create_header_and_payload_object(NULL, playback ? AVS_TOKEN_AUDIOPLAYER : AVS_TOKEN_SPEECHSYNTHESIZER,
                                                 playback ? AVS_TOKEN_PLAYBACKSTATE : AVS_TOKEN_SPEECHSTATE, &header, &payload, WICED_FALSE);
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * And fill in the payload.
     */

    if (playback)
    {
        /*
         * If playback is active, get the current playback status.
         */

        if (client->playback_state.state == AVS_PLAYBACK_STATE_PLAYING || client->playback_state.state == AVS_PLAYBACK_STATE_PAUSED ||
            client->playback_state.state == AVS_PLAYBACK_STATE_BUFFER_UNDERRUN)
        {
            client->params.event_callback(client, client->params.userdata, AVSC_APP_EVENT_GET_PLAYBACK_STATE, (void*)&client->playback_state);
        }

        offset_ms = client->playback_state.offset_ms;
        token     = client->playback_state.token;
        state     = client->playback_state.state;
    }
    else
    {
        token = client->speech_state.token;
        state = client->speech_state.state;
        if (state == AVS_PLAYBACK_STATE_PLAYING)
        {
            wiced_time_get_time(&now);
            offset_ms = now - client->speech_start;
        }
        else
        {
            offset_ms = client->speech_state.offset_ms;
        }
    }

    cJSON_AddStringToObject(payload, AVS_TOKEN_TOKEN,          token);
    cJSON_AddNumberToObject(payload, AVS_TOKEN_OFFSET_IN_MS,   offset_ms);
    cJSON_AddStringToObject(payload, AVS_TOKEN_PLAYERACTIVITY, playback_state_names[state]);

    return root;
}


cJSON* avsc_create_context_json(avs_client_t* client)
{
    cJSON* context;
    cJSON* state;

    context = cJSON_CreateArray();
    if (context == NULL)
    {
        return NULL;
    }

    /*
     * We need to create the JSON tree for the context container.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/context for format details.
     */

    state = avsc_create_play_state_json(client, WICED_TRUE);
    if (state == NULL)
    {
        goto _getout;
    }
    cJSON_AddItemToArray(context, state);

    state = avsc_create_play_state_json(client, WICED_FALSE);
    if (state == NULL)
    {
        goto _getout;
    }
    cJSON_AddItemToArray(context, state);

    state = avsc_create_volume_state_json(&client->volume_state);
    if (state == NULL)
    {
        goto _getout;
    }
    cJSON_AddItemToArray(context, state);

    state = avsc_create_all_alerts_state_json(client->alert_list);
    if (state == NULL)
    {
        goto _getout;
    }
    cJSON_AddItemToArray(context, state);

    return context;

  _getout:
    if (context != NULL)
    {
        cJSON_Delete(context);
    }
    return NULL;
}


cJSON* avsc_create_synchronize_state_json(avs_client_t* client)
{
    cJSON* root;
    cJSON* context;
    cJSON* event;
    cJSON* header;
    cJSON* payload;

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    context = avsc_create_context_json(client);
    if (context == NULL)
    {
        goto _no_good;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_CONTEXT, context);

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_SYSTEM, AVS_TOKEN_SYNCHRONIZESTATE, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _no_good;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    return root;

  _no_good:
    if (root)
    {
        cJSON_Delete(root);
    }

    return NULL;
}


cJSON* avsc_create_recognize_json(avs_client_t* client)
{
    cJSON* root;
    cJSON* context;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    char* profile;
    char buf[20];

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    context = avsc_create_context_json(client);
    if (context == NULL)
    {
        goto _no_good;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_CONTEXT, context);

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_SPEECHRECOGNIZER, AVS_TOKEN_ROCOGNIZE, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _no_good;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    /*
     * Add in the dialog id.
     */

    client->active_dialog_id = client->dialog_id;
    snprintf(buf, sizeof(buf), AVS_FORMAT_STRING_DID, client->dialog_id++);
    cJSON_AddStringToObject(header, AVS_TOKEN_DIALOGID, buf);

    /*
     * And finally the payload fields.
     */

    if (client->params.ASR_profile == AVS_ASR_PROFILE_CLOSE)
    {
        profile = AVS_AUDIO_PROFILE_CLOSE;
    }
    else if (client->params.ASR_profile == AVS_ASR_PROFILE_NEAR)
    {
        profile = AVS_AUDIO_PROFILE_NEAR;
    }
    else
    {
        profile = AVS_AUDIO_PROFILE_FAR;
    }

    cJSON_AddStringToObject(payload, AVS_TOKEN_PROFILE, profile);
    cJSON_AddStringToObject(payload, AVS_TOKEN_FORMAT,  AVS_AUDIO_FORMAT);

    return root;

  _no_good:
    if (root)
    {
        cJSON_Delete(root);
    }

    return NULL;
}


char* avsc_create_speech_event_json_text(avs_client_t* client, char* name, char* token)
{
    cJSON* root;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    char* JSON_text;

    /*
     * We need to create the JSON tree for the speech event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechsynthesizer for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_SPEECHSYNTHESIZER, name, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    cJSON_AddStringToObject(payload, AVS_TOKEN_TOKEN, token);

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

  _bail:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    return NULL;
}


char* avsc_create_speech_timed_out_event_json_text(avs_client_t* client)
{
    cJSON* root;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    char* JSON_text;

    /*
     * We need to create the JSON tree for the speech timed out event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechrecognizer for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_SPEECHRECOGNIZER, AVS_TOKEN_EXPECTSPEECHTIMEDOUT, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

  _bail:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    return NULL;
}


char* avsc_create_playback_event_json_text(avs_client_t* client, AVSC_IOCTL_T type, avsc_playback_event_t* pb_event)
{
    cJSON* root;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    cJSON* state;
    cJSON* error;
    char* JSON_text;
    char* name;

    switch (type)
    {
        case AVSC_IOCTL_PLAYBACK_STARTED:           name = AVS_TOKEN_PLAYBACKSTARTED;               break;
        case AVSC_IOCTL_PLAYBACK_NEARLY_FINISHED:   name = AVS_TOKEN_PLAYBACKNEARLYFINISHED;        break;
        case AVSC_IOCTL_PROGRESS_DELAY_ELAPSED:     name = AVS_TOKEN_PROGRESSREPORTDELAYELAPSED;    break;
        case AVSC_IOCTL_PROGRESS_INTERVAL_ELAPSED:  name = AVS_TOKEN_PROGRESSREPORTINTERVALELAPSED; break;
        case AVSC_IOCTL_PLAYBACK_STUTTER_STARTED:   name = AVS_TOKEN_PLAYBACKSTUTTERSTARTED;        break;
        case AVSC_IOCTL_PLAYBACK_STUTTER_FINISHED:  name = AVS_TOKEN_PLAYBACKSTUTTERFINISHED;       break;
        case AVSC_IOCTL_PLAYBACK_FINISHED:          name = AVS_TOKEN_PLAYBACKFINISHED;              break;
        case AVSC_IOCTL_PLAYBACK_FAILED:            name = AVS_TOKEN_PLAYBACKFAILED;                break;
        case AVSC_IOCTL_PLAYBACK_STOPPED:           name = AVS_TOKEN_PLAYBACKSTOPPED;               break;
        case AVSC_IOCTL_PLAYBACK_PAUSED:            name = AVS_TOKEN_PLAYBACKPAUSED;                break;
        case AVSC_IOCTL_PLAYBACK_RESUMED:           name = AVS_TOKEN_PLAYBACKRESUMED;               break;
        default:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Bad playback event %d\n", type);
            return NULL;
    }

    /*
     * We need to create the JSON tree for the playback event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/audioplayer for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_AUDIOPLAYER, name, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    cJSON_AddStringToObject(payload, AVS_TOKEN_TOKEN, pb_event->token);

    if (type != AVSC_IOCTL_PLAYBACK_FAILED)
    {
        cJSON_AddNumberToObject(payload, AVS_TOKEN_OFFSET_IN_MS, pb_event->offset_ms);

        if (type == AVSC_IOCTL_PLAYBACK_STUTTER_FINISHED)
        {
            cJSON_AddNumberToObject(payload, AVS_TOKEN_STUTTERDURATION_IN_MS, pb_event->duration_ms);
        }
    }
    else
    {
        state = cJSON_CreateObject();
        error = cJSON_CreateObject();
        if (state == NULL || error == NULL)
        {
            goto _bail;
        }
        cJSON_AddItemToObject(payload, AVS_TOKEN_CURRENTPLAYBACKSTATE, state);
        cJSON_AddItemToObject(payload, AVS_TOKEN_ERROR, error);

        cJSON_AddStringToObject(state, AVS_TOKEN_TOKEN, pb_event->token);
        cJSON_AddNumberToObject(state, AVS_TOKEN_OFFSET_IN_MS, pb_event->offset_ms);
        cJSON_AddStringToObject(state, AVS_TOKEN_PLAYERACTIVITY, playback_state_names[pb_event->state]);

        cJSON_AddStringToObject(error, AVS_TOKEN_TYPE, error_type_names[pb_event->error]);
        cJSON_AddStringToObject(error, AVS_TOKEN_MESSAGE, pb_event->error_msg);
    }

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

  _bail:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    return NULL;
}


char* avsc_create_playback_queue_cleared_event_json_text(avs_client_t* client)
{
    cJSON* root;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    char* JSON_text;

    /*
     * We need to create the JSON tree for the playback queue cleared event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/audioplayer for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_AUDIOPLAYER, AVS_TOKEN_PLAYBACKQUEUECLEARED, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

  _bail:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    return NULL;
}


char* avsc_create_alert_event_json_text(avs_client_t* client, AVSC_IOCTL_T type, char* token)
{
    cJSON* root;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    char* JSON_text;
    char* name;

    switch (type)
    {
        case AVSC_IOCTL_SET_ALERT_SUCCEEDED:        name = AVS_TOKEN_SETALERTSUCCEEDED;             break;
        case AVSC_IOCTL_SET_ALERT_FAILED:           name = AVS_TOKEN_SETALERTFAILED;                break;
        case AVSC_IOCTL_DELETE_ALERT_SUCCEEDED:     name = AVS_TOKEN_DELETEALERTSUCCEEDED;          break;
        case AVSC_IOCTL_DELETE_ALERT_FAILED:        name = AVS_TOKEN_DELETEALERTFAILED;             break;
        case AVSC_IOCTL_ALERT_STARTED:              name = AVS_TOKEN_ALERTSTARTED;                  break;
        case AVSC_IOCTL_ALERT_STOPPED:              name = AVS_TOKEN_ALERTSTOPPED;                  break;
        case AVSC_IOCTL_ALERT_FOREGROUND:           name = AVS_TOKEN_ALERTENTEREDFOREGROUND;        break;
        case AVSC_IOCTL_ALERT_BACKGROUND:           name = AVS_TOKEN_ALERTENTEREDBACKGROUND;        break;
        default:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Bad alert event %d\n", type);
            return NULL;
    }

    /*
     * We need to create the JSON tree for the alert event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/alerts for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_ALERTS, name, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    cJSON_AddStringToObject(payload, AVS_TOKEN_TOKEN, token);

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

  _bail:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    return NULL;
}


char* avsc_create_inactivity_report_event_json_text(avs_client_t* client, uint32_t seconds)
{
    cJSON* root;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    char* JSON_text;

    /*
     * We need to create the JSON tree for the system event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_SYSTEM, AVS_TOKEN_USERINACTIVITYREPORT, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    cJSON_AddNumberToObject(payload, AVS_TOKEN_INACTIVETIME, seconds);

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

    _bail:
      if (root != NULL)
      {
          cJSON_Delete(root);
      }
      return NULL;
}


char* avsc_create_playback_controller_event_json_text(avs_client_t* client, AVSC_IOCTL_T type)
{
    cJSON* root;
    cJSON* context;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    char* JSON_text;
    char* name;

    switch (type)
    {
        case AVSC_IOCTL_PLAY_COMMAND_ISSUED:        name = AVS_TOKEN_PLAYCOMMANDISSUED;          break;
        case AVSC_IOCTL_PAUSE_COMMAND_ISSUED:       name = AVS_TOKEN_PAUSECOMMANDISSUED;         break;
        case AVSC_IOCTL_NEXT_COMMAND_ISSUED:        name = AVS_TOKEN_NEXTCOMMANDISSUED;          break;
        case AVSC_IOCTL_PREVIOUS_COMMAND_ISSUED:    name = AVS_TOKEN_PREVIOUSCOMMANDISSUED;      break;
        default:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Bad playback controller event %d\n", type);
            return NULL;
    }

    /*
     * We need to create the JSON tree for the playback controller event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/playbackcontroller for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * We need to include the status context for the playback controller events.
     */

    context = avsc_create_context_json(client);
    if (context == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_CONTEXT, context);

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_PLAYBACKCONTROLLER, name, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

  _bail:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    return NULL;
}


char* avsc_create_speaker_event_json_text(avs_client_t* client, AVSC_IOCTL_T type, uint32_t volume, wiced_bool_t muted)
{
    cJSON* root;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    char* JSON_text;
    char* name;

    switch (type)
    {
        case AVSC_IOCTL_VOLUME_CHANGED:     name = AVS_TOKEN_VOLUMECHANGED;     break;
        case AVSC_IOCTL_MUTE_CHANGED:       name = AVS_TOKEN_MUTECHANGED;       break;
        default:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Bad speaker event %d\n", type);
            return NULL;
    }

    /*
     * We need to create the JSON tree for the speaker event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speaker for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_SPEAKER, name, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    /*
     * Add in the volume and mute fields.
     */

    cJSON_AddNumberToObject(payload, AVS_TOKEN_VOLUME, volume);
    cJSON_AddBoolToObject(payload, AVS_TOKEN_MUTED, muted);

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

  _bail:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    return NULL;
}


char* avsc_create_settings_updated_event_json_text(avs_client_t* client, AVS_CLIENT_LOCALE_T locale)
{
    cJSON* root;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    cJSON* settings;
    cJSON* locale_root;
    char* name;
    char* JSON_text;

    switch (locale)
    {
        case AVS_CLIENT_LOCALE_EN_US:   name = AVS_TOKEN_EN_US;     break;
        case AVS_CLIENT_LOCALE_EN_GB:   name = AVS_TOKEN_EN_GB;     break;
        case AVS_CLIENT_LOCALE_DE_DE:   name = AVS_TOKEN_DE_DE;     break;
        default:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Bad locale %d\n", locale);
            return NULL;
    }

    /*
     * We need to create the JSON tree for the settings updated event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/settings for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_SETTINGS, AVS_TOKEN_SETTINGSUPDATED, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    /*
     * Create the settings array.
     */

    settings = cJSON_CreateArray();
    if (settings == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(payload, AVS_TOKEN_SETTINGSARRAY, settings);

    locale_root = cJSON_CreateObject();
    if (locale_root == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToArray(settings, locale_root);

    cJSON_AddStringToObject(locale_root, AVS_TOKEN_KEY,   AVS_TOKEN_LOCALE);
    cJSON_AddStringToObject(locale_root, AVS_TOKEN_VALUE, name);

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

  _bail:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    return NULL;
}


char* avsc_create_exception_encountered_event_json_text(avs_client_t* client, avsc_exception_event_t* exception)
{
    cJSON* root;
    cJSON* context;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    cJSON* error;
    char* type;
    char* JSON_text;

    /*
     * We need to create the JSON tree for the system exception encountered event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    context = avsc_create_context_json(client);
    if (context == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_CONTEXT, context);

    /*
     * Create the event node.
     */

    event = avsc_create_header_and_payload_object(client, AVS_TOKEN_SYSTEM, AVS_TOKEN_EXCEPTIONENCOUNTERED, &header, &payload, WICED_TRUE);
    if (event == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event);

    /*
     * Create the error node.
     */

    error = cJSON_CreateObject();
    if (error == NULL)
    {
        goto _bail;
    }
    cJSON_AddItemToObject(payload, AVS_TOKEN_ERROR, error);

    /*
     * Now add in the exception information.
     */

    if (exception->error_type == AVS_ERROR_UNEXPECTED_INFORMATION)
    {
        type = AVS_TOKEN_UNEXPECTED_INFORMATION;
    }
    else if (exception->error_type == AVS_ERROR_UNSUPPORTED)
    {
        type = AVS_TOKEN_UNSUPPORTED_OPERATION;
    }
    else
    {
        type = AVS_TOKEN_INTERNAL_ERROR;
    }
    cJSON_AddStringToObject(payload, AVS_TOKEN_UNPARSEDDIRECTIVE, exception->directive);
    cJSON_AddStringToObject(error, AVS_TOKEN_TYPE, type);
    cJSON_AddStringToObject(error, AVS_TOKEN_MESSAGE, exception->error_message);

    /*
     * Create the JSON text for sending to AVS. The caller is responsible for freeing the text when done.
     */

    JSON_text = cJSON_PrintUnformatted(root);

    /*
     * Don't forget to free the JSON tree now that we are done with it.
     */

    cJSON_Delete(root);

    return JSON_text;

  _bail:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    return NULL;
}
