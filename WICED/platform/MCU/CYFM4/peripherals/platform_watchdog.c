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
 * If a watchdog reset occurred previously, a warning message will be printed to the console.
 */

#include "platform_cmsis.h"
#include "platform_constants.h"
#include "platform_peripheral.h"
#include "platform_stdio.h"
#include "platform_isr.h"
#include "platform_isr_interface.h"
#include "platform_assert.h"
#include "wwd_assert.h"
#include "wwd_rtos.h"
#include "wiced_defaults.h"
#include "platform_config.h" /* For CPU_CLOCK_HZ */

#include "wdg.h"
#include "reset.h"


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#if defined( APPLICATION_WATCHDOG_TIMEOUT_SECONDS )
#define WATCHDOG_TIMEOUT_SECONDS    APPLICATION_WATCHDOG_TIMEOUT_SECONDS
#else
#define WATCHDOG_TIMEOUT_SECONDS    MAX_WATCHDOG_TIMEOUT_SECONDS
#endif /* APPLICATION_WATCHDOG_TIMEOUT_SECONDS */

/* Watchdog prescalers. */
#define WATCHDOG_PRESCALER              ( (1 << SWC_PSR_Val) * ( 1 << APBC0_PSR_Val ) )

/* Clock frequency of the watchdog counter.
 * Primarily used to convert seconds to ticks.
 */
#define WATCHDOG_TIMEOUT_MULTIPLIER     ( CPU_CLOCK_HZ / WATCHDOG_PRESCALER )

/* Number of ticks until the watchdog bites. */
#define WATCHDOG_TIMEOUT_TICKS          ( WATCHDOG_TIMEOUT_SECONDS * WATCHDOG_TIMEOUT_MULTIPLIER )

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

#ifndef WICED_DISABLE_WATCHDOG
static void watchdog_irq( void );
#endif /* WICED_DISABLE_WATCHDOG */

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

#ifndef WICED_DISABLE_WATCHDOG
static void watchdog_irq( void )
{
    /* Load zero to the counter to reset, as the original load
     * value is set to the counter.  Since the interrupt isn't
     * cleared the next underflow will trigger a reset.
    */
    Swwdg_WriteWdgLoad(0UL);

    /* Uncomment this line to break just before the watchdog bites.
     * On other platforms, this code is enabled by checking if
     * a debugger is attached - unclear how to do that on this
     * platform.
     */
    /* WICED_TRIGGER_BREAKPOINT( ); */
}
#endif /* WICED_DISABLE_WATCHDOG */

/* Watchdog prescalers are configured as part of clock setup. */
platform_result_t platform_watchdog_init( void )
{
#ifndef WICED_DISABLE_WATCHDOG
    en_result_t         en_result;
    stc_swwdg_config_t  stc_swwdg_config;

    PDL_ZERO_STRUCT( stc_swwdg_config );

    stc_swwdg_config.u32LoadValue   = WATCHDOG_TIMEOUT_TICKS;
    stc_swwdg_config.bResetEnable   = TRUE;
    stc_swwdg_config.pfnSwwdgIrqCb  = watchdog_irq;

    en_result = Swwdg_Init( &stc_swwdg_config );
    if ( en_result != Ok )
    {
        return PLATFORM_ERROR;
    }

    en_result = Swwdg_Start();
    if ( en_result != Ok )
    {
        return PLATFORM_ERROR;
    }

    return PLATFORM_SUCCESS;
#else
    return PLATFORM_FEATURE_DISABLED;
#endif
}

platform_result_t platform_watchdog_kick( void )
{
#ifndef WICED_DISABLE_WATCHDOG
    /* Reload watchdog counter. */
    Swwdg_Feed();
    return PLATFORM_SUCCESS;
#else
    return PLATFORM_FEATURE_DISABLED;
#endif
}

wiced_bool_t platform_watchdog_check_last_reset( void )
{
#ifndef WICED_DISABLE_WATCHDOG
    wiced_bool_t        result;
    en_result_t         en_result;
    stc_reset_result_t  stc_reset_result;

    /* Get the reset cause that was stored earlier in the initialization
     * sequence.
     */
    en_result = Reset_GetStoredCause( &stc_reset_result );
    if ( en_result != Ok )
    {
        return WICED_FALSE;
    }

    result = ( stc_reset_result.bSoftwareWatchdog == TRUE)  ? WICED_TRUE : WICED_FALSE;

    return result;
#endif

    return WICED_FALSE;
}
