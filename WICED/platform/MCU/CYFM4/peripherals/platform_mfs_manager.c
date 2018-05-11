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
 * CY FM4 MFS Interface Manager
 */

#include <stdint.h>
#include <string.h>
#include "wiced_platform.h"
#include "platform_config.h"
#include "platform_peripheral.h"
#include "platform_sleep.h"
#include "platform_assert.h"
#include "wwd_assert.h"
#include "wiced_rtos.h"

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

#ifdef CYFM4_MFS_RUNTIME_CONFIG
static wiced_bool_t mfs_manager_inited = WICED_FALSE;

/*
 * MFS channel availability status.
 * Zero indicates channel is available.
 * One indicates channel is allocated.
 * Bit 0 => MFS channel 0,
 * Bit 1 => MFS channel 1,
 * Bit 2 => MFS channel 2,
 * etc.
 */
static uint32_t mfs_manager_channels = 0;
#endif /* CYFM4_MFS_RUNTIME_CONFIG */

/******************************************************
 *               Function Definitions
 ******************************************************/

#ifdef CYFM4_MFS_RUNTIME_CONFIG
static platform_result_t platform_mfs_manager_init( void )
{
    platform_result_t ret;
    uint32_t          flags;

    if ( PLATFORM_MFS_CHANNEL_MAX > (sizeof(mfs_manager_channels) * 8) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    WICED_SAVE_INTERRUPTS( flags );

    if ( mfs_manager_inited != WICED_TRUE )
    {
        /* Initialize MFS manager */
        mfs_manager_channels = 0;
        mfs_manager_inited = WICED_TRUE;
    }

    ret = (mfs_manager_inited == WICED_TRUE) ? PLATFORM_SUCCESS : PLATFORM_ERROR;

    WICED_RESTORE_INTERRUPTS( flags );

    return ret;
}
#endif /* CYFM4_MFS_RUNTIME_CONFIG */

platform_result_t platform_mfs_manager_acquire_channel( uint32_t channel )
{
#ifdef CYFM4_MFS_RUNTIME_CONFIG
    platform_result_t ret;
    uint32_t          flags;

    if ( channel >= PLATFORM_MFS_CHANNEL_MAX )
    {
        return PLATFORM_UNSUPPORTED;
    }

    if ( platform_mfs_manager_init() != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    WICED_SAVE_INTERRUPTS( flags );

    if ( (mfs_manager_channels & (0x1 << channel)) == 0 )
    {
        mfs_manager_channels |= (0x1 << channel);
        ret = PLATFORM_SUCCESS;
    }
    else
    {
        ret = PLATFORM_ERROR;
    }

    WICED_RESTORE_INTERRUPTS( flags );

    return ret;
#else
    return PLATFORM_SUCCESS;
#endif /* CYFM4_MFS_RUNTIME_CONFIG */

}

platform_result_t platform_mfs_manager_release_channel( uint32_t channel )
{
#ifdef CYFM4_MFS_RUNTIME_CONFIG
    uint32_t flags;

    if ( channel >= PLATFORM_MFS_CHANNEL_MAX )
    {
        return PLATFORM_UNSUPPORTED;
    }

    if ( platform_mfs_manager_init() != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    WICED_SAVE_INTERRUPTS( flags );

    mfs_manager_channels &= ~(0x1 << channel);

    WICED_RESTORE_INTERRUPTS( flags );

    return PLATFORM_SUCCESS;
#else
    return PLATFORM_SUCCESS;
#endif /* CYFM4_MFS_RUNTIME_CONFIG */
}
