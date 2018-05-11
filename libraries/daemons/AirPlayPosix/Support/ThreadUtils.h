/*
	Copyright (C) 2009-2013 Apple Inc. All Rights Reserved.
*/

#ifndef __ThreadUtils_h__
#define __ThreadUtils_h__

#include "CommonServices.h"
#include "DebugServices.h"

	#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	pthread_setname_np_compat
	@abstract	Sets the name of the current thread.
*/

	#define pthread_setname_np_compat( NAME )				pthread_setname_np(0, NAME )

#if( DEBUG )
	#define debug_pthread_setname_np( NAME )				pthread_setname_np_compat( NAME )
#else
	#define debug_pthread_setname_np( NAME )				do {} while( 0 )
#endif
	
#define mach_policy_to_name( POLICY ) ( \
	( (POLICY) == POLICY_TIMESHARE )	? "POLICY_TIMESHARE"	: \
	( (POLICY) == POLICY_FIFO )			? "POLICY_FIFO"			: \
	( (POLICY) == POLICY_RR )			? "POLICY_RR"			: \
										  "<<UNKNOWN>>" )

#define pthread_policy_to_name( POLICY )		( \
	( (POLICY) == SCHED_OTHER )	? "SCHED_OTHER"	: \
	( (POLICY) == SCHED_FIFO )	? "SCHED_FIFO"	: \
	( (POLICY) == SCHED_RR )	? "SCHED_RR"	: \
								  "<<UNKNOWN>>" )
//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	SetThreadName
	@abstract	Sets the name of the current thread.
*/

	#define		SetThreadName( NAME )		pthread_setname_np_compat( NAME )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	GetThreadPriority / SetThreadPriority
	@abstract	Gets/sets the priority of the current thread.
*/

#define kThreadPriority_TimeConstraint		INT_MAX

int			GetMachThreadPriority( int *outPolicy, OSStatus *outErr );
OSStatus	SetThreadPriority( int inPriority );

#ifdef __cplusplus
}
#endif

#endif	// __ThreadUtils_h__
