/*
	Copyright (C) 2010-2012 Apple Inc. All Rights Reserved.
*/

#ifndef	__CFPrefUtils_h__
#define	__CFPrefUtils_h__

#include "CommonServices.h"

#include CF_HEADER

#ifdef	__cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFPrefsGetDouble
	@abstract	Gets a double from a CF pref, converting as necessary.
	@discussion
	
	This tries to guess at a reasonable integer given a CF type:
	
	CFBoolean:			true = 1, false = 0.
	CFNumber (float):	Returned as-is.
	CFNumber (integer):	CF conversion to double.
	CFString:			"true"/"yes" = 1.
						"false"/"no" = 0.
						Decimal integer text, converted to integer.
						0x-prefixed, converted from hex to integer.
						0-prefixed, converted from octal to integer.
	CFData:				Converts big endian integer value to integer. Error if > 8 bytes.
*/

double	CFPrefsGetDouble( CFStringRef inAppID, CFStringRef inKey, OSStatus *outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFPrefsGetInt64
	@abstract	Gets an int64_t from a CF pref, converting as necessary.
	@discussion
	
	This tries to guess at a reasonable integer given a CF type:
	
	CFBoolean:			true = 1, false = 0.
	CFNumber (float):	Saturates to 64-bit integer min/max or truncates to integer.
	CFNumber (integer):	64-bit integer.
	CFString:			"true"/"yes" = 1.
						"false"/"no" = 0.
						Decimal integer text, converted to integer.
						0x-prefixed, converted from hex to integer.
						0-prefixed, converted from octal to integer.
	CFData:				Converts big endian integer value to integer. Error if > 8 bytes.
*/

#define CFPrefsGetBoolean( APP_ID, KEY, ERR_PTR )	( ( CFPrefsGetInt64( (APP_ID), (KEY), (ERR_PTR) ) != 0 ) ? true : false )
int64_t	CFPrefsGetInt64( CFStringRef inAppID, CFStringRef inKey, OSStatus *outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFPrefsSetInt64
	@abstract	Sets a pref to a 64-bit integer value.
*/

#define		CFPrefsSetBoolean( APP_ID, KEY, X )	CFPreferencesSetAppValue( (KEY), (X) ? kCFBooleanTrue : kCFBooleanFalse, (APP_ID) )
OSStatus	CFPrefsSetInt64( CFStringRef inAppID, CFStringRef inKey, int64_t inValue );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFPrefsGetCString
	@abstract	Gets a C string from a pref.
*/

char *	CFPrefsGetCString( CFStringRef inAppID, CFStringRef inKey, char *inBuf, size_t inMaxLen, OSStatus *outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFPrefsGetData
	@abstract	Gets a binary data representation from a string pref.
*/

uint8_t *	CFPrefsGetData( CFStringRef inAppID, CFStringRef inKey, void *inBuf, size_t inMaxLen, size_t *outLen, OSStatus *outErr );

#ifdef __cplusplus
}
#endif

#endif	// __CFPrefUtils_h__
