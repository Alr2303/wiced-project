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

#include "apple_homekit_dct.h"
#include "homekit_application_dct_common.h"

/** @file
 *
 * HomeKit Application developer header file
 *
 * This header file is intended to expose some Application APIs
 * which is common across all HomeKit snippet application.
 *
 * This list is expected to grow.
 *
**/

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

/**
 * Get the MAC address of the device and change last 3 octet with the random number.
 * For first time bootup and every factory reset it will give random Device ID.
 * Persist Device ID during reboot.
 *
 * @param[in] Device ID string buffer
 *
 */
void     homekit_get_random_device_id( char device_id_buffer[STRING_MAX_MAC] );

/**
 * Convert MAC address of the device into the string format [XX:XX:XX:XX:XX:XX].
 *
 * @param[in] MAC address of the device
 * @param[in] MAC adderss string buffer
 */
char*    mac_address_to_string( const void* mac_address, char* mac_address_string_format );

/**
 * Check if hte device is factory reset or not. Returns WICED_TRUE if the device is factory reset. If not WCIED_FALSE
 *
 * @return @ref wiced_bool_t
 *
 */
wiced_bool_t is_device_factory_reset(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
