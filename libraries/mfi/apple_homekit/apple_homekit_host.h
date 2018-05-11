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

#include "apple_homekit.h"
/******************************************************
 *                      Macros
 ******************************************************/

#define HOMEKIT_HOST_CALLOC(size, count)  calloc(size, count)

/******************************************************
 *                    Constants
 ******************************************************/

// Globally unique ID for the accessory
//(e.g. the primary MAC address, such as "00:11:22:33:44:55").
#define MFI_CONFIGURE_TXT_RECORD_KEY_ACCESSORY_ID                  "id"

// Current configuration number
#define MFI_CONFIGURE_TXT_RECORD_KEY_CURRENT_CONFIGURATION_NUMBER  "c#"

// Feature flags
#define MFI_CONFIGURE_TXT_RECORD_KEY_FEATURE_FLAGS                 "ff"

// Model Name
#define MFI_CONFIGURE_TXT_RECORD_KEY_MODEL_NAME                    "md"

// Protocol Version String
#define MFI_CONFIGURE_TXT_RECORD_KEY_PROTOCOL_VERSION_STRING       "pv"

// Status Flags
#define MFI_CONFIGURE_TXT_RECORD_KEY_STATUS_FLAGS                  "sf"

// State Number
#define MFI_CONFIGURE_TXT_RECORD_KEY_CURRENT_STATE_NUMBER          "s#"

#define MFI_HOMEKIT_CATEGORY_IDENTIFIER                            "ci"

#define MFI_CONFIGURE_INTERFACE          WICED_CONFIG_INTERFACE

#define MFI_CONFIGURE_PORT               80

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

wiced_result_t homekit_host_advertise_service( apple_homekit_accessory_config_t* homekit_accessory_config, char* current_state_number, char* current_configuration_number );

wiced_result_t homekit_host_text_record_initialise( void );

wiced_result_t homekit_host_remove_service( void );

wiced_result_t homekit_host_text_record_update_key_value_pair( char* key, char* value );

wiced_result_t homekit_host_deinit( void );


#ifdef __cplusplus
} /* extern "C" */
#endif
