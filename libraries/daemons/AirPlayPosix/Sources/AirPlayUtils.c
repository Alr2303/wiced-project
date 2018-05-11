/*
	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
*/

#include <stdlib.h>
#include <string.h>

#include "AirPlayUtils.h"

#include "AirPlayCommon.h"
#include "CommonServices.h"
#include "DebugServices.h"

#include COREAUDIO_HEADER

//===========================================================================================================================
//	Internals
//===========================================================================================================================

#define RTPJitterBufferLock( CTX )			dispatch_semaphore_wait( (CTX)->nodeLock, DISPATCH_TIME_FOREVER )
#define RTPJitterBufferUnlock( CTX )		dispatch_semaphore_signal( (CTX)->nodeLock )

#define RTPJitterBufferBufferedSamples( CTX )	( (uint32_t)( \
	( ctx->busyList->prev->pkt.pkt.rtp.header.ts - ctx->busyList->next->pkt.pkt.rtp.header.ts ) + \
	( ctx->busyList->prev->pkt.len / ctx->bytesPerFrame ) ) )

#define RTPJitterBufferSamplesToMs( CTX, X )	( ( ( 1000 * (X) ) + ( (CTX)->sampleRate / 2 ) ) / (CTX)->sampleRate )
#define RTPJitterBufferMsToSamples( CTX, X )	( ( (X) * (CTX)->sampleRate ) / 1000 )
#define RTPJitterBufferBufferedMs( CTX )		RTPJitterBufferSamplesToMs( (CTX), RTPJitterBufferBufferedSamples( (CTX) ) )

static void	_RTPJitterBufferReleaseBusyNode( RTPJitterBufferContext *ctx, RTPPacketNode *inNode );

ulog_define( AirPlayJitterBuffer, kLogLevelNotice, kLogFlags_Default, "AirPlay", "AirPlayJitterBuffer:rate=1;1000" );
#define ap_jitter_ulog( LEVEL, ... )		ulog( &log_category_from_name( AirPlayJitterBuffer ), (LEVEL), __VA_ARGS__ )
#define ap_jitter_label( CTX )				( (CTX)->label ? (CTX)->label : "Default" )

//===========================================================================================================================
//	AirPlayAudioFormatToASBD
//===========================================================================================================================

OSStatus
	AirPlayAudioFormatToASBD( 
		AirPlayAudioFormat				inFormat, 
		AudioStreamBasicDescription *	outASBD, 
		uint32_t *						outBitsPerChannel )
{
	switch( inFormat )
	{
		case kAirPlayAudioFormat_PCM_8KHz_16Bit_Mono:		ASBD_FillPCM( outASBD,  8000, 16, 16, 1 ); break;
		case kAirPlayAudioFormat_PCM_8KHz_16Bit_Stereo:		ASBD_FillPCM( outASBD,  8000, 16, 16, 2 ); break;
		case kAirPlayAudioFormat_PCM_16KHz_16Bit_Mono:		ASBD_FillPCM( outASBD, 16000, 16, 16, 1 ); break;
		case kAirPlayAudioFormat_PCM_16KHz_16Bit_Stereo:	ASBD_FillPCM( outASBD, 16000, 16, 16, 2 ); break;
		case kAirPlayAudioFormat_PCM_24KHz_16Bit_Mono:		ASBD_FillPCM( outASBD, 24000, 16, 16, 1 ); break;
		case kAirPlayAudioFormat_PCM_24KHz_16Bit_Stereo:	ASBD_FillPCM( outASBD, 24000, 16, 16, 2 ); break;
		case kAirPlayAudioFormat_PCM_32KHz_16Bit_Mono:		ASBD_FillPCM( outASBD, 32000, 16, 16, 1 ); break;
		case kAirPlayAudioFormat_PCM_32KHz_16Bit_Stereo:	ASBD_FillPCM( outASBD, 32000, 16, 16, 2 ); break;
		case kAirPlayAudioFormat_PCM_44KHz_16Bit_Mono:		ASBD_FillPCM( outASBD, 44100, 16, 16, 1 ); break;
		case kAirPlayAudioFormat_PCM_44KHz_16Bit_Stereo:	ASBD_FillPCM( outASBD, 44100, 16, 16, 2 ); break;
		case kAirPlayAudioFormat_PCM_44KHz_24Bit_Mono:		ASBD_FillPCM( outASBD, 44100, 24, 24, 1 ); break;
		case kAirPlayAudioFormat_PCM_44KHz_24Bit_Stereo:	ASBD_FillPCM( outASBD, 44100, 24, 24, 2 ); break;
		case kAirPlayAudioFormat_PCM_48KHz_16Bit_Mono:		ASBD_FillPCM( outASBD, 48000, 16, 16, 1 ); break;
		case kAirPlayAudioFormat_PCM_48KHz_16Bit_Stereo:	ASBD_FillPCM( outASBD, 48000, 16, 16, 2 ); break;
		case kAirPlayAudioFormat_PCM_48KHz_24Bit_Mono:		ASBD_FillPCM( outASBD, 48000, 24, 24, 1 ); break;
		case kAirPlayAudioFormat_PCM_48KHz_24Bit_Stereo:	ASBD_FillPCM( outASBD, 48000, 24, 24, 2 ); break;
		case kAirPlayAudioFormat_ALAC_44KHz_16Bit_Stereo:	ASBD_FillALAC( outASBD, 44100, 16, 2 ); break;
		case kAirPlayAudioFormat_ALAC_44KHz_24Bit_Stereo:	ASBD_FillALAC( outASBD, 44100, 24, 2 ); break;
		case kAirPlayAudioFormat_ALAC_48KHz_16Bit_Stereo:	ASBD_FillALAC( outASBD, 48000, 16, 2 ); break;
		case kAirPlayAudioFormat_ALAC_48KHz_24Bit_Stereo:	ASBD_FillALAC( outASBD, 48000, 24, 2 ); break;
		default: dlogassert( "Bad format 0x%X", inFormat ); return( kUnsupportedErr );
	}
	if( outBitsPerChannel )
	{
		if( ( inFormat == kAirPlayAudioFormat_ALAC_44KHz_16Bit_Stereo )	||
			( inFormat == kAirPlayAudioFormat_ALAC_48KHz_16Bit_Stereo )	||
			( inFormat == kAirPlayAudioFormat_AAC_LC_44KHz_Stereo )		||
			( inFormat == kAirPlayAudioFormat_AAC_LC_48KHz_Stereo )		||
			( inFormat == kAirPlayAudioFormat_AAC_ELD_44KHz_Stereo )	||
			( inFormat == kAirPlayAudioFormat_AAC_ELD_48KHz_Stereo ) )
		{
			*outBitsPerChannel = 16;
		}
		else if( ( inFormat == kAirPlayAudioFormat_ALAC_44KHz_24Bit_Stereo ) ||
				 ( inFormat == kAirPlayAudioFormat_ALAC_48KHz_24Bit_Stereo ) )
		{
			
			*outBitsPerChannel = 24;
		}
		else
		{
			*outBitsPerChannel = outASBD->mBitsPerChannel;
		}
	}
	return( kNoErr );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	RTPJitterBufferInit
//===========================================================================================================================

OSStatus
	RTPJitterBufferInit( 
		RTPJitterBufferContext *ctx, 
		uint32_t				inSampleRate, 
		uint32_t				inBytesPerFrame, 
		uint32_t				inBufferMs )
{
	OSStatus		err;
	size_t			i, n;
	
	memset( ctx, 0, sizeof( *ctx ) );
	
	ctx->nodeLock = dispatch_semaphore_create( 1 );
	require_action( ctx->nodeLock, exit, err = kNoResourcesErr );
	
	n = 50; // ~400 ms at 352 samples per packet and 44100 Hz.
	ctx->packets = (RTPPacketNode *) malloc( n * sizeof( *ctx->packets ) );
	require_action( ctx->packets, exit, err = kNoMemoryErr );
	
	n -= 1;
	for( i = 0; i < n; ++i )
	{
		ctx->packets[ i ].next = &ctx->packets[ i + 1 ];
	}
	ctx->packets[ i ].next	= NULL;
	ctx->freeList			= ctx->packets;
	ctx->sentinel.prev		= &ctx->sentinel;
	ctx->sentinel.next		= &ctx->sentinel;
	ctx->busyList			= &ctx->sentinel;
	ctx->sampleRate			= inSampleRate;
	ctx->bytesPerFrame		= inBytesPerFrame;
	ctx->himark				= RTPJitterBufferMsToSamples( ctx, inBufferMs );
	ctx->buffering			= true;
	err = kNoErr;
	
exit:
	if( err ) RTPJitterBufferFree( ctx );
	return( err );
}

//===========================================================================================================================
//	RTPJitterBufferFree
//===========================================================================================================================

void	RTPJitterBufferFree( RTPJitterBufferContext *ctx )
{
	if( ( ctx->nLate > 0 ) || ( ctx->nGaps > 0 ) || ( ctx->nSkipped > 0 ) || ( ctx->nRebuffer > 0 ) )
	{
		ap_jitter_ulog( kLogLevelNotice | kLogLevelFlagDontRateLimit, 
			"### %s: Buffering issues during session: Late=%u Missing=%u Gaps=%u Rebuffers=%u\n", 
			ap_jitter_label( ctx ), ctx->nLate, ctx->nGaps, ctx->nSkipped, ctx->nRebuffer );
	}
	ctx->nLate		= 0;
	ctx->nGaps		= 0;
	ctx->nSkipped	= 0;
	ctx->nRebuffer	= 0;
	
	dispatch_forget( &ctx->nodeLock );
	ForgetMem( &ctx->packets );
}

//===========================================================================================================================
//	RTPJitterBufferReset
//===========================================================================================================================

void	RTPJitterBufferReset( RTPJitterBufferContext *ctx )
{
	RTPPacketNode *		stop;
	RTPPacketNode *		node;
	RTPPacketNode *		next;
	
	RTPJitterBufferLock( ctx );
	
	stop = ctx->busyList;
	for( node = stop->next; node != stop; node = next )
	{
		next = node->next;
		_RTPJitterBufferReleaseBusyNode( ctx, node );
	}
	ctx->started   = false;
	ctx->buffering = true;
	
	RTPJitterBufferUnlock( ctx );
}

//===========================================================================================================================
//	RTPJitterBufferGetFreeNode
//===========================================================================================================================

OSStatus	RTPJitterBufferGetFreeNode( RTPJitterBufferContext *ctx, RTPPacketNode **outNode )
{
	OSStatus			err;
	RTPPacketNode *		node;
	RTPPacketNode *		stop;
	
	RTPJitterBufferLock( ctx );
	node = ctx->freeList;
	if( node )
	{
		ctx->freeList = node->next;
	}
	else
	{
		// No free nodes so steal the oldest node.
		
		stop = ctx->busyList;
		node = stop->next;
		require_action( node != stop, exit, err = kInternalErr );
		node->next->prev = node->prev;
		node->prev->next = node->next;
	}
	*outNode = node;
	err = kNoErr;
	
exit:
	RTPJitterBufferUnlock( ctx );
	return( err );
}

//===========================================================================================================================
//	RTPJitterBufferPutFreeNode
//===========================================================================================================================

void	RTPJitterBufferPutFreeNode( RTPJitterBufferContext *ctx, RTPPacketNode *inNode )
{
	RTPJitterBufferLock( ctx );
	inNode->next = ctx->freeList;
	ctx->freeList = inNode;
	RTPJitterBufferUnlock( ctx );
}

//===========================================================================================================================
//	RTPJitterBufferPutBusyNode
//===========================================================================================================================

OSStatus	RTPJitterBufferPutBusyNode( RTPJitterBufferContext *ctx, RTPPacketNode *inNode )
{
	uint32_t const		ts = inNode->pkt.pkt.rtp.header.ts;
	OSStatus			err;
	RTPPacketNode *		stop;
	RTPPacketNode *		node;
	uint32_t			span;
	
	RTPJitterBufferLock( ctx );
	
	// Drop the node if we're not enabled.
	
	if( ctx->disabled )
	{
		inNode->next = ctx->freeList;
		ctx->freeList = inNode;
		err = kNoErr;
		goto exit;
	}
	
	// Insert the new node in timestamp order. It's most likely to be a later timestamp so start at the end.
	
	stop = ctx->busyList;
	for( node = stop->prev; ( node != stop ) && Mod32_GT( node->pkt.pkt.rtp.header.ts, ts ); node = node->prev ) {}
	require_action( ( node == stop ) || ( node->pkt.pkt.rtp.header.ts != ts ), exit, err = kDuplicateErr );
	
	inNode->prev		= node;
	inNode->next		= node->next;
	inNode->next->prev	= inNode;
	inNode->prev->next	= inNode;
	
	// If we've reached the high watermark then transition from buffering to playing.
	
	if( ctx->started )
	{
		if( ctx->buffering )
		{
			span = RTPJitterBufferBufferedSamples( ctx );
			if( span >= ctx->himark )
			{
				ctx->buffering = false;
				ap_jitter_ulog( kLogLevelVerbose | kLogLevelFlagDontRateLimit, 
					"%s: Buffering complete: %d ms buffered\n", 
					ap_jitter_label( ctx ), RTPJitterBufferSamplesToMs( ctx, span ) );
			}
		}
	}
	else
	{
		ctx->nextTS   = ts;
		ctx->started  = true;
	}
	err = kNoErr;
	
exit:
	RTPJitterBufferUnlock( ctx );
	return( err );
}

//===========================================================================================================================
//	_RTPJitterBufferReleaseBusyNode
//===========================================================================================================================

static void	_RTPJitterBufferReleaseBusyNode( RTPJitterBufferContext *ctx, RTPPacketNode *inNode )
{
	inNode->next->prev	= inNode->prev;
	inNode->prev->next	= inNode->next;
	inNode->next		= ctx->freeList;
	ctx->freeList		= inNode;
}

//===========================================================================================================================
//	RTPJitterBufferRead
//===========================================================================================================================

OSStatus	RTPJitterBufferRead( RTPJitterBufferContext *ctx, void *inBuffer, size_t inLen )
{
	uint32_t			nowTS = ctx->nextTS;
	uint32_t const		limTS = (uint32_t)( nowTS + ( inLen / ctx->bytesPerFrame ) );
	uint8_t *			dst   = (uint8_t *) inBuffer;
	RTPPacketNode *		stop;
	RTPPacketNode *		node;
	RTPPacketNode *		next;
	uint32_t			srcTS, endTS, delta;
	size_t				len;
	Boolean				cap;
	
	RTPJitterBufferLock( ctx );
	if( ctx->buffering )
	{
		memset( inBuffer, 0, inLen );
		goto exit;
	}
	
	stop = ctx->busyList;
	for( node = stop->next; node != stop; node = next )
	{
		srcTS = node->pkt.pkt.rtp.header.ts;
		if( Mod32_GE( srcTS, limTS ) ) break;
		
		// If the node is before the timing window, it's too late so go to the next one.
		
		endTS = (uint32_t)( srcTS + ( node->pkt.len / ctx->bytesPerFrame ) );
		if( Mod32_LE( endTS, nowTS ) )
		{
			++ctx->nLate;
			ap_jitter_ulog( kLogLevelNotice, "%s: Late: %d ms (%u total)\n", ap_jitter_label( ctx ), 
				RTPJitterBufferSamplesToMs( ctx, (int)( nowTS - endTS ) ), ctx->nLate );
			goto next;
		}
		
		// If the node has samples before the timing window, they're late so skip them.
		
		if( Mod32_LT( srcTS, nowTS ) )
		{
			++ctx->nSkipped;
			ap_jitter_ulog( kLogLevelNotice, "%s: Skip: %d ms (%u total)\n", ap_jitter_label( ctx ), 
				RTPJitterBufferSamplesToMs( ctx, (int)( nowTS - srcTS ) ), ctx->nSkipped );
			
			delta = nowTS - srcTS;
			len   = delta * ctx->bytesPerFrame;
			node->ptr += len;
			node->pkt.len -= len;
			node->pkt.pkt.rtp.header.ts += delta;
			srcTS = nowTS;
		}
		
		// If the node starts after the beginning of the timing window, there's a gap so fill in silence.
		
		else if( Mod32_GT( srcTS, nowTS ) )
		{
			++ctx->nGaps;
			ap_jitter_ulog( kLogLevelNotice, "%s: Gap:  %d ms (%u total)\n", ap_jitter_label( ctx ), 
				RTPJitterBufferSamplesToMs( ctx, (int)( srcTS - nowTS ) ), ctx->nGaps );
			
			delta = srcTS - nowTS;
			len   = delta * ctx->bytesPerFrame;
			memset( dst, 0, len );
			dst   += len;
			nowTS += delta;
		}
		
		// Copy into the playout buffer.
		
		cap = Mod32_GT( endTS, limTS );
		if( cap ) endTS = limTS;
		
		delta = endTS - srcTS;
		len   = delta * ctx->bytesPerFrame;
		memcpy( dst, node->ptr, len );
		dst   += len;
		nowTS += delta;
		if( cap )
		{
			node->ptr     += len;
			node->pkt.len -= len;
			node->pkt.pkt.rtp.header.ts	+= delta;
			break;
		}
		
	next:
		next = node->next;
		_RTPJitterBufferReleaseBusyNode( ctx, node );
	}
	
	// If more samples are needed for this timing window then we've run dry so fill in silence and start buffering.
	
	if( Mod32_LT( nowTS, limTS ) )
	{
		++ctx->nRebuffer;
		ap_jitter_ulog( kLogLevelNotice | kLogLevelFlagDontRateLimit, 
			"%s: Re-buffering: %d ms buffered, %d ms missing (%u total)\n", ap_jitter_label( ctx ), 
			RTPJitterBufferBufferedMs( ctx ), RTPJitterBufferSamplesToMs( ctx, (int)( limTS - nowTS ) ), ctx->nRebuffer );
		
		delta = limTS - nowTS;
		len = delta * ctx->bytesPerFrame;
		memset( dst, 0, len );
		nowTS = limTS;
		ctx->started = false;
		ctx->buffering = true;
	}
	ctx->nextTS = nowTS;
	
exit:
	RTPJitterBufferUnlock( ctx );
	return( kNoErr );
}
