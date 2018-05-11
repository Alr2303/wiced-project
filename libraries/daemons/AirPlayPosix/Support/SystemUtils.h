/*
	Copyright (C) 2010-2013 Apple Inc. All Rights Reserved.
*/
/*!
	@header		System information API
	@discussion	APIs for getting information about the system, such as device name, model, etc.
*/

#ifndef	__SystemUtils_h__
#define	__SystemUtils_h__

#include "CommonServices.h"

#include CF_HEADER

#ifdef __cplusplus
extern "C" {
#endif

	#define kGestaltBuildVersion				CFSTR( "BuildVersion" )
	#define kGestaltDeviceClass					CFSTR( "DeviceClass" )
	#define kGestaltDeviceName					CFSTR( "DeviceName" )
	#define kGestaltDeviceSupports1080p			CFSTR( "DeviceSupports1080p" )
	#define kGestaltEthernetMacAddress			CFSTR( "EthernetMacAddress" )
	#define kGestaltInternalBuild				CFSTR( "InternalBuild" )
	#define kGestaltUniqueDeviceID				CFSTR( "UniqueDeviceID" )
	#define kGestaltUserAssignedDeviceName		CFSTR( "UserAssignedDeviceName" )
	#define kGestaltWifiAddress					CFSTR( "WifiAddress" )
	#define kGestaltWifiAddressData				CFSTR( "WifiAddressData" )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	GestaltSetHook
	@internal
	@abstract	Sets a hook function to intercept all Gestalt requests and optionally override them.
	@discussion
	
	A hook function must either return NULL with outErr (if non-NULL) filled in with a non-zero error code.
	Or it must return a non-NULL object that the caller is required to release and fill in outErr (if non-NULL) with kNoErr.
	kNotHandledErr should be returned if hook does not want to provide an override (the default handler will handle it).
*/
typedef CFTypeRef ( *GestaltHook_f )( CFStringRef inQuestion, CFDictionaryRef inOptions, OSStatus *outErr, void *inContext );
void	GestaltSetHook( GestaltHook_f inHook, void *inContext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	GestaltCopyAnswer
	@internal
	@abstract	Returns information about the system. Caller must release result if non-NULL.
*/
CFTypeRef	GestaltCopyAnswer( CFStringRef inQuestion, CFDictionaryRef inOptions, OSStatus *outErr );
Boolean		GestaltGetBoolean( CFStringRef inQuestion, CFDictionaryRef inOptions, OSStatus *outErr );
char *		GestaltGetCString( CFStringRef inQuestion, CFDictionaryRef inOptions, char *inBuf, size_t inMaxLen, OSStatus *outErr );
uint8_t *
	GestaltGetData( 
		CFStringRef		inQuestion, 
		CFDictionaryRef	inOptions, 
		void *			inBuf, 
		size_t			inMaxLen, 
		size_t *		outLen, 
		OSStatus *		outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	GetDeviceModelString
	@abstract	Gets the model string for the device (e.g. "Device1,1").

	@param		inBuf		Buffer to fill the device model string in. Should be atleast "inMaxLen" bytes.
	@param		inMaxLen	Number of bytes in the "inBuf" buffer.

	@result		returns the pointer to the buffer in case of success, NULL in case of failure.
*/
char *	GetDeviceModelString( char *inBuf, size_t inMaxLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	GetDeviceInternalModelString
	@internal
	@abstract	Gets the internal model string for the device (e.g. "K48").
*/
char *	GetDeviceInternalModelString( char *inBuf, size_t inMaxLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	GetDeviceName
	@abstract	Gets the most user-friendly name available for the device.

	@param		inBuf		Buffer to fill the device name in. Should be atleast "inMaxLen" bytes.
	@param		inMaxLen	Number of bytes in the "inBuf" buffer.

	@result		returns the pointer to the buffer in case of success, NULL in case of failure.
*/
char *	GetDeviceName( char *inBuf, size_t inMaxLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	GetDeviceUniqueID
	@internal
	@abstract	Gets the UDID (or something similar). It's a 40-hex digit string inBuf needs to be 41+ bytes.
*/
char *	GetDeviceUniqueID( char *inBuf, size_t inMaxLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	GetSystemBuildVersionString
	@abstract	Gets the build version of the OS (e.g. "12A85").

	@param		inBuf		Buffer to fill the build version string in. Should be atleast "inMaxLen" bytes.
	@param		inMaxLen	Number of bytes in the "inBuf" buffer.

	@result		returns the pointer to the buffer in case of success, NULL in case of failure.
*/
char *	GetSystemBuildVersionString( char *inBuf, size_t inMaxLen );

#ifdef __cplusplus
}
#endif

#endif // __SystemUtils_h__
