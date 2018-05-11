/*
	Copyright (C) 2010-2013 Apple Inc. All Rights Reserved.
*/

#ifndef	__AsyncConnection_h__
#define	__AsyncConnection_h__

#include "CommonServices.h"

#include LIBDISPATCH_HEADER

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct AsyncConnection *		AsyncConnectionRef;

//---------------------------------------------------------------------------------------------------------------------------
/*!	@typedef	AsyncConnectionFlags
	@abstract	Flags to affect how the connections work.
*/

typedef uint32_t		AsyncConnectionFlags;
#define kAsyncConnectionFlags_None					0
#define kAsyncConnectionFlag_P2P					( 1 << 0 ) //! Allows connections on P2P interfaces.
#define kAsyncConnectionFlag_SuppressUnusable		( 1 << 1 ) //! Suppress trying to connect on seemingly unusable interfaces.
#define kAsyncConnectionFlag_Reachability			( 1 << 2 ) //! Use the reachability APIs before trying to connect.
#define kAsyncConnectionFlag_BoundInterface			( 1 << 3 ) //! Set bound interface before connect if interface index available.

//---------------------------------------------------------------------------------------------------------------------------
/*!	@callback	AsyncConnectionHandlerFunc
	@abstract	Function to be called when an async connection completes (success or failure).
	
	@param		inSock		Connected socket on success or kInvalidSocketRef on failure. If valid, you must close it.
	@param		inError		kNoErr on success or an error code on failure.
	@param		inArg		Context pointer that was provided to AsyncConnection_Connect via "inHandlerArg".
	
	@discussion
	
	Note: this callback will always be called exactly once.
*/

typedef void ( *AsyncConnectionHandlerFunc )( SocketRef inSock, OSStatus inError, void *inArg );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@callback	AsyncConnectionProgressFunc
	@abstract	Function to be called for progress events during the connection process.
*/

#define kAsyncConnectionPhase_QueryingSRV		1 // Details: Destination string.
#define kAsyncConnectionPhase_ResolvingDNS		2 // Details: Destination string.
#define kAsyncConnectionPhase_Connecting		3 // Details: sockaddr ptr.
#define kAsyncConnectionPhase_Connected			4 // Details: sockaddr ptr.

typedef void ( *AsyncConnectionProgressFunc )( int inPhase, const void *inDetails, void *inArg );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@callback	AsyncConnectionProgressFunc
	@abstract	Function to be called while waiting for connections to complete.
*/

typedef OSStatus ( *AsyncConnectionWaitFunc )( void *inContext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AsyncConnection_Connect
	@abstract	Starts an async connect.
	
	@param		outConnection	Receives a connection object that can be used to cancel the connect.
	@param		inDestination	Destination to connect to. The following formats are supported:
	
		Bonjour service name:	<friendly name>.<service type>._tcp.<domain>[%<interface index>][:<port number>]
								For example: "Brother\032HL-5370DW\032series._http._tcp.local.%4:80".
								
								This name usually comes from a Bonjour browse of the service type of the device.
								You can use the name, type, and domain with DNSServiceConstructFullName to form
								the name to pass here. That function will escape things as needed.
		
		DNS name:				<DNS name>[%<interface index>][:<port number>]
								For example: "Brother-HL-5370DW-series.local.:80".
		
		IPv4 address:			<IPv4 address>[:<port number>]
								For example: "10.0.20.10:80"
		
		IPv6 address:			[[]<IPv6 address>[%<scope>][]][:<port number>]
								For example: "[fe80::20d:93ff:feca:f7af%4]:5000"
	
	@param		inDefaultPort				Default TCP port to use if no other port is known. < 0 means force it.
	@param		inTimeoutNanos				Max nanoseconds to wait for a connection. Use 0 for no timeout.
	@param		inSocketSendBufferSize		Socket Send buffer size. See SocketSetBufferSize for details.
	@param		inSocketRecvBufferSize		Socket Receive buffer size. See SocketSetBufferSize for details.
	@param		inHandlerFunc				Function to be called on success or failure.
	@param		inHandlerArg				Argument to be passed to the handler function.
	@param		inQueue						Dispatch queue to invoke the handler on.
*/

#define kAsyncConnectionNoTimeout		0

OSStatus
	AsyncConnection_Connect( 
		AsyncConnectionRef *			outConnection, 
		const char *					inDestination, 
		int								inDefaultPort, 
		AsyncConnectionFlags			inFlags, 
		uint64_t						inTimeoutNanos, 
		int								inSocketSendBufferSize, 
		int								inSocketRecvBufferSize, 
		AsyncConnectionProgressFunc		inProgressFunc, 
		void *							inProgressArg, 
		AsyncConnectionHandlerFunc		inHandlerFunc, 
		void *							inHandlerArg, 
		dispatch_queue_t				inQueue );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AsyncConnection_Release
	@abstract	Asynchronously cancel the connection, invoke the callback (if it hasn't already), and release its memory.
	@discussion
	
	The handler will always be invoked exactly once so you must always wait for the handler to be invoked before 
	invalidating any resources that might be used by the handler function.
*/

void	AsyncConnection_Release( AsyncConnectionRef inConnection );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AsyncConnection_ConnectSync
	@abstract	Synchronous helper function for legacy code.
*/

OSStatus
	AsyncConnection_ConnectSync( 
		const char *				inDestination, 
		int							inDefaultPort, 
		AsyncConnectionFlags		inFlags, 
		uint64_t					inTimeoutNanos, 
		int							inSocketSendBufferSize, 
		int							inSocketRecvBufferSize, 
		AsyncConnectionProgressFunc	inProgressFunc, 
		void *						inProgressArg, 
		SocketRef *					outSock );

OSStatus
	AsyncConnection_ConnectSyncEx( 
		const char *				inDestination, 
		int							inDefaultPort, 
		AsyncConnectionFlags		inFlags, 
		uint64_t					inTimeoutNanos, 
		int							inSocketSendBufferSize, 
		int							inSocketRecvBufferSize, 
		AsyncConnectionProgressFunc	inProgressFunc, 
		void *						inProgressArg, 
		AsyncConnectionWaitFunc		inWaitCallBack, 
		void *						inWaitContext, 
		SocketRef *					outSock );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AsyncConnection_Test
	@abstract	Unit test.
*/

#if( DEBUG )
	OSStatus	AsyncConnection_Test( void );
#endif

#ifdef __cplusplus
}
#endif

#endif	// __AsyncConnection_h__
