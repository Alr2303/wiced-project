/*
 * $ Copyright Broadcom Corporation $
 */

/*******************************************************************************
 * arch/arm/src/bcm4390x/bcm4390x_ehci.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
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
 *******************************************************************************/

/*******************************************************************************
 * Included Files
 *******************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/kmalloc.h>
#include <nuttx/wqueue.h>
#include <nuttx/usb/usb.h>
#include <nuttx/usb/usbhost.h>
#include <nuttx/usb/ehci.h>
#include <nuttx/usb/usbhost_trace.h>

#include "up_arch.h"
//#include "cache.h"

#include "bcm4390x_usbhost.h"
#include "bcm4390x_ehci.h"
#include "platform_appscr4.h"
#include "platform_cache.h"
#include "platform_peripheral.h"
#include "bcm4390x_ohci.h"

#ifdef CONFIG_BCM4390X_EHCI

#define arch_clean_dcache(x, y) platform_dcache_clean_range((const volatile void *)x, (y - x))
#define arch_invalidate_dcache(x, y) platform_dcache_inv_range(( volatile void *)x, (y - x))
/*******************************************************************************
 * Pre-processor Definitions
 *******************************************************************************/
/* Configuration ***************************************************************/
/* Pre-requisites */

#ifndef CONFIG_SCHED_WORKQUEUE
#  error Work queue support is required (CONFIG_SCHED_WORKQUEUE)
#endif

/* Configurable number of Queue Head (QH) structures.  The default is one per
 * Root hub port plus one for EP0.
 */

#ifndef CONFIG_BCM4390X_EHCI_NQHS
#  define CONFIG_BCM4390X_EHCI_NQHS (BCM4390X_EHCI_NRHPORT + 1)
#endif

/* Configurable number of Queue Element Transfer Descriptor (qTDs).  The default
 * is one per root hub plus three from EP0.
 */

#ifndef CONFIG_BCM4390X_EHCI_NQTDS
#  define CONFIG_BCM4390X_EHCI_NQTDS (BCM4390X_EHCI_NRHPORT + 3)
#endif

/* Configurable size of a request/descriptor buffers */

#ifndef CONFIG_BCM4390X_EHCI_BUFSIZE
#  define CONFIG_BCM4390X_EHCI_BUFSIZE 128
#endif

/* Debug options */

#ifndef CONFIG_DEBUG
#  undef CONFIG_BCM4390X_EHCI_REGDEBUG
#endif

/* Isochronous transfers are not currently supported */

#undef CONFIG_USBHOST_ISOC_DISABLE
#define CONFIG_USBHOST_ISOC_DISABLE 1

/* Simplify DEBUG checks */

#ifndef CONFIG_DEBUG
#  undef CONFIG_DEBUG_VERBOSE
#  undef CONFIG_DEBUG_USB
#endif

/* Driver-private Definitions **************************************************/

#define EHCI_HC_RH_PPC      0x00000010
#define EHCI_HC_PS_PP       0x1000

/* This is the set of interrupts handled by this driver */

#define EHCI_HANDLED_INTS (EHCI_INT_USBINT | EHCI_INT_USBERRINT | \
                           EHCI_INT_PORTSC |  EHCI_INT_SYSERROR | \
                           EHCI_INT_AAINT)

/* The periodic frame list is a 4K-page aligned array of Frame List Link
 * pointers. The length of the frame list may be programmable. The programmability
 * of the periodic frame list is exported to system software via the HCCPARAMS
 * register. If non-programmable, the length is 1024 elements. If programmable,
 * the length can be selected by system software as one of 256, 512, or 1024
 * elements.
 */

#define FRAME_LIST_SIZE 1024

/* DMA *************************************************************************/
#define bcm4390x_physramaddr(a)     ((a) == 0 ? 0 : (uintptr_t)platform_addr_cached_to_uncached((const void *)(a)))
#define bcm4390x_virtramaddr(a)     ((a) == 0 ? 0 : (uintptr_t)platform_addr_uncached_to_cached((const void *)(a)))

/*******************************************************************************
 * Private Types
 *******************************************************************************/
/* Internal representation of the EHCI Queue Head (QH) */

struct bcm4390x_epinfo_s;
struct bcm4390x_qh_s
{
  /* Fields visible to hardware */

  struct ehci_qh_s hw;         /* Hardware representation of the queue head */

  /* Internal fields used by the EHCI driver */

  struct bcm4390x_epinfo_s *epinfo; /* Endpoint used for the transfer */
  uint32_t fqp;                /* First qTD in the list (physical address) */
  uint8_t pad[8];              /* Padding to assure 32-byte alignment */
};

/* Internal representation of the EHCI Queue Element Transfer Descriptor (qTD) */

struct bcm4390x_qtd_s
{
  /* Fields visible to hardware */

  struct ehci_qtd_s hw;        /* Hardware representation of the queue head */

  /* Internal fields used by the EHCI driver */
};

/* The following is used to manage lists of free QHs and qTDs */

struct bcm4390x_list_s
{
  struct bcm4390x_list_s *flink;    /* Link to next entry in the list */
                               /* Variable length entry data follows */
};

/* List traversal callout functions */

typedef int (*foreach_qh_t)(struct bcm4390x_qh_s *qh, uint32_t **bp, void *arg);
typedef int (*foreach_qtd_t)(struct bcm4390x_qtd_s *qtd, uint32_t **bp, void *arg);

/* This structure describes one endpoint. */

struct bcm4390x_epinfo_s
{
  uint8_t epno:7;              /* Endpoint number */
  uint8_t dirin:1;             /* 1:IN endpoint 0:OUT endpoint */
  uint8_t devaddr:7;           /* Device address */
  uint8_t toggle:1;            /* Next data toggle */
#ifndef CONFIG_USBHOST_INT_DISABLE
  uint8_t interval;            /* Polling interval */
#endif
  uint8_t status;              /* Retained token status bits (for debug purposes) */
  volatile bool iocwait;       /* TRUE: Thread is waiting for transfer completion */
  uint16_t maxpacket:11;       /* Maximum packet size */
  uint16_t xfrtype:2;          /* See USB_EP_ATTR_XFER_* definitions in usb.h */
  uint16_t speed:2;            /* See USB_*_SPEED definitions in ehci.h */
  int result;                  /* The result of the transfer */
  uint32_t xfrd;               /* On completion, will hold the number of bytes transferred */
  sem_t iocsem;                /* Semaphore used to wait for transfer completion */
};

/* This structure retains the state of one root hub port */

struct bcm4390x_rhport_s
{
  /* Common device fields.  This must be the first thing defined in the
   * structure so that it is possible to simply cast from struct usbhost_s
   * to struct bcm4390x_rhport_s.
   */

  struct usbhost_driver_s drvr;

  /* Root hub port status */

  volatile bool connected;     /* Connected to device */
  volatile bool lowspeed;      /* Low speed device attached */
  uint8_t rhpndx;              /* Root hub port index */
  struct bcm4390x_epinfo_s ep0;     /* EP0 endpoint info */

  /* The bound device class driver */

  struct usbhost_class_s *class;
};

/* This structure retains the overall state of the USB host controller */

struct bcm4390x_ehci_s
{
  volatile bool pscwait;       /* TRUE: Thread is waiting for port status change event */
  sem_t exclsem;               /* Support mutually exclusive access */
  sem_t pscsem;                /* Semaphore to wait for port status change events */

  struct bcm4390x_epinfo_s ep0;     /* Endpoint 0 */
  struct bcm4390x_list_s *qhfree;   /* List of free Queue Head (QH) structures */
  struct bcm4390x_list_s *qtdfree;  /* List of free Queue Element Transfer Descriptor (qTD) */
  struct work_s work;          /* Supports interrupt bottom half */

  /* Root hub ports */

  struct bcm4390x_rhport_s rhport[BCM4390X_EHCI_NRHPORT];
};

/*******************************************************************************
 * Private Function Prototypes
 *******************************************************************************/

/* Register operations ********************************************************/

static uint16_t bcm4390x_read16(const uint8_t *addr);
static uint32_t bcm4390x_read32(const uint8_t *addr);
#if 0 /* Not used */
static void bcm4390x_write16(uint16_t memval, uint8_t *addr);
static void bcm4390x_write32(uint32_t memval, uint8_t *addr);
#endif

#ifdef CONFIG_ENDIAN_BIG
#error "Defined CONFIG_ENDIAN_BIG"
static uint16_t bcm4390x_swap16(uint16_t value);
static uint32_t bcm4390x_swap32(uint32_t value);
#else
#  define bcm4390x_swap16(value) (value)
#  define bcm4390x_swap32(value) (value)
#endif

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static void bcm4390x_printreg(volatile uint32_t *regaddr, uint32_t regval,
         bool iswrite);
static void bcm4390x_checkreg(volatile uint32_t *regaddr, uint32_t regval,
         bool iswrite);
static uint32_t bcm4390x_getreg(volatile uint32_t *regaddr);
static void bcm4390x_putreg(uint32_t regval, volatile uint32_t *regaddr);
#else
static inline uint32_t bcm4390x_getreg(volatile uint32_t *regaddr);
static inline void bcm4390x_putreg(uint32_t regval, volatile uint32_t *regaddr);
#endif
static int ehci_wait_usbsts(uint32_t maskbits, uint32_t donebits,
         unsigned int delay);

/* Semaphores ******************************************************************/

static void bcm4390x_takesem(sem_t *sem);
#define bcm4390x_givesem(s) sem_post(s);

/* Allocators ******************************************************************/

static struct bcm4390x_qh_s *bcm4390x_qh_alloc(void);
static void bcm4390x_qh_free(struct bcm4390x_qh_s *qh);
static struct bcm4390x_qtd_s *bcm4390x_qtd_alloc(void);
static void bcm4390x_qtd_free(struct bcm4390x_qtd_s *qtd);

/* List Management *************************************************************/

static int bcm4390x_qh_foreach(struct bcm4390x_qh_s *qh, uint32_t **bp,
         foreach_qh_t handler, void *arg);
static int bcm4390x_qtd_foreach(struct bcm4390x_qh_s *qh, foreach_qtd_t handler,
         void *arg);
static int bcm4390x_qtd_discard(struct bcm4390x_qtd_s *qtd, uint32_t **bp, void *arg);
static int bcm4390x_qh_discard(struct bcm4390x_qh_s *qh);

/* Cache Operations ************************************************************/

static int bcm4390x_qtd_flush(struct bcm4390x_qtd_s *qtd, uint32_t **bp, void *arg);
static int bcm4390x_qh_flush(struct bcm4390x_qh_s *qh);

/* Endpoint Transfer Handling **************************************************/

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static void bcm4390x_qtd_print(struct bcm4390x_qtd_s *qtd);
static void bcm4390x_qh_print(struct bcm4390x_qh_s *qh);
static int bcm4390x_qtd_dump(struct bcm4390x_qtd_s *qtd, uint32_t **bp, void *arg);
static int bcm4390x_qh_dump(struct bcm4390x_qh_s *qh, uint32_t **bp, void *arg);
#else
#  define bcm4390x_qtd_print(qtd)
#  define bcm4390x_qh_print(qh)
#  define bcm4390x_qtd_dump(qtd, bp, arg) OK
#  define bcm4390x_qh_dump(qh, bp, arg)   OK
#endif

static int bcm4390x_ioc_setup(struct bcm4390x_rhport_s *rhport, struct bcm4390x_epinfo_s *epinfo);
static int bcm4390x_ioc_wait(struct bcm4390x_epinfo_s *epinfo);
static void bcm4390x_qh_enqueue(struct bcm4390x_qh_s *qhead, struct bcm4390x_qh_s *qh);
static struct bcm4390x_qh_s *bcm4390x_qh_create(struct bcm4390x_rhport_s *rhport,
         struct bcm4390x_epinfo_s *epinfo);
static int bcm4390x_qtd_addbpl(struct bcm4390x_qtd_s *qtd, const void *buffer, size_t buflen);
static struct bcm4390x_qtd_s *bcm4390x_qtd_setupphase(struct bcm4390x_epinfo_s *epinfo,
         const struct usb_ctrlreq_s *req);
static struct bcm4390x_qtd_s *bcm4390x_qtd_dataphase(struct bcm4390x_epinfo_s *epinfo,
         void *buffer, int buflen, uint32_t tokenbits);
static struct bcm4390x_qtd_s *bcm4390x_qtd_statusphase(uint32_t tokenbits);
static ssize_t bcm4390x_async_transfer(struct bcm4390x_rhport_s *rhport,
         struct bcm4390x_epinfo_s *epinfo, const struct usb_ctrlreq_s *req,
         uint8_t *buffer, size_t buflen);
#ifndef CONFIG_USBHOST_INT_DISABLE
static ssize_t bcm4390x_intr_transfer(struct bcm4390x_rhport_s *rhport,
         struct bcm4390x_epinfo_s *epinfo, uint8_t *buffer, size_t buflen);
#endif

/* Interrupt Handling **********************************************************/

static int bcm4390x_qtd_ioccheck(struct bcm4390x_qtd_s *qtd, uint32_t **bp, void *arg);
static int bcm4390x_qh_ioccheck(struct bcm4390x_qh_s *qh, uint32_t **bp, void *arg);
static inline void bcm4390x_ioc_bottomhalf(void);
static inline void bcm4390x_portsc_bottomhalf(void);
static inline void bcm4390x_syserr_bottomhalf(void);
static inline void bcm4399x_async_advance_bottomhalf(void);
//static void bcm4390x_ehci_bottomhalf(FAR void *arg);
static int bcm4390x_ehci_isr(int irq, FAR void *context);

/* USB Host Controller Operations **********************************************/

static int bcm4390x_ehci_wait(FAR struct usbhost_connection_s *conn,
         FAR const bool *connected);
static int bcm4390x_ehci_enumerate(FAR struct usbhost_connection_s *conn, int rhpndx);

static int bcm4390x_ehci_ep0configure(FAR struct usbhost_driver_s *drvr, uint8_t funcaddr,
         uint16_t maxpacketsize);
static int bcm4390x_ehci_getdevinfo(FAR struct usbhost_driver_s *drvr,
         FAR struct usbhost_devinfo_s *devinfo);
static int bcm4390x_ehci_epalloc(FAR struct usbhost_driver_s *drvr,
         const FAR struct usbhost_epdesc_s *epdesc, usbhost_ep_t *ep);
static int bcm4390x_ehci_epfree(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep);
static int bcm4390x_ehci_alloc(FAR struct usbhost_driver_s *drvr,
         FAR uint8_t **buffer, FAR size_t *maxlen);
static int bcm4390x_ehci_free(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer);
static int bcm4390x_ehci_ioalloc(FAR struct usbhost_driver_s *drvr,
         FAR uint8_t **buffer, size_t buflen);
static int bcm4390x_ehci_iofree(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer);
static int bcm4390x_ehci_ctrlin(FAR struct usbhost_driver_s *drvr,
         FAR const struct usb_ctrlreq_s *req, FAR uint8_t *buffer);
static int bcm4390x_ehci_ctrlout(FAR struct usbhost_driver_s *drvr,
         FAR const struct usb_ctrlreq_s *req, FAR const uint8_t *buffer);
static int bcm4390x_ehci_transfer(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep,
         FAR uint8_t *buffer, size_t buflen);
static void bcm4390x_ehci_disconnect(FAR struct usbhost_driver_s *drvr);

/* Initialization **************************************************************/

static int bcm4390x_ehci_reset(void);

/*******************************************************************************
 * Private Data
 *******************************************************************************/
/* In this driver implementation, support is provided for only a single a single
 * USB device.  All status information can be simply retained in a single global
 * instance.
 */

static struct bcm4390x_ehci_s g_ehci;

/* This is the connection/enumeration interface */

static struct usbhost_connection_s g_ehciconn;

/* The head of the asynchronous queue */

static struct bcm4390x_qh_s g_asynchead __attribute__ ((aligned(32)));

#ifndef CONFIG_USBHOST_INT_DISABLE
/* The head of the periodic queue */

static struct bcm4390x_qh_s g_intrhead   __attribute__ ((aligned(32)));

/* The frame list */

#ifdef CONFIG_BCM4390X_EHCI_PREALLOCATE
static uint32_t g_framelist[FRAME_LIST_SIZE] __attribute__ ((aligned(4096)));
#else
static uint32_t *g_framelist;
#endif
#endif /* CONFIG_USBHOST_INT_DISABLE */

#ifdef CONFIG_BCM4390X_EHCI_PREALLOCATE
/* Pools of pre-allocated data structures.  These will all be linked into the
 * free lists within g_ehci.  These must all be aligned to 32-byte boundaries
 */

/* Queue Head (QH) pool */

static struct bcm4390x_qh_s g_qhpool[CONFIG_BCM4390X_EHCI_NQHS]
                       __attribute__ ((aligned(32)));

/* Queue Element Transfer Descriptor (qTD) pool */

static struct bcm4390x_qtd_s g_qtdpool[CONFIG_BCM4390X_EHCI_NQTDS]
                        __attribute__ ((aligned(32)));

#else
/* Pools of dynamically data structures.  These will all be linked into the
 * free lists within g_ehci.  These must all be aligned to 32-byte boundaries
 */

/* Queue Head (QH) pool */

static struct bcm4390x_qh_s *g_qhpool;

/* Queue Element Transfer Descriptor (qTD) pool */

static struct bcm4390x_qtd_s *g_qtdpool;

#endif

/*******************************************************************************
 * Private Functions
 *******************************************************************************/
/*******************************************************************************
 * Register Operations
 *******************************************************************************/
/*******************************************************************************
 * Name: bcm4390x_read16
 *
 * Description:
 *   Read 16-bit little endian data
 *
 *******************************************************************************/

static uint16_t bcm4390x_read16(const uint8_t *addr)
{
#ifdef CONFIG_ENDIAN_BIG
  return (uint16_t)addr[0] << 8 | (uint16_t)addr[1];
#else
  return (uint16_t)addr[1] << 8 | (uint16_t)addr[0];
#endif
}

/*******************************************************************************
 * Name: bcm4390x_read32
 *
 * Description:
 *   Read 32-bit little endian data
 *
 *******************************************************************************/

static inline uint32_t bcm4390x_read32(const uint8_t *addr)
{
#ifdef CONFIG_ENDIAN_BIG
  return (uint32_t)bcm4390x_read16(&addr[0]) << 16 |
         (uint32_t)bcm4390x_read16(&addr[2]);
#else
  return (uint32_t)bcm4390x_read16(&addr[2]) << 16 |
         (uint32_t)bcm4390x_read16(&addr[0]);
#endif
}


/*******************************************************************************
 * Name: bcm4390x_swap16
 *
 * Description:
 *   Swap bytes on a 16-bit value
 *
 *******************************************************************************/

#ifdef CONFIG_ENDIAN_BIG
static uint16_t bcm4390x_swap16(uint16_t value)
{
  return ((value >> 8) & 0xff) | ((value & 0xff) << 8);
}
#endif

/*******************************************************************************
 * Name: bcm4390x_swap32
 *
 * Description:
 *   Swap bytes on a 32-bit value
 *
 *******************************************************************************/

#ifdef CONFIG_ENDIAN_BIG
static uint32_t bcm4390x_swap32(uint32_t value)
{
  return (uint32_t)bcm4390x_swap16((uint16_t)((value >> 16) & 0xffff)) |
         (uint32_t)bcm4390x_swap16((uint16_t)(value & 0xffff)) << 16;
}
#endif

/*******************************************************************************
 * Name: bcm4390x_printreg
 *
 * Description:
 *   Print the contents of a BCM4390X EHCI register
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static void bcm4390x_printreg(volatile uint32_t *regaddr, uint32_t regval,
                          bool iswrite)
{
  lldbg("%08x%s%08x\n", (uintptr_t)regaddr, iswrite ? "<-" : "->", regval);
}
#endif

/*******************************************************************************
 * Name: bcm4390x_checkreg
 *
 * Description:
 *   Check if it is time to output debug information for accesses to a BCM4390X
 *   EHCI register
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static void bcm4390x_checkreg(volatile uint32_t *regaddr, uint32_t regval, bool iswrite)
{
  static uint32_t *prevaddr = NULL;
  static uint32_t preval = 0;
  static uint32_t count = 0;
  static bool     prevwrite = false;

  /* Is this the same value that we read from/wrote to the same register last time?
   * Are we polling the register?  If so, suppress the output.
   */

  if (regaddr == prevaddr && regval == preval && prevwrite == iswrite)
    {
      /* Yes.. Just increment the count */

      count++;
    }
  else
    {
      /* No this is a new address or value or operation. Were there any
       * duplicate accesses before this one?
       */

      if (count > 0)
        {
          /* Yes.. Just one? */

          if (count == 1)
            {
              /* Yes.. Just one */

              bcm4390x_printreg(prevaddr, preval, prevwrite);
            }
          else
            {
              /* No.. More than one. */

              lldbg("[repeats %d more times]\n", count);
            }
        }

      /* Save the new address, value, count, and operation for next time */

      prevaddr  = (uint32_t *)regaddr;
      preval    = regval;
      count     = 0;
      prevwrite = iswrite;

      /* Show the new register access */

      bcm4390x_printreg(regaddr, regval, iswrite);
    }
}
#endif

/*******************************************************************************
 * Name: bcm4390x_getreg
 *
 * Description:
 *   Get the contents of an BCM4390X register
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static uint32_t bcm4390x_getreg(volatile uint32_t *regaddr)
{
  /* Read the value from the register */

  uint32_t regval = *regaddr;

  /* Check if we need to print this value */

  bcm4390x_checkreg(regaddr, regval, false);
  return regval;
}
#else
static inline uint32_t bcm4390x_getreg(volatile uint32_t *regaddr)
{
  return *regaddr;
}
#endif

/*******************************************************************************
 * Name: bcm4390x_putreg
 *
 * Description:
 *   Set the contents of an BCM4390X register to a value
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static void bcm4390x_putreg(uint32_t regval, volatile uint32_t *regaddr)
{
  /* Check if we need to print this value */

  bcm4390x_checkreg(regaddr, regval, true);

  /* Write the value */

  *regaddr = regval;
}
#else
static inline void bcm4390x_putreg(uint32_t regval, volatile uint32_t *regaddr)
{
  *regaddr = regval;
}
#endif

/*******************************************************************************
 * Name: ehci_wait_usbsts
 *
 * Description:
 *   Wait for either (1) a field in the USBSTS register to take a specific
 *   value, (2) for a timeout to occur, or (3) a error to occur.  Return
 *   a value to indicate which terminated the wait.
 *
 *******************************************************************************/

static int ehci_wait_usbsts(uint32_t maskbits, uint32_t donebits,
                            unsigned int delay)
{
  uint32_t regval;
  unsigned int timeout;

  timeout = 0;
  do
    {
      /* Wait 5usec before trying again */

      up_mdelay(5);
      timeout += 5;

      /* Read the USBSTS register and check for a system error */

      regval = bcm4390x_getreg(&HCOR->usbsts);
      if ((regval & EHCI_INT_SYSERROR) != 0)
        {
          usbhost_trace1(EHCI_TRACE1_SYSTEMERROR, regval);
          return -EIO;
        }

      /* Mask out the bits of interest */

      regval &= maskbits;

      /* Loop until the masked bits take the specified value or until a
       * timeout occurs.
       */
    }
  while (regval != donebits && timeout < delay);

  /* We got here because either the waited for condition or a timeout
   * occurred.  Return a value to indicate which.
   */

  return (regval == donebits) ? OK : -ETIMEDOUT;
}

/*******************************************************************************
 * Semaphores
 *******************************************************************************/
/*******************************************************************************
 * Name: bcm4390x_takesem
 *
 * Description:
 *   This is just a wrapper to handle the annoying behavior of semaphore
 *   waits that return due to the receipt of a signal.
 *
 *******************************************************************************/

static void bcm4390x_takesem(sem_t *sem)
{
  /* Take the semaphore (perhaps waiting) */

  while (sem_wait(sem) != 0)
    {
      /* The only case that an error should occr here is if the wait was
       * awakened by a signal.
       */

      ASSERT(errno == EINTR);
    }
}

/*******************************************************************************
 * Allocators
 *******************************************************************************/
/*******************************************************************************
 * Name: bcm4390x_qh_alloc
 *
 * Description:
 *   Allocate a Queue Head (QH) structure by removing it from the free list
 *
 * Assumption:  Caller holds the exclsem
 *
 *******************************************************************************/

static struct bcm4390x_qh_s *bcm4390x_qh_alloc(void)
{
  struct bcm4390x_qh_s *qh;

  /* Remove the QH structure from the freelist */

  qh = (struct bcm4390x_qh_s *)g_ehci.qhfree;
  if (qh)
    {
      g_ehci.qhfree = ((struct bcm4390x_list_s *)qh)->flink;
      memset(qh, 0, sizeof(struct bcm4390x_qh_s));
    }

  return qh;
}

/*******************************************************************************
 * Name: bcm4390x_qh_free
 *
 * Description:
 *   Free a Queue Head (QH) structure by returning it to the free list
 *
 * Assumption:  Caller holds the exclsem
 *
 *******************************************************************************/

static void bcm4390x_qh_free(struct bcm4390x_qh_s *qh)
{
  struct bcm4390x_list_s *entry = (struct bcm4390x_list_s *)qh;

  /* Put the QH structure back into the free list */

  entry->flink  = g_ehci.qhfree;
  g_ehci.qhfree = entry;
}

/*******************************************************************************
 * Name: bcm4390x_qtd_alloc
 *
 * Description:
 *   Allocate a Queue Element Transfer Descriptor (qTD) by removing it from the
 *   free list
 *
 * Assumption:  Caller holds the exclsem
 *
 *******************************************************************************/

static struct bcm4390x_qtd_s *bcm4390x_qtd_alloc(void)
{
  struct bcm4390x_qtd_s *qtd;

  /* Remove the qTD from the freelist */

  qtd = (struct bcm4390x_qtd_s *)g_ehci.qtdfree;
  if (qtd)
    {
      g_ehci.qtdfree = ((struct bcm4390x_list_s *)qtd)->flink;
      memset(qtd, 0, sizeof(struct bcm4390x_qtd_s));
    }

  return qtd;
}

/*******************************************************************************
 * Name: bcm4390x_qtd_free
 *
 * Description:
 *   Free a Queue Element Transfer Descriptor (qTD) by returning it to the free
 *   list
 *
 * Assumption:  Caller holds the exclsem
 *
 *******************************************************************************/

static void bcm4390x_qtd_free(struct bcm4390x_qtd_s *qtd)
{
  struct bcm4390x_list_s *entry = (struct bcm4390x_list_s *)qtd;

  /* Put the qTD back into the free list */

  entry->flink   = g_ehci.qtdfree;
  g_ehci.qtdfree = entry;
}

/*******************************************************************************
 * List Management
 *******************************************************************************/

/*******************************************************************************
 * Name: bcm4390x_qh_foreach
 *
 * Description:
 *   Give the first entry in a list of Queue Head (QH) structures, call the
 *   handler for each QH structure in the list (including the one at the head
 *   of the list).
 *
 *******************************************************************************/

static int bcm4390x_qh_foreach(struct bcm4390x_qh_s *qh, uint32_t **bp, foreach_qh_t handler,
                          void *arg)
{
  struct bcm4390x_qh_s *next;
  uintptr_t physaddr;
  int ret;

  DEBUGASSERT(qh && handler);

  while (qh)
    {
      /* Is this the end of the list?  Check the horizontal link pointer (HLP)
       * terminate (T) bit.  If T==1, then the HLP address is not valid.
       */
      physaddr = bcm4390x_swap32(qh->hw.hlp);

      if ((physaddr & QH_HLP_T) != 0)
        {
          /* Set the next pointer to NULL.  This will terminate the loop. */

          next = NULL;
        }

      /* Is the next QH the asynchronous list head which will always be at
       * the end of the asynchronous queue?
       */

      else if (bcm4390x_virtramaddr(physaddr & QH_HLP_MASK) == (uintptr_t)&g_asynchead)
        {
          /* That will also terminate the loop */

          next = NULL;
        }

      /* Otherwise, there is a QH structure after this one that describes
       * another transaction.
       */

      else
        {
          physaddr = bcm4390x_swap32(qh->hw.hlp) & QH_HLP_MASK;
          next     = (struct bcm4390x_qh_s *)bcm4390x_virtramaddr(physaddr);
        }

      /* Perform the user action on this entry.  The action might result in
       * unlinking the entry!  But that is okay because we already have the
       * next QH pointer.
       *
       * Notice that we do not manage the back pointer (bp).  If the callout
       * uses it, it must update it as necessary.
       */

      ret = handler(qh, bp, arg);

      /* If the handler returns any non-zero value, then terminate the traversal
       * early.
       */

      if (ret != 0)
        {
          return ret;
        }

      /* Set up to visit the next entry */

      qh = next;
    }

  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_qtd_foreach
 *
 * Description:
 *   Give a Queue Head (QH) instance, call the handler for each qTD structure
 *   in the queue.
 *
 *******************************************************************************/

static int bcm4390x_qtd_foreach(struct bcm4390x_qh_s *qh, foreach_qtd_t handler, void *arg)
{
  struct bcm4390x_qtd_s *qtd;
  struct bcm4390x_qtd_s *next;
  uintptr_t physaddr;
  uint32_t *bp;
  int ret;

  DEBUGASSERT(qh && handler);

  /* Handle the special case where the queue is empty */

  bp       = &qh->fqp;         /* Start of qTDs in original list */
  physaddr = bcm4390x_swap32(*bp);  /* Physical address of first qTD in CPU order */

  if ((physaddr & QTD_NQP_T) != 0)
    {
      return 0;
    }

  /* Start with the first qTD in the list */

  qtd  = (struct bcm4390x_qtd_s *)bcm4390x_virtramaddr(physaddr);
  next = NULL;

  /* And loop until we encounter the end of the qTD list */

  while (qtd)
    {
      /* Is this the end of the list?  Check the next qTD pointer (NQP)
       * terminate (T) bit.  If T==1, then the NQP address is not valid.
       */

      if ((bcm4390x_swap32(qtd->hw.nqp) & QTD_NQP_T) != 0)
        {
          /* Set the next pointer to NULL.  This will terminate the loop. */

          next = NULL;
        }
      else
        {
          physaddr = bcm4390x_swap32(qtd->hw.nqp) & QTD_NQP_NTEP_MASK;
          next     = (struct bcm4390x_qtd_s *)bcm4390x_virtramaddr(physaddr);
        }

      /* Perform the user action on this entry.  The action might result in
       * unlinking the entry!  But that is okay because we already have the
       * next qTD pointer.
       *
       * Notice that we do not manage the back pointer (bp).  If the callout
       * uses it, it must update it as necessary.
       */

      ret = handler(qtd, &bp, arg);

      /* If the handler returns any non-zero value, then terminate the traversal
       * early.
       */

      if (ret != 0)
        {
          return ret;
        }

      /* Set up to visit the next entry */

      qtd = next;
    }

  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_qtd_discard
 *
 * Description:
 *   This is a bcm4390x_qtd_foreach callback.  It simply unlinks the QTD, updates
 *   the back pointer, and frees the QTD structure.
 *
 *******************************************************************************/

static int bcm4390x_qtd_discard(struct bcm4390x_qtd_s *qtd, uint32_t **bp, void *arg)
{
  DEBUGASSERT(qtd && bp && *bp);

  /* Remove the qTD from the list by updating the forward pointer to skip
   * around this qTD.  We do not change that pointer because are repeatedly
   * removing the aTD at the head of the QH list.
   */

  **bp = qtd->hw.nqp;

  /* Then free the qTD */

  bcm4390x_qtd_free(qtd);
  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_qh_discard
 *
 * Description:
 *   Free the Queue Head (QH) and all qTD's attached to the QH.
 *
 * Assumptions:
 *   The QH structure itself has already been unlinked from whatever list it
 *   may have been in.
 *
 *******************************************************************************/

static int bcm4390x_qh_discard(struct bcm4390x_qh_s *qh)
{
  int ret;

  DEBUGASSERT(qh);

  /* Free all of the qTD's attached to the QH */

  ret = bcm4390x_qtd_foreach(qh, bcm4390x_qtd_discard, NULL);
  if (ret < 0)
    {
      usbhost_trace1(EHCI_TRACE1_QTDFOREACH_FAILED, -ret);
    }

  /* Then free the QH itself */

  bcm4390x_qh_free(qh);
  return ret;
}


/*******************************************************************************
 * Name: bcm4390x_qtd_flush
 *
 * Description:
 *   This is a callback from bcm4390x_qtd_foreach.  It simply flushes D-cache for
 *   address range of the qTD entry.
 *
 *******************************************************************************/

static int bcm4390x_qtd_flush(struct bcm4390x_qtd_s *qtd, uint32_t **bp, void *arg)
{
  /* Flush the D-Cache, i.e., make the contents of the memory match the contents
   * of the D-Cache in the specified address range and invalidate the D-Cache
   * to force re-loading of the data from memory when next accessed.
   */

 arch_clean_dcache((uintptr_t)&qtd->hw,
                    (uintptr_t)&qtd->hw + sizeof(struct ehci_qtd_s));
  arch_invalidate_dcache((uintptr_t)&qtd->hw,
                         (uintptr_t)&qtd->hw + sizeof(struct ehci_qtd_s));

  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_qh_flush
 *
 * Description:
 *   Invalidate the Queue Head and all qTD entries in the queue.
 *
 *******************************************************************************/

static int bcm4390x_qh_flush(struct bcm4390x_qh_s *qh)
{
  /* Flush the QH first.  This will write the contents of the D-cache to RAM and
   * invalidate the contents of the D-cache so that the next access will be
   * reloaded from D-Cache.
   */

#if 0 /* Didn't behave as expected */
  arch_flush_dcache((uintptr_t)&qh->hw,
                    (uintptr_t)&qh->hw + sizeof(struct ehci_qh_s));
#else
  arch_clean_dcache((uintptr_t)&qh->hw,
                     (uintptr_t)&qh->hw + sizeof(struct ehci_qh_s));
  arch_invalidate_dcache((uintptr_t)&qh->hw,
                         (uintptr_t)&qh->hw + sizeof(struct ehci_qh_s));
#endif

  /* Then flush all of the qTD entries in the queue */

  return bcm4390x_qtd_foreach(qh, bcm4390x_qtd_flush, NULL);
}

/*******************************************************************************
 * Endpoint Transfer Handling
 *******************************************************************************/

/*******************************************************************************
 * Name: bcm4390x_qtd_print
 *
 * Description:
 *   Print the context of one qTD
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static void bcm4390x_qtd_print(struct bcm4390x_qtd_s *qtd)
{
  udbg("  QTD[%p]:\n", qtd);
  udbg("    hw:\n");
  udbg("      nqp: %08x alt: %08x token: %08x\n",
       qtd->hw.nqp, qtd->hw.alt, qtd->hw.token);
  udbg("      bpl: %08x %08x %08x %08x %08x\n",
       qtd->hw.bpl[0], qtd->hw.bpl[1], qtd->hw.bpl[2],
       qtd->hw.bpl[3], qtd->hw.bpl[4]);
}
#endif

/*******************************************************************************
 * Name: bcm4390x_qh_print
 *
 * Description:
 *   Print the context of one QH
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static void bcm4390x_qh_print(struct bcm4390x_qh_s *qh)
{
  struct bcm4390x_epinfo_s *epinfo;
  struct ehci_overlay_s *overlay;

  udbg("QH[%p]:\n", qh);
  udbg("  hw:\n");
  udbg("    hlp: %08x epchar: %08x epcaps: %08x cqp: %08x\n",
       qh->hw.hlp, qh->hw.epchar, qh->hw.epcaps, qh->hw.cqp);

  overlay = &qh->hw.overlay;
  udbg("  overlay:\n");
  udbg("    nqp: %08x alt: %08x token: %08x\n",
       overlay->nqp, overlay->alt, overlay->token);
  udbg("    bpl: %08x %08x %08x %08x %08x\n",
       overlay->bpl[0], overlay->bpl[1], overlay->bpl[2],
       overlay->bpl[3], overlay->bpl[4]);

  udbg("  fqp:\n", qh->fqp);

  epinfo = qh->epinfo;
  udbg("  epinfo[%p]:\n", epinfo);
  if (epinfo)
    {
      udbg("    EP%d DIR=%s FA=%08x TYPE=%d MaxPacket=%d\n",
           epinfo->epno, epinfo->dirin ? "IN" : "OUT", epinfo->devaddr,
           epinfo->xfrtype, epinfo->maxpacket);
      udbg("    Toggle=%d iocwait=%d speed=%d result=%d\n",
           epinfo->toggle, epinfo->iocwait, epinfo->speed, epinfo->result);
    }
}
#endif

/*******************************************************************************
 * Name: bcm4390x_qtd_dump
 *
 * Description:
 *   This is a bcm4390x_qtd_foreach callout function.  It dumps the context of one
 *   qTD
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static int bcm4390x_qtd_dump(struct bcm4390x_qtd_s *qtd, uint32_t **bp, void *arg)
{
  bcm4390x_qtd_print(qtd);
  return OK;
}
#endif

/*******************************************************************************
 * Name: bcm4390x_qh_dump
 *
 * Description:
 *   This is a bcm4390x_qh_foreach callout function.  It dumps a QH structure and
 *   all of the qTD structures linked to the QH.
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_EHCI_REGDEBUG
static int bcm4390x_qh_dump(struct bcm4390x_qh_s *qh, uint32_t **bp, void *arg)
{
  bcm4390x_qh_print(qh);
  return bcm4390x_qtd_foreach(qh, bcm4390x_qtd_dump, NULL);
}
#endif

/*******************************************************************************
 * Name: bcm4390x_ioc_setup
 *
 * Description:
 *   Set the request for the IOC event well BEFORE enabling the transfer (as
 *   soon as we are absolutely committed to the to avoid transfer).  We do this
 *   to minimize race conditions.  This logic would have to be expanded if we
 *   want to have more than one packet in flight at a time!
 *
 * Assumption:  The caller holds tex EHCI exclsem
 *
 *******************************************************************************/

static int bcm4390x_ioc_setup(struct bcm4390x_rhport_s *rhport, struct bcm4390x_epinfo_s *epinfo)
{
  irqstate_t flags;
  int ret = -ENODEV;

  DEBUGASSERT(rhport && epinfo && !epinfo->iocwait);

  /* Is the device still connected? */

  flags = irqsave();
  if (rhport->connected)
    {
      /* Then set wdhwait to indicate that we expect to be informed when
       * either (1) the device is disconnected, or (2) the transfer
       * completed.
       */

      epinfo->iocwait = true;   /* We want to be awakened by IOC interrupt */
      epinfo->status  = 0;      /* No status yet */
      epinfo->xfrd    = 0;      /* Nothing transferred yet */
      epinfo->result  = -EBUSY; /* Transfer in progress */
      ret             = OK;     /* We are good to go */
    }

  irqrestore(flags);
  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ioc_wait
 *
 * Description:
 *   Wait for the IOC event.
 *
 * Assumption:  The caller does *NOT* hold the EHCI exclsem.  That would cause
 * a deadlock when the bottom-half, worker thread needs to take the semaphore.
 *
 *******************************************************************************/

static int bcm4390x_ioc_wait(struct bcm4390x_epinfo_s *epinfo)
{
  /* Wait for the IOC event.  Loop to handle any false alarm semaphore counts. */

  while (epinfo->iocwait)
    {

      bcm4390x_takesem(&epinfo->iocsem);
    }

  return epinfo->result;
}

/*******************************************************************************
 * Name: bcm4390x_qh_enqueue
 *
 * Description:
 *   Add a new, ready-to-go QH w/attached qTDs to the asynchonous queue.
 *
 * Assumptions:  The caller holds the EHCI exclsem
 *
 *******************************************************************************/

static void bcm4390x_qh_enqueue(struct bcm4390x_qh_s *qhead, struct bcm4390x_qh_s *qh)
{
  uintptr_t physaddr;

  /* Set the internal fqp field.  When we transverse the QH list later,
   * we need to know the correct place to start because the overlay may no
   * longer point to the first qTD entry.
   */

  qh->fqp = qh->hw.overlay.nqp;
  (void)bcm4390x_qh_dump(qh, NULL, NULL);

  /* Add the new QH to the head of the asynchronous queue list.
   *
   * First, attach the old head as the new QH HLP and flush the new QH and its
   * attached qTDs to RAM.
   */

  qh->hw.hlp = qhead->hw.hlp;
//  dbg("%s, qh: 0x%lx, qh->hw.hlp: 0x%lx\n", __FUNCTION__, (uint32_t)qh, (uint32_t)qh->hw.hlp);
  bcm4390x_qh_flush(qh);

  /* Then set the new QH as the first QH in the asychronous queue and flush the
   * modified head to RAM.
   */

  physaddr = (uintptr_t)bcm4390x_physramaddr((uintptr_t)qh);
  qhead->hw.hlp = bcm4390x_swap32(physaddr | QH_HLP_TYP_QH);
  arch_clean_dcache((uintptr_t)&qhead->hw,
                    (uintptr_t)&qhead->hw + sizeof(struct ehci_qh_s));
}

/*******************************************************************************
 * Name: bcm4390x_qh_create
 *
 * Description:
 *   Create a new Queue Head (QH)
 *
 *******************************************************************************/

static struct bcm4390x_qh_s *bcm4390x_qh_create(struct bcm4390x_rhport_s *rhport,
                                      struct bcm4390x_epinfo_s *epinfo)
{
  struct bcm4390x_qh_s *qh;
  uint32_t regval;

  /* Allocate a new queue head structure */

  qh = bcm4390x_qh_alloc();
  if (qh == NULL)
    {
      usbhost_trace1(EHCI_TRACE1_QHALLOC_FAILED, 0);
      return NULL;
    }

  /* Save the endpoint information with the QH itself */

  qh->epinfo = epinfo;

  /* Write QH endpoint characteristics:
   *
   * FIELD    DESCRIPTION                     VALUE/SOURCE
   * -------- ------------------------------- --------------------
   * DEVADDR  Device address                  Endpoint structure
   * I        Inactivate on Next Transaction  0
   * ENDPT    Endpoint number                 Endpoint structure
   * EPS      Endpoint speed                  Endpoint structure
   * DTC      Data toggle control             1
   * MAXPKT   Max packet size                 Endpoint structure
   * C        Control endpoint                Calculated
   * RL       NAK count reloaded              8
   */

  regval = ((uint32_t)epinfo->devaddr   << QH_EPCHAR_DEVADDR_SHIFT) |
           ((uint32_t)epinfo->epno      << QH_EPCHAR_ENDPT_SHIFT) |
           ((uint32_t)epinfo->speed     << QH_EPCHAR_EPS_SHIFT) |
           QH_EPCHAR_DTC |
           ((uint32_t)epinfo->maxpacket << QH_EPCHAR_MAXPKT_SHIFT) |
           ((uint32_t)8 << QH_EPCHAR_RL_SHIFT);

  /* Paragraph 3.6.3: "Control Endpoint Flag (C). If the QH.EPS field
   * indicates the endpoint is not a high-speed device, and the endpoint
   * is an control endpoint, then software must set this bit to a one.
   * Otherwise it should always set this bit to a zero."
   */

  if (epinfo->speed   != EHCI_HIGH_SPEED &&
      epinfo->xfrtype == USB_EP_ATTR_XFER_CONTROL)
    {
      regval |= QH_EPCHAR_C;
    }

  /* Save the endpoint characteristics word with the correct byte order */

  qh->hw.epchar = bcm4390x_swap32(regval);

  /* Write QH endpoint capabilities
   *
   * FIELD    DESCRIPTION                     VALUE/SOURCE
   * -------- ------------------------------- --------------------
   * SSMASK   Interrupt Schedule Mask         Depends on epinfo->xfrtype
   * SCMASK   Split Completion Mask           0
   * HUBADDR  Hub Address                     Always 0 for now
   * PORT     Port number                     RH port index + 1
   * MULT     High band width multiplier      1
   *
   * REVISIT:  Future HUB support will require the HUB port number
   * and HUB device address to be included here.
   */

  regval = ((uint32_t)0                    << QH_EPCAPS_HUBADDR_SHIFT) |
           ((uint32_t)(rhport->rhpndx + 1) << QH_EPCAPS_PORT_SHIFT) |
           ((uint32_t)1                    << QH_EPCAPS_MULT_SHIFT);

#ifndef CONFIG_USBHOST_INT_DISABLE
  if (epinfo->xfrtype == USB_EP_ATTR_XFER_INT)
    {
      /* Here, the S-Mask field in the queue head is set to 1, indicating
       * that the transaction for the endpoint should be executed on the bus
       * during micro-frame 0 of the frame.
       *
       * REVISIT: The polling interval should be controlled by the which
       * entry is the framelist holds the QH pointer for a given micro-frame
       * and the QH pointer should be replicated for different polling rates.
       * This implementation currently just sets all frame_list entry to
       * all the same interrupt queue.  That should work but will not give
       * any control over polling rates.
       */

      regval |= ((uint32_t)1               << QH_EPCAPS_SSMASK_SHIFT);
    }
#endif

  qh->hw.epcaps = bcm4390x_swap32(regval);

  /* Mark this as the end of this list.  This will be overwritten if/when the
   * next qTD is added to the queue.
   */

  qh->hw.hlp         = bcm4390x_swap32(QH_HLP_T);
  qh->hw.overlay.nqp = bcm4390x_swap32(QH_NQP_T);
  qh->hw.overlay.alt = bcm4390x_swap32(QH_AQP_T);
  return qh;
}

/*******************************************************************************
 * Name: bcm4390x_qtd_addbpl
 *
 * Description:
 *   Add a buffer pointer list to a qTD.
 *
 *******************************************************************************/

static int bcm4390x_qtd_addbpl(struct bcm4390x_qtd_s *qtd, const void *buffer, size_t buflen)
{
  uint32_t physaddr;
  uint32_t nbytes;
  uint32_t next;
  int ndx;

  /* Flush the contents of the data buffer to RAM so that the correct contents
   * will be accessed for an OUT DMA.
   */

#if 0 /* Didn't behave as expected */
  arch_flush_dcache((uintptr_t)buffer, (uintptr_t)buffer + buflen);
#else
  arch_clean_dcache((uintptr_t)buffer, (uintptr_t)buffer + buflen);
  arch_invalidate_dcache((uintptr_t)buffer, (uintptr_t)buffer + buflen);
#endif

  /* Loop, adding the aligned physical addresses of the buffer to the buffer page
   * list.  Only the first entry need not be aligned (because only the first
   * entry has the offset field). The subsequent entries must begin on 4KB
   * address boundaries.
   */

  physaddr = (uint32_t)bcm4390x_physramaddr((uintptr_t)buffer);

  for (ndx = 0; ndx < 5; ndx++)
    {
      /* Write the physical address of the buffer into the qTD buffer pointer
       * list.
       */

      qtd->hw.bpl[ndx] = bcm4390x_swap32(physaddr);

      /* Get the next buffer pointer (in the case where we will have to transfer
       * more then one chunk).  This buffer must be aligned to a 4KB address
       * boundary.
       */

      next = (physaddr + 4096) & ~4095;

      /* How many bytes were included in the last buffer?  Was it the whole
       * thing?
       */

      nbytes = next - physaddr;
      if (nbytes >= buflen)
        {
          /* Yes... it was the whole thing.  Break out of the loop early. */

          break;
        }

      /* Adjust the buffer length and physical address for the next time
       * through the loop.
       */

      buflen  -= nbytes;
      physaddr = next;
    }

  /* Handle the case of a huge buffer > 4*4KB = 16KB */

  if (ndx >= 5)
    {
      usbhost_trace1(EHCI_TRACE1_BUFTOOBIG, buflen);
      return -EFBIG;
    }

  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_qtd_setupphase
 *
 * Description:
 *   Create a SETUP phase request qTD.
 *
 *******************************************************************************/

static struct bcm4390x_qtd_s *bcm4390x_qtd_setupphase(struct bcm4390x_epinfo_s *epinfo,
                                            const struct usb_ctrlreq_s *req)
{
  struct bcm4390x_qtd_s *qtd;
  uint32_t regval;
  int ret;

  /* Allocate a new Queue Element Transfer Descriptor (qTD) */

  qtd = bcm4390x_qtd_alloc();
  if (qtd == NULL)
    {
      usbhost_trace1(EHCI_TRACE1_REQQTDALLOC_FAILED, 0);
      return NULL;
    }

  /* Mark this as the end of the list (this will be overwritten if another
   * qTD is added after this one).
   */

  qtd->hw.nqp = bcm4390x_swap32(QTD_NQP_T);
  qtd->hw.alt = bcm4390x_swap32(QTD_AQP_T);

  /* Write qTD token:
   *
   * FIELD    DESCRIPTION                     VALUE/SOURCE
   * -------- ------------------------------- --------------------
   * STATUS   Status                          QTD_TOKEN_ACTIVE
   * PID      PID Code                        QTD_TOKEN_PID_SETUP
   * CERR     Error Counter                   3
   * CPAGE    Current Page                    0
   * IOC      Interrupt on complete           0
   * NBYTES   Total Bytes to Transfer         USB_SIZEOF_CTRLREQ
   * TOGGLE   Data Toggle                     0
   */

  regval = QTD_TOKEN_ACTIVE | QTD_TOKEN_PID_SETUP |
           ((uint32_t)3                  << QTD_TOKEN_CERR_SHIFT) |
           ((uint32_t)USB_SIZEOF_CTRLREQ << QTD_TOKEN_NBYTES_SHIFT);

  qtd->hw.token = bcm4390x_swap32(regval);

  /* Add the buffer data */

  ret = bcm4390x_qtd_addbpl(qtd, req, USB_SIZEOF_CTRLREQ);
  if (ret < 0)
    {
      usbhost_trace1(EHCI_TRACE1_ADDBPL_FAILED, -ret);
      bcm4390x_qtd_free(qtd);
      return NULL;
    }

  /* Add the data transfer size to the count in the epinfo structure */

  epinfo->xfrd += USB_SIZEOF_CTRLREQ;

  return qtd;
}

/*******************************************************************************
 * Name: bcm4390x_qtd_dataphase
 *
 * Description:
 *   Create a data transfer or SET data phase qTD.
 *
 *******************************************************************************/

static struct bcm4390x_qtd_s *bcm4390x_qtd_dataphase(struct bcm4390x_epinfo_s *epinfo,
                                           void *buffer, int buflen,
                                           uint32_t tokenbits)
{
  struct bcm4390x_qtd_s *qtd;
  uint32_t regval;
  int ret;

  /* Allocate a new Queue Element Transfer Descriptor (qTD) */

  qtd = bcm4390x_qtd_alloc();
  if (qtd == NULL)
    {
      usbhost_trace1(EHCI_TRACE1_DATAQTDALLOC_FAILED, 0);
      return NULL;
    }

  /* Mark this as the end of the list (this will be overwritten if another
   * qTD is added after this one).
   */

  qtd->hw.nqp = bcm4390x_swap32(QTD_NQP_T);
  qtd->hw.alt = bcm4390x_swap32(QTD_AQP_T);

  /* Write qTD token:
   *
   * FIELD    DESCRIPTION                     VALUE/SOURCE
   * -------- ------------------------------- --------------------
   * STATUS   Status                          QTD_TOKEN_ACTIVE
   * PID      PID Code                        Contained in tokenbits
   * CERR     Error Counter                   3
   * CPAGE    Current Page                    0
   * IOC      Interrupt on complete           Contained in tokenbits
   * NBYTES   Total Bytes to Transfer         buflen
   * TOGGLE   Data Toggle                     Contained in tokenbits
   */

  regval = tokenbits | QTD_TOKEN_ACTIVE |
           ((uint32_t)3       << QTD_TOKEN_CERR_SHIFT) |
           ((uint32_t)buflen  << QTD_TOKEN_NBYTES_SHIFT);

  qtd->hw.token = bcm4390x_swap32(regval);

  /* Add the buffer information to the bufffer pointer list */

  ret = bcm4390x_qtd_addbpl(qtd, buffer, buflen);
  if (ret < 0)
    {
      usbhost_trace1(EHCI_TRACE1_ADDBPL_FAILED, -ret);
      bcm4390x_qtd_free(qtd);
      return NULL;
    }

  /* Add the data transfer size to the count in the epinfo structure */

  epinfo->xfrd += buflen;

  return qtd;
}

/*******************************************************************************
 * Name: bcm4390x_qtd_statusphase
 *
 * Description:
 *   Create a STATUS phase request qTD.
 *
 *******************************************************************************/

static struct bcm4390x_qtd_s *bcm4390x_qtd_statusphase(uint32_t tokenbits)
{
  struct bcm4390x_qtd_s *qtd;
  uint32_t regval;

  /* Allocate a new Queue Element Transfer Descriptor (qTD) */

  qtd = bcm4390x_qtd_alloc();
  if (qtd == NULL)
    {
      usbhost_trace1(EHCI_TRACE1_REQQTDALLOC_FAILED, 0);
      return NULL;
    }

  /* Mark this as the end of the list (this will be overwritten if another
   * qTD is added after this one).
   */

  qtd->hw.nqp = bcm4390x_swap32(QTD_NQP_T);
  qtd->hw.alt = bcm4390x_swap32(QTD_AQP_T);

  /* Write qTD token:
   *
   * FIELD    DESCRIPTION                     VALUE/SOURCE
   * -------- ------------------------------- --------------------
   * STATUS   Status                          QTD_TOKEN_ACTIVE
   * PID      PID Code                        Contained in tokenbits
   * CERR     Error Counter                   3
   * CPAGE    Current Page                    0
   * IOC      Interrupt on complete           QTD_TOKEN_IOC
   * NBYTES   Total Bytes to Transfer         0
   * TOGGLE   Data Toggle                     Contained in tokenbits
   */

  regval = tokenbits | QTD_TOKEN_ACTIVE | QTD_TOKEN_IOC |
           ((uint32_t)3 << QTD_TOKEN_CERR_SHIFT);

  qtd->hw.token = bcm4390x_swap32(regval);
  return qtd;
}

/*******************************************************************************
 * Name: bcm4390x_async_transfer
 *
 * Description:
 *   Process a IN or OUT request on any asynchronous endpoint (bulk or control).
 *   This function will enqueue the request and wait for it to complete.  Bulk
 *   data transfers differ in that req == NULL and there are not SETUP or STATUS
 *   phases.
 *
 *   This is a blocking function; it will not return until the control transfer
 *   has completed.
 *
 * Assumption:  The caller holds the EHCI exclsem.  The caller must be aware
 *   that the EHCI exclsem will released while waiting for the transfer to
 *   complete, but will be re-aquired when before returning.  The state of
 *   EHCI resources could be very different upon return.
 *
 * Returned value:
 *   On success, this function returns the number of bytes actually transferred.
 *   For control transfers, this size includes the size of the control request
 *   plus the size of the data (which could be short); For bulk transfers, this
 *   will be the number of data bytes transfers (which could be short).
 *
 *******************************************************************************/

static ssize_t bcm4390x_async_transfer(struct bcm4390x_rhport_s *rhport,
                                  struct bcm4390x_epinfo_s *epinfo,
                                  const struct usb_ctrlreq_s *req,
                                  uint8_t *buffer, size_t buflen)
{
  struct bcm4390x_qh_s *qh;
  struct bcm4390x_qtd_s *qtd;
  uintptr_t physaddr;
  uint32_t *flink;
  uint32_t *alt;
  uint32_t toggle;
  uint32_t regval;
  bool dirin = false;
  int ret;

  /* Terse output only if we are tracing */

#ifdef CONFIG_USBHOST_TRACE
  usbhost_vtrace2(EHCI_VTRACE2_ASYNCXFR, epinfo->epno, buflen);
#else
  uvdbg("RHport%d EP%d: buffer=%p, buflen=%d, req=%p\n",
        rhport->rhpndx+1, epinfo->epno, buffer, buflen, req);
#endif

  DEBUGASSERT(rhport && epinfo);

  /* A buffer may or may be supplied with an EP0 SETUP transfer.  A buffer will
   * always be present for normal endpoint data transfers.
   */

  DEBUGASSERT(req || (buffer && buflen > 0));

  /* Set the request for the IOC event well BEFORE enabling the transfer. */

  ret = bcm4390x_ioc_setup(rhport, epinfo);
  if (ret != OK)
    {
      usbhost_trace1(EHCI_TRACE1_DEVDISCONNECTED, -ret);
      return ret;
    }

  /* Create and initialize a Queue Head (QH) structure for this transfer */

  qh = bcm4390x_qh_create(rhport, epinfo);
  if (qh == NULL)
    {
      usbhost_trace1(EHCI_TRACE1_QHCREATE_FAILED, 0);
      ret = -ENOMEM;
      goto errout_with_iocwait;
    }

  /* Initialize the QH link and get the next data toggle (not used for SETUP
   * transfers)
   */

  flink  = &qh->hw.overlay.nqp;
  toggle = (uint32_t)epinfo->toggle << QTD_TOKEN_TOGGLE_SHIFT;
  ret    = -EIO;

  /* Is the an EP0 SETUP request?  If so, req will be non-NULL and we will
   * queue two or three qTDs:
   *
   *   1) One for the SETUP phase,
   *   2) One for the DATA phase (if there is data), and
   *   3) One for the STATUS phase.
   *
   * If this is not an EP0 SETUP request, then only a data transfer will be
   * enqueued.
   */

  if (req != NULL)
    {
      /* Allocate a new Queue Element Transfer Descriptor (qTD) for the SETUP
       * phase of the request sequence.
       */

      qtd = bcm4390x_qtd_setupphase(epinfo, req);
      if (qtd == NULL)
        {
          usbhost_trace1(EHCI_TRACE1_QTDSETUP_FAILED, 0);
          goto errout_with_qh;
        }

      /* Link the new qTD to the QH head. */

      physaddr = bcm4390x_physramaddr((uintptr_t)qtd);
      *flink = bcm4390x_swap32(physaddr);

      /* Get the new forward link pointer and data toggle */

      flink  = &qtd->hw.nqp;
      toggle = QTD_TOKEN_TOGGLE;
    }

  /* A buffer may or may be supplied with an EP0 SETUP transfer.  A buffer will
   * always be present for normal endpoint data transfers.
   */

  alt = NULL;
  if (buffer != NULL && buflen > 0)
    {
      uint32_t tokenbits;

      /* Extra TOKEN bits include the data toggle, the data PID, and if
       * there is no request, an indication to interrupt at the end of this
       * transfer.
       */

      tokenbits = toggle;

      /* Get the data token direction.
       *
       * If this is a SETUP request, use the direction contained in the
       * request.  The IOC bit is not set.
       */

      if (req)
        {
          if ((req->type & USB_REQ_DIR_MASK) == USB_REQ_DIR_IN)
            {
              tokenbits |= QTD_TOKEN_PID_IN;
              dirin      = true;
            }
          else
            {
              tokenbits |= QTD_TOKEN_PID_OUT;
              dirin      = false;
            }
        }

      /* Otherwise, the endpoint is uni-directional.  Get the direction from
       * the epinfo structure.  Since this is not an EP0 SETUP request,
       * nothing follows the data and we want the IOC interrupt when the
       * data transfer completes.
       */

      else if (epinfo->dirin)
        {
          tokenbits |= (QTD_TOKEN_PID_IN | QTD_TOKEN_IOC);
          dirin      = true;
        }
      else
        {
          tokenbits |= (QTD_TOKEN_PID_OUT | QTD_TOKEN_IOC);
          dirin      = false;
        }

      /* Allocate a new Queue Element Transfer Descriptor (qTD) for the data
       * buffer.
       */

      qtd = bcm4390x_qtd_dataphase(epinfo, buffer, buflen, tokenbits);
      if (qtd == NULL)
        {
          usbhost_trace1(EHCI_TRACE1_QTDDATA_FAILED, 0);
          goto errout_with_qh;
        }

      /* Link the new qTD to either QH head of the SETUP qTD. */

      physaddr = bcm4390x_physramaddr((uintptr_t)qtd);
      *flink = bcm4390x_swap32(physaddr);

      /* Set the forward link pointer to this new qTD */

      flink = &qtd->hw.nqp;

      /* If this was an IN transfer, then setup a pointer alternate link.
       * The EHCI hardware will use this link if a short packet is received.
       */

      if (dirin)
        {
          alt = &qtd->hw.alt;
        }
    }

  /* If this is an EP0 SETUP request, then enqueue one more qTD for the
   * STATUS phase transfer.
   */

  if (req != NULL)
    {
      /* Extra TOKEN bits include the data toggle and the correct data PID. */

      uint32_t tokenbits = toggle;

      /* The status phase direction is the opposite of the data phase.  If
       * this is an IN request, then we received the buffer and we will send
       * the zero length packet handshake.
       */

      if ((req->type & USB_REQ_DIR_MASK) == USB_REQ_DIR_IN)
        {
          tokenbits |= QTD_TOKEN_PID_OUT;
        }

      /* Otherwise, this in an OUT request.  We send the buffer and we expect
       * to receive the NULL packet handshake.
       */

      else
        {
          tokenbits |= QTD_TOKEN_PID_IN;
        }

      /* Allocate a new Queue Element Transfer Descriptor (qTD) for the status */

      qtd = bcm4390x_qtd_statusphase(tokenbits);
      if (qtd == NULL)
        {
          usbhost_trace1(EHCI_TRACE1_QTDSTATUS_FAILED, 0);
          goto errout_with_qh;
        }

      /* Link the new qTD to either the SETUP or data qTD. */

      physaddr = bcm4390x_physramaddr((uintptr_t)qtd);
      *flink = bcm4390x_swap32(physaddr);

      /* In an IN data qTD was also enqueued, then linke the data qTD's
       * alternate pointer to this STATUS phase qTD in order to handle short
       * transfers.
       */

      if (alt)
        {
          *alt = bcm4390x_swap32(physaddr);
        }
    }

  /* Disable the asynchronous schedule */

  regval  = bcm4390x_getreg(&HCOR->usbcmd);
  regval &= ~EHCI_USBCMD_ASEN;
  bcm4390x_putreg(regval, &HCOR->usbcmd);

  /* Add the new QH to the head of the asynchronous queue list */

  bcm4390x_qh_enqueue(&g_asynchead, qh);

  /* Re-enable the asynchronous schedule */

  regval |= EHCI_USBCMD_ASEN;
  bcm4390x_putreg(regval, &HCOR->usbcmd);

  /* Release the EHCI semaphore while we wait.  Other threads need the
   * opportunity to access the EHCI resources while we wait.
   *
   * REVISIT:  Is this safe?  NO.  This is a bug and needs rethinking.
   * We need to lock all of the port-resources (not EHCI common) until
   * the transfer is complete.  But we can't use the common EHCI exclsem
   * or we will deadlock while waiting (because the working thread that
   * wakes this thread up needs the exclsem).
   */
//#warning REVISIT
  bcm4390x_givesem(&g_ehci.exclsem);

  /* Wait for the IOC completion event */

  ret = bcm4390x_ioc_wait(epinfo);

  /* Re-aquire the EHCI semaphore.  The caller expects to be holding
   * this upon return.
   */

  bcm4390x_takesem(&g_ehci.exclsem);

  /* Did bcm4390x_ioc_wait() report an error? */

  if (ret < 0)
    {
      usbhost_trace1(EHCI_TRACE1_TRANSFER_FAILED, -ret);
      goto errout_with_iocwait;
    }

  /* Transfer completed successfully.  Return the number of bytes
   * transferred.
   */

  return epinfo->xfrd;

  /* Clean-up after an error */

errout_with_qh:
  bcm4390x_qh_discard(qh);
errout_with_iocwait:
  epinfo->iocwait = false;
  return (ssize_t)ret;
}

/*******************************************************************************
 * Name: bcm4390x_intr_transfer
 *
 * Description:
 *   Process a IN or OUT request on any interrupt endpoint by inserting a qTD
 *   into the periodic frame list.
 *
 *  Paragraph 4.10.7 "Adding Interrupt Queue Heads to the Periodic Schedule"
 *    "The link path(s) from the periodic frame list to a queue head establishes
 *     in which frames a transaction can be executed for the queue head. Queue
 *     heads are linked into the periodic schedule so they are polled at
 *     the appropriate rate. System software sets a bit in a queue head's
 *     S-Mask to indicate which micro-frame with-in a 1 millisecond period a
 *     transaction should be executed for the queue head. Software must ensure
 *     that all queue heads in the periodic schedule have S-Mask set to a non-
 *     zero value. An S-mask with a zero value in the context of the periodic
 *     schedule yields undefined results.
 *
 *    "If the desired poll rate is greater than one frame, system software can
 *     use a combination of queue head linking and S-Mask values to spread
 *     interrupts of equal poll rates through the schedule so that the
 *     periodic bandwidth is allocated and managed in the most efficient
 *     manner possible."
 *
 *  Paragraph 4.6 "Periodic Schedule"
 *
 *    "The periodic schedule is used to manage all isochronous and interrupt
 *     transfer streams. The base of the periodic schedule is the periodic
 *     frame list. Software links schedule data structures to the periodic
 *     frame list to produce a graph of scheduled data structures. The graph
 *     represents an appropriate sequence of transactions on the USB. ...
 *     isochronous transfers (using iTDs and siTDs) with a period of one are
 *     linked directly to the periodic frame list. Interrupt transfers (are
 *     managed with queue heads) and isochronous streams with periods other
 *     than one are linked following the period-one iTD/siTDs. Interrupt
 *     queue heads are linked into the frame list ordered by poll rate.
 *     Longer poll rates are linked first (e.g. closest to the periodic
 *     frame list), followed by shorter poll rates, with queue heads with a
 *     poll rate of one, on the very end."
 *
 * Assumption:  The caller holds the EHCI exclsem.  The caller must be aware
 *   that the EHCI exclsem will released while waiting for the transfer to
 *   complete, but will be re-aquired when before returning.  The state of
 *   EHCI resources could be very different upon return.
 *
 * Returned value:
 *   On success, this function returns the number of bytes actually transferred.
 *   For control transfers, this size includes the size of the control request
 *   plus the size of the data (which could be short); For bulk transfers, this
 *   will be the number of data bytes transfers (which could be short).
 *
 *******************************************************************************/

#ifndef CONFIG_USBHOST_INT_DISABLE
static ssize_t bcm4390x_intr_transfer(struct bcm4390x_rhport_s *rhport,
                                 struct bcm4390x_epinfo_s *epinfo,
                                 uint8_t *buffer, size_t buflen)
{
  struct bcm4390x_qh_s *qh;
  struct bcm4390x_qtd_s *qtd;
  uintptr_t physaddr;
  uint32_t tokenbits;
  uint32_t regval;
  int ret;

  /* Terse output only if we are tracing */

#ifdef CONFIG_USBHOST_TRACE
  usbhost_vtrace2(EHCI_VTRACE2_INTRXFR, epinfo->epno, buflen);
#else
  uvdbg("RHport%d EP%d: buffer=%p, buflen=%d\n",
        rhport->rhpndx+1, epinfo->epno, buffer, buflen);
#endif

  DEBUGASSERT(rhport && epinfo && buffer && buflen > 0);

  /* Set the request for the IOC event well BEFORE enabling the transfer. */

  ret = bcm4390x_ioc_setup(rhport, epinfo);
  if (ret != OK)
    {
      usbhost_trace1(EHCI_TRACE1_DEVDISCONNECTED, -ret);
      return ret;
    }

  /* Create and initialize a Queue Head (QH) structure for this transfer */

  qh = bcm4390x_qh_create(rhport, epinfo);
  if (qh == NULL)
    {
      usbhost_trace1(EHCI_TRACE1_QHCREATE_FAILED, 0);
      ret = -ENOMEM;
      goto errout_with_iocwait;
    }

  /* Extra TOKEN bits include the data toggle, the data PID, and and indication to interrupt at the end of this
   * transfer.
   */

  tokenbits = (uint32_t)epinfo->toggle << QTD_TOKEN_TOGGLE_SHIFT;

  /* Get the data token direction. */

  if (epinfo->dirin)
    {
      tokenbits |= (QTD_TOKEN_PID_IN | QTD_TOKEN_IOC);
    }
  else
    {
      tokenbits |= (QTD_TOKEN_PID_OUT | QTD_TOKEN_IOC);
    }

  /* Allocate a new Queue Element Transfer Descriptor (qTD) for the data
   * buffer.
   */

  qtd = bcm4390x_qtd_dataphase(epinfo, buffer, buflen, tokenbits);
  if (qtd == NULL)
    {
      usbhost_trace1(EHCI_TRACE1_QTDDATA_FAILED, 0);
      goto errout_with_qh;
    }

  /* Link the new qTD to the QH. */

  physaddr = bcm4390x_physramaddr((uintptr_t)qtd);
  qh->hw.overlay.nqp = bcm4390x_swap32(physaddr);

  /* Disable the periodic schedule */

  regval  = bcm4390x_getreg(&HCOR->usbcmd);
  regval &= ~EHCI_USBCMD_PSEN;
  bcm4390x_putreg(regval, &HCOR->usbcmd);

  /* Add the new QH to the head of the interrupt transfer list */

  bcm4390x_qh_enqueue(&g_intrhead, qh);

  /* Re-enable the periodic schedule */

  regval |= EHCI_USBCMD_PSEN;
  bcm4390x_putreg(regval, &HCOR->usbcmd);

  /* Release the EHCI semaphore while we wait.  Other threads need the
   * opportunity to access the EHCI resources while we wait.
   *
   * REVISIT:  Is this safe?  NO.  This is a bug and needs rethinking.
   * We need to lock all of the port-resources (not EHCI common) until
   * the transfer is complete.  But we can't use the common EHCI exclsem
   * or we will deadlock while waiting (because the working thread that
   * wakes this thread up needs the exclsem).
   */
//#warning REVISIT
  bcm4390x_givesem(&g_ehci.exclsem);

  /* Wait for the IOC completion event */

  ret = bcm4390x_ioc_wait(epinfo);

  /* Re-aquire the EHCI semaphore.  The caller expects to be holding
   * this upon return.
   */

  bcm4390x_takesem(&g_ehci.exclsem);

  /* Did bcm4390x_ioc_wait() report an error? */

  if (ret < 0)
    {
      usbhost_trace1(EHCI_TRACE1_TRANSFER_FAILED, -ret);
      goto errout_with_iocwait;
    }

  /* Transfer completed successfully.  Return the number of bytes transferred */

  return epinfo->xfrd;

  /* Clean-up after an error */

errout_with_qh:
  bcm4390x_qh_discard(qh);
errout_with_iocwait:
  epinfo->iocwait = false;
  return (ssize_t)ret;
}
#endif

/*******************************************************************************
 * EHCI Interrupt Handling
 *******************************************************************************/

/*******************************************************************************
 * Name: bcm4390x_qtd_ioccheck
 *
 * Description:
 *   This function is a bcm4390x_qtd_foreach() callback function.  It services one
 *   qTD in the asynchronous queue.  It removes all of the qTD structures that
 *   are no longer active.
 *
 *******************************************************************************/

static int bcm4390x_qtd_ioccheck(struct bcm4390x_qtd_s *qtd, uint32_t **bp, void *arg)
{
  struct bcm4390x_epinfo_s *epinfo = (struct bcm4390x_epinfo_s *)arg;
  DEBUGASSERT(qtd && epinfo);

  /* Make sure we reload the QH from memory */

  arch_invalidate_dcache((uintptr_t)&qtd->hw,
                         (uintptr_t)&qtd->hw + sizeof(struct ehci_qtd_s));
  bcm4390x_qtd_print(qtd);

  /* Remove the qTD from the list
   *
   * NOTE that we don't check if the qTD is active nor do we check if there
   * are any errors reported in the qTD.  If the transfer halted due to
   * an error, then qTDs in the list after the error qTD will still appear
   * to be active.
   */

  **bp = qtd->hw.nqp;

  /* Subtract the number of bytes left untransferred.  The epinfo->xfrd
   * field is initialized to the total number of bytes to be transferred
   * (all qTDs in the list).  We subtract out the number of untransferred
   * bytes on each transfer and the final result will be the number of bytes
   * actually transferred.
   */

  epinfo->xfrd -= (bcm4390x_swap32(qtd->hw.token) & QTD_TOKEN_NBYTES_MASK) >>
    QTD_TOKEN_NBYTES_SHIFT;

  /* Release this QH by returning it to the free list */

  bcm4390x_qtd_free(qtd);
  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_qh_ioccheck
 *
 * Description:
 *   This function is a bcm4390x_qh_foreach() callback function.  It services one
 *   QH in the asynchronous queue.  It check all attached qTD structures and
 *   remove all of the structures that are no longer active.  if all of the
 *   qTD structures are removed, then QH itself will also be removed.
 *
 *******************************************************************************/

static int bcm4390x_qh_ioccheck(struct bcm4390x_qh_s *qh, uint32_t **bp, void *arg)
{
  struct bcm4390x_epinfo_s *epinfo;
  uint32_t token;
  int ret;

  DEBUGASSERT(qh && bp);

  /* Make sure we reload the QH from memory */

  arch_invalidate_dcache((uintptr_t)&qh->hw,
                         (uintptr_t)&qh->hw + sizeof(struct ehci_qh_s));
  bcm4390x_qh_print(qh);

  /* Get the endpoint info pointer from the extended QH data.  Only the
   * g_asynchead QH can have a NULL epinfo field.
   */

  epinfo = qh->epinfo;
  DEBUGASSERT(epinfo);

  /* Paragraph 3.6.3: "The nine DWords in [the Transfer Overlay] area represent
   * a transaction working space for the host controller. The general
   * operational model is that the host controller can detect whether the
   * overlay area contains a description of an active transfer. If it does
   * not contain an active transfer, then it follows the Queue Head Horizontal
   * Link Pointer to the next queue head. The host controller will never follow
   * the Next Transfer Queue Element or Alternate Queue Element pointers unless
   * it is actively attempting to advance the queue ..."
   */

  /* Is the qTD still active? */

  token = bcm4390x_swap32(qh->hw.overlay.token);
  usbhost_vtrace2(EHCI_VTRACE2_IOCCHECK, epinfo->epno, token);

  if ((token & QH_TOKEN_ACTIVE) != 0)
    {
      /* Yes... we cannot process the QH while it is still active.  Return
       * zero to visit the next QH in the list.
       */
      return OK;
    }

  /* Remove all active, attached qTD structures from the inactive QH */

  ret = bcm4390x_qtd_foreach(qh, bcm4390x_qtd_ioccheck, (void *)qh->epinfo);
  if (ret < 0)
    {
      usbhost_trace1(EHCI_TRACE1_QTDFOREACH_FAILED, -ret);
    }

  /* If there is no longer anything attached to the QH, then remove it from
   * the asynchronous queue.
   */

  if ((bcm4390x_swap32(qh->fqp) & QTD_NQP_T) != 0)
    {
      /* Set the forward link of the previous QH to point to the next
       * QH in the list.
       */

      **bp = qh->hw.hlp;
      arch_clean_dcache((uintptr_t)*bp, (uintptr_t)*bp + sizeof(uint32_t));

      /* Check for errors, update the data toggle */

      if ((token & QH_TOKEN_ERRORS) == 0)
        {
          /* No errors.. Save the last data toggle value */

          epinfo->toggle = (token >> QTD_TOKEN_TOGGLE_SHIFT) & 1;

          /* Report success */

          epinfo->status  = 0;
          epinfo->result  = OK;
        }
      else
        {
          /* An error occurred */

          epinfo->status = (token & QH_TOKEN_STATUS_MASK) >> QH_TOKEN_STATUS_SHIFT;

          /* The HALT condition is set on a variety of conditions:  babble, error
           * counter countdown to zero, or a STALL.  If we can rule out babble
           * (babble bit not set) and if the error counter is non-zero, then we can
           * assume a STALL. In this case, we return -PERM to inform the class
           * driver of the stall condition.
           */

          if ((token & (QH_TOKEN_BABBLE | QH_TOKEN_HALTED)) == QH_TOKEN_HALTED &&
              (token & QH_TOKEN_CERR_MASK) != 0)
            {
              /* It is a stall,  Note the that the data toggle is reset
               * after the stall.
               */

              usbhost_trace2(EHCI_TRACE2_EPSTALLED, epinfo->epno, token);
              epinfo->result = -EPERM;
              epinfo->toggle = 0;
            }
          else
            {
              /* Otherwise, it is some kind of data transfer error */

              usbhost_trace2(EHCI_TRACE2_EPIOERROR, epinfo->epno, token);
              epinfo->result = -EIO;
            }
        }

      /* Is there a thread waiting for this transfer to complete? */

      if (epinfo->iocwait)
        {
          /* Yes... wake it up */

          epinfo->iocwait = 0;
          bcm4390x_givesem(&epinfo->iocsem);

        }

      /* Then release this QH by returning it to the free list */

      bcm4390x_qh_free(qh);
    }
  else
    {
      /* Otherwise, the horizontal link pointer of this QH will become the next back pointer.
       */

      *bp = &qh->hw.hlp;
    }

  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_ioc_bottomhalf
 *
 * Description:
 *   EHCI USB Interrupt (USBINT) "Bottom Half" interrupt handler
 *
 *  "The Host Controller sets this bit to 1 on the  completion of a USB
 *   transaction, which results in the retirement of a Transfer Descriptor that
 *   had its IOC bit set.
 *
 *  "The Host Controller also sets this bit to 1 when a short packet is detected
 *   (actual number of bytes received was less than the expected number of
 *   bytes)."
 *
 * Assumptions:  The caller holds the EHCI exclsem
 *
 *******************************************************************************/

static inline void bcm4390x_ioc_bottomhalf(void)
{
  struct bcm4390x_qh_s *qh;
  uint32_t *bp;
  int ret;

  /* Check the Asynchronous Queue */
  /* Make sure that the head of the asynchronous queue is invalidated */

  arch_invalidate_dcache((uintptr_t)&g_asynchead.hw,
                         (uintptr_t)&g_asynchead.hw + sizeof(struct ehci_qh_s));

  /* Set the back pointer to the forward qTD pointer of the asynchronous
   * queue head.
   */

  bp = (uint32_t *)&g_asynchead.hw.hlp;

  qh = (struct bcm4390x_qh_s *)bcm4390x_virtramaddr(bcm4390x_swap32(*bp) & QH_HLP_MASK);
  if (qh)
    {
      /* Then traverse and operate on every QH and qTD in the asynchronous
       * queue
       */
      ret = bcm4390x_qh_foreach(qh, &bp, bcm4390x_qh_ioccheck, NULL);
      if (ret < 0)
        {
          usbhost_trace1(EHCI_TRACE1_QHFOREACH_FAILED, -ret);
        }
    }

#ifndef CONFIG_USBHOST_INT_DISABLE
  /* Check the Interrupt Queue */
  /* Make sure that the head of the interrupt queue is invalidated */

  arch_invalidate_dcache((uintptr_t)&g_intrhead.hw,
                         (uintptr_t)&g_intrhead.hw + sizeof(struct ehci_qh_s));

  /* Set the back pointer to the forward qTD pointer of the asynchronous
   * queue head.
   */

  bp = (uint32_t *)&g_intrhead.hw.hlp;
  qh = (struct bcm4390x_qh_s *)bcm4390x_virtramaddr(bcm4390x_swap32(*bp) & QH_HLP_MASK);
  if (qh)
    {
      /* Then traverse and operate on every QH and qTD in the asynchronous
       * queue.
       */

      ret = bcm4390x_qh_foreach(qh, &bp, bcm4390x_qh_ioccheck, NULL);
      if (ret < 0)
        {
          usbhost_trace1(EHCI_TRACE1_QHFOREACH_FAILED, -ret);
        }
    }
#endif
}

/*******************************************************************************
 * Name: bcm4390x_portsc_bottomhalf
 *
 * Description:
 *   EHCI Port Change Detect "Bottom Half" interrupt handler
 *
 *  "The Host Controller sets this bit to a one when any port for which the Port
 *   Owner bit is set to zero ... has a change bit transition from a zero to a
 *   one or a Force Port Resume bit transition from a zero to a one as a result
 *   of a J-K transition detected on a suspended port.  This bit will also be set
 *   as a result of the Connect Status Change being set to a one after system
 *   software has relinquished ownership of a connected port by writing a one
 *   to a port's Port Owner bit...
 *
 *  "This bit is allowed to be maintained in the Auxiliary power well.
 *   Alternatively, it is also acceptable that on a D3 to D0 transition of the
 *   EHCI HC device, this bit is loaded with the OR of all of the PORTSC change
 *   bits (including: Force port resume, over-current change, enable/disable
 *   change and connect status change)."
 *
 *******************************************************************************/

static inline void bcm4390x_portsc_bottomhalf(void)
{
  struct bcm4390x_rhport_s *rhport;
  uint32_t portsc;
  int rhpndx;

  /* Handle root hub status change on each root port */

  for (rhpndx = 0; rhpndx < BCM4390X_EHCI_NRHPORT; rhpndx++)
    {
      rhport = &g_ehci.rhport[rhpndx];
      portsc = bcm4390x_getreg(&HCOR->portsc[rhpndx]);

      usbhost_vtrace2(EHCI_VTRACE2_PORTSC, rhpndx + 1, portsc);

      /* Handle port connection status change (CSC) events */

      if ((portsc & EHCI_PORTSC_CSC) != 0)
        {
          usbhost_vtrace1(EHCI_VTRACE1_PORTSC_CSC, portsc);

          /* Check current connect status */

          if ((portsc & EHCI_PORTSC_CCS) != 0)
            {
              /* Connected ... Did we just become connected? */

              if (!rhport->connected)
                {
                  /* Yes.. connected. */

                  rhport->connected = true;

                  usbhost_vtrace2(EHCI_VTRACE2_PORTSC_CONNECTED,
                                  rhpndx + 1, g_ehci.pscwait);

                  /* Notify any waiters */
                  if (g_ehci.pscwait)
                    {
                      g_ehci.pscwait = false;
                      bcm4390x_givesem(&g_ehci.pscsem);

                    }
                }
              else
                {
                  usbhost_vtrace1(EHCI_VTRACE1_PORTSC_CONNALREADY, portsc);
                }
            }
          else
            {
              /* Disconnected... Did we just become disconnected? */

              if (rhport->connected)
                {
                  /* Yes.. disconnect the device */
                  udbg("%s: EHCI ISR, disconnect the device\n", __FUNCTION__);
                  usbhost_vtrace2(EHCI_VTRACE2_PORTSC_DISCONND,
                                  rhpndx+1, g_ehci.pscwait);

                  rhport->connected = false;
                  rhport->lowspeed  = false;

                  /* Are we bound to a class instance? */

                  if (rhport->class)
                    {
                      /* Yes.. Disconnect the class */

                      CLASS_DISCONNECTED(rhport->class);
                      rhport->class = NULL;
                    }

                  /* Notify any waiters for the Root Hub Status change
                   * event.
                   */

                  if (g_ehci.pscwait)
                    {

                      g_ehci.pscwait = false;
                      bcm4390x_givesem(&g_ehci.pscsem);

                    }
                }
              else
                {
                   usbhost_vtrace1(EHCI_VTRACE1_PORTSC_DISCALREADY, portsc);
                }
            }
        }

      /* Clear all pending port interrupt sources by writing a '1' to the
       * corresponding bit in the PORTSC register.  In addition, we need
       * to preserve the values of all R/W bits (RO bits don't matter)
       */

      bcm4390x_putreg(portsc, &HCOR->portsc[rhpndx]);
    }
}

/*******************************************************************************
 * Name: bcm4390x_syserr_bottomhalf
 *
 * Description:
 *   EHCI Host System Error "Bottom Half" interrupt handler
 *
 *  "The Host Controller sets this bit to 1 when a serious error occurs during a
 *   host system access involving the Host Controller module. ... When this
 *   error occurs, the Host Controller clears the Run/Stop bit in the Command
 *   register to prevent further execution of the scheduled TDs."
 *
 *******************************************************************************/

static inline void bcm4390x_syserr_bottomhalf(void)
{
  usbhost_trace1(EHCI_TRACE1_SYSERR_INTR, 0);
  PANIC();
}

/*******************************************************************************
 * Name: bcm4399x_async_advance_bottomhalf
 *
 * Description:
 *   EHCI Async Advance "Bottom Half" interrupt handler
 *
 *  "System software can force the host controller to issue an interrupt the
 *   next time the host controller advances the asynchronous schedule by writing
 *   a one to the Interrupt on Async Advance Doorbell bit in the USBCMD
 *   register. This status bit indicates the assertion of that interrupt
 *   source."
 *
 *******************************************************************************/

static inline void bcm4399x_async_advance_bottomhalf(void)
{
  usbhost_vtrace1(EHCI_VTRACE1_AAINTR, 0);

  /* REVISIT: Could remove all tagged QH entries here */
}

/*******************************************************************************
 * Name: bcm4390x_ehci_bottomhalf
 *
 * Description:
 *   EHCI "Bottom Half" interrupt handler
 *
 *******************************************************************************/

static void bcm4390x_ehci_bottomhalf(FAR void *arg)
{
  uint32_t pending = (uint32_t)arg;

  /* We need to have exclusive access to the EHCI data structures.  Waiting here
   * is not a good thing to do on the worker thread, but there is no real option
   * (other than to reschedule and delay).
   */

  bcm4390x_takesem(&g_ehci.exclsem);

  /* Handle all unmasked interrupt sources */
  /* USB Interrupt (USBINT)
   *
   *  "The Host Controller sets this bit to 1 on the completion of a USB
   *   transaction, which results in the retirement of a Transfer Descriptor
   *   that had its IOC bit set.
   *
   *  "The Host Controller also sets this bit to 1 when a short packet is
   *   detected (actual number of bytes received was less than the expected
   *   number of bytes)."
   *
   * USB Error Interrupt (USBERRINT)
   *
   *  "The Host Controller sets this bit to 1 when completion of a USB
   *   transaction results in an error condition (e.g., error counter
   *   underflow). If the TD on which the error interrupt occurred also
   *   had its IOC bit set, both this bit and USBINT bit are set. ..."
   *
   * We do the same thing in either case:  Traverse the asynchonous queue
   * and remove all of the transfers that are no longer active.
   */

  if ((pending & (EHCI_INT_USBINT | EHCI_INT_USBERRINT)) != 0)
    {
      if ((pending & EHCI_INT_USBERRINT) != 0)
        {
          usbhost_trace1(EHCI_TRACE1_USBERR_INTR, pending);
        }
      else
        {
          usbhost_vtrace1(EHCI_VTRACE1_USBINTR, pending);
        }
//      printf("%s, USB transaction done\n", __FUNCTION__);
      bcm4390x_ioc_bottomhalf();
    }

  /* Port Change Detect
   *
   *  "The Host Controller sets this bit to a one when any port for which
   *   the Port Owner bit is set to zero ... has a change bit transition
   *   from a zero to a one or a Force Port Resume bit transition from a zero
   *   to a one as a result of a J-K transition detected on a suspended port.
   *   This bit will also be set as a result of the Connect Status Change
   *   being set to a one after system software has relinquished ownership
   *    of a connected port by writing a one to a port's Port Owner bit...
   *
   *  "This bit is allowed to be maintained in the Auxiliary power well.
   *   Alternatively, it is also acceptable that on a D3 to D0 transition
   *   of the EHCI HC device, this bit is loaded with the OR of all of the
   *   PORTSC change bits (including: Force port resume, over-current change,
   *   enable/disable change and connect status change)."
   */

  if ((pending & EHCI_INT_PORTSC) != 0)
    {
      bcm4390x_portsc_bottomhalf();
    }

  /* Frame List Rollover
   *
   *  "The Host Controller sets this bit to a one when the Frame List Index ...
   *   rolls over from its maximum value to zero. The exact value at which
   *   the rollover occurs depends on the frame list size. For example, if
   *   the frame list size (as programmed in the Frame List Size field of the
   *   USBCMD register) is 1024, the Frame Index Register rolls over every
   *   time FRINDEX[13] toggles. Similarly, if the size is 512, the Host
   *   Controller sets this bit to a one every time FRINDEX[12] toggles."
   */

  /* Host System Error
   *
   *  "The Host Controller sets this bit to 1 when a serious error occurs
   *   during a host system access involving the Host Controller module. ...
   *   When this error occurs, the Host Controller clears the Run/Stop bit
   *   in the Command register to prevent further execution of the scheduled
   *   TDs."
   */

  if ((pending & EHCI_INT_SYSERROR) != 0)
    {
      bcm4390x_syserr_bottomhalf();
    }

  /* Interrupt on Async Advance
   *
   *  "System software can force the host controller to issue an interrupt
   *   the next time the host controller advances the asynchronous schedule
   *   by writing a one to the Interrupt on Async Advance Doorbell bit in
   *   the USBCMD register. This status bit indicates the assertion of that
   *   interrupt source."
   */

  if ((pending & EHCI_INT_AAINT) != 0)
    {
      bcm4399x_async_advance_bottomhalf();
    }

  /* We are done with the EHCI structures */

  bcm4390x_givesem(&g_ehci.exclsem);

  /* Re-enable relevant EHCI interrupts.  Interrupts should still be enabled
   * at the level of the AIC.
   */

  bcm4390x_putreg(EHCI_HANDLED_INTS, &HCOR->usbintr);
}

/*******************************************************************************
 * Name: bcm4390x_ehci_isr
 *
 * Description:
 *   EHCI "Top Half" interrupt handler
 *
 *******************************************************************************/

static int bcm4390x_ehci_isr(int irq, FAR void *context)
{


  uint32_t usbsts;
  uint32_t pending;
  uint32_t regval;

  /* Read Interrupt Status and mask out interrupts that are not enabled. */

  usbsts = bcm4390x_getreg(&HCOR->usbsts);
  regval = bcm4390x_getreg(&HCOR->usbintr);

#ifdef CONFIG_USBHOST_TRACE
  usbhost_vtrace1(EHCI_VTRACE1_TOPHALF, usbsts & regval);
#else
  ullvdbg("USBSTS: %08x USBINTR: %08x\n", usbsts, regval);
#endif

  /* Handle all unmasked interrupt sources */

  pending = usbsts & regval;
  if (pending != 0)
    {
      /* Schedule interrupt handling work for the high priority worker thread
       * so that we are not pressed for time and so that we can interrupt with
       * other USB threads gracefully.
       *
       * The worker should be available now because we implement a handshake
       * by controlling the EHCI interrupts.
       */
      DEBUGASSERT(work_available(&g_ehci.work));
      DEBUGVERIFY(work_queue(HPWORK, &g_ehci.work, bcm4390x_ehci_bottomhalf,
                            (FAR void *)pending, 0));

      /* Disable further EHCI interrupts so that we do not overrun the work
       * queue.
       */

      bcm4390x_putreg(0, &HCOR->usbintr);

      /* Clear all pending status bits by writing the value of the pending
       * interrupt bits back to the status register.
       */

      bcm4390x_putreg(usbsts & EHCI_INT_ALLINTS, &HCOR->usbsts);
    }

  return OK;
}


/*******************************************************************************
 * Name: bcm4390x_isr
 *
 * Description:
 *   Common UHPHS interrupt handler.  When both OHCI and EHCI are enabled, EHCI
 *   owns the interrupt and provides the interrupting event to both the OHCI and
 *   EHCI controllers.
 *
 *******************************************************************************/

static int bcm4390x_isr(int irq, FAR void *context)
{
   int ohci;
   int ehci;

   /* Provide the interrupting event to both the EHCI and OHCI top half */
   ohci = bcm4390x_ohci_isr(irq, context);
   ehci = bcm4390x_ehci_isr(irq, context);
   (void)ehci;
   /* Return OK only if both handlers returned OK */

   return ohci == OK ? ehci : ohci;
}

/*******************************************************************************
 * USB Host Controller Operations
 *******************************************************************************/
/*******************************************************************************
 * Name: bcm4390x_ehci_wait
 *
 * Description:
 *   Wait for a device to be connected or disconnected to/from a root hub port.
 *
 * Input Parameters:
 *   conn - The USB host connection instance obtained as a parameter from the call to
 *      the USB driver initialization logic.
 *   connected - A pointer to an array of 3 boolean values corresponding to
 *      root hubs 1, 2, and 3.  For each boolean value: TRUE: Wait for a device
 *      to be connected on the root hub; FALSE: wait for device to be
 *      disconnected from the root hub.
 *
 * Returned Values:
 *   And index [0, 1, or 2} corresponding to the root hub port number {1, 2,
 *   or 3} is returned when a device is connected or disconnected. This
 *   function will not return until either (1) a device is connected or
 *   disconnected to/from any root hub port or until (2) some failure occurs.
 *   On a failure, a negated errno value is returned indicating the nature of
 *   the failure
 *
 * Assumptions:
 *   - Called from a single thread so no mutual exclusion is required.
 *   - Never called from an interrupt handler.
 *
 *******************************************************************************/

static int bcm4390x_ehci_wait(FAR struct usbhost_connection_s *conn,
                    FAR const bool *connected)
{
  irqstate_t flags;
  int rhpndx;

  /* Loop until a change in the connection state changes on one of the root hub
   * ports or until an error occurs.
   */
  flags = irqsave();
  for (;;)
    {
      /* Check for a change in the connection state on any root hub port */

      for (rhpndx = 0; rhpndx < BCM4390X_EHCI_NRHPORT; rhpndx++)
        {
          /* Has the connection state changed on the RH port? */
          if (g_ehci.rhport[rhpndx].connected != connected[rhpndx])
            {
              /* Yes.. Return the RH port number to inform the caller which
               * port has the connection change.
               */

              irqrestore(flags);
              usbhost_vtrace2(EHCI_VTRACE2_MONWAKEUP,
                              rhpndx + 1, g_ehci.rhport[rhpndx].connected);
              return rhpndx;
            }
        }

      /* No changes on any port. Wait for a connection/disconnection event
       * and check again
       */

      g_ehci.pscwait = true;
      bcm4390x_takesem(&g_ehci.pscsem);
    }
}

/*******************************************************************************
 * Name: bcm4390x_ehci_enumerate
 *
 * Description:
 *   Enumerate the connected device.  As part of this enumeration process,
 *   the driver will (1) get the device's configuration descriptor, (2)
 *   extract the class ID info from the configuration descriptor, (3) call
 *   usbhost_findclass() to find the class that supports this device, (4)
 *   call the create() method on the struct usbhost_registry_s interface
 *   to get a class instance, and finally (5) call the configdesc() method
 *   of the struct usbhost_class_s interface.  After that, the class is in
 *   charge of the sequence of operations.
 *
 * Input Parameters:
 *   conn - The USB host connection instance obtained as a parameter from the call to
 *      the USB driver initialization logic.
 *   rphndx - Root hub port index.  0-(n-1) corresponds to root hub port 1-n.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   - Only a single class bound to a single device is supported.
 *   - Called from a single thread so no mutual exclusion is required.
 *   - Never called from an interrupt handler.
 *
 *******************************************************************************/

static int bcm4390x_ehci_enumerate(FAR struct usbhost_connection_s *conn, int rhpndx)
{
  struct bcm4390x_rhport_s *rhport;
  volatile uint32_t *regaddr;
  uint32_t regval;
  int ret;

  DEBUGASSERT(rhpndx >= 0 && rhpndx < BCM4390X_EHCI_NRHPORT);
  rhport = &g_ehci.rhport[rhpndx];

  /* Are we connected to a device?  The caller should have called the wait()
   * method first to be assured that a device is connected.
   */
  while (!rhport->connected)
    {
      /* No, return an error */

      dbg("port not connected\n");
      usbhost_vtrace1(EHCI_VTRACE1_ENUM_DISCONN, 0);
      return -ENODEV;
    }

  /* USB 2.0 spec says at least 50ms delay before port reset.
   * REVISIT:  I think this is wrong.  It needs to hold the port in
   * reset for 50Msec, not wait 50Msec before resetting.
   */

  usleep(100*1000);


  /* Paragraph 2.3.9:
   *
   *  "Line Status ... These bits reflect the current logical levels of the
   *   D+ (bit 11) and D- (bit 10) signal lines. These bits are used for
   *   detection of low-speed USB devices prior to the port reset and enable
   *   sequence. This field is valid only when the port enable bit is zero
   *   and the current connect status bit is set to a one."
   *
   *   Bits[11:10] USB State Interpretation
   *   ----------- --------- --------------
   *   00b         SE0       Not Low-speed device, perform EHCI reset
   *   10b         J-state   Not Low-speed device, perform EHCI reset
   *   01b         K-state   Low-speed device, release ownership of port
   */

  regval = bcm4390x_getreg(&HCOR->portsc[rhpndx]);
  if ((regval & EHCI_PORTSC_LSTATUS_MASK) == EHCI_PORTSC_LSTATUS_KSTATE)
    {
      /* Paragraph 2.3.9:
       *
       *   "Port Owner ... This bit unconditionally goes to a 0b when the
       *    Configured bit in the CONFIGFLAG register makes a 0b to 1b
       *    transition. This bit unconditionally goes to 1b whenever the
       *    Configured bit is zero.
       *
       *   "System software uses this field to release ownership of the
       *    port to a selected host controller (in the event that the
       *    attached device is not a high-speed device). Software writes
       *    a one to this bit when the attached device is not a high-speed
       *    device. A one in this bit means that a companion host
       *    controller owns and controls the port. ....
       *
       * Paragraph 4.2:
       *
       *   "When a port is routed to a companion HC, it remains under the
       *    control of the companion HC until the device is disconnected
       *    from the root por ... When a disconnect occurs, the disconnect
       *    event is detected by both the companion HC port control and the
       *    EHCI port ownership control. On the event, the port ownership
       *    is returned immediately to the EHCI controller. The companion
       *    HC stack detects the disconnect and acknowledges as it would
       *    in an ordinary standalone implementation. Subsequent connects
       *    will be detected by the EHCI port register and the process will
       *    repeat."
       */
      rhport->ep0.speed = EHCI_LOW_SPEED;
      regval |= EHCI_PORTSC_OWNER;
      bcm4390x_putreg(regval, &HCOR->portsc[rhpndx]);

      /* And return a failure */
      udbg("Set port to OCHI controller\n");
      rhport->connected = false;
      return -EPERM;
    }
  else
    {
      /* Assume full-speed for now */

      rhport->ep0.speed = EHCI_FULL_SPEED;
    }

  /* Put the root hub port in reset.
   *
   * Paragraph 2.3.9:
   *
   *  "The HCHalted bit in the USBSTS register should be a zero before
   *   software attempts to use [the Port Reset] bit. The host controller
   *   may hold Port Reset asserted to a one when the HCHalted bit is a one.
   */

  DEBUGASSERT((bcm4390x_getreg(&HCOR->usbsts) & EHCI_USBSTS_HALTED) == 0);

  /* paragraph 2.3.9:
   *
   *  "When software writes a one to [the Port Reset] bit (from a zero), the
   *   bus reset sequence as defined in the USB Specification Revision 2.0 is
   *   started.  Software writes a zero to this bit to terminate the bus reset
   *   sequence.  Software must keep this bit at a one long enough to ensure
   *   the reset sequence, as specified in the USB Specification Revision 2.0,
   *   completes. Note: when software writes this bit to a one, it must also
   *   write a zero to the Port Enable bit."
   */

  regaddr = &HCOR->portsc[rhport->rhpndx];
  regval  = bcm4390x_getreg(regaddr);
  regval &= ~EHCI_PORTSC_PE;
  regval |= EHCI_PORTSC_RESET;
  bcm4390x_putreg(regval, regaddr);

  /* USB 2.0 "Root hubs must provide an aggregate reset period of at least
   * 50 ms."
   */

  usleep(50*1000);

  regval  = bcm4390x_getreg(regaddr);
  regval &= ~EHCI_PORTSC_RESET;
  bcm4390x_putreg(regval, regaddr);

  /* Wait for the port reset to complete
   *
   * Paragraph 2.3.9:
   *
   *  "Note that when software writes a zero to this bit there may be a
   *   delay before the bit status changes to a zero. The bit status will
   *   not read as a zero until after the reset has completed. If the port
   *   is in high-speed mode after reset is complete, the host controller
   *   will automatically enable this port (e.g. set the Port Enable bit
   *   to a one). A host controller must terminate the reset and stabilize
   *   the state of the port within 2 milliseconds of software transitioning
   *   this bit from a one to a zero ..."
   */

  while ((bcm4390x_getreg(regaddr) & EHCI_PORTSC_RESET) != 0);
  usleep(200*1000);

  /* Paragraph 4.2.2:
   *
   *  "... The reset process is actually complete when software reads a zero
   *   in the PortReset bit. The EHCI Driver checks the PortEnable bit in the
   *   PORTSC register. If set to a one, the connected device is a high-speed
   *   device and EHCI Driver (root hub emulator) issues a change report to the
   *   hub driver and the hub driver continues to enumerate the attached device."
   *
   *  "At the time the EHCI Driver receives the port reset and enable request
   *   the LineStatus bits might indicate a low-speed device. Additionally,
   *   when the port reset process is complete, the PortEnable field may
   *   indicate that a full-speed device is attached. In either case the EHCI
   *   driver sets the PortOwner bit in the PORTSC register to a one to
   *   release port ownership to a companion host controller."
   */

  regval = bcm4390x_getreg(&HCOR->portsc[rhpndx]);
  if ((regval & EHCI_PORTSC_PE) != 0)
    {
      /* High speed device */
      dbg("high speed\n");
      rhport->ep0.speed = EHCI_HIGH_SPEED;
    }
  else
    {
      /* Low- or Full- speed device.  Set the port ownership bit.
       *
       * Paragraph 4.2:
       *
       *   "When a port is routed to a companion HC, it remains under the
       *    control of the companion HC until the device is disconnected
       *    from the root por ... When a disconnect occurs, the disconnect
       *    event is detected by both the companion HC port control and the
       *    EHCI port ownership control. On the event, the port ownership
       *    is returned immediately to the EHCI controller. The companion
       *    HC stack detects the disconnect and acknowledges as it would
       *    in an ordinary standalone implementation. Subsequent connects
       *    will be detected by the EHCI port register and the process will
       *    repeat."
       */

      regval |= EHCI_PORTSC_OWNER;
      bcm4390x_putreg(regval, &HCOR->portsc[rhpndx]);

      /* And return a failure */
      dbg("Set port owner ship to others\n");
      rhport->connected = false;
      return -EPERM;
    }

  /* Let the common usbhost_enumerate do all of the real work.  Note that the
   * FunctionAddress (USB address) is set to the root hub port number + 1
   * for now.
   *
   * REVISIT:  Hub support will require better device address assignment.
   * See include/nuttx/usb/usbhost_devaddr.h.
   */

  usbhost_vtrace2(EHCI_VTRACE2_CLASSENUM, rhpndx+1, rhpndx+1);

  ret = usbhost_enumerate(&g_ehci.rhport[rhpndx].drvr, rhpndx+1, &rhport->class);
  if (ret < 0)
    {
      dbg("calling usbhost_enum failed\n");
      usbhost_trace2(EHCI_TRACE2_CLASSENUM_FAILED, rhpndx+1, -ret);
    }


  return ret;
}

/************************************************************************************
 * Name: bcm4390x_ehci_ep0configure
 *
 * Description:
 *   Configure endpoint 0.  This method is normally used internally by the
 *   enumerate() method but is made available at the interface to support
 *   an external implementation of the enumeration logic.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   funcaddr - The USB address of the function containing the endpoint that EP0
 *     controls.  A funcaddr of zero will be received if no address is yet assigned
 *     to the device.
 *   maxpacketsize - The maximum number of bytes that can be sent to or
 *    received from the endpoint in a single data packet
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

static int bcm4390x_ehci_ep0configure(FAR struct usbhost_driver_s *drvr, uint8_t funcaddr,
                            uint16_t maxpacketsize)
{
  struct bcm4390x_rhport_s *rhport = (struct bcm4390x_rhport_s *)drvr;
  struct bcm4390x_epinfo_s *epinfo;

  DEBUGASSERT(rhport &&
              funcaddr >= 0 && funcaddr <= BCM4390X_EHCI_NRHPORT &&
              maxpacketsize < 2048);

  epinfo = &rhport->ep0;

  /* We must have exclusive access to the EHCI data structures. */

  bcm4390x_takesem(&g_ehci.exclsem);

  /* Remember the new device address and max packet size */

  epinfo->devaddr   = funcaddr;
  epinfo->maxpacket = maxpacketsize;

  bcm4390x_givesem(&g_ehci.exclsem);
  return OK;
}

/************************************************************************************
 * Name: bcm4390x_ehci_getdevinfo
 *
 * Description:
 *   Get information about the connected device.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   devinfo - A pointer to memory provided by the caller in which to return the
 *      device information.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

static int bcm4390x_ehci_getdevinfo(FAR struct usbhost_driver_s *drvr,
                          FAR struct usbhost_devinfo_s *devinfo)
{
  DEBUGASSERT(drvr && devinfo);

  devinfo->speed = DEVINFO_SPEED_HIGH;

  return OK;
}

/************************************************************************************
 * Name: bcm4390x_ehci_epalloc
 *
 * Description:
 *   Allocate and configure one endpoint.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   epdesc - Describes the endpoint to be allocated.
 *   ep - A memory location provided by the caller in which to receive the
 *      allocated endpoint desciptor.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

static int bcm4390x_ehci_epalloc(FAR struct usbhost_driver_s *drvr,
                       const FAR struct usbhost_epdesc_s *epdesc, usbhost_ep_t *ep)
{
  struct bcm4390x_rhport_s *rhport = (struct bcm4390x_rhport_s *)drvr;
  struct bcm4390x_epinfo_s *epinfo;

  /* Sanity check.  NOTE that this method should only be called if a device is
   * connected (because we need a valid low speed indication).
   */

  DEBUGASSERT(drvr && epdesc && ep);

  /* Terse output only if we are tracing */

#ifdef CONFIG_USBHOST_TRACE
  usbhost_vtrace2(EHCI_VTRACE2_EPALLOC, epdesc->addr, epdesc->xfrtype);
#else
  uvdbg("EP%d DIR=%s FA=%08x TYPE=%d Interval=%d MaxPacket=%d\n",
        epdesc->addr, epdesc->in ? "IN" : "OUT", epdesc->funcaddr,
        epdesc->xfrtype, epdesc->interval, epdesc->mxpacketsize);
#endif

  /* Allocate a endpoint information structure */

  epinfo = (struct bcm4390x_epinfo_s *)kmm_zalloc(sizeof(struct bcm4390x_epinfo_s));
  if (!epinfo)
    {
      usbhost_trace1(EHCI_TRACE1_EPALLOC_FAILED, 0);
      return -ENOMEM;
    }

  /* Initialize the endpoint container (which is really just another form of
   * 'struct usbhost_epdesc_s', packed differently and with additional
   * information.  A cleaner design might just embed struct usbhost_epdesc_s
   * inside of struct bcm4390x_epinfo_s and just memcpy here.
   */

  epinfo->epno      = epdesc->addr;
  epinfo->dirin     = epdesc->in;
  epinfo->devaddr   = epdesc->funcaddr;
#ifndef CONFIG_USBHOST_INT_DISABLE
  epinfo->interval  = epdesc->interval;
#endif
  epinfo->maxpacket = epdesc->mxpacketsize;
  epinfo->xfrtype   = epdesc->xfrtype;
  epinfo->speed     = rhport->ep0.speed;
  sem_init(&epinfo->iocsem, 0, 0);

  /* Success.. return an opaque reference to the endpoint information structure
   * instance
   */

  *ep = (usbhost_ep_t)epinfo;
  return OK;
}

/************************************************************************************
 * Name: bcm4390x_ehci_epfree
 *
 * Description:
 *   Free and endpoint previously allocated by DRVR_EPALLOC.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   ep - The endpint to be freed.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

static int bcm4390x_ehci_epfree(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep)
{
  struct bcm4390x_epinfo_s *epinfo = (struct bcm4390x_epinfo_s *)ep;

  /* There should not be any pending, transfers */

  DEBUGASSERT(drvr && epinfo && epinfo->iocwait == 0);

  /* Free the container */

  kmm_free(epinfo);
  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_ehci_alloc
 *
 * Description:
 *   Some hardware supports special memory in which request and descriptor data
 *   can be accessed more efficiently.  This method provides a mechanism to
 *   allocate the request/descriptor memory.  If the underlying hardware does
 *   not support such "special" memory, this functions may simply map to kmm_malloc.
 *
 *   This interface was optimized under a particular assumption.  It was
 *   assumed that the driver maintains a pool of small, pre-allocated buffers
 *   for descriptor traffic.  NOTE that size is not an input, but an output:
 *   The size of the pre-allocated buffer is returned.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call
 *      to the class create() method.
 *   buffer - The address of a memory location provided by the caller in which
 *      to return the allocated buffer memory address.
 *   maxlen - The address of a memory location provided by the caller in which
 *      to return the maximum size of the allocated buffer memory.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   - Called from a single thread so no mutual exclusion is required.
 *   - Never called from an interrupt handler.
 *
 *******************************************************************************/

static int bcm4390x_ehci_alloc(FAR struct usbhost_driver_s *drvr,
                     FAR uint8_t **buffer, FAR size_t *maxlen)
{
  int ret = -ENOMEM;
  DEBUGASSERT(drvr && buffer && maxlen);

  /* There is no special requirements for transfer/descriptor buffers. */

  *buffer = (FAR uint8_t *)kmm_malloc(CONFIG_BCM4390X_EHCI_BUFSIZE);
  if (*buffer)
    {
      *maxlen = CONFIG_BCM4390X_EHCI_BUFSIZE;
      ret = OK;
    }

  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ehci_free
 *
 * Description:
 *   Some hardware supports special memory in which request and descriptor data
 *   can be accessed more efficiently.  This method provides a mechanism to
 *   free that request/descriptor memory.  If the underlying hardware does not
 *   support such "special" memory, this functions may simply map to kmm_free().
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call
 *      to the class create() method.
 *   buffer - The address of the allocated buffer memory to be freed.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   - Never called from an interrupt handler.
 *
 *******************************************************************************/

static int bcm4390x_ehci_free(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer)
{
  DEBUGASSERT(drvr && buffer);

  /* No special action is require to free the transfer/descriptor buffer memory */

  kmm_free(buffer);
  return OK;
}

/************************************************************************************
 * Name: bcm4390x_ehci_ioalloc
 *
 * Description:
 *   Some hardware supports special memory in which larger IO buffers can
 *   be accessed more efficiently.  This method provides a mechanism to allocate
 *   the request/descriptor memory.  If the underlying hardware does not support
 *   such "special" memory, this functions may simply map to kumm_malloc.
 *
 *   This interface differs from DRVR_ALLOC in that the buffers are variable-sized.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   buffer - The address of a memory location provided by the caller in which to
 *     return the allocated buffer memory address.
 *   buflen - The size of the buffer required.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

static int bcm4390x_ehci_ioalloc(FAR struct usbhost_driver_s *drvr, FAR uint8_t **buffer,
                       size_t buflen)
{
  DEBUGASSERT(drvr && buffer && buflen > 0);

  /* The only special requirements for I/O buffers are they might need to be user
   * accessible (depending on how the class driver implements its buffering).
   */

  *buffer = (FAR uint8_t *)kumm_malloc(buflen);
  return *buffer ? OK : -ENOMEM;
}

/************************************************************************************
 * Name: bcm4390x_ehci_iofree
 *
 * Description:
 *   Some hardware supports special memory in which IO data can  be accessed more
 *   efficiently.  This method provides a mechanism to free that IO buffer
 *   memory.  If the underlying hardware does not support such "special" memory,
 *   this functions may simply map to kumm_free().
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   buffer - The address of the allocated buffer memory to be freed.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

static int bcm4390x_ehci_iofree(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer)
{
  DEBUGASSERT(drvr && buffer);

  /* No special action is require to free the I/O buffer memory */

  kumm_free(buffer);
  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_ehci_ctrlin and bcm4390x_ehci_ctrlout
 *
 * Description:
 *   Process a IN or OUT request on the control endpoint.  These methods
 *   will enqueue the request and wait for it to complete.  Only one transfer may
 *   be queued; Neither these methods nor the transfer() method can be called
 *   again until the control transfer functions returns.
 *
 *   These are blocking methods; these functions will not return until the
 *   control transfer has completed.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   req - Describes the request to be sent.  This request must lie in memory
 *      created by DRVR_ALLOC.
 *   buffer - A buffer used for sending the request and for returning any
 *     responses.  This buffer must be large enough to hold the length value
 *     in the request description. buffer must have been allocated using
 *     DRVR_ALLOC
 *
 *   NOTE: On an IN transaction, req and buffer may refer to the same allocated
 *   memory.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   - Only a single class bound to a single device is supported.
 *   - Called from a single thread so no mutual exclusion is required.
 *   - Never called from an interrupt handler.
 *
 *******************************************************************************/

static int bcm4390x_ehci_ctrlin(FAR struct usbhost_driver_s *drvr,
                      FAR const struct usb_ctrlreq_s *req,
                      FAR uint8_t *buffer)
{
  struct bcm4390x_rhport_s *rhport = (struct bcm4390x_rhport_s *)drvr;
  uint16_t len;
  ssize_t nbytes;

  DEBUGASSERT(rhport && req);

  len = bcm4390x_read16(req->len);

  /* Terse output only if we are tracing */

#ifdef CONFIG_USBHOST_TRACE
  usbhost_vtrace2(EHCI_VTRACE2_CTRLINOUT, rhport->rhpndx + 1, req->req);
#else
//  dbg("RHPort%d type: %02x req: %02x value: %02x%02x index: %02x%02x len: %04x\n",
//        rhport->rhpndx + 1, req->type, req->req, req->value[1], req->value[0],
//        req->index[1], req->index[0], len);
#endif

  /* We must have exclusive access to the EHCI hardware and data structures. */

  bcm4390x_takesem(&g_ehci.exclsem);

  /* Now perform the transfer */

  nbytes = bcm4390x_async_transfer(rhport, &rhport->ep0, req, buffer, len);
  bcm4390x_givesem(&g_ehci.exclsem);
  return nbytes >=0 ? OK : (int)nbytes;
}

static int bcm4390x_ehci_ctrlout(FAR struct usbhost_driver_s *drvr,
                       FAR const struct usb_ctrlreq_s *req,
                       FAR const uint8_t *buffer)
{
  /* bcm4390x_ehci_ctrlin can handle both directions.  We just need to work around the
   * differences in the function signatures.
   */

  return bcm4390x_ehci_ctrlin(drvr, req, (uint8_t *)buffer);
}

/*******************************************************************************
 * Name: bcm4390x_ehci_transfer
 *
 * Description:
 *   Process a request to handle a transfer descriptor.  This method will
 *   enqueue the transfer request and return immediately.  Only one transfer may be
 *   queued;.
 *
 *   This is a blocking method; this functions will not return until the
 *   transfer has completed.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   ep - The IN or OUT endpoint descriptor for the device endpoint on which to
 *      perform the transfer.
 *   buffer - A buffer containing the data to be sent (OUT endpoint) or received
 *     (IN endpoint).  buffer must have been allocated using DRVR_ALLOC
 *   buflen - The length of the data to be sent or received.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure:
 *
 *     EAGAIN - If devices NAKs the transfer (or NYET or other error where
 *              it may be appropriate to restart the entire transaction).
 *     EPERM  - If the endpoint stalls
 *     EIO    - On a TX or data toggle error
 *     EPIPE  - Overrun errors
 *
 * Assumptions:
 *   - Only a single class bound to a single device is supported.
 *   - Called from a single thread so no mutual exclusion is required.
 *   - Never called from an interrupt handler.
 *
 *******************************************************************************/

static int bcm4390x_ehci_transfer(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep,
                        FAR uint8_t *buffer, size_t buflen)
{
  struct bcm4390x_rhport_s *rhport = (struct bcm4390x_rhport_s *)drvr;
  struct bcm4390x_epinfo_s *epinfo = (struct bcm4390x_epinfo_s *)ep;
  ssize_t nbytes;

  DEBUGASSERT(rhport && epinfo && buffer && buflen > 0);

  /* We must have exclusive access to the EHCI hardware and data structures. */

  bcm4390x_takesem(&g_ehci.exclsem);

  /* Perform the transfer */

  switch (epinfo->xfrtype)
    {
      case USB_EP_ATTR_XFER_BULK:
        nbytes = bcm4390x_async_transfer(rhport, epinfo, NULL, buffer, buflen);
        break;

#ifndef CONFIG_USBHOST_INT_DISABLE
      case USB_EP_ATTR_XFER_INT:
        nbytes = bcm4390x_intr_transfer(rhport, epinfo, buffer, buflen);
        break;
#endif

#ifndef CONFIG_USBHOST_ISOC_DISABLE
      case USB_EP_ATTR_XFER_ISOC:
# warning "Isochronous endpoint support not emplemented"
#endif
      case USB_EP_ATTR_XFER_CONTROL:
      default:
        usbhost_trace1(EHCI_TRACE1_BADXFRTYPE, epinfo->xfrtype);
        nbytes = -ENOSYS;
        break;
    }

  bcm4390x_givesem(&g_ehci.exclsem);
  return nbytes >=0 ? OK : (int)nbytes;
}

/*******************************************************************************
 * Name: bcm4390x_ehci_disconnect
 *
 * Description:
 *   Called by the class when an error occurs and driver has been disconnected.
 *   The USB host driver should discard the handle to the class instance (it is
 *   stale) and not attempt any further interaction with the class driver instance
 *   (until a new instance is received from the create() method).  The driver
 *   should not called the class' disconnected() method.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *
 * Returned Values:
 *   None
 *
 * Assumptions:
 *   - Only a single class bound to a single device is supported.
 *   - Never called from an interrupt handler.
 *
 *******************************************************************************/

static void bcm4390x_ehci_disconnect(FAR struct usbhost_driver_s *drvr)
{
  struct bcm4390x_rhport_s *rhport = (struct bcm4390x_rhport_s *)drvr;
  DEBUGASSERT(rhport);

  /* Unbind the class */
  /* REVISIT:  Is there more that needs to be done? */

  rhport->class = NULL;
}

/*******************************************************************************
 * Initialization
 *******************************************************************************/
/*******************************************************************************
 * Name: bcm4390x_ehci_reset
 *
 * Description:
 *   Set the HCRESET bit in the USBCMD register to reset the EHCI hardware.
 *
 *   Table 2-9. USBCMD 嚙磊SB Command Register Bit Definitions
 *
 *    "Host Controller Reset (HCRESET) ... This control bit is used by software
 *     to reset the host controller. The effects of this on Root Hub registers
 *     are similar to a Chip Hardware Reset.
 *
 *    "When software writes a one to this bit, the Host Controller resets its
 *     internal pipelines, timers, counters, state machines, etc. to their
 *     initial value. Any transaction currently in progress on USB is
 *     immediately terminated. A USB reset is not driven on downstream
 *     ports.
 *
 *    "PCI Configuration registers are not affected by this reset. All
 *     operational registers, including port registers and port state machines
 *     are set to their initial values. Port ownership reverts to the companion
 *     host controller(s)... Software must reinitialize the host controller ...
 *     in order to return the host controller to an operational state.
 *
 *    "This bit is set to zero by the Host Controller when the reset process is
 *     complete. Software cannot terminate the reset process early by writing a
 *     zero to this register. Software should not set this bit to a one when
 *     the HCHalted bit in the USBSTS register is a zero. Attempting to reset
 *     an actively running host controller will result in undefined behavior."
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; A negated errno value is returned
 *   on failure.
 *
 * Assumptions:
 * - Called during the initializaation of the EHCI.
 *
 *******************************************************************************/

static int bcm4390x_ehci_reset(void)
{
  uint32_t regval;
  unsigned int timeout;

  /* Make sure that the EHCI is halted:  "When [the Run/Stop] bit is set to 0,
   * the Host Controller completes the current transaction on the USB and then
   * halts. The HC Halted bit in the status register indicates when the Hos
   * Controller has finished the transaction and has entered the stopped state..."
   */

  bcm4390x_putreg(0, &HCOR->usbcmd);

  /* "... Software should not set [HCRESET] to a one when the HCHalted bit in
   *  the USBSTS register is a zero. Attempting to reset an actively running
   *   host controller will result in undefined behavior."
   */

  timeout = 0;
  do
    {
      /* Wait one microsecond and update the timeout counter */

      up_mdelay(1);
      timeout++;

      /* Get the current value of the USBSTS register.  This loop will terminate
       * when either the timeout exceeds one millisecond or when the HCHalted
       * bit is no longer set in the USBSTS register.
       */

      regval = bcm4390x_getreg(&HCOR->usbsts);
    }
  while (((regval & EHCI_USBSTS_HALTED) == 0) && (timeout < 1000));

  /* Is the EHCI still running?  Did we timeout? */

  if ((regval & EHCI_USBSTS_HALTED) == 0)
    {
      usbhost_trace1(EHCI_TRACE1_HCHALTED_TIMEOUT, regval);
      return -ETIMEDOUT;
    }

  /* Now we can set the HCReset bit in the USBCMD register to initiate the reset */

  regval  = bcm4390x_getreg(&HCOR->usbcmd);
  regval |= EHCI_USBCMD_HCRESET;
  bcm4390x_putreg(regval, &HCOR->usbcmd);

  /* Wait for the HCReset bit to become clear */

  do
    {
      /* Wait five microsecondw and update the timeout counter */

      up_mdelay(5);
      timeout += 5;

      /* Get the current value of the USBCMD register.  This loop will terminate
       * when either the timeout exceeds one second or when the HCReset
       * bit is no longer set in the USBSTS register.
       */

      regval = bcm4390x_getreg(&HCOR->usbcmd);
    }
  while (((regval & EHCI_USBCMD_HCRESET) != 0) && (timeout < 1000000));

  /* Return either success or a timeout */

  return (regval & EHCI_USBCMD_HCRESET) != 0 ? -ETIMEDOUT : OK;
}

/*******************************************************************************
 * Global Functions
 *******************************************************************************/
/*******************************************************************************
 * Name: bcm4390x_ehci_initialize
 *
 * Description:
 *   Initialize USB EHCI host controller hardware.
 *
 * Input Parameters:
 *   controller -- If the device supports more than one EHCI interface, then
 *     this identifies which controller is being initialized.  Normally, this
 *     is just zero.
 *
 * Returned Value:
 *   And instance of the USB host interface.  The controlling task should
 *   use this interface to (1) call the wait() method to wait for a device
 *   to be connected, and (2) call the enumerate() method to bind the device
 *   to a class driver.
 *
 * Assumptions:
 * - This function should called in the initialization sequence in order
 *   to initialize the USB device functionality.
 * - Class drivers should be initialized prior to calling this function.
 *   Otherwise, there is a race condition if the device is already connected.
 *
 *******************************************************************************/

FAR struct usbhost_connection_s *bcm4390x_ehci_initialize( void )
{
  uint32_t regval;
#if defined(CONFIG_DEBUG_USB) && defined(CONFIG_DEBUG_VERBOSE)
  uint16_t regval16;
  unsigned int nports;
#endif
  uintptr_t physaddr;
  int ret;
  int i;

  /* Sanity checks */
  DEBUGASSERT(((uintptr_t)&g_asynchead & 0x1f) == 0);
  DEBUGASSERT((sizeof(struct bcm4390x_qh_s) & 0x1f) == 0);
  DEBUGASSERT((sizeof(struct bcm4390x_qtd_s) & 0x1f) == 0);

#ifdef CONFIG_BCM4390X_EHCI_PREALLOCATE
  DEBUGASSERT(((uintptr_t)&g_qhpool & 0x1f) == 0);
  DEBUGASSERT(((uintptr_t)&g_qtdpool & 0x1f) == 0);
#endif

#ifndef CONFIG_USBHOST_INT_DISABLE
  DEBUGASSERT(((uintptr_t)&g_intrhead & 0x1f) == 0);
#ifdef CONFIG_BCM4390X_EHCI_PREALLOCATE
  DEBUGASSERT(((uintptr_t)g_framelist & 0xfff) == 0);
#endif
#endif /* CONFIG_USBHOST_INT_DISABLE */


  /* Note that no pin configuration is required.  All USB HS pins have
   * dedicated function
   */

  /* Software Configuration ****************************************************/

  usbhost_vtrace1(EHCI_VTRACE1_INITIALIZING, 0);

  /* Initialize the EHCI state data structure */

  sem_init(&g_ehci.exclsem, 0, 1);
  sem_init(&g_ehci.pscsem,  0, 0);

  /* Initialize EP0 */

  sem_init(&g_ehci.ep0.iocsem, 0, 1);

  /* Initialize the root hub port structures */

  for (i = 0; i < BCM4390X_EHCI_NRHPORT; i++)
    {
      struct bcm4390x_rhport_s *rhport = &g_ehci.rhport[i];
      rhport->rhpndx              = i;

      /* Initialize the device operations */

      rhport->drvr.ep0configure   = bcm4390x_ehci_ep0configure;
      rhport->drvr.getdevinfo     = bcm4390x_ehci_getdevinfo;
      rhport->drvr.epalloc        = bcm4390x_ehci_epalloc;
      rhport->drvr.epfree         = bcm4390x_ehci_epfree;
      rhport->drvr.alloc          = bcm4390x_ehci_alloc;
      rhport->drvr.free           = bcm4390x_ehci_free;
      rhport->drvr.ioalloc        = bcm4390x_ehci_ioalloc;
      rhport->drvr.iofree         = bcm4390x_ehci_iofree;
      rhport->drvr.ctrlin         = bcm4390x_ehci_ctrlin;
      rhport->drvr.ctrlout        = bcm4390x_ehci_ctrlout;
      rhport->drvr.transfer       = bcm4390x_ehci_transfer;
      rhport->drvr.disconnect     = bcm4390x_ehci_disconnect;

      /* Initialize EP0 */

      rhport->ep0.xfrtype         = USB_EP_ATTR_XFER_CONTROL;
      rhport->ep0.speed           = EHCI_FULL_SPEED;
      rhport->ep0.maxpacket       = 8;
      sem_init(&rhport->ep0.iocsem, 0, 0);
    }

#ifndef CONFIG_BCM4390X_EHCI_PREALLOCATE
  /* Allocate a pool of free Queue Head (QH) structures */

  g_qhpool = (struct bcm4390x_qh_s *)
        kmm_memalign(32, CONFIG_BCM4390X_EHCI_NQHS * sizeof(struct bcm4390x_qh_s));
  if (!g_qhpool)
    {
      usbhost_trace1(EHCI_TRACE1_QHPOOLALLOC_FAILED, 0);
      dbg("%s: EHCI QHPOOL  alloc failed\n", __FUNCTION__);
      return NULL;
    }
#endif

  /* Initialize the list of free Queue Head (QH) structures */

  for (i = 0; i < CONFIG_BCM4390X_EHCI_NQHS; i++)
    {
      /* Put the QH structure in a free list */

      bcm4390x_qh_free(&g_qhpool[i]);
    }

#ifndef CONFIG_BCM4390X_EHCI_PREALLOCATE
  /* Allocate a pool of free  Transfer Descriptor (qTD) structures */

  g_qtdpool = (struct bcm4390x_qtd_s *)
        kmm_memalign(32, CONFIG_BCM4390X_EHCI_NQTDS * sizeof(struct bcm4390x_qtd_s));
  if (!g_qtdpool)
    {
      usbhost_trace1(EHCI_TRACE1_QTDPOOLALLOC_FAILED, 0);
      kmm_free(g_qhpool);
      dbg("%s: EHCI QTD  alloc failed\n", __FUNCTION__);

      return NULL;
    }
#endif

#if !defined(CONFIG_BCM4390X_EHCI_PREALLOCATE) && !defined(CONFIG_USBHOST_INT_DISABLE)
  /* Allocate the periodic framelist  */

  g_framelist = (uint32_t *)
        kmm_memalign(4096, FRAME_LIST_SIZE * sizeof(uint32_t));
  if (!g_framelist)
    {
      usbhost_trace1(EHCI_TRACE1_PERFLALLOC_FAILED, 0);
      kmm_free(g_qhpool);
      kmm_free(g_qtdpool);
      dbg("%s: EHCI framelist alloc failed\n", __FUNCTION__);
      return NULL;
    }
#endif

  /* Initialize the list of free Transfer Descriptor (qTD) structures */

  for (i = 0; i < CONFIG_BCM4390X_EHCI_NQTDS; i++)
    {
      /* Put the TD in a free list */

      bcm4390x_qtd_free(&g_qtdpool[i]);
    }

  /* EHCI Hardware Configuration ***********************************************/
  /* Host Controller Initialization. Paragraph 4.1 */
  /* Reset the EHCI hardware */

  ret = bcm4390x_ehci_reset();
  if (ret < 0)
    {
      dbg("%s: Reset ehci failed\n", __FUNCTION__);
      usbhost_trace1(EHCI_TRACE1_RESET_FAILED, -ret);
      return NULL;
    }

  /* "In order to initialize the host controller, software should perform the
   *  following steps:
   *
   *  嚙�"Program the CTRLDSSEGMENT register with 4-Gigabyte segment where all
   *     of the interface data structures are allocated. [64-bit mode]
   *  嚙�"Write the appropriate value to the USBINTR register to enable the
   *     appropriate interrupts.
   *  嚙�"Write the base address of the Periodic Frame List to the PERIODICLIST
   *     BASE register. If there are no work items in the periodic schedule,
   *     all elements of the Periodic Frame List should have their T-Bits set
   *     to a one.
   *  嚙�"Write the USBCMD register to set the desired interrupt threshold,
   *     frame list size (if applicable) and turn the host controller ON via
   *     setting the Run/Stop bit.
   *  嚙� Write a 1 to CONFIGFLAG register to route all ports to the EHCI controller
   *     ...
   *
   * "At this point, the host controller is up and running and the port registers
   *  will begin reporting device connects, etc. System software can enumerate a
   *  port through the reset process (where the port is in the enabled state). At
   *  this point, the port is active with SOFs occurring down the enabled por
   *  enabled Highspeed ports, but the schedules have not yet been enabled. The
   *  EHCI Host controller will not transmit SOFs to enabled Full- or Low-speed
   *  ports.
   */

  /* Disable all interrupts */

  bcm4390x_putreg(0, &HCOR->usbintr);

  /* Clear pending interrupts.  Bits in the USBSTS register are cleared by
   * writing a '1' to the corresponding bit.
   */

  bcm4390x_putreg(EHCI_INT_ALLINTS, &HCOR->usbsts);

#if defined(CONFIG_DEBUG_USB) && defined(CONFIG_DEBUG_VERBOSE)
  /* Show the EHCI version */

  regval16 = bcm4390x_swap16(HCCR->hciversion);
  usbhost_vtrace2(EHCI_VTRACE2_HCIVERSION, regval16 >> 8, regval16 & 0xff);

  /* Verify that the correct number of ports is reported */

  regval = bcm4390x_getreg(&HCCR->hcsparams);
  nports = (regval & EHCI_HCSPARAMS_NPORTS_MASK) >> EHCI_HCSPARAMS_NPORTS_SHIFT;

  usbhost_vtrace2(EHCI_VTRACE2_HCSPARAMS, nports, regval);
  DEBUGASSERT(nports == BCM4390X_EHCI_NRHPORT);

  /* Show the HCCPARAMS register */

  regval = bcm4390x_getreg(&HCCR->hccparams);
  usbhost_vtrace1(EHCI_VTRACE1_HCCPARAMS, regval);
#endif

//Enable Port power of port #0

  regval = bcm4390x_getreg(&HCCR->hcsparams);

  if (regval & EHCI_HC_RH_PPC) {
      regval = bcm4390x_getreg(&HCOR->portsc[0]);
      bcm4390x_putreg(regval | EHCI_HC_PS_PP, &HCOR->portsc[0]);

  }
  /* Initialize the head of the asynchronous queue/reclamation list.
   *
   * "In order to communicate with devices via the asynchronous schedule,
   *  system software must write the ASYNDLISTADDR register with the address
   *  of a control or bulk queue head. Software must then enable the
   *  asynchronous schedule by writing a one to the Asynchronous Schedule
   *  Enable bit in the USBCMD register. In order to communicate with devices
   *  via the periodic schedule, system software must enable the periodic
   *  schedule by writing a one to the Periodic Schedule Enable bit in the
   *  USBCMD register. Note that the schedules can be turned on before the
   *  first port is reset (and enabled)."
   */

  memset(&g_asynchead, 0, sizeof(struct bcm4390x_qh_s));
  physaddr                     = bcm4390x_physramaddr((uintptr_t)&g_asynchead);
  g_asynchead.hw.hlp           = bcm4390x_swap32(physaddr | QH_HLP_TYP_QH);
  g_asynchead.hw.epchar        = bcm4390x_swap32(QH_EPCHAR_H | QH_EPCHAR_EPS_FULL);
  g_asynchead.hw.overlay.nqp   = bcm4390x_swap32(QH_NQP_T);
  g_asynchead.hw.overlay.alt   = bcm4390x_swap32(QH_NQP_T);
  g_asynchead.hw.overlay.token = bcm4390x_swap32(QH_TOKEN_HALTED);
  g_asynchead.fqp              = bcm4390x_swap32(QTD_NQP_T);

  arch_clean_dcache((uintptr_t)&g_asynchead.hw,
                    (uintptr_t)&g_asynchead.hw + sizeof(struct ehci_qh_s));

  /* Set the Current Asynchronous List Address. */

  bcm4390x_putreg(bcm4390x_swap32(physaddr), &HCOR->asynclistaddr);

#ifndef CONFIG_USBHOST_INT_DISABLE
  /* Initialize the head of the periodic list.  Since Isochronous
   * endpoints are not not yet supported, each element of the
   * frame list is initialized to point to the Interrupt Queue
   * Head (g_intrhead).
   */

  memset(&g_intrhead, 0, sizeof(struct bcm4390x_qh_s));
  g_intrhead.hw.hlp           = bcm4390x_swap32(QH_HLP_T);
  g_intrhead.hw.overlay.nqp   = bcm4390x_swap32(QH_NQP_T);
  g_intrhead.hw.overlay.alt   = bcm4390x_swap32(QH_NQP_T);
  g_intrhead.hw.overlay.token = bcm4390x_swap32(QH_TOKEN_HALTED);
  g_intrhead.hw.epcaps        = bcm4390x_swap32(QH_EPCAPS_SSMASK(1));

  /* Attach the periodic QH to Period Frame List */

  physaddr = bcm4390x_physramaddr((uintptr_t)&g_intrhead);
  for (i = 0; i < FRAME_LIST_SIZE; i++)
    {
      g_framelist[i] = bcm4390x_swap32(physaddr) | PFL_TYP_QH;
    }

  /* Set the Periodic Frame List Base Address. */

  arch_clean_dcache((uintptr_t)&g_intrhead.hw,
                    (uintptr_t)&g_intrhead.hw + sizeof(struct ehci_qh_s));
  arch_clean_dcache((uintptr_t)g_framelist,
                    (uintptr_t)g_framelist + FRAME_LIST_SIZE * sizeof(uint32_t));

  physaddr = bcm4390x_physramaddr((uintptr_t)g_framelist);
  bcm4390x_putreg(bcm4390x_swap32(physaddr), &HCOR->periodiclistbase);
#endif

  /* Enable the asynchronous schedule and, possibly enable the periodic
   * schedule and set the frame list size.
   */

  regval  = bcm4390x_getreg(&HCOR->usbcmd);
  regval &= ~(EHCI_USBCMD_HCRESET | EHCI_USBCMD_FLSIZE_MASK |
              EHCI_USBCMD_FLSIZE_MASK | EHCI_USBCMD_PSEN |
              EHCI_USBCMD_IAADB | EHCI_USBCMD_LRESET);
  regval |= EHCI_USBCMD_ASEN;

#ifndef CONFIG_USBHOST_INT_DISABLE
  regval |= EHCI_USBCMD_PSEN;
#  if FRAME_LIST_SIZE == 1024
  regval |= EHCI_USBCMD_FLSIZE_1024;
#  elif FRAME_LIST_SIZE == 512
  regval |= EHCI_USBCMD_FLSIZE_512;
#  elif FRAME_LIST_SIZE == 512
  regval |= EHCI_USBCMD_FLSIZE_256;
#  else
#    error Unsupported frame size list size
#  endif
#endif

  bcm4390x_putreg(regval, &HCOR->usbcmd);


  /* Route all ports to this host controller by setting the CONFIG flag. */

    regval  = bcm4390x_getreg(&HCOR->configflag);
    regval |= EHCI_CONFIGFLAG;
    bcm4390x_putreg(regval, &HCOR->configflag);

  /* Start the host controller by setting the RUN bit in the USBCMD regsiter. */

  regval  = bcm4390x_getreg(&HCOR->usbcmd);
  regval |= EHCI_USBCMD_RUN;
  bcm4390x_putreg(regval, &HCOR->usbcmd);

  /* Wait for the EHCI to run (i.e., no longer report halted) */

  ret = ehci_wait_usbsts(EHCI_USBSTS_HALTED, 0, 100*1000);
  if (ret < 0)
    {
      dbg("%s: EHCI Wait usb status failed\n", __FUNCTION__);
      usbhost_trace1(EHCI_TRACE1_RUN_FAILED, bcm4390x_getreg(&HCOR->usbsts));
      return NULL;
    }

  /* Interrupt Configuration ***************************************************/
  /* Attach USB host controller interrupt handler.  If OHCI is also enabled,
   * then we have to use a common UHPHS interrupt handler.
   */

  platform_irq_remap_sink( Core11_ExtIRQn, USB_REMAPPED_ExtIRQn );
  ret = irq_attach(USB_REMAPPED_ExtIRQn, bcm4390x_isr);
  if (ret != 0)
    {
      dbg("%s: IRQ attach failed\n", __FUNCTION__);
      return NULL;
    }

  /* Enable EHCI interrupts.  Interrupts are still disabled at the level of
   * the AIC.
   */

  bcm4390x_putreg(EHCI_HANDLED_INTS, &HCOR->usbintr);


  up_mdelay(50);

  /* If there is a USB device in the slot at power up, then we will not
   * get the status change interrupt to signal us that the device is
   * connected.  We need to set the initial connected state accordingly.
   */

  for (i = 0; i < BCM4390X_EHCI_NRHPORT; i++)
    {
      g_ehci.rhport[i].connected =
        ((bcm4390x_getreg(&HCOR->portsc[i]) & EHCI_PORTSC_CCS) != 0);
      if (g_ehci.rhport[i].connected)
          udbg("port: %d connected!\n", i);
    }

  /* Enable interrupts at the interrupt controller */

  up_enable_irq(USB_REMAPPED_ExtIRQn); /* enable USB interrupt */

  usbhost_vtrace1(EHCI_VTRACE1_INIITIALIZED, 0);

  /* Initialize and return the connection interface */

  g_ehciconn.wait      = bcm4390x_ehci_wait;
  g_ehciconn.enumerate = bcm4390x_ehci_enumerate;

  return &g_ehciconn;
}

#endif /* CONFIG_BCM4390X_EHCI */
