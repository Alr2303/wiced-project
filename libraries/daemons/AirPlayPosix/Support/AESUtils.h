/*
	Copyright (C) 2007-2013 Apple Inc. All Rights Reserved.
*/
/*!
	@header			AES API
	@discussion		APIs and platform interfaces for using the Advanced Encryption Standard (AES).
	
	Support is provided for the following cryptographic libraries:
	
	- AES reference implementation (rijndael-alg-*.c).
	- Apple's CommonCrypto.
	- Cypherbridge's uSSL.
	- Brian Gladman's AES.
	- OpenSSL.
	
	If one of these libraries is not available, these APIs will need to be implemented for your platform.
*/

#ifndef	__AESUtils_h__
#define	__AESUtils_h__

#include "CommonServices.h"
#include "DebugServices.h"

// AES_UTILS_USE_COMMON_CRYPTO: 1=Use CommonCrypto. 0=Use some other crypto library.

#if( !defined( AES_UTILS_USE_COMMON_CRYPTO ) )
		#define AES_UTILS_USE_COMMON_CRYPTO		0
#endif

// AES_UTILS_USE_USSL: Use Cypherbridge's uSSL crypto library

#if( !defined( AES_UTILS_USE_USSL ) )
		#define AES_UTILS_USE_USSL		0
#endif

// AES_UTILS_HAS_GCM

#if( !defined( AES_UTILS_HAS_COMMON_CRYPTO_GCM ) )
		#define AES_UTILS_HAS_COMMON_CRYPTO_GCM		0
#endif

#if( !defined( AES_UTILS_HAS_GLADMAN_GCM ) )
	#if( __has_include( "gcm.h" ) )
		#define AES_UTILS_HAS_GLADMAN_GCM		1
	#else
		#define AES_UTILS_HAS_GLADMAN_GCM		0
	#endif
#endif

#if( AES_UTILS_HAS_COMMON_CRYPTO_GCM || AES_UTILS_HAS_GLADMAN_GCM )
	#define AES_UTILS_HAS_GCM		1
#endif

#if( !AES_UTILS_HAS_COMMON_CRYPTO_GCM && AES_UTILS_HAS_GLADMAN_GCM )
	#include "gcm.h"
#endif

// Compatibility.

#if( AES_UTILS_USE_COMMON_CRYPTO )
	#include <CommonCrypto/CommonCryptor.h>
#elif( AES_UTILS_USE_GLADMAN_AES )
	#include "aes.h"
#elif( AES_UTILS_USE_USSL )
	#include "wiced_security.h"
#elif( !TARGET_NO_OPENSSL )
	#include <openssl/aes.h>
#else
	
	// Emulate the OpenSSL API with the rijndael-alg-fst.c API.
	
	#define AES_ENCRYPT			1
	#define AES_DECRYPT			0
	
	#define AES_BLOCK_SIZE		16
	
	typedef struct
	{
		uint32_t		key[ 4 * ( 14 + 1 ) ]; // Up to 14 rounds.
		
	}	AES_KEY;
	
	extern void rijndaelKeySetupEnc(uint32_t rk[/*44*/], const uint8_t cipherKey[]);
	extern void rijndaelKeySetupDec(uint32_t rk[/*44*/], const uint8_t cipherKey[]);
	extern void rijndaelEncrypt(const uint32_t rk[/*44*/], const uint8_t pt[16], uint8_t ct[16]);
	extern void rijndaelDecrypt(const uint32_t rk[/*44*/], const uint8_t ct[16], uint8_t pt[16]);
	
	STATIC_INLINE int AES_set_encrypt_key( const unsigned char *inUserKey, const int inBits, AES_KEY *inKey )
	{
		(void) inBits; // Assumes 128-bit.
		
		rijndaelKeySetupEnc( inKey->key, inUserKey );
		return( 0 );	
	}
	
	STATIC_INLINE int AES_set_decrypt_key( const unsigned char *inUserKey, const int inBits, AES_KEY *inKey )
	{
		(void) inBits; // Assumes 128-bit.
		
		rijndaelKeySetupDec( inKey->key, inUserKey );
		return( 0 );	
	}
	
	STATIC_INLINE void AES_encrypt( const unsigned char *inPlain, unsigned char *outCrypt, const AES_KEY *inKey )
	{
		rijndaelEncrypt( inKey->key, inPlain, outCrypt );
	}
	
	STATIC_INLINE void AES_decrypt( const unsigned char *inCrypt, unsigned char *outPlain, const AES_KEY *inKey )
	{
		rijndaelDecrypt( inKey->key, inCrypt, outPlain );
	}
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#pragma mark -
#pragma mark == AES-CTR ==
#endif

//===========================================================================================================================
/*!	@group		AES 128-bit Counter Mode API
	@abstract	API to encrypt or decrypt using AES-128 in counter mode.
	@discussion
	
	Call AES_CTR_Init to initialize the context. Don't use the context until it has been initialized.
	Call AES_CTR_Update to encrypt or decrypt N bytes of input and generate N bytes of output.
	Call AES_CTR_Final to finalize the context. After finalizing, you must call AES_CTR_Init to use it again.
	
	See the unit test for an example of using it.
*/
#define kAES_CTR_Size		16

typedef struct
{
#if( AES_UTILS_USE_COMMON_CRYPTO )
	CCCryptorRef		cryptor;				//! PRIVATE: Internal CommonCrypto ref.
#elif( AES_UTILS_USE_GLADMAN_AES )
	aes_encrypt_ctx		ctx;					//! PRIVATE: Gladman AES context.
#elif( AES_UTILS_USE_USSL )
	aes_context 		ctx;					//! PRIVATE: uSSL AES context.
#else
	AES_KEY				key;					//! PRIVATE: Internal AES key.
#endif
	uint8_t				ctr[ kAES_CTR_Size ];	//! PRIVATE: Big endian counter.
	uint8_t				buf[ kAES_CTR_Size ];	//! PRIVATE: Keystream buffer.
	size_t				used;					//! PRIVATE: Number of bytes of the keystream buffer that we've used.
	
	Boolean				legacy;					//! true=do legacy, chunked encrypting/decrypting.
	
}	AES_CTR_Context;

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AES_CTR_Init
	@abstract	Initializes a context for AES-128 in counter mode. Must be called before other AES_CTR_* functions.
	
	@param		inContext	Context to be initialized.
	@param		inKey		16-byte key material to be used.
	@param		inNonce		16-byte nonce/IV to be used. This must be chosen such that a key/nonce pair is never used twice.
	
	@result		kNoErr if successful or an error code indicating failure.
*/
OSStatus
	AES_CTR_Init( 
		AES_CTR_Context *	inContext, 
		const uint8_t		inKey[ kAES_CTR_Size ], 
		const uint8_t		inNonce[ kAES_CTR_Size ] );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AES_CTR_Update
	@abstract	Encrypts or decrypts N bytes of data using AES-128 in counter mode.
	
	@param		inContext	Context previously initialized with AES_CTR_Init.
	@param		inSrc		Pointer to data to encrypt/decrypt. Must be at least inLen bytes.
	@param		inLen		Number of bytes to encrypt/decrypt.
	@param		inDst		Pointer to buffer where output data is stored. Must be at least inLen bytes.
							inDst may be equal to inSrc for in-place encryption/decryption, but they cannot otherwise overlap.
	
	@result		kNoErr if successful or an error code indicating failure.
*/
OSStatus	AES_CTR_Update( AES_CTR_Context *inContext, const void *inSrc, size_t inLen, void *inDst );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AES_CTR_Final
	@abstract	Finalizes a context for AES-128 in counter mode when no longer needed. Context must not be used after this.
	
	@param		inContext	Context to be finalized.
*/
void	AES_CTR_Final( AES_CTR_Context *inContext );

#if 0
#pragma mark -
#pragma mark == AES-CBC Frame ==
#endif

//===========================================================================================================================
/*!	@group		AES 128-bit CBC Frame Mode API
	@abstract	API to encrypt or decrypt using AES-128 in CBC frame mode.
	@discussion
	
	Call AES_CBCFrame_Init to initialize the context. Don't use the context until it has been initialized.
	Call AES_CBCFrame_Update to encrypt or decrypt N bytes of input and generate N bytes of output.
	Call AES_CBCFrame_Final to finalize the context. After finalizing, you must call AES_CBCFrame_Init to use it again.
	
	See the unit test for an example of using it.
*/
#define kAES_CBCFrame_Size		16

typedef struct
{
	// PRIVATE: don't touch any of these fields. Do everything with the API.
	
#if( AES_UTILS_USE_COMMON_CRYPTO )
	CCCryptorRef			cryptor;					//! PRIVATE: Internal CommonCrypto ref.
#elif( AES_UTILS_USE_GLADMAN_AES )
	union
	{
		aes_encrypt_ctx		encrypt;					//! PRIVATE: Gladman AES context for encryption.
		aes_decrypt_ctx		decrypt;					//! PRIVATE: Gladman AES context for decryption.
		
	}	ctx;
	int						encrypt;
#elif( AES_UTILS_USE_USSL )
	aes_context 			ctx;						//! PRIVATE: uSSL AES context.
	int						encrypt;
#else
	int						mode;						//! PRIVATE: AES_ENCRYPT or AES_DECRYPT.
	AES_KEY					key;						//! PRIVATE: Internal AES key.
#endif
	uint8_t					iv[ kAES_CBCFrame_Size ];	//! PRIVATE: Initialization vector.
	
}	AES_CBCFrame_Context;

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AES_CBCFrame_Init
	@abstract	Initializes a context for AES-128 in CBC frame mode. Must be called before other AES_CBCFrame_* functions.
	
	@param		inContext	Context to be initialized.
	@param		inKey		16-byte key material to be used.
	@param		inIV		16-byte initialization vector to be used.
	@param		inEncypt	true to encrypt, false to decrypt.
	
	@result		kNoErr if successful or an error code indicating failure.
*/
OSStatus
	AES_CBCFrame_Init( 
		AES_CBCFrame_Context *	inContext, 
		const uint8_t			inKey[ kAES_CBCFrame_Size ], 
		const uint8_t			inIV[ kAES_CBCFrame_Size ], 
		Boolean					inEncrypt );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AES_CBCFrame_Update
	@abstract	Encrypts or decrypts N bytes of data using AES-128 in CBC frame mode.
	
	@param		inContext	Context previously initialized with AES_CBCFrame_Init.
	@param		inSrc		Pointer to data to encrypt/decrypt. Must be at least inLen bytes.
	@param		inLen		Number of bytes to encrypt/decrypt.
	@param		inDst		Pointer to buffer where output data is stored. Must be at least inLen bytes.
							inDst may be equal to inSrc for in-place encryption/decryption, but they cannot otherwise overlap.
	
	@result		kNoErr if successful or an error code indicating failure.
*/
OSStatus	AES_CBCFrame_Update( AES_CBCFrame_Context *inContext, const void *inSrc, size_t inLen, void *inDst );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AES_CBCFrame_Update2
	@abstract	Encrypts or decrypts 2 chunnks of data, N bytes each using AES-128 in CBC frame mode.
	
	@param		inContext	Context previously initialized with AES_CBCFrame_Init.
	@param		inSrc1		Pointer to first chunk of data to encrypt/decrypt. Must be at least inLen1 bytes.
	@param		inLen1		Number of bytes to encrypt/decrypt from inSrc1.
	@param		inSrc2		Pointer to second chunk of data to encrypt/decrypt. Must be at least inLen2 bytes.
	@param		inLen2		Number of bytes to encrypt/decrypt from inSrc2.
	@param		inDst		Pointer to buffer where output data is stored. Must be at least inLen1 + inLen2 bytes.
	
	@result		kNoErr if successful or an error code indicating failure.
*/
OSStatus
	AES_CBCFrame_Update2( 
		AES_CBCFrame_Context *	inContext, 
		const void *			inSrc1, 
		size_t					inLen1, 
		const void *			inSrc2, 
		size_t					inLen2, 
		void *					inDst );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AES_CBCFrame_Final
	@abstract	Finalizes a context for AES-128 in CBC frame mode when no longer needed. Context must not be used after this.
	
	@param		inContext	Context to be finalized.
*/
void	AES_CBCFrame_Final( AES_CBCFrame_Context *inContext );

#if 0
#pragma mark -
#pragma mark == AES-ECB ==
#endif

#define kAES_ECB_Size		16

#if( AES_UTILS_USE_COMMON_CRYPTO )
	#define kAES_ECB_Mode_Encrypt		kCCEncrypt
	#define kAES_ECB_Mode_Decrypt		kCCDecrypt
#elif( AES_UTILS_USE_GLADMAN_AES )
	#define kAES_ECB_Mode_Encrypt		1
	#define kAES_ECB_Mode_Decrypt		0
#elif( AES_UTILS_USE_USSL )
	#define kAES_ECB_Mode_Encrypt		AES_ENCRYPT
	#define kAES_ECB_Mode_Decrypt		AES_DECRYPT
#else
	#define kAES_ECB_Mode_Encrypt		1
	#define kAES_ECB_Mode_Decrypt		0
	
	typedef void	( *AESCryptFunc )( const unsigned char *in, unsigned char *out, const AES_KEY *key );
#endif

typedef struct
{
#if( AES_UTILS_USE_COMMON_CRYPTO )
	CCCryptorRef		cryptor;		//! PRIVATE: Internal CommonCrypto ref.
#elif( AES_UTILS_USE_GLADMAN_AES )
	union
	{
		aes_encrypt_ctx		encrypt;	//! PRIVATE: Gladman AES context for encryption.
		aes_decrypt_ctx		decrypt;	//! PRIVATE: Gladman AES context for decryption.
		
	}	ctx;
	uint32_t				encrypt;
#elif( AES_UTILS_USE_USSL )
	aes_context 			ctx;		//! PRIVATE: uSSL AES context.
	uint32_t				mode;
#else
	AES_KEY				key;			//! PRIVATE: Internal AES key.
	AESCryptFunc		cryptFunc;		//! PRIVATE: Pointer to AES_encrypt to AES_decrypt.
#endif
	
}	AES_ECB_Context;

OSStatus	AES_ECB_Init( AES_ECB_Context *inContext, uint32_t inMode, const uint8_t inKey[ kAES_ECB_Size ] );
OSStatus	AES_ECB_Update( AES_ECB_Context *inContext, const void *inSrc, size_t inLen, void *inDst );
void		AES_ECB_Final( AES_ECB_Context *inContext );

#if 0
#pragma mark -
#pragma mark == AES-GCM ==
#endif

#if( AES_UTILS_HAS_GCM )

#define kAES_CGM_Size			16
#define kAES_CGM_Nonce_None		NULL // When passed to AES_GCM_Init it means the caller is using a per-message nonce.
#define kAES_CGM_Nonce_Auto		NULL // When passed to AES_GCM_Encrypt, it means use the internal, auto-incremented nonce.

typedef struct
{
#if( AES_UTILS_HAS_COMMON_CRYPTO_GCM )
	CCCryptorRef		cryptor;
#elif( AES_UTILS_HAS_GLADMAN_GCM )
	gcm_ctx				ctx;
#else
	#error "GCM enabled, but no implementation?"
#endif
	uint8_t				nonce[ kAES_CGM_Size ];
	
}	AES_GCM_Context;

OSStatus
	AES_GCM_Init( 
		AES_GCM_Context *	inContext, 
		const uint8_t		inKey[ kAES_CGM_Size ], 
		const uint8_t		inNonce[ kAES_CGM_Size ] ); // May be kAES_CGM_Nonce_None for per-message nonces.

void	AES_GCM_Final( AES_GCM_Context *inContext );

OSStatus	AES_GCM_InitMessage( AES_GCM_Context *inContext, const uint8_t *inNonce );
OSStatus	AES_GCM_FinalizeMessage( AES_GCM_Context *inContext, uint8_t outAuthTag[ kAES_CGM_Size ] );
OSStatus	AES_GCM_VerifyMessage( AES_GCM_Context *inContext, const uint8_t inAuthTag[ kAES_CGM_Size ] );

OSStatus	AES_GCM_AddAAD( AES_GCM_Context *inContext, const void *inPtr, size_t inLen );
OSStatus	AES_GCM_Encrypt( AES_GCM_Context *inContext, const void *inSrc, size_t inLen, void *inDst );
OSStatus	AES_GCM_Decrypt( AES_GCM_Context *inContext, const void *inSrc, size_t inLen, void *inDst );

#endif // AES_UTILS_HAS_GCM

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	AESUtils_Test
	@internal
	@abstract	Unit test.
*/

#ifdef __cplusplus
}
#endif

#endif	// __AESUtils_h__
