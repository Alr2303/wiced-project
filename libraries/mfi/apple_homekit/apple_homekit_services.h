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

/******************************************************
 *                    Constants
 ******************************************************/

#define HOMEKIT_ACCESSORY_INFORMATION_SERVICE_UUID           "3E"
#define HOMEKIT_FAN_SERVICE_UUID                             "40"
#define HOMEKIT_GARAGE_DOOR_OPENER_SERVICE_UUID              "41"
#define HOMEKIT_LIGHTBULB_SERVICE_UUID                       "43"
#define HOMEKIT_LOCK_MANAGEMENT_SERVICE_UUID                 "44"
#define HOMEKIT_LOCK_MECHANISM_SERVICE_UUID                  "45"
#define HOMEKIT_OUTLET_SERVICE_UUID                          "47"
#define HOMEKIT_SWITCH_SERVICE_UUID                          "49"
#define HOMEKIT_THERMOSTAT_SERVICE_UUID                      "4A"
#define HOMEKIT_AIR_QUALITY_SENSOR_SERVICE_UUID              "8D"
#define HOMEKIT_SECURITY_SYSTEM_SERVICE_UUID                 "7E"
#define HOMEKIT_CARBON_MONOXIDE_SENSOR_SERVICE_UUID          "7F"
#define HOMEKIT_CONTACT_SENSOR_SERVICE_UUID                  "80"
#define HOMEKIT_DOOR_SERVICE_UUID                            "81"
#define HOMEKIT_HUMIDITY_SENSOR_SERVICE_UUID                 "82"
#define HOMEKIT_LEAK_SENSOR_SERVICE_UUID                     "83"
#define HOMEKIT_LIGHT_SENSOR_SERVICE_UUID                    "84"
#define HOMEKIT_MOTION_SENSOR_SERVICE_UUID                   "85"
#define HOMEKIT_OCCUPANCY_SENSOR_SERVICE_UUID                "86"
#define HOMEKIT_SMOKE_SENSOR_SERVICE_UUID                    "87"
#define HOMEKIT_STATEFUL_PROGRAMMABLE_SWITCH_SERVICE_UUID    "88"
#define HOMEKIT_STATELESS_PROGRAMMABLE_SWITCH_SERVICE_UUID   "89"
#define HOMEKIT_TEMPERATURE_SENSOR_SERVICE_UUID              "8A"
#define HOMEKIT_WINDOW_SERVICE_UUID                          "8B"
#define HOMEKIT_WINDOW_COVERING_SERVICE_UUID                 "8C"
#define HOMEKIT_BATTERY_SERVICE_UUID                         "96"
#define HOMEKIT_CARBON_DIOXIDE_SENSOR_SERVICE_UUID           "97"

/* Custom service ( Generate UUID as defined by RFC4122 for custom service or characteristic )*/
#define HOMEKIT_FIRMWARE_UPGRADE_SERVICE_UUID                "190F64B4-D3A1-4135-A7CB-0DAF1F782B3D"

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
/*****************************************************************************/
/**
 *  @defgroup mfi                           Apple MFi Protocols
 *  @ingroup  ipcoms
 *
 *  @addtogroup mfi_homekit                 HomeKit
 *  @ingroup mfi
 *
 *  @addtogroup mfi_homekit_service_init    Service Initialization
 *  @ingroup mfi_homekit
 *
 *  Functions to initialize HomeKit service
 *
 *  @{
 */
/*****************************************************************************/

/**
 * @{
 *
 * Initializes the given HomeKit service.
 *
 * @param   <ServiceName>_service [in] : Pointer to homekit service structure to be initialized.
 * @param   instance_id           [in] : Instance ID for the service. The instance ID should be unique across services and characteristics for the given accessory, valid range is from 1 to 65535.
 *
 * @return None
 */
void wiced_homekit_initialise_lightbulb_service                    ( wiced_homekit_services_t* lightbulb_service, uint16_t instance_id );
void wiced_homekit_initialise_garage_door_opener_service           ( wiced_homekit_services_t* garage_door_opener_service, uint16_t instance_id );
void wiced_homekit_initialise_accessory_information_service        ( wiced_homekit_services_t* accessory_information_service, uint16_t instance_id );
void wiced_homekit_initialise_firmware_upgrade_service             ( wiced_homekit_services_t* firmware_upgrade_service, uint16_t instance_id );
void wiced_homekit_initialise_thermostat_service                   ( wiced_homekit_services_t* thermostat_service, uint16_t instance_id );
void wiced_homekit_initialise_fan_service                          ( wiced_homekit_services_t* fan_service, uint16_t instance_id );
void wiced_homekit_initialise_outlet_service                       ( wiced_homekit_services_t* outlet_service, uint16_t instance_id );
void wiced_homekit_initialise_lock_management_service              ( wiced_homekit_services_t* lock_management_service, uint16_t instance_id );
void wiced_homekit_initialise_lock_mechanism_service               ( wiced_homekit_services_t* lock_mechanism_service, uint16_t instance_id );
void wiced_homekit_initialise_switch_service                       ( wiced_homekit_services_t* switch_service, uint16_t instance_id );
void wiced_homekit_initialise_air_quality_sensor_service           ( wiced_homekit_services_t* air_quality_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_security_system_service              ( wiced_homekit_services_t* security_system_service, uint16_t instance_id );
void wiced_homekit_initialise_carbon_monoxide_sensor_service       ( wiced_homekit_services_t* carbon_monoxide_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_contact_sensor_service               ( wiced_homekit_services_t* contact_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_door_service                         ( wiced_homekit_services_t* door_service, uint16_t instance_id );
void wiced_homekit_initialise_humidity_sensor_service              ( wiced_homekit_services_t* humidity_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_leak_sensor_service                  ( wiced_homekit_services_t* leak_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_light_sensor_service                 ( wiced_homekit_services_t* light_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_motion_sensor_service                ( wiced_homekit_services_t* motion_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_occupancy_sensor_service             ( wiced_homekit_services_t* occupancy_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_smoke_sensor_service                 ( wiced_homekit_services_t* smoke_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_temperature_sensor_service           ( wiced_homekit_services_t* temperature_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_window_service                       ( wiced_homekit_services_t* window_service, uint16_t instance_id );
void wiced_homekit_initialise_window_covering_service              ( wiced_homekit_services_t* window_covering_service, uint16_t instance_id );
void wiced_homekit_initialise_battery_service                      ( wiced_homekit_services_t* battery_service, uint16_t instance_id );
void wiced_homekit_initialise_carbon_dioxide_sensor_service        ( wiced_homekit_services_t* carbon_dioxide_sensor_service, uint16_t instance_id );
void wiced_homekit_initialise_stateful_programmable_switch_service ( wiced_homekit_services_t* stateful_programmable_switch_service, uint16_t instance_id );
void wiced_homekit_initialise_stateless_programmable_switch_service( wiced_homekit_services_t* stateless_programmable_switch_service, uint16_t instance_id );

/** @} */
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
