/*
	Copyright (C) 2010-2013 Apple Inc. All Rights Reserved.
*/

#include "ThreadUtils.h"

//===========================================================================================================================
//	GetMachThreadPriority
//===========================================================================================================================

//===========================================================================================================================
//	SetThreadPriority
//===========================================================================================================================

OSStatus	SetThreadPriority( int inPriority )
{
	OSStatus		err;
	
	if( inPriority == kThreadPriority_TimeConstraint )
	{
			dlogassert( "Platform doesn't support time constraint threads" );
			err = kUnsupportedErr;
			goto exit;
	}
	else
	{
		int						policy;
		struct sched_param		sched;
		
		err = pthread_getschedparam( pthread_self(), &policy, &sched );
		require_noerr( err, exit );
		
		sched.sched_priority = inPriority;
		err = pthread_setschedparam( pthread_self(), SCHED_FIFO, &sched );
		require_noerr( err, exit );
	}
	
exit:
	return( err );
}
