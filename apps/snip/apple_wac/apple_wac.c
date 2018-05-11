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
 * Apple Wireless Accessory Configuration (WAC) Application
 *
 * This application snippet demonstrates how to use the Apple WAC protocol. Refer to the following
 * link for details:
 * https://mfi.apple.com/MFiWeb/getFAQ.action
 *
 * Features demonstrated
 *  - Apple WAC protocol
 *
 * To demonstrate the app, work through the following steps.
 *  1. Plug the WICED eval board into your computer
 *  2. Open a terminal application and connect to the WICED eval board
 *  3. Build and download the application (to the WICED board)
 *  4. After the download completes, go to 'Settings' | 'Wi-Fi' | 'Set up a new Device' on your iOS7
 *     device
 *  5. Select 'Apple WAC' and follow the instructions on the device
 *  6. After the WICED device joins the network, it blinks the LEDs alternately
 *
 */
#include "wiced.h"
#include "apple_wac.h"

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

char* protocols[ 1 ] =
{
    [ 0 ] = "com.apple.one",
};

/* info structure for WAC, see apple_wac.h for more details */
apple_wac_info_t apple_wac_info =
{
    .supports_homekit       = WICED_FALSE,
    .supports_airplay       = WICED_FALSE,
    .supports_airprint      = WICED_FALSE,
    .supports_5ghz_networks = WICED_FALSE,
    .has_app                = WICED_TRUE,
    .firmware_revision      = "1.0.0",
    .hardware_revision      = "2.0.0",
    .serial_number          = "001122334455",
    .name                   = "Apple WAC",
    .model                  = "A6969",
    .manufacturer           = "Test",
    .oui                    = (uint8_t *)"\xAA\xBB\xCC",
    .mfi_protocols          = &protocols[0],
    .num_mfi_protocols      = 1,
    .bundle_seed_id         = "ABCA66M49V",
    .mdns_desired_hostname  = "wac-test",
    .mdns_nice_name         = "WAC Test",
    .soft_ap_channel        = 6,
    .device_random_id       = "AA:AA:AA:AA:AA:AA",
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_result_t result;
#if defined(WICED_LED1) || defined(WICED_LED2)
    uint8_t i;
#endif

    wiced_init( );

    result = apple_wac_configure( &apple_wac_info );

    /* Clear once WAC is complete, and no longer used */
    if ( apple_wac_info.out_new_name != NULL )
    {
        free( apple_wac_info.out_new_name );
    }

    /* Clear once WAC is complete, and no longer used */
    if ( apple_wac_info.out_play_password != NULL )
    {
        free( apple_wac_info.out_play_password );
    }

    /* Call into the WAC library to start configuration, blocking until it's completed */
    if (  result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("\n WAC Failed %u\n", (unsigned int)result));
        return;
    }

    WPRINT_APP_INFO(("\n WAC Completed\n"));
#if defined(WICED_LED1) || defined(WICED_LED2)
    /* Flash the LEDs after configuration has completed */
    for ( i = 0; i < 15; i++ )
    {
#if defined(WICED_LED1)
        wiced_gpio_output_high( WICED_LED1 );
#endif
#if defined(WICED_LED2)
        wiced_gpio_output_low( WICED_LED2 );
#endif

        wiced_rtos_delay_milliseconds( 300 );

#if defined(WICED_LED1)
        wiced_gpio_output_low( WICED_LED1 );
#endif
#if defined(WICED_LED2)
        wiced_gpio_output_high( WICED_LED2 );
#endif

        wiced_rtos_delay_milliseconds( 300 );
    }
#endif
}
