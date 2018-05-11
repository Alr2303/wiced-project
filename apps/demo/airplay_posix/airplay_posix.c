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
 * The network to be used can be changed by the #define WICED_NETWORK_INTERFACE in wifi_config_dct.h
 * In the case of using AP or STA mode, change the AP_SSID and AP_PASSPHRASE accordingly.
 *
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <net/if.h>
#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <nuttx/arch.h>
#include "arch/chip/wifi.h"

#include "netlib.h"
#include "plat_helper.h"
#include "airplay_posix_button_monitor.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
#if 0
typedef struct
{
    airplay_dct_t *dct_app;
    platform_dct_wifi_config_t* dct_wifi;
    pthread_t mdnsd_pid;
    pthread_t mdnsd_pid;
} airplay_posix_context_t;
#endif

 /****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
extern int mdns_responder(int argc, char **argv);
extern int AirPlayMain( int argc, char **argv);
extern int apple_wac_configure(void);

/****************************************************************************
 * Name: airplayposix_main
 ****************************************************************************/

int airplayposix_main(int argc, char *argv[])
{
  int exitval = 0;

  airplay_button_manager_init();

  printf("Starting WAC ...\n");
  apple_wac_configure();
  usleep(500000);

  printf("Starting mDNSResponder ...\n");
  plat_helper_mdnsd_start();
  usleep(500000);

  printf("Starting AirPlay...\n");
  plat_helper_airplayd_start();

  /* cosole ??? */

  for (;;)
  {
    usleep(1000 * 1000);
  }

  return exitval;
}
