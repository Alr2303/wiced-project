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
#include "apple_homekit_services.h"

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

/******************************************************
 *               Function Definitions
 ******************************************************/

void wiced_homekit_initialise_lightbulb_service( wiced_homekit_services_t* lightbulb_service, uint16_t instance_id )
{
    lightbulb_service->type = HOMEKIT_LIGHTBULB_SERVICE_UUID;
    lightbulb_service->instance_id = instance_id;
}

void wiced_homekit_initialise_garage_door_opener_service( wiced_homekit_services_t* garage_door_opener_service, uint16_t instance_id )
{
    garage_door_opener_service->type = HOMEKIT_GARAGE_DOOR_OPENER_SERVICE_UUID;
    garage_door_opener_service->instance_id = instance_id;
}

void wiced_homekit_initialise_accessory_information_service( wiced_homekit_services_t* accessory_information_service, uint16_t instance_id )
{
    accessory_information_service->type = HOMEKIT_ACCESSORY_INFORMATION_SERVICE_UUID;
    accessory_information_service->instance_id = instance_id;
}

void wiced_homekit_initialise_thermostat_service( wiced_homekit_services_t* thermostat_service, uint16_t instance_id )
{
    thermostat_service->type = HOMEKIT_THERMOSTAT_SERVICE_UUID;
    thermostat_service->instance_id = instance_id;
}

void wiced_homekit_initialise_fan_service( wiced_homekit_services_t* fan_service, uint16_t instance_id )
{
    fan_service->type = HOMEKIT_FAN_SERVICE_UUID;
    fan_service->instance_id = instance_id;
}

void wiced_homekit_initialise_outlet_service( wiced_homekit_services_t* outlet_service, uint16_t instance_id )
{
    outlet_service->type = HOMEKIT_OUTLET_SERVICE_UUID;
    outlet_service->instance_id = instance_id;
}

void wiced_homekit_initialise_lock_management_service( wiced_homekit_services_t* lock_management_service, uint16_t instance_id )
{
    lock_management_service->type = HOMEKIT_LOCK_MANAGEMENT_SERVICE_UUID;
    lock_management_service->instance_id = instance_id;
}

void wiced_homekit_initialise_lock_mechanism_service( wiced_homekit_services_t* lock_mechanism_service, uint16_t instance_id )
{
    lock_mechanism_service->type = HOMEKIT_LOCK_MECHANISM_SERVICE_UUID;
    lock_mechanism_service->instance_id = instance_id;
}

void wiced_homekit_initialise_switch_service( wiced_homekit_services_t* switch_service, uint16_t instance_id )
{
    switch_service->type = HOMEKIT_SWITCH_SERVICE_UUID;
    switch_service->instance_id = instance_id;
}

void wiced_homekit_initialise_air_quality_sensor_service( wiced_homekit_services_t* air_quality_sensor_service, uint16_t instance_id )
{
    air_quality_sensor_service->type = HOMEKIT_AIR_QUALITY_SENSOR_SERVICE_UUID;
    air_quality_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_security_system_service( wiced_homekit_services_t* security_system_service, uint16_t instance_id )
{
    security_system_service->type = HOMEKIT_SECURITY_SYSTEM_SERVICE_UUID;
    security_system_service->instance_id = instance_id;
}

void wiced_homekit_initialise_carbon_monoxide_sensor_service( wiced_homekit_services_t* carbon_monoxide_sensor_service, uint16_t instance_id )
{
    carbon_monoxide_sensor_service->type = HOMEKIT_CARBON_MONOXIDE_SENSOR_SERVICE_UUID;
    carbon_monoxide_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_contact_sensor_service( wiced_homekit_services_t* contact_sensor_service, uint16_t instance_id )
{
    contact_sensor_service->type = HOMEKIT_CONTACT_SENSOR_SERVICE_UUID;
    contact_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_door_service( wiced_homekit_services_t* door_service, uint16_t instance_id )
{
    door_service->type = HOMEKIT_DOOR_SERVICE_UUID;
    door_service->instance_id = instance_id;
}

void wiced_homekit_initialise_humidity_sensor_service( wiced_homekit_services_t* humidity_sensor_service, uint16_t instance_id )
{
    humidity_sensor_service->type = HOMEKIT_HUMIDITY_SENSOR_SERVICE_UUID;
    humidity_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_leak_sensor_service( wiced_homekit_services_t* leak_sensor_service, uint16_t instance_id )
{
    leak_sensor_service->type = HOMEKIT_LEAK_SENSOR_SERVICE_UUID;
    leak_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_light_sensor_service( wiced_homekit_services_t* light_sensor_service, uint16_t instance_id )
{
    light_sensor_service->type = HOMEKIT_LIGHT_SENSOR_SERVICE_UUID;
    light_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_motion_sensor_service( wiced_homekit_services_t* motion_sensor_service, uint16_t instance_id )
{
    motion_sensor_service->type = HOMEKIT_MOTION_SENSOR_SERVICE_UUID;
    motion_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_occupancy_sensor_service( wiced_homekit_services_t* occupancy_sensor_service, uint16_t instance_id )
{
    occupancy_sensor_service->type = HOMEKIT_OCCUPANCY_SENSOR_SERVICE_UUID;
    occupancy_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_smoke_sensor_service( wiced_homekit_services_t* smoke_sensor_service, uint16_t instance_id )
{
    smoke_sensor_service->type = HOMEKIT_SMOKE_SENSOR_SERVICE_UUID;
    smoke_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_stateful_programmable_switch_service( wiced_homekit_services_t* stateful_programmable_switch_service, uint16_t instance_id )
{
    stateful_programmable_switch_service->type = HOMEKIT_STATEFUL_PROGRAMMABLE_SWITCH_SERVICE_UUID;
    stateful_programmable_switch_service->instance_id = instance_id;
}

void wiced_homekit_initialise_stateless_programmable_switch_service( wiced_homekit_services_t* stateless_programmable_switch_service, uint16_t instance_id )
{
    stateless_programmable_switch_service->type = HOMEKIT_STATELESS_PROGRAMMABLE_SWITCH_SERVICE_UUID;
    stateless_programmable_switch_service->instance_id = instance_id;
}

void wiced_homekit_initialise_temperature_sensor_service( wiced_homekit_services_t* temperature_sensor_service, uint16_t instance_id )
{
    temperature_sensor_service->type = HOMEKIT_TEMPERATURE_SENSOR_SERVICE_UUID;
    temperature_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_window_service( wiced_homekit_services_t* window_service, uint16_t instance_id )
{
    window_service->type = HOMEKIT_WINDOW_SERVICE_UUID;
    window_service->instance_id = instance_id;
}

void wiced_homekit_initialise_window_covering_service( wiced_homekit_services_t* window_covering_service, uint16_t instance_id )
{
    window_covering_service->type = HOMEKIT_WINDOW_COVERING_SERVICE_UUID;
    window_covering_service->instance_id = instance_id;
}

void wiced_homekit_initialise_battery_service( wiced_homekit_services_t* battery_service, uint16_t instance_id )
{
    battery_service->type = HOMEKIT_BATTERY_SERVICE_UUID;
    battery_service->instance_id = instance_id;
}

void wiced_homekit_initialise_carbon_dioxide_sensor_service( wiced_homekit_services_t* carbon_dioxide_sensor_service, uint16_t instance_id )
{
    carbon_dioxide_sensor_service->type = HOMEKIT_CARBON_DIOXIDE_SENSOR_SERVICE_UUID;
    carbon_dioxide_sensor_service->instance_id = instance_id;
}

void wiced_homekit_initialise_firmware_upgrade_service( wiced_homekit_services_t* firmware_upgrade_service, uint16_t instance_id )
{
    firmware_upgrade_service->type = HOMEKIT_FIRMWARE_UPGRADE_SERVICE_UUID;
    firmware_upgrade_service->instance_id = instance_id;
}
