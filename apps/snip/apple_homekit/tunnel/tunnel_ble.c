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
 * HomeKit Tunnel Bluetooth LE Accessory utility functions
 *
 */

#include "wiced.h"
#include "apple_homekit.h"
#include "apple_homekit_characteristics.h"
#include "apple_homekit_services.h"
#include "apple_homekit_developer.h"

#include "bt_target.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_l2c.h"
#include "wiced_bt_uuid.h"
#include "wiced_bt_cfg.h"

#include "tunnel_ble.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define HAP_GENERIC_UUID        0x91,0x52,0x76,0xbb,0x26,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00
#define HAP_SVC_INST_ID_UUID    0xd1,0xa0,0x83,0x50,0x00,0xaa,0xd3,0x87,0x17,0x48,0x59,0xa7,0x5d,0xe9,0x04,0xe6
#define HAP_CHAR_INST_ID_UUID   0x9a,0x93,0x96,0xd7,0xbd,0x6a,0xd9,0xb5,0x16,0x46,0xd2,0x81,0xfe,0xf0,0x46,0xdc

/******************************************************
 *                    Constants
 ******************************************************/

/*
 * HAP-BLE PDU Control field
 */
#define HAP_CONTROL_FIELD_REQUEST                           0x00
#define HAP_CONTROL_FIELD_RESPONSE                          0x02
#define HAP_CONTROL_FIELD_FRAGMENT                          0x80

/*
 * HAP Opcode
 */
#define HAP_CHARACTERISTIC_SIGNATURE_READ                   0x01
#define HAP_CHARACTERISTIC_WRITE                            0x02
#define HAP_CHARACTERISTIC_READ                             0x03
#define HAP_CHARACTERISTIC_TIMED_WRITE                      0x04
#define HAP_CHARACTERISTIC_EXECUTE_WRITE                    0x05

/*
 * HAP-BLE PDU additional parameter types
 */
#define HAP_PARAM_VALUE                                     0x01
#define HAP_PARAM_ADDITIONAL_AUTHORIZATION_DATA             0x02
#define HAP_PARAM_ORIGIN                                    0x03
#define HAP_PARAM_CHARACTERISTIC_TYPE                       0x04
#define HAP_PARAM_CHARACTERISTIC_INSTANCE_ID                0x05
#define HAP_PARAM_SERVICE_TYPE                              0x06
#define HAP_PARAM_SERVICE_INSTANCE_ID                       0x07
#define HAP_PARAM_TTL                                       0x08
#define HAP_PARAM_RETURN_RESPONSE                           0x09
#define HAP_PARAM_HAP_CHARACTERISTIC_PROPERTIES_DESCRIPTOR  0x0a
#define HAP_PARAM_GATT_USER_DESCRIPTION_DESCRIPTOR          0x0b
#define HAP_PARAM_GATT_PRESENTATION_FORMAT_DESCRIPTOR       0x0c
#define HAP_PARAM_GATT_VALID_RANGE                          0x0d
#define HAP_PARAM_HAP_STEP_VALUE_DESCRIPTOR                 0x0e

/*
 * HAP status code
 */
#define HAP_STATUS_SUCCESS                                  0x00
#define HAP_STATUS_UNSUPPORTED_PDU                          0x01
#define HAP_STATUS_MAX_PROCEDURES                           0x02
#define HAP_STATUS_INSUFFICIENT_AUTHORIZATION               0x03
#define HAP_STATUS_INVALID_INSTANCE_ID                      0x04
#define HAP_STATUS_INSUFFICIENT_AUTHENTICATION              0x05
#define HAP_STATUS_INVALID_REQUEST                          0x06

#define HAP_PDU_REQUEST_HEADER_SIZE         5
#define HAP_PDU_RESPONSE_HEADER_SIZE        3
#define HAP_PDU_FRAGMENT_HEADER_SIZE        2
#define HAP_PDU_BODY_LEN_SIZE               2
#define HAP_PDU_REQUEST_BODY_VALUE_OFFSET   (HAP_PDU_REQUEST_HEADER_SIZE + HAP_PDU_BODY_LEN_SIZE)
#define HAP_PDU_RESPONSE_BODY_VALUE_OFFSET  (HAP_PDU_RESPONSE_HEADER_SIZE + HAP_PDU_BODY_LEN_SIZE)

#define GATT_MTU_SIZE                       517

/******************************************************
 *                   Enumerations
 ******************************************************/

/* BLE discovery states */
enum
{
    BLE_DISC_STATE_IDLE,
    BLE_DISC_STATE_CONNECT,
    BLE_DISC_STATE_READ_NAME,
    BLE_DISC_STATE_DISCOVER_SERVICES,
    BLE_DISC_STATE_DISCOVER_CHARACTERISTICS,
    BLE_DISC_STATE_DISCOVER_DESCRIPTORS,
    BLE_DISC_STATE_READ_SVC_INST_ID,
    BLE_DISC_STATE_READ_CHAR_INST_ID,
    BLE_DISC_STATE_READ_SIGNATURE
};

/* Tunnel BLE accessory states */
enum
{
    TUNNEL_BLE_STATE_IDLE,
    TUNNEL_BLE_STATE_CONNECTING,
    TUNNEL_BLE_STATE_CONNECTED,
    TUNNEL_BLE_STATE_PENDING,
};

/* BLE request type */
enum
{
    BLE_REQUEST_READ,
    BLE_REQUEST_WRITE,
    BLE_REQUEST_CONFIG
};

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/* BLE control block */
typedef struct
{
    wiced_bool_t                    bt_stack_up;
    wiced_bt_ble_scan_type_t        scan_state;
    uint32_t                        conn_timer_count;
    uint32_t                        scan_timer_count;

    ble_discovery_data_t*           p_discovery_data;
} tunnel_ble_cb_t;

#pragma pack(1)
typedef struct
{
    uint8_t         control;        // Control Field (1 Byte)
    uint8_t         opcode;         // HAP Opcode (1 Byte)
    uint8_t         tid;            // Transaction Identifier (1 Byte)
    uint8_t         cid[2];         // Characteristic Instance Identifier (2 Bytes)
    uint8_t         body_len[2];    // PDU Body Length (2 Bytes)
    uint8_t         body_value[1];  // Additional Params and Values (1-n Bytes)
} tunnel_ble_hap_request_t;

typedef struct
{
    uint8_t         control;        // Control Field (1 Byte)
    uint8_t         tid;            // Transaction Identifier (1 Byte)
    uint8_t         status;         // Status (1Byte)
    uint8_t         body_len[2];    // PDU Body Length (2 Bytes)
    uint8_t         body_value[1];  // Additional Params and Values (1-n Bytes)
} tunnel_ble_hap_response_t;

typedef struct
{
    uint8_t         type;
    uint8_t         length;
    uint8_t         value[1];
} tunnel_ble_hap_pdu_tlv8_t;
#pragma pack()

typedef struct
{
    uint8_t*                        p_data;
    uint16_t                        data_len;
} tunnel_ble_req_write_t;

typedef struct
{
    uint16_t                        client_config;
} tunnel_ble_req_config_t;

typedef struct
{
    uint8_t                         req_type;
    tunneled_characteristic_t*      p_char;
    union
    {
        tunnel_ble_req_write_t      write;
        tunnel_ble_req_config_t     config;
    } u;
} tunnel_ble_request_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_bt_dev_status_t tunnel_bt_management_callback(wiced_bt_management_evt_t event,
        wiced_bt_management_evt_data_t *p_event_data);
static wiced_bt_gatt_status_t tunnel_bt_application_init(void);
static wiced_bt_gatt_status_t tunnel_bt_gatt_callback( wiced_bt_gatt_evt_t event,
        wiced_bt_gatt_event_data_t *p_data);
static void tunnel_ble_scan_result_cback(wiced_bt_ble_scan_results_t *p_scan_result,
        uint8_t *p_adv_data);
static wiced_bt_gatt_status_t tunnel_bt_gatt_conn_st(wiced_bt_gatt_connection_status_t *p_data);
static wiced_bt_gatt_status_t tunnel_bt_gatt_op_cmpl(wiced_bt_gatt_operation_complete_t *p_data);
static wiced_bt_gatt_status_t tunnel_bt_gatt_disc_result(wiced_bt_gatt_discovery_result_t *p_data);
static wiced_bt_gatt_status_t tunnel_bt_gatt_disc_cmpl(wiced_bt_gatt_discovery_complete_t *p_data);
static void tunnel_ble_connection_up(wiced_bt_gatt_connection_status_t *p_data);
static void tunnel_ble_connection_down(wiced_bt_gatt_connection_status_t *p_data);
static void tunnel_ble_process_read_rsp(wiced_bt_gatt_operation_complete_t *p_data);
static void tunnel_ble_process_write_rsp(wiced_bt_gatt_operation_complete_t *p_data);
static void tunnel_ble_process_indication(wiced_bt_gatt_operation_complete_t *p_data);
static void tunnel_ble_discover_characteristics(uint16_t conn_id);
static void tunnel_ble_discover_descriptors(uint16_t conn_id);
static void tunnel_ble_read_service_instance_id(uint16_t conn_id);
static void tunnel_ble_read_characteristic_instance_id(uint16_t conn_id);
static void tunnel_ble_read_characteristic_signature(uint16_t conn_id);
static void tunnel_ble_discovery_finished(wiced_bt_gatt_status_t status);
static void tunnel_ble_process_service(wiced_bt_gatt_group_value_t *p_data);
static void tunnel_ble_process_characteristic(wiced_bt_gatt_char_declaration_t *p_data);
static void tunnel_ble_process_descriptor(wiced_bt_gatt_char_descr_info_t *p_data);
static void tunnel_ble_process_characteristic_signature(wiced_bt_gatt_data_t* p_data,
        ble_discovery_char_t* p_char);

static wiced_bool_t tunnel_gatt_connect(wiced_bt_device_address_t bd_addr, uint8_t addr_type);
static wiced_bool_t tunnel_gatt_cancel_connect(wiced_bt_device_address_t bd_addr);
static void tunnel_gatt_send_discover(uint16_t conn_id, wiced_bt_gatt_discovery_type_t type,
        uint16_t uuid, uint16_t s_handle, uint16_t e_handle);
static void tunnel_gatt_send_read_by_handle(uint16_t conn_id, uint16_t handle);
static void tunnel_gatt_send_read_by_type(uint16_t conn_id, uint16_t s_handle, uint16_t e_handle,
        uint16_t uuid);
static void tunnel_gatt_send_write(uint16_t conn_id, wiced_bt_gatt_write_type_t type,
        uint16_t handle, uint8_t *p_data, uint16_t data_len);
static void tunnel_ble_send_hap_signature_read(uint16_t conn_id, uint16_t handle, uint16_t iid);
static wiced_bool_t tunnel_ble_send_queued_request(tunneled_accessory_t* p_acc);
static wiced_bool_t tunnel_ble_is_hap_uuid(wiced_bt_uuid_t uuid);
static wiced_bool_t tunnel_ble_is_service_inst_id(wiced_bt_uuid_t uuid);
static wiced_bool_t tunnel_ble_is_characteristic_inst_id(wiced_bt_uuid_t uuid);
static wiced_bool_t tunnel_ble_is_client_characteristic_configuration(wiced_bt_uuid_t uuid);
static void tunnel_debug_dump_discovery();

#ifdef HCI_LOG_IP
typedef void (*hci_log_cback_t)(int trace_type, BT_HDR *p_msg);
extern void btu_register_hci_log_cback(hci_log_cback_t hci_log_cback);

static void hci_log_handler(int trace_type, BT_HDR *p_msg);
#endif

/******************************************************
 *               Variable Definitions
 ******************************************************/

/* Bluetooth stack and buffer pool configuration tables */
extern const wiced_bt_cfg_settings_t wiced_bt_cfg_settings;
extern const wiced_bt_cfg_buf_pool_t wiced_bt_cfg_buf_pools[];

tunnel_ble_cb_t ble_cb;

#ifdef HCI_LOG_IP
#define LOG_SPY_UDP_PORT            0x9426
#define LOG_MAX_DATA_LENGTH         128

static wiced_udp_socket_t           log_socket;
static uint32_t                     log_ip_addr = HCI_LOG_IP;
#endif

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t tunnel_ble_init()
{
    memset(&ble_cb, 0, sizeof(tunnel_ble_cb_t));

    /* Initialize Bluetooth controller and host stack */
    wiced_bt_stack_init(tunnel_bt_management_callback, &wiced_bt_cfg_settings,
            wiced_bt_cfg_buf_pools);

#ifdef HCI_LOG_IP
    /* Register callbacks to log HCI commands and events */
    btu_register_hci_log_cback(hci_log_handler);

    /* Create UDP socket for logging */
    if (wiced_udp_create_socket(&log_socket, LOG_SPY_UDP_PORT, WICED_STA_INTERFACE) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("UDP socket creation failed\r\n"));
    }
#endif

    while (!ble_cb.bt_stack_up)
    {
        wiced_rtos_delay_milliseconds(250);
    }

    return WICED_SUCCESS;
}

wiced_result_t tunnel_ble_start()
{
    ble_cb.scan_timer_count = TUNNEL_BLE_SCAN_DURATION * TICKS_PER_SECOND;

    return WICED_SUCCESS;
}

wiced_result_t tunnel_ble_start_scan()
{
    ble_cb.scan_state = BTM_BLE_SCAN_TYPE_HIGH_DUTY;
    wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_TRUE, tunnel_ble_scan_result_cback);
    ble_cb.scan_timer_count = TUNNEL_BLE_SCAN_DURATION * TICKS_PER_SECOND;

    return WICED_SUCCESS;
}

wiced_result_t tunnel_ble_stop_scan()
{
    ble_cb.scan_state = BTM_BLE_SCAN_TYPE_NONE;
    wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_NONE, WICED_TRUE, tunnel_ble_scan_result_cback);
    ble_cb.scan_timer_count = TUNNEL_BLE_NO_SCAN_DURATION * TICKS_PER_SECOND;

    return WICED_SUCCESS;
}

wiced_result_t tunnel_ble_connect_accessory(tunneled_accessory_t* p_accessory)
{
    WPRINT_APP_DEBUG(("tunnel_ble_connect_accessory state:%d\n", p_accessory->state));
    if (p_accessory->state == TUNNEL_BLE_STATE_IDLE)
    {
        tunnel_gatt_connect(p_accessory->bd_addr, p_accessory->addr_type);
    }

    return WICED_SUCCESS;
}

wiced_result_t tunnel_ble_disconnect_accessory(tunneled_accessory_t* p_accessory)
{
    WPRINT_APP_DEBUG(("tunnel_ble_disconnect_accessory state:%d conn_id:%d\n", p_accessory->state, p_accessory->conn_id));
    if (p_accessory->state == TUNNEL_BLE_STATE_IDLE)
        return WICED_NOT_CONNECTED;

    wiced_bt_gatt_disconnect(p_accessory->conn_id);
    return WICED_SUCCESS;
}

wiced_result_t tunnel_ble_read_characteristic(tunneled_accessory_t* p_accessory,
        tunneled_characteristic_t* p_char)
{
    WPRINT_APP_DEBUG(("Read characteristic state:%d conn_id:%d\n", p_accessory->state, p_accessory->conn_id));
    if (p_accessory->state == TUNNEL_BLE_STATE_CONNECTED)
    {
        tunnel_gatt_send_read_by_handle(p_accessory->conn_id, p_char->handle);
    }
    else
    {
        tunnel_ble_request_t* p_req =
                (tunnel_ble_request_t*)GKI_getbuf(sizeof(tunnel_ble_request_t));
        if (!p_req)
            return WICED_OUT_OF_HEAP_SPACE;

        if (p_accessory->state == TUNNEL_BLE_STATE_IDLE)
        {
            tunnel_gatt_connect(p_accessory->bd_addr, p_accessory->addr_type);
        }

        p_req->req_type = BLE_REQUEST_READ;
        p_req->p_char = p_char;
        GKI_enqueue(&p_accessory->req_queue, p_req);
    }

    p_accessory->state = TUNNEL_BLE_STATE_PENDING;
    while (p_accessory->state == TUNNEL_BLE_STATE_PENDING)
    {
        wiced_rtos_delay_milliseconds(250);
    }

    return WICED_SUCCESS;
}

wiced_result_t tunnel_ble_write_characteristic(tunneled_accessory_t* p_accessory,
        tunneled_characteristic_t* p_char, uint8_t* p_data, uint16_t data_len)
{
    WPRINT_APP_DEBUG(("Write characteristic state:%d conn_id:%d\n", p_accessory->state, p_accessory->conn_id));
    if (p_accessory->state == TUNNEL_BLE_STATE_CONNECTED)
    {
        tunnel_gatt_send_write(p_accessory->conn_id, GATT_WRITE, p_char->handle, p_data, data_len);
    }
    else
    {
        tunnel_ble_request_t* p_req = (tunnel_ble_request_t*)GKI_getbuf(
                sizeof(tunnel_ble_request_t) + data_len);
        if (!p_req)
            return WICED_OUT_OF_HEAP_SPACE;

        if (p_accessory->state == TUNNEL_BLE_STATE_IDLE)
        {
            tunnel_gatt_connect(p_accessory->bd_addr, p_accessory->addr_type);
        }

        p_req->req_type = BLE_REQUEST_WRITE;
        p_req->p_char = p_char;
        p_req->u.write.p_data = (uint8_t*)(p_req + 1);
        memcpy(p_req->u.write.p_data, p_data, data_len);
        p_req->u.write.data_len = data_len;
        GKI_enqueue(&p_accessory->req_queue, p_req);
    }

    p_accessory->state = TUNNEL_BLE_STATE_PENDING;
    while (p_accessory->state == TUNNEL_BLE_STATE_PENDING)
    {
        wiced_rtos_delay_milliseconds(250);
    }

    return WICED_SUCCESS;
}

wiced_result_t tunnel_ble_configure_characteristic(tunneled_accessory_t* p_accessory,
        tunneled_characteristic_t* p_char, uint16_t client_config)
{
    WPRINT_APP_DEBUG(("Configure characteristic state:%d conn_id:%d\n", p_accessory->state, p_accessory->conn_id));
    if (p_accessory->state == TUNNEL_BLE_STATE_CONNECTED)
    {
        tunnel_gatt_send_write(p_accessory->conn_id, GATT_WRITE, p_char->ccc_handle,
                (uint8_t*)&client_config, sizeof(uint16_t));
    }
    else
    {
        tunnel_ble_request_t* p_req = (tunnel_ble_request_t*)GKI_getbuf(
                sizeof(tunnel_ble_request_t) + sizeof(uint16_t));
        if (!p_req)
            return WICED_OUT_OF_HEAP_SPACE;

        if (p_accessory->state == TUNNEL_BLE_STATE_IDLE)
        {
            tunnel_gatt_connect(p_accessory->bd_addr, p_accessory->addr_type);
        }

        p_req->req_type = BLE_REQUEST_CONFIG;
        p_req->p_char = p_char;
        p_req->u.config.client_config = client_config;
        GKI_enqueue(&p_accessory->req_queue, p_req);
    }

    p_accessory->state = TUNNEL_BLE_STATE_PENDING;
    while (p_accessory->state == TUNNEL_BLE_STATE_PENDING)
    {
        wiced_rtos_delay_milliseconds(250);
    }

    return WICED_SUCCESS;
}

void tunnel_ble_timeout()
{
    if(!ble_cb.bt_stack_up)
        return;

    if (ble_cb.conn_timer_count && --ble_cb.conn_timer_count == 0)
    {
        if (ble_cb.p_discovery_data)
        {
            if (ble_cb.p_discovery_data->discovery_state == BLE_DISC_STATE_CONNECT)
            {
                tunnel_gatt_cancel_connect(ble_cb.p_discovery_data->bd_addr);
                tunnel_ble_discovery_finished(WICED_BT_GATT_ERROR);
                tunnel_ble_start_scan();
            }
        }
    }

    if (ble_cb.scan_timer_count && --ble_cb.scan_timer_count == 0)
    {
        if (ble_cb.scan_state == BTM_BLE_SCAN_TYPE_NONE)
        {
            tunnel_ble_start_scan();
        }
        else
        {
            tunnel_ble_stop_scan();
        }
    }
}

/******************************************************
 *               Static Functions
 ******************************************************/

wiced_bt_dev_status_t tunnel_bt_management_callback(wiced_bt_management_evt_t event,
        wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t result = WICED_BT_SUCCESS;

    WPRINT_APP_DEBUG(("tunnel_bt_management_callback: 0x%02x\n", event));

    switch (event)
    {
    /* Bluetooth stack enabled */
    case BTM_ENABLED_EVT:
        WPRINT_APP_INFO(("Bluetooth stack enabled status=%d\n", p_event_data->enabled.status));
        if (p_event_data->enabled.status == WICED_BT_SUCCESS)
        {
            tunnel_bt_application_init();
            ble_cb.bt_stack_up = WICED_TRUE;
        }
        break;

    case BTM_DISABLED_EVT:
        WPRINT_APP_INFO(("Bluetooth stack disabled\n"));
        ble_cb.bt_stack_up = WICED_FALSE;
        break;

    case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
        p_event_data->pairing_io_capabilities_ble_request.local_io_cap  = BTM_IO_CAPABILITIES_NONE;
        p_event_data->pairing_io_capabilities_ble_request.oob_data      = BTM_OOB_NONE;
        p_event_data->pairing_io_capabilities_ble_request.auth_req      = BTM_LE_AUTH_REQ_BOND | BTM_LE_AUTH_REQ_MITM;
        p_event_data->pairing_io_capabilities_ble_request.max_key_size  = 0x10;
        p_event_data->pairing_io_capabilities_ble_request.init_keys     = BTM_LE_KEY_PENC | BTM_LE_KEY_PID;
        p_event_data->pairing_io_capabilities_ble_request.resp_keys     = BTM_LE_KEY_PENC | BTM_LE_KEY_PID;
        break;

    case BTM_PAIRING_COMPLETE_EVT:
        break;

    case BTM_SECURITY_REQUEST_EVT:
        wiced_bt_ble_security_grant( p_event_data->security_request.bd_addr, WICED_BT_SUCCESS );
        break;

    case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
        break;

    case BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
        result = WICED_BT_ERROR;
        break;

    case BTM_BLE_SCAN_STATE_CHANGED_EVT:
        WPRINT_APP_DEBUG(("Bluetooth scan state=%d\n", p_event_data->ble_scan_state_changed));
//        ble_cb.scan_state = p_event_data->ble_scan_state_changed;
        break;

    case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
        break;

    default:
        break;
    }

    return result;
}

/*
 * This function is executed in the BTM_ENABLED_EVT management callback.
 */
wiced_bt_gatt_status_t tunnel_bt_application_init(void)
{
    WPRINT_APP_DEBUG(("tunnel_bt_application_init\n"));

    /* Register with stack to receive GATT callback */
    wiced_bt_gatt_register(tunnel_bt_gatt_callback);

    return WICED_BT_GATT_SUCCESS;
}

/*
 * Callback for various GATT events.  As this application performs only as a GATT server, some of
 * the events are omitted.
 */
static wiced_bt_gatt_status_t tunnel_bt_gatt_callback( wiced_bt_gatt_evt_t event,
        wiced_bt_gatt_event_data_t *p_data)
{
    wiced_bt_gatt_status_t result = WICED_BT_GATT_INVALID_PDU;

    WPRINT_APP_DEBUG(("tunnel_bt_gatt_callback: 0x%02x\n", event));

    switch(event)
    {
    case GATT_CONNECTION_STATUS_EVT:
        result = tunnel_bt_gatt_conn_st(&p_data->connection_status);
        break;

    case GATT_OPERATION_CPLT_EVT:
        result = tunnel_bt_gatt_op_cmpl(&p_data->operation_complete);
        break;

    case GATT_DISCOVERY_RESULT_EVT:
        result = tunnel_bt_gatt_disc_result(&p_data->discovery_result);
        break;

    case GATT_DISCOVERY_CPLT_EVT:
        result = tunnel_bt_gatt_disc_cmpl(&p_data->discovery_complete);
        break;

    default:
        break;
    }

    return result;
}

wiced_result_t tunnel_ble_start_discovery(wiced_bt_device_address_t bd_addr, uint8_t addr_type,
        char* local_name, tunnel_ble_adv_manu_data_t* p_manu_data)
{
    wiced_result_t result = WICED_BT_ERROR;

    if (ble_cb.p_discovery_data)
        return WICED_BT_BUSY;

    ble_cb.p_discovery_data = (ble_discovery_data_t*)calloc(1, sizeof(ble_discovery_data_t));
    if (!ble_cb.p_discovery_data)
        return WICED_OUT_OF_HEAP_SPACE;

    WPRINT_APP_DEBUG(("Start discovery on %s(%02x:%02x:%02x:%02x:%02x:%02x)\n", local_name,
            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]));
    if (tunnel_gatt_connect(bd_addr, addr_type))
    {
        memcpy(ble_cb.p_discovery_data->bd_addr, bd_addr, BD_ADDR_LEN);
        ble_cb.p_discovery_data->addr_type = addr_type;
        strncpy(ble_cb.p_discovery_data->local_name, local_name,
                TUNNEL_ACCESSORY_MAX_NAME_LEN - 1);
        memcpy(&ble_cb.p_discovery_data->adv_manu_data, p_manu_data,
                sizeof(tunnel_ble_adv_manu_data_t));
        ble_cb.p_discovery_data->discovery_state = BLE_DISC_STATE_CONNECT;
        ble_cb.conn_timer_count = 5 * TICKS_PER_SECOND;
        result = WICED_BT_SUCCESS;
    }
    else
    {
        free(ble_cb.p_discovery_data);
        ble_cb.p_discovery_data = NULL;
    }

    return result;
}

/*
 * Process advertisement packet received
 */
void tunnel_ble_scan_result_cback(wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data)
{
    tunnel_ble_adv_manu_data_t* p_manu_data;
    uint8_t* p_data;
    uint8_t length;

    if (p_scan_result)
    {
        p_manu_data = (tunnel_ble_adv_manu_data_t*)wiced_bt_ble_check_advertising_data(
                p_adv_data, BTM_BLE_ADVERT_TYPE_MANUFACTURER, &length);
        if (p_manu_data &&
            length >= sizeof(tunnel_ble_adv_manu_data_t) &&
            p_manu_data->company_id == 0x004c &&
            p_manu_data->type == 6)
        {
            tunneled_accessory_t* p_acc = tunnel_get_accessory_by_bdaddr(
                    p_scan_result->remote_bd_addr);
            if (p_acc)
            {
                if (p_manu_data->status_flags == 0)
                {
                    tunnel_ble_advertisement_update(p_acc, p_manu_data->global_state_num);
                }
                else
                {
                    WPRINT_APP_DEBUG(("Remove unpaired BLE accessory\n"));
                    tunnel_remove_accessory(p_acc);
                }
            }
            else if (p_manu_data->status_flags == 0)
            {
                char local_name[16];

                /* Get local name */
                p_data = wiced_bt_ble_check_advertising_data(p_adv_data,
                        BTM_BLE_ADVERT_TYPE_NAME_COMPLETE, &length);
                if (!p_data)
                {
                    p_data = wiced_bt_ble_check_advertising_data(p_adv_data,
                            BTM_BLE_ADVERT_TYPE_NAME_SHORT, &length);
                }
                if (p_data)
                {
                    memcpy(local_name, p_data, length);
                    local_name[length] = 0;
                }

                /* Start discovery */
                tunnel_ble_stop_scan();
                tunnel_ble_start_discovery(p_scan_result->remote_bd_addr,
                        p_scan_result->ble_addr_type, local_name, p_manu_data);
            }
        }
    }
}

wiced_bt_gatt_status_t tunnel_bt_gatt_conn_st(wiced_bt_gatt_connection_status_t *p_data)
{
    uint8_t * bd_addr = p_data->bd_addr;
    WPRINT_APP_DEBUG(("Device %02x:%02x:%02x:%02x:%02x:%02x %s, conn_id %d reason 0x%x\n",
            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
            p_data->connected ? "connected" : "disconnected", p_data->conn_id, p_data->reason));

    if (p_data->connected)
    {
        tunnel_ble_connection_up(p_data);
    }
    else
    {
        tunnel_ble_connection_down(p_data);
    }

    return WICED_BT_GATT_SUCCESS;
}

/*
 * GATT operation started by the client has been completed
 */
wiced_bt_gatt_status_t tunnel_bt_gatt_op_cmpl(wiced_bt_gatt_operation_complete_t *p_data)
{
    tunneled_accessory_t* p_acc;

    WPRINT_APP_DEBUG(("tunnel_bt_gatt_op_cmpl op %d status %d\n", p_data->op, p_data->status));

    switch (p_data->op)
    {
    case GATTC_OPTYPE_READ:
        tunnel_ble_process_read_rsp(p_data);
        break;

    case GATTC_OPTYPE_WRITE:
        tunnel_ble_process_write_rsp(p_data);
        break;

    case GATTC_OPTYPE_CONFIG:
        WPRINT_APP_DEBUG(("peer mtu:%d\n", p_data->response_data.mtu));

        p_acc = tunnel_get_accessory_by_conn_id(p_data->conn_id);
        if (p_acc)
        {
            tunnel_ble_send_queued_request(p_acc);
        }
        break;

    case GATTC_OPTYPE_NOTIFICATION:
        break;

    case GATTC_OPTYPE_INDICATION:
        tunnel_ble_process_indication(p_data);
        wiced_bt_gatt_send_indication_confirm(p_data->conn_id, p_data->response_data.handle);
        break;
    }
    return WICED_BT_GATT_SUCCESS;
}

wiced_bt_gatt_status_t tunnel_bt_gatt_disc_result(wiced_bt_gatt_discovery_result_t *p_data)
{
    WPRINT_APP_DEBUG(("tunnel_bt_gatt_disc_result type %d\n", p_data->discovery_type));

    if (!ble_cb.p_discovery_data || ble_cb.p_discovery_data->discovery_conn_id != p_data->conn_id)
        return WICED_BT_GATT_SUCCESS;

    switch (p_data->discovery_type)
    {
    case GATT_DISCOVER_SERVICES_ALL:
        tunnel_ble_process_service(&p_data->discovery_data.group_value);
        break;

    case GATT_DISCOVER_SERVICES_BY_UUID:
        break;

    case GATT_DISCOVER_CHARACTERISTICS:
        tunnel_ble_process_characteristic(&p_data->discovery_data.characteristic_declaration);
        break;

    case GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS:
        tunnel_ble_process_descriptor(&p_data->discovery_data.char_descr_info);
        break;
    }
    return WICED_BT_GATT_SUCCESS;
}

wiced_bt_gatt_status_t tunnel_bt_gatt_disc_cmpl(wiced_bt_gatt_discovery_complete_t *p_data)
{
    WPRINT_APP_DEBUG(("tunnel_gatt_disc_cmpl conn %d type %d st 0x%02x\n", p_data->conn_id,
            p_data->disc_type, p_data->status));

    if (!ble_cb.p_discovery_data || ble_cb.p_discovery_data->discovery_conn_id != p_data->conn_id)
        return WICED_BT_GATT_SUCCESS;

    switch (p_data->disc_type)
    {
    case GATT_DISCOVER_SERVICES_ALL:
        ble_cb.p_discovery_data->discover_svc_idx = 0;
        tunnel_ble_discover_characteristics(p_data->conn_id);
        break;

    case GATT_DISCOVER_SERVICES_BY_UUID:
        break;

    case GATT_DISCOVER_CHARACTERISTICS:
        ble_cb.p_discovery_data->discover_char_idx = 0;
        tunnel_ble_discover_descriptors(p_data->conn_id);
        break;

    case GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS:
        ble_cb.p_discovery_data->discover_char_idx++;
        tunnel_ble_discover_descriptors(p_data->conn_id);
        break;
    }

    return WICED_BT_GATT_SUCCESS;
}

static wiced_bool_t tunnel_ble_send_queued_request(tunneled_accessory_t* p_acc)
{
    tunnel_ble_request_t* p_req;

    if ((p_req = GKI_dequeue(&p_acc->req_queue)) != NULL)
    {
        switch (p_req->req_type)
        {
        case BLE_REQUEST_READ:
            tunnel_gatt_send_read_by_handle(p_acc->conn_id, p_req->p_char->handle);
            break;
        case BLE_REQUEST_WRITE:
            tunnel_gatt_send_write(p_acc->conn_id, GATT_WRITE, p_req->p_char->handle,
                    p_req->u.write.p_data, p_req->u.write.data_len);
            break;
        case BLE_REQUEST_CONFIG:
            tunnel_gatt_send_write(p_acc->conn_id, GATT_WRITE,
                    p_req->p_char->ccc_handle, (uint8_t*)&p_req->u.config.client_config,
                    sizeof(uint16_t));
            break;
        }
        p_acc->state = TUNNEL_BLE_STATE_PENDING;
        GKI_freebuf(p_req);
        return WICED_TRUE;
    }

    p_acc->state = TUNNEL_BLE_STATE_CONNECTED;
    return WICED_FALSE;
}

void tunnel_ble_connection_up(wiced_bt_gatt_connection_status_t *p_data)
{
    if (ble_cb.p_discovery_data &&
        memcmp(ble_cb.p_discovery_data->bd_addr, p_data->bd_addr, BD_ADDR_LEN) == 0 &&
        ble_cb.p_discovery_data->discovery_state == BLE_DISC_STATE_CONNECT)
    {
        /* reduce connection interval to speed up discovery process */
        wiced_bt_l2cap_update_ble_conn_params(p_data->bd_addr, 6, 6, 0, 200);

        ble_cb.p_discovery_data->discovery_conn_id = p_data->conn_id;
        ble_cb.p_discovery_data->discovery_state = BLE_DISC_STATE_READ_NAME;
        tunnel_gatt_send_read_by_type(p_data->conn_id, 1, 0xffff, UUID_CHARACTERISTIC_DEVICE_NAME);
    }
    else
    {
        tunneled_accessory_t* p_acc = tunnel_get_accessory_by_bdaddr(p_data->bd_addr);

        if (!p_acc)
            return;

        p_acc->conn_id = p_data->conn_id;

        wiced_bt_gatt_configure_mtu(p_data->conn_id, GATT_MTU_SIZE);

        tunnel_ble_connected_callback(p_acc, WICED_TRUE);
    }
}

void tunnel_ble_connection_down(wiced_bt_gatt_connection_status_t *p_data)
{
    tunneled_accessory_t* p_acc = tunnel_get_accessory_by_bdaddr(p_data->bd_addr);
    tunnel_ble_request_t* p_req;

    if (!p_acc)
        return;

    if (ble_cb.p_discovery_data)
    {
        tunnel_ble_discovery_finished(WICED_BT_GATT_ERROR);
    }

    while ((p_req = GKI_dequeue(&p_acc->req_queue)) != NULL)
    {
        switch (p_req->req_type)
        {
        case BLE_REQUEST_READ:
            tunnel_ble_read_callback(p_acc, p_req->p_char, WICED_BT_GATT_ERROR, NULL, 0);
            break;
        case BLE_REQUEST_WRITE:
            tunnel_ble_write_callback(p_acc, p_req->p_char, WICED_BT_GATT_ERROR);
            break;
        case BLE_REQUEST_CONFIG:
            tunnel_ble_write_callback(p_acc, p_req->p_char, WICED_BT_GATT_ERROR);
            break;
        }
        GKI_freebuf(p_req);
    }
    p_acc->state = TUNNEL_BLE_STATE_IDLE;
    p_acc->conn_id = 0;

    tunnel_ble_connected_callback(p_acc, WICED_FALSE);
}

static uint16_t tunnel_ble_get_uint16_data(wiced_bt_gatt_data_t* p_data)
{
    uint16_t u16;
    uint8_t* p = p_data->p_data + p_data->offset;

    u16 = (uint16_t)p[0];
    if (p_data->len >= 2)
    {
        u16 += ((uint16_t)(p[1])) << 8;
    }

    return u16;
}

void tunnel_ble_process_read_rsp(wiced_bt_gatt_operation_complete_t *p_data)
{
    wiced_bt_gatt_data_t* p_gatt_data = &p_data->response_data.att_value;

    if (ble_cb.p_discovery_data && ble_cb.p_discovery_data->discovery_conn_id == p_data->conn_id)
    {
        ble_discovery_service_t* p_service;
        ble_discovery_char_t* p_char;

        p_service = &ble_cb.p_discovery_data->services[ble_cb.p_discovery_data->discover_svc_idx];
        p_char = &p_service->characteristics[ble_cb.p_discovery_data->discover_char_idx];

        switch (ble_cb.p_discovery_data->discovery_state)
        {
        case BLE_DISC_STATE_READ_NAME:
            if (p_data->status == WICED_BT_GATT_SUCCESS)
            {
                uint16_t len = p_gatt_data->len;
                if (len >= TUNNEL_ACCESSORY_MAX_NAME_LEN)
                    len = TUNNEL_ACCESSORY_MAX_NAME_LEN - 1;
                memcpy(ble_cb.p_discovery_data->local_name, p_gatt_data->p_data, len);
                ble_cb.p_discovery_data->local_name[len] = 0;
            }
            ble_cb.p_discovery_data->discovery_state = BLE_DISC_STATE_DISCOVER_SERVICES;
            tunnel_gatt_send_discover(p_data->conn_id, GATT_DISCOVER_SERVICES_ALL, 0, 1, 0xffff);
            break;
        case BLE_DISC_STATE_READ_SVC_INST_ID:
            if (p_data->status == WICED_BT_GATT_SUCCESS)
                p_service->inst_id = tunnel_ble_get_uint16_data(p_gatt_data);
            ble_cb.p_discovery_data->discover_svc_idx++;
            tunnel_ble_read_service_instance_id(p_data->conn_id);
            break;
        case BLE_DISC_STATE_READ_CHAR_INST_ID:
            if (p_data->status == WICED_BT_GATT_SUCCESS)
                p_char->inst_id = tunnel_ble_get_uint16_data(p_gatt_data);
            ble_cb.p_discovery_data->discover_char_idx++;
            tunnel_ble_read_characteristic_instance_id(p_data->conn_id);
            break;
        case BLE_DISC_STATE_READ_SIGNATURE:
            if (p_data->status == WICED_BT_GATT_SUCCESS)
                tunnel_ble_process_characteristic_signature(p_gatt_data, p_char);
            ble_cb.p_discovery_data->discover_char_idx++;
            tunnel_ble_read_characteristic_signature(p_data->conn_id);
            break;
        }
    }
    else
    {
        tunneled_accessory_t* p_acc = NULL;
        tunneled_characteristic_t* p_char = NULL;

        WPRINT_APP_DEBUG(("BLE read response status:%d, length:%d, offset:%d\n",
                p_data->status, p_gatt_data->len, p_gatt_data->offset));
        p_acc = tunnel_get_accessory_by_conn_id(p_data->conn_id);
        if (!p_acc)
            return;

        p_char = tunnel_get_characteristic_by_ble_handle(p_acc, p_gatt_data->handle);
        if (p_char)
        {
            tunnel_ble_read_callback(p_acc, p_char, p_data->status,
                    p_gatt_data->p_data + p_gatt_data->offset, p_gatt_data->len);
        }

        tunnel_ble_send_queued_request(p_acc);
    }
}

void tunnel_ble_process_write_rsp(wiced_bt_gatt_operation_complete_t *p_data)
{
    wiced_bt_gatt_data_t *p_gatt_data = &p_data->response_data.att_value;

    if (ble_cb.p_discovery_data && ble_cb.p_discovery_data->discovery_conn_id == p_data->conn_id)
    {
        if (ble_cb.p_discovery_data->hap_req_sent)
        {
            tunnel_gatt_send_read_by_handle(p_data->conn_id, p_gatt_data->handle);
            ble_cb.p_discovery_data->hap_req_sent = WICED_FALSE;
        }
    }
    else
    {
        tunneled_accessory_t* p_acc = NULL;
        tunneled_characteristic_t* p_char = NULL;

        WPRINT_APP_DEBUG(("BLE write response status:%d\n", p_data->status));
        p_acc = tunnel_get_accessory_by_conn_id(p_data->conn_id);
        if (!p_acc)
            return;

        p_char = tunnel_get_characteristic_by_ble_handle(p_acc, p_gatt_data->handle);
        if (p_char)
        {
            tunnel_ble_write_callback(p_acc, p_char, p_data->status);
        }

        tunnel_ble_send_queued_request(p_acc);
    }
}

void tunnel_ble_process_indication(wiced_bt_gatt_operation_complete_t *p_data)
{
    wiced_bt_gatt_data_t *p_gatt_data = &p_data->response_data.att_value;
    tunneled_accessory_t* p_acc = NULL;
    tunneled_characteristic_t* p_char = NULL;

    WPRINT_APP_DEBUG(("BLE indication status:%d\n", p_data->status));
    p_acc = tunnel_get_accessory_by_conn_id(p_data->conn_id);
    if (!p_acc)
        return;

    p_char = tunnel_get_characteristic_by_ble_handle(p_acc, p_gatt_data->handle);
    if (p_char)
    {
        tunnel_ble_indication_callback(p_acc, p_char, p_data->status);
    }
}

/*
 * Find the ending handle for a characteristic
 */
static uint16_t tunnel_ble_find_e_handle(ble_discovery_service_t* p_service, uint16_t s_handle)
{
    uint16_t e_handle = p_service->e_handle;
    uint8_t i;

    for (i = 0; i < p_service->num_of_chars; i++)
    {
        if (p_service->characteristics[i].handle > s_handle &&
            p_service->characteristics[i].handle < e_handle)
        {
            e_handle = p_service->characteristics[i].handle - 1;
        }
    }

    return e_handle;
}

/*
 * This function starts discovering characteristics in the BLE service
 */
void tunnel_ble_discover_characteristics(uint16_t conn_id)
{
    ble_discovery_service_t* p_service;

    if (ble_cb.p_discovery_data->discover_svc_idx < ble_cb.p_discovery_data->num_of_svcs)
    {
        ble_cb.p_discovery_data->discovery_state = BLE_DISC_STATE_DISCOVER_CHARACTERISTICS;
        p_service = &ble_cb.p_discovery_data->services[ble_cb.p_discovery_data->discover_svc_idx];
        tunnel_gatt_send_discover(conn_id, GATT_DISCOVER_CHARACTERISTICS, 0, p_service->s_handle,
                p_service->e_handle);
    }
    else
    {
        ble_cb.p_discovery_data->discover_svc_idx = 0;
        tunnel_ble_read_service_instance_id(conn_id);
    }
}

/*
 * This function starts discovering characteristic descriptors in the BLE service
 */
void tunnel_ble_discover_descriptors(uint16_t conn_id)
{
    ble_discovery_service_t* p_service;
    ble_discovery_char_t* p_char;

    p_service = &ble_cb.p_discovery_data->services[ble_cb.p_discovery_data->discover_svc_idx];

    if (ble_cb.p_discovery_data->discover_char_idx < p_service->num_of_chars)
    {
        ble_cb.p_discovery_data->discovery_state = BLE_DISC_STATE_DISCOVER_DESCRIPTORS;
        p_char = &p_service->characteristics[ble_cb.p_discovery_data->discover_char_idx];
        tunnel_gatt_send_discover(conn_id, GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS, 0,
                p_char->handle, tunnel_ble_find_e_handle(p_service, p_char->handle));
    }
    else
    {
        ble_cb.p_discovery_data->discover_svc_idx++;
        tunnel_ble_discover_characteristics(conn_id);
    }
}

/*
 * This function reads instance ID of a HAP service
 */
void tunnel_ble_read_service_instance_id(uint16_t conn_id)
{
    ble_discovery_service_t* p_service;

    if (ble_cb.p_discovery_data->discover_svc_idx < ble_cb.p_discovery_data->num_of_svcs)
    {
        p_service = &ble_cb.p_discovery_data->services[ble_cb.p_discovery_data->discover_svc_idx];
        ble_cb.p_discovery_data->discovery_state = BLE_DISC_STATE_READ_SVC_INST_ID;
        tunnel_gatt_send_read_by_handle(conn_id, p_service->inst_id_handle);
    }
    else
    {
        ble_cb.p_discovery_data->discover_svc_idx = 0;
        ble_cb.p_discovery_data->discover_char_idx = 0;
        tunnel_ble_read_characteristic_instance_id(conn_id);
    }
}

/*
 * This function reads instance ID of a HAP characteristic
 */
void tunnel_ble_read_characteristic_instance_id(uint16_t conn_id)
{
    ble_discovery_service_t* p_service;
    ble_discovery_char_t* p_char;

    if (ble_cb.p_discovery_data->discover_svc_idx < ble_cb.p_discovery_data->num_of_svcs)
    {
        p_service = &ble_cb.p_discovery_data->services[ble_cb.p_discovery_data->discover_svc_idx];
        if (ble_cb.p_discovery_data->discover_char_idx < p_service->num_of_chars)
        {
            p_char = &p_service->characteristics[ble_cb.p_discovery_data->discover_char_idx];
            ble_cb.p_discovery_data->discovery_state = BLE_DISC_STATE_READ_CHAR_INST_ID;
            tunnel_gatt_send_read_by_handle(conn_id, p_char->inst_id_handle);
        }
        else
        {
            ble_cb.p_discovery_data->discover_svc_idx++;
            ble_cb.p_discovery_data->discover_char_idx = 0;
            tunnel_ble_read_characteristic_instance_id(conn_id);
        }
    }
    else
    {
        ble_cb.p_discovery_data->discover_svc_idx = 0;
        ble_cb.p_discovery_data->discover_char_idx = 0;
        tunnel_ble_read_characteristic_signature(conn_id);
    }
}

/*
 * This function reads signature of a HAP characteristic
 */
void tunnel_ble_read_characteristic_signature(uint16_t conn_id)
{
    ble_discovery_service_t* p_service;
    ble_discovery_char_t* p_char;

    if (ble_cb.p_discovery_data->discover_svc_idx < ble_cb.p_discovery_data->num_of_svcs)
    {
        p_service = &ble_cb.p_discovery_data->services[ble_cb.p_discovery_data->discover_svc_idx];
        if (ble_cb.p_discovery_data->discover_char_idx < p_service->num_of_chars)
        {
            p_char = &p_service->characteristics[ble_cb.p_discovery_data->discover_char_idx];
            ble_cb.p_discovery_data->discovery_state = BLE_DISC_STATE_READ_SIGNATURE;
            tunnel_ble_send_hap_signature_read(conn_id, p_char->handle, p_char->inst_id);
            ble_cb.p_discovery_data->hap_req_sent = WICED_TRUE;
        }
        else
        {
            ble_cb.p_discovery_data->discover_svc_idx++;
            ble_cb.p_discovery_data->discover_char_idx = 0;
            tunnel_ble_read_characteristic_signature(conn_id);
        }
    }
    else
    {
        WPRINT_APP_DEBUG(("Discovery complete\n"));
        tunnel_debug_dump_discovery();
        tunnel_ble_discovery_finished(WICED_BT_GATT_SUCCESS);
        wiced_bt_gatt_disconnect(conn_id);
        tunnel_ble_start_scan();
    }
}

/*
 * This function sends discovery data back to tunnel
 */
void tunnel_ble_discovery_finished(wiced_bt_gatt_status_t status)
{
    if (status == WICED_BT_GATT_SUCCESS)
        tunnel_ble_discovery_callback(ble_cb.p_discovery_data);

    if (ble_cb.p_discovery_data)
    {
        free(ble_cb.p_discovery_data);
        ble_cb.p_discovery_data = NULL;
    }
}

/*
 * Get the 16-bit uuid
 */
static uint16_t tunnel_ble_get_uuid16(wiced_bt_uuid_t uuid)
{
    uint16_t uuid16 = 0;

    if (uuid.len == LEN_UUID_16)
    {
        uuid16 = uuid.uu.uuid16;
    }
    else if (uuid.len == LEN_UUID_32)
    {
        uuid16 = (uint16_t)uuid.uu.uuid32;
    }
    else if (uuid.len == LEN_UUID_128)
    {
        uuid16 = (uint16_t)uuid.uu.uuid128[12] + ((uint16_t)uuid.uu.uuid128[13] << 8);
    }

    return uuid16;
}

void tunnel_ble_process_service(wiced_bt_gatt_group_value_t *p_data)
{
    ble_discovery_service_t* p_service;
    uint16_t uuid16 = tunnel_ble_get_uuid16(p_data->service_type);

    WPRINT_APP_DEBUG(("Service uuid 0x%x s_handle 0x%x e_handle 0x%x\n", uuid16, p_data->s_handle,
            p_data->e_handle));
    if (tunnel_ble_is_hap_uuid(p_data->service_type))
    {
        if (ble_cb.p_discovery_data->num_of_svcs < TUNNEL_BLE_ACCESSORY_MAX_SERVICES)
        {
            p_service = &ble_cb.p_discovery_data->services[ble_cb.p_discovery_data->num_of_svcs++];
            p_service->uuid = uuid16;
            p_service->s_handle = p_data->s_handle;
            p_service->e_handle = p_data->e_handle;
        }
        else
        {
            WPRINT_APP_ERROR(("Number of services exceeds limit\n"));
        }
    }
}

void tunnel_ble_process_characteristic(wiced_bt_gatt_char_declaration_t *p_data)
{
    ble_discovery_service_t* p_service;
    ble_discovery_char_t* p_char;
    uint16_t uuid16 = tunnel_ble_get_uuid16(p_data->char_uuid);

    WPRINT_APP_DEBUG(("Characteristic uuid 0x%x handle 0x%x properties 0x%x\n", uuid16,
            p_data->handle, p_data->characteristic_properties));

    p_service = &ble_cb.p_discovery_data->services[ble_cb.p_discovery_data->discover_svc_idx];
    if (tunnel_ble_is_service_inst_id(p_data->char_uuid))
    {
        p_service->inst_id_handle = p_data->val_handle;
    }
    else if (tunnel_ble_is_hap_uuid(p_data->char_uuid))
    {
        if (p_service->num_of_chars < TUNNEL_BLE_ACCESSORY_MAX_CHARACTERISTICS)
        {
            p_char = &p_service->characteristics[p_service->num_of_chars++];
            p_char->uuid = uuid16;
            p_char->handle = p_data->val_handle;
            p_char->ble_properties = p_data->characteristic_properties;
        }
        else
        {
            WPRINT_APP_ERROR(("Number of characteristic exceeds limit\n"));
        }
    }
}

void tunnel_ble_process_descriptor(wiced_bt_gatt_char_descr_info_t *p_data)
{
    ble_discovery_service_t* p_service;
    ble_discovery_char_t* p_char;

    p_service = &ble_cb.p_discovery_data->services[ble_cb.p_discovery_data->discover_svc_idx];
    p_char = &p_service->characteristics[ble_cb.p_discovery_data->discover_char_idx];
    if (tunnel_ble_is_characteristic_inst_id(p_data->type))
    {
        WPRINT_APP_DEBUG(("Characteristic instance ID descriptor handle 0x%x\n", p_data->handle));
        p_char->inst_id_handle = p_data->handle;
    }
    else if (tunnel_ble_is_client_characteristic_configuration(p_data->type))
    {
        WPRINT_APP_DEBUG(("Characteristic config descriptor handle 0x%x\n", p_data->handle));
        p_char->ccc_handle = p_data->handle;
    }
}

tunnel_ble_hap_pdu_tlv8_t* tunnel_ble_find_hap_param(uint8_t type, uint8_t* p_data,
        uint32_t data_len)
{
    tunnel_ble_hap_response_t* p_response;
    uint8_t* ptr;
    uint8_t* p_param_end;
    uint16_t body_len;
    tunnel_ble_hap_pdu_tlv8_t* p_ret = NULL;

    if (data_len < HAP_PDU_RESPONSE_BODY_VALUE_OFFSET)
        return NULL;

    p_response = (tunnel_ble_hap_response_t*)p_data;
    ptr = p_response->body_value;
    body_len = (uint16_t)p_response->body_len[0] + (uint16_t)(p_response->body_len[1] << 8);
    p_param_end = ptr + body_len;

    while(ptr < p_param_end)
    {
        tunnel_ble_hap_pdu_tlv8_t* p_tlv = (tunnel_ble_hap_pdu_tlv8_t*)ptr;
        if (p_tlv->type == type)
        {
            p_ret = p_tlv;
            break;
        }

        ptr += p_tlv->length + 2;
    }

    return p_ret;
}

void tunnel_ble_process_characteristic_signature(wiced_bt_gatt_data_t* p_data,
        ble_discovery_char_t* p_char)
{
    uint8_t* p = p_data->p_data + p_data->offset;
    tunnel_ble_hap_response_t* p_response = (tunnel_ble_hap_response_t*)p;
    tunnel_ble_hap_pdu_tlv8_t* p_tlv;

    if (p_response->status == HAP_STATUS_SUCCESS)
    {
        p_tlv = tunnel_ble_find_hap_param(HAP_PARAM_HAP_CHARACTERISTIC_PROPERTIES_DESCRIPTOR, p,
                p_data->len);
        if (p_tlv && p_tlv->length == 2)
        {
            p_char->hap_properties = (uint16_t)p_tlv->value[0] + ((uint16_t)p_tlv->value[1] << 8);
        }

        p_tlv = tunnel_ble_find_hap_param(HAP_PARAM_GATT_PRESENTATION_FORMAT_DESCRIPTOR, p,
                p_data->len);
        if (p_tlv && p_tlv->length == 7)
        {
            p_char->format = p_tlv->value[0];
            p_char->unit = (uint16_t)p_tlv->value[2] + ((uint16_t)p_tlv->value[3] << 8);
        }

        p_tlv = tunnel_ble_find_hap_param(HAP_PARAM_GATT_VALID_RANGE, p, p_data->len);
        if (p_tlv)
        {
            int value_len = p_tlv->length / 2;

            p_char->optional_params = WICED_TRUE;
            memcpy(p_char->minimum, p_tlv->value, value_len);
            memcpy(p_char->maximum, &p_tlv->value[value_len], value_len);
        }

        p_tlv = tunnel_ble_find_hap_param(HAP_PARAM_HAP_STEP_VALUE_DESCRIPTOR, p, p_data->len);
        if (p_tlv)
        {
            memcpy(p_char->step, p_tlv->value, p_tlv->length);
        }
    }
}

wiced_bool_t tunnel_gatt_connect(wiced_bt_device_address_t bd_addr, uint8_t addr_type)
{
    WPRINT_APP_DEBUG(("Connect BLE accessory %02x:%02x:%02x:%02x:%02x:%02x\n",
            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]));
    return wiced_bt_gatt_le_connect(bd_addr, addr_type, BLE_CONN_MODE_HIGH_DUTY, WICED_TRUE);
}

wiced_bool_t tunnel_gatt_cancel_connect(wiced_bt_device_address_t bd_addr)
{
    return wiced_bt_gatt_cancel_connect(bd_addr, WICED_TRUE);
}

void tunnel_gatt_send_discover(uint16_t conn_id, wiced_bt_gatt_discovery_type_t type,
        uint16_t uuid, uint16_t s_handle, uint16_t e_handle)
{
    wiced_bt_gatt_discovery_param_t param;

    memset(&param, 0, sizeof(param));
    if (uuid != 0)
    {
        param.uuid.len = LEN_UUID_16;
        param.uuid.uu.uuid16 = uuid;
    }
    param.s_handle = s_handle;
    param.e_handle = e_handle;

    wiced_bt_gatt_send_discover(conn_id, type, &param);
}

void tunnel_gatt_send_read_by_handle(uint16_t conn_id, uint16_t handle)
{
    wiced_bt_gatt_read_param_t param;

    memset(&param, 0, sizeof(param));
    param.by_handle.handle = handle;

    wiced_bt_gatt_send_read(conn_id, GATT_READ_BY_HANDLE, &param);
}

void tunnel_gatt_send_read_by_type(uint16_t conn_id, uint16_t s_handle, uint16_t e_handle, uint16_t uuid)
{
    wiced_bt_gatt_read_param_t param;

    memset(&param, 0, sizeof(param));
    param.service.s_handle = s_handle;
    param.service.e_handle = e_handle;
    param.service.uuid.len = LEN_UUID_16;
    param.service.uuid.uu.uuid16 = uuid;

    wiced_bt_gatt_send_read(conn_id, GATT_READ_BY_TYPE, &param);
}

void tunnel_gatt_send_write(uint16_t conn_id, wiced_bt_gatt_write_type_t type, uint16_t handle,
        uint8_t *p_data, uint16_t data_len)
{
    wiced_bt_gatt_value_t *p_value = (wiced_bt_gatt_value_t *)GKI_getbuf(GATT_RESPONSE_SIZE(data_len));

    if (p_value)
    {
        memset(p_value, 0, sizeof(wiced_bt_gatt_value_t));
        p_value->handle = handle;
        p_value->len = data_len;
        memcpy(p_value->value, p_data, data_len);
        wiced_bt_gatt_send_write(conn_id, type, p_value);
        GKI_freebuf(p_value);
    }
}

uint8_t tunnel_ble_get_hap_tid()
{
    return 1;
}

void tunnel_ble_send_hap_signature_read(uint16_t conn_id, uint16_t handle, uint16_t inst_id)
{
    tunnel_ble_hap_request_t request;

    request.control = HAP_CONTROL_FIELD_REQUEST;
    request.opcode = HAP_CHARACTERISTIC_SIGNATURE_READ;
    request.tid = tunnel_ble_get_hap_tid();
    request.cid[0] = (uint8_t)(inst_id & 0xff);
    request.cid[1] = (uint8_t)((inst_id >> 8) & 0xff);

    tunnel_gatt_send_write(conn_id, GATT_WRITE, handle, (uint8_t*)&request,
            HAP_PDU_REQUEST_HEADER_SIZE);
}

wiced_bool_t tunnel_ble_is_hap_uuid(wiced_bt_uuid_t uuid)
{
    uint8_t hap_uuid[LEN_UUID_128] = {HAP_GENERIC_UUID};

    if (uuid.len == LEN_UUID_128 &&
        memcmp(uuid.uu.uuid128, hap_uuid, 12) == 0)
    {
        return WICED_TRUE;
    }
    return WICED_FALSE;
}

wiced_bool_t tunnel_ble_is_service_inst_id(wiced_bt_uuid_t uuid)
{
    uint8_t svc_inst_id_uuid[LEN_UUID_128] = {HAP_SVC_INST_ID_UUID};

    if (uuid.len == LEN_UUID_128 &&
        memcmp(uuid.uu.uuid128, svc_inst_id_uuid, LEN_UUID_128) == 0)
    {
        return WICED_TRUE;
    }
    return WICED_FALSE;
}

wiced_bool_t tunnel_ble_is_characteristic_inst_id(wiced_bt_uuid_t uuid)
{
    uint8_t char_inst_id_uuid[LEN_UUID_128] = {HAP_CHAR_INST_ID_UUID};

    if (uuid.len == LEN_UUID_128 &&
        memcmp(uuid.uu.uuid128, char_inst_id_uuid, LEN_UUID_128) == 0)
    {
        return WICED_TRUE;
    }
    return WICED_FALSE;
}

wiced_bool_t tunnel_ble_is_client_characteristic_configuration(wiced_bt_uuid_t uuid)
{
    if (uuid.len == LEN_UUID_16 &&
        uuid.uu.uuid16 == UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION)
    {
        return WICED_TRUE;
    }
    return WICED_FALSE;
}

void tunnel_debug_dump_discovery()
{
#if 0
    ble_discovery_data_t* p_data = ble_cb.p_discovery_data;

    WPRINT_APP_DEBUG(("Discovered %d services\n", p_data->num_of_svcs));
    for (int i = 0; i < p_data->num_of_svcs; i++)
    {
        ble_discovery_service_t* p_service = &p_data->services[i];
        WPRINT_APP_DEBUG(("Service 0x%04x inst ID %d has %d characteristics\n", p_service->uuid,
                p_service->inst_id, p_service->num_of_chars));
        for (int j = 0; j < p_service->num_of_chars; j++)
        {
            ble_discovery_char_t* p_char = &p_service->characteristics[j];
            WPRINT_APP_DEBUG(("Characteristic 0x%04x inst ID %d\n",p_char->uuid,p_char->inst_id));
            WPRINT_APP_DEBUG(("  handle %d ccc_handle %d properties 0x%x\n",p_char->handle,
                    p_char->ccc_handle, p_char->ble_properties));
            WPRINT_APP_DEBUG(("  permission 0x%x format 0x%x unit 0x%x\n",p_char->hap_properties,
                    p_char->format, p_char->unit));
            WPRINT_APP_DEBUG(("  min %d max %d step %d\n", (int)p_char->minimum,
                    (int)p_char->maximum, (int)p_char->step));
        }

    }
#endif
}

#ifdef HCI_LOG_IP
static wiced_result_t wiced_log_msg(int trace_type, void *p_data, uint16_t data_len)
{
    wiced_packet_t*          packet;
    char*                    udp_data;
    uint16_t                 available_data_length;
    BT_HDR*                  p_hdr;
    const wiced_ip_address_t INITIALISER_IPV4_ADDRESS( target_ip_addr, log_ip_addr );

    /* Create the UDP packet */
    if (wiced_packet_create_udp(&log_socket, LOG_MAX_DATA_LENGTH, &packet, (uint8_t**)&udp_data, &available_data_length) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("UDP tx packet creation failed\r\n"));
        return WICED_ERROR;
    }

    /* copy log information to the UDP packet data */
    p_hdr                  = (BT_HDR *)udp_data;
    p_hdr->event           = trace_type;
    p_hdr->len             = data_len;
    p_hdr->offset          = 0;
    p_hdr->layer_specific  = 2;

    available_data_length -= sizeof (BT_HDR);

    memcpy(p_hdr + 1, p_data, data_len < available_data_length ? data_len : available_data_length);

    /* Set the end of the data portion */
    wiced_packet_set_data_end(packet, (uint8_t*)udp_data + LOG_MAX_DATA_LENGTH);

    /* Send the UDP packet */
    if (wiced_udp_send(&log_socket, &target_ip_addr, LOG_SPY_UDP_PORT, packet) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("UDP packet send failed\r\n"));
        wiced_packet_delete(packet);  /* Delete packet, since the send failed */
        return WICED_ERROR;
    }
    return WICED_SUCCESS;

}

static void hci_log_handler(int trace_type, BT_HDR *p_msg)
{
    wiced_log_msg(trace_type, (uint8_t*)(p_msg + 1) + p_msg->offset, p_msg->len);
}
#endif
