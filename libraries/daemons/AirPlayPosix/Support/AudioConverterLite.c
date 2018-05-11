/*
	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
*/

#include "AudioConverterLite.h"

#if( AUDIO_CONVERTER_LITE_ENABLED )

#include "CommonServices.h"
#include "DebugServices.h"

#if( !defined( AUDIO_CONVERTER_ALAC ) )
	#define AUDIO_CONVERTER_ALAC		1
#endif

#if( AUDIO_CONVERTER_ALAC )
	#include "AppleLosslessDecoder.h"
	#include "BitUtilities.h"
#endif

//===========================================================================================================================
//	Internals
//===========================================================================================================================

typedef struct AudioConverterPrivate *		AudioConverterPrivateRef;
struct AudioConverterPrivate
{
#if( AUDIO_CONVERTER_ALAC )
	AppleLosslessDecoderRef		alacDecoder;
#endif
};

//===========================================================================================================================
//	AudioConverterNew_compat
//===========================================================================================================================

OSStatus
	AudioConverterNew_compat( 
		const AudioStreamBasicDescription *	inSourceFormat,
		const AudioStreamBasicDescription *	inDestinationFormat,
		AudioConverterRef *					outAudioConverter )
{
	OSStatus						err;
	AudioConverterPrivateRef		me;
	
#if( !AUDIO_CONVERTER_ALAC )	
	(void) inSourceFormat;
#endif
	
	me = (AudioConverterPrivateRef) calloc( 1, sizeof( *me ) );
	require_action( me, exit, err = kNoMemoryErr );
	
	if( 0 ) {}
#if( AUDIO_CONVERTER_ALAC )
	else if( inSourceFormat->mFormatID == kAudioFormatAppleLossless )
	{
		err = AppleLosslessDecoder_Create( &me->alacDecoder );
		require_noerr( err, exit );
	}
#endif
	else
	{
		err = kUnsupportedErr;
		goto exit;
	}
	
	require_action_quiet( inDestinationFormat->mFormatID == kAudioFormatLinearPCM, exit, err = kUnsupportedErr );
	
	*outAudioConverter = (AudioConverterRef) me;
	me = NULL;
	
exit:
	if( me ) AudioConverterDispose_compat( (AudioConverterRef) me );
	return( err );
}

//===========================================================================================================================
//	AudioConverterDispose_compat
//===========================================================================================================================

OSStatus	AudioConverterDispose_compat( AudioConverterRef inConverter )
{
	AudioConverterPrivateRef const		me = (AudioConverterPrivateRef) inConverter;
	
#if( AUDIO_CONVERTER_ALAC )
	if( me->alacDecoder )
	{
		AppleLosslessDecoder_Delete( me->alacDecoder );
		me->alacDecoder = NULL;
	}
#endif
	free( me );
	return( kNoErr );
}

//===========================================================================================================================
//	AudioConverterReset_compat
//===========================================================================================================================

OSStatus	AudioConverterReset_compat( AudioConverterRef me )
{
	(void) me;
	
	// Nothing to do here.
	
	return( kNoErr );
}

//===========================================================================================================================
//	AudioConverterSetProperty_compat
//===========================================================================================================================

OSStatus
	AudioConverterSetProperty_compat( 
		AudioConverterRef			inConverter,
		AudioConverterPropertyID	inPropertyID,
		UInt32						inSize,
		const void *				inData )
{
	AudioConverterPrivateRef const		me = (AudioConverterPrivateRef) inConverter;
	OSStatus							err;
	
#if( !AUDIO_CONVERTER_ALAC )
	(void) me;
	(void) inPropertyID;
	(void) inSize;
	(void) inData;
#endif
	
	if( 0 ) {}
#if( AUDIO_CONVERTER_ALAC )
	else if( inPropertyID == kAudioConverterDecompressionMagicCookie )
	{
		const ALACParams * const		alacParams = (const ALACParams *) inData;
		
		require_action( inSize == sizeof( *alacParams ), exit, err = kSizeErr );
		require_action( me->alacDecoder, exit, err = kIncompatibleErr );
		
		err = AppleLosslessDecoder_Configure( me->alacDecoder, alacParams );
		require_noerr( err, exit );
	}
#endif
	else
	{
		err = kUnsupportedErr;
		goto exit;
	}
	
exit:
	return( err );
}

//===========================================================================================================================
//	AudioConverterFillComplexBuffer_compat
//===========================================================================================================================

OSStatus
	AudioConverterFillComplexBuffer_compat( 
		AudioConverterRef					inConverter,
		AudioConverterComplexInputDataProc	inInputDataProc,
		void *								inInputDataProcUserData,
		UInt32 *							ioOutputDataPacketSize,
		AudioBufferList *					outOutputData,
		AudioStreamPacketDescription *		outPacketDescription )
{
#if( AUDIO_CONVERTER_ALAC )
	AudioConverterPrivateRef const		me = (AudioConverterPrivateRef) inConverter;
	OSStatus							err;
	BitBuffer							bits;
	AudioBufferList						bufferList;
	UInt32								packetCount;
	AudioStreamPacketDescription *		packetDesc;
	uint32_t							sampleCount;
	
	(void) outPacketDescription;
	
	packetCount = 1;
	packetDesc  = NULL;
	err = inInputDataProc( inConverter, &packetCount, &bufferList, &packetDesc, inInputDataProcUserData );
	require_noerr_quiet( err, exit );
	
	BitBufferInit( &bits, (uint8_t *) bufferList.mBuffers[ 0 ].mData, bufferList.mBuffers[ 0 ].mDataByteSize );
	sampleCount = *ioOutputDataPacketSize;
	err = AppleLosslessDecoder_Decode( me->alacDecoder, &bits, (uint8_t *) outOutputData->mBuffers[ 0 ].mData, 
		sampleCount, 2, &sampleCount );
	require_noerr_quiet( err, exit );
	
	*ioOutputDataPacketSize = sampleCount;
	
exit:
	return( err );
#else
	(void) inConverter;
	(void) inInputDataProc;
	(void) inInputDataProcUserData;
	(void) ioOutputDataPacketSize;
	(void) outOutputData;
	(void) outPacketDescription;
	
	return( kUnsupportedErr );
#endif
}

#endif // AUDIO_CONVERTER_LITE_ENABLED
