/*
	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
*/
	 
#ifndef __SHAUtils_h_
#define __SHAUtils_h_

#include "CommonServices.h"

#ifdef __cplusplus
extern "C" {
#endif

#if( TARGET_HAS_MOCANA_SSL )
	#include "mtypes.h"
	#include "merrors.h"
	#include "hw_accel.h"
	#include "sha1.h"
#elif( !TARGET_HAS_COMMON_CRYPTO && !TARGET_HAS_SHA_UTILS && !TARGET_NO_OPENSSL )
	#include <openssl/sha.h>
#endif

//===========================================================================================================================
//	SHA-1
//===========================================================================================================================

typedef struct
{
	uint64_t		length;
	uint32_t		state[ 5 ];
	uint32_t		curlen;
	uint8_t			buf[ 64 ];
	
}	SHA_CTX_compat;

int	SHA1_Init_compat( SHA_CTX_compat *ctx );
int	SHA1_Update_compat( SHA_CTX_compat *ctx, const void *inData, size_t inLen );
int	SHA1_Final_compat( unsigned char *outDigest, SHA_CTX_compat *ctx );
unsigned char *	SHA1_compat( const void *inData, size_t inLen, unsigned char *outDigest );

OSStatus	SHA1_Test( void );

//===========================================================================================================================
//	SHA-512
//===========================================================================================================================

typedef struct
{
	uint64_t  		length;
	uint64_t		state[ 8 ];
	size_t			curlen;
	uint8_t			buf[ 128 ];
	
}	SHA512_CTX_compat;

int	SHA512_Init_compat( SHA512_CTX_compat *ctx );
int	SHA512_Update_compat( SHA512_CTX_compat *ctx, const void *inData, size_t inLen );
int	SHA512_Final_compat( unsigned char *outDigest, SHA512_CTX_compat *ctx );
unsigned char *	SHA512_compat( const void *inData, size_t inLen, unsigned char *outDigest );

OSStatus	SHA512_Test( void );

//===========================================================================================================================
//	SHA-3 (Keccak)
//===========================================================================================================================

#define SHA3_DIGEST_LENGTH		64
#define SHA3_F					1600
#define SHA3_C					( SHA3_DIGEST_LENGTH * 8 * 2 )	// 512=1024
#define SHA3_R					( SHA3_F - SHA3_C )				// 512=576
#define SHA3_BLOCK_SIZE			( SHA3_R / 8 )

typedef struct
{
	uint64_t		state[ SHA3_F / 64 ];
	size_t			leftover;
	uint8_t			buffer[ SHA3_BLOCK_SIZE ];
	
}	SHA3_CTX_compat;

int	SHA3_Init_compat( SHA3_CTX_compat *ctx );
int	SHA3_Update_compat( SHA3_CTX_compat *ctx, const void *inData, size_t inLen );
int	SHA3_Final_compat( unsigned char *outDigest, SHA3_CTX_compat *ctx );
uint8_t *	SHA3_compat( const void *inData, size_t inLen, uint8_t outDigest[ 64 ] );

OSStatus	SHA3_Test( void );

#ifdef __cplusplus
}
#endif

#endif // __SHAUtils_h_
