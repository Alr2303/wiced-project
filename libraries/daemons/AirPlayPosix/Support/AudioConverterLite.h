/*
	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
*/

#ifndef	__AudioConverterLite_h__
#define	__AudioConverterLite_h__

#include "CommonServices.h"

#if( AUDIO_CONVERTER_LITE_ENABLED )

// If we have the AudioToolbox framework, include it to get all its types then though we won't use its APIs.
// This is needed to avoid conflicts if we defined our own versions then somebody else included AudioToolbox.h.

#if( __has_include( <AudioToolbox/AudioToolbox.h> ) )
	#include <AudioToolbox/AudioToolbox.h>
	
	#define HAS_AUDIO_TOOLBOX		1
#endif

#ifdef	__cplusplus
extern "C" {
#endif

//===========================================================================================================================
//	Types
//===========================================================================================================================

#if( !HAS_AUDIO_TOOLBOX )

typedef struct AudioConverterPrivate *		AudioConverterRef;
typedef UInt32								AudioConverterPropertyID;

typedef struct
{
	UInt32		mNumberChannels;
	UInt32		mDataByteSize;
	void *		mData;
	
}	AudioBuffer;

typedef struct
{
	UInt32			mNumberBuffers;
	AudioBuffer		mBuffers[ 1 ];
	
}	AudioBufferList;

typedef struct
{
    int64_t		mStartOffset;
    UInt32		mVariableFramesInPacket;
    UInt32		mDataByteSize;
    
}	AudioStreamPacketDescription;

typedef OSStatus
	( *AudioConverterComplexInputDataProc )( 
		AudioConverterRef				inAudioConverter,
		UInt32 *						ioNumberDataPackets,
		AudioBufferList *				ioData,
		AudioStreamPacketDescription **	outDataPacketDescription,
		void *							inUserData );

#endif // HAS_AUDIO_TOOLBOX

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

OSStatus
	AudioConverterNew_compat( 
		const AudioStreamBasicDescription *	inSourceFormat,
		const AudioStreamBasicDescription *	inDestinationFormat,
		AudioConverterRef *					outAudioConverter );
OSStatus	AudioConverterDispose_compat( AudioConverterRef inAudioConverter );
OSStatus	AudioConverterReset_compat( AudioConverterRef inAudioConverter );
OSStatus
	AudioConverterSetProperty_compat( 
		AudioConverterRef			inAudioConverter,
		AudioConverterPropertyID	inPropertyID,
		UInt32						inSize,
		const void *				inData );
OSStatus
	AudioConverterFillComplexBuffer_compat( 
		AudioConverterRef					inAudioConverter,
		AudioConverterComplexInputDataProc	inInputDataProc,
		void *								inInputDataProcUserData,
		UInt32 *							ioOutputDataPacketSize,
		AudioBufferList *					outOutputData,
		AudioStreamPacketDescription *		outPacketDescription );

// The AudioToolbox headers are available, map our compat functions to real functions so they'll call the lite versions.
// Otherwise, map the real functions to the compat functions so the people can use the real versions in both cases.

#if( HAS_AUDIO_TOOLBOX )
	#define AudioConverterNew_compat					AudioConverterNew
	#define AudioConverterDispose_compat				AudioConverterDispose
	#define AudioConverterReset_compat					AudioConverterReset
	#define AudioConverterSetProperty_compat			AudioConverterSetProperty
	#define AudioConverterFillComplexBuffer_compat		AudioConverterFillComplexBuffer
#else
	#define AudioConverterNew							AudioConverterNew_compat
	#define AudioConverterDispose						AudioConverterDispose_compat
	#define AudioConverterReset							AudioConverterReset_compat
	#define AudioConverterSetProperty					AudioConverterSetProperty_compat
	#define AudioConverterFillComplexBuffer				AudioConverterFillComplexBuffer_compat
#endif

#endif // AUDIO_CONVERTER_LITE_ENABLED

#ifdef __cplusplus
}
#endif

#endif	// __AudioConverterLite_h__
