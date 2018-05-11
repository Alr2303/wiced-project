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
#include "homekit_certification_test_dct.h"

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
     .app_dct.accessory_mode                                 = 0,
     .app_dct.is_factoryrest                                 = 1,
     .app_dct.dct_random_device_id                           = "XX:XX:XX:XX:XX:XX",
     .app_dct.dct_current_door_state_value                   = "0",
     .app_dct.dct_target_door_state_value                    = "0",
     .app_dct.dct_lock_mechnism_current_value                = "0",
     .app_dct.dct_lock_mechnism_target_value                 = "0",
     .app_dct.dct_heating_cooling_current_value              = "0",
     .app_dct.dct_heating_cooling_target_value               = "0",
     .app_dct.dct_heating_cooling_current_temperature_value  = "25",
     .app_dct.dct_heating_cooling_target_temperature_value   = "25",
     .app_dct.dct_heating_cooling_units_value                = "0",
     .app_dct.dct_lightbulb_on_value                         = "false",
     .app_dct.dct_fan_on_value                               = "false",
     .app_dct.dct_outlet_on_value                            = "false",
     .app_dct.dct_outlet_in_use_value                        = "false",
     .app_dct.dct_switch_on_value                            = "false",
     .app_dct.dct_lock_management_command_value              = "\"AAEA\"",
     .app_dct.dct_lock_mechnism_current_state_value          = "0",
     .app_dct.dct_lock_mechnism_target_state_value           = "0",
#ifdef IOS9_ENABLED_SERVICES
     .app_dct.dct_security_system_current_state_value        = "0",
     .app_dct.dct_security_system_target_state_value         = "0",
     .app_dct.dct_door_current_position_value                = "0",
     .app_dct.dct_door_target_position_value                 = "0",
     .app_dct.dct_programmable_switch_output_state_value     = "0",
     .app_dct.dct_window_current_position_value              = "0",
     .app_dct.dct_window_target_position_value               = "0",
     .app_dct.dct_window_position_state_value                = "0",
     .app_dct.dct_current_position_value                     = "0",
     .app_dct.dct_target_position_value                      = "0",
     .app_dct.dct_position_state_value                       = "0",
#endif
};

/******************************************************
 *               Function Definitions
 ******************************************************/
