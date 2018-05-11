/*
    File:    apple_wac.h
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

#pragma once

#include "wiced.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define MAX_STRING_MAC_ADDRESS           18   /* ( 2 characters/byte * 6 bytes ) + ( 5 colon characters ) + NULL = 18 */
/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/** MFi auth chip location
 */
typedef enum
{
    AUTH_CHIP_LOCATION_INTERNAL,    /**< The MFi auth chip is inside the WICED+ package  */
    AUTH_CHIP_LOCATION_EXTERNAL     /**< The MFi auth chip is connected via the external I2C bus */
} mfi_auth_chip_location_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct {

	// Features of the accessory for WAC to identify
    wiced_bool_t    supports_homekit;             // Accessory supports HomeKit v1
    wiced_bool_t    supports_homekit_v2;             // Accessory supports HomeKit v2
	wiced_bool_t	supports_airplay;				// Accessory supports AirPlay
	wiced_bool_t	supports_airprint;				// Accessory supports AirPrint
	wiced_bool_t	supports_5ghz_networks;			// Accessory supports joining 5GHz Wi-Fi networks
	wiced_bool_t	has_app;						// Accessory has an app associated with it

	mfi_auth_chip_location_t auth_chip_location; 	// MFi auth chip location

	char* mdns_desired_hostname;
	char* mdns_nice_name;

    // These value are required by WAC to identify the accessory to the Apple device.
    // See "MFi Accessory Firmware Specification," Section 2.3, for more details.

    char * 	firmware_revision;			// Version of the accessory's firmware, e.g. 1.0.0
    char * 	hardware_revision;			// Version of the accessory's hardware, e.g. 1.0.0
    char * 	serial_number;				// Accessory's serial number, will use MAC address if NULL

    char *  	name;                   // Accessory name, e.g. Thermostat
    char *  	model;               	// Accessory model number, e.g. A1234
    char *  	manufacturer;           // Accessory manufacturer, e.g. Apple
    uint8_t *	oui;					// Accessory OUI, e.g. "\xAA\xBB\xCC"

    // Optional parameters used for app matching. If any protocols are specified,
    // a bundle seed ID must also be specified. See "MFi Accessory Firmware Specification,"
    // Section 1.7, for more details.

    char **	mfi_protocols;
    uint8_t	num_mfi_protocols;
    char *  bundle_seed_id;             // Accessory manufacturer's BundleSeedID

    // Optional channel number for the Soft AP
    // Please note that this *should* be a 2.4GHz channel number
    // Use 0 to let WAC auto-select a 2.4GHz channel
    uint8_t soft_ap_channel;

    // Output parameters; must be freed after usage

    char *  out_new_name;               // Accessory may get a new name
    char *  out_play_password;          // Accessory may be assigned an Airplay playback password

    char    device_random_id[MAX_STRING_MAC_ADDRESS];
} apple_wac_info_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/*****************************************************************************/
/**
 *
 *  @defgroup mfi           Apple MFi Protocols
 *  @ingroup  ipcoms
 *
 *  @addtogroup mfi_wac     WAC
 *  @ingroup mfi
 *
 * This library implements Apple's WAC (Wireless Accessory Configuration) protocol
 *
 *  @{
 */
/*****************************************************************************/

/** Configure the accessory using Apple's Wireless Accessory Configuration (WAC) protocol
 * - Starts Apple's WAC process and connects to WiFi network using the WiFi AP credentials received during the process.
 * - If WAC process is not initiated by configuring device (Ex: iPhone, MacBook, etc), this function times out after 30 minutes and returns error.
 *
 * @param 	apple_wac_info [in] : Pointer to structure that provides info to WAC
 *
 * @return @ref wiced_result_t
 */
wiced_result_t apple_wac_configure( apple_wac_info_t *apple_wac_info );

/** @} */
