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

#include <malloc.h>

#include "wiced.h"
#include "wiced_log.h"

#include "command_console.h"
#include "wifi/command_console_wifi.h"
#include "dct/command_console_dct.h"

#include "avs_app.h"
#include "avs_app_debug.h"


/******************************************************
 *                      Macros
 ******************************************************/

#define AVS_APP_CONSOLE_COMMANDS \
    { (char*) "exit",           avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Exit application" }, \
    { (char*) "config",         avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Display / change config values" }, \
    { (char*) "log",            avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Set the current log level" }, \
    { (char*) "netlog",         avs_app_console_command,    0, NULL, NULL, (char *)"[ip]",              (char *)"Enable/disable network logging" }, \
    { (char*) "memory",         avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Display the current system memory info" }, \
    { (char*) "capture",        avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Begin audio capture" }, \
    { (char*) "volume",         avs_app_console_command,    1, NULL, NULL, (char *)"<volume>",          (char *)"Change the playback volume" }, \
    { (char*) "connect",        avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Connect to AVS" }, \
    { (char*) "disconnect",     avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Disconnect from AVS" }, \
    { (char*) "report",         avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Send UserInactivityReport to AVS" }, \
    { (char*) "locale",         avs_app_console_command,    1, NULL, NULL, (char *)"<us | gb | de>",    (char *)"Send local SettingsUpdated event to AVS" }, \
    { (char*) "play",           avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Send a PlaybackController PlayCommandIssued event to AVS" }, \
    { (char*) "pause",          avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Send a PlaybackController PauseCommandIssued event to AVS" }, \
    { (char*) "next",           avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Send a PlaybackController NextCommandIssued event to AVS" }, \
    { (char*) "prev",           avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Send a PlaybackController PreviousCommandIssued event to AVS" }, \
    { (char*) "memlog",         avs_app_console_command,    1, NULL, NULL, (char *)"<size in bytes>",   (char *)"Enable/disable in memory logging (0 disables)" }, \
    { (char*) "memdump",        avs_app_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Dump in memory log" }, \

/******************************************************
 *                    Constants
 ******************************************************/

#define AVS_APP_CONSOLE_COMMAND_MAX_LENGTH          (100)
#define AVS_APP_CONSOLE_COMMAND_HISTORY_LENGTH      (10)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AVS_APP_CONSOLE_CMD_EXIT = 0,
    AVS_APP_CONSOLE_CMD_CONFIG,
    AVS_APP_CONSOLE_CMD_LOG,
    AVS_APP_CONSOLE_CMD_NETLOG,
    AVS_APP_CONSOLE_CMD_MEMORY,
    AVS_APP_CONSOLE_CMD_CAPTURE,
    AVS_APP_CONSOLE_CMD_VOLUME,
    AVS_APP_CONSOLE_CMD_CONNECT,
    AVS_APP_CONSOLE_CMD_DISCONNECT,
    AVS_APP_CONSOLE_CMD_REPORT,
    AVS_APP_CONSOLE_CMD_LOCALE,
    AVS_APP_CONSOLE_CMD_PLAY,
    AVS_APP_CONSOLE_CMD_PAUSE,
    AVS_APP_CONSOLE_CMD_NEXT,
    AVS_APP_CONSOLE_CMD_PREVIOUS,
    AVS_APP_CONSOLE_CMD_MEMLOG,
    AVS_APP_CONSOLE_CMD_MEMDUMP,

    AVS_APP_CONSOLE_CMD_MAX
} AVS_APP_CONSOLE_CMDS_T;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

static int avs_app_console_command(int argc, char *argv[]);

/******************************************************
 *               Variables Definitions
 ******************************************************/

static char avs_app_command_buffer[AVS_APP_CONSOLE_COMMAND_MAX_LENGTH];
static char avs_app_command_history_buffer[AVS_APP_CONSOLE_COMMAND_MAX_LENGTH * AVS_APP_CONSOLE_COMMAND_HISTORY_LENGTH];

const command_t avs_app_command_table[] =
{
#ifdef AVS_APP_ENABLE_WIFI_CMDS
    WIFI_COMMANDS
#endif
    DCT_CONSOLE_COMMANDS
    AVS_APP_CONSOLE_COMMANDS
    CMD_TABLE_END
};

static char* command_lookup[AVS_APP_CONSOLE_CMD_MAX] =
{
    "exit",
    "config",
    "log",
    "netlog",
    "memory",
    "capture",
    "volume",
    "connect",
    "disconnect",
    "report",
    "locale",
    "play",
    "pause",
    "next",
    "prev",
    "memlog",
    "memdump",
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static void avs_app_console_dct_callback(console_dct_struct_type_t struct_changed, void* app_data)
{
    avs_app_t* app = (avs_app_t*)app_data;

    /* sanity check */
    if (app == NULL)
    {
        return;
    }

    switch (struct_changed)
    {
        case CONSOLE_DCT_STRUCT_TYPE_WIFI:
            /* Get WiFi configuration */
            wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_RELOAD_DCT_WIFI);
            break;

        case CONSOLE_DCT_STRUCT_TYPE_NETWORK:
            /* Get network configuration */
            wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_RELOAD_DCT_NETWORK);
            break;

        default:
            break;
    }
}


static int avs_app_console_command(int argc, char *argv[])
{
    avs_app_t* app = g_avs_app;
    avs_app_msg_t msg;
    AVS_CLIENT_LOCALE_T locale;
    uint32_t event = 0;
    uint32_t size;
    char* ptr;
    int log_level;
    int volume;
    int i;

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Received command: %s\n", argv[0]);

    if (app == NULL || app->tag != AVS_APP_TAG_VALID)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "avs_app_console_command() Bad app structure\n");
        return ERR_CMD_OK;
    }

    /*
     * Lookup the command in our table.
     */

    for (i = 0; i < AVS_APP_CONSOLE_CMD_MAX; ++i)
    {
        if (strcmp(command_lookup[i], argv[0]) == 0)
            break;
    }

    if (i >= AVS_APP_CONSOLE_CMD_MAX)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unrecognized command: %s\n", argv[0]);
        return ERR_CMD_OK;
    }

    switch (i)
    {
        case AVS_APP_CONSOLE_CMD_EXIT:
            event = AVS_APP_EVENT_SHUTDOWN;
            break;

        case AVS_APP_CONSOLE_CMD_CONFIG:
            avs_app_set_config(&app->dct_tables, argc, argv);
            break;

        case AVS_APP_CONSOLE_CMD_LOG:
            if (argc == 1)
            {
                wiced_log_printf("Current log levels:\n");
                for (i = 0; i < WLF_MAX; i++)
                {
                    wiced_log_printf("    Facility %d: level %d\n", i, wiced_log_get_facility_level(i));
                }
            }
            else if (argc == 2)
            {
                log_level = atoi(argv[1]);
                wiced_log_printf("Setting new log levels to %d (0 - off, %d - max debug)\n", log_level, WICED_LOG_DEBUG4);
                wiced_log_set_all_levels(log_level);
            }
            break;

        case AVS_APP_CONSOLE_CMD_NETLOG:
#ifdef AVS_APP_ENABLE_NETWORK_LOGGING
            if (argc > 1)
            {
                if (!app->network_logging_active)
                {
                    wiced_ip_address_t ip_addr;
                    uint32_t a, b, c, d;

                    /*
                     * Use the specified an IP address.
                     */

                    if (sscanf(argv[1], "%lu.%lu.%lu.%lu", &a, &b, &c, &d) != 4)
                    {
                        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Invalid TCP server IP address\n");
                        return ERR_INSUFFICENT_ARGS;
                    }

                    SET_IPV4_ADDRESS(ip_addr, MAKE_IPV4_ADDRESS(a, b, c, d));

                    if (avs_app_debug_create_tcp_data_socket(&ip_addr, 0) == WICED_SUCCESS)
                    {
                        wiced_log_set_platform_output(avs_app_debug_tcp_log_output_handler);
                        app->network_logging_active = WICED_TRUE;
                    }
                }
                else
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Network logging is already active\n");
                }
            }
            else if (app->network_logging_active)
            {
                wiced_log_set_platform_output(avs_app_log_output_handler);
                avs_app_debug_close_tcp_data_socket();
                app->network_logging_active = WICED_FALSE;
            }
#else
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Network logging is not enabled in this build\n");
#endif
            break;

        case AVS_APP_CONSOLE_CMD_MEMORY:
            {
                extern unsigned char _heap[];
                extern unsigned char _eheap[];
                extern unsigned char *sbrk_heap_top;
                volatile struct mallinfo mi = mallinfo();

                wiced_log_printf("sbrk heap size:    %7lu\n", (uint32_t)_eheap - (uint32_t)_heap);
                wiced_log_printf("sbrk current free: %7lu \n", (uint32_t)_eheap - (uint32_t)sbrk_heap_top);

                wiced_log_printf("malloc allocated:  %7d\n", mi.uordblks);
                wiced_log_printf("malloc free:       %7d\n", mi.fordblks);

                wiced_log_printf("\ntotal free memory: %7lu\n", mi.fordblks + (uint32_t)_eheap - (uint32_t)sbrk_heap_top);
            }
            break;

        case AVS_APP_CONSOLE_CMD_VOLUME:
            volume = atoi(argv[1]);
            volume = MIN(volume, AVS_APP_VOLUME_MAX);
            volume = MAX(volume, AVS_APP_VOLUME_MIN);
            msg.type = AVS_APP_MSG_SET_VOLUME;
            msg.arg1 = (uint32_t)volume;
            if (wiced_rtos_push_to_queue(&app->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                event = AVS_APP_EVENT_MSG_QUEUE;
            }
            else
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error pushing volume msg\n");
            }
            break;

        case AVS_APP_CONSOLE_CMD_CAPTURE:
            event = AVS_APP_EVENT_CAPTURE;
            break;

        case AVS_APP_CONSOLE_CMD_CONNECT:
            event = AVS_APP_EVENT_CONNECT;
            break;

        case AVS_APP_CONSOLE_CMD_DISCONNECT:
            event = AVS_APP_EVENT_DISCONNECT;
            break;

        case AVS_APP_CONSOLE_CMD_REPORT:
            event = AVS_APP_EVENT_REPORT;
            break;

        case AVS_APP_CONSOLE_CMD_LOCALE:
            if (strcmp(argv[1], "us") == 0)
            {
                locale = AVS_CLIENT_LOCALE_EN_US;
            }
            else if (strcmp(argv[1], "gb") == 0)
            {
                locale = AVS_CLIENT_LOCALE_EN_GB;
            }
            else if (strcmp(argv[1], "de") == 0)
            {
                locale = AVS_CLIENT_LOCALE_DE_DE;
            }
            else
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unknown locale: %s\n", argv[1]);
                break;
            }
            msg.type = AVS_APP_MSG_SET_LOCALE;
            msg.arg1 = (uint32_t)locale;
            if (wiced_rtos_push_to_queue(&app->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                event = AVS_APP_EVENT_MSG_QUEUE;
            }
            else
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error pushing locale msg\n");
            }
            break;

        case AVS_APP_CONSOLE_CMD_PLAY:
            event = AVS_APP_EVENT_PLAY;
            break;

        case AVS_APP_CONSOLE_CMD_PAUSE:
            event = AVS_APP_EVENT_PAUSE;
            break;

        case AVS_APP_CONSOLE_CMD_NEXT:
            event = AVS_APP_EVENT_NEXT;
            break;

        case AVS_APP_CONSOLE_CMD_PREVIOUS:
            event = AVS_APP_EVENT_PREVIOUS;
            break;

        case AVS_APP_CONSOLE_CMD_MEMLOG:
            size = atoi(argv[1]);
            if (size > 0)
            {
                if (app->logbuf)
                {
                    ptr = realloc(app->logbuf, size);
                    if (ptr != NULL)
                    {
                        app->logbuf      = ptr;
                        app->logbuf_size = size;
                        if (app->logbuf_idx > app->logbuf_size)
                        {
                            app->logbuf_idx = app->logbuf_size;
                        }
                    }
                    else
                    {
                        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to resize in memory logging buffer\n");
                    }
                }
                else
                {
                    app->logbuf = malloc(size);
                    if (app->logbuf != NULL)
                    {
                        app->logbuf_size = size;
                        app->logbuf_idx  = 0;
                    }
                    else
                    {
                        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to allocate in memory logging buffer\n");
                    }
                }
            }
            else
            {
                avs_app_dump_memory_log(app);
                app->logbuf_size = 0;
                app->logbuf_idx  = 0;
                free(app->logbuf);
                app->logbuf = NULL;
            }
            break;

        case AVS_APP_CONSOLE_CMD_MEMDUMP:
            if (app->logbuf)
            {
                avs_app_dump_memory_log(app);
            }
            else
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "In memory logging not active\n");
            }
            break;
    }

    if (event)
    {
        /*
         * Send off the event to the main loop.
         */

        wiced_rtos_set_event_flags(&app->events, event);
    }

    return ERR_CMD_OK;
}


wiced_result_t avs_app_console_start(avs_app_t *app)
{
    wiced_result_t result = WICED_SUCCESS;

    /*
     * Create the command console.
     */

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Start the command console\n");
    result = command_console_init(STDIO_UART, sizeof(avs_app_command_buffer), avs_app_command_buffer, AVS_APP_CONSOLE_COMMAND_HISTORY_LENGTH, avs_app_command_history_buffer, " ");
    if (result != WICED_SUCCESS)
    {
        return result;
    }
    console_add_cmd_table(avs_app_command_table);
    console_dct_register_callback(avs_app_console_dct_callback, app);

    return result;
}


wiced_result_t avs_app_console_stop(avs_app_t *app)
{
    /*
     * Shutdown the console.
     */

    console_dct_register_callback(NULL, NULL);
    command_console_deinit();

    return WICED_SUCCESS;
}

