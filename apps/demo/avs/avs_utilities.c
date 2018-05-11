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
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "wiced.h"
#include "avs.h"
#include "avs_utilities.h"

#include "cJSON.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AVS_MIME_TYPE_JSON      "application/json"

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/**
 * urlencode all non-alphanumeric characters in the C-string 'src'
 * store result in the C-string 'dest'
 *
 * return the length of the url encoded C-string
 */

int avs_urlencode(char* dest, const char* src)
{
    char* d;
    int i;

    for (i = 0, d = dest; src[i]; i++)
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

    return (int)(d - dest);
}


/**
 * Package up and send data to the main thread in via message buffers.
 *
 * @param avs       : Pointer to the main application structure
 * @param msg_type  : Message type to send
 * @param stream_id : HTTP2 stream id for the data
 * @param data      : Pointer to the data to send
 * @param length    : Length of the data to send
 *
 * @return Status of the operation.
 */

wiced_result_t avs_send_msg_buffers(avs_t* avs, AVS_MSG_TYPE_T msg_type, uint32_t stream_id, uint8_t* data, uint32_t length)
{
    avs_msg_t msg;
    objhandle_t handle;
    char* ptr;
    uint32_t len;
    uint32_t total_bytes;
    uint32_t bufsize;
    uint32_t avail_size;
    uint32_t cur_size;
    wiced_result_t result = WICED_SUCCESS;

    /*
     * A note about the message traffic we see returned from AVS servers. When returning a large
     * data stream (i.e. audio speech synthesizer directive), we often see a larger (16K) packet
     * followed by a bunch of small (10-50 byte) packets. To avoid running out of buffers by using
     * them to transfer small amounts of data, we will consolidate speech synthesizer data and
     * only send the buffer to the main thread when it is full or the data ends (signified by
     * a NULL or zero length buffer).
     */

    msg.type = msg_type;
    msg.arg1 = stream_id;

    if (data == NULL || length == 0)
    {
        if (avs->current_handle != NULL)
        {
            msg.arg2 = (uint32_t)avs->current_handle;
            if (wiced_rtos_push_to_queue(&avs->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) != WICED_SUCCESS)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error pushing %d event\n", msg_type);
                bufmgr_buf_free(avs->current_handle);
                result = WICED_ERROR;
            }
            avs->current_handle = NULL;
        }

        /*
         * Send a empty message to signal the end of the data.
         */

        msg.arg2 = 0;
        if (wiced_rtos_push_to_queue(&avs->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
        {
            wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_MSG_QUEUE);
        }
        else
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error pushing %d event\n", msg_type);
            result = WICED_ERROR;
        }

        return result;
    }

    bufsize = bufmgr_pool_get_bufsize(avs->buf_pool);
    total_bytes = 0;
    while (total_bytes < length)
    {
        if (msg_type == AVS_MSG_TYPE_SPEECH_SYNTHESIZER && avs->current_handle)
        {
            handle = avs->current_handle;
            bufmgr_buf_get_data(handle, &ptr, &cur_size);
            avail_size = bufsize - cur_size;
        }
        else
        {
            cur_size = 0;
            if (bufmgr_pool_alloc_buf(avs->buf_pool, &handle, (char **)&ptr, &avail_size, BUF_ALLOC_TIMEOUT_MS) != BUFMGR_OK)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG1, "Unable to allocate buffer from pool...try again\n");
                continue;
            }
        }

        if (avail_size > length - total_bytes)
        {
            len = length - total_bytes;
        }
        else
        {
            len = avail_size;
        }
        memcpy(&ptr[cur_size], &data[total_bytes], len);
        bufmgr_buf_set_size(handle, cur_size + len);

        if (msg_type == AVS_MSG_TYPE_SPEECH_SYNTHESIZER && cur_size + len < bufsize)
        {
            avs->current_handle = handle;
        }
        else
        {
            msg.arg2 = (uint32_t)handle;
            if (wiced_rtos_push_to_queue(&avs->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error pushing %d event\n", msg_type);
                bufmgr_buf_free(handle);
                result = WICED_ERROR;
            }
            avs->current_handle = NULL;
        }
        total_bytes += len;
    }

    return result;
}


/**
 * Find the start of the JSON text in the message buffer.
 *
 * @param buf           : Pointer to the buffer data
 * @param buflen        : Length of the buffer data
 * @param msg_boundary  : Pointer to the message boundary text
 *
 * @return Pointer to the start of the JSON text or NULL
 */

char* avs_find_JSON_start(char* buf, uint32_t buflen, char* msg_boundary)
{
    char* ptr;
    char* json;
    char* end;
    int len;

    /*
     * And isolate the JSON portion of the message.
     */

    ptr = strnstr(buf, buflen, msg_boundary, strlen(msg_boundary));
    if (ptr == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to find message boundary in directive\n");
        return NULL;
    }

    /*
     * On subsequent directives, AVS doesn't always start with the message boundary separator.
     * I can only guess this is because the previous directive ended with the message boundary.
     * So we need to look for a content type of application/json ("Content-Type: application/json").
     */

    len = strlen(AVS_MIME_TYPE_JSON);
    json = strnstr(buf, buflen, AVS_MIME_TYPE_JSON, len);
    if (json == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to find JSON content type in directive\n");
        return NULL;
    }
    json += len;

    /*
     * We stop searching at the message boundary if it follows the JSON mime type or at the end of the data.
     */

    if (ptr < json)
    {
        end = &buf[buflen];
    }
    else
    {
        end = ptr;
    }

    while (*json != '{' && json < end)
    {
        json++;
    }

    if (*json != '{')
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to find JSON start in directive\n");
        return NULL;
    }

    return json;
}


char* avs_create_speech_event_JSON(avs_t* avs, char* name)
{
    cJSON* root;
    cJSON* event;
    cJSON* header;
    cJSON* payload;
    char* JSON_text;
    char buf[20];

    /*
     * We need to create the JSON tree for the speech event.
     * See http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechsynthesizer for format details.
     */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    cJSON_AddItemToObject(root, AVS_TOKEN_EVENT, event = cJSON_CreateObject());
    if (event == NULL)
    {
        goto _bail;
    }

    cJSON_AddItemToObject(event, AVS_TOKEN_HEADER,   header = cJSON_CreateObject());
    cJSON_AddItemToObject(event, AVS_TOKEN_PAYLOAD, payload = cJSON_CreateObject());
    if (header == NULL || payload == NULL)
    {
        goto _bail;
    }

    cJSON_AddStringToObject(header, AVS_TOKEN_NAMESPACE, AVS_TOKEN_SPEECHSYNTHESIZER);
    cJSON_AddStringToObject(header, AVS_TOKEN_NAME, name);
    snprintf(buf, sizeof(buf), AVS_FORMAT_STRING_MID, avs->message_id++);
    cJSON_AddStringToObject(header, AVS_TOKEN_MESSAGEID, buf);

    cJSON_AddStringToObject(payload, AVS_TOKEN_TOKEN, avs->token_speak);

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
