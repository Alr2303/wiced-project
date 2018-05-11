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
#include "wiced_result.h"
#include "apple_homekit_host.h"
#include "gedday.h"
#include "wwd_assert.h"
#include "wiced_wifi.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define MFI_CONFIGURE_SERVICE_TYPE_LOCAL            "_hap._tcp.local"
#define HOMEKIT_UNIQUE_GEDDAY_HOST_NAME_PREFIX      "WICED-hap-"
#define HOMEKIT_MAC_OCTET4_OFFSET                   (9)
#define HOMEKIT_SERVICE_RECORD_TTL                  (GEDDAY_RECORD_TTL)
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
extern char*          wiced_homekit_mac_address_to_c_string( const void* mac_address, char* string_buffer );

/******************************************************
 *               Variables Definitions
 ******************************************************/

gedday_text_record_t  homekit_text_record;
char                  homekit_service_instance_name[64];
uint8_t               text_buffer[128];
char                  desired_gedday_host_name[64] = HOMEKIT_UNIQUE_GEDDAY_HOST_NAME_PREFIX;

/******************************************************
 *               Function Definitions
 ******************************************************/
wiced_result_t homekit_host_advertise_service( apple_homekit_accessory_config_t* homekit_accessory_config, char* current_state_number, char* current_configuration_number )
{
    char            temporary_string[10];
    wiced_mac_t     mac;
    char            mac_string[STRING_MAX_MAC];
    uint8_t         offset;
    uint8_t         mac_offset;

    /* Initilaise Gedday globals */
    memset(temporary_string, 0x00, sizeof(temporary_string));

    wiced_assert( "", homekit_accessory_config->name != NULL );

    WICED_VERIFY( gedday_text_record_set_key_value_pair(&homekit_text_record, (char*)MFI_CONFIGURE_TXT_RECORD_KEY_ACCESSORY_ID, homekit_accessory_config->device_id) );
    WICED_VERIFY( gedday_text_record_set_key_value_pair(&homekit_text_record, (char*)MFI_CONFIGURE_TXT_RECORD_KEY_MODEL_NAME, homekit_accessory_config->name) );
    WICED_VERIFY( gedday_text_record_set_key_value_pair(&homekit_text_record, (char*)MFI_CONFIGURE_TXT_RECORD_KEY_PROTOCOL_VERSION_STRING, homekit_accessory_config->mfi_protocol_version_string) );
    WICED_VERIFY( gedday_text_record_set_key_value_pair(&homekit_text_record, (char*)MFI_CONFIGURE_TXT_RECORD_KEY_CURRENT_STATE_NUMBER, current_state_number ) );
    WICED_VERIFY( gedday_text_record_set_key_value_pair(&homekit_text_record, (char*)MFI_CONFIGURE_TXT_RECORD_KEY_CURRENT_CONFIGURATION_NUMBER, current_configuration_number ) );

    unsigned_to_decimal_string( homekit_accessory_config->mfi_config_flags, temporary_string, 1, sizeof( temporary_string ) );

    WICED_VERIFY( gedday_text_record_set_key_value_pair(&homekit_text_record, (char*)MFI_CONFIGURE_TXT_RECORD_KEY_STATUS_FLAGS, temporary_string ) );

    unsigned_to_decimal_string(  homekit_accessory_config->mfi_config_features, temporary_string, 1, sizeof( temporary_string ) );

    WICED_VERIFY( gedday_text_record_set_key_value_pair(&homekit_text_record, (char*)MFI_CONFIGURE_TXT_RECORD_KEY_FEATURE_FLAGS, temporary_string ) );

    unsigned_to_decimal_string(  homekit_accessory_config->accessory_category_identifier, temporary_string, 1, sizeof( temporary_string ) );

    WICED_VERIFY( gedday_text_record_set_key_value_pair(&homekit_text_record, (char*)MFI_HOMEKIT_CATEGORY_IDENTIFIER, temporary_string ) );

    /* Generate an unique gedday host name */
    wiced_wifi_get_mac_address( &mac );
    wiced_homekit_mac_address_to_c_string( mac.octet, mac_string );
    offset = sizeof(HOMEKIT_UNIQUE_GEDDAY_HOST_NAME_PREFIX) - 1;
    mac_offset = HOMEKIT_MAC_OCTET4_OFFSET;
    memcpy((desired_gedday_host_name + offset), &mac_string[mac_offset], 2);
    offset = (uint8_t)(offset + 2);
    mac_offset = (uint8_t)(mac_offset + 3);
    memcpy((desired_gedday_host_name + offset), &mac_string[mac_offset], 2);
    offset = (uint8_t)(offset + 2);
    mac_offset = (uint8_t)(mac_offset + 3);
    memcpy((desired_gedday_host_name + offset), &mac_string[mac_offset], 2);
    offset = (uint8_t)(offset + 2);
    *(desired_gedday_host_name + offset) = '\0';

    //printf("Unique Gedday Host Name = [%s]\n", desired_gedday_host_name);

    WICED_VERIFY( gedday_init( homekit_accessory_config->interface, desired_gedday_host_name ) );

    memset( homekit_service_instance_name, 0x0, sizeof( homekit_service_instance_name ) );
    memcpy( homekit_service_instance_name, homekit_accessory_config->name, strlen( homekit_accessory_config->name ) );

    WICED_VERIFY( gedday_add_service( homekit_accessory_config->name, MFI_CONFIGURE_SERVICE_TYPE_LOCAL, MFI_CONFIGURE_PORT, HOMEKIT_SERVICE_RECORD_TTL, (const char*)gedday_text_record_get_string( &homekit_text_record ) ) );

    return gedday_add_dynamic_text_record( homekit_accessory_config->name, MFI_CONFIGURE_SERVICE_TYPE_LOCAL, &homekit_text_record );
}

wiced_result_t homekit_host_remove_service( void )
{
    gedday_remove_service( homekit_service_instance_name, MFI_CONFIGURE_SERVICE_TYPE_LOCAL );

    return gedday_text_record_delete( &homekit_text_record );
}

wiced_result_t homekit_host_text_record_update_key_value_pair( char* key, char* value )
{
    WICED_VERIFY( gedday_text_record_set_key_value_pair(&homekit_text_record, key, value ) );

    return gedday_update_service(homekit_service_instance_name, MFI_CONFIGURE_SERVICE_TYPE_LOCAL);
}

wiced_result_t homekit_host_text_record_initialise( void )
{
    memset(text_buffer, 0x00, sizeof(text_buffer));

   return gedday_text_record_create(&homekit_text_record, sizeof( text_buffer ), text_buffer );
}

wiced_result_t homekit_host_deinit( void )
{
    gedday_deinit();

    return WICED_SUCCESS;
}
