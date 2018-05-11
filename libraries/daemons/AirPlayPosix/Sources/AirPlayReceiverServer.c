/*
	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
*/

#include "AirPlayReceiverServer.h"

#include "AirPlayCommon.h"
#include "AirPlaySettings.h"
#include "AirPlayUtils.h"
#include "AirPlayVersion.h"
#include "AirTunesServer.h"
#include "RandomNumberUtils.h"
#include "StringUtils.h"
#include "SystemUtils.h"
#include "TickUtils.h"

#if( TARGET_OS_DARWIN || ( TARGET_OS_POSIX && DEBUG ) )
	#include "DebugIPCUtils.h"
#endif

#include CF_HEADER
#include CF_RUNTIME_HEADER
#include "dns_sd.h"
#include LIBDISPATCH_HEADER

#if( AIRPLAY_DACP )
	#include "AirTunesDACP.h"
#endif

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

#if( AIRPLAY_CONFIG_FILES )
	static char *
		AirPlayGetConfigCString( 
			CFDictionaryRef *	ioConfig, 
			CFStringRef			inKey, 
			char *				inBuffer, 
			size_t				inMaxLen, 
			OSStatus *			outErr );
#endif
static OSStatus	AirPlayGetDeviceModel( char *inBuffer, size_t inMaxLen );

static void _GetTypeID( void *inContext );
static void	_Finalize( CFTypeRef inCF );
#if( AIRPLAY_DACP )
	static void	_HandleDACPStatus( OSStatus inStatus, void *inContext );
#endif
#if( AIRPLAY_PASSWORDS )
	static void	_UpdatePINTimerFired( void *inArg );
	static void	_UpdatePINTimerCanceled( void *inArg );
#endif
static void	_UpdatePrefs( AirPlayReceiverServerRef inServer );

static void
	_BonjourRegistrationHandler( 
		DNSServiceRef		inRef, 
		DNSServiceFlags		inFlags, 
		DNSServiceErrorType	inError, 
		const char *		inName,
		const char *		inType,
		const char *		inDomain,
		void *				inContext );
static void	_RestartBonjour( AirPlayReceiverServerRef inServer );
static void	_RetryBonjour( void *inArg );
static void	_StopBonjour( AirPlayReceiverServerRef inServer, const char *inReason );
static void	_UpdateBonjour( AirPlayReceiverServerRef inServer );
static OSStatus	_UpdateBonjourAirPlay( AirPlayReceiverServerRef inServer );
#if( AIRPLAY_RAOP_SERVER )
	static OSStatus	_UpdateBonjourRAOP( AirPlayReceiverServerRef inServer );
#endif

static void	_StartServers( AirPlayReceiverServerRef inServer );
static void	_StopServers( AirPlayReceiverServerRef inServer );
static void	_UpdateServers( AirPlayReceiverServerRef inServer );

//===========================================================================================================================
//	Globals
//===========================================================================================================================

static const CFRuntimeClass		kAirPlayReceiverServerClass = 
{
	0,							// version
	"AirPlayReceiverServer",	// className
	NULL,						// init
	NULL,						// copy
	_Finalize,					// finalize
	NULL,						// equal -- NULL means pointer equality.
	NULL,						// hash  -- NULL means pointer hash.
	NULL,						// copyFormattingDesc
	NULL,						// copyDebugDesc
	NULL,						// reclaim
	NULL						// refcount
};

static dispatch_once_t			gAirPlayReceiverServerInitOnce = 0;
static CFTypeID					gAirPlayReceiverServerTypeID = _kCFRuntimeNotATypeID;
AirPlayReceiverServerRef		gAirPlayReceiverServer = NULL; // $$$ TO DO: Temporary hack for transition. Remove ASAP.

ulog_define( AirPlayReceiverServer, kLogLevelNotice, kLogFlags_Default, "AirPlay",  NULL );
#define aprs_ucat()					&log_category_from_name( AirPlayReceiverServer )
#define aprs_ulog( LEVEL, ... )		ulog( aprs_ucat(), (LEVEL), __VA_ARGS__ )
#define aprs_dlog( LEVEL, ... )		dlogc( aprs_ucat(), (LEVEL), __VA_ARGS__ )

//===========================================================================================================================
//	AirPlayCopyServerInfo
//===========================================================================================================================

CFDictionaryRef	AirPlayCopyServerInfo( AirPlayReceiverSessionRef inSession, CFArrayRef inProperties, OSStatus *outErr )
{
	CFDictionaryRef				result = NULL;
	OSStatus					err;
	CFMutableDictionaryRef		info;
	uint8_t						buf[ 32 ];
	char						str[ PATH_MAX + 1 ];
	uint64_t					u64;
	CFTypeRef					obj;
	
	(void) inProperties;
	
	info = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	require_action( info, exit, err = kNoMemoryErr );
	
	// AudioInputFormats
	
	obj = AirPlayReceiverServerPlatformCopyProperty( gAirPlayReceiverServer, kCFObjectFlagDirect, 
		CFSTR( kAirPlayProperty_AudioInputFormats ), NULL, NULL );
	if( obj )
	{
		CFDictionarySetValue( info, CFSTR( kAirPlayProperty_AudioInputFormats ), obj );
		CFRelease( obj );
	}
	
	// AudioOutputFormats
	
	obj = AirPlayReceiverServerPlatformCopyProperty( gAirPlayReceiverServer, kCFObjectFlagDirect, 
		CFSTR( kAirPlayProperty_AudioOutputFormats ), NULL, NULL );
	if( obj )
	{
		CFDictionarySetValue( info, CFSTR( kAirPlayProperty_AudioInputFormats ), obj );
		CFRelease( obj );
	}
	
	// BluetoothIDs
	
	obj = AirPlayReceiverServerPlatformCopyProperty( gAirPlayReceiverServer, kCFObjectFlagDirect, 
		CFSTR( kAirPlayProperty_BluetoothIDs ), NULL, NULL );
	if( obj )
	{
		CFDictionarySetValue( info, CFSTR( kAirPlayProperty_BluetoothIDs ), obj );
		CFRelease( obj );
	}
	
	// DeviceID
	
	AirPlayGetDeviceID( buf );
	MACAddressToCString( buf, str );
	CFDictionarySetCString( info, CFSTR( kAirPlayKey_DeviceID ), str, kSizeCString );
	
	// Displays
	
	if( inSession )
	{
		obj = AirPlayReceiverSessionPlatformCopyProperty( inSession, kCFObjectFlagDirect, 
			CFSTR( kAirPlayProperty_Displays ), NULL, NULL );
		if( obj )
		{
			CFDictionarySetValue( info, CFSTR( kAirPlayKey_Displays ), obj );
			CFRelease( obj );
		}
	}
	
	// DockIcon
	
	if( inSession )
	{
		#if( AIRPLAY_CONFIG_FILES )
		obj = NULL;
		*str = '\0';
		AirPlayGetConfigCString( NULL, CFSTR( kAirPlayProperty_DockIconPath ), str, sizeof( str ), NULL );
		if( *str != '\0' ) obj = CFDataCreateWithFilePath( str, NULL );
		if( !obj )
		#endif
		{
			obj = AirPlayReceiverServerPlatformCopyProperty( gAirPlayReceiverServer, kCFObjectFlagDirect, 
				CFSTR( kAirPlayProperty_DockIcon ), NULL, NULL );
		}
		if( obj )
		{
			CFDictionarySetValue( info, CFSTR( kAirPlayProperty_DockIcon ), obj );
			CFRelease( obj );
		}
	}
	
	// Features
	
	u64 = AirPlayGetFeatures();
	if( u64 != 0 ) CFDictionarySetInt64( info, CFSTR( kAirPlayKey_Features ), u64 );
	
	// FirmwareRevision
	
	obj = AirPlayReceiverServerPlatformCopyProperty( gAirPlayReceiverServer, kCFObjectFlagDirect,
		CFSTR( kAirPlayKey_FirmwareRevision ), NULL, NULL );
	if( obj )
	{
		CFDictionarySetValue( info, CFSTR( kAirPlayKey_FirmwareRevision ), obj );
		CFRelease( obj );
	}
	
	// HardwareRevision
	
	obj = AirPlayReceiverServerPlatformCopyProperty( gAirPlayReceiverServer, kCFObjectFlagDirect,
		CFSTR( kAirPlayKey_HardwareRevision ), NULL, NULL );
	if( obj )
	{
		CFDictionarySetValue( info, CFSTR( kAirPlayKey_HardwareRevision ), obj );
		CFRelease( obj );
	}
	
	// Manufacturer
	
	obj = AirPlayReceiverServerPlatformCopyProperty( gAirPlayReceiverServer, kCFObjectFlagDirect,
		CFSTR( kAirPlayKey_Manufacturer ), NULL, NULL );
	if( obj )
	{
		CFDictionarySetValue( info, CFSTR( kAirPlayKey_Manufacturer ), obj );
		CFRelease( obj );
	}
	
	// Model
	
	*str = '\0';
	AirPlayGetDeviceModel( str, sizeof( str ) );
	CFDictionarySetCString( info, CFSTR( kAirPlayKey_Model ), str, kSizeCString );
	
	// Modes
	
	if( inSession )
	{
		obj = AirPlayReceiverSessionPlatformCopyProperty( inSession, kCFObjectFlagDirect, 
			CFSTR( kAirPlayProperty_Modes ), NULL, NULL );
		if( obj )
		{
			CFDictionarySetValue( info, CFSTR( kAirPlayProperty_Modes ), obj );
			CFRelease( obj );
		}
	}
	
	// Name
	
	*str = '\0';
	AirPlayGetDeviceName( str, sizeof( str ) );
	CFDictionarySetCString( info, CFSTR( kAirPlayKey_Name ), str, kSizeCString );
	
	// ProtocolVersion
	
	if( strcmp( kAirPlayProtocolVersionStr, "1.0" ) != 0 )
	{
		CFDictionarySetCString( info, CFSTR( kAirPlayKey_ProtocolVersion ), kAirPlayProtocolVersionStr, kSizeCString );
	}
	
	// SettingsIcon
	
	if( inSession )
	{
		#if( AIRPLAY_CONFIG_FILES )
		obj = NULL;
		*str = '\0';
		AirPlayGetConfigCString( NULL, CFSTR( kAirPlayProperty_SettingsIconPath ), str, sizeof( str ), NULL );
		if( *str != '\0' ) obj = CFDataCreateWithFilePath( str, NULL );
		if( !obj )
		#endif
		{
			obj = AirPlayReceiverServerPlatformCopyProperty( gAirPlayReceiverServer, kCFObjectFlagDirect,
				CFSTR( kAirPlayProperty_SettingsIcon ), NULL, NULL );
		}
		if( obj )
		{
			CFDictionarySetValue( info, CFSTR( kAirPlayProperty_SettingsIcon ), obj );
			CFRelease( obj );
		}
	}
	
	// SourceVersion
	
	CFDictionarySetValue( info, CFSTR( kAirPlayKey_SourceVersion ), CFSTR( kAirPlaySourceVersionStr ) );
	
	// StatusFlags
	
	u64 = AirPlayGetStatusFlags();
	if( u64 != 0 ) CFDictionarySetInt64( info, CFSTR( kAirPlayKey_StatusFlags ), u64 );
	
	result = info;
	info = NULL;
	err = kNoErr;
	
exit:
	CFReleaseNullSafe( info );
	if( outErr ) *outErr = err;
	return( result );
}

#if( AIRPLAY_CONFIG_FILES )
//===========================================================================================================================
//	AirPlayGetConfigCString
//===========================================================================================================================

static char *
	AirPlayGetConfigCString( 
		CFDictionaryRef *	ioConfig, 
		CFStringRef			inKey, 
		char *				inBuffer, 
		size_t				inMaxLen, 
		OSStatus *			outErr )
{
	CFDictionaryRef		config = ioConfig ? *ioConfig : NULL;
	char *				result = "";
	OSStatus			err;
	CFTypeRef			value;
	
#if( defined( AIRPLAY_DOCK_ICON_PATH ) )
	if( CFEqual( inKey, CFSTR( kAirPlayProperty_DockIconPath ) ) )
	{
		strlcpy( inBuffer, AIRPLAY_DOCK_ICON_PATH, inMaxLen );
		if( inMaxLen > 0 ) result = inBuffer;
		err = kNoErr;
		goto exit;
	}
#endif
	
#if( defined( AIRPLAY_SETTINGS_ICON_PATH ) )
	if( CFEqual( inKey, CFSTR( kAirPlayProperty_SettingsIconPath ) ) )
	{
		strlcpy( inBuffer, AIRPLAY_SETTINGS_ICON_PATH, inMaxLen );
		if( inMaxLen > 0 ) result = inBuffer;
		err = kNoErr;
		goto exit;
	}
#endif
	
	if( !config )
	{
		config = (CFDictionaryRef) CFPropertyListCreateFromFilePath( AIRPLAY_CONFIG_FILE_PATH, kCFPropertyListImmutable, &err );
		require_noerr_quiet( err, exit );
		if( config && !CFIsType( config, CFDictionary ) )
		{
			dlogassert( "Bad type for config file: %s", AIRPLAY_CONFIG_FILE_PATH );
			CFRelease( config );
			config = NULL;
		}
	}
	
	value = CFDictionaryGetValue( config, inKey );
	require_action_quiet( value, exit, err = kNotFoundErr );
	
	result = CFGetCString( value, inBuffer, inMaxLen );
	err = kNoErr;
	
exit:
	if( ioConfig )		*ioConfig = config;
	else if( config )	CFRelease( config );
	if( outErr )		*outErr = err;
	return( result );
}
#endif

//===========================================================================================================================
//	AirPlayGetDeviceID
//===========================================================================================================================

uint64_t	AirPlayGetDeviceID( uint8_t *outDeviceID )
{
	memcpy( outDeviceID, gAirPlayReceiverServer->deviceID, 6 );
	return( ReadBig48( gAirPlayReceiverServer->deviceID ) );
}

//===========================================================================================================================
//	AirPlayGetDeviceName
//===========================================================================================================================

OSStatus	AirPlayGetDeviceName( char *inBuffer, size_t inMaxLen )
{
	OSStatus		err;
	
	require_action( inMaxLen > 0, exit, err = kSizeErr );
	
	*inBuffer = '\0';
	AirPlaySettings_GetCString( NULL, CFSTR( kAirPlayPrefKey_Name ), inBuffer, inMaxLen, NULL );
#if( AIRPLAY_CONFIG_FILES )
	if( *inBuffer == '\0' ) AirPlayGetConfigCString( NULL, CFSTR( kAirPlayKey_Name ), inBuffer, inMaxLen, NULL );
#endif
#if( defined( AIRPLAY_DEVICE_NAME ) )
	if( *inBuffer == '\0' ) strlcpy( inBuffer, AIRPLAY_DEVICE_NAME, inMaxLen );	// Use hard-coded default name if provided.
#endif
	if( *inBuffer == '\0' ) GetDeviceName( inBuffer, inMaxLen );				// If no custom name, try the system name.
	if( *inBuffer == '\0' ) strlcpy( inBuffer, "AirPlay", inMaxLen );			// If no system name, use a default.
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlayGetDeviceModel
//===========================================================================================================================

static OSStatus	AirPlayGetDeviceModel( char *inBuffer, size_t inMaxLen )
{
	OSStatus		err;
	
	require_action( inMaxLen > 0, exit, err = kSizeErr );
	
	*inBuffer = '\0';
#if( AIRPLAY_CONFIG_FILES )
	AirPlayGetConfigCString( NULL, CFSTR( kAirPlayKey_Model ), inBuffer, inMaxLen, NULL );
#endif
#if( defined( AIRPLAY_DEVICE_MODEL ) )
	if( *inBuffer == '\0' ) strlcpy( inBuffer, inMaxLen, AIRPLAY_DEVICE_MODEL );	// Use hard-coded model if provided.
#endif
	if( *inBuffer == '\0' ) strlcpy( inBuffer, "AirPlayGeneric1,1", inMaxLen );		// If no model, use a default.
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlayGetFeatures
//===========================================================================================================================

AirPlayFeatures	AirPlayGetFeatures( void )
{
	AirPlayFeatures		features = 0;
	OSStatus			err;
	Boolean				b;
	
	// Mark as used to avoid special case for conditionalized cases.
	
	(void) err;
	(void) b;
	
	features |= kAirPlayFeature_Audio;
	features |= kAirPlayFeature_RedundantAudio;	
#if( AIRPLAY_META_DATA )
	features |= kAirPlayFeature_AudioMetaDataArtwork;
	features |= kAirPlayFeature_AudioMetaDataProgress;
	features |= kAirPlayFeature_AudioMetaDataText;
#endif
#if( AIRPLAY_MFI )
	features |= kAirPlayFeature_AudioAES_128_MFi_SAPv1;
#endif
	features |= kAirPlayFeature_AudioPCM;
	features |= kAirPlayFeature_AudioAppleLossless;
	features |= kAirPlayFeature_AudioUnencrypted;
	features |= kAirPlayFeature_UnifiedBonjour;
	
	features |= AirPlayReceiverServerPlatformGetInt64( gAirPlayReceiverServer, CFSTR( kAirPlayProperty_Features ), NULL, NULL );
	return( features );
}

//===========================================================================================================================
//	AirPlayGetPairingPublicKeyID
//===========================================================================================================================

//===========================================================================================================================
//	AirPlayGetStatusFlags
//===========================================================================================================================

AirPlayStatusFlags	AirPlayGetStatusFlags( void )
{
	AirPlayStatusFlags		flags = 0;
	
#if( AIRPLAY_PASSWORDS )
	if( gAirPlayReceiverServer->pinMode )
	{
		flags |= kAirPlayStatusFlag_PINMode;
	}
	else if( *gAirPlayReceiverServer->playPassword != '\0' )
	{
		flags |= kAirPlayStatusFlag_PasswordRequired;
	}
#endif
	flags |= (AirPlayStatusFlags) AirPlayReceiverServerPlatformGetInt64( gAirPlayReceiverServer, 
		CFSTR( kAirPlayProperty_StatusFlags ), NULL, NULL );
	return( flags );
}

//===========================================================================================================================
//	AirPlaySetupServerLogging
//===========================================================================================================================

void	AirPlaySetupServerLogging( void )
{
#if( LOGUTILS_CF_PREFERENCES )
	LogSetAppID( CFSTR( kAirPlayPrefAppID ) );
#endif
#if( DEBUG || TARGET_OS_DARWIN )
	{
		LogControl( 
			"?AirPlayReceiverCore:level=chatty"
			",AirPlayScreenServerCore:level=trace"
			",AirTunesServerCore:level=info"
			",MediaControlServerCore:level=trace");
		dlog_control( "?DebugServices.*:level=info" );
	}
#endif
#if  ( TARGET_OS_POSIX && DEBUG )
	DebugIPC_EnsureInitialized( NULL, NULL );
#endif
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	AirPlayReceiverServerGetTypeID
//===========================================================================================================================

CFTypeID	AirPlayReceiverServerGetTypeID( void )
{
	dispatch_once_f( &gAirPlayReceiverServerInitOnce, NULL, _GetTypeID );
	return( gAirPlayReceiverServerTypeID );
}

//===========================================================================================================================
//	AirPlayReceiverServerCreate
//===========================================================================================================================

OSStatus	AirPlayReceiverServerCreate( AirPlayReceiverServerRef *outServer )
{
	OSStatus						err;
	AirPlayReceiverServerRef		me;
	size_t							extraLen;
	int								i;
	uint64_t						u64;
	
	extraLen = sizeof( *me ) - sizeof( me->base );
	me = (AirPlayReceiverServerRef) _CFRuntimeCreateInstance( NULL, AirPlayReceiverServerGetTypeID(), (CFIndex) extraLen, NULL );
	require_action( me, exit, err = kNoMemoryErr );
	memset( ( (uint8_t *) me ) + sizeof( me->base ), 0, extraLen );
	
	me->overscanOverride = -1; // Default to auto.
	me->timeoutDataSecs  = kAirPlayDataTimeoutSecs;
	
	me->queue = dispatch_get_main_queue();
	dispatch_retain( me->queue );
	
	for( i = 1; i < 10; ++i )
	{
		u64 = GetPrimaryMACAddress( me->deviceID, NULL );
		if( u64 != 0 ) break;
		sleep( 1 );
	}
	
	err = AirPlayReceiverServerPlatformInitialize( me );
	require_noerr( err, exit );
	
	*outServer = me;
	gAirPlayReceiverServer = me;
	me = NULL;
	err = kNoErr;
	
exit:
	if( me ) CFRelease( me );
	return( err );
}

//===========================================================================================================================
//	AirPlayReceiverServerControl
//===========================================================================================================================

OSStatus
	AirPlayReceiverServerControl( 
		CFTypeRef			inServer, 
		uint32_t			inFlags, 
		CFStringRef			inCommand, 
		CFTypeRef			inQualifier, 
		CFDictionaryRef		inParams, 
		CFDictionaryRef *	outParams )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inServer;
	OSStatus							err;
	
	if( 0 ) {}
	
#if( AIRPLAY_DACP )
	// SendDACP
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_SendDACP ) ) )
	{
		if( server->serversStarted )
		{
			char *		dacpCommand;
			
			dacpCommand = CFDictionaryCopyCString( inParams, CFSTR( kAirPlayKey_DACPCommand ), &err );
			require_noerr( err, exit );
			
			AirTunesServer_ScheduleDACPCommand( dacpCommand );
			free( dacpCommand );
		}
	}
#endif
	
	// UpdateBonjour
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_UpdateBonjour ) ) )
	{
		_UpdateBonjour( server );
	}
	
#if( AIRPLAY_PASSWORDS )
	// UpdatePIN
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_UpdatePIN ) ) )
	{
		if( server->started && server->pinMode )
		{
			aprs_ulog( kLogLevelVerbose, "Scheduling PIN change\n" );
			
			dispatch_source_forget( &server->pinTimer );
			server->pinTimer = dispatch_source_create( DISPATCH_SOURCE_TYPE_TIMER, 0, 0, server->queue );
			require_action( server->pinTimer, exit, err = kUnknownErr );
			
			CFRetain( server );
			dispatch_set_context( server->pinTimer, server );
			dispatch_source_set_event_handler_f( server->pinTimer, _UpdatePINTimerFired );
			dispatch_source_set_cancel_handler_f( server->pinTimer, _UpdatePINTimerCanceled );
			dispatch_source_set_timer( server->pinTimer, dispatch_time_seconds( 30 ), INT64_MAX, kNanosecondsPerSecond );
			dispatch_resume( server->pinTimer );
		}
	}
#endif
	
	// PrefsChanged
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayEvent_PrefsChanged ) ) )
	{
		if( server->started )
		{
			aprs_ulog( kLogLevelNotice, "Prefs changed\n" );
			_UpdatePrefs( server );
		}
	}
	
	// StartServer / StopServer
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_StartServer ) ) )
	{
		server->started = true;
		_UpdatePrefs( server );
	}
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_StopServer ) ) )
	{
		if( server->started )
		{
			server->started = false;
			_StopServers( server );
		}
	}
	
	// SessionDied
	
	else if( CFEqual( inCommand, CFSTR( kAirPlayCommand_SessionDied ) ) )
	{
		AirTunesServer_FailureOccurred( kTimeoutErr );
	}
	
	// Other
	
	else
	{
		err = AirPlayReceiverServerPlatformControl( server, inFlags, inCommand, inQualifier, inParams, outParams );
		goto exit;
	}
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlayReceiverServerCopyProperty
//===========================================================================================================================

CFTypeRef
	AirPlayReceiverServerCopyProperty( 
		CFTypeRef	inServer, 
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		OSStatus *	outErr )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inServer;
	OSStatus							err;
	CFTypeRef							value = NULL;
	
	if( 0 ) {}
	
	// Playing
	
	else if( CFEqual( inProperty, CFSTR( kAirPlayProperty_Playing ) ) )
	{
		value = server->playing ? kCFBooleanTrue : kCFBooleanFalse;
		CFRetain( value );
	}
	
	// Other
	
	else
	{
		value = AirPlayReceiverServerPlatformCopyProperty( server, inFlags, inProperty, inQualifier, &err );
		goto exit;
	}
	err = kNoErr;
	
exit:
	if( outErr ) *outErr = err;
	return( value );
}

//===========================================================================================================================
//	AirPlayReceiverServerSetProperty
//===========================================================================================================================

OSStatus
	AirPlayReceiverServerSetProperty( 
		CFTypeRef	inServer, 
		uint32_t	inFlags, 
		CFStringRef	inProperty, 
		CFTypeRef	inQualifier, 
		CFTypeRef	inValue )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inServer;
	OSStatus							err;
	
	if( 0 ) {}
	
	// DeviceID
	
	else if( CFEqual( inProperty, CFSTR( kAirPlayProperty_DeviceID ) ) )
	{
		CFGetData( inValue, server->deviceID, sizeof( server->deviceID ), NULL, NULL );
	}
	
	// Playing
	
	else if( CFEqual( inProperty, CFSTR( kAirPlayProperty_Playing ) ) )
	{
		server->playing = CFGetBoolean( inValue, NULL );
		
		// If we updated Bonjour while we were playing and had to defer it, do it now that we've stopped playing.
		
		if( !server->playing && server->started && server->serversStarted && server->bonjourRestartPending )
		{
			_RestartBonjour( server );
		}
	}
	
	// Other
	
	else
	{
		err = AirPlayReceiverServerPlatformSetProperty( server, inFlags, inProperty, inQualifier, inValue );
		goto exit;
	}
	err = kNoErr;
	
exit:
	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	_GetTypeID
//===========================================================================================================================

static void _GetTypeID( void *inContext )
{
	(void) inContext;
	
	gAirPlayReceiverServerTypeID = _CFRuntimeRegisterClass( &kAirPlayReceiverServerClass );
	check( gAirPlayReceiverServerTypeID != _kCFRuntimeNotATypeID );
}

//===========================================================================================================================
//	_Finalize
//===========================================================================================================================

static void	_Finalize( CFTypeRef inCF )
{
	AirPlayReceiverServerRef const		me = (AirPlayReceiverServerRef) inCF;
	
	_StopServers( me );
	AirPlayReceiverServerPlatformFinalize( me );
	dispatch_forget( &me->queue );
	gAirPlayReceiverServer = NULL;
}

#if( AIRPLAY_DACP )
//===========================================================================================================================
//	_HandleDACPStatus
//===========================================================================================================================

static void	_HandleDACPStatus( OSStatus inStatus, void *inContext )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inContext;
	
	AirPlayReceiverServerControlAsyncF( server, CFSTR( kAirPlayEvent_DACPStatus ), NULL, NULL, NULL, NULL, 
		"{%kO=%i}", CFSTR( kAirPlayKey_Status ), inStatus );
}
#endif

#if( AIRPLAY_PASSWORDS )
//===========================================================================================================================
//	_UpdatePINTimer
//===========================================================================================================================

static void	_UpdatePINTimerFired( void *inArg )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inArg;
	
	dispatch_source_forget( &server->pinTimer );
	if( server->pinMode )
	{
		
		RandomString( kDecimalDigits, sizeof_string( kDecimalDigits ), 4, 4, server->playPassword );
		aprs_dlog( kLogLevelNotice, "Updated PIN to %s\n", server->playPassword );
	}
	if( server->serversStarted ) _UpdateServers( server );
}

static void	_UpdatePINTimerCanceled( void *inArg )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inArg;
	
	aprs_dlog( kLogLevelNotice, "PIN canceled\n" );
	CFRelease( server );
}
#endif

//===========================================================================================================================
//	_UpdatePrefs
//===========================================================================================================================

static void	_UpdatePrefs( AirPlayReceiverServerRef inServer )
{
	OSStatus		err;
	Boolean			b;
	int				i;
	char			cstr[ 64 ];
	Boolean			restartBonjour = false;
	Boolean			updateServers  = false;
	
	AirPlaySettings_Synchronize();
	
	// Enabled. If disabled, stop the servers and skip all the rest.
	
	b = AirPlaySettings_GetBoolean( NULL, CFSTR( kAirPlayPrefKey_Enabled ), &err );
	if( err ) b = true;
	if( !b )
	{
		_StopServers( inServer );
		goto exit;
	}
	
	// BonjourDisabled
	
	b = AirPlaySettings_GetBoolean( NULL, CFSTR( kAirPlayPrefKey_BonjourDisabled ), NULL );
	if( b != inServer->bonjourDisabled )
	{
		aprs_dlog( kLogLevelNotice, "Bonjour Disabled: %s -> %s\n", !b ? "yes" : "no", b ? "yes" : "no" );
		inServer->bonjourDisabled = b;
		_UpdateBonjour( inServer );
	}
	
	// DenyInterruptions
	
	b = AirPlaySettings_GetBoolean( NULL, CFSTR( kAirPlayPrefKey_DenyInterruptions ), NULL );
	if( b != inServer->denyInterruptions )
	{
		aprs_dlog( kLogLevelNotice, "Deny Interruptions: %s -> %s\n", !b ? "yes" : "no", b ? "yes" : "no" );
		inServer->denyInterruptions = b;
		updateServers = true;
	}
	
	// Name
	
	*cstr = '\0';
	AirPlayGetDeviceName( cstr, sizeof( cstr ) );
	if( strcmp( cstr, inServer->name ) != 0 )
	{
		aprs_ulog( kLogLevelNotice, "Name changed '%s' -> '%s'\n", inServer->name, cstr );
		strlcpy( inServer->name, cstr, sizeof( inServer->name ) );
		restartBonjour = true;
		updateServers  = true;
	}
	
	// OverscanOverride
	
	i = (int) AirPlaySettings_GetInt64( NULL, CFSTR( kAirPlayPrefKey_OverscanOverride ), &err );
	if( err ) i = -1;
	if( i != inServer->overscanOverride )
	{
		aprs_dlog( kLogLevelNotice, "Overscan override: %s -> %s\n", 
			( inServer->overscanOverride  < 0 ) ? "auto" :
			( inServer->overscanOverride == 0 ) ? "no"   :
												  "yes", 
			( i  < 0 ) ? "auto" :
			( i == 0 ) ? "no"   :
						 "yes" );
		inServer->overscanOverride = i;
	}

#if( AIRPLAY_PASSWORDS )
	// PINMode
	
	b = AirPlaySettings_GetBoolean( NULL, CFSTR( kAirPlayPrefKey_PINMode ), NULL );
	if( b != inServer->pinMode )
	{
		aprs_dlog( kLogLevelNotice, "PIN mode: %s -> %s\n", !b ? "yes" : "no", b ? "yes" : "no" );
		inServer->pinMode = b;
		_UpdatePINTimerFired( inServer );
		restartBonjour = true;
		updateServers  = true;
	}
	
	// PlayPassword
	
	if( !inServer->pinMode )
	{
		*cstr = '\0';
		AirPlaySettings_GetCString( NULL, CFSTR( kAirPlayPrefKey_PlayPassword ), cstr, sizeof( cstr ), &err );
		if( strcmp( cstr, inServer->playPassword ) != 0 )
		{
			strlcpy( inServer->playPassword, cstr, sizeof( inServer->playPassword ) );
			aprs_dlog( kLogLevelNotice, "Password changed\n" );
			restartBonjour = true;
			updateServers  = true;
		}
	}
#endif
	
	// QoSDisabled
	
	b = AirPlaySettings_GetBoolean( NULL, CFSTR( kAirPlayPrefKey_QoSDisabled ), NULL );
	if( b != inServer->qosDisabled )
	{
		aprs_dlog( kLogLevelNotice, "QoS disabled: %s -> %s\n", !b ? "yes" : "no", b ? "yes" : "no" );
		inServer->qosDisabled = b;
		updateServers = true;
	}
	
	// TimeoutData
	
	inServer->timeoutDataSecs = (int) AirPlaySettings_GetInt64( NULL, CFSTR( kAirPlayPrefKey_TimeoutDataSecs ), &err );
	if( err || ( inServer->timeoutDataSecs <= 0 ) ) inServer->timeoutDataSecs = kAirPlayDataTimeoutSecs;
	
	// Tell the platform so it can re-read any prefs it might have.
	
	AirPlayReceiverServerPlatformControl( inServer, kCFObjectFlagDirect, CFSTR( kAirPlayCommand_UpdatePrefs ), NULL, NULL, NULL );
	
	// Finally, act on any settings changes.
	
	if( !inServer->serversStarted )
	{
		_StartServers( inServer );
	}
	else
	{
		if( restartBonjour ) _RestartBonjour( inServer );
		if( updateServers )  _UpdateServers( inServer );
	}
	
exit:
	return;
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	_BonjourRegistrationHandler
//===========================================================================================================================

static void
	_BonjourRegistrationHandler( 
		DNSServiceRef		inRef, 
		DNSServiceFlags		inFlags, 
		DNSServiceErrorType	inError, 
		const char *		inName,
		const char *		inType,
		const char *		inDomain,
		void *				inContext )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inContext;
	DNSServiceRef *						servicePtr  = NULL;
	const char *						serviceType = "?";
	
	(void) inFlags;
	
	if( inRef == server->bonjourAirPlay )
	{
		servicePtr  = &server->bonjourAirPlay;
		serviceType = kAirPlayBonjourServiceType;
	}
#if( AIRPLAY_RAOP_SERVER )
	else if( inRef == server->bonjourRAOP )
	{
		servicePtr  = &server->bonjourRAOP;
		serviceType = kRAOPBonjourServiceType;
	}
#endif
	
	if( inError == kDNSServiceErr_ServiceNotRunning )
	{
		aprs_ulog( kLogLevelNotice, "### Bonjour server crashed for %s\n", serviceType );
		if( servicePtr ) DNSServiceForget( servicePtr );
		_UpdateBonjour( server );
	}
	else if( inError )
	{
		aprs_ulog( kLogLevelNotice, "### Bonjour registration error for %s: %#m\n", serviceType, inError );
	}
	else
	{
		aprs_ulog( kLogLevelNotice, "Registered Bonjour %s.%s%s\n", inName, inType, inDomain );
	}
}

//===========================================================================================================================
//	_RestartBonjour
//===========================================================================================================================

static void	_RestartBonjour( AirPlayReceiverServerRef inServer )
{
	inServer->bonjourRestartPending = false;
	
	// Ignore if we've been disabled.
	
	if( !inServer->started )
	{
		aprs_dlog( kLogLevelNotice, "Ignoring Bonjour restart while disabled\n" );
		goto exit;
	}
	
	// Only restart bonjour if we're not playing because some clients may stop playing if they see us go away.
	// If we're playing, just mark the Bonjour restart as pending and we'll process it when we stop playing.
	
	if( inServer->playing )
	{
		aprs_dlog( kLogLevelNotice, "Deferring Bonjour restart until we've stopped playing\n" );
		inServer->bonjourRestartPending = true;
		goto exit;
	}
	
	// Ignore if Bonjour hasn't been started.
	
	if( !inServer->bonjourAirPlay )
	{
		aprs_dlog( kLogLevelNotice, "Ignoring Bonjour restart since Bonjour wasn't started\n" );
		goto exit;
	}
	
	// Some clients stop resolves after ~2 minutes to reduce multicast traffic so if we changed something in the 
	// TXT record, such as the password state, those clients wouldn't be able to detect that state change.
	// To work around this, completely remove the Bonjour service, wait 2 seconds to give time for Bonjour to 
	// flush out the removal then re-add the Bonjour service so client will re-resolve it.
	
	_StopBonjour( inServer, "restart" );
	
	aprs_dlog( kLogLevelNotice, "Waiting for 2 seconds for Bonjour to remove service\n" );
	sleep( 2 );
	
	aprs_dlog( kLogLevelNotice, "Starting Bonjour service for restart\n" );
	_UpdateBonjour( inServer );
	
exit:
	return;
}

//===========================================================================================================================
//	_RetryBonjour
//===========================================================================================================================

static void	_RetryBonjour( void *inArg )
{
	AirPlayReceiverServerRef const		server = (AirPlayReceiverServerRef) inArg;
	
	aprs_ulog( kLogLevelNotice, "Retrying Bonjour after failure\n" );
	dispatch_source_forget( &server->bonjourRetryTimer );
	server->bonjourStartTicks = 0;
	_UpdateBonjour( server );
}

//===========================================================================================================================
//	_StopBonjour
//===========================================================================================================================

static void	_StopBonjour( AirPlayReceiverServerRef inServer, const char *inReason )
{
	dispatch_source_forget( &inServer->bonjourRetryTimer );
	inServer->bonjourStartTicks = 0;
	
	if( inServer->bonjourAirPlay )
	{
		DNSServiceForget( &inServer->bonjourAirPlay );
		aprs_ulog( kLogLevelNotice, "Deregistered Bonjour %s for %s\n", kAirPlayBonjourServiceType, inReason );
	}
#if( AIRPLAY_RAOP_SERVER )
	if( inServer->bonjourRAOP )
	{
		DNSServiceForget( &inServer->bonjourRAOP );
		aprs_ulog( kLogLevelNotice, "Deregistered Bonjour %s for %s\n", kRAOPBonjourServiceType, inReason );
	}
#endif
}

//===========================================================================================================================
//	_UpdateBonjour
//
//	Update all Bonjour services.
//===========================================================================================================================

static void	_UpdateBonjour( AirPlayReceiverServerRef inServer )
{
	OSStatus				err, firstErr = kNoErr;
	dispatch_source_t		timer;
	uint64_t				ms;
	
	if( !inServer->serversStarted || inServer->bonjourDisabled )
	{
		_StopBonjour( inServer, "disable" );
		return;
	}
	if( inServer->bonjourStartTicks == 0 ) inServer->bonjourStartTicks = UpTicks();
	
	err = _UpdateBonjourAirPlay( inServer );
	if( !firstErr ) firstErr = err;
	
#if( AIRPLAY_RAOP_SERVER )
	err = _UpdateBonjourRAOP( inServer );
	if( !firstErr ) firstErr = err;
#endif
	
	if( !firstErr )
	{
		dispatch_source_forget( &inServer->bonjourRetryTimer );
	}
	else if( !inServer->bonjourRetryTimer )
	{
		ms = UpTicksToMilliseconds( UpTicks() - inServer->bonjourStartTicks );
		ms = ( ms < 11113 ) ? ( 11113 - ms ) : 1; // Use 11113 to avoid being syntonic with 10 second re-launching.
		aprs_ulog( kLogLevelNotice, "### Bonjour failed, retrying in %llu ms: %#m\n", ms, firstErr );
		
		inServer->bonjourRetryTimer = timer = dispatch_source_create( DISPATCH_SOURCE_TYPE_TIMER, 0, 0, inServer->queue );
		check( timer );
		if( timer )
		{
			dispatch_set_context( timer, inServer );
			dispatch_source_set_event_handler_f( timer, _RetryBonjour );
			dispatch_source_set_timer( timer, dispatch_time_milliseconds( ms ), INT64_MAX, kNanosecondsPerSecond );
			dispatch_resume( timer );
		}
	}
}

//===========================================================================================================================
//	_UpdateBonjourAirPlay
//
//	Update Bonjour for the _airplay._tcp service.
//===========================================================================================================================

static OSStatus	_UpdateBonjourAirPlay( AirPlayReceiverServerRef inServer )
{
	OSStatus			err;
	TXTRecordRef		txtRec;
	uint8_t				txtBuf[ 256 ];
	char				cstr[ 256 ];
	const char *		ptr;
	AirPlayFeatures		features;
	int					n;
	uint32_t			u32;
	uint16_t			port;
	DNSServiceFlags		flags;
	const char *		domain;
	
	TXTRecordCreate( &txtRec, (uint16_t) sizeof( txtBuf ), txtBuf );
	
	// DeviceID
	
	AirPlayGetDeviceID( inServer->deviceID );
	MACAddressToCString( inServer->deviceID, cstr );
	err = TXTRecordSetValue( &txtRec, kAirPlayTXTKey_DeviceID, (uint8_t) strlen( cstr ), cstr );
	require_noerr( err, exit );
	
	// Features
	
	features = AirPlayGetFeatures();
	u32 = (uint32_t)( ( features >> 32 ) & UINT32_C( 0xFFFFFFFF ) );
	if( u32 != 0 )
	{
		n = snprintf( cstr, sizeof( cstr ), "0x%X,0x%X", (uint32_t)( features & UINT32_C( 0xFFFFFFFF ) ), u32 );
	}
	else
	{
		n = snprintf( cstr, sizeof( cstr ), "0x%X", (uint32_t)( features & UINT32_C( 0xFFFFFFFF ) ) );
	}
	err = TXTRecordSetValue( &txtRec, kAirPlayTXTKey_Features, (uint8_t) n, cstr );
	require_noerr( err, exit );
	
	// Flags
	
	u32 = AirPlayGetStatusFlags();
	if( u32 != 0 )
	{
		n = snprintf( cstr, sizeof( cstr ), "0x%x", u32 );
		err = TXTRecordSetValue( &txtRec, kAirPlayTXTKey_Flags, (uint8_t) n, cstr );
		require_noerr( err, exit );
	}
	
	// Model
	
	*cstr = '\0';
	AirPlayGetDeviceModel( cstr, sizeof( cstr ) );
	if( *cstr != '\0' )
	{
		err = TXTRecordSetValue( &txtRec, kAirPlayTXTKey_Model, (uint8_t) strlen( cstr ), cstr );
		require_noerr( err, exit );
	}
	
#if( AIRPLAY_PASSWORDS )
	// Password
	
	if( !inServer->pinMode && inServer->playPassword && ( *inServer->playPassword != '\0' ) )
	{
		err = TXTRecordSetValue( &txtRec, kAirPlayTXTKey_Password, 1, "1" );
		require_noerr( err, exit );
	}
	
	// PIN Mode
	
	if( inServer->pinMode )
	{
		err = TXTRecordSetValue( &txtRec, kAirPlayTXTKey_PIN, 1, "1" );
		require_noerr( err, exit );
	}
#endif
	
	// Protocol version
	
	ptr = kAirPlayProtocolVersionStr;
	if( strcmp( ptr, "1.0" ) != 0 )
	{
		err = TXTRecordSetValue( &txtRec, kAirPlayTXTKey_ProtocolVersion, (uint8_t) strlen( ptr ), ptr );
		require_noerr( err, exit );
	}
	
	// Source Version
	
	ptr = kAirPlaySourceVersionStr;
	err = TXTRecordSetValue( &txtRec, kAirPlayTXTKey_SourceVersion, (uint8_t) strlen( ptr ), ptr );
	require_noerr( err, exit );
	
	// Update the service.
	
	if( inServer->bonjourAirPlay )
	{
		err = DNSServiceUpdateRecord( inServer->bonjourAirPlay, NULL, 0, TXTRecordGetLength( &txtRec ), 
			TXTRecordGetBytesPtr( &txtRec ), 0 );
		if( !err ) aprs_ulog( kLogLevelNotice, "Updated Bonjour TXT for %s\n", kAirPlayBonjourServiceType );
		else
		{
			// Update record failed or isn't supported so deregister to force a re-register below.
			
			DNSServiceForget( &inServer->bonjourAirPlay );
		}
	}
	if( !inServer->bonjourAirPlay )
	{
		ptr = NULL;
			*cstr = '\0';
			AirPlayGetDeviceName( cstr, sizeof( cstr ) );
			ptr = cstr;
		
		flags = 0;
		domain = kAirPlayBonjourServiceDomain;
		
		port = (uint16_t) AirTunesServer_GetListenPort();
		require_action( port > 0, exit, err = kInternalErr );
		
		err = DNSServiceRegister( &inServer->bonjourAirPlay, flags, kDNSServiceInterfaceIndexAny, 
			ptr, kAirPlayBonjourServiceType, domain, NULL, htons( port ), 
			TXTRecordGetLength( &txtRec ), TXTRecordGetBytesPtr( &txtRec ), _BonjourRegistrationHandler, inServer );
		require_noerr_quiet( err, exit );
		
		aprs_ulog( kLogLevelNotice, "Registering Bonjour %s port %d\n", kAirPlayBonjourServiceType, port );
	}
		
exit:
	TXTRecordDeallocate( &txtRec );
	return( err );
}

#if( AIRPLAY_RAOP_SERVER )
//===========================================================================================================================
//	_UpdateBonjourRAOP
//
//	Update Bonjour for the _raop._tcp server.
//===========================================================================================================================

static OSStatus	_UpdateBonjourRAOP( AirPlayReceiverServerRef inServer )
{
	OSStatus			err;
	TXTRecordRef		txtRec;
	uint8_t				txtBuf[ 256 ];
	char				str[ 256 ];
	const char *		ptr;
	size_t				len;
	uint32_t			u32;
	int					n;
	AirPlayFeatures		features;
	uint8_t				deviceID[ 6 ];
	const uint8_t *		a;
	uint16_t			port;
	uint32_t			flags;
	const char *		domain;
	
	TXTRecordCreate( &txtRec, (uint16_t) sizeof( txtBuf ), txtBuf );
	
	// CompressionTypes
	
	u32  = kAirPlayCompressionType_PCM;
	u32 |= kAirPlayCompressionType_ALAC;
	BitListString_Make( u32, str, &len );
	err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_CompressionTypes, (uint8_t) len, str );
	require_noerr( err, exit );
	
	// Digest Auth from RFC-2617 (i.e. use lowercase hex digits)
	
	err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_RFC2617DigestAuth, sizeof_string( "true" ), "true" );
	require_noerr( err, exit );
	
	// Encryption Types
	
	u32  = kAirPlayEncryptionType_None;
#if( AIRPLAY_MFI )
	u32 |= kAirPlayEncryptionType_MFi_SAPv1;
#endif
	BitListString_Make( u32, str, &len );
	err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_EncryptionTypes, (uint8_t) len, str );
	require_noerr( err, exit );
	
	// Features
	
	features = AirPlayGetFeatures();
	u32 = (uint32_t)( ( features >> 32 ) & UINT32_C( 0xFFFFFFFF ) );
	if( u32 != 0 )
	{
		len = (size_t) snprintf( str, sizeof( str ), "0x%X,0x%X", (uint32_t)( features & UINT32_C( 0xFFFFFFFF ) ), u32 );
	}
	else
	{
		len = (size_t) snprintf( str, sizeof( str ), "0x%X", (uint32_t)( features & UINT32_C( 0xFFFFFFFF ) ) );
	}
	err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_Features, (uint8_t) len, str );
	require_noerr( err, exit );
	
#if( AIRPLAY_META_DATA )
	// MetaDataTypes -- Deprecated as of Mac OS X 10.9 and iOS 7.0 and later, but needed for older senders.
	
	u32 = 0;
	if( features & kAirPlayFeature_AudioMetaDataArtwork )	u32 |= kAirTunesMetaDataType_Artwork;
	if( features & kAirPlayFeature_AudioMetaDataProgress )	u32 |= kAirTunesMetaDataType_Progress;
	if( features & kAirPlayFeature_AudioMetaDataText )		u32 |= kAirTunesMetaDataType_Text;
	if( u32 != 0 )
	{
		BitListString_Make( u32, str, &len );
		err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_MetaDataTypes, (uint8_t) len, str );
		require_noerr( err, exit );
	}
#endif
	
	// Model
	
	*str = '\0';
	AirPlayGetDeviceModel( str, sizeof( str ) );
	err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_AppleModel, (uint8_t) strlen( str ), str );
	require_noerr( err, exit );
	
#if( AIRPLAY_PASSWORDS )
	// PasswordRequired
	
	if( !inServer->pinMode && ( *inServer->playPassword != '\0' ) )
	{
		err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_Password, sizeof_string( "true" ), "true" );
		require_noerr( err, exit );
	}
#endif
	
	// StatusFlags
	
	u32 = AirPlayGetStatusFlags();
	if( u32 != 0 )
	{
		n = snprintf( str, sizeof( str ), "0x%x", u32 );
		err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_StatusFlags, (uint8_t) n, str );
		require_noerr( err, exit );
	}
	
	// TransportTypes (audio)
	
	err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_TransportTypes, 3, "UDP" );
	require_noerr( err, exit );
	
	// Version (protocol)
	
	n = snprintf( str, sizeof( str ), "%u", 0x00010001 ); // 1.1 (legacy version)
	err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_ProtocolVersion, (uint8_t) n, str );
	require_noerr( err, exit );
	
	// Version (source)
	
	ptr = kAirPlaySourceVersionStr;
	err = TXTRecordSetValue( &txtRec, kRAOPTXTKey_SourceVersion, (uint8_t) strlen( ptr ), ptr );
	require_noerr( err, exit );
	
	// Update the service.
	
	if( inServer->bonjourRAOP )
	{
		err = DNSServiceUpdateRecord( inServer->bonjourRAOP, NULL, 0, TXTRecordGetLength( &txtRec ), 
			TXTRecordGetBytesPtr( &txtRec ), 0 );
		if( !err ) aprs_ulog( kLogLevelNotice, "Updated Bonjour TXT for %s\n", kRAOPBonjourServiceType );
		else
		{
			// Update record failed or isn't supported so deregister to force a re-register below.
			
			DNSServiceForget( &inServer->bonjourRAOP );
		}
	}
	if( !inServer->bonjourRAOP )
	{
		// Publish the service. The name is in the format <mac address>@<speaker name> such as "001122AABBCC@My Device".
		
		AirPlayGetDeviceID( deviceID );
		a = deviceID;
		n = snprintf( str, sizeof( str ), "%02X%02X%02X%02X%02X%02X@", a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ], a[ 5 ] );
		err = AirPlayGetDeviceName( &str[ n ], sizeof( str ) - n );
		require_noerr( err, exit );
		
		flags = 0;
		domain = kRAOPBonjourServiceDomain;
		
		port = (uint16_t) AirTunesServer_GetListenPort();
		err = DNSServiceRegister( &inServer->bonjourRAOP, flags, kDNSServiceInterfaceIndexAny, 
			str, kRAOPBonjourServiceType, domain, NULL, htons( port ), 
			TXTRecordGetLength( &txtRec ), TXTRecordGetBytesPtr( &txtRec ), _BonjourRegistrationHandler, inServer );
		require_noerr_quiet( err, exit );
		
		aprs_ulog( kLogLevelNotice, "Registering Bonjour %s.%s port %u\n", str, kRAOPBonjourServiceType, port );
	}
	
exit:
	TXTRecordDeallocate( &txtRec );
	return( err );
}
#endif // AIRPLAY_RAOP_SERVER

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	_StartServers
//===========================================================================================================================

static void	_StartServers( AirPlayReceiverServerRef inServer )
{
	OSStatus		err;
	
	DEBUG_USE_ONLY( err );
	
	if( inServer->serversStarted ) return;
	
	// Create the servers first.
	
	err = AirTunesServer_EnsureCreated();
	check_noerr( err );
#if( AIRPLAY_DACP )
	AirTunesServer_SetDACPCallBack( _HandleDACPStatus, inServer );
#endif
	
	// After all the servers are created, apply settings then start them.
	
	_UpdateServers( inServer );
	AirTunesServer_Start();
	inServer->serversStarted = true;
	_UpdateBonjour( inServer );
	
	AirPlayReceiverServerPlatformControl( inServer, kCFObjectFlagDirect, CFSTR( kAirPlayCommand_StartServer ), 
		NULL, NULL, NULL );
	
	aprs_ulog( kLogLevelNotice, "AirPlay servers started\n" );
}

//===========================================================================================================================
//	_StopServers
//===========================================================================================================================

static void	_StopServers( AirPlayReceiverServerRef inServer )
{
#if( AIRPLAY_PASSWORDS )
	dispatch_source_forget( &inServer->pinTimer );
#endif
	
	_StopBonjour( inServer, "stop" );
	if( inServer->serversStarted )
	{
		AirPlayReceiverServerPlatformControl( inServer, kCFObjectFlagDirect, CFSTR( kAirPlayCommand_StopServer ), 
			NULL, NULL, NULL );
	}
	AirTunesServer_EnsureDeleted();
	if( inServer->serversStarted )
	{
		aprs_ulog( kLogLevelNotice, "AirPlay servers stopped\n" );
		inServer->serversStarted = false;
	}
}

//===========================================================================================================================
//	_UpdateServers
//===========================================================================================================================

static void	_UpdateServers( AirPlayReceiverServerRef inServer )
{
	AirTunesServer_SetDenyInterruptions( inServer->denyInterruptions );
	if( inServer->serversStarted )
	{
		_UpdateBonjour( inServer );
	}
}
