/*
 * $ Copyright Broadcom Corporation $
 */
#pragma once

#include "wiced_result.h"
#include "wiced_audio.h"
#include "wiced_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_AUDIO_THREAD_STACK_SIZE               (4000)
#define WICED_AUDIO_THREAD_PRIORITY                 (3)
#define WICED_AUDIO_THREAD_NAME                     ("Wiced audio thread")

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef void (*pull_data_callback_t)( uint32_t* sample_time, uint64_t wallclock_time, void* buffer, size_t length, void* context);

/******************************************************
 *                 Structures
 ******************************************************/
typedef struct
{
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bits_per_sample;
} wiced_airplay_audio_params_t;

typedef struct
{
    wiced_audio_session_ref  ref;
    wiced_audio_session_ref  sh;
    wiced_thread_t           audio_thread;
    wiced_thread_t*          audio_thread_ptr;
    pull_data_callback_t     read;
    uint32_t                 airplay_context;
    uint8_t*                 audio_buffer_ptr;
    uint16_t                 audio_buffer_size;
    wiced_bool_t             is_started;
    uint8_t                  frame_size;
    uint16_t                 sample_rate;
    wiced_bool_t             quit;
} wiced_airplay_audio_platform_t;


/******************************************************
 *               Function Declarations
 ******************************************************/

typedef wiced_airplay_audio_platform_t* wiced_airplay_audio_platform_ref;

extern wiced_result_t  wiced_airplay_audio_platform_init           ( uint32_t                           airplay_context,
                                                                     wiced_airplay_audio_params_t*      params,
                                                                     wiced_airplay_audio_platform_ref*  ref,
                                                                     pull_data_callback_t               cb,
                                                                     platform_audio_device_id_t         device_id );
extern wiced_result_t  wiced_airplay_audio_platform_deinit         ( wiced_airplay_audio_platform_ref ref );
extern wiced_result_t  wiced_airplay_audio_platform_start          ( wiced_airplay_audio_platform_ref ref );
extern wiced_result_t  wiced_airplay_audio_platform_stop           ( wiced_airplay_audio_platform_ref ref );
extern wiced_result_t  wiced_airplay_audio_platform_set_volume     ( wiced_airplay_audio_platform_ref ref, double volume_in_db );
extern uint32_t        wiced_airplay_audio_platform_get_latency    ( wiced_airplay_audio_platform_ref audio );

#ifdef PLATFORM_SKEW_ADJUST
extern wiced_result_t  wiced_airplay_audio_platform_adjust_for_skew ( wiced_airplay_audio_platform_ref ref, signed int sskew);
#endif /* PLATFORM_SKEW_ADJUST */

#ifdef __cplusplus
}
#endif
