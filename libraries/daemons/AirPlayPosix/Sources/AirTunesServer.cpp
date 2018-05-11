/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/

#include "AirTunesServer.h"

#include <new>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "AirPlayCommon.h"
#include "AirPlayReceiverServer.h"
#include "AirPlayReceiverSession.h"
#include "AirPlaySettings.h"
#include "AirPlayVersion.h"
#include "Base64Utils.h"
#include "CFUtils.h"
#include "CommonServices.h"
#include "DebugServices.h"
#include "HTTPServerCPP.hpp"
#include "HTTPUtils.h"
#include "NetUtils.h"
#include "SDPUtils.h"
#include "RandomNumberUtils.h"
#include "StringUtils.h"
#include "SystemUtils.h"
#include "TickUtils.h"
#include "URLUtils.h"

#if( AIRPLAY_DACP )
	#include "AirTunesDACP.h"
#endif
#if( AIRPLAY_META_DATA )
	#include "DMAP.h"
	#include "DMAPUtils.h"
#endif
#if( AIRPLAY_MFI )
	#include "MFiSAP.h"
#endif

//===========================================================================================================================
//	Internal
//===========================================================================================================================

AirPlayCompressionType				gAirPlayAudioCompressionType	= kAirPlayCompressionType_Undefined;
static AirTunesServer *				gAirTunesServer					= NULL;
static Boolean						gAirTunesPlaybackAllowed		= true;
static Boolean						gAirTunesDenyInterruptions		= false;
#if( AIRPLAY_DACP )
	static AirTunesDACPClientRef	gAirTunesDACPClient				= NULL;
#endif

ulog_define( AirTunesServerCore, kLogLevelNotice, kLogFlags_Default, "AirPlay",  NULL );
#define ats_ulog( LEVEL, ... )			ulog( &log_category_from_name( AirTunesServerCore ), (LEVEL), __VA_ARGS__ )

ulog_define( AirTunesServerHTTP, kLogLevelNotice, kLogFlags_Default, "AirPlay",  NULL );
#define ats_http_ucat()					&log_category_from_name( AirTunesServerHTTP )
#define ats_http_ulog( LEVEL, ... )		ulog( ats_http_ucat(), (LEVEL), __VA_ARGS__ )

	#define PairingDescription()		""

	#define AirTunesConnection_DidSetup()	mDidAudioSetup

#if 0
#pragma mark == AirTunes Control ==
#endif

//===========================================================================================================================
//	AirTunesServer_EnsureCreated
//===========================================================================================================================

OSStatus	AirTunesServer_EnsureCreated( void )
{
	if( !gAirTunesServer )
	{
		return( AirTunesServer::Create( &gAirTunesServer ) );
	}
	return( kNoErr );
}

//===========================================================================================================================
//	AirTunesServer_EnsureDeleted
//===========================================================================================================================

void	AirTunesServer_EnsureDeleted( void )
{
	if( gAirTunesServer )
	{
		gAirTunesServer->Delete();
		gAirTunesServer = NULL;
	}
}

//===========================================================================================================================
//	AirTunesServer_Start
//===========================================================================================================================

OSStatus	AirTunesServer_Start( void )
{
	OSStatus		err;
	
	if( gAirTunesServer )
	{
		err = gAirTunesServer->Start();
		return( err );
	}
	return( kNotInitializedErr );
}

//===========================================================================================================================
//	AirTunesServer_GetListenPort
//===========================================================================================================================

int	AirTunesServer_GetListenPort( void )
{
	if( gAirTunesServer ) return( gAirTunesServer->GetListenPort() );
	return( 0 );
}

//===========================================================================================================================
//	AirTunesServer_SetDenyInterruptions
//===========================================================================================================================

void	AirTunesServer_SetDenyInterruptions( Boolean inDeny )
{
	if( inDeny != gAirTunesDenyInterruptions )
	{
		gAirTunesDenyInterruptions = inDeny;
		
	}
}

//===========================================================================================================================
//	AirTunesServer_FailureOccurred
//===========================================================================================================================

void	AirTunesServer_FailureOccurred( OSStatus inError )
{
	DEBUG_USE_ONLY( inError );
	
	ats_ulog( kLogLevelNotice, "### Failure: %#m\n", inError );
	if( gAirTunesServer )
	{
		gAirTunesServer->RemoveAllConnections();
	}
}

//===========================================================================================================================
//	AirTunesServer_StopAllConnections
//===========================================================================================================================

void	AirTunesServer_StopAllConnections( void )
{
	if( gAirTunesServer ) gAirTunesServer->RemoveAllConnections();
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	_AirTunesServer_PlaybackAllowed
//===========================================================================================================================

static Boolean	_AirTunesServer_PlaybackAllowed( void )
{
	if( gAirTunesPlaybackAllowed )
	{
		OSStatus		err;
		Boolean			b;
		
		b = AirPlayReceiverServerPlatformGetBoolean( gAirPlayReceiverServer, CFSTR( kAirPlayProperty_PlaybackAllowed ), 
			NULL, &err );
		if( b || ( err == kNotHandledErr ) )
		{
			return( true );
		}
	}
	return( false );
}

//===========================================================================================================================
//	AirTunesServer_AllowPlayback
//===========================================================================================================================

void	AirTunesServer_AllowPlayback( void )
{	
	if( !gAirTunesPlaybackAllowed )
	{
		dlog( kLogLevelNotice, "*** ALLOW PLAYBACK ***\n" );
		gAirTunesPlaybackAllowed = true;
		
		#if( AIRPLAY_DACP )
			AirTunesServer_ScheduleDACPCommand( "setproperty?dmcp.device-prevent-playback=0" );
		#endif
	}
}

//===========================================================================================================================
//	AirTunesServer_PreventPlayback
//===========================================================================================================================

void	AirTunesServer_PreventPlayback( void )
{
	if( gAirTunesPlaybackAllowed )
	{
		dlog( kLogLevelNotice, "*** PREVENT PLAYBACK ***\n" );
		gAirTunesPlaybackAllowed = false;
		if( gAirTunesServer ) gAirTunesServer->RemoveAllConnections();
		
		#if( AIRPLAY_DACP )
			AirTunesServer_ScheduleDACPCommand( "setproperty?dmcp.device-prevent-playback=1" );
		#endif
	}
}

#if 0
#pragma mark -
#endif

#if( AIRPLAY_DACP )
//===========================================================================================================================
//	AirTunesServer_SetDACPCallBack
//===========================================================================================================================

OSStatus	AirTunesServer_SetDACPCallBack( AirTunesDACPClientCallBackPtr inCallBack, void *inContext )
{
	return( AirTunesDACPClient_SetCallBack( gAirTunesDACPClient, inCallBack, inContext ) );
}

//===========================================================================================================================
//	AirTunesServer_ScheduleDACPCommand
//===========================================================================================================================

OSStatus	AirTunesServer_ScheduleDACPCommand( const char *inCommand )
{
	return( AirTunesDACPClient_ScheduleCommand( gAirTunesDACPClient, inCommand ) );
}
#endif

#if 0
#pragma mark -
#pragma mark == AirTunesServer ==
#endif

//===========================================================================================================================
//	AirTunesServer
//===========================================================================================================================

AirTunesServer::AirTunesServer( void )
{
	
	mListenPort = -kAirPlayPort_RTSPControl;
	RandomBytes( mHTTPTimedNonceKey, sizeof( mHTTPTimedNonceKey ) );
#if( AIRPLAY_VOLUME_CONTROL )
	mVolume = 0;
#endif
}

//===========================================================================================================================
//	Create [static]
//===========================================================================================================================

OSStatus	AirTunesServer::Create( AirTunesServer **outServer )
{
	OSStatus				err;
	AirTunesServer *		obj;

	gAirTunesServer				= NULL;
	gAirTunesPlaybackAllowed	= true;
	
	obj = NEW AirTunesServer;
	require_action( obj, exit, err = kNoMemoryErr );
	
#if( AIRPLAY_DACP )
	check( gAirTunesDACPClient == NULL );
	err = AirTunesDACPClient_Create( &gAirTunesDACPClient );
	check_noerr( err );
#endif
	
	*outServer = obj;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	Delete
//===========================================================================================================================

void	AirTunesServer::Delete( void )
{
#if( AIRPLAY_DACP )
	if( gAirTunesDACPClient )
	{
		AirTunesDACPClient_Delete( gAirTunesDACPClient );
		gAirTunesDACPClient = NULL;
	}
#endif
	
	HTTPServerCPP::Delete();
}

//===========================================================================================================================
//	Start
//===========================================================================================================================

OSStatus	AirTunesServer::Start( void )
{
	OSStatus		err;
	
	err = HTTPServerCPP::Start();
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	CreateConnection
//===========================================================================================================================

OSStatus	AirTunesServer::CreateConnection( HTTPConnection **outConnection )
{
	OSStatus				err;
	HTTPConnection *		obj;
	
	obj = NEW AirTunesConnection( *this );
	require_action( obj, exit, err = kNoMemoryErr );
	
	*outConnection = obj;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	FindActiveConnection
//===========================================================================================================================

AirTunesConnection *	AirTunesServer::FindActiveConnection( void )
{
	for( HTTPConnection *cnx = mConnectionList; cnx; cnx = cnx->mNextConnection )
	{
		AirTunesConnection * const		atCnx = (AirTunesConnection *) cnx;
		
		if( atCnx->mDidAnnounce )
		{
			return( atCnx );
		}
	}
	return( NULL );
}

//===========================================================================================================================
//	HijackConnections
//===========================================================================================================================

void	AirTunesServer::HijackConnections( AirTunesConnection *inHijacker )
{
	HTTPConnection **		next;
	HTTPConnection *		conn;
	
	// Close any connections that have started audio (should be 1 connection at most).
	// This leaves connections that haven't started audio because they may just be doing OPTIONS requests, etc.
	
	next = &mConnectionList;
	while( ( conn = *next ) != NULL )
	{
		AirTunesConnection * const		atConn = (AirTunesConnection *) conn;
		
		if( ( atConn != inHijacker ) && atConn->mDidAnnounce )
		{
			ats_ulog( kLogLevelNotice, "*** Hijacking connection %##a for %##a\n", &conn->mPeerAddr, &inHijacker->mPeerAddr );
			*next = conn->mNextConnection;
			DeleteConnection( conn );
		}
		else
		{
			next = &conn->mNextConnection;
		}
	}
}

#if 0
#pragma mark -
#pragma mark == AirTunesConnection ==
#endif

//===========================================================================================================================
//	AirTunesConnection
//===========================================================================================================================

AirTunesConnection::AirTunesConnection( HTTPServerCPP &inServer ) : 
	HTTPConnection( inServer ), 
	mAirTunesServer( reinterpret_cast < AirTunesServer & > ( mServer ) ), 
	mClientDeviceID( 0 ), 
	mClientSessionID( 0 ), 
	mDidAnnounce( false ), 
	mDidAudioSetup( false ), 
	mDidRecord( false ), 
	mHTTPAuthentication_IsAuthenticated( false ), 
	mCompressionType( kAirPlayCompressionType_Undefined ), 
#if( AIRPLAY_MFI )	
	mMFiSAP( NULL ), 
	mMFiSAPDone( false ), 
#endif
	mUsingEncryption( false ), 
	mSession( NULL )
{
	
	*mClientName = '\0';
}

//===========================================================================================================================
//	~AirTunesConnection
//===========================================================================================================================

AirTunesConnection::~AirTunesConnection( void )
{
#if( AIRPLAY_DACP )
	if( mDidAudioSetup ) AirTunesDACPClient_StopSession( gAirTunesDACPClient );
#endif
	
	if( mSession )
	{
		AirPlayReceiverSessionTearDown( mSession, mDidAnnounce ? kConnectionErr : kNoErr );
		CFRelease( mSession );
		mSession = NULL;
		AirPlayReceiverServerSetBoolean( gAirPlayReceiverServer, CFSTR( kAirPlayProperty_Playing ), NULL, false );
	}
#if( AIRPLAY_MFI )
	if( mMFiSAP )
	{
		MFiSAP_Delete( mMFiSAP );
		mMFiSAP = NULL;
		mMFiSAPDone = false;
	}
#endif
}

//===========================================================================================================================
//	ProcessRequest
//===========================================================================================================================

OSStatus	AirTunesConnection::ProcessRequest( void )
{
	const char * const		methodPtr	= mRequest.mMethodPtr;
	size_t const			methodLen	= mRequest.mMethodLen;
	const char * const		pathPtr		= mRequest.mURLComps.pathPtr;
	size_t const			pathLen		= mRequest.mURLComps.pathLen;
	OSStatus				err;
	HTTPStatus				status;
	Boolean					logHTTP = true;
	const char *			httpProtocol;
	const char *			cSeqPtr = NULL;
	size_t					cSeqLen = 0;
	
	httpProtocol = ( strnicmp_prefix( mRequest.mProtocolPtr, mRequest.mProtocolLen, "HTTP" ) == 0 ) 
		? "HTTP/1.1" : kAirTunesHTTPVersionStr;
	
	// OPTIONS and /feedback requests are too chatty so don't log them by default.
	
	if( ( ( strnicmpx( methodPtr, methodLen, "OPTIONS" ) == 0 ) || 
		( ( strnicmpx( methodPtr, methodLen, "POST" ) == 0 ) && ( strnicmp_suffix( pathPtr, pathLen, "/feedback" ) == 0 ) ) ) &&
		!log_category_enabled( ats_http_ucat(), kLogLevelChatty ) )
	{
		logHTTP = false;
	}
	if( logHTTP ) LogHTTP( ats_http_ucat(), ats_http_ucat(), mRequest.mHeaderBuffer, mRequest.mHeaderSize, 
		mRequest.mBodyBuffer, mRequest.mBodySize );
	
	if( mSession ) mSession->source.lastActivityTicks = UpTicks();
	mRequest.GetHeaderValue( kHTTPHeader_CSeq, &cSeqPtr, &cSeqLen );
	
	// Parse the client device's ID. If not provided (e.g. older device) then fabricate one from the IP address.
	
	mRequest.ScanFHeaderValue( kAirPlayHTTPHeader_DeviceID, "%llx", &mClientDeviceID );
	if( mClientDeviceID == 0 ) mClientDeviceID = SockAddrToDeviceID( &mPeerAddr );
	
	if( *mClientName == '\0' )
	{
		const char *		namePtr	= NULL;
		size_t				nameLen	= 0;
		
		mRequest.GetHeaderValue( kAirPlayHTTPHeader_ClientName, &namePtr, &nameLen );
		if( nameLen > 0 ) TruncateUTF8( namePtr, nameLen, mClientName, sizeof( mClientName ), true );
	}
	
	// Reject all requests if playback is not allowed.
	
	if( !_AirTunesServer_PlaybackAllowed() )
	{
		ats_ulog( kLogLevelNotice, "### RTSP request denied because playback is not allowed\n" );
		status = kHTTPStatus_NotEnoughBandwidth; // Make us look busy to the sender.
		goto SendResponse;
	}
	
#if( AIRPLAY_PASSWORDS )
	// Verify authentication if required.
	
	if( !mHTTPAuthentication_IsAuthenticated && RequestRequiresAuth() )
	{
		if( *gAirPlayReceiverServer->playPassword != '\0' )
		{
			HTTPAuthorizationInfo		authInfo;
			
			memset( &authInfo, 0, sizeof( authInfo ) );
			authInfo.serverScheme			= kHTTPAuthorizationScheme_Digest;
			authInfo.copyPasswordFunction	= AirTunesConnection::HTTPAuthorization_CopyPassword;
			authInfo.copyPasswordContext	= this;
			authInfo.isValidNonceFunction	= AirTunesConnection::HTTPAuthorization_IsValidNonce;
			authInfo.isValidNonceContext	= this;
			authInfo.headerPtr				= mRequest.mHeaderBuffer;
			authInfo.headerLen				= mRequest.mHeaderSize;
			authInfo.requestMethodPtr		= methodPtr;
			authInfo.requestMethodLen		= methodLen;
			authInfo.requestURLPtr			= mRequest.mURLPtr;
			authInfo.requestURLLen			= mRequest.mURLLen;
			status = HTTPVerifyAuthorization( &authInfo );
			if( status != kHTTPStatus_OK )
			{
				goto SendResponse;
			}
		}
		mHTTPAuthentication_IsAuthenticated = true;
	}
#endif
	
#if( AIRPLAY_DACP )
	// Save off the latest DACP info for sending commands back to the controller.
	
	if( mDidAudioSetup )
	{
		uint64_t		dacpID;
		uint32_t		activeRemoteID;
		
		if( mRequest.ScanFHeaderValue( "DACP-ID", "%llX", &dacpID ) == 1 )
		{
			AirTunesDACPClient_SetDACPID( gAirTunesDACPClient, dacpID );
		}
		if( mRequest.ScanFHeaderValue( "Active-Remote", "%u", &activeRemoteID ) == 1 )
		{
			AirTunesDACPClient_SetActiveRemoteID( gAirTunesDACPClient, activeRemoteID );
		}
	}
#endif
	
	// Process the request. Assume success initially, but we'll change it if there is an error.
	// Note: methods are ordered below roughly to how often they are used (most used earlier).
	
	err = mResponse.SetResponseLine( httpProtocol, kHTTPStatus_OK, NULL );
	require_noerr( err, exit );
	mResponse.mBodySize = 0;
	
	if(      strnicmpx( methodPtr, methodLen, "OPTIONS" )			== 0 ) status = ProcessOptions();
	else if( strnicmpx( methodPtr, methodLen, "SET_PARAMETER" )		== 0 ) status = ProcessSetParameter();
	else if( strnicmpx( methodPtr, methodLen, "FLUSH" )				== 0 ) status = ProcessFlush();
	else if( strnicmpx( methodPtr, methodLen, "GET_PARAMETER" ) 	== 0 ) status = ProcessGetParameter();
	else if( strnicmpx( methodPtr, methodLen, "RECORD" )			== 0 ) status = ProcessRecord();
#if( AIRPLAY_SDP_SETUP )
	else if( strnicmpx( methodPtr, methodLen, "ANNOUNCE" )			== 0 ) status = ProcessAnnounce();
#endif
	else if( strnicmpx( methodPtr, methodLen, "SETUP" )				== 0 ) status = ProcessSetup();
	else if( strnicmpx( methodPtr, methodLen, "TEARDOWN" )			== 0 ) status = ProcessTearDown();
	else if( strnicmpx( methodPtr, methodLen, "GET" )				== 0 )
	{
		if( 0 ) {}
		else if( strnicmp_suffix( pathPtr, pathLen, "/log" )		== 0 ) status = ProcessGetLog();
		else if( strnicmp_suffix( pathPtr, pathLen, "/info" )		== 0 ) status = ProcessInfo();
		else { dlog( kLogLevelNotice, "### Unsupported GET: '%.*s'\n", (int) pathLen, pathPtr ); status = kHTTPStatus_NotFound; }
	}
	else if( strnicmpx( methodPtr, methodLen, "POST" )				== 0 )
	{
		if( 0 ) {}
		#if( AIRPLAY_MFI )
		else if( strnicmp_suffix( pathPtr, pathLen, "/auth-setup" )	== 0 ) status = ProcessAuthSetup();
		#endif
		else if( strnicmp_suffix( pathPtr, pathLen, "/command" )	== 0 ) status = ProcessCommand();
		#if( AIRPLAY_MFI )
		else if( strnicmp_suffix( pathPtr, pathLen, "/diag-info" )	== 0 ) status = kHTTPStatus_OK;
		#endif
		else if( strnicmp_suffix( pathPtr, pathLen, "/feedback" )	== 0 ) status = ProcessFeedback();
		else if( strnicmp_suffix( pathPtr, pathLen, "/info" )		== 0 ) status = ProcessInfo();
		#if( AIRPLAY_METRICS )
		else if( strnicmp_suffix( pathPtr, pathLen, "/metrics" )	== 0 ) status = ProcessMetrics();
		#endif
		else { dlogassert( "Bad POST: '%.*s'", (int) pathLen, pathPtr ); status = kHTTPStatus_NotFound; }
	}
	else { dlogassert( "Bad method: %.*s", (int) methodLen, methodPtr ); status = kHTTPStatus_NotImplemented; }
	
SendResponse:
	
	// If an error occurred, reset the response message with a new status.
	
	if( status != kHTTPStatus_OK )
	{
		err = mResponse.SetResponseLine( httpProtocol, status, NULL );
		require_noerr( err, exit );
		mResponse.mBodySize = 0;
		
		err = mResponse.AppendHeaderValueF( kHTTPHeader_ContentLength, "0" );
		require_noerr( err, exit );
	}
	
	// Server
	
	err = mResponse.AppendHeaderValueF( kHTTPHeader_Server, "AirTunes/%s", kAirPlaySourceVersionStr );
	require_noerr( err, exit );
	
	// WWW-Authenticate
	
	if( status == kHTTPStatus_Unauthorized )
	{
		char		nonce[ 64 ];
		
		err = HTTPMakeTimedNonce( kHTTPTimedNonceETagPtr, kHTTPTimedNonceETagLen, 
			mAirTunesServer.mHTTPTimedNonceKey, sizeof( mAirTunesServer.mHTTPTimedNonceKey ), 
			nonce, sizeof( nonce ), NULL );
		require_noerr( err, exit );
		
		err = mResponse.AppendHeaderValueF( kHTTPHeader_WWWAuthenticate, "Digest realm=\"airplay\", nonce=\"%s\"", nonce );
		require_noerr( err, exit );
	}
	
	// CSeq
	
	if( cSeqPtr )
	{
		err = mResponse.AppendHeaderValueF( kHTTPHeader_CSeq, "%.*s", (int) cSeqLen, cSeqPtr );
		require_noerr( err, exit );
	}
	
	// Apple-Challenge

	err = mResponse.Commit();
	require_noerr( err, exit );
	
	if( logHTTP ) LogHTTP( ats_http_ucat(), ats_http_ucat(), mResponse.mHeaderBuffer, mResponse.mHeaderSize, 
		mResponse.mBodyBuffer, mResponse.mBodySize );
	
	err = StartResponse( &mResponse );
	require_noerr( err, exit );
	
exit:
	return( err );
}

#if( AIRPLAY_SDP_SETUP )
//===========================================================================================================================
//	ProcessAnnounce
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessAnnounce( void )
{
	HTTPStatus			status;
	OSStatus			err;
	const char *		src;
	const char *		end;
	const char *		ptr;
	size_t				len;
	const char *		sectionPtr;
	const char *		sectionEnd;
	const char *		valuePtr;
	size_t				valueLen;
	
	ats_ulog( kAirPlayPhaseLogLevel, "Announce\n" );
	
	// Make sure content type is SDP.
	
	src = NULL;
	len = 0;
	mRequest.GetHeaderValue( kHTTPHeader_ContentType, &src, &len );
	if( strnicmpx( src, len, kMIMEType_SDP ) != 0 )
	{
		dlogassert( "bad Content-Type: '%.*s'", (int) len, src );
		status = kHTTPStatus_NotAcceptable;
		goto exit;
	}
	
	// Reject the request if we're not in the right state.
	
	if( mDidAnnounce )
	{
		dlogassert( "ANNOUNCE not allowed twice" );
		status = kHTTPStatus_MethodNotValidInThisState;
		goto exit;
	}
	
	// If we're denying interrupts then reject if there's already an active session.
	// Otherwise, hijack any active session to start the new one (last-in-wins).
	
	if( gAirTunesDenyInterruptions )
	{
		AirTunesConnection *		activeCnx;
		
		activeCnx = mAirTunesServer.FindActiveConnection();
		if( activeCnx )
		{
			ats_ulog( kLogLevelNotice, "Denying interruption from %##a due to %##a\n", &mPeerAddr, &activeCnx->mPeerAddr );
			status = kHTTPStatus_NotEnoughBandwidth;
			goto exit;
		}
	}
	mAirTunesServer.HijackConnections( this );
	
	// Get the session ID from the origin line with the format: <username> <session-id> ...
	
	src = (const char *) mRequest.mBodyBuffer;
	end = src + mRequest.mBodyBufferUsed;
	err = SDPFindSessionSection( src, end, &sectionPtr, &sectionEnd, &src );
	require_noerr_action( err, exit, status = kHTTPStatus_SessionNotFound );
	
	valuePtr = NULL;
	valueLen = 0;
	SDPFindType( sectionPtr, sectionEnd, 'o', &valuePtr, &valueLen, NULL );
	SNScanF( valuePtr, valueLen, "%*s %llu", &mClientSessionID );
	
	// Get the sender's name from the session information.
	
	valuePtr = NULL;
	valueLen = 0;
	SDPFindType( sectionPtr, sectionEnd, 'i', &valuePtr, &valueLen, NULL );
	TruncateUTF8( valuePtr, valueLen, mClientName, sizeof( mClientName ), true );
	
	// Audio
	
	err = SDPFindMediaSection( src, end, NULL, NULL, &ptr, &len, &src );
	require_noerr_action( err, exit, status = kHTTPStatus_NotAcceptable );
	
	if( strncmp_prefix( ptr, len, "audio" ) == 0 )
	{
		err = ProcessAnnounceAudio();
		require_noerr_action( err, exit, status = kHTTPStatus_NotAcceptable );
	}
	
	// Success.
	
	strlcpy( gAirPlayAudioStats.ifname, mIfName, sizeof( gAirPlayAudioStats.ifname ) );
	mDidAnnounce = true;
	status = kHTTPStatus_OK;
	
exit:
	return( status );
}

//===========================================================================================================================
//	ProcessAnnounceAudio
//===========================================================================================================================

OSStatus	AirTunesConnection::ProcessAnnounceAudio( void )
{
	OSStatus			err;
	const char *		src;
	const char *		end;
	const char *		ptr;
	size_t				len;
	const char *		mediaSectionPtr;
	const char *		mediaSectionEnd;
	int					n;
	int					mediaPayloadType;
	int					payloadType;
	const char *		encodingPtr;
	size_t				encodingLen;
	const char *		aesKeyPtr;
#if( AIRTUNES_FAIRPLAY || AIRPLAY_MFI || AIRTUNES_RSA )
	size_t				aesKeyLen;
#endif
	const char *		aesIVPtr;
	size_t				aesIVLen;
	size_t				decodedLen;
	
	src = (const char *) mRequest.mBodyBuffer;
	end = src + mRequest.mBodyBufferUsed;
	
	//-----------------------------------------------------------------------------------------------------------------------
	//	Compression
	//-----------------------------------------------------------------------------------------------------------------------
	
	err = SDPFindMediaSection( src, end, &mediaSectionPtr, &mediaSectionEnd, &ptr, &len, &src );
	require_noerr( err, exit );
	
	n = SNScanF( ptr, len, "audio 0 RTP/AVP %d", &mediaPayloadType );
	require_action( n == 1, exit, err = kUnsupportedErr );
	
	err = SDPFindAttribute( mediaSectionPtr, mediaSectionEnd, "rtpmap", &ptr, &len, NULL );
	require_noerr( err, exit );
	
	n = SNScanF( ptr, len, "%d %&s", &payloadType, &encodingPtr, &encodingLen );
	require_action( n == 2, exit, err = kMalformedErr );
	require_action( payloadType == mediaPayloadType, exit, err = kMismatchErr );
	
	if( 0 ) {} // Empty if to simplify conditional logic below.
	
	// AppleLossless
	
	else if( strnicmpx( encodingPtr, encodingLen, "AppleLossless" ) == 0 )
	{
		int		frameLength, version, bitDepth, pb, mb, kb, channelCount, maxRun, maxFrameBytes, avgBitRate, sampleRate;
		
		// Parse format parameters. For example: a=fmtp:96 352 0 16 40 10 14 2 255 0 0 44100
		
		err = SDPFindAttribute( mediaSectionPtr, mediaSectionEnd, "fmtp", &ptr, &len, NULL );
		require_noerr( err, exit );
		
		n = SNScanF( ptr, len, "%d %d %d %d %d %d %d %d %d %d %d %d", 
			&payloadType, &frameLength, &version, &bitDepth, &pb, &mb, &kb, &channelCount, &maxRun, &maxFrameBytes, 
			&avgBitRate, &sampleRate );
		require_action( n == 12, exit, err = kMalformedErr );
		require_action( payloadType == mediaPayloadType, exit, err = kMismatchErr );
		
		mCompressionType = kAirPlayCompressionType_ALAC;
		mSamplesPerFrame = frameLength;
	}

	// AAC

	// PCM
	
	else if( ( strnicmpx( encodingPtr, encodingLen, "L16/44100/2" )	== 0 ) || 
			 ( strnicmpx( encodingPtr, encodingLen, "L16" )			== 0 ) )
	{
		mCompressionType = kAirPlayCompressionType_PCM;
		mSamplesPerFrame = kAirPlaySamplesPerPacket_PCM;
		ats_ulog( kLogLevelNotice, "*** Not using compression\n" );
	}
	
	// Unknown audio format.
	
	else
	{
		dlogassert( "Unsupported encoding: '%.*s'", (int) encodingLen, encodingPtr );
		err = kUnsupportedDataErr;
		goto exit;
	}
	gAirPlayAudioCompressionType = mCompressionType;
	
	//-----------------------------------------------------------------------------------------------------------------------
	//	Encryption
	//-----------------------------------------------------------------------------------------------------------------------
	
	aesKeyPtr = NULL;
#if( AIRTUNES_FAIRPLAY || AIRPLAY_MFI || AIRTUNES_RSA )
	aesKeyLen = 0;
#endif
	aesIVPtr  = NULL;
	aesIVLen  = 0;
	
	// MFi
	
#if( AIRPLAY_MFI )
	if( !aesKeyPtr )
	{
		SDPFindAttribute( mediaSectionPtr, mediaSectionEnd, "mfiaeskey", &aesKeyPtr, &aesKeyLen, NULL );
		if( aesKeyPtr )
		{
			// Decode and decrypt the AES session key with MFiSAP.
			
			err = Base64Decode( aesKeyPtr, aesKeyLen, mEncryptionKey, sizeof( mEncryptionKey ), &aesKeyLen );
			require_noerr( err, exit );
			require_action( aesKeyLen == sizeof( mEncryptionKey ), exit, err = kSizeErr );
			
			require_action( mMFiSAP, exit, err = kAuthenticationErr );
			err = MFiSAP_Decrypt( mMFiSAP, mEncryptionKey, aesKeyLen, mEncryptionKey );
			require_noerr( err, exit );
		}
	}
#endif
	
	// RSA
	
	// Decode the AES initialization vector (IV).
	
	SDPFindAttribute( mediaSectionPtr, mediaSectionEnd, "aesiv", &aesIVPtr, &aesIVLen, NULL );
	if( aesKeyPtr && aesIVPtr )
	{
		err = Base64Decode( aesIVPtr, aesIVLen, mEncryptionIV, sizeof( mEncryptionIV ), &decodedLen );
		require_noerr( err, exit );
		require_action( decodedLen == sizeof( mEncryptionIV ), exit, err = kSizeErr );
		
		mUsingEncryption = true;
	}
	else if( aesKeyPtr || aesIVPtr )
	{
		ats_ulog( kLogLevelNotice, "### Key/IV missing: key %p/%p\n", aesKeyPtr, aesIVPtr );
		err = kMalformedErr;
		goto exit;
	}
	else
	{
		mUsingEncryption = false;
	}
	
	// Minimum Latency
	
	mMinLatency = kAirTunesPlayoutDelay; // Default value for old clients.
	ptr = NULL;
	len = 0;
	SDPFindAttribute( mediaSectionPtr, mediaSectionEnd, "min-latency", &ptr, &len, NULL );
	if( len > 0 )
	{
		n = SNScanF( ptr, len, "%u", &mMinLatency );
		require_action( n == 1, exit, err = kMalformedErr );
	}
	
	// Maximum Latency
	
	mMaxLatency = kAirPlayAudioLatencyOther; // Default value for old clients.
	ptr = NULL;
	len = 0;
	SDPFindAttribute( mediaSectionPtr, mediaSectionEnd, "max-latency", &ptr, &len, NULL );
	if( len > 0 )
	{
		n = SNScanF( ptr, len, "%u", &mMaxLatency );
		require_action( n == 1, exit, err = kMalformedErr );
	}
	
exit:
	return( err );
}
#endif // AIRPLAY_SDP_SETUP

//===========================================================================================================================
//	ProcessAuthSetup
//===========================================================================================================================

#if( AIRPLAY_MFI )
HTTPStatus	AirTunesConnection::ProcessAuthSetup( void )
{
	HTTPStatus		status;
	OSStatus		err;
	uint8_t *		outputPtr;
	size_t			outputLen;
	
	ats_ulog( kAirPlayPhaseLogLevel, "MFi\n" );
	outputPtr = NULL;
	require_action( mRequest.mBodyBufferUsed > 0, exit, status = kHTTPStatus_BadRequest );
	
	// Let MFi-SAP process the input data and generate output data.
	
	if( mMFiSAPDone && mMFiSAP )
	{
		MFiSAP_Delete( mMFiSAP );
		mMFiSAP = NULL;
		mMFiSAPDone = false;
	}
	if( !mMFiSAP )
	{
		err = MFiSAP_Create( &mMFiSAP, kMFiSAPVersion1 );
		require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	}
	
	err = MFiSAP_Exchange( mMFiSAP, mRequest.mBodyBuffer, mRequest.mBodyBufferUsed, &outputPtr, &outputLen, &mMFiSAPDone );
	require_noerr_action( err, exit, status = kHTTPStatus_Forbidden );
	
	// Send the MFi-SAP output data in the response.
	
	require_action( outputLen <= sizeof( mResponseBodyBuffer ), exit, status = kHTTPStatus_InternalServerError );
	
	err = mResponse.AppendHeaderValueF( kHTTPHeader_ContentType, kMIMEType_Binary );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	err = mResponse.AppendHeaderValueF( kHTTPHeader_ContentLength, "%zu", outputLen );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	memcpy( mResponseBodyBuffer, outputPtr, outputLen );
	mResponse.SetBody( mResponseBodyBuffer, outputLen );
	
	status = kHTTPStatus_OK;
	
exit:
	if( outputPtr ) free( outputPtr );
	return( status );
}
#endif

//===========================================================================================================================
//	ProcessCommand
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessCommand( void )
{
	HTTPStatus			status;
	OSStatus			err;
	CFDictionaryRef		request;
	CFStringRef			command = NULL;
	CFDictionaryRef		params;
	CFDictionaryRef		response;
	
	request = CFDictionaryCreateWithBytes( mRequest.mBodyBuffer, mRequest.mBodyBufferUsed, &err );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	command = CFDictionaryGetCFString( request, CFSTR( kAirPlayKey_Type ), NULL );
	require_action( command, exit, err = kParamErr; status = kHTTPStatus_ParameterNotUnderstood );
	
	params = CFDictionaryGetCFDictionary( request, CFSTR( kAirPlayKey_Params ), NULL );
	
	// Perform the command and send its response.
	
	require_action( mSession, exit, err = kNotPreparedErr; status = kHTTPStatus_SessionNotFound );
	response = NULL;
	err = AirPlayReceiverSessionControl( mSession, kCFObjectFlagDirect, command, NULL, params, &response );
	require_noerr_action_quiet( err, exit, status = kHTTPStatus_UnprocessableEntity );
	if( !response )
	{
		response = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
		require_action( response, exit, err = kNoMemoryErr; status = kHTTPStatus_InternalServerError );
	}
	
	status = SendPlistResponse( response, &err );
	CFRelease( response );
	
exit:
	if( err ) ats_ulog( kLogLevelNotice, "### Command '%@' failed: %#m, %#m\n", command, status, err );
	CFReleaseNullSafe( request );
	return( status );
}

//===========================================================================================================================
//	ProcessConfig
//===========================================================================================================================

//===========================================================================================================================
//	ProcessFeedback
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessFeedback( void )
{
	HTTPStatus					status;
	OSStatus					err;
	CFDictionaryRef				input  = NULL;
	CFMutableDictionaryRef		output = NULL;
	CFDictionaryRef				dict;
	
	input = CFDictionaryCreateWithBytes( mRequest.mBodyBuffer, mRequest.mBodyBufferUsed, &err );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	output = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	require_action( output, exit, err = kNoMemoryErr; status = kHTTPStatus_InternalServerError );
	
	if( mSession )
	{
		dict = NULL;
		AirPlayReceiverSessionControl( mSession, kCFObjectFlagDirect, CFSTR( kAirPlayCommand_UpdateFeedback ), NULL, 
			input, &dict );
		if( dict )
		{
			CFDictionaryMergeDictionary( output, dict );
			CFRelease( dict );
		}
	}
	
	status = SendPlistResponse( ( CFDictionaryGetCount( output ) > 0 ) ? output : NULL, &err );
	
exit:
	if( err ) ats_ulog( kLogLevelNotice, "### Feedback failed: %#m, %#m\n", status, err );
	CFReleaseNullSafe( input );
	CFReleaseNullSafe( output );
	return( status );
}

//===========================================================================================================================
//	ProcessFlush
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessFlush( void )
{
	OSStatus		err;
	HTTPStatus		status;
	uint16_t		flushSeq;
	uint32_t		flushTS;
	uint32_t		lastPlayedTS;
	
	ats_ulog( kLogLevelInfo, "Flush\n" );
	
	if( !mDidRecord )
	{
		dlogassert( "FLUSH not allowed when not playing" );
		status = kHTTPStatus_MethodNotValidInThisState;
		goto exit;
	}
	
	// Flush everything before the specified seq/ts.
	
	err = HTTPParseRTPInfo( mRequest.mHeaderBuffer, mRequest.mHeaderSize, &flushSeq, &flushTS );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	err = AirPlayReceiverSessionFlushAudio( mSession, flushTS, flushSeq, &lastPlayedTS );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	err = mResponse.AppendHeaderValueF( kHTTPHeader_RTPInfo, "rtptime=%u", lastPlayedTS );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	status = kHTTPStatus_OK;
	
exit:
	return( status );
}

//===========================================================================================================================
//	ProcessGetLog
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessGetLog( void )
{
	HTTPStatus		status;
	OSStatus		err;
	DataBuffer		dataBuf;
	uint8_t *		ptr;
	size_t			len;
	
	DataBuffer_Init( &dataBuf, NULL, 0, 10 * kBytesPerMegaByte );
	
	{
		ats_ulog( kLogLevelNotice, "### Rejecting log request from non-internal build\n" );
		status = kHTTPStatus_NotFound;
		goto exit;
	}
	
	DataBuffer_AppendF( &dataBuf, "AirPlay Diagnostics\n" );
	DataBuffer_AppendF( &dataBuf, "===================\n" );
	AirTunesDebugAppendShowData( "globals", &dataBuf );
	AirTunesDebugAppendShowData( "stats", &dataBuf );
	AirTunesDebugAppendShowData( "rtt", &dataBuf );
	AirTunesDebugAppendShowData( "retrans", &dataBuf );
	AirTunesDebugAppendShowData( "retransDone", &dataBuf );
	
	DataBuffer_AppendF( &dataBuf, "+-+ syslog +--\n" );
	DataBuffer_RunProcessAndAppendOutput( &dataBuf, "syslog" );
	DataBuffer_AppendFile( &dataBuf, kAirPlayPrimaryLogPath );
	
	err = DataBuffer_Commit( &dataBuf, &ptr, &len );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	dataBuf.bufferPtr = NULL;
	
	mResponse.SetBody( ptr, len );
	mResponse.mBodyMallocedBuffer = ptr;
	mResponse.AppendHeaderValueF( kHTTPHeader_ContentType, "text/plain" );
	mResponse.AppendHeaderValueF( kHTTPHeader_ContentLength, "%zu", len );
	status = kHTTPStatus_OK;
	
exit:
	DataBuffer_Free( &dataBuf );
	return( status );
}

//===========================================================================================================================
//	ProcessGetLogs
//===========================================================================================================================

//===========================================================================================================================
//	ProcessInfo
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessInfo( void )
{
	HTTPStatus					status;
	OSStatus					err;
	CFMutableDictionaryRef		request;
	CFMutableArrayRef			qualifier = NULL;
	CFDictionaryRef				response;
	const char *				src;
	const char *				end;
	const char *				namePtr;
	size_t						nameLen;
	char *						nameBuf;
	
	request = CFDictionaryCreateMutableWithBytes( mRequest.mBodyBuffer, mRequest.mBodyBufferUsed, &err );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	qualifier = (CFMutableArrayRef) CFDictionaryGetCFArray( request, CFSTR( kAirPlayKey_Qualifier ), NULL );
	if( qualifier ) CFRetain( qualifier );
	
	src = mRequest.mURLComps.queryPtr;
	end = src + mRequest.mURLComps.queryLen;
	while( ( err = URLGetOrCopyNextVariable( src, end, &namePtr, &nameLen, &nameBuf, NULL, NULL, NULL, &src ) ) == kNoErr )
	{
		err = CFArrayEnsureCreatedAndAppendCString( &qualifier, namePtr, nameLen );
		if( nameBuf ) free( nameBuf );
		require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	}
	
	response = AirPlayCopyServerInfo( mSession, qualifier, &err );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	status = SendPlistResponse( response, &err );
	CFReleaseNullSafe( response );
	
exit:
	CFReleaseNullSafe( qualifier );
	CFReleaseNullSafe( request );
	if( err ) ats_ulog( kLogLevelNotice, "### Get info failed: %#m, %#m\n", status, err );
	return( status );
}

#if( AIRPLAY_METRICS )
//===========================================================================================================================
//	ProcessMetrics
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessMetrics( void )
{
	HTTPStatus			status;
	OSStatus			err;
	CFDictionaryRef		input	= NULL;
	CFDictionaryRef		metrics	= NULL;
	
	ats_ulog( kAirPlayPhaseLogLevel, "Metrics\n" );
	require_action_quiet( mSession, exit, err = kNotInUseErr; status = kHTTPStatus_SessionNotFound );
	
	input = CFDictionaryCreateWithBytes( mRequest.mBodyBuffer, mRequest.mBodyBufferUsed, &err );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	metrics = (CFDictionaryRef) AirPlayReceiverSessionCopyProperty( mSession, kCFObjectFlagDirect, 
		CFSTR( kAirPlayProperty_Metrics ), input, NULL );
	if( !metrics )
	{
		metrics = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
		require_action( metrics, exit, err = kNoMemoryErr; status = kHTTPStatus_InternalServerError );
	}
	
	status = SendPlistResponse( metrics, &err );
	
exit:
	if( err ) ats_ulog( kLogLevelNotice, "### Metrics failed: %#m, %#m\n", status, err );
	CFReleaseNullSafe( input );
	CFReleaseNullSafe( metrics );
	return( status );
}
#endif

//===========================================================================================================================
//	ProcessOptions
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessOptions( void )
{
	mResponse.AppendHeaderValueF( kHTTPHeader_Public, 
		"ANNOUNCE, SETUP, RECORD, PAUSE, FLUSH, TEARDOWN, OPTIONS, GET_PARAMETER, SET_PARAMETER, POST, GET" );
	return( kHTTPStatus_OK );
}

//===========================================================================================================================
//	ProcessPairSetup
//===========================================================================================================================

//===========================================================================================================================
//	ProcessPairVerify
//===========================================================================================================================

//===========================================================================================================================
//	ProcessGetParameter
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessGetParameter( void )
{
	HTTPStatus			status;
	OSStatus			err;
	const char *		src;
	const char *		end;
	size_t				len;
	char				tempStr[ 256 ];
	int					n;
	
	if( !AirTunesConnection_DidSetup() )
	{
		dlogassert( "GET_PARAMETER not allowed before SETUP" );
		status = kHTTPStatus_MethodNotValidInThisState;
		goto exit;
	}
	
	// Check content type.
	
	err = mRequest.GetHeaderValue( kHTTPHeader_ContentType, &src, &len );
	if( err )
	{
		dlogassert( "No Content-Type header" );
		status = kHTTPStatus_BadRequest;
		goto exit;
	}
	if( strnicmpx( src, len, "text/parameters" ) != 0 )
	{
		dlogassert( "Bad Content-Type: '%.*s'", (int) len, src );
		status = kHTTPStatus_HeaderFieldNotValid;
		goto exit;
	}
	
	// Parse parameters. Each parameter is formatted as <name>\r\n
	
	src = (const char *) mRequest.mBodyBuffer;
	end = src + mRequest.mBodyBufferUsed;
	while( src < end )
	{
		char				c;
		const char *		namePtr;
		size_t				nameLen;
		
		namePtr = src;
		while( ( src < end ) && ( ( c = *src ) != '\r' ) && ( c != '\n' ) ) ++src;
		nameLen = (size_t)( src - namePtr );
		if( ( nameLen == 0 ) || ( src >= end ) )
		{
			dlogassert( "Bad parameter: '%.*s'", (int) mRequest.mBodyBufferUsed, mRequest.mBodyBuffer );
			status = kHTTPStatus_ParameterNotUnderstood;
			goto exit;
		}
		
		// Process the parameter.
		
		if( 0 ) {}
		
		#if( AIRPLAY_VOLUME_CONTROL )
		// Volume
		
		else if( strnicmpx( namePtr, nameLen, "volume" ) == 0 )
		{
			Float32		dbVolume;
			
			dbVolume = (Float32) AirPlayReceiverSessionPlatformGetDouble( mSession, CFSTR( kAirPlayProperty_Volume ), 
				NULL, &err );
			require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
			
			n = snprintf( mResponseBodyBuffer, sizeof( mResponseBodyBuffer ), "volume: %f\r\n", dbVolume );
			mResponse.SetBody( mResponseBodyBuffer, (size_t) n );
			
			err = mResponse.AppendHeaderValueF( kHTTPHeader_ContentType, "text/parameters" );
			require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
			
			err = mResponse.AppendHeaderValueF( kHTTPHeader_ContentLength, "%u", n );
			require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
		}
		#endif
		
		// Name
		
		else if( strnicmpx( namePtr, nameLen, "name" ) == 0 )
		{
			err = AirPlayGetDeviceName( tempStr, sizeof( tempStr ) );
			require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
			
			n = snprintf( mResponseBodyBuffer, sizeof( mResponseBodyBuffer ), "name: %s\r\n", tempStr );
			mResponse.SetBody( mResponseBodyBuffer, (size_t) n );
		}
		
		// Other
		
		else
		{
			dlogassert( "Unknown parameter: '%.*s'", (int) nameLen, namePtr );
			status = kHTTPStatus_ParameterNotUnderstood;
			goto exit;
		}
		
		while( ( src < end ) && ( ( ( c = *src ) == '\r' ) || ( c == '\n' ) ) ) ++src;
	}
	
	status = kHTTPStatus_OK;
	
exit:
	return( status );
}

//===========================================================================================================================
//	ProcessSetParameter
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessSetParameter( void )
{
	HTTPStatus			status;
	const char *		src;
	size_t				len;
	
	if( !AirTunesConnection_DidSetup() )
	{
		dlogassert( "SET_PARAMETER not allowed before SETUP" );
		status = kHTTPStatus_MethodNotValidInThisState;
		goto exit;
	}
	
	src = NULL;
	len = 0;
	mRequest.GetHeaderValue( kHTTPHeader_ContentType, &src, &len );
	if( ( len == 0 ) && ( mRequest.mBodyBufferUsed == 0 ) )		status = kHTTPStatus_OK;
#if( AIRPLAY_META_DATA )
	else if( strnicmp_prefix( src, len, "image/" ) == 0 )		status = ProcessSetParameterArtwork( src, len );
	else if( strnicmpx( src, len, kMIMEType_DMAP ) == 0 )		status = ProcessSetParameterDMAP();
#endif
	else if( strnicmpx( src, len, "text/parameters" ) == 0 )	status = ProcessSetParameterText();
	else if( MIMETypeIsPlist( src, len ) )						status = ProcessSetProperty();
	else { dlogassert( "Bad Content-Type: '%.*s'", (int) len, src ); status = kHTTPStatus_HeaderFieldNotValid; goto exit; }
	
exit:
	return( status );
}

#if( AIRPLAY_META_DATA )
//===========================================================================================================================
//	ProcessSetParameterArtwork
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessSetParameterArtwork( const char *inMIMETypeStr, size_t inMIMETypeLen )
{
	HTTPStatus					status;
	OSStatus					err;
	uint32_t					u32;
	CFNumberRef					applyTS  = NULL;
	CFMutableDictionaryRef		metaData = NULL;
	
	err = HTTPParseRTPInfo( mRequest.mHeaderBuffer, mRequest.mHeaderSize, NULL, &u32 );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	applyTS = CFNumberCreateInt64( u32 );
	require_action( applyTS, exit, status = kHTTPStatus_InternalServerError );
	
	metaData = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	require_action( metaData, exit, status = kHTTPStatus_InternalServerError );
	
	CFDictionarySetCString( metaData, kAirPlayMetaDataKey_ArtworkMIMEType, inMIMETypeStr, inMIMETypeLen );
	if( mRequest.mBodyBufferUsed > 0 )
	{
		CFDictionarySetData( metaData, kAirPlayMetaDataKey_ArtworkData, mRequest.mBodyBuffer, mRequest.mBodyBufferUsed );
	}
	else
	{
		CFDictionarySetValue( metaData, kAirPlayMetaDataKey_ArtworkData, kCFNull );
	}
	
	err = AirPlayReceiverSessionSetProperty( mSession, kCFObjectFlagDirect, CFSTR( kAirPlayProperty_MetaData ), 
		applyTS, metaData );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	status = kHTTPStatus_OK;
	
exit:
	CFReleaseNullSafe( applyTS );
	CFReleaseNullSafe( metaData );
	return( status );
}

//===========================================================================================================================
//	ProcessSetParameterDMAP
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessSetParameterDMAP( void )
{
	HTTPStatus					status;
	OSStatus					err;
	CFNumberRef					applyTS  = NULL;
	CFMutableDictionaryRef		metaData = NULL;
	DMAPContentCode				code;
	const uint8_t *				src;
	const uint8_t *				end;
	const uint8_t *				ptr;
	size_t						len;
	int64_t						persistentID = 0, itemID = 0, songID = 0;
	Boolean						hasPersistentID = false, hasItemID = false, hasSongID = false;
	int64_t						s64;
	uint32_t					u32;
	uint8_t						u8;
	double						d;
	CFTypeRef					tempObj;
	
	err = HTTPParseRTPInfo( mRequest.mHeaderBuffer, mRequest.mHeaderSize, NULL, &u32 );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	applyTS = CFNumberCreateInt64( u32 );
	require_action( applyTS, exit, status = kHTTPStatus_InternalServerError );
	
	metaData = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	require_action( metaData, exit, status = kHTTPStatus_InternalServerError );
	
	end = mRequest.mBodyBuffer + mRequest.mBodyBufferUsed;
	err = DMAPContentBlock_GetNextChunk( mRequest.mBodyBuffer, end, &code, &len, &src, NULL );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	require_action( code == kDMAPListingItemCode, exit, status = kHTTPStatus_BadRequest );
	
	end = src + len;
	while( DMAPContentBlock_GetNextChunk( src, end, &code, &len, &ptr, &src ) == kNoErr )
	{
		switch( code )
		{
			case kDAAPSongAlbumCode:
				CFDictionarySetCString( metaData, kAirPlayMetaDataKey_Album, ptr, len );
				break;
			
			case kDAAPSongArtistCode:
				CFDictionarySetCString( metaData, kAirPlayMetaDataKey_Artist, ptr, len );
				break;
			
			case kDAAPSongComposerCode:
				CFDictionarySetCString( metaData, kAirPlayMetaDataKey_Composer, ptr, len );
				break;
			
			case kDAAPSongGenreCode:
				CFDictionarySetCString( metaData, kAirPlayMetaDataKey_Genre, ptr, len );
				break;
			
			case kDMAPItemNameCode:
				CFDictionarySetCString( metaData, kAirPlayMetaDataKey_Title, ptr, len );
				break;
			
			case kDAAPSongDataKindCode:
				CFDictionarySetBoolean( metaData, kAirPlayMetaDataKey_IsStream, 
					( ( len == 1 ) && ( ptr[ 0 ] == kDAAPSongDataKind_RemoteStream ) ) );
				break;
			
			case kDAAPSongTrackNumberCode:
				if( len != 2 ) { dlogassert( "### Bad track number length: %zu\n", len ); break; }
				s64 = ReadBig16( ptr );
				CFDictionarySetInt64( metaData, kAirPlayMetaDataKey_TrackNumber, s64 );
				break;
			
			case kDAAPSongTrackCountCode:
				if( len != 2 ) { dlogassert( "### Bad track count length: %zu\n", len ); break; }
				s64 = ReadBig16( ptr );
				CFDictionarySetInt64( metaData, kAirPlayMetaDataKey_TotalTracks, s64 );
				break;
			
			case kDAAPSongDiscNumberCode:
				if( len != 2 ) { dlogassert( "### Bad disc number length: %zu\n", len ); break; }
				s64 = ReadBig16( ptr );
				CFDictionarySetInt64( metaData, kAirPlayMetaDataKey_DiscNumber, s64 );
				break;
			
			case kDAAPSongDiscCountCode:
				if( len != 2 ) { dlogassert( "### Bad disc count length: %zu\n", len ); break; }
				s64 = ReadBig16( ptr );
				CFDictionarySetInt64( metaData, kAirPlayMetaDataKey_TotalDiscs, s64 );
				break;
			
			case kDMAPPersistentIDCode:
				if( len != 8 ) { dlogassert( "### Bad persistent ID length: %zu\n", len ); break; }
				persistentID = ReadBig64( ptr );
				hasPersistentID = true;
				break;
			
			case kDAAPSongTimeCode:
				if( len != 4 ) { dlogassert( "### Bad song time length: %zu\n", len ); break; }
				s64 = ReadBig32( ptr );
				CFDictionarySetDouble( metaData, kAirPlayMetaDataKey_Duration, s64 / 1000.0 /* ms -> secs */ );
				break;
			
			case kDMAPItemIDCode:
				if( len != 4 ) { dlogassert( "### Bad item ID length: %zu\n", len ); break; }
				itemID = ReadBig32( ptr );
				hasItemID = true;
				break;
			
			case kExtDAAPITMSSongIDCode:
				if( len != 4 ) { dlogassert( "### Bad song ID length: %zu\n", len ); break; }
				songID = ReadBig32( ptr );
				hasSongID = true;
				break;
			
			case kDACPPlayerStateCode:
				if( len != 1 ) { dlogassert( "### Bad player state length: %zu\n", len ); break; }
				u8 = *ptr;
				if(      u8 == kDACPPlayerState_Paused )	d = 0.0;
				else if( u8 == kDACPPlayerState_Stopped )	d = 0.0;
				else if( u8 == kDACPPlayerState_FastFwd )	d = 2.0;
				else if( u8 == kDACPPlayerState_Rewind )	d = -1.0;
				else										d = 1.0;
				CFDictionarySetDouble( metaData, kAirPlayMetaDataKey_Rate, d );
				break;
			
			case 0x63654A56: // 'ceJV'
			case 0x63654A43: // 'ceJC'
			case 0x63654A53: // 'ceJS'
				// These aren't needed, but are sent by some clients so ignore to avoid an assert about it.
				break;
			
			default:
				#if( DEBUG )
				{
					const DMAPContentCodeEntry *		e;
					
					e = DMAPFindEntryByContentCode( code );
					if( e ) dlog( kLogLevelChatty,  "Unhandled DMAP: '%C'  %-36s  %s\n", code, e->name, e->codeSymbol );
				}
				#endif
				break;
		}
	}
	if(      hasPersistentID )	CFDictionarySetInt64( metaData, kAirPlayMetaDataKey_UniqueID, persistentID );
	else if( hasItemID )		CFDictionarySetInt64( metaData, kAirPlayMetaDataKey_UniqueID, itemID );
	else if( hasSongID )		CFDictionarySetInt64( metaData, kAirPlayMetaDataKey_UniqueID, songID );
	else
	{
		// Emulate a unique ID from other info that's like to change with each new song.
		
		s64 = 0;
		
		tempObj = CFDictionaryGetValue( metaData, kAirPlayMetaDataKey_Album );
		if( tempObj ) s64 ^= CFHash( tempObj );
		
		tempObj = CFDictionaryGetValue( metaData, kAirPlayMetaDataKey_Artist );
		if( tempObj ) s64 ^= CFHash( tempObj );
		
		tempObj = CFDictionaryGetValue( metaData, kAirPlayMetaDataKey_Title );
		if( tempObj ) s64 ^= CFHash( tempObj );
		
		tempObj = CFDictionaryGetValue( metaData, kAirPlayMetaDataKey_TrackNumber );
		if( tempObj ) s64 ^= CFHash( tempObj );
		
		CFDictionarySetInt64( metaData, kAirPlayMetaDataKey_UniqueID, s64 );
	}
	
	err = AirPlayReceiverSessionSetProperty( mSession, kCFObjectFlagDirect, CFSTR( kAirPlayProperty_MetaData ), 
		applyTS, metaData );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	status = kHTTPStatus_OK;
	
exit:
	CFReleaseNullSafe( applyTS );
	CFReleaseNullSafe( metaData );
	return( status );
}
#endif // AIRPLAY_META_DATA

//===========================================================================================================================
//	ProcessSetParameterText
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessSetParameterText( void )
{
	HTTPStatus			status;
	OSStatus			err;
	const char *		src;
	const char *		end;
	
	// Parse parameters. Each parameter is formatted as <name>: <value>\r\n
	
	src = (const char *) mRequest.mBodyBuffer;
	end = src + mRequest.mBodyBufferUsed;
	while( src < end )
	{
		char				c;
		const char *		namePtr;
		size_t				nameLen;
		const char *		valuePtr;
		size_t				valueLen;
		
		// Parse the name.
		
		namePtr = src;
		while( ( src < end ) && ( *src != ':' ) ) ++src;
		nameLen = (size_t)( src - namePtr );
		if( ( nameLen == 0 ) || ( src >= end ) )
		{
			dlogassert( "Bad parameter: '%.*s'", (int) mRequest.mBodyBufferUsed, mRequest.mBodyBuffer );
			status = kHTTPStatus_ParameterNotUnderstood;
			goto exit;
		}
		++src;
		while( ( src < end ) && ( ( ( c = *src ) == ' ' ) || ( c == '\t' ) ) ) ++src;
		
		// Parse the value.
		
		valuePtr = src;
		while( ( src < end ) && ( ( c = *src ) != '\r' ) && ( c != '\n' ) ) ++src;
		valueLen = (size_t)( src - valuePtr );
		
		// Process the parameter.
		
		if( 0 ) {}
		
		#if( AIRPLAY_VOLUME_CONTROL )
		// Volume
		
		else if( strnicmpx( namePtr, nameLen, "volume" ) == 0 )
		{
			char		tempStr[ 256 ];
			
			require_action( valueLen < sizeof( tempStr ), exit, status = kHTTPStatus_HeaderFieldNotValid );
			memcpy( tempStr, valuePtr, valueLen );
			tempStr[ valueLen ] = '\0';
			
			mAirTunesServer.mVolume = strtof( tempStr, NULL );
			err = AirPlayReceiverSessionPlatformSetDouble( mSession, CFSTR( kAirPlayProperty_Volume ), 
				NULL, mAirTunesServer.mVolume );
			require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
		}
		#endif
		
		#if( AIRPLAY_META_DATA )
		// Progress
		
		else if( strnicmpx( namePtr, nameLen, "progress" ) == 0 )
		{
			unsigned int				startTS, currentTS, stopTS;
			int							n;
			CFMutableDictionaryRef		progressDict;
			
			n = SNScanF( valuePtr, valueLen, "%u/%u/%u", &startTS, &currentTS, &stopTS );
			require_action( n == 3, exit, status = kHTTPStatus_HeaderFieldNotValid );
			
			progressDict = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
			require_action( progressDict, exit, status = kHTTPStatus_InternalServerError );
			CFDictionarySetInt64( progressDict, CFSTR( kAirPlayKey_StartTimestamp ), startTS );
			CFDictionarySetInt64( progressDict, CFSTR( kAirPlayKey_CurrentTimestamp ), currentTS );
			CFDictionarySetInt64( progressDict, CFSTR( kAirPlayKey_EndTimestamp ), stopTS );
			
			err = AirPlayReceiverSessionSetProperty( mSession, kCFObjectFlagDirect, CFSTR( kAirPlayProperty_Progress ), 
				NULL, progressDict );
			CFRelease( progressDict );
			require_noerr_action_quiet( err, exit, status = kHTTPStatus_UnprocessableEntity );
		}
		#endif
		
		// Other
		
		else
		{
			(void) err;
			(void) valueLen;
			
			dlogassert( "Unknown parameter: '%.*s'", (int) nameLen, namePtr );
			status = kHTTPStatus_ParameterNotUnderstood;
			goto exit;
		}
		
		while( ( src < end ) && ( ( ( c = *src ) == '\r' ) || ( c == '\n' ) ) ) ++src;
	}
	
	status = kHTTPStatus_OK;
	
exit:
	return( status );
}

//===========================================================================================================================
//	ProcessSetProperty
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessSetProperty( void )
{
	HTTPStatus			status;
	OSStatus			err;
	CFDictionaryRef		request;
	CFStringRef			property = NULL;
	CFTypeRef			qualifier;
	CFTypeRef			value;
	
	request = CFDictionaryCreateWithBytes( mRequest.mBodyBuffer, mRequest.mBodyBufferUsed, &err );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	property = CFDictionaryGetCFString( request, CFSTR( kAirPlayKey_Property ), NULL );
	require_action( property, exit, err = kParamErr; status = kHTTPStatus_BadRequest );
	
	qualifier	= CFDictionaryGetValue( request, CFSTR( kAirPlayKey_Qualifier ) );
	value		= CFDictionaryGetValue( request, CFSTR( kAirPlayKey_Value ) );
	
	// Set the property on the session.
	
	require_action( mSession, exit, err = kNotPreparedErr; status = kHTTPStatus_SessionNotFound );
	err = AirPlayReceiverSessionSetProperty( mSession, kCFObjectFlagDirect, property, qualifier, value );
	require_noerr_action_quiet( err, exit, status = kHTTPStatus_UnprocessableEntity );
	
	status = kHTTPStatus_OK;
	
exit:
	if( err ) ats_ulog( kLogLevelNotice, "### Set property '%@' failed: %#m, %#m\n", property, status, err );
	CFReleaseNullSafe( request );
	return( status );
}

//===========================================================================================================================
//	ProcessPerf
//===========================================================================================================================

//===========================================================================================================================
//	ProcessRecord
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessRecord( void )
{
	HTTPStatus							status;
	OSStatus							err;
	const char *						src;
	const char *						end;
	size_t								len;
	const char *						namePtr;
	size_t								nameLen;
	const char *						valuePtr;
	size_t								valueLen;
	AirPlayReceiverSessionStartInfo		startInfo;
	
	ats_ulog( kAirPlayPhaseLogLevel, "Record\n" );
	
	if( !AirTunesConnection_DidSetup() )
	{
		dlogassert( "RECORD not allowed before SETUP" );
		status = kHTTPStatus_MethodNotValidInThisState;
		goto exit;
	}
	
	memset( &startInfo, 0, sizeof( startInfo ) );
	startInfo.clientName	= mClientName;
	startInfo.transportType	= mTransportType;
	
	// Parse session duration info.
	
	src = NULL;
	len = 0;
	mRequest.GetHeaderValue( kAirPlayHTTPHeader_Durations, &src, &len );
	end = src + len;
	while( HTTPParseParameter( src, end, &namePtr, &nameLen, &valuePtr, &valueLen, NULL, &src ) == kNoErr )
	{
		if(      strnicmpx( namePtr, nameLen, "b" )  == 0 ) startInfo.bonjourMs		= TextToInt32( valuePtr, valueLen, 10 );
		else if( strnicmpx( namePtr, nameLen, "c" )  == 0 ) startInfo.connectMs		= TextToInt32( valuePtr, valueLen, 10 );
		else if( strnicmpx( namePtr, nameLen, "au" ) == 0 ) startInfo.authMs		= TextToInt32( valuePtr, valueLen, 10 );
		else if( strnicmpx( namePtr, nameLen, "an" ) == 0 ) startInfo.announceMs	= TextToInt32( valuePtr, valueLen, 10 );
		else if( strnicmpx( namePtr, nameLen, "sa" ) == 0 ) startInfo.setupAudioMs	= TextToInt32( valuePtr, valueLen, 10 );
	}
	
	// Start the session.
	
#if( AIRPLAY_VOLUME_CONTROL )
	err = AirPlayReceiverSessionPlatformSetDouble( mSession, CFSTR( kAirPlayProperty_Volume ), NULL, mAirTunesServer.mVolume );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
#endif
	
	err = AirPlayReceiverSessionStart( mSession, &startInfo );
	if( AirPlayIsBusyError( err ) ) { status = kHTTPStatus_NotEnoughBandwidth; goto exit; }
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	err = mResponse.AppendHeaderValueF( "Audio-Latency", "%u", mSession->platformAudioLatency );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	mDidRecord = true;
	status = kHTTPStatus_OK;
	
exit:
	return( status );
}

//===========================================================================================================================
//	ProcessSetup
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessSetup( void )
{
#if( AIRPLAY_SDP_SETUP )
	OSStatus			err;
#endif
	HTTPStatus			status;
	const char *		ptr;
	size_t				len;
	
	ptr = NULL;
	len = 0;
	mRequest.GetHeaderValue( kHTTPHeader_ContentType, &ptr, &len );
	if( MIMETypeIsPlist( ptr, len ) )
	{
		status = ProcessSetupPlist();
		goto exit;
	}
	
#if( AIRPLAY_SDP_SETUP )
	if( !mDidAnnounce )
	{
		dlogassert( "SETUP not allowed before ANNOUNCE" );
		status = kHTTPStatus_MethodNotValidInThisState;
		goto exit;
	}
	
	err = URLGetNextPathSegment( &mRequest.mURLComps, &ptr, &len );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	ptr = "audio"; // Default to audio for older audio-only clients that don't append /audio.
	len = 5;
	URLGetNextPathSegment( &mRequest.mURLComps, &ptr, &len );
	if(      strnicmpx( ptr, len, "audio" ) == 0 ) status = ProcessSetupSDPAudio();
	else
#endif
	{
		dlogassert( "Bad setup URL '%.*s'", (int) mRequest.mURLLen, mRequest.mURLPtr );
		status = kHTTPStatus_BadRequest;
		goto exit;
	}
	
exit:
	return( status );
}

#if( AIRPLAY_SDP_SETUP )
//===========================================================================================================================
//	ProcessSetupSDPAudio
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessSetupSDPAudio( void )
{
	HTTPStatus					status;
	OSStatus					err;
	CFMutableDictionaryRef		requestParams		= NULL;
	CFMutableArrayRef			requestStreams		= NULL;
	CFMutableDictionaryRef		requestStreamDesc	= NULL;
	CFDictionaryRef				responseParams		= NULL;
	CFArrayRef					outputStreams;
	CFDictionaryRef				outputAudioStreamDesc;
	CFIndex						i, n;
	const char *				src;
	const char *				end;
	char *						dst;
	char *						lim;
	size_t						len;
	const char *				namePtr;
	size_t						nameLen;
	const char *				valuePtr;
	size_t						valueLen;
	char						tempStr[ 128 ];
	int							tempInt;
	int							dataPort, controlPort, eventPort, timingPort;
	Boolean						useEvents = false;
	
	ats_ulog( kAirPlayPhaseLogLevel, "Setup audio\n" );
	
	if( !mDidAnnounce )
	{
		dlogassert( "SETUP not allowed before ANNOUNCE" );
		status = kHTTPStatus_MethodNotValidInThisState;
		goto exit;
	}
	if( mDidAudioSetup )
	{
		dlogassert( "SETUP audio not allowed twice" );
		status = kHTTPStatus_MethodNotValidInThisState;
		goto exit;
	}
	
	requestParams = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	require_action( requestParams, exit, status = kHTTPStatus_InternalServerError );
	
	requestStreams = CFArrayCreateMutable( NULL, 0, &kCFTypeArrayCallBacks );
	require_action( requestStreams, exit, status = kHTTPStatus_InternalServerError );
	CFDictionarySetValue( requestParams, CFSTR( kAirPlayKey_Streams ), requestStreams );
	
	requestStreamDesc = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	require_action( requestStreamDesc, exit, status = kHTTPStatus_InternalServerError );
	CFArrayAppendValue( requestStreams, requestStreamDesc );
	
	// Parse the transport type. The full transport line looks similar to this:
	//
	//		Transport: RTP/AVP/UDP;unicast;interleaved=0-1;mode=record;control_port=6001;timing_port=6002
	
	err = mRequest.GetHeaderValue( kHTTPHeader_Transport, &src, &len );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	end = src + len;
	
	err = HTTPParseParameter( src, end, &namePtr, &nameLen, NULL, NULL, NULL, &src );
	require_noerr_action( err, exit, status = kHTTPStatus_HeaderFieldNotValid );
	
	if( strnicmpx( namePtr, nameLen, "RTP/AVP/UDP" ) == 0 ) {}
	else { dlogassert( "Bad transport: '%.*s'", (int) nameLen, namePtr ); status = kHTTPStatus_HeaderFieldNotValid; goto exit; }
	
	CFDictionarySetInt64( requestStreamDesc, CFSTR( kAirPlayKey_Type ), kAirPlayStreamType_LegacyAudio );
	
	// Parse transport parameters.
	
	while( HTTPParseParameter( src, end, &namePtr, &nameLen, &valuePtr, &valueLen, NULL, &src ) == kNoErr )
	{
		if( strnicmpx( namePtr, nameLen, "control_port" ) == 0 )
		{
			n = SNScanF( valuePtr, valueLen, "%d", &tempInt );
			require_action( n == 1, exit, status = kHTTPStatus_HeaderFieldNotValid );
			CFDictionarySetInt64( requestStreamDesc, CFSTR( kAirPlayKey_Port_Control ), tempInt );
		}
		else if( strnicmpx( namePtr, nameLen, "timing_port" ) == 0 )
		{
			n = SNScanF( valuePtr, valueLen, "%d", &tempInt );
			require_action( n == 1, exit, status = kHTTPStatus_HeaderFieldNotValid );
			CFDictionarySetInt64( requestParams, CFSTR( kAirPlayKey_Port_Timing ), tempInt );
		}
		else if( strnicmpx( namePtr, nameLen, "redundant" ) == 0 )
		{
			n = SNScanF( valuePtr, valueLen, "%d", &tempInt );
			require_action( n == 1, exit, status = kHTTPStatus_HeaderFieldNotValid );
			CFDictionarySetInt64( requestStreamDesc, CFSTR( kAirPlayKey_RedundantAudio ), tempInt );
		}
		else if( strnicmpx( namePtr, nameLen, "mode" ) == 0 )
		{
			if(      strnicmpx( valuePtr, valueLen, "record" )	== 0 ) {}
			else if( strnicmpx( valuePtr, valueLen, "screen" )	== 0 )
			{
				CFDictionarySetBoolean( requestParams, CFSTR( kAirPlayKey_UsingScreen ), true );
			}
			else dlog( kLogLevelWarning, "### Unsupported 'mode' value: %.*s%s%.*s\n", 
				(int) nameLen, namePtr, valuePtr ? "=" : "", (int) valueLen, valuePtr );
		}
		else if( strnicmpx( namePtr, nameLen, "x-events" )		== 0 ) useEvents = true;
		else if( strnicmpx( namePtr, nameLen, "events") 		== 0 ) {} // Ignore legacy event scheme.
		else if( strnicmpx( namePtr, nameLen, "unicast" )		== 0 ) {} // Ignore
		else if( strnicmpx( namePtr, nameLen, "interleaved" )	== 0 ) {} // Ignore
		else dlog( kLogLevelWarning, "### Unsupported transport param: %.*s%s%.*s\n", 
			(int) nameLen, namePtr, valuePtr ? "=" : "", (int) valueLen, valuePtr );
	}
	
	// Set up compression.
	
	if( mCompressionType == kAirPlayCompressionType_PCM ) {}
	else if( mCompressionType == kAirPlayCompressionType_ALAC ) {}
	else { dlogassert( "Bad compression: %d", mCompressionType ); status = kHTTPStatus_HeaderFieldNotValid; goto exit; }
	
	CFDictionarySetInt64( requestStreamDesc, CFSTR( kAirPlayKey_CompressionType ), mCompressionType );
	CFDictionarySetInt64( requestStreamDesc, CFSTR( kAirPlayKey_SamplesPerFrame ), mSamplesPerFrame );
	CFDictionarySetInt64( requestStreamDesc, CFSTR( kAirPlayKey_LatencyMin ), mMinLatency );
	CFDictionarySetInt64( requestStreamDesc, CFSTR( kAirPlayKey_LatencyMax ), mMaxLatency );
	
	// Set up the session.
	
	if( !mSession )
	{
		err = CreateSession( useEvents );
		require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	}
	if( mUsingEncryption )
	{
		err = AirPlayReceiverSessionSetSecurityInfo( mSession, mEncryptionKey, mEncryptionIV );
		require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	}
	err = AirPlayReceiverSessionSetup( mSession, requestParams, &responseParams );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	// Convert the output params to an RTSP transport header.
	
	outputStreams = CFDictionaryGetCFArray( responseParams, CFSTR( kAirPlayKey_Streams ), &err );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	outputAudioStreamDesc = NULL;
	n = CFArrayGetCount( outputStreams );
	for( i = 0; i < n; ++i )
	{
		outputAudioStreamDesc = (CFDictionaryRef) CFArrayGetValueAtIndex( outputStreams, i );
		require_action( CFIsType( outputAudioStreamDesc, CFDictionary ), exit, status = kHTTPStatus_InternalServerError );
		
		tempInt = (int) CFDictionaryGetInt64( outputAudioStreamDesc, CFSTR( kAirPlayKey_Type ), NULL );
		if( tempInt == kAirPlayStreamType_LegacyAudio ) break;
		outputAudioStreamDesc = NULL;
	}
	require_action( outputAudioStreamDesc, exit, status = kHTTPStatus_InternalServerError );
	
	dataPort = (int) CFDictionaryGetInt64( outputAudioStreamDesc, CFSTR( kAirPlayKey_Port_Data ), NULL );
	require_action( dataPort > 0, exit, status = kHTTPStatus_InternalServerError );
	
	controlPort = (int) CFDictionaryGetInt64( outputAudioStreamDesc, CFSTR( kAirPlayKey_Port_Control ), NULL );
	require_action( controlPort > 0, exit, status = kHTTPStatus_InternalServerError );
	
	eventPort = 0;
	if( useEvents )
	{
		eventPort = (int) CFDictionaryGetInt64( responseParams, CFSTR( kAirPlayKey_Port_Event ), NULL );
		require_action( eventPort > 0, exit, status = kHTTPStatus_InternalServerError );
	}
	
	timingPort = (int) CFDictionaryGetInt64( responseParams, CFSTR( kAirPlayKey_Port_Timing ), NULL );
	require_action( timingPort > 0, exit, status = kHTTPStatus_InternalServerError );
	
	// Send the response.
	
	dst = tempStr;
	lim = dst + sizeof( tempStr );
	err = snprintf_add( &dst, lim, "RTP/AVP/UDP;unicast;mode=record;server_port=%d;control_port=%d;timing_port=%d", 
		dataPort, controlPort, timingPort );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	if( eventPort > 0 )
	{
		err = snprintf_add( &dst, lim, ";event_port=%d", eventPort );
		require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	}
	err = mResponse.AppendHeaderValueF( kHTTPHeader_Transport, "%s", tempStr );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	err = mResponse.AppendHeaderValueF( kHTTPHeader_Session, "1" );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
	*tempStr = '\0';
	AirPlayReceiverServerPlatformGetCString( gAirPlayReceiverServer, CFSTR( kAirPlayProperty_AudioJackStatus ), NULL,
		tempStr, sizeof( tempStr ), NULL );
	if( *tempStr == '\0' ) strlcpy( tempStr, kAirPlayAudioJackStatus_ConnectedUnknown, sizeof( tempStr ) );
	err = mResponse.AppendHeaderValueF( "Audio-Jack-Status", "%s", tempStr );
	require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	
#if( AIRPLAY_DACP )
	AirTunesDACPClient_StartSession( gAirTunesDACPClient );
#endif
	AirPlayReceiverServerSetBoolean( gAirPlayReceiverServer, CFSTR( kAirPlayProperty_Playing ), NULL, true );
	
	mDidAudioSetup = true;
	status = kHTTPStatus_OK;
	
exit:
	CFReleaseNullSafe( requestParams );
	CFReleaseNullSafe( requestStreams );
	CFReleaseNullSafe( requestStreamDesc );
	CFReleaseNullSafe( responseParams );
	return( status );
}

//===========================================================================================================================
//	ProcessSetupSDPScreen
//===========================================================================================================================

#endif // AIRPLAY_SDP_SETUP

//===========================================================================================================================
//	ProcessSetupPlist
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessSetupPlist( void )
{
	HTTPStatus					status;
	OSStatus					err;
	CFMutableDictionaryRef		requestParams  = NULL;
	CFDictionaryRef				responseParams = NULL;
	uint8_t						sessionUUID[ 16 ];
	size_t						len;
	uint64_t					u64;
	AirPlayEncryptionType		et;
	
	ats_ulog( kAirPlayPhaseLogLevel, "Setup\n" );
	
	// If we're denying interrupts then reject if there's already an active session.
	// Otherwise, hijack any active session to start the new one (last-in-wins).
	
	if( gAirTunesDenyInterruptions )
	{
		AirTunesConnection *		activeCnx;
		
		activeCnx = mAirTunesServer.FindActiveConnection();
		if( activeCnx && ( activeCnx != this ) )
		{
			ats_ulog( kLogLevelNotice, "Denying interruption from %##a due to %##a\n", &mPeerAddr, &activeCnx->mPeerAddr );
			status = kHTTPStatus_NotEnoughBandwidth;
			err = kNoErr;
			goto exit;
		}
	}
	mAirTunesServer.HijackConnections( this );
	
	requestParams = CFDictionaryCreateMutableWithBytes( mRequest.mBodyBuffer, mRequest.mBodyBufferUsed, &err );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	CFDictionaryGetCString( requestParams, CFSTR( kAirPlayKey_Name ), mClientName, sizeof( mClientName ), NULL );
	
	CFDictionaryGetData( requestParams, CFSTR( kAirPlayKey_SessionUUID ), sessionUUID, sizeof( sessionUUID ), &len, &err );
	if( !err )
	{
		require_action( len == sizeof( sessionUUID ), exit, err = kSizeErr; status = kHTTPStatus_BadRequest );
		mClientSessionID = ReadBig64( sessionUUID );
	}
	
	u64 = CFDictionaryGetMACAddress( requestParams, CFSTR( kAirPlayKey_DeviceID ), NULL, &err );
	if( !err ) mClientDeviceID = u64;
	
	// Set up the session.
	
	if( !mSession )
	{
		err = CreateSession( true );
		require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
		strlcpy( gAirPlayAudioStats.ifname, mIfName, sizeof( gAirPlayAudioStats.ifname ) );
	}
	
	et = (AirPlayEncryptionType) CFDictionaryGetInt64( requestParams, CFSTR( kAirPlayKey_EncryptionType ), &err );
	if( !err && ( et != kAirPlayEncryptionType_None ) )
	{
		uint8_t		key[ 16 ], iv[ 16 ];
		
		err = DecryptKey( requestParams, et, key, iv );
		require_noerr_action( err, exit, status = kHTTPStatus_KeyManagementError );
		
		err = AirPlayReceiverSessionSetSecurityInfo( mSession, key, iv );
		memset( key, 0, sizeof( key ) );
		require_noerr_action( err, exit, status = kHTTPStatus_InternalServerError );
	}
	CFDictionaryRemoveValue( requestParams, CFSTR( kAirPlayKey_EncryptionKey ) );
	CFDictionaryRemoveValue( requestParams, CFSTR( kAirPlayKey_EncryptionIV ) );
	
	err = AirPlayReceiverSessionSetup( mSession, requestParams, &responseParams );
	require_noerr_action( err, exit, status = kHTTPStatus_BadRequest );
	
	mDidAnnounce	= true;
	mDidAudioSetup	= true;
#if( AIRPLAY_DACP )
	AirTunesDACPClient_StartSession( gAirTunesDACPClient );
#endif
	AirPlayReceiverServerSetBoolean( gAirPlayReceiverServer, CFSTR( kAirPlayProperty_Playing ), NULL, true );
	
	status = SendPlistResponse( responseParams, &err );
	require_noerr( err, exit );
	
exit:
	CFReleaseNullSafe( requestParams );
	CFReleaseNullSafe( responseParams );
	if( err ) ats_ulog( kLogLevelNotice, "### Setup session failed: %#m\n", err );
	return( status );
}

//===========================================================================================================================
//	ProcessTearDown
//===========================================================================================================================

HTTPStatus	AirTunesConnection::ProcessTearDown( void )
{
	ats_ulog( kAirPlayPhaseLogLevel, "Tear down\n" );
	
#if( AIRPLAY_DACP )
	if( mDidAudioSetup ) AirTunesDACPClient_StopSession( gAirTunesDACPClient );
#endif
	if( mSession )
	{
		AirPlayReceiverSessionTearDown( mSession, kNoErr );
		CFRelease( mSession );
		mSession = NULL;
		AirPlayReceiverServerSetBoolean( gAirPlayReceiverServer, CFSTR( kAirPlayProperty_Playing ), NULL, false );
	}
	gAirPlayAudioCompressionType = kAirPlayCompressionType_Undefined;
	
	mDidAnnounce	= false;
	mDidAudioSetup	= false;
	mDidRecord		= false;
	
	return( kHTTPStatus_OK );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	CreateSession
//===========================================================================================================================

OSStatus	AirTunesConnection::CreateSession( Boolean inUseEvents )
{
	OSStatus								err;
	AirPlayReceiverSessionCreateParams		createParams;
	
	require_action_quiet( !mSession, exit, err = kNoErr );
	
	memset( &createParams, 0, sizeof( createParams ) );
	createParams.server				= gAirPlayReceiverServer;
	createParams.transportType		= mTransportType;
	createParams.peerAddr			= &mPeerAddr;
	createParams.clientDeviceID		= mClientDeviceID;
	createParams.clientSessionID	= mClientSessionID;
	createParams.useEvents			= inUseEvents;
	
	err = AirPlayReceiverSessionCreate( &mSession, &createParams );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	DecryptKey
//===========================================================================================================================

OSStatus
	AirTunesConnection::DecryptKey( 
		CFDictionaryRef			inRequestParams, 
		AirPlayEncryptionType	inType, 
		uint8_t					inKeyBuf[ 16 ], 
		uint8_t					inIVBuf[ 16 ] )
{
	OSStatus			err;
	const uint8_t *		keyPtr;
	size_t				len;
	
	keyPtr = CFDictionaryGetData( inRequestParams, CFSTR( kAirPlayKey_EncryptionKey ), NULL, 0, &len, &err );
	require_noerr( err, exit );
	
	if( 0 ) {}
#if( AIRPLAY_MFI )
	else if( inType == kAirPlayEncryptionType_MFi_SAPv1 )
	{
		require_action( mMFiSAP, exit, err = kAuthenticationErr );
		err = MFiSAP_Decrypt( mMFiSAP, keyPtr, len, inKeyBuf );
		require_noerr( err, exit );
	}
#endif
	else
	{
		ats_ulog( kLogLevelWarning, "### Bad ET: %d\n", inType );
		err = kParamErr;
		goto exit;
	}
	
	CFDictionaryGetData( inRequestParams, CFSTR( kAirPlayKey_EncryptionIV ), inIVBuf, 16, &len, &err );
	require_noerr( err, exit );
	require_action( len == 16, exit, err = kSizeErr );
	
exit:
	return( err );
}

#if( AIRPLAY_PASSWORDS )
//===========================================================================================================================
//	HTTPAuthorization_CopyPassword [static]
//===========================================================================================================================

HTTPStatus	AirTunesConnection::HTTPAuthorization_CopyPassword( HTTPAuthorizationInfoRef inInfo, char **outPassword )
{
	HTTPStatus		status;
	char *			str;
	
	(void) inInfo; // Unused
	
	str = strdup( gAirPlayReceiverServer->playPassword );
	require_action( str, exit, status = kHTTPStatus_InternalServerError );
	*outPassword = str;
	status = kHTTPStatus_OK;
	
exit:
	return( status );
}

//===========================================================================================================================
//	HTTPAuthorization_IsValidNonce [static]
//===========================================================================================================================

Boolean	AirTunesConnection::HTTPAuthorization_IsValidNonce( HTTPAuthorizationInfoRef inInfo )
{
	AirTunesConnection * const		me = (AirTunesConnection *) inInfo->isValidNonceContext;
	OSStatus						err;
	
	err = HTTPVerifyTimedNonce( inInfo->requestNoncePtr, inInfo->requestNonceLen, 30, 
		kHTTPTimedNonceETagPtr, kHTTPTimedNonceETagLen, 
		me->mAirTunesServer.mHTTPTimedNonceKey, sizeof( me->mAirTunesServer.mHTTPTimedNonceKey ) );
	return( !err );
}
#endif

#if( AIRPLAY_PASSWORDS )
//===========================================================================================================================
//	RequestRequiresAuth
//===========================================================================================================================

Boolean	AirTunesConnection::RequestRequiresAuth( void )
{
	const char * const		methodPtr	= mRequest.mMethodPtr;
	size_t const			methodLen	= mRequest.mMethodLen;
	const char * const		pathPtr		= mRequest.mURLComps.pathPtr;
	size_t const			pathLen		= mRequest.mURLComps.pathLen;
	
	if( strnicmpx( methodPtr, methodLen, "POST" ) == 0 )
	{
		if( 0 ) {}
		#if( AIRPLAY_MFI )
		else if( strnicmp_suffix( pathPtr, pathLen, "/auth-setup" )		== 0 ) return( false );
		#endif
		else if( strnicmp_suffix( pathPtr, pathLen, "/info" )			== 0 ) return( false );
		else if( strnicmp_suffix( pathPtr, pathLen, "/perf" )			== 0 ) return( false );
	}
	else if( strnicmpx( methodPtr, methodLen, "GET" ) == 0 )
	{
		if(      strnicmp_suffix( pathPtr, pathLen, "/info" )			== 0 ) return( false );
		else if( strnicmp_suffix( pathPtr, pathLen, "/log" )			== 0 ) return( false );
		else if( strnicmp_suffix( pathPtr, pathLen, "/logs" )			== 0 ) return( false );
	}
	return( true );
}
#endif

//===========================================================================================================================
//	SendPlistResponse
//===========================================================================================================================

HTTPStatus	AirTunesConnection::SendPlistResponse( CFPropertyListRef inPlist, OSStatus *outErr )
{
	HTTPStatus			status;
	OSStatus			err;
	CFDataRef			data = NULL;
	const uint8_t *		ptr;
	size_t				len;
	
	if( inPlist )
	{
		data = CFPropertyListCreateData( NULL, inPlist, kCFPropertyListBinaryFormat_v1_0, 0, NULL );
		require_action( data, exit, err = kUnknownErr; status = kHTTPStatus_InternalServerError );
		ptr = CFDataGetBytePtr( data );
		len = (size_t) CFDataGetLength( data );
		if( len <= sizeof( mResponseBodyBuffer ) )
		{
			memcpy( mResponseBodyBuffer, ptr, len );
			mResponse.SetBody( mResponseBodyBuffer, len );
		}
		else
		{
			check( mResponse.mBodyMallocedBuffer == NULL );
			mResponse.mBodyMallocedBuffer = (uint8_t *) malloc( len );
			require_action( mResponse.mBodyMallocedBuffer, exit, err = kNoMemoryErr; status = kHTTPStatus_InternalServerError );
			memcpy( mResponse.mBodyMallocedBuffer, ptr, len );
			mResponse.SetBody( mResponse.mBodyMallocedBuffer, len );
		}
		mResponse.AppendHeaderValueF( kHTTPHeader_ContentType, "%s", kMIMEType_AppleBinaryPlist );
	}
	else
	{
		len = 0;
		mResponse.SetBody( NULL, 0 );
	}
	mResponse.AppendHeaderValueF( kHTTPHeader_ContentLength, "%zu", len );
	err = kNoErr;
	status = kHTTPStatus_OK;
	
exit:
	CFReleaseNullSafe( data );
	if( outErr ) *outErr = err;
	return( status );
}
