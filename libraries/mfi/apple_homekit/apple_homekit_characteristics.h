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

/* HomeKit Name Characteristic  */
#define HOMEKIT_NAME_UUID                                          "23"
#define HOMEKIT_NAME_PERMISSIONS                                   WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_NAME_FORMAT                                        WICED_HOMEKIT_STRING_FORMAT
#define HOMEKIT_NAME_MAXIMUM_VALUE                                 0
#define HOMEKIT_NAME_MINIMUM_VALUE                                 0
#define HOMEKIT_NAME_UNIT                                          HOMEKIT_NO_UNIT_SET
#define HOMEKIT_NAME_STEP_VALUE                                    NULL
#define HOMEKIT_NAME_MAXIMUM_LENGTH                                64
#define HOMEKIT_NAME_MAXIMUM_DATA_LENGTH                           0

/* HomeKit ON Characteristic  */
#define HOMEKIT_ON_UUID                                            "25"
#define HOMEKIT_ON_PERMISSIONS                                     WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_ON_FORMAT                                          WICED_HOMEKIT_BOOLEAN_FORMAT
#define HOMEKIT_ON_MAXIMUM_VALUE                                   0    /* Max value is also set as 0 to indicate there is no min max value for boolean data type */
#define HOMEKIT_ON_MINIMUM_VALUE                                   0
#define HOMEKIT_ON_STEP_VALUE                                      NULL
#define HOMEKIT_ON_UNIT                                            HOMEKIT_NO_UNIT_SET
#define HOMEKIT_ON_MAXIMUM_LENGTH                                  0
#define HOMEKIT_ON_MAXIMUM_DATA_LENGTH                             0

/* HomeKit Brightness Characteristic  */
#define HOMEKIT_BRIGHTNESS_UUID                                    "8"
#define HOMEKIT_BRIGHTNESS_PERMISSIONS                             WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_BRIGHTNESS_FORMAT                                  WICED_HOMEKIT_INT_FORMAT
#define HOMEKIT_BRIGHTNESS_MAXIMUM_VALUE                           100
#define HOMEKIT_BRIGHTNESS_MINIMUM_VALUE                           0
#define HOMEKIT_BRIGHTNESS_STEP_VALUE                              "1"
#define HOMEKIT_BRIGHTNESS_UNIT                                    HOMEKIT_UNIT_PERCENTAGE
#define HOMEKIT_BRIGHTNESS_MAXIMUM_LENGTH                          0
#define HOMEKIT_BRIGHTNESS_MAXIMUM_DATA_LENGTH                     0

/* HomeKit Hue Characteristic  */
#define HOMEKIT_HUE_UUID                                           "13"
#define HOMEKIT_HUE_PERMISSIONS                                    WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_HUE_FORMAT                                         WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_HUE_MAXIMUM_VALUE                                  360
#define HOMEKIT_HUE_MINIMUM_VALUE                                  0
#define HOMEKIT_HUE_STEP_VALUE                                     "1"
#define HOMEKIT_HUE_UNIT                                           HOMEKIT_UNIT_ARC_DEGREE
#define HOMEKIT_HUE_MAXIMUM_LENGTH                                 0
#define HOMEKIT_HUE_MAXIMUM_DATA_LENGTH                            0

/* HomeKit Saturation Characteristic  */
#define HOMEKIT_SATURATION_UUID                                    "2F"
#define HOMEKIT_SATURATION_PERMISSIONS                             WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_SATURATION_FORMAT                                  WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_SATURATION_MAXIMUM_VALUE                           100
#define HOMEKIT_SATURATION_MINIMUM_VALUE                           0
#define HOMEKIT_SATURATION_STEP_VALUE                              "1"
#define HOMEKIT_SATURATION_UNIT                                    HOMEKIT_UNIT_PERCENTAGE
#define HOMEKIT_SATURATION_MAXIMUM_LENGTH                          0
#define HOMEKIT_SATURATION_MAXIMUM_DATA_LENGTH                     0

/* HomeKit Identify Characteristic  */
#define HOMEKIT_IDENTIFY_UUID                                      "14"
#define HOMEKIT_IDENTIFY_VALUE                                     "false"
#define HOMEKIT_IDENTIFY_PERMISSIONS                               WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE
#define HOMEKIT_IDENTIFY_FORMAT                                    WICED_HOMEKIT_BOOLEAN_FORMAT
#define HOMEKIT_IDENTIFY_MAXIMUM_VALUE                             0    /* Max value is also set as 0 to indicate there is no min max value for boolean data type */
#define HOMEKIT_IDENTIFY_MINIMUM_VALUE                             0
#define HOMEKIT_IDENTIFY_STEP_VALUE                                NULL
#define HOMEKIT_IDENTIFY_UNIT                                      HOMEKIT_NO_UNIT_SET
#define HOMEKIT_IDENTIFY_MAXIMUM_LENGTH                            0
#define HOMEKIT_IDENTIFY_MAXIMUM_DATA_LENGTH                       0

/* HomeKit Current Door State Characteristic  */
#define HOMEKIT_CURRENT_DOOR_STATE_UUID                            "E"
#define HOMEKIT_CURRENT_DOOR_STATE_PERMISSIONS                      WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CURRENT_DOOR_STATE_FORMAT                           WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_CURRENT_DOOR_STATE_MAXIMUM_VALUE                    4
#define HOMEKIT_CURRENT_DOOR_STATE_MINIMUM_VALUE                    0
#define HOMEKIT_CURRENT_DOOR_STATE_STEP_VALUE                       "1"
#define HOMEKIT_CURRENT_DOOR_STATE_UNIT                             HOMEKIT_NO_UNIT_SET
#define HOMEKIT_CURRENT_DOOR_STATE_MAXIMUM_LENGTH                   0
#define HOMEKIT_CURRENT_DOOR_STATE_MAXIMUM_DATA_LENGTH              0

/* HomeKit Target Door State Characteristic  */
#define HOMEKIT_TARGET_DOOR_STATE_UUID                              "32"
#define HOMEKIT_TARGET_DOOR_STATE_PERMISSIONS                       WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_TARGET_DOOR_STATE_FORMAT                            WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_TARGET_DOOR_STATE_MAXIMUM_VALUE                     1
#define HOMEKIT_TARGET_DOOR_STATE_MINIMUM_VALUE                     0
#define HOMEKIT_TARGET_DOOR_STATE_STEP_VALUE                        "1"
#define HOMEKIT_TARGET_DOOR_STATE_UNIT                              HOMEKIT_NO_UNIT_SET
#define HOMEKIT_TARGET_DOOR_STATE_MAXIMUM_LENGTH                    0
#define HOMEKIT_TARGET_DOOR_STATE_MAXIMUM_DATA_LENGTH               0

/* HomeKit Lock Mechanism Current State Characteristic  */
#define HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_UUID                  "1D"
#define HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_PERMISSIONS            WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_FORMAT                 WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_MAXIMUM_VALUE          3
#define HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_MINIMUM_VALUE          0
#define HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_STEP_VALUE             "1"
#define HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_UNIT                   HOMEKIT_NO_UNIT_SET
#define HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_MAXIMUM_LENGTH         0
#define HOMEKIT_LOCK_MECHANISM_CURRENT_STATE_MAXIMUM_DATA_LENGTH    0

/* HomeKit Obstruction Detected Characteristic  */
#define HOMEKIT_OBSTRUCTION_DETECTED_UUID                           "24"
#define HOMEKIT_OBSTRUCTION_DETECTED_PERMISSIONS                    WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_OBSTRUCTION_DETECTED_FORMAT                         WICED_HOMEKIT_BOOLEAN_FORMAT
#define HOMEKIT_OBSTRUCTION_DETECTED_MAXIMUM_VALUE                  0    /* Max value is also set as 0 to indicate there is no min max value for boolean data type */
#define HOMEKIT_OBSTRUCTION_DETECTED_MINIMUM_VALUE                  0
#define HOMEKIT_OBSTRUCTION_DETECTED_STEP_VALUE                     NULL
#define HOMEKIT_OBSTRUCTION_DETECTED_UNIT                           HOMEKIT_NO_UNIT_SET
#define HOMEKIT_OBSTRUCTION_DETECTED_MAXIMUM_LENGTH                 0
#define HOMEKIT_OBSTRUCTION_DETECTED_MAXIMUM_DATA_LENGTH            0

/* HomeKit Lock Mechanism Target State Characteristic  */
#define HOMEKIT_LOCK_MECHANISM_TARGET_STATE_UUID                   "1E"
#define HOMEKIT_LOCK_MECHANISM_TARGET_STATE_PERMISSIONS            WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_LOCK_MECHANISM_TARGET_STATE_FORMAT                 WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_LOCK_MECHANISM_TARGET_STATE_MAXIMUM_VALUE          1
#define HOMEKIT_LOCK_MECHANISM_TARGET_STATE_MINIMUM_VALUE          0
#define HOMEKIT_LOCK_MECHANISM_TARGET_STATE_STEP_VALUE             "1"
#define HOMEKIT_LOCK_MECHANISM_TARGET_STATE_UNIT                   HOMEKIT_NO_UNIT_SET
#define HOMEKIT_LOCK_MECHANISM_TARGET_STATE_MAXIMUM_LENGTH         0
#define HOMEKIT_LOCK_MECHANISM_TARGET_STATE_MAXIMUM_DATA_LENGTH    0

/* HomeKit Lock Management Logs Characteristic  */
#define HOMEKIT_LOGS_UUID                                           "1F"
#define HOMEKIT_LOGS_PERMISSIONS                                    WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_LOGS_FORMAT                                         WICED_HOMEKIT_TLV8_FORMAT
#define HOMEKIT_LOGS_MAXIMUM_VALUE                                  0
#define HOMEKIT_LOGS_MINIMUM_VALUE                                  0
#define HOMEKIT_LOGS_STEP_VALUE                                     NULL
#define HOMEKIT_LOGS_UNIT                                           HOMEKIT_NO_UNIT_SET
#define HOMEKIT_LOGS_MAXIMUM_LENGTH                                 0
#define HOMEKIT_LOGS_MAXIMUM_DATA_LENGTH                            0

/* HomeKit Manufacturer Information Characteristic */
#define HOMEKIT_MANUFACTURER_UUID                                  "20"
#define HOMEKIT_MANUFACTURER_PERMISSIONS                            WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_MANUFACTURER_FORMAT                                 WICED_HOMEKIT_STRING_FORMAT
#define HOMEKIT_MANUFACTURER_MAXIMUM_VALUE                          0
#define HOMEKIT_MANUFACTURER_MINIMUM_VALUE                          0
#define HOMEKIT_MANUFACTURER_UNIT                                   HOMEKIT_NO_UNIT_SET
#define HOMEKIT_MANUFACTURER_STEP_VALUE                             NULL
#define HOMEKIT_MANUFACTURER_MAXIMUM_LENGTH                         64
#define HOMEKIT_MANUFACTURER_MAXIMUM_DATA_LENGTH                    0

/* HomeKit Model Information Characteristic */
#define HOMEKIT_MODEL_UUID                                          "21"
#define HOMEKIT_MODEL_PERMISSIONS                                   WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_MODEL_FORMAT                                        WICED_HOMEKIT_STRING_FORMAT
#define HOMEKIT_MODEL_MAXIMUM_VALUE                                 0
#define HOMEKIT_MODEL_MINIMUM_VALUE                                 0
#define HOMEKIT_MODEL_UNIT                                          HOMEKIT_NO_UNIT_SET
#define HOMEKIT_MODEL_STEP_VALUE                                    NULL
#define HOMEKIT_MODEL_MAXIMUM_LENGTH                                64
#define HOMEKIT_MODEL_MAXIMUM_DATA_LENGTH                           0

/* HomeKit Serial Number Information Characteristic */
#define HOMEKIT_SERIAL_NUMBER_UUID                                  "30"
#define HOMEKIT_SERIAL_NUMBER_PERMISSIONS                           WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_SERIAL_NUMBER_FORMAT                                WICED_HOMEKIT_STRING_FORMAT
#define HOMEKIT_SERIAL_NUMBER_MAXIMUM_VALUE                         0
#define HOMEKIT_SERIAL_NUMBER_MINIMUM_VALUE                         0
#define HOMEKIT_SERIAL_NUMBER_UNIT                                  HOMEKIT_NO_UNIT_SET
#define HOMEKIT_SERIAL_NUMBER_STEP_VALUE                            NULL
#define HOMEKIT_SERIAL_NUMBER_MAXIMUM_LENGTH                        64
#define HOMEKIT_SERIAL_NUMBER_MAXIMUM_DATA_LENGTH                   0

/* HomeKit Firmware Revision Characteristic */
#define HOMEKIT_FIRMWARE_REVISION_UUID                              "52"
#define HOMEKIT_FIRMWARE_REVISION_PERMISSIONS                       WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_FIRMWARE_REVISION_FORMAT                            WICED_HOMEKIT_STRING_FORMAT
#define HOMEKIT_FIRMWARE_REVISION_MAXIMUM_VALUE                     0
#define HOMEKIT_FIRMWARE_REVISION_MINIMUM_VALUE                     0
#define HOMEKIT_FIRMWARE_REVISION_UNIT                              HOMEKIT_NO_UNIT_SET
#define HOMEKIT_FIRMWARE_REVISION_STEP_VALUE                        NULL
#define HOMEKIT_FIRMWARE_REVISION_MAXIMUM_LENGTH                    0
#define HOMEKIT_FIRMWARE_REVISION_MAXIMUM_DATA_LENGTH               0

/* HomeKit Heating-Cooling Current Characteristic */
#define HOMEKIT_HEATING_COOLING_CURRENT_UUID                        "F"
#define HOMEKIT_HEATING_COOLING_CURRENT_PERMISSIONS                 WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_HEATING_COOLING_CURRENT_FORMAT                      WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_HEATING_COOLING_CURRENT_MAXIMUM_VALUE               2
#define HOMEKIT_HEATING_COOLING_CURRENT_MINIMUM_VALUE               0
#define HOMEKIT_HEATING_COOLING_CURRENT_UNIT                        HOMEKIT_NO_UNIT_SET
#define HOMEKIT_HEATING_COOLING_CURRENT_STEP_VALUE                  "1"
#define HOMEKIT_HEATING_COOLING_CURRENT_MAXIMUM_LENGTH              0
#define HOMEKIT_HEATING_COOLING_CURRENT_MAXIMUM_DATA_LENGTH         0

/* HomeKit Heating-Cooling Target Characteristic */
#define HOMEKIT_HEATING_COOLING_TARGET_UUID                        "33"
#define HOMEKIT_HEATING_COOLING_TARGET_PERMISSIONS                 WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_HEATING_COOLING_TARGET_FORMAT                      WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_HEATING_COOLING_TARGET_MAXIMUM_VALUE               3
#define HOMEKIT_HEATING_COOLING_TARGET_MINIMUM_VALUE               0
#define HOMEKIT_HEATING_COOLING_TARGET_UNIT                        HOMEKIT_NO_UNIT_SET
#define HOMEKIT_HEATING_COOLING_TARGET_STEP_VALUE                  "1"
#define HOMEKIT_HEATING_COOLING_TARGET_MAXIMUM_LENGTH              0
#define HOMEKIT_HEATING_COOLING_TARGET_MAXIMUM_DATA_LENGTH         0

/* HomeKit Temperature Current Characteristic */
#define HOMEKIT_TEMPERATURE_CURRENT_UUID                           "11"
#define HOMEKIT_TEMPERATURE_CURRENT_PERMISSIONS                    WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_TEMPERATURE_CURRENT_FORMAT                         WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_TEMPERATURE_CURRENT_MAXIMUM_VALUE                  100
#define HOMEKIT_TEMPERATURE_CURRENT_MINIMUM_VALUE                  0
#define HOMEKIT_TEMPERATURE_CURRENT_UNIT                           HOMEKIT_UNIT_CELCIUS
#define HOMEKIT_TEMPERATURE_CURRENT_STEP_VALUE                     "0.1"
#define HOMEKIT_TEMPERATURE_CURRENT_MAXIMUM_LENGTH                 0
#define HOMEKIT_TEMPERATURE_CURRENT_MAXIMUM_DATA_LENGTH            0

/* HomeKit Temperature Target Characteristic */
#define HOMEKIT_TEMPERATURE_TARGET_UUID                            "35"
#define HOMEKIT_TEMPERATURE_TARGET_PERMISSIONS                     WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_TEMPERATURE_TARGET_FORMAT                          WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_TEMPERATURE_TARGET_MAXIMUM_VALUE                   38
#define HOMEKIT_TEMPERATURE_TARGET_MINIMUM_VALUE                   10
#define HOMEKIT_TEMPERATURE_TARGET_UNIT                            HOMEKIT_UNIT_CELCIUS
#define HOMEKIT_TEMPERATURE_TARGET_STEP_VALUE                      "0.1"
#define HOMEKIT_TEMPERATURE_TARGET_MAXIMUM_LENGTH                  0
#define HOMEKIT_TEMPERATURE_TARGET_MAXIMUM_DATA_LENGTH             0

/* HomeKit Temperature Units Characteristic */
#define HOMEKIT_TEMPERATURE_UNITS_UUID                             "36"
#define HOMEKIT_TEMPERATURE_UNITS_PERMISSIONS                      WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_TEMPERATURE_UNITS_FORMAT                           WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_TEMPERATURE_UNITS_MAXIMUM_VALUE                    1
#define HOMEKIT_TEMPERATURE_UNITS_MINIMUM_VALUE                    0
#define HOMEKIT_TEMPERATURE_UNITS_UNIT                             HOMEKIT_NO_UNIT_SET
#define HOMEKIT_TEMPERATURE_UNITS_STEP_VALUE                       "1"
#define HOMEKIT_TEMPERATURE_UNITS_MAXIMUM_LENGTH                   0
#define HOMEKIT_TEMPERATURE_UNITS_MAXIMUM_DATA_LENGTH              0

/* Outlet in Use Characteristic */
#define HOMEKIT_OUTLET_IN_USE_UUID                                 "26"
#define HOMEKIT_OUTLET_IN_USE_PERMISSIONS                          WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_OUTLET_IN_USE_FORMAT                               WICED_HOMEKIT_BOOLEAN_FORMAT
#define HOMEKIT_OUTLET_IN_USE_MAXIMUM_VALUE                        0    /* Max value is also set as 0 to indicate there is no min max value for boolean data type */
#define HOMEKIT_OUTLET_IN_USE_MINIMUM_VALUE                        0
#define HOMEKIT_OUTLET_IN_USE_UNIT                                 HOMEKIT_NO_UNIT_SET
#define HOMEKIT_OUTLET_IN_USE_STEP_VALUE                           NULL
#define HOMEKIT_OUTLET_IN_USE_MAXIMUM_LENGTH                       0
#define HOMEKIT_OUTLET_IN_USE_MAXIMUM_DATA_LENGTH                  0

/* Lock Management Control Point Characteristic */
#define HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_UUID                 "19"
#define HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_PERMISSIONS          WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE
#define HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_FORMAT               WICED_HOMEKIT_TLV8_FORMAT
#define HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_MAXIMUM_VALUE        0
#define HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_MINIMUM_VALUE        0
#define HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_UNIT                 HOMEKIT_NO_UNIT_SET
#define HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_STEP_VALUE           NULL
#define HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_MAXIMUM_LENGTH       0
#define HOMEKIT_LOCK_MANAGEMENT_CONTROL_POINT_MAXIMUM_DATA_LENGTH  0

/* Version Characteristic */
#define HOMEKIT_VERSION_UUID                                       "37"
#define HOMEKIT_VERSION_PERMISSIONS                                WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_VERSION_FORMAT                                     WICED_HOMEKIT_STRING_FORMAT
#define HOMEKIT_VERSION_MAXIMUM_VALUE                              0
#define HOMEKIT_VERSION_MINIMUM_VALUE                              0
#define HOMEKIT_VERSION_UNIT                                       HOMEKIT_NO_UNIT_SET
#define HOMEKIT_VERSION_STEP_VALUE                                 NULL
#define HOMEKIT_VERSION_MAXIMUM_LENGTH                             64
#define HOMEKIT_VERSION_MAXIMUM_DATA_LENGTH                        0

/*  Air Particulate Density Characteristic  */
#define HOMEKIT_AIR_PARTICULATE_DENSITY_UUID                       "64"
#define HOMEKIT_AIR_PARTICULATE_DENSITY_PERMISSIONS                WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_AIR_PARTICULATE_DENSITY_FORMAT                     WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_AIR_PARTICULATE_DENSITY_MAXIMUM_VALUE              1000
#define HOMEKIT_AIR_PARTICULATE_DENSITY_MINIMUM_VALUE              0
#define HOMEKIT_AIR_PARTICULATE_DENSITY_UNIT                       HOMEKIT_NO_UNIT_SET
#define HOMEKIT_AIR_PARTICULATE_DENSITY_STEP_VALUE                 NULL
#define HOMEKIT_AIR_PARTICULATE_DENSITY_MAXIMUM_LENGTH             0
#define HOMEKIT_AIR_PARTICULATE_DENSITY_MAXIMUM_DATA_LENGTH        0

/*  Air Particulate Size Characteristic  */
#define HOMEKIT_AIR_PARTICULATE_SIZE_UUID                          "65"
#define HOMEKIT_AIR_PARTICULATE_SIZE_PERMISSIONS                   WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_AIR_PARTICULATE_SIZE_FORMAT                        WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_AIR_PARTICULATE_SIZE_MAXIMUM_VALUE                 1
#define HOMEKIT_AIR_PARTICULATE_SIZE_MINIMUM_VALUE                 0
#define HOMEKIT_AIR_PARTICULATE_SIZE_UNIT                          HOMEKIT_NO_UNIT_SET
#define HOMEKIT_AIR_PARTICULATE_SIZE_STEP_VALUE                    "1"
#define HOMEKIT_AIR_PARTICULATE_SIZE_MAXIMUM_LENGTH                0
#define HOMEKIT_AIR_PARTICULATE_SIZE_MAXIMUM_DATA_LENGTH           0

/*  Air Quality Characteristic  */
#define HOMEKIT_AIR_QUALITY_UUID                                   "95"
#define HOMEKIT_AIR_QUALITY_PERMISSIONS                            WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_AIR_QUALITY_FORMAT                                 WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_AIR_QUALITY_MAXIMUM_VALUE                          5
#define HOMEKIT_AIR_QUALITY_MINIMUM_VALUE                          0
#define HOMEKIT_AIR_QUALITY_UNIT                                   HOMEKIT_NO_UNIT_SET
#define HOMEKIT_AIR_QUALITY_STEP_VALUE                             "1"
#define HOMEKIT_AIR_QUALITY_MAXIMUM_LENGTH                         0
#define HOMEKIT_AIR_QUALITY_MAXIMUM_DATA_LENGTH                    0

/*  Status Active Characteristic  */
#define HOMEKIT_STATUS_ACTIVE_UUID                                 "75"
#define HOMEKIT_STATUS_ACTIVE_PERMISSIONS                          WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_STATUS_ACTIVE_FORMAT                               WICED_HOMEKIT_BOOLEAN_FORMAT
#define HOMEKIT_STATUS_ACTIVE_MAXIMUM_VALUE                        0    /* Max value is also set as 0 to indicate there is no min max value for boolean data type */
#define HOMEKIT_STATUS_ACTIVE_MINIMUM_VALUE                        0
#define HOMEKIT_STATUS_ACTIVE_UNIT                                 HOMEKIT_NO_UNIT_SET
#define HOMEKIT_STATUS_ACTIVE_STEP_VALUE                           NULL
#define HOMEKIT_STATUS_ACTIVE_MAXIMUM_LENGTH                       0
#define HOMEKIT_STATUS_ACTIVE_MAXIMUM_DATA_LENGTH                  0

/*  Status Fault Characteristic  */
#define HOMEKIT_STATUS_FAULT_UUID                                  "77"
#define HOMEKIT_STATUS_FAULT_PERMISSIONS                           WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_STATUS_FAULT_FORMAT                                WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_STATUS_FAULT_MAXIMUM_VALUE                         1
#define HOMEKIT_STATUS_FAULT_MINIMUM_VALUE                         0
#define HOMEKIT_STATUS_FAULT_UNIT                                  HOMEKIT_NO_UNIT_SET
#define HOMEKIT_STATUS_FAULT_STEP_VALUE                            "1"
#define HOMEKIT_STATUS_FAULT_MAXIMUM_LENGTH                        0
#define HOMEKIT_STATUS_FAULT_MAXIMUM_DATA_LENGTH                   0

/*  Status Tampered Characteristic  */
#define HOMEKIT_STATUS_TAMPERED_UUID                               "7A"
#define HOMEKIT_STATUS_TAMPERED_PERMISSIONS                        WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_STATUS_TAMPERED_FORMAT                             WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_STATUS_TAMPERED_MAXIMUM_VALUE                      1
#define HOMEKIT_STATUS_TAMPERED_MINIMUM_VALUE                      0
#define HOMEKIT_STATUS_TAMPERED_UNIT                               HOMEKIT_NO_UNIT_SET
#define HOMEKIT_STATUS_TAMPERED_STEP_VALUE                         "1"
#define HOMEKIT_STATUS_TAMPERED_MAXIMUM_LENGTH                     0
#define HOMEKIT_STATUS_TAMPERED_MAXIMUM_DATA_LENGTH                0

/*  Status Low Battery Characteristic  */
#define HOMEKIT_STATUS_LOW_BATTERY_UUID                            "79"
#define HOMEKIT_STATUS_LOW_BATTERY_PERMISSIONS                     WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_STATUS_LOW_BATTERY_FORMAT                          WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_STATUS_LOW_BATTERY_MAXIMUM_VALUE                   1
#define HOMEKIT_STATUS_LOW_BATTERY_MINIMUM_VALUE                   0
#define HOMEKIT_STATUS_LOW_BATTERY_UNIT                            HOMEKIT_NO_UNIT_SET
#define HOMEKIT_STATUS_LOW_BATTERY_STEP_VALUE                      "1"
#define HOMEKIT_STATUS_LOW_BATTERY_MAXIMUM_LENGTH                  0
#define HOMEKIT_STATUS_LOW_BATTERY_MAXIMUM_DATA_LENGTH             0

/*  Status Jammed Characteristic  */
#define HOMEKIT_STATUS_JAMMED_UUID                                 "78"
#define HOMEKIT_STATUS_JAMMED_PERMISSIONS                          WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_STATUS_JAMMED_FORMAT                               WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_STATUS_JAMMED_MAXIMUM_VALUE                        1
#define HOMEKIT_STATUS_JAMMED_MINIMUM_VALUE                        0
#define HOMEKIT_STATUS_JAMMED_UNIT                                 HOMEKIT_NO_UNIT_SET
#define HOMEKIT_STATUS_JAMMED_STEP_VALUE                           "1"
#define HOMEKIT_STATUS_JAMMED_MAXIMUM_LENGTH                       0
#define HOMEKIT_STATUS_JAMMED_MAXIMUM_DATA_LENGTH                  0

/* Security System Current State Characteristic*/
#define HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_UUID                 "66"
#define HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_PERMISSIONS          WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_FORMAT               WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_MAXIMUM_VALUE        4
#define HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_MINIMUM_VALUE        0
#define HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_UNIT                 HOMEKIT_NO_UNIT_SET
#define HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_STEP_VALUE           "1"
#define HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_MAXIMUM_LENGTH       0
#define HOMEKIT_SECURITY_SYSTEM_CURRENT_STATE_MAXIMUM_DATA_LENGTH  0

/* Security System Target State Characteristic*/
#define HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_UUID                  "67"
#define HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_PERMISSIONS           WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_FORMAT                WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_MAXIMUM_VALUE         3
#define HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_MINIMUM_VALUE         0
#define HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_UNIT                  HOMEKIT_NO_UNIT_SET
#define HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_STEP_VALUE            "1"
#define HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_MAXIMUM_LENGTH        0
#define HOMEKIT_SECURITY_SYSTEM_TARGET_STATE_MAXIMUM_DATA_LENGTH   0

/* Security System Alarm Type Characteristic*/
#define HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_UUID                    "8E"
#define HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_PERMISSIONS             WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_FORMAT                  WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_MAXIMUM_VALUE           1
#define HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_MINIMUM_VALUE           0
#define HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_UNIT                    HOMEKIT_NO_UNIT_SET
#define HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_STEP_VALUE              "1"
#define HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_MAXIMUM_LENGTH          0
#define HOMEKIT_SECURITY_SYSTEM_ALARM_TYPE_MAXIMUM_DATA_LENGTH     0

/* Carbon Monoxide Detected Characteristic*/
#define HOMEKIT_CARBON_MONOXIDE_DETECTED_UUID                      "69"
#define HOMEKIT_CARBON_MONOXIDE_DETECTED_PERMISSIONS               WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CARBON_MONOXIDE_DETECTED_FORMAT                    WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_CARBON_MONOXIDE_DETECTED_MAXIMUM_VALUE             1
#define HOMEKIT_CARBON_MONOXIDE_DETECTED_MINIMUM_VALUE             0
#define HOMEKIT_CARBON_MONOXIDE_DETECTED_UNIT                      HOMEKIT_NO_UNIT_SET
#define HOMEKIT_CARBON_MONOXIDE_DETECTED_STEP_VALUE                "1"
#define HOMEKIT_CARBON_MONOXIDE_DETECTED_MAXIMUM_LENGTH            0
#define HOMEKIT_CARBON_MONOXIDE_DETECTED_MAXIMUM_DATA_LENGTH       0

/* Carbon Monoxide Level Characteristic*/
#define HOMEKIT_CARBON_MONOXIDE_LEVEL_UUID                         "90"
#define HOMEKIT_CARBON_MONOXIDE_LEVEL_PERMISSIONS                  WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CARBON_MONOXIDE_LEVEL_FORMAT                       WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_CARBON_MONOXIDE_LEVEL_MAXIMUM_VALUE                100
#define HOMEKIT_CARBON_MONOXIDE_LEVEL_MINIMUM_VALUE                0
#define HOMEKIT_CARBON_MONOXIDE_LEVEL_UNIT                         HOMEKIT_NO_UNIT_SET
#define HOMEKIT_CARBON_MONOXIDE_LEVEL_STEP_VALUE                   NULL
#define HOMEKIT_CARBON_MONOXIDE_LEVEL_MAXIMUM_LENGTH               0
#define HOMEKIT_CARBON_MONOXIDE_LEVEL_MAXIMUM_DATA_LENGTH          0

/* Carbon Monoxide Peak Level Characteristic*/
#define HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_UUID                    "91"
#define HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_PERMISSIONS             WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_FORMAT                  WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_MAXIMUM_VALUE           100
#define HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_MINIMUM_VALUE           0
#define HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_UNIT                    HOMEKIT_NO_UNIT_SET
#define HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_STEP_VALUE              NULL
#define HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_MAXIMUM_LENGTH          0
#define HOMEKIT_CARBON_MONOXIDE_PEAK_LEVEL_MAXIMUM_DATA_LENGTH     0

/* Contact Sensor State Characteristic*/
#define HOMEKIT_CONTACT_SENSOR_STATE_UUID                          "6A"
#define HOMEKIT_CONTACT_SENSOR_STATE_PERMISSIONS                   WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CONTACT_SENSOR_STATE_FORMAT                        WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_CONTACT_SENSOR_STATE_MAXIMUM_VALUE                 1
#define HOMEKIT_CONTACT_SENSOR_STATE_MINIMUM_VALUE                 0
#define HOMEKIT_CONTACT_SENSOR_STATE_UNIT                          HOMEKIT_NO_UNIT_SET
#define HOMEKIT_CONTACT_SENSOR_STATE_STEP_VALUE                    "1"
#define HOMEKIT_CONTACT_SENSOR_STATE_MAXIMUM_LENGTH                0
#define HOMEKIT_CONTACT_SENSOR_STATE_MAXIMUM_DATA_LENGTH           0


/* Current Position Characteristic */
#define HOMEKIT_CURRENT_POSITION_UUID                              "6D"
#define HOMEKIT_CURRENT_POSITION_PERMISSIONS                       WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CURRENT_POSITION_FORMAT                            WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_CURRENT_POSITION_MAXIMUM_VALUE                     100
#define HOMEKIT_CURRENT_POSITION_MINIMUM_VALUE                     0
#define HOMEKIT_CURRENT_POSITION_UNIT                              HOMEKIT_UNIT_PERCENTAGE
#define HOMEKIT_CURRENT_POSITION_STEP_VALUE                        "1"
#define HOMEKIT_CURRENT_POSITION_MAXIMUM_LENGTH                    0
#define HOMEKIT_CURRENT_POSITION_MAXIMUM_DATA_LENGTH               0

/* Target Position Characteristic */
#define HOMEKIT_TARGET_POSITION_UUID                               "7C"
#define HOMEKIT_TARGET_POSITION_PERMISSIONS                        WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_TARGET_POSITION_FORMAT                             WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_TARGET_POSITION_MAXIMUM_VALUE                      100
#define HOMEKIT_TARGET_POSITION_MINIMUM_VALUE                      0
#define HOMEKIT_TARGET_POSITION_UNIT                               HOMEKIT_UNIT_PERCENTAGE
#define HOMEKIT_TARGET_POSITION_STEP_VALUE                         "1"
#define HOMEKIT_TARGET_POSITION_MAXIMUM_LENGTH                     0
#define HOMEKIT_TARGET_POSITION_MAXIMUM_DATA_LENGTH                0

/* Position State Characteristic */
#define HOMEKIT_POSITION_STATE_UUID                                "72"
#define HOMEKIT_POSITION_STATE_PERMISSIONS                         WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_POSITION_STATE_FORMAT                              WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_POSITION_STATE_MAXIMUM_VALUE                       2
#define HOMEKIT_POSITION_STATE_MINIMUM_VALUE                       0
#define HOMEKIT_POSITION_STATE_UNIT                                HOMEKIT_NO_UNIT_SET
#define HOMEKIT_POSITION_STATE_STEP_VALUE                          "1"
#define HOMEKIT_POSITION_STATE_MAXIMUM_LENGTH                      0
#define HOMEKIT_POSITION_STATE_MAXIMUM_DATA_LENGTH                 0

/* Leak Detected Characteristic*/
#define HOMEKIT_LEAK_DETECTED_UUID                                 "70"
#define HOMEKIT_LEAK_DETECTED_PERMISSIONS                          WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_LEAK_DETECTED_FORMAT                               WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_LEAK_DETECTED_MAXIMUM_VALUE                        1
#define HOMEKIT_LEAK_DETECTED_MINIMUM_VALUE                        0
#define HOMEKIT_LEAK_DETECTED_UNIT                                 HOMEKIT_NO_UNIT_SET
#define HOMEKIT_LEAK_DETECTED_STEP_VALUE                           "1"
#define HOMEKIT_LEAK_DETECTED_MAXIMUM_LENGTH                       0
#define HOMEKIT_LEAK_DETECTED_MAXIMUM_DATA_LENGTH                  0

/* Current ambient light level Detected Characteristic*/
#define HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_UUID                   "6B"
#define HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_PERMISSIONS            WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_FORMAT                 WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_MAXIMUM_VALUE          100000
#define HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_MINIMUM_VALUE          0.0001f
#define HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_UNIT                   HOMEKIT_UNIT_LUX
#define HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_STEP_VALUE             NULL
#define HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_MAXIMUM_LENGTH         0
#define HOMEKIT_CURRENT_AMBIENT_LIGHT_LEVEL_MAXIMUM_DATA_LENGTH    0


/* Occupancy Detected Characteristic*/
#define HOMEKIT_OCCUPANCY_DETECTED_UUID                            "71"
#define HOMEKIT_OCCUPANCY_DETECTED_PERMISSIONS                     WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_OCCUPANCY_DETECTED_FORMAT                          WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_OCCUPANCY_DETECTED_MAXIMUM_VALUE                   1
#define HOMEKIT_OCCUPANCY_DETECTED_MINIMUM_VALUE                   0
#define HOMEKIT_OCCUPANCY_DETECTED_UNIT                            HOMEKIT_NO_UNIT_SET
#define HOMEKIT_OCCUPANCY_DETECTED_STEP_VALUE                      "1"
#define HOMEKIT_OCCUPANCY_DETECTED_MAXIMUM_LENGTH                  0
#define HOMEKIT_OCCUPANCY_DETECTED_MAXIMUM_DATA_LENGTH             0

/* Smoke Detected Characteristic*/
#define HOMEKIT_SMOKE_DETECTED_UUID                                "76"
#define HOMEKIT_SMOKE_DETECTED_PERMISSIONS                         WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_SMOKE_DETECTED_FORMAT                              WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_SMOKE_DETECTED_MAXIMUM_VALUE                       1
#define HOMEKIT_SMOKE_DETECTED_MINIMUM_VALUE                       0
#define HOMEKIT_SMOKE_DETECTED_UNIT                                HOMEKIT_NO_UNIT_SET
#define HOMEKIT_SMOKE_DETECTED_STEP_VALUE                          "1"
#define HOMEKIT_SMOKE_DETECTED_MAXIMUM_LENGTH                      0
#define HOMEKIT_SMOKE_DETECTED_MAXIMUM_DATA_LENGTH                 0

/* Programmable Switch Event Characteristic*/
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_UUID                     "73"
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_PERMISSIONS              WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_FORMAT                   WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_MAXIMUM_VALUE            1
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_MINIMUM_VALUE            0
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_UNIT                     HOMEKIT_NO_UNIT_SET
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_STEP_VALUE               "1"
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_MAXIMUM_LENGTH           0
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_MAXIMUM_DATA_LENGTH      0

/* Programmable Switch Output state Characteristic*/
#define HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_UUID                   "74"
#define HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_PERMISSIONS            WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_FORMAT                 WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_MAXIMUM_VALUE          1
#define HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_MINIMUM_VALUE          0
#define HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_UNIT                   HOMEKIT_NO_UNIT_SET
#define HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_STEP_VALUE             "1"
#define HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_MAXIMUM_LENGTH         0
#define HOMEKIT_PROGRAMMABLE_SWITCH_OUTPUT_STATE_MAXIMUM_DATA_LENGTH    0

/* Battery Level Characteristic */
#define HOMEKIT_BATTERY_LEVEL_UUID                                 "68"
#define HOMEKIT_BATTERY_LEVEL_PERMISSIONS                          WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_BATTERY_LEVEL_FORMAT                               WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_BATTERY_LEVEL_MAXIMUM_VALUE                        100
#define HOMEKIT_BATTERY_LEVEL_MINIMUM_VALUE                        0
#define HOMEKIT_BATTERY_LEVEL_UNIT                                 HOMEKIT_UNIT_PERCENTAGE
#define HOMEKIT_BATTERY_LEVEL_STEP_VALUE                           "1"
#define HOMEKIT_BATTERY_LEVEL_MAXIMUM_LENGTH                       0
#define HOMEKIT_BATTERY_LEVEL_MAXIMUM_DATA_LENGTH                  0

/* Charging State Characteristic */
#define HOMEKIT_CHARGING_STATE_UUID                                "8F"
#define HOMEKIT_CHARGING_STATE_PERMISSIONS                         WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CHARGING_STATE_FORMAT                              WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_CHARGING_STATE_MAXIMUM_VALUE                       1
#define HOMEKIT_CHARGING_STATE_MINIMUM_VALUE                       0
#define HOMEKIT_CHARGING_STATE_UNIT                                HOMEKIT_NO_UNIT_SET
#define HOMEKIT_CHARGING_STATE_STEP_VALUE                          "1"
#define HOMEKIT_CHARGING_STATE_MAXIMUM_LENGTH                      0
#define HOMEKIT_CHARGING_STATE_MAXIMUM_DATA_LENGTH                 0

/* Carbon Dioxide Detected Characteristic*/
#define HOMEKIT_CARBON_DIOXIDE_DETECTED_UUID                       "92"
#define HOMEKIT_CARBON_DIOXIDE_DETECTED_PERMISSIONS                WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CARBON_DIOXIDE_DETECTED_FORMAT                     WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_CARBON_DIOXIDE_DETECTED_MAXIMUM_VALUE              1
#define HOMEKIT_CARBON_DIOXIDE_DETECTED_MINIMUM_VALUE              0
#define HOMEKIT_CARBON_DIOXIDE_DETECTED_UNIT                       HOMEKIT_NO_UNIT_SET
#define HOMEKIT_CARBON_DIOXIDE_DETECTED_STEP_VALUE                 "1"
#define HOMEKIT_CARBON_DIOXIDE_DETECTED_MAXIMUM_LENGTH             0
#define HOMEKIT_CARBON_DIOXIDE_DETECTED_MAXIMUM_DATA_LENGTH        0

/* Carbon Dioxide Level Characteristic*/
#define HOMEKIT_CARBON_DIOXIDE_LEVEL_UUID                          "93"
#define HOMEKIT_CARBON_DIOXIDE_LEVEL_PERMISSIONS                   WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CARBON_DIOXIDE_LEVEL_FORMAT                        WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_CARBON_DIOXIDE_LEVEL_MAXIMUM_VALUE                 100000
#define HOMEKIT_CARBON_DIOXIDE_LEVEL_MINIMUM_VALUE                 0
#define HOMEKIT_CARBON_DIOXIDE_LEVEL_UNIT                          HOMEKIT_NO_UNIT_SET
#define HOMEKIT_CARBON_DIOXIDE_LEVEL_STEP_VALUE                    NULL
#define HOMEKIT_CARBON_DIOXIDE_LEVEL_MAXIMUM_LENGTH                0
#define HOMEKIT_CARBON_DIOXIDE_LEVEL_MAXIMUM_DATA_LENGTH           0

/* Carbon Dioxide Peak Level Characteristic*/
#define HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_UUID                     "94"
#define HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_PERMISSIONS              WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_FORMAT                   WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_MAXIMUM_VALUE            100000
#define HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_MINIMUM_VALUE            0
#define HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_UNIT                     HOMEKIT_NO_UNIT_SET
#define HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_STEP_VALUE               NULL
#define HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_MAXIMUM_LENGTH           0
#define HOMEKIT_CARBON_DIOXIDE_PEAK_LEVEL_MAXIMUM_DATA_LENGTH      0

/* Administrator Only Access Characteristic  */
#define HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_UUID                     "1"
#define HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_PERMISSIONS              WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_FORMAT                   WICED_HOMEKIT_BOOLEAN_FORMAT
#define HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_MAXIMUM_VALUE            0    /* Max value is also set as 0 to indicate there is no min max value for boolean data type */
#define HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_MINIMUM_VALUE            0
#define HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_UNIT                     HOMEKIT_NO_UNIT_SET
#define HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_STEP_VALUE               NULL
#define HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_MAXIMUM_LENGTH           0
#define HOMEKIT_ADMINISTRATOR_ONLY_ACCESS_MAXIMUM_DATA_LENGTH      0

/* Audio Feedback Characteristic  */
#define HOMEKIT_AUDIO_FEEDBACK_UUID                                "5"
#define HOMEKIT_AUDIO_FEEDBACK_PERMISSIONS                         WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_AUDIO_FEEDBACK_FORMAT                              WICED_HOMEKIT_BOOLEAN_FORMAT
#define HOMEKIT_AUDIO_FEEDBACK_MAXIMUM_VALUE                       0    /* Max value is also set as 0 to indicate there is no min max value for boolean data type */
#define HOMEKIT_AUDIO_FEEDBACK_MINIMUM_VALUE                       0
#define HOMEKIT_AUDIO_FEEDBACK_UNIT                                HOMEKIT_NO_UNIT_SET
#define HOMEKIT_AUDIO_FEEDBACK_STEP_VALUE                          NULL
#define HOMEKIT_AUDIO_FEEDBACK_MAXIMUM_LENGTH                      0
#define HOMEKIT_AUDIO_FEEDBACK_MAXIMUM_DATA_LENGTH                 0

/* Cooling Threshold Temperature Characteristic  */
#define HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_UUID                 "D"
#define HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_PERMISSIONS          WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_FORMAT               WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_MAXIMUM_VALUE        35
#define HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_MINIMUM_VALUE        10
#define HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_UNIT                 HOMEKIT_UNIT_CELCIUS
#define HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_STEP_VALUE           "0.1"
#define HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_MAXIMUM_LENGTH       0
#define HOMEKIT_COOLING_THRESHOLD_TEMPERATURE_MAXIMUM_DATA_LENGTH  0

/* Current Relative Humidity Characteristic  */
#define HOMEKIT_CURRENT_RELATIVE_HUMIDITY_UUID                     "10"
#define HOMEKIT_CURRENT_RELATIVE_HUMIDITY_PERMISSIONS              WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CURRENT_RELATIVE_HUMIDITY_FORMAT                   WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_CURRENT_RELATIVE_HUMIDITY_MAXIMUM_VALUE            100
#define HOMEKIT_CURRENT_RELATIVE_HUMIDITY_MINIMUM_VALUE            0
#define HOMEKIT_CURRENT_RELATIVE_HUMIDITY_UNIT                     HOMEKIT_UNIT_PERCENTAGE
#define HOMEKIT_CURRENT_RELATIVE_HUMIDITY_STEP_VALUE               "1"
#define HOMEKIT_CURRENT_RELATIVE_HUMIDITY_MAXIMUM_LENGTH           0
#define HOMEKIT_CURRENT_RELATIVE_HUMIDITY_MAXIMUM_DATA_LENGTH      0

/* Heating Threshold Temperature Characteristic  */
#define HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_UUID                 "12"
#define HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_PERMISSIONS          WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_FORMAT               WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_MAXIMUM_VALUE        25
#define HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_MINIMUM_VALUE        0
#define HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_UNIT                 HOMEKIT_UNIT_CELCIUS
#define HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_STEP_VALUE           "0.1"
#define HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_MAXIMUM_LENGTH       0
#define HOMEKIT_HEATING_THRESHOLD_TEMPERATURE_MAXIMUM_DATA_LENGTH  0

/* Lock Last Known Action Characteristic  */
#define HOMEKIT_LOCK_LAST_KNOWN_ACTION_UUID                        "1C"
#define HOMEKIT_LOCK_LAST_KNOWN_ACTION_PERMISSIONS                 WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_LOCK_LAST_KNOWN_ACTION_FORMAT                      WICED_HOMEKIT_UINT8_FORMAT
#define HOMEKIT_LOCK_LAST_KNOWN_ACTION_MAXIMUM_VALUE               8
#define HOMEKIT_LOCK_LAST_KNOWN_ACTION_MINIMUM_VALUE               0
#define HOMEKIT_LOCK_LAST_KNOWN_ACTION_UNIT                        HOMEKIT_NO_UNIT_SET
#define HOMEKIT_LOCK_LAST_KNOWN_ACTION_STEP_VALUE                  "1"
#define HOMEKIT_LOCK_LAST_KNOWN_ACTION_MAXIMUM_LENGTH              0
#define HOMEKIT_LOCK_LAST_KNOWN_ACTION_MAXIMUM_DATA_LENGTH         0

/* Lock Management Auto Security Timeout Characteristic  */
#define HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_UUID                    "1A"
#define HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_PERMISSIONS             WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_FORMAT                  WICED_HOMEKIT_UINT32_FORMAT
#define HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_MAXIMUM_VALUE           0
#define HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_MINIMUM_VALUE           0
#define HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_UNIT                    HOMEKIT_UNIT_SECOND
#define HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_STEP_VALUE              NULL
#define HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_MAXIMUM_LENGTH          0
#define HOMEKIT_LOCK_AUTO_SECURITY_TIMEOUT_MAXIMUM_DATA_LENGTH     0

/* HomeKit Log Characteristic  */
#define HOMEKIT_LOG_UUID                                           "1F"
#define HOMEKIT_LOG_PERMISSIONS                                    WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_LOG_FORMAT                                         WICED_HOMEKIT_TLV8_FORMAT
#define HOMEKIT_LOG_MAXIMUM_VALUE                                  0
#define HOMEKIT_LOG_MINIMUM_VALUE                                  0
#define HOMEKIT_LOG_UNIT                                           HOMEKIT_NO_UNIT_SET
#define HOMEKIT_LOG_STEP_VALUE                                     NULL
#define HOMEKIT_LOG_MAXIMUM_LENGTH                                 0
#define HOMEKIT_LOG_MAXIMUM_DATA_LENGTH                            0

/* Motion Detected Characteristic  */
#define HOMEKIT_MOTION_DETECTED_UUID                               "22"
#define HOMEKIT_MOTION_DETECTED_PERMISSIONS                        WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_MOTION_DETECTED_FORMAT                             WICED_HOMEKIT_BOOLEAN_FORMAT
#define HOMEKIT_MOTION_DETECTED_MAXIMUM_VALUE                      0    /* Max value is also set as 0 to indicate there is no min max value for boolean data type */
#define HOMEKIT_MOTION_DETECTED_MINIMUM_VALUE                      0
#define HOMEKIT_MOTION_DETECTED_UNIT                               HOMEKIT_NO_UNIT_SET
#define HOMEKIT_MOTION_DETECTED_STEP_VALUE                         NULL
#define HOMEKIT_MOTION_DETECTED_MAXIMUM_LENGTH                     0
#define HOMEKIT_MOTION_DETECTED_MAXIMUM_DATA_LENGTH                0

/* Rotation Direction Characteristic  */
#define HOMEKIT_ROTATION_DIRECTION_UUID                            "28"
#define HOMEKIT_ROTATION_DIRECTION_PERMISSIONS                     WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_ROTATION_DIRECTION_FORMAT                          WICED_HOMEKIT_INT_FORMAT
#define HOMEKIT_ROTATION_DIRECTION_MAXIMUM_VALUE                   1
#define HOMEKIT_ROTATION_DIRECTION_MINIMUM_VALUE                   0
#define HOMEKIT_ROTATION_DIRECTION_UNIT                            HOMEKIT_NO_UNIT_SET
#define HOMEKIT_ROTATION_DIRECTION_STEP_VALUE                      "1"
#define HOMEKIT_ROTATION_DIRECTION_MAXIMUM_LENGTH                  0
#define HOMEKIT_ROTATION_DIRECTION_MAXIMUM_DATA_LENGTH             0

/* Rotation Speed Characteristic  */
#define HOMEKIT_ROTATION_SPEED_UUID                                "29"
#define HOMEKIT_ROTATION_SPEED_PERMISSIONS                         WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_ROTATION_SPEED_FORMAT                              WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_ROTATION_SPEED_MAXIMUM_VALUE                       100
#define HOMEKIT_ROTATION_SPEED_MINIMUM_VALUE                       0
#define HOMEKIT_ROTATION_SPEED_UNIT                                HOMEKIT_NO_UNIT_SET
#define HOMEKIT_ROTATION_SPEED_STEP_VALUE                          "1"
#define HOMEKIT_ROTATION_SPEED_MAXIMUM_LENGTH                      0
#define HOMEKIT_ROTATION_SPEED_MAXIMUM_DATA_LENGTH                 0

/* Target Relative Humidity Characteristic  */
#define HOMEKIT_TARGET_RELATIVE_HUMIDITY_UUID                      "34"
#define HOMEKIT_TARGET_RELATIVE_HUMIDITY_PERMISSIONS               WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_TARGET_RELATIVE_HUMIDITY_FORMAT                    WICED_HOMEKIT_FLOAT_FORMAT
#define HOMEKIT_TARGET_RELATIVE_HUMIDITY_MAXIMUM_VALUE             100
#define HOMEKIT_TARGET_RELATIVE_HUMIDITY_MINIMUM_VALUE             0
#define HOMEKIT_TARGET_RELATIVE_HUMIDITY_UNIT                      HOMEKIT_UNIT_PERCENTAGE
#define HOMEKIT_TARGET_RELATIVE_HUMIDITY_STEP_VALUE                "1"
#define HOMEKIT_TARGET_RELATIVE_HUMIDITY_MAXIMUM_LENGTH            0
#define HOMEKIT_TARGET_RELATIVE_HUMIDITY_MAXIMUM_DATA_LENGTH       0

/* Current Horizontal Tilt Angle Characteristic  */
#define HOMEKIT_CURRENT_H_ANGLE_UUID                               "6C"
#define HOMEKIT_CURRENT_H_ANGLE_PERMISSIONS                        WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CURRENT_H_ANGLE_FORMAT                             WICED_HOMEKIT_INT_FORMAT
#define HOMEKIT_CURRENT_H_ANGLE_MAXIMUM_VALUE                      90
#define HOMEKIT_CURRENT_H_ANGLE_MINIMUM_VALUE                      -90
#define HOMEKIT_CURRENT_H_ANGLE_UNIT                               HOMEKIT_UNIT_ARC_DEGREE
#define HOMEKIT_CURRENT_H_ANGLE_STEP_VALUE                         "1"
#define HOMEKIT_CURRENT_H_ANGLE_MAXIMUM_LENGTH                     0
#define HOMEKIT_CURRENT_H_ANGLE_MAXIMUM_DATA_LENGTH                0

/* Current Vertical Tilt Angle Characteristic  */
#define HOMEKIT_CURRENT_V_ANGLE_UUID                               "6E"
#define HOMEKIT_CURRENT_V_ANGLE_PERMISSIONS                        WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_CURRENT_V_ANGLE_FORMAT                             WICED_HOMEKIT_INT_FORMAT
#define HOMEKIT_CURRENT_V_ANGLE_MAXIMUM_VALUE                      90
#define HOMEKIT_CURRENT_V_ANGLE_MINIMUM_VALUE                      -90
#define HOMEKIT_CURRENT_V_ANGLE_UNIT                               HOMEKIT_UNIT_ARC_DEGREE
#define HOMEKIT_CURRENT_V_ANGLE_STEP_VALUE                         "1"
#define HOMEKIT_CURRENT_V_ANGLE_MAXIMUM_LENGTH                     0
#define HOMEKIT_CURRENT_V_ANGLE_MAXIMUM_DATA_LENGTH                0

/* Target Horizontal Tilt Angle Characteristic */
#define HOMEKIT_TARGET_H_ANGLE_UUID                                "7B"
#define HOMEKIT_TARGET_H_ANGLE_PERMISSIONS                         WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_TARGET_H_ANGLE_FORMAT                              WICED_HOMEKIT_INT_FORMAT
#define HOMEKIT_TARGET_H_ANGLE_MAXIMUM_VALUE                       90
#define HOMEKIT_TARGET_H_ANGLE_MINIMUM_VALUE                       -90
#define HOMEKIT_TARGET_H_ANGLE_UNIT                                HOMEKIT_UNIT_ARC_DEGREE
#define HOMEKIT_TARGET_H_ANGLE_STEP_VALUE                          "1"
#define HOMEKIT_TARGET_H_ANGLE_MAXIMUM_LENGTH                      0
#define HOMEKIT_TARGET_H_ANGLE_MAXIMUM_DATA_LENGTH                 0

/* Target Vertical Tilt Angle Characteristic  */
#define HOMEKIT_TARGET_V_ANGLE_UUID                                "7D"
#define HOMEKIT_TARGET_V_ANGLE_PERMISSIONS                         WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_TARGET_V_ANGLE_FORMAT                              WICED_HOMEKIT_INT_FORMAT
#define HOMEKIT_TARGET_V_ANGLE_MAXIMUM_VALUE                       90
#define HOMEKIT_TARGET_V_ANGLE_MINIMUM_VALUE                       -90
#define HOMEKIT_TARGET_V_ANGLE_UNIT                                HOMEKIT_UNIT_ARC_DEGREE
#define HOMEKIT_TARGET_V_ANGLE_STEP_VALUE                          "1"
#define HOMEKIT_TARGET_V_ANGLE_MAXIMUM_LENGTH                      0
#define HOMEKIT_TARGET_V_ANGLE_MAXIMUM_DATA_LENGTH                 0

/* Hold Position Characteristic */
#define HOMEKIT_HOLD_POSITION_UUID                                 "6F"
#define HOMEKIT_HOLD_POSITION_PERMISSIONS                          WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE
#define HOMEKIT_HOLD_POSITION_FORMAT                               WICED_HOMEKIT_BOOLEAN_FORMAT
#define HOMEKIT_HOLD_POSITION_MAXIMUM_VALUE                        0    /* Max value is also set as 0 to indicate there is no min max value for boolean data type */
#define HOMEKIT_HOLD_POSITION_MINIMUM_VALUE                        0
#define HOMEKIT_HOLD_POSITION_UNIT                                 HOMEKIT_NO_UNIT_SET
#define HOMEKIT_HOLD_POSITION_STEP_VALUE                           NULL
#define HOMEKIT_HOLD_POSITION_MAXIMUM_LENGTH                       0
#define HOMEKIT_HOLD_POSITION_MAXIMUM_DATA_LENGTH                  0

/* Firmware Characteristic */
#define HOMEKIT_FIRMWARE_UUID                                      "52"
#define HOMEKIT_FIRMWARE_PERMISSIONS                               WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_FIRMWARE_FORMAT                                    WICED_HOMEKIT_STRING_FORMAT
#define HOMEKIT_FIRMWARE_MAXIMUM_VALUE                             0
#define HOMEKIT_FIRMWARE_MINIMUM_VALUE                             0
#define HOMEKIT_FIRMWARE_UNIT                                      HOMEKIT_NO_UNIT_SET
#define HOMEKIT_FIRMWARE_STEP_VALUE                                NULL
#define HOMEKIT_FIRMWARE_MAXIMUM_LENGTH                            0
#define HOMEKIT_FIRMWARE_MAXIMUM_DATA_LENGTH                       0

/* Hardware Characteristic */
#define HOMEKIT_HARDWARE_UUID                                      "53"
#define HOMEKIT_HARDWARE_PERMISSIONS                               WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_HARDWARE_FORMAT                                    WICED_HOMEKIT_STRING_FORMAT
#define HOMEKIT_HARDWARE_MAXIMUM_VALUE                             0
#define HOMEKIT_HARDWARE_MINIMUM_VALUE                             0
#define HOMEKIT_HARDWARE_UNIT                                      HOMEKIT_NO_UNIT_SET
#define HOMEKIT_HARDWARE_STEP_VALUE                                NULL
#define HOMEKIT_HARDWARE_MAXIMUM_LENGTH                            0
#define HOMEKIT_HARDWARE_MAXIMUM_DATA_LENGTH                       0

/* Software Characteristic */
#define HOMEKIT_SOFTWARE_UUID                                      "54"
#define HOMEKIT_SOFTWARE_PERMISSIONS                               WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_SOFTWARE_FORMAT                                    WICED_HOMEKIT_STRING_FORMAT
#define HOMEKIT_SOFTWARE_MAXIMUM_VALUE                             0
#define HOMEKIT_SOFTWARE_MINIMUM_VALUE                             0
#define HOMEKIT_SOFTWARE_UNIT                                      HOMEKIT_NO_UNIT_SET
#define HOMEKIT_SOFTWARE_STEP_VALUE                                NULL
#define HOMEKIT_SOFTWARE_MAXIMUM_LENGTH                            0
#define HOMEKIT_SOFTWARE_MAXIMUM_DATA_LENGTH                       0

/* Custom Characteristic */
#define HOMEKIT_COLOR_UUID                                         "B97B8832-16C5-4316-B4CE-3AC40C77C884"  /* Generate UUID as defined by RFC4122 for custom service or characteristic  */
#define HOMEKIT_COLOR_PERMISSIONS                                  WICED_HOMEKIT_PERMISSIONS_PAIRED_READ | WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_EVENTS
#define HOMEKIT_COLOR_FORMAT                                       WICED_HOMEKIT_INT_FORMAT
#define HOMEKIT_COLOR_MAXIMUM_VALUE                                360
#define HOMEKIT_COLOR_MINIMUM_VALUE                                0
#define HOMEKIT_COLOR_UNIT                                         HOMEKIT_NO_UNIT_SET
#define HOMEKIT_COLOR_STEP_VALUE                                   "1"
#define HOMEKIT_COLOR_MAXIMUM_LENGTH                               0
#define HOMEKIT_COLOR_MAXIMUM_DATA_LENGTH                          0

/* Custom Characteristic */
#define HOMEKIT_POWER_UUID                                         "23F9E6CD-B45D-4C79-B972-75575E65FB8B"  /* Generate UUID as defined by RFC4122 for custom service or characteristic  */
#define HOMEKIT_POWER_PERMISSIONS                                  WICED_HOMEKIT_PERMISSIONS_PAIRED_READ
#define HOMEKIT_POWER_FORMAT                                       WICED_HOMEKIT_INT_FORMAT
#define HOMEKIT_POWER_MAXIMUM_VALUE                                100
#define HOMEKIT_POWER_MINIMUM_VALUE                                0
#define HOMEKIT_POWER_UNIT                                         HOMEKIT_UNIT_PERCENTAGE
#define HOMEKIT_POWER_STEP_VALUE                                   "1"
#define HOMEKIT_POWER_MAXIMUM_LENGTH                               0
#define HOMEKIT_POWER_MAXIMUM_DATA_LENGTH                          0

/* Custom Characteristic */
#define HOMEKIT_SYSTEM_UPGRADE_UUID                                "826ADAD0-B3E7-4D81-86D4-E99D3CA39B88"  /* Generate UUID as defined by RFC4122 for custom service or characteristic  */
#define HOMEKIT_SYSTEM_UPGRADE_PERMISSIONS                         WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE | WICED_HOMEKIT_PERMISSIONS_HIDDEN
#define HOMEKIT_SYSTEM_UPGRADE_FORMAT                              WICED_HOMEKIT_TLV8_FORMAT
#define HOMEKIT_SYSTEM_UPGRADE_MAXIMUM_VALUE                       0
#define HOMEKIT_SYSTEM_UPGRADE_MINIMUM_VALUE                       0
#define HOMEKIT_SYSTEM_UPGRADE_UNIT                                HOMEKIT_NO_UNIT_SET
#define HOMEKIT_SYSTEM_UPGRADE_STEP_VALUE                          NULL
#define HOMEKIT_SYSTEM_UPGRADE_MAXIMUM_LENGTH                      0
#define HOMEKIT_SYSTEM_UPGRADE_MAXIMUM_DATA_LENGTH                 0


#define NO_MAXIMUM_MINIMUM_VALUE_SET                0

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
 *  @defgroup mfi                                   Apple MFi Protocols
 *  @ingroup  ipcoms
 *
 *  @addtogroup mfi_homekit                         HomeKit
 *  @ingroup mfi
 *
 *  @addtogroup mfi_homekit_characteristics_init    Characteristic Initialization
 *  @ingroup mfi_homekit
 *
 *  Functions to initialize HomeKit Characteristic
 *
 *  @{
 */
/*****************************************************************************/

/**
 * @{
 *
 * Initializes the given HomeKit characteristic.
 *
 * @param   <CharacteristicName>_characteristic [in] : Pointer to homekit characteristic structure to be initialized.
 * @param   value [in]                               : Initial value of the characteristic.
 * @param   value_length [in]                        : Length of of the characteristic value.
 * @param   value_name [in]                          : Name for the characteristic which is a pointer to 'null' terminated string (or) passed as NULL.
 * @param   description [in]                         : Pointer to 'null' terminated string containing description for the characteristic (or) passed as NULL.
 * @param   instance_id [in]                         : Instance ID for the characteristic. The instance ID should be unique across services and characteristics for the given accessory, valid range is from 1 to 65535.
 *
 * @return None
 */
void wiced_homekit_initialise_on_characteristic                             ( wiced_homekit_characteristics_t* on_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_name_characteristic                           ( wiced_homekit_characteristics_t* name_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_saturation_characteristic                     ( wiced_homekit_characteristics_t* saturation_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_brightness_characteristic                     ( wiced_homekit_characteristics_t* brightness_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_hue_characteristic                            ( wiced_homekit_characteristics_t* hue_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_current_door_state_characteristic             ( wiced_homekit_characteristics_t* current_door_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_target_door_state_characteristic              ( wiced_homekit_characteristics_t* target_door_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_obstruction_detected_characteristic           ( wiced_homekit_characteristics_t* obstruction_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_lock_mechanism_current_state_characteristic   ( wiced_homekit_characteristics_t* lock_mechanism_current_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_lock_mechanism_target_state_characteristic    ( wiced_homekit_characteristics_t* lock_mechanism_target_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_logs_characteristic                           ( wiced_homekit_characteristics_t* logs_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_manufacturer_characteristic                   ( wiced_homekit_characteristics_t* manufacturer_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_model_characteristic                          ( wiced_homekit_characteristics_t* model_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_serial_number_characteristic                  ( wiced_homekit_characteristics_t* serial_number_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_firmwar_revision_characteristic               ( wiced_homekit_characteristics_t* characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_heating_cooling_current_characteristic        ( wiced_homekit_characteristics_t* heating_cooling_current_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_heating_cooling_target_characteristic         ( wiced_homekit_characteristics_t* heating_cooling_target_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_temperature_current_characteristic            ( wiced_homekit_characteristics_t* temperature_current_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_temperature_target_characteristic             ( wiced_homekit_characteristics_t* temperature_target_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_temperature_units_characteristic              ( wiced_homekit_characteristics_t* temperature_units_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_outlet_in_use_characteristic                  ( wiced_homekit_characteristics_t* outlet_in_use_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_version_characteristic                        ( wiced_homekit_characteristics_t* version_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_lock_management_control_point_characteristic  ( wiced_homekit_characteristics_t* lock_management_control_point_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_air_particulate_density_characteristic        ( wiced_homekit_characteristics_t* air_particulate_density_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_air_particulate_size_characteristic           ( wiced_homekit_characteristics_t* air_particulate_size_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_air_quality_characteristic                    ( wiced_homekit_characteristics_t* air_quality_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_status_active_characteristic                  ( wiced_homekit_characteristics_t* status_active_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_status_fault_characteristic                   ( wiced_homekit_characteristics_t* status_fault_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_status_tampered_characteristic                ( wiced_homekit_characteristics_t* status_tampered_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_status_low_battery_characteristic             ( wiced_homekit_characteristics_t* status_low_battery_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_status_jammed_characteristic                  ( wiced_homekit_characteristics_t* status_jammed_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_security_system_current_state_characteristic  ( wiced_homekit_characteristics_t* security_system_current_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_security_system_target_state_characteristic   ( wiced_homekit_characteristics_t* security_system_target_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_security_system_alarm_type_characteristic     ( wiced_homekit_characteristics_t* security_system_alarm_type_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_carbon_monoxide_detected_characteristic       ( wiced_homekit_characteristics_t* carbon_monoxide_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_carbon_monoxide_level_characteristic          ( wiced_homekit_characteristics_t* carbon_monoxide_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_carbon_monoxide_peak_level_characteristic     ( wiced_homekit_characteristics_t* carbon_monoxide_peak_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_contact_sensor_state_characteristic           ( wiced_homekit_characteristics_t* contact_sensor_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_current_position_characteristic               ( wiced_homekit_characteristics_t* current_position_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_target_position_characteristic                ( wiced_homekit_characteristics_t* target_position_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_position_state_characteristic                 ( wiced_homekit_characteristics_t* position_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_current_relative_humidity_characteristic      ( wiced_homekit_characteristics_t* current_relative_humidity_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_leak_detected_characteristic                  ( wiced_homekit_characteristics_t* leak_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_current_ambient_light_level_characteristic    ( wiced_homekit_characteristics_t* current_ambient_light_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_occupancy_detected_characteristic             ( wiced_homekit_characteristics_t* occupancy_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_smoke_detected_characteristic                 ( wiced_homekit_characteristics_t* smoke_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_programmable_switch_event_characteristic      ( wiced_homekit_characteristics_t* programmable_switch_event_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_programmable_switch_output_state_characteristic ( wiced_homekit_characteristics_t* programmable_switch_output_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_battery_level_characteristic                  ( wiced_homekit_characteristics_t* battery_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_charging_state_characteristic                 ( wiced_homekit_characteristics_t* charging_state_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_carbon_dioxide_detected_characteristic        ( wiced_homekit_characteristics_t* carbon_dioxide_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_carbon_dioxide_level_characteristic           ( wiced_homekit_characteristics_t* carbon_dioxide_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_carbon_dioxide_peak_level_characteristic      ( wiced_homekit_characteristics_t* carbon_dioxide_peak_level_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_administrator_only_access_characteristic      ( wiced_homekit_characteristics_t* administrator_only_access_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_audio_feedback_characteristic                 ( wiced_homekit_characteristics_t* audio_feedback_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_cooling_threshold_temperature_characteristic  ( wiced_homekit_characteristics_t* cooling_threshold_temperature_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_heating_threshold_temperature_characteristic  ( wiced_homekit_characteristics_t* heating_threshold_temperature_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_lock_last_known_action_characteristic         ( wiced_homekit_characteristics_t* lock_last_known_action_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_lock_auto_security_timeout_characteristic     ( wiced_homekit_characteristics_t* lock_auto_security_timeout_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_log_characteristic                            ( wiced_homekit_characteristics_t* log_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_motion_detected_characteristic                ( wiced_homekit_characteristics_t* motion_detected_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_rotation_direction_characteristic             ( wiced_homekit_characteristics_t* rotation_diretion_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_rotation_speed_characteristic                 ( wiced_homekit_characteristics_t* rotation_speed_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_target_relative_humidity_characteristic       ( wiced_homekit_characteristics_t* target_relative_humidity_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_current_horizontal_angle_characteristic       ( wiced_homekit_characteristics_t* current_horizontal_angle_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_current_vertical_angle_characteristic         ( wiced_homekit_characteristics_t* current_vertical_angle_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_target_horizontal_angle_characteristic        ( wiced_homekit_characteristics_t* target_horizontal_angle_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_target_vertical_angle_characteristic          ( wiced_homekit_characteristics_t* target_vertical_angle_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_hold_position_characteristic                  ( wiced_homekit_characteristics_t* hold_position_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_firmware_characteristic                       ( wiced_homekit_characteristics_t* firmware_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_hardware_characteristic                       ( wiced_homekit_characteristics_t* hardware_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_software_characteristic                       ( wiced_homekit_characteristics_t* software_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );

/* Initialize custom characteristic */
void wiced_homekit_initialise_color_characteristic          ( wiced_homekit_characteristics_t* color_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_power_characteristic          ( wiced_homekit_characteristics_t* power_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );
void wiced_homekit_initialise_system_upgrade_characteristic ( wiced_homekit_characteristics_t* system_upgrade_characteristic, char* value, uint8_t value_length, char* value_name, char* description, uint16_t instance_id );

/** @} */

/** Initializes identify HomeKit characteristic.
 *
 * @param   identify_characteristic [in] : Pointer to Identify homekit characteristic structure.
 * @param   identify_callback [in]       : Pointer to the call back function to be invoked when identify request is received from controller.
 *
 * @return None
 */
void wiced_homekit_initialise_identify_characteristic                       ( wiced_homekit_characteristics_t* identify_characteristic, wiced_homekit_identify_callback_t identify_callback, uint16_t instance_id );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
