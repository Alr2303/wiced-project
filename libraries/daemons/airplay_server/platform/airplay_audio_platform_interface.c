/*
 * $ Copyright Broadcom Corporation $
 */


#include "airplay_audio_platform_interface.h"
#include "airplay_time_platform_interface.h"
#include "wwd_assert.h"
#include "wiced_rtos.h"
#include "platform.h"
#include "wiced_utilities.h"
#include "platform_audio.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define NUM_USECONDS_IN_SECONDS                     ( 1000 * 1000 )
#define NUM_MSECONDS_IN_SECONDS                     ( 1000 )
#define MAX_WICED_AUDIO_INIT_ATTEMPTS               ( 5 )
#define INTERVAL_BETWEEN_WICED_AUDIO_INIT_ATTEMPS   ( 100 )

#define WICED_AUDIO_BUFFER_SIZE                     ( 5 * 1024 )
#define WICED_AIRPLAY_AUDIO_DEFAULT_PERIOD_SIZE     ( 1 * 1024 ) /* 5 periods per 5K audio buffer */

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

static void wiced_airplay_audio_thread ( uint32_t context );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static uint8_t  audio_thread_stack[ WICED_AUDIO_THREAD_STACK_SIZE ] __attribute__((section(".ccm")));
uint32_t        audio_hw_pointer_value_at_host_time;

static wiced_airplay_audio_platform_t audio_platform_context;

/******************************************************
 *               Function Definitions
 ******************************************************/

static void wiced_airplay_audio_thread(uint32_t context)
{
    uint8_t*                    ptr;
    uint16_t                    n;
    wiced_result_t              result;
    uint64_t                    host_time;
    uint32_t                    sampling_time      = 0;
    uint16_t                    max_frames;
    uint16_t                    frames_avail;
    uint16_t                    available_bytes    = 0;
    (void)host_time;
    max_frames = WICED_AUDIO_BUFFER_SIZE / audio_platform_context.frame_size ;

    while( audio_platform_context.quit != WICED_TRUE )
    {
        /* Wait till at least one audio period is available for writing */
        if( audio_platform_context.is_started == WICED_FALSE )
        {
            /* Request the entire audio buffer. */
            result = wiced_audio_wait_buffer( audio_platform_context.sh, WICED_AUDIO_BUFFER_SIZE, 0 );
            wiced_assert( "Cant get a period from the buffer.", result == WICED_SUCCESS );
            available_bytes = WICED_AUDIO_BUFFER_SIZE;
        }
        else
        {
            /* Wait till at least one period is available for writing */
            uint32_t wait_time =        ((NUM_USECONDS_IN_SECONDS / audio_platform_context.sample_rate) * WICED_AIRPLAY_AUDIO_DEFAULT_PERIOD_SIZE)/NUM_MSECONDS_IN_SECONDS;
            result = wiced_audio_wait_buffer( audio_platform_context.sh, WICED_AIRPLAY_AUDIO_DEFAULT_PERIOD_SIZE, wait_time + 10 );
            if( result != WICED_SUCCESS )
            {
                WICED_AUDIO_DEBUG_PRINT(("No period\n"));

                /* Must do a recovery there */
                wiced_audio_stop( audio_platform_context.sh );
                audio_platform_context.is_started = WICED_FALSE;
                continue;
            }
            available_bytes = WICED_AIRPLAY_AUDIO_DEFAULT_PERIOD_SIZE;
        }
        while (available_bytes > 0)
        {
            n = available_bytes;

            /* Get available bytes.
             * Called multiple times in the case where a chunk is returned.
             */
            result = wiced_audio_get_buffer( audio_platform_context.sh, &ptr, &n);
            wiced_assert("Can't get an audio buffer!", result == WICED_SUCCESS);
            if ( result != WICED_SUCCESS)
            {
                /* Potentially underrun. */
                WICED_AUDIO_ERROR_PRINT(("No audio buffer chunk available!\n"));
                break;
            }

            wiced_assert("not a multiple of the frame size!", (n % audio_platform_context.frame_size) == 0);
            frames_avail = n / audio_platform_context.frame_size;
            wiced_assert( "Underrun!", frames_avail > 0);
            if( frames_avail <= 0 )
            {
                /* Must do a recovery there */
                wiced_audio_stop( audio_platform_context.sh );
                WICED_AUDIO_ERROR_PRINT(("Audio buffer underrun occurred before fill!\n"));
                audio_platform_context.is_started = WICED_FALSE;
                break;
            }

            WICED_DISABLE_INTERRUPTS();
            host_time = airplay_platform_get_ticks();
            wiced_audio_get_current_hw_pointer( audio_platform_context.sh, &audio_hw_pointer_value_at_host_time );
            WICED_ENABLE_INTERRUPTS();

            frames_avail = MIN( frames_avail, max_frames );
            audio_platform_context.read( &sampling_time, host_time, ptr, ( frames_avail * audio_platform_context.frame_size ), (void*) audio_platform_context.airplay_context );

            sampling_time += frames_avail;
            result = wiced_audio_release_buffer( audio_platform_context.sh, ( frames_avail * audio_platform_context.frame_size ) );
            wiced_assert("Underrun!", result == WICED_SUCCESS);
            if( result == WICED_ERROR )
            {
                wiced_audio_stop( audio_platform_context.sh );
                WICED_AUDIO_ERROR_PRINT(("Audio buffer underrun occured after fill!\n"));
                audio_platform_context.is_started = WICED_FALSE;
                break;
            }
            available_bytes -= ( frames_avail * audio_platform_context.frame_size );
            if( audio_platform_context.is_started == WICED_FALSE )
            {
                if (available_bytes == 0)
                {
                    /* Only start when we've fulfilled the request. */
                    if ( wiced_audio_start( audio_platform_context.sh ) != WICED_SUCCESS )
                    {
                        WICED_AUDIO_INFO_PRINT(("Starting audio failed !\n"));
                    }
                    else
                    {
                        audio_platform_context.is_started = WICED_TRUE;
                        WICED_AUDIO_INFO_PRINT(("Starting audio \n"));
                    }
                }
            }

        } /* while (available_bytes > 0) */

    } /* while( audio_platform_context.quit != WICED_TRUE ) */

    wiced_audio_stop( audio_platform_context.sh );
    WICED_AUDIO_INFO_PRINT(("Audio Stopped\n"));
    WICED_END_OF_CURRENT_THREAD( );

}

uint32_t wiced_airplay_audio_platform_get_latency( wiced_airplay_audio_platform_ref ref )
{
    uint32_t latency = 0;
    if ( wiced_audio_get_latency( ref->sh, &latency ) == WICED_SUCCESS )
    {
        return latency;
    }
    else
    {
        return 0;
    }
}

wiced_result_t wiced_airplay_audio_platform_init ( uint32_t                           airplay_context,
                                                   wiced_airplay_audio_params_t*      params,
                                                   wiced_airplay_audio_platform_ref*  ref,
                                                   pull_data_callback_t               cb,
                                                   platform_audio_device_id_t         device_id )
{
    wiced_audio_config_t config;
    wiced_result_t       result;
    uint8_t              attempts = 0;

    do
    {
        /* By default we are going to use the first audio device */
        result = wiced_audio_init( (device_id  != AUDIO_DEVICE_ID_NONE) ? device_id : PLATFORM_DEFAULT_AUDIO_OUTPUT, &audio_platform_context.sh, WICED_AIRPLAY_AUDIO_DEFAULT_PERIOD_SIZE );
    } while( ( result != WICED_SUCCESS ) && ( ++attempts < MAX_WICED_AUDIO_INIT_ATTEMPTS ) && ( wiced_rtos_delay_milliseconds(INTERVAL_BETWEEN_WICED_AUDIO_INIT_ATTEMPS)== WICED_SUCCESS ) );

    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    config.sample_rate     = params->sample_rate;
    config.channels        = params->channels;
    config.bits_per_sample = params->bits_per_sample;
    config.frame_size      = (params->channels * params->bits_per_sample) / 8;

    audio_platform_context.frame_size       = config.frame_size;
    audio_platform_context.is_started       = WICED_FALSE;
    wiced_assert("No pull data callback is given",cb!=NULL);
    audio_platform_context.read             = cb;
    audio_platform_context.airplay_context  = airplay_context;
    audio_platform_context.quit             = WICED_FALSE;
    audio_platform_context.audio_thread_ptr = NULL;

    result = wiced_audio_create_buffer( audio_platform_context.sh, WICED_AUDIO_BUFFER_SIZE, NULL, NULL );
    result = wiced_audio_configure( audio_platform_context.sh, &config );

    audio_platform_context.sample_rate = params->sample_rate;
    *ref = &audio_platform_context;

    return WICED_SUCCESS;
}

wiced_result_t wiced_airplay_audio_platform_deinit( wiced_airplay_audio_platform_ref audio_proc )
{
    wiced_audio_deinit( audio_platform_context.sh );
    return WICED_SUCCESS;
}

wiced_result_t  wiced_airplay_audio_platform_set_volume    ( wiced_airplay_audio_platform_ref audio, double volume_in_db )
{
    wiced_audio_set_volume( audio->sh, volume_in_db );
    return WICED_SUCCESS;
}

wiced_result_t wiced_airplay_audio_platform_start( wiced_airplay_audio_platform_ref audio_proc )
{
    wiced_result_t result;

    /* Create an audio thread which will be constantly pulling data from airplay buffers , decode it and pushing it */
    /* into an audio buffer */
    result = wiced_rtos_create_thread_with_stack( &audio_platform_context.audio_thread, WICED_AUDIO_THREAD_PRIORITY, WICED_AUDIO_THREAD_NAME, wiced_airplay_audio_thread, audio_thread_stack, WICED_AUDIO_THREAD_STACK_SIZE, NULL );
    wiced_jump_when_not_true( ( result == WICED_SUCCESS ), exit );
    audio_platform_context.audio_thread_ptr = &audio_platform_context.audio_thread;

exit:
    return WICED_SUCCESS;
}

wiced_result_t wiced_airplay_audio_platform_stop( wiced_airplay_audio_platform_ref audio_proc )
{
    if ( audio_proc->audio_thread_ptr != NULL )
    {
        audio_proc->quit = WICED_TRUE;

        wiced_rtos_thread_force_awake( &audio_proc->audio_thread );
        wiced_rtos_thread_join( &audio_proc->audio_thread );
        wiced_rtos_delete_thread( &audio_proc->audio_thread );
    }
    wiced_audio_deinit( audio_proc->sh );

    return WICED_SUCCESS;
}
