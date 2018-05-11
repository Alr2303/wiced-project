/*
	Copyright (C) 2007-2012 Apple Inc. All Rights Reserved.
*/
	 
#ifndef __MD5Utils_h__
#define __MD5Utils_h__

#include "CommonServices.h"

#ifdef __cplusplus
extern "C" {
#endif

#if  ( TARGET_HAS_MOCANA_SSL )
	#include "mtypes.h"
	#include "merrors.h"
	#include "hw_accel.h"
	#include "md5.h"
#elif( !TARGET_HAS_COMMON_CRYPTO_DIGEST )

	// OpenSSL-compatible mappings.

	#define MD5_CTX			MD5Context
	#define MD5_Init		MD5Init
	#define MD5_Update		MD5Update
	#define MD5_Final		MD5Final
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@struct		MD5Context
	@abstract	Structure used for context between MD5 calls.
*/

typedef struct
{
	uint32_t		buf[ 4 ];
	uint32_t		bytes[ 2 ];
	uint32_t		in[ 16 ];
	
}	MD5Context;

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MD5
	@abstract	Convenience routine to generate an MD5 from a single buffer of data.
*/

typedef void ( *MD5Func )( const void *inSourcePtr, size_t inSourceSize, uint8_t outKey[ 16 ] );

void	MD5OneShot( const void *inSourcePtr, size_t inSourceSize, uint8_t outKey[ 16 ] );
void	MD5OneShot_V1( const void *inSourcePtr, size_t inSourceSize, uint8_t outKey[ 16 ] );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MD5Init
	@abstract	Initializes the MD5 message digest.
*/

void	MD5Init( MD5Context *context );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MD5Update
	@abstract	Updates the MD5 message digest with the specified data.
*/

typedef void ( *MD5UpdateFunc )( MD5Context *context, void const *inBuf, size_t len );

void	MD5Update( MD5Context *context, void const *inBuf, size_t len );
void	MD5Update_V1( MD5Context *context, void const *inBuf, size_t len );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MD5Final
	@abstract	Finalizes and generates the resulting message digest.
*/

typedef void ( *MD5FinalFunc )( uint8_t digest[ 16 ], MD5Context *context );

void	MD5Final( uint8_t digest[ 16 ], MD5Context *context );
void	MD5Final_V1( uint8_t digest[ 16 ], MD5Context *context );

#if 0
#pragma mark == Debugging ==
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MD5UtilsTest
	@abstract	Unit test.
*/

#if( DEBUG )
	OSStatus	MD5UtilsTest( void );
#endif

#ifdef __cplusplus
}
#endif

#endif	// __MD5Utils_h__
