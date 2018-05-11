/*
	Linux platform plugin for MFi-SAP authentication/encryption.
	
	Copyright (C) 2013 Apple Inc. All Rights Reserved.
	
	This defaults to using i2c bus /dev/i2c-1 with an MFi auth IC device address of 0x11 (RST pulled high).
	These can be overridden in the makefile with the following:
	
	CFLAGS += -DMFI_AUTH_DEVICE_PATH=\"/dev/i2c-0\"		# MFi auth IC on i2c bus /dev/i2c-0.
	CFLAGS += -DMFI_AUTH_DEVICE_ADDRESS=0x10			# MFi auth IC at address 0x10 (RST pulled low).

	Copyright (C) 2014 Broadcom Inc. All Rights Reserved.

	This file orignated from the POSIX Airplay release at
	Support/MFiServerPlatformLinux.c.
*/

#ifndef _FOR_PLAT_WAC_SERVER
#include "MFiSAP.h"
#else
#include "PlatformMFiAuth.h"
#endif /* _FOR_PLAT_WAC_SERVER */

#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#ifndef _FOR_PLAT_WAC_SERVER
#include "CommonServices.h"
#include "DebugServices.h"
#include "TickUtils.h"
#else
#include "Common.h"
#include "Debug.h"
#include "TimeUtils.h"
#endif /* _FOR_PLAT_WAC_SERVER */


//===========================================================================================================================
//	Constants
//===========================================================================================================================
#define MIN_OF(a, b) ((a > b) ? b : a)

#if( defined( MFI_AUTH_DEVICE_PATH ) )
	#define kMFiAuthDevicePath					MFI_AUTH_DEVICE_PATH
#else
	#define kMFiAuthDevicePath					"/dev/i2c-1"
#endif

#if( defined( MFI_AUTH_DEVICE_ADDRESS ) )
	#define kMFiAuthDeviceAddress				MFI_AUTH_DEVICE_ADDRESS
#else
	#define kMFiAuthDeviceAddress				0x11
#endif

#define kMFiAuthRetryDelayMics					5000 // 5 ms.

#define kMFiAuthReg_AuthControlStatus			0x10
	#define kMFiAuthFlagError						0x80
	#define kMFiAuthControl_GenerateSignature		1
#define kMFiAuthReg_SignatureSize				0x11
#define kMFiAuthReg_SignatureData				0x12
#define kMFiAuthReg_ChallengeSize				0x20
#define kMFiAuthReg_ChallengeData				0x21
#define kMFiAuthReg_DeviceCertificateSize		0x30
#define kMFiAuthReg_DeviceCertificateData1		0x31 // Note: auto-increments so next read is Data2, Data3, etc.

#if( defined( MFI_AUTH_HOST_MSG_LENGTH_LIMIT ) )
        #define kMFiAuthHostMsgLengthLimit                      MFI_AUTH_HOST_MSG_LENGTH_LIMIT
#else
	#define kMFiAuthHostMsgLengthLimit			0
#endif

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

static OSStatus
	_DoI2C( 
		int				inFD, 
		uint8_t			inRegister, 
		const uint8_t *	inWritePtr, 
		size_t			inWriteLen, 
		uint8_t *		inReadBuf, 
		size_t			inReadLen );

//===========================================================================================================================
//	Globals
//===========================================================================================================================

static uint8_t *		gMFiCertificatePtr	= NULL;
static size_t			gMFiCertificateLen	= 0;

#ifdef _FOR_PLAT_WAC_SERVER
static inline uint64_t SecondsToUpTicks( uint64_t x )
{
        static uint64_t sMultiplier = 0;
        if ( sMultiplier == 0 ) sMultiplier = UpTicksPerSecond();
        return( x * sMultiplier );
}

#define MFiPlatform_Initialize PlatformMFiAuthInitialize
#define MFiPlatform_Finalize PlatformMFiAuthFinalize
#define MFiPlatform_CreateSignature PlatformMFiAuthCreateSignature
#define MFiPlatform_CopyCertificate PlatformMFiAuthCopyCertificate

#define ForgetMem( X ) do { if( *(X) ) { free( *(X) ); *(X) = NULL; } } while( 0 )

#define dlog(UnusedLogLevel, M, ...) do {} while(0)

#define plat_mfiauth_log(M, ...) custom_log("PlatformMFiAuth", M, ##__VA_ARGS__)

#else /* !_FOR_PLAT_WAC_SERVER */

#define plat_mfiauth_log(M, ...) do { fprintf(stderr, M, ##__VA_ARGS__); } while(0)

#endif /* _FOR_PLAT_WAC_SERVER */

//===========================================================================================================================
//	MFiPlatform_Initialize
//===========================================================================================================================

OSStatus	MFiPlatform_Initialize( void )
{
	// Cache the certificate at startup since the certificate doesn't change and this saves ~200 ms each time.
	plat_mfiauth_log("%s()+", __FUNCTION__);
	MFiPlatform_CopyCertificate( &gMFiCertificatePtr, &gMFiCertificateLen );
	plat_mfiauth_log("%s()-", __FUNCTION__);
	return( kNoErr );
}

//===========================================================================================================================
//	MFiPlatform_Finalize
//===========================================================================================================================

void	MFiPlatform_Finalize( void )
{
	plat_mfiauth_log("%s()+", __FUNCTION__);
	ForgetMem( &gMFiCertificatePtr );
	gMFiCertificateLen = 0;
	plat_mfiauth_log("%s()-", __FUNCTION__);
}

//===========================================================================================================================
//	MFiPlatform_CreateSignature
//===========================================================================================================================

OSStatus MFiPlatform_CreateSignature( const void * inDigestPtr, 
				      size_t       inDigestLen, 
				      uint8_t **   outSignaturePtr,
				      size_t *	   outSignatureLen )
{
	OSStatus		err;
	int				fd;
	uint8_t			buf[ 32 ];
	size_t			signatureLen;
	uint8_t *		signaturePtr;
	plat_mfiauth_log("%s()+", __FUNCTION__);
	dlog( kLogLevelVerbose, "MFi auth create signature\n" );
	
	fd = open( kMFiAuthDevicePath, O_RDWR );
	err = map_fd_creation_errno( fd );
	require_noerr( err, exit );
	
	err = ioctl( fd, I2C_SLAVE, kMFiAuthDeviceAddress );
	err = map_global_noerr_errno( err );
	require_noerr( err, exit );
	
	// Write the data to sign.
	// Note: writes to the size register auto-increment to the data register that follows it.
	
	require_action( inDigestLen == 20, exit, err = kSizeErr );
	buf[ 0 ] = (uint8_t)( ( inDigestLen >> 8 ) & 0xFF );
	buf[ 1 ] = (uint8_t)(   inDigestLen        & 0xFF );
	memcpy( &buf[ 2 ], inDigestPtr, inDigestLen );
	err = _DoI2C( fd, kMFiAuthReg_ChallengeSize, buf, 2 + inDigestLen, NULL, 0 );
	require_noerr( err, exit );
	
	// Generate the signature.
	
	buf[ 0 ] = kMFiAuthControl_GenerateSignature;
	err = _DoI2C( fd, kMFiAuthReg_AuthControlStatus, buf, 1, NULL, 0 );
	require_noerr( err, exit );
	
	err = _DoI2C( fd, kMFiAuthReg_AuthControlStatus, NULL, 0, buf, 1 );
	require_noerr( err, exit );
	require_action( !( buf[ 0 ] & kMFiAuthFlagError ), exit, err = kUnknownErr );
	
	// Read the signature.
	
	err = _DoI2C( fd, kMFiAuthReg_SignatureSize, NULL, 0, buf, 2 );
	require_noerr( err, exit );
	signatureLen = ( buf[ 0 ] << 8 ) | buf[ 1 ];
	require_action( signatureLen > 0, exit, err = kSizeErr );
	
	signaturePtr = (uint8_t *) malloc( signatureLen );
	require_action( signaturePtr, exit, err = kNoMemoryErr );
	
	err = _DoI2C( fd, kMFiAuthReg_SignatureData, NULL, 0, signaturePtr, signatureLen );
	if( err ) free( signaturePtr );
	require_noerr( err, exit );

	dlog( kLogLevelVerbose, "MFi auth created signature:\n%.2H\n", signaturePtr, (int) signatureLen, (int) signatureLen );
	*outSignaturePtr = signaturePtr;
	*outSignatureLen = signatureLen;
	
exit:
	if( fd >= 0 )	close( fd );
	if( err )		dlog( kLogLevelWarning, "### MFi auth create signature failed: %#m\n", err );
	plat_mfiauth_log("%s()-", __FUNCTION__);
	return( err );
}

//===========================================================================================================================
//	MFiPlatform_CopyCertificate
//===========================================================================================================================

OSStatus	MFiPlatform_CopyCertificate( uint8_t **outCertificatePtr, size_t *outCertificateLen )
{
	OSStatus		err;
	size_t			certificateLen;
	uint8_t *		certificatePtr;
	int				fd = -1;
	uint8_t			buf[ 2 ];
	plat_mfiauth_log("%s()+", __FUNCTION__);
	dlog( kLogLevelVerbose, "MFi auth copy certificate\n" );
	
	// If the certificate has already been cached then return that as an optimization since it doesn't change.
	
	if( gMFiCertificateLen > 0 )
	{
		certificatePtr = (uint8_t *) malloc( gMFiCertificateLen );
		require_action( certificatePtr, exit, err = kNoMemoryErr );
		memcpy( certificatePtr, gMFiCertificatePtr, gMFiCertificateLen );
		
		*outCertificatePtr = certificatePtr;
		*outCertificateLen = gMFiCertificateLen;
		err = kNoErr;
		goto exit;
	}
	
	fd = open( kMFiAuthDevicePath, O_RDWR );
	err = map_fd_creation_errno( fd );
	require_noerr( err, exit );
	
	err = ioctl( fd, I2C_SLAVE, kMFiAuthDeviceAddress );
	err = map_global_noerr_errno( err );
	require_noerr( err, exit );

	err = _DoI2C( fd, kMFiAuthReg_DeviceCertificateSize, NULL, 0, buf, 2 );
	require_noerr( err, exit );
	certificateLen = ( buf[ 0 ] << 8 ) | buf[ 1 ];
	require_action( certificateLen > 0, exit, err = kSizeErr );

	certificatePtr = (uint8_t *) malloc( certificateLen );
	require_action( certificatePtr, exit, err = kNoMemoryErr );

	// Note: reads from the data1 register auto-increment to data2, data3, etc. registers that follow it.

	err = _DoI2C( fd, kMFiAuthReg_DeviceCertificateData1, NULL, 0, certificatePtr, certificateLen );
	if( err ) free( certificatePtr );
	require_noerr( err, exit );

	dlog( kLogLevelVerbose, "MFi auth copy certificate done: %zu bytes\n", certificateLen );
	*outCertificatePtr = certificatePtr;
	*outCertificateLen = certificateLen;

exit:
	if( fd >= 0 )	close( fd );
	if( err )		dlog( kLogLevelWarning, "### MFi auth copy certificate failed: %#m\n", err );
	plat_mfiauth_log("%s()-", __FUNCTION__);
	return( err );
}

//===========================================================================================================================
//	_DoI2C
//===========================================================================================================================

static OSStatus
	_DoI2C( 
		int				inFD, 
		uint8_t			inRegister, 
		const uint8_t *	inWritePtr, 
		size_t			inWriteLen, 
		uint8_t *		inReadBuf, 
		size_t			inReadLen )
{
	OSStatus		err;
	uint64_t		deadline;
	int				tries;
	ssize_t			n;
	uint8_t			buf[ 1 + inWriteLen ];
	size_t			len;
	plat_mfiauth_log("%s()+", __FUNCTION__);
	deadline = UpTicks() + SecondsToUpTicks( 2 );
	if( inReadBuf )
	{
		// Combined mode transactions are not supported so set the register and do a separate read.
		
		for( tries = 1; ; ++tries )
		{
			n = write( inFD, &inRegister, 1 );
			err = map_global_value_errno( n == 1, n );
			if( !err ) break;

			dlog( kLogLevelVerbose, "### MFi auth set register 0x%02X failed (try %d): %#m\n", inRegister, tries, err );
			usleep( kMFiAuthRetryDelayMics );
			require_action( UpTicks() < deadline, exit, err = kTimeoutErr );
		}
		if (!kMFiAuthHostMsgLengthLimit) {
			for( tries = 1; ; ++tries )
			{
				n = read( inFD, inReadBuf, inReadLen);
				err = map_global_value_errno( n == ( (ssize_t) inReadLen ), n );
				if( !err ) break;

				dlog( kLogLevelVerbose, "### MFi auth read register 0x%02X, %zu bytes failed (try %d): %#m\n", 
				      inRegister, inReadLen, tries, err );
				usleep( kMFiAuthRetryDelayMics );
				require_action( UpTicks() < deadline, exit, err = kTimeoutErr );
			}
		}
		else
		{
			size_t bytes_read = 0;

			do {
				for( tries = 1; ; ++tries )
				{
					size_t chunk_length = MIN_OF(kMFiAuthHostMsgLengthLimit, (inReadLen - bytes_read));
					n = read( inFD, inReadBuf + bytes_read, chunk_length);
					err = map_global_value_errno( n == ( (ssize_t) chunk_length ), n );
					if( !err ) break;

					dlog( kLogLevelVerbose, "### MFi auth read register 0x%02X, %zu bytes failed (try %d): %#m\n", 
					      inRegister, chunk_length, tries, err );
					usleep( kMFiAuthRetryDelayMics );
					require_action( UpTicks() < deadline, exit, err = kTimeoutErr );
				}

				bytes_read += n;

			} while (bytes_read < inReadLen);

		}
	}
	else
	{
		// Gather the register and data so it can be done as a single write transaction.
		
		buf[ 0 ] = inRegister;
		memcpy( &buf[ 1 ], inWritePtr, inWriteLen );
		len = 1 + inWriteLen;
		
		for( tries = 1; ; ++tries )
		{
			n = write( inFD, buf, len );
			err = map_global_value_errno( n == ( (ssize_t) len ), n );
			if( !err ) break;

			dlog( kLogLevelVerbose, "### MFi auth write register 0x%02X, %zu bytes failed (try %d): %#m\n", 
				inRegister, inWriteLen, tries, err );
			usleep( kMFiAuthRetryDelayMics );
			require_action( UpTicks() < deadline, exit, err = kTimeoutErr );
		}
	}

exit:
	if( err ) dlog( kLogLevelWarning, "### MFi auth register 0x%02X failed after %d tries: %#m\n", inRegister, tries, err );
	plat_mfiauth_log("%s()-", __FUNCTION__);
	return( err );
}
