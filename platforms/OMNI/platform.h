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
 * Defines peripherals available for use on Avnet AWS combo board
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
/*
BCM94343W_AVN platform pin definitions
*/

/* Pin Definitions
 *
Platform pin definitions ...
+--------------------------------------------------------------------------------------------------------+
| Enum ID       |Pin |   Pin Name on    |    Module     | STM32| Peripheral  |    Board     | Peripheral  |
|               | #  |      Module      |  GPIO Alias   | Port | Available   |  Connection  |   Alias     |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_1  | 23 |                  | WICED_GPIO_1  | A  0 |             |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_2  | 24 | P1_V_ADC         | WICED_GPIO_2  | A  1 | ADC1_IN1    |              | ADC         |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_3  | 25 | P2_V_ADC         | WICED_GPIO_3  | A  2 | ADC1_IN2    |              | ADC         |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_4  | 26 |                  | WICED_GPIO_4  | A  3 |             |              |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_5  | 29 | SPI1_NSS         | WICED_GPIO_5  | A  4 | SPI1_NSS    | SFLASH       | SPI         |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_6  | 30 | SPI1_SCK         | WICED_GPIO_6  | A  5 | SPI1_SCK    | SFLASH       | SPI         |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_7  | 31 | SPI1_MISO        | WICED_GPIO_7  | A  6 | SPI1_MISO   | SFLASH       | SPI         |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_8  | 32 | SPI1_MOSI        | WICED_GPIO_8  | A  7 | SPI1_MOSI   | SFLASH       | SPI         |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_9  | 67 | MCO_CLK          | WICED_GPIO_9  | A  8 | MCO_1       | WLAN         |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_10 | 68 | UART1_TXD        | WICED_GPIO_10 | A  9 | USART1_TX   | UART DEBUG   |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_11 | 69 | UART1_RXD        | WICED_GPIO_11 | A 10 | USART1_RX   | UART DEBUG   |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_12 | 70 | UART6_TXD        | WICED_GPIO_12 | A 11 | USART6_TX   | SENSOR       |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_13 | 71 | UART6_RXD        | WICED_GPIO_13 | A 12 | USART6_RX   | SENSOR       |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_14 | 72 | JTAG_SWDIO       | WICED_GPIO_14 | A 13 | JTMS-SWDIO  | JTAG         |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_15 | 76 | JTAG_SWCLK       | WICED_GPIO_15 | A 14 | JTCK-SWCLK  | JTAG         |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_16 | 77 | JTAG_TDI         | WICED_GPIO_16 | A 15 | JTDI        | JTAG         |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_17 | 35 | LED_SENSOR_G     | WICED_GPIO_17 | B  0 | GPIO        | LED          | PWM         |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_18 | 36 | LED_SENSOR_Y     | WICED_GPIO_18 | B  1 | GPIO        | LED          | PWM         |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_19 | 37 | BOOT1            | WICED_GPIO_19 | B  2 | BOOT1       | BOOT1(GND)   |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_20 | 89 | JTAG_TD0         | WICED_GPIO_20 | B  3 | JTDO-SWO    | JTAG         |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_21 | 90 |                  | WICED_GPIO_21 | B  4 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_22 | 91 | WL_REG_ON        | WICED_GPIO_22 | B  5 | GPIO        | WLAN         |             |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_23 | 92 | I2C1_SCL         | WICED_GPIO_23 | B  6 | I2C1_SCL    | SENSOR       | I2C         |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_24 | 93 | I2C1_SDA         | WICED_GPIO_24 | B  7 | I2C1_SDA    | SENSOR       | I2C         |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_25 | 95 | LED_BAT_Y        | WICED_GPIO_25 | B  8 | GPIO        | LED          | PWM         |
|---------------+----+------------------+---------------+------+-------------+--------------+-------------|
| WICED_GPIO_26 | 96 |                  | WICED_GPIO_26 | B  9 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_27 | 47 |                  | WICED_GPIO_27 | B 10 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_28 | NA |                  | WICED_GPIO_28 | B 11 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_29 | 51 |                  | WICED_GPIO_29 | B 12 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_30 | 52 |                  | WICED_GPIO_30 | B 13 |             |              |             | 
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_31 | 53 |                  | WICED_GPIO_31 | B 14 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_32 | 54 |                  | WICED_GPIO_32 | B 15 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_33 | 15 |                  | WICED_GPIO_33 | C  0 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_34 | 16 |                  | WICED_GPIO_34 | C  1 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_35 | 17 |                  | WICED_GPIO_35 | C  2 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_36 | 18 | BAT_GAUGE        | WICED_GPIO_36 | C  3 | ADC1_IN13   | BAT          | ADC         |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_37 | 33 | BT_REG_ON        | WICED_GPIO_37 | C  4 | GPIO        | BT           |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_38 | 34 | BT_HOST_WAKE     | WICED_GPIO_38 | C  5 | GPIO        | BT           |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_39 | 63 | WL_HOST_WAKE     | WICED_GPIO_39 | C  6 | GPIO        | WLAN         |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_40 | 64 | BT_DEV_WAKE      | WICED_GPIO_40 | C  7 | GPIO        | BT           |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_41 | 65 | SDIO_D0          | WICED_GPIO_41 | C  8 | SDIO_D0     | WLAN         | SDIO        |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_42 | 66 | SDIO_D1          | WICED_GPIO_42 | C  9 | SDIO_D1     | WLAN         | SDIO        |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_43 | 78 | SDIO_D2          | WICED_GPIO_43 | C 10 | SDIO_D2     | WLAN         | SDIO        |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_44 | 79 | SDIO_D3          | WICED_GPIO_44 | C 11 | SDIO_D3     | WLAN         | SDIO        |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_45 | 80 | SDIO_CLK         | WICED_GPIO_45 | C 12 | SDIO_CK     | WLAN         | SDIO        |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_46 |  7 |                  | WICED_GPIO_46 | C 13 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_47 |  8 | PC14-OSC32_IN    | WICED_GPIO_47 | C 14 | OSC32_IN    | OSC          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_48 |  9 | PC15-OSC32_OUT   | WICED_GPIO_48 | C 15 | OSC32_OUT   | OSC          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_49 | 81 |                  | WICED_GPIO_49 | D  0 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_50 | 82 | AC_OK            | WICED_GPIO_50 | D  1 | GPIO        | AC           | GPIO        |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_51 | 83 | SDIO_CMD         | WICED_GPIO_51 | D  2 | SDIO_CMD    | WLAN         | SDIO        |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_52 | 84 | BT_CTS           | WICED_GPIO_52 | D  3 | USART2_CTS  | BT           |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_53 | 85 | BT_RTS           | WICED_GPIO_53 | D  4 | USART2_RTS  | BT           |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_54 | 86 | BT_TXD           | WICED_GPIO_54 | D  5 | USART2_TX   | BT           |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_55 | 87 | BT_RXD           | WICED_GPIO_55 | D  6 | USART2_RX   | BT           |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_56 | 88 |                  | WICED_GPIO_56 | D  7 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_57 | 55 |                  | WICED_GPIO_57 | D  8 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_58 | 56 |                  | WICED_GPIO_58 | D  9 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_59 | 57 |                  | WICED_GPIO_59 | D 10 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_60 | 58 |                  | WICED_GPIO_60 | D 11 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_61 | 59 | USB1_DET         | WICED_GPIO_61 | D 12 | GPIO        | KEY          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_62 | 60 | USB2_DET         | WICED_GPIO_62 | D 13 | GPIO        | KEY          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_63 | 61 | USB3_DET         | WICED_GPIO_63 | D 14 | GPIO        | KEY          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_64 | 62 | USB4_DET         | WICED_GPIO_64 | D 15 | GPIO        | KEY          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_65 | 97 |                  | WICED_GPIO_65 | E  0 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_66 | 98 | CHG_ON           | WICED_GPIO_66 | E  1 | GPIO        | PMIC         |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_67 |  1 | LED_CPU_G        | WICED_GPIO_67 | E  2 | GPIO        | LED          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_68 |  2 | LED_CPU_Y        | WICED_GPIO_68 | E  3 | GPIO        | LED          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_69 |  3 | LED_WIFI_G       | WICED_GPIO_69 | E  4 | GPIO        | LED          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_70 |  4 | LED_WIFI_Y       | WICED_GPIO_70 | E  5 | GPIO        | LED          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_71 |  5 | LED_FLT_R        | WICED_GPIO_71 | E  6 | GPIO        | LED          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_72 | 38 | P1_PWR_ON        | WICED_GPIO_72 | E  7 | GPIO        | KEY          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_73 | 39 | P2_PWR_ON        | WICED_GPIO_73 | E  8 | GPIO        | KEY          |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_74 | 40 |                  | WICED_GPIO_74 | E  9 |             |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_75 | 41 | SEL_0            | WICED_GPIO_75 | E 10 | GPIO        | UART6        |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_76 | 42 | CHG_OK           | WICED_GPIO_76 | E 11 | GPIO        | Charge       |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_77 | 43 | USB1_BIT_ON      | WICED_GPIO_77 | E 12 | GPIO        |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_78 | 44 | USB2_BIT_ON      | WICED_GPIO_78 | E 13 | GPIO        |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_79 | 45 | USB3_BIT_ON      | WICED_GPIO_79 | E 14 | GPIO        |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
| WICED_GPIO_80 | 46 | USB4_BIT_ON      | WICED_GPIO_80 | E 15 | GPIO        |              |             |
+---------------+----+------------------+------+---------------+-------------+--------------+-------------+
*/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_GPIO_1,
    WICED_GPIO_2,
    WICED_GPIO_3,
    WICED_GPIO_4,
    WICED_GPIO_5,
    WICED_GPIO_6,
    WICED_GPIO_7,
    WICED_GPIO_8,
    WICED_GPIO_9,
    WICED_GPIO_10,
    WICED_GPIO_11,
    WICED_GPIO_12,
    WICED_GPIO_13,
    WICED_GPIO_14,
    WICED_GPIO_15,
    WICED_GPIO_16,
    WICED_GPIO_17,
    WICED_GPIO_18,
    WICED_GPIO_19,
    WICED_GPIO_20,
    WICED_GPIO_21,
    WICED_GPIO_22,
    WICED_GPIO_23,
    WICED_GPIO_24,
    WICED_GPIO_25,
    WICED_GPIO_26,
    WICED_GPIO_27,
    WICED_GPIO_28,
    WICED_GPIO_29,
    WICED_GPIO_30,
    WICED_GPIO_31,
    WICED_GPIO_32,
    WICED_GPIO_33,
    WICED_GPIO_34,
    WICED_GPIO_35,
    WICED_GPIO_36,
    WICED_GPIO_37,
    WICED_GPIO_38,
    WICED_GPIO_39,
    WICED_GPIO_40,
    WICED_GPIO_41,
    WICED_GPIO_42,
    WICED_GPIO_43,
    WICED_GPIO_44,
    WICED_GPIO_45,
    WICED_GPIO_46,
    WICED_GPIO_47,
    WICED_GPIO_48,
    WICED_GPIO_49,
    WICED_GPIO_50,
    WICED_GPIO_51,
    WICED_GPIO_52,
    WICED_GPIO_53,
    WICED_GPIO_54,
    WICED_GPIO_55,
    WICED_GPIO_56,
    WICED_GPIO_57,
    WICED_GPIO_58,
    WICED_GPIO_59,
    WICED_GPIO_60,
    WICED_GPIO_61,
    WICED_GPIO_62,
    WICED_GPIO_63,
    WICED_GPIO_64,
    WICED_GPIO_65,
    WICED_GPIO_66,
    WICED_GPIO_67,
    WICED_GPIO_68,
    WICED_GPIO_69,
    WICED_GPIO_70,
    WICED_GPIO_71,
    WICED_GPIO_72,
    WICED_GPIO_73,
    WICED_GPIO_74,
    WICED_GPIO_75,
    WICED_GPIO_76,
    WICED_GPIO_77,
    WICED_GPIO_78,
    WICED_GPIO_79,
    WICED_GPIO_80,
    WICED_GPIO_MAX, /* Denotes the total number of GPIO port aliases. Not a valid GPIO alias */
    WICED_GPIO_32BIT = 0x7FFFFFFF,
} wiced_gpio_t;

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
    WICED_PWM_MAX, /* Denotes the total number of PWM port aliases. Not a valid PWM alias */
    WICED_PWM_32BIT = 0x7FFFFFFF,
} wiced_pwm_t;

typedef enum
{
    WICED_ADC_1,		/* p1_v */
    WICED_ADC_2,		/* p2_v */
    WICED_ADC_3,		/* bat */
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

/* Logical Button-ids which map to phyiscal buttons on the board */
typedef enum
{
    PLATFORM_BUTTON_DUMMY,
    PLATFORM_BUTTON_MAX, /* Denotes the total number of Buttons on the board. Not a valid Button Alias */
} platform_button_t;

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_PLATFORM_BUTTON_COUNT  ( 0 )

/* UART port used for standard I/O */
#define STDIO_UART ( WICED_UART_1 )
#define SENSOR_UART ( WICED_UART_2 )
#define SENSOR_UART_SEL	(WICED_GPIO_75)

/* SPI flash is present */
#define WICED_PLATFORM_INCLUDES_SPI_FLASH
#define WICED_SPI_FLASH_CS  ( WICED_GPIO_5 )

/* ADC alternative name */
#define WICED_ADC_P1_V		WICED_ADC_1
#define WICED_ADC_P2_V		WICED_ADC_2
#define WICED_ADC_BAT		WICED_ADC_3

/* GPIO OUT */
#define GPO_CHG_ON		WICED_GPIO_66
#define GPO_USB1_BIT_ON		WICED_GPIO_77
#define GPO_USB2_BIT_ON		WICED_GPIO_78
#define GPO_USB3_BIT_ON		WICED_GPIO_79
#define GPO_USB4_BIT_ON		WICED_GPIO_80
#define GPO_P1_POWER_ON		WICED_GPIO_72
#define GPO_P2_POWER_ON		WICED_GPIO_73

/* GPIO IN */
#define GPI_AC_OK		WICED_GPIO_50
#define GPI_CHARGE_OK		WICED_GPIO_76
#define GPI_USB1_DET		WICED_GPIO_61
#define GPI_USB2_DET		WICED_GPIO_62
#define GPI_USB3_DET		WICED_GPIO_63
#define GPI_USB4_DET		WICED_GPIO_64

/* Components connected to external I/Os */
#define PLATFORM_LED_COUNT      ( 8 )

#define WICED_LED_SENSOR_G	WICED_GPIO_17
#define WICED_LED_SENSOR_Y	WICED_GPIO_18
#define WICED_LED_BAT_R		WICED_GPIO_25
#define WICED_LED_CPU_G		WICED_GPIO_67
#define WICED_LED_CPU_Y		WICED_GPIO_68
#define WICED_LED_WIFI_G	WICED_GPIO_69
#define WICED_LED_WIFI_Y	WICED_GPIO_70
#define WICED_LED_FAULT_R	WICED_GPIO_71

/* /\* Bootloader OTA and OTA2 factory reset during settings *\/ */
/* #ifdef PLATFORM_FACTORY_RESET_TIMEOUT */
/* #undef PLATFORM_FACTORY_RESET_TIMEOUT */
/* #endif */
/* #define PLATFORM_FACTORY_RESET_BUTTON_INDEX     ( PLATFORM_BUTTON_5 ) */
/* #define PLATFORM_FACTORY_RESET_TIMEOUT          ( 8000 ) */

/* /\* Generic button checking defines *\/ */
/* #define PLATFORM_BUTTON_PRESS_CHECK_PERIOD      ( 100 ) */
/* #define PLATFORM_BUTTON_PRESSED_STATE           (   0 ) */

/* #define PLATFORM_GREEN_LED_INDEX                ( WICED_LED_INDEX_2 ) */
/* #define PLATFORM_RED_LED_INDEX                  ( WICED_LED_INDEX_1 ) */

#ifdef __cplusplus
} /*extern "C" */
#endif
