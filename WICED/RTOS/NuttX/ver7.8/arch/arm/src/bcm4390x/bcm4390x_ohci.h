/*
 * $ Copyright Broadcom Corporation $
 */

/****************************************************************************
 * arch/arm/src/bcm4390x/bcm4390x_ohci.h
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_BCM4390X_OHCI_H
#define __ARCH_ARM_SRC_BCM4390X_OHCI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/usb/ohci.h>

#include "chip.h"
#include "platform_map.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* The BCM4390X supports 1 root hub ports */

#define BCM4390X_OHCI_NRHPORT        1

/* Register offsets *********************************************************/
/* See nuttx/usb/ohci.h */

/* Register addresses *******************************************************/

#define BCM4390X_OHCI_HCIREV      PLATFORM_OHCI_REGBASE(OHCI_HCIREV_OFFSET)
#define BCM4390X_OHCI_CTRL        PLATFORM_OHCI_REGBASE(OHCI_CTRL_OFFSET)
#define BCM4390X_OHCI_CMDST       PLATFORM_OHCI_REGBASE(OHCI_CMDST_OFFSET)
#define BCM4390X_OHCI_INTST       PLATFORM_OHCI_REGBASE(OHCI_INTST_OFFSET)
#define BCM4390X_OHCI_INTEN       PLATFORM_OHCI_REGBASE(OHCI_INTEN_OFFSET)
#define BCM4390X_OHCI_INTDIS      PLATFORM_OHCI_REGBASE(OHCI_INTDIS_OFFSET)

/* Memory pointers (section 7.2) */

#define BCM4390X_OHCI_HCCA        PLATFORM_OHCI_REGBASE(OHCI_HCCA_OFFSET)
#define BCM4390X_OHCI_PERED       PLATFORM_OHCI_REGBASE(OHCI_PERED_OFFSET)
#define BCM4390X_OHCI_CTRLHEADED  PLATFORM_OHCI_REGBASE(OHCI_CTRLHEADED_OFFSET)
#define BCM4390X_OHCI_CTRLED      PLATFORM_OHCI_REGBASE(OHCI_CTRLED_OFFSET)
#define BCM4390X_OHCI_BULKHEADED  PLATFORM_OHCI_REGBASE(OHCI_BULKHEADED_OFFSET)
#define BCM4390X_OHCI_BULKED      PLATFORM_OHCI_REGBASE(OHCI_BULKED_OFFSET)
#define BCM4390X_OHCI_DONEHEAD    PLATFORM_OHCI_REGBASE(OHCI_DONEHEAD_OFFSET)

/* Frame counters (section 7.3) */

#define BCM4390X_OHCI_FMINT       PLATFORM_OHCI_REGBASE(OHCI_FMINT_OFFSET)
#define BCM4390X_OHCI_FMREM       PLATFORM_OHCI_REGBASE(OHCI_FMREM_OFFSET)
#define BCM4390X_OHCI_FMNO        PLATFORM_OHCI_REGBASE(OHCI_FMNO_OFFSET)
#define BCM4390X_OHCI_PERSTART    PLATFORM_OHCI_REGBASE(OHCI_PERSTART_OFFSET)

/* Root hub ports (section 7.4) */

#define BCM4390X_OHCI_LSTHRES     PLATFORM_OHCI_REGBASE(OHCI_LSTHRES_OFFSET)
#define BCM4390X_OHCI_RHDESCA     PLATFORM_OHCI_REGBASE(OHCI_RHDESCA_OFFSET)
#define BCM4390X_OHCI_RHDESCB     PLATFORM_OHCI_REGBASE(OHCI_RHDESCB_OFFSET)
#define BCM4390X_OHCI_RHSTATUS    PLATFORM_OHCI_REGBASE(OHCI_RHSTATUS_OFFSET)

#define BCM4390X_OHCI_RHPORTST(n) PLATFORM_OHCI_REGBASE(OHCI_RHPORTST_OFFSET(n))
#define BCM4390X_OHCI_RHPORTST1   PLATFORM_OHCI_REGBASE(OHCI_RHPORTST1_OFFSET)
#define BCM4390X_OHCI_RHPORTST2   PLATFORM_OHCI_REGBASE(OHCI_RHPORTST2_OFFSET)
#define BCM4390X_OHCI_RHPORTST3   PLATFORM_OHCI_REGBASE(OHCI_RHPORTST3_OFFSET)

/* Register bit definitions *************************************************/
/* See include/nuttx/usb/ohci.h */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
FAR struct usbhost_connection_s *bcm4390x_ohci_initialize( void );
int bcm4390x_ohci_isr(int irq, FAR void *context);

#endif /* __ARCH_ARM_SRC_BCM4390X_OHCI_H */
