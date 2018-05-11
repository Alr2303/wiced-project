/*
    File:    MFiSAP-WICED.c
    Package: WACServer
    Version: WAC_Server_Wiced

    Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
    capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
    Apple software is governed by and subject to the terms and conditions of your MFi License,
    including, but not limited to, the restrictions specified in the provision entitled ”Public
    Software”, and is further subject to your agreement to the following additional terms, and your
    agreement that the use, installation, modification or redistribution of this Apple software
    constitutes acceptance of these additional terms. If you do not agree with these additional terms,
    please do not use, install, modify or redistribute this Apple software.

    Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
    you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
    license, under Apple's copyrights in this original Apple software (the "Apple Software"), to use,
    reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
    redistribute the Apple Software, with or without modifications, in binary form. While you may not
    redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
    form, you must retain this notice and the following text and disclaimers in all such redistributions
    of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
    used to endorse or promote products derived from the Apple Software without specific prior written
    permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
    express or implied, are granted by Apple herein, including but not limited to any patent rights that
    may be infringed by your derivative works or by other works in which the Apple Software may be
    incorporated.

    Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
    fixes or enhancements to Apple in connection with this software (“Feedback”), you hereby grant to
    Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
    reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
    distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
    and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
    acknowledge and agree that Apple may exercise the license granted above without the payment of
    royalties or further consideration to Participant.

    The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
    IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
    AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
    IN COMBINATION WITH YOUR PRODUCTS.

    IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
    AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
    (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    Copyright (C) 2013 Apple Inc. All Rights Reserved.
*/

#include "MFiSAP.h"

#include "wwd_assert.h"
#include "wiced_rtos.h"
#include "wiced_security.h"
#include "wiced_time.h"
#include "wwd_debug.h"
#include "wiced_crypto.h"
#include "wiced_mfi.h"

#include "AssertMacros.h"

//===========================================================================================================================
//	Constants
//===========================================================================================================================

// MFiSAPState

typedef uint8_t		MFiSAPState;

#define kMFiSAPState_Invalid		0 // Not initialized.
#define kMFiSAPState_Init			1 // Intialized and ready for either client or server.
#define kMFiSAPState_ServerM1		5 // Server ready to process client's M1 and generate server's M2.
#define kMFiSAPState_ServerDone		6 // Server successfully completed exchange process.

// Misc

#define kMFiSAP_AES_IV_SaltPtr		"AES-IV"
#define kMFiSAP_AES_IV_SaltLen		( sizeof( kMFiSAP_AES_IV_SaltPtr ) - 1 )

#define kMFiSAP_AES_KEY_SaltPtr		"AES-KEY"
#define kMFiSAP_AES_KEY_SaltLen		( sizeof( kMFiSAP_AES_KEY_SaltPtr ) - 1 )

#define kMFiSAP_VersionLen			1
#define kMFiSAP_ECDHKeyLen			32
#define kMFiSAP_AESKeyLen			16

#define kAES_CTR_Size				16

#define require_action_wiced( X, LABEL, ACTION )   wiced_assert("error", X); require_action( X, LABEL, ACTION )
#define require_noerr_string_wiced( ERR, LABEL, STR )  wiced_assert("error", ERR==0); require_noerr_string( ERR, LABEL, STR )

//===========================================================================================================================
//	Structures
//===========================================================================================================================

typedef struct
{
	aes_context_t 		ctx;					//! PRIVATE: uSSL AES context.
	uint8_t				ctr[ kAES_CTR_Size ];	//! PRIVATE: Big endian counter.
	uint8_t				buf[ kAES_CTR_Size ];	//! PRIVATE: Keystream buffer.
	size_t				used;					//! PRIVATE: Number of bytes of the keystream buffer that we've used.

	wiced_bool_t		legacy;					//! true=do legacy, chunked encrypting/decrypting.

}	AES_CTR_Context;

struct MFiSAP
{
	MFiSAPState			state;
	uint8_t				version;
	uint8_t             sharedSecret[ kMFiSAP_ECDHKeyLen ];
	AES_CTR_Context		aesMasterContext;
	wiced_bool_t		aesMasterValid;
};

//===========================================================================================================================
//	Globals
//===========================================================================================================================

static wiced_time_t		gMFiSAP_LastTicks		= 0;
static unsigned int		gMFiSAP_ThrottleCounter	= 0;

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

static wiced_result_t
	_MFiSAP_Exchange_ServerM1(
		MFiSAPRef		inRef,
		const uint8_t *	inInputPtr,
		size_t			inInputLen,
		uint8_t **		outOutputPtr,
		size_t *		outOutputLen );

static wiced_result_t	_RandomBytes( uint8_t *inBuffer, size_t inByteCount );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		AES 128-bit Counter Mode SPI
	@abstract	SPI to encrypt or decrypt using AES-128 in counter mode.
	@discussion

	Call _AES_CTR_Init to initialize the context. Don't use the context until it has been initialized.
	Call _AES_CTR_Update to encrypt or decrypt N bytes of input and generate N bytes of output.
	Call _AES_CTR_Final to finalize the context. After finalizing, you must call _AES_CTR_Init to use it again.
*/
static wiced_result_t
	_AES_CTR_Init(
		AES_CTR_Context *	inContext,
		const uint8_t		inKey[ kAES_CTR_Size ],
		const uint8_t		inNonce[ kAES_CTR_Size ] );
static wiced_result_t 	_AES_CTR_Update( AES_CTR_Context *inContext, const void *inSrc, size_t inLen, void *inDst );
static void 			_AES_CTR_Final( AES_CTR_Context *inContext );

//===========================================================================================================================
//	MFiSAP_Create
//===========================================================================================================================

wiced_result_t	MFiSAP_Create( MFiSAPRef *outRef, uint8_t inVersion )
{
	wiced_result_t	err;
	MFiSAPRef		obj;

	require_action_wiced( inVersion == kMFiSAPVersion1, exit, err = WICED_WLAN_BAD_VERSION );

	obj = (MFiSAPRef) calloc( 1, sizeof( *obj ) );
	require_action_wiced( obj, exit, err = WICED_OUT_OF_HEAP_SPACE );

	obj->state   = kMFiSAPState_Init;
	obj->version = inVersion;

	*outRef = obj;
	err = WICED_SUCCESS;

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
		_AES_CTR_Final( &inRef->aesMasterContext );
	}
	memset( inRef, 0, sizeof( *inRef ) ); // Clear sensitive data and reset state.
	free( inRef );
}

//===========================================================================================================================
//	MFiSAP_Exchange
//===========================================================================================================================

wiced_result_t
	MFiSAP_Exchange(
		MFiSAPRef		inRef,
		const uint8_t *	inInputPtr,
		size_t			inInputLen,
		uint8_t **		outOutputPtr,
		size_t *		outOutputLen,
		wiced_bool_t *	outDone )
{
	wiced_result_t	err;
    uint8_t         temp_version;

	if ( inInputPtr == NULL )
	{
	    return WICED_BADARG;
	}

	if( inRef->state == kMFiSAPState_Init )
	{
        /* This is the initial case before the configuring device has sent any configuration packet */
		inRef->state = kMFiSAPState_ServerM1;
	}
    else if( inRef->state == kMFiSAPState_ServerDone )
    {
        /* This case occurs if the configuring device sends another configuration packet, i.e. a retry, which may happen if the AP is at long range */
        WPRINT_LIB_DEBUG(("MFiSAP: config retry\n"));
        temp_version = inRef->version;
        memset( inRef, 0, sizeof( *inRef ) );
        inRef->state = kMFiSAPState_ServerM1;
        inRef->version = temp_version;
    }

	switch( inRef->state )
	{
	    case kMFiSAPState_Invalid :
	    case kMFiSAPState_Init    :
	        err = WICED_ERROR;
	        break;
		case kMFiSAPState_ServerM1:
			err = _MFiSAP_Exchange_ServerM1( inRef, inInputPtr, inInputLen, outOutputPtr, outOutputLen );
			break;
		case kMFiSAPState_ServerDone:
            err  = WICED_SUCCESS;
		    break;
		default:
			WPRINT_LIB_ERROR(( "bad state %d\n", inRef->state ));
			err	= WICED_ERROR;
			break;
	}

    if( err != WICED_SUCCESS )
    {
        inRef->state = kMFiSAPState_Invalid; // Invalidate on errors to prevent retries.
        *outDone = WICED_FALSE;
        return WICED_ERROR;
    }

    ++inRef->state;
    *outDone = WICED_TRUE;

    return WICED_SUCCESS;

}

//===========================================================================================================================
//	_MFiSAP_Exchange_ServerM1
//
//	Server: process client's M1 and generate M2.
//===========================================================================================================================

static wiced_result_t
	_MFiSAP_Exchange_ServerM1(
		MFiSAPRef		inRef,
		const uint8_t *	inInputPtr,
		size_t			inInputLen,
		uint8_t **		outOutputPtr,
		size_t *		outOutputLen )
{
	wiced_result_t		err;
	const uint8_t *		inputEnd;
	const uint8_t *		clientECDHPublicKey;
	uint8_t				ourPrivateKey[ kMFiSAP_ECDHKeyLen ];
	uint8_t				ourPublicKey[ kMFiSAP_ECDHKeyLen ];
	sha1_context		sha1Context;
	uint8_t				digest[ 20 ];
	uint8_t *			signaturePtr;
	size_t				signatureLen;
	uint8_t *			certificatePtr;
	size_t				certificateLen;
	uint8_t				aesMasterKey[ kMFiSAP_AESKeyLen ];
	uint8_t				aesMasterIV[ kMFiSAP_AESKeyLen ];
	uint8_t *			buf;
	uint8_t *			dst;
	size_t				len;
	wiced_time_t		currentTime;

	signaturePtr   = NULL;
	certificatePtr = NULL;

	// Throttle requests to no more than 1 per second and linearly back off to 4 seconds max.

	wiced_time_get_time( &currentTime );
	if( ( currentTime - gMFiSAP_LastTicks ) < SECONDS )
	{
		if( gMFiSAP_ThrottleCounter < 4 ) ++gMFiSAP_ThrottleCounter;
		wiced_rtos_delay_milliseconds( gMFiSAP_ThrottleCounter * SECONDS );
	}
	else
	{
		gMFiSAP_ThrottleCounter = 0;
	}
	wiced_time_get_time( &gMFiSAP_LastTicks );

	// Validate inputs. Input data must be: <1:version> <32:client's ECDH public key>.

	inputEnd = inInputPtr + inInputLen;
	require_action_wiced( inputEnd > inInputPtr, exit, err = WICED_WLAN_BADLEN ); // Detect bad length causing ptr wrap.

	require_action_wiced( ( inputEnd - inInputPtr ) >= kMFiSAP_VersionLen, exit, err = WICED_WLAN_BUFTOOLONG );
	inRef->version = *inInputPtr++;
	require_action_wiced( inRef->version == kMFiSAPVersion1, exit, err = WICED_WLAN_BAD_VERSION );

	require_action_wiced( ( inputEnd - inInputPtr ) >= kMFiSAP_ECDHKeyLen, exit, err = WICED_WLAN_BUFTOOLONG );
	clientECDHPublicKey = inInputPtr;
	inInputPtr += kMFiSAP_ECDHKeyLen;

	require_action_wiced( inInputPtr == inputEnd, exit, err = WICED_WLAN_BADLEN );

	// Generate a random ECDH key pair.

	err = _RandomBytes( ourPrivateKey, sizeof( ourPrivateKey ) );
	require_noerr_string_wiced( err, exit, "random failure" );
	curve25519( ourPublicKey, ourPrivateKey, NULL );

	// Use our private key and the client's public key to generate the shared secret.
	// Hash the shared secret and truncate it to form the AES master key.
	// Hash the shared secret with salt to derive the AES master IV.

	curve25519( inRef->sharedSecret, ourPrivateKey, clientECDHPublicKey );
	sha1_starts( &sha1Context );
	sha1_update( &sha1Context, (unsigned char *)kMFiSAP_AES_KEY_SaltPtr, kMFiSAP_AES_KEY_SaltLen );
	sha1_update( &sha1Context, inRef->sharedSecret, sizeof( inRef->sharedSecret ) );
	sha1_finish( &sha1Context, digest );
	memcpy( aesMasterKey, digest, sizeof( aesMasterKey ) );

	sha1_starts( &sha1Context );
	sha1_update( &sha1Context, (unsigned char *)kMFiSAP_AES_IV_SaltPtr, kMFiSAP_AES_IV_SaltLen );
	sha1_update( &sha1Context, inRef->sharedSecret, sizeof( inRef->sharedSecret ) );
	sha1_finish( &sha1Context, digest );
	memcpy( aesMasterIV, digest, sizeof( aesMasterIV ) );

	// Use the auth chip to sign a hash of <32:our ECDH public key> <32:client's ECDH public key>.

	sha1_starts( &sha1Context );
	sha1_update( &sha1Context, ourPublicKey, sizeof( ourPublicKey ) );
	sha1_update( &sha1Context, (unsigned char *)clientECDHPublicKey, kMFiSAP_ECDHKeyLen );
	sha1_finish( &sha1Context, digest );
	err = MFiPlatform_CreateSignature( digest, sizeof( digest ), &signaturePtr, &signatureLen );
	require_noerr_string_wiced( err, exit, "sign failure" );

	// Copy the auth chip's certificate so the client can verify the signature.

	err = MFiPlatform_CopyCertificate( &certificatePtr, &certificateLen );
	require_noerr_string_wiced( err, exit, "copy cert failure" );

	// Encrypt the signature with the AES master key and master IV.

	err = _AES_CTR_Init( &inRef->aesMasterContext, aesMasterKey, aesMasterIV );
	require_noerr_string_wiced( err, exit, "aes init failure" );

	err = _AES_CTR_Update( &inRef->aesMasterContext, signaturePtr, signatureLen, signaturePtr );
	if( err ) _AES_CTR_Final( &inRef->aesMasterContext );
	require_noerr_string_wiced( err, exit, "aes encrypt failure" );

	inRef->aesMasterValid = WICED_TRUE;

	// Return the response:
	//
	//		<32:our ECDH public key>
	//		<4:big endian certificate length>
	//		<N:certificate data>
	//		<4:big endian signature length>
	//		<N:encrypted signature data>

	len = kMFiSAP_ECDHKeyLen + 4 + certificateLen + 4 + signatureLen;
	buf = (uint8_t *) malloc( len );
	require_action_wiced( buf, exit, err = WICED_OUT_OF_HEAP_SPACE );
	dst = buf;

	memcpy( dst, ourPublicKey, sizeof( ourPublicKey ) );
	dst += sizeof( ourPublicKey );

	*dst++ = (uint32_t)( ( certificateLen >> 24 ) & 0xFF );
	*dst++ = (uint32_t)( ( certificateLen >> 16 ) & 0xFF );
	*dst++ = (uint32_t)( ( certificateLen >>  8 ) & 0xFF );
	*dst++ = (uint32_t)(   certificateLen         & 0xFF );
	memcpy( dst, certificatePtr, certificateLen );
	dst += certificateLen;

	*dst++ = (uint32_t)( ( signatureLen >> 24 ) & 0xFF );
	*dst++ = (uint32_t)( ( signatureLen >> 16 ) & 0xFF );
	*dst++ = (uint32_t)( ( signatureLen >>  8 ) & 0xFF );
	*dst++ = (uint32_t)(   signatureLen         & 0xFF );
	memcpy( dst, signaturePtr, signatureLen );
	dst += signatureLen;

	check( dst == ( buf + len ) );
	*outOutputPtr = buf;
	*outOutputLen = (size_t)( dst - buf );

exit:
    MFiPlatform_DestroyCertificate( certificatePtr );
    MFiPlatform_DestroySignature( signaturePtr );
	return( err );
}

//===========================================================================================================================
//	MFiSAP_Encrypt
//===========================================================================================================================

wiced_result_t	MFiSAP_Encrypt( MFiSAPRef inRef, const void *inPlaintext, size_t inLen, void *inCiphertextBuf )
{
	wiced_result_t	err;

	require_action_wiced( ( inRef->state == kMFiSAPState_ServerDone ), exit, err = WICED_WLAN_NOTREADY );

	err = _AES_CTR_Update( &inRef->aesMasterContext, inPlaintext, inLen, inCiphertextBuf );
	require_noerr_string_wiced( err, exit, "aes encrypt failure" );

exit:
	return( err );
}

//===========================================================================================================================
//	MFiSAP_Decrypt
//===========================================================================================================================

wiced_result_t	MFiSAP_Decrypt( MFiSAPRef inRef, const void *inCiphertext, size_t inLen, void *inPlaintextBuf )
{
	wiced_result_t	err;

	require_action_wiced( ( inRef->state == kMFiSAPState_ServerDone ), exit, err = WICED_WLAN_NOTREADY );

	err = _AES_CTR_Update( &inRef->aesMasterContext, inCiphertext, inLen, inPlaintextBuf );
	require_noerr_string_wiced( err, exit, "aes decrypt failure" );

exit:
	return( err );
}

//===========================================================================================================================
//	_RandomBytes
//
//	Fills a buffer with cryptographically strong pseudo-random bytes.
//===========================================================================================================================

static wiced_result_t _RandomBytes( uint8_t *inBuffer, size_t inByteCount )
{
	return wiced_mfi_get_random_bytes( inBuffer, (uint16_t)inByteCount  );
}

//===========================================================================================================================
//	_AES_CTR_Init
//
//	Initialize the context. Don't use the context until it has been initialized.
//===========================================================================================================================

static wiced_result_t
	_AES_CTR_Init(
		AES_CTR_Context *	inContext,
		const uint8_t		inKey[ kAES_CTR_Size ],
		const uint8_t		inNonce[ kAES_CTR_Size ] )
{
	aes_setkey_enc( &inContext->ctx, (unsigned char *)inKey, kAES_CTR_Size * 8 );
	memcpy( inContext->ctr, inNonce, kAES_CTR_Size );
	inContext->used = 0;
	inContext->legacy = WICED_FALSE;
	return( WICED_SUCCESS );
}

//===========================================================================================================================
//	AES_CTR_Increment
//===========================================================================================================================

static __inline__ __attribute__( ( always_inline ) )
 	void AES_CTR_Increment( uint8_t *inCounter )
{
	int		i;

	// Note: counter is always big endian so this adds from right to left.

	for( i = kAES_CTR_Size - 1; i >= 0; --i )
	{
		if( ++( inCounter[ i ] ) != 0 )
		{
			break;
		}
	}
}

//===========================================================================================================================
//	_AES_CTR_Update
//
//	Encrypt or decrypt N bytes of input and generate N bytes of output.
//===========================================================================================================================

static wiced_result_t _AES_CTR_Update( AES_CTR_Context *inContext, const void *inSrc, size_t inLen, void *inDst )
{
	wiced_result_t		err;
	const uint8_t *		src;
	uint8_t *			dst;
	uint8_t *			buf;
	size_t				used;
	size_t				i;

	// inSrc and inDst may be the same, but otherwise, the buffers must not overlap.

#ifdef DEBUG
	if( inSrc != inDst ) check_ptr_overlap( inSrc, inLen, inDst, inLen );
#endif

	src = (const uint8_t *) inSrc;
	dst = (uint8_t *) inDst;

	// If there's any buffered key material from a previous block then use that first.

	buf  = inContext->buf;
	used = inContext->used;
	while( ( inLen > 0 ) && ( used != 0 ) )
	{
		*dst++ = *src++ ^ buf[ used++ ];
		used %= kAES_CTR_Size;
		inLen -= 1;
	}
	inContext->used = used;

	// Process whole blocks.

	while( inLen >= kAES_CTR_Size )
	{
		aes_crypt_ecb( &inContext->ctx, AES_ENCRYPT, inContext->ctr, buf );
		AES_CTR_Increment( inContext->ctr );

		for( i = 0; i < kAES_CTR_Size; ++i )
		{
			dst[ i ] = src[ i ] ^ buf[ i ];
		}
		src   += kAES_CTR_Size;
		dst   += kAES_CTR_Size;
		inLen -= kAES_CTR_Size;
	}

	// Process any trailing sub-block bytes. Extra key material is buffered for next time.

	if( inLen > 0 )
	{
		aes_crypt_ecb( &inContext->ctx, AES_ENCRYPT, inContext->ctr, buf );
		AES_CTR_Increment( inContext->ctr );

		for( i = 0; i < inLen; ++i )
		{
			*dst++ = *src++ ^ buf[ used++ ];
		}

		// For legacy mode, always leave the used amount as 0 so we always increment the counter each time.

		if( !inContext->legacy )
		{
			inContext->used = used;
		}
	}
	err = WICED_SUCCESS;

	return( err );
}

//===========================================================================================================================
//	_AES_CTR_Final
//
//	Finalize the context. After finalizing, you must call AES_CTR_Init to use it again.
//===========================================================================================================================

static void _AES_CTR_Final( AES_CTR_Context *inContext )
{
	memset( inContext, 0, sizeof( *inContext ) ); // Clear sensitive data.
}
