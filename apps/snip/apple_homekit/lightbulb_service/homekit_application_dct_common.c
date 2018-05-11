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

#include "wiced_framework.h"
#include "homekit_application_dct_common.h"

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
/**
 * Initialize your Application DCT here
 *
 * HomeKit resides in the Application layer, and must share the Application DCT.
 *
 * Please take note that HomeKit DCT resides in the first WICED_HOMEKIT_DCT_BUFFER_SIZE of
 * the shared Application DCT. Take special note of this when writing to the offset parameter
 * of the write DCT API
 *
 */
DEFINE_APP_DCT(wiced_homekit_app_shared_dct_t)
{
     HOMEKIT_RESERVED_DCT_SECTION,
     .app_dct.is_factoryrest             = 1,
     .app_dct.dct_random_device_id       = "XX:XX:XX:XX:XX:XX",
     .app_dct.lightbulb_on_value         = "false",
     .app_dct.lightbulb_hue_value        = "360",
     .app_dct.lightbulb_saturation_value = "100",
     .app_dct.lightbulb_brightness_value = "100",
     .app_dct.lightbulb_color_value      = "360",
};

/******************************************************
 *               Function Definitions
 ******************************************************/
