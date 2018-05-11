/*
	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
*/
/*!
    @header AirPlay Server Platform APIs
    @discussion Platform APIs related to AirPlay Server.
*/

#ifndef	__AirPlayReceiverServer_h__
#define	__AirPlayReceiverServer_h__

#include "AirPlayCommon.h"
#include "CFUtils.h"
#include "CommonServices.h"
#include "DebugServices.h"

#include CF_HEADER
#include CF_RUNTIME_HEADER
#include "dns_sd.h"
#include LIBDISPATCH_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#pragma mark == Creation ==
#endif

typedef struct AirPlayReceiverServerPrivate *		AirPlayReceiverServerRef;
typedef struct AirPlayReceiverSessionPrivate *		AirPlayReceiverSessionRef;

extern AirPlayReceiverServerRef		gAirPlayReceiverServer;

CFTypeID	AirPlayReceiverServerGetTypeID( void );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerCreate
	@internal
	@abstract	Creates the server and initializes the server. Caller must CFRelease it when done.
*/
OSStatus	AirPlayReceiverServerCreate( AirPlayReceiverServerRef *outServer );

#if 0
#pragma mark -
#pragma mark == Control ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerControl
	@internal
	@abstract	Controls the server.
*/
OSStatus
	AirPlayReceiverServerControl( 
		CFTypeRef			inServer, // Must be AirPlayReceiverServerRef.
		uint32_t			inFlags, 
		CFStringRef			inCommand, 
		CFTypeRef			inQualifier, 
		CFDictionaryRef		inParams, 
		CFDictionaryRef *	outParams );

// Convenience accessors.

#define AirPlayReceiverServerControlF( SERVER, COMMAND, QUALIFIER, OUT_RESPONSE, FORMAT, ... ) \
	CFObjectControlSyncF( (SERVER), NULL, AirPlayReceiverServerControl, kCFObjectFlagDirect, \
		(COMMAND), (QUALIFIER), (OUT_RESPONSE), (FORMAT), __VA_ARGS__ )

#define AirPlayReceiverServerControlV( SERVER, COMMAND, QUALIFIER, OUT_RESPONSE, FORMAT, ARGS ) \
	CFObjectControlSyncV( (SERVER), NULL, AirPlayReceiverServerControl, kCFObjectFlagDirect, \
		(COMMAND), (QUALIFIER), (OUT_RESPONSE), (FORMAT), (ARGS) )

#define AirPlayReceiverServerControlAsync( SERVER, COMMAND, QUALIFIER, PARAMS, RESPONSE_QUEUE, RESPONSE_FUNC, RESPONSE_CONTEXT ) \
	CFObjectControlAsync( (SERVER), (SERVER)->queue, AirPlayReceiverServerControl, kCFObjectFlagAsync, \
		(COMMAND), (QUALIFIER), (PARAMS), (RESPONSE_QUEUE), (RESPONSE_FUNC), (RESPONSE_CONTEXT) )

#define AirPlayReceiverServerControlAsyncF( SERVER, COMMAND, QUALIFIER, RESPONSE_QUEUE, RESPONSE_FUNC, RESPONSE_CONTEXT, FORMAT, ... ) \
	CFObjectControlAsyncF( (SERVER), (SERVER)->queue, AirPlayReceiverServerControl, kCFObjectFlagAsync, \
		(COMMAND), (QUALIFIER), (RESPONSE_QUEUE), (RESPONSE_FUNC), (RESPONSE_CONTEXT), (FORMAT), __VA_ARGS__ )

#define AirPlayReceiverServerControlAsyncV( SERVER, COMMAND, QUALIFIER, RESPONSE_QUEUE, RESPONSE_FUNC, RESPONSE_CONTEXT, FORMAT, ARGS ) \
	CFObjectControlAsyncV( (SERVER), (SERVER)->queue, AirPlayReceiverServerControl, kCFObjectFlagAsync, \
		(COMMAND), (QUALIFIER), (RESPONSE_QUEUE), (RESPONSE_FUNC), (RESPONSE_CONTEXT) (FORMAT), (ARGS) )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerPostEvent
	@abstract	Post events to AirPlay stack to notify of platform changes

	@param	inServer	Server Identifier of the AirPlay Server 
	@param	inEvent	    Even being notified
	@param	inQualifier	Reserved
	@param	inParams	Reserved

	@result	kNoErr if successful or an error code indicating failure.

	@discussion	

	Platform should call this function to notify AirPlay stack of the following events:
    
	<pre>
	@textblock

              inEvent
              -------
        - kAirPlayEvent_PrefsChanged
             Notify AirPlay about change in accessory preference like password change, name change etc.

	@/textblock
	</pre>

*/
OSStatus AirPlayReceiverServerPostEvent( CFTypeRef inServer, CFStringRef inEvent, CFTypeRef inQualifier, CFDictionaryRef inParams );

#define AirPlayReceiverServerPostEvent( SERVER, EVENT, QUALIFIER, PARAMS ) \
	CFObjectControlAsync( (SERVER), (SERVER)->queue, AirPlayReceiverServerControl, kCFObjectFlagAsync, \
		(EVENT), (QUALIFIER), (PARAMS), NULL, NULL, NULL )

#if 0
#pragma mark -
#pragma mark == Properties ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerCopyProperty
	@internal
	@abstract	Copies a property from the server.
*/
CF_RETURNS_RETAINED
CFTypeRef
	AirPlayReceiverServerCopyProperty( 
		CFTypeRef	inServer, 
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		OSStatus *	outErr );

// Convenience accessors.

#define AirPlayReceiverServerGetBoolean( SERVER, PROPERTY, QUALIFIER, OUT_ERR ) \
	CFObjectGetPropertyBooleanSync( (SERVER), NULL, AirPlayReceiverServerCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (OUT_ERR) )

#define AirPlayReceiverServerGetCString( SERVER, PROPERTY, QUALIFIER, BUF, MAX_LEN, OUT_ERR ) \
	CFObjectGetPropertyCStringSync( (SERVER), NULL, AirPlayReceiverServerCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (BUF), (MAX_LEN), (OUT_ERR) )

#define AirPlayReceiverServerGetDouble( SERVER, PROPERTY, QUALIFIER, OUT_ERR ) \
	CFObjectGetPropertyDoubleSync( (SERVER), NULL, AirPlayReceiverServerCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (OUT_ERR) )

#define AirPlayReceiverServerGetInt64( SERVER, PROPERTY, QUALIFIER, OUT_ERR ) \
	CFObjectGetPropertyInt64Sync( (SERVER), NULL, AirPlayReceiverServerCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (OUT_ERR) )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerSetProperty
	@internal
	@abstract	Sets a property on the server.
*/
OSStatus
	AirPlayReceiverServerSetProperty( 
		CFTypeRef	inServer, // Must be AirPlayReceiverServerRef.
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		CFTypeRef	inValue );

// Convenience accessors.

#define AirPlayReceiverServerSetBoolean( SERVER, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyBoolean( (SERVER), NULL, AirPlayReceiverServerSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverServerSetCString( SERVER, PROPERTY, QUALIFIER, STR, LEN ) \
	CFObjectSetPropertyCString( (SERVER), NULL, AirPlayReceiverServerSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (STR), (LEN) )

#define AirPlayReceiverServerSetData( SERVER, PROPERTY, QUALIFIER, PTR, LEN ) \
	CFObjectSetPropertyData( (SERVER), NULL, AirPlayReceiverServerSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (PTR), (LEN) )

#define AirPlayReceiverServerSetDouble( SERVER, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyDouble( (SERVER), NULL, AirPlayReceiverServerSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverServerSetInt64( SERVER, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyInt64( (SERVER), NULL, AirPlayReceiverServerSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverServerSetPropertyF( SERVER, PROPERTY, QUALIFIER, FORMAT, ... ) \
	CFObjectSetPropertyF( (SERVER), NULL, AirPlayReceiverServerSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (FORMAT), __VA_ARGS__ )

#define AirPlayReceiverServerSetPropertyV( SERVER, PROPERTY, QUALIFIER, FORMAT, ARGS ) \
	CFObjectSetPropertyV( (SERVER), NULL, AirPlayReceiverServerSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (FORMAT), (ARGS) )

#if( AIRPLAY_DACP )
//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerSendDACPCommand
	@abstract	Sends a DACP command from the accessory to the controller.
	
	@param	inServer	Server Identifier of the AirPlay Server 
	@param	inCommand	C string DACP command to send. See DACPCommon.h for string constants.

	@result	kNoErr if successful or an error code indicating failure.

	@discussion	

	Platform should call this function to send DACP commands to the controlling device. 
    
	<pre>
	@textblock
              inCommand
              ---------
        - kDACPCommandStr_BeginFastFwd
        - kDACPCommandStr_BeginRewind
        - kDACPCommandStr_MuteToggle
        - kDACPCommandStr_NextItem
        - kDACPCommandStr_Pause
        - kDACPCommandStr_Play
        - kDACPCommandStr_PlayPause
        - kDACPCommandStr_PlayResume
        - kDACPCommandStr_PrevItem
        - kDACPCommandStr_SetProperty
        - kDACPCommandStr_ShuffleToggle
        - kDACPCommandStr_Stop
        - kDACPCommandStr_VolumeDown
        - kDACPCommandStr_VolumeUp
        - kDACPProperty_DevicePreventPlayback
        - kDACPProperty_DeviceBusy
        - kDACPProperty_DeviceVolume

	@/textblock
	</pre>

*/
OSStatus AirPlayReceiverServerSendDACPCommand( CFTypeRef inServer, CFStringRef inCommand );

#define AirPlayReceiverServerSendDACPCommand( SERVER, COMMAND ) \
	AirPlayReceiverServerControlF( (SERVER), CFSTR( kAirPlayCommand_SendDACP ), NULL, NULL, \
		"{%kO=%s}", CFSTR( kAirPlayKey_DACPCommand ), (COMMAND) )
#endif

#if 0
#pragma mark -
#pragma mark == Platform ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerPlatformInitialize
	@abstract	Initializes the platform-specific aspects of the server. Called once when the server is created.

	@param	inServer	Server Identifier for the AirPlay Server being initialized

	@result	kNoErr if successful or an error code indicating failure.
*/
OSStatus	AirPlayReceiverServerPlatformInitialize( AirPlayReceiverServerRef inServer );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerPlatformFinalize
	@abstract	Finalizes the platform-specific aspects of the server. Called once when the server is released.

	@param	inServer	Server Identifier for the AirPlay Server being released

	@result	kNoErr if successful or an error code indicating failure.
*/
void	AirPlayReceiverServerPlatformFinalize( AirPlayReceiverServerRef inServer );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerPlatformControl
	@abstract	Controls the platform-specific aspects of the server.

	@param	inServer	Server Identifier of the AirPlay Server 
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
        - kAirPlayCommand_SetConfig           CFDict            CFDict
             Set WiFi Access Point Configuration to be used

        - kAirPlayCommand_ApplyConfig          None              None
             Apply the previously ser WiFi Access Point Configuration to be used

        - kAirPlayCommand_StartServer          None              None
             Perform Platform specific operation for AirPlay server start

        - kAirPlayCommand_StopServer           None              None
             Perform Platform specific operation for AirPlay server stop

        - kAirPlayCommand_UpdatePrefs          None              None
             Reload Platform specific preferences

	@/textblock
	</pre>

*/
OSStatus
	AirPlayReceiverServerPlatformControl( 
		CFTypeRef			inServer, // Must be AirPlayReceiverServerRef.
		uint32_t			inFlags, 
		CFStringRef			inCommand, 
		CFTypeRef			inQualifier, 
		CFDictionaryRef		inParams, 
		CFDictionaryRef *	outParams );

// Convenience accessors.

#define AirPlayReceiverServerPlatformControlF( SERVER, COMMAND, QUALIFIER, OUT_PARAMS, FORMAT, ... ) \
	CFObjectControlSyncF( (SERVER), NULL, AirPlayReceiverServerPlatformControl, kCFObjectFlagDirect, \
		(COMMAND), (QUALIFIER), (OUT_PARAMS), (FORMAT), __VA_ARGS__ )

#define AirPlayReceiverServerPlatformControlV( SERVER, COMMAND, QUALIFIER, OUT_PARAMS, FORMAT, ARGS ) \
	CFObjectControlSyncF( (SERVER), NULL, AirPlayReceiverServerPlatformControl, kCFObjectFlagDirect, \
		(COMMAND), (QUALIFIER), (OUT_PARAMS), (FORMAT), (ARGS) )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerPlatformCopyProperty
	@abstract	Copies/gets a platform-specific property from the server.

	@param	inServer	Server Identifier of the AirPlay Server 
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
        - kAirPlayProperty_AudioJackStatus              CFSTR   
             Return the status of the audio jack. Possible values are:
                            kAirPlayAudioJackStatus_Disconnected
                            kAirPlayAudioJackStatus_ConnectedAnalog
                            kAirPlayAudioJackStatus_ConnectedDigital
                            kAirPlayAudioJackStatus_ConnectedUnknown

        - kAirPlayProperty_Features                     CFNUMBER (kCFNumberSInt64Type)
             Return the features (bitwise ORed) supported by the accessory. Possible values are:
                            kAirPlayFeature_AudioMetaDataArtwork
                            kAirPlayFeature_AudioMetaDataProgress
                            kAirPlayFeature_AudioMetaDataText

        - kAirPlayProperty_PlaybackAllowed              CFBOOLEAN 
             Return true if Airplay playback is currently allowed. Possible values are:
                            kCFBooleanFalse
                            kCFBooleanTrue

        - kAirPlayProperty_SkewCompensation             CFBOOLEAN 
             Return true if the platform supports its own fine-grained skew compensation.
                            kCFBooleanFalse
                            kCFBooleanTrue

        - kAirPlayProperty_StatusFlags                  CFNUMBER (kCFNumberSInt64Type)
             Return the status (bitwise ORed) of the accessory. Possible values are:
                            kAirPlayStatusFlag_None
                            kAirPlayStatusFlag_Unconfigured
                            kAirPlayStatusFlag_AudioLink
                            kAirPlayStatusFlag_PasswordRequired         

	@/textblock
	</pre>

*/
CF_RETURNS_RETAINED
CFTypeRef
	AirPlayReceiverServerPlatformCopyProperty( 
		CFTypeRef	inServer, // Must be AirPlayReceiverServerRef.
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		OSStatus *	outErr );

// Convenience accessors.

#define AirPlayReceiverServerPlatformGetBoolean( SERVER, PROPERTY, QUALIFIER, OUT_ERR ) \
	CFObjectGetPropertyBooleanSync( (SERVER), NULL, AirPlayReceiverServerPlatformCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (OUT_ERR) )

#define AirPlayReceiverServerPlatformGetCString( SERVER, PROPERTY, QUALIFIER, BUF, MAX_LEN, OUT_ERR ) \
	CFObjectGetPropertyCStringSync( (SERVER), NULL, AirPlayReceiverServerPlatformCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (BUF), (MAX_LEN), (OUT_ERR) )

#define AirPlayReceiverServerPlatformGetDouble( SERVER, PROPERTY, QUALIFIER, OUT_ERR ) \
	CFObjectGetPropertyDoubleSync( (SERVER), NULL, AirPlayReceiverServerPlatformCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (OUT_ERR) )

#define AirPlayReceiverServerPlatformGetInt64( SERVER, PROPERTY, QUALIFIER, OUT_ERR ) \
	CFObjectGetPropertyInt64Sync( (SERVER), NULL, AirPlayReceiverServerPlatformCopyProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (OUT_ERR) )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayReceiverServerPlatformSetProperty
	@abstract	Sets a platform-specific property on the server.

	@param	inServer	Server Identifier of the AirPlay Server 
	@param	inFlags		-
	@param	inProperty	Property to be set
	@param	inQualifier	-
	@param	inValue		Value to be set

	@result	kNoErr if successful or an error code indicating failure.

	@discussion	

	The following properties need to be handled:
	<pre>
	@textblock

              inProperty                                
              ----------                               
                   -                                  

	@/textblock
	</pre>

*/
OSStatus
	AirPlayReceiverServerPlatformSetProperty( 
		CFTypeRef	inServer, // Must be AirPlayReceiverServerRef.
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		CFTypeRef	inValue );

// Convenience accessors.

#define AirPlayReceiverServerPlatformSetBoolean( SERVER, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyBoolean( (SERVER), NULL, AirPlayReceiverServerPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverServerPlatformSetCString( SERVER, PROPERTY, QUALIFIER, STR, LEN ) \
	CFObjectSetPropertyCString( (SERVER), NULL, AirPlayReceiverServerPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (STR), (LEN) )

#define AirPlayReceiverServerPlatformSetDouble( SERVER, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyDouble( (SERVER), NULL, AirPlayReceiverServerPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#define AirPlayReceiverServerPlatformSetInt64( SERVER, PROPERTY, QUALIFIER, VALUE ) \
	CFObjectSetPropertyInt64( (SERVER), NULL, AirPlayReceiverServerPlatformSetProperty, kCFObjectFlagDirect, \
		(PROPERTY), (QUALIFIER), (VALUE) )

#if 0
#pragma mark -
#pragma mark == Server Internals ==
#endif

//===========================================================================================================================
//	Server Internals
//===========================================================================================================================

struct AirPlayReceiverServerPrivate
{
	CFRuntimeBase				base;					// CF type info. Must be first.
	void *						platformPtr;			// Pointer to the platform-specific data.
	dispatch_queue_t			queue;					// Internal queue used by the server.
	
	// Bonjour
	
	Boolean						bonjourRestartPending;	// True if we're waiting until playback stops to restart Bonjour.
	uint64_t					bonjourStartTicks;		// Time when Bonjour was successfully started.
	dispatch_source_t			bonjourRetryTimer;		// Timer to retry Bonjour registrations if they fail.
	DNSServiceRef				bonjourAirPlay;			// _airplay._tcp Bonjour service.
#if( AIRPLAY_RAOP_SERVER )
	DNSServiceRef				bonjourRAOP;			// _raop._tcp Bonjour service for legacy AirPlay Audio.
#endif
	
	// Servers
	
#if( AIRPLAY_PASSWORDS )
	dispatch_source_t			pinTimer;				// Timer to update the PIN after a grace period.
#endif
	Boolean						playing;				// True if we're currently playing.
	Boolean						serversStarted;			// True if the network servers have been started.
	Boolean						started;				// True if we've been started. Prefs may still disable network servers.
	
	// Settings
	
	Boolean						bonjourDisabled;		// True if Bonjour is disabled.
	Boolean						denyInterruptions;		// True if hijacking is disabled.
	uint8_t						deviceID[ 6 ];			// Globally unique device ID (i.e. primary MAC address).
	char						name[ 64 ];				// Name that people will see in the AirPlay pop up (e.g. "Kitchen Apple TV").
	int							overscanOverride;		// -1=Compensate if the TV is overscanned. 0=Don't compensate. 1=Always compensate.
#if( AIRPLAY_PASSWORDS )
	Boolean						pinMode;				// True if we're using a new, random PIN for each AirPlay session.
	char						playPassword[ 64 ];		// Password to AirPlay. If empty, no password is required.
#endif
	Boolean						qosDisabled;			// If true, don't use QoS.
	int							timeoutDataSecs;		// Timeout for data (defaults to kAirPlayDataTimeoutSecs).
};

#if 0
#pragma mark
#pragma mark == Server Utils ==
#endif

//===========================================================================================================================
//	Server Utils
//===========================================================================================================================

CF_RETURNS_RETAINED
CFDictionaryRef		AirPlayCopyServerInfo( AirPlayReceiverSessionRef inSession, CFArrayRef inProperties, OSStatus *outErr );
uint64_t			AirPlayGetDeviceID( uint8_t *outDeviceID );
OSStatus			AirPlayGetDeviceName( char *inBuffer, size_t inMaxLen );
AirPlayFeatures		AirPlayGetFeatures( void );
AirPlayStatusFlags	AirPlayGetStatusFlags( void );
void				AirPlaySetupServerLogging( void );

#ifdef __cplusplus
}
#endif

#endif // __AirPlayReceiverServer_h__
