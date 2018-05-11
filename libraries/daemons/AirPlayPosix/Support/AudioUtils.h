/*
	Copyright (C) 2010-2013 Apple Inc. All Rights Reserved.
*/
/*!
	@header Audio Interface
	@discussion Provides APis related to Audio rendering on the accessory.
*/

#ifndef	__AudioUtils_h__
#define	__AudioUtils_h__

#include "CommonServices.h"

#include CF_HEADER
#include COREAUDIO_HEADER

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IsDTSEncodedData
	@internal
	@abstract	Returns true if the buffer contains data that looks like DTS-encoded data.
*/

Boolean	IsDTSEncodedData( const void *inData, size_t inByteCount );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	IsSilencePCM
	@internal
	@abstract	Returns true if the buffer contains 16-bit PCM samples of only silence (0's).
*/

Boolean	IsSilencePCM( const void *inData, size_t inByteCount );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		SineTable
	@abstract	API for generating sine waves.
*/

typedef struct SineTable *		SineTableRef;
struct SineTable
{
	int			sampleRate;
	int			frequency;
	int			position;
	int16_t		values[ 1 ]; // Variable length array.
};

OSStatus	SineTable_Create( SineTableRef *outTable, int inSampleRate, int inFrequency );
void		SineTable_Delete( SineTableRef inTable );
void		SineTable_GetSamples( SineTableRef inTable, int inBalance, int inSampleCount, void *inSampleBuffer );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		VolumeAdjuster
	@abstract	API for adjusting volume to minimize glitches.
*/

typedef struct
{
	Q16x16		currentVolume;
	Q16x16		targetVolume;
	int32_t		volumeIncrement; // Q2.30 format.
	uint32_t	rampStepsRemaining;
	
}	VolumeAdjusterContext;

void	VolumeAdjusterInit( VolumeAdjusterContext *ctx );
void	VolumeAdjusterSetVolumeDB( VolumeAdjusterContext *ctx, Float32 inDB );
void	VolumeAdjusterApply( VolumeAdjusterContext *ctx, int16_t *inSamples, size_t inCount, size_t inChannels );

#if 0
#pragma mark -
#pragma mark == AudioStream ==
#endif

//===========================================================================================================================
//	AudioStream
//===========================================================================================================================

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamGetTypeID
	@internal
	@abstract	Gets the CF type ID of all AudioStream objects.
*/
CFTypeID	AudioStreamGetTypeID( void );

typedef struct AudioStreamPrivate *		AudioStreamRef;

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamCreate
	@abstract	Creates a new AudioStream.
*/
OSStatus	AudioStreamCreate( AudioStreamRef *outStream );
#define 	AudioStreamForget( X ) do { if( *(X) ) { AudioStreamStop( *(X), false ); CFRelease( *(X) ); *(X) = NULL; } } while( 0 )

typedef void
	( *AudioStremAudioCallback_f )( 
		uint32_t	inSampleTime, 
		uint64_t	inHostTime, 
		void *		inBuffer, 
		size_t		inLen, 
		void *		inContext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamSetAudioCallback
	@abstract	Sets a function to be called for audio input or output (depending on the direction of the stream).
*/
void	AudioStreamSetAudioCallback( AudioStreamRef inStream, AudioStremAudioCallback_f inFunc, void *inContext );

typedef uint32_t		AudioStreamFlags;
#define kAudioStreamFlag_Input		( 1 << 0 )	// Use this stream to read data from a microphone or other input.
#define kAudioStreamFlag_Output		0			// Absence of the input flag means to use this stream for audio output.

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamSetFlags
	@abstract	Streams the AudioStream.

	inFlags will be kAudioStreamFlag_Output.
*/
OSStatus	AudioStreamSetFlags( AudioStreamRef inStream, AudioStreamFlags inFlags );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamSetFormat
	@abstract	Sets the format to provided to the callback for input or the format provided by the callback for output.
*/
OSStatus	AudioStreamSetFormat( AudioStreamRef inStream, const AudioStreamBasicDescription *inFormat );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamGetLatency
	@abstract	Gets the minimum latency the system can achieve (may be higher if samples are already queued).
*/
uint32_t	AudioStreamGetLatency( AudioStreamRef inStream, OSStatus *outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamSetPreferredLatency
	@abstract	Sets the lowest latency the caller thinks it will need. Defaults to 100 ms.
*/
OSStatus	AudioStreamSetPreferredLatency( AudioStreamRef inStream, uint32_t inMics );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamSetThreadName 
	@abstract	Sets the name of threads created by the clock.
*/
OSStatus	AudioStreamSetThreadName( AudioStreamRef inStream, const char *inName );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamSetThreadPriority
	@abstract	Sets the priority of threads created by the clock.
*/
void		AudioStreamSetThreadPriority( AudioStreamRef inStream, int inPriority );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamGetVolume
	@abstract	Gets the current volume of the stream as a linear 0.0-1.0 volume.
*/
double	AudioStreamGetVolume( AudioStreamRef inStream, OSStatus *outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamSetVolume
	@abstract	Sets the volume of the stream to a linear 0.0-1.0 volume.
*/
OSStatus	AudioStreamSetVolume( AudioStreamRef inStream, double inVolume );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamPrepare
	@abstract	Prepares the audio stream so things like latency can be reported, but doesn't start playing audio.
*/
OSStatus	AudioStreamPrepare( AudioStreamRef inStream );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamStart
	@abstract	Starts the stream (callbacks will start getting invoked after this).
*/
OSStatus	AudioStreamStart( AudioStreamRef inStream );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamStop
	@abstract	Stops the stream.
*/
void	AudioStreamStop( AudioStreamRef inStream, Boolean inDrain );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AudioStreamTest
	@internal
	@abstract	Unit test.
*/
OSStatus	AudioStreamTest( Boolean inInput );

#ifdef __cplusplus
}
#endif

#endif	// __AudioUtils_h__
