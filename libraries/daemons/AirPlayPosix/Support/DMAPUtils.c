/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/

#include "DMAPUtils.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "CFUtils.h"
#include "CommonServices.h"
#include "DebugServices.h"
#include "DMAP.h"
#include "StringUtils.h"

#include CF_HEADER

//===========================================================================================================================
//	DMAPContentBlock_Init
//===========================================================================================================================

void	DMAPContentBlock_Init( DMAPContentBlock *inBlock, void *inStaticBuffer, size_t inStaticBufferSize )
{
	inBlock->staticBuffer		= (uint8_t *) inStaticBuffer;
	inBlock->staticBufferSize	= inStaticBufferSize;
	inBlock->buffer				= (uint8_t *) inStaticBuffer;
	inBlock->bufferSize			= inStaticBufferSize;
	inBlock->bufferUsed			= 0;
	inBlock->malloced			= 0;
	inBlock->containerLevel		= 0;
	inBlock->firstErr			= kNoErr;
}

//===========================================================================================================================
//	DMAPContentBlock_Free
//===========================================================================================================================

void	DMAPContentBlock_Free( DMAPContentBlock *inBlock )
{
	if( inBlock->malloced )
	{
		check( inBlock->buffer );
		free( inBlock->buffer );
	}
	inBlock->buffer			= inBlock->staticBuffer;
	inBlock->bufferSize		= inBlock->staticBufferSize;
	inBlock->bufferUsed		= 0;
	inBlock->malloced		= 0;
	inBlock->containerLevel	= 0;
	inBlock->firstErr		= kNoErr;
}

//===========================================================================================================================
//	DMAPContentBlock_Commit
//===========================================================================================================================

OSStatus	DMAPContentBlock_Commit( DMAPContentBlock *inBlock, const uint8_t **outPtr, size_t *outSize )
{
	OSStatus		err;
	
	err = inBlock->firstErr;
	require_noerr_string( err, exit, "earlier error occurred" );
	require_action_string( inBlock->containerLevel == 0, exit, err = kStateErr, "all containers not closed" );
	
	inBlock->firstErr = kAlreadyInUseErr; // Mark in-use to prevent further changes to it.
	
	*outPtr  = inBlock->buffer;
	*outSize = inBlock->bufferUsed;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_OpenContainer
//===========================================================================================================================

OSStatus	DMAPContentBlock_OpenContainer( DMAPContentBlock *inBlock, DMAPContentCode inCode )
{
	OSStatus		err;
	uint32_t		header[ 2 ];
	
	require_action( inBlock->containerLevel < countof( inBlock->containerOffsets ), exit, err = kNoResourcesErr );
	
	WriteBig32( &header[ 0 ], inCode );
	WriteBig32( &header[ 1 ], 0 ); // Placeholder
	
	err = DMAPContentBlock_AddData( inBlock, header, 8 );
	require_noerr( err, exit );
	
	inBlock->containerOffsets[ inBlock->containerLevel++ ] = inBlock->bufferUsed - 4; // Offset to the container size.
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_CloseContainer
//===========================================================================================================================

OSStatus	DMAPContentBlock_CloseContainer( DMAPContentBlock *inBlock )
{
	OSStatus		err;
	size_t			offset;
	uint8_t *		ptr;
	size_t			len;
	
	require_action( inBlock->containerLevel > 0, exit, err = kNotInUseErr );
	offset = inBlock->containerOffsets[ --inBlock->containerLevel ];
	require_action( inBlock->bufferUsed >= ( offset + 4 ), exit, err = kInternalErr );
	
	// Update the container's total size with the amount of space used by the container.
	
	ptr = &inBlock->buffer[ offset ];
	len = inBlock->bufferUsed - ( offset + 4 );
	WriteBig32( ptr, (uint32_t) len );
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddInt8
//===========================================================================================================================

OSStatus	DMAPContentBlock_AddInt8( DMAPContentBlock *inBlock, DMAPContentCode inCode, uint8_t inValue )
{
	OSStatus		err;
	uint8_t			buf[ 9 ];
	
	WriteBig32( &buf[ 0 ], inCode );
	WriteBig32( &buf[ 4 ], 1 );
	buf[ 8 ] = inValue;
	
	err = DMAPContentBlock_AddData( inBlock, buf, 9 );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddInt16
//===========================================================================================================================

OSStatus	DMAPContentBlock_AddInt16( DMAPContentBlock *inBlock, DMAPContentCode inCode, uint16_t inValue )
{
	OSStatus		err;
	uint8_t			buf[ 10 ];
	
	WriteBig32( &buf[ 0 ], inCode );
	WriteBig32( &buf[ 4 ], 2 );
	WriteBig16( &buf[ 8 ], inValue );
	
	err = DMAPContentBlock_AddData( inBlock, buf, 10 );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddInt32
//===========================================================================================================================

OSStatus	DMAPContentBlock_AddInt32( DMAPContentBlock *inBlock, DMAPContentCode inCode, uint32_t inValue )
{
	OSStatus		err;
	uint8_t			buf[ 12 ];
	
	WriteBig32( &buf[ 0 ], inCode );
	WriteBig32( &buf[ 4 ], 4 );
	WriteBig32( &buf[ 8 ], inValue );
	
	err = DMAPContentBlock_AddData( inBlock, buf, 12 );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddInt64
//===========================================================================================================================

OSStatus	DMAPContentBlock_AddInt64( DMAPContentBlock *inBlock, DMAPContentCode inCode, uint64_t inValue )
{
	OSStatus		err;
	uint8_t			buf[ 16 ];
	
	WriteBig32( &buf[ 0 ], inCode );
	WriteBig32( &buf[ 4 ], 8 );
	WriteBig64( &buf[ 8 ], inValue );
	
	err = DMAPContentBlock_AddData( inBlock, buf, 16 );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddUTF8
//===========================================================================================================================

OSStatus	DMAPContentBlock_AddUTF8( DMAPContentBlock *inBlock, DMAPContentCode inCode, const char *inStr )
{
	OSStatus		err;
	size_t			len;
	uint8_t			buf[ 12 ];
		
	WriteBig32( &buf[ 0 ], inCode );
	len = strlen( inStr );
	WriteBig32( &buf[ 4 ], (uint32_t) len );
	
	err = DMAPContentBlock_AddData( inBlock, buf, 8 );
	require_noerr( err, exit );
	
	err = DMAPContentBlock_AddData( inBlock, inStr, len );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddContentCode
//===========================================================================================================================

OSStatus	DMAPContentBlock_AddCodeInfo( DMAPContentBlock *inBlock, DMAPContentCode inCode, const char *inName, uint16_t inType )
{
	OSStatus		err;
	
	err = DMAPContentBlock_OpenContainer( inBlock, kDMAPDictionaryCollectionCode );
	require_noerr( err, exit );
		
		err = DMAPContentBlock_AddInt32( inBlock, kDMAPContentCodesNumberCode, inCode );
		require_noerr( err, exit );
		
		err = DMAPContentBlock_AddUTF8( inBlock, kDMAPContentCodesNameCode, inName );
		require_noerr( err, exit );
		
		err = DMAPContentBlock_AddInt16( inBlock, kDMAPContentCodesTypeCode, inType );
		require_noerr( err, exit );
		
	err = DMAPContentBlock_CloseContainer( inBlock );	
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddPlaceHolderUInt32
//===========================================================================================================================

OSStatus	DMAPContentBlock_AddPlaceHolderUInt32( DMAPContentBlock *inBlock, DMAPContentCode inCode, size_t *outOffset )
{
	OSStatus		err;
	uint8_t			buf[ 8 ];
	size_t			dataOffset;
	
	WriteBig32( &buf[ 0 ], inCode );
	WriteBig32( &buf[ 4 ], 4 );
	
	err = DMAPContentBlock_AddData( inBlock, buf, 8 );
	require_noerr( err, exit );
	
	dataOffset = inBlock->bufferUsed;
	
	WriteBig32( &buf[ 0 ], 0 ); // Placeholder
	
	err = DMAPContentBlock_AddData( inBlock, buf, 4 );
	require_noerr( err, exit );
	
	*outOffset = dataOffset;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_SetPlaceHolder
//===========================================================================================================================

OSStatus	DMAPContentBlock_SetPlaceHolderUInt32( DMAPContentBlock *inBlock, size_t inOffset, uint32_t inValue )
{
	OSStatus		err;
	
	require_action( inBlock->bufferUsed >= ( inOffset + 4 ), exit, err = kInternalErr );
	
	WriteBig32( &inBlock->buffer[ inOffset ], inValue );
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddData
//===========================================================================================================================

OSStatus	DMAPContentBlock_AddData( DMAPContentBlock *inBlock, const void *inData, size_t inSize )
{
	OSStatus		err;
	size_t			newSize;
	uint8_t *		newPtr;
	
	// Don't even try if there's already an error pending since it's not in a good state.
	
	err = inBlock->firstErr;
	require_noerr( err, exit );
	
	// Resize the buffer if it's too small.
	
	newSize = inBlock->bufferUsed + inSize;
	if( newSize > inBlock->bufferSize )
	{
		if( newSize < 2048 ) newSize = 4096;
		else				 newSize *= 2;
		newPtr = (uint8_t *) malloc( newSize );
		require_action( newPtr, exit, err = kNoMemoryErr );
		
		if( inBlock->bufferUsed > 0 )
		{
			memcpy( newPtr, inBlock->buffer, inBlock->bufferUsed );
		}
		if( inBlock->malloced )
		{
			check_string( inBlock->buffer, "malloced flag set, but no buffer?" );
			free( inBlock->buffer );
		}
		inBlock->buffer		= newPtr;
		inBlock->bufferSize	= newSize;
		inBlock->malloced	= 1;
	}
	if( inData ) memcpy( &inBlock->buffer[ inBlock->bufferUsed ], inData, inSize );
	inBlock->bufferUsed += inSize;
	
exit:
	if( err && !inBlock->firstErr ) inBlock->firstErr = err;
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddCFObjectByKey
//===========================================================================================================================

OSStatus
	DMAPContentBlock_AddCFObjectByKey( 
		DMAPContentBlock *	inBlock, 
		DMAPContentCode		inCode, 
		DMAPCodeValueType	inType, 
		CFDictionaryRef		inDict, 
		CFStringRef			inKey )
{
	OSStatus			err;
	CFTypeRef			obj;
	int64_t				s64;
	char *				utf8;
	CFAbsoluteTime		t;
	
	obj = CFDictionaryGetValue( inDict, inKey );
	require_action_quiet( obj, exit, err = kNoErr );
	
	if( inType == kDMAPCodeValueType_Undefined )
	{
		const DMAPContentCodeEntry *		dcce;
		
		dcce = DMAPFindEntryByContentCode( inCode );
		require_action( dcce, exit, err = kTypeErr );
		
		inType = dcce->type;
	}
	
	switch( inType )
	{
		case kDMAPCodeValueType_UInt8:
		case kDMAPCodeValueType_SInt8:
			s64 = CFGetInt64( obj, &err );
			require_noerr( err, exit );
			
			err = DMAPContentBlock_AddInt8( inBlock, inCode, (uint8_t) s64 );
			require_noerr( err, exit );
			break;
		
		case kDMAPCodeValueType_UInt16:
		case kDMAPCodeValueType_SInt16:
			s64 = CFGetInt64( obj, &err );
			require_noerr( err, exit );
			
			err = DMAPContentBlock_AddInt16( inBlock, inCode, (uint16_t) s64 );
			require_noerr( err, exit );
			break;
		
		case kDMAPCodeValueType_UInt32:
		case kDMAPCodeValueType_SInt32:
			s64 = CFGetInt64( obj, &err );
			require_noerr( err, exit );
			
			err = DMAPContentBlock_AddInt32( inBlock, inCode, (uint32_t) s64 );
			require_noerr( err, exit );
			break;
		
		case kDMAPCodeValueType_UInt64:
		case kDMAPCodeValueType_SInt64:
			s64 = CFGetInt64( obj, &err );
			require_noerr( err, exit );
			
			err = DMAPContentBlock_AddInt64( inBlock, inCode, (uint64_t) s64 );
			require_noerr( err, exit );
			break;
		
		case kDMAPCodeValueType_UTF8Characters:
			utf8 = CFCopyCString( obj, &err );
			require_noerr( err, exit );
			
			err = DMAPContentBlock_AddUTF8( inBlock, inCode, utf8 );
			free( utf8 );
			require_noerr( err, exit );
			break;
		
		case kDMAPCodeValueType_Date:
			require_action( CFGetTypeID( obj ) == CFDateGetTypeID(), exit, err = kTypeErr );
			
			t = kCFAbsoluteTimeIntervalSince1970 + CFDateGetAbsoluteTime( (CFDateRef) obj );
			err = DMAPContentBlock_AddInt32( inBlock, inCode, (uint32_t) t );
			require_noerr( err, exit );
			break;
		
		default:
			dlogassert( "unsupported type: %d", inType );
			err = kUnsupportedErr;
			goto exit;
	}
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_AddHexCFStringByKey
//===========================================================================================================================

OSStatus
	DMAPContentBlock_AddHexCFStringByKey( 
		DMAPContentBlock *	inBlock, 
		DMAPContentCode		inCode, 
		DMAPCodeValueType	inType, 
		CFDictionaryRef		inDict, 
		CFStringRef			inKey )
{
	OSStatus		err;
	CFStringRef		cfStr;
	Boolean			good;
	char			str[ 64 ];
	uint64_t		u64;
	int				n;
	
	cfStr = (CFStringRef) CFDictionaryGetValue( inDict, inKey );
	require_action_quiet( cfStr, exit, err = kNoErr );
	require_action( CFGetTypeID( cfStr ) == CFStringGetTypeID(), exit, err = kTypeErr );
	
	good = CFStringGetCString( cfStr, str, (CFIndex) sizeof( str ), kCFStringEncodingUTF8 );
	require_action( good, exit, err = kUnknownErr );
	
	n = SNScanF( str, kSizeCString, "%llx", &u64 );
	require_action( n == 1, exit, err = kValueErr );
	
	if( inType == kDMAPCodeValueType_Undefined )
	{
		const DMAPContentCodeEntry *		dcce;
		
		dcce = DMAPFindEntryByContentCode( inCode );
		require_action( dcce, exit, err = kTypeErr );
		
		inType = dcce->type;
	}
	
	switch( inType )
	{
		case kDMAPCodeValueType_UInt8:
		case kDMAPCodeValueType_SInt8:
			err = DMAPContentBlock_AddInt8( inBlock, inCode, (uint8_t) u64 );
			require_noerr( err, exit );
			break;
		
		case kDMAPCodeValueType_UInt16:
		case kDMAPCodeValueType_SInt16:
			err = DMAPContentBlock_AddInt16( inBlock, inCode, (uint16_t) u64 );
			require_noerr( err, exit );
			break;
		
		case kDMAPCodeValueType_UInt32:
		case kDMAPCodeValueType_SInt32:
			err = DMAPContentBlock_AddInt32( inBlock, inCode, (uint32_t) u64 );
			require_noerr( err, exit );
			break;
		
		case kDMAPCodeValueType_UInt64:
		case kDMAPCodeValueType_SInt64:
			err = DMAPContentBlock_AddInt64( inBlock, inCode, u64 );
			require_noerr( err, exit );
			break;
		
		default:
			dlogassert( "unsupported type: %d", inType );
			err = kUnsupportedErr;
			goto exit;
	}
	
exit:
	return( err );
}

//===========================================================================================================================
//	DMAPContentBlock_GetNextChunk
//===========================================================================================================================

OSStatus
	DMAPContentBlock_GetNextChunk( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		DMAPContentCode *	outCode, 
		size_t *			outSize, 
		const uint8_t **	outData, 
		const uint8_t **	outSrc )
{
	OSStatus			err;
	DMAPContentCode		code;
	uint32_t			size;
	
	require_action_quiet( inSrc < inEnd, exit, err = kNotFoundErr );
	require_action( ( inEnd - inSrc ) >= 8, exit, err = kUnderrunErr );
	code = ReadBig32( inSrc ); inSrc += 4;
	size = ReadBig32( inSrc ); inSrc += 4;
	require_action( ( (size_t)( inEnd - inSrc ) ) >= size, exit, err = kUnderrunErr );
	
	*outCode = code;
	*outSize = size;
	*outData = inSrc;
	if( outSrc ) *outSrc  = inSrc + size;
	err = kNoErr;
	
exit:
	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	DMAPCreateCFObjectFromData
//===========================================================================================================================

DEBUG_STATIC OSStatus
	__DMAPCreateCFObjectFromDataLevel( 
		CFMutableArrayRef		inParentArray,
		CFMutableDictionaryRef	inParentDict,  
		const uint8_t **		ioSrc, 
		const uint8_t *			inEnd, 
		CFTypeRef *				outObj );

OSStatus	DMAPCreateCFObjectFromData( const void *inData, size_t inSize, CFTypeRef *outObj )
{
	OSStatus			err;
	const uint8_t *		src;
	const uint8_t *		end;
	
	src = (const uint8_t *) inData;
	end = src + inSize;
	err = __DMAPCreateCFObjectFromDataLevel( NULL, NULL, &src, end, outObj );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//
//	__DMAPCreateCFObjectFromDataLevel
//
DEBUG_STATIC OSStatus
	__DMAPCreateCFObjectFromDataLevel( 
		CFMutableArrayRef		inParentArray,
		CFMutableDictionaryRef	inParentDict,  
		const uint8_t **		ioSrc, 
		const uint8_t *			inEnd, 
		CFTypeRef *				outObj )
{
	OSStatus							err;
	const uint8_t *						src;
	const uint8_t *						next;
	uint32_t							code;
	uint32_t							size;
	const DMAPContentCodeEntry *		entry;
	CFTypeRef							obj;
	
	obj = NULL;
	
	src = *ioSrc;
	for( ; ( inEnd - src ) >= 8; src = next )
	{
		code = ReadBig32( src ); src += 4;
		size = ReadBig32( src ); src += 4;
		next = src + size;
		if( ( next < src ) || ( next > inEnd ) )
		{
			dlogassert( "### bad size %u with %tu remaining", size, inEnd - src );
			err = kOverrunErr;
			goto exit;
		}
		
		entry = DMAPFindEntryByContentCode( code );
		if( !entry )
		{
			dlog( kDebugLevelNotice, "### skipping unknown DMAP code '%C'\n", code );
			continue;
		}
		
		if( entry->type == kDMAPCodeValueType_DictionaryHeader )
		{
			obj = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
			require_action( obj, exit, err = kNoMemoryErr );
			
			err = __DMAPCreateCFObjectFromDataLevel( NULL, (CFMutableDictionaryRef) obj, &src, next, NULL );
			require_noerr( err, exit );
			if( src != next ) dlogassert( "### DMAP sub-item end gap of %tu\n", next - src );
		}
		else if( entry->type == kDMAPCodeValueType_ArrayHeader )
		{
			obj = CFArrayCreateMutable( NULL, 0, &kCFTypeArrayCallBacks );
			require_action( obj, exit, err = kNoMemoryErr );
			
			err = __DMAPCreateCFObjectFromDataLevel( (CFMutableArrayRef) obj, NULL, &src, next, NULL );
			require_noerr( err, exit );
			if( src != next ) dlogassert( "### DMAP sub-item end gap of %tu\n", next - src );
		}
		else
		{
			err = DMAPCreateSimpleCFObject( entry->type, src, size, &obj );
			require_noerr( err, exit );
		}
		
		if( inParentArray )
		{
			CFArrayAppendValue( inParentArray, obj );
			CFRelease( obj );
			obj = NULL;
		}
		else if( inParentDict )
		{
			CFStringRef		nameCF;
			
			nameCF = CFStringCreateWithCString( NULL, entry->name, kCFStringEncodingUTF8 );
			require_action( nameCF, exit, err = kNoMemoryErr );
			
			CFDictionarySetValue( inParentDict, nameCF, obj );
			CFRelease( nameCF );
			CFRelease( obj );
			obj = NULL;
		}
		else
		{
			src = next;
			
			check( outObj );
			if( outObj )
			{
				*outObj = obj;
				obj = NULL;
			}
			break;
		}
	}
	if( src != inEnd ) dlogassert( "### DMAP data truncated (%tu remaining)\n", inEnd - src );
	err = kNoErr;
	
exit:
	if( obj ) CFRelease( obj );
	*ioSrc = src;
	return( err );
}

//===========================================================================================================================
//	DMAPCreateSimpleCFObject
//===========================================================================================================================

OSStatus	DMAPCreateSimpleCFObject( DMAPCodeValueType inType, const void *inData, size_t inSize, CFTypeRef *outObj )
{
	OSStatus		err;
	CFTypeRef		obj;
	int16_t			s16;
	int32_t			s32;
	int64_t			s64;
	
	switch( inType )
	{
		case kDMAPCodeValueType_UInt8:
			require_action( inSize == 1, exit, err = kSizeErr );
			
			s16 = Read8( inData );
			obj = CFNumberCreate( NULL, kCFNumberSInt16Type, &s16 );
			require_action( obj, exit, err = kUnknownErr );
			break;
		
		case kDMAPCodeValueType_SInt8:
			require_action( inSize == 1, exit, err = kSizeErr );
			
			obj = CFNumberCreate( NULL, kCFNumberSInt8Type, inData );
			require_action( obj, exit, err = kUnknownErr );
			break;
		
		case kDMAPCodeValueType_UInt16:
			require_action( inSize == 2, exit, err = kSizeErr );
			
			s32 = ReadBig16( inData );
			obj = CFNumberCreate( NULL, kCFNumberSInt32Type, &s32 );
			require_action( obj, exit, err = kUnknownErr );
			break;
		
		case kDMAPCodeValueType_SInt16:
			require_action( inSize == 2, exit, err = kSizeErr );
			
			s16 = ReadBig16( inData );
			obj = CFNumberCreate( NULL, kCFNumberSInt16Type, &s16 );
			require_action( obj, exit, err = kUnknownErr );
			break;
		
		case kDMAPCodeValueType_UInt32:
			require_action( inSize == 4, exit, err = kSizeErr );
			
			s64 = ReadBig32( inData );
			obj = CFNumberCreate( NULL, kCFNumberSInt64Type, &s64 );
			require_action( obj, exit, err = kUnknownErr );
			break;
		
		case kDMAPCodeValueType_SInt32:
			require_action( inSize == 4, exit, err = kSizeErr );
			
			s32 = ReadBig32( inData );
			obj = CFNumberCreate( NULL, kCFNumberSInt32Type, &s32 );
			require_action( obj, exit, err = kUnknownErr );
			break;
		
		case kDMAPCodeValueType_UInt64:
			require_action( inSize == 8, exit, err = kSizeErr );
			
			s64 = ReadBig64( inData );
			obj = CFNumberCreate( NULL, kCFNumberSInt64Type, &s64 );
			require_action( obj, exit, err = kUnknownErr );
			break;
		
		case kDMAPCodeValueType_SInt64:
			require_action( inSize == 8, exit, err = kSizeErr );
			
			s64 = ReadBig64( inData );
			obj = CFNumberCreate( NULL, kCFNumberSInt64Type, &s64 );
			require_action( obj, exit, err = kUnknownErr );
			break;
		
		case kDMAPCodeValueType_UTF8Characters:
			obj = CFStringCreateWithBytes( NULL, (const uint8_t *) inData, (CFIndex) inSize, kCFStringEncodingUTF8, false );
			require_action( obj, exit, err = kUnknownErr );
			break;
	
		case kDMAPCodeValueType_Version:
			require_action( inSize == 4, exit, err = kSizeErr );
			
			s64 = ReadBig32( inData );
			obj = CFNumberCreate( NULL, kCFNumberSInt64Type, &s64 );
			require_action( obj, exit, err = kUnknownErr );
			break;
		
		default:
			dlogassert( "unsupported type: %d", inType );
			err = kUnsupportedErr;
			goto exit;
	}
	
	*outObj = obj;
	err = kNoErr;
	
exit:
	return( err );
}

#if 0
#pragma mark -
#pragma mark == Debugging ==
#endif

#if( DEBUG )
//===========================================================================================================================
//	DMAPContentBlock_Dump
//===========================================================================================================================

#define kDumpLogLevel		( kLogLevelMax | kLogLevelFlagContinuation )

#define	print_indent( N )	dlog( kDumpLogLevel, "%*s", (int)( (N) * 4 ), "" );

static OSStatus	DMAPContentBlock_DumpChunk( int inIndent, int inCompact, const uint8_t **ioPtr, const uint8_t *inEnd );

OSStatus	DMAPContentBlock_Dump( int inIndent, const void *inData, size_t inSize )
{
	OSStatus			err;
	const uint8_t *		ptr;
	const uint8_t *		end;
	
	ptr = (const uint8_t *) inData;
	end = ptr + inSize;
	while( ptr < end )
	{
		err = DMAPContentBlock_DumpChunk( inIndent, inIndent, &ptr, end );
		require_noerr( err, exit );
	}
	check( ptr == end );
	err = kNoErr;
	
exit:
	return( err );
}

static OSStatus	DMAPContentBlock_DumpChunk( int inIndent, int inCompact, const uint8_t **ioPtr, const uint8_t *inEnd )
{
	OSStatus							err;
	const uint8_t *						ptr;
	const uint8_t *						end;
	DMAPContentCode						code;
	uint32_t							size;
	const DMAPContentCodeEntry *		e;
	const char *						codeSymbol;
	const char *						name;
	DMAPCodeValueType					type;
	
	// Parse the header.
	
	ptr = *ioPtr;
	require_action( ( (size_t)( inEnd - ptr ) ) >= 8, exit, err = kUnderrunErr );
	code = ReadBig32( &ptr[ 0 ] );
	size = ReadBig32( &ptr[ 4 ] );
	ptr += 8;
	end = ptr + size;
	require_action( ( end >= ptr ) && ( end <= inEnd ), exit, err = kOverrunErr );
	
	// Print the header.
	
	e = DMAPFindEntryByContentCode( code );
	if( e )
	{
		codeSymbol	= e->codeSymbol;
		name		= e->name;
		type		= e->type;
	}
	else
	{
		codeSymbol	= "<unknown>";
		name		= "";
		type		= kDMAPCodeValueType_Undefined;
	}
	if( inCompact )
	{
		print_indent( inIndent );
		dlog( kDumpLogLevel, "'%C' %-40s %-40s %4u bytes: ", code, codeSymbol, name, size );
	}
	else
	{
		print_indent( inIndent );
		dlog( kDumpLogLevel, "{\n" );
		++inIndent;
		
		print_indent( inIndent );
		dlog( kDumpLogLevel, "'%C' %s \"%s\"\n", code, codeSymbol, name );
		
		print_indent( inIndent );
		dlog( kDumpLogLevel, "%u bytes\n", size );
	}
	
	// Print the value.
	
	if( !inCompact && ( type != kDMAPCodeValueType_ArrayHeader ) && ( type != kDMAPCodeValueType_DictionaryHeader ) )
	{
		print_indent( inIndent );
	}
	switch( type )
	{
		case kDMAPCodeValueType_UInt8:
		{
			uint8_t		x;
			
			if( size != 1 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 1 byte\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			
			x = *ptr;
			dlog( kDumpLogLevel, "%u\n", x );
			break;
		}
		
		case kDMAPCodeValueType_SInt8:
		{
			int8_t		x;
			
			if( size != 1 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 1 byte\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			x = (int8_t) *ptr;
			dlog( kDumpLogLevel, "%d\n", x );
			break;
		}
		
		case kDMAPCodeValueType_UInt16:
		{
			int16_t		x;
			
			if( size != 2 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 2 bytes\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			x = (int16_t) ReadBig16( ptr );
			dlog( kDumpLogLevel, "%u\n", x );
			break;
		}
		
		case kDMAPCodeValueType_SInt16:
		{
			uint16_t		x;
			
			if( size != 2 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 2 bytes\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			x = ReadBig16( ptr );
			dlog( kDumpLogLevel, "%d\n", x );
			break;
		}
		
		case kDMAPCodeValueType_UInt32:
		{
			uint32_t		x;
			
			if( size != 4 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 4 bytes\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			x = ReadBig32( ptr );
			dlog( kDumpLogLevel, "%u\n", x );
			break;
		}
		
		case kDMAPCodeValueType_SInt32:
		{
			int32_t		x;
			
			if( size != 4 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 4 bytes\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			x = (int32_t) ReadBig32( ptr );
			dlog( kDumpLogLevel, "%d\n", x );
			break;
		}
		
		case kDMAPCodeValueType_UInt64:
		{
			uint64_t		x;
			
			if( size != 8 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 8 bytes\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			x = ReadBig64( ptr );
			dlog( kDumpLogLevel, "%llu\n", (unsigned long long) x );
			break;
		}
		
		case kDMAPCodeValueType_SInt64:
		{
			int64_t		x;
			
			if( size != 8 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 8 bytes\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			x = (int64_t) ReadBig64( ptr );
			dlog( kDumpLogLevel, "%d\n", (long long) x );
			break;
		}
		
		case kDMAPCodeValueType_UTF8Characters:
			dlog( kDumpLogLevel, "\"%.*s\"\n", (int) size, ptr );
			break;
		
		case kDMAPCodeValueType_Date:
		{
			time_t			t;
			struct tm *		dateTM;
			char			dateStr[ 64 ];
			
			if( size != 4 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 4 bytes\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			t = (time_t) ReadBig32( ptr );
			dateTM = gmtime( &t );
			dateStr[ 0 ] = '\0';
			strftime( dateStr, sizeof( dateStr ), "%Y-%m-%d %H:%M:%S UTC", dateTM );
			dlog( kDumpLogLevel, "%s\n", dateStr );
			break;
		}
		
		case kDMAPCodeValueType_Version:
		{
			uint32_t		version;
			
			if( size != 4 )
			{
				dlog( kLogLevelWarning, "### type %d value %u bytes instead of 4 bytes\n", type, size );
				err = kSizeErr;
				goto exit;
			}
			version = ReadBig32( ptr );
			dlog( kDumpLogLevel, "%u.%u\n", ( version >> 16 ), ( version & 0xFFFF ) );
			break;
		}
		
		case kDMAPCodeValueType_ArrayHeader:
		case kDMAPCodeValueType_DictionaryHeader:
			if( inCompact )
			{
				dlog( kDumpLogLevel, "\n" );
			}
			else
			{
				print_indent( inIndent );
				dlog( kDumpLogLevel, "{\n" );
			}
			while( ptr < end )
			{
				++inIndent;
				err = DMAPContentBlock_DumpChunk( inIndent, inCompact, &ptr, end );
				--inIndent;
				require_noerr( err, exit );
			}
			if( !inCompact )
			{
				print_indent( inIndent );
				dlog( kDumpLogLevel, "}\n" );
			}
			break;
		
		default:
			dlog( kLogLevelWarning, "### unknown value type: %d\n", type );
			break;
	}
	if( ( type != kDMAPCodeValueType_ArrayHeader ) && ( type != kDMAPCodeValueType_DictionaryHeader ) )
	{
		ptr += size;
	}
	
	// Print the trailer.
	
	if( !inCompact )
	{
		--inIndent;
		print_indent( inIndent );
		dlog( kDumpLogLevel, "}\n" );
	}
	err = kNoErr;
	
exit:
	*ioPtr = ptr;
	return( err );
}
#endif	// DEBUG

