/*
    File:    wac.c
    Package: WACServer
    Version: WAC_Server_Wiced

    Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
    capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
    Apple software is governed by and subject to the terms and conditions of your MFi License,
    including, but not limited to, the restrictions specified in the provision entitled ”Public
    Software”, and is further subject to your agreement to the following additional terms, and your
    agreement that the use, installation, modification or redistribution of this Apple software
    constitutes acceptance of these additional terms. If you do not agree with these additional terms,
    please do not use, install, modify or redistribute this Apple software.

    Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
    you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
    license, under Apple's copyrights in this original Apple software (the "Apple Software"), to use,
    reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
    redistribute the Apple Software, with or without modifications, in binary form. While you may not
    redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
    form, you must retain this notice and the following text and disclaimers in all such redistributions
    of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
    used to endorse or promote products derived from the Apple Software without specific prior written
    permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
    express or implied, are granted by Apple herein, including but not limited to any patent rights that
    may be infringed by your derivative works or by other works in which the Apple Software may be
    incorporated.

    Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
    fixes or enhancements to Apple in connection with this software (“Feedback”), you hereby grant to
    Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
    reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
    distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
    and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
    acknowledge and agree that Apple may exercise the license granted above without the payment of
    royalties or further consideration to Participant.

    The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
    IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
    AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
    IN COMBINATION WITH YOUR PRODUCTS.

    IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
    AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
    (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    Copyright (C) 2013 Apple Inc. All Rights Reserved.
*/

#include <inttypes.h>

#include "wiced.h"
#include "apple_device_ie.h"
#include "MFiSAP.h"
#include "AssertMacros.h"
#include "apple_wac.h"
#include "http_server.h"
#include "tlv.h"
#include "gedday.h"
#include "wwd_events.h"
#include "wiced_utilities.h"
#include "wiced_crypto.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define kWACForWICEDVersion                111

//#define DEBUG
#ifdef DEBUG
    #define wac_log( fmt, ... )                printf( ( "[WAC for WICED v%d] ============== [L%d] : " fmt "\r\n" ), kWACForWICEDVersion, __LINE__, ##__VA_ARGS__ );
#else
    #define wac_log( fmt, ... )
#endif

#define wac_err( fmt, ...)                     printf( ( "[ERROR - WAC for WICED v%d] ======= [L%d] : " fmt "\r\n" ), kWACForWICEDVersion, __LINE__, ##__VA_ARGS__ );

/**
 * These Macros provide abstraction between Apple mDNS Responder (Bonjour) and Broadcom Gedday
 */
#define MFI_CONFIGURE_SERVICE_TYPE                            "_mfi-config._tcp.local"
#define MFI_CONFIGURE_SERVICE_TYPE_DOMAIN

#define mdns_init( interface, desired_hostname, nice_name )   ( gedday_init( interface, desired_hostname ), WICED_SUCCESS )

#define MDNS_DECLARE_SERVICE( name )

#define MDNS_DECLARE_TEXT_RECORD( name )                      static gedday_text_record_t name; \
                                                              static uint8_t              name ## _buffer[128]

#define mdns_make_text_record( txt_record_varname )           gedday_text_record_create( &txt_record_varname, sizeof(txt_record_varname ## _buffer), txt_record_varname ## _buffer )

#define mdns_delete_text_record( txt_record_varname )         gedday_text_record_delete( &txt_record_varname )

#define mdns_text_set_pair( txt_record_varname, key_str, value, value_length )  gedday_text_record_set_key_value_pair( &txt_record_varname, key_str, value )

#define mdns_add_service( instance_name, service_name, domain, port, ttl, txt_record_varname, service_varname )    gedday_add_service( instance_name, service_name, port, ttl, (char*)gedday_text_record_get_string(&txt_record_varname))

#define mdns_update_service( instance_name, service_name )    gedday_update_service( instance_name, service_name )

#define mdns_remove_service( instance_name, service_name )    gedday_remove_service( instance_name, service_name )

#define mdns_stop( )                                          ( gedday_deinit( ), WICED_SUCCESS )

#define require_noerr_string_wiced( ERR, LABEL, STR )   wiced_assert( STR, ERR==0); require_noerr_string( ERR, LABEL, STR )


/******************************************************
 *                    Constants
 ******************************************************/

/*---------------------------------------------------------------------------------------------------------------------------
    @group        MFi Config Service Constants
    @abstract    Constants for service type, TXT record keys, flags, and features.
*/

/* Globally unique ID for the accessory (e.g. the primary MAC address, such as "00:11:22:33:44:55"). */
#define MFI_CONFIGURE_TEXT_RECORD_KEY_DEVICE_ID             "deviceid"

/* Feature flag bits (e.g. "0x3" for bits 0 and 1). */
#define MFI_CONFIGURE_TEXT_RECORD_KEY_FEATURES              "features"

/* Status flags (e.g. 0x04 for bit 3). */
#define MFI_CONFIGURE_TEXT_RECORD_KEY_FLAGS                 "flags"

#define MFI_CONFIGURE_TEXT_RECORD_PROTOCOL_VERSION          "protovers"

/* Configuration seed number. This is 0-255 and updates each time the software configuration changes.
 * This must be saved persistently and remembered across a reboot, power cycle, etc.
 */
#define MFI_CONFIGURE_TEXT_RECORD_KEY_SEED                  "seed"

#define MFI_CONFIGURE_TEXT_RECORD_KEY_SOURCE_VERSION        "srcvers"

#define MFI_CONFIGURE_TEXT_RECORD_PROTOCOL_VERSION_MAJOR    1
#define MFI_CONFIGURE_TEXT_RECORD_PROTOCOL_VERSION_MINOR    0

/* [Status Flag] Problem has been detected. */
#define MFI_CONFIGURE_FLAGS_PROBLEM_DETECTED                0x01

/* [Status Flag] Device is not configured. */
#define MFI_CONFIGURE_FLAGS_UNCONFIGURED                    0x02

/* [Feature] App associated with this accessory. */
#define MFI_CONFIGURE_FEATURES_HAS_APPLICATION              0x00000001

/* [Feature] Accessory supports TLV-based configuration. */
#define MFI_CONFIGURE_FEATURES_SUPPORTS_TLV                 0x00000004

/* Max pass phrase length for WEP security is 32 Byte.
 * WEP Key Format for WICED:
 *    -----------------------------------
 *    | Index | Length |   Pass Phrase  |
 *    -----------------------------------
 *      1-byte  1-byte        n-byte (max 32 bytes)
 * */
#define WEP_PASS_PHRASE_MAX_LENGTH                          (34)

#define MFI_CONFIG_TTL_VALUE                                (GEDDAY_RECORD_TTL)
/*---------------------------------------------------------------------------------------------------------------------------
    @group        WACConstants
    @abstract    Constants for element IDs, keys, etc.
*/

/* [String] Password used to change settings on the accessory (e.g. configure the network, etc.). */
#define WAC_KEY_ADMIN_PASSWORD                      "adminPassword"
#define WAC_TLV_ADMIN_PASSWORD                      0x00

/* [String] Unique 10 character string assigned by Apple to an app via the Provisioning Portal (e.g. "24D4XFAF43"). */
#define WAC_KEY_BUNDLE_SEED_ID                      "bundleSeedID"
#define WAC_TLV_BUNDLE_SEED_ID                      0x01

/* [String] Firmware revision of the accessory. */
#define WAC_KEY_FIRMWARE_REVISION                   "firmwareRevision"
#define WAC_TLV_FIRMWARE_REVISION                   0x02

/* [String] Hardware revision of the accessory. */
#define WAC_KEY_HARDWARE_REVISION                   "hardwareRevision"
#define WAC_TLV_HARDWARE_REVISION                   0x03

/* [String] BCP-47 language to configure the device for.
 * See <http://www.iana.org/assignments/language-subtag-registry> for a full list.
 */
#define WAC_KEY_LANGUAGE                            "language"
#define WAC_TLV_LANGUAGE                            0x04

/* [String] Manufacturer of the accessory (e.g. "Apple"). */
#define WAC_KEY_MANUFACTURER                        "manufacturer"
#define WAC_TLV_MANUFACTURER                        0x05

/* [Array] Array of reverse-DNS strings describing supported MFi accessory protocols (e.g. "com.acme.gadget"). */
#define WAC_KEY_MFI_PROTOCOLS                       "mfiProtocols"
#define WAC_TLV_MFI_PROTOCOLS                       0x06 /* Single MFi protocol string per element. */

/* [String] Model name of the device (e.g. AppleTV1,1). */
#define WAC_KEY_MODEL                               "model"
#define WAC_TLV_MODEL                               0x07

/* [String] Name that accessory should use to advertise itself (e.g. what shows up in iTunes for an AirPlay accessory). */
#define WAC_KEY_NAME                                "name"
#define WAC_TLV_NAME                                0x08

/* [String] Password used to AirPlay to the accessory. */
#define WAC_KEY_PLAY_PASSWORD                       "playPassword"
#define WAC_TLV_PLAY_PASSWORD                       0x09

/* [String] Serial number of the accessory. */
#define WAC_KEY_SERIAL_NUMBER                       "serialNumber"
#define WAC_TLV_SERIAL_NUMBER                       0x0A

/* [Data] WiFi PSK for joining a WPA-protected WiFi network.
 * If it's between 8 and 63 bytes each being 32-126 decimal, inclusive then it's a pre-hashed password.
 * Otherwise, it's expected to be a pre-hashed, 256-bit pre-shared key.
 */
#define WAC_KEY_WIFI_PSK                            "wifiPSK"
#define WAC_TLV_WIFI_PSK                            0x0B

/* [String] WiFi SSID (network name) for the accessory to join. */
#define WAC_KEY_WIFI_SSID                           "wifiSSID"
#define WAC_TLV_WIFI_SSID                           0x0C

#define WAC_TLV_MAX_STRING_SIZE                     255
#define WAC_TLV_TYPE_LENGTH_SIZE                    2

#define MFI_CONFIG_INTERFACE                        WICED_CONFIG_INTERFACE
#define MFI_CONFIG_PORT                             80

#define SOURCE_VERSION                              "1.20"
#define PROTOCOL_VERSION                            "1.0"

/* String lengths */
#define STRING_MAX_SEED                             4    /* max of 255 + NULL = 4 */
#define STRING_MAX_FEATURES                         11   /* 0xXX + NULL = 5 */
#define STRING_MAX_FLAGS                            5    /* 0xXXXXXXXX + NULL = 11 */

#define WIFI_SSID_LENGTH_OCTETS                     32
#define WIFI_PASSPHRASE_LENGTH_OCTETS               64
#define PLAY_PASSWORD_LENGTH_OCTETS                 32

#define WAC_MINUTE_TIMEOUT_IN_SEC                   (60)
#define WAC_MINUTE_TIMEOUT_IN_MSEC                  (WAC_MINUTE_TIMEOUT_IN_SEC * 1000)

#define WAC_TIMEOUT_IN_MINUTES                      (15) /*  As per, "Accessory Interface Spec R26", section 44.7 */
#define WAC_TIMEOUT_IN_SEC                          (WAC_TIMEOUT_IN_MINUTES * WAC_MINUTE_TIMEOUT_IN_SEC)
#define WAC_TIMEOUT_IN_MSEC                         (WAC_TIMEOUT_IN_MINUTES * WAC_MINUTE_TIMEOUT_IN_MSEC)
#define WAC_CONFIGURED_TIMEOUT_IN_SEC               (WAC_MINUTE_TIMEOUT_IN_SEC)
#define MDNS_RESEND_TIMER_RESEND_DEALY_IN_MSEC      1500

#define WAC_SCAN_TIMEOUT                            5000
#define WAC_URL_PROCESSOR_STACK_SIZE                10000
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
 *               Static Function Declarations
 ******************************************************/

void                  wiced_prepare_for_ap_start( void );
extern wiced_result_t wiced_ip_up( wiced_interface_t interface, wiced_network_config_t config, const wiced_ip_setting_t* ip_settings );
extern wiced_result_t wiced_ip_down( wiced_interface_t interface );
static wiced_result_t advertise_mfi_configure_mdns_service( char* friendly_name, uint8_t seed, char* source_version, wiced_bool_t has_app, uint8_t set_flags, char* random_device_id, uint16_t ttl, wiced_semaphore_t* sem, uint32_t req_timeout_in_sec );
static wiced_result_t create_response_tlv( apple_wac_info_t* wac_info_local );
static wiced_result_t parse_configuration_tlv( const void* const tlv_record, const size_t tlv_length, char **parsed_ssid, char** parsed_psk, char** parsed_name, char** parsed_play_password );
static wiced_result_t set_unique_configuration_access_point_ssid( char unique_ssid_output[WIFI_SSID_LENGTH_OCTETS] );
static wiced_result_t write_first_ap_entry_to_dct( const char* ssid, wiced_security_t security_type, const char* passphrase );
static void           generate_wep_pass_phrase( char* pass_phrase, char* formatted_passphrase );

/* HTTP request handlers */

static int32_t process_authentication_setup_request ( const char* url_path,
                               const char* url_parameters,
                               wiced_http_response_stream_t* tcp_stream,
                               void* argument,
                               wiced_http_message_body_t* http_message_body );

static int32_t process_configuration_request    ( const char* url_path,
                               const char* url_parameters,
                               wiced_http_response_stream_t* tcp_stream,
                               void* argument,
                               wiced_http_message_body_t* http_message_body );

static int32_t process_configured_request( const char* url_path,
                               const char* url_parameters,
                               wiced_http_response_stream_t* tcp_stream,
                               void* argument,
                               wiced_http_message_body_t* http_message_body );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static MFiSAPRef            mfi_sap;
static void *               raw_config_data;
static size_t               raw_config_data_length;
static wiced_semaphore_t    config_device_disconnected_event;
static wiced_semaphore_t    config_complete_event;
static uint8_t *            response_tlv;
static uint32_t             response_tlv_length;
apple_wac_info_t *          wac_info;
wiced_http_server_t         persistant_http_server;
static wiced_bool_t         config_received = WICED_FALSE;
static wiced_mac_t          device_id_octet;
//static wiced_timer_t        mdns_resend_timer;

static const wiced_ip_setting_t gSoftAPIPSettings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};

static wiced_config_soft_ap_t gSoftAPConfig =
{
    {0, {0,}},
    WICED_SECURITY_OPEN,
    6,
    0,
    "",
    CONFIG_VALIDITY_VALUE
};

static START_OF_HTTP_PAGE_DATABASE(soft_ap_page_database)
    { "/auth-setup", "application/octet-stream",            WICED_RAW_DYNAMIC_URL_CONTENT , .url_content.dynamic_data   =  {process_authentication_setup_request ,NULL },},
    { "/config",     "application/x-tlv8",                  WICED_RAW_DYNAMIC_URL_CONTENT , .url_content.dynamic_data   =  {process_configuration_request    ,NULL },},
END_OF_HTTP_PAGE_DATABASE();

static START_OF_HTTP_PAGE_DATABASE(sta_page_database)
    { "/configured", "*/*",                                 WICED_RAW_DYNAMIC_URL_CONTENT , .url_content.dynamic_data   =  {process_configured_request,NULL },},
END_OF_HTTP_PAGE_DATABASE();

/******************************************************
 *               Function Definitions
 ******************************************************/

/* This callback is in the WWD thread context. The WWD thread stack must be at least 4 KBytes to allow logging */
static void soft_ap_events_callback( wiced_wifi_softap_event_t event, const wiced_mac_t* mac_address )
{
    wiced_result_t  err;

    wac_log ( "Configuring device (%02X:%02X:%02X:%02X:%02X:%02X) %s",
            mac_address->octet[0],
            mac_address->octet[1],
            mac_address->octet[2],
            mac_address->octet[3],
            mac_address->octet[4],
            mac_address->octet[5],  ( ( event == WICED_AP_STA_JOINED_EVENT ) ? "joined" : ( ( event == WICED_AP_STA_LEAVE_EVENT ) ? "disconnected" : "unknown" ) ) ) ;

if ( ( event == WICED_AP_STA_LEAVE_EVENT ) && ( config_received == WICED_TRUE ) )
    {
        err = wiced_rtos_set_semaphore( &config_device_disconnected_event );
        require_noerr_string_wiced( err, exit, "config device connected semaphore fail" );

        wac_log ( "Configuring device disconnection notified" );
    }

exit:
    return;
}

/*===========================================================================================================================
 *  apple_wac_configure
 *=========================================================================================================================*/
wiced_result_t apple_wac_configure( apple_wac_info_t *wac_info_local )
{
    wiced_config_soft_ap_t* config_ap   = &gSoftAPConfig;
    wiced_result_t          err;
    wiced_bool_t            config_device_disconnected_event_initialised;
    wiced_bool_t            config_complete_event_initialised;
    wiced_bool_t            mfi_platform_initialised;
    char*                   destination_psk;
    char*                   destination_ssid;
    wiced_security_t        destination_security;
    wiced_scan_result_t     ap_info;
    char                    wep_passphrase[WEP_PASS_PHRASE_MAX_LENGTH];
    uint16_t                max_sockets = 3;
    uint8_t                 selected_channel;
    uint8_t                 random_seed_value;
    char*                   advertised_name;
    int                     i;

    mfi_sap = NULL;
    raw_config_data = NULL;
    raw_config_data_length = 0;
    response_tlv = NULL;
    response_tlv_length = 0;

    config_received = WICED_FALSE;
    config_device_disconnected_event_initialised = WICED_FALSE;
    config_complete_event_initialised = WICED_FALSE;
    mfi_platform_initialised = WICED_FALSE;
    destination_psk = NULL;
    destination_ssid = NULL;

    require_action( wac_info_local, exit, err = WICED_BADARG );

    wac_info = wac_info_local;

    wac_log( "Starting WAC for WICED" );

    /* Preserve the device ID for further references */
    for (i = 0; i < 6; i++)
    {
        device_id_octet.octet[i] = (uint8_t)strtol( (wac_info_local->device_random_id) + (i * 3), NULL, 16 );
    }

    /* Platform initialization ---------------------------------------------------------------------------------------------- */

    /* Initialize the blocking semaphores */
    err = wiced_rtos_init_semaphore( &config_device_disconnected_event );
    require_noerr_string_wiced( err, exit, "Failed to initialized config received semaphore" );
    config_device_disconnected_event_initialised = WICED_TRUE;

    err = wiced_rtos_init_semaphore( &config_complete_event );
    require_noerr_string_wiced( err, exit, "Failed to initialized config complete semaphore" );
    config_complete_event_initialised = WICED_TRUE;

    /* Create the response TLV */
    if ( wac_info_local->has_app )
    {
        err = create_response_tlv( wac_info_local );
        require_noerr_string_wiced( err, exit, "Failed to initialized the response TLV data" );
    }

    /* Initialize MFi-SAP */
    wiced_bool_t isAuthInternal = wac_info_local->auth_chip_location == AUTH_CHIP_LOCATION_INTERNAL ? WICED_TRUE : WICED_FALSE;
    err = MFiPlatform_Initialize( isAuthInternal );
    require_noerr_string_wiced( err, exit, "Failed to initialize MFi auth chip" );
    mfi_platform_initialised = WICED_TRUE;

    err = MFiSAP_Create( &mfi_sap, kMFiSAPVersion1 );
    require_noerr_string_wiced( err, exit, "Failed to create MFi-SAP" );

    wac_log( "MFi-SAP initialized" );

    /* Ensure SSID based on the MAC address */
    char uniqueSSID[WIFI_SSID_LENGTH_OCTETS];
    err = set_unique_configuration_access_point_ssid( uniqueSSID );
    require_noerr_string_wiced( err, exit, "Failed to create unique SSID for software AP" );

    /* Software AP mode ------------------------------------------------------------------------------------------------------------ */
    /* Start the configuration interface (Software AP) */
    selected_channel = wac_info_local->soft_ap_channel ? wac_info_local->soft_ap_channel : config_ap->channel;
    err = wwd_wifi_ap_init( &config_ap->SSID, config_ap->security, (uint8_t*)config_ap->security_key, strlen( config_ap->security_key ), selected_channel );
    require_noerr_string_wiced( err, exit, "Failed to init software AP" );

    wiced_prepare_for_ap_start();

    err = wwd_wifi_ap_up();
    require_noerr_string_wiced( err, exit, "Failed to start software AP" );

    err = wiced_ip_up( MFI_CONFIG_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &gSoftAPIPSettings );
    require_noerr_string_wiced( err, exit, "Failed to init software AP IP interface" );

    wac_log( "Software AP enabled with SSID \"%s\", channel %u",  (char *)config_ap->SSID.value, selected_channel );

    wiced_wifi_register_softap_event_handler( soft_ap_events_callback );

    /* Start an HTTP server on the advertised port */
    err = wiced_http_server_start( &persistant_http_server, MFI_CONFIG_PORT, max_sockets, soft_ap_page_database, MFI_CONFIG_INTERFACE, WAC_URL_PROCESSOR_STACK_SIZE );
    require_noerr_string_wiced( err, exit, "Failed to start persistent HTTP server on software AP" );

    wac_log( "Persistent HTTP server started on software AP" );

    /* Initialize mDNS */
    err = mdns_init( MFI_CONFIG_INTERFACE, wac_info_local->mdns_desired_hostname, wac_info_local->mdns_nice_name );
    require_noerr_string_wiced( err, exit, "Failed to start mDNS on Software AP" );

    wac_log( "mDNS initialized on Software AP" );

    /* Generating random seed value */
    wiced_crypto_get_random( (void*) &random_seed_value, 1 );

    /* Advertise the mfi-config service with the unconfigured flag set */
    wac_log( "Advertise mDNS service with name [%s]", wac_info_local->name );
    err = advertise_mfi_configure_mdns_service( wac_info_local->name, random_seed_value, SOURCE_VERSION, wac_info_local->has_app, MFI_CONFIGURE_FLAGS_UNCONFIGURED, wac_info_local->device_random_id, MFI_CONFIG_TTL_VALUE, &config_device_disconnected_event, WAC_TIMEOUT_IN_SEC );
    if( err != WICED_SUCCESS )
    {
        wac_log( "Failed to advertise mDNS service on infrastructure network" );
    }
    else
    {
        wac_log( "%s service advertising on software AP", MFI_CONFIGURE_SERVICE_TYPE );
        wac_log( "/config received" );
    }

    /* Switching modes ------------------------------------------------------------------------------------------------------ */

    /* Shutdown the HTTP server */
    wac_log( "Shutdown HTTP server on software AP" );
    if( wiced_http_server_stop( &persistant_http_server ) != WICED_SUCCESS )
    {
        wac_log( "Failed to shutdown persistent HTTP server on software AP" );
    }
    else
    {
        wac_log( "HTTP server shutdown on software AP" );
    }

    /* Shutdown mDNS */
    mdns_remove_service(wac_info_local->name, MFI_CONFIGURE_SERVICE_TYPE);
    if( mdns_stop( ) != WICED_SUCCESS )
    {
        wac_log( "Failed to stop mDNS on Software AP" );
    }
    else
    {
        wac_log( "mDNS shutdown on Software AP" );
    }

    /* Shutdown the configuration interface */
    wwd_wifi_deauth_all_associated_client_stas( WWD_DOT11_RC_UNSPECIFIED, MFI_CONFIG_INTERFACE );
    if( wiced_network_down( MFI_CONFIG_INTERFACE ) != WICED_SUCCESS )
    {
        wac_log( "Failed to bring down MFI config interface" );
    }
    else
    {
        wac_log( "Software AP shutdown" );
    }
    wiced_wifi_unregister_softap_event_handler( );

    if ( err != WICED_SUCCESS)
    {
        goto exit;
    }

    /* Parse the config data */
    destination_security = WICED_SECURITY_UNKNOWN;
    wac_info_local->out_new_name = NULL;
    wac_info_local->out_play_password = NULL;
    err = parse_configuration_tlv( raw_config_data, raw_config_data_length, &destination_ssid, &destination_psk, &(wac_info_local->out_new_name), &(wac_info_local->out_play_password) );

    require_noerr_string_wiced( err, exit, "Failed to parse config TLV data from controller" );
    require_action_string( destination_ssid, exit, err = WICED_BADOPTION, "Failed to get SSID from controller" );

    if ( wac_info_local->out_new_name != NULL )
    {
        advertised_name = wac_info_local->out_new_name;
    }
    else
    {
        advertised_name = wac_info_local->name;
    }

    wiced_network_set_hostname( advertised_name );

    /* Identify the security type for the SSID received in the configuration */
    wac_log( "Identify the security type of the AP" );
    err = wiced_wifi_find_ap( destination_ssid, &ap_info, NULL );
    require_noerr_string_wiced( err, exit, "Failed to find given Access Point." );
    destination_security = ap_info.security;

    /* If security type is WEP, construct the pass phrase according to WICED convention */
    if( (destination_security == WICED_SECURITY_WEP_PSK ) || ( destination_security == WICED_SECURITY_WEP_SHARED ) )
    {
        generate_wep_pass_phrase(destination_psk, wep_passphrase);
        /* Set the first AP entry in the DCT */
        err = write_first_ap_entry_to_dct( destination_ssid, destination_security, wep_passphrase );
        require_noerr_string_wiced( err, exit, "Failed to write infrastructure credentials to DCT" );
    }
    else
    {
        /* Set the first AP entry in the DCT */
        err = write_first_ap_entry_to_dct( destination_ssid, destination_security, destination_psk );
        require_noerr_string_wiced( err, exit, "Failed to write infrastructure credentials to DCT" );
    }

    /* STA mode ------------------------------------------------------------------------------------------------------------- */

    config_received = WICED_FALSE;
    /* Bring up the STA interface */
    wac_log( "Bring up network on STA interface" );
    err = wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );

    /* If accessory not able to connect to AP with the WEP+OPEN mode then try to connect with the WEP+SHARED security type */
    if( (err != WICED_SUCCESS) && (destination_security == WICED_SECURITY_WEP_PSK) )
    {
        destination_security = WICED_SECURITY_WEP_SHARED;

        /* changing security type to WEP+SHARED and writing into the DCT. */
        err = write_first_ap_entry_to_dct( destination_ssid, destination_security, wep_passphrase );
        require_noerr_string_wiced( err, exit, "Failed to write infrastructure credentials to DCT" );

        /* Bring up the STA interface again after changing the security type to WEP+SHARED in DCT  */
        err = wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
        require_noerr_string_wiced( err, exit, "Failed to join infrastructure network in WEP+SHARED security mode." );
    }
    wac_log( "START HTTP server on infrastructure network" );
    /* Start an HTTP server on the advertised port for the configured pages */
    err = wiced_http_server_start( &persistant_http_server, MFI_CONFIG_PORT, max_sockets, sta_page_database, WICED_STA_INTERFACE, WAC_URL_PROCESSOR_STACK_SIZE );
    require_noerr_string_wiced( err, exit, "Failed to start persistent HTTP server on infrastructure network" );
    wac_log( "Persistent HTTP server started on infrastructure network" );

    /* Initialize mDNS */
    wac_log( "Initialize mDNS on infrastructure network" );
    err = mdns_init( WICED_STA_INTERFACE, advertised_name, advertised_name );
    require_noerr_string_wiced( err, exit, "Failed to start mDNS on infrastructure network" );

    wac_log( "mDNS initialized on infrastructure network" );

    /* Advertise the mfi-config service with an incremented seed value and no flags */
    wac_log( "Advertise mDNS service with name [%s]", advertised_name );
    err = advertise_mfi_configure_mdns_service( advertised_name, (random_seed_value + 1), SOURCE_VERSION, wac_info_local->has_app, 0, wac_info_local->device_random_id, MFI_CONFIG_TTL_VALUE, &config_complete_event, WAC_CONFIGURED_TIMEOUT_IN_SEC );
    if( err != WICED_SUCCESS )
    {
        wac_log( "Failed to advertise mDNS service on infrastructure network" );
    }
    else
    {
        wac_log( "%s registered on infrastructure network", MFI_CONFIGURE_SERVICE_TYPE );
        wac_log( "/configured received" );
    }

    /* Shutdown mDNS */
    wac_log( "Remove mDNS server" );
    mdns_remove_service(advertised_name, MFI_CONFIGURE_SERVICE_TYPE);

    wac_log( "Shutdown mDNS" );
    if( mdns_stop( ) != WICED_SUCCESS )
    {
        wac_log( "Failed to stop mDNS on software AP" );
    }

    /* Shutdown HTTP server */
    wac_log( "Shutdown HTTP server" );
    if( wiced_http_server_stop( &persistant_http_server ) != WICED_SUCCESS )
    {
        wac_log( "Failed to stop persistent HTTP server on infrastructure network" );
    }

exit:

    wac_log( "Exit WAC process" );

    if ( raw_config_data != NULL )
    {
        free( raw_config_data );
        raw_config_data = NULL;
        raw_config_data_length = 0;
    }

    if ( response_tlv != NULL)
    {
        free( response_tlv );
        response_tlv = NULL;
        response_tlv_length = 0;
    }

    /* Clean up locally accessible variables */
    if ( destination_ssid != NULL )
    {
        free( destination_ssid );
        destination_ssid = NULL;
    }

    if ( destination_psk != NULL )
    {
        free( destination_psk );
        destination_psk = NULL;
    }

    /* Tear down MFi-SAP */
    if ( mfi_sap != NULL )
    {
        MFiSAP_Delete( mfi_sap );
        mfi_sap = NULL;
    }

    if ( mfi_platform_initialised == WICED_TRUE )
    {
        MFiPlatform_Finalize( );
    }

    /* Tear down semaphores */
    if ( config_device_disconnected_event_initialised == WICED_TRUE )
    {
        wiced_rtos_deinit_semaphore( &config_device_disconnected_event );
    }
    if ( config_complete_event_initialised == WICED_TRUE )
    {
        wiced_rtos_deinit_semaphore( &config_complete_event );
    }

    return err;
}

/*===========================================================================================================================
 *  process_authentication_setup_request
 *
 *  Process the data sent to /auth-setup when in Software AP mode.
 *=========================================================================================================================*/

static int32_t
    process_authentication_setup_request( const char* url_path,
                                          const char* url_parameters,
                                          wiced_http_response_stream_t* tcp_stream,
                                          void* argument,
                                          wiced_http_message_body_t* http_message_body )
{
    wac_log( "Request received for /auth-setup" );

    wiced_result_t err;
    wiced_bool_t   is_done    = WICED_FALSE;
    uint8_t *      out_data   = NULL;
    size_t         out_length = 0;

    /* Setup MFi-SAP with the controller's data */
    err = MFiSAP_Exchange( mfi_sap, http_message_body->data, http_message_body->message_data_length, &out_data, &out_length, &is_done );
    require_noerr_string_wiced( err, exit, "SAP exchange failed" );

    /* Send the MFi-SAP response */
    err =  wiced_http_response_stream_write_header( tcp_stream,  HTTP_200_TYPE, out_length,  HTTP_CACHE_ENABLED,  http_message_body->mime_type );
    require_noerr_string_wiced( err, exit, "SAP response write fail" );

    err = wiced_http_response_stream_write( tcp_stream, out_data, out_length );
    require_noerr_string_wiced( err, exit, "SAP response send fail" );

    err = wiced_http_response_stream_flush( tcp_stream );
    require_noerr_string_wiced( err, exit, "SAP response flush fail" );

    if ( is_done == WICED_TRUE )
    {
        wac_log( "MFi-SAP complete" );
    }

exit:
    if ( out_data != NULL )
    {
        free( out_data );
        out_data = NULL;
    }

    return err;
}

/*===========================================================================================================================
 *  process_configuration_request
 *
 *  Process the data sent to /config when in Software AP mode.
 *=========================================================================================================================*/

static int32_t
    process_configuration_request( const char* url_path,
                                   const char* url_parameters,
                                   wiced_http_response_stream_t* tcp_stream,
                                   void* argument,
                                   wiced_http_message_body_t* http_message_body )
{
    wac_log( "Request received for /config" );
    wiced_packet_mime_type_t inMIMEType      = http_message_body->mime_type;
    const uint8_t *inData                    = (const uint8_t*)http_message_body->data;
    const uint32_t inDataLength              = http_message_body->message_data_length;

    wiced_result_t    err;
    void *            decrypted_config_data;
    void *            encrypted_response_data;

    decrypted_config_data = NULL;
    encrypted_response_data = NULL;

    require_action( inData, exit, err = WICED_BADARG );
    require_action( inDataLength, exit, err = WICED_WLAN_BADLEN );
    require_action( inMIMEType == MIME_TYPE_TLV, exit, err = WICED_UNSUPPORTED );

    /* Decrypt the config data sent from the controller. */

    decrypted_config_data = calloc( inDataLength, sizeof( uint8_t ) );
    require_action( decrypted_config_data, exit, err = WICED_OUT_OF_HEAP_SPACE );

    err = MFiSAP_Decrypt( mfi_sap, inData, inDataLength, decrypted_config_data );
    require_noerr_string_wiced( err, exit, "config req decrypt fail" );

    /* Set the global variables for access from a different thread */

    raw_config_data = decrypted_config_data;
    raw_config_data_length = inDataLength;

    /* Encrypt the response data if there is any */

    if ( response_tlv != NULL )
    {
        encrypted_response_data = calloc( response_tlv_length, sizeof( uint8_t ) );
        require_action( encrypted_response_data, exit, err = WICED_OUT_OF_HEAP_SPACE );

        err = MFiSAP_Encrypt( mfi_sap, response_tlv, response_tlv_length, encrypted_response_data );
        require_noerr_string_wiced( err, exit, "config req encrypt fail" );
    }

    /* Send the response, including the encrypted app information */
    err = wiced_http_response_stream_write_header( tcp_stream,  HTTP_200_TYPE, response_tlv_length,  HTTP_CACHE_ENABLED,  http_message_body->mime_type );
    require_noerr_string_wiced( err, exit, "config req response write fail" );

    if ( encrypted_response_data )
    {
        err = wiced_http_response_stream_write( tcp_stream, encrypted_response_data, response_tlv_length );
        require_noerr_string_wiced( err, exit, "config req response send fail" );
    }

    err = wiced_http_response_stream_flush( tcp_stream );
    require_noerr_string_wiced( err, exit, "config req response flush fail" );

    wac_log( "Encrypted response sent to controller" );

    /* Send a FIN */
    err = wiced_http_response_stream_disconnect( tcp_stream );
    require_noerr_string_wiced( err, exit, "config req response disconnect/FIN fail" );

    wac_log( "Request /config processing was successfully completed !" );
    config_received = WICED_TRUE;

exit:
    /* Clean up */

    if ( ( raw_config_data == NULL ) && ( decrypted_config_data != NULL ) )
    {
        free( decrypted_config_data );
        decrypted_config_data = NULL;
    }

    if ( encrypted_response_data != NULL )
    {
        free( encrypted_response_data );
        encrypted_response_data = NULL;
    }

    return err;
}

/*===========================================================================================================================
 *  process_configured_request
 *
 *  Process the data sent to /configured when in STA mode.
 *=========================================================================================================================*/

static int32_t
   process_configured_request( const char* url_path,
                               const char* url_parameters,
                               wiced_http_response_stream_t* tcp_stream,
                               void* argument,
                               wiced_http_message_body_t* http_message_body )
{
    wac_log( "Request received for /configured" );

    wiced_result_t     err = 0;

    /* Send an empty response */
    wiced_http_response_stream_disable_chunked_transfer( tcp_stream );
    err =  wiced_http_response_stream_write_header( tcp_stream,  HTTP_200_TYPE, 0,  HTTP_CACHE_ENABLED, MIME_TYPE_ALL );
    require_noerr_string_wiced( err, exit, "configured req response write fail" );

    err =  wiced_http_response_stream_flush( tcp_stream );
    require_noerr_string_wiced( err, exit, "configured req response send fail" );

    /* Send a FIN */
    err = wiced_http_response_stream_disconnect( tcp_stream );
    require_noerr_string_wiced( err, exit, "/configured req response disconnect/FIN fail" );

    /* Signal that configuration is complete */
    err = wiced_rtos_set_semaphore( &config_complete_event );
    require_noerr_string_wiced( err, exit, "configured req semaphore fail" );

    wac_log( "Request /configured processing was successfully completed !" );
    config_received = WICED_TRUE;

exit:

    return err;
}

/*===========================================================================================================================
 *  advertise_mfi_configure_mdns_service
 *
 *  Advertises the mfi-config mDNS service
 *=========================================================================================================================*/

static wiced_result_t advertise_mfi_configure_mdns_service( char *friendly_name, uint8_t seed, char* source_version, wiced_bool_t has_app, uint8_t set_flags, char* random_device_id, uint16_t ttl, wiced_semaphore_t* sem, uint32_t req_timeout_in_sec )
{
    wiced_result_t  err;
    int             print_error;
    char            features_string[STRING_MAX_FEATURES];
    char            flags_string[STRING_MAX_FLAGS];
    char            seed_string[STRING_MAX_SEED];
    int             i;
    int             retry_count;

    MDNS_DECLARE_TEXT_RECORD( wac_text_record );

    /* Create the MFi Configuration TXT record */
    err = mdns_make_text_record( wac_text_record );
    require_noerr_string_wiced( err, exit, "Failed to initialize MFi Configuration TXT record" );

    /* Add Device ID TXT record */
    err = mdns_text_set_pair( wac_text_record, MFI_CONFIGURE_TEXT_RECORD_KEY_DEVICE_ID, random_device_id, strnlen( random_device_id, MAX_STRING_MAC_ADDRESS ) );
    require_noerr_string_wiced( err, exit, "Failed to add MAC address to MFi Configuration TXT record" );
    wac_log ( "mDNS : Device ID = [%s]", random_device_id );

    /* This implementation always has the "Supports TLV" feature in the TXT record but
     * we need to read the passed in configuration to determine if it has an app or not
     */
    uint32_t features = MFI_CONFIGURE_FEATURES_SUPPORTS_TLV;
    if( has_app )
    {
        features |= MFI_CONFIGURE_FEATURES_HAS_APPLICATION;
    }

    /* Convert the features into a string and add it to the TXT record */
    print_error = snprintf( features_string, sizeof( features_string ), "0x%" PRIx32, features );
    require_action_string( print_error >= 0, exit, err = WICED_WLAN_TXFAIL, "Failed to convert features to string" );
    err = mdns_text_set_pair( wac_text_record, MFI_CONFIGURE_TEXT_RECORD_KEY_FEATURES, features_string, strnlen( features_string, STRING_MAX_FEATURES ) );
    require_noerr_string_wiced( err, exit, "Failed to add features to MFi Configuration TXT record" );

    /* Convert the flags into a string and add it to the TXT record */
    print_error = snprintf( flags_string, sizeof( flags_string ), "0x%x", set_flags );
    require_action_string( print_error >= 0, exit, err = WICED_WLAN_TXFAIL, "Failed to convert flags to string" );
    err = mdns_text_set_pair( wac_text_record, MFI_CONFIGURE_TEXT_RECORD_KEY_FLAGS, flags_string, strnlen( flags_string, STRING_MAX_FLAGS ) );
    require_noerr_string_wiced( err, exit, "Failed to add flags to MFi Configuration TXT record" );

    /* Add Protocol Version to the TXT record */
    err = mdns_text_set_pair( wac_text_record, MFI_CONFIGURE_TEXT_RECORD_PROTOCOL_VERSION, PROTOCOL_VERSION, strlen( PROTOCOL_VERSION ) );
    require_noerr_string_wiced( err, exit, "Failed to add HomeKit status flags TXT record" );

    /* Convert the seed into a string and add it to the TXT record */
    print_error = snprintf( seed_string, sizeof( seed_string ), "%d", seed );
    require_action_string( print_error >= 0, exit, err = WICED_ERROR, "Failed to convert seed to string" );
    err = mdns_text_set_pair(wac_text_record, MFI_CONFIGURE_TEXT_RECORD_KEY_SEED, seed_string, strnlen( seed_string, STRING_MAX_SEED ) );
    require_noerr_string_wiced( err, exit, "Failed to add seed to MFi Configuration TXT record" );

    /* Add Source Version to the TXT record */
    err = mdns_text_set_pair( wac_text_record, MFI_CONFIGURE_TEXT_RECORD_KEY_SOURCE_VERSION, source_version, strlen( source_version ) );
    require_noerr_string_wiced( err, exit, "Failed to add source version to MFi Configuration TXT record" );

    /* Register the service */
    MDNS_DECLARE_SERVICE( dns_service );
    err = mdns_add_service( friendly_name, MFI_CONFIGURE_SERVICE_TYPE, MFI_CONFIGURE_SERVICE_TYPE_DOMAIN, MFI_CONFIG_PORT, ttl, wac_text_record, dns_service );
    wac_log( "mdns_add_service returned err = [%d]!", err );
    require_noerr_string_wiced( err, exit, "Failed to register the MFi Configuration service" );

    wac_log ( "Advertised MFi config mDNS service" );

    retry_count = (req_timeout_in_sec * 1000) / MDNS_RESEND_TIMER_RESEND_DEALY_IN_MSEC;
    wac_log( "Retry count available = [%d]!", retry_count);

    /* MDNS service re-advertisement mechanism */
    for (i = 0; i < retry_count; i++)
    {
        /* Ensure the configuring device has disconnected before bringing down SoftAP */
        wac_log( "Response wait count = [%d]!", i + 1 );
        err = wiced_rtos_get_semaphore( sem, MDNS_RESEND_TIMER_RESEND_DEALY_IN_MSEC );
        wac_log( "wiced_rtos_get_semaphore returned err = [%d]!", err );
        if (  err == WICED_WWD_TIMEOUT )
        {
            wac_log( "Haven't received response yet. Re-advertise the service again. Count = [%d]!", i + 1 );
            err = mdns_update_service( friendly_name, MFI_CONFIGURE_SERVICE_TYPE );
            require_noerr_string_wiced( err, exit, "Failed to re-advertise the MFi Configuration service" );
            continue;
        }
        else if (  config_received == WICED_FALSE )
        {
            /* Config is not received from the configuring device. Something not right! */
            wac_log( "Haven't received response yet !" );
            err = WICED_ERROR;
            goto exit;
        }
        wac_log( "Accessory received response for the service advertised!" );
        err = WICED_SUCCESS;
        goto exit;
    }
    wac_log( "Accessory in WAC mode for more than 30 minutes. Wait is over!" );
    err = WICED_ERROR;

exit:
    mdns_delete_text_record( wac_text_record );

    return err;
}

/*===========================================================================================================================
 *  write_first_ap_entry_to_dct
 *
 *  Sets the first AP entry in WICED's DCT, which will be the network joined on first boot.
 *=========================================================================================================================*/

static wiced_result_t write_first_ap_entry_to_dct( const char *ssid, wiced_security_t security_type, const char *passphrase )
{
    wiced_result_t              err;
    platform_dct_wifi_config_t* local_wifi_dct;

    require_action( ssid, exit, err = WICED_BADARG );

    /* Read the current DCT data */
    err = wiced_dct_read_lock( (void**)&local_wifi_dct, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
    require_noerr_string_wiced( err, exit, "DCT read fail" );

    /* coverity-33268 [Buffer not null terminated]
                  FALSE-POSITIVE: Coverity tool is assuming that destination should be NULL terminated because use of "strncpy" function.
                  AP's SSID can be 32 Bytes long and by design "SSID.value" stores non-null terminated string. "SSID.value" is always read
                  using "SSID.length". Refer "wiced_network_up" function to check the use of SSID structure.*/

    /* Set the SSID */
    strncpy( (char *)local_wifi_dct->stored_ap_list[0].details.SSID.value, ssid, WIFI_SSID_LENGTH_OCTETS );
    local_wifi_dct->stored_ap_list[0].details.SSID.length = strnlen( ssid, WIFI_SSID_LENGTH_OCTETS );

    /* Set the security type */

    local_wifi_dct->stored_ap_list[0].details.security = security_type;

    /* Set the passphrase */

    if ( ( security_type != WICED_SECURITY_OPEN ) && ( passphrase != NULL ) )
    {
        memcpy(local_wifi_dct->stored_ap_list[0].security_key,passphrase,WIFI_PASSPHRASE_LENGTH_OCTETS );
        if( ( security_type == WICED_SECURITY_WEP_PSK ) || ( security_type == WICED_SECURITY_WEP_SHARED ))
        {
            /* WICED WEP Pass-Phrase format is INDEX + KEY_LENGTH + PASSPHRASE
             * Actual pass phrase length is KEY_LENGTH + 2. */
            local_wifi_dct->stored_ap_list[0].security_key_length = (uint8_t)*(passphrase+1) + 2;
        }
        else
        {
            local_wifi_dct->stored_ap_list[0].security_key_length = strnlen( passphrase, WIFI_PASSPHRASE_LENGTH_OCTETS );
        }
    }
    /* Save updated config details */

    local_wifi_dct->device_configured = WICED_TRUE;

    err = wiced_dct_write( (const void*) local_wifi_dct, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
    wiced_dct_read_unlock( (void*)local_wifi_dct, WICED_TRUE );
    require_noerr_string_wiced( err, exit, "DCT write fail" );

exit:
    return err;
}

/*===========================================================================================================================
 *    create_response_tlv
 *=========================================================================================================================*/

static wiced_result_t create_response_tlv( apple_wac_info_t *wac_info_local )
{
    wiced_result_t     err;
    uint8_t *        mfi_protocol_sizes;
    uint8_t *        tlv_data;

    response_tlv = NULL;
    response_tlv_length = 0;

    mfi_protocol_sizes = NULL;
    tlv_data = NULL;

    require_action( wac_info_local, exit, err = WICED_BADARG );
    require_action( wac_info_local->bundle_seed_id, exit, err = WICED_BADARG );
    require_action( wac_info_local->num_mfi_protocols > 0, exit, err = WICED_BADARG );
    require_action( wac_info_local->mfi_protocols, exit, err = WICED_BADARG );
    require_action( wac_info_local->name, exit, err = WICED_BADARG );
    require_action( wac_info_local->manufacturer, exit, err = WICED_BADARG );
    require_action( wac_info_local->model, exit, err = WICED_BADARG );
    require_action( wac_info_local->firmware_revision, exit, err = WICED_BADARG );
    require_action( wac_info_local->hardware_revision, exit, err = WICED_BADARG );
    require_action( wac_info_local->serial_number, exit, err = WICED_BADARG );

    /* Get the overall length */

    uint32_t overallLength = 0;

    uint8_t nameSize = strnlen( wac_info_local->name, WAC_TLV_MAX_STRING_SIZE );
    overallLength += nameSize + WAC_TLV_TYPE_LENGTH_SIZE;

    uint8_t manufacturerSize = strnlen( wac_info_local->manufacturer, WAC_TLV_MAX_STRING_SIZE );
    overallLength += manufacturerSize + WAC_TLV_TYPE_LENGTH_SIZE;

    uint8_t modelSize = strnlen( wac_info_local->model, WAC_TLV_MAX_STRING_SIZE );
    overallLength += modelSize + WAC_TLV_TYPE_LENGTH_SIZE;

    uint8_t firmwareRevisionSize = strnlen( wac_info_local->firmware_revision, WAC_TLV_MAX_STRING_SIZE );
    overallLength += firmwareRevisionSize + WAC_TLV_TYPE_LENGTH_SIZE;

    uint8_t hardwareRevisionSize = strnlen( wac_info_local->hardware_revision, WAC_TLV_MAX_STRING_SIZE );
    overallLength += hardwareRevisionSize + WAC_TLV_TYPE_LENGTH_SIZE;

    uint8_t serialNumberSize = strnlen( wac_info_local->serial_number, WAC_TLV_MAX_STRING_SIZE );
    overallLength += serialNumberSize + WAC_TLV_TYPE_LENGTH_SIZE;

    mfi_protocol_sizes = calloc( wac_info_local->num_mfi_protocols, sizeof( uint8_t ) );
    require_action( mfi_protocol_sizes, exit, err = WICED_OUT_OF_HEAP_SPACE );

    uint8_t i;
    for( i = 0; i < wac_info_local->num_mfi_protocols; i++ )
    {
        mfi_protocol_sizes[i] = strnlen( wac_info_local->mfi_protocols[i], WAC_TLV_MAX_STRING_SIZE );
        overallLength += mfi_protocol_sizes[i] + WAC_TLV_TYPE_LENGTH_SIZE;
    }

    uint8_t bundleSeedIDSize = strnlen( wac_info_local->bundle_seed_id, WAC_TLV_MAX_STRING_SIZE );
    overallLength += bundleSeedIDSize + WAC_TLV_TYPE_LENGTH_SIZE;

    /* Allocate space for the entire TLV */

    tlv_data = calloc( overallLength, sizeof( uint8_t ) );
    require_action( tlv_data, exit, err = WICED_OUT_OF_HEAP_SPACE );

    uint8_t *tlvPtr = tlv_data;

    /* Accessory Name */

    *tlvPtr++ = WAC_TLV_NAME;
    *tlvPtr++ = nameSize;
    memcpy( tlvPtr, wac_info_local->name, nameSize );
    tlvPtr += nameSize;

    /* Accessory Manufacturer */

    *tlvPtr++ = WAC_TLV_MANUFACTURER;
    *tlvPtr++ = manufacturerSize;
    memcpy( tlvPtr, wac_info_local->manufacturer, manufacturerSize );
    tlvPtr += manufacturerSize;

    /* Accessory Model */

    *tlvPtr++ = WAC_TLV_MODEL;
    *tlvPtr++ = modelSize;
    memcpy( tlvPtr, wac_info_local->model, modelSize );
    tlvPtr += modelSize;

    /* Firmware Revision */

    *tlvPtr++ = WAC_TLV_FIRMWARE_REVISION;
    *tlvPtr++ = firmwareRevisionSize;
    memcpy( tlvPtr, wac_info_local->firmware_revision, firmwareRevisionSize );
    tlvPtr += firmwareRevisionSize;

    /* Hardware Revision */

    *tlvPtr++ = WAC_TLV_HARDWARE_REVISION;
    *tlvPtr++ = hardwareRevisionSize;
    memcpy( tlvPtr, wac_info_local->hardware_revision, hardwareRevisionSize );
    tlvPtr += hardwareRevisionSize;

    /* Serial Number */

    *tlvPtr++ = WAC_TLV_SERIAL_NUMBER;
    *tlvPtr++ = serialNumberSize;
    memcpy( tlvPtr, wac_info_local->serial_number, serialNumberSize );
    tlvPtr += serialNumberSize;

    /* MFi Protocols */

    for( i = 0; i < wac_info_local->num_mfi_protocols; i++ )
    {
        *tlvPtr++ = WAC_TLV_MFI_PROTOCOLS;
        *tlvPtr++ = mfi_protocol_sizes[i];
        memcpy( tlvPtr, wac_info_local->mfi_protocols[i], mfi_protocol_sizes[i] );
        tlvPtr += mfi_protocol_sizes[i];
    }

    /* BundleSeedID */

    *tlvPtr++ = WAC_TLV_BUNDLE_SEED_ID;
    *tlvPtr++ = bundleSeedIDSize;
    memcpy( tlvPtr, wac_info_local->bundle_seed_id, bundleSeedIDSize );
    tlvPtr += bundleSeedIDSize;

    require_action( ( tlvPtr - overallLength ) == tlv_data, exit, err = WICED_WLAN_BADLEN );

    /* Set the global variables */

    response_tlv = tlv_data;
    response_tlv_length = overallLength;
    err = WICED_SUCCESS;

exit:
    if ( ( response_tlv == NULL ) && ( tlv_data != NULL ) )
    {
        free( tlv_data );
        tlv_data = NULL;
    }
    if ( mfi_protocol_sizes )
    {
        free( mfi_protocol_sizes );
        mfi_protocol_sizes = NULL;
    }

    return err;
}

/*===========================================================================================================================
 *    parse_configuration_tlv
 *=========================================================================================================================*/

static wiced_result_t
    parse_configuration_tlv( const void * const tlv_record,
                     const size_t tlv_length,
                     char **parsed_ssid,
                     char **parsed_psk,
                     char **parsed_name,
                     char **parsed_play_password )
{
    wiced_result_t                err;
    char *                        data_storage_workspace;
    tlv8_data_t *                 tlv_storage_workspace;

    data_storage_workspace = NULL;
    tlv_storage_workspace = NULL;

    require_action( tlv_record, exit, err = WICED_BADARG );
    require_action( tlv_length, exit, err = WICED_BADARG );
    require_action( parsed_ssid, exit, err = WICED_BADARG );
    require_action( parsed_psk, exit, err = WICED_BADARG );

    /* Get SSID */

    tlv_storage_workspace = tlv_find_tlv8( (uint8_t *)tlv_record, tlv_length, WAC_TLV_WIFI_SSID );
    require_action( tlv_storage_workspace, exit, err = WICED_NOT_FOUND );
    require_action( tlv_storage_workspace->type == WAC_TLV_WIFI_SSID, exit, err = WICED_BADOPTION );
    require_action( tlv_storage_workspace->length <= WIFI_SSID_LENGTH_OCTETS, exit, err = WICED_WLAN_BADSSIDLEN );
    require_action( tlv_storage_workspace->data, exit, err = WICED_WLAN_BADADDR );

    data_storage_workspace = (char*) calloc( tlv_storage_workspace->length + 1, sizeof( char ) );
    require_action( data_storage_workspace, exit, err = WICED_OUT_OF_HEAP_SPACE );

    memcpy( data_storage_workspace, tlv_storage_workspace->data, tlv_storage_workspace->length );
    *parsed_ssid = data_storage_workspace;
    data_storage_workspace = NULL;
    tlv_storage_workspace = NULL;

    /* Get passphrase */

    tlv_storage_workspace = tlv_find_tlv8( (uint8_t *)tlv_record, tlv_length, WAC_TLV_WIFI_PSK );
    if ( tlv_storage_workspace )
    {
        require_action( tlv_storage_workspace->type == WAC_TLV_WIFI_PSK, exit, err = WICED_BADOPTION );
        require_action( tlv_storage_workspace->length <= WIFI_PASSPHRASE_LENGTH_OCTETS, exit, err = WICED_WLAN_BADLEN );
        require_action( tlv_storage_workspace->data, exit, err = WICED_WLAN_BADADDR );

        data_storage_workspace = (char*) calloc( tlv_storage_workspace->length + 1, sizeof( char ) );
        require_action( data_storage_workspace, exit, err = WICED_OUT_OF_HEAP_SPACE );

        memcpy( data_storage_workspace, tlv_storage_workspace->data, tlv_storage_workspace->length );
        *parsed_psk = data_storage_workspace;
        data_storage_workspace = NULL;
        tlv_storage_workspace = NULL;
    }

    /* Get name */

    if ( parsed_name )
    {
        tlv_storage_workspace = tlv_find_tlv8( (uint8_t *)tlv_record, tlv_length, WAC_TLV_NAME );
        if ( tlv_storage_workspace )
        {
            require_action( tlv_storage_workspace->type == WAC_TLV_NAME, exit, err = WICED_BADOPTION );
            require_action( tlv_storage_workspace->length, exit, err = WICED_WLAN_BADLEN );
            require_action( tlv_storage_workspace->data, exit, err = WICED_WLAN_BADADDR );

            data_storage_workspace = (char*) calloc( tlv_storage_workspace->length + 1, sizeof( char ) );
            require_action( data_storage_workspace, exit, err = WICED_OUT_OF_HEAP_SPACE );

            memcpy( data_storage_workspace, tlv_storage_workspace->data, tlv_storage_workspace->length );
            *parsed_name = data_storage_workspace;
            data_storage_workspace = NULL;
            tlv_storage_workspace = NULL;
        }
    }

    /* Get play password */

    if ( parsed_play_password )
    {
        tlv_storage_workspace = tlv_find_tlv8( (uint8_t *)tlv_record, tlv_length, WAC_TLV_PLAY_PASSWORD );
        if ( tlv_storage_workspace )
        {
            require_action( tlv_storage_workspace->type == WAC_TLV_PLAY_PASSWORD, exit, err = WICED_BADOPTION );
            require_action( tlv_storage_workspace->length <= PLAY_PASSWORD_LENGTH_OCTETS, exit, err = WICED_WLAN_BADLEN );
            require_action( tlv_storage_workspace->data, exit, err = WICED_WLAN_BADADDR );

            data_storage_workspace = (char*) calloc( tlv_storage_workspace->length + 1, sizeof( char ) );
            require_action( data_storage_workspace, exit, err = WICED_OUT_OF_HEAP_SPACE );

            memcpy( data_storage_workspace, tlv_storage_workspace->data, tlv_storage_workspace->length );
            *parsed_play_password = data_storage_workspace;
            data_storage_workspace = NULL;
            tlv_storage_workspace = NULL;
        }
    }

    err = WICED_SUCCESS;

exit:
    return err;
}

/*===========================================================================================================================
 *  set_unique_configuration_access_point_ssid
 *
 *  Creates a unique SSID by appending the last three bytes of the MAC address to "Unconfigured Device"
 *  and then sets the config AP's SSID to the new, unique SSID.
 *=========================================================================================================================*/

static wiced_result_t set_unique_configuration_access_point_ssid( char unique_ssid_output[WIFI_SSID_LENGTH_OCTETS] )
{
    wiced_result_t  err         = WICED_SUCCESS;
    int             print_error;
    require_action( unique_ssid_output, exit, err = WICED_BADARG );

    /* Add the last three bytes of the MAC address to the unique software AP SSID */

    memset( unique_ssid_output, 0, WIFI_SSID_LENGTH_OCTETS );
    print_error = snprintf( unique_ssid_output, WIFI_SSID_LENGTH_OCTETS, "Unconfigured Device %02x%02x%02x", device_id_octet.octet[3], device_id_octet.octet[4], device_id_octet.octet[5] );
    require_action_string( print_error >= 0, exit, err = WICED_WLAN_TXFAIL, "Failed to get MAC address" );

    /* Set the global config AP's SSID to the new, unique SSID */

    gSoftAPConfig.SSID.length = strnlen( unique_ssid_output, WIFI_SSID_LENGTH_OCTETS );
    memcpy( gSoftAPConfig.SSID.value, unique_ssid_output, WIFI_SSID_LENGTH_OCTETS );

exit:
    return err;
}

/*===========================================================================================================================
 *    wiced_prepare_for_ap_start
 *
 *     This function gets called in wiced_wifi_start_ap() before bringing up the software AP interface. We add the Apple Device IE
 *     here because several folks were reporting parsing issues on iOS. This function prototype is in wwd_ap.c and
 *    it looks like this: extern void wiced_prepare_for_ap_start( void ) __attribute__((weak));
 *
 *  In WICED-SDK 2.4.0 the function is called in Wiced/WWD/internal/chips/43362a2/wwd_ap.c:260,
 *  just before the BSS_UP command is sent.
 *=========================================================================================================================*/

void wiced_prepare_for_ap_start( void )
{
    uint32_t         flags;
    wiced_result_t   err;

    /* Default set of flags in the Apple Device IE */

    flags = kAppleDeviceIEFlags_DeviceIsUnconfigured |
            kAppleDeviceIEFlags_SupportsMFiConfigurationV1 |
            kAppleDeviceIEFlags_Supports24GHzNetworks;

    /* Read the passed in configuration to see if more flags need to be added */

    if ( wac_info->supports_airplay )
    {
        flags |= kAppleDeviceIEFlags_SupportsAirPlay;
    }
    if ( wac_info->supports_airprint )
    {
        flags |= kAppleDeviceIEFlags_SupportsAirPrint;
    }
    if ( wac_info->supports_5ghz_networks )
    {
        flags |= kAppleDeviceIEFlags_Supports5GHzNetworks;
    }
    if ( wac_info->supports_homekit )
    {
        flags |= kAppleDeviceIEFlags_SupportsHomeKitV1;
    }

    if ( wac_info->supports_homekit_v2 )
     {
         flags |= kAppleDeviceIEFlags_SupportsHomeKitV2;
     }
    /* Create the IE data and add it to the software AP interface */

    err = wiced_add_apple_device_ie( flags,
                                     wac_info->name,
                                     wac_info->manufacturer,
                                     wac_info->model,
                                     wac_info->oui,
                                     NULL,
                                     NULL,
                                     device_id_octet.octet );
    require_noerr_string_wiced( err, exit, "Failed to add Apple Device IE to software AP interface" );

    wac_log( "Added Apple Device IE to software AP interface" );

exit:
    return;
}

/*===========================================================================================================================
 *  mac_address_to_string
 *=========================================================================================================================*/

#define kHexDigitsUppercase         "0123456789ABCDEF"

static void generate_wep_pass_phrase( char* pass_phrase, char* formatted_passphrase )
{
    wiced_assert("Bad args",pass_phrase!=NULL);
    wiced_assert("Bad args",formatted_passphrase!=NULL);

    wac_log("Pass Phrase = [%s]\n", pass_phrase);

    /* TODO: Need to make compatible for all index, currently assuming it is always 0 index.   */
    /* Write the index */
    *formatted_passphrase = 0;
    formatted_passphrase++;

    /* Write the length of pass-phrase */
    *formatted_passphrase = strlen(pass_phrase);
    formatted_passphrase++;

    /* Store the pass-phrase */
    memcpy(formatted_passphrase, pass_phrase, strlen(pass_phrase));
    formatted_passphrase[strlen(pass_phrase)] = '\0';
}
