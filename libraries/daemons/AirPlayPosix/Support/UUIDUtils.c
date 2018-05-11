/*
	Copyright (C) 2001-2013 Apple Inc. All Rights Reserved.
*/

#include "UUIDUtils.h"

#include "CommonServices.h"
#include "DebugServices.h"
#include "RandomNumberUtils.h"

//===========================================================================================================================
//	UUIDGet
//===========================================================================================================================

void	UUIDGet( void *outUUID )
{
	RandomBytes( outUUID, 16 );
	UUIDSetVersion( outUUID, kUUIDVersion_4_Random );
	UUIDMakeRFC4122( outUUID );
}

//===========================================================================================================================
//	UUIDGetHost
//===========================================================================================================================

void	UUIDGetHost( void *outUUID )
{
	(void) outUUID;
	
	dlogassert( "don't know how to get a host UUID for this platform" );
}

#if 0
#pragma mark -
#endif

