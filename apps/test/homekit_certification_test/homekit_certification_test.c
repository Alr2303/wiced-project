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
 * HomeKit Certification Test
 *
 * This certification test application allows all characteristic values on the board to be read and written to via
 * a console type application.
 *
 * It also lists all services and characteristics present on the board.
 *
 * The test application has 2 build options:
 *    - MFi mode
 *    - Non-MFi mode
 *
 *     MFi Mode
 *        This uses WAC to start a soft AP and bring the Wiced HomeKit Bridge Accessory onto the network
 *        It also advertises MFi support, and uses MFi certificate during the pairing process
 *        to prove it is an MFi device.
 *
 *        Build string in this mode:
 *          a) With OTA support on platform:
 *              test.homekit_certification_test-BCM943909WCD1_3 USE_MFI=1 download download_apps run
 *          b) With OTA2 support on platform:
 *              test.homekit_certification_test-BCM943909WCD1_3 USE_MFI=1 download ota2_factory_download run
 *          c) With fixed setup-code support:
 *              test.homekit_certification_test-BCM943909WCD1_3 USE_MFI=1 USE_FIXED_SETUP_CODE=1 download run
 *
 *     Non-MFi Mode
 *        This uses the wifi credentials stored in wifi_config_dct.h to bring up the Wiced HomeKit Bridge Accessory
 *        This advertises pin only support, and does not use MFi certificate during pairing
 *
 *        Build string in this mode:
 *          a) With OTA support on platform:
 *              test.homekit_certification_test-BCM943909WCD1_3 download download_apps run
 *          b) With OTA2 support on platform:
 *              test.homekit_certification_test-BCM943909WCD1_3 download ota2_factory_download run
 *          c) With fixed setup-code support:
 *              test.homekit_certification_test-BCM943909WCD1_3 USE_FIXED_SETUP_CODE=1 download run
 *
 * Once the Wiced HomeKit Certification Accessory is on the network, it can be discovered by the HAT application or iPhone/iPad app.
 * From here you should be able to perform a pair setup and perform a discovery of all the services
 * and characteristics.
 *
 *
 * Application Instructions
 *   1. If running non-MFi mode, modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials
 *      in the wifi_config_dct.h header file to match your Wi-Fi access point
 *
 *   2. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *
 *   3. After the download completes, the terminal displays WICED startup
 *      information and starts WICED Configuration Mode.
 *
 *      If running MFi mode, use the AirPort utility on the MAC (or  iPhone/iPad ), to bring up the Wiced HomeKit Accessory
 *
 * Wiced HomeKit Bridge Certification Accessory Demonstration
 *   To demonstrate the Wiced HomeKit Certification Bridge Accessory, start the HAT application once the Wiced HomeKit Bridge Accessory has joined
 *   your local network.
 *   - In the HAT application, click on the "Start Pairing" button, and wait for the "Accessory Password" to be shown
 *     on the serial output.
 *   - Enter the password shown on the display ( including the dashes )
 *   - Once the pairing is complete, click on the "Discover" button to perform a 'Pair-Verify' and get a listing of all
 *     services and characteristics for the accessory.
 *   - You can now modify a characteristic value which has read/write permissions
 *   - To add multiple controllers, perform the following steps:
 *     1. In the HAT application, click on the bottom right '+' sign and add another IP device
 *     2. For this controller, press "Start" to start listening for HomeKit accessories
 *     3. Go to the paired admin controller, and select the newly created IP device, then select "Add Pairing"
 *     4. Once the pairing has been added, you can return to the added controller (IP device), and begin discovery.
 *   - To enable events, go to the 'on' characteristic and press the 'enable' button next to "Event Notifications"
 *   - To send an event back to the controller, toggle D1 LED on the eval board by pressing SW1. This should send
 *     back either an on or off event
 *
 * Controlling Accessory through the command console
 *  A command console has been added to make local modifications to characteristic values/states on the eval board.
 *  Typing help, will give you a list of commands, and their usage.
 *
 *  A brief description of commands is given here:
 *
 *  list_accessories                                                           : Command takes no arguments and lists all the accessories present on the eval board
 *  list_services                  <accessory id>                              : Command takes one argument, the accessory instance ID, and lists all services that belong to it
 *  list_characteristics           <accessory id> <service id>                 : Command takes two arguments, accessory and service id, and lists all characteristics that belong to the specified service
 *  read_characteristic            <accessory id> <characteristic id>          : Command takes two arguments, accessory and characteristics instance id, and reads the value of the characteristic
 *  write_characteristic           <accessory id> <characteristic id> <value>  : Command takes three arguments, the accessory and characteristics instance id, and the value to update the specified characteristic to
 *  remove_characteristic          <accessory id> <characteristic id>          : Command takes two arguments, accessory and characteristic id, and removes the characteristic from its service
 *  return_removed_characteristics <accessory id> <characteristic id>          : Command takes two argument, the accessory and characteristic id, and returns removed characteristics under given accessory
 *  remove_service                 <accessory id> <service id>                 : Command takes two arguments, accessory and service id, and removes the service from the accessory
 *  return_removed_services        <accessory id> <service id>                 : Command takes two argument, the accessory and service id, and return removed services for given accessory
 *  clear_pairings                                                             : Command takes no arguments and clears all pairings from DCT
 *  factory_reset                                                              : Command takes no arguments and set the factory default configuration in DCT while OTA2 support is enabled
 *  enable_low_power_mode                                                      : Command takes no arguments and enables wifi power save mode.
 *  disable_low_power_mode                                                     : Command takes no arguments and disables wifi power save mode.
 *
 * NOTES:
 *   The HomeKit specification imposes some restrictions on the usage of the accessory, which may not be intuitive.
 *   Please keep these points in mind when exploring the Wiced HomeKit Accessory snippet.
 *
 *   1. One can not remove a pairing unless the device has gone through "Pair-Verify", which is part of the Discover
 *      process. This means once 'Pair Setup' is complete, the pairing can not be removed until the Services of the accessory have
 *      been discovered.
 *
 *   2. Only controllers which are admin controllers can perform add/remove pairings ( the first controller to pair automatically
 *      becomes an admin controller )
 *
 *   3. Once a controller is paired, no other controllers can go through pair setup. The only way to bring add another controller
 *      will be through Add/Remove pairing from the admin controller.
 *
 *   4. Controllers are persistently stored. This means if you reset HAT application, without removing the pairing beforehand,
 *      the appliance will still think it is paired to the controller. In this case you must perform a factory reset by downloading
 *      the firmware again.
 *
 *   5. HomeKit iOS9 defined services and characteristics are ONLY enable for BCM94390x platforms.
 *
 *   6. Only restricted list of characteristics and services are enabled in this application for BCM9WCDPLUS114 platform.
 *      This is done due to the SRAM size limitation on the platform.
 *
 */

#include "wiced.h"
#include "apple_homekit.h"
#include "apple_homekit_characteristics.h"
#include "apple_homekit_services.h"
#include "apple_homekit_developer.h"
#include "command_console.h"
#include "apple_homekit_dct.h"
#include "homekit_certification_test_dct.h"
#include "base64.h"
#include "platform/wwd_bus_interface.h"
#include "wiced_deep_sleep.h"
#include "wiced_crypto.h"

#ifdef OTA2_SUPPORT
#include "wiced_ota2_image.h"
#endif

#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI
#include "apple_wac.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#ifdef USE_FIXED_SETUP_CODE
#define FIXED_SETUP_CODE                "606-34-410"
#endif

#ifdef HOMEKIT_ICLOUD_ENABLE
#define APPLE_CA_CERTIFICATE_FOR_ICLOUD_SERVER        ""    // Root CA certificate issued by Apple should used here
#endif

#define COMPARE_MATCH                    0

#define TEST_COMMAND_LENGTH             (256)
#define TEST_COMMAND_HISTORY_LENGTH     (10)
#define MAX_NAME_LENGTH                 (64 + 2 + 1) /* Max. Length of Name + 2 - For double quotes + 1 - null Character */

#define LIST_HOMEKIT_SERVICES_COMMAND_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 1,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes one argument, the accessory instance id. Eg: \"list_services 2\"",\
          .brief        = "Lists all services for an Accessory with a given Instance ID"\
        },

#define LIST_HOMEKIT_ACCESSORY_MODE_COMMAND_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 0,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes no arguments. Eg: \"accessory_mode\"",     \
          .brief        = "Lists the accessory modes"\
        },

#define CONFIGURE_HOMEKIT_ACCESSORY_MODE_COMMAND_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 0,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes 1 argument. Eg: \"configure_accessory_mode 1\"",     \
          .brief        = "Configures the accessory to the specified mode"\
        },

#define LIST_HOMEKIT_ACCESSORIES_COMMAND_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 0,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes no arguments. Eg: \"list_accessories\"",     \
          .brief        = "Lists all the accessories present on the board"\
        },

#define LIST_HOMEKIT_CHARACTERISTICS_COMMAND_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 2,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes 2 argument: Accessory ID and Service ID Eg: \"list_characteristics 1 2\"",     \
          .brief        = "Lists all the characteristics for a given service on a given accessory"\
        },

#define READ_HOMEKIT_CHARACTERISTIC_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 2,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes 2 argument: Accessory ID and Characteristics ID Eg: \"read_characteristics 1 2\"",\
          .brief        = "Read characteristic specified by its Accessory ID and Characteristics instance ID"\
        },

#define WRITE_HOMEKIT_CHARACTERISTIC_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 3,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes 3 argument: Accessory ID, Characteristics ID and the value to write. Eg: \"write_characteristics 1 2 1\"",\
          .brief        = "Write characteristic specified by its Accessory ID and Characteristics instance ID using the given value"\
        },

#define CLEAR_ALL_PAIRINGS_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 0,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes no arguments. Eg: \"clear_pairings\"",\
          .brief        = "Clear all pairings on device"\
        },

#define REMOVE_HOMEKIT_CHARACTERISTIC_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 2,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes 2 argument: Accessory ID, Characteristic ID of characteristic to remove. Eg: \"remove_characteristic 1 2\"",\
          .brief        = "Remove a characteristic from a given accessory service"\
        },

#define RETURN_HOMEKIT_CHARACTERISTIC_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 2,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes 2 argument: Accessory ID and Characteristic ID of which characteristics were removed with \"remove_characteristics\" will be returned. Eg: \"return_removed_characteristics 1 26\"",\
          .brief        = "Return a characteristic previsouly removed from a given accessory service"\
        },

#define REMOVE_HOMEKIT_SERVICE_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 2,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes 2 argument: Accessory ID, Service ID of service to remove. Eg: \"remove_service 1 2\"",\
          .brief        = "Remove a service from a given accessory"\
        },

#define RETURN_HOMEKIT_SERVICE_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 2,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes 2 argument: Accessory ID and Service ID. Removed services with \"remove_service\" will be returned. Eg: \"return_removed_services 1 4 \"",\
          .brief        = "Remove a service from a given accessory"\
        },

#ifdef OTA2_SUPPORT
#define FACTORY_RESET_ON_REBOOT(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 0,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes no arguments. Eg: \"factory_reset\"",\
          .brief        = "Does factory reset on reboot"\
        },
#endif

#define ENABLE_LOW_POWER_MODE(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 0,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes no arguments. Eg: \"enable_low_power\"",\
          .brief        = "Enables WiFi Power Save mode"\
        },

#define DISABLE_LOW_POWER_MODE(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 0,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = "Command takes no arguments. Eg: \"disable_low_power\"",\
          .brief        = "Disables WiFi Power Save mode"\
        },

/******************************************************
 *                    Constants
 ******************************************************/
#define ACCESSORY_INFORMATION_SERVICE_NAME                "Accessory Information Service"
#define GARAGE_DOOR_OPENER_SERVICE_NAME                   "Garage Door Opener Service"
#define THERMOSTAT_SERVICE_NAME                           "Thermostat Service"
#define LIGHTBUBL_SERVICE_NAME                            "Lightbulb Service"
#define FAN_SERVICE_NAME                                  "Fan Service"
#define OUTLET_SERVICE_NAME                               "Outlet Service"
#define SWITCH_SERVICE_NAME                               "Switch Service"
#define LOCK_MECHANISM_SERVICE_NAME                       "Lock Mechanism Service"
#define LOCK_MANAGEMENT_SERVICE_NAME                      "Lock Management Service"

#ifdef IOS9_ENABLED_SERVICES
#define AIR_QUALITY_SENSOR_SERVICE_NAME                   "Air Quality Sensor Service"
#define SECURITY_SYSTEM_SERVICE_NAME                      "Security System Service"
#define CARBON_MONOXIDE_SENSOR_SERVICE_NAME               "Carbon Monoxide Sensor Service"
#define CONTACT_SENSOR_SERVICE_NAME                       "Contact Sensor Service"
#define DOOR_SERVICE_NAME                                 "Door service"
#define HUMIDITY_SENSOR_SERVICE_NAME                      "Humidity Sensor Service"
#define LEAK_SENSOR_SERVICE_NAME                          "Leak Sensor Service"
#define LIGHT_SENSOR_SERVICE_NAME                         "Light Sensor Service"
#define MOTION_SENSOR_SERVICE_NAME                        "Motion Sensor Service"
#define OCCUPANCY_SENSOR_SERVICE_NAME                     "Occupancy Sensor Service"
#define SMOKE_SENSOR_SERVICE_NAME                         "Smoke Sensor Service"
#define TEMPERATURE_SENSOR_SERVICE_NAME                   "Temperature Sensor Service"
#define WINDOW_SERVICE_NAME                               "Window Service"
#define WINDOW_COVERING_SERVICE_NAME                      "Window Covering Service"
#define BATTERY_SERVICE_NAME                              "Battery Service"
#define CARBON_DIOXIDE_SENSOR_SERVICE_NAME                "Carbon Dioxide Sensor Service"
#define STATEFUL_PROGRAMMABLE_SWITCH_SERVICE_NAME         "Stateful Programmable Switch Service"
#define STATELESS_PROGRAMMABLE_SWITCH_SERVICE_NAME        "Stateless Programmable Switch Service"
#endif

#define IDENTIFY_CHARACTERISTIC                           "Identify Characteristic"
#define MANUFACTURER_CHARACTERISTIC                       "Manufacturer Characteristic"
#define MODEL_CHARACTERISTIC                              "Model Characteristic"
#define NAME_CHARACTERISTIC                               "Name Characteristic"
#define SERIAL_NUMBER_CHARACTERISTIC                      "Serial Number Characteristic"

#define CURRENT_DOOR_STATE_CHARACTERISTIC                 "Current Door State Characteristic"
#define TARGET_DOOR_STATE_CHARACTERISTIC                  "Target Door State Characteristic"
#define OBSTRUCTION_DETECTED_CHARACTERISTIC               "Obstruction Detected Characteristic"
#define LOCK_MECHANISM_CURRENT_STATE_CHARACTERISTIC       "Lock Mechanism Current State Characteristic"
#define LOCK_MECHANISM_TARGET_STATE_CHARACTERISTIC        "Lock Mechanism Target State Characteristic"

#define CURRENT_HEATING_COOLING_MODE_CHARACTERISTIC       "Heating Cooling Current Mode Characteristic"
#define TARGET_HEATING_COOLING_MODE_CHARACTERISTIC        "Heating Cooling Target Mode Characteristic"
#define TEMPERATURE_CURRENT_CHARACTERISTIC                "Temperature Current Characteristic"
#define TEMPERATURE_TARGET_CHARACTERISTIC                 "Temperature Target Characteristic"
#define TEMPERATURE_UNIT_CHARACTERISTIC                   "Temperature Unit Characteristic"

#define ON_CHARACTERISTIC                                 "On Characteristic"
#define HUE_CHARACTERISTIC                                "Hue Characteristic"
#define BRIGHTNESS_CHARACTERISTIC                         "Brightness Characteristic"
#define SATURATION_CHARACTERISTIC                         "Saturation Characteristic"
#define NAME_CHARACTERISTIC                               "Name Characteristic"

#define OUTLET_IN_USE_CHARACTERISTIC                      "Outlet In Use Characteristic"

#define CONTROL_POINT_CHARACTERISTIC                      "Control Point Characteristic"
#define VERSION_CHARACTERISTIC                            "Version Characteristic"

#ifdef IOS9_ENABLED_SERVICES
#define FIRMWARE_REVISION_CHARACTERISTIC                  "Firmware Revision Characteristic"
#define AIR_QUALITY_CHARACTERISTIC                        "Air Quality Characteristic"
#define LEAK_DETECTED_CHARACTERISTIC                      "Leak Detected Characteristic"
#define MOTION_DETECTED_CHARACTERISTIC                    "Motion Detected Characteristic"
#define OCCUPANCY_DETECTED_CHARACTERISTIC                 "Occupancy Detected Characteristic"
#define SMOKE_DETECTED_CHARACTERISTIC                     "Smoke Detected Characteristic"

#define SECURITY_SYSTEM_CURRENT_STATE_CHARACTERISTIC      "Security System Current State Characteristic"
#define SECURITY_SYSTEM_TARGET_STATE_CHARACTERISTIC       "Security System Target State Characteristic"

#define CARBON_MONOXIDE_DETECTED_CHARACTERISTIC           "Carbon Monoxide Detected Characteristic"
#define CARBON_DIOXIDE_DETECTED_CHARACTERISTIC            "Carbon Dioxide Detected Characteristic"
#define CONTACT_SENSOR_STATE_CHARACTERISTIC               "Contact Sensor State Characteristic"
#define CURRENT_RELATIVE_HUMIDITY_CHARACTERISTIC          "Current Relative Humidity Characteristic"
#define CURRENT_LIGHT_LEVEL_CHARACTERISTIC                "Current Light Level Characteristic"

#define CURRENT_POSITION_CHARACTERISTIC                   "Current Position Characteristic"
#define TARGET_POSITION_CHARACTERISTIC                    "Target Position Characteristic"
#define POSITION_STATE_CHARACTERISTIC                     "Position State Characteristic"

#define BATTERY_LEVEL_CHARACTERISTIC                      "Battery Level Characteristic"
#define CHARGING_STATE_CHARACTERISTIC                     "Charging State Characteristic"
#define STATUS_LOW_BATTERY_CHARACTERISTIC                 "Status Low Battery Characteristic"

#define PROGRAMMABLE_SWITCH_EVENT_CHARACTERISTIC         "Programmable Switch Event Characteristic"
#define PROGRAMMABLE_SWITCH_OUTPUT_STATE_CHARACTERISTIC  "Programmable Switch Output State Characteristic"

#define SECURITY_SYSTEM_TARGET_STATE_STAY_ARM            '0'
#define SECURITY_SYSTEM_TARGET_STATE_AWAY_ARM            '1'
#define SECURITY_SYSTEM_TARGET_STATE_NIGHT_ARM           '2'
#define SECURITY_SYSTEM_TARGET_STATE_DISARM              '3'
#endif

#define LOCK_MECHANISM_TARGET_STATE_UNSECURED_LOCK       '0'
#define LOCK_MECHANISM_TARGET_STATE_SECURED_LOCK         '1'
#define HEATING_COOLING_TARGET_STATE_OFF                 '0'
#define HEATING_COOLING_TARGET_STATE_HEAT                '1'
#define HEATING_COOLING_TARGET_STATE_COOL                '2'
#define HEATING_COOLING_TARGET_STATE_AUTO                '3'
#define HEATING_COOLING_DEFAULT_STATE_OFF                "0"
#define HEX_DIGITS_UPPERCASE                             "0123456789ABCDEF"

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef enum accessory_service_type
{
    ACCESSORY_MODE_OTHER = 0,
    ACCESSORY_MODE_GARAGE_DOOR_OPENER,
    ACCESSORY_MODE_THERMOSTAT,
    ACCESSORY_MODE_LIGHT_BULB,
    ACCESSORY_MODE_FAN,
    ACCESSORY_MODE_OUTLET,
    ACCESSORY_MODE_SWITCH,
    ACCESSORY_MODE_LOCK_MECHANISM,
    ACCESSORY_MODE_LOCK_MANAGEMENT,
#ifdef IOS9_ENABLED_SERVICES
    ACCESSORY_MODE_SECURITY_SYSTEM,
    ACCESSORY_MODE_AIR_QUALITY_SENSOR,
    ACCESSORY_MODE_CARBON_MONOXIDE_SENSOR,
    ACCESSORY_MODE_CONTACT_SENSOR,
    ACCESSORY_MODE_DOOR,
    ACCESSORY_MODE_HUMIDITY_SENSOR,
    ACCESSORY_MODE_LEAK_SENSOR,
    ACCESSORY_MODE_LIGHT_SENSOR,
    ACCESSORY_MODE_OCCUPANCY_SENSOR,
    ACCESSORY_MODE_MOTION_SENSOR,
    ACCESSORY_MODE_SMOKE_SENSOR,
    ACCESSORY_MODE_STATEFUL_PROGRAMMABLE_SWITCH,
    ACCESSORY_MODE_STATELESS_PROGRAMMABLE_SWITCH,
    ACCESSORY_MODE_TEMPERATURE_SENSOR,
    ACCESSORY_MODE_WINDOW,
    ACCESSORY_MODE_WINDOW_COVERING,
    ACCESSORY_MODE_BATTERY,
    ACCESSORY_MODE_CARBON_DIOXIDE_SENSOR,
#endif
} accessory_mode_t;

typedef enum
{
    READ_LOGS_FROM_TIME = 0x00,
    CLEAR_LOGS          = 0x01,
    SET_CURRENT_TIME    = 0x02,
}lock_control_point_type_t;

typedef struct
{
    wiced_homekit_characteristics_t* characteristic;
    char* name;
}characteristic_name_list_t;

typedef struct
{
    wiced_homekit_services_t*   service;
    characteristic_name_list_t* characteristic_list;
    uint8_t                     characteristic_list_size;
    char* name;
}service_name_list_t;

typedef struct
{
    wiced_homekit_accessories_t* accessory;
    char*                        name;
    service_name_list_t*         service_list;
    uint8_t                      service_list_size;
} accessory_name_list_t;

typedef void (*mode_fp) (void);
typedef struct
{
    accessory_mode_t            mode;
    char*                       name;
    mode_fp                     func;
} accessory_mode_info_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

wiced_result_t wiced_homekit_actions_on_remote_value_update( wiced_homekit_characteristic_value_update_list_t* update_list );
wiced_result_t wiced_homekit_display_password( uint8_t* srp_pairing_password );
wiced_result_t identify_action( uint16_t accessory_id, uint16_t characteristic_id );

static int read_characteristic_command            ( int argc, char *argv[] );
static int write_characteristic_command           ( int argc, char *argv[] );
static int list_accessory_modes_command           ( int argc, char *argv[] );
static int configure_accessory_mode_command       ( int argc, char *argv[] );
static int list_accessories_command               ( int argc, char *argv[] );
static int list_services_command                  ( int argc, char *argv[] );
static int list_characteritics_command            ( int argc, char *argv[] );
static int clear_pairings_command                 ( int argc, char *argv[] );
static int remove_characteristic_command          ( int argc, char *argv[] );
static int return_removed_characteristics_command ( int argc, char *argv[] );
static int remove_service_command                 ( int argc, char *argv[] );
static int return_removed_services_command        ( int argc, char *argv[] );
#ifdef OTA2_SUPPORT
static int factory_reset_command                  ( int argc, char *argv[] );
#endif
static int enable_low_power_mode_command          ( int argc, char *argv[] );
static int disable_low_power_mode_command         ( int argc, char *argv[] );

static void configure_multi_service_accessory();
static void configure_garage_door_opener_accessory();
static void configure_thermostat_accessory();
static void configure_light_bulb_accessory();
static void configure_fan_accessory();
static void configure_outlet_accessory();
static void configure_switch_accessory();
static void configure_lock_mechanism_accessory();
static void configure_lock_management_accessory();
#ifdef IOS9_ENABLED_SERVICES
static void configure_air_quality_sensor_accessory();
static void configure_security_system_accessory();
static void configure_carbon_monoxide_sensor_accessory();
static void configure_contact_sensor_accessory();
static void configure_door_accessory();
static void configure_humidity_sensor_accessory();
static void configure_leak_sensor_accessory();
static void configure_light_sensor_accessory();
static void configure_occupancy_sensor_accessory();
static void configure_motion_sensor_accessory();
static void configure_smoke_sensor_accessory();
static void configure_stateful_programmable_switch_accessory();
static void configure_stateless_programmable_switch_accessory();
static void configure_temperature_sensor_accessory();
static void configure_window_accessory();
static void configure_window_covering_accessory();
static void configure_battery_accessory();
static void configure_carbon_dioxide_sensor_accessory();
#endif

static void     homekit_get_random_device_id( char device_id_buffer[STRING_MAX_MAC] );
/******************************************************
 *               Variable Definitions
 ******************************************************/
static char test_command_buffer        [ TEST_COMMAND_LENGTH ];
static char test_command_history_buffer[ TEST_COMMAND_LENGTH * TEST_COMMAND_HISTORY_LENGTH ];


const command_t test_command_table[ ] =
{
    LIST_HOMEKIT_ACCESSORY_MODE_COMMAND_ENTRY       ( list_accessory_modes,           list_accessory_modes_command           )
    CONFIGURE_HOMEKIT_ACCESSORY_MODE_COMMAND_ENTRY  ( configure_accessory_mode,       configure_accessory_mode_command       )
    LIST_HOMEKIT_ACCESSORIES_COMMAND_ENTRY          ( list_accessories,               list_accessories_command               )
    LIST_HOMEKIT_SERVICES_COMMAND_ENTRY             ( list_services,                  list_services_command                  )
    LIST_HOMEKIT_CHARACTERISTICS_COMMAND_ENTRY      ( list_characteristics,           list_characteritics_command            )
    READ_HOMEKIT_CHARACTERISTIC_ENTRY               ( read_characteristic,            read_characteristic_command            )
    WRITE_HOMEKIT_CHARACTERISTIC_ENTRY              ( write_characteristic,           write_characteristic_command           )
    REMOVE_HOMEKIT_CHARACTERISTIC_ENTRY             ( remove_characteristic,          remove_characteristic_command          )
    RETURN_HOMEKIT_CHARACTERISTIC_ENTRY             ( return_removed_characteristics, return_removed_characteristics_command )
    REMOVE_HOMEKIT_SERVICE_ENTRY                    ( remove_service,                 remove_service_command                 )
    RETURN_HOMEKIT_SERVICE_ENTRY                    ( return_removed_services,        return_removed_services_command        )
    CLEAR_ALL_PAIRINGS_ENTRY                        ( clear_pairings,                 clear_pairings_command                 )
#ifdef OTA2_SUPPORT
    FACTORY_RESET_ON_REBOOT                         ( factory_reset,                  factory_reset_command                  )
#endif
    ENABLE_LOW_POWER_MODE                           ( enable_low_power_mode,          enable_low_power_mode_command          )
    DISABLE_LOW_POWER_MODE                          ( disable_low_power_mode,         disable_low_power_mode_command         )
    COMMAND_TABLE_ENDING()
};

char accessory_name[MAX_NAME_LENGTH];

#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI

char* protocols[] =
{
    [ 0 ] = "com.apple.one",
};

/* info structure for WAC, see apple_wac.h for more details */
apple_wac_info_t apple_wac_info =
{
    .supports_homekit       = WICED_TRUE,
    .supports_airplay       = WICED_FALSE,
    .supports_airprint      = WICED_FALSE,
    .supports_5ghz_networks = WICED_FALSE,
    .has_app                = WICED_TRUE,
    .firmware_revision      = "1.0.0",
    .hardware_revision      = "2.0.0",
    .serial_number          = "001122334455",
    .model                  = "A6969",
    .manufacturer           = "Test",
    .oui                    = (uint8_t *)"\xAA\xBB\xCC",
    .mfi_protocols          = &protocols[0],
    .num_mfi_protocols      = 1,
    .bundle_seed_id         = "ABCA66M49V",
    .mdns_desired_hostname  = "homekit-certification-test",
    .mdns_nice_name         = accessory_name,
    .device_random_id       = "XX:XX:XX:XX:XX:XX",
};
#endif

apple_homekit_accessory_config_t apple_homekit_discovery_settings =
{
#ifndef WICED_HOMEKIT_DO_NOT_USE_MFI
    .mfi_config_features         = MFI_CONFIG_FEATURES_SUPPORTS_MFI_AND_PIN,
#else
    .mfi_config_features         = MFI_CONFIG_FEATURES_SUPPORTS_PIN_ONLY,
#endif
    .mfi_protocol_version_string    = "1.0",
    .accessory_category_identifier  = BRIDGE
};

static uint16_t iid = 1; /* Instance ID should always be > 0 */
static uint16_t aid = 1;

/* Accessory definitions */
wiced_homekit_accessories_t certification_services_accessory;
uint8_t                     current_accessory_mode;

/* Accessory Service and Characteristics */
wiced_homekit_services_t        certification_services_accessory_information_service;
wiced_homekit_characteristics_t certification_services_identify_characteristic;
wiced_homekit_characteristics_t certification_services_manufacturer_characteristic;
wiced_homekit_characteristics_t certification_services_model_characteristic;
wiced_homekit_characteristics_t certification_services_name_characteristic;
#ifdef IOS9_ENABLED_SERVICES
wiced_homekit_characteristics_t certification_services_firmware_revision;
#endif
wiced_homekit_characteristics_t certification_services_serial_number_characteristic;

/* Bridge name characteristic values */
char bridge_name_value[MAX_NAME_LENGTH];

/* Accessory characteristic values */
char certification_services_name_value[MAX_NAME_LENGTH];
char certification_services_manufacturer_value[MAX_NAME_LENGTH]           = "\"Cypress\"";
char certification_services_model_value[MAX_NAME_LENGTH]                  = "\"WICED-HK Device\"";
char certification_services_serial_number_value[MAX_NAME_LENGTH]          = "\"8-8-8-8-8-8-8-8\"";
#ifdef IOS9_ENABLED_SERVICES
char certification_services_firmware_revision_value[]                     = "\"10000\"";
#endif

/* Garage door services and characteristics */
wiced_homekit_services_t        garage_door_opener_service;
wiced_homekit_characteristics_t current_door_state_characteristic;
wiced_homekit_characteristics_t target_door_state_characteristic;
wiced_homekit_characteristics_t obstruction_detected_characteristic;
wiced_homekit_characteristics_t lock_mechanism_current_state_characteristic;
wiced_homekit_characteristics_t lock_mechanism_target_state_characteristic;
wiced_homekit_characteristics_t garage_door_name_characteristic;

/* Garage Door opener characteristics values */
char current_door_state_value[4]                  = "0";
char target_door_state_value[4]                   = "0";
char obstruction_detected_value[6]                = "false";
char lock_mechnism_current_value[4]               = "0";
char lock_mechnism_target_value[4]                = "0";
char garage_door_name[MAX_NAME_LENGTH]            = "\""GARAGE_DOOR_OPENER_SERVICE_NAME"\"";

/* Thermostat services and characteristics */
wiced_homekit_services_t        thermostat_service;
wiced_homekit_characteristics_t current_heating_cooling_mode_characteristic;
wiced_homekit_characteristics_t target_heating_cooling_mode_characteristic;
wiced_homekit_characteristics_t temperature_current_characteristic;
wiced_homekit_characteristics_t temperature_target_characteristic;
wiced_homekit_characteristics_t temperature_units_characteristic;
wiced_homekit_characteristics_t thermostat_name_characteristic;

/* Thermostat characteristics values */
char current_heating_cooling_mode_value[4]        = "0";
char target_heating_cooling_mode_value[4]         = "0";
char heating_cooling_current_temperature_value[5] = "25";
char heating_cooling_target_temperature_value[5]  = "25";
char heating_cooling_units_value[4]               = "0";
char heating_cooling_name_value[MAX_NAME_LENGTH]  = "\""THERMOSTAT_SERVICE_NAME"\"";

/* Lightbulb services and characteristics */
wiced_homekit_services_t        lightbulb_service;
wiced_homekit_characteristics_t lightbulb_on_characteristic;
wiced_homekit_characteristics_t lightbulb_name_characteristic;

/* Lightbulb characteristics values */
char lightbulb_name_value[MAX_NAME_LENGTH] = "\""LIGHTBUBL_SERVICE_NAME"\"";
char lightbulb_on_value[6]                  = "false";

/* Fan service and characteristics */
wiced_homekit_services_t        fan_service;
wiced_homekit_characteristics_t fan_on_characteristic;
wiced_homekit_characteristics_t fan_name_characteristic;

/* Fan characteristics values */
char fan_on_value[6]                  = "false";
char fan_name_value[MAX_NAME_LENGTH]  = "\""FAN_SERVICE_NAME"\"";

/* Outlet services and characteristics */
wiced_homekit_services_t        outlet_service;
wiced_homekit_characteristics_t outlet_on_characteristic;
wiced_homekit_characteristics_t outlet_in_use_characteristic;
wiced_homekit_characteristics_t outlet_name_characteristic;

/* outlet characteristics values */
char outlet_name_value[MAX_NAME_LENGTH]   = "\""OUTLET_SERVICE_NAME"\"";
char outlet_on_value[6]                   = "false";
char outlet_in_use_value[6]               = "false";

/* Switch services and characteristics */
wiced_homekit_services_t        switch_service;
wiced_homekit_characteristics_t switch_on_characteristic;
wiced_homekit_characteristics_t switch_name_characteristic;

/* switch characteristics values */
char switch_name_value[MAX_NAME_LENGTH]       = "\""SWITCH_SERVICE_NAME"\"";
char switch_on_value[6]                       = "false";

/* Lock Management services and characteristics */
wiced_homekit_services_t        lock_management_service;
wiced_homekit_characteristics_t lock_management_control_point_characteristic;
wiced_homekit_characteristics_t lock_management_version_characteristic;
wiced_homekit_characteristics_t lock_management_name_characteristic;

/* Lock Management characteristics values */
char lock_management_control_point_value[20]       = "\"AAEA\"";
char lock_management_version_value[32]             = "\"1.0\"";
char lock_management_name_value[MAX_NAME_LENGTH]   = "\""LOCK_MANAGEMENT_SERVICE_NAME"\"";

/* Lock Mechanism service and characteristics */
wiced_homekit_services_t        lock_mechanism_service;
wiced_homekit_characteristics_t lock_mechanism_current_characteristic;
wiced_homekit_characteristics_t lock_mechanism_target_characteristic;
wiced_homekit_characteristics_t lock_mechanism_name_characteristic;

/* Lock Mechanism characteristics values */
char lock_mechnism_current_state_value[4]       = "0";
char lock_mechnism_target_state_value[4]        = "0";
char lock_mechnism_name_value[MAX_NAME_LENGTH]  = "\""LOCK_MECHANISM_SERVICE_NAME"\"";

#ifdef IOS9_ENABLED_SERVICES
/* Air Quality Sensor services and characteristics */
wiced_homekit_services_t        air_quality_sensor_service;
wiced_homekit_characteristics_t air_quality_characteristic;
wiced_homekit_characteristics_t air_quality_name_characteristic;

/* Air Quality Sensor characteristics values */
char air_quality_name_value[MAX_NAME_LENGTH]       = "\""AIR_QUALITY_SENSOR_SERVICE_NAME"\"";
char air_quality_value[2]                          = "0";

/* Security system services and characteristics */
wiced_homekit_services_t        security_system_service;
wiced_homekit_characteristics_t security_system_current_state_characteristic;
wiced_homekit_characteristics_t security_system_target_state_characteristic;
wiced_homekit_characteristics_t security_system_name_characteristic;

/* Security system characteristics values */
char security_system_name_value[MAX_NAME_LENGTH]       = "\""SECURITY_SYSTEM_SERVICE_NAME"\"";
char security_system_current_state_value[4]            = "0";
char security_system_target_state_value[4]             = "0";

/* Carbon Monoxide Sensor services and characteristics */
wiced_homekit_services_t        carbon_monoxide_sensor_service;
wiced_homekit_characteristics_t carbon_monoxide_detected_characteristic;
wiced_homekit_characteristics_t carbon_monoxide_name_characteristic;

/* Carbon Monoxide Sensor characteristics values */
char carbon_monoxide_name_value[MAX_NAME_LENGTH]       = "\""CARBON_MONOXIDE_SENSOR_SERVICE_NAME"\"";
char carbon_monoxide_detected_value[2]                 = "0";

/* Contact Sensor services and characteristics */
wiced_homekit_services_t        contact_sensor_service;
wiced_homekit_characteristics_t contact_sensor_state_characteristic;
wiced_homekit_characteristics_t contact_sensor_name_characteristic;

/* Contact Sensor characteristics values */
char contact_sensor_name_value[MAX_NAME_LENGTH]        = "\""CONTACT_SENSOR_SERVICE_NAME"\"";
char contact_sensor_state_value[2]                     = "0";

/* Door services and characteristics */
wiced_homekit_services_t        door_service;
wiced_homekit_characteristics_t door_current_position_characteristic;
wiced_homekit_characteristics_t door_target_position_characteristic;
wiced_homekit_characteristics_t door_position_state_characteristic;
wiced_homekit_characteristics_t door_name_characteristic;

/* Door characteristics values */
char door_name_value[MAX_NAME_LENGTH]       = "\""DOOR_SERVICE_NAME"\"";
char door_current_position_value[4]         = "0";
char door_target_position_value[4]          = "0";
char door_position_state_value[4]           = "0";

/* Humidity Sensor services and characteristics */
wiced_homekit_services_t        humidity_sensor_service;
wiced_homekit_characteristics_t current_relative_humidity_characteristic;
wiced_homekit_characteristics_t humidity_sensor_name_characteristic;

/* Humidity Sensor characteristics values */
char humidity_sensor_name_value[MAX_NAME_LENGTH]       = "\""HUMIDITY_SENSOR_SERVICE_NAME"\"";
char current_relative_humidity_value[4]                = "0";

/* Leak Sensor services and characteristics */
wiced_homekit_services_t        leak_sensor_service;
wiced_homekit_characteristics_t leak_detected_characteristic;
wiced_homekit_characteristics_t leak_sensor_name_characteristic;

/* Leak Sensor characteristics values */
char leak_sensor_name_value[MAX_NAME_LENGTH]       = "\""LEAK_SENSOR_SERVICE_NAME"\"";
char leak_detected_value[2]                        = "0";

/* Light Sensor services and characteristics */
wiced_homekit_services_t        light_sensor_service;
wiced_homekit_characteristics_t current_light_level_characteristic;
wiced_homekit_characteristics_t light_sensor_name_characteristic;

/* Light Sensor characteristics values */
char light_sensor_name_value[MAX_NAME_LENGTH]       = "\""LIGHT_SENSOR_SERVICE_NAME"\"";
char current_light_level_value[11]                  = "0";

/* Motion Sensor services and characteristics */
wiced_homekit_services_t        motion_sensor_service;
wiced_homekit_characteristics_t motion_detected_characteristic;
wiced_homekit_characteristics_t motion_sensor_name_characteristic;

/* Motion Sensor characteristics values */
char motion_sensor_name_value[MAX_NAME_LENGTH]       = "\""MOTION_SENSOR_SERVICE_NAME"\"";
char motion_detected_value[6]                        = "false";

/* Occupancy Sensor services and characteristics */
wiced_homekit_services_t        occupancy_sensor_service;
wiced_homekit_characteristics_t occupancy_detected_characteristic;
wiced_homekit_characteristics_t occupancy_sensor_name_characteristic;

/* Occupancy Sensor characteristics values */
char occupancy_sensor_name_value[MAX_NAME_LENGTH]      = "\""OCCUPANCY_SENSOR_SERVICE_NAME"\"";
char occupancy_detected_value[2]                       = "0";

/* Smoke Sensor services and characteristics */
wiced_homekit_services_t        smoke_sensor_service;
wiced_homekit_characteristics_t smoke_detected_characteristic;
wiced_homekit_characteristics_t smoke_sensor_name_characteristic;

/* Smoke Sensor characteristics values */
char smoke_sensor_name_value[MAX_NAME_LENGTH]      = "\""SMOKE_SENSOR_SERVICE_NAME"\"";
char smoke_detected_value[2]                       = "0";

/* Stateful Programmable Switch services and characteristics */
wiced_homekit_services_t        stateful_programmable_switch_service;
wiced_homekit_characteristics_t programmable_switch_event_characteristic;
wiced_homekit_characteristics_t programmable_switch_output_state_characteristic;
wiced_homekit_characteristics_t stateful_programmable_switch_name_characteristic;

/* Stateful Programmable Switch characteristics values */
char stateful_programmable_switch_name_value[MAX_NAME_LENGTH]      = "\""STATEFUL_PROGRAMMABLE_SWITCH_SERVICE_NAME"\"";
char programmable_switch_event_value[2]                            = "0";
char programmable_switch_output_state_value[2]                     = "0";

/* Stateless Programmable Switch services and characteristics */
wiced_homekit_services_t        stateless_programmable_switch_service;
wiced_homekit_characteristics_t stateless_programmable_switch_event_characteristic;
wiced_homekit_characteristics_t stateless_programmable_switch_name_characteristic;

/* Stateless Programmable Switch characteristics values */
char stateless_programmable_switch_name_value[MAX_NAME_LENGTH]      = "\""STATELESS_PROGRAMMABLE_SWITCH_SERVICE_NAME"\"";
char stateless_programmable_switch_event_value[2]                   = "0";

/* Temperature Sensor services and characteristics */
wiced_homekit_services_t        temperature_sensor_service;
wiced_homekit_characteristics_t current_temperature_characteristic;
wiced_homekit_characteristics_t temperature_sensor_name_characteristic;

/* Temperature Sensor characteristics values */
char temperature_sensor_name_value[MAX_NAME_LENGTH]      = "\""TEMPERATURE_SENSOR_SERVICE_NAME"\"";
char current_temperature_value[5]                        = "0";

/* Window services and characteristics */
wiced_homekit_services_t        window_service;
wiced_homekit_characteristics_t window_current_position_characteristic;
wiced_homekit_characteristics_t window_target_position_characteristic;
wiced_homekit_characteristics_t window_position_state_characteristic;
wiced_homekit_characteristics_t window_name_characteristic;

/* Window characteristics values */
char window_name_value[MAX_NAME_LENGTH]      = "\""WINDOW_SERVICE_NAME"\"";
char window_current_position_value[4]        = "0";
char window_target_position_value[4]         = "0";
char window_position_state_value[4]          = "0";

/* Window Covering services and characteristics */
wiced_homekit_services_t        window_covering_service;
wiced_homekit_characteristics_t current_position_characteristic;
wiced_homekit_characteristics_t target_position_characteristic;
wiced_homekit_characteristics_t position_state_characteristic;
wiced_homekit_characteristics_t window_covering_name_characteristic;

/* Window Covering characteristics values */
char window_covering_name_value[MAX_NAME_LENGTH]  = "\""WINDOW_COVERING_SERVICE_NAME"\"";
char current_position_value[4]                    = "0";
char target_position_value[4]                     = "0";
char position_state_value[4]                      = "0";

/* Battery services and characteristics */
wiced_homekit_services_t        battery_service;
wiced_homekit_characteristics_t battery_level_characteristic;
wiced_homekit_characteristics_t charging_state_characteristic;
wiced_homekit_characteristics_t status_low_battery_characteristic;
wiced_homekit_characteristics_t battery_name_characteristic;

/* Battery characteristics values */
char battery_name_value[MAX_NAME_LENGTH]   = "\""BATTERY_SERVICE_NAME"\"";
char battery_level_value[4]                = "0";
char charging_state_value[2]               = "0";
char status_low_battery_value[2]           = "0";

/* Carbon Dioxide Sensor services and characteristics */
wiced_homekit_services_t        carbon_dioxide_sensor_service;
wiced_homekit_characteristics_t carbon_dioxide_detected_characteristic;
wiced_homekit_characteristics_t carbon_dioxide_name_characteristic;

/* Carbon Dioxide Sensor characteristics values */
char carbon_dioxide_name_value[MAX_NAME_LENGTH]       = "\""CARBON_DIOXIDE_SENSOR_SERVICE_NAME"\"";
char carbon_dioxide_detected_value[2]                 = "0";
#endif

characteristic_name_list_t certification_services_accessory_information_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &certification_services_identify_characteristic,
        .name           = IDENTIFY_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &certification_services_manufacturer_characteristic,
        .name           = MANUFACTURER_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &certification_services_model_characteristic,
        .name           = MODEL_CHARACTERISTIC
    },
    [3] =
    {
        .characteristic = &certification_services_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    },
#ifdef IOS9_ENABLED_SERVICES
    [4] =
    {
        .characteristic = &certification_services_firmware_revision,
        .name           = FIRMWARE_REVISION_CHARACTERISTIC
    },
#endif
    [5] =
    {
        .characteristic = &certification_services_serial_number_characteristic,
        .name           = SERIAL_NUMBER_CHARACTERISTIC
    }
};

characteristic_name_list_t garage_door_opener_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &current_door_state_characteristic,
        .name           = CURRENT_DOOR_STATE_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &target_door_state_characteristic,
        .name           = TARGET_DOOR_STATE_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &obstruction_detected_characteristic,
        .name           = OBSTRUCTION_DETECTED_CHARACTERISTIC
    },
    [3] =
    {
        .characteristic = &lock_mechanism_current_state_characteristic,
        .name           = LOCK_MECHANISM_CURRENT_STATE_CHARACTERISTIC
    },
    [4] =
    {
        .characteristic = &garage_door_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    },
    [5] =
    {
        .characteristic = &lock_mechanism_target_state_characteristic,
        .name           = LOCK_MECHANISM_TARGET_STATE_CHARACTERISTIC
    }
};

characteristic_name_list_t thermostat_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &current_heating_cooling_mode_characteristic,
        .name           = CURRENT_HEATING_COOLING_MODE_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &target_heating_cooling_mode_characteristic,
        .name           = TARGET_HEATING_COOLING_MODE_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &temperature_current_characteristic,
        .name           = TEMPERATURE_CURRENT_CHARACTERISTIC
    },
    [3] =
    {
        .characteristic = &temperature_target_characteristic,
        .name           = TEMPERATURE_TARGET_CHARACTERISTIC
    },
    [4] =
    {
        .characteristic = &temperature_units_characteristic,
        .name           = TEMPERATURE_UNIT_CHARACTERISTIC
    },
    [5] =
    {
        .characteristic = &thermostat_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t lightbulb_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &lightbulb_on_characteristic,
        .name           = ON_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &lightbulb_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t fan_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &fan_on_characteristic,
        .name           = ON_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &fan_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t outlet_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &outlet_on_characteristic,
        .name           = ON_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &outlet_in_use_characteristic,
        .name           = OUTLET_IN_USE_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &outlet_name_characteristic,
        .name           = NAME_CHARACTERISTIC,
    }
};

characteristic_name_list_t switch_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &switch_on_characteristic,
        .name           = ON_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &switch_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};
characteristic_name_list_t lock_management_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &lock_management_control_point_characteristic,
        .name           = CONTROL_POINT_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &lock_management_version_characteristic,
        .name           = VERSION_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &lock_management_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};
characteristic_name_list_t lock_mechanism_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &lock_mechanism_current_characteristic,
        .name           = CONTROL_POINT_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &lock_mechanism_target_characteristic,
        .name           = VERSION_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &lock_mechanism_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};
#ifdef IOS9_ENABLED_SERVICES
characteristic_name_list_t air_quality_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &air_quality_characteristic,
        .name           = AIR_QUALITY_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &air_quality_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t security_system_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &security_system_current_state_characteristic,
        .name           = SECURITY_SYSTEM_CURRENT_STATE_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &security_system_target_state_characteristic,
        .name           = SECURITY_SYSTEM_TARGET_STATE_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &security_system_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t carbon_monoxide_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &carbon_monoxide_detected_characteristic,
        .name           = CARBON_MONOXIDE_DETECTED_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &carbon_monoxide_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t contact_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &contact_sensor_state_characteristic,
        .name           = CONTACT_SENSOR_STATE_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &contact_sensor_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t door_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &door_current_position_characteristic,
        .name           = CURRENT_POSITION_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &door_target_position_characteristic,
        .name           = TARGET_POSITION_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &door_position_state_characteristic,
        .name           = POSITION_STATE_CHARACTERISTIC
    },
    [3] =
    {
        .characteristic = &door_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t humidity_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &current_relative_humidity_characteristic,
        .name           = CURRENT_RELATIVE_HUMIDITY_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &humidity_sensor_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t leak_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &leak_detected_characteristic,
        .name           = LEAK_DETECTED_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &leak_sensor_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t light_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &current_light_level_characteristic,
        .name           = CURRENT_LIGHT_LEVEL_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &light_sensor_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t motion_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &motion_detected_characteristic,
        .name           = MOTION_DETECTED_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &motion_sensor_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t occupancy_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &occupancy_detected_characteristic,
        .name           = OCCUPANCY_DETECTED_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &occupancy_sensor_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t smoke_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &smoke_detected_characteristic,
        .name           = SMOKE_DETECTED_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &smoke_sensor_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t stateful_programmable_switch_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &programmable_switch_event_characteristic,
        .name           = PROGRAMMABLE_SWITCH_EVENT_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &programmable_switch_output_state_characteristic,
        .name           = PROGRAMMABLE_SWITCH_OUTPUT_STATE_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &stateful_programmable_switch_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t stateless_programmable_switch_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &stateless_programmable_switch_event_characteristic,
        .name           = PROGRAMMABLE_SWITCH_EVENT_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &stateless_programmable_switch_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t temperature_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &current_temperature_characteristic,
        .name           = TEMPERATURE_CURRENT_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &temperature_sensor_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t window_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &window_current_position_characteristic,
        .name           = CURRENT_POSITION_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &window_target_position_characteristic,
        .name           = TARGET_POSITION_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &window_position_state_characteristic,
        .name           = POSITION_STATE_CHARACTERISTIC
    },
    [3] =
    {
        .characteristic = &window_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t window_covering_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &current_position_characteristic,
        .name           = CURRENT_POSITION_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &target_position_characteristic,
        .name           = TARGET_POSITION_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &position_state_characteristic,
        .name           = POSITION_STATE_CHARACTERISTIC
    },
    [3] =
    {
        .characteristic = &window_covering_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t battery_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &battery_level_characteristic,
        .name           = BATTERY_LEVEL_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &charging_state_characteristic,
        .name           = CHARGING_STATE_CHARACTERISTIC
    },
    [2] =
    {
        .characteristic = &status_low_battery_characteristic,
        .name           = STATUS_LOW_BATTERY_CHARACTERISTIC
    },
    [3] =
    {
        .characteristic = &battery_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};

characteristic_name_list_t carbon_dioxide_sensor_service_characteristic_list[] =
{
    [0] =
    {
        .characteristic = &carbon_dioxide_detected_characteristic,
        .name           = CARBON_DIOXIDE_DETECTED_CHARACTERISTIC
    },
    [1] =
    {
        .characteristic = &carbon_dioxide_name_characteristic,
        .name           = NAME_CHARACTERISTIC
    }
};
#endif

service_name_list_t certification_accessory_service_list[] =
{
        [0] =
        {
            .service                  = &certification_services_accessory_information_service,
            .name                     = ACCESSORY_INFORMATION_SERVICE_NAME,
            .characteristic_list      = certification_services_accessory_information_service_characteristic_list,
            .characteristic_list_size = sizeof(certification_services_accessory_information_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [1] =
        {
            .service                  = &garage_door_opener_service,
            .name                     = GARAGE_DOOR_OPENER_SERVICE_NAME,
            .characteristic_list      = garage_door_opener_service_characteristic_list,
            .characteristic_list_size = sizeof(garage_door_opener_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [2] =
        {
            .service                  = &thermostat_service,
            .name                     = THERMOSTAT_SERVICE_NAME,
            .characteristic_list      = thermostat_service_characteristic_list,
            .characteristic_list_size = sizeof(thermostat_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [3] =
        {
            .service                  = &lightbulb_service,
            .name                     = LIGHTBUBL_SERVICE_NAME,
            .characteristic_list      = lightbulb_service_characteristic_list,
            .characteristic_list_size = sizeof(lightbulb_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [4] =
        {
            .service                  = &fan_service,
            .name                     = FAN_SERVICE_NAME,
            .characteristic_list      = fan_service_characteristic_list,
            .characteristic_list_size = sizeof(fan_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [5] =
        {
            .service                  = &outlet_service,
            .name                     = OUTLET_SERVICE_NAME,
            .characteristic_list      = outlet_service_characteristic_list,
            .characteristic_list_size = sizeof(outlet_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [6] =
        {
            .service                  = &switch_service,
            .name                     = SWITCH_SERVICE_NAME,
            .characteristic_list      = switch_service_characteristic_list,
            .characteristic_list_size = sizeof(switch_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [7] =
        {
            .service                  = &lock_management_service,
            .name                     = LOCK_MANAGEMENT_SERVICE_NAME,
            .characteristic_list      = lock_management_service_characteristic_list,
            .characteristic_list_size = sizeof(lock_management_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [8] =
        {
            .service                  = &lock_mechanism_service,
            .name                     = LOCK_MECHANISM_SERVICE_NAME,
            .characteristic_list      = lock_mechanism_service_characteristic_list,
            .characteristic_list_size = sizeof(lock_mechanism_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
#ifdef IOS9_ENABLED_SERVICES
        [9] =
        {
            .service                  = &air_quality_sensor_service,
            .name                     = AIR_QUALITY_SENSOR_SERVICE_NAME,
            .characteristic_list      = air_quality_service_characteristic_list,
            .characteristic_list_size = sizeof(air_quality_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [10] =
        {
            .service                  = &security_system_service,
            .name                     = SECURITY_SYSTEM_SERVICE_NAME,
            .characteristic_list      = security_system_service_characteristic_list,
            .characteristic_list_size = sizeof(security_system_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [11] =
        {
            .service                  = &carbon_monoxide_sensor_service,
            .name                     = CARBON_MONOXIDE_SENSOR_SERVICE_NAME,
            .characteristic_list      = carbon_monoxide_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(carbon_monoxide_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [12] =
        {
            .service                  = &contact_sensor_service,
            .name                     = CONTACT_SENSOR_SERVICE_NAME,
            .characteristic_list      = contact_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(contact_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [13] =
        {
            .service                  = &door_service,
            .name                     = DOOR_SERVICE_NAME,
            .characteristic_list      = door_service_characteristic_list,
            .characteristic_list_size = sizeof(door_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [14] =
        {
            .service                  = &humidity_sensor_service,
            .name                     = HUMIDITY_SENSOR_SERVICE_NAME,
            .characteristic_list      = humidity_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(humidity_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [15] =
        {
            .service                  = &leak_sensor_service,
            .name                     = LEAK_SENSOR_SERVICE_NAME,
            .characteristic_list      = leak_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(leak_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [16] =
        {
            .service                  = &light_sensor_service,
            .name                     = LIGHT_SENSOR_SERVICE_NAME,
            .characteristic_list      = light_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(light_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [17] =
        {
            .service                  = &occupancy_sensor_service,
            .name                     = OCCUPANCY_SENSOR_SERVICE_NAME,
            .characteristic_list      = occupancy_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(occupancy_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [18] =
        {
            .service                  = &motion_sensor_service,
            .name                     = MOTION_SENSOR_SERVICE_NAME,
            .characteristic_list      = motion_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(motion_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [19] =
        {
            .service                  = &smoke_sensor_service,
            .name                     = SMOKE_SENSOR_SERVICE_NAME,
            .characteristic_list      = smoke_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(smoke_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [20] =
        {
            .service                  = &stateful_programmable_switch_service,
            .name                     = STATEFUL_PROGRAMMABLE_SWITCH_SERVICE_NAME,
            .characteristic_list      = stateful_programmable_switch_service_characteristic_list,
            .characteristic_list_size = sizeof(stateful_programmable_switch_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [21] =
        {
            .service                  = &stateless_programmable_switch_service,
            .name                     = STATELESS_PROGRAMMABLE_SWITCH_SERVICE_NAME,
            .characteristic_list      = stateless_programmable_switch_service_characteristic_list,
            .characteristic_list_size = sizeof(stateless_programmable_switch_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [22] =
        {
            .service                  = &temperature_sensor_service,
            .name                     = TEMPERATURE_SENSOR_SERVICE_NAME,
            .characteristic_list      = temperature_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(temperature_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [23] =
        {
            .service                  = &window_service,
            .name                     = WINDOW_SERVICE_NAME,
            .characteristic_list      = window_service_characteristic_list,
            .characteristic_list_size = sizeof(window_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [24] =
        {
            .service                  = &window_covering_service,
            .name                     = WINDOW_COVERING_SERVICE_NAME,
            .characteristic_list      = window_covering_service_characteristic_list,
            .characteristic_list_size = sizeof(window_covering_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [25] =
        {
            .service                  = &battery_service,
            .name                     = BATTERY_SERVICE_NAME,
            .characteristic_list      = battery_service_characteristic_list,
            .characteristic_list_size = sizeof(battery_service_characteristic_list)/sizeof(characteristic_name_list_t)
        },
        [26] =
        {
            .service                  = &carbon_dioxide_sensor_service,
            .name                     = CARBON_DIOXIDE_SENSOR_SERVICE_NAME,
            .characteristic_list      = carbon_dioxide_sensor_service_characteristic_list,
            .characteristic_list_size = sizeof(carbon_dioxide_sensor_service_characteristic_list)/sizeof(characteristic_name_list_t)
        }
#endif
};

accessory_name_list_t accessory_name_list[] =
{
    [0] =
    {
        .accessory         = &certification_services_accessory,
        .name              = accessory_name,
        .service_list      = certification_accessory_service_list,
        .service_list_size = sizeof(certification_accessory_service_list)/sizeof(service_name_list_t)
    }
};

accessory_mode_info_t   accessory_mode_list[] =
{
    { .mode = ACCESSORY_MODE_OTHER,                         .name = "MultiService Accessory"                 , .func = configure_multi_service_accessory },
    { .mode = ACCESSORY_MODE_GARAGE_DOOR_OPENER,            .name = "Garage Door Opener Accessory"           , .func = configure_garage_door_opener_accessory },
    { .mode = ACCESSORY_MODE_THERMOSTAT,                    .name = "Thermostat Accessory"                   , .func = configure_thermostat_accessory },
    { .mode = ACCESSORY_MODE_LIGHT_BULB,                    .name = "Light Bulb Accessory"                   , .func = configure_light_bulb_accessory },
    { .mode = ACCESSORY_MODE_FAN,                           .name = "Fan Accessory"                          , .func = configure_fan_accessory },
    { .mode = ACCESSORY_MODE_OUTLET,                        .name = "Outlet Accessory"                       , .func = configure_outlet_accessory },
    { .mode = ACCESSORY_MODE_SWITCH,                        .name = "Switch Accessory"                       , .func = configure_switch_accessory },
    { .mode = ACCESSORY_MODE_LOCK_MECHANISM,                .name = "Lock Mechanism Accessory"               , .func = configure_lock_mechanism_accessory },
    { .mode = ACCESSORY_MODE_LOCK_MANAGEMENT,               .name = "Lock Management Accessory"              , .func = configure_lock_management_accessory },
#ifdef IOS9_ENABLED_SERVICES
    { .mode = ACCESSORY_MODE_AIR_QUALITY_SENSOR,            .name = "Air Quality Sensor Accessory"           , .func = configure_air_quality_sensor_accessory },
    { .mode = ACCESSORY_MODE_SECURITY_SYSTEM,               .name = "Security System Accessory"              , .func = configure_security_system_accessory },
    { .mode = ACCESSORY_MODE_CARBON_MONOXIDE_SENSOR,        .name = "Carbon Monoxide Sensor Accessory"       , .func = configure_carbon_monoxide_sensor_accessory },
    { .mode = ACCESSORY_MODE_CONTACT_SENSOR,                .name = "Contact Sensor Accessory"               , .func = configure_contact_sensor_accessory },
    { .mode = ACCESSORY_MODE_DOOR,                          .name = "Door Accessory"                         , .func = configure_door_accessory },
    { .mode = ACCESSORY_MODE_HUMIDITY_SENSOR,               .name = "Humidity Sensor Accessory"              , .func = configure_humidity_sensor_accessory },
    { .mode = ACCESSORY_MODE_LEAK_SENSOR,                   .name = "Leak Sensor Accessory"                  , .func = configure_leak_sensor_accessory },
    { .mode = ACCESSORY_MODE_LIGHT_SENSOR,                  .name = "Light Sensor Accessory"                 , .func = configure_light_sensor_accessory },
    { .mode = ACCESSORY_MODE_OCCUPANCY_SENSOR,              .name = "Occupancy Sensor Accessory"             , .func = configure_occupancy_sensor_accessory },
    { .mode = ACCESSORY_MODE_MOTION_SENSOR,                 .name = "Motion Sensor Accessory"                , .func = configure_motion_sensor_accessory },
    { .mode = ACCESSORY_MODE_SMOKE_SENSOR,                  .name = "Smoke Sensor Accessory"                 , .func = configure_smoke_sensor_accessory },
    { .mode = ACCESSORY_MODE_STATEFUL_PROGRAMMABLE_SWITCH,  .name = "Stateful Programmable Switch Accessory" , .func = configure_stateful_programmable_switch_accessory },
    { .mode = ACCESSORY_MODE_STATELESS_PROGRAMMABLE_SWITCH, .name = "Stateless Programmable Switch Accessory", .func = configure_stateless_programmable_switch_accessory },
    { .mode = ACCESSORY_MODE_TEMPERATURE_SENSOR,            .name = "Temperature Sensor Accessory"           , .func = configure_temperature_sensor_accessory },
    { .mode = ACCESSORY_MODE_WINDOW,                        .name = "Window Accessory"                       , .func = configure_window_accessory },
    { .mode = ACCESSORY_MODE_WINDOW_COVERING,               .name = "Window Covering Accessory"              , .func = configure_window_covering_accessory },
    { .mode = ACCESSORY_MODE_BATTERY,                       .name = "Battery Accessory"                      , .func = configure_battery_accessory },
    { .mode = ACCESSORY_MODE_CARBON_DIOXIDE_SENSOR,         .name = "Carbon Dioxide Sensor Accessory"        , .func = configure_carbon_dioxide_sensor_accessory },
#endif
};
/******************************************************
 *               Function Definitions
 ******************************************************/
void prepare_certificate_accesory_information_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise all accessory information services */
    wiced_homekit_initialise_accessory_information_service( &certification_services_accessory_information_service, iid++ );

    /* Initialize information Service Characteristics for certification accessory */
    wiced_homekit_initialise_name_characteristic         ( &certification_services_name_characteristic, certification_services_name_value, strlen( certification_services_name_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_identify_characteristic     ( &certification_services_identify_characteristic, identify_action, iid++ );
    wiced_homekit_initialise_manufacturer_characteristic ( &certification_services_manufacturer_characteristic, certification_services_manufacturer_value, strlen( certification_services_manufacturer_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_model_characteristic        ( &certification_services_model_characteristic, certification_services_model_value, strlen( certification_services_model_value ), NULL, NULL, iid++ );
#ifdef IOS9_ENABLED_SERVICES
    wiced_homekit_initialise_firmwar_revision_characteristic( &certification_services_firmware_revision, certification_services_firmware_revision_value, strlen( certification_services_firmware_revision_value ), NULL, NULL, iid++ );
#endif
    wiced_homekit_initialise_serial_number_characteristic( &certification_services_serial_number_characteristic, certification_services_serial_number_value, strlen( certification_services_serial_number_value ), NULL, NULL, iid++ );

    wiced_homekit_add_service( accessory, &certification_services_accessory_information_service );

    wiced_homekit_add_characteristic( &certification_services_accessory_information_service, &certification_services_identify_characteristic      );
    wiced_homekit_add_characteristic( &certification_services_accessory_information_service, &certification_services_manufacturer_characteristic  );
    wiced_homekit_add_characteristic( &certification_services_accessory_information_service, &certification_services_model_characteristic         );
    wiced_homekit_add_characteristic( &certification_services_accessory_information_service, &certification_services_name_characteristic          );
#ifdef IOS9_ENABLED_SERVICES
    wiced_homekit_add_characteristic( &certification_services_accessory_information_service, &certification_services_firmware_revision );
#endif
    wiced_homekit_add_characteristic( &certification_services_accessory_information_service, &certification_services_serial_number_characteristic );
}

void prepare_garage_door_opener_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( current_door_state_value,    sizeof(current_door_state_value),    "%s", app_dct->dct_current_door_state_value    );
    snprintf( target_door_state_value,     sizeof(target_door_state_value),     "%s", app_dct->dct_target_door_state_value     );
    snprintf( lock_mechnism_current_value, sizeof(lock_mechnism_current_value), "%s", app_dct->dct_lock_mechnism_current_value );
    snprintf( lock_mechnism_target_value,  sizeof(lock_mechnism_target_value),  "%s", app_dct->dct_lock_mechnism_target_value  );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialize garage door opener service */
    wiced_homekit_initialise_garage_door_opener_service( &garage_door_opener_service, iid++ );

    /* Initialise garage door characteristics */
    wiced_homekit_initialise_current_door_state_characteristic          ( &current_door_state_characteristic, current_door_state_value, strlen( current_door_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_target_door_state_characteristic           ( &target_door_state_characteristic, target_door_state_value, strlen( target_door_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_obstruction_detected_characteristic        ( &obstruction_detected_characteristic, obstruction_detected_value, strlen( obstruction_detected_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_lock_mechanism_current_state_characteristic( &lock_mechanism_current_state_characteristic, lock_mechnism_current_value, strlen( lock_mechnism_current_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_lock_mechanism_target_state_characteristic ( &lock_mechanism_target_state_characteristic, lock_mechnism_target_value, strlen( lock_mechnism_target_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic                        ( &garage_door_name_characteristic,  garage_door_name, strlen( garage_door_name ), NULL, NULL, iid++ );

    /* Add garage door opener service to accessory */
    wiced_homekit_add_service( accessory, &garage_door_opener_service );

    /* Add the required characteristics to the garage opener service */
    wiced_homekit_add_characteristic( &garage_door_opener_service, &current_door_state_characteristic );
    wiced_homekit_add_characteristic( &garage_door_opener_service, &target_door_state_characteristic );
    wiced_homekit_add_characteristic( &garage_door_opener_service, &lock_mechanism_current_state_characteristic );
    wiced_homekit_add_characteristic( &garage_door_opener_service, &lock_mechanism_target_state_characteristic );
    wiced_homekit_add_characteristic( &garage_door_opener_service, &obstruction_detected_characteristic );
    wiced_homekit_add_characteristic( &garage_door_opener_service, &garage_door_name_characteristic );
}

void prepare_thermostat_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( current_heating_cooling_mode_value,        sizeof(current_heating_cooling_mode_value),        "%s", app_dct->dct_heating_cooling_current_value             );
    snprintf( target_heating_cooling_mode_value,         sizeof(target_heating_cooling_mode_value),         "%s", app_dct->dct_heating_cooling_target_value              );
    snprintf( heating_cooling_current_temperature_value, sizeof(heating_cooling_current_temperature_value), "%s", app_dct->dct_heating_cooling_current_temperature_value );
    snprintf( heating_cooling_target_temperature_value,  sizeof(heating_cooling_target_temperature_value),  "%s", app_dct->dct_heating_cooling_target_temperature_value  );
    snprintf( heating_cooling_units_value,               sizeof(heating_cooling_units_value),               "%s", app_dct->dct_heating_cooling_units_value               );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise Thermostat Service */
    wiced_homekit_initialise_thermostat_service( &thermostat_service, iid++ );

    /* Initialise Thermostat characteristics */
    wiced_homekit_initialise_heating_cooling_current_characteristic ( &current_heating_cooling_mode_characteristic, current_heating_cooling_mode_value, strlen( current_heating_cooling_mode_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_heating_cooling_target_characteristic  ( &target_heating_cooling_mode_characteristic, target_heating_cooling_mode_value, strlen( target_heating_cooling_mode_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_temperature_current_characteristic     ( &temperature_current_characteristic, heating_cooling_current_temperature_value, strlen( heating_cooling_current_temperature_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_temperature_target_characteristic      ( &temperature_target_characteristic, heating_cooling_target_temperature_value, strlen( heating_cooling_target_temperature_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_temperature_units_characteristic       ( &temperature_units_characteristic, heating_cooling_units_value, strlen( heating_cooling_units_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic                    ( &thermostat_name_characteristic, heating_cooling_name_value, strlen( heating_cooling_name_value ), NULL, NULL, iid++ );

    /* Add Thermostat Service to Accessory */
    wiced_homekit_add_service( accessory, &thermostat_service );

    /* Add the required characteristics to the Thermostat Service */
    wiced_homekit_add_characteristic( &thermostat_service, &current_heating_cooling_mode_characteristic );
    wiced_homekit_add_characteristic( &thermostat_service, &target_heating_cooling_mode_characteristic );
    wiced_homekit_add_characteristic( &thermostat_service, &temperature_current_characteristic );
    wiced_homekit_add_characteristic( &thermostat_service, &temperature_target_characteristic );
    wiced_homekit_add_characteristic( &thermostat_service, &temperature_units_characteristic );
    wiced_homekit_add_characteristic( &thermostat_service, &thermostat_name_characteristic );
}

/**********************************
 * Light bulb service
 **********************************/
void prepare_light_bulb_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( lightbulb_on_value, sizeof(lightbulb_on_value), "%s", app_dct->dct_lightbulb_on_value );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise lightbulb service */
    wiced_homekit_initialise_lightbulb_service( &lightbulb_service, iid++ );

    /* Initialise Lightbulb characteristics */
    wiced_homekit_initialise_on_characteristic           ( &lightbulb_on_characteristic,         lightbulb_on_value,         strlen( lightbulb_on_value ),         NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic         ( &lightbulb_name_characteristic,       lightbulb_name_value,       strlen( lightbulb_name_value ),       NULL, NULL, iid++ );

    /* Add Lightbulb service to accessory */
    wiced_homekit_add_service( accessory, &lightbulb_service );

    /* Add the required characteristics to the Lightbulb service */
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_on_characteristic );
    wiced_homekit_add_characteristic( &lightbulb_service, &lightbulb_name_characteristic );
}

/**********************************
 * Fan service
 **********************************/
void prepare_fan_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( fan_on_value, sizeof(fan_on_value), "%s", app_dct->dct_fan_on_value );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise fan service */
    wiced_homekit_initialise_fan_service( &fan_service, iid++ );

    /* Initialise Fan characteristics */
    wiced_homekit_initialise_on_characteristic( &fan_on_characteristic, fan_on_value, strlen( fan_on_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &fan_name_characteristic, fan_name_value, strlen( fan_name_value ), NULL, NULL, iid++ );

    /* Add fan service */
    wiced_homekit_add_service( accessory, &fan_service );

    wiced_homekit_add_characteristic( &fan_service, &fan_on_characteristic );
    wiced_homekit_add_characteristic( &fan_service, &fan_name_characteristic );
}

/**********************************
 * Outlet Service
 **********************************/
void prepare_outlet_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( outlet_on_value,     sizeof(outlet_on_value),     "%s", app_dct->dct_outlet_on_value     );
    snprintf( outlet_in_use_value, sizeof(outlet_in_use_value), "%s", app_dct->dct_outlet_in_use_value );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise outlet service */
    wiced_homekit_initialise_outlet_service( &outlet_service, iid++ );

    /* Initialise outlet characteristics */
    wiced_homekit_initialise_on_characteristic( &outlet_on_characteristic, outlet_on_value, strlen( outlet_on_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &outlet_name_characteristic, outlet_name_value, strlen( outlet_name_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_outlet_in_use_characteristic( &outlet_in_use_characteristic, outlet_in_use_value, strlen( outlet_in_use_value ), NULL, NULL, iid++ );

    /* Add outlet service to accessory */
    wiced_homekit_add_service( accessory, &outlet_service );

    /* Add the required characteristics to the outlet service */
    wiced_homekit_add_characteristic( &outlet_service, &outlet_on_characteristic );
    wiced_homekit_add_characteristic( &outlet_service, &outlet_name_characteristic );
    wiced_homekit_add_characteristic( &outlet_service, &outlet_in_use_characteristic );
}

/**********************************
 * Switch Service
 **********************************/
void prepare_switch_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( switch_on_value, sizeof(switch_on_value), "%s", app_dct->dct_switch_on_value );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise switch service */
    wiced_homekit_initialise_switch_service( &switch_service, iid++ );

    /* Initialise switch characteristics */
    wiced_homekit_initialise_on_characteristic( &switch_on_characteristic, switch_on_value, strlen( switch_on_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &switch_name_characteristic, switch_name_value, strlen( switch_name_value ), NULL, NULL, iid++ );

    /* Add Switch Outlet Service to Accessory */
    wiced_homekit_add_service( accessory, &switch_service );

    /* Add the required characteristics to the Switch Outlet Service */
    wiced_homekit_add_characteristic( &switch_service, &switch_on_characteristic );
    wiced_homekit_add_characteristic( &switch_service, &switch_name_characteristic );
}

/**********************************
 * Lock Mechanism Service
 **********************************/
void prepare_lock_mechanism_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( lock_mechnism_current_state_value, sizeof(lock_mechnism_current_state_value), "%s", app_dct->dct_lock_mechnism_current_state_value );
    snprintf( lock_mechnism_target_state_value,  sizeof(lock_mechnism_target_state_value),  "%s", app_dct->dct_lock_mechnism_target_state_value  );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise Lock mechanism service and characteristics */
    wiced_homekit_initialise_lock_mechanism_service( &lock_mechanism_service, iid++ );

    wiced_homekit_initialise_lock_mechanism_current_state_characteristic  ( &lock_mechanism_current_characteristic, lock_mechnism_current_state_value, strlen( lock_mechnism_current_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_lock_mechanism_target_state_characteristic   ( &lock_mechanism_target_characteristic,  lock_mechnism_target_state_value,  strlen( lock_mechnism_target_state_value ),  NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic                          ( &lock_mechanism_name_characteristic,    lock_mechnism_name_value,          strlen( lock_mechnism_name_value ),          NULL, NULL, iid++ );

    /* Add Lock mechanism service */
    wiced_homekit_add_service( accessory, &lock_mechanism_service );

    wiced_homekit_add_characteristic( &lock_mechanism_service, &lock_mechanism_current_characteristic );
    wiced_homekit_add_characteristic( &lock_mechanism_service, &lock_mechanism_target_characteristic );
    wiced_homekit_add_characteristic( &lock_mechanism_service, &lock_mechanism_name_characteristic );
}

/**********************************
 * Lock Management Service
 **********************************/
void prepare_lock_management_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( lock_management_control_point_value, sizeof(lock_management_control_point_value), "%s", app_dct->dct_lock_management_command_value );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise lock management service and characteristics */
    wiced_homekit_initialise_lock_management_service ( &lock_management_service, iid++ );

    wiced_homekit_initialise_version_characteristic                       ( &lock_management_version_characteristic,        lock_management_version_value, strlen( lock_management_version_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_lock_management_control_point_characteristic ( &lock_management_control_point_characteristic,  lock_management_control_point_value, strlen( lock_management_control_point_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic                          ( &lock_management_name_characteristic,           lock_management_name_value,    strlen( lock_management_name_value ),    NULL, NULL, iid++ );

    /* Add Lock Management service */
    wiced_homekit_add_service( accessory, &lock_management_service );

    wiced_homekit_add_characteristic( &lock_management_service, &lock_management_control_point_characteristic );
    wiced_homekit_add_characteristic( &lock_management_service, &lock_management_version_characteristic );
    wiced_homekit_add_characteristic( &lock_management_service, &lock_management_name_characteristic );
}

#ifdef IOS9_ENABLED_SERVICES
/**********************************
 * Air-quality Service
 **********************************/
void prepare_air_quality_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Air Quality Sensor service and characteristics */
    wiced_homekit_initialise_air_quality_sensor_service( &air_quality_sensor_service, iid++ );

    wiced_homekit_initialise_air_quality_characteristic( &air_quality_characteristic, air_quality_value, strlen( air_quality_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &air_quality_name_characteristic, air_quality_name_value, strlen( air_quality_name_value ), NULL, NULL, iid++ );

    /* Add Air Quality Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &air_quality_sensor_service );

    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_quality_characteristic );
    wiced_homekit_add_characteristic( &air_quality_sensor_service, &air_quality_name_characteristic );
}

/**********************************
 * Security system Service
 **********************************/
void prepare_security_system_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( security_system_current_state_value, sizeof(security_system_current_state_value), "%s", app_dct->dct_security_system_current_state_value );
    snprintf( security_system_target_state_value,  sizeof(security_system_target_state_value),  "%s", app_dct->dct_security_system_target_state_value  );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise Security system service and characteristics */
    wiced_homekit_initialise_security_system_service( &security_system_service, iid++ );

    wiced_homekit_initialise_security_system_current_state_characteristic( &security_system_current_state_characteristic, security_system_current_state_value, strlen( security_system_current_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_security_system_target_state_characteristic( &security_system_target_state_characteristic, security_system_target_state_value, strlen( security_system_target_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &security_system_name_characteristic, security_system_name_value, strlen( security_system_name_value ), NULL, NULL, iid++ );

    /* Add Security system service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &security_system_service );

    wiced_homekit_add_characteristic( &security_system_service, &security_system_current_state_characteristic );
    wiced_homekit_add_characteristic( &security_system_service, &security_system_target_state_characteristic );
    wiced_homekit_add_characteristic( &security_system_service, &security_system_name_characteristic );
}

/**********************************
 * Carbon Monoxide Service
 **********************************/
void prepare_carbon_monoxide_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Carbon Monoxide Sensor service and characteristics */
    wiced_homekit_initialise_carbon_monoxide_sensor_service( &carbon_monoxide_sensor_service, iid++ );
    wiced_homekit_initialise_carbon_monoxide_detected_characteristic( &carbon_monoxide_detected_characteristic, carbon_monoxide_detected_value, strlen( carbon_monoxide_detected_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &carbon_monoxide_name_characteristic, carbon_monoxide_name_value, strlen( carbon_monoxide_name_value ), NULL, NULL, iid++ );

    /* Add Carbon Monoxide service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &carbon_monoxide_sensor_service );
    wiced_homekit_add_characteristic( &carbon_monoxide_sensor_service, &carbon_monoxide_detected_characteristic );
    wiced_homekit_add_characteristic( &carbon_monoxide_sensor_service, &carbon_monoxide_name_characteristic );
}

/**********************************
 * Initialize contact sensor Service
 **********************************/
void prepare_contact_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Contact Sensor service and characteristics */
    wiced_homekit_initialise_contact_sensor_service( &contact_sensor_service, iid++ );
    wiced_homekit_initialise_contact_sensor_state_characteristic( &contact_sensor_state_characteristic, contact_sensor_state_value, strlen( contact_sensor_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &contact_sensor_name_characteristic, contact_sensor_name_value, strlen( contact_sensor_name_value ), NULL, NULL, iid++ );

    /* Add Contact Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &contact_sensor_service );
    wiced_homekit_add_characteristic( &contact_sensor_service, &contact_sensor_state_characteristic );
    wiced_homekit_add_characteristic( &contact_sensor_service, &contact_sensor_name_characteristic );
}

/**********************************
 * Door service
 **********************************/
void prepare_door_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( door_current_position_value, sizeof(door_current_position_value), "%s", app_dct->dct_door_current_position_value );
    snprintf( door_target_position_value,  sizeof(door_target_position_value),  "%s", app_dct->dct_door_target_position_value  );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise Door service and characteristics */
    wiced_homekit_initialise_door_service( &door_service, iid++ );
    wiced_homekit_initialise_current_position_characteristic( &door_current_position_characteristic, door_current_position_value, strlen( door_current_position_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_target_position_characteristic( &door_target_position_characteristic, door_target_position_value, strlen( door_target_position_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_position_state_characteristic( &door_position_state_characteristic, door_position_state_value, strlen( door_position_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &door_name_characteristic, door_name_value, strlen( door_name_value ), NULL, NULL, iid++ );

    /* Add Door service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &door_service );
    wiced_homekit_add_characteristic( &door_service, &door_current_position_characteristic );
    wiced_homekit_add_characteristic( &door_service, &door_target_position_characteristic );
    wiced_homekit_add_characteristic( &door_service, &door_position_state_characteristic );
    wiced_homekit_add_characteristic( &door_service, &door_name_characteristic );
}

/**********************************
 * Humidity sensor service
 **********************************/
void prepare_humidity_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Humidity Sensor service and characteristics */
    wiced_homekit_initialise_humidity_sensor_service( &humidity_sensor_service, iid++ );
    wiced_homekit_initialise_current_relative_humidity_characteristic( &current_relative_humidity_characteristic, current_relative_humidity_value, strlen( current_relative_humidity_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &humidity_sensor_name_characteristic, humidity_sensor_name_value, strlen( humidity_sensor_name_value ), NULL, NULL, iid++ );

    /* Add Humidity Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &humidity_sensor_service );
    wiced_homekit_add_characteristic( &humidity_sensor_service, &current_relative_humidity_characteristic );
    wiced_homekit_add_characteristic( &humidity_sensor_service, &humidity_sensor_name_characteristic );
}

/**********************************
 * Leak sensor service
 **********************************/
void prepare_leak_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Leak Sensor service and characteristics */
    wiced_homekit_initialise_leak_sensor_service( &leak_sensor_service, iid++ );
    wiced_homekit_initialise_leak_detected_characteristic( &leak_detected_characteristic, leak_detected_value, strlen( leak_detected_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &leak_sensor_name_characteristic, leak_sensor_name_value, strlen( leak_sensor_name_value ), NULL, NULL, iid++ );

    /* Add Leak Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &leak_sensor_service );
    wiced_homekit_add_characteristic( &leak_sensor_service, &leak_detected_characteristic );
    wiced_homekit_add_characteristic( &leak_sensor_service, &leak_sensor_name_characteristic );
}

/**********************************
 * Light sensor service
 **********************************/
void prepare_light_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Light Sensor service and characteristics */
    wiced_homekit_initialise_light_sensor_service( &light_sensor_service, iid++ );
    wiced_homekit_initialise_current_ambient_light_level_characteristic( &current_light_level_characteristic, current_light_level_value, strlen( current_light_level_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &light_sensor_name_characteristic, light_sensor_name_value, strlen( light_sensor_name_value ), NULL, NULL, iid++ );

    /* Add  Light Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &light_sensor_service );
    wiced_homekit_add_characteristic( &light_sensor_service, &current_light_level_characteristic );
    wiced_homekit_add_characteristic( &light_sensor_service, &light_sensor_name_characteristic );
}

/**********************************
 * Motion sensor service
 **********************************/
void prepare_motion_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Motion Sensor service and characteristics */
    wiced_homekit_initialise_motion_sensor_service( &motion_sensor_service, iid++ );
    wiced_homekit_initialise_motion_detected_characteristic( &motion_detected_characteristic, motion_detected_value, strlen( motion_detected_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &motion_sensor_name_characteristic, motion_sensor_name_value, strlen( motion_sensor_name_value ), NULL, NULL, iid++ );

    /* Add  Motion Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &motion_sensor_service );
    wiced_homekit_add_characteristic( &motion_sensor_service, &motion_detected_characteristic );
    wiced_homekit_add_characteristic( &motion_sensor_service, &motion_sensor_name_characteristic );
}

/**********************************
 * Occupancy sensor service
 **********************************/
void prepare_occupancy_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Occupancy Sensor service and characteristics */
    wiced_homekit_initialise_occupancy_sensor_service( &occupancy_sensor_service, iid++ );
    wiced_homekit_initialise_occupancy_detected_characteristic( &occupancy_detected_characteristic, occupancy_detected_value, strlen( occupancy_detected_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &occupancy_sensor_name_characteristic, occupancy_sensor_name_value, strlen( occupancy_sensor_name_value ), NULL, NULL, iid++ );

    /* Add Occupancy Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &occupancy_sensor_service );
    wiced_homekit_add_characteristic( &occupancy_sensor_service, &occupancy_detected_characteristic );
    wiced_homekit_add_characteristic( &occupancy_sensor_service, &occupancy_sensor_name_characteristic );
}

/**********************************
 * Smoke sensor service
 **********************************/
void prepare_smoke_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Smoke Sensor service and characteristics */
    wiced_homekit_initialise_smoke_sensor_service( &smoke_sensor_service, iid++ );
    wiced_homekit_initialise_smoke_detected_characteristic( &smoke_detected_characteristic, smoke_detected_value, strlen( smoke_detected_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &smoke_sensor_name_characteristic, smoke_sensor_name_value, strlen( smoke_sensor_name_value ), NULL, NULL, iid++ );

    /* Add Smoke Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &smoke_sensor_service );
    wiced_homekit_add_characteristic( &smoke_sensor_service, &smoke_detected_characteristic );
    wiced_homekit_add_characteristic( &smoke_sensor_service, &smoke_sensor_name_characteristic );
}

/**********************************
 * Stateful programmable switch service
 **********************************/
void prepare_stateful_programmable_switch_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( programmable_switch_output_state_value, sizeof(programmable_switch_output_state_value), "%s", app_dct->dct_programmable_switch_output_state_value );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise Stateful Programmable Switch service and characteristics */
    wiced_homekit_initialise_stateful_programmable_switch_service( &stateful_programmable_switch_service, iid++ );
    wiced_homekit_initialise_programmable_switch_event_characteristic( &programmable_switch_event_characteristic, programmable_switch_event_value, strlen( programmable_switch_event_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_programmable_switch_output_state_characteristic( &programmable_switch_output_state_characteristic, programmable_switch_output_state_value, strlen( programmable_switch_output_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &stateful_programmable_switch_name_characteristic, stateful_programmable_switch_name_value, strlen( stateful_programmable_switch_name_value ), NULL, NULL, iid++ );

    /* Add Stateful Programmable Switch service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &stateful_programmable_switch_service );
    wiced_homekit_add_characteristic( &stateful_programmable_switch_service, &programmable_switch_event_characteristic );
    wiced_homekit_add_characteristic( &stateful_programmable_switch_service, &programmable_switch_output_state_characteristic );
    wiced_homekit_add_characteristic( &stateful_programmable_switch_service, &stateful_programmable_switch_name_characteristic );
}

/**********************************
 * Stateless programmable switch service
 **********************************/
void prepare_stateless_programmable_switch_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Stateless Programmable Switch service and characteristics */
    wiced_homekit_initialise_stateless_programmable_switch_service( &stateless_programmable_switch_service, iid++);
    wiced_homekit_initialise_programmable_switch_event_characteristic( &stateless_programmable_switch_event_characteristic, stateless_programmable_switch_event_value, strlen( stateless_programmable_switch_event_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &stateless_programmable_switch_name_characteristic, stateless_programmable_switch_name_value, strlen( stateless_programmable_switch_name_value ), NULL, NULL, iid++ );

    /* Add Stateless Programmable Switch service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &stateless_programmable_switch_service );
    wiced_homekit_add_characteristic( &stateless_programmable_switch_service, &stateless_programmable_switch_event_characteristic );
    wiced_homekit_add_characteristic( &stateless_programmable_switch_service, &stateless_programmable_switch_name_characteristic );
}
/**********************************
 * Temperature sensor service
 **********************************/
void prepare_temperature_sensor_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Temperature Sensor service and characteristics */
    wiced_homekit_initialise_temperature_sensor_service( &temperature_sensor_service, iid++ );
    wiced_homekit_initialise_temperature_current_characteristic( &current_temperature_characteristic, current_temperature_value, strlen( current_temperature_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &temperature_sensor_name_characteristic, temperature_sensor_name_value, strlen( temperature_sensor_name_value ), NULL, NULL, iid++ );

    /* Add Temperature Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &temperature_sensor_service );
    wiced_homekit_add_characteristic( &temperature_sensor_service, &current_temperature_characteristic );
    wiced_homekit_add_characteristic( &temperature_sensor_service, &temperature_sensor_name_characteristic );
}
/**********************************
 * Window service
 **********************************/
void prepare_window_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( window_current_position_value, sizeof(window_current_position_value), "%s", app_dct->dct_window_current_position_value );
    snprintf( window_target_position_value,  sizeof(window_target_position_value),  "%s", app_dct->dct_window_target_position_value  );
    snprintf( window_position_state_value,   sizeof(window_position_state_value),   "%s", app_dct->dct_window_position_state_value   );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise Window service and characteristics */
    wiced_homekit_initialise_window_service( &window_service, iid++ );
    wiced_homekit_initialise_current_position_characteristic( &window_current_position_characteristic, window_current_position_value, strlen( window_current_position_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_target_position_characteristic( &window_target_position_characteristic, window_target_position_value, strlen( window_target_position_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_position_state_characteristic( &window_position_state_characteristic, window_position_state_value, strlen( window_position_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &window_name_characteristic, window_name_value, strlen( window_name_value ), NULL, NULL, iid++ );

    /* Add Window service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &window_service );
    wiced_homekit_add_characteristic( &window_service, &window_current_position_characteristic );
    wiced_homekit_add_characteristic( &window_service, &window_target_position_characteristic );
    wiced_homekit_add_characteristic( &window_service, &window_position_state_characteristic );
    wiced_homekit_add_characteristic( &window_service, &window_name_characteristic );
}
/**********************************
 * Window covering service
 **********************************/
void prepare_window_covering_service( wiced_homekit_accessories_t* accessory )
{
    homekit_application_dct_structure_t *app_dct;

    /* Reading the characteristics values from persistent storage */
    wiced_dct_read_lock( (void**) &app_dct, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    snprintf( current_position_value, sizeof(current_position_value), "%s", app_dct->dct_current_position_value );
    snprintf( target_position_value,  sizeof(target_position_value),  "%s", app_dct->dct_target_position_value  );
    snprintf( position_state_value,   sizeof(position_state_value),   "%s", app_dct->dct_position_state_value   );

    wiced_dct_read_unlock( (void*)app_dct, WICED_FALSE );

    /* Initialise Window Covering service and characteristics */
    wiced_homekit_initialise_window_covering_service( &window_covering_service, iid++ );
    wiced_homekit_initialise_current_position_characteristic( &current_position_characteristic, current_position_value, strlen( current_position_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_target_position_characteristic( &target_position_characteristic, target_position_value, strlen( target_position_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_position_state_characteristic( &position_state_characteristic, position_state_value, strlen( position_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &window_covering_name_characteristic, window_covering_name_value, strlen( window_covering_name_value ), NULL, NULL, iid++ );

    /* Add Window Covering service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &window_covering_service );
    wiced_homekit_add_characteristic( &window_covering_service, &current_position_characteristic );
    wiced_homekit_add_characteristic( &window_covering_service, &target_position_characteristic );
    wiced_homekit_add_characteristic( &window_covering_service, &position_state_characteristic );
    wiced_homekit_add_characteristic( &window_covering_service, &window_covering_name_characteristic );
}
/**********************************
 * Battery service
 **********************************/
void prepare_battery_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Battery service and characteristics */
    wiced_homekit_initialise_battery_service( &battery_service, iid++ );
    wiced_homekit_initialise_battery_level_characteristic( &battery_level_characteristic, battery_level_value, strlen( battery_level_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_charging_state_characteristic( &charging_state_characteristic, charging_state_value, strlen( charging_state_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_status_low_battery_characteristic( &status_low_battery_characteristic, status_low_battery_value, strlen( status_low_battery_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &battery_name_characteristic, battery_name_value, strlen( battery_name_value ), NULL, NULL, iid++ );

    /* Add Battery service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &battery_service );
    wiced_homekit_add_characteristic( &battery_service, &battery_level_characteristic );
    wiced_homekit_add_characteristic( &battery_service, &charging_state_characteristic );
    wiced_homekit_add_characteristic( &battery_service, &status_low_battery_characteristic );
    wiced_homekit_add_characteristic( &battery_service, &battery_name_characteristic );
}

/**********************************
 * Carbon dioxide sensor service
 **********************************/
void prepare_carbon_dioxide_service( wiced_homekit_accessories_t* accessory )
{
    /* Initialise Carbon Dioxide Sensor service and characteristics */
    wiced_homekit_initialise_carbon_dioxide_sensor_service( &carbon_dioxide_sensor_service, iid++ );
    wiced_homekit_initialise_carbon_dioxide_detected_characteristic( &carbon_dioxide_detected_characteristic, carbon_dioxide_detected_value, strlen( carbon_dioxide_detected_value ), NULL, NULL, iid++ );
    wiced_homekit_initialise_name_characteristic( &carbon_dioxide_name_characteristic, carbon_dioxide_name_value, strlen( carbon_dioxide_name_value ), NULL, NULL, iid++ );

    /* Add Carbon Dioxide Sensor service and characteristics to Accessory */
    wiced_homekit_add_service( accessory, &carbon_dioxide_sensor_service );
    wiced_homekit_add_characteristic( &carbon_dioxide_sensor_service, &carbon_dioxide_detected_characteristic );
    wiced_homekit_add_characteristic( &carbon_dioxide_sensor_service, &carbon_dioxide_name_characteristic );
}
#endif

static void configure_garage_door_opener_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_garage_door_opener_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = GARAGE_DOOR_OPENER;
}

static void configure_thermostat_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_thermostat_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = THERMOSTAT;
}

static void configure_light_bulb_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_light_bulb_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = LIGHTBULB;
}

static void configure_fan_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_fan_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = FAN;
}

static void configure_outlet_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_outlet_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = OUTLET;
}

static void configure_switch_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_switch_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SWITCH;
}

static void configure_lock_mechanism_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_lock_mechanism_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = DOOR_LOCK;
}

static void configure_lock_management_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_lock_management_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = DOOR_LOCK;
}

#ifdef IOS9_ENABLED_SERVICES
static void configure_air_quality_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_air_quality_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_security_system_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_security_system_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SECURITY_SYSTEM;
}

static void configure_carbon_monoxide_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_carbon_monoxide_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_contact_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_contact_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_door_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_door_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = DOOR;
}

static void configure_humidity_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_humidity_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_leak_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_leak_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_light_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_light_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_occupancy_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_occupancy_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_motion_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_motion_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_smoke_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_smoke_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_stateful_programmable_switch_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_stateful_programmable_switch_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = PROGRAMMABLE_SWITCH;
}

static void configure_stateless_programmable_switch_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_stateless_programmable_switch_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = PROGRAMMABLE_SWITCH;
}

static void configure_temperature_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_temperature_sensor_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}

static void configure_window_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_window_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = WINDOW;
}

static void configure_window_covering_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_window_covering_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = WINDOW_COVERING;
}

static void configure_battery_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_battery_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = OTHER;
}

static void configure_carbon_dioxide_sensor_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_carbon_dioxide_service( &certification_services_accessory );
    apple_homekit_discovery_settings.accessory_category_identifier = SENSOR;
}
#endif

static void configure_multi_service_accessory()
{
    wiced_homekit_add_accessory( &certification_services_accessory, aid++ );
    prepare_certificate_accesory_information_service( &certification_services_accessory );
    prepare_garage_door_opener_service( &certification_services_accessory );
    prepare_thermostat_service( &certification_services_accessory );
    prepare_light_bulb_service( &certification_services_accessory );
    prepare_fan_service( &certification_services_accessory );
    prepare_outlet_service( &certification_services_accessory );
    prepare_switch_service( &certification_services_accessory );
    prepare_lock_mechanism_service( &certification_services_accessory );
    prepare_lock_management_service( &certification_services_accessory );
#ifdef IOS9_ENABLED_SERVICES
    prepare_air_quality_sensor_service( &certification_services_accessory );
    prepare_security_system_service( &certification_services_accessory );
    prepare_carbon_monoxide_sensor_service( &certification_services_accessory );
    prepare_contact_sensor_service( &certification_services_accessory );
    prepare_door_service( &certification_services_accessory );
    prepare_humidity_sensor_service( &certification_services_accessory );
    prepare_leak_sensor_service( &certification_services_accessory );
    prepare_light_sensor_service( &certification_services_accessory );
    prepare_motion_sensor_service( &certification_services_accessory );
    prepare_occupancy_sensor_service( &certification_services_accessory );
    prepare_smoke_sensor_service( &certification_services_accessory );
    prepare_stateful_programmable_switch_service( &certification_services_accessory );
    prepare_stateless_programmable_switch_service( &certification_services_accessory );
    prepare_temperature_sensor_service( &certification_services_accessory );
    prepare_window_service( &certification_services_accessory );
    prepare_window_covering_service( &certification_services_accessory );
    prepare_battery_service( &certification_services_accessory );
    prepare_carbon_dioxide_service( &certification_services_accessory );
#endif

    apple_homekit_discovery_settings.accessory_category_identifier = OTHER;
}

void application_start( )
{
    wiced_result_t  result;
    uint8_t*        pval;
    uint32_t        wiced_heap_allocated_for_database;

    wiced_init( );

    /* Read the characteristics from persistent storage and set the accessory status */
    wiced_dct_read_lock( (void**) &pval, WICED_FALSE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( uint8_t ) );
    current_accessory_mode = *pval;
    wiced_dct_read_unlock( (void*)pval, WICED_FALSE );

    /* Check if it's a valid accessory mode */
    if ( (current_accessory_mode >= 0) && (current_accessory_mode < sizeof(accessory_mode_list) / sizeof(accessory_mode_list[0])) )
    {
        WPRINT_APP_INFO( ("Current Accessory Mode = [%d] : [%s]\r\n", accessory_mode_list[current_accessory_mode].mode, accessory_mode_list[current_accessory_mode].name) );

        /* Update accessory name based on accessory mode */
        snprintf(accessory_name, sizeof(accessory_name), "WICED HK %s", accessory_mode_list[current_accessory_mode].name);
        snprintf(certification_services_name_value, sizeof(certification_services_name_value), "\"%s\"", accessory_name);
    }
    else
    {
        WPRINT_APP_INFO( ("Invalid Accessory Mode = [%d]\n", current_accessory_mode) );
        return;
    }

    apple_homekit_discovery_settings.name = accessory_name;
#ifdef RUN_ON_ETHERNET_INTERFACE
    apple_homekit_discovery_settings.interface = WICED_ETHERNET_INTERFACE;
#else
    apple_homekit_discovery_settings.interface = WICED_STA_INTERFACE;
#endif

#ifdef WICED_HOMEKIT_DO_NOT_USE_MFI
    /* Bring up the network interface */
    result = wiced_network_up( apple_homekit_discovery_settings.interface, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Unable to bring up network connection\n"));
        return;
    }
    homekit_get_random_device_id( apple_homekit_discovery_settings.device_id );
#else

    apple_wac_info.name = apple_homekit_discovery_settings.name;

    homekit_get_random_device_id( apple_homekit_discovery_settings.device_id );
    memcpy( apple_wac_info.device_random_id, apple_homekit_discovery_settings.device_id, sizeof(apple_homekit_discovery_settings.device_id) );

    /* Bring up the network interface */
    result = wiced_network_up( apple_homekit_discovery_settings.interface, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Invalid Entry in DCT (or) the configured WiFi network is not UP at present. Starting WAC\n"));
        result = apple_wac_configure( &apple_wac_info );

        /* Clear once WAC is complete, and no longer used */
        if ( apple_wac_info.out_play_password != NULL )
        {
            free( apple_wac_info.out_play_password );
        }

        if ( result != WICED_SUCCESS )
        {
            /* Clear once WAC is complete, and no longer used */
            if ( apple_wac_info.out_new_name != NULL )
            {
                free( apple_wac_info.out_new_name );
            }

            WPRINT_APP_INFO( ("WAC failure\n") );
            return;
        }

        /* If we were assigned a new name, update with new name */
        if ( apple_wac_info.out_new_name != NULL )
        {
            snprintf( accessory_name, sizeof(accessory_name), "%s", apple_wac_info.out_new_name );
            snprintf(certification_services_name_value, sizeof(certification_services_name_value), "\"%s\"", accessory_name);
            free( apple_wac_info.out_new_name );
        }
    }

#endif

    /* Configure the accessory based on the mode */
    accessory_mode_list[current_accessory_mode].func();

#ifdef HOMEKIT_ICLOUD_ENABLE
    if (strlen(APPLE_CA_CERTIFICATE_FOR_ICLOUD_SERVER) == 0)
    {
        WPRINT_APP_INFO((" === INFO === : iCloud Root CA certificate is not loaded\n"));
        /* Add accessory Relay Service & it's characteristics */
        wiced_homekit_add_relay_service( &certification_services_accessory, iid++, NULL, 0 );
    }
    else
    {
        wiced_homekit_add_relay_service( &certification_services_accessory, iid++, APPLE_CA_CERTIFICATE_FOR_ICLOUD_SERVER, strlen(APPLE_CA_CERTIFICATE_FOR_ICLOUD_SERVER) );
    }
#endif

#ifdef USE_FIXED_SETUP_CODE
    wiced_configure_accessory_password_for_device_with_no_display( (char*)FIXED_SETUP_CODE );
#else
    wiced_configure_accessory_password_for_device_with_display( wiced_homekit_display_password );
#endif

    wiced_register_url_identify_callback( identify_action );

    wiced_register_value_update_callback( wiced_homekit_actions_on_remote_value_update );

    WPRINT_APP_INFO( ("+-------------------+\r\n") );
    WPRINT_APP_INFO( ("+ HomeKit Loading...+\r\n") );
    WPRINT_APP_INFO( ("+-------------------+\r\n") );

    if ( wiced_homekit_start( &apple_homekit_discovery_settings, &wiced_heap_allocated_for_database ) != WICED_SUCCESS )
    {
        return;
    }

    WPRINT_APP_INFO( ("+-------------------+\r\n") );
    WPRINT_APP_INFO( ("+ HomeKit Ready!    +\r\n") );
    WPRINT_APP_INFO( ("+-------------------+\r\n") );

    WPRINT_APP_INFO( ("\r\nBytes in heap allocated for Accessory Database = %lu\r\n", wiced_heap_allocated_for_database ) );
    WPRINT_APP_INFO( ("\r\n\r\n") );

#ifdef USE_FIXED_SETUP_CODE
    WPRINT_APP_INFO( ("\r\n") );
    WPRINT_APP_INFO( ("+--------------------------------+\r\n") );
    WPRINT_APP_INFO( ("+ Pairing Password: [%s] +\r\n", (char*)FIXED_SETUP_CODE ) );
    WPRINT_APP_INFO( ("+--------------------------------+\r\n") );
    WPRINT_APP_INFO( ("\r\n") );
#endif

    WPRINT_APP_INFO( ("****************************************************\r\n") );
    WPRINT_APP_INFO( ("****** HomeKit Certification Test Application ******\r\n") );
    WPRINT_APP_INFO( ("****************************************************\r\n") );

    WPRINT_APP_INFO( ("Type \"help\" for a listing of all commands and options \r\n") );

    command_console_init( STDIO_UART, sizeof(test_command_buffer), test_command_buffer, TEST_COMMAND_HISTORY_LENGTH, test_command_history_buffer, " ");
    console_add_cmd_table( test_command_table );

    while( 1 )
    {
        wiced_rtos_delay_milliseconds( 250 );
    }
}

wiced_result_t wiced_homekit_actions_on_remote_value_update( wiced_homekit_characteristic_value_update_list_t* update_list )
{
    wiced_result_t  result;
    int             i;
    int             j;
    uint16_t        offset;
    uint8_t         data_length;
    char            updated_value[20]; /* Maximum size of characteristics value stored in persistent storage is 20 bytes */
    char*           dct_update_value;
    wiced_homekit_characteristics_t* updated_characteristic  = NULL;
    wiced_homekit_characteristics_t* current_mode            = NULL;

    /* Cycle through list of all updated characteristics */
    for( i=0; i < update_list->number_of_updates ; i++ )
    {
        /* If Fan Accessory, then check to see what services are being activated */
        if( certification_services_accessory.instance_id == update_list->characteristic_descriptor[i].accessory_instance_id )
        {
            WPRINT_APP_INFO( ("\r\n CERTIFICATION ACCESSORY \r\n") );

            if ( target_door_state_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {

                WPRINT_APP_INFO( ("\r\n GARAGE DOOR OPENER SERVICE \r\n") );

                updated_characteristic = &target_door_state_characteristic;

                if( memcmp( target_door_state_characteristic.value.new.value, "0", target_door_state_characteristic.value.new.value_length ) == COMPARE_MATCH )
                {
                    WPRINT_APP_INFO( ("\r\n\t Received target door OPEN request \r\n") );
                }
                else
                {
                    WPRINT_APP_INFO( ("\r\n\t Received target door CLOSE request \r\n") );
                }
                memcpy( updated_value, target_door_state_characteristic.value.new.value, target_door_state_characteristic.value.new.value_length );
                updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
                data_length = updated_characteristic->value.new.value_length + 1;
                offset = HOMEKIT_APP_DCT_OFFSET(dct_target_door_state_value);
            }
            else if ( lock_mechanism_target_state_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                WPRINT_APP_INFO( ("\r\n GARAGE DOOR OPENER SERVICE \r\n") );

                updated_characteristic = &lock_mechanism_target_state_characteristic;

                switch( *lock_mechanism_target_state_characteristic.value.new.value )
                {
                    case LOCK_MECHANISM_TARGET_STATE_UNSECURED_LOCK:
                        WPRINT_APP_INFO( ("\r\n\t Received lock mechanism UNSECURED request \r\n") );
                        break;
                    case LOCK_MECHANISM_TARGET_STATE_SECURED_LOCK:
                        WPRINT_APP_INFO( ("\r\n\t Received mechanism SECURED request \r\n") );
                        break;
                    default:
                        WPRINT_APP_INFO( ("\r\n\t Received mechanism RESERVED LOCK request \r\n") );
                        break;
                }
                memcpy( updated_value, lock_mechanism_target_state_characteristic.value.new.value, lock_mechanism_target_state_characteristic.value.new.value_length );
                updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
                data_length = updated_characteristic->value.new.value_length + 1;
                offset = HOMEKIT_APP_DCT_OFFSET(dct_lock_mechnism_target_value);
            }
            else if ( fan_on_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
            {
                WPRINT_APP_INFO( ("\r\n FAN_SERVICE \r\n") );

                updated_characteristic = &fan_on_characteristic;

                if( ( fan_on_characteristic.value.new.value[0] == '1' ) ||
                    ( fan_on_characteristic.value.new.value[0] == 't' ) )
                {
                    WPRINT_APP_INFO( ("\r\n\t Received fan ON request \r\n") );
                }
                else
                {
                    WPRINT_APP_INFO( ("\r\n\t Received fan OFF request \r\n") );
                }
                memcpy( updated_value, fan_on_characteristic.value.new.value, fan_on_characteristic.value.new.value_length );
                updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
                data_length = updated_characteristic->value.new.value_length + 1;
                offset = HOMEKIT_APP_DCT_OFFSET(dct_fan_on_value);
           }
           else if ( target_heating_cooling_mode_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
           {
               WPRINT_APP_INFO( ("\r\n THERMOSTAT SERVICE \r\n") );

               updated_characteristic = &target_heating_cooling_mode_characteristic;

               switch( target_heating_cooling_mode_characteristic.value.new.value[0] )
               {
                   case HEATING_COOLING_TARGET_STATE_OFF:
                       WPRINT_APP_INFO( ("\r\n\t Received target heating OFF request \r\n") );
                       break;
                   case HEATING_COOLING_TARGET_STATE_HEAT:
                       WPRINT_APP_INFO( ("\r\n\t Received target heating HEAT request \r\n") );
                       current_mode = &current_heating_cooling_mode_characteristic;
                       memcpy( current_mode->value.new.value, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                       current_mode->value.new.value[updated_characteristic->value.new.value_length] = '\0';
                       current_mode->value.new.value_length = updated_characteristic->value.new.value_length;
                       break;
                   case HEATING_COOLING_TARGET_STATE_COOL:
                       WPRINT_APP_INFO( ("\r\n\t Received target heating COOL request \r\n") );
                       current_mode = &current_heating_cooling_mode_characteristic;
                       memcpy( current_mode->value.new.value, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length );
                       current_mode->value.new.value[updated_characteristic->value.new.value_length] = '\0';
                       current_mode->value.new.value_length = updated_characteristic->value.new.value_length;
                       break;
                   case HEATING_COOLING_TARGET_STATE_AUTO:
                       WPRINT_APP_INFO( ("\r\n\t Received target heating AUTO request \r\n") );
                       break;
                   default:
                       break;
               }
               memcpy( updated_value, target_heating_cooling_mode_characteristic.value.new.value, target_heating_cooling_mode_characteristic.value.new.value_length );
               updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
               data_length = updated_characteristic->value.new.value_length + 1;
               offset = HOMEKIT_APP_DCT_OFFSET(dct_heating_cooling_target_value);
           }
           else if ( temperature_target_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
           {
               WPRINT_APP_INFO( ("\r\n THERMOSTAT SERVICE \r\n") );

               updated_characteristic = &temperature_target_characteristic;

               memcpy( updated_value, temperature_target_characteristic.value.new.value, temperature_target_characteristic.value.new.value_length );

               updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */

               WPRINT_APP_INFO( ("\r\n\t Received target temperature as: %s \r\n", updated_value ) );

               data_length = updated_characteristic->value.new.value_length + 1;

               offset = HOMEKIT_APP_DCT_OFFSET(dct_heating_cooling_target_temperature_value);
           }
           else if ( temperature_units_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
           {
               WPRINT_APP_INFO( ("\r\n THERMOSTAT SERVICE \r\n") );

               updated_characteristic = &temperature_units_characteristic;

               if( temperature_units_characteristic.value.new.value[0] == '0')
               {
                   WPRINT_APP_INFO( ("\r\n\t Received temperature unit as CELSIUS \r\n") );
               }
               else
               {
                   WPRINT_APP_INFO( ("\r\n\t Received temperature unit as FAHRENHEIT \r\n") );
               }
               memcpy( updated_value, temperature_units_characteristic.value.new.value, temperature_units_characteristic.value.new.value_length );
               updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
               data_length = updated_characteristic->value.new.value_length + 1;
               offset = HOMEKIT_APP_DCT_OFFSET(dct_heating_cooling_units_value);
           }
           else if ( outlet_on_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
           {
               WPRINT_APP_INFO( ("\r\n OUTLET SERVICE \r\n") );

               updated_characteristic = &outlet_on_characteristic;

               if( ( outlet_on_characteristic.value.new.value[0] == '1' ) ||
                   ( outlet_on_characteristic.value.new.value[0] == 't' ) )
               {
                   WPRINT_APP_INFO( ("\r\n\t Received ON request \r\n") );
               }
               else
               {
                   WPRINT_APP_INFO( ("\r\n\t Received OFF request \r\n") );
               }
               memcpy( updated_value, outlet_on_characteristic.value.new.value, outlet_on_characteristic.value.new.value_length );
               updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
               data_length = updated_characteristic->value.new.value_length + 1;
               offset = HOMEKIT_APP_DCT_OFFSET(dct_outlet_on_value);
           }
           else if ( switch_on_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
           {
               WPRINT_APP_INFO( ("\r\n SWITCH SERVICE \r\n") );

               updated_characteristic = &switch_on_characteristic;

               if( ( switch_on_characteristic.value.new.value[0] == '1' ) ||
                   ( switch_on_characteristic.value.new.value[0] == 't' ) )
               {
                   WPRINT_APP_INFO( ("\r\n\t Received ON request \r\n") );
               }
               else
               {
                   WPRINT_APP_INFO( ("\r\n\t Received OFF request \r\n") );
               }
               memcpy( updated_value, switch_on_characteristic.value.new.value, switch_on_characteristic.value.new.value_length );
               updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
               data_length = updated_characteristic->value.new.value_length + 1;
               offset = HOMEKIT_APP_DCT_OFFSET(dct_switch_on_value);
           }
           else if ( lightbulb_on_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
           {
               WPRINT_APP_INFO( ("\r\n LIGHTBULB SERVICE \r\n") );

               if( ( lightbulb_on_characteristic.value.new.value[0] == '1' ) ||
                   ( lightbulb_on_characteristic.value.new.value[0] == 't' ) )
               {
                   WPRINT_APP_INFO( ("\r\n\t Received light ON request \r\n") );
                   wiced_gpio_output_high( WICED_LED1 );
               }
               else
               {
                   WPRINT_APP_INFO( ("\r\n\t Received light OFF request \r\n" ) );
                   wiced_gpio_output_low( WICED_LED1 );
               }
               updated_characteristic = &lightbulb_on_characteristic;
               memcpy( updated_value, lightbulb_on_characteristic.value.new.value, lightbulb_on_characteristic.value.new.value_length );
               updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
               data_length = updated_characteristic->value.new.value_length + 1;
               offset = HOMEKIT_APP_DCT_OFFSET(dct_lightbulb_on_value);
           }
           else if ( lock_management_control_point_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
           {
               unsigned char tlv8_frame[sizeof(updated_value) + 3];
               int  len, i;
               lock_control_point_type_t type;

               WPRINT_APP_INFO( ("\r\n LOCK MANAGEMENT SERVICE \r\n") );
               len = MIN( sizeof(updated_value) - 1, lock_management_control_point_characteristic.value.new.value_length );
               WPRINT_APP_INFO(("\r\n\t Received tlv8 record before base64 decoding : [%.*s] \r\n", len, lock_management_control_point_characteristic.value.new.value ));

               updated_characteristic = &lock_management_control_point_characteristic;
               memcpy( updated_value, lock_management_control_point_characteristic.value.new.value, len );
               updated_value[len] = '\0'; /* null terminate string */
               data_length = len + 1;
               offset = HOMEKIT_APP_DCT_OFFSET(dct_lock_management_command_value);

               len = base64_decode( (const unsigned char*)lock_management_control_point_characteristic.value.new.value, len, (unsigned char*)tlv8_frame, sizeof(tlv8_frame), BASE64_STANDARD );
               if (len < 0)
               {
                   WPRINT_APP_INFO( ("Invalid TLV8 value received.\n" ) );
                   return WICED_ERROR;
               }

               WPRINT_APP_INFO(("\r\n\t Received tlv8 record after base64 decoding  : ["));

               for( i = 0; i < len; i++)
               {
                   WPRINT_APP_INFO(("%X", tlv8_frame[i] ));
               }
               WPRINT_APP_INFO(("]\r\n"));

               type = tlv8_frame[0];

               switch( type )
               {
                   case READ_LOGS_FROM_TIME:
                       WPRINT_APP_INFO(("\r\n\t Received readLogsFromTime type TLV8 data.\r\n"));
                       break;

                   case CLEAR_LOGS:
                       WPRINT_APP_INFO(("\r\n\t Received clearLogs type TLV8 data.\r\n"));
                       break;

                   case SET_CURRENT_TIME:
                       WPRINT_APP_INFO(("\r\n\t Received setCurrentTime type TLV8 data.\r\n"));
                       break;

                   default:
                       WPRINT_APP_INFO(("\r\n\t Received Manual type [0x%X] TLV8 data.\r\n", tlv8_frame[0]));
                       break;
               }

               WPRINT_APP_INFO(("\r\n\t TLV8 Data Length : [%d]\r\n", tlv8_frame[1]));
               WPRINT_APP_INFO(("\r\n\t TLV8 Data        : ["));
               for( i = 2; i < len; i++)
               {
                   WPRINT_APP_INFO(("%X", tlv8_frame[i] ));
               }
               WPRINT_APP_INFO(("]\r\n"));

           }
           else if ( lock_mechanism_target_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
           {
               WPRINT_APP_INFO( ("\r\n LOCK MECHANISM SERVICE \r\n") );

               updated_characteristic = &lock_mechanism_target_characteristic;

               switch( *lock_mechanism_target_characteristic.value.new.value )
               {
                   case LOCK_MECHANISM_TARGET_STATE_UNSECURED_LOCK:
                       WPRINT_APP_INFO( ("\r\n\t Received lock mechanism UNSECURED request \r\n") );
                       break;
                   case LOCK_MECHANISM_TARGET_STATE_SECURED_LOCK:
                       WPRINT_APP_INFO( ("\r\n\t Received mechanism SECURED request \r\n") );
                       break;
                   default:
                       WPRINT_APP_INFO( ("\r\n\t Received mechanism RESERVED LOCK request \r\n") );
                       break;
               }
               memcpy( updated_value, lock_mechanism_target_characteristic.value.new.value, lock_mechanism_target_characteristic.value.new.value_length );
               updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
               data_length = updated_characteristic->value.new.value_length + 1;
               offset = HOMEKIT_APP_DCT_OFFSET(dct_lock_mechnism_target_state_value);
           }
#ifdef IOS9_ENABLED_SERVICES
           else if ( security_system_target_state_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
           {
               WPRINT_APP_INFO( ("\r\n SECURITY SYSTEM SERVICE \r\n") );

               updated_characteristic = &security_system_target_state_characteristic;

              switch( security_system_target_state_characteristic.value.new.value[0] )
              {
                  case SECURITY_SYSTEM_TARGET_STATE_STAY_ARM:
                      WPRINT_APP_INFO( ("\r\n\t Received target Security STAY ARM request \r\n") );
                      break;
                  case SECURITY_SYSTEM_TARGET_STATE_AWAY_ARM:
                      WPRINT_APP_INFO( ("\r\n\t Received target Security AWAY ARM request \r\n") );
                      break;
                  case SECURITY_SYSTEM_TARGET_STATE_NIGHT_ARM:
                      WPRINT_APP_INFO( ("\r\n\t Received target Security NIGHT ARM request \r\n") );
                      break;
                  case SECURITY_SYSTEM_TARGET_STATE_DISARM:
                      WPRINT_APP_INFO( ("\r\n\t Received target Security DISARM request \r\n") );
                      break;
                  default:
                      break;
              }
              memcpy( updated_value, security_system_target_state_characteristic.value.new.value, security_system_target_state_characteristic.value.new.value_length );
              updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
              data_length = updated_characteristic->value.new.value_length + 1;
              offset = HOMEKIT_APP_DCT_OFFSET(dct_security_system_target_state_value);
          }
         else if ( door_target_position_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
          {
              WPRINT_APP_INFO( ("\r\n DOOR SERVICE \r\n") );

              updated_characteristic = &door_target_position_characteristic;

              memcpy( updated_value, door_target_position_characteristic.value.new.value, door_target_position_characteristic.value.new.value_length );

              updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */

              WPRINT_APP_INFO( ("\r\n\t Received target Door position as: %s \r\n", updated_value) );

              data_length = updated_characteristic->value.new.value_length + 1;

              offset = HOMEKIT_APP_DCT_OFFSET(dct_door_target_position_value);
          }
         else if ( programmable_switch_output_state_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
         {
             WPRINT_APP_INFO( ("\r\n STATEFUL PROGRAMMABLE SWITCH SERVICE \r\n") );

             updated_characteristic = &programmable_switch_output_state_characteristic;

             if( ( programmable_switch_output_state_characteristic.value.new.value[0] == '1' ) ||
                 ( programmable_switch_output_state_characteristic.value.new.value[0] == 't' ) )
             {
                 WPRINT_APP_INFO( ("\r\n\t Received switch output state ON request \r\n") );
             }
             else
             {
                 WPRINT_APP_INFO( ("\r\n\t Received switch output state OFF request \r\n") );
             }
             memcpy( updated_value, programmable_switch_output_state_characteristic.value.new.value, programmable_switch_output_state_characteristic.value.new.value_length );
             updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */
             data_length = updated_characteristic->value.new.value_length + 1;
             offset = HOMEKIT_APP_DCT_OFFSET(dct_programmable_switch_output_state_value);
         }
         else if ( window_target_position_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
          {
              WPRINT_APP_INFO( ("\r\n WINDOW SERVICE \r\n") );

              updated_characteristic = &window_target_position_characteristic;

              memcpy( updated_value, window_target_position_characteristic.value.new.value, window_target_position_characteristic.value.new.value_length );

              updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */

              WPRINT_APP_INFO( ("\r\n\t Received target window position as: %s \r\n", updated_value) );

              data_length = updated_characteristic->value.new.value_length + 1;

              offset = HOMEKIT_APP_DCT_OFFSET(dct_window_target_position_value);
          }
         else if ( target_position_characteristic.instance_id == update_list->characteristic_descriptor[i].characteristic_instance_id )
          {
              WPRINT_APP_INFO( ("\r\n WINDOW COVERING SERVICE \r\n") );

              updated_characteristic = &target_position_characteristic;

              memcpy( updated_value, target_position_characteristic.value.new.value, target_position_characteristic.value.new.value_length );

              updated_value[updated_characteristic->value.new.value_length] = '\0'; /* null terminate string */

              WPRINT_APP_INFO( ("\r\n\t Received target window covering position as: %s \r\n", updated_value) );

              data_length = updated_characteristic->value.new.value_length + 1;

              offset = HOMEKIT_APP_DCT_OFFSET(dct_target_position_value);
          }
#endif
        }
        else
        {
            WPRINT_APP_INFO( ("\r\n\t No actions for accessory: %d\r\n", update_list->characteristic_descriptor[i].accessory_instance_id) );
        }

        if( updated_characteristic != NULL )
        {
            /* If the requested value is different to the current value/state update to new value */
            if( ( memcmp( updated_characteristic->value.new.value, updated_characteristic->value.current.value, updated_characteristic->value.new.value_length ) != COMPARE_MATCH ) ||
                      ( updated_characteristic->value.new.value_length != updated_characteristic->value.current.value_length  ) )
            {
                /**
                 *  Copies the new value requested by controller (value.new) to the current value
                 *  (memory pointed to by value.current)
                 **/
                wiced_homekit_accept_controller_value( updated_characteristic );

                /* Find the accessory index */
                for ( j = 0 ; j < sizeof(accessory_name_list)/sizeof(accessory_name_list_t); j++ )
                {
                    if (update_list->characteristic_descriptor[i].accessory_instance_id == accessory_name_list[j].accessory->instance_id )
                    {
                        /* Add the characteristics to the notification update list */
                        result = wiced_homekit_register_characteristic_value_update( accessory_name_list[j].accessory, updated_characteristic, updated_characteristic->value.new.value, updated_characteristic->value.new.value_length);
                        if( result != WICED_SUCCESS )
                        {
                            WPRINT_APP_INFO( (" ### ERROR : Value update failed. Error = [%d]\n", result));
                        }

                        if ( current_mode != NULL )
                        {
                            /**
                             *  Copies the new value requested by controller (value.new) to the current value
                             *  (memory pointed to by value.current)
                             **/
                            wiced_homekit_accept_controller_value( current_mode );

                            /* Add the characteristics to the notification update list */
                            result = wiced_homekit_register_characteristic_value_update( accessory_name_list[j].accessory, current_mode, current_mode->value.new.value, current_mode->value.new.value_length);
                            if( result != WICED_SUCCESS )
                            {
                                WPRINT_APP_INFO( (" ### ERROR : Current mode value update failed. Error = [%d]\n", result));
                            }
                            current_mode = NULL;
                        }
                        break;
                    }
                }

                /**
                 * Send send all characteristic values which have been updated and had event notifications enabled to controller
                 * Send the list created by calling wiced_homekit_register_characteristic_value_update
                 **/
                wiced_homekit_send_all_updates_for_accessory( );

                /* Preserve the updated characteristic value to the application persistent storage area */
                wiced_dct_read_lock( (void**)&dct_update_value, WICED_TRUE, DCT_HOMEKIT_APP_SECTION, offset, data_length );

                memcpy ( (void*)dct_update_value, (const void*)updated_value, data_length );
                wiced_dct_write( (const void *)dct_update_value, DCT_HOMEKIT_APP_SECTION, offset, data_length  );

                wiced_dct_read_unlock( (void*)dct_update_value, WICED_TRUE );
            }
            else
            {
                WPRINT_APP_INFO( ("\r\n\t Current value already at requested value\r\n") );
            }
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_homekit_display_password( uint8_t* srp_pairing_password )
{
    WPRINT_APP_INFO( ("\r\n") );
    WPRINT_APP_INFO( ("+------------------------------+\r\n") );
    WPRINT_APP_INFO( ("+ Pairing Password: %s +\r\n", (char*)srp_pairing_password ) );
    WPRINT_APP_INFO( ("+------------------------------+\r\n") );
    WPRINT_APP_INFO( ("\r\n") );

    return WICED_SUCCESS;
}

/*
 * NOTE:
 *  Accessory id = 0 is used URL identify
 */
wiced_result_t identify_action( uint16_t accessory_id, uint16_t characteristic_id )
{
    int i;

    /* Check if this is the garage accessory */
    if( certification_services_accessory.instance_id == accessory_id )
    {
        WPRINT_APP_INFO( ("\r\n%s\r\n", accessory_name) );

        /* Flash the LED representing the garage */
        for ( i = 0; i < 5; i++ )
        {
            wiced_gpio_output_low( WICED_LED2 );

            wiced_rtos_delay_milliseconds( 200 );

            wiced_gpio_output_high( WICED_LED2 );

            wiced_rtos_delay_milliseconds( 200 );
        }

        wiced_gpio_output_low( WICED_LED2 );
    }
    else if ( accessory_id == 0)
    {
        WPRINT_APP_INFO( ("\r\n URL IDENTIFY \r\n") );

        /* Flash the LED representing the URL identify */
        for ( i = 0; i < 3; i++ )
        {
            wiced_gpio_output_low( WICED_LED2 );

            wiced_rtos_delay_milliseconds( 300 );

            wiced_gpio_output_high( WICED_LED2 );

            wiced_rtos_delay_milliseconds( 300 );
        }
        wiced_gpio_output_low( WICED_LED2 );
    }
    return WICED_SUCCESS;
}

static int list_accessory_modes_command( int argc, char *argv[] )
{
    int i;

    WPRINT_APP_INFO( ("\tAccessory Mode ID     Accessory Mode\r\n" ) );
    WPRINT_APP_INFO( ("\t-----------------     --------------\r\n") );

    for (i=0 ; i < sizeof(accessory_mode_list)/sizeof(accessory_mode_info_t) ; i++ )
    {
        WPRINT_APP_INFO( ("\t      [%-2d]            %s\r\n", accessory_mode_list[i].mode, accessory_mode_list[i].name) );
    }
    return 0;
}

static int configure_accessory_mode_command( int argc, char *argv[] )
{
    uint8_t             accessory_mode;
    uint8_t*            pval;
    wiced_result_t      result;

    if( argc != 2 )
    {
        WPRINT_APP_INFO( (" Number of arguments must be 1. This represents the accessory Mode ID\r\n") );
        return 0;
    }

    accessory_mode      = atoi(argv[1]);

    /* Validate accessory mode */
    if ( ( accessory_mode < 0 ) || ( accessory_mode >= ( sizeof(accessory_mode_list) / sizeof(accessory_mode_list[0]) ) ) )
    {
        WPRINT_APP_INFO( ("Invalid Mode. Try with a valid accessory mode (0 to %d) again\n", ( sizeof(accessory_mode_list) / sizeof(accessory_mode_list[0]) ) - 1 ) );
        return 0;
    }
    /* Store the mode into persistant storage */
    wiced_dct_read_lock( (void**)&pval, WICED_TRUE, DCT_HOMEKIT_APP_SECTION, 0, sizeof( accessory_mode ) );

    *pval = accessory_mode;
    wiced_dct_write( (const void *)pval, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( accessory_mode )  );

    result = wiced_dct_read_unlock( (void*)pval, WICED_TRUE );
    if ( result != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    clear_pairings_command(1, NULL);
    WPRINT_APP_INFO( ("Accessory Mode is set to [%d]. Reboot accessory for 'Accessory Mode' changes to take effect\n", accessory_mode) );
    return 0;
}

static int list_accessories_command( int argc, char *argv[] )
{
    int i;

    WPRINT_APP_INFO( ("\tAccessory ID          Accessory Name\r\n" ) );
    WPRINT_APP_INFO( ("\t------------          --------------\r\n") );

    for ( i = 0; i < sizeof(accessory_name_list)/sizeof(accessory_name_list_t) ; i++ )
    {
        WPRINT_APP_INFO( ("\t     [%d]              %s\r\n",accessory_name_list[i].accessory->instance_id, accessory_name_list[i].name) );
    }
    return 0;
}

static int list_services_command( int argc, char *argv[] )
{
    int i;
    int j;
    int accessory_id;
    wiced_bool_t accessory_found = WICED_FALSE;

    if( argc != 2 )
    {
        WPRINT_APP_INFO( (" Number of arguments must be 1. This represents the accessory ID\r\n") );
        return 0;
    }
    accessory_id      = atoi(argv[1]);

    WPRINT_APP_INFO( ("\tAccessory ID          Service ID          Service Name\r\n" ) );
    WPRINT_APP_INFO( ("\t------------          ----------          ------------\r\n") );

    for (j = 0; j < sizeof(accessory_name_list)/sizeof(accessory_name_list_t) ; j++ )
    {
        if ( accessory_id == accessory_name_list[j].accessory->instance_id )
        {
            for (i = 0 ; i < accessory_name_list[j].service_list_size ; i++ )
            {
                if( accessory_name_list[j].service_list[i].service->instance_id != 0 )
                {
                    WPRINT_APP_INFO( ("\t     [%d]                 [%-2d]             %s\r\n", accessory_name_list[j].accessory->instance_id, accessory_name_list[j].service_list[i].service->instance_id, accessory_name_list[j].service_list[i].name) );
                }
            }
            accessory_found = WICED_TRUE;
        }
    }
    if ( accessory_found == WICED_FALSE )
    {
        WPRINT_APP_INFO( (" %d is an invalid accessory ID\r\n", accessory_id) );
    }
    return 0;
}

static int list_characteritics_command( int argc, char *argv[] )
{
    int i;
    int j;
    int k;
    int accessory_id;
    int service_id;
    wiced_bool_t accessory_found = WICED_FALSE;
    wiced_bool_t service_found = WICED_FALSE;

    if( argc != 3 )
    {
        WPRINT_APP_INFO( (" Number of arguments must be 2. This represents the accessory ID and service ID\r\n") );
        return 0;
    }
    accessory_id      = atoi(argv[1]);
    service_id        = atoi(argv[2]);

    WPRINT_APP_INFO( ("\tAccessory ID          Characteristics ID          Service Name\r\n" ) );
    WPRINT_APP_INFO( ("\t------------          ------------------          ------------\r\n") );

    for (k = 0; k < sizeof(accessory_name_list)/sizeof(accessory_name_list_t) ; k++ )
    {
        if ( accessory_id == accessory_name_list[k].accessory->instance_id )
        {
            accessory_found = WICED_TRUE;
            for (i=0 ; i < accessory_name_list[k].service_list_size ; i++ )
            {
                if( service_id == accessory_name_list[k].service_list[i].service->instance_id )
                {
                    service_found = WICED_TRUE;
                    /*Cycle through service array for bridge, find and print */
                    for( j=0 ; j < accessory_name_list[k].service_list[i].characteristic_list_size ; j++ )
                    {
                        if(  accessory_name_list[k].service_list[i].characteristic_list[j].characteristic->instance_id != 0 )
                        {
                            WPRINT_APP_INFO( ("\t     [%d]                     [%-2d]                 %s\r\n",accessory_name_list[k].accessory->instance_id, accessory_name_list[k].service_list[i].characteristic_list[j].characteristic->instance_id, accessory_name_list[k].service_list[i].characteristic_list[j].name) );
                        }
                    }
                }
            }
        }
    }
    if ( accessory_found == WICED_FALSE )
    {
        WPRINT_APP_INFO( (" %d is an invalid accessory ID\r\n", accessory_id) );
    }
    else if( service_found == WICED_FALSE )
    {
        WPRINT_APP_INFO( (" %d is an invalid service ID\r\n", service_id) );
    }
    return 0;
}

static int read_characteristic_command( int argc, char *argv[] )
{
    int accessory_id;
    int characteristic_id;
    wiced_homekit_characteristics_t* characteristic;
    char read_value[MAX_NAME_LENGTH];

    if( argc != 3 )
    {
        WPRINT_APP_INFO( (" Number of arguments must be 2. This represents the accessory ID and characteristics ID\r\n") );
        return 0;
    }

    accessory_id      = atoi(argv[1]);
    characteristic_id = atoi(argv[2]);

    if ( wiced_homekit_find_characteristic_with_instance_id( accessory_id, characteristic_id, &characteristic ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Can not find required characteristic\r\n") );
        return -1;
    }

    memcpy( read_value, characteristic->value.current.value, characteristic->value.current.value_length );
    read_value[characteristic->value.current.value_length]='\0';

    WPRINT_APP_INFO( ("Read value = %s\r\n", read_value ) );
    return 0;
}

static int write_characteristic_command( int argc, char *argv[] )
{
    int j;
    int accessory_id;
    int characteristic_id;
    char new_value[MAX_NAME_LENGTH];
    char* dct_update_value;
    uint8_t  new_value_length;
    uint16_t offset = 0;
    wiced_result_t result;
    wiced_homekit_characteristics_t* characteristic;

    if( argc != 4 )
    {
        WPRINT_APP_INFO( (" Number of arguments must be 3. This represents the accessory ID, characteristic ID and value to write\r\n") );
        return 0;
    }

    accessory_id      = atoi(argv[1]);
    characteristic_id = atoi(argv[2]);
    new_value_length  = strlen(argv[3]);

    if ( wiced_homekit_find_characteristic_with_instance_id( accessory_id, characteristic_id, &characteristic ) != WICED_SUCCESS  )
    {
        WPRINT_APP_INFO( ("Can not find required characteristic\r\n") );
        return 1;
    }

    for (j = 0; j < sizeof(accessory_name_list)/sizeof(accessory_name_list_t) ; j++ )
    {
        if ( accessory_id == accessory_name_list[j].accessory->instance_id )
        {
            /**
             * Update the current value length( value.current.value_length), and place this characteristic value
             * into a list of events to be sent to the controller (if controller enabled events for this characteristic)
             **/

            /* If its a string, keep the quotation marks*/
            if( characteristic->value.current.value[0] =='\"' )
            {
                new_value[0]='\"';
                new_value[new_value_length+1]='\"';
                memcpy( &new_value[1], argv[3], strlen(argv[3]) );
                new_value[new_value_length+2] = '\0';
            }
            else
            {
                memcpy( new_value, argv[3], strlen(argv[3]) );
                new_value[new_value_length] = '\0';
            }

            result = wiced_homekit_register_characteristic_value_update( accessory_name_list[j].accessory, characteristic, new_value, strlen(new_value) );

            if( result == WICED_SUCCESS )
            {
                WPRINT_APP_INFO( ("Characteristic updated with value %s\r\n", new_value ) );

                if( characteristic->instance_id == current_door_state_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_current_door_state_value);
                }
                else if ( characteristic->instance_id == target_door_state_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_target_door_state_value);
                }
                else if ( characteristic->instance_id == lock_mechanism_current_state_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_lock_mechnism_current_value);
                }
                else if ( characteristic->instance_id == lock_mechanism_target_state_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_lock_mechnism_target_value);
                }
                else if ( characteristic->instance_id == current_heating_cooling_mode_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_heating_cooling_current_value);
                }
                else if ( characteristic->instance_id == target_heating_cooling_mode_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_heating_cooling_target_value);
                }
                else if ( characteristic->instance_id == temperature_current_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_heating_cooling_current_temperature_value);
                    if ( memcmp( characteristic->value.current.value, temperature_target_characteristic.value.current.value, characteristic->value.current.value_length ) == COMPARE_MATCH )
                    {
                        memcpy( current_heating_cooling_mode_characteristic.value.new.value, HEATING_COOLING_DEFAULT_STATE_OFF, sizeof(HEATING_COOLING_DEFAULT_STATE_OFF) - 1 );
                        current_heating_cooling_mode_characteristic.value.new.value_length = sizeof(HEATING_COOLING_DEFAULT_STATE_OFF) - 1;
                        wiced_homekit_register_characteristic_value_update( accessory_name_list[j].accessory, &current_heating_cooling_mode_characteristic, (char*)HEATING_COOLING_DEFAULT_STATE_OFF, sizeof(HEATING_COOLING_DEFAULT_STATE_OFF) - 1 );
                    }
                }
                else if ( characteristic->instance_id == temperature_target_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_heating_cooling_target_temperature_value);
                }
                else if ( characteristic->instance_id == temperature_units_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_heating_cooling_units_value);
                }
                else if ( characteristic->instance_id == lightbulb_on_characteristic.instance_id )
                {
                    if( ( new_value[0] == '1' ) ||
                        ( new_value[0] == 't' ) )
                    {
                        wiced_gpio_output_high( WICED_LED1 );
                    }
                    else
                    {
                        wiced_gpio_output_low( WICED_LED1 );
                    }
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_lightbulb_on_value);
                }
                else if ( characteristic->instance_id == fan_on_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_fan_on_value);
                }
                else if ( characteristic->instance_id == outlet_in_use_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_outlet_in_use_value);
                }
                else if ( characteristic->instance_id == switch_on_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_switch_on_value);
                }
                else if ( characteristic->instance_id == lock_management_control_point_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_lock_management_command_value);
                }
                else if ( characteristic->instance_id == lock_mechanism_current_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_lock_mechnism_current_state_value);
                }
                else if ( characteristic->instance_id == lock_mechanism_target_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_lock_mechnism_target_state_value);
                }
#ifdef IOS9_ENABLED_SERVICES
                else if ( characteristic->instance_id == security_system_current_state_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_security_system_current_state_value);
                }
                else if ( characteristic->instance_id == security_system_target_state_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_security_system_target_state_value);
                }
                else if ( characteristic->instance_id == door_current_position_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_door_current_position_value);
                }
                else if ( characteristic->instance_id == door_target_position_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_door_target_position_value);
                }
                else if ( characteristic->instance_id == programmable_switch_output_state_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_programmable_switch_output_state_value);
                }
                else if ( characteristic->instance_id == window_current_position_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_window_current_position_value);
                }
                else if ( characteristic->instance_id == window_target_position_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_window_target_position_value);
                }
                else if ( characteristic->instance_id == window_position_state_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_window_position_state_value);
                }
                else if ( characteristic->instance_id == current_position_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_current_position_value);
                }
                else if ( characteristic->instance_id == target_position_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_target_position_value);
                }
                else if ( characteristic->instance_id == position_state_characteristic.instance_id )
                {
                    offset = HOMEKIT_APP_DCT_OFFSET(dct_position_state_value);
                }
#endif
                if ( offset != 0 )
                {
                    /* Preserve the updated characteristic value to the application persistent storage area */
                    wiced_dct_read_lock( (void**)&dct_update_value, WICED_TRUE, DCT_HOMEKIT_APP_SECTION, offset, new_value_length + 1 );

                    memcpy ( (void*)dct_update_value, (const void*)new_value, new_value_length );
                    dct_update_value[ new_value_length ] = '\0';
                    wiced_dct_write( (const void *)dct_update_value, DCT_HOMEKIT_APP_SECTION, offset, new_value_length + 1 );

                    wiced_dct_read_unlock( (void*)dct_update_value, WICED_TRUE );
                }
            }
            else
            {
                WPRINT_APP_INFO( ("Characteristic value %s rejected. Possible range or format error. \r\n", new_value) );
            }
            /**
             * Send send all characteristic values which have been updated and had event notifications enabled to controller
             * Send the list created by calling wiced_homekit_register_characteristic_value_update
             **/
            wiced_homekit_send_all_updates_for_accessory( );
            break;
        }
    }
    return 0;
}

static int clear_pairings_command ( int argc, char *argv[] )
{
    if ( wiced_homekit_clear_all_pairings() != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to clear pairings!\r\n") );
    }
    else
    {
        WPRINT_APP_INFO( ("Cleared all pairings!\r\n") );
    }
    return 0;
}

static int remove_characteristic_command ( int argc, char *argv[] )
{
    uint32_t                            config_number = 0;
    wiced_result_t                      res = WICED_SUCCESS;
    int                                 i;
    int                                 j;
    int                                 k;
    int                                 accessory_id;
    int                                 characteristic_id;
    wiced_homekit_characteristics_t*    characteristic;

    accessory_id      = atoi(argv[1]);
    characteristic_id = atoi(argv[2]);
    if ( wiced_homekit_find_characteristic_with_instance_id( accessory_id, characteristic_id, &characteristic ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Error: Can not find required characteristic\r\n") );
        return -1;
    }

    for (k = 0; k < sizeof(accessory_name_list)/sizeof(accessory_name_list_t) ; k++ )
    {
        if( accessory_id == accessory_name_list[k].accessory->instance_id )
        {
            for (i=0 ; i < accessory_name_list[k].service_list_size ; i++ )
            {
                for( j=0 ; j < accessory_name_list[k].service_list[i].characteristic_list_size ; j++ )
                {
                    if( characteristic_id == accessory_name_list[k].service_list[i].characteristic_list[j].characteristic->instance_id )
                    {
                        if( wiced_homekit_remove_characteristic( accessory_name_list[k].service_list[i].service, characteristic ) == WICED_SUCCESS )
                        {
                            wiced_homekit_recalculate_accessory_database();
                            res = wiced_homekit_get_configuration_number( &config_number );
                            if (res != WICED_SUCCESS)
                            {
                                WPRINT_APP_DEBUG(("Failed to get config number. Error = [%d]\n", res));
                            }
                            config_number++;
                            res = wiced_homekit_set_configuration_number( config_number );
                            if (res != WICED_SUCCESS)
                            {
                                WPRINT_APP_DEBUG(("Failed to set config number. Error = [%d]\n", res));
                            }
                            WPRINT_APP_INFO( ("Removed characteristic\r\n") );
                        }
                        else
                        {
                            WPRINT_APP_INFO( ("Error: Can not remove required/core characteristic \r\n") );
                        }
                        return 0;
                    }
                }
            }
            break;
        }
    }
    WPRINT_APP_INFO( ("Error: Could not remove characteristic\r\n") );
    return -1;
}

static int return_removed_characteristics_command ( int argc, char *argv[] )
{
    uint32_t        config_number = 0;
    wiced_result_t  res = WICED_SUCCESS;
    int             i;
    int             j;
    int             k;
    int             accessory_id;
    int             characteristic_id;

    accessory_id      = atoi(argv[1]);
    characteristic_id = atoi(argv[2]);

    for (k = 0; k < sizeof(accessory_name_list)/sizeof(accessory_name_list_t) ; k++ )
    {
        if( accessory_id == accessory_name_list[k].accessory->instance_id  )
        {
            for (i=0 ; i < accessory_name_list[k].service_list_size ; i++ )
            {
                for( j=0 ; j < accessory_name_list[k].service_list[i].characteristic_list_size ; j++ )
                {
                    if( characteristic_id == accessory_name_list[k].service_list[i].characteristic_list[j].characteristic->instance_id )
                    {
                        wiced_homekit_add_characteristic( accessory_name_list[k].service_list[i].service, accessory_name_list[k].service_list[i].characteristic_list[j].characteristic );
                        wiced_homekit_recalculate_accessory_database();
                        res = wiced_homekit_get_configuration_number( &config_number );
                        if (res != WICED_SUCCESS)
                        {
                            WPRINT_APP_DEBUG(("Failed to get config number. Error = [%d]\n", res));
                        }
                        config_number++;
                        res = wiced_homekit_set_configuration_number( config_number );
                        if (res != WICED_SUCCESS)
                        {
                            WPRINT_APP_DEBUG(("Failed to set config number. Error = [%d]\n", res));
                        }
                        WPRINT_APP_INFO( ("Returned removed characteristic\r\n") );
                    }
                }
            }
            break;
        }
    }
    return 0;
}

static int remove_service_command ( int argc, char *argv[] )
{
    uint32_t        config_number = 0;
    wiced_result_t  res = WICED_SUCCESS;
    int             i;
    int             j;
    int             k;
    int             accessory_id;
    int             service_id;

    accessory_id = atoi(argv[1]);
    service_id   = atoi(argv[2]);

    for (k = 0; k < sizeof(accessory_name_list)/sizeof(accessory_name_list_t) ; k++ )
    {
        if( accessory_id == accessory_name_list[k].accessory->instance_id )
        {
            for (i=0 ; i < accessory_name_list[k].service_list_size ; i++ )
            {
                for( j=0 ; j < accessory_name_list[k].service_list[i].characteristic_list_size ; j++ )
                {
                    if( service_id == accessory_name_list[k].service_list[i].service->instance_id )
                    {
                        wiced_homekit_remove_service( accessory_name_list[k].accessory, accessory_name_list[k].service_list[i].service );
                        wiced_homekit_recalculate_accessory_database();
                        res = wiced_homekit_get_configuration_number( &config_number );
                        if (res != WICED_SUCCESS)
                        {
                            WPRINT_APP_DEBUG(("Failed to get config number. Error = [%d]\n", res));
                        }
                        config_number++;
                        res = wiced_homekit_set_configuration_number( config_number );
                        if (res != WICED_SUCCESS)
                        {
                            WPRINT_APP_DEBUG(("Failed to set config number. Error = [%d]\n", res));
                        }
                        WPRINT_APP_INFO( ("Removed service\r\n") );
                        return 0;
                    }
                }
            }
            break;
        }
    }
    WPRINT_APP_INFO( ("Error: Could not remove service\r\n") );
    return -1;
}
static int return_removed_services_command ( int argc, char *argv[] )
{
    uint32_t        config_number = 0;
    wiced_result_t  res = WICED_SUCCESS;
    int             i;
    int             k;
    int             accessory_id;
    int             service_id;

    accessory_id = atoi(argv[1]);
    service_id   = atoi(argv[2]);

    for (k = 0; k < sizeof(accessory_name_list)/sizeof(accessory_name_list_t) ; k++ )
    {
        if( accessory_id == accessory_name_list[k].accessory->instance_id )
        {
            for (i=0 ; i < accessory_name_list[k].service_list_size ; i++ )
            {
                if( service_id == accessory_name_list[k].service_list[i].service->instance_id )
                {
                    wiced_homekit_add_service( accessory_name_list[k].accessory, accessory_name_list[k].service_list[i].service );
                    wiced_homekit_recalculate_accessory_database();
                    res = wiced_homekit_get_configuration_number( &config_number );
                    if (res != WICED_SUCCESS)
                    {
                        WPRINT_APP_DEBUG(("Failed to get config number. Error = [%d]\n", res));
                    }
                    config_number++;
                    res = wiced_homekit_set_configuration_number( config_number );
                    if (res != WICED_SUCCESS)
                    {
                        WPRINT_APP_DEBUG(("Failed to set config number. Error = [%d]\n", res));
                    }
                    WPRINT_APP_INFO( ("Returned removed service\r\n") );
                }
            }
            break;
        }
    }
    return 0;
}

#ifdef OTA2_SUPPORT
static int factory_reset_command ( int argc, char *argv[] )
{
    if ( wiced_ota2_force_factory_reset_on_reboot() != WICED_SUCCESS )
        {
            WPRINT_APP_INFO( ("Failed to do factory reset!\r\n") );
        }
        else
        {
            WPRINT_APP_INFO( ("Factory reset successful, Please reboot the device !\r\n") );
        }
    return 0;
}
#endif

static int enable_low_power_mode_command ( int argc, char *argv[] )
{
    if ( wiced_wifi_enable_powersave_with_throughput( 100 ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to Enable Wi-Fi Power Save Mode\n") );
        return -1;
    }
    WPRINT_APP_INFO( ("Enabled Wi-Fi Power Save Mode\n") );

    return 0;
}

static int disable_low_power_mode_command ( int argc, char *argv[] )
{
    if ( wiced_wifi_disable_powersave() != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to Disable Wi-Fi Power Save mode.\n") );
        return -1;
    }

    WPRINT_APP_INFO( ("Disabled Wi-Fi Power Save Mode\n") );
    return 0;
}

static void homekit_get_random_device_id( char device_id_buffer[STRING_MAX_MAC] )
{
    homekit_application_dct_structure_t *app_dct;
    uint8_t local_factory_reset_bit;
    uint16_t random_value;
    int i;
    char* octet_pos;

    wiced_dct_read_lock( (void**) &app_dct, WICED_TRUE, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof( *app_dct ) );

    local_factory_reset_bit = app_dct->is_factoryrest;

    if ( local_factory_reset_bit == 1 )
    {
         /* Get random device id */
         octet_pos = device_id_buffer;
         for (i = 0; i < 6; i++)
         {
             wiced_crypto_get_random( (void*)&random_value, 2 );
             snprintf ( octet_pos, 3, "%02X", (uint8_t)random_value );
             octet_pos += 2;
             *octet_pos++ = ':';
         }

         *--octet_pos = '\0';
         WPRINT_APP_INFO( ("Device ID =[%s]\n",device_id_buffer) );

         app_dct->is_factoryrest = 0;
         memcpy ( app_dct->dct_random_device_id, device_id_buffer, strlen( device_id_buffer) );
         wiced_dct_write( (const void *)app_dct, DCT_HOMEKIT_APP_SECTION, HOMEKIT_APP_DCT_OFFSET(accessory_mode), sizeof(*app_dct) );
    }
    else
    {
        /* Take stored device id from the DCT in case of the rebooting the device */
        memcpy ( device_id_buffer, app_dct->dct_random_device_id, strlen(app_dct->dct_random_device_id) );
        WPRINT_APP_INFO( ("Device ID =[%s]\n",device_id_buffer) );
    }

    wiced_dct_read_unlock( (void*)app_dct, WICED_TRUE );
}
