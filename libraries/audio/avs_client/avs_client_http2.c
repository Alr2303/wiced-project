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
 * AVS Client HTTP2 Routines
 *
 */

#include <ctype.h>

#include "wiced_log.h"

#include "dns.h"
#include "cJSON.h"

#include "http2.h"

#include "avs_client.h"
#include "avs_client_private.h"
#include "avs_client_tokens.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HTTP_HEADER_FIELD_STATUS                ":status"
#define HTTP_HEADER_FIELD_CONTENT_TYPE          "content-type"

#define AVS_DIRECTIVES_PATH     "/"AVS_API_VERSION"/directives"
#define AVS_EVENTS_PATH         "/"AVS_API_VERSION"/events"

#define AVS_BOUNDARY_TERM       "####Boundary-3769-2bad-4e82####"
#define AVS_HEADER_CONTENT_TYPE "multipart/form-data; boundary="AVS_BOUNDARY_TERM

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef enum
{
    AVS_PATH_EVENTS,
    AVS_PATH_DIRECTIVES
} AVS_PATH_T;

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

static wiced_result_t http2_disconnect_event_callback(http_connection_ptr_t connect, http_request_id_t last_processed_stream_id, uint32_t error, void* user_data)
{
    avs_client_t* client = (avs_client_t*)user_data;

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Disconnect event\n");

    if (client == NULL || client->tag != AVSC_TAG_VALID)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "http_disconnect_event_callback: Bad app structure\n");
        return WICED_ERROR;
    }

    client->response_discard = WICED_TRUE;
    wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_DISCONNECTED);

    return WICED_SUCCESS;
}


static wiced_result_t http2_recv_header_callback(http_connection_ptr_t connect, http_request_id_t stream_id, http_header_info_t* headers, http_frame_type_t type, uint8_t flags, void* user_data)
{
    avs_client_t* client = (avs_client_t*)user_data;
    WICED_LOG_LEVEL_T level;
    avsc_msg_t msg;
    uint32_t status;
    char* msg_boundary;
    char* ptr;
    char* end;
    int i;

    if (client == NULL || client->tag != AVSC_TAG_VALID)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "http_recv_header_callback: Bad app structure\n");
        return WICED_ERROR;
    }

    if (type == HTTP_FRAME_TYPE_PUSH_PROMISE && (flags & HTTP_REQUEST_FLAGS_PUSH_PROMISE))
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Received a Push Promise request for stream %lu\n", stream_id);
        return WICED_SUCCESS;
    }
    else if (type == HTTP_FRAME_TYPE_PING && (flags & HTTP_REQUEST_FLAGS_PING_RESPONSE))
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Received a ping response\n");
        return WICED_SUCCESS;
    }

    level = wiced_log_get_facility_level(WLF_AVS_CLIENT);

    while (headers)
    {
        if (headers->name_length > 0)
        {
            if (level >= WICED_LOG_DEBUG0)
            {
                wiced_log_printf("%.*s : ", headers->name_length, headers->name);
                wiced_log_printf("%.*s\n", headers->value_length, headers->value);
            }

            if (strncasecmp((char*)headers->name, HTTP_HEADER_FIELD_STATUS, strlen(HTTP_HEADER_FIELD_STATUS)) == 0)
            {
                /*
                 * The status value will be three characters but is not nul terminated. We need to make sure that
                 * we don't pick up any unintended characters from the buffer when we call atoi.
                 */

                for (i = 0, status = 0; isdigit((int)headers->value[i]); i++)
                {
                    status = status * 10 + headers->value[i] - '0';
                }

                /*
                 * Does this stream have content that we'll need to parse?
                 */

                if (status == HTTP_STATUS_OK)
                {
                    /*
                     * The OK status (200) indicates that AVS will be passing data back to us
                     * for this stream.
                     */

                    if (client->http2_response_stream_id == 0 && client->response_ridx == 0 && client->response_widx == 0)
                    {
                        /*
                         * We are good to receive data for this stream.
                         */

                        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Setting response stream_id %lu\n", stream_id);
                        client->http2_response_stream_id = stream_id;
                    }
                    else
                    {
                        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Potential response match while capture active: stream_id %lu, response_id %lu (%lu,%lu)\n",
                                      stream_id, client->http2_response_stream_id, client->response_ridx, client->response_widx);
                    }
                }

                /*
                 * Pass the response to the main loop.
                 */

                msg.type = AVSC_MSG_TYPE_HTTP_RESPONSE;
                msg.arg1 = stream_id;
                msg.arg2 = status;
                if (wiced_rtos_push_to_queue(&client->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
                {
                    wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_MSG_QUEUE);
                }
                else
                {
                    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error pushing http response message\n");
                }
            }
            else if ((stream_id == client->http2_downchannel_stream_id || stream_id == client->http2_response_stream_id) &&
                 strncasecmp((char*)headers->name, HTTP_HEADER_FIELD_CONTENT_TYPE, strlen(HTTP_HEADER_FIELD_CONTENT_TYPE)) == 0)
            {
                /*
                 * We need to pull out and save the boundary separator.
                 * Replace the last character of the value string with a nul. Since we have to process it we
                 * want it to be a nul terminated string and we don't care about the last character.
                 */

                headers->value[headers->value_length] = '\0';
                ptr = strstr((char*)headers->value, "boundary=");
                if (ptr != NULL)
                {
                    ptr += 9;
                    if ((end = strchr(ptr, ';')) != NULL)
                    {
                        *end = '\0';
                    }

                    if ((msg_boundary = strdup(ptr)) == NULL)
                    {
                        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to allocate boundary string\n");
                        return WICED_ERROR;
                    }

                    if (stream_id == client->http2_downchannel_stream_id)
                    {
                        if (client->msg_boundary_downchannel != NULL)
                        {
                            free(client->msg_boundary_downchannel);
                        }
                        client->msg_boundary_downchannel = msg_boundary;
                    }
                    else
                    {
                        if (client->msg_boundary_response != NULL)
                        {
                            free(client->msg_boundary_response);
                        }
                        client->msg_boundary_response = msg_boundary;
                        client->boundary_len          = strlen(msg_boundary);
                    }
                    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Msg boundary for stream %lu is %s\n", stream_id, msg_boundary);
                }
            }
        }
        headers = headers->next;
    }

    if (flags & HTTP_REQUEST_FLAGS_FINISH_REQUEST)
    {
        /*
         * This signals that this is the last HEADERS frame that will be sent for the stream.
         */

        if (stream_id == client->http2_response_stream_id)
        {
            client->http2_response_stream_id = 0;
        }
    }

    return WICED_SUCCESS;
}


static wiced_result_t http2_recv_data_callback(http_connection_ptr_t connect, http_request_id_t stream_id, uint8_t* data, uint32_t length, uint8_t flags, void* user_data)
{
    avs_client_t* client = (avs_client_t*)user_data;
    wiced_result_t result;
    uint32_t bytes_copied;
    uint32_t avail;
    uint32_t bytes;
    char* ptr;
    static int throttle_messages = 0;
    int len;
    int i;

    if (client == NULL || client->tag != AVSC_TAG_VALID)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "http_recv_data_callback: Bad app structure\n");
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Received %lu data bytes for stream %lu, flags 0x%02x (%lu)\n", length, stream_id, flags, client->http2_response_stream_id);

    if (stream_id != client->http2_downchannel_stream_id && client->http2_response_stream_id != 0 && client->http2_response_stream_id != stream_id)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Data received when response capture active (stream id %u, rsid %lu)\n",
                      (unsigned int)stream_id, client->http2_response_stream_id);
    }

    if (client->http2_binary_data_stream_id != stream_id)
    {
        /*
         * We always search for the binary portion of a stream. Otherwise if the log level is changed
         * when receiving a long response we can start trying to log binary data.
         */

        if ((ptr = strstr((char*)data, "Content-Type: application/octet-stream\r\n")) != NULL)
        {
            ptr += 40;
            len  = (int)(ptr - (char*)data);

            /*
             * Note that we're getting to a binary data portion of the response so that we don't
             * accidently try and log the data as text.
             */

            client->http2_binary_data_stream_id = stream_id;
        }
        else
        {
            len = length;
        }

        /*
         * Only output the data if we're at an appropriate log level.
         */

        if (wiced_log_get_facility_level(WLF_AVS_CLIENT) >= WICED_LOG_DEBUG0)
        {
            for (i = 0; i < len; i++)
            {
                wiced_log_printf("%c", data[i]);
            }
            wiced_log_printf("\n");
        }
    }

    if (stream_id == client->http2_downchannel_stream_id)
    {
        /*
         * Incoming directive. Do we have room to store it?
         */

        result = wiced_rtos_lock_mutex(&client->buffer_mutex);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to grab directive buffer mutex\n");
            return WICED_SUCCESS;
        }

        avail = AVSC_DIRECTIVE_BUF_SIZE - client->directive_widx;

        if (avail >= length)
        {
            memcpy(&client->directive_buf[client->directive_widx], data, length);
            client->directive_widx += length;
            wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_DIRECTIVE_DATA);
        }
        else
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Insufficient room to store directive data (%lu,%lu)\n", avail, length);
        }
        wiced_rtos_unlock_mutex(&client->buffer_mutex);
    }
    else if (stream_id == client->http2_response_stream_id && !client->response_discard)
    {
        bytes_copied = 0;
        while (bytes_copied < length && stream_id == client->http2_response_stream_id && !client->response_discard)
        {
            result = wiced_rtos_lock_mutex(&client->buffer_mutex);
            if (result != WICED_SUCCESS)
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to grab response buffer mutex\n");
                return WICED_SUCCESS;
            }

            if (client->response_widx >= client->response_ridx)
            {
                avail = client->params.response_buffer_size - (client->response_widx - client->response_ridx);
            }
            else
            {
                avail = client->response_ridx - client->response_widx;
            }

            if (avail >= length)
            {
                while (bytes_copied < length)
                {
                    bytes = length - bytes_copied;
                    if (client->response_widx >= client->response_ridx && client->params.response_buffer_size - client->response_widx < bytes)
                    {
                        bytes = client->params.response_buffer_size - client->response_widx;
                    }
                    memcpy(&client->response_buffer[client->response_widx], &data[bytes_copied], bytes);
                    client->response_widx = (client->response_widx + bytes) % client->params.response_buffer_size;
                    bytes_copied += bytes;
                }
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG1, "Copy complete (%lu,%lu)\n", client->response_ridx, client->response_widx);
                wiced_rtos_set_event_flags(&client->events, AVSC_EVENT_RESPONSE_DATA);
            }
            wiced_rtos_unlock_mutex(&client->buffer_mutex);

            if (avail < length)
            {
                /*
                 * We don't have this as an else to the check above since the delay needs
                 * to be outside of the mutex.
                 */

                if (!throttle_messages)
                {
                    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG1, "Insufficient room to store response data (%lu,%lu)\n", avail, length);
                    throttle_messages = 1;
                }
                wiced_rtos_delay_milliseconds(2);
            }
        }
        throttle_messages = 0;
        if (flags & HTTP_REQUEST_FLAGS_FINISH_REQUEST)
        {
            client->response_complete        = WICED_TRUE;
            client->http2_response_stream_id = 0;
        }
    }

    return WICED_SUCCESS;
}


/*
 * NOTE: This routine uses client->tmp_buf which will be referenced by the initialized headers array.
 */

static wiced_result_t avsc_initialize_event_headers(avs_client_t* client, http_header_info_t *headers, AVS_PATH_T path_type)
{
    char* method;
    char* path;

    if (path_type == AVS_PATH_DIRECTIVES)
    {
        method = "GET";
        path   = AVS_DIRECTIVES_PATH;
    }
    else
    {
        method = "POST";
        path   = AVS_EVENTS_PATH;
    }

    snprintf(client->tmp_buf, AVSC_TMP_BUF_SIZE, "Bearer %s", client->access_token);

    headers[0].name         = (uint8_t*)":method";
    headers[0].name_length  = strlen(":method");
    headers[0].value        = (uint8_t*)method;
    headers[0].value_length = strlen(method);
    headers[0].next         = &headers[1];

    headers[1].name         = (uint8_t*)":scheme";
    headers[1].name_length  = strlen(":scheme");
    headers[1].value        = (uint8_t*)"https";
    headers[1].value_length = strlen("https");
    headers[1].next         = &headers[2];

    headers[2].name         = (uint8_t*)":path";
    headers[2].name_length  = strlen(":path");
    headers[2].value        = (uint8_t*)path;
    headers[2].value_length = strlen(path);
    headers[2].next         = &headers[3];

    headers[3].name         = (uint8_t*)"authorization";
    headers[3].name_length  = strlen("authorization");
    headers[3].value        = (uint8_t*)client->tmp_buf;
    headers[3].value_length = strlen(client->tmp_buf);

    if (path_type == AVS_PATH_DIRECTIVES)
    {
        headers[3].next         = NULL;
    }
    else
    {
        headers[3].next         = &headers[4];

        headers[4].name         = (uint8_t*)"content-type";
        headers[4].name_length  = strlen("content-type");
        headers[4].value        = (uint8_t*)AVS_HEADER_CONTENT_TYPE;
        headers[4].value_length = strlen(AVS_HEADER_CONTENT_TYPE);
        headers[4].next         = NULL;
    }

    return WICED_SUCCESS;
}


wiced_result_t avsc_http2_initialize(avs_client_t* client)
{
    http_connection_callbacks_t callbacks;
    wiced_result_t              result;

    /*
     * Initialize the HTTP2 library.
     */

    memset(&client->http2_security, 0, sizeof(http_security_info_t));

    callbacks.http_disconnect_callback          = http2_disconnect_event_callback;
    callbacks.http_request_recv_header_callback = http2_recv_header_callback;
    callbacks.http_request_recv_data_callback   = http2_recv_data_callback;

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Initializing HTTP2 library\n");
    result = http_connection_init(&client->http2_connect, &client->http2_security, &callbacks, client->http2_scratch, sizeof(client->http2_scratch), client);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error initializing HTTP2 library (%d)\n", result);
    }

    return result;
}


wiced_result_t avsc_http2_shutdown(avs_client_t* client)
{
    return http_connection_deinit(&client->http2_connect);
}


wiced_result_t avsc_connect_to_avs(avs_client_t* client)
{
    wiced_ip_address_t  ip_address;
    wiced_result_t      result;

    /*
     * Check to see if we're already connected.
     */

    if (client->state > AVSC_STATE_IDLE)
    {
        return WICED_SUCCESS;
    }

    result = avsc_parse_uri_and_lookup_hostname(client, client->params.base_url, &ip_address);
    if (result != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /*
     * Connect to the server.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Connecting to " IPV4_PRINT_FORMAT "\n", IPV4_SPLIT_TO_PRINT(ip_address));
    result = http_connect(&client->http2_connect, &ip_address, client->port, client->params.interface);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error connecting to " IPV4_PRINT_FORMAT " (%d)\n", IPV4_SPLIT_TO_PRINT(ip_address), result);
        return result;
    }

    client->state = AVSC_STATE_CONNECTED;

    return WICED_SUCCESS;
}


wiced_result_t avsc_send_synchronize_state_message(avs_client_t* client)
{
    http_header_info_t  source_headers[5];
    cJSON*              root;
    char*               json_text;
    http_request_id_t   stream_id;
    wiced_result_t      result;

    /*
     * Start by creating the JSON for the SynchronizeState message.
     */

    root = avsc_create_synchronize_state_json(client);
    if (root == NULL)
    {
        return WICED_ERROR;
    }

    /*
     * Now get the json text and create the body of our synchronize state message.
     */

    json_text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Allocation failure creating SynchronizeState message\n");
        return WICED_ERROR;
    }

    snprintf(client->http_buf, AVSC_HTTP_BUF_SIZE, "--%s\r\n%s\r\n\%s\r\n--%s--\r\n", AVS_BOUNDARY_TERM, AVS_JSON_HEADERS, json_text, AVS_BOUNDARY_TERM);

    if (wiced_log_get_facility_level(WLF_AVS_CLIENT) >= WICED_LOG_DEBUG0)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "SynchronizeState message:\n");
        wiced_log_printf("%s\n", client->http_buf);
    }
    free(json_text);

    avsc_initialize_event_headers(client, source_headers, AVS_PATH_EVENTS);

    result = http_request_create(&client->http2_connect, &stream_id);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating synchronize state request\n");
        return result;
    }

    result = http_request_put_headers(&client->http2_connect, stream_id, source_headers, HTTP_REQUEST_FLAGS_FINISH_HEADERS);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error sending synchronize state headers\n");
        return result;
    }

    result = http_request_put_data(&client->http2_connect, stream_id, (uint8_t*)client->http_buf, strlen(client->http_buf), HTTP_REQUEST_FLAGS_FINISH_REQUEST);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error sending synchronize state data\n");
        return result;
    }

    client->state = AVSC_STATE_SYNCHRONIZE;

    return WICED_SUCCESS;
}


wiced_result_t avsc_send_downchannel_message(avs_client_t* client)
{
    http_header_info_t  source_headers[4];
    http_request_id_t   stream_id;
    uint8_t             flags;
    wiced_result_t      result;

    avsc_initialize_event_headers(client, source_headers, AVS_PATH_DIRECTIVES);

    result = http_request_create(&client->http2_connect, &stream_id);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating downchannel request\n");
        return result;
    }

    flags  = HTTP_REQUEST_FLAGS_FINISH_HEADERS | HTTP_REQUEST_FLAGS_FINISH_REQUEST;
    result = http_request_put_headers(&client->http2_connect, stream_id, source_headers, flags);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error sending downchannel message\n");
        return result;
    }

    client->http2_downchannel_stream_id = stream_id;
    client->state                       = AVSC_STATE_DOWNCHANNEL;

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Finished sending downchannel message.\n");

    return WICED_SUCCESS;
}


wiced_result_t avsc_finish_recognize_message(avs_client_t* client)
{
    wiced_result_t result = WICED_SUCCESS;

    if (client->http2_voice_stream_id)
    {
        snprintf(client->http_buf, AVSC_HTTP_BUF_SIZE, "--%s--\r\n", AVS_BOUNDARY_TERM);
        result = http_request_put_data(&client->http2_connect, client->http2_voice_stream_id, (uint8_t*)client->http_buf,
                                       strlen(client->http_buf), HTTP_REQUEST_FLAGS_FINISH_REQUEST);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error completing SpeechRecognizer request\n");
        }
    }
    else
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Finish recognize called when no voice stream\n");
    }

    client->http2_voice_stream_id = 0;

    return WICED_SUCCESS;
}


wiced_result_t avsc_send_recognize_message(avs_client_t* client)
{
    http_header_info_t  source_headers[5];
    cJSON*              root;
    char*               json_text;
    http_request_id_t   stream_id;
    wiced_result_t      result;

    /*
     * Start by creating the JSON for the recognize message.
     */

    root = avsc_create_recognize_json(client);
    if (root == NULL)
    {
        return WICED_ERROR;
    }

    /*
     * Now get the json text and create the body of our recognize message.
     */

    json_text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Allocation failure creating recognize message\n");
        return WICED_ERROR;
    }

    snprintf(client->http_buf, AVSC_HTTP_BUF_SIZE, "--%s\r\n%s\r\n\%s\r\n--%s\r\n%s\r\n", AVS_BOUNDARY_TERM, AVS_JSON_HEADERS, json_text, AVS_BOUNDARY_TERM, AVS_AUDIO_HEADERS);
    free(json_text);

    avsc_initialize_event_headers(client, source_headers, AVS_PATH_EVENTS);

    result = http_request_create(&client->http2_connect, &stream_id);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating recognize request\n");
        return result;
    }

    if (wiced_log_get_facility_level(WLF_AVS_CLIENT) >= WICED_LOG_DEBUG0)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Recognize message on stream %lu:\n", stream_id);
        wiced_log_printf("%s\n", client->http_buf);
        wiced_log_printf("\n");
    }

    result = http_request_put_headers(&client->http2_connect, stream_id, source_headers, HTTP_REQUEST_FLAGS_FINISH_HEADERS);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error sending recognize headers\n");
        return result;
    }

    result = http_request_put_data(&client->http2_connect, stream_id, (uint8_t*)client->http_buf, strlen(client->http_buf), 0);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error sending recognize data\n");
        return result;
    }

    client->http2_voice_stream_id = stream_id;
    client->state                 = AVSC_STATE_SPEECH_REQUEST;

    return WICED_SUCCESS;
}


static wiced_result_t avsc_send_event_message(avs_client_t* client, char* json_text, wiced_bool_t free_text)
{
    http_header_info_t source_headers[5];
    http_request_id_t  stream_id;
    wiced_result_t     result = WICED_SUCCESS;

    /*
     * Construct the full message in the HTTP buffer.
     */

    snprintf(client->http_buf, AVSC_HTTP_BUF_SIZE, "--%s\r\n%s\r\n\%s\r\n--%s--\r\n", AVS_BOUNDARY_TERM, AVS_JSON_HEADERS, json_text, AVS_BOUNDARY_TERM);
    if (free_text)
    {
        free(json_text);
    }

    /*
     * Create the headers for the message and send them off.
     */

    avsc_initialize_event_headers(client, source_headers, AVS_PATH_EVENTS);

    result = http_request_create(&client->http2_connect, &stream_id);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating event request (%d)\n", result);
        return result;
    }

    if (wiced_log_get_facility_level(WLF_AVS_CLIENT) >= WICED_LOG_DEBUG0)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending event message in stream %lu:\n", stream_id);
        wiced_log_printf("%s\n", client->http_buf);
    }

    result = http_request_put_headers(&client->http2_connect, stream_id, source_headers, HTTP_REQUEST_FLAGS_FINISH_HEADERS);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error sending event headers\n");
        return result;
    }

    /*
     * Now the body of the message.
     */

    result = http_request_put_data(&client->http2_connect, stream_id, (uint8_t*)client->http_buf, strlen(client->http_buf), HTTP_REQUEST_FLAGS_FINISH_REQUEST);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error sending event data\n");
    }

    return result;
}


wiced_result_t avsc_send_speech_event(avs_client_t* client, char* token, wiced_bool_t speech_started)
{
    wiced_time_t now;
    char* json_text;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending speech event\n");
    json_text = avsc_create_speech_event_json_text(client, speech_started ? AVS_TOKEN_SPEECHSTARTED : AVS_TOKEN_SPEECHFINISHED, token);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating speech event json\n");
        return WICED_ERROR;
    }

    /*
     * Remember the current state.
     */

    if (speech_started)
    {
        client->speech_state.state = AVS_PLAYBACK_STATE_PLAYING;
        wiced_time_get_time(&client->speech_start);
        client->speech_time_ms = 0;
    }
    else
    {
        client->speech_state.state = AVS_PLAYBACK_STATE_FINISHED;
        wiced_time_get_time(&now);
        client->speech_time_ms = now - client->speech_start;
    }
    strlcpy(client->speech_state.token, token, AVS_MAX_TOKEN_LEN);

    /*
     * Send off the event.
     */

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}


wiced_result_t avsc_send_speech_timed_out_event(avs_client_t* client)
{
    char* json_text;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Sending speech timed out event\n");
    json_text = avsc_create_speech_timed_out_event_json_text(client);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating speech timed out event json\n");
        return WICED_ERROR;
    }

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}


wiced_result_t avsc_send_playback_event(avs_client_t* client, AVSC_IOCTL_T type, avsc_playback_event_t* pb_event)
{
    char* json_text;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending playback event\n");
    json_text = avsc_create_playback_event_json_text(client, type, pb_event);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating playback event json\n");
        return WICED_ERROR;
    }

    /*
     * We need to remember the current playback state.
     */

    switch (type)
    {
        case AVSC_IOCTL_PLAYBACK_STARTED:           client->playback_state.state = AVS_PLAYBACK_STATE_PLAYING;          break;
        case AVSC_IOCTL_PLAYBACK_NEARLY_FINISHED:   client->playback_state.state = AVS_PLAYBACK_STATE_PLAYING;          break;
        case AVSC_IOCTL_PROGRESS_DELAY_ELAPSED:     client->playback_state.state = AVS_PLAYBACK_STATE_PLAYING;          break;
        case AVSC_IOCTL_PROGRESS_INTERVAL_ELAPSED:  client->playback_state.state = AVS_PLAYBACK_STATE_PLAYING;          break;
        case AVSC_IOCTL_PLAYBACK_STUTTER_STARTED:   client->playback_state.state = AVS_PLAYBACK_STATE_BUFFER_UNDERRUN;  break;
        case AVSC_IOCTL_PLAYBACK_STUTTER_FINISHED:  client->playback_state.state = AVS_PLAYBACK_STATE_PLAYING;          break;
        case AVSC_IOCTL_PLAYBACK_FINISHED:          client->playback_state.state = AVS_PLAYBACK_STATE_FINISHED;         break;
        case AVSC_IOCTL_PLAYBACK_FAILED:            client->playback_state.state = AVS_PLAYBACK_STATE_STOPPED;          break;
        case AVSC_IOCTL_PLAYBACK_STOPPED:           client->playback_state.state = AVS_PLAYBACK_STATE_STOPPED;          break;
        case AVSC_IOCTL_PLAYBACK_PAUSED:            client->playback_state.state = AVS_PLAYBACK_STATE_PAUSED;           break;
        case AVSC_IOCTL_PLAYBACK_RESUMED:           client->playback_state.state = AVS_PLAYBACK_STATE_PLAYING;          break;
        default:
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Bad playback type %d\n", type);
            free(json_text);
            return WICED_ERROR;
    }

    client->playback_state.offset_ms = pb_event->offset_ms;
    strlcpy(client->playback_state.token, pb_event->token, AVS_MAX_TOKEN_LEN);

    /*
     * And finally send the event off.
     */

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}


wiced_result_t avsc_send_playback_queue_cleared_event(avs_client_t* client)
{
    char* json_text;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending playback queue cleared event\n");
    json_text = avsc_create_playback_queue_cleared_event_json_text(client);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating playback queue cleared event json\n");
        return WICED_ERROR;
    }

    /*
     * Send the event off.
     */

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}


wiced_result_t avsc_send_alert_event(avs_client_t* client, AVSC_IOCTL_T type, char* token)
{
    char* json_text;
    avs_client_alert_t* alert;
    avs_client_alert_t* prev;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending alert event\n");
    json_text = avsc_create_alert_event_json_text(client, type, token);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating alert event json\n");
        return WICED_ERROR;
    }

    /*
     * Find this alert in our list.
     */

    prev  = NULL;
    alert = client->alert_list;
    while (alert != NULL)
    {
        if (strcmp(alert->token, token) == 0)
        {
            break;
        }
        prev  = alert;
        alert = alert->next;
    }

    if (alert)
    {
        if (type == AVSC_IOCTL_SET_ALERT_FAILED || type == AVSC_IOCTL_DELETE_ALERT_SUCCEEDED || type == AVSC_IOCTL_ALERT_STOPPED)
        {
            if (prev)
            {
                prev->next = alert->next;
            }
            else
            {
                client->alert_list = alert->next;
            }

            free(alert);
        }
        else if (type == AVSC_IOCTL_ALERT_STARTED)
        {
            alert->active = WICED_TRUE;
        }
    }
    else
    {
        /*
         * This isn't an error condition since alert may have different combinations
         * of stopped/deleted events depending on whether alerts are stopped locally
         * or via the Alexa app. Just enabling logging of the condition.
         */

        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Alert %s not found\n", token);
    }

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}


wiced_result_t avsc_send_inactivity_report_event(avs_client_t* client, uint32_t seconds)
{
    char* json_text;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending inactivity report event\n");
    json_text = avsc_create_inactivity_report_event_json_text(client, seconds);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating inactivity report event json\n");
        return WICED_ERROR;
    }

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}


wiced_result_t avsc_send_playback_controller_event(avs_client_t* client, AVSC_IOCTL_T type)
{
    char* json_text;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending playback controller event\n");
    json_text = avsc_create_playback_controller_event_json_text(client, type);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating playback controller event json\n");
        return WICED_ERROR;
    }

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}


wiced_result_t avsc_send_speaker_event(avs_client_t* client, AVSC_IOCTL_T type, uint32_t volume, wiced_bool_t muted)
{
    char* json_text;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending speaker event\n");
    json_text = avsc_create_speaker_event_json_text(client, type, volume, muted);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating speaker event json\n");
        return WICED_ERROR;
    }

    /*
     * Store the current volume information.
     */

    client->volume_state.volume = volume;
    client->volume_state.muted  = muted;

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}


wiced_result_t avsc_send_settings_updated_event(avs_client_t* client, AVS_CLIENT_LOCALE_T locale)
{
    char* json_text;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending settings updated event\n");
    json_text = avsc_create_settings_updated_event_json_text(client, locale);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating settings updated event json\n");
        return WICED_ERROR;
    }

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}


wiced_result_t avsc_send_exception_encountered_event(avs_client_t* client, avsc_exception_event_t* exception)
{
    char* json_text;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending exception encountered event\n");
    json_text = avsc_create_exception_encountered_event_json_text(client, exception);
    if (json_text == NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error creating exception encountered event json\n");
        return WICED_ERROR;
    }

    return avsc_send_event_message(client, json_text, WICED_TRUE);
}
