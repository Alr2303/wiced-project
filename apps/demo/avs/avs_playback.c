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
 * AVS Test Audio Playback Support
 */

#include "avs.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AUDIO_PLAYER_DATA_PREROLL               ( 0 )
#define AUDIO_PLAYER_NUM_HTTP_BUFFERS           ( 120 )
#define AUDIO_PLAYER_NUM_AUDIO_BUFFERS          ( 4 )
#define AUDIO_PLAYER_SIZE_AUDIO_BUFFERS         ( 2048 )
#define AUDIO_PLAYER_DATA_THRESH_HIGH           ( 0 )
#define AUDIO_PLAYER_DATA_THRESH_LOW            ( 0 )

#define AUDIO_PLAYER_VOLUME_STEP                ( 10 )
#define AUDIO_PLAYER_VOLUME_MAX                 ( AVS_VOLUME_MAX )
#define AUDIO_PLAYER_VOLUME_MIN                 ( AVS_VOLUME_MIN )

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


static wiced_result_t avs_ac_event_cbf(audio_client_ref handle, void* userdata, AUDIO_CLIENT_EVENT_T event, void* arg)
{
    wiced_result_t          result   = WICED_SUCCESS;
    avs_t*                  avs = (avs_t*)userdata;
    wiced_audio_config_t*   config   = NULL;

    UNUSED_PARAMETER(handle);

    switch (event)
    {
        case AUDIO_CLIENT_EVENT_ERROR:
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error from audio_client: %d\n", (int)arg);
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
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has reached STARTED !\n");
            break;

        case AUDIO_CLIENT_EVENT_PLAYBACK_PAUSED:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has reached PAUSED !\n");
            break;

        case AUDIO_CLIENT_EVENT_PLAYBACK_STOPPED:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has reached STOPPED !\n");
            /* falling through */

        case AUDIO_CLIENT_EVENT_PLAYBACK_EOS:
            if ( event == AUDIO_CLIENT_EVENT_PLAYBACK_EOS )
            {
                if (avs->state == AVS_STATE_AWAITING_RESPONSE_EOS)
                {
                    avs->state = AVS_STATE_READY;
                }
                wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has reached EOS !\n");
            }
            break;

        case AUDIO_CLIENT_EVENT_DATA_THRESHOLD_HIGH:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has hit HIGH data threshold !\n");
            break;

        case AUDIO_CLIENT_EVENT_DATA_THRESHOLD_LOW:
            wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "audio_client has hit LOW data threshold !\n");
            break;

        default:
            break;
    }

    return result;
}


wiced_result_t avs_audio_playback_init(avs_t* avs)
{
    const platform_audio_device_info_t* audio_device;
    wiced_result_t                      result              = WICED_SUCCESS;
    audio_client_params_t               audio_client_params;

    memset(&audio_client_params, 0, sizeof(audio_client_params));
    audio_client_params.enable_playback             = WICED_TRUE;
    audio_client_params.event_cb                    = avs_ac_event_cbf;
    audio_client_params.userdata                    = avs;
    audio_client_params.interface                   = avs->dct_tables.dct_network->interface;
    audio_client_params.data_buffer_preroll         = AUDIO_PLAYER_DATA_PREROLL;
    audio_client_params.data_buffer_num             = AUDIO_PLAYER_NUM_HTTP_BUFFERS;
    audio_client_params.audio_buffer_num            = AUDIO_PLAYER_NUM_AUDIO_BUFFERS;
    audio_client_params.audio_buffer_size           = AUDIO_PLAYER_SIZE_AUDIO_BUFFERS;
    audio_client_params.data_threshold_high         = AUDIO_PLAYER_DATA_THRESH_HIGH;
    audio_client_params.data_threshold_low          = AUDIO_PLAYER_DATA_THRESH_LOW;
    audio_client_params.device_id                   = avs->dct_tables.dct_app->audio_device_tx;
    audio_client_params.volume                      = avs->dct_tables.dct_app->volume;
    audio_client_params.buffer_get                  = NULL;
    audio_client_params.buffer_release              = NULL;
    audio_client_params.no_length_disable_preroll   = WICED_TRUE;
    audio_client_params.high_threshold_read_inhibit = WICED_FALSE;
    audio_client_params.release_device_on_stop      = WICED_TRUE;

    audio_device = platform_audio_device_get_info_by_id(avs->dct_tables.dct_app->audio_device_tx);
    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Initialize audio PLAYBACK device: %s\n", audio_device ? audio_device->device_name : "");
    avs->audio_client_handle = audio_client_init(&audio_client_params);
    if (avs->audio_client_handle == NULL)
    {
        result = WICED_ERROR;
    }
    else
    {
        avs->audio_client_volume = audio_client_params.volume;
    }

    return result;
}


wiced_result_t avs_audio_playback_deinit(avs_t* avs)
{
    wiced_result_t result;

    result = audio_client_deinit(avs->audio_client_handle);
    avs->audio_client_handle = NULL;

    return result;
}


wiced_result_t avs_audio_playback_volume_up(avs_t* avs)
{
    wiced_result_t result = WICED_ERROR;

    if (avs->audio_client_handle == NULL)
    {
        return result;
    }

    avs->audio_client_volume += AUDIO_PLAYER_VOLUME_STEP;
    if (avs->audio_client_volume > AUDIO_PLAYER_VOLUME_MAX)
    {
        avs->audio_client_volume = AUDIO_PLAYER_VOLUME_MAX;
    }
    result = audio_client_ioctl(avs->audio_client_handle, AUDIO_CLIENT_IOCTL_SET_VOLUME, (void *)avs->audio_client_volume);

    return result;
}


wiced_result_t avs_audio_playback_volume_down(avs_t* avs)
{
    wiced_result_t result = WICED_ERROR;

    if (avs->audio_client_handle == NULL)
    {
        return result;
    }

    if (avs->audio_client_volume < AUDIO_PLAYER_VOLUME_MIN)
    {
        avs->audio_client_volume = AUDIO_PLAYER_VOLUME_MIN;
    }
    else
    {
        avs->audio_client_volume -= AUDIO_PLAYER_VOLUME_STEP;
    }
    result = audio_client_ioctl(avs->audio_client_handle, AUDIO_CLIENT_IOCTL_SET_VOLUME, (void *)avs->audio_client_volume);

    return result;
}
