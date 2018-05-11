/*
	Copyright (C) 2001-2013 Apple Inc. All Rights Reserved.
*/

#include "RandomNumberUtils.h"

#include "CommonServices.h"	// Include early for TARGET_*, etc. definitions.
#include "DebugServices.h"	// Include early for DEBUG_*, etc. definitions.

	#include <errno.h>
	#include <stdlib.h>

#if( TARGET_HAS_MOCANA_SSL )
	#include "Networking/SSL/mtypes.h"
	
	#include "Networking/SSL/merrors.h"
	#include "Networking/SSL/random.h"
#endif

#if( TARGET_HAS_MOCANA_SSL )
//===========================================================================================================================
//	RandomBytes
//===========================================================================================================================

static randomContext *		gRandomContext = NULL;

OSStatus	RandomBytes( void *inBuffer, size_t inByteCount )
{
	uint8_t * const		buf = (uint8_t *) inBuffer;
	OSStatus			err;
	
	if( gRandomContext == NULL )
	{
		err = RANDOM_acquireContext( &gRandomContext );
		require_noerr( err, exit );
	}
	
	err = RANDOM_numberGenerator( gRandomContext, buf, (sbyte4) inByteCount );
	require_noerr( err, exit );
	
exit:
	return( err );
}
#endif

//===========================================================================================================================
//	RandomString
//===========================================================================================================================

char *	RandomString( const char *inCharSet, size_t inCharSetSize, size_t inMinChars, size_t inMaxChars, char *outString )
{
	char *		ptr;
	char *		end;
	
	check( inMinChars <= inMaxChars );
	
	ptr = outString;
	end = ptr + ( inMinChars + ( Random32() % ( ( inMaxChars - inMinChars ) + 1 ) ) );
	while( ptr < end ) *ptr++ = inCharSet[ Random32() % inCharSetSize ];
	*ptr = '\0';
	return( outString );
}
