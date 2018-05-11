/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/
/*!
    @header AirPlay Initialization 
    @discussion Initialization APIs for AirPlay stack.
*/


#ifndef	__AirPlayMain_h__
#define	__AirPlayMain_h__

#include "AirPlayCommon.h"
#include "CommonServices.h"

#ifdef __cplusplus
extern "C" {
#endif

#if( AIRPLAY_THREADED_MAIN )
	int			AirPlayMain( int argc, const char *argv[] );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayStartMain
	@abstract	Kickoff AirPlay

	@result	kNoErr if successful or an error code indicating failure.

	@discussion
	
	This function kicksoff AirPlay stack. Platforms should call this function only if they prefer to run AirPlay stack as part of a
	platform process.
*/
	OSStatus	AirPlayStartMain( void );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirPlayStopMain
	@abstract	Shutdown AirPlay

	@result	kNoErr if successful or an error code indicating failure.

	@discussion
	
	This function shuts down AirPlay stack. Platforms should call this function only if they started AirPlay stack as part of a
	platform process by calling AirPlayStartMain() earlier.
*/
	OSStatus	AirPlayStopMain( void );
#endif

#ifdef __cplusplus
}
#endif

#endif	// __AirPlayMain_h__
