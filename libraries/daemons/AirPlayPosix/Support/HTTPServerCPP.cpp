/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/

#include "HTTPServerCPP.hpp"

#include "CommonServices.h"	// Include early so we can conditionalize on TARGET_* flags.
#include "DebugServices.h"	// Include early so we can conditionalize on DEBUG flags.

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <new>

	#include <fcntl.h>
	#include <netinet/tcp.h>
	#include <sys/uio.h>

#include "dns_sd.h"
#include "HTTPNetUtils.h"
#include "HTTPUtils.h"
#include "NetUtils.h"
#include "StringUtils.h"
#include "TimeUtils.h"

#include CF_HEADER

using CoreUtils::HTTPMessageCPP;
using CoreUtils::HTTPConnection;
using CoreUtils::HTTPServerCPP;

#if 0
#pragma mark == HTTPServerCPP ==
#endif

//===========================================================================================================================
//	HTTPServerCPP
//===========================================================================================================================

HTTPServerCPP::HTTPServerCPP( void ) :
	mConnectionList( NULL ), 
	mListenerSock( kInvalidSocketRef ), 
	mListenerSockReceiveBufferSize( 0 ), 
	mListenerCFSock( NULL ), 
	mListenPort( 0 ), 
	mListeningPort( 0 ), 
	mAllowP2P( false )
{
}

//===========================================================================================================================
//	~HTTPServerCPP
//===========================================================================================================================

HTTPServerCPP::~HTTPServerCPP( void )
{
	// Because you can't call subclass functions from a base class destructor and some of the variables may need 
	// subclass-specific cleanup, this object must be cleaned up before the destructor is called.
	
	check( mConnectionList == NULL );
	check( mListenerSock == kInvalidSocketRef );
	check( mListenerCFSock == NULL );
}

//===========================================================================================================================
//	Cleanup
//===========================================================================================================================

void	HTTPServerCPP::Cleanup( void )
{
	RemoveAllConnections();
	if( mListenerCFSock )
	{
		CFSocketInvalidate( mListenerCFSock );
		CFRelease( mListenerCFSock );
		mListenerCFSock = NULL;
	}
	mListenerSock = kInvalidSocketRef;
}

//===========================================================================================================================
//	Delete
//===========================================================================================================================

void	HTTPServerCPP::Delete( void )
{
	Cleanup();
	delete this;
}

//===========================================================================================================================
//	Start
//===========================================================================================================================

OSStatus	HTTPServerCPP::Start( void )
{
	OSStatus		err;
	
#if( defined( AF_INET6 ) )
	err = SetupListenerSocketV6();
	if( err )
#endif
	{
		err = SetupListenerSocketV4();
	}
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	SetupListenerSocketV4
//===========================================================================================================================

OSStatus	HTTPServerCPP::SetupListenerSocketV4( void )
{
	OSStatus				err;
	SocketRef				sock;
	int						option;
	int						port;
	struct sockaddr_in		sa4;
	socklen_t				len;
	CFSocketRef				cfSock;
	CFSocketContext			socketContext = { 0, NULL, NULL, NULL, NULL };
	CFRunLoopSourceRef		source;
	
	cfSock = NULL;
	
	sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	err = map_socket_creation_errno( sock );
	require_noerr( err, exit );
	mListenerSock = sock;
	
	if( mAllowP2P ) SocketSetP2P( sock, true );
	
	// Set receive buffer size. This has to be done on the listening socket *before* listen is called because
	// accept does not return until after the window scale option is exchanged during the 3-way handshake. 
	// Since accept returns a new socket, the only way to use a larger window scale option is to set the buffer
	// size on the listening socket since SO_RCVBUF is inherited by the accepted socket. See UNPv1e3 Section 7.5.
	
	if( mListenerSockReceiveBufferSize > 0 )
	{
		option = mListenerSockReceiveBufferSize;
		err = setsockopt( sock, SOL_SOCKET, SO_RCVBUF, (char *) &option, (socklen_t) sizeof( option ) );
		err = map_socket_noerr_errno( sock, err );
		require_noerr( err, exit );
	}
	
	// Turn on SO_REUSEADDR to allow the server to be restarted (see UNPv1e3 Section 7.5).
	
	option = 1;
	err = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &option, (socklen_t) sizeof( option ) );
	err = map_socket_noerr_errno( sock, err );
	require_noerr( err, exit );
	
	// Bind to the wildcard address to allow connections on all interfaces.
	
	port = ( mListenPort < 0 ) ? -mListenPort : mListenPort; // Negated port means "try this port, but allow dynamic".
	
	memset( &sa4, 0, sizeof( sa4 ) );
	SIN_LEN_SET( &sa4 );
	sa4.sin_family		= AF_INET;
	sa4.sin_port		= htons( (uint16_t) port );
	sa4.sin_addr.s_addr	= htonl( INADDR_ANY );
	err = bind( sock, (const struct sockaddr *) &sa4, (socklen_t) sizeof( sa4 ) );
	err = map_socket_noerr_errno( sock, err );
	if( err && ( mListenPort < 0 ) )
	{
		sa4.sin_port = 0;
		err = bind( sock, (const struct sockaddr *) &sa4, (socklen_t) sizeof( sa4 ) );
		err = map_socket_noerr_errno( sock, err );
	}
	require_noerr( err, exit );
	
	len = sizeof( sa4 );
	err = getsockname( sock, (struct sockaddr *) &sa4, &len );
	err = map_socket_noerr_errno( sock, err );
	require_noerr( err, exit );
	
	mListeningPort = ntohs( sa4.sin_port );
	
	// Make the socket non-blocking to avoid a race condition with a TCP RST before accept (see UNPv1e3 Section 16.6).
	
	err = SocketMakeNonBlocking( sock );
	require_noerr( err, exit );
	
	// Start listening for connections on this socket.
	
	err = listen( sock, SOMAXCONN );
	err = map_socket_noerr_errno( sock, err );
	if( err )
	{
		err = listen( sock, 5 );
		err = map_socket_noerr_errno( sock, err );
		require_noerr( err, exit );
	}
	
	// Associate the socket with the runloop for async notification. If this succeeds, the CFSocket owns the native socket.
	
	socketContext.info = this;
	cfSock = CFSocketCreateWithNative( NULL, sock, kCFSocketReadCallBack, ListenerSocketCallBack, &socketContext );
	require_action( cfSock, exit, err = kNoMemoryErr );
	sock = kInvalidSocketRef;
	
	source = CFSocketCreateRunLoopSource( NULL, cfSock, 0 );
	require_action( source, exit, err = kNoMemoryErr );
	
	CFRunLoopAddSource( CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes );
	CFRelease( source );
	
	mListenerCFSock	= cfSock;
	cfSock = NULL;
	
exit:
	ForgetCF( &cfSock );
	ForgetSocket( &sock );
	return( err );
}

//===========================================================================================================================
//	SetupListenerSocketV6
//===========================================================================================================================

#if( defined( AF_INET6 ) )
OSStatus	HTTPServerCPP::SetupListenerSocketV6( void )
{
	OSStatus				err;
	SocketRef				sock;
	int						option;
	int						port;
	struct sockaddr_in6		sa6;
	socklen_t				len;
	CFSocketRef				cfSock;
	CFSocketContext			socketContext = { 0, NULL, NULL, NULL, NULL };
	CFRunLoopSourceRef		source;
	
	cfSock = NULL;
	
	sock = socket( AF_INET6, SOCK_STREAM, IPPROTO_TCP );
	err = map_errno( IsValidSocket( sock ) );
	require_noerr_quiet( err, exit ); // Quiet because IPv6 may not be supported.
	mListenerSock = sock;
	
	if( mAllowP2P ) SocketSetP2P( sock, true );
	
	// Turn off IPV6_V6ONLY to allow IPv4-mapped connections. Some stacks default to IPv6 only.
	
	option = 0;
	err = setsockopt( sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &option, (socklen_t) sizeof( option ) );
	err = map_socket_noerr_errno( sock, err );
	require_noerr( err, exit );
	
	// Set receive buffer size. This has to be done on the listening socket *before* listen is called because
	// accept does not return until after the window scale option is exchanged during the 3-way handshake. 
	// Since accept returns a new socket, the only way to use a larger window scale option is to set the buffer
	// size on the listening socket since SO_RCVBUF is inherited by the accepted socket. See UNPv1e3 Section 7.5.
	
	if( mListenerSockReceiveBufferSize > 0 )
	{
		option = mListenerSockReceiveBufferSize;
		err = setsockopt( sock, SOL_SOCKET, SO_RCVBUF, (char *) &option, (socklen_t) sizeof( option ) );
		err = map_socket_noerr_errno( sock, err );
		require_noerr( err, exit );
	}
	
	// Turn on SO_REUSEADDR to allow the server to be restarted (see UNPv1e3 Section 7.5).
	
	option = 1;
	err = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &option, (socklen_t) sizeof( option ) );
	err = map_socket_noerr_errno( sock, err );
	require_noerr( err, exit );
	
	// Bind to the wildcard address to allow connections on all interfaces.
	
	port = ( mListenPort < 0 ) ? -mListenPort : mListenPort; // Negated port means "try this port, but allow dynamic".
	
	memset( &sa6, 0, sizeof( sa6 ) );
	SIN6_LEN_SET( &sa6 );
	sa6.sin6_family	= AF_INET6;
	sa6.sin6_port	= htons( (uint16_t) port );
	sa6.sin6_addr	= in6addr_any;
	err = bind( sock, (const struct sockaddr *) &sa6, (socklen_t) sizeof( sa6 ) );
	err = map_socket_noerr_errno( sock, err );
	if( err && ( mListenPort < 0 ) )
	{
		sa6.sin6_port = 0;
		err = bind( sock, (const struct sockaddr *) &sa6, (socklen_t) sizeof( sa6 ) );
		err = map_socket_noerr_errno( sock, err );
	}
	require_noerr( err, exit );
	
	len = sizeof( sa6 );
	err = getsockname( sock, (struct sockaddr *) &sa6, &len );
	err = map_socket_noerr_errno( sock, err );
	require_noerr( err, exit );
	
	mListeningPort = ntohs( sa6.sin6_port );
	
	// Make the socket non-blocking to avoid a race condition with a TCP RST before accept (see UNPv1e3 Section 16.6).
	
	err = SocketMakeNonBlocking( sock );
	require_noerr( err, exit );
	
	// Start listening for connections on this socket.
	
	err = listen( sock, SOMAXCONN );
	err = map_socket_noerr_errno( sock, err );
	if( err )
	{
		err = listen( sock, 5 );
		err = map_socket_noerr_errno( sock, err );
		require_noerr( err, exit );
	}
	
	// Associate the socket with the runloop for async notification. If this succeeds, the CFSocket owns the native socket.
	
	socketContext.info = this;
	cfSock = CFSocketCreateWithNative( NULL, sock, kCFSocketReadCallBack, ListenerSocketCallBack, &socketContext );
	require_action( cfSock, exit, err = kNoMemoryErr );
	sock = kInvalidSocketRef;
	
	source = CFSocketCreateRunLoopSource( NULL, cfSock, 0 );
	require_action( source, exit, err = kNoMemoryErr );
	
	CFRunLoopAddSource( CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes );
	CFRelease( source );
	
	mListenerCFSock	= cfSock;
	cfSock = NULL;
	
exit:
	ForgetCF( &cfSock );
	ForgetSocket( &sock );
	return( err );
}
#endif // defined( AF_INET6 ) )

//===========================================================================================================================
//	ListenerSocketCallBack [static]
//===========================================================================================================================

void
	HTTPServerCPP::ListenerSocketCallBack( 
		CFSocketRef				inSock, 
		CFSocketCallBackType	inType, 
		CFDataRef				inAddr, 
		const void *			inData, 
		void *					inContext )
{
	HTTPServerCPP &			me = *( reinterpret_cast < HTTPServerCPP * > ( inContext ) );
	OSStatus				err;
	SocketRef				newSock;
	HTTPConnection *		connection = NULL;
	
	(void) inSock; // Unused
	(void) inType; // Unused
	(void) inAddr; // Unused
	(void) inData; // Unused
	
	newSock = accept( me.mListenerSock, NULL, NULL );
	err = map_socket_creation_errno( newSock );
	if( err && SocketIsDefunct( me.mListenerSock ) )
	{
		dlog( kDebugLevelNotice, "### HTTP server listener socket became defunct\n" );
		me.Cleanup();
		goto exit;
	}
	require_noerr( err, exit );
	
	err = me.CreateConnection( &connection );
	require_noerr( err, exit );
	
	err = connection->Accept( newSock );
	require_noerr( err, exit );
	
	connection->mNextConnection = me.mConnectionList;
	me.mConnectionList = connection;
	
	newSock = kInvalidSocketRef;
	connection = NULL;
	
exit:
	ForgetSocket( &newSock );
	if( connection ) me.DeleteConnection( connection );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	CreateConnection
//===========================================================================================================================

OSStatus	HTTPServerCPP::CreateConnection( HTTPConnection **outConnection )
{
	OSStatus				err;
	HTTPConnection *		obj;
	
	obj = NEW HTTPConnection( *this );
	require_action( obj, exit, err = kNoMemoryErr );
	
	*outConnection = obj;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DeleteConnection
//===========================================================================================================================

OSStatus	HTTPServerCPP::DeleteConnection( HTTPConnection *inConnection )
{
	delete inConnection;
	return( kNoErr );
}

//===========================================================================================================================
//	RemoveConnection
//===========================================================================================================================

OSStatus	HTTPServerCPP::RemoveConnection( HTTPConnection *inConnection )
{
	OSStatus				err;
	HTTPConnection **		p;
	HTTPConnection *		c;
	
	for( p = &mConnectionList; ( ( c = *p ) != NULL ) && ( c != inConnection ); p = &c->mNextConnection ) {}
	require_action( c, exit, err = kNotFoundErr );
	*p = c->mNextConnection;
	
	err = DeleteConnection( c );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	RemoveAllConnections
//===========================================================================================================================

void	HTTPServerCPP::RemoveAllConnections( HTTPConnection *inConnectionToKeep )
{
	HTTPConnection **		next;
	HTTPConnection *		conn;
	
	next = &mConnectionList;
	while( ( conn = *next ) != NULL )
	{
		if( conn == inConnectionToKeep )
		{
			next = &conn->mNextConnection;
			continue;
		}
		
		*next = conn->mNextConnection;
		DeleteConnection( conn );
	}
	
	check( mConnectionList == inConnectionToKeep );
	check( !mConnectionList || ( mConnectionList->mNextConnection == NULL ) );
}

#if 0
#pragma mark -
#pragma mark == HTTPConnection ==
#endif

//===========================================================================================================================
//	HTTPConnection
//===========================================================================================================================

HTTPConnection::HTTPConnection( HTTPServerCPP &inServer ) :
	mServer( inServer ), 
	mNextConnection( NULL ), 
	mSock( kInvalidSocketRef ), 
	mCFSock( NULL ), 
	mState( kHTTPConnectionState_ReadingHeader ), 
	mReadBuffer( NULL ), 
	mReadBufferSize( 32000 ),
	mResponse( NULL ), 
	mWriteIOPtr( NULL ), 
	mWriteIOCount( 0 )
{
	//
}

//===========================================================================================================================
//	~HTTPConnection
//===========================================================================================================================

HTTPConnection::~HTTPConnection( void )
{
	if( mCFSock )
	{
		CFSocketInvalidate( mCFSock );
		CFRelease( mCFSock );
		mCFSock = NULL;
	}
	mSock = kInvalidSocketRef;
	
	ForgetMem( &mReadBuffer );
	
	// Because you can't call subclass functions from a base class destructor and some of the variables may need 
	// subclass-specific cleanup, this object must be cleaned up before the destructor is called.
	
	check( mSock == kInvalidSocketRef );
	check( mCFSock == NULL );
	check( mReadBuffer == NULL );
	check( mResponse == NULL );
}

//===========================================================================================================================
//	Accept
//===========================================================================================================================

OSStatus	HTTPConnection::Accept( SocketRef inSock )
{
	OSStatus				err;
	int						option;
	CFSocketContext			socketContext = { 0, NULL, NULL, NULL, NULL };
	CFRunLoopSourceRef		source;
	socklen_t				len;
	
	// Set up buffering. $$$ TO DO: make this easier to config by subclasses. 
	
	require_action( mReadBufferSize > 0, exit, err = kSizeErr );
	mReadBuffer = (uint8_t *) malloc( mReadBufferSize );
	require_action( mReadBuffer, exit, err = kNoMemoryErr );
	
	// Make the socket non-blocking in case its parent's non-blocking state wasn't inherited properly.
	
	err = SocketMakeNonBlocking( inSock );
	require_noerr( err, exit );
	
	// Disable nagle so responses we send are not delayed. We coalesce writes to minimize small writes anyway.
	
	option = 1;
	err = setsockopt( inSock, IPPROTO_TCP, TCP_NODELAY, (char *) &option, (socklen_t) sizeof( option ) );
	err = map_socket_noerr_errno( inSock, err );
	if( err ) dlog( kDebugLevelWarning, "%###s: ### set TCP_NODELAY failed on sock %d: %#m\n", __ROUTINE__, (int) inSock, err );
	
	// Save off connection info.
	
	len = sizeof( mSelfAddrRaw );
	err = getsockname( inSock, &mSelfAddrRaw.sa, &len );
	err = map_socket_noerr_errno( inSock, err );
	check_noerr( err );
	
	err = SockAddrSimplify( &mSelfAddrRaw, &mSelfAddr );
	check_noerr( err );
	
	len = sizeof( mPeerAddrRaw );
	err = getpeername( inSock, &mPeerAddrRaw.sa, &len );
	err = map_socket_noerr_errno( inSock, err );
	check_noerr( err );
	
	err = SockAddrSimplify( &mPeerAddrRaw, &mPeerAddr );
	check_noerr( err );
	
	*mIfName = '\0';
	mIfFlags = 0;
	mIfExtendedFlags = 0;
	mTransportType = kNetTransportType_Undefined;
	SocketGetInterfaceInfo( inSock, NULL, mIfName, &mIfIndex, NULL, &mIfMedia, &mIfFlags, &mIfExtendedFlags, &mTransportType );
	if( !NetTransportTypeIsP2P( mTransportType ) ) SocketSetP2P( inSock, false ); // Clear if P2P was inherited.
	
	// Associate the socket with the runloop for async notification. If this succeeds, the CFSocket owns the native socket.
	// Note: this says we want write callbacks then disables them because we don't want them initially, but if we don't 
	// create it with them enabled, CF will never give us write callbacks, even if we enable them later.
	
	socketContext.info = this;
	mCFSock = CFSocketCreateWithNative( NULL, inSock, kCFSocketReadCallBack | kCFSocketWriteCallBack, 
		SocketCallBack, &socketContext );
	require_action( mCFSock, exit, err = kNoMemoryErr );
	
	CFSocketDisableCallBacks( mCFSock, kCFSocketWriteCallBack );
	
	source = CFSocketCreateRunLoopSource( NULL, mCFSock, 0 );
	require_action( source, exit, err = kNoMemoryErr );
	
	CFRunLoopAddSource( CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes );
	CFRelease( source );
	
	mSock = inSock;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	SocketCallBack [static]
//===========================================================================================================================

void
	HTTPConnection::SocketCallBack( 
		CFSocketRef				inSock, 
		CFSocketCallBackType	inType, 
		CFDataRef				inAddr,
		const void *			inData,
		void *					inContext )
{
	HTTPConnection &		me = *( reinterpret_cast < HTTPConnection * > ( inContext ) );
	
	(void) inSock; // Unused
	(void) inType; // Unused
	(void) inAddr; // Unused
	(void) inData; // Unused
	
	me.RunStateMachine();
}

//===========================================================================================================================
//	RunStateMachine
//===========================================================================================================================

OSStatus	HTTPConnection::RunStateMachine( void )
{
	OSStatus		err;
	
	for( ;; )
	{
		if( mState == kHTTPConnectionState_ReadingHeader )
		{
			err = HTTPReadHeader( mSock, mRequest.mHeaderBuffer, sizeof( mRequest.mHeaderBuffer ), &mRequest.mHeaderSize );
			if( err == EWOULDBLOCK )	break;
			if( err == kConnectionErr )	goto exit;
			require_noerr( err, exit );
			
			//dlog( kDebugLevelVerbose, "========== HTTP REQUEST ==========\n" );
			//dlog( kDebugLevelVerbose, "%s", mRequest.mHeaderBuffer );
			
			err = mRequest.ParseHeader();
			require_noerr( err, exit );
			
			if( mRequest.mContentLength > mReadBufferSize )
			{
				uint8_t *		buf;
				
				buf = (uint8_t *) malloc( mRequest.mContentLength );
				require_action( buf, exit, err = kNoMemoryErr );
				
				if( mReadBuffer ) free( mReadBuffer );
				mReadBuffer = buf;
				mReadBufferSize = mRequest.mContentLength;
			}
			mRequest.mBodyBuffer		= mReadBuffer;
			mRequest.mBodyBufferSize	= mReadBufferSize;
			mRequest.mBodyBufferUsed	= 0;
			mRequest.mBodySize			= mRequest.mContentLength;
			
			mState = kHTTPConnectionState_ReadingBody;
						
			err = ProcessRequestHeaders();
			require_noerr_quiet( err, exit );
		}
		else if( mState == kHTTPConnectionState_ReadingBody )
		{
			require_action( mRequest.mBodySize <= mRequest.mBodyBufferSize, exit, err = kNoSpaceErr );
			err = SocketReadData( mSock, mRequest.mBodyBuffer, mRequest.mBodySize, &mRequest.mBodyBufferUsed );
			if( err == EWOULDBLOCK ) break;
			require_noerr( err, exit );
			
			mState = kHTTPConnectionState_ProcessingRequest;
			
			err = ProcessRequest();
			goto exit;
		}
		else if( mState == kHTTPConnectionState_ProcessingRequest )
		{
			// Nothing to do until the delegate starts sending the response so hold off read callbacks.
			
			CFSocketDisableCallBacks( mCFSock, kCFSocketReadCallBack );
			break;
		}
		else if( mState == kHTTPConnectionState_WritingResponse )
		{
			err = WriteResponse();
			if( err == EWOULDBLOCK ) break;
			if( err == EPIPE )		 goto exit;
			require_noerr( err, exit );
			break;
		}
		else if( mState == kHTTPConnectionState_WaitingForClose )
		{
			uint8_t		buf[ 16 ];
			ssize_t		n;
			
			n = recv( mSock, (char *) buf, sizeof( buf ), 0 );
			if( n > 0 )
			{
				dlog( kDebugLevelError, "[%s] %###s: ### recv'd on close: %#H\n", __PROGRAM__, __ROUTINE__, buf, (int) n, 16 );
			}
			else if( n < 0 )
			{
				err = socket_value_errno( mSock, n );
				dlog( kDebugLevelError, "[%s] %###s: ### recv error on close: %#m\n", __PROGRAM__, __ROUTINE__, err );
			}
			
			err = mServer.RemoveConnection( this );
			check_noerr( err );
			break;
		}
		else
		{
			dlog( kDebugLevelError, "[%s] %###s: ### unexpected state: %d\n", __PROGRAM__, __ROUTINE__, mState );
			err = kExecutionStateErr;
			goto exit;
		}
	}
	err = kNoErr;
	
exit:
	if( err ) mServer.RemoveConnection( this );
	return( err );
}

//===========================================================================================================================
//	StartResponse
//===========================================================================================================================

OSStatus	HTTPConnection::StartResponse( HTTPMessageCPP *inResponse )
{
	OSStatus			err;
	struct iovec *		iov;
	
	require_action( inResponse->mHeaderSize > 0, exit, err = kNotPreparedErr );
	require_action( inResponse->mHeaderSize < sizeof( inResponse->mHeaderBuffer ), exit, err = kSizeErr );
	require_action( inResponse->mHeaderBuffer[ inResponse->mHeaderSize ] == '\0', exit, err = kMalformedErr );
	require_action( mState == kHTTPConnectionState_ProcessingRequest, exit, err = kStateErr );
	require_action( mResponse == NULL, exit, err = kAlreadyInUseErr );
	
	mResponse	= inResponse;
	mState		= kHTTPConnectionState_WritingResponse;
	
	// Write the header and body data together. This coalesces packets to reduce TCP delayed ACK issues.
	
	iov = mWriteIOArray;
	mWriteIOPtr = iov;
	iov[ 0 ].iov_base = inResponse->mHeaderBuffer;
	iov[ 0 ].iov_len  = (unsigned int) inResponse->mHeaderSize;
	mWriteIOCount = 1;
	
	if( inResponse->mBodySize > 0 )
	{
		iov[ 1 ].iov_base = (char *) inResponse->mBodyBuffer;
		iov[ 1 ].iov_len  = (unsigned int) inResponse->mBodySize;
		mWriteIOCount = 2;
	}
	
	err = WriteResponse();
	if( err == EWOULDBLOCK )	err = kNoErr;
	if( err == EPIPE )			goto exit;
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	WriteResponse
//===========================================================================================================================

OSStatus	HTTPConnection::WriteResponse( void )
{
	OSStatus		err;
	
	check( mState == kHTTPConnectionState_WritingResponse );
	
	for( ;; )
	{
		err = SocketWriteData( mSock, &mWriteIOPtr, &mWriteIOCount );
		if( err == EWOULDBLOCK )
		{
			// Write callbacks are auto-disabled after each callback so re-enable to get a callback when we can send again.
			
			CFSocketEnableCallBacks( mCFSock, kCFSocketWriteCallBack );
			break;
		}
		if( err == EPIPE ) goto exit;
		require_noerr( err, exit );
		
		// Write completed successfully. If the message supplies more data then start writing the next chunk.
		
		mResponse->mBodySize = 0;
			
		err = mResponse->SupplyData();
		require_noerr( err, exit );
		
		if( mResponse->mBodySize > 0 )
		{
			mWriteIOArray[ 0 ].iov_base = (char *) mResponse->mBodyBuffer;
			mWriteIOArray[ 0 ].iov_len  = (unsigned int) mResponse->mBodySize;
			mWriteIOPtr   = mWriteIOArray;
			mWriteIOCount = 1;
			continue;
		}
		
		// The response has been completely sent. 
		
		ProcessResponseDone();
		
		if( mRequest.mPersistent )
		{
			mState = kHTTPConnectionState_ReadingHeader;
		}
		else
		{
			// Signal to the peer that we're done writing then wait for the peer to close.
			
			err = shutdown( mSock, SHUT_WR_COMPAT );
			err = map_socket_noerr_errno( mSock, err );
			require_noerr( err, exit );
			
			mState = kHTTPConnectionState_WaitingForClose;
		}
		
		mRequest.Cleanup();
		mResponse->Cleanup();
		mResponse = NULL;
		
		CFSocketEnableCallBacks( mCFSock, kCFSocketReadCallBack );
		break;
	}
	
exit:
	return( err );
}

#if 0
#pragma mark -
#pragma mark == HTTPMessageCPP ==
#endif

//===========================================================================================================================
//	HTTPMessageCPP
//===========================================================================================================================

HTTPMessageCPP::HTTPMessageCPP( void ) :
	mHeaderSize( 0 ), 
	mBodyBuffer( NULL ),
	mBodyBufferSize( 0 ), 
	mBodyBufferUsed( 0 ), 
	mBodySize( 0 ), 
	mBodyMallocedBuffer( NULL ), 
	mMethodPtr( NULL ), 
	mMethodLen( 0 ), 
	mURLPtr( NULL ), 
	mURLLen( 0 ), 
	mProtocolPtr( NULL ), 
	mProtocolLen( 0 ), 
	mStatusCode( 0 ), 
	mReasonPhrasePtr( NULL ), 
	mReasonPhraseLen( 0 ), 
	mChannelID( 0 ), 
	mContentLength( 0 ), 
	mPersistent( false )
{
	memset( &mURLComps, 0, sizeof( mURLComps ) );
}

//===========================================================================================================================
//	~HTTPMessageCPP
//===========================================================================================================================

HTTPMessageCPP::~HTTPMessageCPP( void )
{
	ForgetMem( &mBodyMallocedBuffer );
}

//===========================================================================================================================
//	ParseHeader
//
//	Parses a header that has been read in by HTTPReadHeader. This assumes the mHeaderBuffer and mHeaderSize fields are set.
//===========================================================================================================================

OSStatus	HTTPMessageCPP::ParseHeader( void )
{
	OSStatus				err;
	const char *			src;
	const char *			ptr;
	char					c;
	int						n;
	const char *			value;
	size_t					valueSize;
	int						x;
	
	check( mHeaderSize < sizeof( mHeaderBuffer ) );
	check( mHeaderBuffer[ mHeaderSize ] == '\0' );
	
	// Reset fields up-front to good defaults to simplify handling of unused fields later.
	
	mMethodPtr			= "";
	mMethodLen			= 0;
	mURLPtr				= "";
	mURLLen				= 0;
	mProtocolPtr		= "";
	mProtocolLen		= 0;
	mStatusCode			= 0;
	mReasonPhrasePtr	= "";
	mReasonPhraseLen	= 0;
	mChannelID			= 0;
	mContentLength		= 0;
	mPersistent			= false;
	
	// Check for a 4-byte interleaved binary data header (see RFC 2326 section 10.12). It has the following format:
	//
	//		'$' <1:channelID> <2:dataSize in network byte order> ... followed by dataSize bytes of binary data.
	
	src = mHeaderBuffer;
	if( ( src[ 0 ] == '$' ) && ( mHeaderSize == 4 ) )
	{
		const uint8_t *		usrc;
		
		usrc = (const uint8_t *) src;
		mChannelID		=   usrc[ 1 ];
		mContentLength	= ( usrc[ 2 ] << 8 ) | usrc[ 3 ];
		
		mMethodPtr = src;
		mMethodLen = 1;
		
		err = kNoErr;
		goto exit;
	}
	
	// Parse the start line. This will also determine if it's a request or response.
	// Requests are in the format <method> <url> <protocol>/<majorVersion>.<minorVersion>, for example:
	//
	//		GET /abc/xyz.html HTTP/1.1
	//		GET http://www.host.com/abc/xyz.html HTTP/1.1
	//		GET http://user:password@www.host.com/abc/xyz.html HTTP/1.1
	//
	// Responses are in the format <protocol>/<majorVersion>.<minorVersion> <statusCode> <reasonPhrase>, for example:
	//
	//		HTTP/1.1 404 Not Found
	
	for( ptr = src; ( ( c = *ptr ) != '\0' ) && ( c != ' ' ) && ( c != '/' ); ++ptr ) {}
	require_action( c != '\0', exit, err = kMalformedErr );
	
	if( c == ' ' )	// Request
	{
		mMethodPtr = src;
		mMethodLen = (size_t)( ptr - src );
		require_action( mMethodLen > 0, exit, err = kMalformedErr );
		++ptr;
		
		// Parse the URL.
		
		mURLPtr = ptr;
		while( ( ( c = *ptr ) != '\0' ) && ( c != ' ' ) ) ++ptr;
		mURLLen = (size_t)( ptr - mURLPtr );
		require_action( c != '\0', exit, err = kMalformedErr );
		++ptr;
		
		err = URLParseComponents( mURLPtr, mURLPtr + mURLLen, &mURLComps, NULL );
		require_noerr( err, exit );
		
		// Parse the protocol.
		
		mProtocolPtr = ptr;
		while( ( ( c = *ptr ) != '\0' ) && ( c != '\r' ) && ( c != '\n' ) ) ++ptr;
		mProtocolLen = (size_t)( ptr - mProtocolPtr );
		require_action( c != '\0', exit, err = kMalformedErr );
		++ptr;
	}
	else			// Response
	{
		// Parse the rest of the protocol.
		
		mProtocolPtr = src;
		for( ++ptr; ( ( c = *ptr ) != '\0' ) && ( c != ' ' ); ++ptr ) {}
		mProtocolLen = (size_t)( ptr - mProtocolPtr );
		require_action( c != '\0', exit, err = kMalformedErr );
		++ptr;
		
		// Parse the status code.
		
		x = 0;
		for( ; ( ( c = *ptr ) >= '0' ) && ( c <= '9' ); ++ptr ) x = ( x * 10 ) + ( c - '0' ); 
		mStatusCode = x;
		if( c == ' ' ) ++ptr;
		
		// Parse the reason phrase.
		
		mReasonPhrasePtr = ptr;
		while( ( ( c = *ptr ) != '\0' ) && ( c != '\r' ) && ( c != '\n' ) ) ++ptr;
		mReasonPhraseLen = (size_t)( ptr - mReasonPhrasePtr );
		require_action( c != '\0', exit, err = kMalformedErr );
		++ptr;
	}
	require_action( *ptr != '\0', exit, err = kMalformedErr );
	
	// Determine persistence. Note: HTTP 1.0 defaults to non-persistent if a Connection header field is not present.
	
	err = GetHeaderValue( "Connection", &value, &valueSize );
	if( err )	mPersistent = ( strnicmpx( mProtocolPtr, mProtocolLen, "HTTP/1.0" ) != 0 );
	else		mPersistent = ( strnicmpx( value, valueSize, "close" ) != 0 );
	
	// Special case HEAD requests to ignore the content length since it does not represent the length of this body.
	
	if( strnicmpx( mMethodPtr, mMethodLen, "HEAD" ) == 0 )
	{
		mContentLength = 0;
	}
	else
	{
		n = ScanFHeaderValue( "Content-Length", "%u", &mContentLength );
		if( n != 1 )
		{
			mContentLength = 0;
		}
	}
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	GetHeaderValue
//===========================================================================================================================

OSStatus	HTTPMessageCPP::GetHeaderValue( const char *inName, const char **outValue, size_t *outValueSize ) const
{
	return( HTTPGetHeaderField( mHeaderBuffer, mHeaderSize, inName, NULL, NULL, outValue, outValueSize, NULL ) );
}

//===========================================================================================================================
//	ScanFHeaderValue
//===========================================================================================================================

int	HTTPMessageCPP::ScanFHeaderValue( const char *inName, const char *inFormat, ... ) const
{
	int					n;
	const char *		value;
	size_t				valueSize;
	va_list				args;
	
	n = HTTPGetHeaderField( mHeaderBuffer, mHeaderSize, inName, NULL, NULL, &value, &valueSize, NULL );
	require_noerr_quiet( n, exit );
	
	va_start( args, inFormat );
	n = VSNScanF( value, valueSize, inFormat, args );
	va_end( args );
	
exit:
	return( n );	
}

//===========================================================================================================================
//	SetResponseLine
//===========================================================================================================================

OSStatus	HTTPMessageCPP::SetResponseLine( const char *inProtocol, int inStatusCode, const char *inReasonPhrase )
{
	OSStatus		err;
	
	err = HTTPSetResponseLine( mHeaderBuffer, sizeof( mHeaderBuffer ), &mHeaderSize, inProtocol, inStatusCode, inReasonPhrase );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	AppendHeaderValueF
//===========================================================================================================================

OSStatus	HTTPMessageCPP::AppendHeaderValueF( const char *inName, const char *inFormat, ... )
{
	OSStatus		err;
	size_t			freeSpace;
	va_list			args;
	int				n;
	
	require_action( mHeaderSize > 0, exit, err = kNotPreparedErr );
	freeSpace = sizeof( mHeaderBuffer ) - mHeaderSize;
	
	va_start( args, inFormat );
	n = SNPrintF( &mHeaderBuffer[ mHeaderSize ], freeSpace, "%s: %V\r\n", inName, inFormat, &args );
	va_end( args );
	require_action( ( n > 0 ) && ( n < ( (int) freeSpace ) ), exit, err = kOverrunErr );
	
	mHeaderSize += n;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	Cleanup
//===========================================================================================================================

void	HTTPMessageCPP::Cleanup( void )
{
	mHeaderSize = 0;
	mBodySize = 0;
	ForgetMem( &mBodyMallocedBuffer );
}

//===========================================================================================================================
//	SetBody
//===========================================================================================================================

OSStatus	HTTPMessageCPP::SetBody( const void *inData, size_t inSize )
{
	mBodyBuffer		= (uint8_t *) inData;
	mBodyBufferSize	= inSize;
	mBodyBufferUsed	= 0;
	mBodySize		= inSize;
	return( kNoErr );
}

//===========================================================================================================================
//	Commit
//===========================================================================================================================

OSStatus	HTTPMessageCPP::Commit( void )
{
	OSStatus		err;
	
	require_action( mHeaderSize > 0, exit, err = kNotPreparedErr );
	require_action( ( sizeof( mHeaderBuffer ) - mHeaderSize ) >= 3, exit, err = kOverrunErr );
	
	mHeaderBuffer[ mHeaderSize++ ] = '\r';
	mHeaderBuffer[ mHeaderSize++ ] = '\n';
	mHeaderBuffer[ mHeaderSize ]   = '\0';
	err = kNoErr;
	
exit:
	return( err );
}

#if 0
#pragma mark -
#pragma mark == Utilities ==
#endif

//===========================================================================================================================
//	HTTPSetResponseLine
//===========================================================================================================================

OSStatus
	HTTPSetResponseLine( 
		char *			inBuffer, 
		size_t			inBufferSize, 
		size_t *		outHeaderSize, 
		const char *	inProtocol, 
		int				inStatusCode, 
		const char *	inReasonPhrase )
{
	OSStatus		err;
	int				n;
	
	if( !inReasonPhrase )
	{
		inReasonPhrase = HTTPGetReasonPhrase( inStatusCode );
		require_action( *inReasonPhrase != '\0', exit, err = kNotFoundErr );
	}
	
	n = snprintf( inBuffer, inBufferSize, "%s %u %s\r\n", inProtocol, inStatusCode, inReasonPhrase );
	require_action( ( n > 0 ) && ( n < ( (int) inBufferSize ) ), exit, err = kOverrunErr );
	
	*outHeaderSize = (size_t) n;
	err = kNoErr;
	
exit:
	return( err );
}
