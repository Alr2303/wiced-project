/*
	Copyright (C) 2008 Apple Inc. All Rights Reserved.
*/

#ifndef	__HTTPNetUtilsDotH__
#define	__HTTPNetUtilsDotH__

#include "CommonServices.h"
#include "DebugServices.h"
#include "HTTPUtils.h"
#include "NetUtils.h"

#ifdef	__cplusplus
	extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPDownloadFile
	@abstract	Downloads a file via HTTP into a malloc'd buffer.
*/

OSStatus	HTTPDownloadFile( const char *inURL, size_t inMaxSize, char **outData, size_t *outSize );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPReadHeader
	@abstract	Reads an HTTP header (start line, all header fields, etc.) into the specified buffer in a non-blocking manner.
	@discussion
	
	This may need to be called multiple times to read the entire header. Callers should initialize the offset to 0 then 
	call this function each time the socket is readable until it returns one of the following results:

	kNoErr:
		The header was read successfully into the buffer and null terminated. *ioOffset is the header size, 
		including a blank line, if needed (e.g. RTSP interleaved binary data does not need a blank line).
	
	EWOULDBLOCK:
		There was not enough data immediately available in the socket buffer to completely read the header.
		The caller should call this function again with the same buffer and offset when more data is available.
	
	kNoSpaceErr:
		The header could not be read completely because the buffer was not big enough. The caller can resize the 
		buffer and call this function again with an updated buffer size. If the buffer is resized, the existing 
		contents and the offset must remain the same or this function may not work correctly. Unbounded resizing 
		should be used with caution because it could lead to a denial of service attack due to resource exhaustion.
	
	Any other error:
		The header could not be read because of an error. The socket should be closed.
*/

OSStatus	HTTPReadHeader( SocketRef inSock, void *inBuffer, size_t inBufferSize, size_t *ioOffset );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	NetSocket_HTTPReadHeader
	@abstract	Reads an HTTP header and parses it.
	@discussion
	
	This function blocks until it reads the complete header, a timeout occurs (if the timeout is >= 0), or the NetSocket
	is canceled. If it returns kNoErr, the HTTPHeader will be fully initialized and parsed. No pre-initialization needed.
	
	WARNING: This associates the HTTPHeader's buffer as part of the NetSocket structure so even after this call returns.
	This is used to handle leftover body data since the internal read may have read more than just the header.
	Subsequent calls to NetSocket_Read() will read from that leftover data until it is consumed.
*/

OSStatus	NetSocket_HTTPReadHeader( NetSocketRef inSock, HTTPHeader *inHeader, int32_t inTimeoutSecs );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	SocketReadHTTPHeader
	@abstract	Reads an HTTP header (start line, all header fields, etc.) into the specified buffer in a non-blocking manner.
	@discussion
	
	This may need to be called multiple times to read the entire header. Callers should initialize the inHeader->len to 0 
	then call this function each time the socket is readable, until it returns one of the following results:
	
	kNoErr:
		The header was read successfully into the buffer and fully parsed.
		Any extra data that was read, but not used is tracked by inHeader->extraDataPtr and inHeader->extraDataLen.
	
	EWOULDBLOCK:
		There was not enough data available to completely read the header.
		The caller should call this function again with same header when more data is available.
	
	kNoSpaceErr:
		The header could not be read completely because the buffer was not big enough. The header is probably bad.
	
	Any other error:
		The header could not be read because of an error. The socket should be closed.
*/

OSStatus	SocketReadHTTPHeader( SocketRef inSock, HTTPHeader *inHeader );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPNetUtils_Test	
	@abstract	Unit test.
*/

#ifdef	__cplusplus
	}
#endif

#endif	// __HTTPNetUtilsDotH__
