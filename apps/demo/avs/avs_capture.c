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
 * AVS Test Audio Capture Support
 */

#include "avs.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define RX_PERIOD_SIZE              (4 * AVS_SAMPLES_PER_PERIOD)
#define RX_NUM_AUDIO_BUFS           (10)
#define RX_BUFFER_SIZE              WICED_AUDIO_BUFFER_ARRAY_DIM_SIZEOF(RX_NUM_AUDIO_BUFS, RX_PERIOD_SIZE)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    AVS_RX_EVENT_SHUTDOWN       = (1 << 0),
    AVS_RX_EVENT_START          = (1 << 1),
    AVS_RX_EVENT_STOP           = (1 << 2),
} AVS_RX_EVENTS_T;

#define AVS_RX_ALL_EVENTS       (-1)

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

static wiced_audio_config_t rx_audio_config =
{
    .sample_rate        = 16000,
    .channels           = 2,
    .bits_per_sample    = 16,
    .frame_size         = 4,
    .volume             = 80,
};

static uint8_t rx_buffer[RX_BUFFER_SIZE];

/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_result_t initialize_audio_device(const platform_audio_device_id_t device_id, wiced_audio_config_t* config,
                                              uint8_t* buffer, size_t buffer_length, int period_size, wiced_audio_session_ref* session)
{
    wiced_result_t result = WICED_SUCCESS;

    /* Initialize device. */
    result = wiced_audio_init(device_id, session, period_size);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "wiced_audio_init returns %d\n", result);
        return result;
    }

    /* Initialize audio buffer. */
    result = wiced_audio_create_buffer(*session, buffer_length, WICED_AUDIO_BUFFER_ARRAY_PTR(buffer), NULL);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "wiced_audio_create_buffer returns %d\n", result);
        goto exit_with_error;
    }

    /* Configure session. */
    result = wiced_audio_configure(*session, config);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "wiced_audio_configure returns %d\n", result);
        goto exit_with_error;
    }

    return result;

exit_with_error:
    if (wiced_audio_deinit(*session) != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "wiced_audio_deinit returns error\n");
    }
    *session = NULL;
    return result;
}


static wiced_result_t process_rx_audio(avs_t* avs)
{
    pcm_buf_t* pcmbuf;
    uint8_t *buf;
    uint8_t *src;
    uint8_t *dst;
    uint16_t avail;
    wiced_result_t result;
    const uint32_t timeout = 200;
    int i;

    /*
     * Are we in a stopped state?
     */

    if (avs->rx_run == WICED_FALSE)
    {
        return WICED_SUCCESS;
    }

    if (avs->rx_configured == WICED_FALSE)
    {
        result = initialize_audio_device(avs->dct_tables.dct_app->audio_device_rx, &rx_audio_config, rx_buffer,
                                         sizeof(rx_buffer), RX_PERIOD_SIZE, &avs->rx_session);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to initialize/configure audio RX (%d)\n", result);
            return WICED_SUCCESS;
        }
        avs->rx_configured = WICED_TRUE;
    }

    if (avs->rx_started == WICED_FALSE)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Start rx audio\n");
        /* Start RX. */
        result = wiced_audio_start(avs->rx_session);
        if (result != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Start rx failed\n");
            return WICED_SUCCESS;
        }
        avs->rx_started = WICED_TRUE;
    }

    result = wiced_audio_wait_buffer(avs->rx_session, RX_PERIOD_SIZE, timeout);
    if (result != WICED_SUCCESS)
    {
        if (wiced_audio_stop(avs->rx_session) != WICED_SUCCESS)
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error stopping audio\n");
        }
        avs->rx_started = WICED_FALSE;

        return WICED_SUCCESS;
    }

    /*
     *  Get data from capture audio device
     */

    avail  = RX_PERIOD_SIZE;
    result = wiced_audio_get_buffer(avs->rx_session, &buf, &avail);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "wiced_audio_get_buffer() failed\n");
        return result;
    }

    /*
     * Process the audio here.
     */

    pcmbuf = &avs->pcm_bufs[avs->pcm_write_idx];

    if (pcmbuf->inuse)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "PCM buffers full\n");
        return WICED_ERROR;
    }

    dst = pcmbuf->buf;
    src = buf;

    /*
     * Copy over the audio data, converting from stereo to mono as we go.
     */

    for (i = 0; i < avail; i += 4)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        src += 2;
    }
    pcmbuf->inuse  = 1;
    pcmbuf->buflen = avail / 2;
    avs->pcm_write_idx = (avs->pcm_write_idx + 1) % AVS_NUM_PCM_BUFS;

    /*
     * Release audio buffer back to capture device
     */

    if ((result = wiced_audio_release_buffer(avs->rx_session, avail)) != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "wiced_audio_release_buffer() failed\n");
    }

    /*
     * And tell the main loop that there is audio data available.
     */

    wiced_rtos_set_event_flags(&avs->events, AVS_EVENT_PCM_AUDIO_DATA);

    return result;
}


static void avs_rx_thread(uint32_t context)
{
    avs_t* avs = (avs_t*)context;
    wiced_result_t result;
    uint32_t timeout;
    uint32_t events;

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Begin avs RX mainloop\n");

    while (WICED_TRUE)
    {
        if (avs->rx_run)
        {
            timeout = WICED_NO_WAIT;
        }
        else
        {
            timeout = WICED_WAIT_FOREVER;
        }

        events = 0;
        result = wiced_rtos_wait_for_event_flags(&avs->rx_events, AVS_RX_ALL_EVENTS, &events,
                                                 WICED_TRUE, WAIT_FOR_ANY_EVENT, timeout);

        if (events & AVS_RX_EVENT_SHUTDOWN)
        {
            break;
        }

        if (events & AVS_RX_EVENT_START)
        {
            if (avs->rx_run == WICED_FALSE)
            {
                avs->rx_run = WICED_TRUE;

                if (avs->rx_configured == WICED_FALSE)
                {
                    /*
                     * Initialize the RX audio device.
                     */

                    result = initialize_audio_device(avs->dct_tables.dct_app->audio_device_rx, &rx_audio_config, rx_buffer,
                                                     sizeof(rx_buffer), RX_PERIOD_SIZE, &avs->rx_session);
                    if (result != WICED_SUCCESS)
                    {
                        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to initialize/configure audio RX (%d)\n", result);
                    }
                    else
                    {
                        avs->rx_configured = WICED_TRUE;
                    }
                }

                result = wiced_audio_start(avs->rx_session);
                if (result != WICED_SUCCESS)
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to start audio RX (%d)\n", result);
                }
                else
                {
                    avs->rx_started = WICED_TRUE;
                }
            }
        }

        if (events & AVS_RX_EVENT_STOP)
        {
            if (avs->rx_run)
            {
                if (avs->rx_started)
                {
                    wiced_audio_stop(avs->rx_session);
                }
                if (avs->rx_configured)
                {
                    wiced_audio_deinit(avs->rx_session);
                    avs->rx_session = NULL;
                }
                avs->rx_started    = WICED_FALSE;
                avs->rx_configured = WICED_FALSE;
                avs->rx_run        = WICED_FALSE;
            }
        }

        if (avs->rx_run)
        {
            process_rx_audio(avs);
        }
    }

    if (avs->rx_started)
    {
        avs->rx_started = WICED_FALSE;
        wiced_audio_stop(avs->rx_session);
    }

    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "End avs RX mainloop\n");
}


wiced_result_t avs_audio_rx_thread_start(avs_t* avs)
{
    const platform_audio_device_info_t* audio_device;
    wiced_result_t result;

    if (avs->rx_thread_ptr != NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "RX thread already active\n");
        return WICED_ERROR;
    }

    /*
     * Create the rx event flags.
     */

    result = wiced_rtos_init_event_flags(&avs->rx_events);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error initializing RX event flags\n");
        return result;
    }

    /*
     * Initialize the RX audio device.
     */

    audio_device = platform_audio_device_get_info_by_id(avs->dct_tables.dct_app->audio_device_rx);
    wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Initialize audio device: %s\n", audio_device ? audio_device->device_name : "");

    /*
     * Create the main RX thread.
     */

    result = wiced_rtos_create_thread_with_stack(&avs->rx_thread, AVS_RX_THREAD_PRIORITY, "RX thread", avs_rx_thread,
                                                 avs->rx_thread_stack_buffer, AVS_RX_THREAD_STACK_SIZE, avs);
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unable to create RX thread (%d)\n", result);
        return result;
    }

    avs->rx_thread_ptr = &avs->rx_thread;

    return WICED_SUCCESS;
}


wiced_result_t avs_audio_rx_thread_stop(avs_t* avs)
{
    if (avs->rx_thread_ptr == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "No RX thread active\n");
        return WICED_ERROR;
    }

    wiced_rtos_set_event_flags(&avs->rx_events, AVS_RX_EVENT_SHUTDOWN);

    wiced_rtos_thread_force_awake(&avs->rx_thread);
    wiced_rtos_thread_join(&avs->rx_thread);
    wiced_rtos_delete_thread(&avs->rx_thread);

    avs->rx_thread_ptr = NULL;

    if (avs->rx_session != NULL)
    {
        wiced_audio_deinit(avs->rx_session);
        avs->rx_session = NULL;
    }

    wiced_rtos_deinit_event_flags(&avs->rx_events);

    return WICED_SUCCESS;
}


wiced_result_t avs_audio_rx_capture_enable(avs_t* avs, wiced_bool_t enable)
{
    if (avs->rx_thread_ptr == NULL)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "No RX thread active\n");
        return WICED_ERROR;
    }

    if (enable)
    {
        memset(avs->pcm_bufs, 0, sizeof(avs->pcm_bufs));
        avs->pcm_write_idx = 0;
        avs->pcm_read_idx  = 0;
        wiced_rtos_set_event_flags(&avs->rx_events, AVS_RX_EVENT_START);
    }
    else
    {
        wiced_rtos_set_event_flags(&avs->rx_events, AVS_RX_EVENT_STOP);
    }

    return WICED_SUCCESS;
}
