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

/**
 * HomeKit uses the application DCT (DCT_APP_SECTION).
 * To allow for sharing of applicaiton DCT
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "apple_homekit_dct.h"
#include "apple_homekit.h"
/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * Application should define its DCT structure here
 * The homekit_application_dct_structure_t given below is
 * just an example. Application is free to redefine this structure
 * based on it's need and use it.
 */
typedef struct
{
    uint8_t      is_factoryrest;
    char         dct_random_device_id[18];
    char         lightbulb_on_value[6];
    char         lightbulb_brightness_value[6];
    char         lightbulb_hue_value[6];
    char         lightbulb_saturation_value[6];
    char         lightbulb_color_value[6];
} homekit_application_dct_structure_t;

/**
 * Pass in the Application DCT structure to this macro
 */
APPEND_APPLICATION_DCT_TO_HOMEKIT_DCT( homekit_application_dct_structure_t )

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
