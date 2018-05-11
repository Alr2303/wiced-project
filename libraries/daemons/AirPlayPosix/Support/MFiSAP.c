/*
	Copyright (C) 2010-2013 Apple Inc. All Rights Reserved.
	
	Protocol:		Station-to-Station <http://en.wikipedia.org/wiki/Station-to-Station_protocol>.
	Key Exchange:	Curve25519 Elliptic-Curve Diffie-Hellman <http://cr.yp.to/ecdh.html>.
	Signing:		RSA <http://en.wikipedia.org/wiki/RSA#Signing_messages>.
	Encryption:		AES-128 in countermode <http://en.wikipedia.org/wiki/Advanced_Encryption_Standard>.
*/

#include "MFiSAP.h"

#include "AESUtils.h"
#include "curve25519-donna.h"
#include "DebugServices.h"
#include "RandomNumberUtils.h"
#include "TickUtils.h"

#include SHA_HEADER

//===========================================================================================================================
//	Constants
//===========================================================================================================================

// MFiSAPState

typedef uint8_t		MFiSAPState;
#define kMFiSAPState_Invalid		0 // Not initialized.
#define kMFiSAPState_Init			1 // Initialized and ready for either client or server.
#define kMFiSAPState_ClientM1		2 // Client ready to generate M1.
#define kMFiSAPState_ClientM2		3 // Client ready to process server's M2.
#define kMFiSAPState_ClientDone		4 // Client successfully completed exchange process.
#define kMFiSAPState_ServerM1		5 // Server ready to process client's M1 and generate server's M2.
#define kMFiSAPState_ServerDone		6 // Server successfully completed exchange process.

// Misc

#define kMFiSAP_AES_IV_SaltPtr		"AES-IV"
#define kMFiSAP_AES_IV_SaltLen		( sizeof( kMFiSAP_AES_IV_SaltPtr ) - 1 )

#define kMFiSAP_AES_KEY_SaltPtr		"AES-KEY"
#define kMFiSAP_AES_KEY_SaltLen		( sizeof( kMFiSAP_AES_KEY_SaltPtr ) - 1 )

#define kMFiSAP_AESKeyLen			16
#define kMFiSAP_ECDHKeyLen			32
#define kMFiSAP_VersionLen			1

//===========================================================================================================================
//	Structures
//===========================================================================================================================

struct MFiSAP
{
	MFiSAPState			state;
	uint8_t				version;
	uint8_t				sharedSecret[ kMFiSAP_ECDHKeyLen ];
	AES_CTR_Context		aesMasterContext;
	Boolean				aesMasterValid;
};

//===========================================================================================================================
//	Globals
//===========================================================================================================================

#if( MFI_SAP_ENABLE_SERVER && MFI_SAP_THROTTLE_SERVER )
	static uint64_t			gMFiSAP_LastTicks		= 0;
	static unsigned int		gMFiSAP_ThrottleCounter	= 0;
#endif

#if( MFI_SAP_ENABLE_SERVER && MFI_SAP_SERVER_FUNCTION_PTRS )
	static MFiPlatform_CreateSignature_f		gMFiPlatform_CreateSignature = NULL;
	static MFiPlatform_CopyCertificate_f		gMFiPlatform_CopyCertificate = NULL;
#endif

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

#if( MFI_SAP_ENABLE_SERVER )
	static OSStatus
		__MFiSAP_Exchange_ServerM1( 
			MFiSAPRef		inRef, 
			const uint8_t *	inInputPtr,
			size_t			inInputLen, 
			uint8_t **		outOutputPtr,
			size_t *		outOutputLen );
#endif

//===========================================================================================================================
//	MFiSAP_SetServerPlatformFunctions
//===========================================================================================================================

#if( MFI_SAP_ENABLE_SERVER && MFI_SAP_SERVER_FUNCTION_PTRS )
void
	MFiSAP_SetServerPlatformFunctions( 
		MFiPlatform_CreateSignature_f inCreateSignature, 
		MFiPlatform_CopyCertificate_f inCopyCertificate )
{
	gMFiPlatform_CreateSignature = inCreateSignature;
	gMFiPlatform_CopyCertificate = inCopyCertificate;
}
#endif

//===========================================================================================================================
//	MFiSAP_Create
//===========================================================================================================================

OSStatus	MFiSAP_Create( MFiSAPRef *outRef, uint8_t inVersion )
{
	OSStatus		err;
	MFiSAPRef		obj;
	
	require_action( inVersion == kMFiSAPVersion1, exit, err = kVersionErr );
	
	obj = (MFiSAPRef) calloc( 1, sizeof( *obj ) );
	require_action( obj, exit, err = kNoMemoryErr );
	
	obj->state   = kMFiSAPState_Init;
	obj->version = inVersion;
	
	*outRef = obj;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	MFiSAP_Delete
//===========================================================================================================================

void	MFiSAP_Delete( MFiSAPRef inRef )
{
	if( inRef->aesMasterValid )
	{
		AES_CTR_Final( &inRef->aesMasterContext );
	}
	memset( inRef, 0, sizeof( *inRef ) ); // Clear sensitive data and reset state.
	free( inRef );
}

//===========================================================================================================================
//	MFiSAP_Exchange
//===========================================================================================================================

OSStatus
	MFiSAP_Exchange( 
		MFiSAPRef		inRef, 
		const uint8_t *	inInputPtr,
		size_t			inInputLen, 
		uint8_t **		outOutputPtr,
		size_t *		outOutputLen, 
		Boolean *		outDone )
{
	OSStatus		err;
	Boolean			done = false;
	
	if( inRef->state == kMFiSAPState_Init )
	{
		inRef->state = ( inInputPtr == NULL ) ? kMFiSAPState_ClientM1 : kMFiSAPState_ServerM1;
	}
	switch( inRef->state )
	{
		
		#if( MFI_SAP_ENABLE_SERVER )
		case kMFiSAPState_ServerM1:
			err = __MFiSAP_Exchange_ServerM1( inRef, inInputPtr, inInputLen, outOutputPtr, outOutputLen );
			require_noerr( err, exit );
			done = true;
			break;
		#endif
		
		default:
			dlogassert( "bad state %d\n", inRef->state );
			err	= kStateErr;
			goto exit;
	}
	++inRef->state;
	*outDone = done;
	
exit:
	if( err ) inRef->state = kMFiSAPState_Invalid; // Invalidate on errors to prevent retries.
	return( err );
}

//===========================================================================================================================
//	__MFiSAP_Exchange_ClientM1
//
//	Client: Generate client start message.
//===========================================================================================================================

//===========================================================================================================================
//	__MFiSAP_Exchange_ClientM2
//
//	Client: verify server's response and generate M3.
//===========================================================================================================================

//===========================================================================================================================
//	__MFiSAP_Exchange_ServerM1
//
//	Server: process client's M1 and generate M2.
//===========================================================================================================================

#if( MFI_SAP_ENABLE_SERVER )
static OSStatus
	__MFiSAP_Exchange_ServerM1( 
		MFiSAPRef		inRef, 
		const uint8_t *	inInputPtr,
		size_t			inInputLen, 
		uint8_t **		outOutputPtr,
		size_t *		outOutputLen )
{
	OSStatus			err;
	const uint8_t *		inputEnd;
	const uint8_t *		clientECDHPublicKey;
	uint8_t				ourPrivateKey[ kMFiSAP_ECDHKeyLen ];
	uint8_t				ourPublicKey[ kMFiSAP_ECDHKeyLen ];
	SHA_CTX				sha1Context;
	uint8_t				digest[ 20 ];
	uint8_t *			signaturePtr = NULL;
	size_t				signatureLen;
	uint8_t *			certificatePtr = NULL;
	size_t				certificateLen;
	uint8_t				aesMasterKey[ kMFiSAP_AESKeyLen ];
	uint8_t				aesMasterIV[ kMFiSAP_AESKeyLen ];
	uint8_t *			buf;
	uint8_t *			dst;
	size_t				len;
	
	// Throttle requests to no more than 1 per second and linearly back off to 4 seconds max.
	
#if( MFI_SAP_THROTTLE_SERVER )
	if( ( UpTicks() - gMFiSAP_LastTicks ) < UpTicksPerSecond() )
	{
		if( gMFiSAP_ThrottleCounter < 4 ) ++gMFiSAP_ThrottleCounter;
		SleepForUpTicks( gMFiSAP_ThrottleCounter * UpTicksPerSecond() );
	}
	else
	{
		gMFiSAP_ThrottleCounter = 0;
	}
	gMFiSAP_LastTicks = UpTicks();
#endif
	
	// Validate inputs. Input data must be: <1:version> <32:client's ECDH public key>.
	
	inputEnd = inInputPtr + inInputLen;
	require_action( inputEnd > inInputPtr, exit, err = kSizeErr ); // Detect bad length causing ptr wrap.
	
	require_action( ( inputEnd - inInputPtr ) >= kMFiSAP_VersionLen, exit, err = kSizeErr );
	inRef->version = *inInputPtr++;
	require_action( inRef->version == kMFiSAPVersion1, exit, err = kVersionErr );
	
	require_action( ( inputEnd - inInputPtr ) >= kMFiSAP_ECDHKeyLen, exit, err = kSizeErr );
	clientECDHPublicKey = inInputPtr;
	inInputPtr += kMFiSAP_ECDHKeyLen;
	
	require_action( inInputPtr == inputEnd, exit, err = kSizeErr );
	
	// Generate a random ECDH key pair.
	
	err = RandomBytes( ourPrivateKey, sizeof( ourPrivateKey ) );
	require_noerr( err, exit );
	curve25519_donna( ourPublicKey, ourPrivateKey, NULL );
	
	// Use our private key and the client's public key to generate the shared secret.
	// Hash the shared secret and truncate it to form the AES master key.
	// Hash the shared secret with salt to derive the AES master IV.
	
	curve25519_donna( inRef->sharedSecret, ourPrivateKey, clientECDHPublicKey );
	SHA1_Init( &sha1Context );
	SHA1_Update( &sha1Context, kMFiSAP_AES_KEY_SaltPtr, kMFiSAP_AES_KEY_SaltLen );
	SHA1_Update( &sha1Context, inRef->sharedSecret, sizeof( inRef->sharedSecret ) );
	SHA1_Final( digest, &sha1Context );
	memcpy( aesMasterKey, digest, sizeof( aesMasterKey ) );
	
	SHA1_Init( &sha1Context );
	SHA1_Update( &sha1Context, kMFiSAP_AES_IV_SaltPtr, kMFiSAP_AES_IV_SaltLen );
	SHA1_Update( &sha1Context, inRef->sharedSecret, sizeof( inRef->sharedSecret ) );
	SHA1_Final( digest, &sha1Context );
	memcpy( aesMasterIV, digest, sizeof( aesMasterIV ) );
	
	// Use the auth chip to sign a hash of <32:our ECDH public key> <32:client's ECDH public key>.
	// And copy the auth chip's certificate so the client can verify the signature.
	
	SHA1_Init( &sha1Context );
	SHA1_Update( &sha1Context, ourPublicKey, sizeof( ourPublicKey ) );
	SHA1_Update( &sha1Context, clientECDHPublicKey, kMFiSAP_ECDHKeyLen );
	SHA1_Final( digest, &sha1Context );
#if( MFI_SAP_SERVER_FUNCTION_PTRS )
	require_action( gMFiPlatform_CreateSignature, exit, err = kNotPreparedErr );
	err = gMFiPlatform_CreateSignature( digest, sizeof( digest ), &signaturePtr, &signatureLen );
	require_noerr( err, exit );
#else
	err = MFiPlatform_CreateSignature( digest, sizeof( digest ), &signaturePtr, &signatureLen );
	require_noerr( err, exit );
#endif
	
#if( MFI_SAP_SERVER_FUNCTION_PTRS )
	require_action( gMFiPlatform_CopyCertificate, exit, err = kNotPreparedErr );
	err = gMFiPlatform_CopyCertificate( &certificatePtr, &certificateLen );
	require_noerr( err, exit );
#else
	err = MFiPlatform_CopyCertificate( &certificatePtr, &certificateLen );
	require_noerr( err, exit );
#endif
	
	// Encrypt the signature with the AES master key and master IV.
	
	err = AES_CTR_Init( &inRef->aesMasterContext, aesMasterKey, aesMasterIV );
	require_noerr( err, exit );
	
	err = AES_CTR_Update( &inRef->aesMasterContext, signaturePtr, signatureLen, signaturePtr );
	if( err ) AES_CTR_Final( &inRef->aesMasterContext );
	require_noerr( err, exit );
	
	inRef->aesMasterValid = true;
	
	// Return the response:
	//
	//		<32:our ECDH public key>
	//		<4:big endian certificate length>
	//		<N:certificate data>
	//		<4:big endian signature length>
	//		<N:encrypted signature data>
	
	len = kMFiSAP_ECDHKeyLen + 4 + certificateLen + 4 + signatureLen;
	buf = (uint8_t *) malloc( len );
	require_action( buf, exit, err = kNoMemoryErr );
	dst = buf;
	
	memcpy( dst, ourPublicKey, sizeof( ourPublicKey ) );
	dst += sizeof( ourPublicKey );
	
	*dst++ = (uint8_t)( ( certificateLen >> 24 ) & 0xFF );
	*dst++ = (uint8_t)( ( certificateLen >> 16 ) & 0xFF );
	*dst++ = (uint8_t)( ( certificateLen >>  8 ) & 0xFF );
	*dst++ = (uint8_t)(   certificateLen         & 0xFF );
	memcpy( dst, certificatePtr, certificateLen );
	dst += certificateLen;
	
	*dst++ = (uint8_t)( ( signatureLen >> 24 ) & 0xFF );
	*dst++ = (uint8_t)( ( signatureLen >> 16 ) & 0xFF );
	*dst++ = (uint8_t)( ( signatureLen >>  8 ) & 0xFF );
	*dst++ = (uint8_t)(   signatureLen         & 0xFF );
	memcpy( dst, signaturePtr, signatureLen );
	dst += signatureLen;
	
	check( dst == ( buf + len ) );
	*outOutputPtr = buf;
	*outOutputLen = (size_t)( dst - buf );
	
exit:
	if( certificatePtr )	free( certificatePtr );
	if( signaturePtr )		free( signaturePtr );
	return( err );
}
#endif // MFI_SAP_ENABLE_SERVER

//===========================================================================================================================
//	MFiSAP_Encrypt
//===========================================================================================================================

OSStatus	MFiSAP_Encrypt( MFiSAPRef inRef, const void *inPlaintext, size_t inLen, void *inCiphertextBuf )
{
	OSStatus		err;
	
	require_action( ( inRef->state == kMFiSAPState_ClientDone ) || 
					( inRef->state == kMFiSAPState_ServerDone ), 
					exit, err = kStateErr );
	
	err = AES_CTR_Update( &inRef->aesMasterContext, inPlaintext, inLen, inCiphertextBuf );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	MFiSAP_Decrypt
//===========================================================================================================================

OSStatus	MFiSAP_Decrypt( MFiSAPRef inRef, const void *inCiphertext, size_t inLen, void *inPlaintextBuf )
{
	OSStatus		err;
	
	require_action( ( inRef->state == kMFiSAPState_ClientDone ) || 
					( inRef->state == kMFiSAPState_ServerDone ), 
					exit, err = kStateErr );
	
	err = AES_CTR_Update( &inRef->aesMasterContext, inCiphertext, inLen, inPlaintextBuf );
	require_noerr( err, exit );
	
exit:
	return( err );
}

//===========================================================================================================================
//	MFiSAP_DeriveAESKey
//===========================================================================================================================

OSStatus
	MFiSAP_DeriveAESKey(
		MFiSAPRef		inRef, 
		const void *	inKeySaltPtr, 
		size_t			inKeySaltLen, 
		const void *	inIVSaltPtr, 
		size_t			inIVSaltLen, 
		uint8_t			outKey[ 16 ], 
		uint8_t			outIV[ 16 ] )
{
	OSStatus		err;
	SHA512_CTX		shaCtx;
	uint8_t			buf[ 64 ];
	
	require_action( ( inRef->state == kMFiSAPState_ClientDone ) || 
					( inRef->state == kMFiSAPState_ServerDone ), 
					exit, err = kStateErr );
	if( outKey )
	{
		SHA512_Init( &shaCtx );
		SHA512_Update( &shaCtx, inKeySaltPtr, inKeySaltLen );
		SHA512_Update( &shaCtx, inRef->sharedSecret, 32 );
		SHA512_Final( buf, &shaCtx );
		memcpy( outKey, buf, 16 );
	}
	if( outIV )
	{
		SHA512_Init( &shaCtx );
		SHA512_Update( &shaCtx, inIVSaltPtr, inIVSaltLen );
		SHA512_Update( &shaCtx, inRef->sharedSecret, 32 );
		SHA512_Final( buf, &shaCtx );
		memcpy( outIV, buf, 16 );
	}
	err = kNoErr;
	
exit:
	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	MFiClient_Test
//===========================================================================================================================

//===========================================================================================================================
//	MFiSAP_Test
//===========================================================================================================================

