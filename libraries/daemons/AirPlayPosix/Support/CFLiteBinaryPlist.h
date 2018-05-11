/*
	Copyright (C) 2006-2012 Apple Inc. All Rights Reserved.
*/

#ifndef	__CFLiteBinaryPlist_h__
#define	__CFLiteBinaryPlist_h__

#include "CommonServices.h"
#include "DebugServices.h"

	#include <stddef.h>

#include CF_HEADER

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFBinaryPlistCreateData
	@abstract	Converts an object to a binary plist. Note: Any container objects in the returned object are mutable.
*/

OSStatus	CFBinaryPlistCreateData( CFAllocatorRef inAllocator, CFTypeRef inObj, CFDataRef *outData );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFBinaryPlistCreateFromData
	@abstract	Converts a binary plist to an object.
*/

OSStatus	CFBinaryPlistCreateFromData( CFAllocatorRef inAllocator, const void *inData, size_t inSize, void *outCFObj );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFBinaryPlistV0CreateData
	@abstract	Converts an object to a version 0 binary plist (i.e. compatible with Mac/iOS binary plists).
*/

CFDataRef	CFBinaryPlistV0CreateData( CFTypeRef inObj, OSStatus *outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFBinaryPlistV0CreateWithData
	@abstract	Converts binary plist data to an object.
*/

CFPropertyListRef	CFBinaryPlistV0CreateWithData( const void *inPtr, size_t inLen, OSStatus *outErr );

#if 0
#pragma mark == Debugging ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CFLiteBinaryPlistTest
	@abstract	Unit test.
*/

OSStatus	CFLiteBinaryPlistTest( void );

#ifdef __cplusplus
}
#endif

#endif // __CFLiteBinaryPlist_h__
