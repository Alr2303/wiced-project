/*
    File:    AppleDeviceIE.h
    Package: WACServer
    Version: WAC_POSIX_Server_1.22
    
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

/******************************************************
 *                    Constants
 ******************************************************/

// Flags

#define kAppleDeviceIEFlags_SupportsAirPlay					0x80000000
#define kAppleDeviceIEFlags_DeviceIsUnconfigured			0x40000000
#define kAppleDeviceIEFlags_SupportsMFiConfigurationV1		0x20000000
#define kAppleDeviceIEFlags_SupportsWakeOnWireless			0x10000000
#define kAppleDeviceIEFlags_InterferenceRobustnessEnabled	0x08000000
#define kAppleDeviceIEFlags_DeviceDetectedRemotePPPoE		0x04000000
#define kAppleDeviceIEFlags_WPSCapable						0x02000000
#define kAppleDeviceIEFlags_WPSActive						0x01000000
#define kAppleDeviceIEFlags_SupportsAirPrint				0x00800000
#define kAppleDeviceIEFlags_Supports24GHzNetworks			0x00020000
#define kAppleDeviceIEFlags_Supports5GHzNetworks			0x00010000
#define kAppleDeviceIEFlags_Reserved                        0x00008000
#define kAppleDeviceIEFlags_SupportsHomeKitV1               0x00004000
#define kAppleDeviceIEFlags_SupportsHomeKitV2               0x00002000


// Name, Manufacturer, and Model should obey this length.
// The length is 255 because the length value must fit in a single byte.

#define kAppleDeviceIEMaxStringLength						255

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Add the Apple Device IE to the AP interface
 *
 * 	This function adds the Apple Device IE to the AP interface with the following
 *	parameters:
 *
 * 	@param 	flags 			: Flags about the device, see the kAppleDeviceIEFlags* constants.
 * 	@param 	name 			: Friendly name of the device.
 * 	@param 	manufacturer	: Machine-parsable manufacturer of the device, e.g. "Apple".
 *	@param  model 			: Machine-parsable model of the device, e.g. "Device1,1".
 *  @param  oui 			: OUI of the device including this IE.
 *  @param 	dwds 			: <1:DWDS Role><1:DWDS Flags>, can be NULL if not applicable.
 *  @param  bluetooth_mac 	: MAC address of the Bluetooth radio, can be NULL if not applicable.
 *  @param  device_id 		: Globally unique ID of the device, typically the primary MAC of the device.
 *
 * 	@return WICED_SUCCESS 	: IE was successfully added
 *          WICED_ERROR   	: if an error occurred
 */
wiced_result_t wiced_add_apple_device_ie( const uint32_t flags,
                                          const char *name, 
                                          const char *manufacturer, 
                                          const char *model, 
                                          const uint8_t *oui, 
                                          const uint8_t *dwds, 
                                          const uint8_t *bluetooth_mac, 
                                          const uint8_t *device_id );

