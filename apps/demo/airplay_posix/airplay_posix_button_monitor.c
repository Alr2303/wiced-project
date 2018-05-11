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

#include <errno.h>
#include <math.h>
#include <poll.h>
#include <stdlib.h>

#include <nuttx/config.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <mqueue.h>

#include <wiced.h>
#include <platform_button.h>

#include <Support/DACPCommon.h>

/******************************************************
 *                      Macros
 ******************************************************/

#define PRINT_INFO( fmt, ... )      printf( "AirplayButtonMonitor: " fmt "\n", ##__VA_ARGS__ )
#define PRINT_ERROR( fmt, ... )     printf( "AirplayButtonMonitor: " fmt "\n", ##__VA_ARGS__ )

/******************************************************
 *                    Constants
 ******************************************************/

#define BUTTON_DEBOUNCE_MS      ( 100 )
#define BUTTON_WAC_CLICK_MS     ( 5000 )

/******************************************************
 *                   Enumerations
 ******************************************************/

/* Mapping of PLATFORM_BUTTON_n buttons to following actions */
typedef enum
{
    BUTTON_ACTION_PREV_ITEM,
    BUTTON_ACTION_VOL_DOWN,
    BUTTON_ACTION_VOL_UP,
    BUTTON_ACTION_PLAY_PAUSE,
    BUTTON_ACTION_WAC,
    BUTTON_ACTION_NEXT_ITEM
} BUTTON_ACTION;

typedef enum
{
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASED
} BUTTON_STATE;

struct button
{
    BUTTON_STATE    state;
    struct timespec time_pressed;
    struct timespec time_released;
};

struct button_event_msg
{
    platform_button_t   id;
    uint32_t            duration;
};

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern int apple_wac_reset( void );
extern int PlatformDACPClientSendCommand( const char* commandStr );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static bool             initialized;
static pthread_t        worker_thread;
static struct button    buttons[WICED_PLATFORM_BUTTON_COUNT];
static char             mq_name[16];
static mqd_t            mq;

/******************************************************
 *               Function Definitions
 ******************************************************/

static uint32_t timespec_diff_in_ms( struct timespec* start, struct timespec* stop )
{
    uint32_t ms;

    if ( ( stop->tv_nsec - start->tv_nsec ) < 0 )
    {
        ms = ( stop->tv_sec - start->tv_sec - 1 ) * 1000;
        ms += ( stop->tv_nsec - start->tv_nsec + 1000000000 ) / 1000000;
    }
    else
    {
        ms = ( stop->tv_sec - start->tv_sec ) * 1000;
        ms += ( stop->tv_nsec - start->tv_nsec ) / 1000000;
    }

    return ms;
}

static void button_state_change_cb( platform_button_t id, wiced_bool_t is_pressed )
{
    struct button* button;

    if ( id >= ( sizeof( buttons ) / sizeof( buttons[0] ) ) )
        return;

    button = &buttons[id];

    if ( is_pressed )
    {
        clock_gettime( CLOCK_MONOTONIC, &button->time_pressed );
        button->state = BUTTON_STATE_PRESSED;
    }
    else if ( button->state != BUTTON_STATE_RELEASED )
    {
        struct button_event_msg msg;

        clock_gettime( CLOCK_MONOTONIC, &button->time_released );
        button->state = BUTTON_STATE_RELEASED;

        msg.duration = timespec_diff_in_ms( &button->time_pressed, &button->time_released );

        /* If release event comes before debounce duration, ignore it */
        if( msg.duration <= BUTTON_DEBOUNCE_MS )
            return;

        /* Send a message to worker thread */
        msg.id = id;
        mq_send( mq, (FAR const char*)&msg, sizeof( msg ), 1 );
    }
}

static void* button_event_worker_thread( void* unused )
{
    (void)unused;

    while ( 1 )
    {
        struct button_event_msg msg;
        int                     prio;
        ssize_t                 size;

        size = mq_receive( mq, (char *)&msg, sizeof( msg ), &prio );
        if( size != sizeof( msg ) )
        {
            PRINT_ERROR( "Ignoring invalid message" );
            continue;
        }

        switch ( msg.id )
        {
            case BUTTON_ACTION_PREV_ITEM:
                PlatformDACPClientSendCommand( kDACPCommandStr_PrevItem );
                break;
            case BUTTON_ACTION_VOL_DOWN:
                PlatformDACPClientSendCommand( kDACPCommandStr_VolumeDown );
                break;
            case BUTTON_ACTION_VOL_UP:
                PlatformDACPClientSendCommand( kDACPCommandStr_VolumeUp );
                break;
            case BUTTON_ACTION_PLAY_PAUSE:
                PlatformDACPClientSendCommand( kDACPCommandStr_PlayPause );
                break;
            case BUTTON_ACTION_WAC:
                if( msg.duration >= BUTTON_WAC_CLICK_MS)
                {
                    apple_wac_reset();
                }
                break;
            case BUTTON_ACTION_NEXT_ITEM:
                PlatformDACPClientSendCommand( kDACPCommandStr_NextItem );
                break;
            default:
                PRINT_INFO( "Ignoring button %u click", msg.id );
                break;
        }

    }

    return NULL;
}

int airplay_button_manager_init( void )
{
    int             err = 0;
    struct mq_attr  attr;

    if ( initialized )
    {
        return err;
    }

    /* Create message queue */
    attr.mq_maxmsg  = 16;
    attr.mq_msgsize = sizeof( struct button_event_msg );
    attr.mq_curmsgs = 0;
    attr.mq_flags   = 0;

    snprintf( mq_name, sizeof( mq_name ), "/tmp/%0lx", (unsigned long)mq_name );

    mq = mq_open( mq_name, O_RDWR | O_CREAT, 0644, &attr );
    if( mq == (mqd_t)-1 )
    {
        PRINT_ERROR( "Failed to create message queue" );
        err = -1;
        goto exit;
    }

    PRINT_INFO( "Created message queue (%s)", mq_name );

    // Initialize the buttons
#if (WICED_PLATFORM_BUTTON_COUNT > 0)
    platform_button_init( PLATFORM_BUTTON_1 );
    platform_button_enable( PLATFORM_BUTTON_1 );
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 1)
    platform_button_init( PLATFORM_BUTTON_2 );
    platform_button_enable( PLATFORM_BUTTON_2 );
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 2)
    platform_button_init( PLATFORM_BUTTON_3 );
    platform_button_enable( PLATFORM_BUTTON_3 );
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 3)
    platform_button_init( PLATFORM_BUTTON_4 );
    platform_button_enable( PLATFORM_BUTTON_4 );
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 4)
    platform_button_init( PLATFORM_BUTTON_5 );
    platform_button_enable( PLATFORM_BUTTON_5 );
#endif
#if (WICED_PLATFORM_BUTTON_COUNT > 5)
    platform_button_init( PLATFORM_BUTTON_6 );
    platform_button_enable( PLATFORM_BUTTON_6 );
#endif

    /* Create worker thread */
    err = pthread_create( &worker_thread, NULL, button_event_worker_thread, NULL );
    if( err != 0 )
    {
        PRINT_ERROR( "Failed to create message queue" );
        err = -1;
        goto exit;
    }

    PRINT_INFO( "Created worker thread" );

    /* Register callback for button events */
    platform_button_register_state_change_callback( button_state_change_cb );

    initialized = true;

exit:
    return err;
}
