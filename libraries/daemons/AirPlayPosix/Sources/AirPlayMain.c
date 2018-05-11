/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/

#include "AirPlayMain.h"

#include "AirPlayCommon.h"
#include "AirPlayReceiverServer.h"
#include "AirPlayVersion.h"
#include "CommonServices.h"
#include "DebugServices.h"
#include "RandomNumberUtils.h"
#include "StringUtils.h"
#include "SystemUtils.h"
#include "ThreadUtils.h"

	#include <errno.h>
	#include <signal.h>
	#include <syslog.h>
#ifdef __NUTTX__
    #include <unistd.h>
#endif

#if( AIRPLAY_MFI )
	#include "MFiSAP.h"
#endif

//===========================================================================================================================
//	Internals
//===========================================================================================================================

#if( AIRPLAY_THREADED_MAIN )
	static void *	AirPlayMainThread( void *inArg );
	
	static pthread_t		gAirPlayMainThread;
	static pthread_t *		gAirPlayMainThreadPtr = NULL;
#endif

//===========================================================================================================================
//	main
//===========================================================================================================================

#if( AIRPLAY_THREADED_MAIN )
int	AirPlayMain( int argc, const char *argv[] )
#else
int	main( int argc, const char *argv[] )
#endif
{
	OSStatus						err;
	AirPlayReceiverServerRef		server = NULL;
	int								argi;
	const char *					arg;
	
#ifndef __NUTTX__
	for( argi = 1; argi < argc; )
	{
		// --daemon -- Run as a daemon.
		
		arg = argv[ argi++ ];
		if( strcmp( arg, "--daemon" ) == 0 )
		{
			err = daemon( 0, 0 );
			err = map_global_noerr_errno( err );
			if( err ) { syslog( LOG_WARNING, "### Run as daemon failed: %d", err ); exit( 1 ); }
		}
	}
	signal( SIGPIPE, SIG_IGN ); // Ignore SIGPIPE signals so we get EPIPE errors from APIs instead of a signal.
#endif /* __NUTTX__ */
	err = CFRunLoopEnsureInitialized();
	require_noerr( err, exit );
	
	AirPlaySetupServerLogging();
	err = AirPlayReceiverServerCreate( &server );
	require_noerr( err, exit );
	
	// Parse command line arguments.
	
	for( argi = 1; argi < argc; )
	{
		arg = argv[ argi++ ];
		
		if( 0 ) {}
		
		// -r -- Override the MAC address used for Bonjour, etc. with a random one for testing.
		
		else if( strcmp( arg, "-r" ) == 0 )
		{
			uint8_t		tempBuf[ 6 ];
			
			RandomBytes( tempBuf, sizeof( tempBuf ) );
			err = AirPlayReceiverServerSetData( server, CFSTR( kAirPlayProperty_DeviceID ), NULL, tempBuf, 6 );
			require_noerr( err, exit );
		}
		
		// -V -- Print the version and exit.
		
		else if( strcmp( arg, "-V" ) == 0 )
		{
			fprintf( stderr, "AirPlay version %s\n", kAirPlaySourceVersionStr );
			goto exit;
		}
		
		// Unknown
		
		else
		{
			fprintf( stderr, "warning: unknown option '%s'\n", arg );
		}
	}
	
	// Quit if built for AppleTV, but not running on AppleTV.
	
#if( AIRPLAY_MFI )
	MFiPlatform_Initialize();
#endif
	
	err = AirPlayReceiverServerControl( server, kCFObjectFlagDirect, CFSTR( kAirPlayCommand_StartServer ), NULL, NULL, NULL );
	require_noerr( err, exit );
	
	CFRunLoopRun();
	
	AirPlayReceiverServerControl( server, kCFObjectFlagDirect, CFSTR( kAirPlayCommand_StopServer ), NULL, NULL, NULL );
	
#if( AIRPLAY_MFI )
	MFiPlatform_Finalize();
#endif
	
exit:
	CFReleaseNullSafe( server );
	return( err ? 1 : 0 );
}

#if( AIRPLAY_THREADED_MAIN )
//===========================================================================================================================
//	AirPlayStartMain
//===========================================================================================================================

OSStatus	AirPlayStartMain( void )
{
	OSStatus		err;
	
	require_action( gAirPlayMainThreadPtr == NULL, exit, err = kAlreadyInUseErr );
	
	err = pthread_create( &gAirPlayMainThread, NULL, AirPlayMainThread, NULL );
	require_noerr( err, exit );
	gAirPlayMainThreadPtr = &gAirPlayMainThread;
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlayStopMain
//===========================================================================================================================

OSStatus	AirPlayStopMain( void )
{
	OSStatus		err;
	
	require_action( gAirPlayMainThreadPtr, exit, err = kNotInUseErr );
	
	CFRunLoopStop( CFRunLoopGetMain() );
	
	err = pthread_join( gAirPlayMainThread, NULL );
	check_noerr( err );
	gAirPlayMainThreadPtr = NULL;
	
exit:
	return( err );
}

//===========================================================================================================================
//	AirPlayMainThread
//===========================================================================================================================

static void *	AirPlayMainThread( void *inArg )
{
	(void) inArg;
	
	pthread_setname_np_compat( "AirPlayMain" );
	AirPlayMain( 0, NULL );
	return( NULL );
}
#endif // AIRPLAY_THREADED_MAIN
