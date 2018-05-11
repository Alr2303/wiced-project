/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Defines peripherals available for use on BCM943362WCD4 board
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                   Enumerations
 ******************************************************/
/*
EMW3165 on EMB-3165-A platform pin definitions ...
+-------------------------------------------------------------------------+
| Enum ID       |Pin | STM32| Peripheral  |    Board     |   Peripheral   |
|               | #  | Port | Available   |  Connection  |     Alias      |
|---------------+----+------+-------------+--------------+----------------|
|               | 1  | NC   |             |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_2   | 2  | B  2 |   GPIO      |              |                |
|---------------+----+------+-------------+--------------+----------------|
|               | 3  |  NC  |             |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_4   | 4  | A  7 | TIM1_CH1N   |              |                |
|               |    |      | TIM3_CH2    |              |                |
|               |    |      | SPI1_MOSI   |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_5   | 5  | A  15| JTDI        |              |                |
|               |    |      | TIM2_CH1    |              |                |
|               |    |      | TIM2_ETR    |              |                |
|               |    |      | SPI1_NSS    |              |                |
|               |    |      | SPI3_NSS    |              |                |
|               |    |      | USART1_TX   |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_6   | 6  | B  3 | TIM2_CH2    |              |                |
|               |    |      | GPIO        |              |                |
|               |    |      | SPI1_SCK    |              |                |
|               |    |      | USART1_RX   |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_7   | 7  | B  4 | JTRST       |              |                |
|               |    |      | GPIO        |              |                |
|               |    |      | SDIO_D0     |              |                |
|               |    |      | TIM3_CH1    |              |                |
|               |    |      | SPI1_MISO   |              |                |
|               |    |      | SPI3_MISO   |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_8   | 8  | A  2 | TIM2_CH3    |              | MICO_UART_1_TX |
|               |    |      | TIM5_CH3    |              |                |
|               |    |      | TIM9_CH1    |              |                |
|               |    |      | USART2_TX   |              |                |
|               |    |      | GPIO        |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_9   | 9  | A  1 | TIM2_CH2    |EasyLink_BUTTON|               |
|               |    |      | TIM5_CH2    |              |                |
|               |    |      | USART2_RTS  |              |                |
|               |    |      | GPIO        |              |                |
|---------------+----+------+-------------+--------------+----------------|
|               | 10 | VBAT |             |
|---------------+----+------+-------------+--------------+----------------|
|               | 11 | NC   |             |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_12  | 12 | A  3 | TIM2_CH4    |              | MICO_UART_1_RX |
|               |    |      | TIM5_CH4    |              |                |
|               |    |      | TIM9_CH2    |              |                |
|               |    |      | USART2_RX   |              |                |
|               |    |      | GPIO        |              |                |
|---------------+----+------+-------------+--------------+----------------|
|               | 13 | NRST |             |              |  MICRO_RST_N   |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_14  | 14 | A 0  | WAKE_UP     |              |                |
|---------------+----+------+-------------+--------------+----------------|
|               | 15 | NC   |             |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_16  | 16 | C 13 |     -       |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_17  | 17 | B 10 |  TIM2_CH3   | MICO_SYS_LED |                |
|               |    |      |  I2C2_SCL   |              |                |
|               |    |      |  GPIO       |              |                |
|---------------+----+------+-------------+--------------+----------------|
| MICO_GPIO_18  | 18 | B  9 | TIM4_CH4    |              |                |
|               |    |      | TIM11_CH1   |              |                |
|               |    |      | I2C1_SDA    |              |                |
|               |    |      | GPIO        |              |                |
+---------------+----+--------------------+--------------+----------------+
| MICO_GPIO_19  | 19 | B 12 | GPIO        |              |                |
+---------------+----+--------------------+--------------+----------------+
|               | 20 | GND  |             |              |                |
+---------------+----+--------------------+--------------+----------------+
|               | 21 | GND  |             |              |                |
+---------------+----+--------------------+--------------+----------------+
| MICO_GPIO_22  | 22 | B  3 |             |              |                |
+---------------+----+--------------------+--------------+----------------+
| MICO_GPIO_23  | 23 | NC   |             |              |                |
+---------------+----+--------------------+--------------+----------------+
| MICO_GPIO_24  | 24 | B  4 |             |              |                |
+---------------+----+--------------------+--------------+----------------+
| MICO_GPIO_25  | 25 | A 14 | JTCK-SWCLK  |  SWCLK       |                |
|               |    |      |  GPIO       |              |                |  
+---------------+----+--------------------+--------------+----------------+
|MICO_GPIO_26   | 26 | A 13 | JTMS-SWDIO  |  SWDIO       |                |
|               |    |      |  GPIO       |              |                |    
+---------------+----+--------------------+--------------+----------------+
|MICO_GPIO_27   | 27 | A 12 | TIM1_ETR    |              | USART1_RTS     |
|               |    |      | USART1_RTS  |              |                |    
|               |    |      | USART6_RX   |              |                |    
|               |    |      | USB_FS_DP   |              |                |    
|               |    |      | GPIO        |              |                |    
+---------------+----+--------------------+--------------+----------------+
|               | 28 | NC   |             |              |                |
+---------------+----+--------------------+--------------+----------------+
| MICO_GPIO_29  | 29 | A 10 | GPIO        |              |                |
|               |    |      | TIM1_CH3    |              |                |
|               |    |      | USART1_RX   |              |                |
|               |    |      | USB_FS_ID   |              |                |
+---------------+----+--------------------+--------------+----------------+
| MICO_GPIO_30  | 30 | B  6 | GPIO        |              |                |
|               |    |      | TIM4_CH1    |              |                |
|               |    |      | USART1_TX   |              |                |
|               |    |      | I2C1_SCL    |              |                |
+---------------+----+--------------------+--------------+----------------+
| MICO_SYS_LED  | 31 | B  8 | GPIO        |              |                |
|               |    |      | TIM4_CH3    |              |                |
|               |    |      | TIM10_CH1   |              |                |  
+---------------+----+--------------------+--------------+----------------+
|               | 32 |  NC  |             |              |                | 
+---------------+----+--------------------+--------------+----------------+  
| MICO_GPIO_33  | 33 | B 13 | TIM1_CH1N   |              |                |  
|               |    |      | SPI2_SCK    |              |                |  
|               |    |      | GPIO        |              |                | 
+---------------+----+--------------------+--------------+----------------+  
| MICO_GPIO_34  | 34 | A  5 | TIM2_CH1    |              |                |  
|               |    |      | GPIO        |              |                | 
+---------------+----+--------------------+--------------+----------------+  
| MICO_GPIO_35  | 35 | A  11| TIM1_CH4    |              |  USART1_CTS    |  
|               |    |      | SPI4_MISO   |              |                |  
|               |    |      | USART1_CTS  |              |                |  
|               |    |      | USART6_TX   |              |                |  
|               |    |      | USB_FS_DM   |              |                |  
|               |    |      | GPIO        |              |                |
+---------------+----+--------------------+--------------+----------------+  
| MICO_GPIO_36  | 36 | B  1 | TIM1_CH3N   | BOOT_SEL     |                |  
|               |    |      | TIM3_CH4    |              |                |  
|               |    |      | GPIO        |              |                | 
+---------------+----+--------------------+--------------+----------------+  
| MICO_GPIO_37  | 37 | B  0 | TIM1_CH2N   | MFG_SEL      |                |  
|               |    |      | TIM3_CH3    |              |                | 
|               |    |      | GPIO        |              |                | 
+---------------+----+--------------------+--------------+----------------+  
| MICO_GPIO_38  | 38 | A  4 | USART2_CK   | MICO_RF_LED |                | 
|               |    |      | GPIO        |              |                | 
+---------------+----+--------------------+--------------+----------------+  
|               | 39 | VDD  |             |              |                | 
+---------------+----+--------------------+--------------+----------------+  
|               | 40 | VDD  |             |              |                |
+---------------+----+--------------------+--------------+----------------+  
|               | 41 | ANT  |             |              |                |
+---------------+----+--------------------+--------------+----------------+  
Notes
1. These mappings are defined in <MICO-SDK>/Platform/BCM943362WCD4/platform.c
2. STM32F2xx Datasheet  -> http://www.st.com/web/en/resource/technical/document/datasheet/CD00237391.pdf
3. STM32F2xx Ref Manual -> http://www.st.com/web/en/resource/technical/document/reference_manual/CD00225773.pdf
*/

typedef enum
{
    WICED_GPIO_2,		/* PB2_PWR_LED */
    WICED_GPIO_4,		/* FLASH_SPI_MOSI */
    WICED_GPIO_5,		/* FLASH_SPI_CS */
    WICED_GPIO_6,		/* FLASH_SPI_CLK */
    WICED_GPIO_7,		/* FLASH_SPI_MISO */
    WICED_GPIO_8,		/* PA2_AC_SENSE (/ACFAULT of 78M6610) */
    WICED_GPIO_9,		/* PA1_VOC_SENSE (ADC1_1) */
    WICED_GPIO_12,		/* PA3_PIR_SENSE (I) */
    WICED_GPIO_14,		/* PA0_PWR_KEY (I) */
    WICED_GPIO_16,		/* PC13_BT_CONNECT (I) */
    WICED_GPIO_17,		/* PB10_TOUCH_IN (I) */
    WICED_GPIO_18,		/* PB9_I2C_SDA (Sensor) */
    WICED_GPIO_19,		/* BT_nRST (O) */
    /* WICED_GPIO_22,   */
    /* WICED_GPIO_23,   */
    /* WICED_GPIO_24,   */
    /* WICED_GPIO_25,	*/	/* WIFI_SWCLK */
    /* WICED_GPIO_26,   */	/* WIFI_SWDIO */
    WICED_GPIO_27,  		/* PA12_UART6_RXD */
    WICED_GPIO_29,		/* PA10_UART1_RXD */
    WICED_GPIO_30,		/* PB6_UART1_TXD */
    WICED_GPIO_31,		/* PB8_I2C_SCL (Sensor) */
    WICED_GPIO_33,		/* PB13_BLED_PWM */
    WICED_GPIO_34,		/* PA5_WLED_PWM */
    WICED_GPIO_35,		/* PA11_UART6_TXD */
    WICED_GPIO_36,		/* PB1_GLED_PWM */
    WICED_GPIO_37,		/* PB0_RLED_PWM */
    WICED_GPIO_38,		/* PA4_AC_ON (O) */
    WICED_GPIO_MAX, /* Denotes the total number of GPIO port aliases. Not a valid GPIO alias */
    WICED_GPIO_32BIT = 0x7FFFFFFF,
} wiced_gpio_t;

/* GPIO Alias  */
#define STDIO_UART1_RX		WICED_GPIO_29
#define STDIO_UART1_TX		WICED_GPIO_30

#define BT_UART6_RX		WICED_GPIO_27
#define BT_UART6_TX		WICED_GPIO_35

#define FLASH_PIN_SPI_CS	WICED_GPIO_5
#define FLASH_PIN_SPI_CLK	WICED_GPIO_6
#define FLASH_PIN_SPI_MOSI	WICED_GPIO_4
#define FLASH_PIN_SPI_MISO	WICED_GPIO_7

#define BT_RESET		WICED_GPIO_19
#define BT_PAIR			WICED_GPIO_16

#define WICED_FACTORY		WICED_GPIO_14
#define WICED_PIR		WICED_GPIO_12
#define WICED_TOUCH_IN		WICED_GPIO_17

#define PWM_BLED		WICED_PWM_1
#define PWM_WLED		WICED_PWM_2 /* reversed */
#define PWM_GLED		WICED_PWM_3
#define PWM_RLED		WICED_PWM_4

#define ADC_VOC			WICED_ADC_1

typedef enum
{
    WICED_SPI_1,
    WICED_SPI_MAX, /* Denotes the total number of SPI port aliases. Not a valid SPI alias */
    WICED_SPI_32BIT = 0x7FFFFFFF,
} wiced_spi_t;

typedef enum
{
    WICED_I2C_1,
    WICED_I2C_MAX,
    WICED_I2C_32BIT = 0x7FFFFFFF,
} wiced_i2c_t;

typedef enum
{
    WICED_I2S_NONE,
    WICED_I2S_MAX, /* Denotes the total number of I2S port aliases.  Not a valid I2S alias */
    WICED_I2S_32BIT = 0x7FFFFFFF
} wiced_i2s_t;

typedef enum
{
    WICED_PWM_1,
    WICED_PWM_2,
    WICED_PWM_3,
    WICED_PWM_4,
    WICED_PWM_MAX, /* Denotes the total number of PWM port aliases. Not a valid PWM alias */
    WICED_PWM_32BIT = 0x7FFFFFFF,
} wiced_pwm_t;

typedef enum
{
    WICED_ADC_1,
    WICED_ADC_MAX, /* Denotes the total number of ADC port aliases. Not a valid ADC alias */
    WICED_ADC_32BIT = 0x7FFFFFFF,
} wiced_adc_t;

typedef enum
{
    WICED_UART_1,
    WICED_UART_2,
    WICED_UART_MAX, /* Denotes the total number of UART port aliases. Not a valid UART alias */
    WICED_UART_32BIT = 0x7FFFFFFF,
} wiced_uart_t;

typedef enum
{
  WICED_SPI_FLASH,
  WICED_INTERNAL_FLASH,
  WICED_FLASH_MAX,
} wiced_flash_t;


/******************************************************
 *                    Constants
 ******************************************************/

/* UART port used for standard I/O */
#define STDIO_UART ( WICED_UART_1 )
#define BT_UART ( WICED_UART_2 )

/* SPI flash is present */
#define WICED_PLATFORM_INCLUDES_SPI_FLASH
#define WICED_SPI_FLASH_CS ( FLASH_PIN_SPI_CS )

#ifdef __cplusplus
} /*extern "C" */
#endif
