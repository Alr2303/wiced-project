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
 * Platform pin definitions ...
 * WICED_GPIO_1  ~ WICED_GPIO_16: PA0~15
 * WICED_GPIO_17 ~ WICED_GPIO_32: PB0~15
 * WICED_GPIO_33 ~ WICED_GPIO_48: PC0~15
 * WICED_GPIO_49 ~ WICED_GPIO_64: PD0~15
 * WICED_GPIO_65 ~ WICED_GPIO_80: PE0~15
 */

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
	WICED_GPIO_1,  WICED_GPIO_2,  WICED_GPIO_3,  WICED_GPIO_4,  WICED_GPIO_5,  WICED_GPIO_6,  WICED_GPIO_7,  WICED_GPIO_8,
	WICED_GPIO_9,  WICED_GPIO_10, WICED_GPIO_11, WICED_GPIO_12, WICED_GPIO_13, WICED_GPIO_14, WICED_GPIO_15, WICED_GPIO_16,
	WICED_GPIO_17, WICED_GPIO_18, WICED_GPIO_19, WICED_GPIO_20, WICED_GPIO_21, WICED_GPIO_22, WICED_GPIO_23, WICED_GPIO_24,
	WICED_GPIO_25, WICED_GPIO_26, WICED_GPIO_27, WICED_GPIO_28, WICED_GPIO_29, WICED_GPIO_30, WICED_GPIO_31, WICED_GPIO_32,
	WICED_GPIO_33, WICED_GPIO_34, WICED_GPIO_35, WICED_GPIO_36, WICED_GPIO_37, WICED_GPIO_38, WICED_GPIO_39, WICED_GPIO_40,
	WICED_GPIO_41, WICED_GPIO_42, WICED_GPIO_43, WICED_GPIO_44, WICED_GPIO_45, WICED_GPIO_46, WICED_GPIO_47, WICED_GPIO_48,
	WICED_GPIO_49, WICED_GPIO_50, WICED_GPIO_51, WICED_GPIO_52, WICED_GPIO_53, WICED_GPIO_54, WICED_GPIO_55, WICED_GPIO_56,
	WICED_GPIO_57, WICED_GPIO_58, WICED_GPIO_59, WICED_GPIO_60, WICED_GPIO_61, WICED_GPIO_62, WICED_GPIO_63, WICED_GPIO_64,
	WICED_GPIO_65, WICED_GPIO_66, WICED_GPIO_67, WICED_GPIO_68, WICED_GPIO_69, WICED_GPIO_70, WICED_GPIO_71, WICED_GPIO_72,
	WICED_GPIO_73, WICED_GPIO_74, WICED_GPIO_75, WICED_GPIO_76, WICED_GPIO_77, WICED_GPIO_78, WICED_GPIO_79, WICED_GPIO_80,
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
	WICED_PWM_1,
	WICED_PWM_2,
	WICED_PWM_MAX, /* Denotes the total number of PWM port aliases. Not a valid PWM alias */
	WICED_PWM_32BIT = 0x7FFFFFFF,
} wiced_pwm_t;

typedef enum
{
	WICED_ADC_1,
	WICED_ADC_2,
	WICED_ADC_MAX, /* Denotes the total number of ADC port aliases. Not a valid ADC alias */
    WICED_ADC_32BIT = 0x7FFFFFFF,
} wiced_adc_t;

typedef enum
{
	WICED_UART_1,
	WICED_UART_MAX, /* Denotes the total number of UART port aliases. Not a valid UART alias */
	WICED_UART_32BIT = 0x7FFFFFFF,
} wiced_uart_t;

/* Logical Button-ids which map to phyiscal buttons on the board */
typedef enum
{
	PLATFORM_BUTTON_1,
	PLATFORM_BUTTON_MAX, /* Denotes the total number of Buttons on the board. Not a valid Button Alias */
} platform_button_t;

/******************************************************
 *                    Constants
 ******************************************************/

/* UART port used for standard I/O */
#define STDIO_UART		WICED_UART_1

/* SPI flash is present */
#define WICED_PLATFORM_INCLUDES_SPI_FLASH
#define WICED_SPI_FLASH_CS	WICED_GPIO_5

/* GPIO OUT */
#define GPO_LOAD_TEST		WICED_GPIO_12      
#define GPO_ENABLE_FAST_CHARGE	WICED_GPIO_25
#define GPO_CHARGE_CONTROL	WICED_GPIO_50

/* GPIO IN */
#define WICED_PLATFORM_BUTTON_COUNT  ( 1 )
#define GPIO_BUTTON_USB_DETECT WICED_GPIO_57

/* Components connected to external I/Os */
#define PLATFORM_LED_COUNT      ( 1 )

#define USBCG_LED_POWER WICED_GPIO_12
#define LED_RED_PWM WICED_GPIO_31
#define LED_GREEN_PWM WICED_GPIO_32

#ifdef __cplusplus
} /*extern "C" */
#endif
