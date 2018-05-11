/*
	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
*/

#include "AudioUtils.h"

#include <errno.h>
#include <math.h>
#include <poll.h>
#include <stdlib.h>

#include "CommonServices.h"
#include "DebugServices.h"
#include "MathUtils.h"
#include "TickUtils.h"
#include "ThreadUtils.h"

#include <alsa/asoundlib.h>

#include CF_HEADER
#include CF_RUNTIME_HEADER
#include LIBDISPATCH_HEADER

//===========================================================================================================================
//	AudioStream
//===========================================================================================================================

struct AudioStreamPrivate
{
	CFRuntimeBase					base;					// CF type info. Must be first.
	int								pipeRead;				// Read side of command pipe (for thread to read commands from).
	int								pipeWrite;				// Write side of command pipe (for writing commands to the thread).
	pthread_t						thread;					// Thread processing audio.
	pthread_t *						threadPtr;				// Ptr to audio thread for NULL testing.
	snd_pcm_t *						pcmHandle;				// Handle to the audio engine for PCM data.
	uint8_t *						audioBuffer;			// Buffer between the user callback and the OS audio stack.
	size_t							audioMaxLen;			// Number of bytes the audioBuffer can hold.
	struct pollfd *					pollFDArray;			// File descriptor entries to wait on.
	int								pollFDCount;			// Number of file descriptors to wait on.
	
	AudioStremAudioCallback_f		audioCallbackPtr;		// Function to call to give input audio or get output audio.
	void *							audioCallbackCtx;		// Context to pass to audio callback function.
	AudioStreamFlags				flags;					// Flags indicating input or output, etc.
	AudioStreamBasicDescription		format;					// Format of the audio data.
	uint32_t						preferredLatencyMics;	// Max latency the app can tolerate.
	char *							threadName;				// Name to use when creating threads.
	int								threadPriority;			// Priority to run threads.
	Boolean							hasThreadPriority;		// True if a thread priority has been set.
};

static void		_AudioStreamGetTypeID( void *inContext );
static void		_AudioStreamFinalize( CFTypeRef inCF );
static void *	_AudioStreamThread( void *inArg );
static OSStatus	_AudioStreamOpenDefaultDevice( AudioStreamRef me );
static OSStatus	_AudioStreamRecover( AudioStreamRef me, int inError );

static dispatch_once_t			gAudioStreamInitOnce = 0;
static CFTypeID					gAudioStreamTypeID = _kCFRuntimeNotATypeID;
static const CFRuntimeClass		kAudioStreamClass = 
{
	0,						// version
	"AudioStream",			// className
	NULL,					// init
	NULL,					// copy
	_AudioStreamFinalize,	// finalize
	NULL,					// equal -- NULL means pointer equality.
	NULL,					// hash  -- NULL means pointer hash.
	NULL,					// copyFormattingDesc
	NULL,					// copyDebugDesc
	NULL,					// reclaim
	NULL					// refcount
};

//===========================================================================================================================
//	Logging
//===========================================================================================================================

ulog_define( AudioUtilsALSA, kLogLevelTrace, kLogFlags_Default, "AudioUtilsALSA", NULL );
#define asla_dlog( LEVEL, ... )		dlogc( &log_category_from_name( AudioUtilsALSA ), (LEVEL), __VA_ARGS__ )
#define asla_ulog( LEVEL, ... )		ulog( &log_category_from_name( AudioUtilsALSA ), (LEVEL), __VA_ARGS__ )

//===========================================================================================================================
//	AudioStreamGetTypeID
//===========================================================================================================================

CFTypeID	AudioStreamGetTypeID( void )
{
	dispatch_once_f( &gAudioStreamInitOnce, NULL, _AudioStreamGetTypeID );
	return( gAudioStreamTypeID );
}

static void _AudioStreamGetTypeID( void *inContext )
{
	(void) inContext;
	
	gAudioStreamTypeID = _CFRuntimeRegisterClass( &kAudioStreamClass );
	check( gAudioStreamTypeID != _kCFRuntimeNotATypeID );
}

//===========================================================================================================================
//	AudioStreamCreate
//===========================================================================================================================

OSStatus	AudioStreamCreate( AudioStreamRef *outStream )
{
	OSStatus			err;
	AudioStreamRef		me;
	size_t				extraLen;
	
	extraLen = sizeof( *me ) - sizeof( me->base );
	me = (AudioStreamRef) _CFRuntimeCreateInstance( NULL, AudioStreamGetTypeID(), (CFIndex) extraLen, NULL );
	require_action( me, exit, err = kNoMemoryErr );
	memset( ( (uint8_t *) me ) + sizeof( me->base ), 0, extraLen );
	
	me->pipeRead  = kInvalidFD;
	me->pipeWrite = kInvalidFD;
	
	*outStream = me;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	_AudioStreamFinalize
//===========================================================================================================================

static void	_AudioStreamFinalize( CFTypeRef inCF )
{
	AudioStreamRef const		me = (AudioStreamRef) inCF;
	
	check( !IsValidFD( me->pipeRead ) );
	check( !IsValidFD( me->pipeWrite ) );
	check( !me->threadPtr );
	check( !me->pcmHandle );
	check( !me->audioBuffer );
	check( !me->pollFDArray );
	ForgetMem( &me->threadName );
}

//===========================================================================================================================
//	AudioStreamSetAudioCallback
//===========================================================================================================================

void	AudioStreamSetAudioCallback( AudioStreamRef me, AudioStremAudioCallback_f inFunc, void *inContext )
{
	me->audioCallbackPtr = inFunc;
	me->audioCallbackCtx = inContext;
}

//===========================================================================================================================
//	AudioStreamSetFlags
//===========================================================================================================================

OSStatus	AudioStreamSetFlags( AudioStreamRef me, AudioStreamFlags inFlags )
{
	me->flags = inFlags;
	return( kNoErr );
}

//===========================================================================================================================
//	AudioStreamSetFormat
//===========================================================================================================================

OSStatus	AudioStreamSetFormat( AudioStreamRef me, const AudioStreamBasicDescription *inFormat )
{
	me->format = *inFormat;
	return( kNoErr );
}

//===========================================================================================================================
//	AudioStreamGetLatency
//===========================================================================================================================

uint32_t	AudioStreamGetLatency( AudioStreamRef me, OSStatus *outErr )
{
	uint32_t				result = 0;
	OSStatus				err;
	snd_pcm_uframes_t		bufferSize, periodSize;
	
	require_action( me->pcmHandle, exit, err = kNotPreparedErr );
	
	err = snd_pcm_get_params( me->pcmHandle, &bufferSize, &periodSize );
	require_noerr( err, exit );
	
	result = (uint32_t)( ( ( (uint64_t) bufferSize ) * kMicrosecondsPerSecond ) / me->format.mSampleRate );
	
exit:
	if( outErr ) *outErr = err;
	return( result );
}

//===========================================================================================================================
//	AudioStreamSetPreferredLatency
//===========================================================================================================================

OSStatus	AudioStreamSetPreferredLatency( AudioStreamRef me, uint32_t inMics )
{
	me->preferredLatencyMics = inMics;
	return( kNoErr );
}

//===========================================================================================================================
//	AudioStreamSetThreadName
//===========================================================================================================================

OSStatus	AudioStreamSetThreadName( AudioStreamRef me, const char *inName )
{
	OSStatus		err;
	char *			name;
	
	if( inName )
	{
		name = strdup( inName );
		require_action( name, exit, err = kNoMemoryErr );
	}
	else
	{
		name = NULL;
	}
	if( me->threadName ) free( me->threadName );
	me->threadName = name;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	AudioStreamSetThreadPriority
//===========================================================================================================================

void	AudioStreamSetThreadPriority( AudioStreamRef me, int inPriority )
{
	me->threadPriority = inPriority;
	me->hasThreadPriority = true;
}

//===========================================================================================================================
//	AudioStreamGetVolume
//===========================================================================================================================

double	AudioStreamGetVolume( AudioStreamRef me, OSStatus *outErr )
{
	double								normalizedVolume = 1.0;
	OSStatus							err;
	snd_mixer_t *						mixer = NULL;
	snd_mixer_selem_id_t *				sid;
	snd_mixer_elem_t *					element;
	snd_mixer_selem_channel_id_t		channel;
	long								minDB, maxDB, db;
	long								minVolume, maxVolume, volume;
	
	(void) me;
	
	err = snd_mixer_open( &mixer, 0 );
	require_noerr( err, exit );
	
	err = snd_mixer_attach( mixer, "default" );
	require_noerr( err, exit );
	
	err = snd_mixer_selem_register( mixer, NULL, NULL );
	require_noerr( err, exit );
	
	err = snd_mixer_load( mixer );
	require_noerr( err, exit );
	
	snd_mixer_selem_id_alloca( &sid );
	snd_mixer_selem_id_set_name( sid, "Master" );
	snd_mixer_selem_id_set_index( sid, 0 );
	element = snd_mixer_find_selem( mixer, sid );
	if( !element )
	{
		snd_mixer_selem_id_set_name( sid, "PCM" );
		element = snd_mixer_find_selem( mixer, sid );
	}
	require_action( element, exit, err = kNotFoundErr );
	
	// Try db volume first so we can detect overdriving and cap it.
	
	err = snd_mixer_selem_get_playback_dB_range( element, &minDB, &maxDB );
	if( !err )
	{
		if( maxDB > 0 ) maxDB = 0; // Cap at 0 dB to avoid overdriving.
		for( channel = 0; channel < SND_MIXER_SCHN_LAST; ++channel )
		{
			if( snd_mixer_selem_has_playback_channel( element, channel ) )
			{
				err = snd_mixer_selem_get_playback_dB( element, channel, &db );
				if( err ) continue;
				
				normalizedVolume = exp10( ( db - maxDB ) / 6000.0 );
				normalizedVolume = Clamp( normalizedVolume, 0.0, 1.0 );
				goto exit;
			}
		}
	}
	
	// No db volume support so try linear volume.
	
	err = snd_mixer_selem_get_playback_volume_range( element, &minVolume, &maxVolume );
	if( !err )
	{
		for( channel = 0; channel < SND_MIXER_SCHN_LAST; ++channel )
		{
			if( snd_mixer_selem_has_playback_channel( element, channel ) )
			{
				err = snd_mixer_selem_get_playback_volume( element, channel, &volume );
				if( err ) continue;
				
				normalizedVolume = TranslateValue( volume, minVolume, maxVolume, 0.0, 1.0 );
				normalizedVolume = Clamp( normalizedVolume, 0.0, 1.0 );
				goto exit;
			}
		}
	}
	
	// No volume controls so assume it's always full volume and leave the result as 1.0.
	
	err = kNoErr;
	
exit:
	if( mixer )		snd_mixer_close( mixer );
	if( outErr )	*outErr = err;
	return( normalizedVolume );
}

//===========================================================================================================================
//	AudioStreamGetVolume
//===========================================================================================================================

//===========================================================================================================================
//	AudioStreamSetVolume
//===========================================================================================================================

OSStatus	AudioStreamSetVolume( AudioStreamRef me, double inVolume )
{
	OSStatus					err;
	snd_mixer_t *				mixer = NULL;
	snd_mixer_selem_id_t *		sid;
	snd_mixer_elem_t *			element;
	long						minDB, maxDB, newDB;
	long						minVolume, maxVolume, newVolume;
	
	(void) me;
	
	err = snd_mixer_open( &mixer, 0 );
	require_noerr( err, exit );
	
	err = snd_mixer_attach( mixer, "default" );
	require_noerr( err, exit );
	
	err = snd_mixer_selem_register( mixer, NULL, NULL );
	require_noerr( err, exit );
	
	err = snd_mixer_load( mixer );
	require_noerr( err, exit );
	
	snd_mixer_selem_id_alloca( &sid );
	snd_mixer_selem_id_set_name( sid, "Master" );
	snd_mixer_selem_id_set_index( sid, 0 );
	element = snd_mixer_find_selem( mixer, sid );
	if( !element )
	{
		snd_mixer_selem_id_set_name( sid, "PCM" );
		element = snd_mixer_find_selem( mixer, sid );
	}
	require_action( element, exit, err = kNotFoundErr );
	
	// Try db volume first so we can detect overdriving and cap it.
	
	err = snd_mixer_selem_get_playback_dB_range( element, &minDB, &maxDB );
	if( !err )
	{
		if( maxDB > 0 ) maxDB = 0; // Cap at 0 dB to avoid overdriving.
		if(      inVolume <= 0 ) newDB = minDB;
		else if( inVolume >= 1 ) newDB = maxDB;
		else
		{
			newDB = (long)( ( 6000.0 * log10( inVolume ) ) + maxDB );
			newDB = Clamp( newDB, minDB, maxDB );
		}
		err = snd_mixer_selem_set_playback_dB_all( element, newDB, 0 );
		if( !err ) goto exit;
	}
	
	// No db volume support so try linear volume.
	
	err = snd_mixer_selem_get_playback_volume_range( element, &minVolume, &maxVolume );
	if( !err )
	{
		if(      inVolume <= 0 ) newVolume = minVolume;
		else if( inVolume >= 1 ) newVolume = maxVolume;
		else
		{
			newVolume = (long) TranslateValue( inVolume, 0.0, 1.0, minVolume, maxVolume );
			newVolume = Clamp( newVolume, minVolume, maxVolume );
		}
		err = snd_mixer_selem_set_playback_volume_all( element, newVolume );
		if( !err ) goto exit;
	}
	
	// No volume controls so assume we can't change the volume so just return noErr for now.
	
	err = kNoErr;
	
exit:
	if( mixer ) snd_mixer_close( mixer );
	return( err );
}

//===========================================================================================================================
//	AudioStreamSetVolume
//===========================================================================================================================

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	AudioStreamPrepare
//===========================================================================================================================

OSStatus	AudioStreamPrepare( AudioStreamRef me )
{
	OSStatus				err;
	Boolean					be, sign;
	snd_pcm_format_t		format;
	snd_pcm_uframes_t		bufferSize, periodSize;
	
	// Convert the ASBD to snd format.
	
	require_action( me->format.mFormatID == kAudioFormatLinearPCM, exit, err = kFormatErr );
	be   = ( me->format.mFormatFlags & kAudioFormatFlagIsBigEndian )		? true : false;
	sign = ( me->format.mFormatFlags & kAudioFormatFlagIsSignedInteger )	? true : false;
	if( me->format.mBitsPerChannel == 8 )
	{
		if( sign )			format = SND_PCM_FORMAT_S8;
		else				format = SND_PCM_FORMAT_U8;
	}
	else if( me->format.mBitsPerChannel == 16 )
	{
		if( be && sign )	format = SND_PCM_FORMAT_S16_BE;
		else if( be )		format = SND_PCM_FORMAT_U16_BE;
		else if( sign )		format = SND_PCM_FORMAT_S16_LE;
		else				format = SND_PCM_FORMAT_U16_LE;
	}
	else if( me->format.mBitsPerChannel == 24 )
	{
		if( be && sign )	format = SND_PCM_FORMAT_S24_BE;
		else if( be )		format = SND_PCM_FORMAT_U24_BE;
		else if( sign )		format = SND_PCM_FORMAT_S24_LE;
		else				format = SND_PCM_FORMAT_U24_LE;
	}
	else
	{
		dlogassert( "Unsupported audio format: %{asbd}", &me->format );
		err = kUnsupportedErr;
		goto exit;
	}
	
	// Open the OS audio engine and configure it.
	
	err = _AudioStreamOpenDefaultDevice( me );
	require_noerr_quiet( err, exit );
	
	err = snd_pcm_set_params( me->pcmHandle, format, SND_PCM_ACCESS_RW_INTERLEAVED, me->format.mChannelsPerFrame, 
		me->format.mSampleRate, true, ( me->preferredLatencyMics > 0 ) ? me->preferredLatencyMics : 100000 );
	require_noerr( err, exit );
	
	err = snd_pcm_prepare( me->pcmHandle );
	require_noerr( err, exit );
	
	err = snd_pcm_get_params( me->pcmHandle, &bufferSize, &periodSize );
	asla_ulog( kLogLevelNotice, "### ALSA bufferSize=%d periodSize=%d\n", bufferSize, periodSize );
	require_noerr( err, exit );
	if( bufferSize <= 0 ) bufferSize = 4096;
	
	me->audioMaxLen = 3 * ( bufferSize * me->format.mBytesPerFrame ); // 3x for double-buffer + slop.
	me->audioBuffer = (uint8_t *) malloc( me->audioMaxLen );
	require_action( me->audioBuffer, exit, err = kNoMemoryErr );
	
exit:
	if( err ) AudioStreamStop( me, false );
	return( err );
}

//===========================================================================================================================
//	AudioStreamStart
//===========================================================================================================================

OSStatus	AudioStreamStart( AudioStreamRef me )
{
	OSStatus		err;
	int				pipeFDs[ 2 ];
	int				n;
	
	if( !me->pcmHandle )
	{
		err = AudioStreamPrepare( me );
		require_noerr( err, exit );
	}
	
	// Set up file descriptors to wait on.
	
	err = pipe( pipeFDs );
	err = map_global_noerr_errno( err );
	require_noerr( err, exit );
	me->pipeRead  = pipeFDs[ 0 ];
	me->pipeWrite = pipeFDs[ 1 ];
	
	n = snd_pcm_poll_descriptors_count( me->pcmHandle );
	require_action( n > 0, exit, err = kUnknownErr );
	
	me->pollFDCount = 1 + n;
	me->pollFDArray = (struct pollfd *) calloc( me->pollFDCount, sizeof( *me->pollFDArray ) );
	require_action( me->pollFDArray, exit, err = kNoMemoryErr );
	
	me->pollFDArray[ 0 ].fd		= me->pipeRead;
	me->pollFDArray[ 0 ].events = POLLIN;
	
	n = snd_pcm_poll_descriptors( me->pcmHandle, &me->pollFDArray[ 1 ], n );
	require_action( n > 0, exit, err = kUnknownErr );
	
	// Start a thread to process audio.
	
	err = pthread_create( &me->thread, NULL, _AudioStreamThread, me );
	require_noerr( err, exit );
	me->threadPtr = &me->thread;
	CFRetain( me );
	
exit:
	if( err ) AudioStreamStop( me, false );
	return( err );
}

//===========================================================================================================================
//	AudioStreamStop
//===========================================================================================================================

void	AudioStreamStop( AudioStreamRef me, Boolean inDrain )
{
	OSStatus		err;
	ssize_t			n;
	Boolean			doRelease = false;
	
	DEBUG_USE_ONLY( err );
	
	if( me->threadPtr )
	{
		n = write( me->pipeWrite, "q", 1 );
		err = map_global_value_errno( n == 1, n );
		check_noerr( err );
		
		err = pthread_join( me->thread, NULL );
		check_noerr( err );
		me->threadPtr = NULL;
		doRelease = true;
	}
	ForgetMem( &me->pollFDArray );
	ForgetFD( &me->pipeRead );
	ForgetFD( &me->pipeWrite );
	ForgetMem( &me->audioBuffer );
	if( me->pcmHandle )
	{
		if( inDrain )
		{
				err = snd_pcm_drain( me->pcmHandle );
				check_noerr( err );
		}
		snd_pcm_close( me->pcmHandle );
		me->pcmHandle = NULL;
	}
	if( doRelease ) CFRelease( me );
}

//===========================================================================================================================
//	_AudioStreamThread
//===========================================================================================================================

static void *	_AudioStreamThread( void *inArg )
{
	AudioStreamRef const		me = (AudioStreamRef) inArg;
	ssize_t						n;
	OSStatus					err, oldErr;
	snd_pcm_t * const			pcmHandle		= me->pcmHandle;
	struct pollfd * const		allPollFDArray	= me->pollFDArray;
	int							allPollFDCount	= me->pollFDCount;
	struct pollfd * const		pcmPollFDArray	= &me->pollFDArray[ 1 ];
	int							pcmPollFDCount	= me->pollFDCount - 1;
	Boolean const				input			= ( me->flags & kAudioStreamFlag_Input ) ? true : false;
	size_t const				bytesPerUnit	= me->format.mBytesPerFrame;
	snd_pcm_sframes_t const		maxFrames		= (snd_pcm_sframes_t)( me->audioMaxLen / bytesPerUnit );
	uint64_t					hostTime;
	uint32_t					sampleTime		= 0;
	unsigned short				revents;
	snd_pcm_sframes_t			frames;
	uint8_t *					ptr;
	
	SetThreadName( me->threadName ? me->threadName : "AudioStreamThread" );
	if( me->hasThreadPriority ) SetThreadPriority( me->threadPriority );
	
	if( input )
	{
		err = snd_pcm_start( pcmHandle );
		check_noerr( err );
	}
	for( ;; )
	{
		n = poll( allPollFDArray, allPollFDCount, -1 );
		err = poll_errno( n );
		if( err == EINTR ) continue;
		if( err ) { dlogassert( "poll() error: %#m", err ); usleep( 10000 ); continue; }
		if( allPollFDArray[ 0 ].revents & POLLIN ) break; // Only event is quit so stop anytime pipe is readable.
		
		revents = 0;
		err = snd_pcm_poll_descriptors_revents( pcmHandle, pcmPollFDArray, pcmPollFDCount, &revents );
		if( err < 0 )
		{
			usleep( 10000 );
			oldErr = err;
			err = _AudioStreamRecover( me, oldErr );
			asla_ulog( kLogLevelNotice, "### ALSA poll revents error %#m -> %#m\n", oldErr, err );
			continue;
		}
		if( revents & ( POLLERR | POLLNVAL ) )
		{
			usleep( 10000 );
			switch( snd_pcm_state( pcmHandle ) )
			{
				case SND_PCM_STATE_XRUN:			oldErr = -EPIPE;	break;
				case SND_PCM_STATE_SUSPENDED:		oldErr = -ESTRPIPE;	break;
				case SND_PCM_STATE_DISCONNECTED:	oldErr = -ENODEV;	break;
				default:							oldErr = -EIO;		break;
			}
			err = _AudioStreamRecover( me, oldErr );
			asla_ulog( kLogLevelNotice, "### ALSA revent error %#m -> %#m\n", oldErr, err );
			continue;
		}
		if( !( revents & ( POLLIN | POLLOUT ) ) ) continue;
		
		frames = snd_pcm_avail_update( pcmHandle );
		if( frames <= 0 )
		{
			err = _AudioStreamRecover( me, frames );
			asla_ulog( kLogLevelNotice, "### ALSA avail update error %#m -> %#m\n", (OSStatus) frames, err );
			continue;
		}
		
		hostTime = UpTicks();
		ptr = me->audioBuffer;
		if( input )
		{
			n = snd_pcm_readi( pcmHandle, ptr, frames );
			if( n <= 0 )
			{
				err = _AudioStreamRecover( me, n );
				asla_ulog( kLogLevelNotice, "### ALSA read error %#m -> %#m\n", (OSStatus) n, err );
				continue;
			}
			
			me->audioCallbackPtr( sampleTime, hostTime, ptr, (size_t)( n * bytesPerUnit ), me->audioCallbackCtx );
			sampleTime += n;
		}
		else
		{
			frames = Min( frames, maxFrames );
			me->audioCallbackPtr( sampleTime, hostTime, ptr, (size_t)( frames * bytesPerUnit ), me->audioCallbackCtx );
			sampleTime += frames;
			
			while( frames > 0 )
			{
				n = snd_pcm_writei( pcmHandle, ptr, frames );
				if( n <= 0 )
				{
					err = _AudioStreamRecover( me, n );
					asla_ulog( kLogLevelNotice, "### ALSA write error %#m -> %#m\n", (OSStatus) n, err );
					continue;
				}
				ptr += ( n * bytesPerUnit );
				frames -= n;
			}
		}
	}
	return( NULL );
}

//===========================================================================================================================
//	_AudioStreamThread
//===========================================================================================================================

//===========================================================================================================================
//	_AudioStreamOpenDefaultDevice
//===========================================================================================================================

static OSStatus	_AudioStreamOpenDefaultDevice( AudioStreamRef me )
{
	OSStatus				err;
	snd_pcm_stream_t		direction;
	int						cardIndex, deviceIndex;
	char					name[ 32 ];
	snd_ctl_t *				ctlHandle;
	snd_pcm_info_t *		pcmInfo;
	
	direction = ( me->flags & kAudioStreamFlag_Input ) ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;
	
	err = snd_pcm_open( &me->pcmHandle, "default", direction, 0 );
	if( !err ) goto exit;
	
	err = snd_pcm_open( &me->pcmHandle, "plug:dmix", direction, 0 );
	if( !err ) goto exit;

	asla_dlog( kLogLevelVerbose, "Default audio %s device not configured, searching for alternate\n", 
		( me->flags & kAudioStreamFlag_Input ) ? "input" : "output" );
	
	// Walk all the cards and devices and use the first one that supports the audio direction and can be opened.
	
	cardIndex = -1;
	snd_pcm_info_alloca( &pcmInfo );
	for( ;; )
	{
		ctlHandle = NULL;
		err = snd_card_next( &cardIndex );
		require_noerr_quiet( err, exit );
		require_action_quiet( cardIndex >= 0, exit, err = kNotFoundErr );
		
		snprintf( name, sizeof( name ), "hw:%u", cardIndex );
		asla_dlog( kLogLevelChatty, "Checking %s \n", name );
		err = snd_ctl_open( &ctlHandle, name, 0 );
		require_noerr_quiet( err, nextCard );
		
		deviceIndex = -1;
		for( ;; )
		{
			err = snd_ctl_pcm_next_device( ctlHandle, &deviceIndex );
			require_noerr_quiet( err, nextCard );
			require_action_quiet( deviceIndex >= 0, nextCard, err = kNotFoundErr );
			
			snd_pcm_info_set_device( pcmInfo, deviceIndex );
			snd_pcm_info_set_subdevice( pcmInfo, 0 );
			snd_pcm_info_set_stream( pcmInfo, direction );
			err = snd_ctl_pcm_info( ctlHandle, pcmInfo );
			if( err ) continue;
			
			snprintf( name, sizeof( name ), "plughw:%u", cardIndex );
			err = snd_pcm_open( &me->pcmHandle, name, direction, 0 );
			asla_dlog( kLogLevelChatty, "Trying %s: %#m\n", name, err );
			if( !err ) break;
		}
		
	nextCard:
		if( ctlHandle ) snd_ctl_close( ctlHandle );
		if( !err ) break;
	}
	
exit:
	return( err );
}

//===========================================================================================================================
//	_AudioStreamRecover
//===========================================================================================================================

static OSStatus	_AudioStreamRecover( AudioStreamRef me, int inError )
{
	OSStatus		err;
	int				i;
	
	if( inError == -EINTR )
	{
		err = kNoErr;
	}
	else if( inError == -EPIPE ) // Underrun
	{
		err = snd_pcm_prepare( me->pcmHandle );
	}
	else if( inError == -ESTRPIPE ) // Hardware suspended
	{
		for( i = 0; i < 100; ++i )
		{
			err = snd_pcm_resume( me->pcmHandle );
			if( err != -EAGAIN ) break;
			usleep( 10000 );
		}
		if( err ) err = snd_pcm_prepare( me->pcmHandle );
	}
	else
	{
		err = inError;
	}
	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	AudioStreamTest
//===========================================================================================================================

static void
	_AudioStreamTestInput( 
		uint32_t	inSampleTime, 
		uint64_t	inHostTime, 
		void *		inBuffer, 
		size_t		inLen, 
		void *		inContext );

static void
	_AudioStreamTestOutput( 
		uint32_t	inSampleTime, 
		uint64_t	inHostTime, 
		void *		inBuffer, 
		size_t		inLen, 
		void *		inContext );

OSStatus	AudioStreamTest( Boolean inInput )
{
	OSStatus						err;
	SineTableRef					sineTable = NULL;
	AudioStreamRef					me = NULL;
	AudioStreamBasicDescription		asbd;
	size_t							written = 0;
	
	err = SineTable_Create( &sineTable, 44100, 800 );
	require_noerr( err, exit );
	
	err = AudioStreamCreate( &me );
	require_noerr( err, exit );
	
	if( inInput )
	{
		AudioStreamSetAudioCallback( me, _AudioStreamTestInput, &written );
		AudioStreamSetFlags( me, kAudioStreamFlag_Input );
	}
	else
	{
		AudioStreamSetAudioCallback( me, _AudioStreamTestOutput, sineTable );
	}
	
	ASBD_FillPCM( &asbd, 44100, 16, 16, 2 );
	err = AudioStreamSetFormat( me, &asbd );
	require_noerr( err, exit );
	
	err = AudioStreamSetPreferredLatency( me, 100000 );
	require_noerr( err, exit );
	
	err = AudioStreamStart( me );
	require_noerr( err, exit );
	
	sleep( 5 );
	
	if( inInput )
	{
		require_action( written > 0, exit, err = kReadErr );
	}
	
exit:
	AudioStreamForget( &me );
	if( sineTable ) SineTable_Delete( sineTable );
	dlog( kDebugLevelMax, "%###s: %s\n", __ROUTINE__, !err ? "PASSED" : "FAILED" );
	return( err );
}

static void
	_AudioStreamTestInput( 
		uint32_t	inSampleTime, 
		uint64_t	inHostTime, 
		void *		inBuffer, 
		size_t		inLen, 
		void *		inContext )
{
	size_t * const		writtenPtr = (size_t *) inContext;
	
	(void) inSampleTime;
	(void) inHostTime;
	(void) inBuffer;
	
	*writtenPtr += inLen;
}

static void
	_AudioStreamTestOutput( 
		uint32_t	inSampleTime, 
		uint64_t	inHostTime, 
		void *		inBuffer, 
		size_t		inLen, 
		void *		inContext )
{
	SineTableRef const		sineTable = (SineTableRef) inContext;
	
	(void) inSampleTime;
	(void) inHostTime;
	
	SineTable_GetSamples( sineTable, 0, inLen / 4, inBuffer );
}

