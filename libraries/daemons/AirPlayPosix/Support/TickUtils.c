/*
	Copyright (C) 2001-2012 Apple Inc. All Rights Reserved.
*/

#include "TickUtils.h"

#include "CommonServices.h" // Include early for TARGET_* conditionals.
#include "DebugServices.h"  // Include early for DEBUG* conditionals.

	#include <errno.h>
	
	#include <sys/time.h>

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	SleepForUpTicks
//===========================================================================================================================

void	SleepForUpTicks( uint64_t inTicks )
{
	uint64_t			ticksPerSec;
	struct timespec		ts, remaining;
	
	ticksPerSec = UpTicksPerSecond();
	ts.tv_sec   = (int32_t)(     inTicks / ticksPerSec );
	ts.tv_nsec  = (int32_t)( ( ( inTicks % ticksPerSec ) * kNanosecondsPerSecond ) / ticksPerSec );
	while( nanosleep( &ts, &remaining ) < 0 )
	{
		if( errno == EINTR )
		{
			ts = remaining;
		}
		else
		{
			dlogassert( "nanosleep( %d.%09d ) error: %#m", (int) ts.tv_sec, (int) ts.tv_nsec, errno );
			break;
		}
	}
}

//===========================================================================================================================
//	SleepUntilUpTicks
//===========================================================================================================================

#if( TickUtils_HAS_SLEEP_PRIMITIVE )
void	SleepUntilUpTicks( uint64_t inTicks )
{
	uint64_t		ticks;

	ticks = UpTicks();
	if( ticks < inTicks )
	{
		SleepForUpTicks( inTicks - ticks );
	}
}
#endif

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	UpTicksTo*
//===========================================================================================================================

uint64_t	UpTicksToSeconds( uint64_t inTicks )
{
	static double		sMultiplier = 0;
	
	if( sMultiplier == 0 ) sMultiplier = 1.0 / ( (double) UpTicksPerSecond() );
	return( (uint64_t)( ( (double) inTicks ) * sMultiplier ) );
}

uint64_t	UpTicksToMilliseconds( uint64_t inTicks )
{
	static double		sMultiplier = 0;
	
	if( sMultiplier == 0 ) sMultiplier = ( (double) kMillisecondsPerSecond ) / ( (double) UpTicksPerSecond() );
	return( (uint64_t)( ( (double) inTicks ) * sMultiplier ) );
}

uint64_t	UpTicksToMicroseconds( uint64_t inTicks )
{
	static double		sMultiplier = 0;
	
	if( sMultiplier == 0 ) sMultiplier = ( (double) kMicrosecondsPerSecond ) / ( (double) UpTicksPerSecond() );
	return( (uint64_t)( ( (double) inTicks ) * sMultiplier ) );
}

uint64_t	UpTicksToNanoseconds( uint64_t inTicks )
{
	static double		sMultiplier = 0;
	
	if( sMultiplier == 0 ) sMultiplier = ( (double) kNanosecondsPerSecond ) / ( (double) UpTicksPerSecond() );
	return( (uint64_t)( ( (double) inTicks ) * sMultiplier ) );
}

uint64_t	UpTicksToNTP( uint64_t inTicks )
{
	uint64_t		ticksPerSec;
	
	ticksPerSec = UpTicksPerSecond();
	return( ( ( inTicks / ticksPerSec ) << 32 ) | ( ( ( inTicks % ticksPerSec ) << 32 ) / ticksPerSec ) );
}

//===========================================================================================================================
//	UpTicksToTimeValTimeout
//===========================================================================================================================

struct timeval *	UpTicksToTimeValTimeout( uint64_t inDeadlineTicks, struct timeval *inTimeVal )
{
	uint64_t		ticks;
	uint64_t		mics;
	
	if( inDeadlineTicks != kUpTicksForever )
	{
		ticks = UpTicks();
		if( inDeadlineTicks > ticks )
		{
			mics = UpTicksToMicroseconds( inDeadlineTicks - ticks );
			inTimeVal->tv_sec  = (int)( mics / 1000000 );
			inTimeVal->tv_usec = (int)( mics % 1000000 );
		}
		else
		{
			inTimeVal->tv_sec  = 0;
			inTimeVal->tv_usec = 0;
		}
		return( inTimeVal );
	}
	return( NULL );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	*ToUpTicks
//===========================================================================================================================

uint64_t	SecondsToUpTicks( uint64_t x )
{
	static uint64_t		sMultiplier = 0;
	
	if( sMultiplier == 0 ) sMultiplier = UpTicksPerSecond();
	return( x * sMultiplier );
}

uint64_t	MillisecondsToUpTicks( uint64_t x )
{
	static double		sMultiplier = 0;
	
	if( sMultiplier == 0 ) sMultiplier = ( (double) UpTicksPerSecond() ) / ( (double) kMillisecondsPerSecond );
	return( (uint64_t)( ( (double) x ) * sMultiplier ) );
}

uint64_t	MicrosecondsToUpTicks( uint64_t x )
{
	static double		sMultiplier = 0;
	
	if( sMultiplier == 0 ) sMultiplier = ( (double) UpTicksPerSecond() ) / ( (double) kMicrosecondsPerSecond );
	return( (uint64_t)( ( (double) x ) * sMultiplier ) );
}

uint64_t	NanosecondsToUpTicks( uint64_t x )
{
	static double		sMultiplier = 0;
	
	if( sMultiplier == 0 ) sMultiplier = ( (double) UpTicksPerSecond() ) / ( (double) kNanosecondsPerSecond );
	return( (uint64_t)( ( (double) x ) * sMultiplier ) );
}

uint64_t	NTPtoUpTicks( uint64_t inNTP )
{
	uint64_t		ticksPerSec, ticks;
	
	ticksPerSec = UpTicksPerSecond();
	ticks =      ( inNTP >> 32 )					* ticksPerSec;
	ticks += ( ( ( inNTP & UINT32_C( 0xFFFFFFFF ) ) * ticksPerSec ) >> 32 );
	return( ticks );
}
