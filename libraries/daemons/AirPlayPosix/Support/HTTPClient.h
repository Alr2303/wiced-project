/*
	Copyright (C) 2011-2013 Apple Inc. All Rights Reserved.
*/

#ifndef	__HTTPClient_h__
#define	__HTTPClient_h__

#include "CommonServices.h"
#include "HTTPMessage.h"

#include LIBDISPATCH_HEADER

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPClientCreate
	@abstract	Creates a new HTTP client.
*/
typedef struct HTTPClientPrivate *		HTTPClientRef;

CFTypeID	HTTPClientGetTypeID( void );
OSStatus	HTTPClientCreate( HTTPClientRef *outClient );
OSStatus	HTTPClientCreateWithSocket( HTTPClientRef *outClient, SocketRef inSock );
#define 	HTTPClientForget( X ) do { if( *(X) ) { HTTPClientInvalidate( *(X) ); CFRelease( *(X) ); *(X) = NULL; } } while( 0 )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPClientInvalidate
	@abstract	Cancels all outstanding operations.
*/
void	HTTPClientInvalidate( HTTPClientRef inClient );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPClientGetPeerAddress
	@abstract	Gets the address of the connected peer.
	@discussion	Only valid after a connection has been established.
*/
OSStatus	HTTPClientGetPeerAddress( HTTPClientRef inClient, void *inSockAddr, size_t inMaxLen, size_t *outLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPClientSetDestination
	@abstract	Sets the destination hostname, IP address, URL, etc. of the HTTP server to talk to.
	@discussion	Note: this cannot be changed once set.
*/
OSStatus	HTTPClientSetDestination( HTTPClientRef inClient, const char *inDestination, int inDefaultPort );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPClientSetDispatchQueue
	@abstract	Sets the GCD queue to perform all operations on.
	@discussion	Note: this cannot be changed once operations have started.
*/
void	HTTPClientSetDispatchQueue( HTTPClientRef inClient, dispatch_queue_t inQueue );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPClientSetFlags
	@abstract	Enables or disables P2P connections.
*/
typedef uint32_t		HTTPClientFlags;
#define kHTTPClientFlag_None					0
#define kHTTPClientFlag_P2P						( 1 << 0 ) // Enable P2P connections.
#define kHTTPClientFlag_SuppressUnusable		( 1 << 1 ) // Suppress trying to connect on seemingly unusable interfaces.
#define kHTTPClientFlag_Reachability			( 1 << 2 ) // Use the reachability APIs before trying to connect.
#define kHTTPClientFlag_BoundInterface			( 1 << 3 ) // Set bound interface before connect if interface index available.

void	HTTPClientSetFlags( HTTPClientRef inClient, HTTPClientFlags inFlags, HTTPClientFlags inMask );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPClientSetLogging
	@abstract	Sets the log category to use for HTTP message logging.
*/
void	HTTPClientSetLogging( HTTPClientRef inClient, LogCategory *inLogCategory );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPClientSetP2P
	@abstract	Enables or disables P2P connections.
*/
void	HTTPClientSetP2P( HTTPClientRef inClient, Boolean inAllowP2P );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	HTTPClientSendMessage
	@abstract	Sends an HTTP message.
*/
OSStatus	HTTPClientSendMessage( HTTPClientRef inClient, HTTPMessageRef inMsg );
OSStatus	HTTPClientSendMessageSync( HTTPClientRef inClient, HTTPMessageRef inMsg );

#ifdef __cplusplus
}
#endif

#endif // __HTTPClient_h__
