/*
	Copyright (C) 2004-2012 Apple Inc. All Rights Reserved.
*/

#ifndef	__AppleLosslessDecoder_h__
#define	__AppleLosslessDecoder_h__

#include "CommonServices.h"

#ifdef	__cplusplus
	extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		AppleLosslessDecoder
	@abstract	API to decode audio using AppleLossless.
*/

typedef struct AppleLosslessDecoder *		AppleLosslessDecoderRef;
struct BitBuffer;

OSStatus	AppleLosslessDecoder_Create( AppleLosslessDecoderRef *outDecoder );
void		AppleLosslessDecoder_Delete( AppleLosslessDecoderRef inDecoder );

OSStatus	AppleLosslessDecoder_Configure( AppleLosslessDecoderRef inDecoder, const ALACParams *inParams );
OSStatus
	AppleLosslessDecoder_Decode( 
		AppleLosslessDecoderRef	inDecoder, 
		struct BitBuffer *		inBits, 
		uint8_t *				sampleBuffer, 
		uint32_t				numSamples, 
		uint32_t				numChannels, 
		uint32_t *				outNumSamples );

#ifdef	__cplusplus
	}
#endif

#endif	// __AppleLosslessDecoder_h__
