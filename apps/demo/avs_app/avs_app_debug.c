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

/** @file AVS app debug support routines
 *
 */

#include "avs_app_debug.h"

#include "wiced_log.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define TCP_PACKET_MAX_DATA_LENGTH        1450
#define TCP_DEFAULT_SERVER_IP_ADDRESS     MAKE_IPV4_ADDRESS(192,168,0,200)
#define TCP_DEFAULT_SERVER_PORT           19702
#define TCP_CLIENT_CONNECT_TIMEOUT        500
#define TCP_CONNECTION_NUMBER_OF_RETRIES  3

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

static wiced_tcp_socket_t   avs_app_debug_socket;
static wiced_tcp_socket_t*  avs_app_debug_socket_ptr;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t avs_app_debug_create_tcp_data_socket(wiced_ip_address_t* ip_addr, int port)
{
    wiced_ip_address_t server_ip_address;
    wiced_result_t     result;
    int                connection_retries;

    if (avs_app_debug_socket_ptr != NULL)
    {
        return WICED_ERROR;
    }

    if (ip_addr == NULL)
    {
        SET_IPV4_ADDRESS(server_ip_address, TCP_DEFAULT_SERVER_IP_ADDRESS);
    }
    else
    {
        memcpy(&server_ip_address, ip_addr, sizeof(wiced_ip_address_t));
    }

    if (port <= 0)
    {
        port = TCP_DEFAULT_SERVER_PORT;
    }

    /* Create a TCP socket */
    if (wiced_tcp_create_socket(&avs_app_debug_socket, WICED_STA_INTERFACE) != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "TCP socket creation failed\n");
        return WICED_ERROR;
    }

    /* Bind to the socket */
    wiced_tcp_bind(&avs_app_debug_socket, port);

    /* Connect to the remote TCP server, try several times */
    connection_retries = 0;
    do
    {
        result = wiced_tcp_connect(&avs_app_debug_socket, &server_ip_address, port, TCP_CLIENT_CONNECT_TIMEOUT);
        connection_retries++;
    } while (result != WICED_SUCCESS && connection_retries < TCP_CONNECTION_NUMBER_OF_RETRIES);

    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to connect to the debug log server!\n");
        wiced_tcp_delete_socket(&avs_app_debug_socket);
        return result;
    }

    avs_app_debug_socket_ptr = &avs_app_debug_socket;

    return result;
}


wiced_result_t avs_app_debug_close_tcp_data_socket(void)
{
    if (!avs_app_debug_socket_ptr)
    {
        return WICED_ERROR;
    }

    avs_app_debug_socket_ptr = NULL;

    wiced_tcp_disconnect(&avs_app_debug_socket);
    wiced_tcp_delete_socket(&avs_app_debug_socket);

    return WICED_SUCCESS;
}


wiced_result_t avs_app_debug_send_tcp_data(uint8_t* data, int datalen)
{
    wiced_packet_t* packet;
    char*           tx_data;
    uint16_t        available_data_length;
    wiced_result_t  result;

    if (avs_app_debug_socket_ptr == NULL)
    {
        return WICED_ERROR;
    }

    if (datalen > TCP_PACKET_MAX_DATA_LENGTH)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Too much data %d (max %d)\n", datalen, TCP_PACKET_MAX_DATA_LENGTH);
        return WICED_ERROR;
    }

    /* Create the TCP packet. Memory for the tx_data is automatically allocated */
    if (wiced_packet_create_tcp(avs_app_debug_socket_ptr, TCP_PACKET_MAX_DATA_LENGTH, &packet, (uint8_t**)&tx_data, &available_data_length) != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "TCP packet creation failed\n");
        return WICED_ERROR;
    }

    /* Write the message into tx_data"  */
    memcpy(tx_data, data, datalen);

    /* Set the end of the data portion */
    wiced_packet_set_data_end(packet, (uint8_t*)tx_data + datalen);

    /* Send the TCP packet */
    result = wiced_tcp_send_packet(avs_app_debug_socket_ptr, packet);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Log TCP packet send failed (%d)\n", result);

        /* Delete packet, since the send failed */
        wiced_packet_delete(packet);

        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


int avs_app_debug_tcp_log_output_handler(WICED_LOG_LEVEL_T level, char *logmsg)
{
    avs_app_debug_send_tcp_data((uint8_t*)logmsg, strlen(logmsg));

    return 0;
}
