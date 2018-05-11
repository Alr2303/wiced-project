/*
 * Copyright 2016, Cypress Semiconductor Corporation or a subsidiary of
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
 * Defines board support package for BCM943362WCD4 board
 */
#include "platform.h"
#include "platform_config.h"
#include "platform_init.h"
#include "platform_isr.h"
#include "platform_peripheral.h"
#include "wwd_platform_common.h"
#include "wwd_rtos_isr.h"
#include "wiced_defaults.h"
#include "wiced_platform.h"

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

/******************************************************
 *               Variable Definitions
 ******************************************************/

/* GPIO pin table. Used by WICED/platform/MCU/wiced_platform_common.c */
const platform_gpio_t platform_gpio_pins[] =
{
    [WICED_GPIO_2]   = { GPIOB,  2 },
    [WICED_GPIO_4]   = { GPIOA,  7 },
    [WICED_GPIO_5]   = { GPIOA, 15 },
    [WICED_GPIO_6]   = { GPIOB,  3 },
    [WICED_GPIO_7]   = { GPIOB,  4 },
    [WICED_GPIO_8]   = { GPIOA,  2 },
    [WICED_GPIO_9]   = { GPIOA,  1 },
    [WICED_GPIO_12]  = { GPIOA,  3 },
    [WICED_GPIO_14]  = { GPIOA,  0 },
    [WICED_GPIO_16]  = { GPIOC, 13 },
    [WICED_GPIO_17]  = { GPIOB, 10 },
    [WICED_GPIO_18]  = { GPIOB,  9 },
    [WICED_GPIO_19]  = { GPIOB, 12 },
    /* [WICED_GPIO_22]  = { GPIOB,  3 }, */
    /* [WICED_GPIO_23]  = { GPIOA, 15 }, */
    /* [WICED_GPIO_24]  = { GPIOB,  4 }, */
    /* WICED_GPIO_25,	*/	/* WIFI_SWCLK */
    /* WICED_GPIO_26,   */	/* WIFI_SWDIO */
    [WICED_GPIO_27]  = { GPIOA, 12},
    [WICED_GPIO_29]  = { GPIOA, 10 },
    [WICED_GPIO_30]  = { GPIOB,  6 },
    [WICED_GPIO_31]  = { GPIOB,  8 },
    [WICED_GPIO_33]  = { GPIOB, 13 },
    [WICED_GPIO_34]  = { GPIOA,  5 },
    [WICED_GPIO_35]  = { GPIOA, 11 },
    [WICED_GPIO_36]  = { GPIOB,  1 },
    [WICED_GPIO_37]  = { GPIOB,  0 },
    [WICED_GPIO_38]  = { GPIOA,  4 },
};

/* ADC peripherals. Used WICED/platform/MCU/wiced_platform_common.c */
const platform_adc_t platform_adc_peripherals[] =
{
    [WICED_ADC_1] = {ADC1, ADC_Channel_1, RCC_APB2Periph_ADC1, 1, &platform_gpio_pins[WICED_GPIO_9]},
};

/* PWM peripherals. Used by WICED/platform/MCU/wiced_platform_common.c */
const platform_pwm_t platform_pwm_peripherals[] =
{
    [WICED_PWM_1]  = {TIM1, 1, RCC_APB2Periph_TIM1, GPIO_AF_TIM1, &platform_gpio_pins[WICED_GPIO_33]}, /* PB13, TIM1_CH1N */
    [WICED_PWM_2]  = {TIM2, 1, RCC_APB1Periph_TIM2, GPIO_AF_TIM2, &platform_gpio_pins[WICED_GPIO_34]}, /* PA5, TIM2_CH1/TIM2_ET */
    [WICED_PWM_3]  = {TIM1, 3, RCC_APB2Periph_TIM1, GPIO_AF_TIM1, &platform_gpio_pins[WICED_GPIO_36] }, /* PB1, TIM1_CH3N */
    [WICED_PWM_4]  = {TIM1, 2, RCC_APB2Periph_TIM1, GPIO_AF_TIM1, &platform_gpio_pins[WICED_GPIO_37] }, /* PB0, TIM1_CH2N */
};

/* SPI peripherals. Used by WICED/platform/MCU/wiced_platform_common.c */
const platform_spi_t platform_spi_peripherals[] =
{
    [WICED_SPI_1]  =
    {
        .port                  = SPI1,
        .gpio_af               = GPIO_AF_SPI1,
        .peripheral_clock_reg  = RCC_APB2Periph_SPI1,
        .peripheral_clock_func = RCC_APB2PeriphClockCmd,
        .pin_mosi              = &platform_gpio_pins[FLASH_PIN_SPI_MOSI],
        .pin_miso              = &platform_gpio_pins[FLASH_PIN_SPI_MISO],
        .pin_clock             = &platform_gpio_pins[FLASH_PIN_SPI_CLK],
        .tx_dma =
        {
            .controller        = DMA2,
            .stream            = DMA2_Stream5,
            .channel           = DMA_Channel_3,
            .irq_vector        = DMA2_Stream5_IRQn,
            .complete_flags    = DMA_FLAG_TCIF5, //DMA_HISR_TCIF5,
            .error_flags       = ( DMA_HISR_TEIF5 | DMA_HISR_FEIF5 | DMA_HISR_DMEIF5 ),
        },
        .rx_dma =
        {
            .controller        = DMA2,
            .stream            = DMA2_Stream0,
            .channel           = DMA_Channel_3,
            .irq_vector        = DMA2_Stream0_IRQn,
            .complete_flags    = DMA_FLAG_TCIF0, //DMA_LISR_TCIF0,
            .error_flags       = ( DMA_LISR_TEIF0 | DMA_LISR_FEIF0 | DMA_LISR_DMEIF0 ),
        },
    }
};

/* UART peripherals and runtime drivers. Used by WICED/platform/MCU/wiced_platform_common.c */
const platform_uart_t platform_uart_peripherals[] =
{
    [WICED_UART_1] =
    {
        .port               = USART1,
        .tx_pin             = &platform_gpio_pins[STDIO_UART1_TX],
        .rx_pin             = &platform_gpio_pins[STDIO_UART1_RX],
        /* .cts_pin            = &platform_gpio_pins[STDIO_UART1_CTS], */
        /* .rts_pin            = &platform_gpio_pins[STDIO_UART1_RTS], */
        .tx_dma_config =
        {
            .controller     = DMA2,
            .stream         = DMA2_Stream7,
            .channel        = DMA_Channel_4,
            .irq_vector     = DMA2_Stream7_IRQn,
            .complete_flags = DMA_HISR_TCIF7,
            .error_flags    = ( DMA_HISR_TEIF7 | DMA_HISR_FEIF7 ),
        },
        .rx_dma_config =
        {
            .controller     = DMA2,
            .stream         = DMA2_Stream2,
            .channel        = DMA_Channel_4,
            .irq_vector     = DMA2_Stream2_IRQn,
            .complete_flags = DMA_LISR_TCIF2,
            .error_flags    = ( DMA_LISR_TEIF2 | DMA_LISR_FEIF2 | DMA_LISR_DMEIF2 ),
        },
    },
    [WICED_UART_2] =
    {
        .port               = USART6,
        .tx_pin             = &platform_gpio_pins[BT_UART6_TX],
        .rx_pin             = &platform_gpio_pins[BT_UART6_RX],
        /* .cts_pin            = &platform_gpio_pins[BT_UART6_CTS], */
        /* .rts_pin            = &platform_gpio_pins[BT_UART6_RTS], */
        .tx_dma_config =
        {
            .controller     = DMA2,
            .stream         = DMA2_Stream6,
            .channel        = DMA_Channel_5,
            .irq_vector     = DMA2_Stream6_IRQn,
            .complete_flags = DMA_HISR_TCIF6,
            .error_flags    = ( DMA_HISR_TEIF6 | DMA_HISR_FEIF6 ),
        },
        .rx_dma_config =
        {
            .controller     = DMA2,
            .stream         = DMA2_Stream1,
            .channel        = DMA_Channel_5,
            .irq_vector     = DMA2_Stream1_IRQn,
            .complete_flags = DMA_LISR_TCIF1,
            .error_flags    = ( DMA_LISR_TEIF1 | DMA_LISR_FEIF1 | DMA_LISR_DMEIF1 ),
        },
    },
};
platform_uart_driver_t platform_uart_drivers[WICED_UART_MAX];

const platform_i2c_t platform_i2c_peripherals[] =
{
    [WICED_I2C_1] =
    {
        .port                    = I2C1,
        .pin_scl                 = &platform_gpio_pins[WICED_GPIO_31],
        .pin_sda                 = &platform_gpio_pins[WICED_GPIO_18],
        .peripheral_clock_reg    = RCC_APB1Periph_I2C1,
        .tx_dma                  = DMA1,
        .tx_dma_peripheral_clock = RCC_AHB1Periph_DMA1,
        .tx_dma_stream           = DMA1_Stream7,
        .rx_dma_stream           = DMA1_Stream0,
        .tx_dma_stream_id        = 7,
        .rx_dma_stream_id        = 0,
        .tx_dma_channel          = DMA_Channel_1,
        .rx_dma_channel          = DMA_Channel_1,
        .gpio_af                 = GPIO_AF_I2C1
    },
};

/* SPI flash. Exposed to the applications through include/wiced_platform.h */
#if defined ( WICED_PLATFORM_INCLUDES_SPI_FLASH )
const wiced_spi_device_t wiced_spi_flash =
{
    .port        = WICED_SPI_1,
    .chip_select = FLASH_PIN_SPI_CS,
    .speed       = 5000000,
    .mode        = (SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH | SPI_NO_DMA | SPI_MSB_FIRST ),
    .bits        = 8
};
#endif

/* UART standard I/O configuration */
#if !defined(WICED_DISABLE_STDIO)
static platform_uart_config_t stdio_config =
{
    .baud_rate    = 115200,
    .data_width   = DATA_WIDTH_8BIT,
    .parity       = NO_PARITY,
    .stop_bits    = STOP_BITS_1,
    .flow_control = FLOW_CONTROL_DISABLED,
};
#endif

/* Wi-Fi control pins. Used by WICED/platform/MCU/wwd_platform_common.c
 * SDIO: WWD_PIN_BOOTSTRAP[1:0] = b'00
 * gSPI: WWD_PIN_BOOTSTRAP[1:0] = b'01
 */
const platform_gpio_t wifi_control_pins[] =
{
   [WWD_PIN_RESET]       = { GPIOB, 14 },
};

/* Wi-Fi SDIO bus pins. Used by WICED/platform/STM32F2xx/WWD/wwd_SDIO.c */
const platform_gpio_t wifi_sdio_pins[] =
{
    [WWD_PIN_SDIO_OOB_IRQ] = { GPIOA,  0 },
    [WWD_PIN_SDIO_CLK    ] = { GPIOB, 15 },
    [WWD_PIN_SDIO_CMD    ] = { GPIOA,  6 },
    [WWD_PIN_SDIO_D0     ] = { GPIOB,  7 },
    [WWD_PIN_SDIO_D1     ] = { GPIOA,  8 },
    [WWD_PIN_SDIO_D2     ] = { GPIOA,  9 },
    [WWD_PIN_SDIO_D3     ] = { GPIOB,  5 },
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void platform_init_peripheral_irq_priorities( void )
{
    /* Interrupt priority setup. Called by WICED/platform/MCU/STM32F2xx/platform_init.c */
    NVIC_SetPriority( RTC_WKUP_IRQn    ,  1 ); /* RTC Wake-up event   */
    NVIC_SetPriority( SDIO_IRQn        ,  2 ); /* WLAN SDIO           */
    NVIC_SetPriority( DMA2_Stream3_IRQn,  3 ); /* WLAN SDIO DMA       */
    /* NVIC_SetPriority( DMA1_Stream3_IRQn,  3 ); /\* WLAN SPI DMA        *\/ */
    NVIC_SetPriority( USART1_IRQn      ,  6 ); /* WICED_UART_1        */
    NVIC_SetPriority( USART2_IRQn      ,  6 ); /* WICED_UART_2        */
    NVIC_SetPriority( USART6_IRQn      ,  6 ); /* WICED_UART_6        */
    NVIC_SetPriority( DMA2_Stream7_IRQn,  7 ); /* WICED_UART_1 TX DMA */
    NVIC_SetPriority( DMA2_Stream2_IRQn,  7 ); /* WICED_UART_1 RX DMA */
    NVIC_SetPriority( DMA2_Stream6_IRQn,  7 ); /* WICED_UART_6 TX DMA */
    NVIC_SetPriority( DMA2_Stream1_IRQn,  7 ); /* WICED_UART_6 RX DMA */
    NVIC_SetPriority( DMA1_Stream7_IRQn,  8 ); /* I2C TX */
    NVIC_SetPriority( DMA1_Stream0_IRQn,  8 ); /* I2C RX */
    NVIC_SetPriority( EXTI0_IRQn       , 14 ); /* GPIO                */
    NVIC_SetPriority( EXTI1_IRQn       , 14 ); /* GPIO                */
    NVIC_SetPriority( EXTI2_IRQn       , 14 ); /* GPIO                */
    NVIC_SetPriority( EXTI3_IRQn       , 14 ); /* GPIO                */
    NVIC_SetPriority( EXTI4_IRQn       , 14 ); /* GPIO                */
    NVIC_SetPriority( EXTI9_5_IRQn     , 14 ); /* GPIO                */
    NVIC_SetPriority( EXTI15_10_IRQn   , 14 ); /* GPIO                */
}

void platform_init_external_devices( void )
{
#ifndef WICED_NO_WIFI

#ifndef WICED_DISABLE_STDIO
    /* Initialise UART standard I/O */
    platform_stdio_init( &platform_uart_drivers[STDIO_UART], &platform_uart_peripherals[STDIO_UART], &stdio_config );
#endif

    /* initialize bluetooth & sensor board pins */
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_2], OUTPUT_PUSH_PULL );
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_8], INPUT_HIGH_IMPEDANCE );
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_9], INPUT_HIGH_IMPEDANCE );
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_12], INPUT_HIGH_IMPEDANCE );
    platform_gpio_init( &platform_gpio_pins[WICED_FACTORY], INPUT_HIGH_IMPEDANCE );
    platform_gpio_init( &platform_gpio_pins[BT_PAIR], INPUT_PULL_UP );
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_17], INPUT_HIGH_IMPEDANCE );
    platform_gpio_init( &platform_gpio_pins[BT_RESET], OUTPUT_PUSH_PULL );

    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_33], OUTPUT_PUSH_PULL );
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_33], OUTPUT_PUSH_PULL );
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_34], OUTPUT_PUSH_PULL );
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_36], OUTPUT_PUSH_PULL );
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_37], OUTPUT_PUSH_PULL );
    platform_gpio_init( &platform_gpio_pins[WICED_GPIO_38], OUTPUT_PUSH_PULL );

    platform_gpio_output_low( &platform_gpio_pins[BT_RESET] );
    host_rtos_delay_milliseconds( 100 );
#endif
}

/* Checks if a factory reset is requested */
#if 1
uint32_t platform_get_factory_reset_button_time ( uint32_t max_time )
{
	return 0;
}
#else
wiced_bool_t platform_check_factory_reset( void )
{
    uint32_t factory_reset_counter = 0;
    while (  ( 0 == platform_gpio_input_get( &platform_gpio_pins[ WICED_FACTORY ] ) )
          && ( ( factory_reset_counter += 100 ) <= 4000 )
          && ( WICED_SUCCESS == (wiced_result_t)host_rtos_delay_milliseconds( 100 ) )
          )
    {
        /* Factory reset button is being pressed. */
        /* User Must press it for 4 seconds to ensure it was not accidental */
        if ( factory_reset_counter == 4000 )
        {
            return WICED_TRUE;
        }
    }
    return WICED_FALSE;
}
#endif
/******************************************************
 *           Interrupt Handler Definitions
 ******************************************************/

WWD_RTOS_DEFINE_ISR( usart1_irq )
{
    platform_uart_irq( &platform_uart_drivers[WICED_UART_1] );
}

WWD_RTOS_DEFINE_ISR( usart2_irq )
{
    platform_uart_irq( &platform_uart_drivers[WICED_UART_2] );
}

WWD_RTOS_DEFINE_ISR( usart1_tx_dma_irq )
{
    platform_uart_tx_dma_irq( &platform_uart_drivers[WICED_UART_1] );
}

WWD_RTOS_DEFINE_ISR( usart2_tx_dma_irq )
{
    platform_uart_tx_dma_irq( &platform_uart_drivers[WICED_UART_2] );
}

WWD_RTOS_DEFINE_ISR( usart1_rx_dma_irq )
{
    platform_uart_rx_dma_irq( &platform_uart_drivers[WICED_UART_1] );
}

WWD_RTOS_DEFINE_ISR( usart2_rx_dma_irq )
{
    platform_uart_rx_dma_irq( &platform_uart_drivers[WICED_UART_2] );
}

/******************************************************
 *            Interrupt Handlers Mapping
 ******************************************************/

/* These DMA assignments can be found STM32F2xx datasheet DMA section */
WWD_RTOS_MAP_ISR( usart1_irq       , USART1_irq       )
WWD_RTOS_MAP_ISR( usart1_tx_dma_irq, DMA2_Stream7_irq )
WWD_RTOS_MAP_ISR( usart1_rx_dma_irq, DMA2_Stream2_irq )
WWD_RTOS_MAP_ISR( usart2_irq       , USART6_irq       )
WWD_RTOS_MAP_ISR( usart2_tx_dma_irq, DMA2_Stream6_irq )
WWD_RTOS_MAP_ISR( usart2_rx_dma_irq, DMA2_Stream1_irq )
