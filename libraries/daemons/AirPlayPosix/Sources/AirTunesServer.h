/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/

#ifndef	__AirTunesServer_h__
#define	__AirTunesServer_h__

#include "AirPlayCommon.h"
#include "AirPlayReceiverSession.h"
#include "CommonServices.h"
#include "DebugServices.h"
#include "dns_sd.h"
#ifdef __cplusplus
	#include "HTTPServerCPP.hpp"
#endif
#include "HTTPUtils.h"

#if( AIRPLAY_DACP )
	#include "AirTunesDACP.h"
#endif
#if( AIRPLAY_MFI )
	#include "MFiSAP.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		AirTunes Server Control API
	@abstract	Controls the AirTunes Server.
*/

extern AirPlayCompressionType		gAirPlayAudioCompressionType;

OSStatus	AirTunesServer_EnsureCreated( void );
void		AirTunesServer_EnsureDeleted( void );
OSStatus	AirTunesServer_Start( void );
int			AirTunesServer_GetListenPort( void );

void		AirTunesServer_SetDenyInterruptions( Boolean inDeny );

void		AirTunesServer_FailureOccurred( OSStatus inError );

void		AirTunesServer_StopAllConnections( void );

void		AirTunesServer_AllowPlayback( void );
void		AirTunesServer_PreventPlayback( void );

#if( AIRPLAY_DACP )
	OSStatus	AirTunesServer_ScheduleDACPCommand( const char *inCommand );
	OSStatus	AirTunesServer_SetDACPCallBack( AirTunesDACPClientCallBackPtr inCallBack, void *inContext );
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@class		AirTunesServer
	@abstract	AirTunesServer.
*/

#ifdef __cplusplus

using CoreUtils::HTTPMessageCPP;
using CoreUtils::HTTPConnection;
using CoreUtils::HTTPServerCPP;

class AirTunesConnection; // Forward declaration.

class AirTunesServer : public HTTPServerCPP
{
	friend class AirTunesConnection;
	
	protected:
		
		#if( AIRPLAY_VOLUME_CONTROL )
			Float32		mVolume;
		#endif
		uint8_t			mHTTPTimedNonceKey[ 16 ];
		
	public:
		
		static OSStatus		Create( AirTunesServer **outServer );
		virtual void		Delete( void );
		virtual OSStatus	Start( void );
		
		virtual OSStatus		CreateConnection( HTTPConnection **outConnection );
		AirTunesConnection *	FindActiveConnection( void );
		void					HijackConnections( AirTunesConnection *inHijacker );
		
	protected:
	
		AirTunesServer( void );												// Prohibit manual creation.
		AirTunesServer( const AirTunesServer &inOriginal );					// Prohibit copying.
		AirTunesServer &	operator=( const AirTunesServer &inOriginal );	// Prohibit assignment.
		virtual ~AirTunesServer( void ) {}									// Prohibit manual deletion.
};

#endif // __cplusplus

//---------------------------------------------------------------------------------------------------------------------------
/*!	@class		AirTunesConnection
	@abstract	AirTunesConnection.
*/

#ifdef __cplusplus

class AirTunesConnection : public HTTPConnection
{
	friend class AirTunesServer;
	
	protected:
		
		AirTunesServer &				mAirTunesServer;
		uint64_t						mClientDeviceID;
		char							mClientName[ 128 ];
		uint64_t						mClientSessionID;
		bool							mDidAnnounce;
		bool							mDidAudioSetup;
		bool							mDidRecord;
		
		Boolean							mHTTPAuthentication_IsAuthenticated;
		
		HTTPMessageCPP					mResponse;
		char							mResponseBodyBuffer[ 32 * 1024 ];
		
		AirPlayCompressionType			mCompressionType;
		uint32_t						mSamplesPerFrame;
		
		#if( AIRPLAY_MFI )
			MFiSAPRef					mMFiSAP;
			Boolean						mMFiSAPDone;
		#endif
		uint8_t							mEncryptionKey[ 16 ];
		uint8_t							mEncryptionIV[ 16 ];
		Boolean							mUsingEncryption;
		
		uint32_t						mMinLatency, mMaxLatency;
		
		AirPlayReceiverSessionRef		mSession;
		
	public:
	
		AirTunesConnection( HTTPServerCPP &inServer );
		virtual ~AirTunesConnection( void );
		
		virtual OSStatus	ProcessRequest( void );
		#if( AIRPLAY_SDP_SETUP )
			HTTPStatus		ProcessAnnounce( void );
			OSStatus		ProcessAnnounceAudio( void );
		#endif
		#if( AIRPLAY_MFI )
			HTTPStatus		ProcessAuthSetup( void );
		#endif
		HTTPStatus			ProcessCommand( void );
		HTTPStatus			ProcessFeedback( void );
		HTTPStatus			ProcessFlush( void );
		HTTPStatus			ProcessGetLog( void );
		HTTPStatus			ProcessInfo( void );
		#if( AIRPLAY_METRICS )
			HTTPStatus		ProcessMetrics( void );
		#endif
		HTTPStatus			ProcessOptions( void );
		HTTPStatus			ProcessGetParameter( void );
		HTTPStatus			ProcessSetParameter( void );
		#if( AIRPLAY_META_DATA )
		HTTPStatus			ProcessSetParameterArtwork( const char *inMIMETypeStr, size_t inMIMETypeLen );
		HTTPStatus			ProcessSetParameterDMAP( void );
		#endif
		HTTPStatus			ProcessSetParameterText( void );
		HTTPStatus			ProcessSetProperty( void );
		HTTPStatus			ProcessRecord( void );
		HTTPStatus			ProcessSetup( void );
		HTTPStatus			ProcessSetupPlist( void );
		#if( AIRPLAY_SDP_SETUP )
			HTTPStatus		ProcessSetupSDPAudio( void );
		#endif
		HTTPStatus			ProcessTearDown( void );
		
		OSStatus	CreateSession( Boolean inUseEvents );
		OSStatus
			DecryptKey( 
			CFDictionaryRef			inRequestParams, 
			AirPlayEncryptionType	inType, 
			uint8_t					inKeyBuf[ 16 ], 
			uint8_t					inKeyIV[ 16 ] );
		#if( AIRPLAY_PASSWORDS )
			static HTTPStatus	HTTPAuthorization_CopyPassword( HTTPAuthorizationInfoRef inInfo, char **outPassword );
			static Boolean		HTTPAuthorization_IsValidNonce( HTTPAuthorizationInfoRef inInfo );
		#endif
		
		#if( AIRPLAY_PASSWORDS )
			Boolean			RequestRequiresAuth( void );
		#endif
		HTTPStatus			SendPlistResponse( CFPropertyListRef inPlist, OSStatus *outErr );
};

#endif // __cplusplus

#ifdef __cplusplus
}
#endif

#endif	// __AirTunesServer_h__
