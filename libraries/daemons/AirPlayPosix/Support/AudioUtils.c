/*
	Copyright (C) 2010 Apple Inc. All Rights Reserved.
*/

#include "AudioUtils.h"

#include <math.h>
#include <stdlib.h>

#include "DebugServices.h"

//===========================================================================================================================
//	IsDTSEncodedData
//===========================================================================================================================

Boolean	IsDTSEncodedData( const void *inData, size_t inByteCount )
{
	const uint16_t *		p;
	const uint16_t *		e;
	
	p = (const uint16_t *) inData;
	e = p + ( inByteCount / 2 );
	for( ; ( e - p ) > 3; ++p )
	{
		if( ( p[ 0 ] == 0x7FFE ) && ( p[ 1 ] == 0x8001 ) )							return( true ); // Normal DTS Sync
		if( ( p[ 0 ] == 0x1FFF ) && ( p[ 1 ] == 0xE800 ) && ( p[ 2 ] == 0x07F1 ) )	return( true ); // 14-bit DTS Sync
	}
	return( false );
}

//===========================================================================================================================
//	IsSilencePCM
//===========================================================================================================================

Boolean	IsSilencePCM( const void *inData, size_t inByteCount )
{
	const int16_t *		src;
	const int16_t *		end;
	
	src = (const int16_t *) inData;
	end = src + ( inByteCount / 2 );
	for( ; src != end; ++src )
	{
		if( *src != 0 )
		{
			return( false );
		}
	}
	return( true );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	SineWave_Init
//===========================================================================================================================

OSStatus	SineTable_Create( SineTableRef *outTable, int inSampleRate, int inFrequency )
{
	OSStatus			err;
	SineTableRef		obj;
	double				scale;
	int					i;
	
	obj = (SineTableRef) malloc( offsetof( struct SineTable, values ) + ( inSampleRate * sizeof( int16_t ) ) );
	require_action( obj, exit, err = kNoMemoryErr );
	
	obj->sampleRate	= inSampleRate;
	obj->frequency	= inFrequency;
	obj->position	= 0;
	
	scale = 2.0 * 3.14159265358 / inSampleRate;
	for( i = 0; i < inSampleRate; ++i )
	{
		obj->values[ i ] = (int16_t)( 32767 * sin( ( (double) i ) * scale ) );
	}
	
	*outTable = obj;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	SineTable_Delete
//===========================================================================================================================

void	SineTable_Delete( SineTableRef inTable )
{
	free( inTable );
}

//===========================================================================================================================
//	SineWave_GetSamples
//===========================================================================================================================

void	SineTable_GetSamples( SineTableRef inTable, int inBalance, int inSampleCount, void *inSampleBuffer )
{
	const int16_t * const		values		= inTable->values;
	const int					sampleRate	= inTable->sampleRate;
	const int					frequency	= inTable->frequency;
	int							position;
	int16_t						sample;
	int16_t *					dst;
	int16_t *					lim;
	
	position = inTable->position;
	dst = (int16_t *) inSampleBuffer;
	lim = dst + ( inSampleCount * 2 );
	while( dst < lim )
	{
		sample = values[ position ];
		position += frequency;
		if( position  > sampleRate )
		{
			position -= sampleRate;
		}
		if( inBalance < 0 )
		{
			*dst++ = sample;
			*dst++ = 0;
		}
		else if( inBalance == 0 )
		{
			*dst++ = sample;
			*dst++ = sample;
		}
		else
		{
			*dst++ = 0;
			*dst++ = sample;
		}
	}
	inTable->position = position;
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	VolumeAdjusterInit
//===========================================================================================================================

void	VolumeAdjusterInit( VolumeAdjusterContext *ctx )
{
	ctx->currentVolume = kQ16_1pt0;
	ctx->targetVolume  = kQ16_1pt0;
}

//===========================================================================================================================
//	VolumeAdjusterSetVolumeDB
//===========================================================================================================================

void	VolumeAdjusterSetVolumeDB( VolumeAdjusterContext *ctx, Float32 inDB )
{
	ctx->targetVolume		= ( inDB != 0 ) ? ( (Q16x16)( kQ16_1pt0 * powf( 10, inDB / 20 ) ) ) : kQ16_1pt0;
	ctx->volumeIncrement	= ( ( ctx->targetVolume - ctx->currentVolume ) << 14 ) / 256; // Convert to Q2.30 format / 256.
	ctx->rampStepsRemaining	= 256;
}

//===========================================================================================================================
//	VolumeAdjusterApply
//===========================================================================================================================

void	VolumeAdjusterApply( VolumeAdjusterContext *ctx, int16_t *inSamples, size_t inCount, size_t inChannels )
{
	int16_t *		dst = inSamples;
	Q16x16			scale;
	
	if( ctx->currentVolume != ctx->targetVolume )
	{
		int32_t		ramp;
		size_t		i, n, ii;
		
		ramp = ctx->currentVolume << 14; // Convert Q16.16 to Q2.30 format.
		n = Min( inCount, ctx->rampStepsRemaining );
		for( i = 0; i < n; ++i )
		{
			ramp += ctx->volumeIncrement;
			scale = ramp >> 14; // Convert back to Q16.16 format.
			for( ii = 0; ii < inChannels; ++ii )
			{
				*dst = (int16_t)( ( ( *dst * scale ) + kQ16_0pt5 ) >> 16 );
				++dst;
			}
		}
		ctx->rampStepsRemaining -= n;
		if( ctx->rampStepsRemaining == 0 )
		{
			ctx->currentVolume = ctx->targetVolume;
		}
	}
	if( ctx->currentVolume == 0 )
	{
		const int16_t * const		lim = inSamples + ( inCount * inChannels );
		
		while( dst < lim )
		{
			*dst++ = 0;
		}
	}
	else if( ctx->currentVolume != kQ16_1pt0 )
	{
		const int16_t * const		lim = inSamples + ( inCount * inChannels );
		
		scale = ctx->currentVolume;
		for( ; dst < lim; ++dst )
		{
			*dst = (int16_t)( ( ( *dst * scale ) + kQ16_0pt5 ) >> 16 );
		}
	}
}
