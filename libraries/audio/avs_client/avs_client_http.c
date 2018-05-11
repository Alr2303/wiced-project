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
 * AVS Client HTTP Routines
 *
 */

#include "wiced_tcpip.h"
#include "wiced_tls.h"
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

#define CONNECT_TIMEOUT_MS                      (2 * 1000)
#define RECEIVE_TIMEOUT_MS                      (2 * 1000)

#define HTTP11_HEADER_CONTENT_LENGTH            "Content-Length"
#define HTTP11_HEADER_STATUS_OK                 "HTTP/1.1 200"

#define AVS_AUTHORIZATION_URL           "https://api.amazon.com/auth/o2/token"

#define AVS_AUTHORIZATION_GRANT_WEB_BASED   "grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s\r\n"
#define AVS_AUTHORIZATION_GRANT_APP_BASED   "grant_type=refresh_token&refresh_token=%s&client_id=%s\r\n"

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

static char avs_HTTP_get_refresh_token_template[] =
{
    "POST /auth/o2/token HTTP/1.1\r\n"
    "Host: api.amazon.com\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: %d\r\n"
    "Cache-Control: no-cache\r\n"
    "\r\n"
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_result_t avsc_socket_connect(avs_client_t* client)
{
    wiced_result_t          result;
    wiced_ip_address_t      ip_address;
    wiced_tls_context_t*    tls_context;
    int                     connect_tries;

    result = avsc_parse_uri_and_lookup_hostname(client, AVS_AUTHORIZATION_URL, &ip_address);
    if (result != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_INFO, "Connect to host: %s (" IPV4_PRINT_FORMAT "), port %d, path %s\n",
                  client->hostname, IPV4_SPLIT_TO_PRINT(ip_address), client->port, client->path);

    /*
     * Open a socket for the connection.
     */

    if (client->socket_ptr == NULL)
    {
        result = wiced_tcp_create_socket(&client->socket, client->params.interface);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to create socket (%d)\n", result);
            return result;
        }
        client->socket_ptr = &client->socket;
    }

    if (client->tls == WICED_TRUE)
    {
        /*
         * Allocate the TLS context and associate with our socket.
         * If we set the socket context_malloced flag to true, the TLS
         * context will be freed automatically when the socket is deleted.
         */

        tls_context = malloc(sizeof(wiced_tls_context_t));
        if (tls_context == NULL)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to allocate tls context\n");
            wiced_tcp_delete_socket(client->socket_ptr);
            client->socket_ptr = NULL;
            return result;
        }
        wiced_tls_init_context(tls_context, NULL, NULL);
        wiced_tcp_enable_tls(client->socket_ptr, tls_context);
        client->socket_ptr->context_malloced = WICED_TRUE;
    }

    /*
     * Now try and connect.
     */

    connect_tries = 0;
    result = WICED_ERROR;
    while (connect_tries < 3 && result == WICED_ERROR)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Connection attempt %d\n", connect_tries);
        result = wiced_tcp_connect(client->socket_ptr, &ip_address, client->port, CONNECT_TIMEOUT_MS);
        connect_tries++;;
    }

    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Connect to host %s:%d failed (%d)\n", client->hostname, client->port, result);
        wiced_tcp_delete_socket(client->socket_ptr);
        client->socket_ptr = NULL;
        return result;
    }

    return WICED_SUCCESS;
}


wiced_result_t avsc_get_access_token(avs_client_t* client)
{
    wiced_packet_t* packet = NULL;
    char*           body_ptr;
    char*           ptr;
    cJSON*          root;
    cJSON*          access;
    uint8_t*        data;
    uint16_t        avail_data_length;
    uint16_t        total_data_length;
    wiced_result_t  result = WICED_SUCCESS;
    int             len_to_read;
    int             http_idx;
    int             body_idx;
    int             len;
    int             idx;

    /*
     * See if we can connect to the HTTP server.
     */

    if (avsc_socket_connect(client) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /*
     * Send off the POST request to the HTTP server.
     * We need to start off by building the content body so we can get the size for the header field.
     */

    avsc_urlencode(client->directive_buf, client->params.refresh_token);
    if (client->params.client_secret != NULL)
    {
        snprintf(client->tmp_buf, AVSC_TMP_BUF_SIZE, AVS_AUTHORIZATION_GRANT_WEB_BASED,
                 client->directive_buf, client->params.client_id, client->params.client_secret);
    }
    else
    {
        snprintf(client->tmp_buf, AVSC_TMP_BUF_SIZE, AVS_AUTHORIZATION_GRANT_APP_BASED, client->directive_buf, client->params.client_id);
    }
    len = strlen(client->tmp_buf);
    snprintf(client->http_buf, AVSC_HTTP_BUF_SIZE, avs_HTTP_get_refresh_token_template, len);
    strcat(client->http_buf, client->tmp_buf);

    wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Sending query:\n%s\n", client->http_buf);

    result = wiced_tcp_send_buffer(client->socket_ptr, client->http_buf, (uint16_t)strlen(client->http_buf));
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Unable to send HTTP post (%d)\n", result);
        result = WICED_ERROR;
        goto _exit;
    }

    /*
     * Now wait for a response.
     */

    idx         = 0;
    body_ptr    = NULL;
    body_idx    = 0;
    len_to_read = 0;
    http_idx    = 0;
    do
    {
        result = wiced_tcp_receive(client->socket_ptr, &packet, RECEIVE_TIMEOUT_MS);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error returned from socket receive (%d)\n", result);
            break;
        }

        result = wiced_packet_get_data(packet, 0, &data, &avail_data_length, &total_data_length);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error getting packet data (%d)\n", result);
            result = WICED_ERROR;
            goto _exit;
        }

        if (http_idx + avail_data_length < AVSC_HTTP_BUF_SIZE - 1)
        {
            len = avail_data_length;
        }
        else
        {
            len = AVSC_HTTP_BUF_SIZE - 1 - http_idx;
        }

        memcpy(&client->http_buf[http_idx], data, len);
        http_idx += len;

        wiced_packet_delete(packet);
        packet = NULL;

        if (body_ptr == NULL)
        {
            while (idx < http_idx)
            {
                if (client->http_buf[idx] == avs_HTTP_body_separator[body_idx])
                {
                    if (avs_HTTP_body_separator[++body_idx] == '\0')
                    {
                        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Found end of HTTP header\n");

                        /*
                         * We've found the end of the HTTP header fields. Search for a content length token
                         * so we can know how much data to expect in the body.
                         */

                        body_ptr = &client->http_buf[idx + 1];
                        ptr      = strcasestr(client->http_buf, HTTP11_HEADER_CONTENT_LENGTH);
                        if (ptr != NULL)
                        {
                            ptr += sizeof(HTTP11_HEADER_CONTENT_LENGTH);
                            while (ptr && *ptr == ' ')
                            {
                                ptr++;
                            }
                            if (ptr != NULL)
                            {
                                len_to_read = atoi(ptr);
                                if (len_to_read > 0)
                                {
                                    len_to_read += (int)(body_ptr - client->http_buf);
                                }
                                else
                                {
                                    len_to_read = 0;
                                }
                            }
                        }
                        break;
                    }
                }
                else
                {
                    body_idx = 0;
                }
                idx++;
            }
        }

        if (len_to_read > 0 && http_idx >= len_to_read)
        {
            break;
        }

    } while (result == WICED_SUCCESS);

    client->http_buf[http_idx] = '\0';

    if (wiced_log_get_facility_level(WLF_AVS_CLIENT) >= WICED_LOG_DEBUG0)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Received response of %d bytes\n", http_idx);
        for (len = 0; len < http_idx; len++)
        {
            wiced_log_printf("%c", client->http_buf[len]);
        }
        wiced_log_printf("\n\n");
    }

    /*
     * Check the response code.
     */

    if (strncasecmp(client->http_buf, HTTP11_HEADER_STATUS_OK, strlen(HTTP11_HEADER_STATUS_OK)) != 0)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Bad HTTP status returned from server: %.*s\n", 12, client->http_buf);
        result = WICED_ERROR;
        goto _exit;
    }

    /*
     * Run the content body through the JSON parser.
     */

    if (body_ptr != NULL)
    {
        wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Running the response through the cJSON parser...\n");

        root   = cJSON_Parse(body_ptr);
        access = cJSON_GetObjectItem(root, AVS_TOKEN_ACCESS);
        if (access)
        {
            /*
             * Success. We have a new access token.
             */

            if (client->access_token)
            {
                free(client->access_token);
            }
            client->access_token = strdup(access->valuestring);

            if (client->access_token != NULL)
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_DEBUG0, "Access token: %s\n", access->valuestring);
            }
            else
            {
                wiced_log_msg(WLF_AVS_CLIENT, WICED_LOG_ERR, "Error allocating access token\n");
            }
        }

        /*
         * Don't forget to free the JSON tree.
         */

        cJSON_Delete(root);
    }

  _exit:
    if (client->socket_ptr)
    {
        wiced_tcp_disconnect(client->socket_ptr);
        wiced_tcp_delete_socket(client->socket_ptr);
        client->socket_ptr = NULL;
    }

    if (packet != NULL)
    {
        wiced_packet_delete(packet);
        packet = NULL;
    }

    return result;
}
