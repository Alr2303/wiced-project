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

#include <string.h>
#include <stdlib.h>
#include "wiced_rtos.h"
#include "wiced_defaults.h"
#include "RTOS/wwd_rtos_interface.h"
#include "RTOS/wiced_rtos_common.h"
#include "platform/wwd_platform_interface.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "timers.h"
#include "wiced_time.h"
#include "internal/wiced_internal_api.h"
#include "wwd_assert.h"
#include "platform_init.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef APPLICATION_STACK_SIZE
#define APPLICATION_STACK_SIZE WICED_DEFAULT_APPLICATION_STACK_SIZE
#endif

#ifndef TIMER_THREAD_STACK_SIZE
#define TIMER_THREAD_STACK_SIZE      1024
#endif
#define TIMER_QUEUE_LENGTH  5

#define SYSTEM_MONITOR_THREAD_STACK_SIZE   512

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef TimerCallbackFunction_t native_timer_handler_t;
typedef TaskFunction_t          native_thread_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    event_handler_t function;
    void*           arg;
} timer_queue_message_t;

typedef struct
{
    event_handler_t function;
    void*           arg;
} wiced_event_message_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static void application_thread_main( void *arg );

/******************************************************
 *               Variable Definitions
 ******************************************************/

extern const uint32_t ms_to_tick_ratio;
wiced_worker_thread_t wiced_hardware_io_worker_thread;
wiced_worker_thread_t wiced_networking_worker_thread;

static TaskHandle_t  app_thread_handle;
#ifndef WICED_DISABLE_WATCHDOG
static TaskHandle_t  system_monitor_thread_handle;
#endif /* WICED_DISABLE_WATCHDOG */
static wiced_time_t wiced_time_offset = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

/**
 *  Main function - starts ThreadX
 *  Called from the crt0 _start function
 *
 */
#ifndef ALTERNATE_MAIN
int main( void )
{

#if defined ( __IAR_SYSTEMS_ICC__ )
/* IAR allows init functions in __low_level_init(), but it is run before global
 * variables have been initialised, so the following init still needs to be done
 * When using GCC, this is done in crt0_GCC.c
 */
    platform_init_mcu_infrastructure( );
    platform_init_external_devices( );
#endif /* #elif defined ( __IAR_SYSTEMS_ICC__ ) */

#ifndef WICED_DISABLE_WATCHDOG
    /* Start the watchdog kicking thread */
    xTaskCreate( (TaskFunction_t) system_monitor_thread_main, "system monitor", SYSTEM_MONITOR_THREAD_STACK_SIZE/sizeof( portSTACK_TYPE ), NULL, RTOS_HIGHEST_PRIORITY, &system_monitor_thread_handle);
#endif /* WICED_DISABLE_WATCHDOG */
    /* Create an initial thread */
    xTaskCreate( application_thread_main, "app_thread", APPLICATION_STACK_SIZE/sizeof( portSTACK_TYPE ), NULL, WICED_PRIORITY_TO_NATIVE_PRIORITY(WICED_APPLICATION_PRIORITY), &app_thread_handle);

    /* Start the FreeRTOS scheduler - this call should never return */
    vTaskStartScheduler( );

    /* Should never get here, unless there is an error in vTaskStartScheduler */
    return 0;
}
#endif /* ifndef ALTERNATE_MAIN */

static void application_thread_main( void *arg )
{
    UNUSED_PARAMETER( arg );
    application_start( );

    malloc_leak_check(NULL, LEAK_CHECK_THREAD);
    vTaskDelete( NULL );
}


wiced_result_t wiced_rtos_create_thread( wiced_thread_t* thread, uint8_t priority, const char* name, wiced_thread_function_t function, uint32_t stack_size, void* arg )
{
    /* Limit priority to default lib priority */
    if ( priority > RTOS_HIGHEST_PRIORITY )
    {
        priority = RTOS_HIGHEST_PRIORITY;
    }

    return host_rtos_create_thread_with_arg( WICED_GET_THREAD_HANDLE( thread ), function, name, NULL, stack_size, WICED_PRIORITY_TO_NATIVE_PRIORITY( priority ), (uint32_t) arg );
}

wiced_result_t wiced_rtos_create_thread_with_stack( wiced_thread_t* thread, uint8_t priority, const char* name, wiced_thread_function_t function, void* stack, uint32_t stack_size, void* arg )
{
    /* Limit priority to default lib priority */
    if ( priority > RTOS_HIGHEST_PRIORITY )
    {
        priority = RTOS_HIGHEST_PRIORITY;
    }

    return  host_rtos_create_thread_with_arg( WICED_GET_THREAD_HANDLE( thread ), function, name, stack, stack_size, WICED_PRIORITY_TO_NATIVE_PRIORITY( priority ), (uint32_t)arg );
}

wiced_result_t wiced_rtos_delete_thread(wiced_thread_t* thread)
{
    wiced_result_t result;

    malloc_leak_check( &thread, LEAK_CHECK_THREAD );

    result = host_rtos_finish_thread( WICED_GET_THREAD_HANDLE( thread ) );

    if ( result != WICED_WWD_SUCCESS )
    {
        return result;
    }

    return host_rtos_delete_terminated_thread( WICED_GET_THREAD_HANDLE( thread ) );
}


wiced_result_t wiced_rtos_is_current_thread( wiced_thread_t* thread )
{
    if ( xTaskGetCurrentThread( ) == *thread )
    {
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}

wiced_result_t wiced_rtos_check_stack( void )
{
    /* TODO: Add stack checking here. */

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_thread_force_awake( wiced_thread_t* thread )
{
    xTaskAbortDelay(*thread);
    return WICED_SUCCESS;
}


wiced_result_t wiced_time_get_time(wiced_time_t* time_ptr)
{
    *time_ptr = (wiced_time_t) ( xTaskGetTickCount( ) * ms_to_tick_ratio ) + wiced_time_offset;
    return WICED_SUCCESS;
}

wiced_result_t wiced_time_set_time( const wiced_time_t* time_ptr )
{
    wiced_time_offset = *time_ptr - (wiced_time_t) ( xTaskGetTickCount( ) * ms_to_tick_ratio );
    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_init_mutex( wiced_mutex_t* mutex )
{
    wiced_assert("Bad args", mutex != NULL);

    /* Mutex uses priority inheritance */
    *mutex = xSemaphoreCreateMutex( );
    if ( *mutex == NULL )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_rtos_lock_mutex( wiced_mutex_t* mutex )
{
    wiced_assert("Bad args", mutex != NULL);

    if ( xSemaphoreTake( *mutex, WICED_WAIT_FOREVER ) != pdPASS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_unlock_mutex( wiced_mutex_t* mutex )
{
    wiced_assert("Bad args", mutex != NULL);

    if ( xSemaphoreGive( *mutex ) != pdPASS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_deinit_mutex( wiced_mutex_t* mutex )
{
    wiced_assert("Bad args", mutex != NULL);

    vSemaphoreDelete( *mutex );
    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_init_queue( wiced_queue_t* queue, const char* name, uint32_t message_size, uint32_t number_of_messages )
{
    UNUSED_PARAMETER( name );

    return host_rtos_init_queue( WICED_GET_QUEUE_HANDLE( queue ), NULL, number_of_messages * message_size, message_size );
}


#if 0 /* Not yet implemented by other RTOSs */
wiced_result_t wiced_rtos_push_to_queue_front( wiced_queue_t* queue, void* message, uint32_t timeout_ms )
{
    if ( xQueueSendToFront( *queue, message, (portTickType) ( timeout_ms / ms_to_tick_ratio ) ) != pdPASS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}
#endif

wiced_result_t wiced_rtos_get_queue_occupancy( wiced_queue_t* queue, uint32_t *count )
{
    UNUSED_PARAMETER( queue );
    UNUSED_PARAMETER( count );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_rtos_deinit_queue( wiced_queue_t* queue )
{
    return host_rtos_deinit_queue( WICED_GET_QUEUE_HANDLE( queue ) );
}

wiced_result_t wiced_rtos_is_queue_empty( wiced_queue_t* queue )
{
    signed portBASE_TYPE result;

    taskENTER_CRITICAL();
    result = xQueueIsQueueEmptyFromISR( *queue );
    taskEXIT_CRITICAL();

    return ( result != 0 ) ? WICED_SUCCESS : WICED_ERROR;
}

wiced_result_t wiced_rtos_is_queue_full( wiced_queue_t* queue )
{
    signed portBASE_TYPE result;

    taskENTER_CRITICAL();
    result = xQueueIsQueueFullFromISR( *queue );
    taskEXIT_CRITICAL();

    return ( result != 0 ) ? WICED_SUCCESS : WICED_ERROR;
}

static void timer_callback( TimerHandle_t handle )
{
    wiced_timer_t* timer = (wiced_timer_t*) pvTimerGetTimerID( handle );

    if ( timer->function )
    {
        timer->function( timer->arg );
    }
}

wiced_result_t wiced_rtos_init_timer( wiced_timer_t* timer, uint32_t time_ms, timer_handler_t function, void* arg )
{
    wiced_assert("Bad args", timer != NULL);

    timer->function = function;
    timer->arg      = arg;

    timer->handle = xTimerCreate(  (const char *)"", (TickType_t)( time_ms / ms_to_tick_ratio ), pdTRUE, timer, (native_timer_handler_t) timer_callback );
    if ( timer->handle == NULL )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_start_timer( wiced_timer_t* timer )
{
    if ( xTimerReset( timer->handle, WICED_WAIT_FOREVER ) != pdPASS )
    {
        return WICED_ERROR;
    }

    if ( xTimerStart( timer->handle, WICED_WAIT_FOREVER ) != pdPASS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_stop_timer( wiced_timer_t* timer )
{
    if ( xTimerStop( timer->handle, WICED_WAIT_FOREVER ) != pdPASS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_deinit_timer( wiced_timer_t* timer )
{
    if ( xTimerDelete( timer->handle, WICED_WAIT_FOREVER ) != pdPASS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}


wiced_result_t wiced_rtos_is_timer_running( wiced_timer_t* timer )
{
    return ( xTimerIsTimerActive( timer->handle ) != 0 ) ? WICED_SUCCESS : WICED_ERROR;
}


wiced_result_t wiced_rtos_init_event_flags( wiced_event_flags_t* event_flags )
{
    UNUSED_PARAMETER( event_flags );
    wiced_assert( "Unsupported\n", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_rtos_wait_for_event_flags( wiced_event_flags_t* event_flags, uint32_t flags_to_wait_for, uint32_t* flags_set, wiced_bool_t clear_set_flags, wiced_event_flags_wait_option_t wait_option, uint32_t timeout_ms )
{
    UNUSED_PARAMETER( event_flags );
    UNUSED_PARAMETER( flags_to_wait_for );
    UNUSED_PARAMETER( flags_set );
    UNUSED_PARAMETER( clear_set_flags );
    UNUSED_PARAMETER( wait_option );
    UNUSED_PARAMETER( timeout_ms );
    wiced_assert( "Unsupported\n", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_rtos_set_event_flags( wiced_event_flags_t* event_flags, uint32_t flags_to_set )
{
    UNUSED_PARAMETER( event_flags );
    UNUSED_PARAMETER( flags_to_set );
    wiced_assert( "Unsupported\n", 0!=0 );
    return WICED_UNSUPPORTED;
}

wiced_result_t wiced_rtos_deinit_event_flags( wiced_event_flags_t* event_flags )
{
    UNUSED_PARAMETER( event_flags );
    wiced_assert( "Unsupported\n", 0!=0 );
    return WICED_UNSUPPORTED;
}
