/*
	Copyright (C) 2001-2012 Apple Inc. All Rights Reserved.
*/
/*!
	@header			Random Bytes
	@discussion		Platform interfaces for random bytes for cyryptograpic purposes
*/

#ifndef	__RandomNumberUtils_h__
#define	__RandomNumberUtils_h__

#if( defined( RandomNumberUtils_PLATFORM_HEADER ) )
	#include  RandomNumberUtils_PLATFORM_HEADER
#endif

#include "CommonServices.h"
#include "DebugServices.h"

	#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	RandomBytes
	@abstract	Fills a buffer with cryptographically strong pseudo-random bytes.

	@param		inBuffer		Buffer to receiver the rnadom bytes. Must be atleast inByteCount bytes.
	@param		inByteCount		Number of bytes to be filled in "inBuffer". 

	@result		kNoErr if successful or an error code indicating failure.
*/

OSStatus	RandomBytes( void *inBuffer, size_t inByteCount );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	RandomString
	@internal
	@abstract	Generates a null-terminated string of a random length between min/maxChars with random characters.
	
	@param		inCharSet		String containing the characters to use to fill the random string.
	@param		inCharSetSize	Number of characters in "inCharSet". 
	@param		inMinChars		Min number of characters to generate. Does not include the null terminator.
	@param		inMaxChars		Max number of characters to fill in. Does not include the null terminator.
	@param		outString		Buffer to receive the string. Must be at least inMaxChars + 1 bytes.
	
	@result		Ptr to beginning of textual string (same as input buffer).
*/

char *	RandomString( const char *inCharSet, size_t inCharSetSize, size_t inMinChars, size_t inMaxChars, char *outString );

#ifdef __cplusplus
}
#endif

#endif	// __RandomNumberUtils_h__
