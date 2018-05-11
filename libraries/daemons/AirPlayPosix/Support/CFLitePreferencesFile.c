/*
	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
*/

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>

#include "CommonServices.h"
#include "DebugServices.h"
#include "MiscUtils.h"
#include "ThreadUtils.h"

#include CF_HEADER

//===========================================================================================================================
//	Internals
//===========================================================================================================================

static pthread_mutex_t				gLock  = PTHREAD_MUTEX_INITIALIZER;
static CFMutableDictionaryRef		gPrefs = NULL;

static void						_CFPreferencesCopyKeyListApplier( const void *inKey, const void *inValue, void *inContext );
static CFMutableDictionaryRef	_CopyDictionaryFromFile( CFStringRef inAppID );
static OSStatus					_WritePlistToFile( CFStringRef inAppID, CFPropertyListRef inPlist );

//===========================================================================================================================
//	CFPreferencesCopyKeyList_compat
//===========================================================================================================================

CFArrayRef	CFPreferencesCopyKeyList_compat( CFStringRef inAppID, CFStringRef inUser, CFStringRef inHost )
{
	CFArrayRef					result		= NULL;
	CFDictionaryRef				appDict;
	CFMutableDictionaryRef		appDictCopy = NULL;
	CFMutableArrayRef			keys		= NULL;
	
	(void) inUser;
	(void) inHost;
	
	pthread_mutex_lock( &gLock );
	
	if( !gPrefs )
	{
		gPrefs = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
		require( gPrefs, exit );
	}
	
	appDict = (CFDictionaryRef) CFDictionaryGetValue( gPrefs, inAppID );
	if( !appDict )
	{
		appDictCopy = _CopyDictionaryFromFile( inAppID );
		if( !appDictCopy )
		{
			appDictCopy = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
			require( appDictCopy, exit );
		}
		CFDictionarySetValue( gPrefs, inAppID, appDictCopy );
		appDict = appDictCopy;
	}
	
	keys = CFArrayCreateMutable( NULL, 0, &kCFTypeArrayCallBacks );
	require( keys, exit );
	
	CFDictionaryApplyFunction( appDict, _CFPreferencesCopyKeyListApplier, keys );
	result = keys;
	keys = NULL;
	
exit:
	CFReleaseNullSafe( keys );
	CFReleaseNullSafe( appDictCopy );
	pthread_mutex_unlock( &gLock );
	return( result );
}

//===========================================================================================================================
//	_CFPreferencesCopyKeyListApplier
//===========================================================================================================================

static void	_CFPreferencesCopyKeyListApplier( const void *inKey, const void *inValue, void *inContext )
{
	CFMutableArrayRef const		keys = (CFMutableArrayRef) inContext;
	
	(void) inValue;
	
	CFArrayAppendValue( keys, inKey );
}

//===========================================================================================================================
//	CFPreferencesCopyAppValue_compat
//===========================================================================================================================

CFPropertyListRef	CFPreferencesCopyAppValue_compat( CFStringRef inKey, CFStringRef inAppID )
{
	CFPropertyListRef			value = NULL;
	CFDictionaryRef				appDict;
	CFMutableDictionaryRef		appDictCopy = NULL;
	
	pthread_mutex_lock( &gLock );
	
	if( !gPrefs )
	{
		gPrefs = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
		require( gPrefs, exit );
	}
	
	appDict = (CFDictionaryRef) CFDictionaryGetValue( gPrefs, inAppID );
	if( !appDict )
	{
		appDictCopy = _CopyDictionaryFromFile( inAppID );
		if( !appDictCopy )
		{
			appDictCopy = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
			require( appDictCopy, exit );
		}
		CFDictionarySetValue( gPrefs, inAppID, appDictCopy );
		appDict = appDictCopy;
	}
	
	value = CFDictionaryGetValue( appDict, inKey );
	CFRetainNullSafe( value );
	
exit:
	CFReleaseNullSafe( appDictCopy );
	pthread_mutex_unlock( &gLock );
	return( value );
}

//===========================================================================================================================
//	CFPreferencesSetAppValue_compat
//===========================================================================================================================

void	CFPreferencesSetAppValue_compat( CFStringRef inKey, CFPropertyListRef inValue, CFStringRef inAppID )
{
	CFMutableDictionaryRef		appDict;
	CFMutableDictionaryRef		appDictCopy = NULL;
	
	pthread_mutex_lock( &gLock );
	
	if( !gPrefs )
	{
		gPrefs = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
		require( gPrefs, exit );
	}
	
	appDict = (CFMutableDictionaryRef) CFDictionaryGetValue( gPrefs, inAppID );
	if( !appDict )
	{
		appDictCopy = _CopyDictionaryFromFile( inAppID );
		if( !appDictCopy )
		{
			appDictCopy = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
			require( appDictCopy, exit );
		}
		CFDictionarySetValue( gPrefs, inAppID, appDictCopy );
		appDict = appDictCopy;
	}
	
	if( inValue )	CFDictionarySetValue( appDict, inKey, inValue );
	else			CFDictionaryRemoveValue( appDict, inKey );
	_WritePlistToFile( inAppID, appDict );
	
exit:
	CFReleaseNullSafe( appDictCopy );
	pthread_mutex_unlock( &gLock );
}

//===========================================================================================================================
//	CFPreferencesAppSynchronize_compat
//===========================================================================================================================

Boolean	CFPreferencesAppSynchronize_compat( CFStringRef inAppID )
{
	// Remove the app dictionary (if it exists) to cause it to be re-read on the next get.
	
	if( gPrefs ) CFDictionaryRemoveValue( gPrefs, inAppID );
	return( true );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	_CopyDictionaryFromFile
//===========================================================================================================================

static CFMutableDictionaryRef	_CopyDictionaryFromFile( CFStringRef inAppID )
{
	CFMutableDictionaryRef		dict = NULL;
	OSStatus					err;
	char						homePath[ PATH_MAX ];
	char						path[ PATH_MAX ];
	FILE *						file = NULL;
	CFMutableDataRef			data = NULL;
	uint8_t *					buf  = NULL;
	size_t						len, n;
	
	// Read from "~/Library/Preferences/<app ID>.plist".
	
	*homePath = '\0';
	GetHomePath( homePath, sizeof( homePath ) );
	*path = '\0';
	SNPrintF( path, sizeof( path ), "%s/Library/Preferences/%@.plist", homePath, inAppID );
	
	file = fopen( path, "rb" );
	err = map_global_value_errno( file, file );
	require_noerr_quiet( err, exit );
	
	data = CFDataCreateMutable( NULL, 0 );
	require_action( data, exit, err = kNoMemoryErr );
	
	len = 32 * 1024;
	buf = (uint8_t *) malloc( len );
	require( buf, exit );
	
	for( ;; )
	{
		n = fread( buf, 1, len, file );
		if( n == 0 ) break;
		CFDataAppendBytes( data, buf, (CFIndex) n );
	}
	
	dict = (CFMutableDictionaryRef) CFPropertyListCreateWithData( NULL, data, kCFPropertyListMutableContainers, NULL, NULL );
	if( dict && !CFIsType( dict, CFDictionary ) )
	{
		dlogassert( "Prefs must be a dictionary: %@", dict );
		CFRelease( dict );
		dict = NULL;
	}
	require_quiet( dict, exit );
	
exit:
	if( buf ) free( buf );
	CFReleaseNullSafe( data );
	if( file ) fclose( file );
	return( dict );
}

//===========================================================================================================================
//	_WritePlistToFile
//===========================================================================================================================

static OSStatus	_WritePlistToFile( CFStringRef inAppID, CFPropertyListRef inPlist )
{
	OSStatus			err;
	char				homePath[ PATH_MAX ];
	char				path[ PATH_MAX ];
	CFDataRef			data = NULL;
	int					fd = -1;
	const uint8_t *		ptr;
	const uint8_t *		end;
	ssize_t				n;
	
	// Create the ~/Library/Preferences parent folder if it doesn't already exist.
	
	*homePath = '\0';
	GetHomePath( homePath, sizeof( homePath ) );
	*path = '\0';
	SNPrintF( path, sizeof( path ), "%s/Library/Preferences", homePath );
	
	err = mkpath( path, S_IRWXU, S_IRWXU );
	err = map_global_noerr_errno( err );
	if( err && ( err != EEXIST ) ) dlogassert( "Make parent %s failed: %#m", path, err );
	
	// Write the plist to "~/Library/Preferences/<app ID>.plist".
	
	data = CFPropertyListCreateData( NULL, inPlist, kCFPropertyListBinaryFormat_v1_0, 0, NULL );
	require_action( data, exit, err = kUnknownErr );
	
	*path = '\0';
	SNPrintF( path, sizeof( path ), "%s/Library/Preferences/%@.plist", homePath, inAppID );
	fd = open( path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR );
	err = map_fd_creation_errno( fd );
	require_noerr( err, exit );
	
	ptr = CFDataGetBytePtr( data );
	end = ptr + CFDataGetLength( data );
	for( ; ptr != end; ptr += n )
	{
		n = write( fd, ptr, (size_t)( end - ptr ) );
		err = map_global_value_errno( n > 0, n );
		require_noerr( err, exit );
	}
	
exit:
	if( fd >= 0 ) close( fd );
	CFReleaseNullSafe( data );
	return( err );
}

#if 0
#pragma mark -
#endif

