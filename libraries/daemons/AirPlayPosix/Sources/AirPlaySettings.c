/*
	Copyright (C) 2010-2012 Apple Inc. All Rights Reserved.
*/

#include "AirPlaySettings.h"

#include "AirPlayCommon.h"
#include "CFUtils.h"
#include "SystemUtils.h"

#include CF_HEADER

//===========================================================================================================================
//	Internals
//===========================================================================================================================

#define kAirPlaySettingsBundleID		CFSTR( "com.apple.MediaToolbox" )
#define kAirPlaySettingsFilename		CFSTR( "AirPlaySettings.plist" )

//===========================================================================================================================
//	AirPlaySettings_CopyKeys
//===========================================================================================================================

EXPORT_GLOBAL
CFArrayRef	AirPlaySettings_CopyKeys( OSStatus *outErr )
{
	CFMutableArrayRef		allKeys;
	OSStatus				err;
	CFArrayRef				keys;
	
	allKeys = CFArrayCreateMutable( NULL, 0, &kCFTypeArrayCallBacks );
	require_action( allKeys, exit, err = kNoMemoryErr );
	
	keys = CFPreferencesCopyKeyList( CFSTR( kAirPlayPrefAppID ), kCFPreferencesCurrentUser, kCFPreferencesAnyHost );
	if( keys )
	{
		CFArrayAppendArray( allKeys, keys, CFRangeMake( 0, CFArrayGetCount( keys ) ) );
		CFRelease( keys );
	}
	
	err = kNoErr;
	
exit:
	if( outErr ) *outErr = err;
	return( allKeys );
}

//===========================================================================================================================
//	AirPlaySettings_CopyValue
//===========================================================================================================================

EXPORT_GLOBAL
CFTypeRef	AirPlaySettings_CopyValue( CFDictionaryRef *ioSettings, CFStringRef inKey, OSStatus *outErr )
{
	return( AirPlaySettings_CopyValueEx( ioSettings, inKey, 0, outErr ) );
}

EXPORT_GLOBAL
CFTypeRef	AirPlaySettings_CopyValueEx( CFDictionaryRef *ioSettings, CFStringRef inKey, CFTypeID inType, OSStatus *outErr )
{
	CFDictionaryRef		settings = (CFDictionaryRef)( ioSettings ? *ioSettings : NULL );
	CFTypeRef			value;
	OSStatus			err;
	
	value = CFPreferencesCopyAppValue( inKey, CFSTR( kAirPlayPrefAppID ) );
	if( value && ( inType != 0 ) && ( CFGetTypeID( value ) != inType ) )
	{
		dlog( kLogLevelNotice, "### Wrote type for AirPlay setting '%@': '%@'\n", inKey, value );
		CFRelease( value );
		value = NULL;
		err = kTypeErr;
		goto exit;
	}
	err = value ? kNoErr : kNotFoundErr;
	
exit:
	if( ioSettings )	*ioSettings = settings;
	else if( settings )	CFRelease( settings );
	if( outErr )		*outErr = err;
	return( value );
}

//===========================================================================================================================
//	AirPlaySettings_GetCString
//===========================================================================================================================

EXPORT_GLOBAL
char *
	AirPlaySettings_GetCString( 
		CFDictionaryRef *	ioSettings, 
		CFStringRef			inKey, 
		char *				inBuf, 
		size_t				inMaxLen, 
		OSStatus *			outErr )
{
	char *			value;
	CFTypeRef		cfValue;
	
	cfValue = AirPlaySettings_CopyValue( ioSettings, inKey, outErr );
	if( cfValue )
	{	
		value = CFGetCString( cfValue, inBuf, inMaxLen );
		CFRelease( cfValue );
		return( value );
	}
	return( NULL );
}

//===========================================================================================================================
//	AirPlaySettings_GetDouble
//===========================================================================================================================

EXPORT_GLOBAL
double	AirPlaySettings_GetDouble( CFDictionaryRef *ioSettings, CFStringRef inKey, OSStatus *outErr )
{
	double			value;
	CFTypeRef		cfValue;
	
	cfValue = AirPlaySettings_CopyValue( ioSettings, inKey, outErr );
	if( cfValue )
	{
		value = CFGetDouble( cfValue, outErr );
		CFRelease( cfValue );
		return( value );
	}
	return( 0 );
}

//===========================================================================================================================
//	AirPlaySettings_GetInt64
//===========================================================================================================================

EXPORT_GLOBAL
int64_t	AirPlaySettings_GetInt64( CFDictionaryRef *ioSettings, CFStringRef inKey, OSStatus *outErr )
{
	int64_t			value;
	CFTypeRef		cfValue;
	
	cfValue = AirPlaySettings_CopyValue( ioSettings, inKey, outErr );
	if( cfValue )
	{
		value = CFGetInt64( cfValue, outErr );
		CFRelease( cfValue );
		return( value );
	}
	return( 0 );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	AirPlaySettings_SetCString
//===========================================================================================================================

EXPORT_GLOBAL
OSStatus	AirPlaySettings_SetCString( CFStringRef inKey, const char *inStr, size_t inLen )
{
	OSStatus		err;
	CFStringRef		cfStr;
	
	if( inLen == kSizeCString )
	{
		cfStr = CFStringCreateWithCString( NULL, inStr, kCFStringEncodingUTF8 );
	}
	else
	{
		cfStr = CFStringCreateWithBytes( NULL, (const uint8_t *) inStr, (CFIndex) inLen, kCFStringEncodingUTF8, false );
	}
	require_action( cfStr, exit, err = kFormatErr );
	
	err = AirPlaySettings_SetValue( inKey, cfStr );
	CFRelease( cfStr );
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlaySettings_SetDouble
//===========================================================================================================================

EXPORT_GLOBAL
OSStatus	AirPlaySettings_SetDouble( CFStringRef inKey, double inValue )
{
	return( AirPlaySettings_SetNumber( inKey, kCFNumberDoubleType, &inValue ) );
}

//===========================================================================================================================
//	AirPlaySettings_SetInt64
//===========================================================================================================================

EXPORT_GLOBAL
OSStatus	AirPlaySettings_SetInt64( CFStringRef inKey, int64_t inValue )
{
	OSStatus		err;
	CFNumberRef		num;
	
	num = CFNumberCreateInt64( inValue );
	require_action( num, exit, err = kUnknownErr );
	
	err = AirPlaySettings_SetValue( inKey, num );
	CFRelease( num );
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlaySettings_SetNumber
//===========================================================================================================================

EXPORT_GLOBAL
OSStatus	AirPlaySettings_SetNumber( CFStringRef inKey, CFNumberType inType, const void *inValue )
{
	OSStatus		err;
	CFNumberRef		num;
	
	num = CFNumberCreate( NULL, inType, inValue );
	require_action( num, exit, err = kUnknownErr );
	
	err = AirPlaySettings_SetValue( inKey, num );
	CFRelease( num );
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlaySettings_SetValue
//===========================================================================================================================

EXPORT_GLOBAL
OSStatus	AirPlaySettings_SetValue( CFStringRef inKey, CFTypeRef inValue )
{
	CFPreferencesSetAppValue( inKey, inValue, CFSTR( kAirPlayPrefAppID ) );
	return( kNoErr );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	AirPlaySettings_RemoveValue
//===========================================================================================================================

EXPORT_GLOBAL
OSStatus	AirPlaySettings_RemoveValue( CFStringRef inKey )
{
	CFPreferencesSetAppValue( inKey, NULL, CFSTR( kAirPlayPrefAppID ) );
	return( kNoErr );
}

//===========================================================================================================================
//	AirPlaySettings_Synchronize
//===========================================================================================================================

EXPORT_GLOBAL
void	AirPlaySettings_Synchronize( void )
{
	CFPreferencesAppSynchronize( CFSTR( kAirPlayPrefAppID ) );
}

#if 0
#pragma mark -
#endif

