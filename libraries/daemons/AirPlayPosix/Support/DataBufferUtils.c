/*
	Copyright (C) 2008-2013 Apple Inc. All Rights Reserved.
*/

#include "CommonServices.h"
#include "DebugServices.h"

	#include <errno.h>
	#include <limits.h>
	#include <stddef.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

#include "DataBufferUtils.h"

#define	kDataBufferFileBufferSize		( 32 * 1024 )

//===========================================================================================================================
//	DataBuffer_Init
//===========================================================================================================================

void	DataBuffer_Init( DataBuffer *inDB, void *inStaticBufferPtr, size_t inStaticBufferLen, size_t inMaxGrowLen )
{
	inDB->staticBufferPtr	= (uint8_t *) inStaticBufferPtr;
	inDB->staticBufferLen	= inStaticBufferLen;
	inDB->maxGrowLen		= inMaxGrowLen;
	inDB->bufferPtr			= (uint8_t *) inStaticBufferPtr;
	inDB->bufferLen			= 0;
	inDB->bufferMaxLen		= inStaticBufferLen;
	inDB->malloced			= 0;
	inDB->firstErr			= kNoErr;
}

//===========================================================================================================================
//	DataBuffer_Free
//===========================================================================================================================

void	DataBuffer_Free( DataBuffer *inDB )
{
	if( inDB->malloced && inDB->bufferPtr )
	{
		free_compat( inDB->bufferPtr );
	}
	inDB->bufferPtr		= inDB->staticBufferPtr;
	inDB->bufferLen		= 0;
	inDB->bufferMaxLen	= inDB->staticBufferLen;
	inDB->malloced		= 0;
	inDB->firstErr		= kNoErr;
}

//===========================================================================================================================
//	DataBuffer_Disown
//===========================================================================================================================

uint8_t *	DataBuffer_Disown( DataBuffer *inDB )
{
	uint8_t *		buf;
	
	buf = DataBuffer_GetPtr( inDB );
	inDB->bufferPtr = NULL;
	inDB->malloced  = 0;
	DataBuffer_Free( inDB );
	return( buf );
}

//===========================================================================================================================
//	DataBuffer_Commit
//===========================================================================================================================

OSStatus	DataBuffer_Commit( DataBuffer *inDB, uint8_t **outPtr, size_t *outLen )
{
	OSStatus		err;
	
	err = inDB->firstErr;
	require_noerr_string( err, exit, "earlier error occurred" );
	
	inDB->firstErr = kAlreadyInUseErr; // Mark in-use to prevent further changes to it until DataBuffer_Free.
	
	if( outPtr ) *outPtr = DataBuffer_GetPtr( inDB );
	if( outLen ) *outLen = DataBuffer_GetLen( inDB );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DataBuffer_Detach
//===========================================================================================================================

OSStatus	DataBuffer_Detach( DataBuffer *inDB, uint8_t **outPtr, size_t *outLen )
{
	OSStatus		err;
	uint8_t *		ptr;
	size_t			len;
	
	len = inDB->bufferLen;
	if( inDB->malloced )
	{
		check( inDB->bufferPtr || ( len == 0 ) );
		ptr = inDB->bufferPtr;
	}
	else
	{
		ptr = (uint8_t *) malloc_compat( ( len > 0 ) ? len : 1 ); // malloc( 0 ) is undefined so use at least 1.
		require_action( ptr, exit, err = kNoMemoryErr );
		
		if( len > 0 ) memcpy( ptr, inDB->bufferPtr, len );
	}
	inDB->bufferPtr		= inDB->staticBufferPtr;
	inDB->bufferLen		= 0;
	inDB->bufferMaxLen	= inDB->staticBufferLen;
	inDB->malloced		= 0;
	inDB->firstErr		= kNoErr;
	
	*outPtr = ptr;
	*outLen = len;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DataBuffer_DetachCString
//===========================================================================================================================

OSStatus	DataBuffer_DetachCString( DataBuffer *inDB, char **outStr )
{
	OSStatus		err;
	uint8_t *		ptr;
	size_t			len;
	
	err = DataBuffer_Append( inDB, "", 1 );
	require_noerr( err, exit );
	
	ptr = NULL;
	err = DataBuffer_Detach( inDB, &ptr, &len );
	require_noerr( err, exit );
	
	*outStr = (char *) ptr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DataBuffer_Replace
//===========================================================================================================================

OSStatus	DataBuffer_Replace( DataBuffer *inDB, size_t inOffset, size_t inOldLen, const void *inNewData, size_t inNewLen )
{
	OSStatus		err;
	size_t			endOffset;
	size_t			oldLen;
	
	err = inDB->firstErr;
	require_noerr( err, exit2 );
	
	if( inNewLen == kSizeCString ) inNewLen = strlen( (const char *) inNewData );
	if( inNewData ) check_ptr_overlap( inDB->bufferPtr, inDB->bufferLen, inNewData, inNewLen );
	
	if( inOffset > inDB->bufferLen )
	{
		endOffset = inOffset + inNewLen;
		require_action( inOldLen == 0, exit, err = kSizeErr );
		require_action( endOffset >= inOffset, exit, err = kSizeErr );
		
		err = DataBuffer_Resize( inDB, endOffset, NULL );
		require_noerr( err, exit2 );
	}
	else
	{
		endOffset = inOffset + inOldLen;
		require_action( endOffset >= inOffset, exit, err = kSizeErr );
		require_action( endOffset <= inDB->bufferLen, exit, err = kRangeErr );
		
		// Shift any data after the data being replaced to make room (growing) or take up slack (shrinking).
		
		oldLen = inDB->bufferLen;
		if( inNewLen > inOldLen )
		{
			err = DataBuffer_Grow( inDB, inNewLen - inOldLen, NULL );
			require_noerr( err, exit2 );
		}
		if( oldLen != endOffset )
		{
			memmove( inDB->bufferPtr + inOffset + inNewLen, inDB->bufferPtr + endOffset, oldLen - endOffset );
		}
		if( inNewLen < inOldLen )
		{
			inDB->bufferLen -= inOldLen - inNewLen;
		}
	}
	
	// Copy in any new data.
	
	if( inNewData )
	{
		memmove( inDB->bufferPtr + inOffset, inNewData, inNewLen );
	}
	goto exit2;
	
exit:
	inDB->firstErr = err;
	
exit2:
	return( err );
}

//===========================================================================================================================
//	DataBuffer_Resize
//===========================================================================================================================

OSStatus	DataBuffer_Resize( DataBuffer *inDB, size_t inNewLen, void *outPtr )
{
	OSStatus		err;
	size_t			oldLen;
	size_t			newMaxLen;
	uint8_t *		newPtr;
		
	err = inDB->firstErr;
	require_noerr( err, exit2 );
	
	// Grow or shink as needed. If growing larger than the max size of the current buffer, reallocate.
	
	oldLen = inDB->bufferLen;
	if( inNewLen > oldLen )
	{
		if( inNewLen > inDB->bufferMaxLen )
		{
			require_action( inNewLen <= inDB->maxGrowLen, exit, err = kOverrunErr );
			
			if(      inNewLen <    256 ) newMaxLen = 256;
			else if( inNewLen <   4096 ) newMaxLen = 4096;
			else if( inNewLen < 131072 ) newMaxLen = AlignUp( inNewLen, 16384 );
			else						 newMaxLen = AlignUp( inNewLen, 131072 );
			
			newPtr = (uint8_t *) malloc_compat( newMaxLen );
			require_action( newPtr, exit, err = kNoMemoryErr );
			
			if( inDB->bufferLen > 0 )
			{
				memcpy( newPtr, inDB->bufferPtr, inDB->bufferLen );
			}
			if( inDB->malloced && inDB->bufferPtr )
			{
				free_compat( inDB->bufferPtr );
			}
			inDB->bufferMaxLen	= newMaxLen;
			inDB->bufferPtr		= newPtr;
			inDB->malloced		= 1;
		}
		inDB->bufferLen = inNewLen;
		if( outPtr ) *( (void **) outPtr ) = inDB->bufferPtr + oldLen;
	}
	else
	{
		// $$$ TO DO: Consider shrinking the buffer if it's shrinking by a large amount.
		
		inDB->bufferLen = inNewLen;
		if( outPtr ) *( (void **) outPtr ) = inDB->bufferPtr;
	}
	goto exit2;
	
exit:
	inDB->firstErr = err;
	
exit2:
	return( err );
}

//===========================================================================================================================
//	DataBuffer_Shrink
//===========================================================================================================================

OSStatus	DataBuffer_Shrink( DataBuffer *inDB, size_t inAmount )
{
	OSStatus		err;
		
	err = inDB->firstErr;
	require_noerr( err, exit2 );
	
	require_action( inAmount <= inDB->bufferLen, exit, err = kSizeErr );
	inDB->bufferLen -= inAmount;
	goto exit2;
	
exit:
	inDB->firstErr = err;
	
exit2:
	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	DataBuffer_Append
//===========================================================================================================================

OSStatus	DataBuffer_Append( DataBuffer *inDB, const void *inData, size_t inLen )
{
	OSStatus		err;
	uint8_t *		ptr;
	
	if( inLen == kSizeCString ) inLen = strlen( (const char *) inData );
	err = DataBuffer_Grow( inDB, inLen, &ptr );
	require_noerr( err, exit );
	
	memcpy( ptr, inData, inLen );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DataBuffer_AppendF
//===========================================================================================================================

static int	__DataBuffer_PrintFCallBack( const char *inStr, size_t inLen, void *inContext );

OSStatus	DataBuffer_AppendF( DataBuffer *inDB, const char *inFormat, ... )
{
	OSStatus		err;
	va_list			args;
	
	va_start( args, inFormat );
	err = DataBuffer_AppendFVAList( inDB, inFormat, args );
	va_end( args );
	return( err );
}

OSStatus	DataBuffer_AppendFNested( DataBuffer *inDB, const char *inTemplate, const char *inFormat, ... )
{
	OSStatus		err;
	va_list			args;
	
	va_start( args, inFormat );
	err = DataBuffer_AppendF( inDB, inTemplate, inFormat, &args );
	va_end( args );
	return( err );
}

OSStatus	DataBuffer_AppendFVAList( DataBuffer *inDB, const char *inFormat, va_list inArgs )
{
	OSStatus		err;
	int				n;
	
	n = VCPrintF( __DataBuffer_PrintFCallBack, inDB, inFormat, inArgs );
	require_action( n >= 0, exit, err = n );
	err = kNoErr;
	
exit:
	return( err );
}

static int	__DataBuffer_PrintFCallBack( const char *inStr, size_t inLen, void *inContext )
{
	int		result;
	
	result = (int) DataBuffer_Append( (DataBuffer *) inContext, inStr, inLen );
	require_noerr( result, exit );
	result = (int) inLen;
	
exit:
	return( result );
}

//===========================================================================================================================
//	DataBuffer_AppendANSIFile
//===========================================================================================================================

#if( TARGET_HAS_C_LIB_IO )
OSStatus	DataBuffer_AppendANSIFile( DataBuffer *inBuffer, FILE *inFile )
{
	OSStatus		err;
	uint8_t *		buf;
	size_t			n;
	
	buf = (uint8_t *) malloc_compat( kDataBufferFileBufferSize );
	require_action( buf, exit, err = kNoMemoryErr );
	
	for( ;; )
	{
		n = fread( buf, 1, kDataBufferFileBufferSize, inFile );
		if( n == 0 ) break;
		
		err = DataBuffer_Append( inBuffer, buf, n );
		require_noerr( err, exit );
	}
	err = kNoErr;
	
exit:
	if( buf ) free_compat( buf );
	return( err );
}
#endif

//===========================================================================================================================
//	DataBufferAppendFile
//===========================================================================================================================

#if( TARGET_HAS_C_LIB_IO )
OSStatus	DataBuffer_AppendFile( DataBuffer *inBuffer, const char *inPath )
{
	OSStatus		err;
	FILE *			f;
	
	f = fopen( inPath, "rb" );
	err = map_errno( f );
	require_noerr_quiet( err, exit );
	
	err = DataBuffer_AppendANSIFile( inBuffer, f );
	fclose( f );
	require_noerr( err, exit );
	
exit:
	return( err );
}
#endif

//===========================================================================================================================
//	DataBuffer_RunProcessAndAppendOutput
//===========================================================================================================================

OSStatus	DataBuffer_RunProcessAndAppendOutput( DataBuffer *inBuffer, const char *inCmdLine )
{

#ifndef __NUTTX__
	OSStatus		err;
	FILE *			f;
	
	f = popen( inCmdLine, "r" );
	err = map_errno( f );
	require_noerr_quiet( err, exit );
	
	err = DataBuffer_AppendANSIFile( inBuffer, f );
	if( f ) pclose( f );
	require_noerr( err, exit );
	
exit:
	return( err );
#else
	return 0;
#endif
}

#if 0
#pragma mark -
#pragma mark == Debugging ==
#endif

