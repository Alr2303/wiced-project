/*
	Copyright (C) 2013 Apple Inc. All Rights Reserved.
	
	POSIX platform plugin for AirPlay.
*/

#include "AirPlayCommon.h"
#include "AirPlayReceiverServer.h"
#include "AirPlayReceiverSession.h"
#include "AirPlaySettings.h"
#include "AudioUtils.h"
#include "StringUtils.h"
#include "SystemUtils.h"

#if 0
#pragma mark == Structures ==
#endif

//===========================================================================================================================
//	Structures
//===========================================================================================================================

typedef struct
{
	AirPlayReceiverSessionRef		session;
	AudioStreamRef					mainAudioOutput;
	Boolean							audioStarted;
	
}	AirPlayReceiverSessionPlatformData;

#if 0
#pragma mark == Prototypes ==
#endif

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

static OSStatus	_SetUpStreams( AirPlayReceiverSessionRef inSession, CFDictionaryRef inParams, CFDictionaryRef *outParams );
static void		_TearDownStreams( AirPlayReceiverSessionRef inSession, CFDictionaryRef inParams );
static void
	_LegacyAudioOutputCallBack( 
		uint32_t	inSampleTime, 
		uint64_t	inHostTime, 
		void *		inBuffer, 
		size_t		inLen, 
		void *		inContext );
static void
	_MainAudioOutputCallBack( 
		uint32_t	inSampleTime, 
		uint64_t	inHostTime, 
		void *		inBuffer, 
		size_t		inLen, 
		void *		inContext );

#if 0
#pragma mark == Globals ==
#endif

//===========================================================================================================================
//	Globals
//===========================================================================================================================

ulog_define( AirPlayReceiverPlatform, kLogLevelTrace, kLogFlags_Default, "AirPlay",  NULL );
#define atrp_ucat()					&log_category_from_name( AirPlayReceiverPlatform )
#define atrp_ulog( LEVEL, ... )		ulog( atrp_ucat(), (LEVEL), __VA_ARGS__ )

#if 0
#pragma mark -
#pragma mark == Server-level APIs ==
#endif

//===========================================================================================================================
//	AirPlayReceiverServerPlatformInitialize
//===========================================================================================================================

OSStatus	AirPlayReceiverServerPlatformInitialize( AirPlayReceiverServerRef inServer )
{
	(void) inServer;

	return( kNoErr );
}

//===========================================================================================================================
//	AirPlayReceiverServerPlatformFinalize
//===========================================================================================================================

void	AirPlayReceiverServerPlatformFinalize( AirPlayReceiverServerRef inServer )
{
	(void) inServer;
}

//===========================================================================================================================
//	AirPlayReceiverServerPlatformControl
//===========================================================================================================================

OSStatus
	AirPlayReceiverServerPlatformControl( 
		CFTypeRef			inServer, 
		uint32_t			inFlags, 
		CFStringRef			inCommand, 
		CFTypeRef			inQualifier, 
		CFDictionaryRef		inParams, 
		CFDictionaryRef *	outParams )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inServer;
	OSStatus							err;
	
	(void) server;
	(void) inCommand;
	(void) inQualifier;
	(void) inFlags;
	(void) inParams;
	(void) outParams;
	
	if( 0 ) {}
	
	// Unknown
	
	else
	{
		err = kNotHandledErr;
		goto exit;
	}
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlayReceiverServerPlatformCopyProperty
//===========================================================================================================================

CFTypeRef
	AirPlayReceiverServerPlatformCopyProperty( 
		CFTypeRef	inServer, 
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		OSStatus *	outErr )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inServer;
	CFTypeRef							value  = NULL;
	OSStatus							err;
	
	(void) server;
	(void) inFlags;
	(void) inQualifier;
	
	if( 0 ) {}
	
	// SystemFlags
	
	else if( CFEqual( inProperty, CFSTR( kAirPlayProperty_StatusFlags ) ) )
	{
		// Always report an audio link until we can detect it correctly.
		
		value = CFNumberCreateInt64( kAirPlayStatusFlag_AudioLink );
		require_action( value, exit, err = kUnknownErr );
	}
	
	// Unknown
	
	else
	{
		err = kNotHandledErr;
		goto exit;
	}
	err = kNoErr;
	
exit:
	if( outErr ) *outErr = err;
	return( value );
}

//===========================================================================================================================
//	AirPlayReceiverServerPlatformSetProperty
//===========================================================================================================================

OSStatus
	AirPlayReceiverServerPlatformSetProperty( 
		CFTypeRef	inServer, 
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		CFTypeRef	inValue )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inServer;
	
	(void) server;
	(void) inFlags;
	(void) inProperty;
	(void) inQualifier;
	(void) inValue;
	
	return( kNotHandledErr );
}

#if 0
#pragma mark -
#pragma mark == Session-level APIs ==
#endif

//===========================================================================================================================
//	AirPlayReceiverSessionPlatformInitialize
//===========================================================================================================================

OSStatus	AirPlayReceiverSessionPlatformInitialize( AirPlayReceiverSessionRef inSession )
{
	OSStatus									err;
	AirPlayReceiverSessionPlatformData *		spd;
	
	spd = (AirPlayReceiverSessionPlatformData *) calloc( 1, sizeof( *spd ) );
	require_action( spd, exit, err = kNoMemoryErr );
	spd->session = inSession;
	inSession->platformPtr = spd;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlayReceiverSessionPlatformFinalize
//===========================================================================================================================

void	AirPlayReceiverSessionPlatformFinalize( AirPlayReceiverSessionRef inSession )
{
	AirPlayReceiverSessionPlatformData * const		spd = (AirPlayReceiverSessionPlatformData *) inSession->platformPtr;
	
	if( !spd ) return;
	
	AudioStreamForget( &spd->mainAudioOutput );
	spd->audioStarted = false;
	free( spd );
	inSession->platformPtr = NULL;
}

//===========================================================================================================================
//	AirPlayReceiverSessionPlatformControl
//===========================================================================================================================

OSStatus
	AirPlayReceiverSessionPlatformControl( 
		CFTypeRef			inSession, 
		uint32_t			inFlags, 
		CFStringRef			inCommand, 
		CFTypeRef			inQualifier, 
		CFDictionaryRef		inParams, 
		CFDictionaryRef *	outParams )
{
	AirPlayReceiverSessionRef const					session = (AirPlayReceiverSessionRef) inSession;
	AirPlayReceiverSessionPlatformData * const		spd = (AirPlayReceiverSessionPlatformData *) session->platformPtr;
	OSStatus										err;
	
	(void) inFlags;
	(void) inQualifier;
	(void) inParams;
	(void) outParams;
	
	if( 0 ) {}
	
	// ModesChanged
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_ModesChanged ) ) )
	{
		// $$$ TO DO: Implement modes support.
	}
	
	// SetUpStreams
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_SetUpStreams ) ) )
	{
		err = _SetUpStreams( session, inParams, outParams );
		require_noerr( err, exit );
	}
	
	// TearDownStreams
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_TearDownStreams ) ) )
	{
		_TearDownStreams( session, inParams );
	}
	
	// StartSession
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_StartSession ) ) )
	{
		if( !spd->audioStarted )
		{
			if( spd->mainAudioOutput )
			{
				err = AudioStreamStart( spd->mainAudioOutput );
				require_noerr( err, exit );
			}
			spd->audioStarted = true;
		}
	}
	
	// Unknown
	
	else
	{
		err = kNotHandledErr;
		goto exit;
	}
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlayReceiverSessionPlatformCopyProperty
//===========================================================================================================================

CFTypeRef
	AirPlayReceiverSessionPlatformCopyProperty( 
		CFTypeRef	inSession, 
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		OSStatus *	outErr )
{
	AirPlayReceiverSessionRef const					session = (AirPlayReceiverSessionRef) inSession;
	AirPlayReceiverSessionPlatformData * const		spd		= (AirPlayReceiverSessionPlatformData *) session->platformPtr;
	OSStatus										err;
	CFTypeRef										value = NULL;
	
	(void) session;
	(void) spd;
	(void) inFlags;
	(void) inQualifier;
	
	if( 0 ) {}
	
#if( AIRPLAY_VOLUME_CONTROL )
	// Volume
	
	else if( CFEqual( inProperty, CFSTR( kAirPlayProperty_Volume ) ) )
	{
		double		volume, dB;
		
		if( spd && spd->audioStarted && spd->mainAudioOutput )
		{
			volume = AudioStreamGetVolume( spd->mainAudioOutput, NULL );
		}
		else
		{
			volume = 1.0;
		}
		dB = ( volume > 0 ) ? ( 20.0f * log10f( volume ) ) : kAirTunesSilenceVolumeDB;
		value = CFNumberCreate( NULL, kCFNumberDoubleType, &dB );
		require_action( value, exit, err = kNoMemoryErr );
	}
#endif
	
	// Unknown
	
	else
	{
		err = kNotHandledErr;
		goto exit;
	}
	err = kNoErr;
	
exit:
	if( outErr ) *outErr = err;
	return( value );
}

//===========================================================================================================================
//	AirPlayReceiverSessionPlatformSetProperty
//===========================================================================================================================

OSStatus
	AirPlayReceiverSessionPlatformSetProperty( 
		CFTypeRef	inSession, 
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		CFTypeRef	inValue )
{
	AirPlayReceiverSessionRef const					session	= (AirPlayReceiverSessionRef) inSession;
	AirPlayReceiverSessionPlatformData * const		spd		= (AirPlayReceiverSessionPlatformData *) session->platformPtr;
	OSStatus										err;
	
	(void) session;
	(void) spd;
	(void) inFlags;
	(void) inProperty;
	(void) inQualifier;
	(void) inValue;
	
	if( 0 ) {}
	
#if( AIRPLAY_VOLUME_CONTROL )
	// Volume
	
	else if( CFEqual( inProperty, CFSTR( kAirPlayProperty_Volume ) ) )
	{
		if( spd && spd->audioStarted && spd->mainAudioOutput )
		{
			double const		dB = (Float32) CFGetDouble( inValue, NULL );
			double				linear;
			
			if(      dB <= kAirTunesMinVolumeDB ) linear = 0.0;
			else if( dB >= kAirTunesMaxVolumeDB ) linear = 1.0;
			else linear = TranslateValue( dB, kAirTunesMinVolumeDB, kAirTunesMaxVolumeDB, 0.0, 1.0 );
			linear = Clamp( linear, 0.0, 1.0 );
			atrp_ulog( kLogLevelNotice, "Setting volume to dB=%f, linear=%f\n", dB, linear );
			err = AudioStreamSetVolume( spd->mainAudioOutput, linear );
			require_noerr( err, exit );
		}
	}
#endif
	
	// Unknown
	
	else
	{
		err = kNotHandledErr;
		goto exit;
	}
	err = kNoErr;
	
exit:
	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	_SetUpStreams
//===========================================================================================================================

static OSStatus	_SetUpStreams( AirPlayReceiverSessionRef inSession, CFDictionaryRef inParams, CFDictionaryRef *outParams )
{
	AirPlayReceiverSessionPlatformData * const		spd = (AirPlayReceiverSessionPlatformData *) inSession->platformPtr;
	AirPlayAudioStreamContext *						ctx;
	OSStatus										err;
	CFArrayRef										requestStreams;
	CFIndex											i, n;
	CFDictionaryRef									streamDesc;
	AirPlayStreamType								type;
	AudioStremAudioCallback_f						cb;
	AudioStreamBasicDescription						asbd;
	uint64_t										u64;
	
	(void) outParams;
	
	requestStreams = CFDictionaryGetCFArray( inParams, CFSTR( kAirPlayKey_Streams ), NULL );
	n = requestStreams ? CFArrayGetCount( requestStreams ) : 0;
	for( i = 0; i < n; ++i )
	{
		streamDesc = (CFDictionaryRef) CFArrayGetValueAtIndex( requestStreams, i );
		require_action( CFIsType( streamDesc, CFDictionary ), exit, err = kTypeErr );
		
		type = (AirPlayStreamType) CFDictionaryGetInt64( streamDesc, CFSTR( kAirPlayKey_Type ), NULL );
		switch( type )
		{
			case kAirPlayStreamType_LegacyAudio:
			case kAirPlayStreamType_MainAudio:
				ctx = &inSession->mainAudioCtx;
				AudioStreamForget( &spd->mainAudioOutput );
				err = AudioStreamCreate( &spd->mainAudioOutput );
				require_noerr( err, exit );
				
				cb = ( ctx->type == kAirPlayStreamType_LegacyAudio ) ? _LegacyAudioOutputCallBack : _MainAudioOutputCallBack;
				AudioStreamSetAudioCallback( spd->mainAudioOutput, cb, inSession );
				
				ASBD_FillPCM( &asbd, ctx->sampleRate, ctx->bitsPerSample, RoundUp( ctx->bitsPerSample, 8 ), ctx->channels );
				AudioStreamSetFormat( spd->mainAudioOutput, &asbd );
				AudioStreamSetThreadName( spd->mainAudioOutput, "AirPlayAudioMainOutput" );
				AudioStreamSetThreadPriority( spd->mainAudioOutput, kAirPlayThreadPriority_AudioReceiver );
				
				err = AudioStreamPrepare( spd->mainAudioOutput );
				require_noerr( err, exit );
				
				u64 = AudioStreamGetLatency( spd->mainAudioOutput, NULL );
				inSession->platformAudioLatency = (uint32_t)( ( u64 * asbd.mSampleRate ) / kMicrosecondsPerSecond );
				atrp_ulog( kLogLevelInfo, "Updated platform latency to %u samples\n", inSession->platformAudioLatency );
				
				break;
			
			default:
				break;
		}
	}
	
	err = kNoErr;
	
exit:
	if( err ) _TearDownStreams( inSession, inParams );
	return( err );
}

//===========================================================================================================================
//	_TearDownStreams
//===========================================================================================================================

static void	_TearDownStreams( AirPlayReceiverSessionRef inSession, CFDictionaryRef inParams )
{
	AirPlayReceiverSessionPlatformData * const		spd = (AirPlayReceiverSessionPlatformData *) inSession->platformPtr;
	
	(void) inParams;
	
	AudioStreamForget( &spd->mainAudioOutput );
}

//===========================================================================================================================
//	_LegacyAudioOutputCallBack
//===========================================================================================================================

static void
	_LegacyAudioOutputCallBack( 
		uint32_t	inSampleTime, 
		uint64_t	inHostTime, 
		void *		inBuffer, 
		size_t		inLen, 
		void *		inContext )
{
	AirPlayReceiverSessionRef const		session = (AirPlayReceiverSessionRef) inContext;
	OSStatus							err;
	
	err = AirPlayReceiverSessionReadAudio( session, kAirPlayStreamType_LegacyAudio, inSampleTime, inHostTime, 
		inBuffer, inLen );
	require_noerr( err, exit );
	
exit:
	return;
}

//===========================================================================================================================
//	_MainAudioOutputCallBack
//===========================================================================================================================

static void
	_MainAudioOutputCallBack( 
		uint32_t	inSampleTime, 
		uint64_t	inHostTime, 
		void *		inBuffer, 
		size_t		inLen, 
		void *		inContext )
{
	AirPlayReceiverSessionRef const		session = (AirPlayReceiverSessionRef) inContext;
	OSStatus							err;
	
	err = AirPlayReceiverSessionReadAudio( session, kAirPlayStreamType_MainAudio, inSampleTime, inHostTime, inBuffer, inLen );
	require_noerr( err, exit );
	
exit:
	return;
}

#if 0
#pragma mark -
#pragma mark == Utilities ==
#endif

