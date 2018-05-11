/*
	Copyright (C) 2001-2013 Apple Inc. All Rights Reserved.
*/
/*!
    @header		Tick API
    @discussion APIs for providing a high-resolution, low-latency tick counter and conversions.
*/

#ifndef __TickUtils_h__
#define	__TickUtils_h__

#if( defined( TickUtils_PLATFORM_HEADER ) )
	#include  TickUtils_PLATFORM_HEADER
#endif

#include "CommonServices.h"	// Include early for TARGET_* conditionals.
#include "DebugServices.h"	// Include early for DEBUG_* conditionals.

	#include <time.h>

	#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		Tick counter access.
	@abstract	Provides access to the raw tick counter, ticks per second, and convenience functions for common units.
	@discussion
	
	If your platform is not already supported then you can implement UpTicks() and UpTicksPerSecond() in your own file
	and link it in. Alternatively, if you have an existing API and want to avoid the overhead of wrapping your function 
	then you can define TickUtils_PLATFORM_HEADER to point to your custom header file and inside that file you can 
	define UpTicks() and UpTicksPerSecond() to point to your existing functions. This assumes they are API compatible.
	
	Implementors of these APIs must be careful to avoid temporary integer overflows. Even with 64-bit values, it's 
	easy to exceed the range of a 64-bit value when conversion to/from very small units or very large counts.
*/
//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	UpTicks
	@abstract	Monotonically increasing number of ticks since the system started.
	@discussion	This function returns current value of a monotonically increasing tick counter.
*/
	uint64_t	UpTicks( void );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	UpTicksPerSecond
	@abstract	Number of ticks per second
	@discussion	This function returns number of ticks per second
*/
	uint64_t	UpTicksPerSecond( void );

#define UpTicks32()					( (uint32_t)( UpTicks() & UINT32_C( 0xFFFFFFFF ) ) )

#if( !TickUtils_CONVERSION_OVERRIDES )
	#define UpSeconds()				UpTicksToSeconds( UpTicks() )
	#define UpMilliseconds()		UpTicksToMilliseconds( UpTicks() )
	#define UpMicroseconds()		UpTicksToMicroseconds( UpTicks() )
	#define UpNanoseconds()			UpTicksToNanoseconds( UpTicks() )
#endif

#define UpNTP()		UpTicksToNTP( UpTicks() )

	void	SleepForUpTicks( uint64_t inTicks );
	void	SleepUntilUpTicks( uint64_t inTicks );
	#define TickUtils_HAS_SLEEP_PRIMITIVE		1

#define kUpTicksNow			UINT64_C( 0 )
#define kUpTicksForever		UINT64_C( 0xFFFFFFFFFFFFFFFF )

uint64_t	UpTicksToSeconds( uint64_t inTicks );
uint64_t	UpTicksToMilliseconds( uint64_t inTicks );
uint64_t	UpTicksToMicroseconds( uint64_t inTicks );
uint64_t	UpTicksToNanoseconds( uint64_t inTicks );
uint64_t	UpTicksToNTP( uint64_t inTicks );

uint64_t	SecondsToUpTicks( uint64_t x );
uint64_t	MillisecondsToUpTicks( uint64_t x );
uint64_t	MicrosecondsToUpTicks( uint64_t x );
uint64_t	NanosecondsToUpTicks( uint64_t x );
uint64_t	NTPtoUpTicks( uint64_t inNTP );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	UpTicksToTimeValTimeout
	@internal
	@abstract	Converts an absolute, UpTicks deadline to a timeval timeout, suitable for passing to APIs like select.
	@discussion	This handles deadlines that have already expired (immediate timeout) and kUpTicksForever (no timeout).
*/
struct timeval *	UpTicksToTimeValTimeout( uint64_t inDeadline, struct timeval *inTimeVal );

#ifdef __cplusplus
}
#endif

#endif 	// __TickUtils_h__
