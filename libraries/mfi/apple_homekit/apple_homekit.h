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

#include "JSON.h"
#include "http_server.h"
#include "linked_list.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HOMEKIT_UUID                                    "-0000-1000-8000-0026BB765291" /* Append any leading zeroes required to make 8 digits */

#define WICED_HOMEKIT_EVENT                             "ev"

#define WICED_HOMEKIT_DESCRIPTION                       "description"

#define HOMEKIT_MAX_TRANSACTION_ID_BYTES                (40)

#define HOMEKIT_MAX_UPDATES                             (100)

#define HOMEKIT_MAX_TYPE                                (37) /* max uuid of 36 plus null */

#define HOMEKIT_MAX_CONTROLLERS                         (16)

#define HOMEKIT_MAX_ICLOUD_CONTROLLERS                  (8)

#define HOMEKIT_MAX_ACTIVE_CONNECTIONS                  (8)

#define MAX_READ_REQUESTS                               (20) /*100?*/

#define STRING_MAX_MAC                                  (18)    // ( 2 characters/byte * 6 bytes ) + ( 5 colon characters ) + NULL = 18

#ifdef HOMEKIT_ICLOUD_ENABLE
#define HOMEKIT_ICLOUD_CONTROLLER_IDENTIFIER_LENGTH     (45 + 1)    // TODO: Confirm with Apple about the length
#define HOMEKIT_RELAY_ACCESS_TOKEN_MAX_LEN              (244)       // TODO: Confirm with Apple about the length
#define WICED_HOMEKIT_DCT_BUFFER_SIZE                   ((1024 * 7))
#else
#define WICED_HOMEKIT_DCT_BUFFER_SIZE                   (2048)
#endif

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    OTHER                   =  1,
    BRIDGE                  =  2,
    FAN                     =  3,
    GARAGE_DOOR_OPENER      =  4,
    LIGHTBULB               =  5,
    DOOR_LOCK               =  6,
    OUTLET                  =  7,
    SWITCH                  =  8,
    THERMOSTAT              =  9,
    SENSOR                  = 10,
    SECURITY_SYSTEM         = 11,
    DOOR                    = 12,
    WINDOW                  = 13,
    WINDOW_COVERING         = 14,
    PROGRAMMABLE_SWITCH     = 15,
    RANGE_EXTENDER          = 16,
    RESERVED
} accessory_category_identifier_t;

typedef enum
{
    MFI_CONFIG_FLAGS_PAIRED                   = 0x00000000,
    MFI_CONFIG_FLAGS_NOT_PAIRED               = 0x00000001,
    MFI_CONFIG_FLAGS_UNCONFIGURED             = 0x00000002, /* [Status Flag] Device is not configured to join wifi network. */
    MFI_CONFIGURE_FLAGS_PROBLEM_DETECTED      = 0x00000004  /* [Status Flag] Problem has been detected. */
} wiced_mfi_configure_flags;

typedef enum
{
    MFI_CONFIG_FEATURES_UNDEFINED              = 0x00000000,
    MFI_CONFIG_FEATURES_SUPPORTS_MFI_AND_PIN   = 0x00000001,
    MFI_CONFIG_FEATURES_SUPPORTS_PIN_ONLY      = 0x00000002
} wiced_mfi_feature_flags;

typedef enum
{
    WICED_HOMEKIT_PERMISSIONS_PAIRED_READ       =  0x1,
    WICED_HOMEKIT_PERMISSIONS_PAIRED_WRITE      = (0x1 << 1),
    WICED_HOMEKIT_PERMISSIONS_EVENTS            = (0x1 << 2),
    WICED_HOMEKIT_PERMISSIONS_ADDITIONAL_AUTH   = (0x1 << 3),
    WICED_HOMEKIT_PERMISSIONS_TIMED_WRITE       = (0x1 << 4),
    WICED_HOMEKIT_PERMISSIONS_HIDDEN            = (0x1 << 5),
} wiced_homekit_permissions_flag_t;

typedef enum
{
    WICED_HOMEKIT_BOOLEAN_FORMAT,
    WICED_HOMEKIT_UINT8_FORMAT,
    WICED_HOMEKIT_UINT16_FORMAT,
    WICED_HOMEKIT_UINT32_FORMAT,
    WICED_HOMEKIT_UINT64_FORMAT,
    WICED_HOMEKIT_INT_FORMAT,
    WICED_HOMEKIT_FLOAT_FORMAT,
    WICED_HOMEKIT_STRING_FORMAT,
    WICED_HOMEKIT_TLV8_FORMAT,
    WICED_HOMEKIT_DATA_FORMAT,
    WICED_HOMEKIT_NO_FORMAT_SET
} wiced_homekit_format_t;

typedef enum
{
    HOMEKIT_NO_ERROR,
    HOMEKIT_ERROR_INVALID_VALUE_IN_WRITE_REQUEST,
    HOMEKIT_ERROR_RESOURCE_DOES_NOT_EXIST,
    HOMEKIT_ERROR_OPERATION_TIMED_OUT,
    HOMEKIT_ERROR_OUT_OF_RESOURCES,
    HOMEKIT_ERROR_NOTIFICATION_IS_NOT_SUPPORTED,
    HOMEKIT_ERROR_CAN_NOT_READ_FROM_WRITE_ONLY_CHARACTERISTIC,
    HOMEKIT_ERROR_CAN_NOT_WRITE_TO_READ_ONLY_CHARACTERISTIC,
    HOMEKIT_ERROR_RESOURCE_IS_BUSY_TRY_AGAIN,
    HOMEKIT_ERROR_UNABLE_TO_COMMUNICATE_WITH_REQUESTED_SERVICE,
    HOMEKIT_ERROR_REQUEST_DENIED_DUE_TO_INSUFFICIENT_PRIVILIGES,
    HOMEKIT_ERROR_UNSUPPORTED,
} wiced_homekit_error_code_t;

typedef enum
{
    HOMEKIT_UNIT_CELCIUS,
    HOMEKIT_UNIT_PERCENTAGE,
    HOMEKIT_UNIT_ARC_DEGREE,
    HOMEKIT_UNIT_SECOND,
    HOMEKIT_UNIT_LUX,
    HOMEKIT_NO_UNIT_SET
} wiced_homekit_unit_t;

typedef enum
{
    HOMEKIT_READ_STATE_NUMBER_REQUEST,
    HOMEKIT_WRITE_STATE_NUMBER_REQUEST
} wiced_homekit_external_state_number_storage_request_t;

typedef enum
{
    HOMEKIT_RESPONSE_TYPE_READ,
    HOMEKIT_RESPONSE_TYPE_WRITE,
    HOMEKIT_RESPONSE_TYPE_CONFIG,
    HOMEKIT_RESPONSE_TYPE_EVENT
} wiced_homekit_response_type_t;

typedef enum
{
    HOMEKIT_STATUS_SUCCESS = 0,
    HOMEKIT_STATUS_INSUFFICIENT_PRIVILEGES = -70401,
    HOMEKIT_STATUS_UNABLE_TO_COMMUNICATE = -70402,
    HOMEKIT_STATUS_RESOURCE_IS_BUSY = -70403,
    HOMEKIT_STATUS_CAN_NOT_WRITE = -70404,
    HOMEKIT_STATUS_CAN_NOT_READ = -70405,
    HOMEKIT_STATUS_NOTIFICATION_NOT_SUPPORTED = -70406,
    HOMEKIT_STATUS_OUT_OF_RESOURCES = -70407,
    HOMEKIT_STATUS_OPERATION_TIMED_OUT = -70408,
    HOMEKIT_STATUS_RESOURCE_DOES_NOT_EXIST = -70409,
    HOMEKIT_STATUS_INVALID_VALUE_IN_WRITE = -70410,
} wiced_homekit_status_code_t;

typedef enum
{
    HOMEKIT_TLV_NO_ERROR             = 0x0,
    HOMEKIT_TLV_ERROR_UNKNOWN_ERROR  = 0x1,
    HOMEKIT_TLV_ERROR_AUTHENTICATION = 0x2,
    HOMEKIT_TLV_ERROR_BACKOFF        = 0x3,
    HOMEKIT_TLV_ERROR_MAX_PEERS      = 0x4,
    HOMEKIT_TLV_ERROR_MAX_TRIES      = 0x5,
    HOMEKIT_TLV_ERROR_UNAVAILABLE    = 0X6,
    HOMEKIT_TLV_ERROR_BUSY           = 0X7,
    HOMEKIT_TLV_ERROR_RESERVED,
} wiced_homekit_tlv_error_codes_t;

typedef enum wiced_homekit_generic_event
{
    HOMEKIT_GENERIC_EVENT_PAIRING_REQUESTED,
    HOMEKIT_GENERIC_EVENT_PAIRING_FAILED,
    HOMEKIT_GENERIC_EVENT_PAIRED,
    HOMEKIT_GENERIC_EVENT_REMOVE_PAIRING_REQUESTED,
    HOMEKIT_GENERIC_EVENT_REMOVE_PAIRING_FAILED,
    HOMEKIT_GENERIC_EVENT_PAIRING_REMOVED,
    HOMEKIT_GENERIC_EVENT_CONTROLLER_CONNECTED,
    HOMEKIT_GENERIC_EVENT_CONTROLLER_DISCONNECTED,
    NO_EVENT,
} wiced_homekit_generic_event_t;
/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef wiced_result_t (*wiced_homekit_display_password_callback_t)( uint8_t* accessory_password );

typedef wiced_result_t (*wiced_homekit_generic_callback_t)( void );

typedef wiced_result_t (*wiced_homekit_identify_callback_t)( uint16_t accessory_id, uint16_t characteristic_id );

typedef wiced_result_t (*wiced_homekit_state_number_external_store_callback_t)( uint32_t* state_number, wiced_homekit_external_state_number_storage_request_t store_request_type );

/******************************************************
 *                    Structures
 ******************************************************/
#pragma pack(1)

typedef struct
{
    uint16_t                   accessory_index;
    uint16_t                   characteristic_index;
} wiced_homekit_index_list_t;

typedef struct
{
    wiced_homekit_index_list_t update_list[HOMEKIT_MAX_UPDATES];
    uint8_t                    number_of_updates;
} wiced_homekit_update_list_t;

typedef struct
{
    uint16_t     accessory_instance_id[MAX_READ_REQUESTS];
    uint16_t     characteristic_instance_id[MAX_READ_REQUESTS];
    wiced_bool_t meta;
    wiced_bool_t perms;
    wiced_bool_t type;
    wiced_bool_t ev;
    wiced_bool_t status;
} wiced_homekit_characteristic_read_parameters_t;

typedef wiced_result_t (*wiced_homekit_characteristic_read_callback_t)( wiced_homekit_characteristic_read_parameters_t* read_params, wiced_http_response_stream_t* stream );

typedef struct
{
    uint16_t accessory_instance_id;
    uint16_t characteristic_instance_id;
    uint16_t value_length;
    char*    value;
} wiced_homekit_characteristic_descriptor_t;

typedef struct
{
    wiced_homekit_characteristic_descriptor_t characteristic_descriptor[ HOMEKIT_MAX_UPDATES ];
    uint8_t                                   number_of_updates;
} wiced_homekit_characteristic_value_update_list_t;

typedef wiced_result_t (*wiced_homekit_characteristic_update_callback_t)( wiced_homekit_characteristic_value_update_list_t* update_list );

typedef wiced_result_t (*wiced_homekit_characteristic_write_callback_t)( wiced_homekit_characteristic_value_update_list_t* update_list, wiced_http_response_stream_t* stream );

typedef struct wiced_homekit_generic_event_info
{
    wiced_homekit_generic_event_t   type;
    union event_data
    {
        wiced_homekit_tlv_error_codes_t error_code;     /* Use to return HomeKit TLV8 error in failure cases */
        uint8_t controller_id;                          /* Use to identify the controller for generic events */
    }info;
} wiced_homekit_generic_event_info_t;

typedef wiced_result_t (*wiced_homekit_accessory_generic_event_callback_t)( wiced_homekit_generic_event_info_t homekit_generic_event_info );

typedef wiced_result_t (*wiced_homekit_read_persistent_data_callback_t) ( void* data_ptr, uint32_t offset, uint32_t data_length );

typedef wiced_result_t (*wiced_homekit_write_persistent_data_callback_t)( void* data_ptr, uint32_t offset, uint32_t data_length );

typedef struct
{
  wiced_http_response_stream_t*  controller_id[HOMEKIT_MAX_ACTIVE_CONNECTIONS];
} wiced_homekit_controller_id_list_t;

typedef struct
{
    linked_list_node_t accessory_node;
    linked_list_t      services_list;
}wiced_homekit_accessories_private_data_t;

typedef struct
{
    wiced_bool_t       custom_service;
    linked_list_t      characteristics_list;
    linked_list_node_t services_node;
}wiced_homekit_services_private_data_t;

typedef struct wiced_homekit_value_descriptor
{
    uint8_t  name_length;
    uint16_t value_length;
    char*    name;
    char*    value;
    struct wiced_homekit_value_descriptor* next;
}wiced_homekit_value_descriptor_t;

typedef struct
{
    wiced_homekit_value_descriptor_t  current;
    wiced_homekit_value_descriptor_t  new;
} wiced_homekit_value_t;

#ifdef HOMEKIT_ICLOUD_ENABLE
typedef struct
{
  uint8_t  contorller_relay_id[ HOMEKIT_ICLOUD_CONTROLLER_IDENTIFIER_LENGTH ];
  uint8_t  contorller_access_token[ HOMEKIT_RELAY_ACCESS_TOKEN_MAX_LEN ];
  uint8_t  contorller_access_token_len;
} wiced_homekit_recipient_list_t;
#endif

typedef struct
{
    wiced_bool_t                       value_updated;
    wiced_bool_t                       event_updated;
    wiced_bool_t                       custom_characteristic;
    wiced_bool_t                       no_value;              /* null value characteristic */
    wiced_bool_t                       required_characteristic; /* Is this an optional or required characteristic */
    linked_list_node_t                 characteristics_node;
    wiced_homekit_controller_id_list_t controller_event_list; /* List of all controllers registered to receive events */
#ifdef HOMEKIT_ICLOUD_ENABLE
    wiced_homekit_recipient_list_t     icloud_controller_event_list[HOMEKIT_MAX_ACTIVE_CONNECTIONS];  /* List of controllers register to receive events through iCloud */
#endif
    wiced_homekit_error_code_t         status;                /* only used to indicate errors reception errors */
}wiced_homekit_characteristics_private_data_t;

typedef struct
{
    uint16_t                              instance_id;
    const char*                           type;
    wiced_homekit_value_t                 value;
    wiced_homekit_identify_callback_t     identify_callback;
    wiced_homekit_permissions_flag_t      permissions;
    const char*                           description;
    wiced_homekit_format_t                format;
    wiced_homekit_unit_t                  unit;
    float                                 minimum_value;
    float                                 maximum_value;
    const char*                           minimum_step;
    int                                   maximum_length;
    int                                   maximum_data_length;
    char*                                 authData;
    uint32_t                              authData_length;
    wiced_bool_t                          remote;
    wiced_bool_t                          event;
    wiced_homekit_characteristics_private_data_t  private_data; /* Do not modify this member */
} wiced_homekit_characteristics_t;



typedef struct
{
    const char*                           type;
    uint16_t                              instance_id;
    wiced_homekit_characteristics_t*      characteristics;
    wiced_bool_t                          primary_service;
    wiced_bool_t                          hidden_service;
    linked_list_t                         linked_services_list;
    wiced_homekit_services_private_data_t private_data;   /* Do not modify this member */
} wiced_homekit_services_t;

typedef struct
{
    uint16_t                                 instance_id;
    wiced_bool_t                             tunneled;       /* TRUE: tunneled accessory */
    wiced_homekit_services_t*                service;
    wiced_homekit_accessories_private_data_t private_data;   /* Do not modify this member */
} wiced_homekit_accessories_t;

typedef struct
{
    char*                           name;
    char*                           mfi_protocol_version_string;
    wiced_mfi_feature_flags         mfi_config_features;
    wiced_mfi_configure_flags       mfi_config_flags;
    accessory_category_identifier_t accessory_category_identifier;
    char                            device_id[STRING_MAX_MAC];
    wiced_interface_t               interface;
} apple_homekit_accessory_config_t;

typedef struct
{
    char* name;
    char* model;
    char* manufacturer;
    char* serial_number;
    wiced_homekit_generic_callback_t identify_callback;
} wiced_homekit_accessory_information_service_t;

typedef struct
{
   uint8_t wiced_homekit_dct_buf[ WICED_HOMEKIT_DCT_BUFFER_SIZE ];
} wiced_homekit_dct_space_t;

#pragma pack()

typedef struct
{
    int status;
    uint16_t    accessory_id;
    uint16_t    instance_id;
    uint8_t     data_length;
    uint8_t*    data;
} wiced_homekit_response_data_t;

typedef uint32_t (*wiced_homekit_write_accessory_database_callback_t)( wiced_homekit_accessories_t* accessory, uint8_t* send_buffer, uint32_t offset );

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/*****************************************************************************/
/**
 *  @addtogroup mfi_homekit     HomeKit
 *  @ingroup mfi
 *
 *  @addtogroup mfi_homekit_core    Core
 *  @ingroup mfi_homekit
 *
 * HomeKit is the home automation protocol/framework developed by Apple for controlling accessories
 * connected to the home network.
 *
 * This module implements Apple HomeKit protocol for developing HomeKit enabled Wi-Fi accessories.
 *
 *  @{
 */
/*****************************************************************************/
/**
 * Initializes the components required for the library and starts HomeKit accessory server.
 *  - Allocates the memory required for storing accessory database.
 *  - Starts HTTP/1.1 server.
 *  - Register and advertise HomeKit mDNS service.
 *
 * @param   homekit_accessory_info [in]      :   Accessory configuration information
 * @param   heap_allocated_for_homekit [out] :   Pointer to a variable used for storing the size of Heap which allocated for the accessory database.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_start( apple_homekit_accessory_config_t* homekit_accessory_info, uint32_t* heap_allocated_for_homekit );

/**
 * Tears down HomeKit accessory server.
 *  - Stops HTTP/1.1 server.
 *  - Unregister HomeKit mDNS service.
 *  - Free the memory allocated for accessory database.
 *
 * @return @ref wiced_result_t
 *
 */
wiced_result_t wiced_homekit_stop( void );

/**
 * Registers the callback function for pairing password. The callback function is invoked when accessory receives pairing request from controller.
 *
 * NOTE: This function should be used only when accessory has a display and want to generate dynamic pairing password instead of using a static pairing password.
 *
 * @param   callback [in] : The callback function to be invoked with the dynamic pairing password, when controller request a paring with accessory.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_configure_accessory_password_for_device_with_display( wiced_homekit_display_password_callback_t callback );

/**
 * Assigns fixed pairing password for the accessory.
 *
 * NOTE:
 *       Password should be in format : XXX-XX-XXX
 *          where, X is a digit between 0-9
 *       The following passwords are invalid and should not be used:
 *         000-00-000
 *         111-11-111
 *         222-22-222
 *         333-33-333
 *         444-44-444
 *         555-55-555
 *         666-66-666
 *         777-77-777
 *         888-88-888
 *         999-99-999
 *         123-45-678
 *         876-54-321
 *
 * @param   password [in] : Pairing password string in the required format which is a 'null' terminated string
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_configure_accessory_password_for_device_with_no_display( char* password);

/**
 * Find accessory details for the given accessory ID
 *
 * @param   accessory_id [in] : Accessory ID
 * @param   accessory [out]   : Pointer to accessory details (for the given accessory ID) which is stored on return
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_find_accessory_with_instance_id( uint16_t accessory_id, wiced_homekit_accessories_t** accessory );

/**
 * Find characteristic details that belongs to given accessory ID and characteristic ID
 *
 * @param   accessory_id [in]      : Accessory ID
 * @param   characteristic_id [in] : Characteristic ID
 * @param   characteristic [out]   : Pointer to characteristic details (for the given accessory ID and characteristic ID) which is stored on return
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_find_characteristic_with_instance_id( uint16_t accessory_id, uint16_t characteristic_id, wiced_homekit_characteristics_t** characteristic );

/**
 * Register callback function to be invoked when controller request accessory to identify itself
 *
 * @param   identify_callback [in] :  Callback to be invoked when identify request is issued by controller
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_register_url_identify_callback( wiced_homekit_identify_callback_t identify_callback );

/**
 * Notify accessory characteristic changes to the controllers. Only the controllers which has registered for event notification (with the accessory) would get the characteristic value change notifications.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_send_all_updates_for_accessory( void );

/**
 * Register the accessory object to which service will be attached.
 *
 * The reference to accessory object is maintained in the library till the remove accessory API is invoked.
 *
 * @param   accessory [in]    : Accessory to be registered
 * @param   accessory_id [in] : Accessory ID
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_add_accessory( wiced_homekit_accessories_t* accessory, uint16_t acessory_id );

/**
 * Removes the given accessory (i.e., all the references to the given accessory object is removed/cleared in the library)
 *
 * If this accessory was malloced by the caller, it's callers responsibility to free the object.
 *
 * @param   accessory [in] : Accessory to be removed
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_remove_accessory( wiced_homekit_accessories_t* accessory );

/**
 * Add service to the accessory.
 *
 * The service object should be initialized before getting passed to this function.
 *
 * @param   accessory [in] : The accessory to which service will be attached
 * @param   service [in]   : The service to be attached to the accessory
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_add_service( wiced_homekit_accessories_t* accessory, wiced_homekit_services_t* service );

/**
 * Remove service from the given accessory.
 *
 * Make sure this service was added to the accessory before. And it's part of the accessory.
 * If this service was malloced by the caller, it's callers responsibility to free the object.
 *
 * @param   accessory [in] : The accessory from which service to be removed
 * @param   service [in]   : The service to be removed from the accessory
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_remove_service( wiced_homekit_accessories_t* accessory, wiced_homekit_services_t* service );

/**
 * Add characteristic to the specified service.
 *
 * The characteristic object and service object should be initialized before getting passed to this function.
 *
 * @param   service [in]        : The service to which the characteristic to be attached
 * @param   characteristic [in] : The characteristic to be attach to the service
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_add_characteristic( wiced_homekit_services_t* service, wiced_homekit_characteristics_t* characteristic );

/**
 * Remove characteristic from a specified service.
 *
 * Make sure the characteristic was added to the service before. And it's part of the service.
 * If this characteristic was malloced by the caller, it's callers responsibility to free the object.
 *
 * @param   service [in]        : The service from which the characteristic to be removed
 * @param   characteristic [in] : The characteristic to be removed from the service
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_remove_characteristic( wiced_homekit_services_t* service, wiced_homekit_characteristics_t* characteristic );

/**
 * Registers the callback functions for tunneled accessory.
 *
 * @param   database_callback [in] : The callback to be invoked when controller reads accessory database
 * @param   read_callback [in]     : The callback to be invoked when controller reads accessory characteristic
 * @param   write_callback [in]    : The callback to be invoked when controller writes accessory characteristic
 * @param   event_callback [in]    : The callback to be invoked when accessory's characteristic event value changes
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_register_tunneled_accessory_callbacks( wiced_homekit_write_accessory_database_callback_t database_callback,
        wiced_homekit_characteristic_read_callback_t read_callback, wiced_homekit_characteristic_write_callback_t write_callback,
        wiced_homekit_characteristic_write_callback_t event_callback);

/**
 * Registers the callback function to be invoked when controller changes accessory characteristics value
 *
 * @param   value_callback [in] : The callback to be invoked when a accessory's characteristic value is changed by the controller
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_register_value_update_callback( wiced_homekit_characteristic_update_callback_t value_callback );

/**
 * Registers / Unregisters the callback function for generic events. ( @ref wiced_homekit_generic_event_t )
 *
 * @param   generic_event_callback [in] : The callback function to be invoked for generic events. If NULL, it unregisters the callback
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_register_generic_event_callback( wiced_homekit_accessory_generic_event_callback_t generic_event_callback );

/**
 * This function is used for registering the changes made to accessory characteristic value
 *
 * The characteristic value changes are added to the internal value update list. And the controller who has enabled event for the characteristic would
 * receive the notification when @ref wiced_homekit_send_all_updates_for_accessory() function is invoked.
 *
 * NOTE: This updates the memory location "value.current" associated to this characteristic during initialization. New value should fit in this memory.
 *
 * @param   accessory [in]       : Pointer to accessory in which the characteristic belongs to
 * @param   characteristics [in] : Pointer to characteristic that has been updated
 * @param   value [in]           : New value of characteristic
 * @param   value_length [in]    : Length of 'value', in bytes
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_register_characteristic_value_update( wiced_homekit_accessories_t* accessory, wiced_homekit_characteristics_t* characteristic, char* value, uint8_t value_length );

/**
 * Updates the current characteristics value using the new characteristic value
 *
 * Use this to write the new controller value (value.new) to current value (value.current).
 *
 * @param   characteristic [in] : Pointer to the characteristic in which the value to be updated
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_accept_controller_value( wiced_homekit_characteristics_t* characteristic );

/**
 * Returns the size of the memory allocated for accessory database
 *
 * @return Accessory database size, in bytes
 */
uint32_t wiced_homekit_get_current_accessory_database_size( void );

/**
 * Clears the information maintained in persistent storage by the library
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_clear_homekit_dct( void );

/**
 * Recalculate and reallocate accessory database after accessory/service(s)/characteristic(s) is/are added or removed
 *
 * @return Accessory database size, in bytes after reallocation
 */
uint32_t wiced_homekit_recalculate_accessory_database( void );

/**
 * Sends response for read characteristics
 *
 * @param   stream [in]          : HTTP stream to be used for sending the read characteristic response
 * @param   read_parameters [in] : Read characteristic's parameters
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_send_read_characteristic_response( wiced_http_response_stream_t* stream, wiced_homekit_characteristic_read_parameters_t* read_parameters );

/**
 * API to send read/write/config/event responses from app
 *
 * @param   stream [in]           : HTTP stream to be used for sending the response
 * @param   type [in]             : Response type (read/write/config/event)
 * @param   response_data [in]    : Array of response data
 * @param   num_of_responses [in] : Number of responses
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_send_responses ( wiced_http_response_stream_t* stream, wiced_homekit_response_type_t type, wiced_homekit_response_data_t response_data[], uint8_t num_of_responses );

/**
 * API to disconnect all HAP controllers
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_disconnect_all_controllers( void );

/**
 * Register the callback function to be called by HomeKit library to manage (store/retrieve) the library persistent data.
 *
 * @param   read_callback [in]  : Callback function to be used to read the persistent data.
 * @param   write_callback [in] : Callback function to be used to write the persistent data.

 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_register_persistent_data_handling_callback( wiced_homekit_read_persistent_data_callback_t read_callback, wiced_homekit_write_persistent_data_callback_t write_callback );

/**
 * Add relay service to the accessory.
 *
 * @param   accessory [in]       : The accessory to which relay service to be added
 * @param   instance_id [in]     : Instance ID start value to be assigned for the relay service. homekit library will internally assign 3 instance IDs following instance_id for the relay characteristics. The instance ID should be unique across services and characteristics for the given accessory, valid range is from 1 to 65535.
 * @param   rootca_cert [in]     : Root CA certificate of HomeKit iCloud courier server
 * @param   rootca_cert_len [in] : Length of the Root CA certificate
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_add_relay_service( wiced_homekit_accessories_t* accessory, uint16_t instance_id, const char* rootca_cert, uint32_t rootca_cert_len );

/**
 * Remove relay service from the accessory.
 *
 * @param   accessory [in] : The accessory from which relay service to be removed
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_remove_relay_service( wiced_homekit_accessories_t* accessory );

/**
 * The service to which the other service to be linked
 *
 * @param   service [in]        : The service to which the other services to be linked
 *
 * @param   link_service [in]   : The service which is getting linked
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_link_services(wiced_homekit_services_t* service, wiced_homekit_services_t* link_service);


/**
 * Set primary service in an accessory
 *
 * @param   accessory [in]       : The accessory for which service will be set as primary
 *
 * @param   primary_service [in] : The service which needs to be set as primary
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_service_set_primary( wiced_homekit_accessories_t* accessory, wiced_homekit_services_t* primary_service);


/**
 * Set/clear hidden service in an accessory
 *
 * @param   accessory [in]      : The accessory for which service will be set as hidden
 *
 * @param   hidden_service [in] : The service which needs to be set as hidden
 *
 * @return @ref wiced_result_t
 *
 */
wiced_result_t wiced_homekit_service_set_hidden(wiced_homekit_services_t* service, wiced_bool_t hidden_service);

/**
 * Set configuration number. And also notifies the configuration number changes to controllers if homekit library is already running.
 *
 * @param   config_number [in]  : Configuration number to be set. Valid range : 1 to 4294967295.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_set_configuration_number(uint32_t config_number);

/**
 * Get current configuration number.
 *
 * @param   config_number [out] : Pointer to the variable for storing the configuration number being read.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_homekit_get_configuration_number(uint32_t* config_number);

/** @} */

#ifdef __cplusplus
} /*extern "C" */
#endif
