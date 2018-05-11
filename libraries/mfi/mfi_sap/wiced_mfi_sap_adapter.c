/*
 * $ Copyright Broadcom Corporation $
 */
#include "MFiSAP.h"
#include "wiced_mfi.h"

/******************************************************
 *                    Constants
 ******************************************************/
#define WICED_MFI_RSA_SIGNATURE_SIZE (256)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/
static uint8_t wiced_mfi_rsa_signature_buf[WICED_MFI_RSA_SIGNATURE_SIZE];

/******************************************************
 *               Function Definitions
 ******************************************************/

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t  MFiPlatform_Initialize( wiced_bool_t inIsAuthInternal )
{
    (void) inIsAuthInternal;
    return wiced_mfi_init();
}

void            MFiPlatform_Finalize( void )
{
    wiced_mfi_deinit();
}

wiced_result_t  MFiPlatform_CreateSignature( const void* inDigestPtr, size_t inDigestLen, uint8_t** outSignaturePtr, size_t* outSignatureLen )
{
    wiced_result_t result;
    uint16_t signature_length_temp = 0;
    result = wiced_mfi_create_signature( inDigestPtr, (uint16_t)inDigestLen, wiced_mfi_rsa_signature_buf, sizeof(wiced_mfi_rsa_signature_buf), &signature_length_temp );
    *outSignatureLen = (size_t)signature_length_temp;
    *outSignaturePtr = wiced_mfi_rsa_signature_buf;
    return result;
}

void            MFiPlatform_DestroySignature( const void * inSignaturePtr )
{
    return;
}

wiced_result_t  MFiPlatform_CopyCertificate( uint8_t** outCertificatePtr, size_t* outCertificateLen )
{
    return wiced_mfi_copy_certificate( outCertificatePtr, outCertificateLen );
}

void            MFiPlatform_DestroyCertificate( const void * inCertificatePtr )
{
    return;
}
