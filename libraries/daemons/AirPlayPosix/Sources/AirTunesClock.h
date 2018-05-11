/*
	Copyright (C) 2007-2012 Apple Inc. All Rights Reserved.
*/

#ifndef	__AirTunesClock_h__
#define	__AirTunesClock_h__

#include <math.h>

#include "CommonServices.h"
#include "DebugServices.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#pragma mark == API ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@struct		AirTunesTime
	@abstract	Structure representing time for AirTunes.
*/

typedef struct
{
	int32_t		secs; //! Number of seconds since 1970-01-01 00:00:00 (Unix time).
	uint64_t	frac; //! Fraction of a second in units of 1/2^64.

}	AirTunesTime;

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesClock_Initialize
	@abstract	Initializes the clock/timing engine.
*/

OSStatus	AirTunesClock_Initialize( void );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesClock_Finalize
	@abstract	Finalizes the clock/timing engine.
*/

OSStatus	AirTunesClock_Finalize( void );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesClock_Adjust
	@abstract	Starts the clock adjustment and discipline process based on the specified clock offset.
	
	@result		true if the clock was stepped. false if slewing.
*/

Boolean	AirTunesClock_Adjust( int64_t inOffsetNanoseconds, Boolean inReset );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesClock_GetSynchronizedNTPTime
	@abstract	Gets the current time, synchronized to the source clock as an NTP timestamp.
*/

uint64_t	AirTunesClock_GetSynchronizedNTPTime( void );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesClock_GetSynchronizedTime
	@abstract	Gets the current time, synchronized to the source clock.
*/

void	AirTunesClock_GetSynchronizedTime( AirTunesTime *outTime );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesClock_GetSynchronizedTimeNearUpTicks
	@abstract	Gets an estimate of the synchronized time near the specified UpTicks.
*/

void	AirTunesClock_GetSynchronizedTimeNearUpTicks( AirTunesTime *outTime, uint64_t inTicks );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesClock_GetUpTicksNearSynchronizedNTPTime
	@abstract	Gets an estimate of the local UpTicks() near an NTP timestamp on the synchronized timeline.
*/

uint64_t	AirTunesClock_GetUpTicksNearSynchronizedNTPTime( uint64_t inNTPTime );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesClock_GetUpTicksNearSynchronizedNTPTimeMid32
	@abstract	Gets an estimate of the local UpTicks() near the middle 32 bits of an NTP timestamp on the synchronized timeline.
*/

uint64_t	AirTunesClock_GetUpTicksNearSynchronizedNTPTimeMid32( uint32_t inNTPMid32 );

#if 0
#pragma mark == Utils ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesTime_AddFrac
	@abstract	Adds a fractional seconds (1/2^64 units) value to a time.
*/

STATIC_INLINE void	AirTunesTime_AddFrac( AirTunesTime *inTime, uint64_t inFrac )
{
	uint64_t		frac;
	
	frac = inTime->frac;
	inTime->frac = frac + inFrac;
	if( frac > inTime->frac ) inTime->secs += 1; // Increment seconds on fraction wrap.
}

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesTime_Add
	@abstract	Adds one time to another time.
*/

STATIC_INLINE void	AirTunesTime_Add( AirTunesTime *inTime, const AirTunesTime *inTimeToAdd )
{
	uint64_t		frac;
	
	frac = inTime->frac;
	inTime->frac = frac + inTimeToAdd->frac;
	if( frac > inTime->frac ) inTime->secs += 1; // Increment seconds on fraction wrap.
	inTime->secs += inTimeToAdd->secs;
}

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesTime_Sub
	@abstract	Subtracts one time from another time.
*/

STATIC_INLINE void	AirTunesTime_Sub( AirTunesTime *inTime, const AirTunesTime *inTimeToSub )
{
	uint64_t	frac;
	
	frac = inTime->frac;
	inTime->frac = frac - inTimeToSub->frac;
	if( frac < inTime->frac ) inTime->secs -= 1; // Decrement seconds on fraction wrap.
	inTime->secs -= inTimeToSub->secs;
}

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesTime_ToFP
	@abstract	Converts an AirTunesTime to a floating-point seconds value.
*/

STATIC_INLINE double	AirTunesTime_ToFP( const AirTunesTime *inTime )
{
	return( ( (double) inTime->secs ) + ( ( (double) inTime->frac ) * ( 1.0 / 18446744073709551615.0 ) ) );
}

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesTime_FromFP
	@abstract	Converts a floating-point seconds value to an AirTunesTime.
*/

STATIC_INLINE void	AirTunesTime_FromFP( AirTunesTime *inTime, double inFP )
{
	double		secs;
	double		frac;
	
	secs = floor( inFP );
	frac = inFP - secs;
	frac *= 18446744073709551615.0;
	
	inTime->secs = (int32_t) secs;
	inTime->frac = (uint64_t) frac;
}

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesTime_ToNanoseconds
	@abstract	Converts an AirTunesTime to nanoseconds.
*/

STATIC_INLINE uint64_t	AirTunesTime_ToNanoseconds( const AirTunesTime *inTime )
{
	uint64_t		ns;
	
	ns  = ( (uint64_t) inTime->secs ) * 1000000000;
	ns += ( UINT64_C( 1000000000 ) * ( (uint32_t)( inTime->frac >> 32 ) ) ) >> 32;
	return( ns );
}

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AirTunesTime_ToNTP
	@abstract	Converts an AirTunesTime to a 64-bit NTP timestamp.
*/

#define AirTunesTime_ToNTP( AT )		( ( ( (uint64_t) (AT)->secs ) << 32 ) | ( (AT)->frac >> 32 ) )

#ifdef __cplusplus
}
#endif

#endif	// __AirTunesClock_h__
