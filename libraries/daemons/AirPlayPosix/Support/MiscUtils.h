/*
	Copyright (C) 2001-2013 Apple Inc. All Rights Reserved.
*/

#ifndef	__MiscUtils_h__
#define	__MiscUtils_h__

#include "CommonServices.h"
#include "DebugServices.h"

	#include <stdarg.h>
	#include <stddef.h>
	#include <stdlib.h>

	#include CF_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#pragma mark == FramesPerSecond ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		FramesPerSecond Statistics
	@abstract	Tracks an exponential moving average frames per second.
*/		

typedef struct
{
	double			smoothingFactor;
	double			ticksPerSecF;
	uint64_t		periodTicks;
	uint64_t		lastTicks;
	uint32_t		totalFrameCount;
	uint32_t		lastFrameCount;
	double			lastFPS;
	double			averageFPS;
	
}	FPSData;

void	FPSInit( FPSData *inData, int inPeriods );
void	FPSUpdate( FPSData *inData, uint32_t inFrameCount );

#if 0
#pragma mark == Misc ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	QSortPtrs
	@abstract	Sorts an array of pointers according to a comparison function.
*/

typedef int ( *ComparePtrsFunctionPtr )( const void *inLeft, const void *inRight, void *inContext );

void	QSortPtrs( void *inPtrArray, size_t inPtrCount, ComparePtrsFunctionPtr inCmp, void *inContext );
int		CompareIntPtrs( const void *inLeft, const void *inRight, void *inContext );
int		CompareStringPtrs( const void *inLeft, const void *inRight, void *inContext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	memcmp_constant_time
	@abstract	Compares memory so that the time it takes does not depend on the data being compared.
	@discussion	This is needed to avoid certain timing attacks in cryptographic software.
*/

int	memcmp_constant_time( const void *inA, const void *inB, size_t inLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MemReverse
	@abstract	Copies data from one block to another and reverses the order of bytes in the process.
	@discussion	"inSrc" may be the same as "inDst", but may not point to an arbitrary location inside it.
*/

void	MemReverse( const void *inSrc, size_t inLen, void *inDst );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	Swap16Mem
	@abstract	Endian swaps each 16-bit chunk of memory.
	@discussion	"inSrc" may be the same as "inDst", but may not point to an arbitrary location inside it.
*/

void	Swap16Mem( const void *inSrc, size_t inLen, void *inDst );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	SwapUUID
	@abstract	Endian swaps a UUID to convert it between big and little endian.
	@discussion	"inSrc" may be the same as "inDst", but may not point to an arbitrary location inside it.
*/

void	SwapUUID( const void *inSrc, void *inDst );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CopySmallFile
	@abstract	Copies a file from one path to another. Only intended for copying small files.
*/

	OSStatus	CopySmallFile( const char *inSrcPath, const char *inDstPath );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	ReadANSIFile / WriteANSIFile
	@abstract	Reads/writes all the specified data (or up to the end of file for reads).
*/

OSStatus	ReadANSIFile( FILE *inFile, void *inBuf, size_t inSize, size_t *outSize );
OSStatus	WriteANSIFile( FILE *inFile, const void *inBuf, size_t inSize );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	CreateTXTRecordWithCString
	@abstract	Creates a malloc'd TXT record from a string. See ParseQuotedEscapedString for escaping/quoting details.
*/

OSStatus	CreateTXTRecordWithCString( const char *inString, uint8_t **outTXTRec, size_t *outTXTLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	GetHomePath
	@abstract	Gets a path to the home directory for the current process.
*/
char *	GetHomePath( char *inBuffer, size_t inMaxLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	NormalizePath
	@abstract	Normalizes a path to expand tidle's at the beginning and resolve ".", "..", and sym links.
	
	@param		inSrc		Source path to normalize.
	@param		inLen		Number of bytes in "inSrc" or kSizeCString if it's a NUL-terminated string.
	@param		inDst		Buffer to received normalized string. May be the same as "inSrc" to normalized in-place.
	@param		inMaxLen	Max number of bytes (including a NUL terminator) to write to "inDst".
	@param		inFlags		Flags to control the normalization process.
	
	@result		Ptr to inDst or "" if inMaxLen is 0.
	
	@discussion
	
	If the path is exactly "~" then expand to the current user's home directory.
	If the path is exactly "~user" then expand to "user"'s home directory.
	If the path begins with "~/" then expand the "~/" to the current user's home directory.
	If the path begins with "~user/" then expand the "~user/" to "user"'s home directory.
*/

#define kNormalizePathDontExpandTilde		( 1 << 0 ) // Don't replace "~" or "~user" with user home directory path.
#define kNormalizePathDontResolve			( 1 << 1 ) // Don't resolve ".", "..", or sym links.

char *	NormalizePath( const char *inSrc, size_t inLen, char *inDst, size_t inMaxLen, uint32_t inFlags );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	NumberListStringCreateFromUInt8Array

	@abstract	Creates a number list string (e.g. "1,2-3,7-9") from an array of numbers. Caller must free string on success.
*/

OSStatus	NumberListStringCreateFromUInt8Array( const uint8_t *inArray, size_t inCount, char **outStr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	RemovePath
	@abstract	Removes a file or directory. If it's a directory, it recursively removes all items inside.
*/

OSStatus	RemovePath( const char *inPath );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	RunningWindowsVistaOrLater
	@abstract	Returns true if we're running Windows Vista or later.
*/

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	RunProcessAndCaptureOutput
	@abstract	Runs a process specified via a command line and captures everything it writes to stdout.
*/

OSStatus	RunProcessAndCaptureOutput( const char *inCmdLine, char **outResponse );
OSStatus	RunProcessAndCaptureOutputEx( const char *inCmdLine, char **outResponse, size_t *outLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	SCDynamicStoreCopyComputerName
	@abstract	Returns a copy of the computer name string.
	
	@param		inStore			Must be NULL.
	@param		outEncoding		Receives the encoding of the string. May be NULL.
*/

	typedef struct SCDynamicStore *		SCDynamicStoreRef;

	CFStringRef	SCDynamicStoreCopyLocalHostName( SCDynamicStoreRef inStore );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	systemf
	@abstract	Invokes a command line built using a format string.
	
	@param		inLogPrefix		Optional prefix to print before logging the command line. If NULL, no logging is performed.
	@param		inFormat		printf-style format string used to build the command line.
	
	@result		If the command line was executed, the exit status of it is returned. Otherwise, errno is returned.
*/

int	systemf( const char *inLogPrefix, const char *inFormat, ... );

#if 0
#pragma mark == Packing/Unpacking ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		Packing/Unpacking
	@abstract	Functions for packing and unpacking data.
*/

OSStatus	PackData( void *inBuffer, size_t inMaxSize, size_t *outSize, const char *inFormat, ... );
OSStatus	PackDataVAList( void *inBuffer, size_t inMaxSize, size_t *outSize, const char *inFormat, va_list inArgs );

OSStatus	UnpackData( const void *inData, size_t inSize, const char *inFormat, ... );
OSStatus	UnpackDataVAList( const void *inData, size_t inSize, const char *inFormat, va_list inArgs );

#if 0
#pragma mark == EDID ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		EDID
	@abstract	Parsing of EDID's.
*/

#define kEDIDKey_CEABlock				CFSTR( "ceaBlock" )
	#define kCEAKey_Revision				CFSTR( "revision" )
#define kEDIDKey_EDIDRevision			CFSTR( "edidRevision" )
#define kEDIDKey_EDIDVersion			CFSTR( "edidVersion" )
#define kEDIDKey_HDMI					CFSTR( "hdmi" )
	#define kHDMIKey_AudioLatencyMs					CFSTR( "audioLatencyMs" )
	#define kHDMIKey_AudioLatencyInterlacedMs		CFSTR( "audioLatencyInterlacedMs" )
	#define kHDMIKey_VideoLatencyMs					CFSTR( "videoLatencyMs" )
	#define kHDMIKey_VideoLatencyInterlacedMs		CFSTR( "videoLatencyInterlacedMs" )
	#define kHDMIKey_SourcePhysicalAddress			CFSTR( "sourcePhysicalAddress" )
#define kEDIDKey_Manufacturer			CFSTR( "manufacturer" )
#define kEDIDKey_MonitorName			CFSTR( "monitorName" )
#define kEDIDKey_MonitorSerialNumber	CFSTR( "monitorSerialNumber" )
#define kEDIDKey_ProductID				CFSTR( "productID" )
#define kEDIDKey_RawBytes				CFSTR( "rawBytes" )
#define kEDIDKey_SerialNumber			CFSTR( "serialNumber" )
#define kEDIDKey_WeekOfManufacture		CFSTR( "weekOfManufacture" )
#define kEDIDKey_YearOfManufacture		CFSTR( "yearOfManufacture" )

CFDictionaryRef	CreateEDIDDictionaryWithBytes( const uint8_t *inData, size_t inSize, OSStatus *outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	H264ConvertAVCCtoAnnexBHeader
	@abstract	Parses a H.264 AVCC atom and converts it to an Annex-B header and gets the nal_size for subsequent frames.
	
	@param	inHeaderBuf			Buffer to receive an Annex-B header containing the AVCC data. May be NULL to just find the size.
	@param	inHeaderMaxLen		Max length of inHeaderBuf. May be zero to just find the size.
	@param	outHeaderLen		Ptr to receive the length of the Annex-B header. May be NULL.
	@param	outNALSize			Ptr to receive the number of bytes in the nal_size field before each AVCC-style frame. May be NULL.
	@param	outNext				Ptr to receive pointer to byte following the last byte that was parsed. May be NULL.
*/
OSStatus
	H264ConvertAVCCtoAnnexBHeader( 
		const uint8_t *		inAVCCPtr,
		size_t				inAVCCLen,
		uint8_t *			inHeaderBuf,
		size_t				inHeaderMaxLen,
		size_t *			outHeaderLen,
		size_t *			outNALSize,
		const uint8_t **	outNext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	H264EscapeEmulationPrevention
	@abstract	Parses the content of an H.264 NAL unit and performs emulation prevention.
	
	@param	inSrc			Ptr to start of a single NAL unit. This must not have a start prefix or NAL size.
	@param	inEnd			Ptr to end of NAL unit data.
	@param	outDataPtr		Ptr to data to write to the stream.
	@param	outDataLen		Number of bytes of data to write. May be 0.
	@param	outSuffixPtr	Ptr to suffix data to write after writing the data from outDataPtr.
	@param	outSuffixLen	Number of suffix bytes to write. May be 0.
	@param	outSrc			Receives ptr to pass as inSrc for the next call.
	
	@result	True if this function needs to be called again. False if all the data has been processed.
	
	@example
	
	while( H264EscapeEmulationPrevention( nalDataPtr, nalDataEnd, &dataPtr, &dataLen, &suffixPtr, &suffixLen, &nalDataPtr ) )
	{
		if( dataLen   > 0 ) fwrite( dataPtr,   1, dataLen,   file );
		if( suffixLen > 0 ) fwrite( suffixPtr, 1, suffixLen, file );
	}
*/
Boolean
	H264EscapeEmulationPrevention( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		const uint8_t **	outDataPtr, 
		size_t *			outDataLen, 
		const uint8_t **	outSuffixPtr, 
		size_t *			outSuffixLen, 
		const uint8_t **	outSrc );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	H264RemoveEmulationPrevention
	@abstract	Parses the content of an H.264 NAL unit and removes emulation prevention bytes.
	
	@param	inSrc			Ptr to start of a single NAL unit. This must not have a start prefix or NAL size.
	@param	inEnd			Ptr to end of NAL unit data.
	@param	inBuf			Buffer to write data with emulation prevention bytes removed. May be the same as inSrc.
	@param	inMaxLen		Max number of bytes allowed to write to inBuf.
	@param	outLen			Number of bytes written to inBuf.
*/
OSStatus
	H264RemoveEmulationPrevention( 
		const uint8_t *	inSrc, 
		size_t			inLen, 
		uint8_t *		inBuf, 
		size_t			inMaxLen, 
		size_t *		outLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	H264GetNextNALUnit
	@abstract	Parses a H.264 AVCC-style stream (NAL size-prefixed NAL units) and returns each piece of NAL unit data.
	
	@param	inSrc			Ptr to start of a single NAL unit. This must begin with a NAL size header.
	@param	inEnd			Ptr to end of the NAL unit.
	@param	outDataPtr		Ptr to data to NAL unit data (minus NAL size header).
	@param	outDataLen		Number of bytes of NAL unit data.
	@param	outSrc			Receives ptr to pass as inSrc for the next call.
*/
OSStatus
	H264GetNextNALUnit( 
		const uint8_t *		inSrc, 
		const uint8_t *		inEnd, 
		size_t				inNALSize, 
		const uint8_t * *	outDataPtr, 
		size_t *			outDataLen,
		const uint8_t **	outSrc );

#if 0
#pragma mark == MirroredRingBuffer ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@struct		MirroredRingBuffer
	@abstract	Structure for managing the ring buffer.
*/

typedef struct
{
	uint8_t *			buffer;			// Buffer backing the ring buffer.
	const uint8_t *		end;			// End of the buffer. Useful for check if a pointer is within the ring buffer.
	uint32_t			size;			// Max number of bytes the ring buffer can hold.
	uint32_t			mask;			// Mask for power-of-2 sized buffers to wrap without using a slower mod operator.
	uint32_t			readOffset;		// Current offset for reading. Don't access directly. Use macros.
	uint32_t			writeOffset;	// Current offset for writing. Don't access directly. Use macros.
	
}	MirroredRingBuffer;

#define kMirroredRingBufferInit		{ NULL, NULL, 0, 0, 0, 0 }

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		MirroredRingBuffer init/free.
	@abstract	Initializes/frees a ring buffer.
*/

OSStatus	MirroredRingBufferInit( MirroredRingBuffer *inRing, size_t inMinSize, Boolean inPowerOf2 );
void		MirroredRingBufferFree( MirroredRingBuffer *inRing );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		MirroredRingBufferAccessors
	@abstract	Macros for accessing the ring buffer.
*/

// Macros for getting read/write pointers for power-of-2 sized ring buffers.
#define MirroredRingBufferGetReadPtr( RING )			( &(RING)->buffer[ (RING)->readOffset  & (RING)->mask ] )
#define MirroredRingBufferGetWritePtr( RING )			( &(RING)->buffer[ (RING)->writeOffset & (RING)->mask ] )

// Macros for getting read/write pointers for non-power-of-2 sized ring buffers.
#define MirroredRingBufferGetReadPtrSlow( RING )		( &(RING)->buffer[ (RING)->readOffset  % (RING)->size ] )
#define MirroredRingBufferGetWritePtrSlow( RING )		( &(RING)->buffer[ (RING)->writeOffset % (RING)->size ] )

#define MirroredRingBufferReadAdvance( RING, COUNT )	do { (RING)->readOffset  += (COUNT); } while( 0 )
#define MirroredRingBufferWriteAdvance( RING, COUNT )	do { (RING)->writeOffset += (COUNT); } while( 0 )
#define MirroredRingBufferReset( RING )					do { (RING)->readOffset = (RING)->writeOffset; } while( 0 )

#define MirroredRingBufferContainsPtr( RING, PTR )		( ( ( (const uint8_t *)(PTR) ) >= (RING)->buffer ) && \
														  ( ( (const uint8_t *)(PTR) ) <  (RING)->end ) )
#define MirroredRingBufferGetBytesUsed( RING )			( (uint32_t)( (RING)->writeOffset - (RING)->readOffset ) )
#define MirroredRingBufferGetBytesFree( RING )			( (RING)->size - MirroredRingBufferGetBytesUsed( RING ) )

#if 0
#pragma mark == Security ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		HasCodeSigningRequirementByPath / HasCodeSigningRequirementByPID
	@abstract	Checks if the path or PID is signed and has the specified requirement. 
*/

#if 0
#pragma mark == Debugging ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MiscUtilsTest
	
	@abstract	Unit test.
*/

#if( DEBUG )
	OSStatus	MiscUtilsTest( void );
#endif

#ifdef __cplusplus
}
#endif

#endif	// __MiscUtils_h__
