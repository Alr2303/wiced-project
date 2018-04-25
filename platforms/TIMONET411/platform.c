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
 * Defines board support package for SPIL N08 based BCM94343WWCDA combo board
 */
#include <platform_peripheral.h>
#include "platform.h"
#include "platform_config.h"
#include "platform_init.h"
#include "platform_isr.h"
#include "platform_peripheral.h"
#include "wwd_platform_common.h"
#include "wwd_rtos_isr.h"
#include "wiced_defaults.h"
#include "wiced_platform.h"
#include "platform_bluetooth.h"
#include "platform_button.h"
#include "gpio_button.h"

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* GPIO pin table. Used by WICED/platform/MCU/wiced_platform_common.c */
const platform_gpio_t platform_gpio_pins[] =
{
	[WICED_GPIO_1 ]={GPIOA, 0},[WICED_GPIO_2 ]={GPIOA, 1},[WICED_GPIO_3 ]={GPIOA, 2},[WICED_GPIO_4 ]={GPIOA, 3},
	[WICED_GPIO_5 ]={GPIOA, 4},[WICED_GPIO_6 ]={GPIOA, 5},[WICED_GPIO_7 ]={GPIOA, 6},[WICED_GPIO_8 ]={GPIOA, 7},
	[WICED_GPIO_9 ]={GPIOA, 8},[WICED_GPIO_10]={GPIOA, 9},[WICED_GPIO_11]={GPIOA,10},[WICED_GPIO_12]={GPIOA,11},
	[WICED_GPIO_13]={GPIOA,12},[WICED_GPIO_14]={GPIOA,13},[WICED_GPIO_15]={GPIOA,14},[WICED_GPIO_16]={GPIOA,15},
	[WICED_GPIO_17]={GPIOB, 0},[WICED_GPIO_18]={GPIOB, 1},[WICED_GPIO_19]={GPIOB, 2},[WICED_GPIO_20]={GPIOB, 3},
	[WICED_GPIO_21]={GPIOB, 4},[WICED_GPIO_22]={GPIOB, 5},[WICED_GPIO_23]={GPIOB, 6},[WICED_GPIO_24]={GPIOB, 7},
	[WICED_GPIO_25]={GPIOB, 8},[WICED_GPIO_26]={GPIOB, 9},[WICED_GPIO_27]={GPIOB,10},[WICED_GPIO_28]={GPIOB,11},
	[WICED_GPIO_29]={GPIOB,12},[WICED_GPIO_30]={GPIOB,13},[WICED_GPIO_31]={GPIOB,14},[WICED_GPIO_32]={GPIOB,15},
	[WICED_GPIO_33]={GPIOC, 0},[WICED_GPIO_34]={GPIOC, 1},[WICED_GPIO_35]={GPIOC, 2},[WICED_GPIO_36]={GPIOC, 3},
	[WICED_GPIO_37]={GPIOC, 4},[WICED_GPIO_38]={GPIOC, 5},[WICED_GPIO_39]={GPIOC, 6},[WICED_GPIO_40]={GPIOC, 7},
	[WICED_GPIO_41]={GPIOC, 8},[WICED_GPIO_42]={GPIOC, 9},[WICED_GPIO_43]={GPIOC,10},[WICED_GPIO_44]={GPIOC,11},
	[WICED_GPIO_45]={GPIOC,12},[WICED_GPIO_46]={GPIOC,13},[WICED_GPIO_47]={GPIOC,14},[WICED_GPIO_48]={GPIOC,15},
	[WICED_GPIO_49]={GPIOD, 0},[WICED_GPIO_50]={GPIOD, 1},[WICED_GPIO_51]={GPIOD, 2},[WICED_GPIO_52]={GPIOD, 3},
	[WICED_GPIO_53]={GPIOD, 4},[WICED_GPIO_54]={GPIOD, 5},[WICED_GPIO_55]={GPIOD, 6},[WICED_GPIO_56]={GPIOD, 7},
	[WICED_GPIO_57]={GPIOD, 8},[WICED_GPIO_58]={GPIOD, 9},[WICED_GPIO_59]={GPIOD,10},[WICED_GPIO_60]={GPIOD,11},
	[WICED_GPIO_61]={GPIOD,12},[WICED_GPIO_62]={GPIOD,13},[WICED_GPIO_63]={GPIOD,14},[WICED_GPIO_64]={GPIOD,15},
	[WICED_GPIO_65]={GPIOE, 0},[WICED_GPIO_66]={GPIOE, 1},[WICED_GPIO_67]={GPIOE, 2},[WICED_GPIO_68]={GPIOE, 3},
	[WICED_GPIO_69]={GPIOE, 4},[WICED_GPIO_70]={GPIOE, 5},[WICED_GPIO_71]={GPIOE, 6},[WICED_GPIO_72]={GPIOE, 7},
	[WICED_GPIO_73]={GPIOE, 8},[WICED_GPIO_74]={GPIOE, 9},[WICED_GPIO_75]={GPIOE,10},[WICED_GPIO_76]={GPIOE,11},
	[WICED_GPIO_77]={GPIOE,12},[WICED_GPIO_78]={GPIOE,13},[WICED_GPIO_79]={GPIOE,14},[WICED_GPIO_80]={GPIOE,15},
};

/* ADC peripherals. Used WICED/platform/MCU/wiced_platform_common.c */
const platform_adc_t platform_adc_peripherals[] =
{
	[WICED_ADC_1] = {ADC1, ADC_Channel_1, RCC_APB2Periph_ADC1, 1, &platform_gpio_pins[WICED_GPIO_2]},
	[WICED_ADC_2] = {ADC1, ADC_Channel_2, RCC_APB2Periph_ADC1, 1, &platform_gpio_pins[WICED_GPIO_3]},
};

/* PWM peripherals. Used by WICED/platform/MCU/wiced_platform_common.c */
const platform_pwm_t platform_pwm_peripherals[] =
{
	[WICED_PWM_1]  = {TIM1, 2, RCC_APB2Periph_TIM1, GPIO_AF_TIM1, &platform_gpio_pins[WICED_GPIO_31]}, /* PB14, TIM1_CH2N */
	[WICED_PWM_2]  = {TIM1, 3, RCC_APB2Periph_TIM1, GPIO_AF_TIM1, &platform_gpio_pins[WICED_GPIO_32]}, /* PB15, TIM1_CH3N */
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
		.pin_mosi              = &platform_gpio_pins[WICED_GPIO_8],
		.pin_miso              = &platform_gpio_pins[WICED_GPIO_7],
		.pin_clock             = &platform_gpio_pins[WICED_GPIO_6],
		.tx_dma =
		{
			.controller        = DMA2,
			.stream            = DMA2_Stream5,
			.channel           = DMA_Channel_3,
			.irq_vector        = DMA2_Stream5_IRQn,
			.complete_flags    = DMA_HISR_TCIF5,
			.error_flags       = ( DMA_HISR_TEIF5 | DMA_HISR_FEIF5 | DMA_HISR_DMEIF5 ),
		},
		.rx_dma =
		{
			.controller        = DMA2,
			.stream            = DMA2_Stream0,
			.channel           = DMA_Channel_3,
			.irq_vector        = DMA2_Stream0_IRQn,
			.complete_flags    = DMA_LISR_TCIF0,
			.error_flags       = ( DMA_LISR_TEIF0 | DMA_LISR_FEIF0 | DMA_LISR_DMEIF0 ),
		},
	},
};

/* UART peripherals and runtime drivers. Used by WICED/platform/MCU/wiced_platform_common.c */
const platform_uart_t platform_uart_peripherals[] =
{
	[WICED_UART_1] =
	{
		.port               = USART1,
		.tx_pin             = &platform_gpio_pins[WICED_GPIO_10],
		.rx_pin             = &platform_gpio_pins[WICED_GPIO_11],
		.cts_pin            = NULL,
		.rts_pin            = NULL,
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
};
platform_uart_driver_t platform_uart_drivers[WICED_UART_MAX];

/* SPI flash. Exposed to the applications through include/wiced_platform.h */
#if defined ( WICED_PLATFORM_INCLUDES_SPI_FLASH )
const wiced_spi_device_t wiced_spi_flash =
{
	.port        = WICED_SPI_1,
	.chip_select = WICED_SPI_FLASH_CS,
	.speed       = 50000000,
	.mode        = (SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH | SPI_NO_DMA | SPI_MSB_FIRST),
	.bits        = 8
};
#endif

/* UART standard I/O configuration */
#ifndef WICED_DISABLE_STDIO
static const platform_uart_config_t stdio_config =
{
	.baud_rate    = 115200,
	.data_width   = DATA_WIDTH_8BIT,
	.parity       = NO_PARITY,
	.stop_bits    = STOP_BITS_1,
	.flow_control = FLOW_CONTROL_DISABLED,
};
#endif

/* Wi-Fi control pins. Used by WICED/platform/MCU/wwd_platform_common.c */
const platform_gpio_t wifi_control_pins[] =
{
	[WWD_PIN_POWER]       = { GPIOD, 11 },
	[WWD_PIN_32K_CLK]     = { GPIOA,  8 },
	[WWD_PIN_BOOTSTRAP_0] = { GPIOD, 12 },
	[WWD_PIN_BOOTSTRAP_1] = { GPIOD, 13 },
};

/* Wi-Fi SDIO bus pins. Used by WICED/platform/STM32F2xx/WWD/wwd_SDIO.c */
const platform_gpio_t wifi_sdio_pins[] =
{
	[WWD_PIN_SDIO_OOB_IRQ] = { GPIOD, 12 },
	[WWD_PIN_SDIO_CLK]     = { GPIOC, 12 },
	[WWD_PIN_SDIO_CMD]     = { GPIOD,  2 },
	[WWD_PIN_SDIO_D0]      = { GPIOC,  8 },
	[WWD_PIN_SDIO_D1]      = { GPIOC,  9 },
	[WWD_PIN_SDIO_D2]      = { GPIOC, 10 },
	[WWD_PIN_SDIO_D3]      = { GPIOC, 11 },
};

/* Bluetooth control pins. Used by libraries/bluetooth/internal/bus/UART/bt_bus.c */
static const platform_gpio_t internal_bt_control_pins[] =
{
    /* Reset pin unavailable */
	[WICED_BT_PIN_POWER      ] = { GPIOD,  0 },
	[WICED_BT_PIN_HOST_WAKE  ] = { GPIOB,  5 },
	[WICED_BT_PIN_DEVICE_WAKE] = { GPIOD,  7 }
};
const platform_gpio_t* wiced_bt_control_pins[] =
{
    /* Reset pin unavailable */
	[WICED_BT_PIN_POWER      ] = &internal_bt_control_pins[WICED_BT_PIN_POWER      ],
	[WICED_BT_PIN_HOST_WAKE  ] = &internal_bt_control_pins[WICED_BT_PIN_HOST_WAKE  ],
	[WICED_BT_PIN_DEVICE_WAKE] = &internal_bt_control_pins[WICED_BT_PIN_DEVICE_WAKE],
	[WICED_BT_PIN_RESET      ] = NULL,
};

/* Bluetooth UART pins. Used by libraries/bluetooth/internal/bus/UART/bt_bus.c */
static const platform_gpio_t internal_bt_uart_pins[] =
{
	[WICED_BT_PIN_UART_TX ] = { GPIOD, 5 },
	[WICED_BT_PIN_UART_RX ] = { GPIOD, 6 },
	[WICED_BT_PIN_UART_CTS] = { GPIOD, 3 },
	[WICED_BT_PIN_UART_RTS] = { GPIOD, 4 },
};
const platform_gpio_t* wiced_bt_uart_pins[] =
{
	[WICED_BT_PIN_UART_TX ] = &internal_bt_uart_pins[WICED_BT_PIN_UART_TX ],
	[WICED_BT_PIN_UART_RX ] = &internal_bt_uart_pins[WICED_BT_PIN_UART_RX ],
	[WICED_BT_PIN_UART_CTS] = &internal_bt_uart_pins[WICED_BT_PIN_UART_CTS],
	[WICED_BT_PIN_UART_RTS] = &internal_bt_uart_pins[WICED_BT_PIN_UART_RTS],
};

/* Bluetooth UART peripheral and runtime driver. Used by libraries/bluetooth/internal/bus/UART/bt_bus.c */
static const platform_uart_t internal_bt_uart_peripheral =
{
	.port               = USART2,
	.tx_pin             = &internal_bt_uart_pins[WICED_BT_PIN_UART_TX ],
	.rx_pin             = &internal_bt_uart_pins[WICED_BT_PIN_UART_RX ],
	.cts_pin            = &internal_bt_uart_pins[WICED_BT_PIN_UART_CTS],
	.rts_pin            = &internal_bt_uart_pins[WICED_BT_PIN_UART_RTS],
	.tx_dma_config =
	{
		.controller     = DMA1,
		.stream         = DMA1_Stream6,
		.channel        = DMA_Channel_4,
		.irq_vector     = DMA1_Stream6_IRQn,
		.complete_flags = DMA_HISR_TCIF6,
		.error_flags    = ( DMA_HISR_TEIF6 | DMA_HISR_FEIF6 ),
	},
	.rx_dma_config =
	{
		.controller     = DMA1,
		.stream         = DMA1_Stream5,
		.channel        = DMA_Channel_4,
		.irq_vector     = DMA1_Stream5_IRQn,
		.complete_flags = DMA_HISR_TCIF5,
		.error_flags    = ( DMA_HISR_TEIF5 | DMA_HISR_FEIF5 | DMA_HISR_DMEIF5 ),
	},
};
static platform_uart_driver_t internal_bt_uart_driver;
const platform_uart_t*        wiced_bt_uart_peripheral = &internal_bt_uart_peripheral;
platform_uart_driver_t*       wiced_bt_uart_driver     = &internal_bt_uart_driver;

/* Bluetooth UART configuration. Used by libraries/bluetooth/internal/bus/UART/bt_bus.c */
const platform_uart_config_t wiced_bt_uart_config =
{
	.baud_rate    = 115200,
	.data_width   = DATA_WIDTH_8BIT,
	.parity       = NO_PARITY,
	.stop_bits    = STOP_BITS_1,
	.flow_control = FLOW_CONTROL_DISABLED,
};

/*BT chip specific configuration information*/
const platform_bluetooth_config_t wiced_bt_config =
{
	
	.patchram_download_mode      = PATCHRAM_DOWNLOAD_MODE_MINIDRV_CMD,
	.patchram_download_baud_rate = 115200,
	.featured_baud_rate          = 115200
};

gpio_button_t platform_gpio_buttons[] =
{
	[PLATFORM_BUTTON_1] =
	{
		.polarity   = WICED_ACTIVE_LOW,
		.gpio       = GPIO_BUTTON_USB_DETECT,
		.trigger    = IRQ_TRIGGER_BOTH_EDGES,
	},
};

const wiced_gpio_t platform_gpio_leds[PLATFORM_LED_COUNT] =
{
     [WICED_LED_INDEX_1] = USBCG_LED_POWER,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void platform_init_peripheral_irq_priorities( void )
{
	/* Interrupt priority setup. Called by WICED/platform/MCU/STM32F4xx/platform_init.c */
	NVIC_SetPriority( RTC_WKUP_IRQn    ,  1 ); /* RTC Wake-up event   */
	NVIC_SetPriority( SDIO_IRQn        ,  2 ); /* WLAN SDIO           */
	NVIC_SetPriority( DMA2_Stream3_IRQn,  3 ); /* WLAN SDIO DMA       */
	NVIC_SetPriority( SPI1_IRQn        ,  6 ); /* SPI FLASH           */
	NVIC_SetPriority( DMA2_Stream0_IRQn,  6 ); /* SPI FLASH RX DMA    */
	NVIC_SetPriority( DMA2_Stream5_IRQn,  6 ); /* SPI FLASH TX DMA    */
	NVIC_SetPriority( USART1_IRQn      ,  7 ); /* WICED_UART_1        */
	NVIC_SetPriority( USART2_IRQn      ,  7 ); /* BT UART             */
	NVIC_SetPriority( USART6_IRQn      ,  7 ); /* WICED_UART_2        */
	NVIC_SetPriority( DMA2_Stream7_IRQn,  8 ); /* WICED_UART_1 TX DMA */
	NVIC_SetPriority( DMA2_Stream2_IRQn,  8 ); /* WICED_UART_1 RX DMA */
	NVIC_SetPriority( DMA1_Stream6_IRQn,  8 ); /* BT UART TX DMA      */
	NVIC_SetPriority( DMA1_Stream5_IRQn,  8 ); /* BT UART RX DMA      */
	NVIC_SetPriority( DMA2_Stream6_IRQn,  8 ); /* WICED_UART_2 TX DMA */
	NVIC_SetPriority( DMA2_Stream1_IRQn,  8 ); /* WICED_UART_2 RX DMA */
	NVIC_SetPriority( EXTI0_IRQn       , 14 ); /* GPIO                */
	NVIC_SetPriority( EXTI1_IRQn       , 14 ); /* GPIO                */
	NVIC_SetPriority( EXTI2_IRQn       , 14 ); /* GPIO                */
	NVIC_SetPriority( EXTI3_IRQn       , 14 ); /* GPIO                */
	NVIC_SetPriority( EXTI4_IRQn       , 14 ); /* GPIO                */
	NVIC_SetPriority( EXTI9_5_IRQn     , 14 ); /* GPIO                */
	NVIC_SetPriority( EXTI15_10_IRQn   , 14 ); /* GPIO                */
}

/* LEDs on this platform are active LOW */
platform_result_t platform_led_set_state(int led_index, int off_on )
{
    if ((led_index >= 0) && (led_index < PLATFORM_LED_COUNT))
    {
        switch (off_on)
        {
            case WICED_LED_OFF:
                platform_gpio_output_high( &platform_gpio_pins[platform_gpio_leds[led_index]] );
                break;
            case WICED_LED_ON:
                platform_gpio_output_low( &platform_gpio_pins[platform_gpio_leds[led_index]] );
                break;
        }
        return PLATFORM_SUCCESS;
    }
    return PLATFORM_BADARG;
}

void platform_led_init( void )
{
	platform_gpio_init(&platform_gpio_pins[LED_RED_PWM], OUTPUT_PUSH_PULL);
	platform_gpio_init(&platform_gpio_pins[LED_GREEN_PWM], OUTPUT_PUSH_PULL);

	platform_gpio_output_high(&platform_gpio_pins[LED_RED_PWM]);
	platform_gpio_output_high(&platform_gpio_pins[LED_GREEN_PWM]);
}

void platform_init_external_devices( void )
{
	platform_led_init();

    /* adc */
    platform_gpio_init(&platform_gpio_pins[WICED_GPIO_2], INPUT_HIGH_IMPEDANCE);
    platform_gpio_init(&platform_gpio_pins[WICED_GPIO_3], INPUT_HIGH_IMPEDANCE);

    /* gpi, button */
    platform_gpio_init(&platform_gpio_pins[GPIO_BUTTON_USB_DETECT], INPUT_PULL_UP);

    /* gpo */
    platform_gpio_init(&platform_gpio_pins[GPO_LOAD_TEST], OUTPUT_PUSH_PULL);
    platform_gpio_init(&platform_gpio_pins[GPO_CHARGE_CONTROL], OUTPUT_PUSH_PULL);
    platform_gpio_init(&platform_gpio_pins[GPO_ENABLE_FAST_CHARGE], OUTPUT_PUSH_PULL);
    
    platform_gpio_output_low(&platform_gpio_pins[GPO_LOAD_TEST]);
    platform_gpio_output_high(&platform_gpio_pins[GPO_ENABLE_FAST_CHARGE]);
    platform_gpio_output_high(&platform_gpio_pins[GPO_CHARGE_CONTROL]);

#ifndef WICED_DISABLE_STDIO
	/* Initialise UART standard I/O */
	platform_stdio_init( &platform_uart_drivers[STDIO_UART], &platform_uart_peripherals[STDIO_UART], &stdio_config );
#endif
}

/******************************************************
 *           Interrupt Handler Definitions
 ******************************************************/

WWD_RTOS_DEFINE_ISR( usart1_irq )
{
	platform_uart_irq( &platform_uart_drivers[WICED_UART_1] );
}

WWD_RTOS_DEFINE_ISR( usart2_irq )
{
	platform_uart_irq( wiced_bt_uart_driver );
}

WWD_RTOS_DEFINE_ISR( usart1_tx_dma_irq )
{
	platform_uart_tx_dma_irq( &platform_uart_drivers[WICED_UART_1] );
}

WWD_RTOS_DEFINE_ISR( usart2_tx_dma_irq )
{
	platform_uart_tx_dma_irq( wiced_bt_uart_driver );
}

WWD_RTOS_DEFINE_ISR( usart1_rx_dma_irq )
{
	platform_uart_rx_dma_irq( &platform_uart_drivers[WICED_UART_1] );
}

WWD_RTOS_DEFINE_ISR( usart2_rx_dma_irq )
{
	platform_uart_rx_dma_irq( wiced_bt_uart_driver );
}

/******************************************************
 *            Interrupt Handlers Mapping
 ******************************************************/

/* These DMA assignments can be found STM32F2xx datasheet DMA section */
WWD_RTOS_MAP_ISR( usart1_irq        , USART1_irq       )
WWD_RTOS_MAP_ISR( usart2_irq        , USART2_irq       )
WWD_RTOS_MAP_ISR( usart1_tx_dma_irq , DMA2_Stream7_irq )
WWD_RTOS_MAP_ISR( usart2_tx_dma_irq , DMA1_Stream6_irq )
WWD_RTOS_MAP_ISR( usart1_rx_dma_irq , DMA2_Stream2_irq )
WWD_RTOS_MAP_ISR( usart2_rx_dma_irq , DMA1_Stream5_irq )
