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
 * AVS Application Audio Playback Support
 */

#include "avs_app.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AUDIO_PLAYER_DATA_PREROLL               ( 0 )
#define AUDIO_PLAYER_NUM_HTTP_BUFFERS           ( 60 )
#define AUDIO_PLAYER_NUM_AUDIO_BUFFERS          ( 20 )
#define AUDIO_PLAYER_SIZE_AUDIO_BUFFERS         ( 2048 )
#define AUDIO_PLAYER_DATA_THRESH_HIGH           ( 0 )
#define AUDIO_PLAYER_DATA_THRESH_LOW            ( 0 )

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


static wiced_result_t avs_app_ac_event_cbf(audio_client_ref handle, void* userdata, AUDIO_CLIENT_EVENT_T event, void* arg)
{
    wiced_result_t          result   = WICED_SUCCESS;
    avs_app_t*              app = (avs_app_t*)userdata;
    wiced_audio_config_t*   config;
    avs_app_msg_t           msg;

    UNUSED_PARAMETER(handle);

    switch (event)
    {
        case AUDIO_CLIENT_EVENT_ERROR:
            if ((int)arg == AUDIO_CLIENT_ERROR_BAD_CODEC)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unsupported audio codec\n");
            }
            else
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error from audio_client: %d\n", (int)arg);
            }
            wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_PLAYBACK_ERROR);
            break;

        case AUDIO_CLIENT_EVENT_AUDIO_FORMAT:
            config = (wiced_audio_config_t*)arg;
            if ( config == NULL )
            {
                result = WICED_BADARG;
                break;
            }

            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Audio format:\n");
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "    num channels: %u\n", config->channels);
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, " bits per sample: %u\n", config->bits_per_sample);
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "     sample rate: %lu\n", config->sample_rate);
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "      frame size: %u\n", config->frame_size);
            break;

        case AUDIO_CLIENT_EVENT_PLAYBACK_STARTED:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has reached STARTED\n");
            break;

        case AUDIO_CLIENT_EVENT_PLAYBACK_PAUSED:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has reached PAUSED\n");
            break;

        case AUDIO_CLIENT_EVENT_PLAYBACK_STOPPED:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has reached STOPPED\n");
            /* falling through */

        case AUDIO_CLIENT_EVENT_PLAYBACK_EOS:
            if (event == AUDIO_CLIENT_EVENT_PLAYBACK_EOS)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has reached EOS\n");
            }
            wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_PLAYBACK_COMPLETE);
            break;

        case AUDIO_CLIENT_EVENT_DECODE_COMPLETE:
            wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "audio_client has reached DECODE complete\n");
            break;

        case AUDIO_CLIENT_EVENT_DATA_THRESHOLD_HIGH:
            wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "audio_client has hit HIGH data threshold\n");
            break;

        case AUDIO_CLIENT_EVENT_DATA_THRESHOLD_LOW:
            wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "audio_client has hit LOW data threshold\n");
            break;

        case AUDIO_CLIENT_EVENT_PLAYLIST_MIME_TYPE:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client found playlist mime type: %s\n", ((audio_client_playlist_t*)arg)->mime_type);
            app->playlist_type = ((audio_client_playlist_t*)arg)->playlist_type;
            break;

        case AUDIO_CLIENT_EVENT_HTTP_COMPLETE:
            wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "audio_client returned playlist:\n%s\n", (char*)arg);
            msg.type = AVS_APP_MSG_PLAYLIST_LOADED;
            msg.arg1 = (uint32_t)arg;
            if (wiced_rtos_push_to_queue(&app->msgq, &msg, MSGQ_PUSH_TIMEOUT_MS) == WICED_SUCCESS)
            {
                wiced_rtos_set_event_flags(&app->events, AVS_APP_EVENT_MSG_QUEUE);
            }
            else
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error pushing playlist msg\n");
                free(arg);
            }
            break;

        default:
            break;
    }

    return result;
}


wiced_result_t avs_app_audio_playback_init(avs_app_t* app)
{
    const platform_audio_device_info_t* audio_device;
    wiced_result_t                      result              = WICED_SUCCESS;
    audio_client_params_t               audio_client_params;

    memset(&audio_client_params, 0, sizeof(audio_client_params));
    audio_client_params.enable_playback             = WICED_TRUE;
    audio_client_params.event_cb                    = avs_app_ac_event_cbf;
    audio_client_params.userdata                    = app;
    audio_client_params.interface                   = app->dct_tables.dct_network->interface;
    audio_client_params.load_playlist_files         = WICED_TRUE;
    audio_client_params.data_buffer_preroll         = AUDIO_PLAYER_DATA_PREROLL;
    audio_client_params.data_buffer_num             = AUDIO_PLAYER_NUM_HTTP_BUFFERS;
    audio_client_params.audio_buffer_num            = AUDIO_PLAYER_NUM_AUDIO_BUFFERS;
    audio_client_params.audio_buffer_size           = AUDIO_PLAYER_SIZE_AUDIO_BUFFERS;
    audio_client_params.data_threshold_high         = AUDIO_PLAYER_DATA_THRESH_HIGH;
    audio_client_params.data_threshold_low          = AUDIO_PLAYER_DATA_THRESH_LOW;
    audio_client_params.device_id                   = app->dct_tables.dct_app->audio_device_tx;
    audio_client_params.volume                      = app->dct_tables.dct_app->volume;
    audio_client_params.buffer_get                  = NULL;
    audio_client_params.buffer_release              = NULL;
    audio_client_params.no_length_disable_preroll   = WICED_TRUE;
    audio_client_params.high_threshold_read_inhibit = WICED_FALSE;
    audio_client_params.release_device_on_stop      = WICED_TRUE;

    audio_device = platform_audio_device_get_info_by_id(app->dct_tables.dct_app->audio_device_tx);
    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Initialize audio PLAYBACK device: %s\n", audio_device ? audio_device->device_name : "");
    app->audio_client_handle = audio_client_init(&audio_client_params);
    if (app->audio_client_handle == NULL)
    {
        result = WICED_ERROR;
    }
    else
    {
        app->audio_client_volume = audio_client_params.volume;
    }

    return result;
}


wiced_result_t avs_app_audio_playback_deinit(avs_app_t* app)
{
    wiced_result_t result;

    result = audio_client_deinit(app->audio_client_handle);
    app->audio_client_handle = NULL;

    return result;
}
