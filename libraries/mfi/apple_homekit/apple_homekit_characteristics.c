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
#include "apple_homekit_characteristics.h"

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

void wiced_homekit_initialise_first_value( wiced_homekit_value_descriptor_t* characteristic_value, char* value, uint8_t value_length, char* name );

/******************************************************
 *               Variables Definitions
 ******************************************************/
static char identify_value[sizeof("false")];

/******************************************************
 *               Function Definitions
 ******************************************************/

void wiced_homekit_initialise_on_characteristic( wiced_homekit_characteristics_t* on_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
     wiced_homekit_initialise_first_value( &on_characteristic->value.current, value, value_length, value_name );

     on_characteristic->description                   = description;
     on_characteristic->instance_id                   = instance_id;
     on_characteristic->type                          = HOMEKIT_ON_UUID;
     on_characteristic->permissions                   = HOMEKIT_ON_PERMISSIONS;
     on_characteristic->format                        = HOMEKIT_ON_FORMAT;
     on_characteristic->unit                          = HOMEKIT_ON_UNIT;
     on_characteristic->minimum_value                 = HOMEKIT_ON_MINIMUM_VALUE;
     on_characteristic->maximum_value                 = HOMEKIT_ON_MAXIMUM_VALUE;
     on_characteristic->minimum_step                  = HOMEKIT_ON_STEP_VALUE;
     on_characteristic->maximum_length                = HOMEKIT_ON_MAXIMUM_LENGTH;
     on_characteristic->maximum_data_length           = HOMEKIT_ON_MAXIMUM_DATA_LENGTH;
     on_characteristic->identify_callback             = NULL;
}

void wiced_homekit_initialise_name_characteristic( wiced_homekit_characteristics_t* name_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &name_characteristic->value.current, value, value_length, value_name );

    name_characteristic->description           = description;
    name_characteristic->instance_id           = instance_id;
    name_characteristic->type                  = HOMEKIT_NAME_UUID;
    name_characteristic->permissions           = HOMEKIT_NAME_PERMISSIONS;
    name_characteristic->format                = HOMEKIT_NAME_FORMAT;
    name_characteristic->unit                  = HOMEKIT_NAME_UNIT;
    name_characteristic->minimum_value         = HOMEKIT_NAME_MINIMUM_VALUE;
    name_characteristic->maximum_value         = HOMEKIT_NAME_MAXIMUM_VALUE;
    name_characteristic->minimum_step          = HOMEKIT_NAME_STEP_VALUE;
    name_characteristic->maximum_length        = HOMEKIT_NAME_MAXIMUM_LENGTH;
    name_characteristic->maximum_data_length   = HOMEKIT_NAME_MAXIMUM_DATA_LENGTH;
    name_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_saturation_characteristic( wiced_homekit_characteristics_t* saturation_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &saturation_characteristic->value.current, value, value_length, value_name );

    saturation_characteristic->description           = description;
    saturation_characteristic->instance_id           = instance_id;
    saturation_characteristic->type                  = HOMEKIT_SATURATION_UUID;
    saturation_characteristic->permissions           = HOMEKIT_SATURATION_PERMISSIONS;
    saturation_characteristic->format                = HOMEKIT_SATURATION_FORMAT;
    saturation_characteristic->unit                  = HOMEKIT_SATURATION_UNIT;
    saturation_characteristic->minimum_value         = HOMEKIT_SATURATION_MINIMUM_VALUE;
    saturation_characteristic->maximum_value         = HOMEKIT_SATURATION_MAXIMUM_VALUE;
    saturation_characteristic->minimum_step          = HOMEKIT_SATURATION_STEP_VALUE;
    saturation_characteristic->maximum_length        = HOMEKIT_SATURATION_MAXIMUM_LENGTH;
    saturation_characteristic->maximum_data_length   = HOMEKIT_SATURATION_MAXIMUM_DATA_LENGTH;
    saturation_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_brightness_characteristic( wiced_homekit_characteristics_t* brightness_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &brightness_characteristic->value.current, value, value_length, value_name );

    brightness_characteristic->description           = description;
    brightness_characteristic->instance_id           = instance_id;
    brightness_characteristic->type                  = HOMEKIT_BRIGHTNESS_UUID;
    brightness_characteristic->permissions           = HOMEKIT_BRIGHTNESS_PERMISSIONS;
    brightness_characteristic->format                = HOMEKIT_BRIGHTNESS_FORMAT;
    brightness_characteristic->unit                  = HOMEKIT_BRIGHTNESS_UNIT;
    brightness_characteristic->minimum_value         = HOMEKIT_BRIGHTNESS_MINIMUM_VALUE;
    brightness_characteristic->maximum_value         = HOMEKIT_BRIGHTNESS_MAXIMUM_VALUE;
    brightness_characteristic->minimum_step          = HOMEKIT_BRIGHTNESS_STEP_VALUE;
    brightness_characteristic->maximum_length        = HOMEKIT_BRIGHTNESS_MAXIMUM_LENGTH;
    brightness_characteristic->maximum_data_length   = HOMEKIT_BRIGHTNESS_MAXIMUM_DATA_LENGTH;
    brightness_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_hue_characteristic( wiced_homekit_characteristics_t* hue_characteristic, char* value, uint8_t value_length, char* value_name,  char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &hue_characteristic->value.current, value, value_length, value_name );

    hue_characteristic->description           = description;
    hue_characteristic->instance_id           = instance_id;
    hue_characteristic->type                  = HOMEKIT_HUE_UUID;
    hue_characteristic->permissions           = HOMEKIT_HUE_PERMISSIONS;
    hue_characteristic->format                = HOMEKIT_HUE_FORMAT;
    hue_characteristic->unit                  = HOMEKIT_HUE_UNIT;
    hue_characteristic->minimum_value         = HOMEKIT_HUE_MINIMUM_VALUE;
    hue_characteristic->maximum_value         = HOMEKIT_HUE_MAXIMUM_VALUE;
    hue_characteristic->minimum_step          = HOMEKIT_HUE_STEP_VALUE;
    hue_characteristic->maximum_length        = HOMEKIT_HUE_MAXIMUM_LENGTH;
    hue_characteristic->maximum_data_length   = HOMEKIT_HUE_MAXIMUM_DATA_LENGTH;
    hue_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_current_door_state_characteristic( wiced_homekit_characteristics_t* current_door_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &current_door_state_characteristic->value.current, value, value_length, value_name );

    current_door_state_characteristic->description           = description;
    current_door_state_characteristic->instance_id           = instance_id;
    current_door_state_characteristic->type                  = HOMEKIT_CURRENT_DOOR_STATE_UUID;
    current_door_state_characteristic->permissions           = HOMEKIT_CURRENT_DOOR_STATE_PERMISSIONS;
    current_door_state_characteristic->format                = HOMEKIT_CURRENT_DOOR_STATE_FORMAT;
    current_door_state_characteristic->unit                  = HOMEKIT_CURRENT_DOOR_STATE_UNIT;
    current_door_state_characteristic->minimum_value         = HOMEKIT_CURRENT_DOOR_STATE_MINIMUM_VALUE;
    current_door_state_characteristic->maximum_value         = HOMEKIT_CURRENT_DOOR_STATE_MAXIMUM_VALUE;
    current_door_state_characteristic->minimum_step          = HOMEKIT_CURRENT_DOOR_STATE_STEP_VALUE;
    current_door_state_characteristic->maximum_length        = HOMEKIT_CURRENT_DOOR_STATE_MAXIMUM_LENGTH;
    current_door_state_characteristic->maximum_data_length   = HOMEKIT_CURRENT_DOOR_STATE_MAXIMUM_DATA_LENGTH;
    current_door_state_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_target_door_state_characteristic( wiced_homekit_characteristics_t* target_door_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &target_door_state_characteristic->value.current, value, value_length, value_name );

    target_door_state_characteristic->description           = description;
    target_door_state_characteristic->instance_id           = instance_id;
    target_door_state_characteristic->type                  = HOMEKIT_TARGET_DOOR_STATE_UUID;
    target_door_state_characteristic->permissions           = HOMEKIT_TARGET_DOOR_STATE_PERMISSIONS;
    target_door_state_characteristic->format                = HOMEKIT_TARGET_DOOR_STATE_FORMAT;
    target_door_state_characteristic->unit                  = HOMEKIT_TARGET_DOOR_STATE_UNIT;
    target_door_state_characteristic->minimum_value         = HOMEKIT_TARGET_DOOR_STATE_MINIMUM_VALUE;
    target_door_state_characteristic->maximum_value         = HOMEKIT_TARGET_DOOR_STATE_MAXIMUM_VALUE;
    target_door_state_characteristic->minimum_step          = HOMEKIT_TARGET_DOOR_STATE_STEP_VALUE;
    target_door_state_characteristic->maximum_length        = HOMEKIT_TARGET_DOOR_STATE_MAXIMUM_LENGTH;
    target_door_state_characteristic->maximum_data_length   = HOMEKIT_TARGET_DOOR_STATE_MAXIMUM_DATA_LENGTH;
    target_door_state_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_lock_mechanism_current_state_characteristic ( wiced_homekit_characteristics_t* lock_mechanism_current_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &lock_mechanism_current_state_characteristic->value.current, value, value_length, value_name );

    lock_mechanism_current_state_characteristic->description           = description;
    lock_mechanism_current_state_characteristic->instance_id           = instance_id;
    lock_mechanism_current_state_characteristic->type                  = HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_UUID;
    lock_mechanism_current_state_characteristic->permissions           = HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_PERMISSIONS;
    lock_mechanism_current_state_characteristic->format                = HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_FORMAT;
    lock_mechanism_current_state_characteristic->unit                  = HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_UNIT;
    lock_mechanism_current_state_characteristic->minimum_value         = HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_MINIMUM_VALUE;
    lock_mechanism_current_state_characteristic->maximum_value         = HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_MAXIMUM_VALUE;
    lock_mechanism_current_state_characteristic->minimum_step          = HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_STEP_VALUE;
    lock_mechanism_current_state_characteristic->maximum_length        = HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_MAXIMUM_LENGTH;
    lock_mechanism_current_state_characteristic->maximum_data_length   = HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_MAXIMUM_DATA_LENGTH;
    lock_mechanism_current_state_characteristic->identify_callback     = NULL;
}


void wiced_homekit_initialise_lock_mechanism_target_state_characteristic ( wiced_homekit_characteristics_t* lock_mechanism_target_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &lock_mechanism_target_state_characteristic->value.current, value, value_length, value_name );

    lock_mechanism_target_state_characteristic->description           = description;
    lock_mechanism_target_state_characteristic->instance_id           = instance_id;
    lock_mechanism_target_state_characteristic->type                  = HOMEKIT_LOCK_MECHANISM_TARGET_STATE_UUID;
    lock_mechanism_target_state_characteristic->permissions           = HOMEKIT_LOCK_MECHANISM_TARGET_STATE_PERMISSIONS;
    lock_mechanism_target_state_characteristic->format                = HOMEKIT_LOCK_MECHANISM_TARGET_STATE_FORMAT;
    lock_mechanism_target_state_characteristic->unit                  = HOMEKIT_LOCK_MECHANISM_TARGET_STATE_UNIT;
    lock_mechanism_target_state_characteristic->minimum_value         = HOMEKIT_LOCK_MECHANISM_TARGET_STATE_MINIMUM_VALUE;
    lock_mechanism_target_state_characteristic->maximum_value         = HOMEKIT_LOCK_MECHANISM_TARGET_STATE_MAXIMUM_VALUE;
    lock_mechanism_target_state_characteristic->minimum_step          = HOMEKIT_LOCK_MECHANISM_TARGET_STATE_STEP_VALUE;
    lock_mechanism_target_state_characteristic->maximum_length        = HOMEKIT_LOCK_MECHANISM_TARGET_STATE_MAXIMUM_LENGTH;
    lock_mechanism_target_state_characteristic->maximum_data_length   = HOMEKIT_LOCK_MECHANISM_TARGET_STATE_MAXIMUM_DATA_LENGTH;
    lock_mechanism_target_state_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_logs_characteristic ( wiced_homekit_characteristics_t* logs_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &logs_characteristic->value.current, value, value_length, value_name );

    logs_characteristic->description           = description;
    logs_characteristic->instance_id           = instance_id;
    logs_characteristic->type                  = HOMEKIT_LOGS_UUID;
    logs_characteristic->permissions           = HOMEKIT_LOGS_PERMISSIONS;
    logs_characteristic->format                = HOMEKIT_LOGS_FORMAT;
    logs_characteristic->unit                  = HOMEKIT_LOGS_UNIT;
    logs_characteristic->minimum_value         = HOMEKIT_LOGS_MINIMUM_VALUE;
    logs_characteristic->maximum_value         = HOMEKIT_LOGS_MAXIMUM_VALUE;
    logs_characteristic->minimum_step          = HOMEKIT_LOGS_STEP_VALUE;
    logs_characteristic->maximum_length        = HOMEKIT_LOGS_MAXIMUM_LENGTH;
    logs_characteristic->maximum_data_length   = HOMEKIT_LOGS_MAXIMUM_DATA_LENGTH;
    logs_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_obstruction_detected_characteristic ( wiced_homekit_characteristics_t* obstruction_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &obstruction_detected_characteristic->value.current, value, value_length, value_name );

    obstruction_detected_characteristic->description           = description;
    obstruction_detected_characteristic->instance_id           = instance_id;
    obstruction_detected_characteristic->type                  = HOMEKIT_OBSTRUCTION_DETECTED_UUID;
    obstruction_detected_characteristic->permissions           = HOMEKIT_OBSTRUCTION_DETECTED_PERMISSIONS;
    obstruction_detected_characteristic->format                = HOMEKIT_OBSTRUCTION_DETECTED_FORMAT;
    obstruction_detected_characteristic->unit                  = HOMEKIT_OBSTRUCTION_DETECTED_UNIT;
    obstruction_detected_characteristic->minimum_value         = HOMEKIT_OBSTRUCTION_DETECTED_MINIMUM_VALUE;
    obstruction_detected_characteristic->maximum_value         = HOMEKIT_OBSTRUCTION_DETECTED_MAXIMUM_VALUE;
    obstruction_detected_characteristic->minimum_step          = HOMEKIT_OBSTRUCTION_DETECTED_STEP_VALUE;
    obstruction_detected_characteristic->maximum_length        = HOMEKIT_OBSTRUCTION_DETECTED_MAXIMUM_LENGTH;
    obstruction_detected_characteristic->maximum_data_length   = HOMEKIT_OBSTRUCTION_DETECTED_MAXIMUM_DATA_LENGTH;
    obstruction_detected_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_identify_characteristic( wiced_homekit_characteristics_t* identify_characteristic, wiced_homekit_identify_callback_t identify_callback, uint16_t instance_id )
{
    strlcpy(identify_value, (char*)HOMEKIT_IDENTIFY_VALUE, sizeof(identify_value));
    identify_value[sizeof(identify_value) - 1] = '\0';
    wiced_homekit_initialise_first_value( &identify_characteristic->value.current, identify_value, (uint8_t)strlen(identify_value), NULL );
    identify_characteristic->type                  = HOMEKIT_IDENTIFY_UUID;
    identify_characteristic->permissions           = HOMEKIT_IDENTIFY_PERMISSIONS;
    identify_characteristic->format                = HOMEKIT_IDENTIFY_FORMAT;
    identify_characteristic->unit                  = HOMEKIT_IDENTIFY_UNIT;
    identify_characteristic->minimum_value         = HOMEKIT_IDENTIFY_MINIMUM_VALUE;
    identify_characteristic->maximum_value         = HOMEKIT_IDENTIFY_MAXIMUM_VALUE;
    identify_characteristic->minimum_step          = HOMEKIT_IDENTIFY_STEP_VALUE;
    identify_characteristic->maximum_length        = HOMEKIT_IDENTIFY_MAXIMUM_LENGTH;
    identify_characteristic->maximum_data_length   = HOMEKIT_IDENTIFY_MAXIMUM_DATA_LENGTH;
    identify_characteristic->identify_callback     = identify_callback;
    identify_characteristic->instance_id           = instance_id;

}

void wiced_homekit_initialise_manufacturer_characteristic( wiced_homekit_characteristics_t* manufacturer_characteristic, char* value, uint8_t value_length, char* value_name,  char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &manufacturer_characteristic->value.current, value, value_length, value_name );

    manufacturer_characteristic->description           = description;
    manufacturer_characteristic->instance_id           = instance_id;
    manufacturer_characteristic->type                  = HOMEKIT_MANUFACTURER_UUID;
    manufacturer_characteristic->permissions           = HOMEKIT_MANUFACTURER_PERMISSIONS;
    manufacturer_characteristic->format                = HOMEKIT_MANUFACTURER_FORMAT;
    manufacturer_characteristic->unit                  = HOMEKIT_MANUFACTURER_UNIT;
    manufacturer_characteristic->minimum_value         = HOMEKIT_MANUFACTURER_MINIMUM_VALUE;
    manufacturer_characteristic->maximum_value         = HOMEKIT_MANUFACTURER_MAXIMUM_VALUE;
    manufacturer_characteristic->minimum_step          = HOMEKIT_MANUFACTURER_STEP_VALUE;
    manufacturer_characteristic->maximum_length        = HOMEKIT_MANUFACTURER_MAXIMUM_LENGTH;
    manufacturer_characteristic->maximum_data_length   = HOMEKIT_MANUFACTURER_MAXIMUM_DATA_LENGTH;
    manufacturer_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_model_characteristic( wiced_homekit_characteristics_t* model_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &model_characteristic->value.current, value, value_length, value_name );

    model_characteristic->description           = description;
    model_characteristic->instance_id           = instance_id;
    model_characteristic->type                  = HOMEKIT_MODEL_UUID;
    model_characteristic->permissions           = HOMEKIT_MODEL_PERMISSIONS;
    model_characteristic->format                = HOMEKIT_MODEL_FORMAT;
    model_characteristic->unit                  = HOMEKIT_MODEL_UNIT;
    model_characteristic->minimum_value         = HOMEKIT_MODEL_MINIMUM_VALUE;
    model_characteristic->maximum_value         = HOMEKIT_MODEL_MAXIMUM_VALUE;
    model_characteristic->minimum_step          = HOMEKIT_MODEL_STEP_VALUE;
    model_characteristic->maximum_length        = HOMEKIT_MODEL_MAXIMUM_LENGTH;
    model_characteristic->maximum_data_length   = HOMEKIT_MODEL_MAXIMUM_DATA_LENGTH;
    model_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_serial_number_characteristic( wiced_homekit_characteristics_t* serial_number_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &serial_number_characteristic->value.current, value, value_length, value_name );

    serial_number_characteristic->description           = description;
    serial_number_characteristic->instance_id           = instance_id;
    serial_number_characteristic->type                  = HOMEKIT_SERIAL_NUMBER_UUID;
    serial_number_characteristic->permissions           = HOMEKIT_SERIAL_NUMBER_PERMISSIONS;
    serial_number_characteristic->format                = HOMEKIT_SERIAL_NUMBER_FORMAT;
    serial_number_characteristic->unit                  = HOMEKIT_SERIAL_NUMBER_UNIT;
    serial_number_characteristic->minimum_value         = HOMEKIT_SERIAL_NUMBER_MINIMUM_VALUE;
    serial_number_characteristic->maximum_value         = HOMEKIT_SERIAL_NUMBER_MAXIMUM_VALUE;
    serial_number_characteristic->minimum_step          = HOMEKIT_SERIAL_NUMBER_STEP_VALUE;
    serial_number_characteristic->maximum_length        = HOMEKIT_SERIAL_NUMBER_MAXIMUM_LENGTH;
    serial_number_characteristic->maximum_data_length   = HOMEKIT_SERIAL_NUMBER_MAXIMUM_DATA_LENGTH;
    serial_number_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_firmwar_revision_characteristic( wiced_homekit_characteristics_t* characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &characteristic->value.current, value, value_length, value_name );

    characteristic->description           = description;
    characteristic->instance_id           = instance_id;
    characteristic->type                  = HOMEKIT_FIRMWARE_REVISION_UUID;
    characteristic->permissions           = HOMEKIT_FIRMWARE_REVISION_PERMISSIONS;
    characteristic->format                = HOMEKIT_FIRMWARE_REVISION_FORMAT;
    characteristic->unit                  = HOMEKIT_FIRMWARE_REVISION_UNIT;
    characteristic->minimum_value         = HOMEKIT_FIRMWARE_REVISION_MINIMUM_VALUE;
    characteristic->maximum_value         = HOMEKIT_FIRMWARE_REVISION_MAXIMUM_VALUE;
    characteristic->minimum_step          = HOMEKIT_FIRMWARE_REVISION_STEP_VALUE;
    characteristic->maximum_length        = HOMEKIT_FIRMWARE_REVISION_MAXIMUM_LENGTH;
    characteristic->maximum_data_length   = HOMEKIT_FIRMWARE_REVISION_MAXIMUM_DATA_LENGTH;
    characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_heating_cooling_current_characteristic( wiced_homekit_characteristics_t* heating_cooling_current_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &heating_cooling_current_characteristic->value.current, value, value_length, value_name );

    heating_cooling_current_characteristic->description           = description;
    heating_cooling_current_characteristic->instance_id           = instance_id;
    heating_cooling_current_characteristic->type                  = HOMEKIT_HEATING_COOLING_CURRENT_UUID;
    heating_cooling_current_characteristic->permissions           = HOMEKIT_HEATING_COOLING_CURRENT_PERMISSIONS;
    heating_cooling_current_characteristic->format                = HOMEKIT_HEATING_COOLING_CURRENT_FORMAT;
    heating_cooling_current_characteristic->unit                  = HOMEKIT_HEATING_COOLING_CURRENT_UNIT;
    heating_cooling_current_characteristic->minimum_value         = HOMEKIT_HEATING_COOLING_CURRENT_MINIMUM_VALUE;
    heating_cooling_current_characteristic->maximum_value         = HOMEKIT_HEATING_COOLING_CURRENT_MAXIMUM_VALUE;
    heating_cooling_current_characteristic->minimum_step          = HOMEKIT_HEATING_COOLING_CURRENT_STEP_VALUE;
    heating_cooling_current_characteristic->maximum_length        = HOMEKIT_HEATING_COOLING_CURRENT_MAXIMUM_LENGTH;
    heating_cooling_current_characteristic->maximum_data_length   = HOMEKIT_HEATING_COOLING_CURRENT_MAXIMUM_DATA_LENGTH;
    heating_cooling_current_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_heating_cooling_target_characteristic( wiced_homekit_characteristics_t* heating_cooling_target_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &heating_cooling_target_characteristic->value.current, value, value_length, value_name );

    heating_cooling_target_characteristic->description           = description;
    heating_cooling_target_characteristic->instance_id           = instance_id;
    heating_cooling_target_characteristic->type                  = HOMEKIT_HEATING_COOLING_TARGET_UUID;
    heating_cooling_target_characteristic->permissions           = HOMEKIT_HEATING_COOLING_TARGET_PERMISSIONS;
    heating_cooling_target_characteristic->format                = HOMEKIT_HEATING_COOLING_TARGET_FORMAT;
    heating_cooling_target_characteristic->unit                  = HOMEKIT_HEATING_COOLING_TARGET_UNIT;
    heating_cooling_target_characteristic->minimum_value         = HOMEKIT_HEATING_COOLING_TARGET_MINIMUM_VALUE;
    heating_cooling_target_characteristic->maximum_value         = HOMEKIT_HEATING_COOLING_TARGET_MAXIMUM_VALUE;
    heating_cooling_target_characteristic->minimum_step          = HOMEKIT_HEATING_COOLING_TARGET_STEP_VALUE;
    heating_cooling_target_characteristic->maximum_length        = HOMEKIT_HEATING_COOLING_TARGET_MAXIMUM_LENGTH;
    heating_cooling_target_characteristic->maximum_data_length   = HOMEKIT_HEATING_COOLING_TARGET_MAXIMUM_DATA_LENGTH;
    heating_cooling_target_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_temperature_current_characteristic( wiced_homekit_characteristics_t* temperature_current_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &temperature_current_characteristic->value.current, value, value_length, value_name );

    temperature_current_characteristic->description           = description;
    temperature_current_characteristic->instance_id           = instance_id;
    temperature_current_characteristic->type                  = HOMEKIT_TEMPERATURE_CURRENT_UUID;
    temperature_current_characteristic->permissions           = HOMEKIT_TEMPERATURE_CURRENT_PERMISSIONS;
    temperature_current_characteristic->format                = HOMEKIT_TEMPERATURE_CURRENT_FORMAT;
    temperature_current_characteristic->unit                  = HOMEKIT_TEMPERATURE_CURRENT_UNIT;
    temperature_current_characteristic->minimum_value         = HOMEKIT_TEMPERATURE_CURRENT_MINIMUM_VALUE;
    temperature_current_characteristic->maximum_value         = HOMEKIT_TEMPERATURE_CURRENT_MAXIMUM_VALUE;
    temperature_current_characteristic->minimum_step          = HOMEKIT_TEMPERATURE_CURRENT_STEP_VALUE;
    temperature_current_characteristic->maximum_length        = HOMEKIT_TEMPERATURE_CURRENT_MAXIMUM_LENGTH;
    temperature_current_characteristic->maximum_data_length   = HOMEKIT_TEMPERATURE_CURRENT_MAXIMUM_DATA_LENGTH;
    temperature_current_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_temperature_target_characteristic( wiced_homekit_characteristics_t* temperature_target_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &temperature_target_characteristic->value.current, value, value_length, value_name );

    temperature_target_characteristic->description           = description;
    temperature_target_characteristic->instance_id           = instance_id;
    temperature_target_characteristic->type                  = HOMEKIT_TEMPERATURE_TARGET_UUID;
    temperature_target_characteristic->permissions           = HOMEKIT_TEMPERATURE_TARGET_PERMISSIONS;
    temperature_target_characteristic->format                = HOMEKIT_TEMPERATURE_TARGET_FORMAT;
    temperature_target_characteristic->unit                  = HOMEKIT_TEMPERATURE_TARGET_UNIT;
    temperature_target_characteristic->minimum_value         = HOMEKIT_TEMPERATURE_TARGET_MINIMUM_VALUE;
    temperature_target_characteristic->maximum_value         = HOMEKIT_TEMPERATURE_TARGET_MAXIMUM_VALUE;
    temperature_target_characteristic->minimum_step          = HOMEKIT_TEMPERATURE_TARGET_STEP_VALUE;
    temperature_target_characteristic->maximum_length        = HOMEKIT_TEMPERATURE_TARGET_MAXIMUM_LENGTH;
    temperature_target_characteristic->maximum_data_length   = HOMEKIT_TEMPERATURE_TARGET_MAXIMUM_DATA_LENGTH;
    temperature_target_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_temperature_units_characteristic( wiced_homekit_characteristics_t* temperature_units_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &temperature_units_characteristic->value.current, value, value_length, value_name );

    temperature_units_characteristic->description           = description;
    temperature_units_characteristic->instance_id           = instance_id;
    temperature_units_characteristic->type                  = HOMEKIT_TEMPERATURE_UNITS_UUID;
    temperature_units_characteristic->permissions           = HOMEKIT_TEMPERATURE_UNITS_PERMISSIONS;
    temperature_units_characteristic->format                = HOMEKIT_TEMPERATURE_UNITS_FORMAT;
    temperature_units_characteristic->unit                  = HOMEKIT_TEMPERATURE_UNITS_UNIT;
    temperature_units_characteristic->minimum_value         = HOMEKIT_TEMPERATURE_UNITS_MINIMUM_VALUE;
    temperature_units_characteristic->maximum_value         = HOMEKIT_TEMPERATURE_UNITS_MAXIMUM_VALUE;
    temperature_units_characteristic->minimum_step          = HOMEKIT_TEMPERATURE_UNITS_STEP_VALUE;
    temperature_units_characteristic->maximum_length        = HOMEKIT_TEMPERATURE_UNITS_MAXIMUM_LENGTH;
    temperature_units_characteristic->maximum_data_length   = HOMEKIT_TEMPERATURE_UNITS_MAXIMUM_DATA_LENGTH;
    temperature_units_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_outlet_in_use_characteristic( wiced_homekit_characteristics_t* outlet_in_use_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &outlet_in_use_characteristic->value.current, value, value_length, value_name );

    outlet_in_use_characteristic->description           = description;
    outlet_in_use_characteristic->instance_id           = instance_id;
    outlet_in_use_characteristic->type                  = HOMEKIT_OUTLET_IN_USE_UUID;
    outlet_in_use_characteristic->permissions           = HOMEKIT_OUTLET_IN_USE_PERMISSIONS;
    outlet_in_use_characteristic->format                = HOMEKIT_OUTLET_IN_USE_FORMAT;
    outlet_in_use_characteristic->unit                  = HOMEKIT_OUTLET_IN_USE_UNIT;
    outlet_in_use_characteristic->minimum_value         = HOMEKIT_OUTLET_IN_USE_MINIMUM_VALUE;
    outlet_in_use_characteristic->maximum_value         = HOMEKIT_OUTLET_IN_USE_MAXIMUM_VALUE;
    outlet_in_use_characteristic->minimum_step          = HOMEKIT_OUTLET_IN_USE_STEP_VALUE;
    outlet_in_use_characteristic->maximum_length        = HOMEKIT_OUTLET_IN_USE_MAXIMUM_LENGTH;
    outlet_in_use_characteristic->maximum_data_length   = HOMEKIT_OUTLET_IN_USE_MAXIMUM_DATA_LENGTH;
    outlet_in_use_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_version_characteristic( wiced_homekit_characteristics_t* version_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &version_characteristic->value.current, value, value_length, value_name );

    version_characteristic->description           = description;
    version_characteristic->instance_id           = instance_id;
    version_characteristic->type                  = HOMEKIT_VERSION_UUID;
    version_characteristic->permissions           = HOMEKIT_VERSION_PERMISSIONS;
    version_characteristic->format                = HOMEKIT_VERSION_FORMAT;
    version_characteristic->unit                  = HOMEKIT_VERSION_UNIT;
    version_characteristic->minimum_value         = HOMEKIT_VERSION_MINIMUM_VALUE;
    version_characteristic->maximum_value         = HOMEKIT_VERSION_MAXIMUM_VALUE;
    version_characteristic->minimum_step          = HOMEKIT_VERSION_STEP_VALUE;
    version_characteristic->maximum_length        = HOMEKIT_VERSION_MAXIMUM_LENGTH;
    version_characteristic->maximum_data_length   = HOMEKIT_VERSION_MAXIMUM_DATA_LENGTH;
    version_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_lock_management_control_point_characteristic( wiced_homekit_characteristics_t* control_point_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &control_point_characteristic->value.current, value, value_length, value_name );

    control_point_characteristic->description           = description;
    control_point_characteristic->instance_id           = instance_id;
    control_point_characteristic->type                  = HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_UUID;
    control_point_characteristic->permissions           = HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_PERMISSIONS;
    control_point_characteristic->format                = HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_FORMAT;
    control_point_characteristic->unit                  = HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_UNIT;
    control_point_characteristic->minimum_value         = HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_MINIMUM_VALUE;
    control_point_characteristic->maximum_value         = HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_MAXIMUM_VALUE;
    control_point_characteristic->minimum_step          = HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_STEP_VALUE;
    control_point_characteristic->maximum_length        = HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_MAXIMUM_LENGTH;
    control_point_characteristic->maximum_data_length   = HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_MAXIMUM_DATA_LENGTH;
    control_point_characteristic->identify_callback     = NULL;
}


void wiced_homekit_initialise_air_particulate_density_characteristic( wiced_homekit_characteristics_t* air_particulate_density_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &air_particulate_density_characteristic->value.current, value, value_length, value_name );

    air_particulate_density_characteristic->description           = description;
    air_particulate_density_characteristic->instance_id           = instance_id;
    air_particulate_density_characteristic->type                  = HOMEKIT_AIR_PARTICULATE_DENSITY_UUID;
    air_particulate_density_characteristic->permissions           = HOMEKIT_AIR_PARTICULATE_DENSITY_PERMISSIONS;
    air_particulate_density_characteristic->format                = HOMEKIT_AIR_PARTICULATE_DENSITY_FORMAT;
    air_particulate_density_characteristic->unit                  = HOMEKIT_AIR_PARTICULATE_DENSITY_UNIT;
    air_particulate_density_characteristic->minimum_value         = HOMEKIT_AIR_PARTICULATE_DENSITY_MINIMUM_VALUE;
    air_particulate_density_characteristic->maximum_value         = HOMEKIT_AIR_PARTICULATE_DENSITY_MAXIMUM_VALUE;
    air_particulate_density_characteristic->minimum_step          = HOMEKIT_AIR_PARTICULATE_DENSITY_STEP_VALUE;
    air_particulate_density_characteristic->maximum_length        = HOMEKIT_AIR_PARTICULATE_DENSITY_MAXIMUM_LENGTH;
    air_particulate_density_characteristic->maximum_data_length   = HOMEKIT_AIR_PARTICULATE_DENSITY_MAXIMUM_DATA_LENGTH;
    air_particulate_density_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_air_particulate_size_characteristic( wiced_homekit_characteristics_t* air_particulate_size_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &air_particulate_size_characteristic->value.current, value, value_length, value_name );

    air_particulate_size_characteristic->description           = description;
    air_particulate_size_characteristic->instance_id           = instance_id;
    air_particulate_size_characteristic->type                  = HOMEKIT_AIR_PARTICULATE_SIZE_UUID;
    air_particulate_size_characteristic->permissions           = HOMEKIT_AIR_PARTICULATE_SIZE_PERMISSIONS;
    air_particulate_size_characteristic->format                = HOMEKIT_AIR_PARTICULATE_SIZE_FORMAT;
    air_particulate_size_characteristic->unit                  = HOMEKIT_AIR_PARTICULATE_SIZE_UNIT;
    air_particulate_size_characteristic->minimum_value         = HOMEKIT_AIR_PARTICULATE_SIZE_MINIMUM_VALUE;
    air_particulate_size_characteristic->maximum_value         = HOMEKIT_AIR_PARTICULATE_SIZE_MAXIMUM_VALUE;
    air_particulate_size_characteristic->minimum_step          = HOMEKIT_AIR_PARTICULATE_SIZE_STEP_VALUE;
    air_particulate_size_characteristic->maximum_length        = HOMEKIT_AIR_PARTICULATE_SIZE_MAXIMUM_LENGTH;
    air_particulate_size_characteristic->maximum_data_length   = HOMEKIT_AIR_PARTICULATE_SIZE_MAXIMUM_DATA_LENGTH;
    air_particulate_size_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_air_quality_characteristic( wiced_homekit_characteristics_t* air_quality_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &air_quality_characteristic->value.current, value, value_length, value_name );

    air_quality_characteristic->description           = description;
    air_quality_characteristic->instance_id           = instance_id;
    air_quality_characteristic->type                  = HOMEKIT_AIR_QUALITY_UUID;
    air_quality_characteristic->permissions           = HOMEKIT_AIR_QUALITY_PERMISSIONS;
    air_quality_characteristic->format                = HOMEKIT_AIR_QUALITY_FORMAT;
    air_quality_characteristic->unit                  = HOMEKIT_AIR_QUALITY_UNIT;
    air_quality_characteristic->minimum_value         = HOMEKIT_AIR_QUALITY_MINIMUM_VALUE;
    air_quality_characteristic->maximum_value         = HOMEKIT_AIR_QUALITY_MAXIMUM_VALUE;
    air_quality_characteristic->minimum_step          = HOMEKIT_AIR_QUALITY_STEP_VALUE;
    air_quality_characteristic->maximum_length        = HOMEKIT_AIR_QUALITY_MAXIMUM_LENGTH;
    air_quality_characteristic->maximum_data_length   = HOMEKIT_AIR_QUALITY_MAXIMUM_DATA_LENGTH;
    air_quality_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_status_active_characteristic( wiced_homekit_characteristics_t* status_active_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &status_active_characteristic->value.current, value, value_length, value_name );

    status_active_characteristic->description           = description;
    status_active_characteristic->instance_id           = instance_id;
    status_active_characteristic->type                  = HOMEKIT_STATUS_ACTIVE_UUID;
    status_active_characteristic->permissions           = HOMEKIT_STATUS_ACTIVE_PERMISSIONS;
    status_active_characteristic->format                = HOMEKIT_STATUS_ACTIVE_FORMAT;
    status_active_characteristic->unit                  = HOMEKIT_STATUS_ACTIVE_UNIT;
    status_active_characteristic->minimum_value         = HOMEKIT_STATUS_ACTIVE_MINIMUM_VALUE;
    status_active_characteristic->maximum_value         = HOMEKIT_STATUS_ACTIVE_MAXIMUM_VALUE;
    status_active_characteristic->minimum_step          = HOMEKIT_STATUS_ACTIVE_STEP_VALUE;
    status_active_characteristic->maximum_length        = HOMEKIT_STATUS_ACTIVE_MAXIMUM_LENGTH;
    status_active_characteristic->maximum_data_length   = HOMEKIT_STATUS_ACTIVE_MAXIMUM_DATA_LENGTH;
    status_active_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_status_fault_characteristic( wiced_homekit_characteristics_t* status_fault_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &status_fault_characteristic->value.current, value, value_length, value_name );

    status_fault_characteristic->description           = description;
    status_fault_characteristic->instance_id           = instance_id;
    status_fault_characteristic->type                  = HOMEKIT_STATUS_FAULT_UUID;
    status_fault_characteristic->permissions           = HOMEKIT_STATUS_FAULT_PERMISSIONS;
    status_fault_characteristic->format                = HOMEKIT_STATUS_FAULT_FORMAT;
    status_fault_characteristic->unit                  = HOMEKIT_STATUS_FAULT_UNIT;
    status_fault_characteristic->minimum_value         = HOMEKIT_STATUS_FAULT_MINIMUM_VALUE;
    status_fault_characteristic->maximum_value         = HOMEKIT_STATUS_FAULT_MAXIMUM_VALUE;
    status_fault_characteristic->minimum_step          = HOMEKIT_STATUS_FAULT_STEP_VALUE;
    status_fault_characteristic->maximum_length        = HOMEKIT_STATUS_FAULT_MAXIMUM_LENGTH;
    status_fault_characteristic->maximum_data_length   = HOMEKIT_STATUS_FAULT_MAXIMUM_DATA_LENGTH;
    status_fault_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_status_tampered_characteristic( wiced_homekit_characteristics_t* status_tampered_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &status_tampered_characteristic->value.current, value, value_length, value_name );

    status_tampered_characteristic->description           = description;
    status_tampered_characteristic->instance_id           = instance_id;
    status_tampered_characteristic->type                  = HOMEKIT_STATUS_TAMPERED_UUID;
    status_tampered_characteristic->permissions           = HOMEKIT_STATUS_TAMPERED_PERMISSIONS;
    status_tampered_characteristic->format                = HOMEKIT_STATUS_TAMPERED_FORMAT;
    status_tampered_characteristic->unit                  = HOMEKIT_STATUS_TAMPERED_UNIT;
    status_tampered_characteristic->minimum_value         = HOMEKIT_STATUS_TAMPERED_MINIMUM_VALUE;
    status_tampered_characteristic->maximum_value         = HOMEKIT_STATUS_TAMPERED_MAXIMUM_VALUE;
    status_tampered_characteristic->minimum_step          = HOMEKIT_STATUS_TAMPERED_STEP_VALUE;
    status_tampered_characteristic->maximum_length        = HOMEKIT_STATUS_TAMPERED_MAXIMUM_LENGTH;
    status_tampered_characteristic->maximum_data_length   = HOMEKIT_STATUS_TAMPERED_MAXIMUM_DATA_LENGTH;
    status_tampered_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_status_low_battery_characteristic( wiced_homekit_characteristics_t* status_low_battery_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &status_low_battery_characteristic->value.current, value, value_length, value_name );

    status_low_battery_characteristic->description           = description;
    status_low_battery_characteristic->instance_id           = instance_id;
    status_low_battery_characteristic->type                  = HOMEKIT_STATUS_LOW_BATTERY_UUID;
    status_low_battery_characteristic->permissions           = HOMEKIT_STATUS_LOW_BATTERY_PERMISSIONS;
    status_low_battery_characteristic->format                = HOMEKIT_STATUS_LOW_BATTERY_FORMAT;
    status_low_battery_characteristic->unit                  = HOMEKIT_STATUS_LOW_BATTERY_UNIT;
    status_low_battery_characteristic->minimum_value         = HOMEKIT_STATUS_LOW_BATTERY_MINIMUM_VALUE;
    status_low_battery_characteristic->maximum_value         = HOMEKIT_STATUS_LOW_BATTERY_MAXIMUM_VALUE;
    status_low_battery_characteristic->minimum_step          = HOMEKIT_STATUS_LOW_BATTERY_STEP_VALUE;
    status_low_battery_characteristic->maximum_length        = HOMEKIT_STATUS_LOW_BATTERY_MAXIMUM_LENGTH;
    status_low_battery_characteristic->maximum_data_length   = HOMEKIT_STATUS_LOW_BATTERY_MAXIMUM_DATA_LENGTH;
    status_low_battery_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_status_jammed_characteristic( wiced_homekit_characteristics_t* status_jammed_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &status_jammed_characteristic->value.current, value, value_length, value_name );

    status_jammed_characteristic->description           = description;
    status_jammed_characteristic->instance_id           = instance_id;
    status_jammed_characteristic->type                  = HOMEKIT_STATUS_JAMMED_UUID;
    status_jammed_characteristic->permissions           = HOMEKIT_STATUS_JAMMED_PERMISSIONS;
    status_jammed_characteristic->format                = HOMEKIT_STATUS_JAMMED_FORMAT;
    status_jammed_characteristic->unit                  = HOMEKIT_STATUS_JAMMED_UNIT;
    status_jammed_characteristic->minimum_value         = HOMEKIT_STATUS_JAMMED_MINIMUM_VALUE;
    status_jammed_characteristic->maximum_value         = HOMEKIT_STATUS_JAMMED_MAXIMUM_VALUE;
    status_jammed_characteristic->minimum_step          = HOMEKIT_STATUS_JAMMED_STEP_VALUE;
    status_jammed_characteristic->maximum_length        = HOMEKIT_STATUS_JAMMED_MAXIMUM_LENGTH;
    status_jammed_characteristic->maximum_data_length   = HOMEKIT_STATUS_JAMMED_MAXIMUM_DATA_LENGTH;
    status_jammed_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_security_system_current_state_characteristic( wiced_homekit_characteristics_t* security_system_current_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &security_system_current_state_characteristic->value.current, value, value_length, value_name );

    security_system_current_state_characteristic->description           = description;
    security_system_current_state_characteristic->instance_id           = instance_id;
    security_system_current_state_characteristic->type                  = HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_UUID;
    security_system_current_state_characteristic->permissions           = HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_PERMISSIONS;
    security_system_current_state_characteristic->format                = HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_FORMAT;
    security_system_current_state_characteristic->unit                  = HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_UNIT;
    security_system_current_state_characteristic->minimum_value         = HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_MINIMUM_VALUE;
    security_system_current_state_characteristic->maximum_value         = HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_MAXIMUM_VALUE;
    security_system_current_state_characteristic->minimum_step          = HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_STEP_VALUE;
    security_system_current_state_characteristic->maximum_length        = HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_MAXIMUM_LENGTH;
    security_system_current_state_characteristic->maximum_data_length   = HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_MAXIMUM_DATA_LENGTH;
    security_system_current_state_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_security_system_target_state_characteristic( wiced_homekit_characteristics_t* security_system_target_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &security_system_target_state_characteristic->value.current, value, value_length, value_name );

    security_system_target_state_characteristic->description           = description;
    security_system_target_state_characteristic->instance_id           = instance_id;
    security_system_target_state_characteristic->type                  = HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_UUID;
    security_system_target_state_characteristic->permissions           = HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_PERMISSIONS;
    security_system_target_state_characteristic->format                = HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_FORMAT;
    security_system_target_state_characteristic->unit                  = HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_UNIT;
    security_system_target_state_characteristic->minimum_value         = HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_MINIMUM_VALUE;
    security_system_target_state_characteristic->maximum_value         = HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_MAXIMUM_VALUE;
    security_system_target_state_characteristic->minimum_step          = HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_STEP_VALUE;
    security_system_target_state_characteristic->maximum_length        = HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_MAXIMUM_LENGTH;
    security_system_target_state_characteristic->maximum_data_length   = HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_MAXIMUM_DATA_LENGTH;
    security_system_target_state_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_security_system_alarm_type_characteristic( wiced_homekit_characteristics_t* security_system_alarm_type_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &security_system_alarm_type_characteristic->value.current, value, value_length, value_name );

    security_system_alarm_type_characteristic->description           = description;
    security_system_alarm_type_characteristic->instance_id           = instance_id;
    security_system_alarm_type_characteristic->type                  = HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_UUID;
    security_system_alarm_type_characteristic->permissions           = HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_PERMISSIONS;
    security_system_alarm_type_characteristic->format                = HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_FORMAT;
    security_system_alarm_type_characteristic->unit                  = HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_UNIT;
    security_system_alarm_type_characteristic->minimum_value         = HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_MINIMUM_VALUE;
    security_system_alarm_type_characteristic->maximum_value         = HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_MAXIMUM_VALUE;
    security_system_alarm_type_characteristic->minimum_step          = HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_STEP_VALUE;
    security_system_alarm_type_characteristic->maximum_length        = HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_MAXIMUM_LENGTH;
    security_system_alarm_type_characteristic->maximum_data_length   = HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_MAXIMUM_DATA_LENGTH;
    security_system_alarm_type_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_carbon_monoxide_detected_characteristic( wiced_homekit_characteristics_t* carbon_monoxide_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &carbon_monoxide_detected_characteristic->value.current, value, value_length, value_name );

    carbon_monoxide_detected_characteristic->description           = description;
    carbon_monoxide_detected_characteristic->instance_id           = instance_id;
    carbon_monoxide_detected_characteristic->type                  = HOMEKIT_CARBON_MONOXIDE_DETECTED_UUID;
    carbon_monoxide_detected_characteristic->permissions           = HOMEKIT_CARBON_MONOXIDE_DETECTED_PERMISSIONS;
    carbon_monoxide_detected_characteristic->format                = HOMEKIT_CARBON_MONOXIDE_DETECTED_FORMAT;
    carbon_monoxide_detected_characteristic->unit                  = HOMEKIT_CARBON_MONOXIDE_DETECTED_UNIT;
    carbon_monoxide_detected_characteristic->minimum_value         = HOMEKIT_CARBON_MONOXIDE_DETECTED_MINIMUM_VALUE;
    carbon_monoxide_detected_characteristic->maximum_value         = HOMEKIT_CARBON_MONOXIDE_DETECTED_MAXIMUM_VALUE;
    carbon_monoxide_detected_characteristic->minimum_step          = HOMEKIT_CARBON_MONOXIDE_DETECTED_STEP_VALUE;
    carbon_monoxide_detected_characteristic->maximum_length        = HOMEKIT_CARBON_MONOXIDE_DETECTED_MAXIMUM_LENGTH;
    carbon_monoxide_detected_characteristic->maximum_data_length   = HOMEKIT_CARBON_MONOXIDE_DETECTED_MAXIMUM_DATA_LENGTH;
    carbon_monoxide_detected_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_carbon_monoxide_level_characteristic( wiced_homekit_characteristics_t* carbon_monoxide_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &carbon_monoxide_level_characteristic->value.current, value, value_length, value_name );

    carbon_monoxide_level_characteristic->description           = description;
    carbon_monoxide_level_characteristic->instance_id           = instance_id;
    carbon_monoxide_level_characteristic->type                  = HOMEKIT_CARBON_MONOXIDE_LEVEL_UUID;
    carbon_monoxide_level_characteristic->permissions           = HOMEKIT_CARBON_MONOXIDE_LEVEL_PERMISSIONS;
    carbon_monoxide_level_characteristic->format                = HOMEKIT_CARBON_MONOXIDE_LEVEL_FORMAT;
    carbon_monoxide_level_characteristic->unit                  = HOMEKIT_CARBON_MONOXIDE_LEVEL_UNIT;
    carbon_monoxide_level_characteristic->minimum_value         = HOMEKIT_CARBON_MONOXIDE_LEVEL_MINIMUM_VALUE;
    carbon_monoxide_level_characteristic->maximum_value         = HOMEKIT_CARBON_MONOXIDE_LEVEL_MAXIMUM_VALUE;
    carbon_monoxide_level_characteristic->minimum_step          = HOMEKIT_CARBON_MONOXIDE_LEVEL_STEP_VALUE;
    carbon_monoxide_level_characteristic->maximum_length        = HOMEKIT_CARBON_MONOXIDE_LEVEL_MAXIMUM_LENGTH;
    carbon_monoxide_level_characteristic->maximum_data_length   = HOMEKIT_CARBON_MONOXIDE_LEVEL_MAXIMUM_DATA_LENGTH;
    carbon_monoxide_level_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_carbon_monoxide_peak_level_characteristic( wiced_homekit_characteristics_t* carbon_monoxide_peak_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &carbon_monoxide_peak_level_characteristic->value.current, value, value_length, value_name );

    carbon_monoxide_peak_level_characteristic->description           = description;
    carbon_monoxide_peak_level_characteristic->instance_id           = instance_id;
    carbon_monoxide_peak_level_characteristic->type                  = HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_UUID;
    carbon_monoxide_peak_level_characteristic->permissions           = HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_PERMISSIONS;
    carbon_monoxide_peak_level_characteristic->format                = HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_FORMAT;
    carbon_monoxide_peak_level_characteristic->unit                  = HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_UNIT;
    carbon_monoxide_peak_level_characteristic->minimum_value         = HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_MINIMUM_VALUE;
    carbon_monoxide_peak_level_characteristic->maximum_value         = HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_MAXIMUM_VALUE;
    carbon_monoxide_peak_level_characteristic->minimum_step          = HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_STEP_VALUE;
    carbon_monoxide_peak_level_characteristic->maximum_length        = HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_MAXIMUM_LENGTH;
    carbon_monoxide_peak_level_characteristic->maximum_data_length   = HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_MAXIMUM_DATA_LENGTH;
    carbon_monoxide_peak_level_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_contact_sensor_state_characteristic( wiced_homekit_characteristics_t* contact_sensor_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &contact_sensor_state_characteristic->value.current, value, value_length, value_name );

    contact_sensor_state_characteristic->description           = description;
    contact_sensor_state_characteristic->instance_id           = instance_id;
    contact_sensor_state_characteristic->type                  = HOMEKIT_CONTACT_SENSOR_STATE_UUID;
    contact_sensor_state_characteristic->permissions           = HOMEKIT_CONTACT_SENSOR_STATE_PERMISSIONS;
    contact_sensor_state_characteristic->format                = HOMEKIT_CONTACT_SENSOR_STATE_FORMAT;
    contact_sensor_state_characteristic->unit                  = HOMEKIT_CONTACT_SENSOR_STATE_UNIT;
    contact_sensor_state_characteristic->minimum_value         = HOMEKIT_CONTACT_SENSOR_STATE_MINIMUM_VALUE;
    contact_sensor_state_characteristic->maximum_value         = HOMEKIT_CONTACT_SENSOR_STATE_MAXIMUM_VALUE;
    contact_sensor_state_characteristic->minimum_step          = HOMEKIT_CONTACT_SENSOR_STATE_STEP_VALUE;
    contact_sensor_state_characteristic->maximum_length        = HOMEKIT_CONTACT_SENSOR_STATE_MAXIMUM_LENGTH;
    contact_sensor_state_characteristic->maximum_data_length   = HOMEKIT_CONTACT_SENSOR_STATE_MAXIMUM_DATA_LENGTH;
    contact_sensor_state_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_current_position_characteristic( wiced_homekit_characteristics_t* current_position_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &current_position_characteristic->value.current, value, value_length, value_name );

    current_position_characteristic->description           = description;
    current_position_characteristic->instance_id           = instance_id;
    current_position_characteristic->type                  = HOMEKIT_CURRENT_POSITION_UUID;
    current_position_characteristic->permissions           = HOMEKIT_CURRENT_POSITION_PERMISSIONS;
    current_position_characteristic->format                = HOMEKIT_CURRENT_POSITION_FORMAT;
    current_position_characteristic->unit                  = HOMEKIT_CURRENT_POSITION_UNIT;
    current_position_characteristic->minimum_value         = HOMEKIT_CURRENT_POSITION_MINIMUM_VALUE;
    current_position_characteristic->maximum_value         = HOMEKIT_CURRENT_POSITION_MAXIMUM_VALUE;
    current_position_characteristic->minimum_step          = HOMEKIT_CURRENT_POSITION_STEP_VALUE;
    current_position_characteristic->maximum_length        = HOMEKIT_CURRENT_POSITION_MAXIMUM_LENGTH;
    current_position_characteristic->maximum_data_length   = HOMEKIT_CURRENT_POSITION_MAXIMUM_DATA_LENGTH;
    current_position_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_target_position_characteristic( wiced_homekit_characteristics_t* target_position_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &target_position_characteristic->value.current, value, value_length, value_name );

    target_position_characteristic->description           = description;
    target_position_characteristic->instance_id           = instance_id;
    target_position_characteristic->type                  = HOMEKIT_TARGET_POSITION_UUID;
    target_position_characteristic->permissions           = HOMEKIT_TARGET_POSITION_PERMISSIONS;
    target_position_characteristic->format                = HOMEKIT_TARGET_POSITION_FORMAT;
    target_position_characteristic->unit                  = HOMEKIT_TARGET_POSITION_UNIT;
    target_position_characteristic->minimum_value         = HOMEKIT_TARGET_POSITION_MINIMUM_VALUE;
    target_position_characteristic->maximum_value         = HOMEKIT_TARGET_POSITION_MAXIMUM_VALUE;
    target_position_characteristic->minimum_step          = HOMEKIT_TARGET_POSITION_STEP_VALUE;
    target_position_characteristic->maximum_length        = HOMEKIT_TARGET_POSITION_MAXIMUM_LENGTH;
    target_position_characteristic->maximum_data_length   = HOMEKIT_TARGET_POSITION_MAXIMUM_DATA_LENGTH;
    target_position_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_position_state_characteristic( wiced_homekit_characteristics_t* position_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &position_state_characteristic->value.current, value, value_length, value_name );

    position_state_characteristic->description           = description;
    position_state_characteristic->instance_id           = instance_id;
    position_state_characteristic->type                  = HOMEKIT_POSITION_STATE_UUID;
    position_state_characteristic->permissions           = HOMEKIT_POSITION_STATE_PERMISSIONS;
    position_state_characteristic->format                = HOMEKIT_POSITION_STATE_FORMAT;
    position_state_characteristic->unit                  = HOMEKIT_POSITION_STATE_UNIT;
    position_state_characteristic->minimum_value         = HOMEKIT_POSITION_STATE_MINIMUM_VALUE;
    position_state_characteristic->maximum_value         = HOMEKIT_POSITION_STATE_MAXIMUM_VALUE;
    position_state_characteristic->minimum_step          = HOMEKIT_POSITION_STATE_STEP_VALUE;
    position_state_characteristic->maximum_length        = HOMEKIT_POSITION_STATE_MAXIMUM_LENGTH;
    position_state_characteristic->maximum_data_length   = HOMEKIT_POSITION_STATE_MAXIMUM_DATA_LENGTH;
    position_state_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_current_relative_humidity_characteristic( wiced_homekit_characteristics_t* current_relative_humidity_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &current_relative_humidity_characteristic->value.current, value, value_length, value_name );

    current_relative_humidity_characteristic->description           = description;
    current_relative_humidity_characteristic->instance_id           = instance_id;
    current_relative_humidity_characteristic->type                  = HOMEKIT_CURRENT_RELATIVE_HUMIDITY_UUID;
    current_relative_humidity_characteristic->permissions           = HOMEKIT_CURRENT_RELATIVE_HUMIDITY_PERMISSIONS;
    current_relative_humidity_characteristic->format                = HOMEKIT_CURRENT_RELATIVE_HUMIDITY_FORMAT;
    current_relative_humidity_characteristic->unit                  = HOMEKIT_CURRENT_RELATIVE_HUMIDITY_UNIT;
    current_relative_humidity_characteristic->minimum_value         = HOMEKIT_CURRENT_RELATIVE_HUMIDITY_MINIMUM_VALUE;
    current_relative_humidity_characteristic->maximum_value         = HOMEKIT_CURRENT_RELATIVE_HUMIDITY_MAXIMUM_VALUE;
    current_relative_humidity_characteristic->minimum_step          = HOMEKIT_CURRENT_RELATIVE_HUMIDITY_STEP_VALUE;
    current_relative_humidity_characteristic->maximum_length        = HOMEKIT_CURRENT_RELATIVE_HUMIDITY_MAXIMUM_LENGTH;
    current_relative_humidity_characteristic->maximum_data_length   = HOMEKIT_CURRENT_RELATIVE_HUMIDITY_MAXIMUM_DATA_LENGTH;
    current_relative_humidity_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_leak_detected_characteristic( wiced_homekit_characteristics_t* leak_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &leak_detected_characteristic->value.current, value, value_length, value_name );

    leak_detected_characteristic->description           = description;
    leak_detected_characteristic->instance_id           = instance_id;
    leak_detected_characteristic->type                  = HOMEKIT_LEAK_DETECTED_UUID;
    leak_detected_characteristic->permissions           = HOMEKIT_LEAK_DETECTED_PERMISSIONS;
    leak_detected_characteristic->format                = HOMEKIT_LEAK_DETECTED_FORMAT;
    leak_detected_characteristic->unit                  = HOMEKIT_LEAK_DETECTED_UNIT;
    leak_detected_characteristic->minimum_value         = HOMEKIT_LEAK_DETECTED_MINIMUM_VALUE;
    leak_detected_characteristic->maximum_value         = HOMEKIT_LEAK_DETECTED_MAXIMUM_VALUE;
    leak_detected_characteristic->minimum_step          = HOMEKIT_LEAK_DETECTED_STEP_VALUE;
    leak_detected_characteristic->maximum_length        = HOMEKIT_LEAK_DETECTED_MAXIMUM_LENGTH;
    leak_detected_characteristic->maximum_data_length   = HOMEKIT_LEAK_DETECTED_MAXIMUM_DATA_LENGTH;
    leak_detected_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_current_ambient_light_level_characteristic( wiced_homekit_characteristics_t* current_ambient_light_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &current_ambient_light_level_characteristic->value.current, value, value_length, value_name );

    current_ambient_light_level_characteristic->description           = description;
    current_ambient_light_level_characteristic->instance_id           = instance_id;
    current_ambient_light_level_characteristic->type                  = HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_UUID;
    current_ambient_light_level_characteristic->permissions           = HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_PERMISSIONS;
    current_ambient_light_level_characteristic->format                = HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_FORMAT;
    current_ambient_light_level_characteristic->unit                  = HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_UNIT;
    current_ambient_light_level_characteristic->minimum_value         = HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_MINIMUM_VALUE;
    current_ambient_light_level_characteristic->maximum_value         = HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_MAXIMUM_VALUE;
    current_ambient_light_level_characteristic->minimum_step          = HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_STEP_VALUE;
    current_ambient_light_level_characteristic->maximum_length        = HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_MAXIMUM_LENGTH;
    current_ambient_light_level_characteristic->maximum_data_length   = HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_MAXIMUM_DATA_LENGTH;
    current_ambient_light_level_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_occupancy_detected_characteristic( wiced_homekit_characteristics_t* occupancy_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &occupancy_detected_characteristic->value.current, value, value_length, value_name );

    occupancy_detected_characteristic->description           = description;
    occupancy_detected_characteristic->instance_id           = instance_id;
    occupancy_detected_characteristic->type                  = HOMEKIT_OCCUPANCY_DETECTED_UUID;
    occupancy_detected_characteristic->permissions           = HOMEKIT_OCCUPANCY_DETECTED_PERMISSIONS;
    occupancy_detected_characteristic->format                = HOMEKIT_OCCUPANCY_DETECTED_FORMAT;
    occupancy_detected_characteristic->unit                  = HOMEKIT_OCCUPANCY_DETECTED_UNIT;
    occupancy_detected_characteristic->minimum_value         = HOMEKIT_OCCUPANCY_DETECTED_MINIMUM_VALUE;
    occupancy_detected_characteristic->maximum_value         = HOMEKIT_OCCUPANCY_DETECTED_MAXIMUM_VALUE;
    occupancy_detected_characteristic->minimum_step          = HOMEKIT_OCCUPANCY_DETECTED_STEP_VALUE;
    occupancy_detected_characteristic->maximum_length        = HOMEKIT_OCCUPANCY_DETECTED_MAXIMUM_LENGTH;
    occupancy_detected_characteristic->maximum_data_length   = HOMEKIT_OCCUPANCY_DETECTED_MAXIMUM_DATA_LENGTH;
    occupancy_detected_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_smoke_detected_characteristic( wiced_homekit_characteristics_t* smoke_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &smoke_detected_characteristic->value.current, value, value_length, value_name );

    smoke_detected_characteristic->description           = description;
    smoke_detected_characteristic->instance_id           = instance_id;
    smoke_detected_characteristic->type                  = HOMEKIT_SMOKE_DETECTED_UUID;
    smoke_detected_characteristic->permissions           = HOMEKIT_SMOKE_DETECTED_PERMISSIONS;
    smoke_detected_characteristic->format                = HOMEKIT_SMOKE_DETECTED_FORMAT;
    smoke_detected_characteristic->unit                  = HOMEKIT_SMOKE_DETECTED_UNIT;
    smoke_detected_characteristic->minimum_value         = HOMEKIT_SMOKE_DETECTED_MINIMUM_VALUE;
    smoke_detected_characteristic->maximum_value         = HOMEKIT_SMOKE_DETECTED_MAXIMUM_VALUE;
    smoke_detected_characteristic->minimum_step          = HOMEKIT_SMOKE_DETECTED_STEP_VALUE;
    smoke_detected_characteristic->maximum_length        = HOMEKIT_SMOKE_DETECTED_MAXIMUM_LENGTH;
    smoke_detected_characteristic->maximum_data_length   = HOMEKIT_SMOKE_DETECTED_MAXIMUM_DATA_LENGTH;
    smoke_detected_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_programmable_switch_event_characteristic( wiced_homekit_characteristics_t* programmable_switch_event_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &programmable_switch_event_characteristic->value.current, value, value_length, value_name );

    programmable_switch_event_characteristic->description           = description;
    programmable_switch_event_characteristic->instance_id           = instance_id;
    programmable_switch_event_characteristic->type                  = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_UUID;
    programmable_switch_event_characteristic->permissions           = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_PERMISSIONS;
    programmable_switch_event_characteristic->format                = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_FORMAT;
    programmable_switch_event_characteristic->unit                  = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_UNIT;
    programmable_switch_event_characteristic->minimum_value         = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_MINIMUM_VALUE;
    programmable_switch_event_characteristic->maximum_value         = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_MAXIMUM_VALUE;
    programmable_switch_event_characteristic->minimum_step          = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_STEP_VALUE;
    programmable_switch_event_characteristic->maximum_length        = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_MAXIMUM_LENGTH;
    programmable_switch_event_characteristic->maximum_data_length   = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_MAXIMUM_DATA_LENGTH;
    programmable_switch_event_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_programmable_switch_output_state_characteristic( wiced_homekit_characteristics_t* programmable_switch_output_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &programmable_switch_output_state_characteristic->value.current, value, value_length, value_name );

    programmable_switch_output_state_characteristic->description           = description;
    programmable_switch_output_state_characteristic->instance_id           = instance_id;
    programmable_switch_output_state_characteristic->type                  = HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_UUID;
    programmable_switch_output_state_characteristic->permissions           = HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_PERMISSIONS;
    programmable_switch_output_state_characteristic->format                = HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_FORMAT;
    programmable_switch_output_state_characteristic->unit                  = HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_UNIT;
    programmable_switch_output_state_characteristic->minimum_value         = HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_MINIMUM_VALUE;
    programmable_switch_output_state_characteristic->maximum_value         = HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_MAXIMUM_VALUE;
    programmable_switch_output_state_characteristic->minimum_step          = HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_STEP_VALUE;
    programmable_switch_output_state_characteristic->maximum_length        = HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_MAXIMUM_LENGTH;
    programmable_switch_output_state_characteristic->maximum_data_length   = HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_MAXIMUM_DATA_LENGTH;
    programmable_switch_output_state_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_battery_level_characteristic( wiced_homekit_characteristics_t* battery_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &battery_level_characteristic->value.current, value, value_length, value_name );

    battery_level_characteristic->description           = description;
    battery_level_characteristic->instance_id           = instance_id;
    battery_level_characteristic->type                  = HOMEKIT_BATTERY_LEVEL_UUID;
    battery_level_characteristic->permissions           = HOMEKIT_BATTERY_LEVEL_PERMISSIONS;
    battery_level_characteristic->format                = HOMEKIT_BATTERY_LEVEL_FORMAT;
    battery_level_characteristic->unit                  = HOMEKIT_BATTERY_LEVEL_UNIT;
    battery_level_characteristic->minimum_value         = HOMEKIT_BATTERY_LEVEL_MINIMUM_VALUE;
    battery_level_characteristic->maximum_value         = HOMEKIT_BATTERY_LEVEL_MAXIMUM_VALUE;
    battery_level_characteristic->minimum_step          = HOMEKIT_BATTERY_LEVEL_STEP_VALUE;
    battery_level_characteristic->maximum_length        = HOMEKIT_BATTERY_LEVEL_MAXIMUM_LENGTH;
    battery_level_characteristic->maximum_data_length   = HOMEKIT_BATTERY_LEVEL_MAXIMUM_DATA_LENGTH;
    battery_level_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_charging_state_characteristic( wiced_homekit_characteristics_t* charging_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &charging_state_characteristic->value.current, value, value_length, value_name );

    charging_state_characteristic->description           = description;
    charging_state_characteristic->instance_id           = instance_id;
    charging_state_characteristic->type                  = HOMEKIT_CHARGING_STATE_UUID;
    charging_state_characteristic->permissions           = HOMEKIT_CHARGING_STATE_PERMISSIONS;
    charging_state_characteristic->format                = HOMEKIT_CHARGING_STATE_FORMAT;
    charging_state_characteristic->unit                  = HOMEKIT_CHARGING_STATE_UNIT;
    charging_state_characteristic->minimum_value         = HOMEKIT_CHARGING_STATE_MINIMUM_VALUE;
    charging_state_characteristic->maximum_value         = HOMEKIT_CHARGING_STATE_MAXIMUM_VALUE;
    charging_state_characteristic->minimum_step          = HOMEKIT_CHARGING_STATE_STEP_VALUE;
    charging_state_characteristic->maximum_length        = HOMEKIT_CHARGING_STATE_MAXIMUM_LENGTH;
    charging_state_characteristic->maximum_data_length   = HOMEKIT_CHARGING_STATE_MAXIMUM_DATA_LENGTH;
    charging_state_characteristic->identify_callback     = NULL;
}



void wiced_homekit_initialise_carbon_dioxide_detected_characteristic( wiced_homekit_characteristics_t* carbon_dioxide_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &carbon_dioxide_detected_characteristic->value.current, value, value_length, value_name );

    carbon_dioxide_detected_characteristic->description           = description;
    carbon_dioxide_detected_characteristic->instance_id           = instance_id;
    carbon_dioxide_detected_characteristic->type                  = HOMEKIT_CARBON_DIOXIDE_DETECTED_UUID;
    carbon_dioxide_detected_characteristic->permissions           = HOMEKIT_CARBON_DIOXIDE_DETECTED_PERMISSIONS;
    carbon_dioxide_detected_characteristic->format                = HOMEKIT_CARBON_DIOXIDE_DETECTED_FORMAT;
    carbon_dioxide_detected_characteristic->unit                  = HOMEKIT_CARBON_DIOXIDE_DETECTED_UNIT;
    carbon_dioxide_detected_characteristic->minimum_value         = HOMEKIT_CARBON_DIOXIDE_DETECTED_MINIMUM_VALUE;
    carbon_dioxide_detected_characteristic->maximum_value         = HOMEKIT_CARBON_DIOXIDE_DETECTED_MAXIMUM_VALUE;
    carbon_dioxide_detected_characteristic->minimum_step          = HOMEKIT_CARBON_DIOXIDE_DETECTED_STEP_VALUE;
    carbon_dioxide_detected_characteristic->maximum_length        = HOMEKIT_CARBON_DIOXIDE_DETECTED_MAXIMUM_LENGTH;
    carbon_dioxide_detected_characteristic->maximum_data_length   = HOMEKIT_CARBON_DIOXIDE_DETECTED_MAXIMUM_DATA_LENGTH;
    carbon_dioxide_detected_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_carbon_dioxide_level_characteristic( wiced_homekit_characteristics_t* carbon_dioxide_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &carbon_dioxide_level_characteristic->value.current, value, value_length, value_name );

    carbon_dioxide_level_characteristic->description           = description;
    carbon_dioxide_level_characteristic->instance_id           = instance_id;
    carbon_dioxide_level_characteristic->type                  = HOMEKIT_CARBON_DIOXIDE_LEVEL_UUID;
    carbon_dioxide_level_characteristic->permissions           = HOMEKIT_CARBON_DIOXIDE_LEVEL_PERMISSIONS;
    carbon_dioxide_level_characteristic->format                = HOMEKIT_CARBON_DIOXIDE_LEVEL_FORMAT;
    carbon_dioxide_level_characteristic->unit                  = HOMEKIT_CARBON_DIOXIDE_LEVEL_UNIT;
    carbon_dioxide_level_characteristic->minimum_value         = HOMEKIT_CARBON_DIOXIDE_LEVEL_MINIMUM_VALUE;
    carbon_dioxide_level_characteristic->maximum_value         = HOMEKIT_CARBON_DIOXIDE_LEVEL_MAXIMUM_VALUE;
    carbon_dioxide_level_characteristic->minimum_step          = HOMEKIT_CARBON_DIOXIDE_LEVEL_STEP_VALUE;
    carbon_dioxide_level_characteristic->maximum_length        = HOMEKIT_CARBON_DIOXIDE_LEVEL_MAXIMUM_LENGTH;
    carbon_dioxide_level_characteristic->maximum_data_length   = HOMEKIT_CARBON_DIOXIDE_LEVEL_MAXIMUM_DATA_LENGTH;
    carbon_dioxide_level_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_carbon_dioxide_peak_level_characteristic( wiced_homekit_characteristics_t* carbon_dioxide_peak_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &carbon_dioxide_peak_level_characteristic->value.current, value, value_length, value_name );

    carbon_dioxide_peak_level_characteristic->description           = description;
    carbon_dioxide_peak_level_characteristic->instance_id           = instance_id;
    carbon_dioxide_peak_level_characteristic->type                  = HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_UUID;
    carbon_dioxide_peak_level_characteristic->permissions           = HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_PERMISSIONS;
    carbon_dioxide_peak_level_characteristic->format                = HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_FORMAT;
    carbon_dioxide_peak_level_characteristic->unit                  = HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_UNIT;
    carbon_dioxide_peak_level_characteristic->minimum_value         = HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_MINIMUM_VALUE;
    carbon_dioxide_peak_level_characteristic->maximum_value         = HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_MAXIMUM_VALUE;
    carbon_dioxide_peak_level_characteristic->minimum_step          = HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_STEP_VALUE;
    carbon_dioxide_peak_level_characteristic->maximum_length        = HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_MAXIMUM_LENGTH;
    carbon_dioxide_peak_level_characteristic->maximum_data_length   = HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_MAXIMUM_DATA_LENGTH;
    carbon_dioxide_peak_level_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_administrator_only_access_characteristic( wiced_homekit_characteristics_t* administrator_only_access_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &administrator_only_access_characteristic->value.current, value, value_length, value_name );

    administrator_only_access_characteristic->description           = description;
    administrator_only_access_characteristic->instance_id           = instance_id;
    administrator_only_access_characteristic->type                  = HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_UUID;
    administrator_only_access_characteristic->permissions           = HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_PERMISSIONS;
    administrator_only_access_characteristic->format                = HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_FORMAT;
    administrator_only_access_characteristic->unit                  = HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_UNIT;
    administrator_only_access_characteristic->minimum_value         = HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_MINIMUM_VALUE;
    administrator_only_access_characteristic->maximum_value         = HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_MAXIMUM_VALUE;
    administrator_only_access_characteristic->minimum_step          = HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_STEP_VALUE;
    administrator_only_access_characteristic->maximum_length        = HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_MAXIMUM_LENGTH;
    administrator_only_access_characteristic->maximum_data_length   = HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_MAXIMUM_DATA_LENGTH;
    administrator_only_access_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_audio_feedback_characteristic( wiced_homekit_characteristics_t* audio_feedback_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &audio_feedback_characteristic->value.current, value, value_length, value_name );

    audio_feedback_characteristic->description           = description;
    audio_feedback_characteristic->instance_id           = instance_id;
    audio_feedback_characteristic->type                  = HOMEKIT_AUDIO_FEEDBACK_UUID;
    audio_feedback_characteristic->permissions           = HOMEKIT_AUDIO_FEEDBACK_PERMISSIONS;
    audio_feedback_characteristic->format                = HOMEKIT_AUDIO_FEEDBACK_FORMAT;
    audio_feedback_characteristic->unit                  = HOMEKIT_AUDIO_FEEDBACK_UNIT;
    audio_feedback_characteristic->minimum_value         = HOMEKIT_AUDIO_FEEDBACK_MINIMUM_VALUE;
    audio_feedback_characteristic->maximum_value         = HOMEKIT_AUDIO_FEEDBACK_MAXIMUM_VALUE;
    audio_feedback_characteristic->minimum_step          = HOMEKIT_AUDIO_FEEDBACK_STEP_VALUE;
    audio_feedback_characteristic->maximum_length        = HOMEKIT_AUDIO_FEEDBACK_MAXIMUM_LENGTH;
    audio_feedback_characteristic->maximum_data_length   = HOMEKIT_AUDIO_FEEDBACK_MAXIMUM_DATA_LENGTH;
    audio_feedback_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_cooling_threshold_temperature_characteristic( wiced_homekit_characteristics_t* cooling_threshold_temperature_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &cooling_threshold_temperature_characteristic->value.current, value, value_length, value_name );

    cooling_threshold_temperature_characteristic->description           = description;
    cooling_threshold_temperature_characteristic->instance_id           = instance_id;
    cooling_threshold_temperature_characteristic->type                  = HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_UUID;
    cooling_threshold_temperature_characteristic->permissions           = HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_PERMISSIONS;
    cooling_threshold_temperature_characteristic->format                = HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_FORMAT;
    cooling_threshold_temperature_characteristic->unit                  = HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_UNIT;
    cooling_threshold_temperature_characteristic->minimum_value         = HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_MINIMUM_VALUE;
    cooling_threshold_temperature_characteristic->maximum_value         = HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_MAXIMUM_VALUE;
    cooling_threshold_temperature_characteristic->minimum_step          = HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_STEP_VALUE;
    cooling_threshold_temperature_characteristic->maximum_length        = HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_MAXIMUM_LENGTH;
    cooling_threshold_temperature_characteristic->maximum_data_length   = HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_MAXIMUM_DATA_LENGTH;
    cooling_threshold_temperature_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_heating_threshold_temperature_characteristic( wiced_homekit_characteristics_t* heating_threshold_temperature_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &heating_threshold_temperature_characteristic->value.current, value, value_length, value_name );

    heating_threshold_temperature_characteristic->description           = description;
    heating_threshold_temperature_characteristic->instance_id           = instance_id;
    heating_threshold_temperature_characteristic->type                  = HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_UUID;
    heating_threshold_temperature_characteristic->permissions           = HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_PERMISSIONS;
    heating_threshold_temperature_characteristic->format                = HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_FORMAT;
    heating_threshold_temperature_characteristic->unit                  = HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_UNIT;
    heating_threshold_temperature_characteristic->minimum_value         = HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_MINIMUM_VALUE;
    heating_threshold_temperature_characteristic->maximum_value         = HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_MAXIMUM_VALUE;
    heating_threshold_temperature_characteristic->minimum_step          = HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_STEP_VALUE;
    heating_threshold_temperature_characteristic->maximum_length        = HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_MAXIMUM_LENGTH;
    heating_threshold_temperature_characteristic->maximum_data_length   = HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_MAXIMUM_DATA_LENGTH;
    heating_threshold_temperature_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_lock_last_known_action_characteristic( wiced_homekit_characteristics_t* lock_last_known_action_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &lock_last_known_action_characteristic->value.current, value, value_length, value_name );

    lock_last_known_action_characteristic->description           = description;
    lock_last_known_action_characteristic->instance_id           = instance_id;
    lock_last_known_action_characteristic->type                  = HOMEKIT_LOCK_LAST_KNOWN_ACTION_UUID;
    lock_last_known_action_characteristic->permissions           = HOMEKIT_LOCK_LAST_KNOWN_ACTION_PERMISSIONS;
    lock_last_known_action_characteristic->format                = HOMEKIT_LOCK_LAST_KNOWN_ACTION_FORMAT;
    lock_last_known_action_characteristic->unit                  = HOMEKIT_LOCK_LAST_KNOWN_ACTION_UNIT;
    lock_last_known_action_characteristic->minimum_value         = HOMEKIT_LOCK_LAST_KNOWN_ACTION_MINIMUM_VALUE;
    lock_last_known_action_characteristic->maximum_value         = HOMEKIT_LOCK_LAST_KNOWN_ACTION_MAXIMUM_VALUE;
    lock_last_known_action_characteristic->minimum_step          = HOMEKIT_LOCK_LAST_KNOWN_ACTION_STEP_VALUE;
    lock_last_known_action_characteristic->maximum_length        = HOMEKIT_LOCK_LAST_KNOWN_ACTION_MAXIMUM_LENGTH;
    lock_last_known_action_characteristic->maximum_data_length   = HOMEKIT_LOCK_LAST_KNOWN_ACTION_MAXIMUM_DATA_LENGTH;
    lock_last_known_action_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_lock_auto_security_timeout_characteristic( wiced_homekit_characteristics_t* lock_auto_security_timeout_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &lock_auto_security_timeout_characteristic->value.current, value, value_length, value_name );

    lock_auto_security_timeout_characteristic->description           = description;
    lock_auto_security_timeout_characteristic->instance_id           = instance_id;
    lock_auto_security_timeout_characteristic->type                  = HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_UUID;
    lock_auto_security_timeout_characteristic->permissions           = HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_PERMISSIONS;
    lock_auto_security_timeout_characteristic->format                = HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_FORMAT;
    lock_auto_security_timeout_characteristic->unit                  = HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_UNIT;
    lock_auto_security_timeout_characteristic->minimum_value         = HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_MINIMUM_VALUE;
    lock_auto_security_timeout_characteristic->maximum_value         = HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_MAXIMUM_VALUE;
    lock_auto_security_timeout_characteristic->minimum_step          = HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_STEP_VALUE;
    lock_auto_security_timeout_characteristic->maximum_length        = HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_MAXIMUM_LENGTH;
    lock_auto_security_timeout_characteristic->maximum_data_length   = HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_MAXIMUM_DATA_LENGTH;
    lock_auto_security_timeout_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_log_characteristic( wiced_homekit_characteristics_t* log_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &log_characteristic->value.current, value, value_length, value_name );

    log_characteristic->description            = description;
    log_characteristic->instance_id            = instance_id;
    log_characteristic->type                   = HOMEKIT_LOG_UUID;
    log_characteristic->permissions            = HOMEKIT_LOG_PERMISSIONS;
    log_characteristic->format                 = HOMEKIT_LOG_FORMAT;
    log_characteristic->unit                   = HOMEKIT_LOG_UNIT;
    log_characteristic->minimum_value          = HOMEKIT_LOG_MINIMUM_VALUE;
    log_characteristic->maximum_value          = HOMEKIT_LOG_MAXIMUM_VALUE;
    log_characteristic->minimum_step           = HOMEKIT_LOG_STEP_VALUE;
    log_characteristic->maximum_length         = HOMEKIT_LOG_MAXIMUM_LENGTH;
    log_characteristic->maximum_data_length    = HOMEKIT_LOG_MAXIMUM_DATA_LENGTH;
    log_characteristic->identify_callback      = NULL;
}

void wiced_homekit_initialise_motion_detected_characteristic ( wiced_homekit_characteristics_t* motion_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &motion_detected_characteristic->value.current, value, value_length, value_name );

    motion_detected_characteristic->description           = description;
    motion_detected_characteristic->instance_id           = instance_id;
    motion_detected_characteristic->type                  = HOMEKIT_MOTION_DETECTED_UUID;
    motion_detected_characteristic->permissions           = HOMEKIT_MOTION_DETECTED_PERMISSIONS;
    motion_detected_characteristic->format                = HOMEKIT_MOTION_DETECTED_FORMAT;
    motion_detected_characteristic->unit                  = HOMEKIT_MOTION_DETECTED_UNIT;
    motion_detected_characteristic->minimum_value         = HOMEKIT_MOTION_DETECTED_MINIMUM_VALUE;
    motion_detected_characteristic->maximum_value         = HOMEKIT_MOTION_DETECTED_MAXIMUM_VALUE;
    motion_detected_characteristic->minimum_step          = HOMEKIT_MOTION_DETECTED_STEP_VALUE;
    motion_detected_characteristic->maximum_length        = HOMEKIT_MOTION_DETECTED_MAXIMUM_LENGTH;
    motion_detected_characteristic->maximum_data_length   = HOMEKIT_MOTION_DETECTED_MAXIMUM_DATA_LENGTH;
    motion_detected_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_rotation_direction_characteristic ( wiced_homekit_characteristics_t* rotation_direction_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &rotation_direction_characteristic->value.current, value, value_length, value_name );

    rotation_direction_characteristic->description           = description;
    rotation_direction_characteristic->instance_id           = instance_id;
    rotation_direction_characteristic->type                  = HOMEKIT_ROTATION_DIRECTION_UUID;
    rotation_direction_characteristic->permissions           = HOMEKIT_ROTATION_DIRECTION_PERMISSIONS;
    rotation_direction_characteristic->format                = HOMEKIT_ROTATION_DIRECTION_FORMAT;
    rotation_direction_characteristic->unit                  = HOMEKIT_ROTATION_DIRECTION_UNIT;
    rotation_direction_characteristic->minimum_value         = HOMEKIT_ROTATION_DIRECTION_MINIMUM_VALUE;
    rotation_direction_characteristic->maximum_value         = HOMEKIT_ROTATION_DIRECTION_MAXIMUM_VALUE;
    rotation_direction_characteristic->minimum_step          = HOMEKIT_ROTATION_DIRECTION_STEP_VALUE;
    rotation_direction_characteristic->maximum_length        = HOMEKIT_ROTATION_DIRECTION_MAXIMUM_LENGTH;
    rotation_direction_characteristic->maximum_data_length   = HOMEKIT_ROTATION_DIRECTION_MAXIMUM_DATA_LENGTH;
    rotation_direction_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_rotation_speed_characteristic ( wiced_homekit_characteristics_t* rotation_speed_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &rotation_speed_characteristic->value.current, value, value_length, value_name );

    rotation_speed_characteristic->description           = description;
    rotation_speed_characteristic->instance_id           = instance_id;
    rotation_speed_characteristic->type                  = HOMEKIT_ROTATION_SPEED_UUID;
    rotation_speed_characteristic->permissions           = HOMEKIT_ROTATION_SPEED_PERMISSIONS;
    rotation_speed_characteristic->format                = HOMEKIT_ROTATION_SPEED_FORMAT;
    rotation_speed_characteristic->unit                  = HOMEKIT_ROTATION_SPEED_UNIT;
    rotation_speed_characteristic->minimum_value         = HOMEKIT_ROTATION_SPEED_MINIMUM_VALUE;
    rotation_speed_characteristic->maximum_value         = HOMEKIT_ROTATION_SPEED_MAXIMUM_VALUE;
    rotation_speed_characteristic->minimum_step          = HOMEKIT_ROTATION_SPEED_STEP_VALUE;
    rotation_speed_characteristic->maximum_length        = HOMEKIT_ROTATION_SPEED_MAXIMUM_LENGTH;
    rotation_speed_characteristic->maximum_data_length   = HOMEKIT_ROTATION_SPEED_MAXIMUM_DATA_LENGTH;
    rotation_speed_characteristic->identify_callback     = NULL;

}

void wiced_homekit_initialise_target_relative_humidity_characteristic( wiced_homekit_characteristics_t* target_relative_humidity_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &target_relative_humidity_characteristic->value.current, value, value_length, value_name );

    target_relative_humidity_characteristic->description           = description;
    target_relative_humidity_characteristic->instance_id           = instance_id;
    target_relative_humidity_characteristic->type                  = HOMEKIT_TARGET_RELATIVE_HUMIDITY_UUID;
    target_relative_humidity_characteristic->permissions           = HOMEKIT_TARGET_RELATIVE_HUMIDITY_PERMISSIONS;
    target_relative_humidity_characteristic->format                = HOMEKIT_TARGET_RELATIVE_HUMIDITY_FORMAT;
    target_relative_humidity_characteristic->unit                  = HOMEKIT_TARGET_RELATIVE_HUMIDITY_UNIT;
    target_relative_humidity_characteristic->minimum_value         = HOMEKIT_TARGET_RELATIVE_HUMIDITY_MINIMUM_VALUE;
    target_relative_humidity_characteristic->maximum_value         = HOMEKIT_TARGET_RELATIVE_HUMIDITY_MAXIMUM_VALUE;
    target_relative_humidity_characteristic->minimum_step          = HOMEKIT_TARGET_RELATIVE_HUMIDITY_STEP_VALUE;
    target_relative_humidity_characteristic->maximum_length        = HOMEKIT_TARGET_RELATIVE_HUMIDITY_MAXIMUM_LENGTH;
    target_relative_humidity_characteristic->maximum_data_length   = HOMEKIT_TARGET_RELATIVE_HUMIDITY_MAXIMUM_DATA_LENGTH;
    target_relative_humidity_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_current_horizontal_angle_characteristic( wiced_homekit_characteristics_t* current_horizontal_angle_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &current_horizontal_angle_characteristic->value.current, value, value_length, value_name );

    current_horizontal_angle_characteristic->description           = description;
    current_horizontal_angle_characteristic->instance_id           = instance_id;
    current_horizontal_angle_characteristic->type                  = HOMEKIT_CURRENT_H_ANGLE_UUID;
    current_horizontal_angle_characteristic->permissions           = HOMEKIT_CURRENT_H_ANGLE_PERMISSIONS;
    current_horizontal_angle_characteristic->format                = HOMEKIT_CURRENT_H_ANGLE_FORMAT;
    current_horizontal_angle_characteristic->unit                  = HOMEKIT_CURRENT_H_ANGLE_UNIT;
    current_horizontal_angle_characteristic->minimum_value         = HOMEKIT_CURRENT_H_ANGLE_MINIMUM_VALUE;
    current_horizontal_angle_characteristic->maximum_value         = HOMEKIT_CURRENT_H_ANGLE_MAXIMUM_VALUE;
    current_horizontal_angle_characteristic->minimum_step          = HOMEKIT_CURRENT_H_ANGLE_STEP_VALUE;
    current_horizontal_angle_characteristic->maximum_length        = HOMEKIT_CURRENT_H_ANGLE_MAXIMUM_LENGTH;
    current_horizontal_angle_characteristic->maximum_data_length   = HOMEKIT_CURRENT_H_ANGLE_MAXIMUM_DATA_LENGTH;
    current_horizontal_angle_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_current_vertical_angle_characteristic( wiced_homekit_characteristics_t* current_vertical_angle_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &current_vertical_angle_characteristic->value.current, value, value_length, value_name );

    current_vertical_angle_characteristic->description           = description;
    current_vertical_angle_characteristic->instance_id           = instance_id;
    current_vertical_angle_characteristic->type                  = HOMEKIT_CURRENT_V_ANGLE_UUID;
    current_vertical_angle_characteristic->permissions           = HOMEKIT_CURRENT_V_ANGLE_PERMISSIONS;
    current_vertical_angle_characteristic->format                = HOMEKIT_CURRENT_V_ANGLE_FORMAT;
    current_vertical_angle_characteristic->unit                  = HOMEKIT_CURRENT_V_ANGLE_UNIT;
    current_vertical_angle_characteristic->minimum_value         = HOMEKIT_CURRENT_V_ANGLE_MINIMUM_VALUE;
    current_vertical_angle_characteristic->maximum_value         = HOMEKIT_CURRENT_V_ANGLE_MAXIMUM_VALUE;
    current_vertical_angle_characteristic->minimum_step          = HOMEKIT_CURRENT_V_ANGLE_STEP_VALUE;
    current_vertical_angle_characteristic->maximum_length        = HOMEKIT_CURRENT_V_ANGLE_MAXIMUM_LENGTH;
    current_vertical_angle_characteristic->maximum_data_length   = HOMEKIT_CURRENT_V_ANGLE_MAXIMUM_DATA_LENGTH;
    current_vertical_angle_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_target_horizontal_angle_characteristic( wiced_homekit_characteristics_t* target_horizontal_angle_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &target_horizontal_angle_characteristic->value.current, value, value_length, value_name );

    target_horizontal_angle_characteristic->description           = description;
    target_horizontal_angle_characteristic->instance_id           = instance_id;
    target_horizontal_angle_characteristic->type                  = HOMEKIT_TARGET_H_ANGLE_UUID;
    target_horizontal_angle_characteristic->permissions           = HOMEKIT_TARGET_H_ANGLE_PERMISSIONS;
    target_horizontal_angle_characteristic->format                = HOMEKIT_TARGET_H_ANGLE_FORMAT;
    target_horizontal_angle_characteristic->unit                  = HOMEKIT_TARGET_H_ANGLE_UNIT;
    target_horizontal_angle_characteristic->minimum_value         = HOMEKIT_TARGET_H_ANGLE_MINIMUM_VALUE;
    target_horizontal_angle_characteristic->maximum_value         = HOMEKIT_TARGET_H_ANGLE_MAXIMUM_VALUE;
    target_horizontal_angle_characteristic->minimum_step          = HOMEKIT_TARGET_H_ANGLE_STEP_VALUE;
    target_horizontal_angle_characteristic->maximum_length        = HOMEKIT_TARGET_H_ANGLE_MAXIMUM_LENGTH;
    target_horizontal_angle_characteristic->maximum_data_length   = HOMEKIT_TARGET_H_ANGLE_MAXIMUM_DATA_LENGTH;
    target_horizontal_angle_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_target_vertical_angle_characteristic( wiced_homekit_characteristics_t* target_vertical_angle_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &target_vertical_angle_characteristic->value.current, value, value_length, value_name );

    target_vertical_angle_characteristic->description           = description;
    target_vertical_angle_characteristic->instance_id           = instance_id;
    target_vertical_angle_characteristic->type                  = HOMEKIT_TARGET_V_ANGLE_UUID;
    target_vertical_angle_characteristic->permissions           = HOMEKIT_TARGET_V_ANGLE_PERMISSIONS;
    target_vertical_angle_characteristic->format                = HOMEKIT_TARGET_V_ANGLE_FORMAT;
    target_vertical_angle_characteristic->unit                  = HOMEKIT_TARGET_V_ANGLE_UNIT;
    target_vertical_angle_characteristic->minimum_value         = HOMEKIT_TARGET_V_ANGLE_MINIMUM_VALUE;
    target_vertical_angle_characteristic->maximum_value         = HOMEKIT_TARGET_V_ANGLE_MAXIMUM_VALUE;
    target_vertical_angle_characteristic->minimum_step          = HOMEKIT_TARGET_V_ANGLE_STEP_VALUE;
    target_vertical_angle_characteristic->maximum_length        = HOMEKIT_TARGET_V_ANGLE_MAXIMUM_LENGTH;
    target_vertical_angle_characteristic->maximum_data_length   = HOMEKIT_TARGET_V_ANGLE_MAXIMUM_DATA_LENGTH;
    target_vertical_angle_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_hold_position_characteristic( wiced_homekit_characteristics_t* hold_position_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &hold_position_characteristic->value.current, value, value_length, value_name );

    hold_position_characteristic->description           = description;
    hold_position_characteristic->instance_id           = instance_id;
    hold_position_characteristic->type                  = HOMEKIT_HOLD_POSITION_UUID;
    hold_position_characteristic->permissions           = HOMEKIT_HOLD_POSITION_PERMISSIONS;
    hold_position_characteristic->format                = HOMEKIT_HOLD_POSITION_FORMAT;
    hold_position_characteristic->unit                  = HOMEKIT_HOLD_POSITION_UNIT;
    hold_position_characteristic->minimum_value         = HOMEKIT_HOLD_POSITION_MINIMUM_VALUE;
    hold_position_characteristic->maximum_value         = HOMEKIT_HOLD_POSITION_MAXIMUM_VALUE;
    hold_position_characteristic->minimum_step          = HOMEKIT_HOLD_POSITION_STEP_VALUE;
    hold_position_characteristic->maximum_length        = HOMEKIT_HOLD_POSITION_MAXIMUM_LENGTH;
    hold_position_characteristic->maximum_data_length   = HOMEKIT_HOLD_POSITION_MAXIMUM_DATA_LENGTH;
    hold_position_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_firmware_characteristic( wiced_homekit_characteristics_t* firmware_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &firmware_characteristic->value.current, value, value_length, value_name );

    firmware_characteristic->description           = description;
    firmware_characteristic->instance_id           = instance_id;
    firmware_characteristic->type                  = HOMEKIT_FIRMWARE_UUID;
    firmware_characteristic->permissions           = HOMEKIT_FIRMWARE_PERMISSIONS;
    firmware_characteristic->format                = HOMEKIT_FIRMWARE_FORMAT;
    firmware_characteristic->unit                  = HOMEKIT_FIRMWARE_UNIT;
    firmware_characteristic->minimum_value         = HOMEKIT_FIRMWARE_MINIMUM_VALUE;
    firmware_characteristic->maximum_value         = HOMEKIT_FIRMWARE_MAXIMUM_VALUE;
    firmware_characteristic->minimum_step          = HOMEKIT_FIRMWARE_STEP_VALUE;
    firmware_characteristic->maximum_length        = HOMEKIT_FIRMWARE_MAXIMUM_LENGTH;
    firmware_characteristic->maximum_data_length   = HOMEKIT_FIRMWARE_MAXIMUM_DATA_LENGTH;
    firmware_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_hardware_characteristic( wiced_homekit_characteristics_t* hardware_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &hardware_characteristic->value.current, value, value_length, value_name );

    hardware_characteristic->description           = description;
    hardware_characteristic->instance_id           = instance_id;
    hardware_characteristic->type                  = HOMEKIT_HARDWARE_UUID;
    hardware_characteristic->permissions           = HOMEKIT_HARDWARE_PERMISSIONS;
    hardware_characteristic->format                = HOMEKIT_HARDWARE_FORMAT;
    hardware_characteristic->unit                  = HOMEKIT_HARDWARE_UNIT;
    hardware_characteristic->minimum_value         = HOMEKIT_HARDWARE_MINIMUM_VALUE;
    hardware_characteristic->maximum_value         = HOMEKIT_HARDWARE_MAXIMUM_VALUE;
    hardware_characteristic->minimum_step          = HOMEKIT_HARDWARE_STEP_VALUE;
    hardware_characteristic->maximum_length        = HOMEKIT_HARDWARE_MAXIMUM_LENGTH;
    hardware_characteristic->maximum_data_length   = HOMEKIT_HARDWARE_MAXIMUM_DATA_LENGTH;
    hardware_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_software_characteristic( wiced_homekit_characteristics_t* software_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &software_characteristic->value.current, value, value_length, value_name );

    software_characteristic->description           = description;
    software_characteristic->instance_id           = instance_id;
    software_characteristic->type                  = HOMEKIT_SOFTWARE_UUID;
    software_characteristic->permissions           = HOMEKIT_SOFTWARE_PERMISSIONS;
    software_characteristic->format                = HOMEKIT_SOFTWARE_FORMAT;
    software_characteristic->unit                  = HOMEKIT_SOFTWARE_UNIT;
    software_characteristic->minimum_value         = HOMEKIT_SOFTWARE_MINIMUM_VALUE;
    software_characteristic->maximum_value         = HOMEKIT_SOFTWARE_MAXIMUM_VALUE;
    software_characteristic->minimum_step          = HOMEKIT_SOFTWARE_STEP_VALUE;
    software_characteristic->maximum_length        = HOMEKIT_SOFTWARE_MAXIMUM_LENGTH;
    software_characteristic->maximum_data_length   = HOMEKIT_SOFTWARE_MAXIMUM_DATA_LENGTH;
    software_characteristic->identify_callback     = NULL;
}

/* Initialise the custom characteristic */
/* Note: This is an example of custom characteristic */
void wiced_homekit_initialise_color_characteristic( wiced_homekit_characteristics_t* color_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &color_characteristic->value.current, value, value_length, value_name );

    color_characteristic->description           = description;
    color_characteristic->instance_id           = instance_id;
    color_characteristic->type                  = HOMEKIT_COLOR_UUID;
    color_characteristic->permissions           = HOMEKIT_COLOR_PERMISSIONS;
    color_characteristic->format                = HOMEKIT_COLOR_FORMAT;
    color_characteristic->unit                  = HOMEKIT_COLOR_UNIT;
    color_characteristic->minimum_value         = HOMEKIT_COLOR_MINIMUM_VALUE;
    color_characteristic->maximum_value         = HOMEKIT_COLOR_MAXIMUM_VALUE;
    color_characteristic->minimum_step          = HOMEKIT_COLOR_STEP_VALUE;
    color_characteristic->maximum_length        = HOMEKIT_COLOR_MAXIMUM_LENGTH;
    color_characteristic->maximum_data_length   = HOMEKIT_COLOR_MAXIMUM_DATA_LENGTH;
    color_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_power_characteristic( wiced_homekit_characteristics_t* power_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &power_characteristic->value.current, value, value_length, value_name );

    power_characteristic->description           = description;
    power_characteristic->instance_id           = instance_id;
    power_characteristic->type                  = HOMEKIT_POWER_UUID;
    power_characteristic->permissions           = HOMEKIT_POWER_PERMISSIONS;
    power_characteristic->format                = HOMEKIT_POWER_FORMAT;
    power_characteristic->unit                  = HOMEKIT_POWER_UNIT;
    power_characteristic->minimum_value         = HOMEKIT_POWER_MINIMUM_VALUE;
    power_characteristic->maximum_value         = HOMEKIT_POWER_MAXIMUM_VALUE;
    power_characteristic->minimum_step          = HOMEKIT_POWER_STEP_VALUE;
    power_characteristic->maximum_length        = HOMEKIT_POWER_MAXIMUM_LENGTH;
    power_characteristic->maximum_data_length   = HOMEKIT_POWER_MAXIMUM_DATA_LENGTH;
    power_characteristic->identify_callback     = NULL;
}

void wiced_homekit_initialise_system_upgrade_characteristic( wiced_homekit_characteristics_t* system_upgrade_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id )
{
    wiced_homekit_initialise_first_value( &system_upgrade_characteristic->value.current, value, value_length, value_name );

    system_upgrade_characteristic->description           = description;
    system_upgrade_characteristic->instance_id           = instance_id;
    system_upgrade_characteristic->type                  = HOMEKIT_SYSTEM_UPGRADE_UUID;
    system_upgrade_characteristic->permissions           = HOMEKIT_SYSTEM_UPGRADE_PERMISSIONS;
    system_upgrade_characteristic->format                = HOMEKIT_SYSTEM_UPGRADE_FORMAT;
    system_upgrade_characteristic->unit                  = HOMEKIT_SYSTEM_UPGRADE_UNIT;
    system_upgrade_characteristic->minimum_value         = HOMEKIT_SYSTEM_UPGRADE_MINIMUM_VALUE;
    system_upgrade_characteristic->maximum_value         = HOMEKIT_SYSTEM_UPGRADE_MAXIMUM_VALUE;
    system_upgrade_characteristic->minimum_step          = HOMEKIT_SYSTEM_UPGRADE_STEP_VALUE;
    system_upgrade_characteristic->maximum_length        = HOMEKIT_SYSTEM_UPGRADE_MAXIMUM_LENGTH;
    system_upgrade_characteristic->maximum_data_length   = HOMEKIT_SYSTEM_UPGRADE_MAXIMUM_DATA_LENGTH;
    system_upgrade_characteristic->identify_callback     = NULL;
}

/* Note: The value name is assumed to be a null terminated character string */
void wiced_homekit_initialise_first_value( wiced_homekit_value_descriptor_t* characteristic_value, char* value, uint8_t value_length, char* name )
{
    characteristic_value->value            = value;
    characteristic_value->value_length     = value_length;
    characteristic_value->name             = name;
    characteristic_value->name_length      = (uint8_t)strlen( name );
    characteristic_value->next             = NULL;
}
