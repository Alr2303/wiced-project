/*
 * Copyright 2017, Cypress Semiconductor Corporation or a subsidiary of 
 * Cypress Semiconductor Corporation. All Rights Reserved.
 * 
 * This software, associated documentation and materials ("Software"),
 * is owned by Cypress Semiconductor Corporation
 * or one of its subsidiaries ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products. Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */
#include "wiced.h"
#include "iauth_chip.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AUTH_CHIP_RETRIES                    ( 5 )
#define START_UP_TIME_MILLISECONDS           ( 10 + 1 )
#define RESET_HOLD_TIME_MICROSECONDS         ( 50 ) /* should be at least 10 us, we set it to 50 */
#define RESET_EXTRA_HOLD_TIME_MICROSECONDS   ( 1200 )  /* more than 1 millisecond */

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint8_t  register_address;
    uint8_t  length[ 2 ];
    uint8_t  challenge_data[ 20 ];
} auth_chip_challenge_t;

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Function Declarations
 ******************************************************/

/******************************************************
 *                 Variables
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t iauth_chip_init( const platform_mfi_auth_chip_t* platform_params )
{
    wiced_bool_t   probing_result;
    wiced_result_t result;

    WPRINT_AUTH_CHIP_DEBUG(("Init I2C for auth chip. SDA and SCL set high\r\n"));

    /* Init i2c and set SDA and SCL lines to high level before the reset line goes low */
    result = wiced_i2c_init( platform_params->i2c_device );
    if ( result != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }
    WPRINT_AUTH_CHIP_DEBUG(("Init auth-IC with address 0x%02x\r\n", platform_params->i2c_device->address));

    WICED_DISABLE_INTERRUPTS();

    if ( platform_params->reset_pin == WICED_GPIO_NONE )
    {
        /* Reset line in not connected to the MCU, soft reset is not available, just do a normal power up sequence wait for 10ms and probe for the address */
        wiced_rtos_delay_microseconds( START_UP_TIME_MILLISECONDS * 1000 );
    }
    else
    {
        /* Reset line is connected to an MCU and we can perform a soft reset. Just do it */
        wiced_gpio_init( platform_params->reset_pin, OUTPUT_PUSH_PULL );

        /* Set reset line low and keep it low for at least 10 us, we will use 500us */
        wiced_gpio_output_low( platform_params->reset_pin );
        wiced_rtos_delay_microseconds( RESET_HOLD_TIME_MICROSECONDS );

        if ( platform_params->i2c_device->address == 0x11 )
        {
            /* Set reset line high to select address 0x11, for address 0x10 reset line still stays low */
            wiced_gpio_output_high ( platform_params->reset_pin );
        }

        /* Now the address is set and address selection stage is finished */

        /* Wait for 10ms at least to make sure that the first data transmission will start not earlier than 10 milliseconds after the falling edge of the RST signal */
        wiced_rtos_delay_microseconds( START_UP_TIME_MILLISECONDS * 1000 );

        /* Emulate the first transmission by probing address */
        wiced_i2c_probe_device( platform_params->i2c_device, 1 );

        if ( platform_params->i2c_device->address == 0x10 )
        {
            /* Wait for a little bit more than 1 millisecond and pull RESET line high back again */
            wiced_rtos_delay_microseconds( RESET_EXTRA_HOLD_TIME_MICROSECONDS );
            wiced_gpio_output_high( platform_params->reset_pin );
        }
    }

    probing_result = wiced_i2c_probe_device( platform_params->i2c_device, 10 );
    if ( probing_result == WICED_FALSE )
    {
        result = WICED_ERROR;
    }

    WICED_ENABLE_INTERRUPTS();

    if ( result != WICED_SUCCESS )
    {
        WPRINT_AUTH_CHIP_DEBUG(("Auth-IC probing is not successful\r\n"));
    }

    return result;
}


wiced_result_t iauth_chip_deinit( const platform_mfi_auth_chip_t* platform_params )
{
    wiced_result_t result = wiced_i2c_deinit( platform_params->i2c_device );
    wiced_assert("Can not deinitialize the auth chip device", result == WICED_SUCCESS);
    return result;
}

wiced_result_t iauth_chip_write_register ( const platform_mfi_auth_chip_t* platform_params,  iauth_chip_register_t chip_register, iauth_chip_transfer_t* data, uint32_t data_length)
{
    wiced_i2c_message_t message;

    memset( &message, 0, sizeof( message ) );
    message.retries = 50;

    data->register_address = chip_register;
    message.tx_buffer      = data;
    message.tx_length      = data_length + 1;
    return wiced_i2c_transfer( platform_params->i2c_device, &message, 1 );
}

wiced_result_t iauth_chip_read_register ( const platform_mfi_auth_chip_t* platform_params, iauth_chip_register_t chip_register, void* data, uint32_t data_length )
{
    wiced_result_t      result           = WICED_SUCCESS;
    uint8_t             register_address = chip_register;
    wiced_bool_t        disable_dma      = WICED_TRUE; /* FIXME disable-DMA is too platform dependent! */
    wiced_i2c_message_t message[1];

    /* Set the register counter. */
    if ( result == WICED_SUCCESS )
    {
        result = wiced_i2c_init_tx_message( message, &register_address, 1, 50, disable_dma );
        wiced_assert("I2C tx init failed", result == WICED_SUCCESS);
        result = wiced_i2c_transfer( platform_params->i2c_device, message, 1 );
    }

    /* Read from bytes at current register location. */
    if ( result == WICED_SUCCESS )
    {
        /* XXX Need to wait to read if not using combined messages. */
        wiced_rtos_delay_milliseconds( 5 );
        result = wiced_i2c_init_rx_message( message, data, data_length, 50, disable_dma );
        wiced_assert("I2C rx init failed", result == WICED_SUCCESS);
        result = wiced_i2c_transfer( platform_params->i2c_device, message, 1 );
    }

    return result;
}

wiced_result_t iauth_chip_create_signature ( const platform_mfi_auth_chip_t* platform_params, const void* data_to_sign, uint16_t data_size, uint8_t* signature_buffer, uint16_t signature_buffer_size, uint16_t* signature_size )
{
    uint8_t               signature_length_buffer[ 2 ];
    auth_chip_challenge_t challenge_transfer;
    iauth_chip_transfer_t transfer;
    uint8_t               status;
    wiced_result_t        result    = WICED_SUCCESS;
    uint8_t               attempts  = 0;

    memset( &transfer, 0x00, sizeof(transfer) );

    /* Set challenge data and size and write to the AUTH_CHIP_CHALLENGE_DATA_LENGTH  register in the auth chip */
    challenge_transfer.length[ 0 ] = (uint8_t)( ( data_size >> 8 ) & 0xFF );
    challenge_transfer.length[ 1 ] = (uint8_t)(   data_size        & 0xFF );
    if ( data_size < sizeof( challenge_transfer.challenge_data ) )

    {
        return WICED_BADARG;
    }

    memcpy( &challenge_transfer.challenge_data, data_to_sign, data_size );

    do
    {
        WPRINT_AUTH_CHIP_DEBUG(("Set challenge data and size \r\n"));

        result = iauth_chip_write_register( platform_params, IAUTH_CHIP_CHALLENGE_DATA_LENGTH, (iauth_chip_transfer_t*)&challenge_transfer, sizeof(challenge_transfer) );

        wiced_rtos_delay_milliseconds( 1 );
    } while( ( result != WICED_SUCCESS ) && (++attempts < AUTH_CHIP_RETRIES ) );

    wiced_assert("Auth chip write failed", result == WICED_SUCCESS);

    if ( result != WICED_SUCCESS )
    {
        WPRINT_AUTH_CHIP_DEBUG(("Auth chip write failed\r\n"));
        return WICED_ABORTED;
    }

    /* Generate the signature */
    transfer.data[ 0 ] = CONTROL_AND_STATUS_FLAG_GENERATE_SIGNATURE;
    attempts = 0;
    do
    {
        WPRINT_AUTH_CHIP_DEBUG(("Initiate generate signature command \r\n"));
        result = iauth_chip_write_register( platform_params, IAUTH_CHIP_AUTH_CONTROL_AND_STATUS, &transfer, 1);

        wiced_rtos_delay_milliseconds( 1 );

    } while( ( result != WICED_SUCCESS ) && ( ++attempts < AUTH_CHIP_RETRIES ) );

    wiced_assert("Auth chip control set failed", result == WICED_SUCCESS);

    wiced_rtos_delay_milliseconds( 500 );
    if ( result != WICED_SUCCESS )
    {
        return WICED_ABORTED;
    }

    attempts = 0;

    /* Read the status of the signature generation operation */
    do
    {
        WPRINT_AUTH_CHIP_DEBUG(("Reading status of the operation \r\n"));
        result = iauth_chip_read_register( platform_params, IAUTH_CHIP_AUTH_CONTROL_AND_STATUS, &status, 1 );

        wiced_rtos_delay_milliseconds( 1 );

    } while( ( result != WICED_SUCCESS ) && ( ++attempts < AUTH_CHIP_RETRIES ) );

    wiced_assert("I2C transaction to auth chip failed", result == WICED_SUCCESS);

    if ( ( result != WICED_SUCCESS ) || ( status & CONTROL_AND_STATUS_FLAG_ERROR ) )
    {
        return WICED_ABORTED;
    }

    attempts = 0;

    /* Read the signature length */
    do
    {
        WPRINT_AUTH_CHIP_DEBUG(("Reading signature length \r\n"));
        result = iauth_chip_read_register( platform_params, IAUTH_CHIP_SIGNATURE_DATA_LENGTH, signature_length_buffer, 2 );

        wiced_rtos_delay_milliseconds( 1 );
    } while( ( result != WICED_SUCCESS ) && ( ++attempts < AUTH_CHIP_RETRIES ) );

    wiced_assert("Reading signature chip failed", ( result == WICED_SUCCESS ) );

    if ( result != WICED_SUCCESS )
    {
        return WICED_ABORTED;
    }

    *signature_size = ( signature_length_buffer[ 0 ] << 8 ) | signature_length_buffer[ 1 ];
    if (*signature_size > signature_buffer_size)
    {
        *signature_size = 0;
        return WICED_ABORTED;
    }

    WPRINT_AUTH_CHIP_DEBUG(("Signature size is %d\r\n", *signature_size));

    /* Read the signature */
    do
    {
        WPRINT_AUTH_CHIP_DEBUG(("Reading signature \r\n"));
        result = iauth_chip_read_register( platform_params, IAUTH_CHIP_SIGNATURE_DATA, signature_buffer, *signature_size );
    } while ( ( result != WICED_SUCCESS ) && ( ++attempts < AUTH_CHIP_RETRIES ) );

    if ( result != WICED_SUCCESS )
    {
        WPRINT_AUTH_CHIP_DEBUG(("Reading signature failed \r\n"));
        return WICED_ABORTED;
    }

    WPRINT_AUTH_CHIP_DEBUG(("Reading signature done \r\n"));

    return WICED_SUCCESS;
}

wiced_result_t iauth_chip_read_certificate ( const platform_mfi_auth_chip_t* platform_params, uint8_t* certificate_ptr, uint16_t certificate_buffer_size, uint16_t* certificate_size )
{
    uint8_t        cert_length_buf[ 2 ];
    wiced_result_t result;
    uint8_t        attempts                  = 0;
    uint16_t       device_certificate_length = 0;

    do
    {
        WPRINT_AUTH_CHIP_DEBUG(("Reading certificate length \r\n"));
        result = iauth_chip_read_register( platform_params, IAUTH_CHIP_CERTIFICATE_DATA_LENGTH, &cert_length_buf, 2 );
        wiced_rtos_delay_milliseconds( 1 );
    } while ( ( result != WICED_SUCCESS ) &&  ( ++attempts < AUTH_CHIP_RETRIES ) );

    wiced_assert("Reading certificate length failed", result == WICED_SUCCESS);
    if ( result != WICED_SUCCESS )
    {
        WPRINT_AUTH_CHIP_DEBUG(("Reading certificate length failed \r\n"));
        return WICED_ABORTED;
    }

    attempts = 0;
    device_certificate_length = ( cert_length_buf[ 0 ] << 8 ) | cert_length_buf[ 1 ];
    *certificate_size = device_certificate_length;
    wiced_assert( "Accessory device certificate can not be 0 length", device_certificate_length );
    if ( device_certificate_length == 0 || certificate_buffer_size < device_certificate_length )
    {
        return WICED_ABORTED;
    }

    do
    {
        WPRINT_AUTH_CHIP_DEBUG(("Reading certificate contents \r\n"));
        result = iauth_chip_read_register( platform_params, IAUTH_CHIP_CERTIFICATE_DATA, certificate_ptr, device_certificate_length );
    } while( ( result != WICED_SUCCESS ) &&  ( ++attempts < AUTH_CHIP_RETRIES ) );

    wiced_assert("I2C transaction to auth chip failed", result == WICED_SUCCESS);

    if ( result != WICED_SUCCESS )
    {
        WPRINT_AUTH_CHIP_DEBUG(("Reading certificate contents failed \r\n"));
        return WICED_ABORTED;
    }
    return WICED_SUCCESS;
}

