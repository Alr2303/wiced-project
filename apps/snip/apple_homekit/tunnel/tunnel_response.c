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
#include "apple_homekit.h"
#include <string.h>
#include "bt_target.h"
#include "wiced.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_types.h"
#include "tunnel.h"
#include "tunnel_ble.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define WRITE_NULL_TERMINATED_DATA_TO_BUFFER( BUFFER,  DATA, DATA_OFFSET ) \
    {                                                                      \
        if ( (BUFFER) != NULL )                                            \
        {                                                                  \
            memcpy( (BUFFER) + (DATA_OFFSET), (DATA), strlen( (DATA) ) );  \
        }                                                                  \
        (DATA_OFFSET) += strlen( (DATA) );                                 \
    }

#define WRITE_DATA_WITH_LENGTH_TO_BUFFER( BUFFER,  DATA, DATA_LENGTH, DATA_OFFSET ) \
    {                                                                               \
        if ( (BUFFER) != NULL )                                                     \
        {                                                                           \
            memcpy( (BUFFER) + (DATA_OFFSET), (DATA), (DATA_LENGTH) );              \
        }                                                                           \
        (DATA_OFFSET) += ( DATA_LENGTH );                                           \
    }

/******************************************************
 *                    Constants
 ******************************************************/

#define FIXED_LENGTH_OF_HTTP_HEADER  (sizeof( HTTP_HEADER_200 CRLF HTTP_HEADER_CONTENT_TYPE "application/hap+json" CRLF HTTP_HEADER_CONTENT_LENGTH CRLF CRLF ) - 1)

#define SENDING_COMPLETE        ( WICED_TRUE )
#define SENDING_IN_PROGRESS     ( WICED_FALSE )

#define MAX_APPLE_TYPE_LENGTH   ( 8 )

#define ACCESSORIES_STRING_KEY         "\"accessories\""
#define ACCESSORIES_ID_STRING_KEY      "\"aid\""
#define INSTANCE_ID_STRING_KEY         "\"iid\""
#define TYPE_STRING_KEY                "\"type\""

#define SERVICES_STRING_KEY            "\"services\""
#define CHARACTERISTICS_STRING_KEY     "\"characteristics\""
#define VALUE_STRING_KEY               "\"value\""
#define PERMISSIONS_STRING_KEY         "\"perms\""
#define EVENT_NOTIFICATIONS_STRING_KEY "\"ev\""
#define DESCRIPTION_STRING_KEY         "\"description\""
#define FORMAT_STRING_KEY              "\"format\""
#define UNIT_STRING_KEY                "\"unit\""
#define MINIMUM_VALUE_STRING_KEY       "\"minValue\""
#define MAXIMUM_VALUE_STRING_KEY       "\"maxValue\""
#define STEP_VALUE_STRING_KEY          "\"minStep\""
#define MAXIMUM_LENGTH_STRING_KEY      "\"maxLen\""
#define MAXIMUM_DATA_LENGTH_STRING_KEY "\"maxDataLen\""

#define STATUS_STRING_KEY              "\"status\""

#define ERROR_CODE_STRING_KEY          "\"errorCode\""

#define PAIRED_READ_STRING_KEY         "\"pr\""
#define PAIRED_WRITE_STRING_KEY        "\"pw\""
#define TIMED_WRITE_STRING_KEY         "\"tw\""
#define EVENTS_STRING_KEY              "\"ev\""
#define HIDDEN_STRING_KEY              "\"hd\""
#define ADDITIONAL_AUTH_STRING_KEY     "\"aa\""

#define FORMAT_STRING_BOOL_TYPE       "\"bool\""
#define FORMAT_STRING_UINT8_TYPE      "\"uint8\""
#define FORMAT_STRING_UINT16_TYPE     "\"uint16\""
#define FORMAT_STRING_UINT32_TYPE     "\"uint32\""
#define FORMAT_STRING_UINT64_TYPE     "\"uint64\""
#define FORMAT_STRING_INTEGER_TYPE    "\"int\""
#define FORMAT_STRING_FLOAT_TYPE      "\"float\""
#define FORMAT_STRING_STRING_TYPE     "\"string\""
#define FORMAT_STRING_TLV8_TYPE       "\"tlv8\""
#define FORMAT_STRING_DATA_TYPE       "\"data\""

#define UNIT_CELCIUS_STRING_TYPE     "\"celsius\""
#define UNIT_PERCENTAGE_STRING_TYPE  "\"percentage\""
#define UNIT_ARC_DEGREES_STRING_TYPE "\"arcdegrees\""
#define UNIT_SECONDS_STRING_TYPE     "\"seconds\""
#define UNIT_LUX_STRING_TYPE         "\"lux\""

#define APPLICATION_HAP_PLUS_JSON     "application/hap+json"
#define APPLICATION_PAIRING_PLUS_TLV8 "application/pairing+tlv8"
#define MAXIMUM_HTTP_HEADER            APPLICATION_HAP_PLUS_JSON APPLICATION_PAIRING_PLUS_TLV8
#define MAXIMUM_CONTENT_LENGTH         APPLICATION_HAP_PLUS_JSON APPLICATION_PAIRING_PLUS_TLV8

#define RESPONSE_MINIMUM_LENGTH 2048

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    SEND_ACCESSORY_DATABASE_HEADER_STATE,
    SEND_ACCESSORY_DATABASE_FOOTER_STATE,
    SEND_ACCESSORY_DATABASE_BODY_STATE,
    SEND_CHARACTERISTICS_HEADER_STATE,
    SEND_CHARACTERISTICS_FOOTER_STATE,
    SEND_CHARACTERISTICS_BODY_STATE,
    SEND_SERVICES_HEADER_STATE,
    SEND_SERVICES_BODY_STATE,
    SEND_SERVICES_FOOTER_STATE,
    SEND_ACCESSORY_DATABASE_ERROR_STATE
} send_attribute_state;

typedef enum
{
    SEND_UPDATED_CHARACTERISTICS_ONLY,
    SEND_UPDATED_SERVICES_ONLY,
    SEND_FULL_DATABASE
} wiced_accessory_response_t;

typedef enum
{
    EVENT_SEND,
    RESPONSE_SEND
} characteristic_response_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)
typedef struct
{
    uint8_t type;
    uint8_t length;
} tlv8_header_t;
#pragma pack()

/******************************************************
 *               Function Declarations
 ******************************************************/

static wiced_result_t send_unit_type                ( uint16_t unit, uint8_t** local_send_buffer, uint32_t* data_offset);
static wiced_result_t send_permissions              ( uint16_t properties,  uint8_t** local_send_buffer,  uint32_t* data_offset );
static wiced_result_t send_instance_id              ( const char* instance_id_string, uint8_t instance_id, uint8_t** local_send_buffer, uint32_t* data_offset);
static wiced_result_t send_type                     ( wiced_bt_uuid_t uuid, uint8_t** local_send_buffer, uint32_t* data_offset);
static wiced_result_t send_format_type              ( uint8_t format, uint8_t** local_send_buffer, uint32_t* data_offset);
static wiced_result_t send_value                    ( tunneled_accessory_t* accessory, tunneled_characteristic_t* characteristic, uint8_t** local_send_buffer, uint32_t* data_offset);
static uint8_t        value_to_string               (hap_characteristic_value_t value, uint8_t format, char* buffer, int max_length);

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

uint32_t tunnel_write_accessory_database(wiced_homekit_accessories_t* accessory, uint8_t* send_buffer, uint32_t offset)
{
    char                            string_decimal_value[10 + 1];
    tunneled_accessory_t*           p_accessory = NULL;
    tunneled_service_t*             p_service = NULL;
    tunneled_characteristic_t*      p_characteristic = NULL;
    uint32_t                        data_offset = offset;
    wiced_bt_uuid_t                 uuid;
    send_attribute_state            send_homekit_response_state = SEND_ACCESSORY_DATABASE_BODY_STATE;
    wiced_bool_t                    finish = WICED_FALSE;

    p_accessory = tunnel_get_accessory_by_aid(accessory->instance_id);
    if ( p_accessory )
    {
        p_service = p_accessory->svc_list_front;
        if ( p_service )
            p_characteristic = p_service->char_list_front;
    }
    if ( !p_accessory || !p_service || !p_characteristic )
        return 0;

    while ( !finish )
    {
        switch ( send_homekit_response_state )
        {
            case SEND_ACCESSORY_DATABASE_BODY_STATE:
                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "{", data_offset );

                send_instance_id( ACCESSORIES_ID_STRING_KEY, accessory->instance_id , &send_buffer, &data_offset );

                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "," , data_offset );

                send_homekit_response_state = SEND_SERVICES_HEADER_STATE;
                break;

            case SEND_ACCESSORY_DATABASE_FOOTER_STATE:
                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "}", data_offset );

                finish = WICED_TRUE;
                break;

            case SEND_SERVICES_HEADER_STATE:
                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, SERVICES_STRING_KEY ": [", data_offset );
                send_homekit_response_state = SEND_SERVICES_BODY_STATE;
                //Fall through

            case SEND_SERVICES_BODY_STATE:
                if ( p_service == NULL )
                {
                    send_homekit_response_state = SEND_ACCESSORY_DATABASE_ERROR_STATE;
                    break;
                }

                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "{" , data_offset );

                send_instance_id( INSTANCE_ID_STRING_KEY, p_service->inst_id, &send_buffer, &data_offset );
                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "," , data_offset );

                uuid.len = LEN_UUID_16;
                uuid.uu.uuid16 = p_service->uuid;
                send_type( uuid, &send_buffer, &data_offset );
                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, ",", data_offset );

                send_homekit_response_state = SEND_CHARACTERISTICS_HEADER_STATE;
                break;

            case SEND_SERVICES_FOOTER_STATE:
                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "}", data_offset );

                p_service = p_service->next;

                if ( p_service == NULL )
                {
                    p_service = NULL;

                    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "]", data_offset );
                    send_homekit_response_state = SEND_ACCESSORY_DATABASE_FOOTER_STATE;
                }
                else
                {
                    p_characteristic = p_service->char_list_front;

                    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, ",", data_offset );
                    send_homekit_response_state = SEND_SERVICES_BODY_STATE;
                }
                break;

            case SEND_CHARACTERISTICS_HEADER_STATE:
                if ( p_characteristic == NULL )
                {
                    send_homekit_response_state = SEND_ACCESSORY_DATABASE_ERROR_STATE;
                    break;
                }

                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, CHARACTERISTICS_STRING_KEY ": [ ", data_offset );

                send_homekit_response_state = SEND_CHARACTERISTICS_BODY_STATE;
                //fall through

            case SEND_CHARACTERISTICS_BODY_STATE:
                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "{", data_offset );

                if ( p_characteristic == NULL )
                {
                    send_homekit_response_state = SEND_ACCESSORY_DATABASE_ERROR_STATE;
                    break;
                }

                send_instance_id( INSTANCE_ID_STRING_KEY, p_characteristic->inst_id, &send_buffer, &data_offset );
                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "," , data_offset );

                uuid.len = LEN_UUID_16;
                uuid.uu.uuid16 = p_characteristic->uuid;
                send_type( uuid, &send_buffer, &data_offset );

                /* Characteristics without a read only permission should not have their value sent as part of database*/
                if ( (p_characteristic->hap_properties & HAP_CHARACTERISTIC_PROPERTY_READ) ||
                     (p_characteristic->hap_properties & HAP_CHARACTERISTIC_PROPERTY_SECURE_READ) )
                {
                    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, ",", data_offset );

                    send_value( p_accessory, p_characteristic, &send_buffer, &data_offset );
                }

                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "," , data_offset );

                send_permissions( p_characteristic->hap_properties, &send_buffer, &data_offset );

                if ( p_characteristic->format != 0 )
                {
                    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, ",", data_offset );
                    send_format_type( p_characteristic->format, &send_buffer, &data_offset );
                }

                if ( (p_characteristic->unit != 0) && (p_characteristic->unit != HAP_CHARACTERISTIC_FORMAT_UNIT_UNITLESS) )
                {
                    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, ",", data_offset );
                    send_unit_type( p_characteristic->unit, &send_buffer, &data_offset );
                }

                if (p_characteristic->full_size)
                {
                    if ( ( p_characteristic->minimum.value_u32 != 0 ) ||
                         ( p_characteristic->maximum.value_u32 != 0 ) )
                    {
                        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, ",", data_offset );
                        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, MINIMUM_VALUE_STRING_KEY ":" , data_offset );
                        value_to_string(p_characteristic->minimum, p_characteristic->format, string_decimal_value, 10);
                        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, string_decimal_value, data_offset );

                        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, ",", data_offset );
                        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, MAXIMUM_VALUE_STRING_KEY ":" , data_offset );
                        value_to_string(p_characteristic->maximum, p_characteristic->format, string_decimal_value, 10);
                        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, string_decimal_value, data_offset );
                    }

                    if ( p_characteristic->step.value_u32 != 0 )
                    {
                        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, ",", data_offset );
                        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, STEP_VALUE_STRING_KEY ":", data_offset );
                        value_to_string(p_characteristic->step, p_characteristic->format, string_decimal_value, 10);
                        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, string_decimal_value, data_offset );
                    }
                }

                //TODO: description;
                send_homekit_response_state = SEND_CHARACTERISTICS_FOOTER_STATE;
                // Fall through

            case SEND_CHARACTERISTICS_FOOTER_STATE:
                WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "}" , data_offset );

                p_characteristic = p_characteristic->next;

                if ( p_characteristic == NULL )
                {
                    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer, "]" , data_offset );
                    send_homekit_response_state = SEND_SERVICES_FOOTER_STATE;
                }
                else
                {
                    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( send_buffer,"," , data_offset );
                    send_homekit_response_state = SEND_CHARACTERISTICS_BODY_STATE;
                }
                break;

            case SEND_ACCESSORY_DATABASE_ERROR_STATE:
            default:
                return 0;
                break;
        }
    }

    WPRINT_APP_DEBUG(("tunnel_write_accessory_database data size: %d\n", (int)(data_offset - offset)));
    return data_offset - offset;
}

static wiced_result_t send_type( wiced_bt_uuid_t uuid,  uint8_t** local_send_buffer, uint32_t* data_offset)
{
    uint32_t uuid32;
    char uuid_string[40];

    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, TYPE_STRING_KEY ":", *data_offset );
    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, "\"",                *data_offset );

    if ( uuid.len == LEN_UUID_16 || uuid.len == LEN_UUID_32 )
    {
        if ( uuid.len == LEN_UUID_16 )
            uuid32 = (uint32_t)uuid.uu.uuid16;
        else
            uuid32 = uuid.uu.uuid32;

        sprintf(uuid_string, "%08x", (UINT)uuid32);

        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, uuid_string,  *data_offset );
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, HOMEKIT_UUID, *data_offset );
    }
    else if ( uuid.len == LEN_UUID_128 )
    {
        uint32_t uuid1;
        uint16_t uuid2;
        uint16_t uuid3;
        uint16_t uuid4;
        uint16_t uuid5;
        uint32_t uuid6;

        memcpy(&uuid1, &uuid.uu.uuid128[12], sizeof(uint32_t));
        memcpy(&uuid2, &uuid.uu.uuid128[10], sizeof(uint16_t));
        memcpy(&uuid3, &uuid.uu.uuid128[8], sizeof(uint16_t));
        memcpy(&uuid4, &uuid.uu.uuid128[6], sizeof(uint16_t));
        memcpy(&uuid5, &uuid.uu.uuid128[4], sizeof(uint16_t));
        memcpy(&uuid6, &uuid.uu.uuid128[0], sizeof(uint32_t));

        sprintf(uuid_string, "%08x-%04x-%04x-%04x-%04x%08x", (UINT)uuid1, (UINT)uuid2, (UINT)uuid3, (UINT)uuid4, (UINT)uuid5, (UINT)uuid6);

        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, uuid_string,  *data_offset );
    }

    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, "\"",                *data_offset );
    return WICED_SUCCESS;
}

static wiced_result_t send_instance_id( const char* instance_id_string, uint8_t instance_id, uint8_t** local_send_buffer, uint32_t* data_offset)
{
    char string_number[4];

    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, instance_id_string, *data_offset );
    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, ":",                *data_offset );

    unsigned_to_decimal_string( instance_id, string_number, 1, 4);

    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, string_number, *data_offset );

    return WICED_SUCCESS;
}

static wiced_result_t send_permissions(uint16_t properties, uint8_t** local_send_buffer, uint32_t* data_offset)
{
    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, PERMISSIONS_STRING_KEY ":", *data_offset );
    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, "[",                        *data_offset );

    if ( (properties & HAP_CHARACTERISTIC_PROPERTY_READ) ||
         (properties & HAP_CHARACTERISTIC_PROPERTY_SECURE_READ) )
    {
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, PAIRED_READ_STRING_KEY ",", *data_offset );
    }
    if ( (properties & HAP_CHARACTERISTIC_PROPERTY_WRITE) ||
         (properties & HAP_CHARACTERISTIC_PROPERTY_SECURE_WRITE) )
    {
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, PAIRED_WRITE_STRING_KEY ",", *data_offset );
    }
    if (properties & HAP_CHARACTERISTIC_PROPERTY_ADDITIONAL_AUTH_DATA)
    {
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, ADDITIONAL_AUTH_STRING_KEY ",", *data_offset );
    }
    if (properties & HAP_CHARACTERISTIC_PROPERTY_TIMED_WRITE_PROCEDURE)
    {
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, TIMED_WRITE_STRING_KEY ",", *data_offset );
    }
    if (properties & HAP_CHARACTERISTIC_PROPERTY_INVISIBLE_TO_USER)
    {
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, HIDDEN_STRING_KEY ",", *data_offset );
    }
    if ( (properties & HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_CONNECTED) ||
         (properties & HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_DISCONNECTED) )
    {
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, EVENTS_STRING_KEY ",", *data_offset );
    }
    if (properties != 0)
        *data_offset -= 1;

    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, "]", *data_offset );

    return WICED_SUCCESS;
}

static wiced_result_t send_value( tunneled_accessory_t* accessory, tunneled_characteristic_t* characteristic, uint8_t** local_send_buffer, uint32_t* data_offset)
{
    char value_string[20];

    if ( characteristic->inst_id > accessory->max_ble_inst_id )
        tunnel_get_tunneled_accessory_value(accessory, characteristic, value_string, 20);
    else
        tunnel_get_null_value(characteristic->format, value_string, 20);

    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, VALUE_STRING_KEY ":" , *data_offset );
    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, value_string, *data_offset );

    return WICED_SUCCESS;
}

static wiced_result_t send_unit_type( uint16_t unit, uint8_t** local_send_buffer, uint32_t* data_offset)
{
    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, UNIT_STRING_KEY ":", *data_offset );

    switch ( unit )
    {
    case HAP_CHARACTERISTIC_FORMAT_UNIT_CELSIUS:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, UNIT_CELCIUS_STRING_TYPE, *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_UNIT_PERCENTAGE:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, UNIT_PERCENTAGE_STRING_TYPE, *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_UNIT_ARCDEGREES:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, UNIT_ARC_DEGREES_STRING_TYPE, *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_UNIT_SECONDS:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, UNIT_SECONDS_STRING_TYPE, *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_UNIT_LUX:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, UNIT_LUX_STRING_TYPE, *data_offset );
        break;

    default:
        return WICED_BADARG;
        break;
    }

    return WICED_SUCCESS;
}

static wiced_result_t send_format_type( uint8_t format, uint8_t** local_send_buffer, uint32_t* data_offset)
{
    WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, FORMAT_STRING_KEY ":", *data_offset );

    switch( format )
    {
    case HAP_CHARACTERISTIC_FORMAT_BOOL:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, FORMAT_STRING_BOOL_TYPE   , *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_UINT8:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, FORMAT_STRING_UINT8_TYPE  , *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_UINT16:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, FORMAT_STRING_UINT16_TYPE , *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_UINT32:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, FORMAT_STRING_UINT32_TYPE , *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_UINT64:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, FORMAT_STRING_UINT64_TYPE , *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_INT32:
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, FORMAT_STRING_INTEGER_TYPE, *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_FLOAT  :
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, FORMAT_STRING_FLOAT_TYPE  , *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_STRING :
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer, FORMAT_STRING_STRING_TYPE , *data_offset );
        break;

    case HAP_CHARACTERISTIC_FORMAT_TLV8 :
        WRITE_NULL_TERMINATED_DATA_TO_BUFFER( *local_send_buffer,  FORMAT_STRING_TLV8_TYPE  , *data_offset );
        break;

    default:
        return WICED_BADARG;
        break;
    }

    return WICED_SUCCESS;
}

uint8_t value_to_string(hap_characteristic_value_t value, uint8_t format, char* buffer, int max_length)
{
    uint8_t ret = 0;

    memset(buffer, 0x0, max_length);

    switch (format)
    {
    case HAP_CHARACTERISTIC_FORMAT_UINT8:
    case HAP_CHARACTERISTIC_FORMAT_UINT16:
    case HAP_CHARACTERISTIC_FORMAT_UINT32:
    case HAP_CHARACTERISTIC_FORMAT_UINT64:
        ret = unsigned_to_decimal_string(value.value_u32, buffer, 1, max_length);
        break;
    case HAP_CHARACTERISTIC_FORMAT_INT32:
        ret = signed_to_decimal_string(value.value_i32, buffer, 1, max_length);
        break;
    case HAP_CHARACTERISTIC_FORMAT_FLOAT:
        ret = float_to_string(buffer, max_length, value.value_f, 4);
        break;
    }

    return ret;
}
