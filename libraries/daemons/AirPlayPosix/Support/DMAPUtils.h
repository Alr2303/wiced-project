/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/

#ifndef	__DMAPUtils_h__
#define	__DMAPUtils_h__

#include <stddef.h>

#include "CommonServices.h"
#include "DebugServices.h"
#include "DMAP.h"

#include CF_HEADER

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		DMAPContentBlockBuilding
	@abstract	Builds DMAP content blocks.
	@discussion
	
	To build a DMAPContentBlock, you call DMAPContentBlock_Init on it, make a series of add calls, and then use 
	DMAPContentBlock_Commit to do any final preparation. DMAPContentBlock's are often built using a ton of individual 
	add calls. To simplify this, while still providing full error checking, the DMAPContentBlock tracks the first error 
	that occurred and prevents any further operations after that. When DMAPContentBlock_Commit is called, it checks
	that no errors have occurred as well as other things like making sure any open containers have been closed. This 
	allows code to ignore errors from all the intermediate add calls and simply check the final error from 
	DMAPContentBlock_Commit. For example, the following code builds a DAAP server-info response. Also note the style
	convention of indenting adds within containers. This makes it easier to see the overall structure:
	
	OSStatus				err;
	uint8_t					buf[ 256 ];
	DMAPContentBlock		block;
	const uint8_t *			ptr;
	size_t					size;
	
	DMAPContentBlock_Init( &block, buf, sizeof( buf ) );
	
	DMAPContentBlock_OpenContainer( &block, kDMAPServerInfoResponseCode );
		DMAPContentBlock_AddInt32( &block, kDMAPStatusCode, 200 );
		DMAPContentBlock_AddInt16( &block, kDAAPSongDiscCountCode, 5 );
		DMAPContentBlock_AddInt32( &block, kDMAPItemIDCode, 12345678 );
		DMAPContentBlock_AddInt8(  &block, kDACPPlayerStateCode, kDACPPlayerState_Playing );
	DMAPContentBlock_CloseContainer( &block );
	
	err = DMAPContentBlock_Commit( &block, &ptr, &size );
	require_noerr( err, exit );
	
	err = DMAPContentBlock_Dump( 0, ptr, size );
	require_noerr( err, exit );
	
	There are cases where a DMAP content block includes an item, such as a count, that relies information built after
	that item. Since the code wouldn't know the value at the time the item is added, a placeholder can be added and 
	then when the value is known, the placeholder can be updated with the real value. The following code demonstrates
	using the placeholder APIs to do this to build a fictitious database-containers response:
	
	size_t			totalCountOffset;
	size_t			returnedCountOffset;
	uint32_t		count;
	
	DMAPContentBlock_OpenContainer( &block, kDAAPDatabasePlaylistsCode );
		DMAPContentBlock_AddInt32( &block, kDMAPStatusCode, 200 );
		DMAPContentBlock_AddPlaceHolderUInt32( &block, kDMAPSpecifiedTotalCountCode, &totalCountOffset );
		DMAPContentBlock_AddPlaceHolderUInt32( &block, kDMAPReturnedCountCode, &returnedCountOffset );
		
		DMAPContentBlock_OpenContainer( &block, kDMAPListingCollectionCode );
			count = 0;
			
			DMAPContentBlock_OpenContainer( &block, kDMAPListingItemCode );
				DMAPContentBlock_AddInt32( cb, kDMAPItemIDCode, count++ );
			DMAPContentBlock_CloseContainer( &block );
			
			DMAPContentBlock_OpenContainer( &block, kDMAPListingItemCode );
				DMAPContentBlock_AddInt32( cb, kDMAPItemIDCode, count++ );
			DMAPContentBlock_CloseContainer( &block );
		DMAPContentBlock_CloseContainer( &block );
		
		DMAPContentBlock_SetPlaceHolderUInt32( &block, totalCountOffset, count );
		DMAPContentBlock_SetPlaceHolderUInt32( &block, returnedCountOffset, count );
	DMAPContentBlock_CloseContainer( contentBlock );
*/

typedef struct
{
	uint8_t *		staticBuffer;			//! Optional static buffer to use if the content will fit. May be NULL.
	size_t			staticBufferSize;		//! Number of bytes the static buffer can hold.
	uint8_t *		buffer;					//! May be malloc'd or a static buffer.
	size_t			bufferSize;				//! Number of bytes buffer can hold.
	size_t			bufferUsed;				//! Number of bytes in use.
	uint8_t			malloced;				//! Non-zero if buffer was malloc'd. 0 if static or NULL.
	size_t			containerOffsets[ 16 ];	//! Stack of offsets to the size field of each nested container.
	uint32_t		containerLevel;			//! Current index into container stack. 0 if not in a container.
	OSStatus		firstErr;				//! First error that occurred or kNoErr.
	
}	DMAPContentBlock;

void		DMAPContentBlock_Init( DMAPContentBlock *inBlock, void *inStaticBuffer, size_t inStaticBufferSize );
void		DMAPContentBlock_Free( DMAPContentBlock *inBlock );
OSStatus	DMAPContentBlock_Commit( DMAPContentBlock *inBlock, const uint8_t **outPtr, size_t *outSize );
#define		DMAPContentBlock_GetError( BLOCK )	( BLOCK )->firstErr

OSStatus	DMAPContentBlock_OpenContainer( DMAPContentBlock *inBlock, DMAPContentCode inCode );
OSStatus	DMAPContentBlock_CloseContainer( DMAPContentBlock *inBlock );

OSStatus	DMAPContentBlock_AddInt8( DMAPContentBlock *inBlock, DMAPContentCode inCode, uint8_t inValue );
OSStatus	DMAPContentBlock_AddInt16( DMAPContentBlock *inBlock, DMAPContentCode inCode, uint16_t inValue );
OSStatus	DMAPContentBlock_AddInt32( DMAPContentBlock *inBlock, DMAPContentCode inCode, uint32_t inValue );
OSStatus	DMAPContentBlock_AddInt64( DMAPContentBlock *inBlock, DMAPContentCode inCode, uint64_t inValue );
OSStatus	DMAPContentBlock_AddUTF8( DMAPContentBlock *inBlock, DMAPContentCode inCode, const char *inStr );
#define		DMAPContentBlock_AddUTF8If( BLOCK, CODE, STR ) do { if( STR ) DMAPContentBlock_AddUTF8( ( BLOCK ), ( CODE ), ( STR ) ); } while( 0 )

OSStatus	DMAPContentBlock_AddCodeInfo( DMAPContentBlock *inBlock, DMAPContentCode inCode, const char *inName, uint16_t inType );
OSStatus	DMAPContentBlock_AddPlaceHolderUInt32( DMAPContentBlock *inBlock, DMAPContentCode inCode, size_t *outOffset );
OSStatus	DMAPContentBlock_SetPlaceHolderUInt32( DMAPContentBlock *inBlock, size_t inOffset, uint32_t inValue );
OSStatus	DMAPContentBlock_AddData( DMAPContentBlock *inBlock, const void *inData, size_t inSize );

OSStatus
	DMAPContentBlock_AddHexCFStringByKey( 
		DMAPContentBlock *	inBlock, 
		DMAPContentCode		inCode, 
		DMAPCodeValueType	inType, 
		CFDictionaryRef		inDict, 
		CFStringRef			inKey );
OSStatus
	DMAPContentBlock_AddCFObjectByKey( 
		DMAPContentBlock *	inBlock, 
		DMAPContentCode		inCode, 
		DMAPCodeValueType	inType, 
		CFDictionaryRef		inDict, 
		CFStringRef			inKey );

//! @endgroup

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		DMAPContentBlockParsing
	@abstract	Parses DMAP content blocks.
	@discussion
	
	To parse a DMAPContentBlock, you call DMAPContentBlock_GetNextChunk in a loop until it returns an error. For example:
	
	OSStatus			err;
	const uint8_t *		src;
	const uint8_t *		end;
	DMAPContentCode		code;
	size_t				size;
	const uint8_t *		data;
	
	src = <ptr to content block>
	end = src + <size of content block>
	
	while( DMAPContentBlock_GetNextChunk( src, end, &code, &size, &data, &src ) == kNoErr )
	{
		dlog( kDebugLevelNotice, "'%C' (%u bytes): %H\n", code, size, data, size, 32 );
	}
*/

OSStatus
	DMAPContentBlock_GetNextChunk( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		DMAPContentCode *	outCode, 
		size_t *			outSize, 
		const uint8_t **	outData, 
		const uint8_t **	outSrc );

#if( DEBUG )
	OSStatus	DMAPContentBlock_Dump( int inIndent, const void *inData, size_t inSize );
	
	#define dmap_dump( INDENT, PTR, SIZE )		DMAPContentBlock_Dump( (INDENT), (PTR), (SIZE) )
#else
	#define dmap_dump( INDENT, PTR, SIZE )
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	DMAPCreateCFObjectFromData / DMAPCreateSimpleCFObject
	@abstract	Creates a CF object out of DMAP data.
*/

OSStatus	DMAPCreateCFObjectFromData( const void *inData, size_t inSize, CFTypeRef *outObj );
OSStatus	DMAPCreateSimpleCFObject( DMAPCodeValueType inType, const void *inData, size_t inSize, CFTypeRef *outObj );

//! @endgroup

#ifdef __cplusplus
}
#endif

#endif // __DMAPUtils_h__
