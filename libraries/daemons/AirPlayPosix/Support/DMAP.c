/*
	Copyright (C) 2007-2009 Apple Inc. All Rights Reserved.
*/

#include "DMAP.h"

// Include the .i file multiple times with different options to define constants, etc.

#define DMAP_DEFINE_CONTENT_CODE_NAMES		1
#include "DMAP.i"

#define DMAP_DEFINE_CONTENT_CODE_TABLE		1
#include "DMAP.i"

//===========================================================================================================================
//	DMAPFindEntryByContentCode
//===========================================================================================================================

const DMAPContentCodeEntry *	DMAPFindEntryByContentCode( DMAPContentCode inCode )
{
	const DMAPContentCodeEntry *		e;
	
	for( e = kDMAPContentCodeTable; e->code != kDMAPUndefinedCode; ++e )
	{
		if( e->code == inCode )
		{
			return( e );
		}
	}
	return( NULL );
}
