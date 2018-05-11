/*
 * $ Copyright Broadcom Corporation $
 */

#include "wwd_constants.h"
#include "wiced_airplay.h"
#include "wiced_time.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

static wiced_bool_t is_ns_clock_initialised = WICED_FALSE;

/******************************************************
 *               Function Definitions
 ******************************************************/

uint64_t airplay_platform_get_ticks( void )
{
    uint64_t time;
    if ( is_ns_clock_initialised == WICED_FALSE )
    {
        is_ns_clock_initialised = WICED_TRUE;
        wiced_init_nanosecond_clock( );
    }
    time = wiced_get_nanosecond_clock_value( );
    return (uint64_t) time;
}

uint64_t airplay_platform_ticks_per_second( void )
{
    return ( 1000000000ULL );
}


