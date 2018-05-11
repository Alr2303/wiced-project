/*
	Copyright (C) 2001-2013 Apple Inc. All Rights Reserved.
*/

#include "MiscUtils.h"

// Microsoft deprecated standard C APIs like fopen so disable those warnings because the replacement APIs are not portable.

#if( !defined( _CRT_SECURE_NO_DEPRECATE ) )
	#define _CRT_SECURE_NO_DEPRECATE		1
#endif

#include "CommonServices.h"	// Include early for TARGET_*, etc. definitions.
#include "DebugServices.h"	// Include early for DEBUG_*, etc. definitions.

	#include <errno.h>
	#include <stdarg.h>
	#include <stddef.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

#ifndef __NUTTX__
	#include <ftw.h>
#endif
	#include <pthread.h>
#ifndef __NUTTX__
	#include <pwd.h>
#endif
	#include <sys/mman.h>
	#include <sys/sysctl.h>
	#include <unistd.h>

#include "CFUtils.h"
#include "DataBufferUtils.h"
#include "MathUtils.h"
#include "RandomNumberUtils.h"
#include "StringUtils.h"
#include "TickUtils.h"

#if 0
#pragma mark == FramesPerSecond ==
#endif

//===========================================================================================================================
//	FPSInit
//===========================================================================================================================

void	FPSInit( FPSData *inData, int inPeriods )
{
	inData->smoothingFactor = 2.0 / ( inPeriods + 1 );
	inData->ticksPerSecF	= (double) UpTicksPerSecond();
	inData->periodTicks		= UpTicksPerSecond();
	inData->lastTicks		= UpTicks();
	inData->totalFrameCount	= 0;
	inData->lastFrameCount	= 0;
	inData->lastFPS			= 0;
	inData->averageFPS		= 0;
}

//===========================================================================================================================
//	FPSUpdate
//===========================================================================================================================

void	FPSUpdate( FPSData *inData, uint32_t inFrameCount )
{
	uint64_t		nowTicks, deltaTicks;
	uint32_t		deltaFrames;
	double			deltaSecs;
	
	inData->totalFrameCount += inFrameCount;
	
	nowTicks   = UpTicks();
	deltaTicks = nowTicks - inData->lastTicks;
	if( deltaTicks >= inData->periodTicks )
	{
		deltaSecs				= deltaTicks / inData->ticksPerSecF;
		deltaFrames				= inData->totalFrameCount - inData->lastFrameCount;
		inData->lastFrameCount	= inData->totalFrameCount;
		inData->lastTicks		= nowTicks;
		inData->lastFPS			= deltaFrames / deltaSecs;
		inData->averageFPS		= ( ( 1.0 - inData->smoothingFactor ) * inData->averageFPS ) + 
										  ( inData->smoothingFactor   * inData->lastFPS );
	}
}

#if 0
#pragma mark -
#pragma mark == Misc ==
#endif

//===========================================================================================================================
//	qsort-compatibile comparison functions.
//===========================================================================================================================

DEFINE_QSORT_NUMERIC_COMPARATOR( int8_t,	qsort_cmp_int8 )
DEFINE_QSORT_NUMERIC_COMPARATOR( uint8_t,	qsort_cmp_uint8 )
DEFINE_QSORT_NUMERIC_COMPARATOR( int16_t,	qsort_cmp_int16 )
DEFINE_QSORT_NUMERIC_COMPARATOR( uint16_t,	qsort_cmp_uint16 )
DEFINE_QSORT_NUMERIC_COMPARATOR( int32_t,	qsort_cmp_int32 )
DEFINE_QSORT_NUMERIC_COMPARATOR( uint32_t,	qsort_cmp_uint32 )
DEFINE_QSORT_NUMERIC_COMPARATOR( int64_t,	qsort_cmp_int64 )
DEFINE_QSORT_NUMERIC_COMPARATOR( uint64_t,	qsort_cmp_uint64 )
DEFINE_QSORT_NUMERIC_COMPARATOR( float,		qsort_cmp_float )
DEFINE_QSORT_NUMERIC_COMPARATOR( double,	qsort_cmp_double )

//===========================================================================================================================
//	QSortPtrs
//
//	QuickSort code derived from the simple quicksort code from the book "The Practice of Programming".
//===========================================================================================================================

void	QSortPtrs( void *inPtrArray, size_t inPtrCount, ComparePtrsFunctionPtr inCmp, void *inContext )
{
	void ** const		ptrArray = (void **) inPtrArray;
	void *				t;
	size_t				i, last;
	
	if( inPtrCount <= 1 )
		return;
	
	i = Random32() % inPtrCount;
	t = ptrArray[ 0 ];
	ptrArray[ 0 ] = ptrArray[ i ];
	ptrArray[ i ] = t;
	
	last = 0;
	for( i = 1; i < inPtrCount; ++i )
	{
		if( inCmp( ptrArray[ i ], ptrArray[ 0 ], inContext ) < 0 )
		{
			t = ptrArray[ ++last ];
			ptrArray[ last ] = ptrArray[ i ];
			ptrArray[ i ] = t;
		}
	}
	t = ptrArray[ 0 ];
	ptrArray[ 0 ] = ptrArray[ last ];
	ptrArray[ last ] = t;
	
	QSortPtrs( ptrArray, last, inCmp, inContext );
	QSortPtrs( &ptrArray[ last + 1 ], ( inPtrCount - last ) - 1, inCmp, inContext );
}

int	CompareIntPtrs( const void *inLeft, const void *inRight, void *inContext )
{
	int const		a = *( (const int *) inLeft );
	int const		b = *( (const int *) inRight );
	
	(void) inContext;
	
	return( ( a > b ) - ( a < b ) );
}

int	CompareStringPtrs( const void *inLeft, const void *inRight, void *inContext )
{
	(void) inContext; // Unused
	
	return( strcmp( (const char *) inLeft, (const char *) inRight ) );
}

//===========================================================================================================================
//	memcmp_constant_time
//
//	Compares memory so that the time it takes does not depend on the data being compared.
//	This is needed to avoid certain timing attacks in cryptographic software.
//===========================================================================================================================

int	memcmp_constant_time( const void *inA, const void *inB, size_t inLen )
{
	const uint8_t * const		a = (const uint8_t *) inA;
	const uint8_t * const		b = (const uint8_t *) inB;
	int							result = 0;
	size_t						i;
	
	for( i = 0; i < inLen; ++i )
	{
		result |= ( a[ i ] ^ b[ i ] );
	}
	return( result );
}

//===========================================================================================================================
//	MemReverse
//===========================================================================================================================

void	MemReverse( const void *inSrc, size_t inLen, void *inDst )
{
	check( ( inSrc == inDst ) || !PtrsOverlap( inSrc, inLen, inDst, inLen ) );
	
	if( inSrc == inDst )
	{
		if( inLen > 1 )
		{
			uint8_t *		left  = (uint8_t *) inDst;
			uint8_t *		right = left + ( inLen - 1 );
			uint8_t			temp;
		
			while( left < right )
			{
				temp		= *left;
				*left++		= *right;
				*right--	= temp;
			}
		}
	}
	else
	{
		const uint8_t *		src = (const uint8_t *) inSrc;
		const uint8_t *		end = src + inLen;
		uint8_t *			dst = (uint8_t *) inDst;
		
		while( src < end )
		{
			*dst++ = *( --end );
		}
	}
}

//===========================================================================================================================
//	Swap16Mem
//===========================================================================================================================

void	Swap16Mem( const void *inSrc, size_t inLen, void *inDst )
{
	const uint16_t *			src = (const uint16_t *) inSrc;
	const uint16_t * const		end = src + ( inLen / 2 );
	uint16_t *					dst = (uint16_t *) inDst;
	
	check( ( inLen % 2 ) == 0 );
	check( IsPtrAligned( src, 2 ) );
	check( IsPtrAligned( dst, 2 ) );
	check( ( src == dst ) || !PtrsOverlap( src, inLen, dst, inLen ) );
	
	while( src != end )
	{
		*dst++ = ReadSwap16( src );
		++src;
	}
}

//===========================================================================================================================
//	SwapUUID
//===========================================================================================================================

void	SwapUUID( const void *inSrc, void *inDst )
{
	uint8_t * const		dst = (uint8_t *) inDst;
	uint8_t				tmp[ 16 ];
	
	check( ( inSrc == inDst ) || !PtrsOverlap( inSrc, 16, inDst, 16 ) );
	
	memcpy( tmp, inSrc, 16 );
	dst[  0 ] = tmp[  3 ];
	dst[  1 ] = tmp[  2 ];
	dst[  2 ] = tmp[  1 ];
	dst[  3 ] = tmp[  0 ];
	dst[  4 ] = tmp[  5 ];
	dst[  5 ] = tmp[  4 ];
	dst[  6 ] = tmp[  7 ];
	dst[  7 ] = tmp[  6 ];
	dst[  8 ] = tmp[  8 ];
	dst[  9 ] = tmp[  9 ];
	dst[ 10 ] = tmp[ 10 ];
	dst[ 11 ] = tmp[ 11 ];
	dst[ 12 ] = tmp[ 12 ];
	dst[ 13 ] = tmp[ 13 ];
	dst[ 14 ] = tmp[ 14 ];
	dst[ 15 ] = tmp[ 15 ];
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	CopySmallFile
//===========================================================================================================================

OSStatus	CopySmallFile( const char *inSrcPath, const char *inDstPath )
{
	OSStatus		err;
	FILE *			srcFile;
	FILE *			dstFile;
	size_t			bufSize;
	uint8_t *		buf;
	size_t			readSize;
	size_t			writtenSize;
	
	dstFile	= NULL;
	buf		= NULL;
	
	srcFile = fopen( inSrcPath, "rb" );
	err = map_global_value_errno( srcFile, srcFile );
	require_noerr( err, exit );
	
	dstFile = fopen( inDstPath, "wb" );
	err = map_global_value_errno( dstFile, dstFile );
	require_noerr( err, exit );
	
	bufSize = 256 * 1024;
	buf = (uint8_t *) malloc( bufSize );
	require_action( buf, exit, err = kNoMemoryErr );
	
	for( ;; )
	{
		readSize = fread( buf, 1, bufSize, srcFile );
		if( readSize == 0 ) break;
		
		writtenSize = fwrite( buf, 1, readSize, dstFile );
		err = map_global_value_errno( writtenSize == readSize, dstFile );
		require_noerr( err, exit );
	}
	
exit:
	if( buf )		free( buf );
	if( dstFile )	fclose( dstFile );
	if( srcFile )	fclose( srcFile );
	return( err );
}

//===========================================================================================================================
//	ReadANSIFile
//===========================================================================================================================

OSStatus	ReadANSIFile( FILE *inFile, void *inBuf, size_t inSize, size_t *outSize )
{
	uint8_t *		ptr;
	size_t			n;
	
	ptr = (uint8_t *) inBuf;
	while( inSize > 0 )
	{
		n = fread( ptr, 1, inSize, inFile );
		if( n == 0 ) break;
		ptr    += n;
		inSize -= n;
	}
	if( outSize ) *outSize = (size_t)( ptr - ( (uint8_t *) inBuf ) );
	return( kNoErr );
}

//===========================================================================================================================
//	WriteANSIFile
//===========================================================================================================================

OSStatus	WriteANSIFile( FILE *inFile, const void *inBuf, size_t inSize )
{
	const uint8_t *		ptr;
	size_t				n;
	
	ptr = (const uint8_t *) inBuf;
	while( inSize > 0 )
	{
		n = fwrite( ptr, 1, inSize, inFile );
		if( n == 0 ) break;
		ptr    += n;
		inSize -= n;
	}
	return( kNoErr );
}

//===========================================================================================================================
//	CreateTXTRecordWithCString
//===========================================================================================================================

OSStatus	CreateTXTRecordWithCString( const char *inString, uint8_t **outTXTRec, size_t *outTXTLen )
{
	OSStatus			err;
	DataBuffer			dataBuf;
	const char *		src;
	const char *		end;
	char				buf[ 256 ];
	size_t				len;
	uint8_t *			ptr;
	
	DataBuffer_Init( &dataBuf, NULL, 0, SIZE_MAX );
	
	src = inString;
	end = src + strlen( src );
	while( ParseQuotedEscapedString( src, end, ' ', buf, sizeof( buf ), &len, NULL, &src ) )
	{
		err = DataBuffer_Grow( &dataBuf, 1 + len, &ptr );
		require_noerr( err, exit );
		
		*ptr++ = (uint8_t) len;
		memcpy( ptr, buf, len );
	}
	
	err = DataBuffer_Detach( &dataBuf, outTXTRec, outTXTLen );
	require_noerr( err, exit );
	
exit:
	DataBuffer_Free( &dataBuf );
	return( err );	
}

//===========================================================================================================================
//	GetHomePath
//===========================================================================================================================

char *	GetHomePath( char *inBuffer, size_t inMaxLen )
{
	char *				path = NULL;
#ifndef __NUTTX__
	long				len;
	char *				buf;
	struct passwd		pwdStorage;
	struct passwd *		pwdPtr;
	
	if( inMaxLen < 1 ) return( "" );
	*inBuffer = '\0';
	
	len = sysconf( _SC_GETPW_R_SIZE_MAX );
	if( ( len <= 0 ) || ( len > SSIZE_MAX ) ) len = 4096;
	
	buf = (char *) malloc( (size_t) len );
	check( buf );
	if( buf )
	{
		pwdPtr = NULL;
		if( ( getpwuid_r( getuid(), &pwdStorage, buf, (size_t) len, &pwdPtr ) == 0 ) && pwdPtr )
		{
			path = pwdPtr->pw_dir;
		}
	}
	if( !path ) path = getenv( "HOME" );
	if( !path ) path = ( getuid() == 0 ) ? "/root" : ".";
	strlcpy( inBuffer, path, inMaxLen );
	if( buf ) free( buf );
#else /* __NUTTX__ */
	path = "/etc";
	strlcpy( inBuffer, path, inMaxLen );
#endif /* __NUTTX__ */
	return( path );
}

//===========================================================================================================================
//	NormalizePath
//===========================================================================================================================

char *	NormalizePath( const char *inSrc, size_t inLen, char *inDst, size_t inMaxLen, uint32_t inFlags )
{
#ifndef __NUTTX__
	const char *		src = inSrc;
	const char *		end = ( inLen == kSizeCString ) ? ( inSrc + strlen( inSrc ) ) : ( inSrc + inLen );
	const char *		ptr;
	char *				dst;
	char *				lim;
	size_t				len;
	char				buf1[ PATH_MAX ];
	char				buf2[ PATH_MAX ];
	const char *		replacePtr;
	const char *		replaceEnd;
	
	// If the path is exactly "~" then expand to the current user's home directory.
	// If the path is exactly "~user" then expand to "user"'s home directory.
	// If the path begins with "~/" then expand the "~/" to the current user's home directory.
	// If the path begins with "~user/" then expand the "~user/" to "user"'s home directory.
	
	dst = buf1;
	lim = dst + ( sizeof( buf1 ) - 1 );
	if( !( inFlags & kNormalizePathDontExpandTilde ) && ( src < end ) && ( *src == '~' ) )
	{
		replacePtr = NULL;
		replaceEnd = NULL;
		for( ptr = inSrc + 1; ( src < end ) && ( *src != '/' ); ++src ) {}
		len = (size_t)( src - ptr );
		if( len == 0 ) // "~" or "~/". Expand to current user's home directory.
		{
			replacePtr = getenv( "HOME" );
			if( replacePtr ) replaceEnd = replacePtr + strlen( replacePtr );
		}
		else // "~user" or "~user/". Expand to "user"'s home directory.
		{
			struct passwd *		pw;
			
			len = Min( len, sizeof( buf2 ) - 1 );
			memcpy( buf2, ptr, len );
			buf2[ len ] = '\0';
			pw = getpwnam( buf2 );
			if( pw && pw->pw_dir )
			{
				replacePtr = pw->pw_dir;
				replaceEnd = replacePtr + strlen( replacePtr );
			}
		}
		if( replacePtr ) while( ( dst < lim ) && ( replacePtr < replaceEnd ) ) *dst++ = *replacePtr++;
		else src = inSrc;
	}
	while( ( dst < lim ) && ( src < end ) ) *dst++ = *src++;
	*dst = '\0';
	
	// Resolve the path to remove ".", "..", and sym links.
	
	if( !( inFlags & kNormalizePathDontResolve ) )
	{
		if( inMaxLen >= PATH_MAX )
		{
			dst = realpath( buf1, inDst );
			if( !dst ) strlcpy( inDst, buf1, inMaxLen );
		}
		else
		{
			dst = realpath( buf1, buf2 );
			strlcpy( inDst, dst ? buf2 : buf1, inMaxLen );
		}
	}
	else
	{
		strlcpy( inDst, buf1, inMaxLen );
	}
	return( ( inMaxLen > 0 ) ? inDst : "" );
#else
	return NULL;
#endif
}

//===========================================================================================================================
//	NumberListStringCreateFromUInt8Array
//===========================================================================================================================

static int	__NumberListStringCreateFromUInt8ArraySorter( const void *inLeft, const void *inRight );

OSStatus	NumberListStringCreateFromUInt8Array( const uint8_t *inArray, size_t inCount, char **outStr )
{
	OSStatus		err;
	DataBuffer		dataBuf;
	uint8_t *		sortedArray;
	size_t			i;
	uint8_t			x;
	uint8_t			y;
	uint8_t			z;
	char			buf[ 32 ];
	char *			ptr;
	
	sortedArray = NULL;
	DataBuffer_Init( &dataBuf, NULL, 0, SIZE_MAX );
	
	if( inCount > 0 )
	{
		sortedArray = (uint8_t *) malloc( inCount * sizeof( *inArray ) );
		require_action( sortedArray, exit, err = kNoMemoryErr );
		
		memcpy( sortedArray, inArray, inCount * sizeof( *inArray ) );
		qsort( sortedArray, inCount, sizeof( *sortedArray ), __NumberListStringCreateFromUInt8ArraySorter );
		
		i = 0;
		while( i < inCount )
		{
			x = sortedArray[ i++ ];
			y = x;
			while( ( i < inCount ) && ( ( ( z = sortedArray[ i ] ) - y ) <= 1 ) )
			{
				y = z;
				++i;
			}
			
			ptr = buf;
			if( x == y )		ptr  += snprintf( ptr, sizeof( buf ), "%d", x );
			else				ptr  += snprintf( ptr, sizeof( buf ), "%d-%d", x, y );
			if( i < inCount )  *ptr++ = ',';
			
			err = DataBuffer_Append( &dataBuf, buf, (size_t)( ptr - buf ) );
			require_noerr( err, exit );
		}
	}
	
	err = DataBuffer_Append( &dataBuf, "", 1 );
	require_noerr( err, exit );
	
	*outStr = (char *) dataBuf.bufferPtr;
	dataBuf.bufferPtr = NULL;
	
exit:
	if( sortedArray ) free( sortedArray );
	DataBuffer_Free( &dataBuf );
	return( err );
}

static int	__NumberListStringCreateFromUInt8ArraySorter( const void *inLeft, const void *inRight )
{
	return( *( (const uint8_t *)  inLeft ) - *( (const uint8_t *)  inRight ) );
}

//===========================================================================================================================
//	RemovePath
//===========================================================================================================================
#ifndef __NUTTX__
DEBUG_STATIC int	_RemovePathCallBack( const char *inPath, const struct stat *inStat, int inFlags, struct FTW *inFTW );

OSStatus	RemovePath( const char *inPath )
{
	OSStatus		err;
	
	err = nftw( inPath, _RemovePathCallBack, 64, FTW_CHDIR | FTW_DEPTH | FTW_MOUNT | FTW_PHYS );
	err = map_global_noerr_errno( err );
	return( err );
}

DEBUG_STATIC int	_RemovePathCallBack( const char *inPath, const struct stat *inStat, int inFlags, struct FTW *inFTW )
{
	int		err;
	
	(void) inStat;
	(void) inFlags;
	(void) inFTW;
	
	err = remove( inPath );
	err = map_global_noerr_errno( err );
	return( err );
}
#endif
//===========================================================================================================================
//	RunProcessAndCaptureOutput
//===========================================================================================================================

OSStatus	RunProcessAndCaptureOutput( const char *inCmdLine, char **outResponse )
{
	return( RunProcessAndCaptureOutputEx( inCmdLine, outResponse, NULL ) );
}

OSStatus	RunProcessAndCaptureOutputEx( const char *inCmdLine, char **outResponse, size_t *outLen )
{
	OSStatus		err;
	DataBuffer		dataBuf;
	
	DataBuffer_Init( &dataBuf, NULL, 0, SIZE_MAX );
	
	err = DataBuffer_RunProcessAndAppendOutput( &dataBuf, inCmdLine );
	require_noerr_quiet( err, exit );
	
	err = DataBuffer_Append( &dataBuf, "", 1 );
	require_noerr( err, exit );
	
	*outResponse = (char *) DataBuffer_GetPtr( &dataBuf );
	if( outLen ) *outLen =  DataBuffer_GetLen( &dataBuf );
	dataBuf.bufferPtr = NULL;
	
exit:
	DataBuffer_Free( &dataBuf );
	return( err );
}

CFStringRef	SCDynamicStoreCopyLocalHostName( SCDynamicStoreRef inStore )
{
	char			buf[ 256 ];
	CFStringRef		cfStr;
	
	(void) inStore; // Unused
	
	buf[ 0 ] = '\0';
	gethostname( buf, sizeof( buf ) );
	buf[ sizeof( buf ) - 1 ] = '\0';
	
	cfStr = CFStringCreateWithCString( NULL, buf, kCFStringEncodingUTF8 );
	require( cfStr, exit );
		
exit:
	return( cfStr );
}

//===========================================================================================================================
//	systemf
//===========================================================================================================================
#ifndef __NUTTX__
int	systemf( const char *inLogPrefix, const char *inFormat, ... )
{
	int			err;
	va_list		args;
	char *		cmd;
	
	cmd = NULL;
	va_start( args, inFormat );
	VASPrintF( &cmd, inFormat, args );
	va_end( args );
	require_action( cmd, exit, err = kUnknownErr );
	
	if( inLogPrefix ) fprintf( stderr, "%s%s\n", inLogPrefix, cmd );
	err = system( cmd );
	if( err == -1 )
	{
		err = errno;
		if( !err ) err = -1;
	}
	else if( err != 0 )
	{
		err = WEXITSTATUS( err );
	}
	free( cmd );
	
exit:
	return( err );
}
#endif

#if 0
#pragma mark -
#pragma mark == Packing/Unpacking ==
#endif

//===========================================================================================================================
//	PackData
//===========================================================================================================================

OSStatus	PackData( void *inBuffer, size_t inMaxSize, size_t *outSize, const char *inFormat, ... )
{
	OSStatus		err;
	va_list			args;
	
	va_start( args, inFormat );
	err = PackDataVAList( inBuffer, inMaxSize, outSize, inFormat, args );
	va_end( args );
	
	return( err );
}

//===========================================================================================================================
//	PackDataVAList
//===========================================================================================================================

OSStatus	PackDataVAList( void *inBuffer, size_t inMaxSize, size_t *outSize, const char *inFormat, va_list inArgs )
{
	OSStatus			err;
	const char *		fmt;
	char				c;
	const uint8_t *		src;
	const uint8_t *		end;
	uint8_t *			dst;
	uint8_t *			lim;
	uint8_t				u8;
	uint16_t			u16;
	uint32_t			u32;
	size_t				size;
	
	dst = (uint8_t *) inBuffer;
	lim = dst + inMaxSize;
	
	// Loop thru each character in the format string, decode it, and pack the data appropriately.
	
	fmt = inFormat;
	for( c = *fmt; c != '\0'; c = *++fmt )
	{
		int		altForm;
		
		// Ignore non-% characters like spaces since they can aid in format string readability.
		
		if( c != '%' ) continue;
		
		// Flags
		
		altForm = 0;
		for( ;; )
		{
			c = *++fmt;
			if( c == '#' ) altForm += 1;
			else break;
		}
		
		// Format specifiers.
		
		switch( c )
		{
			case 'b':	// %b: Write byte (8-bit); arg=unsigned int
				
				require_action( dst < lim, exit, err = kOverrunErr );
				u8 = (uint8_t) va_arg( inArgs, unsigned int );
				*dst++ = u8;
				break;
			
			case 'H':	// %H: Write big endian half-word (16-bit); arg=unsigned int
				
				require_action( ( lim - dst ) >= 2, exit, err = kOverrunErr );
				u16	= (uint16_t) va_arg( inArgs, unsigned int );
				*dst++ 	= (uint8_t)( ( u16 >>  8 ) & 0xFF );
				*dst++ 	= (uint8_t)(   u16         & 0xFF );
				break;
			
			case 'W':	// %W: Write big endian word (32-bit); arg=unsigned int
				
				require_action( ( lim - dst ) >= 4, exit, err = kOverrunErr );
				u32	= (uint32_t) va_arg( inArgs, unsigned int );
				*dst++ 	= (uint8_t)( ( u32 >> 24 ) & 0xFF );
				*dst++ 	= (uint8_t)( ( u32 >> 16 ) & 0xFF );
				*dst++ 	= (uint8_t)( ( u32 >>  8 ) & 0xFF );
				*dst++ 	= (uint8_t)(   u32         & 0xFF );
				break;
			
			case 's':	// %s/%#s: Write string/length byte-prefixed string; arg=const char *inStr (null-terminated)
						// Note: Null terminator is not written.
				
				src = va_arg( inArgs, const uint8_t * );
				require_action( src, exit, err = kParamErr );
				
				for( end = src; *end; ++end ) {}
				size = (size_t)( end - src );
				
				if( altForm ) // Pascal-style length byte-prefixed string
				{
					require_action( size <= 255, exit, err = kSizeErr );
					require_action( ( 1 + size ) <= ( (size_t)( lim - dst ) ), exit, err = kOverrunErr );
					*dst++ = (uint8_t) size;
				}
				else
				{
					require_action( size <= ( (size_t)( lim - dst ) ), exit, err = kOverrunErr );
				}
				while( src < end )
				{
					*dst++ = *src++;
				}
				break;
			
			case 'n':	// %n: Write N bytes; arg 1=const void *inData, arg 2=unsigned int inSize
				
				src = va_arg( inArgs, const uint8_t * );
				require_action( src, exit, err = kParamErr );
				
				size = (size_t) va_arg( inArgs, unsigned int );
				require_action( size <= ( (size_t)( lim - dst ) ), exit, err = kOverrunErr );
				
				end = src + size;
				while( src < end )
				{
					*dst++ = *src++;
				}
				break;
			
			default:
				dlogassert( "unknown format specifier: %%%c", c );
				err = kUnsupportedErr;
				goto exit;
		}
	}
	
	*outSize = (size_t)( dst - ( (uint8_t *) inBuffer ) );
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	UnpackData
//===========================================================================================================================

OSStatus	UnpackData( const void *inData, size_t inSize, const char *inFormat, ... )
{
	OSStatus		err;
	va_list			args;
	
	va_start( args, inFormat );
	err = UnpackDataVAList( inData, inSize, inFormat, args );
	va_end( args );
	
	return( err );
}

//===========================================================================================================================
//	UnpackDataVAList
//===========================================================================================================================

OSStatus	UnpackDataVAList( const void *inData, size_t inSize, const char *inFormat, va_list inArgs )
{
	OSStatus				err;
	const char *			fmt;
	char					c;
	const uint8_t *			src;
	const uint8_t *			end;
	const uint8_t *			ptr;
	uint16_t				u16;
	uint32_t				u32;
	size_t					size;
	uint8_t *				bArg;
	uint16_t *				hArg;
	uint32_t *				wArg;
	const uint8_t **		ptrArg;
	size_t *				sizeArg;
		
	src = (const uint8_t *) inData;
	end = src + inSize;
	
	// Loop thru each character in the format string, decode it, and unpack the data appropriately.
	
	fmt = inFormat;
	for( c = *fmt; c != '\0'; c = *++fmt )
	{
		int		altForm;
		
		// Ignore non-% characters like spaces since they can aid in format string readability.
		
		if( c != '%' ) continue;
		
		// Flags
		
		altForm = 0;
		for( ;; )
		{
			c = *++fmt;
			if( c == '#' ) altForm += 1;
			else break;
		}
		
		// Format specifiers.
		
		switch( c )
		{
			case 'b':	// %b: Read byte (8-bit); arg=uint8_t *outByte
				
				require_action( altForm == 0, exit, err = kFlagErr );
				require_action_quiet( ( end - src ) >= 1, exit, err = kUnderrunErr );
				bArg = va_arg( inArgs, uint8_t * );
				if( bArg ) *bArg = *src;
				++src;
				break;
			
			case 'H':	// %H: Read big endian half-word (16-bit); arg=uint16_t *outU16
				
				require_action( ( end - src ) >= 2, exit, err = kUnderrunErr );
				u16  = (uint16_t)( *src++ << 8 );
				u16 |= (uint16_t)( *src++ );
				hArg = va_arg( inArgs, uint16_t * );
				if( hArg ) *hArg = u16;
				break;
			
			case 'W':	// %H: Read big endian word (32-bit); arg=uint32_t *outU32
				
				require_action( ( end - src ) >= 4, exit, err = kUnderrunErr );
				u32  = (uint32_t)( *src++ << 24 );
				u32 |= (uint32_t)( *src++ << 16 );
				u32 |= (uint32_t)( *src++ << 8 );
				u32 |= (uint32_t)( *src++ );
				wArg = va_arg( inArgs, uint32_t * );
				if( wArg ) *wArg = u32;
				break;
			
			case 's':	// %s: Read string; arg 1=const char **outStr, arg 2=size_t *outSize (size excludes null terminator).
				
				if( altForm ) // Pascal-style length byte-prefixed string
				{
					require_action( src < end, exit, err = kUnderrunErr );
					size = *src++;
					require_action( size <= (size_t)( end - src ), exit, err = kUnderrunErr );
					
					ptr = src;
					src += size;
				}
				else
				{
					for( ptr = src; ( src < end ) && ( *src != 0 ); ++src ) {}
					require_action( src < end, exit, err = kUnderrunErr );
					size = (size_t)( src - ptr );
					++src;
				}
				
				ptrArg = va_arg( inArgs, const uint8_t ** );
				if( ptrArg ) *ptrArg = ptr;
				
				sizeArg	= va_arg( inArgs, size_t * );
				if( sizeArg ) *sizeArg = size;
				break;
			
			case 'n':	// %n: Read N bytes; arg 1=size_t inSize, arg 2=const uint8_t **outData
				
				size = (size_t) va_arg( inArgs, unsigned int );
				require_action( size <= (size_t)( end - src ), exit, err = kUnderrunErr );
				
				ptrArg = va_arg( inArgs, const uint8_t ** );
				if( ptrArg ) *ptrArg = src;
				
				src += size;
				break;
			
			default:
				dlogassert( "unknown format specifier: %%%c", c );
				err = kUnsupportedErr;
				goto exit;
		}
	}
	err = kNoErr;
	
exit:
	return( err );
}

#if 0
#pragma mark -
#pragma mark == EDID ==
#endif

//===========================================================================================================================
//	CreateEDIDDictionaryWithBytes
//===========================================================================================================================

static OSStatus	_ParseEDID_CEABlock( const uint8_t *inData, CFMutableDictionaryRef inEDIDDict );
static OSStatus	_ParseEDID_CEABlock_HDMI( const uint8_t *inData, size_t inSize, CFMutableDictionaryRef inCAEDict );

CFDictionaryRef	CreateEDIDDictionaryWithBytes( const uint8_t *inData, size_t inSize, OSStatus *outErr )
{
	CFDictionaryRef				result		= NULL;
	CFMutableDictionaryRef		edidDict	= NULL;
	OSStatus					err;
	const uint8_t *				src;
	const uint8_t *				end;
	uint8_t						u8;
	uint32_t					u32;
	char						strBuf[ 256 ];
	const char *				strPtr;
	const char *				strEnd;
	char						c;
	CFStringRef					key;
	CFStringRef					cfStr;
	int							i;
	char *						dst;
	int							extensionCount;
	
	require_action_quiet( inSize >= 128, exit, err = kSizeErr );
	require_action_quiet( memcmp( inData, "\x00\xFF\xFF\xFF\xFF\xFF\xFF\x00", 8 ) == 0, exit, err = kFormatErr );
	
	src = inData;
	end = src + 128;
	for( u8 = 0; src < end; ++src ) u8 += *src;
	require_action_quiet( u8 == 0, exit, err = kChecksumErr );
	
	edidDict = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	require_action( edidDict, exit, err = kNoMemoryErr ); 
	
	CFDictionarySetData( edidDict, kEDIDKey_RawBytes, inData, inSize );
	
	// Manufacturer. It's 3 characters that get encoded as 2 bytes.
	
	strBuf[ 0 ] = (char)( ( ( inData[ 8 ] & 0x7C ) >> 2 ) + '@' );
	strBuf[ 1 ] = (char)( ( ( inData[ 8 ] & 0x03 ) << 3 ) + ( ( inData[ 9 ] & 0xE0 ) >> 5 ) + '@' );
	strBuf[ 2 ] = (char)( (   inData[ 9 ] & 0x1F ) + '@' );
	strBuf[ 3 ] = '\0';
	
	cfStr = CFStringCreateWithCString( NULL, strBuf, kCFStringEncodingUTF8 );
	if( !cfStr )
	{
		// Malformed manufacturer so %-escape the raw bytes.
		
		snprintf( strBuf, sizeof( strBuf ), "<<%%%02X%%%02X>>", inData[ 8 ], inData[ 9 ] );
		cfStr = CFStringCreateWithCString( NULL, strBuf, kCFStringEncodingUTF8 );
		require_action( cfStr, exit, err = kNoMemoryErr );
	}
	CFDictionarySetValue( edidDict, kEDIDKey_Manufacturer, cfStr );
	CFRelease( cfStr );
	
	// Product ID
	
	u32 = inData[ 10 ] | ( inData[ 11 ] << 8 );
	CFDictionarySetInt64( edidDict, kEDIDKey_ProductID, u32 );
	
	// Serial number
	
	u32 = inData[ 12 ] | ( inData[ 13 ] << 8 ) | ( inData[ 14 ] << 16 ) | ( inData[ 15 ] << 24 );
	CFDictionarySetInt64( edidDict, kEDIDKey_SerialNumber, u32 );
	
	// Manufacture date (year and week).
	
	CFDictionarySetInt64( edidDict, kEDIDKey_WeekOfManufacture, inData[ 16 ] );
	CFDictionarySetInt64( edidDict, kEDIDKey_YearOfManufacture, 1990 + inData[ 17 ] );
	
	// EDID versions.
	
	CFDictionarySetInt64( edidDict, kEDIDKey_EDIDVersion, inData[ 18 ] );
	CFDictionarySetInt64( edidDict, kEDIDKey_EDIDRevision, inData[ 19 ] );
	
	// $$$ TO DO: Parse bytes 20-53.
	
	// Parse descriptor blocks.
	
	src = &inData[ 54 ]; // Descriptor Block 1 (4 of them total, 18 bytes each).
	for( i = 0; i < 4; ++i )
	{
		// If the first two bytes are 0 then it's a monitor descriptor.
		
		if( ( src[ 0 ] == 0 ) && ( src[ 1 ] == 0 ) )
		{
			key = NULL;
			u8 = src[ 3 ];
			if(      u8 == 0xFC ) key = kEDIDKey_MonitorName;
			else if( u8 == 0xFF ) key = kEDIDKey_MonitorSerialNumber;
			else {} // $$$ TO DO: parse other descriptor block types.
			if( key )
			{
				dst = strBuf;
				strPtr = (const char *) &src[  5 ];
				strEnd = (const char *) &src[ 18 ];
				while( ( strPtr < strEnd ) && ( ( c = *strPtr ) != '\n' ) && ( c != '\0' ) )
				{
					c = *strPtr++;
					if( ( c >= 32 ) && ( c <= 126 ) )
					{
						*dst++ = c;
					}
					else
					{
						dst += snprintf( dst, 3, "%%%02X", (uint8_t) c );
					}
				}
				*dst = '\0';
				CFDictionarySetCString( edidDict, key, strBuf, kSizeCString );
			}
			else
			{
				// $$$ TO DO: parse other descriptor blocks.
			}
		}
		else
		{
			// $$$ TO DO: parse video timing descriptors.
		}
		
		src += 18; // Move to the next descriptor block.
	}
	
	// Parse extension blocks in EDID versions 1.1 or later.
	
	extensionCount = 0;
	u32 = ( inData[ 18 ] << 8 ) | inData[ 19 ]; // Combine version and revision for easier comparisons.
	if( u32 >= 0x0101 )
	{
		extensionCount = inData[ 126 ];
	}
	src = &inData[ 128 ];
	end = inData + inSize;
	for( i = 0; i < extensionCount; ++i )
	{
		if( ( end - src ) < 128 ) break;
		if( src[ 0 ] == 2 ) _ParseEDID_CEABlock( src, edidDict );
		src += 128;
	}
	
	result = edidDict;
	edidDict = NULL;
	err = kNoErr;	
	
exit:
	if( edidDict )	CFRelease( edidDict );
	if( outErr )	*outErr = err;
	return( result );	
}

//===========================================================================================================================
//	_ParseEDID_CEABlock
//===========================================================================================================================

static OSStatus	_ParseEDID_CEABlock( const uint8_t *inData, CFMutableDictionaryRef inEDIDDict )
{
	OSStatus					err;
	CFMutableDictionaryRef		ceaDict;
	uint8_t						u8;
	const uint8_t *				src;
	const uint8_t *				end;
	uint8_t						dtdOffset;
	uint32_t					regID;
	uint8_t						tag, len;
	
	ceaDict = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	require_action( ceaDict, exit, err = kNoMemoryErr ); 
	CFDictionarySetValue( inEDIDDict, kEDIDKey_CEABlock, ceaDict );
	
	// Verify the checksum.
	
	src = inData;
	end = src + 128;
	for( u8 = 0; src < end; ++src ) u8 += *src;
	require_action_quiet( u8 == 0, exit, err = kChecksumErr );
	
	// Revision.
	
	CFDictionarySetInt64( inEDIDDict, kCEAKey_Revision, inData[ 0 ] );
	
	// $$$ TO DO: Parse general video info.
	
	// Parse each data block.
	
	dtdOffset = inData[ 2 ];
	require_action_quiet( dtdOffset <= 128, exit, err = kRangeErr );
	
	src = &inData[ 4 ];
	end = &inData[ dtdOffset ];
	for( ; src < end; src += ( 1 + len ) )
	{
		tag = src[ 0 ] >> 5;
		len = src[ 0 ] & 0x1F;
		require_action_quiet( ( src + 1 + len ) <= end, exit, err = kUnderrunErr );
		
		switch( tag )
		{
			case 1:
				// $$$ TO DO: Parse Audio Data Block.
				break;
			
			case 2:
				// $$$ TO DO: Parse Video Data Block.
				break;
			
			case 3:
				if( len < 3 ) break;
				regID = src[ 1 ] | ( src[ 2 ] << 8 ) | ( src[ 3 ] << 16 );
				switch( regID )
				{
					case 0x000C03:
						_ParseEDID_CEABlock_HDMI( src, len + 1, ceaDict );
						break;
					
					default:
						break;
				}
				break;
			
			case 4:
				// $$$ TO DO: Parse Speaker Allocation Data Block.
				break;
			
			default:
				break;
		}
	}
	
	// $$$ TO DO: Parse Detailed Timing Descriptors (DTDs).
	
	err = kNoErr;
	
exit:
	if( ceaDict ) CFRelease( ceaDict );
	return( err );
}

//===========================================================================================================================
//	ParseEDID_CEABlock_HDMI
//
//	Note: "inData" points to the data block header byte and "inSize" is the size of the data including the header byte 
//	(inSize = len + 1). This makes a easier to follow byte offsets from the HDMI spec, which are header relative.
//===========================================================================================================================

static OSStatus	_ParseEDID_CEABlock_HDMI( const uint8_t *inData, size_t inSize, CFMutableDictionaryRef inCAEDict )
{
	OSStatus					err;
	CFMutableDictionaryRef		hdmiDict;
	const uint8_t *				ptr;
	const uint8_t *				end;
	int16_t						s16;
	uint32_t					u32;
	uint8_t						latencyFlags;
	const uint8_t *				latencyPtr;
	
	hdmiDict = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	require_action( hdmiDict, exit, err = kNoMemoryErr ); 
	CFDictionarySetValue( inCAEDict, kEDIDKey_HDMI, hdmiDict );
	
	ptr = inData;
	end = inData + inSize;
	require_action_quiet( ( end - ptr ) >= 6, exit, err = kSizeErr );
	
	// Source Physical Address.
	
	u32 = ( ptr[ 4 ] << 8 ) | ptr[ 5 ];
	CFDictionarySetInt64( hdmiDict, kHDMIKey_SourcePhysicalAddress, u32 );
	
	if( inSize > 8 )
	{
		// Parse latency. Note: latency value = ( latency-in-milliseconds / 2 ) + 1 (max value of 251 = 500 ms).
		
		latencyFlags = inData[ 8 ];
		if( latencyFlags & 0x80 )
		{
			latencyPtr = &inData[ 9 ];
			require_action_quiet( ( latencyPtr + 1 ) < end, exit, err = kUnderrunErr );
			
			// Video latency.
			
			s16 = *latencyPtr++;
			s16 = ( s16 - 1 ) * 2;
			CFDictionarySetInt64( hdmiDict, kHDMIKey_VideoLatencyMs, s16 );
			
			// Audio latency.
			
			s16 = *latencyPtr++;
			s16 = ( s16 - 1 ) * 2;
			CFDictionarySetInt64( hdmiDict, kHDMIKey_AudioLatencyMs, s16 );
			
			// Parse interlaced latency.
			
			if( latencyFlags & 0x40 )
			{
				require_action_quiet( ( latencyPtr + 1 ) < end, exit, err = kUnderrunErr );
				
				// Video latency.
				
				s16 = *latencyPtr++;
				s16 = ( s16 - 1 ) * 2;
				CFDictionarySetInt64( hdmiDict, kHDMIKey_VideoLatencyInterlacedMs, s16 );
				
				// Audio latency.
				
				s16 = *latencyPtr++;
				s16 = ( s16 - 1 ) * 2;
				CFDictionarySetInt64( hdmiDict, kHDMIKey_AudioLatencyInterlacedMs, s16 );
			}
		}
	}
	err = kNoErr;
	
exit:
	if( hdmiDict ) CFRelease( hdmiDict );
	return( err );
}

#if 0
#pragma mark -
#pragma mark == H.264 ==
#endif

//===========================================================================================================================
//	H264ConvertAVCCtoAnnexBHeader
//===========================================================================================================================

OSStatus
	H264ConvertAVCCtoAnnexBHeader( 
		const uint8_t *		inAVCCPtr,
		size_t				inAVCCLen,
		uint8_t *			inHeaderBuf,
		size_t				inHeaderMaxLen,
		size_t *			outHeaderLen,
		size_t *			outNALSize,
		const uint8_t **	outNext )
{
	const uint8_t *				src = inAVCCPtr;
	const uint8_t * const		end = src + inAVCCLen;
	uint8_t * const				buf = inHeaderBuf;
	uint8_t *					dst = buf;
	const uint8_t * const		lim = dst + inHeaderMaxLen;
	OSStatus					err;
	size_t						nalSize;
	int							i, n;
	uint16_t					len;
	
	// AVCC Format is documented in ISO/IEC STANDARD 14496-15 (AVC file format) section 5.2.4.1.1.
	//
	// [0x00] version = 1.
	// [0x01] AVCProfileIndication		Profile code as defined in ISO/IEC 14496-10.
	// [0x02] Profile Compatibility		Byte between profile_IDC and level_IDC in SPS from ISO/IEC 14496-10.
	// [0x03] AVCLevelIndication		Level code as defined in ISO/IEC 14496-10.
	// [0x04] LengthSizeMinusOne		0b111111xx where xx is 0, 1, or 3 mapping to 1, 2, or 4 bytes for nal_size.
	// [0x05] SPSCount					0b111xxxxx where xxxxx is the number of SPS entries that follow this field.
	// [0x06] SPS entries				Variable-length SPS array. Each entry has the following structure:
	//		uint16_t	spsLen			Number of bytes in the SPS data until the next entry or the end (big endian).
	//		uint8_t		spsData			SPS entry data.
	// [0xnn] uint8_t	PPSCount		Number of Picture Parameter Sets (PPS) that follow this field.
	// [0xnn] PPS entries				Variable-length PPS array. Each entry has the following structure:
	//		uint16_t	ppsLen			Number of bytes in the PPS data until the next entry or the end (big endian).
	//		uint8_t		ppsData			PPS entry data.
	//
	// Annex-B format is documented in the H.264 spec in Annex-B.
	// Each NAL unit is prefixed with 0x00 0x00 0x00 0x01 and the nal_size from the AVCC is removed.
	
	// Write the SPS NAL units.
	
	require_action( ( end - src ) >= 6, exit, err = kSizeErr );
	nalSize	= ( src[ 4 ] & 0x03 ) + 1;
	n		=   src[ 5 ] & 0x1F;
	src		=  &src[ 6 ];
	for( i = 0; i < n; ++i )
	{
		require_action( ( end - src ) >= 2, exit, err = kUnderrunErr );
		len = (uint16_t)( ( src[ 0 ] << 8 ) | src[ 1 ] );
		src += 2;
		
		require_action( ( end - src ) >= len, exit, err = kUnderrunErr );
		if( inHeaderBuf )
		{
			require_action( ( lim - dst ) >= ( 4 + len ), exit, err = kOverrunErr );
			dst[ 0 ] = 0x00;
			dst[ 1 ] = 0x00;
			dst[ 2 ] = 0x00;
			dst[ 3 ] = 0x01;
			memcpy( dst + 4, src, len );
		}
		src += len;
		dst += ( 4 + len );
	}
	
	// Write PPS NAL units.
	
	if( ( end - src ) >= 1 )
	{
		n = *src++;
		for( i = 0; i < n; ++i )
		{
			require_action( ( end - src ) >= 2, exit, err = kUnderrunErr );
			len = (uint16_t)( ( src[ 0 ] << 8 ) | src[ 1 ] );
			src += 2;
			
			require_action( ( end - src ) >= len, exit, err = kUnderrunErr );
			if( inHeaderBuf )
			{
				require_action( ( lim - dst ) >= ( 4 + len ), exit, err = kOverrunErr );
				dst[ 0 ] = 0x00;
				dst[ 1 ] = 0x00;
				dst[ 2 ] = 0x00;
				dst[ 3 ] = 0x01;
				memcpy( dst + 4, src, len );
			}
			src += len;
			dst += ( 4 + len );
		}
	}
	
	if( outHeaderLen )	*outHeaderLen	= (size_t)( dst - buf );
	if( outNALSize )	*outNALSize		= nalSize;
	if( outNext )		*outNext		= src;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	H264EscapeEmulationPrevention
//===========================================================================================================================

Boolean
	H264EscapeEmulationPrevention( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		const uint8_t **	outDataPtr, 
		size_t *			outDataLen, 
		const uint8_t **	outSuffixPtr, 
		size_t *			outSuffixLen, 
		const uint8_t **	outSrc )
{
	const uint8_t *		ptr;
	
	// To avoid normal data being confused with a start code prefix, insert an emulation prevent byte 0x03 as needed.
	//
	// 0x00,0x00,0x00		-> 0x00,0x00,0x03,0x00
	// 0x00,0x00,0x01		-> 0x00,0x00,0x03,0x01
	// 0x00,0x00,0x02		-> 0x00,0x00,0x03,0x02
	// 0x00,0x00,0x03,0xXX	-> 0x00,0x00,0x03,0x03,0xXX if 0xXX is > 0x03 (e.g. 0x04-0xFF).
	//
	// See H.264 spec section 7.4.1.1 for details.
	
	for( ptr = inSrc; ( inEnd - ptr ) >= 3; ++ptr )
	{
		if( ( ptr[ 0 ] == 0x00 ) && ( ptr[ 1 ] == 0x00 ) )
		{
			if( ptr[ 2 ] <= 2 )
			{
				*outDataPtr		= inSrc;
				*outDataLen		= (size_t)( ( ptr + 2 ) - inSrc );
				if(      ptr[ 2 ] == 0x00 )	*outSuffixPtr = (const uint8_t *) "\x03\x00";
				else if( ptr[ 2 ] == 0x01 )	*outSuffixPtr = (const uint8_t *) "\x03\x01";
				else						*outSuffixPtr = (const uint8_t *) "\x03\x02";
				*outSuffixLen	= 2;
				*outSrc			= ptr + 3;
				return( true );
			}
			else if( ( ptr[ 2 ] == 3 ) && ( ( inEnd - ptr ) >= 4 ) && ( ptr[ 3 ] > 3 ) )
			{
				*outDataPtr		= inSrc;
				*outDataLen		= (size_t)( ( ptr + 3 ) - inSrc );
				*outSuffixPtr	= (const uint8_t *) "\x03";
				*outSuffixLen	= 1;
				*outSrc			= ptr + 3;
				return( true );
			}
		}
	}
	
	// The last byte of a NAL unit must not end with 0x00 so if it's 0x00 then append a 0x03 byte.
	
	*outDataPtr = inSrc;
	*outDataLen = (size_t)( inEnd - inSrc );
	if( ( inSrc != inEnd ) && ( inEnd[ -1 ] == 0x00 ) )
	{
		*outSuffixPtr = (const uint8_t *) "\x03";
		*outSuffixLen = 1;
	}
	else
	{
		*outSuffixPtr = NULL;
		*outSuffixLen = 0;
	}
	*outSrc = inEnd;
	return( inSrc != inEnd );
}

//===========================================================================================================================
//	H264RemoveEmulationPrevention
//===========================================================================================================================

OSStatus
	H264RemoveEmulationPrevention( 
		const uint8_t *	inSrc, 
		size_t			inLen, 
		uint8_t *		inBuf, 
		size_t			inMaxLen, 
		size_t *		outLen )
{
	const uint8_t * const		end = inSrc + inLen;
	uint8_t *					dst = inBuf;
	uint8_t * const				lim = inBuf + inMaxLen;
	OSStatus					err;
	
	// Replace 00 00 03 xx with 0x00 0x00 xx.
	// If the last 2 bytes are 0x00 0x03 then remove the 0x03
	// See H.264 spec section 7.4.1.1 for details.
	
	while( ( end - inSrc ) >= 3 )
	{
		if( ( inSrc[ 0 ] == 0x00 ) && ( inSrc[ 1 ] == 0x00 ) && ( inSrc[ 2 ] == 0x03 ) )
		{
			require_action_quiet( ( lim - dst ) >= 2, exit, err = kOverrunErr );
			dst[ 0 ] = inSrc[ 0 ];
			dst[ 1 ] = inSrc[ 1 ];
			inSrc += 3;
			dst   += 2;
		}
		else
		{
			require_action_quiet( dst < lim, exit, err = kOverrunErr );
			*dst++ = *inSrc++;
		}
	}
	if( ( ( end - inSrc ) >= 2 ) && ( inSrc[ 0 ] == 0x00 ) && ( inSrc[ 1 ] == 0x03 ) )
	{
		require_action_quiet( dst < lim, exit, err = kOverrunErr );
		*dst++ = inSrc[ 0 ];
		inSrc += 2;
	}
	while( ( end - inSrc ) >= 1 )
	{
		require_action_quiet( dst < lim, exit, err = kOverrunErr );
		*dst++ = *inSrc++;
	}
	
	if( outLen ) *outLen = (size_t)( dst - inBuf );
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	H264GetNextNALUnit
//===========================================================================================================================

OSStatus
	H264GetNextNALUnit( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		size_t				inNALSize, 
		const uint8_t * *	outDataPtr, 
		size_t *			outDataLen,
		const uint8_t **	outSrc )
{
	OSStatus		err;
	size_t			len;
	
	require_action_quiet( inSrc != inEnd, exit, err = kEndingErr );
	require_action_quiet( ( (size_t)( inEnd - inSrc ) ) >= inNALSize, exit, err = kUnderrunErr );
	switch( inNALSize )
	{
		case 1:
			len = *inSrc++;
			break;
		
		case 2:
			len  = ( *inSrc++ << 8 );
			len |=   *inSrc++;
			break;
		
		case 4:
			len  = ( *inSrc++ << 24 );
			len |= ( *inSrc++ << 16 );
			len |= ( *inSrc++ <<  8 );
			len |=   *inSrc++;
			break;
		
		default:
			err = kParamErr;
			goto exit;
	}
	require_action_quiet( ( (size_t)( inEnd - inSrc ) ) >= len, exit, err = kUnderrunErr );
	
	*outDataPtr = inSrc;
	*outDataLen = len;
	*outSrc		= inSrc + len;
	err = kNoErr;
	
exit:
	return( err );
}

#if 0
#pragma mark -
#pragma mark == MirroredRingBuffer ==
#endif

//===========================================================================================================================
//	MirroredRingBufferInit
//===========================================================================================================================
#ifndef __NUTTX__
OSStatus	MirroredRingBufferInit( MirroredRingBuffer *inRing, size_t inMinSize, Boolean inPowerOf2 )
{
	char			path[] = "/tmp/MirrorRingBuffer-XXXXXX";
	OSStatus		err;
	long			pageSize;
	int				fd;
	size_t			len;
	uint8_t *		base = (uint8_t *) MAP_FAILED;
	uint8_t *		addr;
	
	// Create a temp file and remove it from the file system, but keep a file descriptor to it.
	
	fd = mkstemp( path );
	err = map_global_value_errno( fd >= 0, fd );
	require_noerr( err, exit );
	
	err = unlink( path );
	err = map_global_noerr_errno( err );
	check_noerr( err );
	
	// Resize the file to the size of the ring buffer.
	
	pageSize = sysconf( _SC_PAGE_SIZE );
	err = map_global_value_errno( pageSize > 0, pageSize );
	require_noerr( err, exit );
	len = RoundUp( inMinSize, (size_t) pageSize );
	if( inPowerOf2 ) len = iceil2( (uint32_t) len );
	err = ftruncate( fd, len );
	err = map_global_noerr_errno( err );
	require_noerr( err, exit );
	
	// Allocate memory for 2x the ring buffer size and map the two halves on top of each other.
	
	base = (uint8_t *) mmap( NULL, len * 2, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0 );
	err = map_global_value_errno( base != MAP_FAILED, base );
	require_noerr( err, exit );
	
	addr = (uint8_t *) mmap( base, len, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0 );
	err = map_global_value_errno( addr == base, addr );
	require_noerr( err, exit );
	
	addr = (uint8_t *) mmap( base + len, len, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0 );
	err = map_global_value_errno( addr == ( base + len ), addr );
	require_noerr( err, exit );
	
	// Success
	
	inRing->buffer		= base;
	inRing->end			= base + len;
	inRing->size		= (uint32_t) len;
	inRing->mask		= (uint32_t)( len - 1 );
	inRing->readOffset	= 0;
	inRing->writeOffset	= 0;
	base = (uint8_t *) MAP_FAILED;
	
exit:
	if( base != MAP_FAILED )	munmap( base, len * 2 );
	if( fd >= 0 )				close( fd );
	return( err );
}

//===========================================================================================================================
//	MirroredRingBufferFree
//===========================================================================================================================

void	MirroredRingBufferFree( MirroredRingBuffer *inRing )
{
	OSStatus		err;
	
	if( inRing->buffer )
	{
		err = munmap( inRing->buffer, inRing->size * 2 );
		err = map_global_noerr_errno( err );
		check_noerr( err );
		inRing->buffer = NULL;
	}
	inRing->end = NULL;
}
#endif

#if 0
#pragma mark -
#pragma mark == Security ==
#endif

#if 0
#pragma mark -
#pragma mark == Debugging ==
#endif

