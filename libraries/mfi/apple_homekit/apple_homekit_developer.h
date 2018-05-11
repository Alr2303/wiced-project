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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** @file
 *
 * HomeKit developer header file
 *
 * This header file is intended to expose some internal API for
 * the purpose of testing or debugging the HomeKit application.
 *
 * This list is expected to grow.
 *
**/

#include "apple_homekit.h"

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/*****************************************************************************/
/**
 *  @defgroup mfi                   Apple MFi Protocols
 *  @ingroup  ipcoms
 *
 *  @addtogroup mfi_homekit         HomeKit
 *  @ingroup mfi
 *
 *  @addtogroup mfi_homekit_dev     Development Helpers
 *  @ingroup mfi_homekit
 *
 *  Used as helper functions during development and not to be used in production code
 *
 *  @{
 */
/*****************************************************************************/

/** Clear keys and other information which are stored in persistent storage during pairing process. And brings the accessory back to unpaired state.
 *
 *  IMPORTANT: Use this function only for Development purposes. This function should not be used in production code, as only controller or factory reset are allowed to clear these values.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t  wiced_homekit_clear_all_pairings( void );

/** Configures the maximum number of active connections which can be established with the accessory (i.e., the maximum number of controllers which can concurrently communicate with the accessory).
 * This does not affect the number of pairings to the controller which is 16.
 * Oldest socket is dropped if max_concurrent_connections is reached and a new request on a paired controller needs to be serviced.
 * The default value of "max-active-connection" is set to 8 as per the specification.
 *
 *  IMPORTANT: Use this function only for Development purposes. This function should not be used in production code, as the maximum value should be set as per the specification.
 *
 * @param   max_connections [in] : Maximum number of concurrent connection. Valid values are from 1 to 8.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_set_number_of_active_connections( uint8_t max_connections );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
