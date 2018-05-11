/*
	Copyright (C) 2010-2013 Apple Inc. All Rights Reserved.
*/
/*!
	@header			MFi-SAP
	@discussion		APIs and platform interfaces for the Made for iPod (MFi) Security Association Protocol (SAP).

    This header contains function prototypes that must be implemented by the platform. These functions
    are called when the Apple software stack needs to interact with the MFi Authentication Coprocessor. Please refer to the relevant
    version of the "Auth IC" document to obtain more details on how to interact with the Authentication Coprocessor. This
    document can be found on the MFi Portal.
    
    @Copyright Apple, Inc.
*/

#ifndef	__MFiSAP_h__
#define	__MFiSAP_h__

#if( defined( MFiSAP_PLATFORM_HEADER ) )
	#include  MFiSAP_PLATFORM_HEADER
#endif

#include "CommonServices.h"
#include "DebugServices.h"

#ifdef __cplusplus
extern "C" {
#endif

//===========================================================================================================================
//	Configuration
//===========================================================================================================================

// MFI_SAP_ENABLE_SERVER: 1=Enable code for the server side of MFi-SAP. 0=Strip out server code.

#if( !defined( MFI_SAP_ENABLE_SERVER ) )
		#define MFI_SAP_ENABLE_SERVER		1
#endif

// MFI_SAP_THROTTLE_SERVER: 1=Enable throttling code for server side of MFi-SAP. 0=Don't throttle requests.

#if( !defined( MFI_SAP_THROTTLE_SERVER ) )
	#define MFI_SAP_THROTTLE_SERVER			1
#endif

// MFI_SAP_SERVER_FUNCTION_PTRS: 1=Set server platform functions manually. 0=Statically link them in by name.

#if( !defined( MFI_SAP_SERVER_FUNCTION_PTRS ) )
		#define MFI_SAP_SERVER_FUNCTION_PTRS	0
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		MFi-SAP 
	@internal
	@abstract	APIs for performing the Made for iPod (MFi) Security Association Protocol (SAP).
*/
#define kMFiSAPVersion1		1 // Curve25519 ECDH key exchange, RSA signing, AES-128 CTR encryption.

typedef struct MFiSAP *		MFiSAPRef;

OSStatus	MFiSAP_Create( MFiSAPRef *outRef, uint8_t inVersion );
void		MFiSAP_Delete( MFiSAPRef inRef );
#define		MFiSAP_Forget( X )	do { if( *(X) ) { MFiSAP_Delete( *(X) ); *(X) = NULL; } } while( 0 )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiSAP_Exchange
	@internal
	@abstract	Perform key exchange.
	@discussion
	
	This is called on both the client and server sides. Each side repeatedly cals MFiSAP_Exchange until it returns an 
	error to indicate a failure or it returns kNoErr and sets "done" to true.
*/
OSStatus
	MFiSAP_Exchange( 
		MFiSAPRef		inRef, 
		const uint8_t *	inInputPtr,
		size_t			inInputLen, 
		uint8_t **		outOutputPtr,
		size_t *		outOutputLen, 
		Boolean *		outDone );
OSStatus	MFiSAP_Encrypt( MFiSAPRef inRef, const void *inPlaintext,  size_t inLen, void *inCiphertextBuf );
OSStatus	MFiSAP_Decrypt( MFiSAPRef inRef, const void *inCiphertext, size_t inLen, void *inPlaintextBuf );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiSAP_DeriveAESKey
	@internal
	@abstract	Derive AES key and IV from the shared secret.
*/
OSStatus
	MFiSAP_DeriveAESKey(
		MFiSAPRef		inRef, 
		const void *	inKeySaltPtr, 
		size_t			inKeySaltLen, 
		const void *	inIVSaltPtr, 
		size_t			inIVSaltLen, 
		uint8_t			outKey[ 16 ], 
		uint8_t			outIV[ 16 ] );

#if( MFI_SAP_SERVER_FUNCTION_PTRS )
//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiSAP_SetServerPlatformFunctions
	@internal
	@abstract	Sets the server platform functions.
*/
typedef OSStatus
	( *MFiPlatform_CreateSignature_f )(
		const void *	inDigestPtr, 
		size_t			inDigestLen, 
		uint8_t **		outSignaturePtr,
		size_t *		outSignatureLen );

typedef OSStatus ( *MFiPlatform_CopyCertificate_f )( uint8_t **outCertificatePtr, size_t *outCertificateLen );

void
	MFiSAP_SetServerPlatformFunctions( 
		MFiPlatform_CreateSignature_f inCreateSignature, 
		MFiPlatform_CopyCertificate_f inCopyCertificate );
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiPlatform_Initialize
	@abstract	Performs any platform-specific initialization needed.
	@result		kNoErr if successful or an error code indicating failure.
	@discussion	MFi accessory implementors must provide this function.
*/
OSStatus	MFiPlatform_Initialize( void );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiPlatform_Finalize
	@abstract	Performs any platform-specific cleanup needed.
	@discussion	MFi accessory implementors must provide this function.
*/
void	MFiPlatform_Finalize( void );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiPlatform_CreateSignature
	@internal
	@abstract	Create an RSA signature from the specified SHA-1 digest using the MFi authentication coprocessor.
	
	@param		inDigestPtr			Ptr to 20-byte SHA-1 digest.
	@param		inDigestLen			Number of bytes in the digest. Must be 20.
	@param		outSignaturePtr		Receives malloc()'d ptr to RSA signature. Caller must free() on success.
	@param		outSignatureLen		Receives number of bytes in RSA signature.
	
	@discussion	MFi accessory implementors must provide this function.
*/
OSStatus
	MFiPlatform_CreateSignature( 
		const void *	inDigestPtr, 
		size_t			inDigestLen, 
		uint8_t **		outSignaturePtr,
		size_t *		outSignatureLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiPlatform_VerifySignature
	@internal
	@abstract	Verifies the MFi auth IC certificate and that the RSA signature is valid for the specified data.
	
	@param		inDataPtr			Data that was signed.
	@param		inDataLen			Number of bytes of data that was signed.
	@param		inSignaturePtr		RSA signature of a SHA-1 hash of the data.
	@param		inSignatureLen		Number of bytes in the signature.
	@param		inCertificatePtr	DER-encoded PKCS#7 message containing the certificate for the signing entity.
	@param		inCertificateLen	Number of bytes in inCertificatePtr.
	
	@discussion	This is only needed on the client side so accessory implementors do not need to provide this function.
*/
OSStatus
	MFiPlatform_VerifySignature( 
		const void *	inDataPtr, 
		size_t			inDataLen, 
		const void *	inSignaturePtr, 
		size_t			inSignatureLen, 
		const void *	inCertificatePtr, 
		size_t			inCertificateLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiPlatform_CopyCertificate
	@abstract	Copy the certificate from the MFi authentication coprocessor. 
	
	@param		outCertificatePtr	Receives malloc()'d ptr to a DER-encoded PKCS#7 message containing the certificate.
									Caller must free() on success.
	@param		outCertificateLen	Number of bytes in the DER-encoded certificate.
	
	@result		kNoErr if successful or an error code indicating failure.

	@discussion	MFi accessory implementors must provide this function.
*/
OSStatus	MFiPlatform_CopyCertificate( uint8_t **outCertificatePtr, size_t *outCertificateLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiPlatform_VerifyCertificate
	@internal
	@abstract	Verifies that a certificate is a valid MFi auth IC certificate.
	
	@param		inCertificatePtr	DER-encoded PKCS#7 message containing the certificate for the signing entity.
	@param		inCertificateLen	Number of bytes in inCertificatePtr.
	
	@discussion	This is only needed on the client side so accessory implementors do not need to provide this function.
*/
OSStatus	MFiPlatform_VerifyCertificate( const void *inCertificatePtr, size_t inCertificateLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@function	MFiSAP_Test
	@internal
	@abstract	Unit test for the MFi-SAP library.
*/
OSStatus	MFiSAP_Test( void );

#ifdef __cplusplus
}
#endif

#endif	// __MFiSAP_h__
