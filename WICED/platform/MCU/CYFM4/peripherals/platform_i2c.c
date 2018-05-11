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

/** @file
 *
 */
#include "stdint.h"
#include "string.h"
#include "platform_peripheral.h"
#include "platform_isr.h"
#include "platform_isr_interface.h"
#include "wwd_rtos.h"
#include "wwd_assert.h"
#include "wiced_platform.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define I2C_START_FLAG              (1U << 0)
#define I2C_STOP_FLAG               (1U << 1)
#define I2C_RW_BIT                  0x1

#define PLATFORM_I2C_MFS_CHANNEL_MAX 10
#define PLATFORM_I2C_MFS_PIN_INDEX_MAX 2

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef platform_result_t (i2c_io_fn_t)( const platform_i2c_t*, const platform_i2c_config_t*, uint8_t flags, uint8_t* buffer, uint16_t buffer_length);

typedef platform_result_t (*i2c_init_func_t)        ( const platform_i2c_t* i2c, const platform_i2c_config_t* config );
typedef platform_result_t (*i2c_deinit_func_t)      ( const platform_i2c_t* i2c, const platform_i2c_config_t* config );
typedef platform_result_t (*i2c_read_func_t)        ( const platform_i2c_t *i2c, const platform_i2c_config_t *config, uint8_t flags, uint8_t *data_in, uint16_t length );
typedef platform_result_t (*i2c_write_func_t)       ( const platform_i2c_t *i2c, const platform_i2c_config_t *config, uint8_t flags, uint8_t *data_out, uint16_t length );
typedef platform_result_t (*i2c_transfer_func_t)    ( const platform_i2c_t* i2c, const platform_i2c_config_t *config, uint16_t flags, void *buffer, uint16_t buffer_length, i2c_io_fn_t* fn );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    i2c_init_func_t init;
    i2c_deinit_func_t deinit;
    i2c_read_func_t read;
    i2c_write_func_t write;
    i2c_transfer_func_t transfer;
} i2c_driver_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

/* MFS I2C bus interface */
static platform_result_t    mfs_i2c_init            ( const platform_i2c_t* i2c, const platform_i2c_config_t* config );
static platform_result_t    mfs_i2c_deinit          ( const platform_i2c_t* i2c, const platform_i2c_config_t* config );
static platform_result_t    mfs_i2c_read            ( const platform_i2c_t* i2c, const platform_i2c_config_t* config, uint8_t flags, uint8_t* data_in, uint16_t length );
static platform_result_t    mfs_i2c_write           ( const platform_i2c_t* i2c, const platform_i2c_config_t* config, uint8_t flags, uint8_t* data_out, uint16_t length );
static platform_result_t    mfs_i2c_transfer        ( const platform_i2c_t* i2c, const platform_i2c_config_t* config, uint16_t flags, void* buffer, uint16_t buffer_length, i2c_io_fn_t* fn );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* MFS I2C bus driver functions */
static const i2c_driver_t i2c_mfs_driver =
{
    .init = mfs_i2c_init,
    .deinit = mfs_i2c_deinit,
    .read = mfs_i2c_read,
    .write = mfs_i2c_write,
    .transfer = mfs_i2c_transfer,
};

/* MFS I2C driver instance */
static const i2c_driver_t* i2c_driver = &i2c_mfs_driver;

/* MFS I2C channel instance */
static volatile stc_mfsn_i2c_t* mfsn_i2c[PLATFORM_I2C_MFS_CHANNEL_MAX] = {&I2C0, &I2C1, &I2C2, &I2C3, &I2C4, &I2C5, &I2C6, &I2C7, &I2C8, &I2C9};

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Mapping of MFS I2C port and SOT pin index to MFS I2C SDA pin function */
static platform_result_t mfs_i2c_set_sda_pin(uint32_t port, uint32_t pin_index)
{
    if ( (port >= PLATFORM_I2C_MFS_CHANNEL_MAX) || ( pin_index >= PLATFORM_I2C_MFS_PIN_INDEX_MAX) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    switch ( port )
    {
        case 0:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT0_0();
                    break;

                case 1:
                    SetPinFunc_SOT0_1();
                    break;
            }
            break;

        case 1:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT1_0();
                    break;

                case 1:
                    SetPinFunc_SOT1_1();
                    break;
            }
            break;

        case 2:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT2_0();
                    break;

                case 1:
                    SetPinFunc_SOT2_1();
                    break;
            }
            break;

        case 3:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT3_0();
                    break;

                case 1:
                    SetPinFunc_SOT3_1();
                    break;
            }
            break;

        case 4:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT4_0();
                    break;

                case 1:
                    SetPinFunc_SOT4_1();
                    break;
            }
            break;

        case 5:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT5_0();
                    break;

                case 1:
                    SetPinFunc_SOT5_1();
                    break;
            }
            break;

        case 6:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT6_0();
                    break;

                case 1:
                    SetPinFunc_SOT6_1();
                    break;
            }
            break;

        case 7:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT7_0();
                    break;

                case 1:
                    SetPinFunc_SOT7_1();
                    break;
            }
            break;

        case 8:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT8_0();
                    break;

                case 1:
                    SetPinFunc_SOT8_1();
                    break;
            }
            break;

        case 9:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SOT9_0();
                    break;

                case 1:
                    SetPinFunc_SOT9_1();
                    break;
            }
            break;
    }

    return PLATFORM_SUCCESS;
}

/* Mapping of MFS I2C port and SCK pin index to MFS I2C SCL pin function */
static platform_result_t mfs_i2c_set_scl_pin(uint32_t port, uint32_t pin_index)
{
    if ( (port >= PLATFORM_I2C_MFS_CHANNEL_MAX) || ( pin_index >= PLATFORM_I2C_MFS_PIN_INDEX_MAX) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    switch ( port )
    {
        case 0:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK0_0();
                    break;

                case 1:
                    SetPinFunc_SCK0_1();
                    break;
            }
            break;

        case 1:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK1_0();
                    break;

                case 1:
                    SetPinFunc_SCK1_1();
                    break;
            }
            break;

        case 2:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK2_0();
                    break;

                case 1:
                    SetPinFunc_SCK2_1();
                    break;
            }
            break;

        case 3:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK3_0();
                    break;

                case 1:
                    SetPinFunc_SCK3_1();
                    break;
            }
            break;

        case 4:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK4_0();
                    break;

                case 1:
                    SetPinFunc_SCK4_1();
                    break;
            }
            break;

        case 5:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK5_0();
                    break;

                case 1:
                    SetPinFunc_SCK5_1();
                    break;
            }
            break;

        case 6:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK6_0();
                    break;

                case 1:
                    SetPinFunc_SCK6_1();
                    break;
            }
            break;

        case 7:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK7_0();
                    break;

                case 1:
                    SetPinFunc_SCK7_1();
                    break;
            }
            break;

        case 8:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK8_0();
                    break;

                case 1:
                    SetPinFunc_SCK8_1();
                    break;
            }
            break;

        case 9:
            switch ( pin_index )
            {
                case 0:
                    SetPinFunc_SCK9_0();
                    break;

                case 1:
                    SetPinFunc_SCK9_1();
                    break;
            }
            break;
    }

    return PLATFORM_SUCCESS;
}

static platform_result_t mfs_i2c_start( const platform_i2c_t* i2c, uint8_t slave )
{
    /* Prepare I2C device address */
    Mfs_I2c_SendData( mfsn_i2c[i2c->port], slave );

    /* Generate I2C start signal */
    if ( Mfs_I2c_GenerateStart( mfsn_i2c[i2c->port] ) != Ok )
    {
        /* Timeout or other error */
        return PLATFORM_ERROR;
    }

    while ( 1 )
    {
        /* XXX: Need to verify if the below check from PDL is accurate? */
        if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cRxTxIrq ) != TRUE )
        {
            break;
        }
    }

    if ( Mfs_I2c_GetAck(mfsn_i2c[i2c->port]) == I2cNAck )
    {
        /* NACK */
        return PLATFORM_ERROR;
    }

    if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cBusErr ) == TRUE )
    {
        /* Bus error occurs? */
        return PLATFORM_ERROR;
    }

    if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cOverrunError ) == TRUE )
    {
        /* Overrun error occurs? */
        return PLATFORM_ERROR;
    }

    return PLATFORM_SUCCESS;
}

static platform_result_t mfs_i2c_stop( const platform_i2c_t* i2c )
{
    /* Generate I2C stop signal */
    if ( Mfs_I2c_GenerateStop( mfsn_i2c[i2c->port] ) != Ok )
    {
        /* Timeout or other error */
        return PLATFORM_ERROR;
    }

    /* Clear Stop condition flag */
    while ( 1 )
    {
        if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cStopDetect ) == TRUE )
        {
            break;
        }
    }

    if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cBusErr ) == TRUE )
    {
        return PLATFORM_ERROR;
    }

    Mfs_I2c_ClrStatus( mfsn_i2c[i2c->port], I2cStopDetect );
    Mfs_I2c_ClrStatus( mfsn_i2c[i2c->port], I2cRxTxIrq );

    return PLATFORM_SUCCESS;
}

static platform_result_t mfs_i2c_writebytes( const platform_i2c_t* i2c, uint8_t* data_out, uint16_t length )
{
    uint16_t i;

    for ( i = 0 ; i < length ; i++ )
    {
        /* Transmit the data */
        Mfs_I2c_SendData( mfsn_i2c[i2c->port], data_out[i] );
        Mfs_I2c_ClrStatus( mfsn_i2c[i2c->port], I2cRxTxIrq );

        /* Wait for end of transmission */
        while ( 1 )
        {
            if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cRxTxIrq ) == TRUE )
            {
                break;
            }
        }

        while ( 1 )
        {
            if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cTxEmpty ) == TRUE )
            {
                break;
            }
        }

        if ( Mfs_I2c_GetAck( mfsn_i2c[i2c->port] ) == I2cNAck )
        {
            /* NACK */
            return PLATFORM_ERROR;
        }

        if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cBusErr ) == TRUE )
        {
            /* Bus error occurs? */
            return PLATFORM_ERROR;
        }

        if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cOverrunError ) == TRUE )
        {
            /* Overrun error occurs? */
            return PLATFORM_ERROR;
        }
    }

    return PLATFORM_SUCCESS;
}

static platform_result_t mfs_i2c_readbytes( const platform_i2c_t* i2c, uint8_t* data_in, uint16_t length, int last )
{
    uint16_t i = 0;

    /* Clear interrupt flag generated by device address send */
    Mfs_I2c_ClrStatus( mfsn_i2c[i2c->port], I2cRxTxIrq );

    if ( Mfs_I2c_GetAck( mfsn_i2c[i2c->port] ) == I2cNAck )
    {
        /* NACK */
        return PLATFORM_ERROR;
    }

    while ( i < length )
    {
        /* Wait for the receive data */
        while ( 1 )
        {
            if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cRxTxIrq ) == TRUE )
            {
                break;
            }
        }

        if ( i == (length - 1) )
        {
            if ( last )
            {
                /* Last byte send a NACK */
                Mfs_I2c_ConfigAck( mfsn_i2c[i2c->port], I2cNAck );
            }
            else
            {
                Mfs_I2c_ConfigAck( mfsn_i2c[i2c->port], I2cAck );
            }
        }
        else
        {
            Mfs_I2c_ConfigAck( mfsn_i2c[i2c->port], I2cAck );
        }

        /* Clear interrupt flag and receive data to RX buffer */
        Mfs_I2c_ClrStatus( mfsn_i2c[i2c->port], I2cRxTxIrq );

        /* Wait for the receive data */
        while ( 1 )
        {
            if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cRxFull ) == TRUE )
            {
                break;
            }
        }

        if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cBusErr ) == TRUE )
        {
            /* Bus error occurs? */
            return PLATFORM_ERROR;
        }

        if ( Mfs_I2c_GetStatus( mfsn_i2c[i2c->port], I2cOverrunError ) == TRUE )
        {
            /* Overrun error occurs? */
            return PLATFORM_ERROR;
        }

        data_in[i++] = Mfs_I2c_ReceiveData( mfsn_i2c[i2c->port] );
    }

    return PLATFORM_SUCCESS;
}

static platform_result_t mfs_i2c_write( const platform_i2c_t *i2c, const platform_i2c_config_t* config, uint8_t flags, uint8_t* data_out, uint16_t length )
{
    platform_result_t ret;
    uint8_t address;

    if ( data_out == NULL || length == 0 )
    {
        return PLATFORM_BADARG;
    }

    /* Slave address and R/W bit */
    address = config->address << 1;
    address &= ~I2C_RW_BIT;

    /* Start Condition */
    if ( (flags & I2C_START_FLAG) != 0 )
    {
        ret = mfs_i2c_start( i2c, address );
        if ( ret != PLATFORM_SUCCESS )
        {
            mfs_i2c_stop( i2c );
            return PLATFORM_ERROR;
        }
    }

    /* Write the data */
    ret = mfs_i2c_writebytes( i2c, data_out, length );

    /* Stop Condition */
    if ( (ret != PLATFORM_SUCCESS) || ((flags & I2C_STOP_FLAG) != 0) )
    {
        mfs_i2c_stop( i2c );
    }

    if ( ret != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    return PLATFORM_SUCCESS;
}

static platform_result_t mfs_i2c_read( const platform_i2c_t *i2c, const platform_i2c_config_t* config, uint8_t flags, uint8_t* data_in, uint16_t length )
{
    platform_result_t ret;
    uint8_t address;

    if ( data_in == NULL || length == 0 )
    {
        return PLATFORM_BADARG;
    }

    /* Slave address and R/W bit */
    address = config->address << 1;
    address |= I2C_RW_BIT;

    /* Start Condition */
    if ( (flags & I2C_START_FLAG) != 0 )
    {
        ret = mfs_i2c_start( i2c, address );
        if ( ret != PLATFORM_SUCCESS )
        {
            mfs_i2c_stop( i2c );
            return PLATFORM_ERROR;
        }
    }

    /* Read the data */
    ret = mfs_i2c_readbytes( i2c, data_in, length, ((flags & I2C_STOP_FLAG) != 0) ? 1 : 0 );

    /* Stop Condition */
    if ( (ret != PLATFORM_SUCCESS) || ((flags & I2C_STOP_FLAG) != 0) )
    {
        mfs_i2c_stop( i2c );
    }

    if ( ret != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    return PLATFORM_SUCCESS;
}

static platform_result_t mfs_i2c_transfer( const platform_i2c_t* i2c, const platform_i2c_config_t *config, uint16_t flags, void *buffer, uint16_t buffer_length, i2c_io_fn_t* fn )
{
    uint8_t i2c_flags = 0;

    if ( ( flags & (WICED_I2C_START_FLAG | WICED_I2C_REPEATED_START_FLAG) ) != 0 )
    {
        i2c_flags |= I2C_START_FLAG;
    }
    if ( ( flags & WICED_I2C_STOP_FLAG ) != 0 )
    {
        i2c_flags |= I2C_STOP_FLAG;
    }

    return (*fn)( i2c, config, i2c_flags, buffer, buffer_length );
}

static platform_result_t mfs_i2c_init( const platform_i2c_t* i2c, const platform_i2c_config_t* config )
{
    stc_mfs_i2c_config_t mfs_i2c_config;

    if ( ( i2c == NULL ) || ( config == NULL ) )
    {
        wiced_assert( "bad argument", 0 );
        return PLATFORM_ERROR;
    }

    if ( ( (config->flags & I2C_DEVICE_NO_DMA) == 0 ) || ( config->address_width != I2C_ADDRESS_WIDTH_7BIT ) )
    {
        /* TBD: I2C DMA and 10-bit address width */
        return PLATFORM_UNSUPPORTED;
    }

    platform_mcu_powersave_disable( );

    /* Configure MFS I2C SCL and SDA pin mapping */
    if ( mfs_i2c_set_scl_pin( i2c->port, i2c->scl_pin_index ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_UNSUPPORTED;
    }

    if ( mfs_i2c_set_sda_pin( i2c->port, i2c->sda_pin_index ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Setup the MFS I2C channel configuration */
    mfs_i2c_config.enMsMode = I2cMaster;
    mfs_i2c_config.bWaitSelection = FALSE;
    mfs_i2c_config.bDmaEnable = FALSE;
    mfs_i2c_config.pstcFifoConfig = NULL;

    switch ( config->speed_mode )
    {
        case I2C_LOW_SPEED_MODE:    /* 10Khz devices */
            mfs_i2c_config.u32BaudRate = 10000u;
            break;

        case I2C_STANDARD_SPEED_MODE:    /* 100Khz devices */
            mfs_i2c_config.u32BaudRate = 100000u;
            break;

        case I2C_HIGH_SPEED_MODE:    /* 400Khz devices */
            mfs_i2c_config.u32BaudRate = 400000u;
            break;

        default:
            return PLATFORM_BADARG;
    }

    /* Attempt to acquire the specified MFS channel for I2C operation mode */
    if ( platform_mfs_manager_acquire_channel( i2c->port ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    /* Initialize the MFS I2C channel instance */
    if ( Mfs_I2c_Init( mfsn_i2c[i2c->port], &mfs_i2c_config ) != Ok )
    {
        platform_mfs_manager_release_channel( i2c->port );
        return PLATFORM_ERROR;
    }

    platform_mcu_powersave_enable( );

    return PLATFORM_SUCCESS;
}

static platform_result_t mfs_i2c_deinit( const platform_i2c_t* i2c, const platform_i2c_config_t* config )
{
    UNUSED_PARAMETER( config );

    if ( i2c == NULL )
    {
        wiced_assert( "bad argument", 0 );
        return PLATFORM_ERROR;
    }

    platform_mcu_powersave_disable( );

    if ( Mfs_I2c_DeInit( mfsn_i2c[i2c->port], FALSE ) != Ok )
    {
        return PLATFORM_ERROR;
    }

    if ( platform_mfs_manager_release_channel( i2c->port ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    platform_mcu_powersave_enable( );

    return PLATFORM_SUCCESS;
}

platform_result_t platform_i2c_init( const platform_i2c_t* i2c, const platform_i2c_config_t* config )
{
    platform_result_t result;

    wiced_assert( "bad argument", ( i2c != NULL ) && ( i2c_driver != NULL ) && ( config != NULL ) && ( config->flags & I2C_DEVICE_USE_DMA ) == 0);

    result = i2c_driver->init(i2c, config);

    return result;
}

platform_result_t platform_i2c_deinit( const platform_i2c_t* i2c, const platform_i2c_config_t* config )
{
    platform_result_t result;

    wiced_assert( "bad argument", ( i2c != NULL ) && ( i2c_driver != NULL ) && ( config != NULL ) );

    result = i2c_driver->deinit(i2c, config);

    return result;
}

wiced_bool_t platform_i2c_probe_device( const platform_i2c_t* i2c, const platform_i2c_config_t* config, int retries )
{
    uint32_t    i;
    uint8_t     dummy[2];
    platform_result_t result;

    /* Read two bytes from the addressed-slave. The slave-address won't be
     * acknowledged if it isn't on the I2C bus. The read won't trigger
     * a NACK from the slave (unless of error), since only the receiver can do that.
     */
    for ( i = 0; i < retries; i++ )
    {
        result = i2c_driver->read(i2c, config, I2C_START_FLAG | I2C_STOP_FLAG, dummy, sizeof dummy);

        if (  result == PLATFORM_SUCCESS )
        {
            return WICED_TRUE;
        }
    }

    return WICED_FALSE;
}

platform_result_t platform_i2c_init_tx_message( platform_i2c_message_t* message, const void* tx_buffer, uint16_t tx_buffer_length, uint16_t retries, wiced_bool_t disable_dma )
{
    wiced_assert( "bad argument", ( message != NULL ) && ( tx_buffer != NULL ) && ( tx_buffer_length != 0 ) );

    /* TBD: I2C DMA support */
    if ( disable_dma == WICED_FALSE )
    {
        return PLATFORM_UNSUPPORTED;
    }

    memset( message, 0x00, sizeof( *message ) );
    message->tx_buffer = tx_buffer;
    message->retries   = retries;
    message->tx_length = tx_buffer_length;
    message->flags     = 0;

    return PLATFORM_SUCCESS;
}

platform_result_t platform_i2c_init_rx_message( platform_i2c_message_t* message, void* rx_buffer, uint16_t rx_buffer_length, uint16_t retries, wiced_bool_t disable_dma )
{
    wiced_assert( "bad argument", ( message != NULL ) && ( rx_buffer != NULL ) && ( rx_buffer_length != 0 ) );

    /* TBD: I2C DMA support */
    if ( disable_dma == WICED_FALSE )
    {
        return PLATFORM_UNSUPPORTED;
    }

    memset( message, 0x00, sizeof( *message ) );

    message->rx_buffer = rx_buffer;
    message->retries   = retries;
    message->rx_length = rx_buffer_length;
    message->flags     = 0;

    return PLATFORM_SUCCESS;
}

platform_result_t platform_i2c_transfer( const platform_i2c_t* i2c, const platform_i2c_config_t* config, platform_i2c_message_t* messages, uint16_t number_of_messages )
{
    platform_result_t result = PLATFORM_SUCCESS;
    uint32_t          message_count;
    uint32_t          retries;

    /* Check for message validity. Combo message is unsupported */
    for ( message_count = 0; message_count < number_of_messages; message_count++ )
    {
        if ( messages[message_count].rx_buffer != NULL && messages[message_count].tx_buffer != NULL )
        {
            return PLATFORM_UNSUPPORTED;
        }

        if ( (messages[message_count].tx_buffer == NULL && messages[message_count].tx_length != 0)  ||
             (messages[message_count].tx_buffer != NULL && messages[message_count].tx_length == 0)  ||
             (messages[message_count].rx_buffer == NULL && messages[message_count].rx_length != 0)  ||
             (messages[message_count].rx_buffer != NULL && messages[message_count].rx_length == 0)  )
        {
            return PLATFORM_BADARG;
        }

        if ( messages[message_count].tx_buffer == NULL && messages[message_count].rx_buffer == NULL )
        {
            return PLATFORM_BADARG;
        }
    }

    for ( message_count = 0; message_count < number_of_messages; message_count++ )
    {
        for ( retries = 0; retries < messages[message_count].retries; retries++ )
        {
            if ( messages[message_count].tx_length != 0 )
            {
                result = i2c_driver->write(i2c, config, I2C_START_FLAG | I2C_STOP_FLAG, (uint8_t*) messages[message_count].tx_buffer, messages[message_count].tx_length);

                if ( result == PLATFORM_SUCCESS )
                {
                    /* Transaction successful. Break from the inner loop and continue with the next message */
                    break;
                }
            }
            else if ( messages[message_count].rx_length != 0 )
            {
                result = i2c_driver->read(i2c, config, I2C_START_FLAG | I2C_STOP_FLAG, (uint8_t*) messages[message_count].rx_buffer, messages[message_count].rx_length);

                if ( result == PLATFORM_SUCCESS )
                {
                    /* Transaction successful. Break from the inner loop and continue with the next message */
                    break;
                }
            }
        }

        /* Check if retry is maxed out. If yes, return immediately */
        if ( retries == messages[message_count].retries && result != PLATFORM_SUCCESS )
        {
            break;
        }
    }

    return result;
}

platform_result_t platform_i2c_write( const platform_i2c_t* i2c, const platform_i2c_config_t* config, uint16_t flags, const void* buffer, uint16_t buffer_length )
{
    platform_result_t result;

    result = i2c_driver->transfer(i2c, config, flags, (void *) buffer, buffer_length, i2c_driver->write);

    return result;
}

platform_result_t platform_i2c_read( const platform_i2c_t* i2c, const platform_i2c_config_t* config, uint16_t flags, void* buffer, uint16_t buffer_length )
{
    platform_result_t result;

    result = i2c_driver->transfer(i2c, config, flags, (void *) buffer, buffer_length, i2c_driver->read);

    return result;
}
