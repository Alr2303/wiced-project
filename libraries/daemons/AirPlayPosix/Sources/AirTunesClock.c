/*
	Copyright (C) 2007-2012 Apple Inc. All Rights Reserved.
*/

#include "AirTunesClock.h"

#include "CommonServices.h"
#include "DebugServices.h"
#include "ThreadUtils.h"
#include "TickUtils.h"

//===========================================================================================================================
//	Private
//===========================================================================================================================

// 64-bit fixed-pointed math (32.32).

typedef int64_t		Fixed64;

#define Fixed64_Add( X, Y )				( ( X ) += ( Y ) )
#define Fixed64_Sub( X, Y )				( ( X ) -= ( Y ) )
#define Fixed64_RightShift( X, N )						\
	do													\
	{													\
		if( ( X ) < 0)	( X ) = -( -( X ) >> ( N ) );	\
		else			( X ) =     ( X ) >> ( N ); 	\
														\
	}	while( 0 )

#define Fixed64_Multiply( X, Y )		( ( X ) *= ( Y ) )
#define Fixed64_Clear( X )				( ( X ) = 0 )
#define Fixed64_GetInteger( X )			( ( ( X ) < 0 ) ? ( -( -( X ) >> 32 ) ) : ( ( X ) >> 32 ) )
#define Fixed64_SetInteger( X, Y )		( ( X ) = ( (int64_t)( Y ) ) << 32 )

// Phase Lock Loop (PLL) constants.

#define kAirTunesClock_MaxPhase			500000000 // Max phase error (nanoseconds).
#define kAirTunesClock_MaxFrequency		   500000 // Max frequence error (nanoseconds per second).
#define kAirTunesClock_PLLShift			        4 // PLL loop gain (bit shift value).

// Prototypes

DEBUG_STATIC void	_AirTunesClock_Tick( void );
DEBUG_STATIC void *	_AirTunesClock_Thread( void *inArg );

// Globals

static AirTunesTime			gAirTunesClock_EpochTime;
static AirTunesTime			gAirTunesClock_UpTime;
static AirTunesTime			gAirTunesClock_LastTime;
static uint64_t				gAirTunesClock_Frequency;
static uint64_t				gAirTunesClock_Scale;
static uint32_t				gAirTunesClock_LastCount;
static int64_t				gAirTunesClock_Adjustment;
static int32_t				gAirTunesClock_LastOffset;		// Last time offset (nanoseconds).
static int32_t				gAirTunesClock_LastAdjustTime;	// Time at last adjustment (seconds).
static Fixed64				gAirTunesClock_Offset;			// Time offset (nanoseconds).
static Fixed64				gAirTunesClock_FrequencyOffset;	// Frequency offset (nanoseconds per second).
static Fixed64				gAirTunesClock_TickAdjust;		// Amount to adjust at each tick (nanoseconds per second).
static int32_t				gAirTunesClock_Second = 1;		// Current second.

static pthread_mutex_t		gAirTunesClock_Lock;
static pthread_mutex_t *	gAirTunesClock_LockPtr		= NULL;
static pthread_t			gAirTunesClock_Thread;
static pthread_t *			gAirTunesClock_ThreadPtr	= NULL;
static Boolean				gAirTunesClock_Running		= false;

//===========================================================================================================================
//	AirTunesClock_Initialize
//===========================================================================================================================

OSStatus	AirTunesClock_Initialize( void )
{
	OSStatus		err;
	
	err = pthread_mutex_init( &gAirTunesClock_Lock, NULL );
	require_noerr( err, exit );
	gAirTunesClock_LockPtr = &gAirTunesClock_Lock;
	
	gAirTunesClock_EpochTime.secs = 0;
	gAirTunesClock_EpochTime.frac = 0;
	
	gAirTunesClock_UpTime.secs = 0;
	gAirTunesClock_UpTime.frac = 0;
	
	gAirTunesClock_LastTime.secs = 0;
	gAirTunesClock_LastTime.frac = 0;
	
	gAirTunesClock_Frequency = UpTicksPerSecond();
	gAirTunesClock_Scale = UINT64_C( 0xFFFFFFFFFFFFFFFF ) / gAirTunesClock_Frequency;
	
	gAirTunesClock_LastCount		= 0;
	gAirTunesClock_Adjustment		= 0;
	gAirTunesClock_LastOffset		= 0;
	gAirTunesClock_LastAdjustTime	= 0;
	
	Fixed64_Clear( gAirTunesClock_Offset );
	Fixed64_Clear( gAirTunesClock_FrequencyOffset );
	Fixed64_Clear( gAirTunesClock_TickAdjust );
	
	gAirTunesClock_Second = 1;
	
	gAirTunesClock_Running = true;
	err = pthread_create( &gAirTunesClock_Thread, NULL, _AirTunesClock_Thread, NULL );
	require_noerr( err, exit );
	gAirTunesClock_ThreadPtr = &gAirTunesClock_Thread;
	
exit:
	if( err ) AirTunesClock_Finalize();
	return( err );
}

//===========================================================================================================================
//	AirTunesClock_Finalize
//===========================================================================================================================

OSStatus	AirTunesClock_Finalize( void )
{
	OSStatus		err;
	
	DEBUG_USE_ONLY( err );
	
	gAirTunesClock_Running = false;
	if( gAirTunesClock_ThreadPtr )
	{
		err = pthread_join( gAirTunesClock_Thread, NULL );
		check_noerr( err );
		gAirTunesClock_ThreadPtr = NULL;
	}
	
	if( gAirTunesClock_LockPtr )
	{
		err = pthread_mutex_destroy( gAirTunesClock_LockPtr );
		check_noerr( err );
		gAirTunesClock_LockPtr = NULL;
	}
	return( kNoErr );
}

//===========================================================================================================================
//	AirTunesClock_Adjust
//===========================================================================================================================

Boolean	AirTunesClock_Adjust( int64_t inOffsetNanoseconds, Boolean inReset )
{
	if( inReset || ( ( inOffsetNanoseconds < -100000000 ) || ( inOffsetNanoseconds > 100000000 ) ) )
	{
		AirTunesTime		at;
		uint64_t			offset;
		
		pthread_mutex_lock( &gAirTunesClock_Lock );
		if( inOffsetNanoseconds < 0 )
		{
			offset  = (uint64_t) -inOffsetNanoseconds;
			at.secs = (int32_t)( offset / 1000000000 );
			at.frac = ( offset % 1000000000 ) * ( UINT64_C( 0xFFFFFFFFFFFFFFFF ) / 1000000000 );
			AirTunesTime_Sub( &gAirTunesClock_EpochTime, &at );
		}
		else
		{
			offset  = (uint64_t) inOffsetNanoseconds;
			at.secs = (int32_t)( offset / 1000000000 );
			at.frac = ( offset % 1000000000 ) * ( UINT64_C( 0xFFFFFFFFFFFFFFFF ) / 1000000000 );
			AirTunesTime_Add( &gAirTunesClock_EpochTime, &at );
		}
		pthread_mutex_unlock( &gAirTunesClock_Lock );
		
		_AirTunesClock_Tick();
		inReset = true;
	}
	else
	{
		int32_t		offset;
		int32_t		mtemp;
		Fixed64		ftemp;
		
		pthread_mutex_lock( &gAirTunesClock_Lock );
		
		// Use a phase-lock loop (PLL) to update the time and frequency offset estimates.
		
		offset = (int32_t) inOffsetNanoseconds;
		if(      offset >  kAirTunesClock_MaxPhase )	gAirTunesClock_LastOffset = kAirTunesClock_MaxPhase;
		else if( offset < -kAirTunesClock_MaxPhase )	gAirTunesClock_LastOffset = -kAirTunesClock_MaxPhase;
		else											gAirTunesClock_LastOffset = offset;
		Fixed64_SetInteger( gAirTunesClock_Offset, gAirTunesClock_LastOffset );
		
		if( gAirTunesClock_LastAdjustTime == 0 )
		{
			gAirTunesClock_LastAdjustTime = gAirTunesClock_Second;
		}
		mtemp = gAirTunesClock_Second - gAirTunesClock_LastAdjustTime;
		Fixed64_SetInteger( ftemp, gAirTunesClock_LastOffset );
		Fixed64_RightShift( ftemp, ( kAirTunesClock_PLLShift + 2 ) << 1 );
		Fixed64_Multiply( ftemp, mtemp );
		Fixed64_Add( gAirTunesClock_FrequencyOffset, ftemp );
		gAirTunesClock_LastAdjustTime = gAirTunesClock_Second;
		if( Fixed64_GetInteger( gAirTunesClock_FrequencyOffset ) > kAirTunesClock_MaxFrequency )
		{
			Fixed64_SetInteger( gAirTunesClock_FrequencyOffset, kAirTunesClock_MaxFrequency );
		}
		else if( Fixed64_GetInteger( gAirTunesClock_FrequencyOffset ) < -kAirTunesClock_MaxFrequency )
		{
			Fixed64_SetInteger( gAirTunesClock_FrequencyOffset, -kAirTunesClock_MaxFrequency );
		}
		
		pthread_mutex_unlock( &gAirTunesClock_Lock );
	}
	return( inReset );
}

//===========================================================================================================================
//	AirTunesClock_GetSynchronizedNTPTime
//===========================================================================================================================

uint64_t	AirTunesClock_GetSynchronizedNTPTime( void )
{
	AirTunesTime		t;
	
	AirTunesClock_GetSynchronizedTime( &t );
	return( AirTunesTime_ToNTP( &t ) );
}

//===========================================================================================================================
//	AirTunesClock_GetSynchronizedTime
//===========================================================================================================================

void	AirTunesClock_GetSynchronizedTime( AirTunesTime *outTime )
{
	pthread_mutex_lock( &gAirTunesClock_Lock );
		*outTime = gAirTunesClock_UpTime;
		AirTunesTime_AddFrac( outTime, gAirTunesClock_Scale * ( UpTicks32() - gAirTunesClock_LastCount ) );
		AirTunesTime_Add( outTime, &gAirTunesClock_EpochTime );
	pthread_mutex_unlock( &gAirTunesClock_Lock );
}

//===========================================================================================================================
//	AirTunesClock_GetSynchronizedTimeNearUpTicks
//===========================================================================================================================

void	AirTunesClock_GetSynchronizedTimeNearUpTicks( AirTunesTime *outTime, uint64_t inTicks )
{
	uint64_t			nowTicks;
	uint32_t			nowTicks32;
	uint64_t			deltaTicks;
	AirTunesTime		deltaTime;
	uint64_t			scale;
	Boolean				future;
	
	nowTicks	= UpTicks();
	nowTicks32	= ( (uint32_t)( nowTicks & UINT32_C( 0xFFFFFFFF ) ) );
	future		= ( inTicks > nowTicks );
	deltaTicks	= future ? ( inTicks - nowTicks ) : ( nowTicks - inTicks );
	
	pthread_mutex_lock( &gAirTunesClock_Lock );
		*outTime = gAirTunesClock_UpTime;
		AirTunesTime_AddFrac( outTime, gAirTunesClock_Scale * ( nowTicks32 - gAirTunesClock_LastCount ) );
		AirTunesTime_Add( outTime, &gAirTunesClock_EpochTime );
		scale = gAirTunesClock_Scale;
	pthread_mutex_unlock( &gAirTunesClock_Lock );
	
	deltaTime.secs = (int32_t)( deltaTicks / gAirTunesClock_Frequency ); // Note: unscaled, but delta expected to be < 1 sec.
	deltaTime.frac = ( deltaTicks % gAirTunesClock_Frequency ) * scale;
	if( future )	AirTunesTime_Add( outTime, &deltaTime );
	else			AirTunesTime_Sub( outTime, &deltaTime );
}

//===========================================================================================================================
//	AirTunesClock_GetUpTicksNearSynchronizedNTPTime
//===========================================================================================================================

uint64_t	AirTunesClock_GetUpTicksNearSynchronizedNTPTime( uint64_t inNTPTime )
{
	uint64_t		nowNTP;
	uint64_t		ticks;
	
	nowNTP = AirTunesClock_GetSynchronizedNTPTime();
	if( inNTPTime >= nowNTP )	ticks = UpTicks() + NTPtoUpTicks( inNTPTime - nowNTP );
	else						ticks = UpTicks() - NTPtoUpTicks( nowNTP    - inNTPTime );
	return( ticks );
}

//===========================================================================================================================
//	AirTunesClock_GetUpTicksNearSynchronizedNTPTimeMid32
//===========================================================================================================================

uint64_t	AirTunesClock_GetUpTicksNearSynchronizedNTPTimeMid32( uint32_t inNTPMid32 )
{
	uint64_t		ntpA, ntpB;
	uint64_t		ticks;
	
	ntpA = AirTunesClock_GetSynchronizedNTPTime();
	ntpB = ( ntpA & UINT64_C( 0xFFFF000000000000 ) ) | ( ( (uint64_t) inNTPMid32 ) << 16 );
	if( ntpB >= ntpA )	ticks = UpTicks() + NTPtoUpTicks( ntpB - ntpA );
	else				ticks = UpTicks() - NTPtoUpTicks( ntpA - ntpB );
	return( ticks );
}

//===========================================================================================================================
//	_AirTunesClock_Tick
//===========================================================================================================================

DEBUG_STATIC void	_AirTunesClock_Tick( void )
{
	uint32_t			count;
	uint32_t			delta;
	AirTunesTime		at;
	int					recalc;
	
	pthread_mutex_lock( &gAirTunesClock_Lock );
	
	// Update the current uptime from the delta between now and the last update.
	
	count = UpTicks32();
	delta = count - gAirTunesClock_LastCount;
	gAirTunesClock_LastCount = count;
	AirTunesTime_AddFrac( &gAirTunesClock_UpTime, gAirTunesClock_Scale * delta );
	
	// Perform NTP adjustments each second.
	
	at = gAirTunesClock_UpTime;
	AirTunesTime_Add( &at, &gAirTunesClock_EpochTime );
	recalc = 0;
	if( at.secs > gAirTunesClock_LastTime.secs )
	{
		Fixed64		ftemp;
		
		ftemp = gAirTunesClock_Offset;
		Fixed64_RightShift( ftemp, kAirTunesClock_PLLShift );
		gAirTunesClock_TickAdjust = ftemp;
		Fixed64_Sub( gAirTunesClock_Offset, ftemp );
		Fixed64_Add( gAirTunesClock_TickAdjust, gAirTunesClock_FrequencyOffset );
		
		gAirTunesClock_Adjustment = gAirTunesClock_TickAdjust;
		recalc = 1;
	}
	
	// Recalculate the scaling factor. We want the number of 1/2^64 fractions of a second per period of 
	// the hardware counter, taking into account the adjustment factor which the NTP PLL processing 
	// provides us with. The adjustment is nanoseconds per second with 32 bit binary fraction and we want 
	// 64 bit binary fraction of second:
	//
	//		x = a * 2^32 / 10^9 = a * 4.294967296
	//
	// The range of adjustment is +/- 5000PPM so inside a 64 bit integer we can only multiply by about 
	// 850 without overflowing, that leaves no suitably precise fractions for multiply before divide.
	// Divide before multiply with a fraction of 2199/512 results in a systematic undercompensation of 
	// 10PPM. On a 5000 PPM adjustment this is a 0.05PPM error. This is acceptable.
	
	if( recalc )
	{
		uint64_t		scale;
		
		scale  = UINT64_C( 1 ) << 63;
		scale += ( gAirTunesClock_Adjustment / 1024 ) * 2199;
		scale /= gAirTunesClock_Frequency;
		gAirTunesClock_Scale = scale * 2;
	}
	
	gAirTunesClock_Second = at.secs;
	gAirTunesClock_LastTime = at;
	
	pthread_mutex_unlock( &gAirTunesClock_Lock );
}

//===========================================================================================================================
//	_AirTunesClock_Thread
//===========================================================================================================================

DEBUG_STATIC void *	_AirTunesClock_Thread( void *inArg )
{
	(void) inArg; // Unused
	
	pthread_setname_np_compat( "AirPlayClock" );
	
	while( gAirTunesClock_Running )
	{
		_AirTunesClock_Tick();
		usleep( 10000 );
	}
	return( NULL );
}
