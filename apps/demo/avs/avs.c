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

/** @file avs
 *
 * Reference AVS application to demonstrate how to interface with AVS using WICED's building blocks.
 *
 * Use case to be demonstrated in this reference application:
 *    - Establish initial connection to AVS and retrieve access token
 *    - Capture the voice data using WICED 43907WAE3 mic interface
 *      (the application currently uses the digital mic next to the SW1/RESET button on 43907WAE3)
 *    - Send the captured PCM voice samples to AVS using HTTP2 request
 *    - Receive the speech synthesizer audio data sent by AVS, which is in the HTTP2 response
 *    - Playback the speech synthesizer audio response (MP3) using WICED audio client
 *
 * Information and documentation of the Alexa APIs are available online at
 * http://developer.amazon.com/public/solutions/alexa/alexa-voice-service/content/avs-api-overview
 *
 * This application demonstrates the use of the AVS SpeechRecognizer and SpeechSynthesizer
 * interfaces along with the necessary event(s) from the System interface to establish the
 * initial AVS connection.
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
 * Fill in the defines below with the strings for the refresh token, client ID, and client secret:
 *
 * #define AVS_REFRESH_TOKEN       ""
 * #define AVS_CLIENT_ID           ""
 * #define AVS_CLIENT_SECRET       ""
 *
 * Once the client device information has been entered, proceed with the following steps:
 *  1. Modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials in the
 *     wifi_config_dct.h header file to match your Wi-Fi access point
 *  2. Plug the WICED eval board into your computer
 *  3. Open a terminal application and connect to the WICED eval board
 *  4. Build and download the application (to the WICED board)
 *
 * After the download completes, the terminal displays WICED startup information and the
 * software will join the specified Wi-Fi network. Next the connection to AVS is established.
 * Once the initial connection to AVS is complete, press the Play/Pause button to start
 * capturing audio with the mic. The audio data is sent to AVS for processing and the audio
 * response will be played out through the default output device.
 * Alternatively, press the Multi button and a pre-recorded audio clip containing the sentence
 * "tell me a joke" to AVS for processing; audio will also be played out through the same output device.
 * Playback volume level can be adjusted by pressing Volume Down and Volume Up.
 */

#include <ctype.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>

#include "wiced.h"
#include "wiced_tcpip.h"
#include "wiced_tls.h"
#include "command_console.h"
#include "wifi/command_console_wifi.h"
#include "dct/command_console_dct.h"
#include "resources.h"

#include "dns.h"
#include "cJSON.h"

#include "http2.h"

#include "avs.h"
#include "avs_utilities.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define AVS_CONSOLE_COMMANDS \
    { (char*) "exit",           avs_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Exit application" }, \
    { (char*) "config",         avs_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Display / change config values" }, \
    { (char*) "log",            avs_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Set the current log level" }, \
    { (char*) "memory",         avs_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Display the current system memory info" }, \
    { (char*) "avs",            avs_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Establish AVS connection" }, \
    { (char*) "talk",           avs_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Send speech request to AVS" }, \
    { (char*) "capture",        avs_console_command,    0, NULL, NULL, (char *)"",                  (char *)"Begin audio capture" }, \
    { (char*) "volume",         avs_console_command,    1, NULL, NULL, (char *)"<volume>",          (char *)"Change the playback volume" }, \
    { (char*) "state",          avs_console_command,    0, NULL, NULL, (char *)"<state>",           (char *)"Get/set app state" }, \


/******************************************************
 *                    Constants
 ******************************************************/

#define AVS_CONSOLE_COMMAND_MAX_LENGTH          (100)
#define AVS_CONSOLE_COMMAND_HISTORY_LENGTH      (10)

#define DNS_TIMEOUT_MS                          (10 * 1000)
#define CONNECT_TIMEOUT_MS                      (2 * 1000)
#define RECEIVE_TIMEOUT_MS                      (2 * 1000)

#define PING_TIMER_PERIOD_MSECS                 (5 * 60 * 1000)
#define DEFAULT_HTTP_PORT                       (80)
#define DEFAULT_HTTPS_PORT                      (443)

#define HTTP11_HEADER_CONTENT_LENGTH            "Content-Length"
#define HTTP11_HEADER_STATUS_OK                 "HTTP/1.1 200"

#define HTTP_HEADER_FIELD_STATUS                ":status"
#define HTTP_HEADER_FIELD_CONTENT_TYPE          "content-type"
#define HTTP_STATUS_OK                          (200)
#define HTTP_STATUS_NO_CONTENT                  (204)

#define AVS_AUTHORIZATION_URL   "https://api.amazon.com/auth/o2/token"

/*
 * When running the AVS application outside of the US, you may need to change the AVS_BASE_URL definition.
 * If a SetEndpoint directive is received with a different URL to use, an error message will be
 * displayed on the console output.
 */

#define AVS_BASE_URL            "https://avs-alexa-na.amazon.com"
//#define AVS_BASE_URL            "https://avs-alexa-eu.amazon.com"

#define AVS_API_VERSION         "v20160207"

#define AVS_DIRECTIVES_PATH     "/"AVS_API_VERSION"/directives"
#define AVS_EVENTS_PATH         "/"AVS_API_VERSION"/events"

#define AVS_BOUNDARY_TERM       "####This is MY boundary####"
#define AVS_HEADER_CONTENT_TYPE "multipart/form-data; boundary="AVS_BOUNDARY_TERM

#define AVS_JSON_HEADERS        "Content-Disposition: form-data; name=\"metadata\"\r\nContent-Type: application/json; charset=UTF-08\r\n"
#define AVS_AUDIO_HEADERS       "Content-Disposition: form-data; name=\"audio\"\r\nContent-Type: application/octet-stream\r\n"

#define AVS_GRANT_TYPE          "refresh_token"
#define AVS_REFRESH_TOKEN       "Fill in"
#define AVS_CLIENT_ID           "Fill in"
#define AVS_CLIENT_SECRET       "Fill in"

#define AVS_CONTEXT_IDLE        "{\"header\":{\"namespace\":\"Alerts\",\"name\":\"AlertsState\"},\"payload\":{\"allAlerts\":[],\"activeAlerts\":[]}},{\"header\":{\"namespace\":\"AudioPlayer\",\"name\":\"PlaybackState\"},\"payload\":{\"token\":\"\",\"offsetInMilliseconds\":0,\"playerActivity\":\"IDLE\"}},{\"header\":{\"namespace\":\"Speaker\",\"name\":\"VolumeState\"},\"payload\":{\"volume\":100,\"muted\":false}},{\"header\":{\"namespace\":\"SpeechSynthesizer\",\"name\":\"SpeechState\"},\"payload\":{\"token\":\"\",\"offsetInMilliseconds\":0,\"playerActivity\":\"IDLE\"}}"
#define AVS_INITIAL_STATE       "{\"context\":["AVS_CONTEXT_IDLE"],\"event\":{\"header\":{\"namespace\":\"System\",\"name\":\"SynchronizeState\",\"messageId\":\"meId-2034610a\"},\"payload\":{}}}"
#define AVS_CANNED_SPEECH_REQUEST      "{\"event\":{\"header\":{\"namespace\":\"SpeechRecognizer\",\"name\":\"Recognize\",\"messageId\":\"mId-b827eb792034610b\",\"dialogRequestId\":\"dId-9827eb792034610a\"},\"payload\":{\"profile\":\"CLOSE_TALK\",\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\"}}}"
#define AVS_SPEECH_REQUEST      "{\"event\":{\"header\":{\"namespace\":\"SpeechRecognizer\",\"name\":\"Recognize\",\"messageId\":\"mId-b827eb79\",\"dialogRequestId\":\"dId-9827eb78\"},\"payload\":{\"profile\":\"NEAR_FIELD\",\"format\":\"AUDIO_L16_RATE_16000_CHANNELS_1\"}}}"


/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AVS_CONSOLE_CMD_EXIT = 0,
    AVS_CONSOLE_CMD_CONFIG,
    AVS_CONSOLE_CMD_LOG,
    AVS_CONSOLE_CMD_MEMORY,
    AVS_CONSOLE_CMD_AVS,
    AVS_CONSOLE_CMD_TALK,
    AVS_CONSOLE_CMD_CAPTURE,
    AVS_CONSOLE_CMD_VOLUME,
    AVS_CONSOLE_CMD_STATE,

    AVS_CONSOLE_CMD_MAX
} AVS_CONSOLE_CMDS_T;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static int avs_console_command(int argc, char *argv[]);


/******************************************************
 *               Variable Definitions
 ******************************************************/

static char avs_command_buffer[AVS_CONSOLE_COMMAND_MAX_LENGTH];
static char avs_command_history_buffer[AVS_CONSOLE_COMMAND_MAX_LENGTH * AVS_CONSOLE_COMMAND_HISTORY_LENGTH];

static char avs_HTTP_get_refresh_token_template[] =
{
    "POST /auth/o2/token HTTP/1.1\r\n"
    "Host: api.amazon.com\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: %d\r\n"
    "Cache-Control: no-cache\r\n"
    "\r\n"
};

static char avs_HTTP_body_separator[] =
{
    "\r\n\r\n"
};

const command_t avs_command_table[] =
{
#ifdef AVS_ENABLE_WIFI_CMDS
    WIFI_COMMANDS
#endif
    DCT_CONSOLE_COMMANDS
    AVS_CONSOLE_COMMANDS
    CMD_TABLE_END
};

static char* command_lookup[AVS_CONSOLE_CMD_MAX] =
{
    "exit",
    "config",
    "log",
    "memory",
    "avs",
    "talk",
    "capture",
    "volume",
    "state",
};

const static char* avs_state_string[AVS_STATE_MAX] =
{
    "IDLE",
    "CONNECTED",
    "DOWNCHANNEL",
    "SYNCHRONIZE",
    "READY",
    "SPEECH_REQUEST",
    "PROCESSING_RESPONSE",
    "PLAYING_RESPONSE",
    "AWAITING_RESPONSE_EOS",
};

avs_t* g_avs = NULL;

/******************************************************
 *               Function Definitions
 ******************************************************/

static int avs_log_output_handler(WICED_LOG_LEVEL_T level, char *logmsg)
{
    write(STDOUT_FILENO, logmsg, strlen(logmsg));

    return 0;
}


static void timer_callback(void *arg)
{
    avs_t* avs = (avs_t *)arg;

    if (avs && avs->tag == AVS_TAG_VALID)
    {
        wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_TIMER);
    }
}


static int avs_console_command(int argc, char *argv[])
{
    uint32_t event = 0;
    int log_level;
    int volume;
    int i;

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Received command: %s\n", argv[0]);

    if (g_avs == NULL || g_avs->tag != AVS_TAG_VALID)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "avs_console_command() Bad app structure\n");
        return ERR_CMD_OK;
    }

    /*
     * Lookup the command in our table.
     */

    for (i = 0; i < AVS_CONSOLE_CMD_MAX; ++i)
    {
        if (strcmp(command_lookup[i], argv[0]) == 0)
            break;
    }

    if (i >= AVS_CONSOLE_CMD_MAX)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unrecognized command: %s\n", argv[0]);
        return ERR_CMD_OK;
    }

    switch (i)
    {
        case AVS_CONSOLE_CMD_EXIT:
            event = AVS_EVENT_SHUTDOWN;
            break;

        case AVS_CONSOLE_CMD_CONFIG:
            avs_set_config(&g_avs->dct_tables, argc, argv);
            break;

        case AVS_CONSOLE_CMD_LOG:
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

        case AVS_CONSOLE_CMD_MEMORY:
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

        case AVS_CONSOLE_CMD_AVS:
            event = AVS_EVENT_AVS;
            break;

        case AVS_CONSOLE_CMD_TALK:
            event = AVS_EVENT_SPEECH_REQUEST;
            break;

        case AVS_CONSOLE_CMD_VOLUME:
            volume = atoi(argv[1]);
            volume = MIN(volume, AVS_VOLUME_MAX);
            volume = MAX(volume, AVS_VOLUME_MIN);
            if (g_avs->audio_client_handle)
            {
                if (audio_client_ioctl(g_avs->audio_client_handle, AUDIO_CLIENT_IOCTL_SET_VOLUME, (void*)volume) == WICED_SUCCESS)
                {
                    g_avs->audio_client_volume = (uint32_t)volume;
                }
            }
            break;

        case AVS_CONSOLE_CMD_CAPTURE:
            event = AVS_EVENT_CAPTURE;
            break;

        case AVS_CONSOLE_CMD_STATE:
            if (argc == 1)
            {
                wiced_log_printf("Current app state: [%s]\n", avs_state_string[g_avs->state]);
            }
            else if (argc == 2)
            {
                uint32_t i;

                for (i = 0; i < AVS_STATE_MAX; i++)
                {
                    if (strcasecmp(avs_state_string[i], argv[1]) == 0)
                    {
                        break;
                    }
                }
                if (i >= AVS_STATE_MAX)
                {
                    wiced_log_printf("[%s] is not a valid state; current app state remains at [%s]\n",
                                     argv[1], avs_state_string[g_avs->state]);
                    wiced_log_printf("Here is the list of valid app states:\n");
                    for (i = 0; i < AVS_STATE_MAX; i++)
                    {
                        wiced_log_printf("%s\n", avs_state_string[i]);
                    }
                    wiced_log_printf("\n");
                }
                else if (i == g_avs->state)
                {
                    wiced_log_printf("Current app state remains at [%s]\n", avs_state_string[i]);
                }
                else
                {
                    g_avs->state = i;
                    wiced_log_printf("Setting new app state to [%s]\n", avs_state_string[g_avs->state]);
                }
            }
            break;
    }

    if (event)
    {
        /*
         * Send off the event to the main loop.
         */

        wiced_rtos_set_event_flags(&g_avs->events, event);
    }

    return ERR_CMD_OK;
}


static void avs_console_dct_callback(console_dct_struct_type_t struct_changed, void* app_data)
{
    avs_t* avs = (avs_t*)app_data;

    /* sanity check */
    if (avs == NULL)
    {
        return;
    }

    switch (struct_changed)
    {
        case CONSOLE_DCT_STRUCT_TYPE_WIFI:
            /* Get WiFi configuration */
            wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_RELOAD_DCT_WIFI);
            break;

        case CONSOLE_DCT_STRUCT_TYPE_NETWORK:
            /* Get network configuration */
            wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_RELOAD_DCT_NETWORK);
            break;

        default:
            break;
    }
}


static wiced_result_t uri_split(const char* uri, avs_t* avs)
{
    const char* uri_start;
    int copy_len;
    int len;

    avs->port = DEFAULT_HTTP_PORT;
    avs->tls  = WICED_FALSE;

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
            avs->tls = WICED_TRUE;
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
        avs->port = atoi(&uri_start[len + 1]);
    }
    else if (avs->tls == WICED_TRUE)
    {
        avs->port = DEFAULT_HTTPS_PORT;
    }

    copy_len = len;
    if (copy_len > AVS_HOSTNAME_LEN - 2)
    {
        copy_len = AVS_HOSTNAME_LEN - 2;
    }
    memcpy(avs->hostname, uri_start, copy_len);
    avs->hostname[copy_len] = '\0';

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
        if (copy_len > AVS_PATH_LEN - 2)
        {
            copy_len = AVS_PATH_LEN - 2;
        }
        memcpy(avs->path, uri_start, copy_len);
        avs->path[copy_len] = '\0';
    }
    else
    {
        avs->path[0] = '/';
        avs->path[1] = '\0';
    }

    return WICED_SUCCESS;
}


static wiced_result_t http2_disconnect_event_callback(http_connection_ptr_t connect, http_request_id_t last_processed_request, uint32_t error, void* user_data)
{
    avs_t* avs = (avs_t*)user_data;

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Disconnect event\n");

    if (avs == NULL || avs->tag != AVS_TAG_VALID)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "http_disconnect_event_callback: Bad app structure\n");
        return WICED_ERROR;
    }

    wiced_rtos_set_event_flags(&g_avs->events, AVS_EVENT_DISCONNECT);

    return WICED_SUCCESS;
}


/*
 * A simple callback for received headers.
 */

static wiced_result_t http2_recv_header_callback(http_connection_ptr_t connect, http_request_id_t request, http_header_info_t* headers, http_frame_type_t type, uint8_t flags, void* user_data)
{
    avs_t* avs = (avs_t*)user_data;
    WICED_LOG_LEVEL_T level;
    avs_msg_t msg;
    char status_buf[4];
    char* msg_boundary;
    char* ptr;
    char* end;

    if (avs == NULL || avs->tag != AVS_TAG_VALID)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "http_recv_header_callback: Bad app structure\n");
        return WICED_ERROR;
    }

    if (type == HTTP_FRAME_TYPE_PUSH_PROMISE && (flags & HTTP_REQUEST_FLAGS_PUSH_PROMISE))
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Received a Push Promise request for stream %u\n", (unsigned int)request);
        return WICED_SUCCESS;
    }
    else if (type == HTTP_FRAME_TYPE_PING && (flags & HTTP_REQUEST_FLAGS_PING_RESPONSE))
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Received a ping response\n");
        return WICED_SUCCESS;
    }

    if (request != avs->http2_stream_id && request != avs->http2_downchannel_stream_id)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Received unexpected HEADER for stream id %u\n", (unsigned int)request);
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Received a header for stream %u, flags 0x%02x\n", (unsigned int)request, flags);

    level = wiced_log_get_facility_level(WLF_DEF);

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

                status_buf[0] = headers->value[0];
                status_buf[1] = headers->value[1];
                status_buf[2] = headers->value[2];
                status_buf[3] = '\0';
                msg.type = AVS_MSG_TYPE_HTTP_RESPONSE;
                msg.arg1 = request;
                msg.arg2 = (uint32_t)atoi(status_buf);
                if (wiced_rtos_push_to_queue(&avs->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
                {
                    wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_MSG_QUEUE);
                }
                else
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error pushing http response event\n");
                }
            }
            else if ((request == avs->http2_downchannel_stream_id || request == avs->http2_capture_stream_id) &&
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
                        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to allocate boundary string\n");
                        return WICED_ERROR;
                    }

                    if (request == avs->http2_downchannel_stream_id)
                    {
                        if (avs->msg_boundary_downchannel != NULL)
                        {
                            free(avs->msg_boundary_downchannel);
                        }
                        avs->msg_boundary_downchannel = msg_boundary;
                    }
                    else
                    {
                        if (avs->msg_boundary_speech != NULL)
                        {
                            free(avs->msg_boundary_speech);
                        }
                        avs->msg_boundary_speech = msg_boundary;
                    }
                    wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Msg boundary for stream %lu is %s\n", request, msg_boundary);
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

        avs->http2_stream_id = 0;
    }

    return WICED_SUCCESS;
}


/*
 * A simple callback for received data.
 */

static wiced_result_t http2_recv_data_callback(http_connection_ptr_t connect, http_request_id_t request, uint8_t* data, uint32_t length, uint8_t flags, void* user_data)
{
    avs_t* avs = (avs_t*)user_data;
    char* ptr;
    uint32_t i;

    if (avs == NULL || avs->tag != AVS_TAG_VALID)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "http_recv_data_callback: Bad app structure\n");
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Received %d DATA bytes for stream %u, flags 0x%02x\n", (int)length, (unsigned int)request, flags);

    if (request != avs->http2_downchannel_stream_id && avs->http2_capture_stream_id != 0 && avs->http2_capture_stream_id != request)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Data received when capture active (%u)\n", (unsigned int)request);
    }

    if (avs->http2_binary_data_stream != request && wiced_log_get_facility_level(WLF_DEF) >= WICED_LOG_DEBUG0)
    {
        if ((ptr = strstr((char*)data, "Content-Type: application/octet-stream\r\n")) != NULL)
        {
            ptr += 40;
            /*wiced_log_*/printf("\n%.*s\n", (int)(ptr-(char*)data), (char*)data);

            /*
             * Note that we're getting to a binary data portion of the response so that we don't
             * accidently try and log the data as text.
             */

            avs->http2_binary_data_stream = request;
        }
        else
        {
            for (i = 0; i < length; i++)
            {
                wiced_log_printf("%c", data[i]);
            }
        }
        wiced_log_printf("\n");
    }

    if (request == avs->http2_downchannel_stream_id)
    {
        /*
         * We have received a directive from AVS. We need to pass it to
         * the main thread for processing.
         *
         * For now we are going to assume that the entire directive will
         * fit in a single buffer. Probably not a safe assumption for a real
         * product but good enough for what we're trying to do in this application.
         */

        avs_send_msg_buffers(avs, AVS_MSG_TYPE_AVS_DIRECTIVE, request, data, length);
    }

    if (request == avs->http2_capture_stream_id)
    {
        avs_send_msg_buffers(avs, AVS_MSG_TYPE_SPEECH_SYNTHESIZER, request, data, length);

        if ((flags & HTTP_REQUEST_FLAGS_FINISH_REQUEST) && (length > 0))
        {
            /*
             * Last packet for this request.
             */

            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "http_recv_data_callback: HTTP_REQUEST_FLAGS_FINISH_REQUEST with length=%lu !!\n", length);
            avs_send_msg_buffers(avs, AVS_MSG_TYPE_SPEECH_SYNTHESIZER, request, NULL, 0);
        }
    }

    return WICED_SUCCESS;
}


static wiced_result_t avs_client_socket_connect(avs_t* avs)
{
    wiced_result_t          result;
    wiced_ip_address_t      ip_address;
    wiced_tls_context_t*    tls_context;
    int                     connect_tries;

    result = uri_split(AVS_AUTHORIZATION_URL, avs);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to parse URL %s\n", AVS_AUTHORIZATION_URL);
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Connect to host: %s, port %d, path %s\n", avs->hostname, avs->port, avs->path);

    /*
     * Are we dealing with an IP address or a hostname?
     */

    if (str_to_ip(avs->hostname, &ip_address) != 0)
    {
        /*
         * Try a hostname lookup.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Attemping DNS lookup for %s\n", avs->hostname);
        result = dns_client_hostname_lookup(avs->hostname, &ip_address, DNS_TIMEOUT_MS, WICED_STA_INTERFACE);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "DNS lookup failed (%d)\n", result);
            return result;
        }
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Using IP address " IPV4_PRINT_FORMAT "\n", IPV4_SPLIT_TO_PRINT(ip_address));

    /*
     * Open a socket for the connection.
     */

    if (avs->socket_ptr == NULL)
    {
        result = wiced_tcp_create_socket(&avs->socket, WICED_STA_INTERFACE);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to create socket (%d)\n", result);
            return result;
        }
        avs->socket_ptr = &avs->socket;
    }

    if (avs->tls == WICED_TRUE)
    {
        /*
         * Allocate the TLS context and associate with our socket.
         * If we set the socket context_malloced flag to true, the TLS
         * context will be freed automatically when the socket is deleted.
         */

        tls_context = malloc(sizeof(wiced_tls_context_t));
        if (tls_context == NULL)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to allocate tls context\n");
            return result;
        }
        wiced_tls_init_context(tls_context, NULL, NULL);
        wiced_tcp_enable_tls(avs->socket_ptr, tls_context);
        avs->socket_ptr->context_malloced = WICED_TRUE;
    }

    /*
     * Now try and connect.
     */

    connect_tries = 0;
    result = WICED_ERROR;
    while (connect_tries < 3 && result == WICED_ERROR)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Connection attempt %d\n", connect_tries);
        result = wiced_tcp_connect(avs->socket_ptr, &ip_address, avs->port, CONNECT_TIMEOUT_MS);
        connect_tries++;;
    }

    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Connect to host %s:%d failed (%d)\n", avs->hostname, avs->port, result);
        wiced_tcp_delete_socket(avs->socket_ptr);
        avs->socket_ptr = NULL;
        return result;
    }

    return WICED_SUCCESS;
}


static wiced_result_t avs_get_access_token(avs_t* avs)
{
    wiced_packet_t* packet = NULL;
    char*           body_ptr;
    char*           ptr;
    cJSON*          root;
    cJSON*          access;
    uint8_t*        data;
    uint16_t        avail_data_length;
    uint16_t        total_data_length;
    wiced_result_t  result;
    int             len_to_read;
    int             body_idx;
    int             len;
    int             idx;

    /*
     * See if we can connect to the HTTP server.
     */

    if (avs_client_socket_connect(avs) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /*
     * Send off the POST request to the HTTP server.
     * We need to start off by building the content body so we can get the size for the header field.
     */

    avs_urlencode(avs->encode_buf, AVS_REFRESH_TOKEN);
    snprintf(avs->tmp_buf, AVS_TMP_BUF_SIZE, "grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s\r\n",
             avs->encode_buf, AVS_CLIENT_ID, AVS_CLIENT_SECRET);
    len = strlen(avs->tmp_buf);
    snprintf(avs->http_buf, AVS_HTTP_BUF_SIZE, avs_HTTP_get_refresh_token_template, len);
    strcat(avs->http_buf, avs->tmp_buf);

    wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Sending query:\n%s\n", avs->http_buf);

    result = wiced_tcp_send_buffer(avs->socket_ptr, avs->http_buf, (uint16_t)strlen(avs->http_buf));
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to send HTTP post (%d)\n", result);
        return WICED_ERROR;
    }

    /*
     * Now wait for a response.
     */

    idx           = 0;
    body_ptr      = NULL;
    body_idx      = 0;
    len_to_read   = 0;
    avs->http_idx = 0;
    do
    {
        result = wiced_tcp_receive(avs->socket_ptr, &packet, RECEIVE_TIMEOUT_MS);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error returned from socket receive (%d)\n", result);
            break;
        }

        result = wiced_packet_get_data(packet, 0, &data, &avail_data_length, &total_data_length);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error getting packet data (%d)\n", result);
            return WICED_ERROR;
        }

        if (avs->http_idx + avail_data_length < AVS_HTTP_BUF_SIZE - 1)
        {
            len = avail_data_length;
        }
        else
        {
            len = AVS_HTTP_BUF_SIZE - 1 - avs->http_idx;
        }

        memcpy(&avs->http_buf[avs->http_idx], data, len);
        avs->http_idx += len;

        wiced_packet_delete(packet);
        packet = NULL;

        if (body_ptr == NULL)
        {
            while (idx < avs->http_idx)
            {
                if (avs->http_buf[idx] == avs_HTTP_body_separator[body_idx])
                {
                    if (avs_HTTP_body_separator[++body_idx] == '\0')
                    {
                        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Found end of HTTP header\n");

                        /*
                         * We've found the end of the HTTP header fields. Search for a content length token
                         * so we can know how much data to expect in the body.
                         */

                        body_ptr = &avs->http_buf[idx + 1];
                        ptr      = strcasestr(avs->http_buf, HTTP11_HEADER_CONTENT_LENGTH);
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
                                    len_to_read += (int)(body_ptr - avs->http_buf);
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

        if (len_to_read > 0 && avs->http_idx >= len_to_read)
        {
            break;
        }

    } while (result == WICED_SUCCESS);

    avs->http_buf[avs->http_idx] = '\0';

    if (wiced_log_get_facility_level(WLF_DEF) >= WICED_LOG_DEBUG0)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Received response of %d bytes\n", avs->http_idx);
        for (len = 0; len < avs->http_idx; len++)
        {
            wiced_log_printf("%c", avs->http_buf[len]);
        }
        wiced_log_printf("\n\n");
    }

    wiced_tcp_disconnect(avs->socket_ptr);
    wiced_tcp_delete_socket(avs->socket_ptr);
    avs->socket_ptr = NULL;

    /*
     * Check the response code.
     */

    if (strncasecmp(avs->http_buf, HTTP11_HEADER_STATUS_OK, strlen(HTTP11_HEADER_STATUS_OK)) != 0)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Bad HTTP status returned from server: %.*s\n", 12, avs->http_buf);
        return WICED_ERROR;
    }

    /*
     * Run the content body through the JSON parser.
     */

    if (body_ptr != NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Running the response through the cJSON parser...\n");

        root   = cJSON_Parse(body_ptr);
        access = cJSON_GetObjectItem(root, AVS_TOKEN_ACCESS);
        if (access)
        {
            /*
             * Success. We have a new access token.
             */

            if (avs->avs_access_token)
            {
                free(avs->avs_access_token);
            }
            avs->avs_access_token = strdup(access->valuestring);

            if (avs->avs_access_token != NULL)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Access token: %s\n", access->valuestring);
            }
            else
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error allocating access token\n");
            }
        }

        /*
         * Don't forget to free the JSON tree.
         */

        cJSON_Delete(root);
    }

    return WICED_SUCCESS;
}


static wiced_result_t connect_to_avs(avs_t* avs)
{
    wiced_ip_address_t  ip_address;
    wiced_result_t      result;

    /*
     * Check to see if we're already connected.
     */

    if (avs->state > AVS_STATE_IDLE)
    {
        return WICED_SUCCESS;
    }

    /*
     * Make sure that we have an access token.
     */

    if (avs->avs_access_token == NULL)
    {
        result = avs_get_access_token(avs);

        if (result != WICED_SUCCESS || avs->avs_access_token == NULL)
        {
            return WICED_ERROR;
        }
    }

    /*
     * Who are we connecting to?
     */

    if (uri_split(AVS_BASE_URL, avs) != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to parse URL %s\n", AVS_BASE_URL);
        return WICED_ERROR;
    }

    /*
     * Are we dealing with an IP address or a hostname?
     */

    if (str_to_ip(avs->hostname, &ip_address) != 0)
    {
        /*
         * Try a hostname lookup.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Attemping DNS lookup for %s\n", avs->hostname);
        result = dns_client_hostname_lookup(avs->hostname, &ip_address, DNS_TIMEOUT_MS, WICED_STA_INTERFACE);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "DNS lookup failed (%d)\n", result);
            return WICED_ERROR;
        }
    }

    /*
     * And connect to the server.
     */

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Connecting to " IPV4_PRINT_FORMAT "\n", IPV4_SPLIT_TO_PRINT(ip_address));
    result = http_connect(&avs->http2_connect, &ip_address, avs->port, WICED_STA_INTERFACE);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error connecting to " IPV4_PRINT_FORMAT " (%d)\n", IPV4_SPLIT_TO_PRINT(ip_address), result);
        return result;
    }

    avs->state = AVS_STATE_CONNECTED;

    return WICED_SUCCESS;
}


/*
 * NOTE: This routine uses avs->tmp_buf which will be referenced by the initialized headers.
 */

static wiced_result_t avs_initialize_event_headers(avs_t* avs, http_header_info_t *headers, wiced_bool_t directives_path)
{
    char* method;
    char* path;

    if (directives_path)
    {
        method = "GET";
        path   = AVS_DIRECTIVES_PATH;
    }
    else
    {
        method = "POST";
        path   = AVS_EVENTS_PATH;
    }

    snprintf(avs->tmp_buf, AVS_TMP_BUF_SIZE, "Bearer %s", avs->avs_access_token);

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
    headers[3].value        = (uint8_t*)avs->tmp_buf;
    headers[3].value_length = strlen(avs->tmp_buf);

    if (directives_path)
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


static wiced_result_t avs_send_speech_event(avs_t* avs, wiced_bool_t speech_started)
{
    http_header_info_t source_headers[5];
    http_request_id_t  request;
    wiced_result_t     result;
    char*              event;

    /*
     * Start by creating the event text.
     */

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Sending speech event\n");
    event = avs_create_speech_event_JSON(avs, speech_started ? AVS_TOKEN_SPEECHSTARTED : AVS_TOKEN_SPEECHFINISHED);
    if (event == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error creating speech event json\n");
        return WICED_ERROR;
    }

    /*
     * Create the headers for the message and send them off.
     */

    avs_initialize_event_headers(avs, source_headers, WICED_FALSE);

    result = http_request_create(&avs->http2_connect, &request);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error creating speech event request\n");
        goto _event_end;
    }

    result = http_request_put_headers(&avs->http2_connect, request, source_headers, HTTP_REQUEST_FLAGS_FINISH_HEADERS);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending speech event headers\n");
        goto _event_end;
    }

    avs->http2_stream_id = request;

    /*
     * Now the body of the message.
     */

    snprintf(avs->http_buf, AVS_HTTP_BUF_SIZE, "--%s\r\n%s\r\n\%s\r\n--%s--\r\n", AVS_BOUNDARY_TERM, AVS_JSON_HEADERS, event, AVS_BOUNDARY_TERM);

    result = http_request_put_data(&avs->http2_connect, request, (uint8_t*)avs->http_buf, strlen(avs->http_buf), HTTP_REQUEST_FLAGS_FINISH_REQUEST);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending speech event data\n");
    }

  _event_end:
    if (event != NULL)
    {
        free(event);
    }

    return result;
}


static wiced_result_t avs_synchronize_state(avs_t* avs)
{
    http_header_info_t source_headers[5];
    http_request_id_t  request;
    wiced_result_t     result;

    avs_initialize_event_headers(avs, source_headers, WICED_FALSE);

    result = http_request_create(&avs->http2_connect, &request);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error creating synchronize state request\n");
        return result;
    }

    result = http_request_put_headers(&avs->http2_connect, request, source_headers, HTTP_REQUEST_FLAGS_FINISH_HEADERS);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending synchronize state headers\n");
        return result;
    }

    /*
     * Create the body of our synchronize state message and send it off.
     */

    snprintf(avs->http_buf, AVS_HTTP_BUF_SIZE, "--%s\r\n%s\r\n\%s\r\n--%s--\r\n", AVS_BOUNDARY_TERM, AVS_JSON_HEADERS, AVS_INITIAL_STATE, AVS_BOUNDARY_TERM);

    result = http_request_put_data(&avs->http2_connect, request, (uint8_t*)avs->http_buf, strlen(avs->http_buf), HTTP_REQUEST_FLAGS_FINISH_REQUEST);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending synchronize state data\n");
        return result;
    }

    avs->http2_stream_id = request;
    avs->state           = AVS_STATE_SYNCHRONIZE;

    return WICED_SUCCESS;
}


static wiced_result_t begin_avs_connection(avs_t* avs)
{
    http_header_info_t source_headers[4];
    http_request_id_t  request;
    uint8_t            flags;
    wiced_result_t     result;

    /*
     * Establish the HTTP2 connection to AVS.
     *
     */
    if (connect_to_avs(avs) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /*
     * Send the message to establish the downchannel stream.
     */

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Establishing downchannel stream...\n");

    avs_initialize_event_headers(avs, source_headers, WICED_TRUE);

    result = http_request_create(&avs->http2_connect, &request);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error creating downchannel request\n");
        return result;
    }

    flags  = HTTP_REQUEST_FLAGS_FINISH_HEADERS | HTTP_REQUEST_FLAGS_FINISH_REQUEST;
    result = http_request_put_headers(&avs->http2_connect, request, source_headers, flags);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending downchannel message\n");
        return result;
    }

    avs->http2_stream_id             = request;
    avs->http2_downchannel_stream_id = request;
    avs->state                       = AVS_STATE_DOWNCHANNEL;

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Finished sending downchannel message...\n");
    return WICED_SUCCESS;
}


static wiced_result_t avs_send_speech_request(avs_t* avs)
{
    http_header_info_t      source_headers[5];
    http_request_id_t       request;
    wiced_result_t          result;
    const resource_hnd_t*   resource;
    uint32_t                bytes_total;
    uint32_t                bytes_read;
    uint32_t                size;

    if (avs->state != AVS_STATE_READY)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "AVS in wrong state for speech request\n");
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Starting speech request...\n");

    resource = &resources_apps_DIR_avs_DIR_joke_wav;
    size     = resources_apps_DIR_avs_DIR_joke_wav.size;

    avs_initialize_event_headers(avs, source_headers, WICED_FALSE);

    result = http_request_create(&avs->http2_connect, &request);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error creating speech recognizer request\n");
        return result;
    }

    result = http_request_put_headers(&avs->http2_connect, request, source_headers, HTTP_REQUEST_FLAGS_FINISH_HEADERS);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending speech recognizer headers\n");
        return result;
    }

    avs->http2_stream_id = request;

    /*
     * Create the first part of the body of our recognize speech message and send it off.
     */

    snprintf(avs->http_buf, AVS_HTTP_BUF_SIZE, "--%s\r\n%s\r\n\%s\r\n--%s\r\n%s\r\n",
             AVS_BOUNDARY_TERM, AVS_JSON_HEADERS, AVS_CANNED_SPEECH_REQUEST, AVS_BOUNDARY_TERM, AVS_AUDIO_HEADERS);

    result = http_request_put_data(&avs->http2_connect, request, (uint8_t*)avs->http_buf, strlen(avs->http_buf), 0);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending recognize speech data\n");
        return result;
    }

    /*
     * Now we need to send the audio data.
     * Make sure that we skip the wav header and just send the raw pcm data.
     */

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Reading audio file...\n");
    bytes_total = 44;
    do
    {
        /*
         * Read the next chunk of audio data.
         */

        result = resource_read(resource, bytes_total, 1400, &bytes_read, avs->encode_buf);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error reading audio data\n");
            return result;
        }

        bytes_total += bytes_read;

        /*
         * And send it off.
         */

        result = http_request_put_data(&avs->http2_connect, request, (uint8_t*)avs->encode_buf, bytes_read, 0);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending audio data\n");
            return result;
        }
    } while (bytes_total < size);

    snprintf(avs->http_buf, AVS_HTTP_BUF_SIZE, "--%s--\r\n", AVS_BOUNDARY_TERM);
    result = http_request_put_data(&avs->http2_connect, request, (uint8_t*)avs->http_buf, strlen(avs->http_buf), HTTP_REQUEST_FLAGS_FINISH_REQUEST);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending final audio data\n");
        return result;
    }

    avs->http2_capture_stream_id = request;
    avs->parse_speech_response   = WICED_TRUE;
    avs->discard_capture_stream  = WICED_FALSE;

    return WICED_SUCCESS;
}


static wiced_result_t avs_start_speech_request(avs_t* avs)
{
    http_header_info_t      source_headers[5];
    http_request_id_t       request;
    wiced_result_t          result;

    if (avs->state != AVS_STATE_READY)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "AVS in wrong state for speech request\n");
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Starting speech request...\n");

    avs_initialize_event_headers(avs, source_headers, WICED_FALSE);

    result = http_request_create(&avs->http2_connect, &request);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error creating speech recognizer request\n");
        return result;
    }

    result = http_request_put_headers(&avs->http2_connect, request, source_headers, HTTP_REQUEST_FLAGS_FINISH_HEADERS);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending speech recognizer headers\n");
        return result;
    }

    avs->http2_stream_id = request;

    /*
     * Create the first part of the body of our recognize speech message and send it off.
     */

    snprintf(avs->http_buf, AVS_HTTP_BUF_SIZE, "--%s\r\n%s\r\n\%s\r\n--%s\r\n%s\r\n",
             AVS_BOUNDARY_TERM, AVS_JSON_HEADERS, AVS_SPEECH_REQUEST, AVS_BOUNDARY_TERM, AVS_AUDIO_HEADERS);

    result = http_request_put_data(&avs->http2_connect, request, (uint8_t*)avs->http_buf, strlen(avs->http_buf), 0);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending recognize speech data\n");
        return result;
    }

    if (avs_audio_rx_capture_enable(avs, WICED_TRUE) != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to start audio capture\n");
        return WICED_ERROR;
    }

    avs->pcm_process_bufs        = WICED_TRUE;
    avs->parse_speech_response   = WICED_TRUE;
    avs->discard_capture_stream  = WICED_FALSE;
    avs->http2_capture_stream_id = request;

    avs->state = AVS_STATE_SPEECH_REQUEST;

    return WICED_SUCCESS;
}


static wiced_result_t process_http_response(avs_t* avs, uint32_t stream_id, uint32_t response_status)
{
    wiced_result_t result = WICED_SUCCESS;

    if (response_status != HTTP_STATUS_OK && response_status != HTTP_STATUS_NO_CONTENT)
    {
        /*
         * Something went wrong. Note that we don't try and recover from all error states
         * as this is a demonstration application. At this point we're probably dead in the
         * water until someone checks the debug output to see what went wrong.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Bad response (%lu) for stream %lu\n", response_status, stream_id);
        return WICED_ERROR;
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Stream %lu received a response status of %lu\n", stream_id, response_status);

    switch (avs->state)
    {
        case AVS_STATE_DOWNCHANNEL:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Sending initial synchronize state...\n");
            result = avs_synchronize_state(avs);
            break;

        case AVS_STATE_SYNCHRONIZE:
            avs->state = AVS_STATE_READY;
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Connection to AVS established\n");

            /*
             * Start the timer for our PING requests.
             */

            if (wiced_rtos_is_timer_running(&avs->timer) != WICED_SUCCESS)
            {
                wiced_rtos_start_timer(&avs->timer);
            }
            break;

        case AVS_STATE_SPEECH_REQUEST:
            /*
             * We've started to receive the response to our speech request.
             */
            if ( response_status == HTTP_STATUS_OK )
            {
                avs->state = AVS_STATE_PROCESSING_RESPONSE;
            }
            else
            {
                avs->state = AVS_STATE_READY;
            }
            break;

        case AVS_STATE_READY:
        default:
            wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "No action to take\n");
            break;
    }

    return result;
}


static void process_timer_event(avs_t* avs)
{
    uint8_t ping_data[HTTP2_PING_DATA_LENGTH];
    wiced_result_t result;

    if (avs->state < AVS_STATE_READY)
    {
        /*
         * The connection is not established. Nothing to do.
         */

        if (wiced_rtos_is_timer_running(&avs->timer) == WICED_SUCCESS)
        {
            wiced_rtos_stop_timer(&avs->timer);
        }
        return;
    }

    /*
     * Send out a ping to let AVS know that we're still alive. If we wanted to be really good,
     * we'd check to make sure that we get a response.
     */

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Send ping message.\n");
    memset(ping_data, 0, HTTP2_PING_DATA_LENGTH);
    result = http_ping_request(&avs->http2_connect, ping_data, HTTP_REQUEST_FLAGS_PING_REQUEST);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error sending PING request (%d)\n", result);
    }
}


static void process_disconnect_event(avs_t* avs)
{
    if (avs->state == AVS_STATE_IDLE)
    {
        /*
         * Already in idle state.
         */

        return;
    }

    /*
     * Stop the timer if it is running.
     */

    if (wiced_rtos_is_timer_running(&avs->timer) == WICED_SUCCESS)
    {
        wiced_rtos_stop_timer(&avs->timer);
    }

    /*
     * Make sure that HTTP2 is disconnected.
     */

    http_disconnect(&avs->http2_connect);

    /*
     * If we were in the middle of processing when we disconnected, make sure that
     * the audio subsystems aren't still active.
     */

    if (avs->state > AVS_STATE_READY)
    {
        avs_audio_rx_capture_enable(avs, WICED_FALSE);
        audio_client_ioctl(avs->audio_client_handle, AUDIO_CLIENT_IOCTL_STOP, NULL);
    }

    avs->state = AVS_STATE_IDLE;

    /*
     * Let's try and make a new connection.
     * Assume that we disconnected after an hour so we'll need to generate a new access token.
     */

    if (avs->avs_access_token)
    {
        free(avs->avs_access_token);
        avs->avs_access_token = NULL;
    }
    wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_AVS);
}


static void process_pcm_data(avs_t* avs)
{
    pcm_buf_t* pcmbuf;
    wiced_result_t result;

    /*
     * Grab the next buffer of data.
     */

    pcmbuf = &avs->pcm_bufs[avs->pcm_read_idx];
    if (!pcmbuf->inuse)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "No PCM buffer available\n");
        return;
    }

    if (avs->pcm_process_bufs)
    {
        result = http_request_put_data(&avs->http2_connect, avs->http2_capture_stream_id, pcmbuf->buf, pcmbuf->buflen, 0);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error writing audio data\n");
        }
    }

    pcmbuf->inuse  = 0;
    pcmbuf->buflen = 0;
    avs->pcm_read_idx = (avs->pcm_read_idx + 1) % AVS_NUM_PCM_BUFS;

    /*
     * Another buffer to process?
     */

    if (avs->pcm_bufs[avs->pcm_read_idx].inuse)
    {
        wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_PCM_AUDIO_DATA);
    }
}


static wiced_result_t process_directive(avs_t* avs, objhandle_t hbuf)
{
    cJSON* root;
    cJSON* header;
    cJSON* node;
    char* buf;
    char* ptr;
    uint32_t size;
    wiced_result_t result = WICED_SUCCESS;

    if (hbuf == NULL)
    {
        return WICED_ERROR;
    }

    /*
     * Get a pointer to the data.
     */

    bufmgr_buf_get_data(hbuf, &buf, &size);

    /*
     * And isolate the JSON portion of the message.
     */

    ptr = avs_find_JSON_start(buf, size, avs->msg_boundary_downchannel);
    if (ptr == NULL)
    {
        result = WICED_ERROR;
        goto _end_directive;
    }

    /*
     * Feed the text through the JSON parser.
     */

    root = cJSON_Parse(ptr);

    if (wiced_log_get_facility_level(WLF_DEF) >= WICED_LOG_DEBUG0)
    {
        ptr = cJSON_Print(root);
        wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Directive JSON:\n%s\n", ptr);
        free(ptr);
    }

    header = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_HEADER);
    node   = cJSON_GetObjectItem(header, AVS_TOKEN_NAMESPACE);
    if (header == NULL || node == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Not a valid directive\n");
        result = WICED_ERROR;
        goto _end_directive;
    }

    /*
     * SpeechRecognizer directive?
     */

    if (strncmp(node->valuestring, AVS_TOKEN_SPEECHRECOGNIZER, sizeof(AVS_TOKEN_SPEECHRECOGNIZER) - 1) == 0)
    {
        node = cJSON_GetObjectItem(header, AVS_TOKEN_NAME);
        if (node && strncmp(node->valuestring, AVS_TOKEN_STOPCAPTURE, sizeof(AVS_TOKEN_STOPCAPTURE) - 1) == 0)
        {
            /*
             * We need to stop any active capture and finish off any HTTP request.
             */

            avs->pcm_process_bufs = WICED_FALSE;
            if (avs_audio_rx_capture_enable(avs, WICED_FALSE) != WICED_SUCCESS)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to stop audio capture\n");
            }

            if (avs->http2_stream_id)
            {
                snprintf(avs->http_buf, AVS_HTTP_BUF_SIZE, "--%s--\r\n", AVS_BOUNDARY_TERM);
                result = http_request_put_data(&avs->http2_connect, avs->http2_stream_id, (uint8_t*)avs->http_buf,
                                               strlen(avs->http_buf), HTTP_REQUEST_FLAGS_FINISH_REQUEST);
                if (result != WICED_SUCCESS)
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error completing SpeechRecognizer request\n");
                }
            }
        }
    }

    /*
     * Don't forget to free the JSON tree.
     */

    cJSON_Delete(root);

  _end_directive:
    bufmgr_buf_free(hbuf);
    return result;
}


static wiced_result_t process_speech_synthesizer_message(avs_t* avs, objhandle_t hbuf)
{
    wiced_result_t result = WICED_SUCCESS;
    cJSON* root = NULL;
    cJSON* header;
    cJSON* payload;
    cJSON* token;
    cJSON* node;
    char* json_text;
    char* ptr;
    char* buf;
    uint32_t size;
    int idx;
    audio_client_push_buf_t ac_push_buf;

    if (avs->discard_capture_stream)
    {
        /*
         * We didn't like the directive we received in response to the speech request.
         * Just discard the buffer and return.
         */

        if (hbuf)
        {
            bufmgr_buf_free(hbuf);
        }
        else
        {
            /*
             * We're all done receiving for this stream. Go back to the ready state.
             */

            wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Last discard buffer. Return to READY state.\n");

            avs->state = AVS_STATE_READY;
        }

        return WICED_SUCCESS;
    }

    if (hbuf == NULL)
    {
        /*
         * We're all done. Do whatever we need to do signal end of playback.
         */

        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Received entire speech event\n");

        if (avs->state == AVS_STATE_PLAYING_RESPONSE)
        {
            ac_push_buf.buf    = NULL;
            ac_push_buf.buflen = 0;
            result = audio_client_ioctl(avs->audio_client_handle, AUDIO_CLIENT_IOCTL_PUSH_BUFFER, &ac_push_buf);
            if (result != WICED_SUCCESS)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "audio_client_ioctl(PUSH_BUFFER) failed with %d\n", result);
                audio_client_ioctl(avs->audio_client_handle, AUDIO_CLIENT_IOCTL_STOP, NULL);
            }

            avs_send_speech_event(avs, WICED_FALSE);

            if (result != WICED_SUCCESS)
            {
                avs->state = AVS_STATE_READY;
            }
            else
            {
                avs->state = AVS_STATE_AWAITING_RESPONSE_EOS;
            }
        }
        else
        {
            avs->state = AVS_STATE_READY;
        }

        return WICED_SUCCESS;
    }

    /*
     * Get a pointer to the data.
     */

    bufmgr_buf_get_data(hbuf, &buf, &size);

    /*
     * Do we need to parse the response?
     */

    if (avs->parse_speech_response)
    {
        /*
         * We assume that we get the entire JSON directive is contained within the first buffer.
         */

        ptr = avs_find_JSON_start(buf, size, avs->msg_boundary_speech);
        if (ptr == NULL)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to find start of speech directive\n");
            result = WICED_ERROR;
            goto _end_syn;
        }

        /*
         * Feed the text through the JSON parser.
         */

        root = cJSON_Parse(ptr);
        if (root == NULL)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to parse JSON directive\n");
            result = WICED_ERROR;
            goto _end_syn;
        }

        if (wiced_log_get_facility_level(WLF_DEF) >= WICED_LOG_DEBUG0)
        {
            json_text = cJSON_Print(root);
            wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Directive JSON:\n%s\n", json_text);
            free(json_text);
        }

        /*
         * Verify that we're receiving what we expect to receive.
         */

        header = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_HEADER);
        node   = cJSON_GetObjectItem(header, AVS_TOKEN_NAMESPACE);

        if (header == NULL || node == NULL)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Not a valid directive\n");
            result = WICED_ERROR;
            goto _end_syn;
        }

        /*
         * SpeechSynthesizer directive?
         */

        if (strncmp(node->valuestring, AVS_TOKEN_SPEECHSYNTHESIZER, sizeof(AVS_TOKEN_SPEECHSYNTHESIZER) - 1) == 0)
        {
            payload = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_PAYLOAD);
            node    = cJSON_GetObjectItem(payload, AVS_TOKEN_FORMAT);
            token   = cJSON_GetObjectItem(payload, AVS_TOKEN_TOKEN);

            if (payload == NULL || node == NULL)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to find audio format\n");
                result = WICED_ERROR;
                goto _end_syn;
            }

            if (strncmp(node->valuestring, AVS_TOKEN_AUDIO_MPEG, sizeof(AVS_TOKEN_AUDIO_MPEG) - 1) != 0)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unknown audio format: %s\n", node->valuestring);
                result = WICED_ERROR;
                goto _end_syn;
            }

            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "MP3 audio format\n");

            /*
             * Grab the token for use in the response messages.
             */

            if (token != NULL)
            {
                if (avs->token_speak != NULL)
                {
                    free(avs->token_speak);
                }
                if ((avs->token_speak = strdup(token->valuestring)) == NULL)
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to allocate speak token\n");
                }
            }
        }
        else
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Not a SpeechSynthesizer directive\n");
            if (strncmp(node->valuestring, AVS_TOKEN_SYSTEM, sizeof(AVS_TOKEN_SYSTEM) - 1) == 0)
            {
                node = cJSON_GetObjectItem(header, AVS_TOKEN_NAME);
                if (node != NULL && strncmp(node->valuestring, AVS_TOKEN_SETENDPOINT, sizeof(AVS_TOKEN_SETENDPOINT) - 1) == 0)
                {
                    payload = cJSON_GetObjectItem(cJSON_GetObjectItem(root, AVS_TOKEN_DIRECTIVE), AVS_TOKEN_PAYLOAD);
                    node    = cJSON_GetObjectItem(payload, AVS_TOKEN_ENDPOINT);
                    if (payload != NULL && node != NULL)
                    {
                        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "SetEndPoint directive received.\nSet AVS_BASE_URL to %s and recompile application.\n", node->valuestring);
                    }
                }
            }
            avs->discard_capture_stream = WICED_TRUE;
            result = WICED_ERROR;
            goto _end_syn;
        }

        /*
         * Release the JSON tree.
         */

        cJSON_Delete(root);
        root = NULL;

        /*
         * All done processing the header information.
         */

        avs->parse_speech_response = WICED_FALSE;

        /*
         * Audio playback: set codec
         */

        result = audio_client_ioctl(avs->audio_client_handle, AUDIO_CLIENT_IOCTL_SET_CODEC, (uint32_t*)AUDIO_CLIENT_CODEC_MP3);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "audio_client_ioctl(SET_CODEC) failed with %d\n", result);
        }
        else
        {
            avs->state = AVS_STATE_PLAYING_RESPONSE;
            avs_send_speech_event(avs, WICED_TRUE);
        }

        /*
         * Let's find the start of the audio data.
         */

        ptr = strnstr(ptr, (uint16_t)(size - (uint32_t)(ptr - buf)), avs->msg_boundary_speech, strlen(avs->msg_boundary_speech));
        if (ptr == NULL || ptr >= &buf[size])
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to find audio message boundary\n");
            result = WICED_ERROR;
            goto _end_syn;
        }

        ptr += strlen(avs->msg_boundary_speech);
        idx = 0;
        while (ptr < &buf[size])
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

        if (ptr >= &buf[size])
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to find start of audio data\n");
            result = WICED_ERROR;
            goto _end_syn;
        }

        size = size - ((uint32_t)ptr - (uint32_t)buf);
        buf = ptr;
    }

#if AVS_DEBUG_CAPTURE
    {
        uint32_t len;

        /*
         * Save the information in our capture buffer. This lets us grab it using the debugger.
         */

        len = size;
        if (avs->capture_idx + size >= AVS_CAPTURE_BUF_SIZE)
        {
            len = AVS_CAPTURE_BUF_SIZE - avs->capture_idx;
        }
        memcpy(&avs->capture[avs->capture_idx], buf, len);
        avs->capture_idx += len;
    }
#endif

    /*
     * Audio playback: push data chunk
     */

    if (avs->state == AVS_STATE_PLAYING_RESPONSE)
    {
        ac_push_buf.buf    = (uint8_t *)buf;
        ac_push_buf.buflen = size;
        /* initialize pushedlen at 0; audio_client will update it */
        ac_push_buf.pushedlen = 0;

        do
        {
            result = audio_client_ioctl(avs->audio_client_handle, AUDIO_CLIENT_IOCTL_PUSH_BUFFER, &ac_push_buf);
            if (result != WICED_SUCCESS)
            {
                if (result == WICED_PENDING)
                {
                    wiced_rtos_delay_milliseconds(5);
                }
                else
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "audio_client_ioctl(PUSH_BUFFER) failed with %d\n", result);
                    break;
                }
            }
        } while (result != WICED_SUCCESS);
    }

  _end_syn:
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    bufmgr_buf_free(hbuf);
    return result;
}


static void process_msg_queue(avs_t* avs)
{
    avs_msg_t msg;
    wiced_result_t result;

    result = wiced_rtos_pop_from_queue(&avs->msgq, &msg, WICED_NO_WAIT);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error reading message from queue\n");
        return;
    }

    switch (msg.type)
    {
        case AVS_MSG_TYPE_HTTP_RESPONSE:
            process_http_response(avs, msg.arg1, msg.arg2);
            break;

        case AVS_MSG_TYPE_AVS_DIRECTIVE:
            process_directive(avs, (objhandle_t)msg.arg2);
            break;

        case AVS_MSG_TYPE_SPEECH_SYNTHESIZER:
            process_speech_synthesizer_message(avs, (objhandle_t)msg.arg2);
            break;

        default:
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unexpected msg %d\n", msg.type);
            break;
    }

    /*
     * If there's another message waiting, make sure that the proper event flag is set.
     */

    if (wiced_rtos_is_queue_empty(&avs->msgq) != WICED_SUCCESS)
    {
        wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_MSG_QUEUE);
    }
}


static void avs_mainloop(avs_t* avs)
{
    wiced_result_t      result;
    uint32_t            events;

    wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "Begin avs mainloop\n");

    /*
     * Start the connection to AVS automagically.
     */

    wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_AVS);

    while (avs->tag == AVS_TAG_VALID)
    {
        events = 0;

        result = wiced_rtos_wait_for_event_flags(&avs->events, AVS_ALL_EVENTS, &events, WICED_TRUE, WAIT_FOR_ANY_EVENT, WICED_WAIT_FOREVER);
        if (result != WICED_SUCCESS)
        {
            continue;
        }

        if (events & AVS_EVENT_SHUTDOWN)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "mainloop received EVENT_SHUTDOWN\n");
            break;
        }

        if (events & AVS_EVENT_AVS)
        {
            begin_avs_connection(avs);
        }

        if (events & AVS_EVENT_SPEECH_REQUEST)
        {
            avs_send_speech_request(avs);
        }

        if (events & AVS_EVENT_MSG_QUEUE)
        {
            process_msg_queue(avs);
        }

        if (events & AVS_EVENT_TIMER)
        {
            process_timer_event(avs);
        }

        if (events & AVS_EVENT_DISCONNECT)
        {
            process_disconnect_event(avs);
        }

        if (events & AVS_EVENT_CAPTURE)
        {
            avs_start_speech_request(avs);
        }

        if (events & AVS_EVENT_PCM_AUDIO_DATA)
        {
            process_pcm_data(avs);
        }

        if (events & AVS_EVENT_VOLUME_UP)
        {
            avs_audio_playback_volume_up(avs);
        }

        if (events & AVS_EVENT_VOLUME_DOWN)
        {
            avs_audio_playback_volume_down(avs);
        }

        if (events & AVS_EVENT_RELOAD_DCT_WIFI)
        {
            avs_config_reload_dct_wifi(&avs->dct_tables);
        }

        if (events & AVS_EVENT_RELOAD_DCT_NETWORK)
        {
            avs_config_reload_dct_network(&avs->dct_tables);
        }
    }   /* while */

    wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "End avs mainloop\n");
}


static void avs_shutdown(avs_t* avs)
{
    /*
     * Shutdown HTTP2
     */

    http_connection_deinit(&avs->http2_connect);

    avs_keypad_deinit(avs);

    /*
     * Shutdown the console.
     */

    command_console_deinit();

    avs->tag = AVS_TAG_INVALID;

    avs_audio_playback_deinit(avs);

    avs_audio_rx_thread_stop(avs);

    wiced_rtos_deinit_event_flags(&avs->events);
    wiced_rtos_deinit_timer(&avs->timer);
    wiced_rtos_deinit_queue(&avs->msgq);

    avs_config_deinit(&avs->dct_tables);

    if (avs->socket_ptr != NULL)
    {
        wiced_tcp_delete_socket(avs->socket_ptr);
        avs->socket_ptr = NULL;
    }

    if (avs->test_uri)
    {
        free(avs->test_uri);
        avs->test_uri = NULL;
    }

    if (avs->avs_access_token)
    {
        free(avs->avs_access_token);
        avs->avs_access_token = NULL;
    }

    if (avs->msg_boundary_downchannel != NULL)
    {
        free(avs->msg_boundary_downchannel);
        avs->msg_boundary_downchannel = NULL;
    }

    if (avs->msg_boundary_speech != NULL)
    {
        free(avs->msg_boundary_speech);
        avs->msg_boundary_speech = NULL;
    }

    if (avs->token_speak != NULL)
    {
        free(avs->token_speak);
        avs->token_speak = NULL;
    }

    if (avs->current_handle != NULL)
    {
        bufmgr_buf_free(avs->current_handle);
        avs->current_handle = NULL;
    }

    if (avs->buf_pool != NULL)
    {
        bufmgr_pool_destroy(avs->buf_pool);
        avs->buf_pool = NULL;
    }

    free(avs);
}


static avs_t* avs_init(void)
{
    http_connection_callbacks_t callbacks;
    avs_t*                      avs;
    wiced_time_t                time;
    wiced_result_t              result;
    uint32_t                    tag;

    tag = AVS_TAG_VALID;

    /* Initialize the device */
    result = wiced_init();
    if (result != WICED_SUCCESS)
    {
        return NULL;
    }

    /* initialize audio */
    platform_init_audio();

    /*
     * Initialize the logging subsystem.
     */

    wiced_log_init(WICED_LOG_INFO, avs_log_output_handler, NULL);

    /*
     * Allocate the main player structure.
     */

    avs = calloc_named("avs", 1, sizeof(avs_t));
    if (avs == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to allocate app structure\n");
        return NULL;
    }

    /*
     * Create the command console.
     */

    wiced_log_printf("Start the command console\n");
    result = command_console_init(STDIO_UART, sizeof(avs_command_buffer), avs_command_buffer,
                                  AVS_CONSOLE_COMMAND_HISTORY_LENGTH, avs_command_history_buffer, " ");
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error starting the command console\n");
        free(avs);
        return NULL;
    }
    console_add_cmd_table(avs_command_table);
    console_dct_register_callback(avs_console_dct_callback, avs);

    /*
     * Initialize keypad / button manager
     */

    result = avs_keypad_init(avs);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing keypad\n");
    }

    /*
     * Create our event flags, timer, and message queue.
     */

    result = wiced_rtos_init_event_flags(&avs->events);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing event flags\n");
        tag = AVS_TAG_INVALID;
    }

    result = wiced_rtos_init_timer(&avs->timer, PING_TIMER_PERIOD_MSECS, timer_callback, avs);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing timer\n");
        tag = AVS_TAG_INVALID;
    }

    result = wiced_rtos_init_queue(&avs->msgq, NULL, sizeof(avs_msg_t), AVS_MSG_QUEUE_SIZE);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing message queue\n");
        tag = AVS_TAG_INVALID;
    }

    /* read in our configurations */
    avs_config_init(&avs->dct_tables);

    /* print out our current configuration */
    avs_config_print_info(&avs->dct_tables);

    /*
     * Create the buffer pool.
     */

    if (bufmgr_pool_create(&avs->buf_pool, "avs_bufs", AVS_BUFMGR_BUF_SIZE,
                           AVS_BUFMSG_NUM_BUFS, AVS_BUFMSG_NUM_BUFS, 0, sizeof(uint32_t)) != BUFMGR_OK)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "bufmgr_pool_create failed\n");
        tag = AVS_TAG_INVALID;
    }

    /*
     * Create the audio capture thread.
     */

    result = avs_audio_rx_thread_start(avs);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing audio rx\n");
        tag = AVS_TAG_INVALID;
    }

    /*
     * Initialize the HTTP2 library.
     */

    memset(&avs->http2_security, 0, sizeof(http_security_info_t));

    callbacks.http_disconnect_callback          = http2_disconnect_event_callback;
    callbacks.http_request_recv_header_callback = http2_recv_header_callback;
    callbacks.http_request_recv_data_callback   = http2_recv_data_callback;

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Initializing HTTP2 connection to AVS\n");
    result = http_connection_init(&avs->http2_connect, &avs->http2_security, &callbacks, avs->http2_scratch, sizeof(avs->http2_scratch), avs);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing HTTP2 connection (%d)\n", result);
        tag = AVS_TAG_INVALID;
    }

    /* Bring up the network interface */
    result = wiced_network_up_default(&avs->dct_tables.dct_network->interface, NULL);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Bringing up network interface failed\n");
        tag = AVS_TAG_INVALID;
    }

    result = avs_audio_playback_init(avs);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing audio tx\n");
    }

    /*
     * Seed the random number generator here. We're relying on the fact that the
     * network initialization takes a (slightly) variable amount of time.
     * Then get the base numbers for our message and dialog IDs.
     */

    wiced_time_get_time(&time);
    srand(time);

    avs->message_id = rand();
    avs->dialog_id  = rand();

    /* set our valid tag */
    avs->tag = tag;

    return avs;
}


void application_start(void)
{
    avs_t* avs;

    /*
     * Main initialization.
     */

    if ((avs = avs_init()) == NULL)
    {
        return;
    }
    g_avs = avs;

    if (avs->tag != AVS_TAG_VALID)
    {
        /*
         * We didn't initialize successfully. Bail out here so that the console
         * will remain active. This lets the user correct any invalid configuration parameters.
         * Mark the structure as valid so the console command processing will work and
         * note that we are intentionally leaking the avs structure memory here.
         */

        avs->tag = AVS_TAG_VALID;
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Main application thread exiting...command console still active\n");
        return;
    }

    /*
     * Drop into our main loop.
     */

    avs_mainloop(avs);

    /*
     * Cleanup and exit.
     */

    g_avs = NULL;
    avs_shutdown(avs);
    avs = NULL;
}
