/*
	Copyright (C) 2011-2013 Apple Inc. All Rights Reserved.
*/

#include "HTTPMessage.h"

#include "CommonServices.h"
#include "HTTPNetUtils.h"
#include "NetUtils.h"

#include CF_RUNTIME_HEADER
#include LIBDISPATCH_HEADER

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

static void _HTTPMessageGetTypeID( void *inContext );
static void	_HTTPMessageFinalize( CFTypeRef inCF );

//===========================================================================================================================
//	Globals
//===========================================================================================================================

static dispatch_once_t			gHTTPMessageInitOnce = 0;
static CFTypeID					gHTTPMessageTypeID = _kCFRuntimeNotATypeID;
static const CFRuntimeClass		kHTTPMessageClass = 
{
	0,						// version
	"HTTPMessage",			// className
	NULL,					// init
	NULL,					// copy
	_HTTPMessageFinalize,	// finalize
	NULL,					// equal -- NULL means pointer equality.
	NULL,					// hash  -- NULL means pointer hash.
	NULL,					// copyFormattingDesc
	NULL,					// copyDebugDesc
	NULL,					// reclaim
	NULL					// refcount
};

//===========================================================================================================================
//	HTTPMessageGetTypeID
//===========================================================================================================================

CFTypeID	HTTPMessageGetTypeID( void )
{
	dispatch_once_f( &gHTTPMessageInitOnce, NULL, _HTTPMessageGetTypeID );
	return( gHTTPMessageTypeID );
}

static void _HTTPMessageGetTypeID( void *inContext )
{
	(void) inContext;
	
	gHTTPMessageTypeID = _CFRuntimeRegisterClass( &kHTTPMessageClass );
	check( gHTTPMessageTypeID != _kCFRuntimeNotATypeID );
}

//===========================================================================================================================
//	HTTPMessageCreate
//===========================================================================================================================

OSStatus	HTTPMessageCreate( HTTPMessageRef *outMessage )
{
	OSStatus			err;
	HTTPMessageRef		me;
	size_t				extraLen;
	
	extraLen = sizeof( *me ) - sizeof( me->base );
	me = (HTTPMessageRef) _CFRuntimeCreateInstance( NULL, HTTPMessageGetTypeID(), (CFIndex) extraLen, NULL );
	require_action( me, exit, err = kNoMemoryErr );
	memset( ( (uint8_t *) me ) + sizeof( me->base ), 0, extraLen );
	
	me->maxBodyLen = kHTTPDefaultMaxBodyLen;
	HTTPMessageReset( me );
	
	*outMessage = me;
	me = NULL;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	_HTTPMessageFinalize
//===========================================================================================================================

static void	_HTTPMessageFinalize( CFTypeRef inCF )
{
	HTTPMessageRef const		me = (HTTPMessageRef) inCF;
	
	HTTPMessageReset( me );
}

//===========================================================================================================================
//	HTTPMessageReset
//===========================================================================================================================

void	HTTPMessageReset( HTTPMessageRef inMsg )
{
	inMsg->header.len	= 0;
	inMsg->headerRead	= false;
	inMsg->bodyPtr		= inMsg->smallBodyBuf;
	inMsg->bodyLen		= 0;
	inMsg->bodyOffset	= 0;
	inMsg->timeoutNanos	= kHTTPNoTimeout;
	ForgetMem( &inMsg->bigBodyBuf );
}

//===========================================================================================================================
//	HTTPMessageReadMessage
//===========================================================================================================================

OSStatus	HTTPMessageReadMessage( SocketRef inSock, HTTPMessageRef inMsg )
{
	HTTPHeader * const		hdr = &inMsg->header;
	OSStatus				err;
	size_t					len;
	
	if( !inMsg->headerRead )
	{
		err = SocketReadHTTPHeader( inSock, hdr );
		require_noerr_quiet( err, exit );
		inMsg->headerRead = true;
		
		require_action( hdr->contentLength <= inMsg->maxBodyLen, exit, err = kSizeErr );
		err = HTTPMessageSetBodyLength( inMsg, (size_t) hdr->contentLength );
		require_noerr( err, exit );
	}
	
	len = hdr->extraDataLen;
	if( len > 0 )
	{
		len = Min( len, inMsg->bodyLen );
		memmove( inMsg->bodyPtr, hdr->extraDataPtr, len );
		hdr->extraDataPtr += len;
		hdr->extraDataLen -= len;
		inMsg->bodyOffset += len;
	}
	
	err = SocketReadData( inSock, inMsg->bodyPtr, inMsg->bodyLen, &inMsg->bodyOffset );
	require_noerr_quiet( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	HTTPMessageSetBody
//===========================================================================================================================

OSStatus	HTTPMessageSetBody( HTTPMessageRef inMsg, const char *inContentType, const void *inData, size_t inLen )
{
	OSStatus		err;
	
	err = inMsg->header.firstErr;
	require_noerr( err, exit );
	
	HTTPMessageSetBodyLength( inMsg, inLen );
	if( inData && ( inData != inMsg->bodyPtr ) )  // Handle inData pointing to the buffer.
	{
		memmove( inMsg->bodyPtr, inData, inLen ); // memmove in case inData is in the middle of the buffer.
	}
	
	HTTPHeader_SetField( &inMsg->header, "Content-Length", "%zu", inLen );
	if( inContentType ) HTTPHeader_SetField( &inMsg->header, "Content-Type", inContentType );
	err = kNoErr;
	
exit:
	if( err && !inMsg->header.firstErr ) inMsg->header.firstErr = err;
	return( err );
}

//===========================================================================================================================
//	HTTPMessageSetBodyLength
//===========================================================================================================================

OSStatus	HTTPMessageSetBodyLength( HTTPMessageRef inMsg, size_t inLen )
{
	OSStatus		err;
	
	ForgetMem( &inMsg->bigBodyBuf );
	if( inLen <= sizeof( inMsg->smallBodyBuf ) )
	{
		inMsg->bodyPtr = inMsg->smallBodyBuf;
	}
	else
	{
		inMsg->bigBodyBuf = (uint8_t *) malloc( inLen );
		require_action( inMsg->bigBodyBuf, exit, err = kNoMemoryErr );
		inMsg->bodyPtr = inMsg->bigBodyBuf;
	}
	inMsg->bodyLen = inLen;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	HTTPMessageSetCompletionBlock
//===========================================================================================================================

