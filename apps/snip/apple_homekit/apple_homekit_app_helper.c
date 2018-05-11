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
#include "wiced_crypto.h"
#include "apple_homekit_app_helper.h"
#include "homekit_application_dct_common.h"
#include "wiced_framework.h"
#include "wiced_wifi.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HEX_DIGITS_UPPERCASE             "0123456789ABCDEF"

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

/******************************************************
 *               Function Definitions
 ******************************************************/

void homekit_get_random_device_id( char device_id_buffer[STRING_MAX_MAC] )
{
    homekit_application_dct_structure_t *app_dct;
    uint8_t local_factory_reset_bit;
    uint16_t random_value;
    int i;
    char* octet_pos;

    wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(is_factoryrest), sizeof( *app_dct ) );

    local_factory_reset_bit = app_dct->is_factoryrest;

    if ( local_factory_reset_bit == 1 )
    {
         /* Get random device id */
         octet_pos = device_id_buffer;
         for (i = 0; i < 6; i++)
         {
             wiced_crypto_get_random( (void*)&random_value, 2 );
             snprintf ( octet_pos, 3, "%02X", (uint8_t)random_value );
             octet_pos += 2;
             *octet_pos++ = ':';
         }

         *--octet_pos = '\0';
         WPRINT_APP_INFO( ("Device ID =[%s]\n",device_id_buffer) );

         app_dct->is_factoryrest = 0;
         memcpy ( app_dct->dct_random_device_id, device_id_buffer, strlen( device_id_buffer) );
         wiced_dct_write( (const void *)app_dct, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(is_factoryrest), sizeof(*app_dct) );
    }
    else
    {
        /* Take stored device id from the DCT in case of the rebooting the device */
        memcpy ( device_id_buffer, app_dct->dct_random_device_id, strlen(app_dct->dct_random_device_id) );
        WPRINT_APP_INFO( ("Device ID =[%s]\n",device_id_buffer) );
    }

    wiced_dct_read_unlock( (void*)app_dct, WICED_TRUE );
}

char* mac_address_to_string( const void *mac_address, char *mac_address_string_format )
{
    const uint8_t *     src;
    const uint8_t *     end;
    char *              dst;
    uint8_t             b;

    src = (const uint8_t *) mac_address;
    end = src + 6; // size of MAC address in bytes
    dst = mac_address_string_format;
    while( src < end )
    {
        if( dst != mac_address_string_format ) *dst++ = ':';
        b = *src++;
        *dst++ = HEX_DIGITS_UPPERCASE[ b >> 4 ];
        *dst++ = HEX_DIGITS_UPPERCASE[ b & 0xF ];
    }
    *dst = '\0';
    return( mac_address_string_format );
}

wiced_bool_t is_device_factory_reset(void)
{
    homekit_application_dct_structure_t *app_dct;
    uint8_t local_factory_reset_bit;

    wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(is_factoryrest), sizeof( *app_dct ) );
    local_factory_reset_bit = app_dct->is_factoryrest;
    wiced_dct_read_unlock( (void*)app_dct, WICED_TRUE );

    return ( local_factory_reset_bit == 1 )? WICED_TRUE : WICED_FALSE;
}
