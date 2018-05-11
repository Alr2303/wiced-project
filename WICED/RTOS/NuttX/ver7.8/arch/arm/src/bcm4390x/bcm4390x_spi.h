/*
 * $ Copyright Broadcom Corporation $
 */

#ifndef __ARCH_ARM_SRC_BCM4390X_BCM4390X_SPI_H
#define __ARCH_ARM_SRC_BCM4390X_BCM4390X_SPI_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>
#include "wiced_platform.h"

#if defined(CONFIG_BCM4390X_SPI1) || defined(CONFIG_BCM4390X_SPI2)

/************************************************************************************
 * Pre-processor Declarations
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/

/************************************************************************************
 * Name: up_spiinitialize
 *
 * Description:
 *   Initialize the selected SPI port.
 *
 * Input Parameter:
 *   Port number (for hardware that has mutiple SPI interfaces)
 *
 ************************************************************************************/

void bcm4390x_spiinitialize(int port, wiced_spi_device_t **wiced_spi_device);

#endif /* CONFIG_BCM4390X_SPI1 || CONFIG_BCM4390X_SPI2 */
#endif /* ARCH_ARM_SRC_BCM4390X_BCM4390X_SPI_H */
