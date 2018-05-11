/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/
/*!
    @header AirPlay Session Platform APIs
    @discussion Platform APIs related to AirPlay Session.
*/

#ifndef	__AirPlayReceiverSession_h__
#define	__AirPlayReceiverSession_h__

#include "AESUtils.h"
#include "AirPlayCommon.h"
#include "AirPlayReceiverServer.h"
#include "AirPlayUtils.h"
#include "AirTunesClock.h"
#include "CommonServices.h"
#include "DataBufferUtils.h"
#include "DebugServices.h"
#include "dns_sd.h"
#include "HTTPClient.h"
#include "MathUtils.h"
#include "MiscUtils.h"
#include "NetUtils.h"
#include "PIDUtils.h"
#include "ThreadUtils.h"

	#include <sys/types.h>
	
	#include <sys/socket.h>
	#include <net/if.h>

#include AUDIO_CONVERTER_HEADER
#include CF_HEADER
#include CF_RUNTIME_HEADER
#include LIBDISPATCH_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#pragma mark == Types ==
#endif

//===========================================================================================================================
//	Types
//===========================================================================================================================

#define kAirTunesDupWindowSize		512 // Size of the dup checking window. Must be >= the max number of retransmits.

// AirPlayRTPBuffer

typedef struct AirPlayRTPBuffer *	AirPlayRTPBufferRef;
struct AirPlayRTPBuffer
{
	AirPlayRTPBufferRef		next;				// PRIVATE: Ptr to the next buffer in the list.
	void *					internalContext;	// PRIVATE: Ptr to context used by the AirPlay library.
	int32_t					retainCount;		// PRIVATE: Number of references to this buffer.
	uint32_t				state;				// State variable app code can use.
	uint8_t *				readPtr;			// Ptr to the buffer we're currently read into.
	size_t					readLen;			// Number of bytes to read in the current state.
	size_t					readOffset;			// Buffer offset where we're currently reading.
	uint8_t					extraBuf[ 256 ];	// Header for any frame wrapping the RTP packet (e.g. RTP-over-TCP).
	RTPHeader				rtpHeader;			// RTP header.
	uint8_t *				payloadPtr;			// READ ONLY: malloc'd ptr for payload data.
	size_t					payloadLen;			// Number of valid bytes in payloadPtr.
	size_t					payloadMaxLen;		// PRIVATE: Max number of bytes payloadPtr can hold.
	void *					userContext;		// Context pointer for app use.
};

// AirTunesBufferNode

typedef struct AirTunesBufferNode	AirTunesBufferNode;
struct AirTunesBufferNode
{
	AirTunesBufferNode *		next;	// Next node in circular list when busy. Next node in null-terminated list when free.
	AirTunesBufferNode *		prev;	// Note: only used when node is on the busy list (not needed for free nodes).
	const RTPPacket *			rtp;	// Ptr to RTP packet header.
	uint8_t *					ptr;	// Ptr to RTP payload. Note: this may not point to the beginning of the payload.
	size_t						size;	// Size of RTP payload. Note: this may be less than the full payload size.
	uint8_t *					data;	// Buffer for the entire RTP packet. All node ptrs point within this buffer.
	uint32_t					ts;		// RTP timestamp where "ptr" points. Updated when processing partial packets.
};

// AirTunesRetransmitNode

typedef struct AirTunesRetransmitNode	AirTunesRetransmitNode;
struct AirTunesRetransmitNode
{
	AirTunesRetransmitNode *		next;
	uint16_t						seq;			// Sequence number of the packet that needs to be retransmitted.
	uint16_t						tries;			// Number of times this retransmit request has been tried (starts at 1).
	uint64_t						startNanos;		// When the retransmit request started being tracked.
	uint64_t						sentNanos;		// When the retransmit request was last sent.
	uint64_t						nextNanos;		// When the retransmit request should be sent next.
#if( DEBUG )
	uint32_t						tryNanos[ 16 ];	// Age at each retransmit try.
#endif
};

// AirTunesRetransmitHistoryNode

#if( DEBUG )
typedef struct
{
	const char *		reason;
	uint16_t			seq;
	uint16_t			tries;
	uint64_t			finalNanos;
	uint32_t			tryNanos[ countof_field( AirTunesRetransmitNode, tryNanos ) ];
	
}	AirTunesRetransmitHistoryNode;
#endif

// AirTunesSource

typedef struct AirTunesSource	AirTunesSource;
struct AirTunesSource
{
	// Periodic operations
	
	unsigned int				receiveCount;					// Number of packets received (and passed some validity checks).
	unsigned int				lastReceiveCount;				// Receive count at our last activity check.
	uint64_t					lastActivityTicks;				// Ticks when we last detected activity.
	uint64_t					maxIdleTicks;					// Number of ticks we can be idle before timing out.
	uint64_t					perSecTicks;					// Number of ticks in a second.
	uint64_t					perSecLastTicks;				// Ticks when we did "per second" processing.
	uint64_t					lastIdleLogTicks;				// Ticks when we last logged about being idle.
	uint64_t					idleLogIntervalTicks;			// Number of ticks between idle logs.
	
	// Time Sync
	
	uint32_t					rtcpTILastTransmitTimeHi;		// Upper 32 bits of transmit time of last RTCP TI request.
	uint32_t					rtcpTILastTransmitTimeLo;		// Lower 32 bits of transmit time of last RTCP TI request.	
	unsigned int				rtcpTISendCount;				// Number of RTCP TI requests we've sent.
	unsigned int				rtcpTIResponseCount;			// Number of RTCP TI responses to our requests we've received.
	unsigned int				rtcpTIAnnounceCount;			// Number of RTCP TI announcements we've received.
	unsigned int				rtcpTIStepCount;				// Number of times the clock had to be stepped.
	
	double						rtcpTIClockRTTAvg;				// Avg round-trip time.
	double						rtcpTIClockRTTMin;				// Min round-trip time.
	double						rtcpTIClockRTTMax;				// Max round-trip time.
	double						rtcpTIClockRTTLeastBad;			// Best RTT we've received that was still considered "bad".
	int							rtcpTIClockRTTBadCount;			// Number of consecutive bad RTT's we've received.
	uint32_t					rtcpTIClockRTTOutliers;			// Total number of huge RTT's we've received.
	
	double						rtcpTIClockOffsetArray[ 3 ];	// Last N NTP clock offsets between us and the server.
	unsigned int				rtcpTIClockOffsetIndex;			// Circular index into the clock offset history array.
	double						rtcpTIClockOffsetAvg;			// Moving average of clock offsets.
	double						rtcpTIClockOffsetMin;			// Minimum clock offset.
	double						rtcpTIClockOffsetMax;			// Maximum clock offset.
	double						rtcpTIClockOffsetLeastBad;		// Best clock offset we've received that was still considered "bad".
	Boolean						rtcpTIResetStats;				// True if the clock offset stats need to be reset.
	
	// NTP<->RTP Timing
	
	Boolean						timeAnnouncePending;			// True if a TimeSync announcement needs to be processed.
	uint8_t						timeAnnounceV_P_M;				// Version (V), Padding (P), and Marker (M) fields from packet.
	uint32_t					timeAnnounceRTPTime;			// rtpTime field from packet.
	uint32_t					timeAnnounceNTPTimeHi;			// ntpTimeHi field from packet.
	uint32_t					timeAnnounceNTPTimeLo;			// ntpTimeLo field from packet.
	uint32_t					timeAnnounceRTPApply;			// rtpApply field from packet.
	
	uint32_t					rtpOffsetActive;				// RTP offset actively in use (different when deferring).
	uint32_t					rtpOffset;						// Current RTP timestamp offset between us and the server.
	Boolean						rtpOffsetApply;					// True when we need to apply a new RTP offset.
	uint32_t					rtpOffsetApplyTimestamp;		// Sender timestamp when we should apply our new RTP offset.
	uint32_t					rtpOffsetLast;					// Last RTP timestamp offset between us and the server.
	uint32_t					rtpOffsetAvg;					// Avg RTP timestamp offset between us and the server.
	uint32_t					rtpOffsetMaxDelta;				// Max delta between the last and instantaneous RTP offsets.
	uint32_t					rtpOffsetMaxSkew;				// Max delta between the last and current RTP timestamp offsets.
	uint32_t					rtpOffsetSkewResetCount;		// Number of timeline resets due to too much skew.
	
	// RTP Offset Skew
	
	Boolean						rtpSkewPlatformAdjust;			// True if the platform layer wants to do its own skew compensation.
	Boolean						rtpSkewAdjusting;				// True if currently adjusting for skew.
	PIDContext					rtpSkewPIDController;			// PID controller for doing skew compensation.
	int							rtpSkewAdjust;					// Current amount of skew we're adjusting/disciplined for.
	unsigned int				rtpSkewAdjustIndex;				// Current sample index used to know when to insert/drop a sample.
	unsigned int				rtpSkewSamplesPerAdjust;		// Number of samples before each insert/drop to adjust for skew.
	
	// RTCP Retransmissions
	
	AirTunesRetransmitNode *	rtcpRTListStorage;				// Storage for list of pending retransmit requests.
	AirTunesRetransmitNode *	rtcpRTFreeList;					// Head of list of free retransmit nodes.
	AirTunesRetransmitNode *	rtcpRTBusyList;					// Head of list of outstanding retransmit requests.
	Boolean						rtcpRTDisable;					// If true, don't send any retransmits.
	int64_t						rtcpRTMinRTTNanos;				// Smallest RTT we've seen.
	int64_t						rtcpRTMaxRTTNanos;				// Largest RTT we've seen.
	int64_t						rtcpRTAvgRTTNanos;				// Moving average RTT.
	int64_t						rtcpRTDevRTTNanos;				// Mean deviation RTT.
	int64_t						rtcpRTTimeoutNanos;				// Current retransmit timeout.
	uint32_t					retransmitSendCount;			// Number of retransmit requests we sent.
	uint32_t					retransmitReceiveCount;			// Number of retransmit responses we sent.
	uint32_t					retransmitFutileCount;			// Number of futile retransmit responses we've received.
	uint32_t					retransmitNotFoundCount;		// Number of retransmit responses received without a request (late).
	uint64_t					retransmitMinNanos;				// Min nanoseconds for a retransmit response.
	uint64_t					retransmitMaxNanos;				// Max nanoseconds for a retransmit response.
	uint64_t					retransmitAvgNanos;				// Average nanoseconds for a retransmit response.
	uint64_t					retransmitRetryMinNanos;		// Min milliseconds for a retransmit response after a retry.
	uint64_t					retransmitRetryMaxNanos;		// Max milliseconds for a retransmit response after a retry.
	uint32_t					maxBurstLoss;					// Largest contiguous number of lost packets.
	uint32_t					bigLossCount;					// Number of times we abort retransmits requests because the loss was too big.
};

// AirPlayAudioStreamContext

typedef struct
{
	uint64_t	hostTime;
	uint32_t	sampleTime;
	
}	AirPlayTimestampTuple;

typedef struct
{
	AirPlayReceiverSessionRef		session;					// Session owning this stream.
	AirPlayStreamType				type;						// Type of stream.
	const char *					label;						// Name of the stream (for logging).
	
	SocketRef						cmdSock;					// Socket for sending commands to the audio thread.
	SocketRef						dataSock;					// Socket for sending main audio input and receiving main output packets.
	pthread_t						thread;						// Thread for receiving and processing packets.
	pthread_t *						threadPtr;					// Ptr to the packet thread. NULL if thread isn't running.
	RTPJitterBufferContext			jitterBuffer;				// Buffer for processing packets.
	uint32_t						sampleRate;					// Sample rate for configured input and output (e.g. 44100).
	uint32_t						channels;					// Number channels (e.g. 2 for stereo).
	uint32_t						bytesPerUnit;				// Number of bytes per unit (e.g. 4 for 16-bit stereo).
	uint32_t						bitsPerSample;				// Number of bits in each sample (e.g. 16 for 16-bit samples).
	uint64_t						rateUpdateNextTicks;		// Next UpTicks when we should we should update the rate estimate.
	uint64_t						rateUpdateIntervalTicks;	// Delay between rate updates.
	AirPlayTimestampTuple			rateUpdateSamples[ 30 ];	// Sample history for rate estimates.
	uint32_t						rateUpdateCount;			// Total number of samples we've recorded for rate updates.
	Float32							rateAvg;					// Moving average of the estimated sample rate.
	uint32_t						sendErrors;					// Number of send errors that occurred.
	
}	AirPlayAudioStreamContext;

// AirPlayReceiverSession

struct AirPlayReceiverSessionPrivate
{
	CFRuntimeBase					base;						// CF type info. Must be first.
	AirPlayReceiverServerRef		server;						// Pointer to the server that owns this session.
	void *							platformPtr;				// Pointer to the platform-specific data.
	pthread_mutex_t					mutex;
	pthread_mutex_t *				mutexPtr;
	dispatch_source_t				periodicTimer;				// Timer for periodic tasks.
	
	NetTransportType				transportType;				// Network transport type for the session.
	sockaddr_ip						peerAddr;					// Address of the sender.
	uint8_t							sessionUUID[ 16 ];			// Random UUID for this AirPlay session.
	OSStatus						startStatus;				// Status of starting the session (i.e. if session failed).
	uint64_t						clientDeviceID;				// Unique device ID of the client sending to us.
	uint64_t						clientSessionID;			// Unique session ID from the client.
	uint64_t						sessionTicks;				// Ticks when this session was started.
	uint64_t						playTicks;					// Ticks when playback started.
	AirPlayCompressionType			compressionType;
	AES_CBCFrame_Context			decryptorStorage;			// Used for decrypting audio content.
	AES_CBCFrame_Context *			decryptor;					// Ptr to decryptor or NULL if content is not encrypted.
	AirTunesSource					source;
	Boolean							screen;						// True if AirPlay Screen. False if normal AirPlay Audio.
	uint32_t						samplesPerFrame;
	uint8_t *						decodeBuffer;
	size_t							decodeBufferSize;
	uint8_t *						readBuffer;
	size_t							readBufferSize;
	uint8_t *						skewAdjustBuffer;			// Temporary buffer for doing skew compensation.
	size_t							skewAdjustBufferSize;
	int								audioQoS;					// QoS to use for audio control and data.
	
	// Control/Events
	
	Boolean							controlSetup;				// True if control is set up.
	Boolean							useEvents;					// True if the client supports events.
	HTTPClientRef					eventClient;				// Client for sending RTSP events back to the sender.
	SocketRef						eventSock;					// Socket for accepting an RTSP event connection from the sender.
	int								eventPort;					// Port we're listening on for an RTSP event connection.
	
	// Main/AltAudio
	
	AirPlayAudioStreamContext		mainAudioCtx;				// Context for main audio input and output.
	AirPlayAudioStreamContext		altAudioCtx;				// Context for alt audio output.
	
	// LegacyAudio
	
	SocketRef						rtpAudioSock;				// Socket for receiving RTP audio packets.
	int								rtpAudioPort;				// Port we're listening on for RTP audio packets.
	SocketRef						rtpAudioCmdSock;			// Socket for sending commands to the RTP audio thread.
	pthread_t						rtpAudioThread;				// Thread for receiving and processing RTP packets.
	pthread_t *						rtpAudioThreadPtr;			// Ptr to RTP thread. NULL if thread isn't running.
	int								redundantAudio;				// If > 0, redundant audio packets are being sent.
	Boolean							rtpAudioDupsInitialized;	// True if the dup checker has been initialized.
	uint16_t						rtpAudioDupsLastSeq;		// Last valid sequence number we've checked.
	uint16_t						rtpAudioDupsArray[ kAirTunesDupWindowSize ]; // Sequence numbers for duplicate checking.
	
	SocketRef						rtcpSock;					// Socket for sending and receiving RTCP packets.
	int								rtcpPortLocal;				// Port we're listening on for RTCP packets.
	int								rtcpPortRemote;				// Port of the peer sending us RTCP packets.
	sockaddr_ip						rtcpRemoteAddr;				// Address of the peer to send RTCP packets to.
	socklen_t						rtcpRemoteLen;				// Length of the sockaddr for the RTCP peer.
	Boolean							rtcpConnected;				// True if the RTCP socket is connected.
	
	// NTP Time Sync
	
	SocketRef						timingSock;
	int								timingPortLocal;			// Local port we listen for time sync response packets on.
	int								timingPortRemote;			// Remote port we send time sync requests to.
	sockaddr_ip						timingRemoteAddr;			// Address of the peer to send timing packets to.
	socklen_t						timingRemoteLen;			// Length of the sockaddr for the timing peer.
	Boolean							timingConnected;			// True if the timing socket is connected.
	SocketRef						timingCmdSock;
	pthread_t						timingThread;
	pthread_t *						timingThreadPtr;
	
	// Buffering
	
	uint32_t						nodeCount;					// Number of buffer nodes.
	size_t							nodeBufferSize;				// Number of bytes of data each node can hold.
	uint8_t *						nodeBufferStorage;			// Backing store for all the node buffers.
	AirTunesBufferNode *			nodeHeaderStorage;			// Array backing store for all the node headers.
	AirTunesBufferNode				busyListSentinelStorage;	// Dummy head node to maintain a circular list with a sentinel.
	AirTunesBufferNode *			busyListSentinel;			// Ptr to dummy node...saves address-of calculations.
	AirTunesBufferNode *			freeList;					// List of free nodes. Standard head pointer, null tail list.
	uint32_t						busyNodeCount;				// Number of nodes currently in use.
	uint32_t						busyByteCount;				// Number of bytes currently in-use.
	uint32_t						totalBufferFrameCount;		// Total number of frames we can buffer. 1 frame is 1L+1R samples.
	
	// Audio
	
	uint32_t						audioLatencyOffset;			// Timestamp offset to compensate for latency.
	uint32_t						platformAudioLatency;		// Samples of latency the platform introduces.
	uint32_t						minLatency;					// Minimum samples of latency (for higher latency peers).
	uint32_t						maxLatency;					// Maximum samples of latency.
	Boolean							flushing;					// Flushing is in progress.
	uint64_t						flushRecentTicks;			// Number of ticks to consider a flush "recent".
	uint64_t						flushLastTicks;				// Ticks when the last flush occurred.
	uint32_t						flushTimeoutTS;				// Stay in flush mode until this timestamp is received.
	uint32_t						flushUntilTS;				// Flush packets with timestamps earlier than this.
	uint16_t						lastRTPSeq;					// Last RTP sequence number received.
	uint32_t						lastRTPTS;					// Last RTP timestamp received.
	Boolean							lastPlayedValid;			// true if lastPlayed info is valid.
	uint32_t						lastPlayedTS;				// Last played RTP timestamp (sender/packet timeline).
	uint16_t						lastPlayedSeq;				// Last played RTP sequence number.
	
	AudioConverterRef				audioConverter;				// Audio converter for decoding packets.
	const uint8_t *					encodedDataPtr;				// Ptr to the data to decode from converter callback.
	const uint8_t *					encodedDataEnd;				// End of the data to decode.
	AudioStreamPacketDescription	encodedPacketDesc;			// Used for passing packet info back to the converter.
	uint32_t						compressionPercentAvg;		// EWMA of the compression percent we're getting.
	
	Boolean							stutterCreditPending;		// True if the next normal glitch will be ignored.
	int								glitchTotal;				// Total number of glitches in the session.
	int								glitchLast;					// Number of glitches when we last checked.
	int								glitchyPeriods;				// Number of periods with glitches.
	int								glitchTotalPeriods;			// Number of periods (with or without glitches).
	uint64_t						glitchNextTicks;			// Next ticks to check the glitch counter.
	uint64_t						glitchIntervalTicks;		// Number of ticks between glitch counter checks.
	
#if( AIRPLAY_META_DATA )
	// Meta Data
	
	CFMutableDictionaryRef			pendingMetaData;			// Meta data waiting to be applied.
	Boolean							pendingArtwork;				// True if artwork is pending.
	Boolean							pendingText;				// True if text meta data is pending.
	uint32_t						metaDataApplyTS;			// RTP timestamp when the meta data should be applied.
	Boolean							metaDataApplied;			// True if we've ever applied meta data.
	
	Boolean							progressApplied;			// true if progress has ever been applied for this session.
	Boolean							progressValid;				// true if progress information is valid.
	uint32_t						progressStartTS;			// RTP timestamp when song started (may be in the past).
	uint32_t						progressCurrentTS;			// RTP timestamp where the song is currently.
	uint32_t						progressStopTS;				// RTP timestamp when the song will stop.
	
	Boolean							progressNextValid;			// true if progress information for the next song is valid.
	uint32_t						progressNextStartTS;		// RTP timestamp when the next song starts (may be in the past).
	uint32_t						progressNextCurrentTS;		// RTP timestamp where the next song is currently.
	uint32_t						progressNextStopTS;			// RTP timestamp when the next song will stop.
#endif
	
};

#if 0
#pragma mark -
#pragma mark == Creation ==
#endif

typedef struct
{
	AirPlayReceiverServerRef		server;				// Server managing the session.
	NetTransportType				transportType;		// Network transport type for the session.
	const sockaddr_ip *				peerAddr;			// sockaddr of the server sending us audio.
	uint64_t						clientDeviceID;		// Unique device ID of the client sending to us.
	uint64_t						clientSessionID;	// Unique session ID from the client.
	Boolean							useEvents;			// True if the client supports events.
	
}	AirPlayReceiverSessionCreateParams;

CFTypeID	AirPlayReceiverSessionGetTypeID( void );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionCreate
	@internal
	@abstract	Creates an AirPlay receiver session.
*/
OSStatus
	AirPlayReceiverSessionCreate( 
		AirPlayReceiverSessionRef *					outSession, 
		const AirPlayReceiverSessionCreateParams *	inParams );

OSStatus
	AirPlayReceiverSessionSetSecurityInfo( 
		AirPlayReceiverSessionRef	inSession, 
		const uint8_t				inKey[ 16 ], 
		const uint8_t				inIV[ 16 ] );

#if 0
#pragma mark -
#pragma mark == Control ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionControl
	@internal
	@abstract	Controls the session.
*/
OSStatus
	AirPlayReceiverSessionControl( 
		CFTypeRef			inSession, // Must be a AirPlayReceiverSessionRef.
		uint32_t			inFlags, 
		CFStringRef			inCommand, 
		CFTypeRef			inQualifier, 
		CFDictionaryRef		inParams, 
		CFDictionaryRef *	outParams );

// Convenience accessors.

#define AirPlayReceiverSessionControlF( SESSION, COMMAND, QUALIFIER, OUT_PARAMS, FORMAT, ... ) \
	CFObjectControlSyncF( (SESSION), NULL, AirPlayReceiverSessionControl, kCFObjectFlagDirect, \
		(COMMAND), (QUALIFIER), (OUT_PARAMS), (FORMAT), __VA_ARGS__ )

#define AirPlayReceiverSessionControlV( SESSION, COMMAND, QUALIFIER, OUT_PARAMS, FORMAT, ARGS ) \
	CFObjectControlSyncF( (SESSION), NULL, AirPlayReceiverSessionControl, kCFObjectFlagDirect, \
		(COMMAND), (QUALIFIER), (OUT_PARAMS), (FORMAT), (ARGS) )

#if 0
#pragma mark -
#pragma mark == Properties ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionCopyProperty
	@internal
	@abstract	Copies a property from the session.
*/
CF_RETURNS_RETAINED
CFTypeRef
	AirPlayReceiverSessionCopyProperty( 
		CFTypeRef	inSession, // Must be AirPlayReceiverSessionRef.
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		OSStatus *	outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionSetProperty
	@internal
	@abstract	Sets a property on the session.
*/
OSStatus
	AirPlayReceiverSessionSetProperty( 
		CFTypeRef	inSession, // Must be AirPlayReceiverSessionRef.
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		CFTypeRef	inValue );

// Convenience accessors.

#define AirPlayReceiverSessionSetBoolean( SESSION, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyBoolean( (SESSION), NULL, AirPlayReceiverSessionSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverSessionSetCString( SESSION, PROPERTY, QUALIFIER, STR, LEN ) \
	CFObjectSetPropertyCString( (SESSION), NULL, AirPlayReceiverSessionSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (STR), (LEN) )

#define AirPlayReceiverSessionSetData( SESSION, PROPERTY, QUALIFIER, PTR, LEN ) \
	CFObjectSetPropertyData( (SESSION), NULL, AirPlayReceiverSessionSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (PTR), (LEN) )

#define AirPlayReceiverSessionSetDouble( SESSION, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyDouble( (SESSION), NULL, AirPlayReceiverSessionSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverSessionSetInt64( SESSION, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyInt64( (SESSION), NULL, AirPlayReceiverSessionSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverSessionSetPropertyF( SESSION, PROPERTY, QUALIFIER, FORMAT, ... ) \
	CFObjectSetPropertyF( (SESSION), NULL, AirPlayReceiverSessionSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (FORMAT), __VA_ARGS__ )

#define AirPlayReceiverSessionSetPropertyV( SESSION, PROPERTY, QUALIFIER, FORMAT, ARGS ) \
	CFObjectSetPropertyV( (SESSION), NULL, AirPlayReceiverSessionSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (FORMAT), (ARGS) )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionSetup
	@internal
	@abstract	Sets up an AirPlay receiver session.
	
	@params		inRequestParams		Input parameters used to configure or update the session.
	@param		outResponseParams	Output parameters to return to the client so it can configure/update its side of the session.
*/
OSStatus
	AirPlayReceiverSessionSetup( 
		AirPlayReceiverSessionRef	inSession, 
		CFDictionaryRef				inRequestParams, 
		CFDictionaryRef *			outResponseParams );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionTearDown
	@internal
	@abstract	Sets up an AirPlay receiver session.
*/
void	AirPlayReceiverSessionTearDown( AirPlayReceiverSessionRef inSession, OSStatus inReason );

typedef struct
{
	const char *			clientName;
	NetTransportType		transportType;
	uint32_t				bonjourMs;
	uint32_t				connectMs;
	uint32_t				authMs;
	uint32_t				announceMs;
	uint32_t				setupAudioMs;
	uint32_t				setupScreenMs;
	uint32_t				recordMs;
	
}	AirPlayReceiverSessionStartInfo;

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionStart
	@internal
	@abstract	Starts the session after it has been set up.
*/
OSStatus	AirPlayReceiverSessionStart( AirPlayReceiverSessionRef inSession, AirPlayReceiverSessionStartInfo *inInfo );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionSendCommand
	@internal
	@abstract	Sends a command to the controller and calls a completion function with the response.
*/
typedef void ( *AirPlayReceiverSessionCommandCompletionFunc )( OSStatus inStatus, CFDictionaryRef inResponse, void *inContext );

OSStatus
	AirPlayReceiverSessionSendCommand( 
		AirPlayReceiverSessionRef					inSession, 
		CFDictionaryRef								inRequest, 
		AirPlayReceiverSessionCommandCompletionFunc	inCompletion, 
		void *										inContext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionFlushAudio
	@internal
	@abstract	Flush any queued audio until the specified timestamp or sequence number.
*/
OSStatus
	AirPlayReceiverSessionFlushAudio( 
		AirPlayReceiverSessionRef	inSession, 
		uint32_t					inFlushUntilTS, 
		uint16_t					inFlushUntilSeq, 
		uint32_t *					outLastTS );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionReadAudio
	@abstract	Reads output audio (e.g. music to play on the accessory) that has been received by the accessory.
	
	@param		inSession		Session to read audio from.
	@param		inType			Audio stream to read from.
	@param		inSampleTime	Audio sample time for the first sample to be placed in the buffer.
	@param		inHostTime		Wall clock time for when the first sample in the buffer should be heard.
	@param		inBuffer		Buffer to receive the audio samples. The format is configured during stream setup.
	@param		inLen			Number of bytes to fill into the buffer.
	
	@discussion
	
	The flow of operation is that audio sent to the accessory for playback is received, de-packetized, decrypted, decoded, 
	and buffered according to timing information in the stream. When the platform's audio stack needs the next chunk of 
	audio to play, it calls this function to read the audio that has been buffered for the specified timestamp. If there 
	isn't enough audio buffered to satisfy this request, the missing data will be filled in with a best guess or silence. 
	The platform can then provide this audio data to its audio stack for playback.
*/
OSStatus
	AirPlayReceiverSessionReadAudio( 
		AirPlayReceiverSessionRef	inSession, 
		AirPlayStreamType			inType, 
		uint32_t					inSampleTime, 
		uint64_t					inHostTime, 
		void *						inBuffer, 
		size_t						inLen );

#if 0
#pragma mark -
#pragma mark == Platform ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionPlatformInitialize
	@abstract	Initializes the platform-specific aspects of the session. Called once per session after it is created.

	@param	inSession	Session Identifier for the AirPlay Session being initialized
	@result	kNoErr if successful or an error code indicating failure.

	@discussion
	
	This gives the platform a chance to set up any per-session state.
	AirPlayReceiverSessionPlatformFinalize will be called when the session ends.
*/
OSStatus	AirPlayReceiverSessionPlatformInitialize( AirPlayReceiverSessionRef	inSession );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionPlatformFinalize
	@abstract	Finalizes the platform-specific aspects of the session. Called once per session before it is deleted.

	@param	inSession	Session Identifier for the AirPlay Session being deleted
	@result	kNoErr if successful or an error code indicating failure.

	@discussion
	
	This gives the platform a chance to clean up any per-session state. This must handle being called during a partial 
	initialization (e.g. failure mid-way through initialization). In certain situations, this may be called without 
	of a TearDownStreams control request so it needs to clean up all per-session state.
*/
void	AirPlayReceiverSessionPlatformFinalize( AirPlayReceiverSessionRef inSession );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionPlatformControl
	@abstract	Controls the platform-specific aspects of the session.

	@param	inServer	Session Identifier of the AirPlay Session 
	@param	inFlags		-
	@param	inCommand	Command:
	@param	inQualifier	-
	@param	inParams	Parameter for the command specified
	@param	outParams	Output parameter returned by the function

	@result	kNoErr if successful or an error code indicating failure.

	@discussion	

	The following commands need to be handled:
	<pre>
	@textblock

              inCommand                      inParams         outParams
              ----------                     --------         ---------
        - kAirPlayCommand_StartSession         None              None
             Tells the platform that a session has started so it can do any active setup.

        - kAirPlayCommand_StopSession          None              None
             Tells the platform that a session has stopped so it should stop any active session-specific operations.

        - kAirPlayCommand_SetUpStreams          None              None
             Sets up audio stream on the platform.

        - kAirPlayCommand_TearDownStreams       None              None
             Tears down audio stream on the platform.

        - kAirPlayCommand_FlushAudio            None              None
             Flush any unplayed audio data. Used when pause is pressed, when the user seeks to a different spot, etc.

	@/textblock
	</pre>

*/
OSStatus
	AirPlayReceiverSessionPlatformControl( 
		CFTypeRef			inSession, // Must be a AirPlayReceiverSessionRef.
		uint32_t			inFlags, 
		CFStringRef			inCommand, 
		CFTypeRef			inQualifier, 
		CFDictionaryRef		inParams, 
		CFDictionaryRef *	outParams );

// Convenience accessors.

#define AirPlayReceiverSessionPlatformControlF( SESSION, COMMAND, QUALIFIER, OUT_PARAMS, FORMAT, ... ) \
	CFObjectControlSyncF( (SESSION), NULL, AirPlayReceiverSessionPlatformControl, kCFObjectFlagDirect, \
		(COMMAND), (QUALIFIER), (OUT_PARAMS), (FORMAT), __VA_ARGS__ )

#define AirPlayReceiverSessionPlatformControlV( SESSION, COMMAND, QUALIFIER, OUT_PARAMS, FORMAT, ARGS ) \
	CFObjectControlSyncF( (SESSION), NULL, AirPlayReceiverSessionPlatformControl, kCFObjectFlagDirect, \
		(COMMAND), (QUALIFIER), (OUT_PARAMS), (FORMAT), (ARGS) )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionPlatformCopyProperty
	@abstract	Copies a platform-specific property from the session.

	@param	inSession	Session Identifier of the AirPlay Session 
	@param	inFlags		-
	@param	inProperty	Property to be copied
	@param	inQualifier	-
	@param	outErr		Status of the copy operation

	@result	CFRetained copy of the value of the property specified. 

	@discussion	

	The following properties need to be handled:
	<pre>
	@textblock

              inProperty                                result
              ----------                                ------
        - kAirPlayProperty_Volume                       CFNUMBER (kCFNumberDoubleType)   
             Return the audio dB attentuation level. Pre-defined values available are:
                            kAirTunesSilenceVolumeDB 
                            kAirTunesMinVolumeDB
                            kAirTunesMaxVolumeDB
                            kAirTunesDisabledVolumeDB

			The following converts between linear and dB volumes:
				      dB      = 20 * log10( linear )
				      linear  = pow( 10, dB / 20 )

	@/textblock
	</pre>

*/
CF_RETURNS_RETAINED
CFTypeRef
	AirPlayReceiverSessionPlatformCopyProperty( 
		CFTypeRef	inSession, // Must be a AirPlayReceiverSessionRef.
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		OSStatus *	outErr );

// Convenience accessors.

#define AirPlayReceiverSessionPlatformGetBoolean( SESSION, PROPERTY, QUALIFIER, OUT_ERR ) \
	CFObjectGetPropertyBooleanSync( (SESSION), NULL, AirPlayReceiverSessionPlatformCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (OUT_ERR) )

#define AirPlayReceiverSessionPlatformGetCString( SESSION, PROPERTY, QUALIFIER, BUF, MAX_LEN, OUT_ERR ) \
	CFObjectGetPropertyCStringSync( (SESSION), NULL, AirPlayReceiverSessionPlatformCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (BUF), (MAX_LEN), (OUT_ERR) )

#define AirPlayReceiverSessionPlatformGetDouble( SESSION, PROPERTY, QUALIFIER, OUT_ERR ) \
	CFObjectGetPropertyDoubleSync( (SESSION), NULL, AirPlayReceiverSessionPlatformCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (OUT_ERR) )

#define AirPlayReceiverSessionPlatformGetInt64( SESSION, PROPERTY, QUALIFIER, OUT_ERR ) \
	CFObjectGetPropertyInt64Sync( (SESSION), NULL, AirPlayReceiverSessionPlatformCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (OUT_ERR) )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionPlatformSetProperty
	@abstract	Sets a platform-specific property on the session.

	@param	inSession	Session Identifier of the AirPlay Session 
	@param	inFlags		-
	@param	inProperty	Property to be set
	@param	inQualifier	-
	@param	inValue		Value to be set

	@result	kNoErr if successful or an error code indicating failure.

	@discussion	

	The following properties need to be handled:
	<pre>
	@textblock

              inProperty                               inValue
              ----------                               -------
        - kAirPlayProperty_MetaData                    CFDict
             Sets the meta data for the current song. The Dictionary keys are:
                            kAirPlayMetaDataKey_Title 
                            kAirPlayMetaDataKey_Album
                            kAirPlayMetaDataKey_Artist
                            kAirPlayMetaDataKey_Composer
							kAirPlayMetaDataKey_ArtworkData

        - kAirPlayProperty_Volume                       CFNUMBER (kCFNumberDoubleType)   
             Sets the audio dB attentuation level. 

			The following converts between linear and dB volumes:
				      dB      = 20 * log10( linear )
				      linear  = pow( 10, dB / 20 )

        - kAirPlayProperty_Progress                    CFDict
             Sets the elapsed time and duration for the current song as it changes. The Dictionary keys are:
                            kAirPlayMetaDataKey_ElapsedTime 
                            kAirPlayMetaDataKey_Duration

        - kAirPlayProperty_Skew                    	   CFNumber
             Sets the current amount of skew detected to allow the platform to compensate for it.

	@/textblock
	</pre>

*/
OSStatus
	AirPlayReceiverSessionPlatformSetProperty( 
		CFTypeRef	inSession, // Must be a AirPlayReceiverSessionRef.
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		CFTypeRef	inValue );

// Convenience accessors.

#define AirPlayReceiverSessionPlatformSetBoolean( SESSION, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyBoolean( (SESSION), NULL, AirPlayReceiverSessionPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverSessionPlatformSetCString( SESSION, PROPERTY, QUALIFIER, STR, LEN ) \
	CFObjectSetPropertyCString( (SESSION), NULL, AirPlayReceiverSessionPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (STR), (LEN) )

#define AirPlayReceiverSessionPlatformSetDouble( SESSION, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyDouble( (SESSION), NULL, AirPlayReceiverSessionPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverSessionPlatformSetInt64( SESSION, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyInt64( (SESSION), NULL, AirPlayReceiverSessionPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverSessionPlatformSetPropertyF( SESSION, PROPERTY, QUALIFIER, FORMAT, ... ) \
	CFObjectSetPropertyF( (SESSION), NULL, AirPlayReceiverSessionPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (FORMAT), __VA_ARGS__ )

#define AirPlayReceiverSessionPlatformSetPropertyV( SESSION, PROPERTY, QUALIFIER, FORMAT, ARGS ) \
	CFObjectSetPropertyV( (SESSION), NULL, AirPlayReceiverSessionPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (FORMAT), (ARGS) )

#if 0
#pragma mark -
#pragma mark == Helpers ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionCreateModesDictionary
	@internal
	@abstract	Creates dictionary for the initial modes or the params to a changeModes request.
	
	@param		inChanges		Changes being request. Initialize with AirPlayModeChangesInit() then set fields.
	@param		inReason		Optional reason for the change. Mainly for diagnostics. May be NULL.
	@param		outErr			Optional error code to indicate a reason for failure. May be NULL.
*/
CFDictionaryRef
	AirPlayReceiverSessionCreateModesDictionary( 
		const AirPlayModeChanges *	inChanges, 
		CFStringRef					inReason, 
		OSStatus *					outErr );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionChangeModes
	@internal
	@abstract	Builds and sends a mode change request to the controller.
	
	@param		inSession		Session to request a mode change on.
	@param		inChanges		Changes being request. Initialize with AirPlayModeChangesInit() then set fields.
	@param		inReason		Optional reason for the change. Mainly for diagnostics. May be NULL.
	@param		inCompletion	Optional completion function to call when the request completes. May be NULL.
	@param		inContext		Optional context ptr to pass to completion function. May be NULL.
*/
OSStatus
	AirPlayReceiverSessionChangeModes( 
		AirPlayReceiverSessionRef					inSession, 
		const AirPlayModeChanges *					inChanges, 
		CFStringRef									inReason, 
		AirPlayReceiverSessionCommandCompletionFunc	inCompletion, 
		void *										inContext );

// Screen

#define AirPlayReceiverSessionTakeScreen( SESSION, PRIORITY, TAKE_CONSTRAINT, BORROW_CONSTRAINT, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeResourceMode( (SESSION), kAirPlayResourceID_MainScreen, kAirPlayTransferType_Take, (PRIORITY), \
		(TAKE_CONSTRAINT), (BORROW_CONSTRAINT), (REASON), (COMPLETION), (CONTEXT) )

#define AirPlayReceiverSessionUntakeScreen( SESSION, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeResourceMode( (SESSION), kAirPlayResourceID_MainScreen, kAirPlayTransferType_Untake, \
		kAirPlayTransferPriority_NotApplicable, kAirPlayConstraint_NotApplicable, kAirPlayConstraint_NotApplicable, \
		(REASON), (COMPLETION), (CONTEXT) )

#define AirPlayReceiverSessionBorrowScreen( SESSION, PRIORITY, UNBORROW_CONSTRAINT, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeResourceMode( (SESSION), kAirPlayResourceID_MainScreen, kAirPlayTransferType_Borrow, (PRIORITY), \
		kAirPlayConstraint_NotApplicable, (UNBORROW_CONSTRAINT), (REASON), (COMPLETION), (CONTEXT) )

#define AirPlayReceiverSessionUnborrowScreen( SESSION, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeResourceMode( (SESSION), kAirPlayResourceID_MainScreen, kAirPlayTransferType_Unborrow, \
		kAirPlayTransferPriority_NotApplicable, kAirPlayConstraint_NotApplicable, kAirPlayConstraint_NotApplicable, \
		(REASON), (COMPLETION), (CONTEXT) )

// MainAudio

#define AirPlayReceiverSessionTakeMainAudio( SESSION, PRIORITY, TAKE_CONSTRAINT, BORROW_CONSTRAINT, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeResourceMode( (SESSION), kAirPlayResourceID_MainAudio, kAirPlayTransferType_Take, (PRIORITY), \
		(TAKE_CONSTRAINT), (BORROW_CONSTRAINT), (REASON), (COMPLETION), (CONTEXT) )

#define AirPlayReceiverSessionUntakeMainAudio( SESSION, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeResourceMode( (SESSION), kAirPlayResourceID_MainAudio, kAirPlayTransferType_Untake, \
		kAirPlayTransferPriority_NotApplicable, kAirPlayConstraint_NotApplicable, kAirPlayConstraint_NotApplicable, \
		(REASON), (COMPLETION), (CONTEXT) )

#define AirPlayReceiverSessionBorrowMainAudio( SESSION, PRIORITY, UNBORROW_CONSTRAINT, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeResourceMode( (SESSION), kAirPlayResourceID_MainAudio, kAirPlayTransferType_Borrow, (PRIORITY), \
		kAirPlayConstraint_NotApplicable, (UNBORROW_CONSTRAINT), (REASON), (COMPLETION), (CONTEXT) )

#define AirPlayReceiverSessionUnborrowMainAudio( SESSION, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeResourceMode( (SESSION), kAirPlayResourceID_MainAudio, kAirPlayTransferType_Unborrow, \
		kAirPlayTransferPriority_NotApplicable, kAirPlayConstraint_NotApplicable, kAirPlayConstraint_NotApplicable, \
		(REASON), (COMPLETION), (CONTEXT) )

OSStatus
	AirPlayReceiverSessionChangeResourceMode( 
		AirPlayReceiverSessionRef					inSession, 
		AirPlayResourceID							inResourceID, 
		AirPlayTransferType							inType, 
		AirPlayTransferPriority						inPriority, 
		AirPlayConstraint							inTakeConstraint, 
		AirPlayConstraint							inBorrowConstraint, 
		CFStringRef									inReason, 
		AirPlayReceiverSessionCommandCompletionFunc	inCompletion, 
		void *										inContext );

// AppStates

#define AirPlayReceiverSessionChangePhoneCall( SESSION, ON_PHONE, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeAppState( (SESSION), kAirPlaySpeechMode_NotApplicable, \
		(ON_PHONE) ? kAirPlayTriState_True : kAirPlayTriState_False, kAirPlayTriState_NotApplicable, \
		(REASON), (COMPLETION), (CONTEXT) )

#define AirPlayReceiverSessionChangeSpeechMode( SESSION, SPEECH_MODE, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeAppState( (SESSION), (SPEECH_MODE), kAirPlayTriState_NotApplicable, \
		kAirPlayTriState_NotApplicable, (REASON), (COMPLETION), (CONTEXT) )

#define AirPlayReceiverSessionChangeTurnByTurn( SESSION, TURN_BY_TURN, REASON, COMPLETION, CONTEXT ) \
	AirPlayReceiverSessionChangeAppState( (SESSION), kAirPlaySpeechMode_NotApplicable, kAirPlayTriState_NotApplicable, \
		(TURN_BY_TURN) ? kAirPlayTriState_True : kAirPlayTriState_False, (REASON), (COMPLETION), (CONTEXT) )

OSStatus
	AirPlayReceiverSessionChangeAppState( 
		AirPlayReceiverSessionRef					inSession, 
		AirPlaySpeechMode							inSpeechMode, 
		AirPlayTriState								inPhoneCall, 
		AirPlayTriState								inTurnByTurn, 	
		CFStringRef									inReason, 
		AirPlayReceiverSessionCommandCompletionFunc	inCompletion, 
		void *										inContext );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverSessionRequestUI
	@internal
	@abstract	Builds and sends request UI command to the controller.
	
	@param		inSession		Session to request a mode change on.
	@param		inURL			Optional UI describing the UI to reuest (e.g. "http://maps.apple.com/?q" to show a map).
	@param		inCompletion	Optional completion function to call when the request completes. May be NULL.
	@param		inContext		Optional context ptr to pass to completion function. May be NULL.
*/
OSStatus
	AirPlayReceiverSessionRequestUI( 
		AirPlayReceiverSessionRef					inSession, 
		CFStringRef									inURL, 
		AirPlayReceiverSessionCommandCompletionFunc	inCompletion, 
		void *										inContext );

#if 0
#pragma mark -
#pragma mark == Legacy API ==
#endif

#if 0
#pragma mark -
#pragma mark == Stats ==
#endif

//===========================================================================================================================
//	Stats
//===========================================================================================================================

typedef struct
{
	EWMA_FP_Data		bufferAvg;
	uint32_t			lostPackets;
	uint32_t			unrecoveredPackets;
	uint32_t			latePackets;
	char				ifname[ IF_NAMESIZE + 1 ];
	
}	AirPlayAudioStats;

extern AirPlayAudioStats		gAirPlayAudioStats;

#if 0
#pragma mark == Debugging ==
#endif

//===========================================================================================================================
//	Debugging
//===========================================================================================================================

#if( DEBUG )
	extern int		gAirTunesDebugLogAllSkew;
	
	OSStatus	AirTunesDebugControl( const char *inCmd, CFStringRef *outOutput );
	void		AirTunesDebugControl_ResetDebugStats( void );
	OSStatus	AirTunesDebugPerf( int inPollIntervalSeconds, AirPlayReceiverSessionRef inSession );
#endif

OSStatus	AirTunesDebugShow( const char *inCmd, CFStringRef *outOutput );
OSStatus	AirTunesDebugAppendShowData( const char *inCmd, DataBuffer *inDB );

#ifdef __cplusplus
}
#endif

#endif	// __AirPlayReceiverSession_h__
