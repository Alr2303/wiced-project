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
 * AVS Test Keypad Support
 */

#include "avs.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define BUTTON_WORKER_STACK_SIZE              ( 4096 )
#define BUTTON_WORKER_QUEUE_SIZE              ( 4 )

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    ACTION_AUDIO_CAPTURE_START,
    ACTION_AUDIO_VOLUME_UP,
    ACTION_AUDIO_VOLUME_DOWN,
    ACTION_AUDIO_TALK,
} app_action_t;

typedef enum
{
    BUTTON_CAPTURE,
    BUTTON_VOLUME_UP,
    BUTTON_VOLUME_DOWN,
    BUTTON_TALK,
} application_button_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

static void avs_keypad_event_handler( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static const wiced_button_manager_configuration_t button_press_configuration =
{
    .short_hold_duration     = 500  * MILLISECONDS,
    .debounce_duration       = 150  * MILLISECONDS,

    .event_handler           = avs_keypad_event_handler,
};

/* Static button configuration */
static const wiced_button_configuration_t keypad_configuration[] =
{
#if (WICED_PLATFORM_BUTTON_COUNT > 0)
    [ BUTTON_CAPTURE     ] = { PLATFORM_BUTTON_PAUSE_PLAY , BUTTON_CLICK_EVENT, ACTION_AUDIO_CAPTURE_START },
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 1)
    [ BUTTON_VOLUME_UP   ] = { PLATFORM_BUTTON_VOLUME_UP  , BUTTON_CLICK_EVENT, ACTION_AUDIO_VOLUME_UP     },
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 2)
    [ BUTTON_VOLUME_DOWN ] = { PLATFORM_BUTTON_VOLUME_DOWN, BUTTON_CLICK_EVENT, ACTION_AUDIO_VOLUME_DOWN   },
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 3)
    [ BUTTON_TALK        ] = { PLATFORM_BUTTON_MULTI_FUNC,  BUTTON_CLICK_EVENT, ACTION_AUDIO_TALK          },
#endif
};

/* Button objects for the button manager */
static button_manager_button_t buttons[] =
{
#if (WICED_PLATFORM_BUTTON_COUNT > 0)
    [ BUTTON_CAPTURE     ] = { &keypad_configuration[ BUTTON_CAPTURE     ] },
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 1)
    [ BUTTON_VOLUME_UP   ] = { &keypad_configuration[ BUTTON_VOLUME_UP   ] },
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 2)
    [ BUTTON_VOLUME_DOWN ] = { &keypad_configuration[ BUTTON_VOLUME_DOWN ] },
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 3)
    [ BUTTON_TALK        ] = { &keypad_configuration[ BUTTON_TALK        ] },
#endif
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static void avs_keypad_event_handler( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state )
{
    if ((g_avs != NULL) && (event == BUTTON_CLICK_EVENT))
    {
        uint32_t event = 0;

        switch (button->configuration->application_event)
        {
            case ACTION_AUDIO_CAPTURE_START:
                event = AVS_EVENT_CAPTURE;
                break;

            case ACTION_AUDIO_VOLUME_UP:
                event = AVS_EVENT_VOLUME_UP;
                break;

            case ACTION_AUDIO_VOLUME_DOWN:
                event = AVS_EVENT_VOLUME_DOWN;
                break;

            case ACTION_AUDIO_TALK:
                event = AVS_EVENT_SPEECH_REQUEST;
                break;

            default:
                break;
        }

        if (event != 0)
        {
            wiced_rtos_set_event_flags(&g_avs->events, event);
        }
    }

    return;
}


wiced_result_t avs_keypad_init(avs_t* avs)
{
    wiced_result_t result;

    result = wiced_rtos_create_worker_thread( &avs->button_worker_thread, WICED_DEFAULT_WORKER_PRIORITY, BUTTON_WORKER_STACK_SIZE, BUTTON_WORKER_QUEUE_SIZE );
    wiced_action_jump_when_not_true( result == WICED_SUCCESS, _exit, wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "wiced_rtos_create_worker_thread() failed !\r\n") );
    result = button_manager_init( &avs->button_manager, &button_press_configuration, &avs->button_worker_thread, buttons, ARRAY_SIZE( buttons ) );
    wiced_action_jump_when_not_true( result == WICED_SUCCESS, _exit, wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "button_manager_init() failed !\r\n") );

 _exit:
    return result;
}


wiced_result_t avs_keypad_deinit(avs_t* avs)
{
    wiced_result_t result;

    result  = button_manager_deinit( &avs->button_manager );
    result |= wiced_rtos_delete_worker_thread( &avs->button_worker_thread );

    return result;
}

