/*
	Copyright (C) 2008-2013 Apple Inc. All Rights Reserved.
*/

#ifndef	__IEEE80211Utils_h__
#define	__IEEE80211Utils_h__

#include "CommonServices.h"
#include "DataBufferUtils.h"
#include "DebugServices.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#pragma mark == Structures ==
#endif

// IEEE80211Header

typedef struct
{
	uint8_t		fc[ 2 ];	// [0x00/0] Frame Control.
	uint8_t		dur[ 2 ];	// [0x02/2] Duration.
	uint8_t		addr1[ 6 ];	// [0x04/4] Address 1 (destination address).
	uint8_t		addr2[ 6 ];	// [0x0A/10] Address 2 (source address).
	uint8_t		addr3[ 6 ];	// [0x10/16] Address 3 (BSSID).
	uint8_t		seq[ 2 ];	// [0x16/22] Sequence
							// [18/24] Total
}	IEEE80211Header;

check_compile_time( offsetof( IEEE80211Header, fc )		== 0x00 );
check_compile_time( offsetof( IEEE80211Header, dur )	== 0x02 );
check_compile_time( offsetof( IEEE80211Header, addr1 )	== 0x04 );
check_compile_time( offsetof( IEEE80211Header, addr2 )	== 0x0A );
check_compile_time( offsetof( IEEE80211Header, addr3 )	== 0x10 );
check_compile_time( offsetof( IEEE80211Header, seq )	== 0x16 );
check_compile_time( sizeof( IEEE80211Header )			== 0x18 );

// AppleActionFrameHeader

typedef struct
{
	uint8_t		category;	// [0x00] Category of the frame. 4 is a public action frame.
	uint8_t		action;		// [0x01] Action type for the frame. 9 is vendor-specific.
	uint8_t		oui[ 3 ];	// [0x02] Vendor OUI. Apple is 0x00, 0x17, 0xF2.
	uint8_t		subType;	// [0x05] Apple-specific action frame sub-type.
							// [0x06] Total
	
}	AppleActionFrameHeader;

check_compile_time( offsetof( AppleActionFrameHeader, category )	== 0x00 );
check_compile_time( offsetof( AppleActionFrameHeader, action )		== 0x01 );
check_compile_time( offsetof( AppleActionFrameHeader, oui )			== 0x02 );
check_compile_time( offsetof( AppleActionFrameHeader, subType )		== 0x05 );
check_compile_time( sizeof( AppleActionFrameHeader )				== 0x06 );


#define kIEEE80211_ActionFrameCategory_Public			0x04 // Public action frame. Defined in IEEE 802.11k-2008.
#define kIEEE80211_ActionFrameAction_VendorSpecific		0x09 // Vendor-specific public action frame. Defined in IEEE 802.11w-2008.

#if 0
#pragma mark == Information Element IDs (EIDs) ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		Information Element ID's
	@abstract	See IEEE 802.11-2007 section 7.3.2.
	@discussion	Use IEGetNext(), IEGetVendorSpecific(), DataBuffer_AppendIE(), DataBuffer_AppendVendorIE(), etc.
*/

#define kIEEE80211_EID_SSID			0	//! [0x00] Payload is up to 32 bytes of SSID (hopefully UTF-8, but not always).
	#define kIEEE80211_SSIDMaxLen	32	//! Max number of bytes in an SSID.
#define kIEEE80211_EID_Vendor		221 //! [0xDD] Vendor-specific element ID. Payload starts with 3-byte OUI.

#if 0
#pragma mark == Information Element Vendor IDs (VIDs) ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		Vendor ID's
	@abstract	3-byte vendor-specific OUI and 1-byte type as 32-bit values for vendor-specific IE's.
	@discussion
	
	The IE vendor extension requires the first 3 bytes of the IE payload (i.e. after the eid and length) to be a 3 byte
	OUI for the vendor and any data that follows the OUI is vendor-specific. A convention has been followed by all the 
	vendor-specific IE's I've ever seen where the byte following the OUI is a type field to indicate the type of the data.
	This effectively provides a 32-bit vendor ID. To simplify APIs for adding and finding vendor-specific IE data, this 
	section defines 32-bit vendor ID's (and VID-specific constants when needed) for vendor ID's we know about and use.
	
	Use IEGetWPSAttribute(), IEGetAppleGeneral(), DataBuffer_AppendWPSAttributeIE(), DataBuffer_AppendAppleGeneralIE(), etc.
	
	See <http://standards.ieee.org/regauth/oui/oui.txt> for a complete list of OUIs.
*/

#if 0
#pragma mark == -- Apple Device IE
#endif

//===========================================================================================================================
//	Apple Device IE
//===========================================================================================================================

#define kIEEE80211_VID_AppleDevice		0x00A04000 // 0x00, 0xA0, 0x40, type 0x00. Format: sub IEs.

#define kAppleDeviceIE_Flags			0x00 // [BitArray]	b0-b7, b8-b15, etc. of flags. See kAppleDeviceIEFlagBit_*.
#define kAppleDeviceIE_Name				0x01 // [UTF-8]		Friendly name of the deice.
#define kAppleDeviceIE_Manufacturer		0x02 // [UTF-8]		Machine-parsable manufacturer of the device (e.g. "Apple").
#define kAppleDeviceIE_Model			0x03 // [UTF-8]		Machine-parsable model of the device (e.g. "MacBook2,3").
#define kAppleDeviceIE_OUI				0x04 // [3 bytes]	OUI of the device including this IE.
#define kAppleDeviceIE_DWDS				0x05 // [2 bytes]	<1:DWDS Role><1:DWDS Flags>. See kDWDS_Role_* and kDWDS_Flags_*.
#define kAppleDeviceIE_BluetoothMAC		0x06 // [6 bytes] 	MAC address of the Bluetooth radio, if applicable.
#define kAppleDeviceIE_DeviceID			0x07 // [6 bytes] 	Globally unique ID of the device.

#define kAppleDeviceIEFlagBit_AirPlay						 0 // 0x80:   Supports AirPlay.
#define kAppleDeviceIEFlagBit_Unconfigured					 1 // 0x40:   Device is unconfigured.
#define kAppleDeviceIEFlagBit_MFiConfigurationV1			 2 // 0x20:   Supports MFi Configuration V1.
#define kAppleDeviceIEFlagBit_WakeOnWireless				 3 // 0x10:   Supports Wake on Wireless (WoW).
#define kAppleDeviceIEFlagBit_InterferenceRobustness		 4 // 0x08:   Device has interference robustness enabled.
#define kAppleDeviceIEFlagBit_DetectedPPPoEServer			 5 // 0x04:   Device detected remote PPPoE server.
#define kAppleDeviceIEFlagBit_WPSCapable					 6 // 0x02:   Supports WPS.
#define kAppleDeviceIEFlagBit_WPSActive						 7 // 0x01:   WPS is active on the device.
#define kAppleDeviceIEFlagBit_AirPrint						 8 // 0x0080: Supports AirPrint.
#define kAppleDeviceIEFlagBit_iAPoverWiFi					 9 // 0x0040: Supports iAP over WiFi.
#define kAppleDeviceIEFlagBit_iOSCar						10 // 0x0020: Supports iOS in the Car.
#define kAppleDeviceIEFlagBit_InternetAccess				11 // 0x0010: Provides Internet access (e.g. 3G/4G).
#define kAppleDeviceIEFlagBit_ACPConfigurationV1			12 // 0x0008: Supports ACP Configuration V1.
#define kAppleDeviceIEFlagBit_RecentPhysicalContact			13 // 0x0004: User in physical contact with device recently.
#define kAppleDeviceIEFlagBit_2pt4GHzWiFi					14 // 0x0002: Supports 2.4 GHz WiFi networks.
#define kAppleDeviceIEFlagBit_5GHzWiFi						15 // 0x0001: Supports 5 GHz WiFi networks.

#define kAppleDeviceIEFlagBit_TotalBits						16 // Total number of defined flag bits.

#if 0
#pragma mark == -- Apple General IE
#endif

//===========================================================================================================================
//	Apple General IE
//===========================================================================================================================

// Old-style Apple vendor-specific IE. Because existing code searches for just the first 3 bytes and may do bad things 
// when it's not formatted as an Apple General IE, the 0x000393 (00 03 93) OUI is only used for the Apple General IE.
// New Apple vendor-specific IEs should use 0x0017F2 (00 17 F2) as the OUI and must use the type to differentiate.

#define kIEEE80211_VID_AppleGeneral		0x00039301 // Format: <1:productID> <2:big endian flags> [sub IEs]
	
	#define kAppleGeneralIE_Key_ProductID						"productID"	//! [CFNumber of ACPAppleProductID]
	
	#define kAppleGeneralIE_Key_Flags							"flags"		//! [CFNumber]
	#define kAppleGeneralIE_Flags_InterferenceRobustness		( 1 << 0 )	//! 0x001: Interface robustness is enabled by the user.
	#define kAppleGeneralIE_Flags_Unconfigured					( 1 << 1 )	//! 0x002: Device is not configured.
	#define kAppleGeneralIE_Flags_FoundRemotePPPoEServer		( 1 << 2 )	//! 0x004: Device found a PPPoE server.
	#define kAppleGeneralIE_Flags_GuestNetwork					( 1 << 3 )	//! 0x008: Guest network available.
	#define kAppleGeneralIE_Flags_LegacyWDS						( 1 << 4 )	//! 0x010: Device has legacy WDS links.
	#define kAppleGeneralIE_Flags_WPSCapable					( 1 << 5 )	//! 0x020: Device is WPS capable.
	#define kAppleGeneralIE_Flags_WPSActive						( 1 << 6 )	//! 0x040: WPS is currently active on the device.
	#define kAppleGeneralIE_Flags_SAWCapable					( 1 << 7 )	//! 0x080: Device is SAW capable.
	#define kAppleGeneralIE_Flags_WoWCapable					( 1 << 8 )	//! 0x100: Wake-On-Wireless capable.
	#define kAppleGeneralIE_Flags_HasRestoreProfiles			( 1 << 9 )	//! 0x200: Device has restore profile(s)
	
	// Sub IEs are just normally formatted IEs with Apple-specific EIDs.
	
	#define kAppleGeneralIE_SubEID_ChannelInfo		0x00 // Format: 1 or more of <<6:BSSID> <1:channelNumber>>
	#define kAppleGeneralIE_Key_ChannelInfo			"channelInfo"	//! [CFArray of CFDictionary]
		#define kAppleGeneralIE_ChannelInfoKey_BSSID	"bssid"		//! [CFString] BSSID hosting channel.
		#define kAppleGeneralIE_ChannelInfoKey_Channel	"channel"	//! [CFNumber] Channel being hosted.

// Base VID for all Apple vendor-specific IE's (except the legacy Apple General IE explained above).
// Code must search for the entire 4-byte VID (3-byte OUI + 1-byte type) when searching for Apple IEs.

#define kIEEE80211_VID_AppleBase				0x0017F200

#if 0
#pragma mark == -- Dynamic WDS IE
#endif

//===========================================================================================================================
//	Dynamic WDS IE
//===========================================================================================================================

#define kIEEE80211_VID_DWDS						0x0017F201 //! Format: <1:subtype=0x00> <1:version=0x01> <1:role> <4:flags>
	
	#define kDWDS_Role_Inactive						0x00
	#define kDWDS_Role_Master						0x01
	#define kDWDS_Role_Relay						0x02
	#define kDWDS_Role_Remote						0x03
	
	#define kDWDS_Flags_AMPDU_NotSupported			( 1 << 0 ) //! [0x01] Supports no aggregation.
	#define kDWDS_Flags_AMPDU_HardwareWorkaround	( 1 << 1 ) //! [0x02] Supports 4 address aggregation.
	#define kDWDS_Flags_AMPDU_Full					( 1 << 2 ) //! [0x04] Supports full aggregation.
	#define kDWDS_Flags_ClosedNetwork				( 1 << 3 ) //! [0x08] Hidden SSID.

#if 0
#pragma mark == -- WiFi Direct IE
#endif

//===========================================================================================================================
//	WiFi Direct IE
//===========================================================================================================================

#define kIEEE80211_VID_WiFiDirect		0x0050F209 //! See WiFi P2P 1.0 spec section 4.1.1.

#if 0
#pragma mark == -- WiFi Display (Miracast) IE
#endif

//===========================================================================================================================
//	WiFi Display (Miracast) IE
//===========================================================================================================================

#define kIEEE80211_VID_WiFiDisplay		0x506F9A0A //! See WiFi Display 1.0 spec section 5.1.1.

#if 0
#pragma mark == -- WPS IE
#endif

//===========================================================================================================================
//	WPS IE
//===========================================================================================================================

#define kIEEE80211_VID_WPS		0x0050F204 //! See WPS 1.0 spec section 7.2.

#if 0
#pragma mark == IE Parsing ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IEGet
	@abstract	Gets an IE by its element ID in an IE blob.
*/

OSStatus
	IEGet( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint8_t				inID, 
		const uint8_t **	outPtr, 
		size_t *			outLen, 
		const uint8_t **	outNext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IEGetNext
	@abstract	Gets the next IE in an IE blob.
*/

OSStatus
	IEGetNext( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint8_t *			outID, 
		const uint8_t **	outData, 
		size_t *			outLen, 
		const uint8_t **	outNext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IEGetVendorSpecific
	@abstract	Searches for the specified vendor-specific IE and returns its data if found.
*/

OSStatus
	IEGetVendorSpecific( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint32_t			inVID, 
		const uint8_t **	outData, 
		size_t *			outLen, 
		const uint8_t **	outNext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IECopyCoalescedVendorSpecific
	@abstract	Gathers the data sections fo all elements of a split, vendor-specific IE into a single, malloc'd buffer.
*/

OSStatus
	IECopyCoalescedVendorSpecific( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint32_t			inVID, 
		uint8_t **			outPtr, 
		size_t *			outLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IEGetAppleGeneral
	@abstract	Searches for an Apple General ID and returns its data if found.
*/

OSStatus	IEGetAppleGeneral( const uint8_t *inSrc, const uint8_t *inEnd, uint8_t *outProductID, uint16_t *outFlags );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IEGetDWDS
	@abstract	Searches for a DWDS ID and returns its data if found.
*/
		
OSStatus	IEGetDWDS( const uint8_t *inSrc, const uint8_t *inEnd, uint8_t *outRole, uint32_t *outFlags );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IEGetTLV16
	@abstract	Searches for the specified 16-bit TLV IE and returns its data if found.
	
	@param		inBuf		Optional ptr to store the TLV data. May be NULL.
	@param		inBufLen	Max number of bytes to write into "inBuf". May be 0.
	@param		outLen		Receives number of bytes copied into "inBuf". If "inBuf" is NULL, returns total size.
*/

OSStatus
	IEGetTLV16( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint32_t			inVID, 
		uint16_t			inAttrID, 
		void *				inBuf, 
		size_t				inBufLen, 
		size_t *			outLen, 
		const uint8_t **	outNext );

#if 0
#pragma mark == IE Building ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		IEBuffer
	@abstract	Functions for building IEs.
*/

typedef struct
{
	uint8_t			buf[ 256 ];
	size_t			len;
	OSStatus		firstErr;
	size_t			savedOffset;
	
}	IEBuffer;

#define 	IEBufferInit( BUF )	do { (BUF)->len = 0; (BUF)->firstErr = kNoErr; (BUF)->savedOffset = 0; } while( 0 )
OSStatus	IEBufferAppendIE( IEBuffer *inBuf, uint8_t inEID, const void *inData, size_t inLen );
OSStatus	IEBufferStartVendorIE( IEBuffer *inBuf, uint32_t inVID );
OSStatus	IEBufferEndVendorIE( IEBuffer *inBuf );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	DataBuffer_AppendIE
	@abstract	Appends an IEEE 802.11 IE.
*/

OSStatus	DataBuffer_AppendIE( DataBuffer *inDB, uint8_t inEID, const void *inData, size_t inLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	DataBuffer_AppendVendorIE
	@abstract	Appends an IEEE 802.11 vendor-specific IE.
*/

OSStatus	DataBuffer_AppendVendorIE( DataBuffer *inDB, uint32_t inVID, const void *inData, size_t inLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	DataBuffer_AppendAppleGeneralIE
	@abstract	Appends an vendor-specific Apple General IE.
*/

OSStatus
	DataBuffer_AppendAppleGeneralIE( 
		DataBuffer *	inDB, 
		uint8_t			inProductID, 
		uint16_t		inFlags, 
		const uint8_t	inRadio1BSSID[ 6 ], uint8_t inRadio1Channel, 
		const uint8_t	inRadio2BSSID[ 6 ], uint8_t inRadio2Channel );

#if 0
#pragma mark == TLV16s ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	TLV16Get
	@abstract	Searches for the specified TLV and returns its data if found.
*/

OSStatus
	TLV16Get( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint16_t			inType, 
		const uint8_t **	outData, 
		size_t *			outLen, 
		const uint8_t **	outNext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	TLV16GetNext
	@abstract	Gets the next TLV.
*/

OSStatus
	TLV16GetNext( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		uint16_t *			outType, 
		const uint8_t **	outData, 
		size_t *			outLen, 
		const uint8_t **	outNext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	DataBuffer_AppendTLV
	@abstract	Appends a 16-bit TLV.
*/

OSStatus	DataBuffer_AppendTLV16( DataBuffer *inDB, uint16_t inType, const void *inData, size_t inLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IEEE80211Utils_Test
	@abstract	Unit test.
*/

#ifdef __cplusplus
}
#endif

#endif	// __IEEE80211Utils_h__
