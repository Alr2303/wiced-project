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
 * To allow for sharing of application DCT
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "apple_homekit.h"
/******************************************************
 *                      Macros
 ******************************************************/
/**
 * This macro creates a shared DCT structure by appending the
 * application DCT structure to the HomeKit DCT structure.
 * Please do not modify this macro.
 */
#define APPEND_APPLICATION_DCT_TO_HOMEKIT_DCT( APP_DCT_T ) \
    typedef struct                                         \
    {                                                      \
        wiced_homekit_dct_space_t homekit_dct;             \
        APP_DCT_T                 app_dct;                 \
    } wiced_homekit_app_shared_dct_t;

#define DCT_HOMEKIT_APP_SECTION        (DCT_APP_SECTION)
#define HOMEKIT_APP_DCT_OFFSET(member) ( ( (uintptr_t)&((wiced_homekit_app_shared_dct_t *)0)->app_dct ) + ( (uintptr_t)&((homekit_application_dct_structure_t *)0)->member ) )

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define HOMEKIT_INITIAL_DCT_VALUE           { [ 0 ... ( WICED_HOMEKIT_DCT_BUFFER_SIZE - 1 ) ] = 0 }

#define HOMEKIT_RESERVED_DCT_SECTION        .homekit_dct.wiced_homekit_dct_buf  = HOMEKIT_INITIAL_DCT_VALUE
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
