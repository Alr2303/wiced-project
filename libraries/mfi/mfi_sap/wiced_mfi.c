/*
 * $ Copyright Broadcom Corporation $
 */

#include "wiced.h"
#include "wiced_crypto.h"
#include "iauth_chip.h"
#include "wiced_mfi.h"
#include "platform_mfi.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AUTH_CHIP_RETRIES                 ( 5 )

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

static uint8_t  device_certificate [ MAX_MFI_DEVICE_CERTIFICATE_SIZE ];
static uint16_t device_certificate_length = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

void wiced_mfi_deinit( void )
{
    WPRINT_MFI_DEBUG(("De-initialise apple auth chip\r\n"));
    iauth_chip_deinit( &platform_auth_chip );
}

wiced_result_t wiced_mfi_init( void )
{
    uint8_t        attempts = 0;
    wiced_result_t result;

    do
    {
        WPRINT_MFI_DEBUG(("Initialise auth chip\r\n"));
        result =  iauth_chip_init( &platform_auth_chip );

        /* Introduce a small delay between tries */
        wiced_rtos_delay_milliseconds( 1 );
    } while ( result != WICED_SUCCESS && ( ++attempts < AUTH_CHIP_RETRIES ) );

    if ( result != WICED_SUCCESS )
    {
        WPRINT_MFI_DEBUG(("Auth chip initialisation failed\r\n"));
        return WICED_ABORTED;
    }

	/* Retreive the certificate from the AUTH chip at startup phase and keep it globally. This is done to avoid reading of the certification every time we setup and mfi session */
	wiced_mfi_copy_certificate( NULL, 0 );
	return WICED_SUCCESS;
}

wiced_result_t wiced_mfi_create_signature( const void* data_to_sign, uint16_t data_size, uint8_t* signature_buffer, uint16_t signature_buffer_size, uint16_t* signature_size )
{
	return iauth_chip_create_signature( &platform_auth_chip, data_to_sign, data_size, signature_buffer, signature_buffer_size, signature_size );
}

wiced_result_t wiced_mfi_copy_certificate( uint8_t** certificate, unsigned int* certificate_size )
{
    wiced_result_t result = WICED_SUCCESS;

	if ( certificate != NULL )
    {
        *certificate      = device_certificate;
        *certificate_size = device_certificate_length;
    }
	else
	{
	    /* Copy certificate from the device on the only once during start up. Store it in the buffer and use buffer on subsequent calls instead of
	     * reading a certificate from the device all times */
	    result =  iauth_chip_read_certificate ( &platform_auth_chip, device_certificate, sizeof(device_certificate), &device_certificate_length );
	}

	return result;
}

wiced_result_t wiced_mfi_get_random_bytes( void* buffer, uint16_t buffer_size )
{
    wiced_result_t  result;
    uint16_t        random  = 0;
    int             i       = 0;
    int             count   = 0;
    uint8_t*        uint8_buffer  = (uint8_t*)buffer;

    do
    {
        result = wiced_crypto_get_random( &random, sizeof( random ) );
        wiced_assert("Cant get random value from the wlan chip", result == WICED_SUCCESS);
        if ( result != WICED_SUCCESS )
        {
            return WICED_ABORTED;
        }
        uint8_buffer[ i++ ] = random & 0x00FF;
        uint8_buffer[ i++ ] = ( random & 0xFF00 ) >> 8;
        count++;
    } while ( count != ( buffer_size / 2 ) );

    return WICED_SUCCESS;
}
