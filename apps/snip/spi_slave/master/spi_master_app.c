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
 * SPI Master Application
 *
 * This application demonstrates how to drive a WICED SPI slave device provided in apps/snip/spi_slave
 *
 */

#include "wiced.h"
#include "spi_master.h"
#include "../spi_slave_app.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

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
 *               Static Function Declarations
 ******************************************************/

static void data_ready_callback( spi_master_t* host );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static spi_master_t             spi_master;
#ifdef BCM958100SVK
static const wiced_spi_device_t spi_device =
{
    .port        = WICED_SPI_2,
    .chip_select = WICED_GPIO_NONE,
    .speed       = SPI_CLOCK_SPEED_HZ,
    .mode        = SPI_MODE,
    .bits        = SPI_BIT_WIDTH
};
#else
static const wiced_spi_device_t spi_device =
{
    .port        = WICED_SPI_1,
    .chip_select = WICED_GPIO_12,
    .speed       = SPI_CLOCK_SPEED_HZ,
    .mode        = SPI_MODE,
    .bits        = SPI_BIT_WIDTH
};
#endif

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( void )
{
    uint32_t       a;
    uint32_t       device_id;
    wiced_result_t result;
    uint32_t       led1_state = WICED_FALSE;
    uint32_t       led2_state = WICED_FALSE;

    /* Initialise the WICED device */
    wiced_init();

    /* Initialise SPI slave device */
#ifdef BCM958100SVK
    spi_master_init( &spi_master, &spi_device, WICED_GPIO_56, data_ready_callback );
#else
    spi_master_init( &spi_master, &spi_device, WICED_GPIO_11, data_ready_callback );
#endif

    /* Read device ID */
    result = spi_master_read_register( &spi_master, REGISTER_DEVICE_ID_ADDRESS, REGISTER_DEVICE_ID_LENGTH, (uint8_t*)&device_id );
    if ( result == WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ( "Device ID : 0x%.08X\n", (unsigned int)device_id ) );
    }
    else
    {
        WPRINT_APP_INFO( ( "Retrieving device ID failed. Please check hardware connections\n" ) );
    }

    WPRINT_APP_INFO( ( "LED 1 and 2 on the SPI slave device will start blinking alternately\n" ) );

    /* Toggle LED1 and 2 alternately */
    for ( a = 0; a < 10; a++ )
    {
        if ( led1_state == WICED_TRUE )
        {
            /* Switch LED1 on and LED2 off */
            led1_state = WICED_FALSE;
            led2_state = WICED_TRUE;
        }
        else
        {
            /* Switch LED1 off and LED2 on */
            led1_state = WICED_TRUE;
            led2_state = WICED_FALSE;
        }

        spi_master_write_register( &spi_master, REGISTER_LED1_CONTROL_ADDRESS, REGISTER_LED1_CONTROL_LENGTH, (uint8_t*)&led1_state );
        spi_master_write_register( &spi_master, REGISTER_LED2_CONTROL_ADDRESS, REGISTER_LED2_CONTROL_LENGTH, (uint8_t*)&led2_state );
        wiced_rtos_delay_milliseconds( 500 );
    }

    WPRINT_APP_INFO( ( "Test invalid register address ..." ) );

    result = spi_master_write_register( &spi_master, 0x0004, 1, (uint8_t*)&led1_state );
    if ( result == WICED_PLATFORM_SPI_SLAVE_ADDRESS_UNAVAILABLE )
    {
        WPRINT_APP_INFO( ( "success\n" ) );
    }
    else
    {
        WPRINT_APP_INFO( ( "failed\n" ) );
    }

}

static void data_ready_callback( spi_master_t* host )
{
    UNUSED_PARAMETER( host );
    /* This callback is called when the SPI slave device asynchronously indicates that a new data is available */
}
