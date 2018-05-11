/*
	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
*/

#ifndef	__AirPlayUtils_h_
#define	__AirPlayUtils_h_

#include "AirPlayCommon.h"

#include COREAUDIO_HEADER
#include LIBDISPATCH_HEADER

#ifdef __cplusplus
extern "C" {
#endif

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

OSStatus
	AirPlayAudioFormatToASBD( 
		AirPlayAudioFormat				inFormat, 
		AudioStreamBasicDescription *	outASBD, 
		uint32_t *						outBitsPerChannel );

//===========================================================================================================================
//	RTPJitterBuffer
//===========================================================================================================================

typedef struct RTPPacketNode	RTPPacketNode;
struct RTPPacketNode
{
	RTPPacketNode *		next;	// Next node in circular list when busy. Next node in singly linked list when free.
	RTPPacketNode *		prev;	// Note: only used when node is on the busy list (not needed for free nodes).
	RTPSavedPacket		pkt;	// Full RTP packet with header and payload.
	uint8_t *			ptr;	// Ptr to RTP payload. Note: this may not point to the beginning of the payload.
};

typedef struct
{
	dispatch_semaphore_t		nodeLock;		// Lock to protect node lists.
	RTPPacketNode *				packets;		// Backing store for all the packets.
	RTPPacketNode *				freeList;		// Singly-linked list of free nodes.
	RTPPacketNode *				busyList;		// Circular list of received packets, sorted by timestamp.
	RTPPacketNode				sentinel;		// Dummy head node to maintain a circular list without NULL checks.
	uint32_t					sampleRate;		// Number of samples per second (e.g. 44100).
	uint32_t					bytesPerFrame;	// Bytes per frame (e.g. 4 for 16-bit stereo samples).
	uint32_t					nextTS;			// Next timestamp to read from.
	uint32_t					himark;			// Number of samples needed before moving from buffering -> playing.
	Boolean						disabled;		// True if all input data should be dropped.
	Boolean						started;		// True if we've received at least one packet after initialization.
	Boolean						buffering;		// True if we're buffering until the high watermark is reached.
	uint32_t					nLate;			// Number of times samples were dropped because of being late.
	uint32_t					nGaps;			// Number of times samples that were missing (e.g. lost packet).
	uint32_t					nSkipped;		// Number of times we had to skip samples (before timing window).
	uint32_t					nRebuffer;		// Number of times we had to re-buffer because we ran dry.
	const char *				label;			// Optional label for logging.
	
}	RTPJitterBufferContext;

OSStatus
	RTPJitterBufferInit( 
		RTPJitterBufferContext *ctx, 
		uint32_t				inSampleRate, 
		uint32_t				inBytesPerFrame, 
		uint32_t				inBufferMs );
void		RTPJitterBufferFree( RTPJitterBufferContext *ctx );
void		RTPJitterBufferReset( RTPJitterBufferContext *ctx );
OSStatus	RTPJitterBufferGetFreeNode( RTPJitterBufferContext *ctx, RTPPacketNode **outNode );
void		RTPJitterBufferPutFreeNode( RTPJitterBufferContext *ctx, RTPPacketNode *inNode );
OSStatus	RTPJitterBufferPutBusyNode( RTPJitterBufferContext *ctx, RTPPacketNode *inNode );
OSStatus	RTPJitterBufferRead( RTPJitterBufferContext *ctx, void *inBuffer, size_t inLen );

#ifdef __cplusplus
}
#endif

#endif	// __AirPlayUtils_h_
