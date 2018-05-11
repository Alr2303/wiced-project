/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/

#ifndef __HTTPServer_hpp__
#define	__HTTPServer_hpp__

#include "CommonServices.h"
#include "dns_sd.h"
#include "DebugServices.h"
#include "NetUtils.h"
#include "URLUtils.h"

	#include <net/if.h>

#include CF_HEADER

namespace CoreUtils
{

// Forward Declarations

class HTTPConnection;

//---------------------------------------------------------------------------------------------------------------------------
/*!	@class		HTTPMessageCPP
	@abstract	Represents an HTTP request or response.
*/

class HTTPMessageCPP
{
	public:
	
		char				mHeaderBuffer[ 4096 ];	//! Buffer holding the start line and all headers.
		size_t				mHeaderSize;			//! Number of bytes in the header.
		
		uint8_t *			mBodyBuffer;			//! Buffer holding the message body.
		size_t				mBodyBufferSize;		//! Maximum number of bytes the body buffer can hold.
		size_t				mBodyBufferUsed;		//! Number of bytes currently in the body buffer.
		size_t				mBodySize;				//! Total number of bytes of body data.
		uint8_t *			mBodyMallocedBuffer;	//! Malloc'd buffer to free when message completes.
		
		// Parsed Info. WARNING: None of these strings are null terminated so you must use the length.
		
		const char *		mMethodPtr;				//! Request method (e.g. "GET"). "$" for interleaved binary data.
		size_t				mMethodLen;				//! Number of bytes in request method.
		const char *		mURLPtr;				//! Request absolute or relative URL or empty if not a request.
		size_t				mURLLen;				//! Number of bytes in URL.
		URLComponents		mURLComps;				//! Parsed URL components.
		const char *		mProtocolPtr;			//! Request or response protocol (e.g. "HTTP/1.1").
		size_t				mProtocolLen;			//! Number of bytes in protocol.
		int					mStatusCode;			//! Response status code (e.g. 200 for HTTP OK).
		const char *		mReasonPhrasePtr;		//! Response reason phrase (e.g. "OK" for an HTTP 200 OK response).
		size_t				mReasonPhraseLen;		//! Number of bytes in reason phrase.
		uint8_t				mChannelID;				//! Interleaved binary data channel ID. 0 for other message types.
		unsigned int		mContentLength;			//! Number of bytes following the header. May be 0.
		bool				mPersistent;			//! true=Do not close the connection after this message.
		
		HTTPMessageCPP( void );
		virtual ~HTTPMessageCPP( void );
		
		virtual OSStatus	ParseHeader( void );
		virtual void		Cleanup( void );
		virtual OSStatus	SupplyData( void ) { return( kNoErr ); }
		
		OSStatus	GetHeaderValue( const char *inName, const char **outValue, size_t *outValueSize ) const;
		int			ScanFHeaderValue( const char *inName, const char *inFormat, ... ) const;
		
		OSStatus	SetResponseLine( const char *inProtocol, int inStatusCode, const char *inReasonPhrase );
		OSStatus	AppendHeaderValueF( const char *inName, const char *inFormat, ... );
		OSStatus	SetBody( const void *inData, size_t inSize );
		OSStatus	Commit( void );
};

//---------------------------------------------------------------------------------------------------------------------------
/*!	@class		HTTPServerCPP
	@abstract	HTTP server.
*/

class HTTPServerCPP
{
	protected:
		
		HTTPConnection *		mConnectionList;
		SocketRef				mListenerSock;
		int						mListenerSockReceiveBufferSize;
		CFSocketRef				mListenerCFSock;
		int						mListenPort;
		int						mListeningPort;
		bool					mAllowP2P;
		
	public:
		
		virtual void		Cleanup( void );
		virtual void		Delete( void );
		virtual OSStatus	Start( void );
		int					GetListenPort( void )	{ return( mListeningPort ); }
		bool				GetP2P( void )			{ return( mAllowP2P ); }
		void				SetP2P( bool inAllow )	{ mAllowP2P = inAllow; }
		virtual OSStatus	SetupListenerSocketV4( void );
		#if( defined( AF_INET6 ) )
			virtual OSStatus	SetupListenerSocketV6( void );
		#endif
		static void
			ListenerSocketCallBack( 
				CFSocketRef				inSock, 
				CFSocketCallBackType	inType, 
				CFDataRef				inAddr, 
				const void *			inData, 
				void *					inContext );
		
		// Connections
		
		virtual OSStatus	CreateConnection( HTTPConnection **outConnection );
		virtual OSStatus	DeleteConnection( HTTPConnection *inConnection );
		OSStatus			RemoveConnection( HTTPConnection *inConnection );
		void				RemoveAllConnections( HTTPConnection *inConnectionToKeep = NULL );
	
	protected:
		
		HTTPServerCPP( void );											// Prohibit manual creation.
		HTTPServerCPP( const HTTPServerCPP &inOriginal );				// Prohibit copying.
		virtual ~HTTPServerCPP( void );									// Prohibit manual deletion.
		HTTPServerCPP &	operator=( const HTTPServerCPP &inOriginal );	// Prohibit assignment.
};

//---------------------------------------------------------------------------------------------------------------------------
/*!	@class		HTTPConnection
	@abstract	HTTP connection.
*/

class HTTPConnection
{
	friend class HTTPServerCPP;
	
	protected:
		
		enum HTTPConnectionState
		{
			kHTTPConnectionState_ReadingHeader		= 0, 
			kHTTPConnectionState_ReadingBody		= 1, 
			kHTTPConnectionState_ProcessingRequest	= 2, 
			kHTTPConnectionState_WritingResponse	= 3, 
			kHTTPConnectionState_WaitingForClose	= 4
		};
		
	public:
		
		HTTPServerCPP &			mServer;
		HTTPConnection *		mNextConnection;
		
		SocketRef				mSock;
		CFSocketRef				mCFSock;
		
		sockaddr_ip				mSelfAddrRaw;				// Raw addr returned by getsockname (may be IPv4-mapped IPv6).
		sockaddr_ip				mSelfAddr;					// If raw is IPv4-mapped IPv6, this will be IPv4.
		sockaddr_ip				mPeerAddrRaw;				// Raw addr returned by getpeername (may be IPv4-mapped IPv6).
		sockaddr_ip				mPeerAddr;					// If raw is IPv4-mapped IPv6, this will be IPv4.
		char					mIfName[ IF_NAMESIZE + 1 ]; // Name of the interface the connection was accepted on.
		uint32_t				mIfIndex;					// Index of the interface the connection was accepted on.
		uint32_t				mIfMedia;					// Media options of the interface the connection was accepted on.
		uint32_t				mIfFlags;					// Flags of the interface the connection was accepted on.
		uint64_t				mIfExtendedFlags;			// Flags of the interface the connection was accepted on.
		NetTransportType		mTransportType;				// Transport type of the interface for this connection.
		
		HTTPConnectionState		mState;						// Current state of the connection.
		
		HTTPMessageCPP 			mRequest;					// Request message currently being read in/processed.
		uint8_t *				mReadBuffer;				// Buffer used for reading in request bodies.
		size_t					mReadBufferSize;			// Size of the read buffer.
		
		HTTPMessageCPP *		mResponse;					// Response message currently being written.
		struct iovec			mWriteIOArray[ 2 ];			// Array for doing gathered writes.
		struct iovec *			mWriteIOPtr;				// Ptr to the first io_vec element to write.
		int						mWriteIOCount;				// Number of io_vec elements to write.
		
	public:
		
		HTTPConnection( HTTPServerCPP &inServer );
		virtual	~HTTPConnection( void );
		
		virtual OSStatus	Accept( SocketRef inSock );
		static void
			SocketCallBack( 
				CFSocketRef				inSock, 
				CFSocketCallBackType	inType, 
				CFDataRef				inAddr,
				const void *			inData,
				void *					inContext );
		virtual OSStatus	RunStateMachine( void );
		
		virtual OSStatus	ProcessRequestHeaders( void )	{ return( kNoErr ); }
		virtual OSStatus	ProcessRequest( void )			{ return( kNoErr ); }
		virtual OSStatus	ProcessResponseDone( void )		{ return( kNoErr ); }
		virtual OSStatus	StartResponse( HTTPMessageCPP *inResponse );
		virtual OSStatus	WriteResponse( void );
	
	protected:
	
		HTTPConnection( void );												// Prohibit manual creation.
		HTTPConnection( const HTTPConnection &inOriginal );					// Prohibit copying.
		HTTPConnection &	operator=( const HTTPConnection &inOriginal );	// Prohibit assignment.
};

} // namespace CoreUtils

// Utilities

OSStatus
	HTTPSetResponseLine( 
		char *			inBuffer, 
		size_t			inBufferSize, 
		size_t *		outHeaderSize, 
		const char *	inProtocol, 
		int				inStatusCode, 
		const char *	inReasonPhrase );

#endif 	// __HTTPServer_hpp__
