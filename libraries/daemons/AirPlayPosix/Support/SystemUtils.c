/*
	Copyright (C) 2010-2013 Apple Inc. All Rights Reserved.
*/

#include "SystemUtils.h"

#include <string.h>

#include "CFUtils.h"
#include "CommonServices.h"
#include "DebugServices.h"
#include "StringUtils.h"

	#include <sys/stat.h>
	#include <sys/sysctl.h>

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	GestaltSetHook
//===========================================================================================================================

static GestaltHook_f		gGestaltHook_f   = NULL;
static void *				gGestaltHook_ctx = NULL;

void	GestaltSetHook( GestaltHook_f inHook, void *inContext )
{
	gGestaltHook_f   = inHook;
	gGestaltHook_ctx = inContext;
}

//===========================================================================================================================
//	GestaltCopyAnswer
//===========================================================================================================================

CFTypeRef	GestaltCopyAnswer( CFStringRef inQuestion, CFDictionaryRef inOptions, OSStatus *outErr )
{
	CFTypeRef		answer;
	
	if( gGestaltHook_f )
	{
		answer = gGestaltHook_f( inQuestion, inOptions, outErr, gGestaltHook_ctx );
		if( answer ) return( answer );
	}
	
	if( outErr ) *outErr = kNotFoundErr;
	return( NULL );
}

//===========================================================================================================================
//	GestaltGetBoolean
//===========================================================================================================================

Boolean	GestaltGetBoolean( CFStringRef inQuestion, CFDictionaryRef inOptions, OSStatus *outErr )
{
	Boolean			b;
	CFTypeRef		obj;
	
	obj = GestaltCopyAnswer( inQuestion, inOptions, outErr );
	if( obj )
	{
		b = CFGetBoolean( obj, outErr );
		CFRelease( obj );
	}
	else
	{
		b = false;
	}
	return( b );
}

//===========================================================================================================================
//	GestaltGetCString
//===========================================================================================================================

char *	GestaltGetCString( CFStringRef inQuestion, CFDictionaryRef inOptions, char *inBuf, size_t inMaxLen, OSStatus *outErr )
{
	CFTypeRef		obj;
	char *			ptr;
	
	obj = GestaltCopyAnswer( inQuestion, inOptions, outErr );
	if( obj )
	{
		ptr = CFGetCString( obj, inBuf, inMaxLen );
		CFRelease( obj );
		if( outErr ) *outErr = kNoErr;
	}
	else
	{
		ptr = inBuf;
	}
	return( ptr );
}

//===========================================================================================================================
//	GestaltGetData
//===========================================================================================================================

uint8_t *
	GestaltGetData( 
		CFStringRef		inQuestion, 
		CFDictionaryRef	inOptions, 
		void *			inBuf, 
		size_t			inMaxLen, 
		size_t *		outLen, 
		OSStatus *		outErr )
{
	uint8_t *		ptr;
	CFTypeRef		obj;
	
	obj = GestaltCopyAnswer( inQuestion, inOptions, outErr );
	if( obj )
	{
		ptr = CFGetData( obj, inBuf, inMaxLen, outLen, outErr );
		CFRelease( obj );
	}
	else
	{
		ptr = NULL;
		if( outLen ) *outLen = 0;
	}
	return( ptr );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	GetDeviceInternalModelString
//===========================================================================================================================

//===========================================================================================================================
//	GetDeviceName
//===========================================================================================================================

char *	GetDeviceName( char *inBuf, size_t inMaxLen )
{
	if( inMaxLen > 0 )
	{
		inBuf[ 0 ] = '\0';
		gethostname( inBuf, inMaxLen );
		inBuf[ inMaxLen - 1 ] = '\0';
		return( inBuf );
	}
	return( "" );
}

//===========================================================================================================================
//	GetDeviceUniqueID
//===========================================================================================================================

//===========================================================================================================================
//	GetSystemBuildVersionString
//===========================================================================================================================

