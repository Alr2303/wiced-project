/*
	Copyright (C) 2007-2011 Apple Inc. All Rights Reserved.
*/

#ifndef	__URLUtils_h__
#define	__URLUtils_h__

#include "CommonServices.h"
#include "DebugServices.h"

#ifdef	__cplusplus
	extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	URLEncodedMaxSize
	@abstract	Calculates the max size of URL encoded data.
	@param		SIZE	Size of the data to be URL encoded.
	@result		Number of bytes when URL encoded.
*/

#define URLEncodedMaxSize( SIZE )		( (SIZE) * 3 ) // Max is each byte converted to "%XX".

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	URLEncode/URLEncodeCopy
	@abstract	Encodes the data for use in a URL (i.e. percent-encoding, such as converting a space to %20.
	@discussion	The "Copy" version returns a malloc'd, null-terminated string holding the encoded data.
*/

typedef enum
{
	kURLEncodeType_Generic	= 0, //! General encoding.
	kURLEncodeType_Query	= 1  //! Make suitable for as URL query string.
	
}	URLEncodeType;

OSStatus
	URLEncode( 
		URLEncodeType	inType, 
		const void *	inSourceData, 
		size_t			inSourceSize, 
		void *			inEncodedDataBuffer, 	// may be NULL (to calculate the encoded size).
		size_t			inEncodedDataBufferSize,
		size_t *		outEncodedSize );

OSStatus
	URLEncodeCopy( 
		URLEncodeType	inType, 
		const void *	inSourceData, 
		size_t			inSourceSize, 
		void *			outEncodedStr, 
		size_t *		outEncodedLen ); // may be NULL

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	URLDecode
	@abstract	Decodes URL-encoded data (i.e. percent-encoded, such as %20 for a space).
	@discussion	The "Copy" version returns a malloc'd, null-terminated string holding the encoded data.
*/

#define URLDecode( ENCODED_PTR, ENCODED_LEN, DECODED_BUF_PTR, DECODED_BUF_LEN, OUT_LEN )	\
	URLDecodeEx( (ENCODED_PTR), (ENCODED_LEN), (DECODED_BUF_PTR), (DECODED_BUF_LEN), (OUT_LEN), NULL )

OSStatus
	URLDecodeEx( 
		const void *	inEncodedData, 
		size_t 			inEncodedSize, 
		void *			inDecodedDataBuffer, // may be NULL (to calculate the decoded size).
		size_t			inDecodedDataBufferSize, 
		size_t *		outDecodedSize, 
		int *			outChanges );

OSStatus
	URLDecodeCopy( 
		const void *	inEncodedData, 
		size_t			inEncodedSize, 
		void *			outDecodedPtr, 
		size_t *		outDecodedSize );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	URLParseComponents
	@abstract	Parses an absolute or relative URL into the general components supported by all URI's.
*/

typedef struct
{
	const char *	schemePtr;
	size_t			schemeLen;
	const char *	userPtr;
	size_t			userLen;
	const char *	passwordPtr;
	size_t			passwordLen;
	const char *	hostPtr;
	size_t			hostLen;
	const char *	pathPtr;
	size_t			pathLen;
	const char *	queryPtr;
	size_t			queryLen;
	const char *	fragmentPtr;
	size_t			fragmentLen;
	
	const char *	segmentPtr; // Ptr to the current resource path segment. Leading slash is removed, if present.
	const char *	segmentEnd; // End of the resource path segments. Trailing slash is removed, if present.
	
}	URLComponents;

OSStatus	URLParseComponents( const char *inSrc, const char *inEnd, URLComponents *outComponents, const char **outSrc );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	URLGetNextPathSegment
	@abstract	Parses the next URL segment from a URL.
	@discussion
	
	If you had a URL such as "/path/to/my/resource/", you would get the following with this function:
	
	kNoErr			"path"
	kNoErr			"to"
	kNoErr			"my"
	kNoErr			"resource"
	kNotFoundErr
	
*/

OSStatus	URLGetNextPathSegment( URLComponents *inComps, const char **outSegmentPtr, size_t *outSegmentLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	URLGetOrCopyNextVariable / URLGetNextVariable
	@abstract	Gets the next variable in an URL query string (e.g. "name=value&name2=value").
	@discussion
	
	This is intended to be used in a loop to process all the variables:
	
	... parse the query string from a URL
	
	while( URLGetNextVariable( queryPtr, queryEnd, &namePtr, &nameLen, &valuePtr, &valueLen, &queryPtr ) == kNoErr )
	{
		char		value[ 256 ];
		
		err = URLDecode( valuePtr, valueLen, value, sizeof( value ), &valueLen );
		... handle error
	}
*/

OSStatus
	URLGetOrCopyNextVariable( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char **	outNamePtr, 
		size_t *		outNameLen, 
		char **			outNameStorage, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		char **			outValueStorage, 
		const char **	outSrc );

OSStatus
	URLGetNextVariable( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char **	outNamePtr, 
		size_t *		outNameLen, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		const char **	outSrc );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	URLGetOrCopyVariable / URLGetVariable
	@abstract	Gets a URL query string variable by name.
	
*/

OSStatus
	URLGetOrCopyVariable( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char *	inName, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		char **			outValueStorage, 
		const char **	outSrc );

OSStatus
	URLGetVariable( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char *	inName, 
		const char **	outValuePtr, 
		size_t *		outValueLen, 
		const char **	outSrc );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	URLUtils_Test	
	@abstract	Unit test.
*/

#ifdef	__cplusplus
	}
#endif

#endif	// __URLUtils_h__
