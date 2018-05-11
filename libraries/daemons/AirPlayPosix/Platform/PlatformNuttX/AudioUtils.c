/*
 * Audio Utils
 *
 * $Copyright (C) 2016 Cypress Semiconductors All Rights Reserved.$
 *
 * $Id: AudioUtils.c $
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

#include <nuttx/config.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <debug.h>
#include <math.h>
#include <nuttx/audio/audio.h>

#include "wiced.h"
#include "wiced_audio.h"
#include "platform_audio.h"

#include CF_HEADER
#include CF_RUNTIME_HEADER
#include LIBDISPATCH_HEADER

//===========================================================================================================================
//	AudioStream
//===========================================================================================================================

#define PERIOD_SIZE			( 1 * 1024 )
#define TX_START_THRESHOLD		( 3 * PERIOD_SIZE )

typedef struct
{
	Boolean				inuse;
	struct ap_buffer_s*		apb;
} buffer_t;

struct AudioStreamPrivate
{
	CFRuntimeBase				base;			// CF type info. Must be first.
	int					pipeRead;		// Read side of command pipe (for thread to read commands from).
	int					pipeWrite;		// Write side of command pipe (for writing commands to the thread).
	pthread_t				thread;			// Thread processing audio.
	pthread_t *				threadPtr;		// Ptr to audio thread for NULL testing.
	AudioStremAudioCallback_f		audioCallbackPtr;	// Function to call to give input audio or get output audio.
	void *					audioCallbackCtx;	// Context to pass to audio callback function.
	AudioStreamFlags			flags;			// Flags indicating input or output, etc.
	AudioStreamBasicDescription		format;			// Format of the audio data.
	uint32_t				preferredLatencyMics;	// Max latency the app can tolerate.
	char *					threadName;		// Name to use when creating threads.
	int					threadPriority;		// Priority to run threads.
	Boolean					hasThreadPriority;	// True if a thread priority has been set.
	Boolean					initialized;		// True if audio subsystem is initialized
	Boolean					reserved;		// True if audio driver session is reserved.
	Boolean					prepared;		// True if audio driver session is prepared.
	Boolean					running;		// True if audio stream is running.
	Boolean					stop;			// True if audio stream is to be stopped.
	int					devFd;			// Audio device file descriptor.
	buffer_t*				buf;			// Ptr to audio buffers.
	struct ap_buffer_info_s			bufInfo;		// Audio buffer info.
	char					mqName[16];		// Message queue name.
	mqd_t					mq;			// Message queue.
	void*					session;		// Ptr to audio session.
};

static void	_AudioStreamGetTypeID( void *inContext );
static void	_AudioStreamFinalize( CFTypeRef inCF );
static void *	_AudioStreamThread( void *inArg );
static OSStatus	_AudioStreamOpenDefaultDevice( AudioStreamRef me );

static dispatch_once_t			gAudioStreamInitOnce = 0;
static CFTypeID				gAudioStreamTypeID = _kCFRuntimeNotATypeID;
static const CFRuntimeClass		kAudioStreamClass =
{
	0,				// version
	"AudioStream",			// className
	NULL,				// init
	NULL,				// copy
	_AudioStreamFinalize,		// finalize
	NULL,				// equal -- NULL means pointer equality.
	NULL,				// hash  -- NULL means pointer hash.
	NULL,				// copyFormattingDesc
	NULL,				// copyDebugDesc
	NULL,				// reclaim
	NULL				// refcount
};

//===========================================================================================================================
//	Logging
//===========================================================================================================================

ulog_define( AudioUtils, kLogLevelNotice, kLogFlags_Default, "AudioUtils", NULL );
#define aud_dlog( LEVEL, ... )		dlogc( &log_category_from_name( AudioUtils ), (LEVEL), __VA_ARGS__ )
#define aud_ulog( LEVEL, ... )		ulog( &log_category_from_name( AudioUtils ), (LEVEL), __VA_ARGS__ )
#define aud_tlog( fmt, ... )		aud_ulog( kLogLevelTrace, fmt "\n", ##__VA_ARGS__ )
#define aud_nlog( fmt, ... )		aud_ulog( kLogLevelNotice, fmt "\n", ##__VA_ARGS__ )
#define aud_elog( fmt, ... )		aud_ulog( kLogLevelError, fmt "\n", ##__VA_ARGS__ )

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
	OSStatus		err;
	AudioStreamRef		me;
	size_t			extraLen;

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
	uint32_t				latency;
	struct audio_caps_desc_s		cap_desc;

#ifdef CONFIG_AUDIO_MULTI_SESSION
	cap_desc.session = me->session;
#endif
	if( ioctl( me->devFd, AUDIOIOC_GETLATENCY, (unsigned long) &cap_desc ) < 0 )
	{
		aud_elog( "Failed to get latency" );
		latency = 0;
	}
	else
	{
		latency = cap_desc.caps.ac_controls.w;
	}

	return( latency );
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
	double		normalizedVolume = 1.0;

	// No volume controls so assume it's always full volume and leave the result as 1.0.

	if( outErr )	*outErr = kNoErr;

	return( normalizedVolume );
}

//===========================================================================================================================
//	AudioStreamSetVolume
//===========================================================================================================================

OSStatus	AudioStreamSetVolume( AudioStreamRef me, double inVolume )
{
	OSStatus				err;
	struct audio_caps_desc_s		cap_desc;

	(void) me;

#ifdef CONFIG_AUDIO_MULTI_SESSION
	cap_desc.session		= me->session;
#endif
	cap_desc.caps.ac_len		= sizeof( struct audio_caps_s );
	cap_desc.caps.ac_type		= AUDIO_TYPE_FEATURE;
	cap_desc.caps.ac_format.hw	= AUDIO_FU_VOLUME;
	cap_desc.caps.ac_controls.hw[0]	= (uint16_t) ( inVolume * 1000.0 );
	if( ioctl( me->devFd, AUDIOIOC_CONFIGURE, (unsigned long) &cap_desc ) < 0 )
	{
		aud_elog( "ERROR: AUDIOIOC_CONFIGURE ioctl failed" );
		err = kUnknownErr;
		goto exit;
	}

	err = kNoErr;
exit:
	return( err );
}

//===========================================================================================================================
//	AudioStreamPrepare
//===========================================================================================================================

OSStatus	AudioStreamPrepare( AudioStreamRef me )
{
	OSStatus				err;
	int					i;
	struct audio_caps_desc_s		capDesc;
	struct mq_attr				attr;

	if( me->prepared ) return kNoErr;

	// Open the OS audio engine and configure it.

	err = _AudioStreamOpenDefaultDevice( me );
	require_noerr_quiet( err, exit );

	// Reserve a session

#ifdef CONFIG_AUDIO_MULTI_SESSION
	if( ioctl( me->devFd, AUDIOIOC_RESERVE, (unsigned long) &me->session ) < 0 )
#else
	if( ioctl( me->devFd, AUDIOIOC_RESERVE, 0) < 0 )
#endif
	{
		aud_elog( "Failed to reserve an audio session" );
		err = kUnknownErr;
		goto exit;
	}

	me->reserved = true;

	aud_nlog( "Reserved an audio session" );

	// Configure device

#ifdef CONFIG_AUDIO_MULTI_SESSION
	capDesc.session			= me->session;
#endif
	capDesc.caps.ac_len		= sizeof( struct audio_caps_s );
	capDesc.caps.ac_type		= AUDIO_TYPE_OUTPUT;

	capDesc.caps.ac_format.hw	= PLATFORM_DEFAULT_AUDIO_OUTPUT;

	capDesc.caps.ac_controls.hw[0]	= me->format.mSampleRate; /* sample rate (Hz) */
	capDesc.caps.ac_controls.b[2]	= me->format.mBitsPerChannel; /* bits per sample */
	capDesc.caps.ac_channels	= me->format.mChannelsPerFrame;;

	if( ioctl( me->devFd, AUDIOIOC_CONFIGURE, (unsigned long) &capDesc ) != OK )
	{
		aud_elog( "Failed to configure audio device" );
		err = kUnknownErr;
		goto exit;
	}

	aud_nlog( "Configured audio device" );

	// Create a message queue

	attr.mq_maxmsg  = 16;
	attr.mq_msgsize = sizeof( struct audio_msg_s );
	attr.mq_curmsgs = 0;
	attr.mq_flags   = 0;

	snprintf( me->mqName, sizeof( me->mqName ), "/tmp/%0lx", (unsigned long) me->mqName );

	me->mq = mq_open( me->mqName, O_RDWR | O_CREAT, 0644, &attr );
	if( me->mq == (mqd_t)-1 )
	{
		aud_elog( "Failed to create message queue" );
		err = kUnknownErr;
		goto exit;
	}

	// Register our message queue with the audio device
	if( ioctl( me->devFd, AUDIOIOC_REGISTERMQ, (unsigned long) me->mq ) != OK )
	{
		aud_elog( "Failed to register message queue" );
		err = kUnknownErr;
		goto exit;
	}

	aud_nlog( "Registered message queue (%s) with audio device", me->mqName );

#if defined(CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS)
	if( ioctl( me->devFd, AUDIOIOC_GETBUFFERINFO, (unsigned long) &me->bufInfo ) != OK )
	{
		aud_elog( "Failed to get buffer info" );
		err = kUnknownErr;
		goto exit;
	}

	aud_nlog( "Buffer info: %u x %u bytes", me->bufInfo.nbuffers, me->bufInfo.buffer_size );

	me->buf = calloc( 1, me->bufInfo.nbuffers * sizeof( buffer_t ) );
	if( me->buf == NULL )
	{
		aud_elog( "Failed to allocate buffer structures" );
		err = kUnknownErr;
		goto exit;
	}

	aud_nlog( "Allocated buffers @ 0x%p", me->buf );

	for( i = 0; i < me->bufInfo.nbuffers; i++ )
	{
		struct audio_buf_desc_s buf_desc;

		buf_desc.numbytes = me->bufInfo.buffer_size;
		buf_desc.u.ppBuffer = &me->buf[i].apb;

		if( ioctl( me->devFd, AUDIOIOC_ALLOCBUFFER, (unsigned long) &buf_desc ) != sizeof( buf_desc ) )
		{
			aud_elog( "Failed to allocate TX buffers" );
			me->bufInfo.nbuffers = i;
			err = kUnknownErr;
			goto exit;
		}

		aud_tlog( "Allocated buffer: %u bytes @ %p", me->buf[i].apb->nmaxbytes, me->buf[i].apb->samp );
	}

	aud_nlog( "Allocated %u TX buffers", me->bufInfo.nbuffers );

#else /* !defined(CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS) */
	err = kUnknownErr;

	goto exit;
#endif /* !defined(CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS) */

	me->prepared = true;

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

	me->stop = false;
	me->running = true;

	// Start a thread to process audio

	aud_nlog( "Starting audio thread" );
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
	Boolean			doRelease = false;

	DEBUG_USE_ONLY( err );

	if( me->threadPtr )
	{
		aud_nlog( "Stopping audio thread" );
		me->stop = true;

		err = pthread_join( me->thread, NULL );
		check_noerr( err );
		me->threadPtr = NULL;
		doRelease = true;
	}

	if( me->initialized )
	{
		// Free buffers

		if( me->buf != NULL )
		{
			int		i;

			for( i = 0; i < me->bufInfo.nbuffers; i++ )
			{
				struct audio_buf_desc_s		buf_desc;

				buf_desc.numbytes = me->bufInfo.buffer_size;
				buf_desc.u.ppBuffer = &me->buf[i].apb;

				if( ioctl( me->devFd, AUDIOIOC_FREEBUFFER, (unsigned long) &buf_desc ) != OK )
				{
					aud_elog( "Failed to free TX buffer" );
				}
				else
				{
					aud_tlog( "Freed buffer: %u bytes @ %p", me->buf[i].apb->nmaxbytes, me->buf[i].apb->samp );
				}
			}

			free( me->buf );
			me->buf = NULL;

			aud_elog( "Freed TX buffers" );
		}

		// Unregister message queue

		if( me->mq != (mqd_t)-1 )
		{
			if( ioctl( me->devFd, AUDIOIOC_UNREGISTERMQ, (unsigned long) me->mq ) < 0 )
			{
				aud_elog( "Failed to unregister message queue (%s)", me->mqName );
			}
			else
			{
				aud_nlog( "Unregistered message queue (%s)", me->mqName );
			}

			// Close message queue

			mq_close( me->mq );
			mq_unlink( me->mqName );
			me->mq = (mqd_t)-1;

			aud_nlog( "Closed message queue (%s)", me->mqName );
		}

		// Release audio session

		if( me->reserved )
		{
#ifdef CONFIG_AUDIO_MULTI_SESSION
			if( ioctl( me->devFd, AUDIOIOC_RELEASE, (unsigned long) &me->session ) < 0 )
#else
			if( ioctl( me->devFd, AUDIOIOC_RELEASE, 0 ) < 0 )
#endif
			{
				aud_elog( "Failed to release audio session" );
			}
			else
			{
				aud_nlog( "Released audio session" );
			}

			me->reserved = false;
		}
	}

	me->prepared = false;

	if( doRelease ) CFRelease( me );
}

//===========================================================================================================================
//	_AudioStreamThread
//===========================================================================================================================

static void *	_AudioStreamThread( void *inArg )
{
	AudioStreamRef const		me = (AudioStreamRef) inArg;
	uint16_t			n;
	size_t const			bytesPerUnit	= me->format.mBytesPerFrame;
	ssize_t const		        maxFrames = PERIOD_SIZE / bytesPerUnit;
	Boolean				is_tx_started = false;
	ssize_t				frames;
	uint64_t			hostTime;
	uint32_t			sampleTime = 0;

	SetThreadName( me->threadName ? me->threadName : "AudioStreamThread" );
	if( me->hasThreadPriority ) SetThreadPriority( me->threadPriority );

	while( !me->stop )
	{
		struct audio_buf_desc_s		bufdesc;
		uint16_t			remaining;

		// Start data transmission

		if( !is_tx_started )
		{
			struct audio_caps_desc_s		cap_desc;

#ifdef CONFIG_AUDIO_MULTI_SESSION
			cap_desc.session = me->session;
#endif
			if( ioctl( me->devFd, AUDIOIOC_GETBUFFERWEIGHT, (unsigned long) &cap_desc ) < 0 )
			{
				aud_elog( "Failed to get TX buffer weight" );
				break;
			}

			if( cap_desc.caps.ac_controls.w >= TX_START_THRESHOLD )
			{
				int ret;

#ifdef CONFIG_AUDIO_MULTI_SESSION
				ret = ioctl( me->devFd, AUDIOIOC_START, (unsigned long) me->session );
#else
				ret = ioctl( me->devFd, AUDIOIOC_START, 0 );
#endif
				if( ret < 0 )
				{
					aud_elog( "Failed to start audio" );
					break;
				}

				aud_nlog( "Audio started" );

				is_tx_started = 1;
			}
		}

		// Wait for slot in transmit buffer

#ifdef CONFIG_AUDIO_MULTI_SESSION
		bufdesc.session   = me->session;
#endif
		bufdesc.numbytes  = 0;
		bufdesc.u.pBuffer = NULL;

		if( is_tx_started )
		{
			struct audio_msg_s		msg;
			int				prio;
			ssize_t				size;

			size = mq_receive( me->mq, (char *) &msg, sizeof( msg ), &prio );
			if( size != sizeof( msg ) )
			{
				aud_elog( "Failed to receive message from message queue" );
				break;
			}

			if( msg.msgId != AUDIO_MSG_DEQUEUE )
			{
				aud_elog( "Unhandled msg (msgId=%u)", msg.msgId );
				break;
			}

			bufdesc.u.pBuffer = msg.u.pPtr;
			bufdesc.numbytes = ((struct ap_buffer_s*) msg.u.pPtr)->nmaxbytes;
		}
		else
		{
			int i;
			for( i = 0; i < me->bufInfo.nbuffers; i++ )
			{
				if( me->buf[i].inuse == 0 )
				{
					bufdesc.u.pBuffer = me->buf[i].apb;
					bufdesc.numbytes = me->buf[i].apb->nmaxbytes;
					me->buf[i].inuse = 1;
					break;
				}
			}
			if( i == me->bufInfo.nbuffers )
			{
				aud_elog( "Ran out of TX buffers" );
				break;
			}
		}

		// Copy available data to transmit buffer

		remaining = PERIOD_SIZE;
		n = remaining;
		while( 0 != remaining )
		{
			uint8_t *		buf;
			uint16_t		avail = remaining;
			int			ret;

			buf = (uint8_t*)bufdesc.u.pBuffer->samp;
			avail = bufdesc.numbytes;

			frames = n / bytesPerUnit;
			if( frames <= 0 )
			{
				aud_elog( "Buffer underrun bytes=%d frames=%d", n, frames );
				break;
			}

			hostTime = UpTicks();
			frames = Min( frames, maxFrames );
			me->audioCallbackPtr( sampleTime, hostTime, buf, (size_t) ( frames * bytesPerUnit ), me->audioCallbackCtx );
			sampleTime += frames;

			ret = ioctl( me->devFd, AUDIOIOC_ENQUEUEBUFFER, (unsigned long) &bufdesc );
			if( ret < 0 )
			{
				aud_elog( "Failed to enqueue TX buffer" );
				break;
			}
			remaining -= avail;
		}
	}

	if( is_tx_started )
	{
		int				ret;
		struct audio_msg_s		msg;
		int				prio;
		ssize_t				size;

		aud_tlog( "Calling ioctl(AUDIOIOC_STOP)" );

#ifdef CONFIG_AUDIO_MULTI_SESSION
		ret = ioctl( me->devFd, AUDIOIOC_STOP, tx->session );
#else
		ret = ioctl( me->devFd, AUDIOIOC_STOP, 0 );
#endif
		if( ret != 0 )
		{
			aud_elog( "Failed to stop TX audio" );
		}
		else
		{
			aud_nlog( "Stopped TX audio" );
		}

		// Wait for AUDIO_MSG_COMPLETE

		while( 1 )
	        {
			size = mq_receive( me->mq, (char *) &msg, sizeof( msg ), &prio );
			if( size != sizeof( msg ) )
			{
				aud_elog( "Failed to receive message from message queue" );
				continue;
			}

			if( msg.msgId == AUDIO_MSG_COMPLETE )
			{
				aud_tlog( "received AUDIO_MSG_COMPLETE" );
				break;
			}
			else
			{
				aud_nlog( "Unhandled msg (msgId=%u)", msg.msgId );
			}
		}

		is_tx_started = 0;
	}

	return( NULL );
}

//===========================================================================================================================
//	_AudioStreamOpenDefaultDevice
//===========================================================================================================================

static OSStatus	_AudioStreamOpenDefaultDevice( AudioStreamRef me )
{
	OSStatus		err;
	struct dirent *		pDevice;
	DIR *			dir = NULL;
	char			path[64];

	if( me->initialized )
	{
		err = kNoErr;
		goto exit;
	}

	// Initialize audio subsystem

	up_audioinitialize();

	// Search for a device in the audio device directory

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
	dir = opendir( "/dev" );
#else
	dir = opendir( CONFIG_AUDIO_DEV_PATH );
#endif  /* CONFIG_AUDIO_DEV_ROOT */
#else
	dir = opendir( "/dev/audio" );
#endif  /* CONFIG_AUDIO_CUSTOM_DEV_PATH */
	if( dir == NULL )
	{
		auddbg( "ERROR: Failed to open /dev/audio: %d", errno );
		err = kUnknownErr;
		goto exit;
	}

	while( ( pDevice = readdir( dir ) ) != NULL )
	{
#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
		snprintf( path,  sizeof( path ), "/dev/%s", pDevice->d_name );
#else
		snprintf( path,  sizeof( path ), CONFIG_AUDIO_DEV_PATH "/%s", pDevice->d_name );
#endif
#else
		snprintf( path,  sizeof( path ), "/dev/audio/%s", pDevice->d_name );
#endif

		if( ( me->devFd = open( path, O_RDWR ) ) == -1 )
		{
			aud_elog( "Failed to open audio device '%s'", path );
			err = kUnknownErr;
			goto exit;
		}

		break;
	}

	aud_nlog( "Opened audio device '%s'", path );

	me->initialized = true;
	err = kNoErr;

exit:
	if( dir != NULL ) closedir( dir );

	return( err );
}

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
	OSStatus				err = 0;
	SineTableRef				sineTable = NULL;
	AudioStreamRef				me = NULL;
	AudioStreamBasicDescription		asbd;
	size_t					written = 0;

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

	aud_nlog( "Audio Test Start");

	ASBD_FillPCM( &asbd, 44100, 16, 16, 2 );
	err = AudioStreamSetFormat( me, &asbd );
	require_noerr( err, exit );

	err = AudioStreamSetPreferredLatency( me, 100000 );
	require_noerr( err, exit );

	err = AudioStreamPrepare( me );
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

int	audiotest( int argc, char *argv[] )
{
    return AudioStreamTest( 0 );
}
