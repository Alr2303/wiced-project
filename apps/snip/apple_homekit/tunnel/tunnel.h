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

#include <gki.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define UUID_SVC_ACCESSORY_INFORMATION          0x003e
#define UUID_SVC_FAN                            0x0040
#define UUID_SVC_GARAGE_DOOR_OPENER             0x0041
#define UUID_SVC_LIGHTBULB                      0x0043
#define UUID_SVC_LOCK_MANAGEMENT                0x0044
#define UUID_SVC_LOCK_MECHANISM                 0x0045
#define UUID_SVC_OUTLET                         0x0047
#define UUID_SVC_SWITCH                         0x0049
#define UUID_SVC_THERMOSTAT                     0x004a
#define UUID_SVC_PAIRING                        0x0055
#define UUID_SVC_TUNNELED_BLE_ACCESSORY         0x0056
#define UUID_SVC_PROTOCOL_INFO                  0x00a2

#define UUID_CHAR_ADMINISTRATOR_ONLY_ACCESS     0x0001
#define UUID_CHAR_AUDIO_FEEDBACK                0x0005
#define UUID_CHAR_BRIGHTNESS                    0x0008
#define UUID_CHAR_COOL_THRESHOLD_TEMPERATURE    0x000d
#define UUID_CHAR_CURRENT_DOOR_STATE            0x000e
#define UUID_CHAR_CURRENT_HEAT_COOL_STATE       0x000f
#define UUID_CHAR_CURRENT_RELATIVE_HUMIDITY     0x0010
#define UUID_CHAR_CURRENT_TEMPERATURE           0x0011
#define UUID_CHAR_HEAT_THRESHOLD_TEMPERATURE    0x0012
#define UUID_CHAR_HUE                           0x0013
#define UUID_CHAR_IDENTIFY                      0x0014
#define UUID_CHAR_LOCK_CONTROL_POINT            0x0019
#define UUID_CHAR_LOCK_AUTO_SECURITY_TIMEOUT    0x001a
#define UUID_CHAR_LOCK_LAST_KNOWN_ACTION        0x001c
#define UUID_CHAR_LOCK_CURRENT_STATE            0x001d
#define UUID_CHAR_LOCK_TARGET_STATE             0x001e
#define UUID_CHAR_LOGS                          0x001f
#define UUID_CHAR_MANUFACTURER                  0x0020
#define UUID_CHAR_MODEL                         0x0021
#define UUID_CHAR_MOTION_DETECTED               0x0022
#define UUID_CHAR_NAME                          0x0023
#define UUID_CHAR_OBSTRUCTION_DETECTED          0x0024
#define UUID_CHAR_ON                            0x0025
#define UUID_CHAR_OUTLET_IN_USE                 0x0026
#define UUID_CHAR_ROTATION_DIRECTION            0x0028
#define UUID_CHAR_ROTATION_SPEED                0x0029
#define UUID_CHAR_SATURATION                    0x002f
#define UUID_CHAR_SERIAL_NUMBER                 0x0030
#define UUID_CHAR_TARGET_DOOR_STATE             0x0032
#define UUID_CHAR_TARGET_HEAT_COOL_STATE        0x0033
#define UUID_CHAR_TARGET_RELATIVE_HUMIDITY      0x0034
#define UUID_CHAR_TARGET_TEMPERATURE            0x0035
#define UUID_CHAR_TEMPERATURE_DISPLAY_UNITS     0x0036
#define UUID_CHAR_VERSION                       0x0037
#define UUID_CHAR_PAIR_SETUP                    0x004c
#define UUID_CHAR_PAIR_VERIFY                   0x004e
#define UUID_CHAR_PAIRING_FEATURES              0x004f
#define UUID_CHAR_PAIRING_PAIRINGS              0x0050
#define UUID_CHAR_FIRMWARE_REVISION             0x0052
#define UUID_CHAR_HARDWARE_REVISION             0x0053
#define UUID_CHAR_SOFTWARE_REVISION             0x0054
#define UUID_CHAR_TUNNELED_ACCESSORY_IDENTIFIER         0x0057
#define UUID_CHAR_TUNNELED_ACCESSORY_STATE_NUMBER       0x0058
#define UUID_CHAR_TUNNELED_ACCESSORY_CONNECTED          0x0059
#define UUID_CHAR_TUNNELED_ACCESSORY_ADVERTISING        0x0060
#define UUID_CHAR_TUNNELED_ACCESSORY_CONNECTION_TIMEOUT 0x0061

#define TUNNEL_MAX_HAP_VALUE_LEN                512

#define TUNNELED_ACCESSORY_CHAR_NUM             6
#define TUNNEL_ACCESSORY_ID_LEN                 6
#define TUNNEL_ACCESSORY_MAX_NAME_LEN           16

#define TIMER_INTERVAL                          1    /* seconds */
#define TICKS_PER_SECOND                        1

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef union
{
    uint8_t                         value_u8;
    uint16_t                        value_u16;
    uint32_t                        value_u32;
    int32_t                         value_i32;
    float                           value_f;
} hap_characteristic_value_t;

typedef struct tunneled_characteristic
{
    /* Note: the members of this struct are arranged to use least memory
     * space possible */

    uint16_t                        uuid;
    uint16_t                        inst_id;
    uint16_t                        handle;
    uint16_t                        ccc_handle;
    uint8_t                         ble_properties;

    uint8_t                         format;
    uint16_t                        hap_properties;

    struct tunneled_characteristic* next;

    uint16_t                        unit;
    uint8_t                         full_size; /* 0 if following members not present, 1 otherwise */

    hap_characteristic_value_t      minimum;
    hap_characteristic_value_t      maximum;
    hap_characteristic_value_t      step;

#ifdef WICED_HOMEKIT_TUNNEL_USER_DESCRIPTION
    char*                           description;
#endif

} tunneled_characteristic_t;

#define FULL_CHARACTERISTIC_SIZE    (sizeof(tunneled_characteristic_t))
#ifdef WICED_HOMEKIT_TUNNEL_USER_DESCRIPTION
#define THIN_CHARACTERISTIC_SIZE    (sizeof(tunneled_characteristic_t) - sizeof(hap_characteristic_value_t) * 3 - sizeof(char*))
#else
#define THIN_CHARACTERISTIC_SIZE    (sizeof(tunneled_characteristic_t) - sizeof(hap_characteristic_value_t) * 3)
#endif

typedef struct tunneled_service
{
    uint16_t                        uuid;
    uint16_t                        inst_id;

    tunneled_characteristic_t*      char_list_front;
    tunneled_characteristic_t*      char_list_rear;

    struct tunneled_service*        next;

} tunneled_service_t;

typedef struct tunneled_accessory
{
    wiced_bt_device_address_t       bd_addr;
    uint8_t                         addr_type;
    wiced_homekit_accessories_t     ip_acc;
    uint8_t                         state;
    uint16_t                        conn_id;

    BUFFER_Q                        req_queue;

    int                             connection_timer_counter;
    int                             advertisement_timer_counter;

    uint16_t                        max_ble_inst_id;
    char                            tunneled_acc_name[TUNNEL_ACCESSORY_MAX_NAME_LEN];
    uint8_t                         tunneled_acc_identifier[TUNNEL_ACCESSORY_ID_LEN];
    uint16_t                        tunneled_acc_state_number;
    wiced_bool_t                    tunneled_acc_connected;
    wiced_bool_t                    tunneled_acc_advertising;
    int                             tunneled_acc_connection_timeout;

    tunneled_service_t*             svc_list_front;
    tunneled_service_t*             svc_list_rear;

    struct tunneled_accessory*      next;

} tunneled_accessory_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

tunneled_accessory_t* tunnel_get_accessory_by_aid(uint16_t aid);
tunneled_accessory_t* tunnel_get_accessory_by_conn_id(uint16_t conn_id);
tunneled_accessory_t* tunnel_get_accessory_by_bdaddr(uint8_t* bd_addr);
tunneled_characteristic_t* tunnel_get_characteristic_by_iid(tunneled_accessory_t* p_acc, uint16_t iid);
tunneled_characteristic_t* tunnel_get_characteristic_by_ble_handle(tunneled_accessory_t* p_acc,
        uint16_t handle);
tunneled_characteristic_t* tunnel_get_service_characteristic_by_uuid(tunneled_accessory_t* p_acc,
        uint16_t svc_uuid, uint16_t char_uuid);

void tunnel_remove_accessory(tunneled_accessory_t* p_accessory);

wiced_result_t tunnel_get_null_value(uint8_t format, char* value_string, uint32_t string_length);
wiced_result_t tunnel_get_tunneled_accessory_value(tunneled_accessory_t* p_acc,
        tunneled_characteristic_t* p_char, char* value_string, uint32_t string_length);

#ifdef __cplusplus
} /*extern "C" */
#endif
