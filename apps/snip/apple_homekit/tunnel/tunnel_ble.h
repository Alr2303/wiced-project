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

#include "tunnel.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/*
 * HAP Characteristic properties
 */
#define HAP_CHARACTERISTIC_PROPERTY_READ                    0x0001
#define HAP_CHARACTERISTIC_PROPERTY_WRITE                   0x0002
#define HAP_CHARACTERISTIC_PROPERTY_ADDITIONAL_AUTH_DATA    0x0004
#define HAP_CHARACTERISTIC_PROPERTY_TIMED_WRITE_PROCEDURE   0x0008
#define HAP_CHARACTERISTIC_PROPERTY_SECURE_READ             0x0010
#define HAP_CHARACTERISTIC_PROPERTY_SECURE_WRITE            0x0020
#define HAP_CHARACTERISTIC_PROPERTY_INVISIBLE_TO_USER       0x0040
#define HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_CONNECTED     0x0080
#define HAP_CHARACTERISTIC_PROPERTY_NOTIFY_IN_DISCONNECTED  0x0100

/*
 * HAP Characteristic presentation format types
 */
#define HAP_CHARACTERISTIC_FORMAT_BOOL                      0x01
#define HAP_CHARACTERISTIC_FORMAT_UINT8                     0x04
#define HAP_CHARACTERISTIC_FORMAT_UINT16                    0x06
#define HAP_CHARACTERISTIC_FORMAT_UINT32                    0x08
#define HAP_CHARACTERISTIC_FORMAT_UINT64                    0x0a
#define HAP_CHARACTERISTIC_FORMAT_INT32                     0x10
#define HAP_CHARACTERISTIC_FORMAT_FLOAT                     0x14
#define HAP_CHARACTERISTIC_FORMAT_STRING                    0x19
#define HAP_CHARACTERISTIC_FORMAT_TLV8                      0x1b
#define HAP_CHARACTERISTIC_FORMAT_DATA                      0x1b

/*
 * HAP Characteristic presentation format unit types
 */
#define HAP_CHARACTERISTIC_FORMAT_UNIT_CELSIUS              0x272f
#define HAP_CHARACTERISTIC_FORMAT_UNIT_ARCDEGREES           0x2763
#define HAP_CHARACTERISTIC_FORMAT_UNIT_PERCENTAGE           0x27ad
#define HAP_CHARACTERISTIC_FORMAT_UNIT_UNITLESS             0x2700
#define HAP_CHARACTERISTIC_FORMAT_UNIT_LUX                  0x2731
#define HAP_CHARACTERISTIC_FORMAT_UNIT_SECONDS              0x2703

#define HAP_PARAM_MAX_RANGE_LEN                 4

#define TUNNEL_BLE_ACCESSORY_MAX_SERVICES           8
#define TUNNEL_BLE_ACCESSORY_MAX_CHARACTERISTICS    10

#define TUNNEL_BLE_SCAN_DURATION                5
#define TUNNEL_BLE_NO_SCAN_DURATION             55

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/* BLE accessory advertisement manufacturer data */
#pragma pack(1)
typedef struct
{
    uint16_t        company_id;
    uint8_t         type;
    uint8_t         adv_interval_len;
    uint8_t         status_flags;
    uint8_t         device_id[TUNNEL_ACCESSORY_ID_LEN];
    uint16_t        category_id;
    uint16_t        global_state_num;
    uint8_t         config_num;
    uint8_t         version;
} tunnel_ble_adv_manu_data_t;
#pragma pack()

/* BLE accessory discovery data structure */
typedef struct
{
    uint16_t                        uuid;
    uint16_t                        inst_id;
    uint16_t                        handle;
    uint16_t                        ccc_handle;
    uint8_t                         ble_properties;

    uint16_t                        hap_properties;
    uint8_t                         format;
    uint16_t                        unit;

    wiced_bool_t                    optional_params;
    uint8_t                         minimum[HAP_PARAM_MAX_RANGE_LEN];
    uint8_t                         maximum[HAP_PARAM_MAX_RANGE_LEN];
    uint8_t                         step[HAP_PARAM_MAX_RANGE_LEN];

    /* Parameters used in discovery process */
    uint16_t                        inst_id_handle;
} ble_discovery_char_t;

typedef struct
{
    uint16_t                        uuid;
    uint16_t                        inst_id;
    uint8_t                         num_of_chars;
    ble_discovery_char_t            characteristics[TUNNEL_BLE_ACCESSORY_MAX_CHARACTERISTICS];

    /* Parameters used in discovery process */
    uint16_t                        s_handle;
    uint16_t                        e_handle;
    uint16_t                        inst_id_handle;
} ble_discovery_service_t;

typedef struct
{
    wiced_bt_device_address_t       bd_addr;
    uint8_t                         addr_type;
    char                            local_name[TUNNEL_ACCESSORY_MAX_NAME_LEN];
    tunnel_ble_adv_manu_data_t      adv_manu_data;
    uint8_t                         num_of_svcs;
    ble_discovery_service_t         services[TUNNEL_BLE_ACCESSORY_MAX_SERVICES];

    /* Parameters used in discovery process */
    uint8_t                         discovery_state;
    uint16_t                        discovery_conn_id;
    uint8_t                         discover_svc_idx;
    uint8_t                         discover_char_idx;
    wiced_bool_t                    hap_req_sent;
} ble_discovery_data_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t tunnel_ble_init();
wiced_result_t tunnel_ble_start();
wiced_result_t tunnel_ble_connect_accessory(tunneled_accessory_t* p_accessory);
wiced_result_t tunnel_ble_disconnect_accessory(tunneled_accessory_t* p_accessory);
wiced_result_t tunnel_ble_read_characteristic(tunneled_accessory_t* p_accessory,
        tunneled_characteristic_t* p_char);
wiced_result_t tunnel_ble_write_characteristic(tunneled_accessory_t* p_accessory,
        tunneled_characteristic_t* p_char, uint8_t* p_data, uint16_t data_len);
wiced_result_t tunnel_ble_configure_characteristic(tunneled_accessory_t* p_accessory,
        tunneled_characteristic_t* p_char, uint16_t client_config);
void tunnel_ble_timeout();

void tunnel_ble_discovery_callback(ble_discovery_data_t* p_data);
void tunnel_ble_connected_callback(tunneled_accessory_t* p_acc, wiced_bool_t connected);
void tunnel_ble_advertisement_update(tunneled_accessory_t* p_acc, uint16_t state_number);
void tunnel_ble_read_callback(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char,
        wiced_bt_gatt_status_t status, uint8_t* p_data, uint16_t data_len);
void tunnel_ble_write_callback(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char,
        wiced_bt_gatt_status_t status);
void tunnel_ble_indication_callback(tunneled_accessory_t* p_acc, tunneled_characteristic_t* p_char,
        wiced_bt_gatt_status_t status);

#ifdef __cplusplus
} /*extern "C" */
#endif
