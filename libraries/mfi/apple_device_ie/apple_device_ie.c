/*
    File:    AppleDeviceIE.c
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

#include "apple_device_ie.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

// OUI

#define kAppleOUI                               ((uint8_t*)"\x00\xA0\x40")
#define kAppleOUISubType                        0x00

// Element IDs

#define kSubIEElementIDFlags                    0x00
#define kSubIEElementIDName                     0x01
#define kSubIEElementIDManufacturer             0x02
#define kSubIEElementIDModel                    0x03
#define kSubIEElementIDOUI                      0x04
#define kSubIEElementIDdWDS                     0x05
#define kSubIEElementIDBluetoothMAC             0x06
#define kSubIEElementIDDeviceID                 0x07

// Constant Lengths

#define kElementIDAndLengthFieldsLength 	2
#define kOUIPayloadLength               	3
#define kDWDSPayloadLength              	2
#define kBluetoothMACPayloadLength      	6
#define kDeviceIDPayloadLength          	6

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
 *               Static Function Declarations
 ******************************************************/

wiced_result_t wiced_add_apple_device_ie( const uint32_t flags,
                                          const char *name, 
                                          const char *manufacturer, 
                                          const char *model, 
                                          const uint8_t *oui, 
                                          const uint8_t *dwds, 
                                          const uint8_t *bluetooth_mac,
                                          const uint8_t *device_id )
{
    uint8_t length;
    uint8_t *payload;
    uint8_t *i;
    uint8_t flags_length;
    uint8_t name_length;
    uint8_t manufacturer_length;
    uint8_t model_length;

    // Flags uses two bytes only if bits 8 - 12 are used

    flags_length = (( flags & 0xFF ) != 0 ) ? 4 : (( flags & 0xFF00 ) != 0 ) ? 3 : (( flags & 0xFF0000 ) != 0 ) ? 2 : 1;

    // Get the length of the strings

    name_length = ( name == NULL ) ? 0 : strnlen( name, kAppleDeviceIEMaxStringLength );
    manufacturer_length = ( manufacturer == NULL ) ? 0 : strnlen( manufacturer, kAppleDeviceIEMaxStringLength );
    model_length = ( model == NULL ) ? 0 : strnlen( model, kAppleDeviceIEMaxStringLength );

    // Calculate the length

    length = kElementIDAndLengthFieldsLength + flags_length;

    if( name != NULL ) 			length += kElementIDAndLengthFieldsLength + name_length;
    if( manufacturer != NULL ) 	length += kElementIDAndLengthFieldsLength + manufacturer_length;
    if( model != NULL ) 		length += kElementIDAndLengthFieldsLength + model_length;
    if( oui != NULL ) 			length += kElementIDAndLengthFieldsLength + kOUIPayloadLength;
    if( dwds != NULL )			length += kElementIDAndLengthFieldsLength + kDWDSPayloadLength;
    if( bluetooth_mac != NULL ) length += kElementIDAndLengthFieldsLength + kBluetoothMACPayloadLength;
    if( device_id != NULL ) 	length += kElementIDAndLengthFieldsLength + kDeviceIDPayloadLength;

    // Create the payload

    payload = malloc( length );
    if ( payload == NULL )
    {
        return WICED_OUT_OF_HEAP_SPACE;
    }

    i = payload;

    // Add the data

    // Flags

    *i++ = kSubIEElementIDFlags;
    *i++ = flags_length;
    *i++ = (uint8_t)(flags >> 24);
    if ( flags_length > 1 )
    {
        *i++ = (uint8_t) ( flags >> 16 );
    }
    if ( flags_length > 2 )
    {
        *i++ = (uint8_t) ( flags >> 8 );
    }
    if ( flags_length > 3 )
    {
        *i++ = (uint8_t) ( flags );
    }

    // Name

    if( name != NULL )
    {
        *i++ = kSubIEElementIDName;
        *i++ = name_length;
        memcpy( i, name, name_length );
        i += name_length;
    }

    // Manufacturer

    if( manufacturer != NULL )
    {
        *i++ = kSubIEElementIDManufacturer;
        *i++ = manufacturer_length;
        memcpy( i, manufacturer, manufacturer_length );
        i += manufacturer_length;
    }

    // Model

    if( model != NULL )
    {
        *i++ = kSubIEElementIDModel;
        *i++ = model_length;
        memcpy( i, model, model_length );
        i += model_length;
    }
    
    // OUI

    if( oui != NULL )
    {
        *i++ = kSubIEElementIDOUI;
        *i++ = kOUIPayloadLength;
        memcpy( i, oui, kOUIPayloadLength );
        i += kOUIPayloadLength;
    }

    // dWDS

    if( dwds != NULL )
    {
        *i++ = kSubIEElementIDdWDS;
        *i++ = kDWDSPayloadLength;
        memcpy( i, dwds, kDWDSPayloadLength );
        i += kDWDSPayloadLength;
    }
        

    // Bluetooth MAC

    if( bluetooth_mac != NULL )
    {
        *i++ = kSubIEElementIDBluetoothMAC;
        *i++ = kBluetoothMACPayloadLength;
        memcpy( i, bluetooth_mac, kBluetoothMACPayloadLength );
        i += kBluetoothMACPayloadLength;
    }

    // Device ID

    if( device_id != NULL )
    {
        *i++ = kSubIEElementIDDeviceID;
        *i++ = kDeviceIDPayloadLength;
        memcpy( i, device_id, kDeviceIDPayloadLength );
        i += kDeviceIDPayloadLength;
    }

    // Add Apple IE

    wiced_result_t ie_result = wwd_wifi_manage_custom_ie( WICED_AP_INTERFACE,
                                     	 	 	 	 	 	WICED_ADD_CUSTOM_IE,
                                     	 	 	 	 	 	kAppleOUI,
                                     	 	 	 	 	 	kAppleOUISubType,
                                     	 	 	 	 	 	payload,
                                     	 	 	 	 	 	length,
                                     	 	 	 	 	 	VENDOR_IE_BEACON | VENDOR_IE_PROBE_RESPONSE );
    if( ie_result != WICED_SUCCESS )
    {
    	WPRINT_LIB_INFO(( "%s: Failed to add Apple Device IE\r\n", __func__ ));
    }

    // Free the payload because wwd_wifi_manage_custom_ie() makes its own copy

    if( payload ) free( payload );

    return ie_result;
}


        

