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

#include "platform_mfi.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/
#ifdef WPRINT_ENABLE_AUTH_CHIP_INFO
    #define WPRINTAUTH_CHIP_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_AUTH_CHIP_INFO(args)
#endif
#ifdef WPRINT_ENABLE_AUTH_CHIP_DEBUG
    #define WPRINT_AUTH_CHIP_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_AUTH_CHIP_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_AUTH_CHIP_ERROR
    #define WPRINT_AUTH_CHIP_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_AUTH_CHIP_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif
/******************************************************
 *                    Constants
 ******************************************************/
#define CONTROL_AND_STATUS_FLAG_ERROR                       ( 0x80 )
#define CONTROL_AND_STATUS_FLAG_GENERATE_SIGNATURE          ( 1 )

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum
{
    IAUTH_CHIP_DEVICE_VERSION                = 0x00,
    IAUTH_CHIP_FIRMWARE_VERSION              = 0x01,
    IAUTH_CHIP_AUTH_PROTOCOL_VERSION_MAJOR   = 0x02,
    IAUTH_CHIP_AUTH_PROTOCOL_VERSION_MINOR   = 0x03,
    IAUTH_CHIP_DEVICE_ID                     = 0x04,
    IAUTH_CHIP_ERROR_CODE                    = 0x05,
    IAUTH_CHIP_AUTH_CONTROL_AND_STATUS       = 0x10,
    IAUTH_CHIP_SIGNATURE_DATA_LENGTH         = 0x11,
    IAUTH_CHIP_SIGNATURE_DATA                = 0x12,
    IAUTH_CHIP_CHALLENGE_DATA_LENGTH         = 0x20,
    IAUTH_CHIP_CHALLENGE_DATA                = 0x21,
    IAUTH_CHIP_CERTIFICATE_DATA_LENGTH       = 0x30,
    IAUTH_CHIP_CERTIFICATE_DATA              = 0x31,
    IAUTH_CHIP_SELF_TEST_CONTROL_AND_STATUS  = 0x40,
    IAUTH_CHIP_SYSTEM_EVENT_COUNTER          = 0x4D,
    IAUTH_CHIP_APPLE_CERTIFICATE_DATA_LENGTH = 0x50,
    IAUTH_CHIP_APPLE_CERTIFICATE_DATA        = 0x51,
} iauth_chip_register_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    uint8_t register_address;
    uint8_t data[ 1 ];
} iauth_chip_transfer_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t iauth_chip_init             ( const platform_mfi_auth_chip_t* platform_params );
wiced_result_t iauth_chip_deinit           ( const platform_mfi_auth_chip_t* platform_params );
wiced_result_t iauth_chip_read_register    ( const platform_mfi_auth_chip_t* platform_params, iauth_chip_register_t chip_register, void* data, uint32_t data_length );
wiced_result_t iauth_chip_write_register   ( const platform_mfi_auth_chip_t* platform_params, iauth_chip_register_t chip_register, iauth_chip_transfer_t* data, uint32_t data_length);
wiced_result_t iauth_chip_create_signature ( const platform_mfi_auth_chip_t* platform_params, const void* data_to_sign, uint16_t data_size, uint8_t* signature_buffer, uint16_t signature_buffer_size, uint16_t* signature_size );
wiced_result_t iauth_chip_read_certificate ( const platform_mfi_auth_chip_t* platform_params, uint8_t* certificate_ptr, uint16_t certificate_buffer_size, uint16_t* certificate_size );

#ifdef __cplusplus
} /* extern "C" */
#endif
