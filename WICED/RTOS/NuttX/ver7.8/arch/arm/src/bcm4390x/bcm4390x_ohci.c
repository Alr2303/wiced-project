/*
 * $ Copyright Broadcom Corporation $
 */

/*******************************************************************************
 * arch/arm/src/bcm4390x/bcm4390x_ohci.c
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

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/kmalloc.h>
#include <nuttx/wqueue.h>
#include <nuttx/usb/usb.h>
#include <nuttx/usb/ohci.h>
#include <nuttx/usb/usbhost.h>
#include <nuttx/usb/usbhost_trace.h>

#include <arch/irq.h>

#include <arch/board/board.h> /* May redefine PIO settings */

#include "up_arch.h"
#include "up_internal.h"

#include "chip.h"
#include "bcm4390x_usbhost.h"
#include "bcm4390x_ohci.h"
#include "platform_appscr4.h"
#include "platform_cache.h"
#include "platform_peripheral.h"


#define arch_clean_dcache(x, y) platform_dcache_clean_range((const volatile void *)x, y - x)
#define arch_invalidate_dcache(x, y) platform_dcache_inv_range(( volatile void *)x, y - x)


#ifdef CONFIG_BCM4390X_OHCI

/*******************************************************************************
 * Definitions
 *******************************************************************************/
/* Configuration ***************************************************************/
/* Pre-requisites */

#ifndef CONFIG_SCHED_WORKQUEUE
#  error Work queue support is required (CONFIG_SCHED_WORKQUEUE)
#endif

/* Configurable number of user endpoint descriptors (EDs).  This number excludes
 * the control endpoint that is always allocated.
 */

#ifndef CONFIG_BCM4390X_OHCI_NEDS
#  define CONFIG_BCM4390X_OHCI_NEDS 2
#endif

/* Configurable number of user transfer descriptors (TDs).  */

#ifndef CONFIG_BCM4390X_OHCI_NTDS
#  define CONFIG_BCM4390X_OHCI_NTDS 3
#endif

#if CONFIG_BCM4390X_OHCI_NTDS < 2
#  error Insufficent number of transfer descriptors (CONFIG_BCM4390X_OHCI_NTDS < 2)
#endif

/* Configurable number of request/descriptor buffers (TDBUFFER) */

#ifndef CONFIG_BCM4390X_OHCI_TDBUFFERS
#  define CONFIG_BCM4390X_OHCI_TDBUFFERS 2
#endif

#if CONFIG_BCM4390X_OHCI_TDBUFFERS < 2
#  error At least two TD buffers are required (CONFIG_BCM4390X_OHCI_TDBUFFERS < 2)
#endif

/* Configurable size of one TD buffer */

#if CONFIG_BCM4390X_OHCI_TDBUFFERS > 0 && !defined(CONFIG_BCM4390X_OHCI_TDBUFSIZE)
#  define CONFIG_BCM4390X_OHCI_TDBUFSIZE 128
#endif

#if (CONFIG_BCM4390X_OHCI_TDBUFSIZE & 3) != 0
#  error "TD buffer size must be an even number of 32-bit words"
#endif

/* Total buffer size */

#define BCM4390X_BUFALLOC (CONFIG_BCM4390X_OHCI_TDBUFFERS * CONFIG_BCM4390X_OHCI_TDBUFSIZE)

/* Debug */

#ifndef CONFIG_DEBUG
#  undef CONFIG_BCM4390X_OHCI_REGDEBUG
#endif

/* OHCI Setup ******************************************************************/
/* Frame Interval / Periodic Start.
 *
 * At 12Mbps, there are 12000 bit time in each 1Msec frame.
 */

#define BITS_PER_FRAME          12000
#define FI                     (BITS_PER_FRAME-1)
#define FSMPS                  ((6 * (FI - 210)) / 7)
#define DEFAULT_FMINTERVAL     ((FSMPS << OHCI_FMINT_FSMPS_SHIFT) | FI)
#define DEFAULT_PERSTART       (((9 * BITS_PER_FRAME) / 10) - 1)


/* Interrupt enable bits */

#ifdef CONFIG_DEBUG_USB
#  define BCM4390X_DEBUG_INTS      (OHCI_INT_SO|OHCI_INT_RD|OHCI_INT_UE|OHCI_INT_OC)
#else
#  define BCM4390X_DEBUG_INTS      0
#endif

#define BCM4390X_NORMAL_INTS       (OHCI_INT_WDH|OHCI_INT_RHSC)
#define BCM4390X_ALL_INTS          (BCM4390X_NORMAL_INTS|BCM4390X_DEBUG_INTS)

/* Periodic Intervals **********************************************************/
/* Periodic intervals 2, 4, 8, 16,and 32 supported */

#define MIN_PERINTERVAL 2
#define MAX_PERINTERVAL 32

/* Descriptors *****************************************************************/
/* Actual number of allocated EDs and TDs will include one for the control ED
 * and one for the tail ED for each RHPort:
 */

#define BCM4390X_OHCI_NEDS       (CONFIG_BCM4390X_OHCI_NEDS + BCM4390X_OHCI_NRHPORT)
#define BCM4390X_OHCI_NTDS       (CONFIG_BCM4390X_OHCI_NTDS + BCM4390X_OHCI_NRHPORT)

/* TD delay interrupt value */

#define TD_DELAY(n)           (uint32_t)((n) << GTD_STATUS_DI_SHIFT)


/* DMA *************************************************************************/
#define bcm4390x_physramaddr(a)		((a) == 0 ? 0 : (uintptr_t)platform_addr_cached_to_uncached((const void *)(a)))
#define bcm4390x_virtramaddr(a)		((a) == 0 ? 0 : (uintptr_t)platform_addr_uncached_to_cached((const void *)(a)))


/*******************************************************************************
 * Private Types
 *******************************************************************************/
/* This structure contains one endpoint list.  The main reason for the existence
 * of this structure is to contain the sem_t value associated with the ED.  It
 * doesn't work well within the ED itself because then the semaphore counter
 * is subject to DMA cache operations (invalidate a modified semaphore count
 * is fatal!).
 */

struct bcm4390x_ohci_eplist_s
{
  volatile bool    wdhwait;    /* TRUE: Thread is waiting for WDH interrupt */
  sem_t            wdhsem;     /* Semaphore used to wait for Writeback Done Head event */
  struct bcm4390x_ohci_ed_s  *ed;        /* Endpoint descriptor (ED) */
  struct bcm4390x_ohci_gtd_s *tail;      /* Tail transfer descriptor (TD) */
};

/* This structure retains the state of one root hub port */

struct bcm4390x_ohci_rhport_s
{
  /* Common device fields.  This must be the first thing defined in the
   * structure so that it is possible to simply cast from struct usbhost_s
   * to struct bcm4390x_ohci_rhport_s.
   */

  struct usbhost_driver_s drvr;

  /* Root hub port status */

  volatile bool connected;     /* Connected to device */
  volatile bool lowspeed;      /* Low speed device attached. */
  uint8_t rhpndx;              /* Root hub port index */
  bool ep0init;                /* True:  EP0 initialized */

  struct bcm4390x_ohci_eplist_s ep0;     /* EP0 endpoint list */

  /* The bound device class driver */

  struct usbhost_class_s *class;
};

/* This structure retains the overall state of the USB host controller */

struct bcm4390x_ohci_s
{
  volatile bool rhswait;       /* TRUE: Thread is waiting for Root Hub Status change */

#ifndef CONFIG_USBHOST_INT_DISABLE
  uint8_t ininterval;          /* Minimum periodic IN EP polling interval: 2, 4, 6, 16, or 32 */
  uint8_t outinterval;         /* Minimum periodic IN EP polling interval: 2, 4, 6, 16, or 32 */
#endif
  sem_t exclsem;               /* Support mutually exclusive access */
  sem_t rhssem;                /* Semaphore to wait Writeback Done Head event */
  struct work_s work;          /* Supports interrupt bottom half */

  /* Root hub ports */

  struct bcm4390x_ohci_rhport_s rhport[BCM4390X_OHCI_NRHPORT];
};

/* The OCHI expects the size of an endpoint descriptor to be 16 bytes.
 * However, the size allocated for an endpoint descriptor is 32 bytes.  This
 * is necessary first because the Cortex-A5 cache line size is 32 bytes and
 * tht is the smallest amount of memory that we can perform cache operations
 * on.  The  16-bytes is also used by the OHCI host driver in order to maintain
 * additional endpoint-specific data.
 */

struct bcm4390x_ohci_ed_s
{
  /* Hardware specific fields */

  struct ohci_ed_s hw;         /* 0-15 */

  /* Software specific fields */

  struct bcm4390x_ohci_eplist_s *eplist; /* 16-19: List structure associated with the ED */
  uint8_t          xfrtype;    /* 20: Transfer type.  See SB_EP_ATTR_XFER_* in usb.h */
  uint8_t          interval;   /* 21: Periodic EP polling interval: 2, 4, 6, 16, or 32 */
  volatile uint8_t tdstatus;   /* 22: TD control status bits from last Writeback Done Head event */
  uint8_t          pad[9];     /* 23-31: Pad to 32-bytes */
};

#define SIZEOF_BCM4390X_OHCI_ED_S 32

/* The OCHI expects the size of an transfer descriptor to be 16 bytes.
 * However, the size allocated for an endpoint descriptor is 32 bytes in
 * RAM.  This extra 16-bytes is used by the OHCI host driver in order to
 * maintain additional endpoint-specific data.
 */

struct bcm4390x_ohci_gtd_s
{
  /* Hardware specific fields */

  struct ohci_gtd_s hw;        /* 0-15 */

  /* Software specific fields */

  struct bcm4390x_ohci_ed_s *ed;         /* 16-19: Pointer to parent ED */
  uint8_t          pad[12];    /* 20-31: Pad to 32 bytes */
};

#define SIZEOF_BCM4390X_OHCI_TD_S 32

/* The following is used to manage lists of free EDs, TDs, and TD buffers */

struct bcm4390x_list_s
{
  struct bcm4390x_list_s *flink;    /* Link to next buffer in the list */
                               /* Variable length buffer data follows */
};

/*******************************************************************************
 * Private Function Prototypes
 *******************************************************************************/

/* Register operations ********************************************************/

#ifdef CONFIG_BCM4390X_OHCI_REGDEBUG
static void bcm4390x_printreg(uint32_t addr, uint32_t val, bool iswrite);
static void bcm4390x_checkreg(uint32_t addr, uint32_t val, bool iswrite);
static uint32_t bcm4390x_getreg(uint32_t addr);
static void bcm4390x_putreg(uint32_t val, uint32_t addr);
#else
# define bcm4390x_getreg(addr)     getreg32(addr)
# define bcm4390x_putreg(val,addr) putreg32(val,addr)
#endif

/* Semaphores ******************************************************************/

static void bcm4390x_takesem(sem_t *sem);
#define bcm4390x_givesem(s) sem_post(s);

/* Byte stream access helper functions *****************************************/

static inline uint16_t bcm4390x_getle16(const uint8_t *val);
#if 0 /* Not used */
static void bcm4390x_putle16(uint8_t *dest, uint16_t val);
#endif

/* OHCI memory pool helper functions *******************************************/

static struct bcm4390x_ohci_ed_s *bcm4390x_ochi_edalloc(void);
static void bcm4390x_ohci_edfree(struct bcm4390x_ohci_ed_s *ed);
static struct bcm4390x_ohci_gtd_s *bcm4390x_ohci_tdalloc(void);
static void bcm4390x_ohci_tdfree(struct bcm4390x_ohci_gtd_s *buffer);
static uint8_t *bcm4390x_ohci_tballoc(void);
static void bcm4390x_ohci_tbfree(uint8_t *buffer);

/* ED list helper functions ****************************************************/

static inline int bcm4390x_ohci_addbulked(struct bcm4390x_ohci_ed_s *ed);
static inline int bcm4390x_ohci_rembulked(struct bcm4390x_ohci_ed_s *ed);

#if !defined(CONFIG_USBHOST_INT_DISABLE) || !defined(CONFIG_USBHOST_ISOC_DISABLE)
static unsigned int bcm4390x_ohci_getinterval(uint8_t interval);
static void bcm4390x_ohci_setinttab(uint32_t value, unsigned int interval, unsigned int offset);
#endif

static inline int bcm4390x_ohci_addinted(const FAR struct usbhost_epdesc_s *epdesc,
                               struct bcm4390x_ohci_ed_s *ed);
static inline int bcm4390x_ohci_reminted(struct bcm4390x_ohci_ed_s *ed);

static inline int bcm4390x_ohci_addisoced(const FAR struct usbhost_epdesc_s *epdesc,
                                struct bcm4390x_ohci_ed_s *ed);
static inline int bcm4390x_ohci_remisoced(struct bcm4390x_ohci_ed_s *ed);

/* Descriptor helper functions *************************************************/

static int  bcm4390x_ohci_enqueuetd(struct bcm4390x_ohci_rhport_s *rhport, struct bcm4390x_ohci_ed_s *ed,
                          uint32_t dirpid, uint32_t toggle,
                          volatile uint8_t *buffer, size_t buflen);
static int  bcm4390x_ohci_ep0enqueue(struct bcm4390x_ohci_rhport_s *rhport);
static void bcm4390x_ohci_ep0dequeue(struct bcm4390x_ohci_rhport_s *rhport);
static int  bcm4390x_ohci_wdhwait(struct bcm4390x_ohci_rhport_s *rhport, struct bcm4390x_ohci_ed_s *ed);
static int  bcm4390x_ohci_ctrltd(struct bcm4390x_ohci_rhport_s *rhport, uint32_t dirpid,
                       uint8_t *buffer, size_t buflen);

/* Interrupt handling **********************************************************/

static void bcm4390x_rhsc_bottomhalf(void);
static void bcm4390x_wdh_bottomhalf(void);
static void bcm4390x_ohci_bottomhalf(void *arg);

/* USB host controller operations **********************************************/

static int bcm4390x_ohci_wait(FAR struct usbhost_connection_s *conn,
                    FAR const bool *connected);
static int bcm4390x_ohci_enumerate(FAR struct usbhost_connection_s *conn, int rhpndx);

static int bcm4390x_ohci_ep0configure(FAR struct usbhost_driver_s *drvr, uint8_t funcaddr,
                            uint16_t maxpacketsize);
static int bcm4390x_ohci_getdevinfo(FAR struct usbhost_driver_s *drvr,
                          FAR struct usbhost_devinfo_s *devinfo);
static int bcm4390x_ohci_epalloc(FAR struct usbhost_driver_s *drvr,
                       const FAR struct usbhost_epdesc_s *epdesc, usbhost_ep_t *ep);
static int bcm4390x_ohci_epfree(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep);
static int bcm4390x_ohci_alloc(FAR struct usbhost_driver_s *drvr,
                     FAR uint8_t **buffer, FAR size_t *maxlen);
static int bcm4390x_ohci_free(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer);
static int bcm4390x_ohci_ioalloc(FAR struct usbhost_driver_s *drvr,
                       FAR uint8_t **buffer, size_t buflen);
static int bcm4390x_ohci_iofree(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer);
static int bcm4390x_ohci_ctrlin(FAR struct usbhost_driver_s *drvr,
                      FAR const struct usb_ctrlreq_s *req,
                      FAR uint8_t *buffer);
static int bcm4390x_ohci_ctrlout(FAR struct usbhost_driver_s *drvr,
                       FAR const struct usb_ctrlreq_s *req,
                       FAR const uint8_t *buffer);
static int bcm4390x_ohci_transfer(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep,
                        FAR uint8_t *buffer, size_t buflen);
static void bcm4390x_ohci_disconnect(FAR struct usbhost_driver_s *drvr);

/*******************************************************************************
 * Private Data
 *******************************************************************************/

/* In this driver implementation, support is provided for only a single a single
 * USB device.  All status information can be simply retained in a single global
 * instance.
 */

static struct bcm4390x_ohci_s g_ohci;

/* This is the connection/enumeration interface */

static struct usbhost_connection_s g_ohciconn;

/* This is a free list of EDs and TD buffers */

static struct bcm4390x_list_s *g_edfree; /* List of unused EDs */
static struct bcm4390x_list_s *g_tdfree; /* List of unused TDs */
static struct bcm4390x_list_s *g_tbfree; /* List of unused transfer buffers */

/* Allocated descriptor memory. These must all be properly aligned
 * and must be positioned in a DMA-able memory region.
 */

/* This must be aligned to a 256-byte boundary */

static struct ohci_hcca_s g_hcca
                          __attribute__ ((aligned (256)));

/* Pools of free descriptors and buffers.  These will all be linked
 * into the free lists declared above.  These must be aligned to 8-byte
 * boundaries (we do 16-byte alignment).
 */

static struct bcm4390x_ohci_ed_s    g_edalloc[BCM4390X_OHCI_NEDS]
                          __attribute__ ((aligned (16)));
static struct bcm4390x_ohci_gtd_s   g_tdalloc[BCM4390X_OHCI_NTDS]
                          __attribute__ ((aligned (16)));
static uint8_t            g_bufalloc[BCM4390X_BUFALLOC]
                          __attribute__ ((aligned (16)));

/*******************************************************************************
 * Public Data
 *******************************************************************************/

/*******************************************************************************
 * Private Functions
 *******************************************************************************/

/*******************************************************************************
 * Name: bcm4390x_printreg
 *
 * Description:
 *   Print the contents of an BCM4390X OHCI register operation
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_OHCI_REGDEBUG
static void bcm4390x_printreg(uint32_t addr, uint32_t val, bool iswrite)
{
  lldbg("%08x%s%08x\n", addr, iswrite ? "<-" : "->", val);
}
#endif

/*******************************************************************************
 * Name: bcm4390x_checkreg
 *
 * Description:
 *   Get the contents of an BCM4390X OHCI register
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_OHCI_REGDEBUG
static void bcm4390x_checkreg(uint32_t addr, uint32_t val, bool iswrite)
{
  static uint32_t prevaddr = 0;
  static uint32_t preval = 0;
  static uint32_t count = 0;
  static bool     prevwrite = false;

  /* Is this the same value that we read from/wrote to the same register last time?
   * Are we polling the register?  If so, suppress the output.
   */

  if (addr == prevaddr && val == preval && prevwrite == iswrite)
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

      prevaddr  = addr;
      preval    = val;
      count     = 0;
      prevwrite = iswrite;

      /* Show the new register access */

      bcm4390x_printreg(addr, val, iswrite);
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

#ifdef CONFIG_BCM4390X_OHCI_REGDEBUG
static uint32_t bcm4390x_getreg(uint32_t addr)
{
  /* Read the value from the register */

  uint32_t val = getreg32(addr);

  /* Check if we need to print this value */

  bcm4390x_checkreg(addr, val, false);
  return val;
}
#endif

/*******************************************************************************
 * Name: bcm4390x_putreg
 *
 * Description:
 *   Set the contents of an BCM4390X register to a value
 *
 *******************************************************************************/

#ifdef CONFIG_BCM4390X_OHCI_REGDEBUG
static void bcm4390x_putreg(uint32_t val, uint32_t addr)
{
  /* Check if we need to print this value */

  bcm4390x_checkreg(addr, val, true);

  /* Write the value */

  putreg32(val, addr);
}
#endif

/****************************************************************************
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

/****************************************************************************
 * Name: bcm4390x_getle16
 *
 * Description:
 *   Get a (possibly unaligned) 16-bit little endian value.
 *
 *******************************************************************************/

static inline uint16_t bcm4390x_getle16(const uint8_t *val)
{
  return (uint16_t)val[1] << 8 | (uint16_t)val[0];
}


/*******************************************************************************
 * Name: bcm4390x_ochi_edalloc
 *
 * Description:
 *   Allocate an endpoint descriptor by removing it from the free list
 *
 *******************************************************************************/

static struct bcm4390x_ohci_ed_s *bcm4390x_ochi_edalloc(void)
{
  struct bcm4390x_ohci_ed_s *ed;

  /* Remove the ED from the freelist */

  ed = (struct bcm4390x_ohci_ed_s *)g_edfree;
  if (ed)
    {
      g_edfree = ((struct bcm4390x_list_s*)ed)->flink;
    }

  return ed;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_edfree
 *
 * Description:
 *   Free an endpoint descriptor by returning to the free list
 *
 *******************************************************************************/

static inline void bcm4390x_ohci_edfree(struct bcm4390x_ohci_ed_s *ed)
{
  struct bcm4390x_list_s *entry = (struct bcm4390x_list_s *)ed;

  /* Put the ED back into the free list */

  entry->flink = g_edfree;
  g_edfree     = entry;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_tdalloc
 *
 * Description:
 *   Allocate an transfer descriptor from the free list
 *
 * Assumptions:
 *   - Never called from an interrupt handler.
 *   - Protected from conconcurrent access to the TD pool by the interrupt
 *     handler
 *   - Protection from re-entrance must be assured by the caller
 *
 *******************************************************************************/

static struct bcm4390x_ohci_gtd_s *bcm4390x_ohci_tdalloc(void)
{
  struct bcm4390x_ohci_gtd_s *ret;
  irqstate_t flags;

  /* Disable interrupts momentarily so that bcm4390x_ohci_tdfree is not called from the
   * interrupt handler.
   */

  flags = irqsave();
  ret   = (struct bcm4390x_ohci_gtd_s *)g_tdfree;
  if (ret)
    {
      g_tdfree = ((struct bcm4390x_list_s*)ret)->flink;
    }

  irqrestore(flags);
  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_tdfree
 *
 * Description:
 *   Free a transfer descriptor by returning it to the free list
 *
 * Assumptions:
 *   - Only called from the WDH interrupt handler (and during initialization).
 *   - Interrupts are disabled in any case.
 *
 *******************************************************************************/

static void bcm4390x_ohci_tdfree(struct bcm4390x_ohci_gtd_s *td)
{
  struct bcm4390x_list_s *tdfree = (struct bcm4390x_list_s *)td;
  DEBUGASSERT(td);

  tdfree->flink = g_tdfree;
  g_tdfree      = tdfree;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_tballoc
 *
 * Description:
 *   Allocate an request/descriptor transfer buffer from the free list
 *
 * Assumptions:
 *   - Never called from an interrupt handler.
 *   - Protection from re-entrance must be assured by the caller
 *
 *******************************************************************************/

static uint8_t *bcm4390x_ohci_tballoc(void)
{
  uint8_t *ret = (uint8_t *)g_tbfree;
  if (ret)
    {
      g_tbfree = ((struct bcm4390x_list_s*)ret)->flink;
    }

  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_tbfree
 *
 * Description:
 *   Return an request/descriptor transfer buffer to the free list
 *
 *******************************************************************************/

static void bcm4390x_ohci_tbfree(uint8_t *buffer)
{
  struct bcm4390x_list_s *tbfree = (struct bcm4390x_list_s *)buffer;

  if (tbfree)
    {
      tbfree->flink = g_tbfree;
      g_tbfree      = tbfree;
    }
}

/*******************************************************************************
 * Name: bcm4390x_ohci_addbulked
 *
 * Description:
 *   Helper function to add an ED to the bulk list.
 *
 *******************************************************************************/

static inline int bcm4390x_ohci_addbulked(struct bcm4390x_ohci_ed_s *ed)
{
#ifndef CONFIG_USBHOST_BULK_DISABLE
  irqstate_t flags;
  uint32_t regval;
  uintptr_t physed;

  /* Disable bulk list processing while we modify the list */

  flags   = irqsave();
  regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval &= ~OHCI_CTRL_BLE;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);

  /* Add the new bulk ED to the head of the bulk list */

  ed->hw.nexted = bcm4390x_getreg(BCM4390X_OHCI_BULKHEADED);
  arch_clean_dcache((uintptr_t)ed, (uintptr_t)ed + sizeof(struct ohci_ed_s));

  physed = bcm4390x_physramaddr((uintptr_t)ed);
  bcm4390x_putreg((uint32_t)physed, BCM4390X_OHCI_BULKHEADED);

  /* Re-enable bulk list processing. */

  regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval |= OHCI_CTRL_BLE;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);

  irqrestore(flags);
  return OK;
#else
  return -ENOSYS;
#endif
}

/*******************************************************************************
 * Name: bcm4390x_ohci_rembulked
 *
 * Description:
 *   Helper function remove an ED from the bulk list.
 *
 *******************************************************************************/

static inline int bcm4390x_ohci_rembulked(struct bcm4390x_ohci_ed_s *ed)
{
#ifndef CONFIG_USBHOST_BULK_DISABLE
  struct bcm4390x_ohci_ed_s *curr;
  struct bcm4390x_ohci_ed_s *prev;
  irqstate_t flags;
  uintptr_t physed;
  uint32_t regval;

  /* Disable bulk list processing while we modify the list */

  flags = irqsave();
  regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval &= ~OHCI_CTRL_BLE;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);

  /* Find the ED in the bulk list.  NOTE: We really should never be mucking
   * with the bulk list while BLE is set.
   */

  physed = bcm4390x_getreg(BCM4390X_OHCI_BULKHEADED);
  for (curr = (struct bcm4390x_ohci_ed_s *)bcm4390x_virtramaddr(physed), prev = NULL;
       curr && curr != ed;
       prev = curr, curr = (struct bcm4390x_ohci_ed_s *)curr->hw.nexted);

  /* Hmmm.. It would be a bug if we do not find the ED in the bulk list. */

  DEBUGASSERT(curr != NULL);

  /* Remove the ED from the bulk list */

  if (curr != NULL)
    {
      /* Is this ED the first on in the bulk list? */

      if (prev == NULL)
        {
          /* Yes... set the head of the bulk list to skip over this ED */

          bcm4390x_putreg(ed->hw.nexted, BCM4390X_OHCI_BULKHEADED);
        }
      else
        {
          /* No.. set the forward link of the previous ED in the list
           * skip over this ED.
           */

          prev->hw.nexted = ed->hw.nexted;
          arch_clean_dcache((uintptr_t)prev,
                            (uintptr_t)prev + sizeof(struct bcm4390x_ohci_ed_s));
        }
    }

  /* Re-enable bulk list processing if the bulk list is still non-empty
   * after removing the ED node.
   */

  if (bcm4390x_getreg(BCM4390X_OHCI_BULKHEADED) != 0)
    {
      /* If the bulk list is now empty, then disable it */

      regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
      regval |= OHCI_CTRL_BLE;
      bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);
    }

  irqrestore(flags);
  return OK;
#else
  return -ENOSYS;
#endif
}

/*******************************************************************************
 * Name: bcm4390x_ohci_getinterval
 *
 * Description:
 *   Convert the endpoint polling interval into a HCCA table increment
 *
 *******************************************************************************/

#if !defined(CONFIG_USBHOST_INT_DISABLE) || !defined(CONFIG_USBHOST_ISOC_DISABLE)
static unsigned int bcm4390x_ohci_getinterval(uint8_t interval)
{
  /* The bInterval field of the endpoint descriptor contains the polling interval
   * for interrupt and isochronous endpoints. For other types of endpoint, this
   * value should be ignored. bInterval is provided in units of 1MS frames.
   */

  if (interval < 3)
    {
      return 2;
    }
  else if (interval < 7)
    {
      return 4;
    }
  else if (interval < 15)
    {
      return 8;
    }
  else if (interval < 31)
    {
      return 16;
    }
  else
    {
      return 32;
    }
}
#endif

/*******************************************************************************
 * Name: bcm4390x_ohci_setinttab
 *
 * Description:
 *   Set the interrupt table to the selected value using the provided interval
 *   and offset.
 *
 *******************************************************************************/

#if !defined(CONFIG_USBHOST_INT_DISABLE) || !defined(CONFIG_USBHOST_ISOC_DISABLE)
static void bcm4390x_ohci_setinttab(uint32_t value, unsigned int interval, unsigned int offset)
{
  uintptr_t inttbl;
  unsigned int i;

  for (i = offset; i < HCCA_INTTBL_WSIZE; i += interval)
    {
      /* Modify the table value */

      g_hcca.inttbl[i] = value;
    }

  /* Make sure that the modified table value is flushed to RAM */

  inttbl = (uintptr_t)g_hcca.inttbl;
  arch_clean_dcache(inttbl, inttbl + sizeof(uint32_t)*HCCA_INTTBL_WSIZE);
}
#endif

/*******************************************************************************
 * Name: bcm4390x_ohci_addinted
 *
 * Description:
 *   Helper function to add an ED to the HCCA interrupt table.
 *
 *   To avoid reshuffling the table so much and to keep life simple in general,
 *    the following rules are applied:
 *
 *     1. IN EDs get the even entries, OUT EDs get the odd entries.
 *     2. Add IN/OUT EDs are scheduled together at the minimum interval of all
 *        IN/OUT EDs.
 *
 *   This has the following consequences:
 *
 *     1. The minimum support polling rate is 2MS, and
 *     2. Some devices may get polled at a much higher rate than they request.
 *
 *******************************************************************************/

static inline int bcm4390x_ohci_addinted(const FAR struct usbhost_epdesc_s *epdesc,
                               struct bcm4390x_ohci_ed_s *ed)
{
#ifndef CONFIG_USBHOST_INT_DISABLE
  irqstate_t flags;
  unsigned int interval;
  unsigned int offset;
  uintptr_t physed;
  uintptr_t physhead;
  uint32_t regval;

  /* Disable periodic list processing.  Does this take effect immediately?  Or
   * at the next SOF... need to check.
   */

  flags   = irqsave();
  regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval &= ~OHCI_CTRL_PLE;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);

  /* Get the quanitized interval value associated with this ED and save it
   * in the ED.
   */

  interval     = bcm4390x_ohci_getinterval(epdesc->interval);
  ed->interval = interval;
  usbhost_vtrace2(OHCI_VTRACE2_INTERVAL, epdesc->interval, interval);

  /* Get the offset associated with the ED direction. IN EDs get the even
   * entries, OUT EDs get the odd entries.
   *
   * Get the new, minimum interval. Add IN/OUT EDs are scheduled together
   * at the minimum interval of all IN/OUT EDs.
   */

  if (epdesc->in)
    {
      offset = 0;
      if (g_ohci.ininterval > interval)
        {
          g_ohci.ininterval = interval;
        }
      else
        {
          interval = g_ohci.ininterval;
        }
    }
  else
    {
      offset = 1;
      if (g_ohci.outinterval > interval)
        {
          g_ohci.outinterval = interval;
        }
      else
        {
          interval = g_ohci.outinterval;
        }
    }

  usbhost_vtrace2(OHCI_VTRACE2_MININTERVAL, interval, offset);

  /* Get the (physical) head of the first of the duplicated entries.  The
   * first offset entry is always guaranteed to contain the common ED list
   * head.
   */

  physhead = g_hcca.inttbl[offset];

  /* Clear all current entries in the interrupt table for this direction */

  bcm4390x_ohci_setinttab(0, 2, offset);

  /* Add the new ED before the old head of the periodic ED list and set the
   * new ED as the head ED in all of the appropriate entries of the HCCA
   * interrupt table.
   */

  ed->hw.nexted = physhead;
  arch_clean_dcache((uintptr_t)ed, (uintptr_t)ed + sizeof(struct ohci_ed_s));

  physed =  bcm4390x_physramaddr((uintptr_t)ed);
  bcm4390x_ohci_setinttab((uint32_t)physed, interval, offset);

  usbhost_vtrace1(OHCI_VTRACE1_PHYSED, physed);

  /* Re-enable periodic list processing */

  regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval |= OHCI_CTRL_PLE;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);

  irqrestore(flags);
  return OK;
#else
  return -ENOSYS;
#endif
}

/*******************************************************************************
 * Name: bcm4390x_ohci_reminted
 *
 * Description:
 *   Helper function to remove an ED from the HCCA interrupt table.
 *
 *   To avoid reshuffling the table so much and to keep life simple in general,
 *    the following rules are applied:
 *
 *     1. IN EDs get the even entries, OUT EDs get the odd entries.
 *     2. Add IN/OUT EDs are scheduled together at the minimum interval of all
 *        IN/OUT EDs.
 *
 *   This has the following consequences:
 *
 *     1. The minimum support polling rate is 2MS, and
 *     2. Some devices may get polled at a much higher rate than they request.
 *
 *******************************************************************************/

static inline int bcm4390x_ohci_reminted(struct bcm4390x_ohci_ed_s *ed)
{
#ifndef CONFIG_USBHOST_INT_DISABLE
  struct bcm4390x_ohci_ed_s *head;
  struct bcm4390x_ohci_ed_s *curr;
  struct bcm4390x_ohci_ed_s *prev;
  irqstate_t flags;
  uintptr_t physhead;
  unsigned int interval;
  unsigned int offset;
  uint32_t regval;

  /* Disable periodic list processing.  Does this take effect immediately?  Or
   * at the next SOF... need to check.
   */

  flags   = irqsave();
  regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval &= ~OHCI_CTRL_PLE;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);

  /* Get the offset associated with the ED direction. IN EDs get the even
   * entries, OUT EDs get the odd entries.
   */

  if ((ed->hw.ctrl & ED_CONTROL_D_MASK) == ED_CONTROL_D_IN)
    {
      offset = 0;
    }
  else
    {
      offset = 1;
    }

  /* Get the head of the first of the duplicated entries.  The first offset
   * entry is always guaranteed to contain the common ED list head.
   */

  physhead = g_hcca.inttbl[offset];
  head     = (struct bcm4390x_ohci_ed_s *)bcm4390x_virtramaddr((uintptr_t)physhead);

#ifdef CONFIG_USBHOST_TRACE
  usbhost_vtrace1(OHCI_VTRACE1_VIRTED, (uintptr_t)ed);
#else
  uvdbg("ed: %08x head: %08x next: %08x offset: %d\n",
        ed, physhead, head ? head->hw.nexted : 0, offset);
#endif

  /* Find the ED to be removed in the ED list */

  for (curr = head, prev = NULL;
       curr && curr != ed;
       prev = curr, curr = (struct bcm4390x_ohci_ed_s *)curr->hw.nexted);

  /* Hmmm.. It would be a bug if we do not find the ED in the bulk list. */

  DEBUGASSERT(curr != NULL);
  if (curr != NULL)
    {
      /* Clear all current entries in the interrupt table for this direction */

      bcm4390x_ohci_setinttab(0, 2, offset);

      /* Remove the ED from the list..  Is this ED the first on in the list? */

      if (prev == NULL)
        {
          /* Yes... set the head of the bulk list to skip over this ED */

          physhead = ed->hw.nexted;
          head     = (struct bcm4390x_ohci_ed_s *)bcm4390x_virtramaddr((uintptr_t)physhead);
        }
      else
        {
          /* No.. set the forward link of the previous ED in the list
           * skip over this ED.
           */

          prev->hw.nexted = ed->hw.nexted;
        }

#ifdef CONFIG_USBHOST_TRACE
      usbhost_vtrace1(OHCI_VTRACE1_VIRTED, (uintptr_t)ed);
#else
      uvdbg("ed: %08x head: %08x next: %08x\n",
            ed, physhead, head ? head->hw.nexted : 0);
#endif

      /* Calculate the new minimum interval for this list */

      interval = MAX_PERINTERVAL;
      for (curr = head; curr; curr = (struct bcm4390x_ohci_ed_s *)curr->hw.nexted)
        {
          if (curr->interval < interval)
            {
              interval = curr->interval;
            }
        }

      usbhost_vtrace2(OHCI_VTRACE2_MININTERVAL, interval, offset);

      /* Save the new minimum interval */

      if ((ed->hw.ctrl && ED_CONTROL_D_MASK) == ED_CONTROL_D_IN)
        {
          g_ohci.ininterval  = interval;
        }
      else
        {
          g_ohci.outinterval = interval;
        }

      /* Set the head ED in all of the appropriate entries of the HCCA interrupt
       * table (head might be NULL).
       */

      bcm4390x_ohci_setinttab((uint32_t)physhead, interval, offset);
    }

  /* Re-enabled periodic list processing */

  if (head != NULL)
    {
      regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
      regval |= OHCI_CTRL_PLE;
      bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);
    }

  irqrestore(flags);
  return OK;
#else
  return -ENOSYS;
#endif
}

/*******************************************************************************
 * Name: bcm4390x_ohci_addisoced
 *
 * Description:
 *   Helper functions to add an ED to the periodic table.
 *
 *******************************************************************************/

static inline int bcm4390x_ohci_addisoced(const FAR struct usbhost_epdesc_s *epdesc,
                                struct bcm4390x_ohci_ed_s *ed)
{
#ifndef CONFIG_USBHOST_ISOC_DISABLE
//#  warning "Isochronous endpoints not yet supported"
#endif
  return -ENOSYS;

}

/*******************************************************************************
 * Name: bcm4390x_ohci_remisoced
 *
 * Description:
 *   Helper functions to remove an ED from the periodic table.
 *
 *******************************************************************************/

static inline int bcm4390x_ohci_remisoced(struct bcm4390x_ohci_ed_s *ed)
{
#ifndef CONFIG_USBHOST_ISOC_DISABLE
//#  warning "Isochronous endpoints not yet supported"
#endif
  return -ENOSYS;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_enqueuetd
 *
 * Description:
 *   Enqueue a transfer descriptor.  Notice that this function only supports
 *   queue on TD per ED.
 *
 *******************************************************************************/

static int bcm4390x_ohci_enqueuetd(struct bcm4390x_ohci_rhport_s *rhport, struct bcm4390x_ohci_ed_s *ed,
                         uint32_t dirpid, uint32_t toggle,
                         volatile uint8_t *buffer, size_t buflen)
{
  struct bcm4390x_ohci_gtd_s *td;
  struct bcm4390x_ohci_gtd_s *tdtail;
  uintptr_t phytd;
  uintptr_t phytail;
  uintptr_t phybuf;
  int ret = -ENOMEM;

  /* Allocate a TD from the free list */

  td = bcm4390x_ohci_tdalloc();
  if (td != NULL)
    {
      /* Skip processing of this ED while we modify the TD list. */

      ed->hw.ctrl      |= ED_CONTROL_K;
      arch_clean_dcache((uintptr_t)ed,
                        (uintptr_t)ed + sizeof(struct ohci_ed_s));

      /* Get the tail ED for this root hub port */

      tdtail            = rhport->ep0.tail;

      /* Get physical addresses to support the DMA */

      phytd             =  bcm4390x_physramaddr((uintptr_t)td);
      phytail           =  bcm4390x_physramaddr((uintptr_t)tdtail);
      phybuf            =  bcm4390x_physramaddr((uintptr_t)buffer);

      /* Initialize the allocated TD and link it before the common tail TD. */

      td->hw.ctrl       = (GTD_STATUS_R | dirpid | TD_DELAY(0) |
                           toggle | GTD_STATUS_CC_MASK);
      tdtail->hw.ctrl   = 0;
      td->hw.cbp        = (uint32_t)phybuf;
      tdtail->hw.cbp    = 0;
      td->hw.nexttd     = (uint32_t)phytail;
      tdtail->hw.nexttd = 0;
      td->hw.be         = (uint32_t)(phybuf + (buflen - 1));
      tdtail->hw.be     = 0;

      /* Configure driver-only fields in the extended TD structure */

      td->ed            = ed;

      /* Link the td to the head of the ED's TD list */

      ed->hw.headp      = (uint32_t)phytd | ((ed->hw.headp) & ED_HEADP_C);
      ed->hw.tailp      = (uint32_t)phytail;

      /* Flush the buffer (if there is one), the new TD, and the modified ED
       * to RAM.
       */

      if (buffer && buflen > 0)
        {
          arch_clean_dcache((uintptr_t)buffer,
                            (uintptr_t)buffer + buflen);
        }

      arch_clean_dcache((uintptr_t)tdtail,
                        (uintptr_t)tdtail + sizeof(struct ohci_gtd_s));
      arch_clean_dcache((uintptr_t)td,
                        (uintptr_t)td + sizeof(struct ohci_gtd_s));

      /* Resume processing of this ED */

      ed->hw.ctrl      &= ~ED_CONTROL_K;
      arch_clean_dcache((uintptr_t)ed,
                        (uintptr_t)ed + sizeof(struct ohci_ed_s));
      ret               = OK;
    }

  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_ep0enqueue
 *
 * Description:
 *   Initialize ED for EP0, add it to the control ED list, and enable control
 *   transfers.
 *
 * Input Parameters:
 *   rhpndx - Root hub port index.
 *
 * Returned Values:
 *   None
 *
 *******************************************************************************/

static int bcm4390x_ohci_ep0enqueue(struct bcm4390x_ohci_rhport_s *rhport)
{
  struct bcm4390x_ohci_ed_s *edctrl;
  struct bcm4390x_ohci_gtd_s *tdtail;
  irqstate_t flags;
  uintptr_t physaddr;
  uint32_t regval;

  DEBUGASSERT(rhport && !rhport->ep0init && rhport->ep0.ed == NULL &&
              rhport->ep0.tail == NULL);

  /* Allocate a control ED and a tail TD */

  flags  = irqsave();
  edctrl = bcm4390x_ochi_edalloc();
  if (!edctrl)
    {
      irqrestore(flags);
      return -ENOMEM;
    }

  tdtail = bcm4390x_ohci_tdalloc();
  if (!tdtail)
    {
      bcm4390x_ohci_edfree(edctrl);
      irqrestore(flags);
      return -ENOMEM;
    }

  rhport->ep0.ed   = edctrl;
  rhport->ep0.tail = tdtail;

  /* ControlListEnable.  This bit is cleared to disable the processing of the
   * Control list.  We should never modify the control list while CLE is set.
   */

  regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval &= ~OHCI_CTRL_CLE;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);

  /* Initialize the common tail TD for this port */

  memset(tdtail, 0, sizeof(struct bcm4390x_ohci_gtd_s));
  tdtail->ed       = edctrl;

  /* Initialize the control endpoint for this port.
   * Set up some default values (like max packetsize = 8).
   * NOTE that the SKIP bit is set until the first readl TD is added.
   */

  memset(edctrl, 0, sizeof(struct bcm4390x_ohci_ed_s));
  (void)bcm4390x_ohci_ep0configure(&rhport->drvr, 0, 8);
  edctrl->hw.ctrl  |= ED_CONTROL_K;
  edctrl->eplist    = &rhport->ep0;

  /* Link the common tail TD to the ED's TD list */

  physaddr          = bcm4390x_physramaddr((uintptr_t)tdtail);
  edctrl->hw.headp  = (uint32_t)physaddr;
  edctrl->hw.tailp  = (uint32_t)physaddr;

  /* The new ED will be the first ED in the list.  Set the nexted
   * pointer of the ED old head of the list
   */

  physaddr          = bcm4390x_getreg(BCM4390X_OHCI_CTRLHEADED);
  edctrl->hw.nexted = physaddr;

  /* Set the control list head to the new ED */

  physaddr          = (uintptr_t)bcm4390x_physramaddr((uintptr_t)edctrl);
  bcm4390x_putreg(physaddr, BCM4390X_OHCI_CTRLHEADED);

  /* Flush the affected control ED and tail TD to RAM */

  arch_clean_dcache((uintptr_t)edctrl,
                    (uintptr_t)edctrl + sizeof(struct ohci_ed_s));
  arch_clean_dcache((uintptr_t)tdtail,
                    (uintptr_t)tdtail + sizeof(struct ohci_gtd_s));

  /* ControlListEnable.  This bit is set to (re-)enable the processing of the
   * Control list.  Note: once enabled, it remains enabled and we may even
   * complete list processing before we get the bit set.
   */

  regval = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval |= OHCI_CTRL_CLE;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);
  irqrestore(flags);
  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_ep0dequeue
 *
 * Description:
 *   Remove the ED for EP0 from the control ED list and possibly disable control
 *   list processing.
 *
 * Input Parameters:
 *   rhpndx - Root hub port index.
 *
 * Returned Values:
 *   None
 *
 *******************************************************************************/

static void bcm4390x_ohci_ep0dequeue(struct bcm4390x_ohci_rhport_s *rhport)
{
  struct bcm4390x_ohci_ed_s *edctrl;
  struct bcm4390x_ohci_ed_s *curred;
  struct bcm4390x_ohci_ed_s *preved;
  struct bcm4390x_ohci_gtd_s *tdtail;
  struct bcm4390x_ohci_gtd_s *currtd;
  irqstate_t flags;
  uintptr_t physcurr;
  uint32_t regval;

  DEBUGASSERT(rhport && rhport->ep0init && rhport->ep0.ed != NULL &&
              rhport->ep0.tail != NULL);

  /* ControlListEnable.  This bit is cleared to disable the processing of the
   * Control list.  We should never modify the control list while CLE is set.
   */

  flags   = irqsave();
  regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval &= ~OHCI_CTRL_CLE;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);

  /* Search the control list to find the entry to be removed (and its
   * precedessor).
   */

  edctrl   = rhport->ep0.ed;
  physcurr = bcm4390x_getreg(BCM4390X_OHCI_CTRLHEADED);

  for (curred = (struct bcm4390x_ohci_ed_s *)bcm4390x_virtramaddr(physcurr),
       preved = NULL;
       curred && curred != edctrl;
       preved = curred,
       curred =(struct bcm4390x_ohci_ed_s *)bcm4390x_virtramaddr(physcurr))
    {
      physcurr = curred->hw.nexted;
    }

  DEBUGASSERT(curred);

  /* Remove the ED from the control list */

  if (preved)
    {
      /* Unlink the ED from the previous ED in the list */

      preved->hw.nexted = edctrl->hw.nexted;

      /* Flush the modified ED to RAM */

      arch_clean_dcache((uintptr_t)preved,
                        (uintptr_t)preved + sizeof(struct ohci_ed_s));
    }
  else
    {
      /* Set the new control list head ED */

      bcm4390x_putreg(edctrl->hw.nexted, BCM4390X_OHCI_CTRLHEADED);

      /* If the control list head is still non-NULL, then (re-)enable
       * processing of the Control list.
       */

      if (edctrl->hw.nexted != 0)
        {
          regval = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
          regval |= OHCI_CTRL_CLE;
          bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);
        }
    }

  irqrestore(flags);

  /* Release any TDs that may still be attached to the ED. */

  tdtail   = rhport->ep0.tail;
  physcurr = edctrl->hw.headp;

  for (currtd = (struct bcm4390x_ohci_gtd_s *)bcm4390x_virtramaddr(physcurr);
       currtd && currtd != tdtail;
       currtd =(struct bcm4390x_ohci_gtd_s *)bcm4390x_virtramaddr(physcurr))
    {
      physcurr = currtd->hw.nexttd;
      bcm4390x_ohci_tdfree(currtd);
    }

  /* Free the tail TD and the control ED allocated for this port */

  bcm4390x_ohci_tdfree(tdtail);
  bcm4390x_ohci_edfree(edctrl);

  rhport->ep0.ed   = NULL;
  rhport->ep0.tail = NULL;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_wdhwait
 *
 * Description:
 *   Set the request for the Writeback Done Head event well BEFORE enabling the
 *   transfer (as soon as we are absolutely committed to the to avoid transfer).
 *   We do this to minimize race conditions.  This logic would have to be expanded
 *   if we want to have more than one packet in flight at a time!
 *
 *******************************************************************************/

static int bcm4390x_ohci_wdhwait(struct bcm4390x_ohci_rhport_s *rhport, struct bcm4390x_ohci_ed_s *ed)
{
  struct bcm4390x_ohci_eplist_s *eplist;
  irqstate_t flags = irqsave();
  int ret = -ENODEV;

  /* Is the device still connected? */

  if (rhport->connected)
    {
      /* Yes.. Get the endpoint list associated with the ED */

      eplist = ed->eplist;
      DEBUGASSERT(eplist);

      /* Then set wdhwait to indicate that we expect to be informed when
       * either (1) the device is disconnected, or (2) the transfer
       * completed.
       */

      eplist->wdhwait = true;
      ret = OK;
    }

  irqrestore(flags);
  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_ctrltd
 *
 * Description:
 *   Process a IN or OUT request on the control endpoint.  This function
 *   will enqueue the request and wait for it to complete.  Only one transfer
 *   may be queued; Neither these methods nor the transfer() method can be
 *   called again until the control transfer functions returns.
 *
 *   These are blocking methods; these functions will not return until the
 *   control transfer has completed.
 *
 *******************************************************************************/

static int bcm4390x_ohci_ctrltd(struct bcm4390x_ohci_rhport_s *rhport, uint32_t dirpid,
                      uint8_t *buffer, size_t buflen)
{
  struct bcm4390x_ohci_eplist_s *eplist;
  struct bcm4390x_ohci_ed_s *edctrl;
  uint32_t toggle;
  uint32_t regval;
  int ret;

  /* Set the request for the Writeback Done Head event well BEFORE enabling the
   * transfer.
   */

  edctrl = rhport->ep0.ed;
  ret = bcm4390x_ohci_wdhwait(rhport, edctrl);
  if (ret != OK)
    {
      usbhost_trace1(OHCI_TRACE1_DEVDISCONN, rhport->rhpndx + 1);
      return ret;
    }

  /* Get the endpoint list structure for the ED */

  eplist = &rhport->ep0;

  /* Configure the toggle field in the TD */

  if (dirpid == GTD_STATUS_DP_SETUP)
    {
      toggle = GTD_STATUS_T_DATA0;
    }
  else
    {
      toggle = GTD_STATUS_T_DATA1;
    }

  /* Then enqueue the transfer */

  edctrl->tdstatus = TD_CC_NOERROR;
  ret = bcm4390x_ohci_enqueuetd(rhport, edctrl, dirpid, toggle, buffer, buflen);
  if (ret == OK)
    {
      /* Set ControlListFilled.  This bit is used to indicate whether there are
       * TDs on the Control list.
       */

      regval = bcm4390x_getreg(BCM4390X_OHCI_CMDST);
      regval |= OHCI_CMDST_CLF;
      bcm4390x_putreg(regval, BCM4390X_OHCI_CMDST);

      /* Release the OHCI semaphore while we wait.  Other threads need the
       * opportunity to access the EHCI resources while we wait.
       *
       * REVISIT:  Is this safe?  NO.  This is a bug and needs rethinking.
       * We need to lock all of the port-resources (not EHCI common) until
       * the transfer is complete.  But we can't use the common OHCI exclsem
       * or we will deadlock while waiting (because the working thread that
       * wakes this thread up needs the exclsem).
       */
      bcm4390x_givesem(&g_ohci.exclsem);

      /* Wait for the Writeback Done Head interrupt  Loop to handle any false
       * alarm semaphore counts.
       */

      while (eplist->wdhwait)
        {
          bcm4390x_takesem(&eplist->wdhsem);
        }

      /* Re-aquire the ECHI semaphore.  The caller expects to be holding
       * this upon return.
       */

      bcm4390x_takesem(&g_ohci.exclsem);

      /* Check the TD completion status bits */

      if (edctrl->tdstatus == TD_CC_NOERROR)
        {
          ret = OK;
        }
      else
        {
          usbhost_trace2(OHCI_TRACE2_BADTDSTATUS, rhport->rhpndx + 1,
                         edctrl->tdstatus);
          ret = -EIO;
        }
    }

  /* Make sure that there is no outstanding request on this endpoint */

  eplist->wdhwait = false;
  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_rhsc_bottomhalf
 *
 * Description:
 *   OHCI root hub status change interrupt handler
 *
 *******************************************************************************/

static void bcm4390x_rhsc_bottomhalf(void)
{
  struct bcm4390x_ohci_rhport_s *rhport;
  uint32_t regaddr;
  uint32_t rhportst;
  int rhpndx;

  /* Handle root hub status change on each root port */

  for (rhpndx = 0; rhpndx < BCM4390X_OHCI_NRHPORT; rhpndx++)
    {
      rhport   = &g_ohci.rhport[rhpndx];

      regaddr  = BCM4390X_OHCI_RHPORTST(rhpndx+1);
      rhportst = bcm4390x_getreg(regaddr);

      usbhost_vtrace2(OHCI_VTRACE2_RHPORTST, rhpndx + 1, (uint16_t)rhportst);

      if ((rhportst & OHCI_RHPORTST_CSC) != 0)
        {
          uint32_t rhstatus = bcm4390x_getreg(BCM4390X_OHCI_RHSTATUS);
          usbhost_vtrace1(OHCI_VTRACE1_CSC, rhstatus);

          /* If DRWE is set, Connect Status Change indicates a remote
           * wake-up event
           */

          if (rhstatus & OHCI_RHSTATUS_DRWE)
            {
              usbhost_vtrace1(OHCI_VTRACE1_DRWE, rhstatus);
            }

          /* Otherwise... Not a remote wake-up event */

          else
            {
              /* Check current connect status */

              if ((rhportst & OHCI_RHPORTST_CCS) != 0)
                {
                  /* Connected ... Did we just become connected? */

                  if (!rhport->connected)
                    {
                      /* Yes.. connected. */
                      rhport->connected = true;

                      usbhost_vtrace2(OHCI_VTRACE2_CONNECTED,
                                      rhpndx + 1, g_ohci.rhswait);

                      /* Notify any waiters */

                      if (g_ohci.rhswait)
                        {
                          g_ohci.rhswait = false;
                          bcm4390x_givesem(&g_ohci.rhssem);

                        }
                    }
                  else
                    {
                      usbhost_vtrace1(OHCI_VTRACE1_ALREADYCONN, rhportst);
                    }

                  /* The LSDA (Low speed device attached) bit is valid
                   * when CCS == 1.
                   */

                  rhport->lowspeed = (rhportst & OHCI_RHPORTST_LSDA) != 0;
                  usbhost_vtrace1(OHCI_VTRACE1_SPEED, rhport->lowspeed);
                }

              /* Check if we are now disconnected */

              else if (rhport->connected)
                {
                  /* Yes.. disconnect the device */

                  usbhost_vtrace2(OHCI_VTRACE2_DISCONNECTED,
                                  rhpndx + 1, g_ohci.rhswait);

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

                  if (g_ohci.rhswait)
                    {
                      g_ohci.rhswait = false;
                      bcm4390x_givesem(&g_ohci.rhssem);

                    }
                }
              else
                {
                  usbhost_vtrace1(OHCI_VTRACE1_ALREADYDISCONN, rhportst);
                }
            }

          /* Clear the status change interrupt */

          bcm4390x_putreg(OHCI_RHPORTST_CSC, regaddr);
        }

      /* Check for port reset status change */

      if ((rhportst & OHCI_RHPORTST_PRSC) != 0)
        {
          /* Release the RH port from reset */

          bcm4390x_putreg(OHCI_RHPORTST_PRSC, regaddr);
        }
    }
}

/*******************************************************************************
 * Name: bcm4390x_wdh_bottomhalf
 *
 * Description:
 *   OHCI write done head interrupt handler
 *
 *******************************************************************************/

static void bcm4390x_wdh_bottomhalf(void)
{
  struct bcm4390x_ohci_eplist_s *eplist;
  struct bcm4390x_ohci_gtd_s *td;
  struct bcm4390x_ohci_gtd_s *next;
  struct bcm4390x_ohci_ed_s *ed;

  /* The host controller just wrote the one finished TDs into the HCCA
   * done head.  This may include multiple packets that were transferred
   * in the preceding frame.
   *
   * Remove the TD from the Writeback Done Head in the HCCA and return
   * it to the free list.  Note that this is safe because the hardware
   * will not modify the writeback done head again until the WDH bit is
   * cleared in the interrupt status register.
   */

  /* Invalidate D-cache to force re-reading of the Done Head */

  arch_invalidate_dcache((uintptr_t)&g_hcca,
                         (uintptr_t)&g_hcca + sizeof(struct ohci_hcca_s));

  /* Now read the done head */

  td = (struct bcm4390x_ohci_gtd_s *)bcm4390x_virtramaddr(g_hcca.donehead);
  g_hcca.donehead = 0;
  DEBUGASSERT(td);

  /* Process each TD in the write done list */

  for (; td; td = next)
    {

      /* Invalidate the just-finished TD from D-cache to force it to be
       * reloaded from memory.
       */

      arch_invalidate_dcache((uintptr_t)td,
                             (uintptr_t)td + sizeof( struct ohci_gtd_s));

      /* Get the ED in which this TD was enqueued */

      ed = td->ed;
      DEBUGASSERT(ed != NULL);

      /* Get the endpoint list that contains the ED */

      eplist = ed->eplist;
      DEBUGASSERT(eplist != NULL);

      /* Also invalidate the control ED so that it two will be re-read from
       * memory.
       */

      arch_invalidate_dcache((uintptr_t)ed,
                             (uintptr_t)ed + sizeof( struct ohci_ed_s));

      /* Save the condition code from the (single) TD status/control
       * word.
       */

      ed->tdstatus = (td->hw.ctrl & GTD_STATUS_CC_MASK) >> GTD_STATUS_CC_SHIFT;

#ifdef HAVE_USBHOST_TRACE
      if (ed->tdstatus != TD_CC_NOERROR)
        {
          /* The transfer failed for some reason... dump some diagnostic info. */

          usbhost_trace2(OHCI_TRACE2_WHDTDSTATUS, ed->tdstatus, ed->xfrtype);
        }
#endif

      /* Return the TD to the free list */

      next = (struct bcm4390x_ohci_gtd_s *)bcm4390x_virtramaddr(td->hw.nexttd);
      bcm4390x_ohci_tdfree(td);

      /* And wake up the thread waiting for the WDH event */

      if (eplist->wdhwait)
        {
          eplist->wdhwait = false;
          bcm4390x_givesem(&eplist->wdhsem);

        }
    }
}

/*******************************************************************************
 * Name: bcm4390x_ohci_bottomhalf
 *
 * Description:
 *   OHCI interrupt bottom half.  This function runs on the high priority worker
 *   thread and was xcheduled when the last interrupt occurred.  The set of
 *   pending interrupts is provided as the argument.  OHCI interrupts were
 *   disabled when this function is scheduled so no further interrupts can
 *   occur until this work re-enables OHCI interrupts
 *
 *******************************************************************************/

static void bcm4390x_ohci_bottomhalf(void *arg)
{
  uint32_t pending = (uint32_t)arg;

  /* We need to have exclusive access to the EHCI data structures.  Waiting here
   * is not a good thing to do on the worker thread, but there is no real option
   * (other than to reschedule and delay).
   */

  bcm4390x_takesem(&g_ohci.exclsem);

  /* Root hub status change interrupt */

  if ((pending & OHCI_INT_RHSC) != 0)
    {
      /* Handle root hub status change on each root port */

      usbhost_vtrace1(OHCI_VTRACE1_RHSC, pending);
      bcm4390x_rhsc_bottomhalf();
    }

  /* Writeback Done Head interrupt */

  if ((pending & OHCI_INT_WDH) != 0)
    {
      /* The host controller just wrote the list of finished TDs into the HCCA
       * done head.  This may include multiple packets that were transferred
       * in the preceding frame.
       */

      usbhost_vtrace1(OHCI_VTRACE1_WDHINTR, pending);
      bcm4390x_wdh_bottomhalf();
    }

#ifdef CONFIG_DEBUG_USB
  if ((pending & BCM4390X_DEBUG_INTS) != 0)
    {
      if ((pending & OHCI_INT_UE) != 0)
        {
          /* An unrecoverable error occurred.  Unrecoverable errors
           * are usually the consequence of bad descriptor contents
           * or DMA errors.
           *
           * Treat this like a normal write done head interrupt.  We
           * just want to see if there is any status information writen
           * to the descriptors (and the normal write done head
           * interrupt will not be occurring).
           */

          usbhost_trace1(OHCI_TRACE1_INTRUNRECOVERABLE, pending);
          bcm4390x_wdh_bottomhalf();
        }
      else
        {
          usbhost_trace1(OHCI_TRACE1_INTRUNHANDLED, pending);
        }
    }
#endif

  /* Now re-enable interrupts */

  bcm4390x_putreg(OHCI_INT_MIE, BCM4390X_OHCI_INTEN);
  bcm4390x_givesem(&g_ohci.exclsem);
}

/*******************************************************************************
 * USB Host Controller Operations
 *******************************************************************************/

/*******************************************************************************
 * Name: bcm4390x_ohci_wait
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

static int bcm4390x_ohci_wait(FAR struct usbhost_connection_s *conn,
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

      for (rhpndx = 0; rhpndx < BCM4390X_OHCI_NRHPORT; rhpndx++)
        {
          /* Has the connection state changed on the RH port? */

          if (g_ohci.rhport[rhpndx].connected != connected[rhpndx])
            {
              /* Yes.. Return the RH port number to inform the caller which
               * port has the connection change.
               */

              irqrestore(flags);
              usbhost_vtrace2(OHCI_VTRACE2_WAKEUP,
                              rhpndx + 1, g_ohci.rhport[rhpndx].connected);
              return rhpndx;
            }
        }

      /* No changes on any port. Wait for a connection/disconnection event
       * and check again
       */

      g_ohci.rhswait = true;
      bcm4390x_takesem(&g_ohci.rhssem);
    }
}

/*******************************************************************************
 * Name: bcm4390x_ohci_enumerate
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

static int bcm4390x_ohci_enumerate(FAR struct usbhost_connection_s *conn, int rhpndx)
{
  struct bcm4390x_ohci_rhport_s *rhport;
  uint32_t regaddr;
  int ret;

  DEBUGASSERT(rhpndx >= 0 && rhpndx < BCM4390X_OHCI_NRHPORT);
  rhport = &g_ohci.rhport[rhpndx];

  /* Are we connected to a device?  The caller should have called the wait()
   * method first to be assured that a device is connected.
   */

  while (!rhport->connected)
    {
      /* No, return an error */

      usbhost_vtrace1(OHCI_VTRACE1_ENUMDISCONN, rhport->rhpndx + 1);
      return -ENODEV;
    }

  /* Add EP0 to the control list */

  if (!rhport->ep0init)
    {
      ret = bcm4390x_ohci_ep0enqueue(rhport);
      if (ret < 0)
        {
          usbhost_trace2(OHCI_TRACE2_EP0ENQUEUE_FAILED, rhport->rhpndx + 1,
                         -ret);
          return ret;
        }

      /* Successfully initialized */

      rhport->ep0init = true;
    }


  /* USB 2.0 spec says at least 50ms delay before port reset */

  up_mdelay(100);

  /* Put the root hub port in reset
   */

  regaddr = BCM4390X_OHCI_RHPORTST(rhpndx+1);
  bcm4390x_putreg(OHCI_RHPORTST_PRS, regaddr);

  /* Wait for the port reset to complete */

  while ((bcm4390x_getreg(regaddr) & OHCI_RHPORTST_PRS) != 0);

  /* Release RH port 1 from reset and wait a bit */

  bcm4390x_putreg(OHCI_RHPORTST_PRSC, regaddr);
  up_mdelay(200);

  /* Let the common usbhost_enumerate do all of the real work.  Note that the
   * FunctionAddress (USB address) is set to the root hub port number for now.
   */

  usbhost_vtrace2(OHCI_VTRACE2_CLASSENUM, rhpndx+1, rhpndx+1);
  ret = usbhost_enumerate(&g_ohci.rhport[rhpndx].drvr, rhpndx+1, &rhport->class);
  if (ret < 0)
    {
      usbhost_trace2(OHCI_TRACE2_CLASSENUM_FAILED, rhpndx+1, -ret);
    }

  return ret;
}

/************************************************************************************
 * Name: bcm4390x_ohci_ep0configure
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

static int bcm4390x_ohci_ep0configure(FAR struct usbhost_driver_s *drvr, uint8_t funcaddr,
                            uint16_t maxpacketsize)
{
  struct bcm4390x_ohci_rhport_s *rhport = (struct bcm4390x_ohci_rhport_s *)drvr;
  struct bcm4390x_ohci_ed_s *edctrl;

  DEBUGASSERT(rhport &&
              funcaddr >= 0 && funcaddr <= BCM4390X_OHCI_NRHPORT &&
              maxpacketsize < 2048);

  edctrl = rhport->ep0.ed;

  /* We must have exclusive access to EP0 and the control list */

  bcm4390x_takesem(&g_ohci.exclsem);

  /* Set the EP0 ED control word */

  edctrl->hw.ctrl = (uint32_t)funcaddr << ED_CONTROL_FA_SHIFT |
                    (uint32_t)maxpacketsize << ED_CONTROL_MPS_SHIFT;

  if (rhport->lowspeed)
   {
     edctrl->hw.ctrl |= ED_CONTROL_S;
   }

  /* Set the transfer type to control */

  edctrl->xfrtype = USB_EP_ATTR_XFER_CONTROL;

  /* Flush the modified control ED to RAM */

  arch_clean_dcache((uintptr_t)edctrl,
                    (uintptr_t)edctrl + sizeof(struct ohci_ed_s));
  bcm4390x_givesem(&g_ohci.exclsem);

  usbhost_vtrace2(OHCI_VTRACE2_EP0CONFIGURE,
                  rhport->rhpndx + 1, (uint16_t)edctrl->hw.ctrl);
  return OK;
}

/************************************************************************************
 * Name: bcm4390x_ohci_getdevinfo
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

static int bcm4390x_ohci_getdevinfo(FAR struct usbhost_driver_s *drvr,
                          FAR struct usbhost_devinfo_s *devinfo)
{
  struct bcm4390x_ohci_rhport_s *rhport = (struct bcm4390x_ohci_rhport_s *)drvr;

  DEBUGASSERT(drvr && devinfo);
  devinfo->speed = rhport->lowspeed ? DEVINFO_SPEED_LOW : DEVINFO_SPEED_FULL;
  return OK;
}

/************************************************************************************
 * Name: bcm4390x_ohci_epalloc
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

static int bcm4390x_ohci_epalloc(FAR struct usbhost_driver_s *drvr,
                       const FAR struct usbhost_epdesc_s *epdesc, usbhost_ep_t *ep)
{
  struct bcm4390x_ohci_rhport_s *rhport = (struct bcm4390x_ohci_rhport_s *)drvr;
  struct bcm4390x_ohci_eplist_s *eplist;
  struct bcm4390x_ohci_ed_s *ed;
  struct bcm4390x_ohci_gtd_s *td;
  uintptr_t physaddr;
  int ret  = -ENOMEM;

  /* Sanity check.  NOTE that this method should only be called if a device is
   * connected (because we need a valid low speed indication).
   */

  DEBUGASSERT(rhport && epdesc && ep && rhport->connected);

  /* Allocate a container for the endpoint data */

  eplist = (struct bcm4390x_ohci_eplist_s *)kmm_zalloc(sizeof(struct bcm4390x_ohci_eplist_s));
  if (!eplist)
    {
      usbhost_trace1(OHCI_TRACE1_EPLISTALLOC_FAILED, 0);
      goto errout;
    }

  /* Initialize the endpoint container */

  sem_init(&eplist->wdhsem, 0, 0);

  /* We must have exclusive access to the ED pool, the bulk list, the periodic list
   * and the interrupt table.
   */

  bcm4390x_takesem(&g_ohci.exclsem);

  /* Allocate an ED and a tail TD for the new endpoint */

  ed = bcm4390x_ochi_edalloc();
  if (!ed)
    {
      usbhost_trace1(OHCI_TRACE1_EDALLOC_FAILED, 0);
      goto errout_with_semaphore;
    }

  td = bcm4390x_ohci_tdalloc();
  if (!td)
    {
      usbhost_trace1(OHCI_TRACE1_TDALLOC_FAILED, 0);
      goto errout_with_ed;
    }

  /* Save the descriptors in the endpoint container */

  eplist->ed   = ed;
  eplist->tail = td;

  /* Configure the endpoint descriptor. */

  memset((void*)ed, 0, sizeof(struct bcm4390x_ohci_ed_s));
  ed->hw.ctrl = (uint32_t)(epdesc->funcaddr)     << ED_CONTROL_FA_SHIFT |
                (uint32_t)(epdesc->addr)         << ED_CONTROL_EN_SHIFT |
                (uint32_t)(epdesc->mxpacketsize) << ED_CONTROL_MPS_SHIFT;

  /* Get the direction of the endpoint */

  if (epdesc->in)
    {
      ed->hw.ctrl |= ED_CONTROL_D_IN;
    }
  else
    {
      ed->hw.ctrl |= ED_CONTROL_D_OUT;
    }

  /* Check for a low-speed device */

  if (rhport->lowspeed)
    {
      ed->hw.ctrl |= ED_CONTROL_S;
    }

  /* Set the transfer type */

  ed->xfrtype = epdesc->xfrtype;

  /* Special Case isochronous transfer types */

#if 0 /* Isochronous transfers not yet supported */
  if (ed->xfrtype == USB_EP_ATTR_XFER_ISOC)
    {
      ed->hw.ctrl |= ED_CONTROL_F;
    }
#endif

  ed->eplist = eplist;
  usbhost_vtrace2(OHCI_VTRACE2_EPALLOC, epdesc->addr, (uint16_t)ed->hw.ctrl);

  /* Configure the tail descriptor. */

  memset(td, 0, sizeof(struct bcm4390x_ohci_gtd_s));
  td->ed = ed;

  /* Link the tail TD to the ED's TD list */

  physaddr     = (uintptr_t)bcm4390x_physramaddr((uintptr_t)td);
  ed->hw.headp = physaddr;
  ed->hw.tailp = physaddr;

  /* Make sure these settings are flushed to RAM */

  arch_clean_dcache((uintptr_t)ed,
                    (uintptr_t)ed + sizeof(struct ohci_ed_s));
  arch_clean_dcache((uintptr_t)td,
                    (uintptr_t)td + sizeof(struct ohci_gtd_s));

  /* Now add the endpoint descriptor to the appropriate list */
  dbg("Add ED , type: 0x%x\n", ed->xfrtype);
  switch (ed->xfrtype)
    {
    case USB_EP_ATTR_XFER_BULK:
      ret = bcm4390x_ohci_addbulked(ed);
      break;

    case USB_EP_ATTR_XFER_INT:
      ret = bcm4390x_ohci_addinted(epdesc, ed);
      break;

    case USB_EP_ATTR_XFER_ISOC:
      ret = bcm4390x_ohci_addisoced(epdesc, ed);
      break;

    case USB_EP_ATTR_XFER_CONTROL:
    default:
      ret = -EINVAL;
      break;
    }

  /* Was the ED successfully added? */

  if (ret != OK)
    {
      /* No.. destroy it and report the error */

      usbhost_trace2(OHCI_TRACE2_EDENQUEUE_FAILED, ed->xfrtype, -ret);
      goto errout_with_td;
    }

  /* Success.. return an opaque reference to the endpoint list container */

  *ep = (usbhost_ep_t)eplist;
  bcm4390x_givesem(&g_ohci.exclsem);
  return OK;

errout_with_td:
  bcm4390x_ohci_tdfree(td);
errout_with_ed:
  bcm4390x_ohci_edfree(ed);
errout_with_semaphore:
  bcm4390x_givesem(&g_ohci.exclsem);
  kmm_free(eplist);
errout:
  return ret;
}

/************************************************************************************
 * Name: bcm4390x_ohci_epfree
 *
 * Description:
 *   Free and endpoint previously allocated by DRVR_EPALLOC.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   ep - The endpoint to be freed.
 *
 * Returned Values:
 *   On success, zero (OK) is returned. On a failure, a negated errno value is
 *   returned indicating the nature of the failure
 *
 * Assumptions:
 *   This function will *not* be called from an interrupt handler.
 *
 ************************************************************************************/

static int bcm4390x_ohci_epfree(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep)
{
#ifdef CONFIG_DEBUG
  struct bcm4390x_ohci_rhport_s *rhport = (struct bcm4390x_ohci_rhport_s *)drvr;
#endif
  struct bcm4390x_ohci_eplist_s *eplist = (struct bcm4390x_ohci_eplist_s *)ep;
  struct bcm4390x_ohci_ed_s *ed;
  int ret;

  DEBUGASSERT(rhport && eplist && eplist->ed && eplist->tail);

  /* There should not be any pending, real TDs linked to this ED */

  ed = eplist->ed;
  DEBUGASSERT((ed->hw.headp & ED_HEADP_ADDR_MASK) == ed->hw.tailp);

  /* We must have exclusive access to the ED pool, the bulk list, the periodic list
   * and the interrupt table.
   */

  bcm4390x_takesem(&g_ohci.exclsem);

  /* Remove the ED to the correct list depending on the transfer type */

  switch (ed->xfrtype)
    {
    case USB_EP_ATTR_XFER_BULK:
      ret = bcm4390x_ohci_rembulked(eplist->ed);
      break;

    case USB_EP_ATTR_XFER_INT:
      ret = bcm4390x_ohci_reminted(eplist->ed);
      break;

    case USB_EP_ATTR_XFER_ISOC:
      ret = bcm4390x_ohci_remisoced(eplist->ed);
      break;

    case USB_EP_ATTR_XFER_CONTROL:
    default:
      ret = -EINVAL;
      break;
    }

  /* Put the ED and tail TDs back into the free list */

  bcm4390x_ohci_edfree(eplist->ed);
  bcm4390x_ohci_tdfree(eplist->tail);

  /* And free the container */

  sem_destroy(&eplist->wdhsem);
  kmm_free(eplist);
  bcm4390x_givesem(&g_ohci.exclsem);
  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_alloc
 *
 * Description:
 *   Some hardware supports special memory in which request and descriptor data can
 *   be accessed more efficiently.  This method provides a mechanism to allocate
 *   the request/descriptor memory.  If the underlying hardware does not support
 *   such "special" memory, this functions may simply map to kmm_malloc.
 *
 *   This interface was optimized under a particular assumption.  It was assumed
 *   that the driver maintains a pool of small, pre-allocated buffers for descriptor
 *   traffic.  NOTE that size is not an input, but an output:  The size of the
 *   pre-allocated buffer is returned.
 *
 * Input Parameters:
 *   drvr - The USB host driver instance obtained as a parameter from the call to
 *      the class create() method.
 *   buffer - The address of a memory location provided by the caller in which to
 *     return the allocated buffer memory address.
 *   maxlen - The address of a memory location provided by the caller in which to
 *     return the maximum size of the allocated buffer memory.
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

static int bcm4390x_ohci_alloc(FAR struct usbhost_driver_s *drvr,
                     FAR uint8_t **buffer, FAR size_t *maxlen)
{
  int ret = -ENOMEM;
  DEBUGASSERT(drvr && buffer && maxlen);

  /* We must have exclusive access to the transfer buffer pool */

  bcm4390x_takesem(&g_ohci.exclsem);

  *buffer = bcm4390x_ohci_tballoc();
  if (*buffer)
    {
      *maxlen = CONFIG_BCM4390X_OHCI_TDBUFSIZE;
      ret = OK;
    }

  bcm4390x_givesem(&g_ohci.exclsem);
  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_free
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

static int bcm4390x_ohci_free(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer)
{
  DEBUGASSERT(drvr && buffer);

  /* We must have exclusive access to the transfer buffer pool */

  bcm4390x_takesem(&g_ohci.exclsem);
  bcm4390x_ohci_tbfree(buffer);
  bcm4390x_givesem(&g_ohci.exclsem);
  return OK;
}

/************************************************************************************
 * Name: bcm4390x_ohci_ioalloc
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

static int bcm4390x_ohci_ioalloc(FAR struct usbhost_driver_s *drvr, FAR uint8_t **buffer,
                       size_t buflen)
{
  DEBUGASSERT(drvr && buffer);

  /* kumm_malloc() should return user accessible, DMA-able memory */

  *buffer = kumm_malloc(buflen);
  return *buffer ? OK : -ENOMEM;
}

/************************************************************************************
 * Name: bcm4390x_ohci_iofree
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

static int bcm4390x_ohci_iofree(FAR struct usbhost_driver_s *drvr, FAR uint8_t *buffer)
{
  DEBUGASSERT(drvr && buffer);

  /* kumm_free is all that is required */

  kumm_free(buffer);
  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_ctrlin and bcm4390x_ohci_ctrlout
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

static int bcm4390x_ohci_ctrlin(FAR struct usbhost_driver_s *drvr,
                      FAR const struct usb_ctrlreq_s *req,
                      FAR uint8_t *buffer)
{
  struct bcm4390x_ohci_rhport_s *rhport = (struct bcm4390x_ohci_rhport_s *)drvr;
  uint16_t len;
  int  ret;

  DEBUGASSERT(rhport && req);

#ifdef CONFIG_USBHOST_TRACE
  usbhost_vtrace2(OHCI_VTRACE2_CTRLIN, rhport->rhpndx + 1, req->req);
#else
  uvdbg("RHPort%d type: %02x req: %02x value: %02x%02x index: %02x%02x len: %02x%02x\n",
        rhport->rhpndx + 1, req->type, req->req, req->value[1], req->value[0],
        req->index[1], req->index[0], req->len[1], req->len[0]);
#endif

  /* We must have exclusive access to EP0 and the control list */

  bcm4390x_takesem(&g_ohci.exclsem);

  len = bcm4390x_getle16(req->len);
  ret = bcm4390x_ohci_ctrltd(rhport, GTD_STATUS_DP_SETUP, (uint8_t*)req, USB_SIZEOF_CTRLREQ);
  if (ret == OK)
    {
      if (len)
        {
          ret = bcm4390x_ohci_ctrltd(rhport, GTD_STATUS_DP_IN, buffer, len);
        }

      if (ret == OK)
        {
          ret = bcm4390x_ohci_ctrltd(rhport, GTD_STATUS_DP_OUT, NULL, 0);
        }
    }

  /* On an IN transaction, we need to invalidate the buffer contents to force
   * it to be reloaded from RAM after the DMA.
   */

  bcm4390x_givesem(&g_ohci.exclsem);
  arch_invalidate_dcache((uintptr_t)buffer, (uintptr_t)buffer + len);
  return ret;
}

static int bcm4390x_ohci_ctrlout(FAR struct usbhost_driver_s *drvr,
                       FAR const struct usb_ctrlreq_s *req,
                       FAR const uint8_t *buffer)
{
  struct bcm4390x_ohci_rhport_s *rhport = (struct bcm4390x_ohci_rhport_s *)drvr;
  uint16_t len;
  int ret;

  DEBUGASSERT(rhport && req);

#ifdef CONFIG_USBHOST_TRACE
  usbhost_vtrace2(OHCI_VTRACE2_CTRLOUT, rhport->rhpndx + 1, req->req);
#else
  uvdbg("RHPort%d type: %02x req: %02x value: %02x%02x index: %02x%02x len: %02x%02x\n",
        rhport->rhpndx + 1, req->type, req->req, req->value[1], req->value[0],
        req->index[1], req->index[0], req->len[1], req->len[0]);
#endif

  /* We must have exclusive access to EP0 and the control list */

  bcm4390x_takesem(&g_ohci.exclsem);

  len = bcm4390x_getle16(req->len);
  ret = bcm4390x_ohci_ctrltd(rhport, GTD_STATUS_DP_SETUP, (uint8_t*)req, USB_SIZEOF_CTRLREQ);
  if (ret == OK)
    {
      if (len)
        {
          ret = bcm4390x_ohci_ctrltd(rhport, GTD_STATUS_DP_OUT, (uint8_t*)buffer, len);
        }

      if (ret == OK)
        {
          ret = bcm4390x_ohci_ctrltd(rhport, GTD_STATUS_DP_IN, NULL, 0);
        }
    }

  bcm4390x_givesem(&g_ohci.exclsem);
  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_transfer
 *
 * Description:
 *   Process a request to handle a transfer descriptor.  This method will
 *   enqueue the transfer request and return immediately.  Only one transfer may be
 *   queued; Neither this method nor the ctrlin or ctrlout methods can be called
 *   again until this function returns.
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

static int bcm4390x_ohci_transfer(FAR struct usbhost_driver_s *drvr, usbhost_ep_t ep,
                        FAR uint8_t *buffer, size_t buflen)
{
  struct bcm4390x_ohci_rhport_s *rhport = (struct bcm4390x_ohci_rhport_s *)drvr;
  struct bcm4390x_ohci_eplist_s *eplist = (struct bcm4390x_ohci_eplist_s *)ep;
  struct bcm4390x_ohci_ed_s *ed;
  uint32_t dirpid;
  uint32_t regval;
  bool in;
  int ret;

  DEBUGASSERT(rhport && eplist && eplist->ed && eplist->tail &&
              buffer && buflen > 0);

  ed = eplist->ed;
  in = (ed->hw.ctrl  & ED_CONTROL_D_MASK) == ED_CONTROL_D_IN;

#ifdef CONFIG_USBHOST_TRACE
  usbhost_vtrace2(OHCI_VTRACE2_TRANSFER,
                  (ed->hw.ctrl  & ED_CONTROL_EN_MASK) >> ED_CONTROL_EN_SHIFT,
                  (uint16_t)buflen);
#else
  uvdbg("EP%d %s toggle: %d maxpacket: %d buflen: %d\n",
        (ed->hw.ctrl  & ED_CONTROL_EN_MASK) >> ED_CONTROL_EN_SHIFT,
        in ? "IN" : "OUT",
        (ed->hw.headp & ED_HEADP_C) != 0 ? 1 : 0,
        (ed->hw.ctrl  & ED_CONTROL_MPS_MASK) >> ED_CONTROL_MPS_SHIFT,
        buflen);
#endif

  /* We must have exclusive access to the endpoint, the TD pool, the I/O buffer
   * pool, the bulk and interrupt lists, and the HCCA interrupt table.
   */

  bcm4390x_takesem(&g_ohci.exclsem);

  /* Set the request for the Writeback Done Head event well BEFORE enabling the
   * transfer.
   */

  ret = bcm4390x_ohci_wdhwait(rhport, ed);
  if (ret != OK)
    {
      usbhost_trace1(OHCI_TRACE1_DEVDISCONN, rhport->rhpndx + 1);
      goto errout;
    }

  /* Get the direction of the endpoint */

  if (in)
    {
      dirpid = GTD_STATUS_DP_IN;
    }
  else
    {
      dirpid = GTD_STATUS_DP_OUT;
    }

  /* Then enqueue the transfer */

  ed->tdstatus = TD_CC_NOERROR;
  ret = bcm4390x_ohci_enqueuetd(rhport, ed, dirpid, GTD_STATUS_T_TOGGLE,
                      buffer, buflen);
  if (ret == OK)
    {
      /* BulkListFilled. This bit is used to indicate whether there are any
       * TDs on the Bulk list.
       */

      if (ed->xfrtype == USB_EP_ATTR_XFER_BULK)
        {
          regval  = bcm4390x_getreg(BCM4390X_OHCI_CMDST);
          regval |= OHCI_CMDST_BLF;
          bcm4390x_putreg(regval, BCM4390X_OHCI_CMDST);
        }

      /* Release the OHCI semaphore while we wait.  Other threads need the
       * opportunity to access the EHCI resources while we wait.
       *
       * REVISIT:  Is this safe?  NO.  This is a bug and needs rethinking.
       * We need to lock all of the port-resources (not EHCI common) until
       * the transfer is complete.  But we can't use the common OHCI exclsem
       * or we will deadlock while waiting (because the working thread that
       * wakes this thread up needs the exclsem).
       */
      bcm4390x_givesem(&g_ohci.exclsem);

      /* Wait for the Writeback Done Head interrupt  Loop to handle any false
       * alarm semaphore counts.
       */

      while (eplist->wdhwait)
        {
          bcm4390x_takesem(&eplist->wdhsem);
        }

      /* Re-aquire the ECHI semaphore.  The caller expects to be holding
       * this upon return.
       */

      bcm4390x_takesem(&g_ohci.exclsem);

      /* Invalidate the D cache to force the ED to be reloaded from RAM */

      arch_invalidate_dcache((uintptr_t)ed,
                             (uintptr_t)ed + sizeof(struct ohci_ed_s));

      /* Check the TD completion status bits */

      if (ed->tdstatus == TD_CC_NOERROR)
        {
          /* On an IN transaction, we also need to invalidate the buffer
           * contents to force it to be reloaded from RAM.
           */

          if (in)
            {
              arch_invalidate_dcache((uintptr_t)buffer,
                                     (uintptr_t)buffer + buflen);
            }

          ret = OK;
        }
      else
        {
          usbhost_trace2(OHCI_TRACE2_BADTDSTATUS, rhport->rhpndx + 1,
                         ed->tdstatus);
          ret = ed->tdstatus == TD_CC_STALL ? -EPERM : -EIO;
        }
    }

errout:
  /* Make sure that there is no outstanding request on this endpoint */

  eplist->wdhwait = false;
  bcm4390x_givesem(&g_ohci.exclsem);
  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_disconnect
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

static void bcm4390x_ohci_disconnect(FAR struct usbhost_driver_s *drvr)
{
  struct bcm4390x_ohci_rhport_s *rhport = (struct bcm4390x_ohci_rhport_s *)drvr;
  DEBUGASSERT(rhport);

  /* Remove the disconnected port from the control list */

  bcm4390x_ohci_ep0dequeue(rhport);
  rhport->ep0init = false;

  /* Unbind the class */

  rhport->class = NULL;
}

/*******************************************************************************
 * Public Functions
 *******************************************************************************/

/*******************************************************************************
 * Name: bcm4390x_ohci_initialize
 *
 * Description:
 *   Initialize USB OHCI host controller hardware.
 *
 * Input Parameters:
 *   controller -- If the device supports more than one OHCI interface, then
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

FAR struct usbhost_connection_s *bcm4390x_ohci_initialize( void )
{
  uintptr_t physaddr;
  uint32_t regval;
  uint8_t *buffer;
  irqstate_t flags;
  int i, hc_rh_des_a , hc_rh_des_b, port_num, port_status;

  /* One time sanity checks */

//  DEBUGASSERT(controller == 0);
  DEBUGASSERT(sizeof(struct bcm4390x_ohci_ed_s)  == SIZEOF_BCM4390X_OHCI_ED_S);
  DEBUGASSERT(sizeof(struct bcm4390x_ohci_gtd_s) == SIZEOF_BCM4390X_OHCI_TD_S);

  /* Initialize the state data structure */

  sem_init(&g_ohci.rhssem,  0, 0);
  sem_init(&g_ohci.exclsem, 0, 1);

#ifndef CONFIG_USBHOST_INT_DISABLE
  g_ohci.ininterval  = MAX_PERINTERVAL;
  g_ohci.outinterval = MAX_PERINTERVAL;
#endif

  /* For OHCI Full-speed operations only, the user has to perform the
   * following:
   *
   *   1) Enable UHP peripheral clock, bit (1 << AT91C_ID_UHPHS) in PMC_PCER
   *      register.
   *   2) Select PLLACK as Input clock of OHCI part, USBS bit in PMC_USB
   *      register.
   *   3) Program the OHCI clocks (UHP48M and UHP12M) with USBDIV field in
   *      PMC_USB register. USBDIV value is calculated regarding the PLLACK
   *      value and USB Full-speed accuracy.
   *   4) Enable the OHCI clocks, UHP bit in PMC_SCER register.
   *
   * Steps 1 and 4 are done here.  2 and 3 are already performed by
   * sam_clockconfig().
   */

  /* Enable UHP peripheral clocking */

  flags   = irqsave();
  /* "One transceiver is shared with the USB High Speed Device (port A). The
   *  selection between Host Port A and USB Device is controlled by the UDPHS
   *  enable bit (EN_UDPHS) located in the UDPHS_CTRL control register."
   */

  irqrestore(flags);

  /* Note that no pin configuration is required.  All USB HS pins have
   * dedicated function
   */

  usbhost_vtrace1(OHCI_VTRACE1_INITIALIZING, 0);

  /* Initialize all the HCCA to 0 */

  memset((void*)&g_hcca, 0, sizeof(struct ohci_hcca_s));

  arch_clean_dcache((uint32_t)&g_hcca,
                    (uint32_t)&g_hcca + sizeof(struct ohci_hcca_s));

  /* Initialize user-configurable EDs */

  for (i = 0; i < BCM4390X_OHCI_NEDS; i++)
    {
      /* Put the ED in a free list */

      bcm4390x_ohci_edfree(&g_edalloc[i]);
    }

  /* Initialize user-configurable TDs */

  for (i = 0; i < BCM4390X_OHCI_NTDS; i++)
    {
      /* Put the TD in a free list */

      bcm4390x_ohci_tdfree(&g_tdalloc[i]);
    }

  /* Initialize user-configurable request/descriptor transfer buffers */

  buffer = g_bufalloc;
  for (i = 0; i < CONFIG_BCM4390X_OHCI_TDBUFFERS; i++)
    {
      /* Put the TD buffer in a free list */

      bcm4390x_ohci_tbfree(buffer);
      buffer += CONFIG_BCM4390X_OHCI_TDBUFSIZE;
    }

  /* Initialize the root hub port structures */

  for (i = 0; i < BCM4390X_OHCI_NRHPORT; i++)
    {
      struct bcm4390x_ohci_rhport_s *rhport = &g_ohci.rhport[i];

      rhport->rhpndx              = i;
      rhport->drvr.ep0configure   = bcm4390x_ohci_ep0configure;
      rhport->drvr.getdevinfo     = bcm4390x_ohci_getdevinfo;
      rhport->drvr.epalloc        = bcm4390x_ohci_epalloc;
      rhport->drvr.epfree         = bcm4390x_ohci_epfree;
      rhport->drvr.alloc          = bcm4390x_ohci_alloc;
      rhport->drvr.free           = bcm4390x_ohci_free;
      rhport->drvr.ioalloc        = bcm4390x_ohci_ioalloc;
      rhport->drvr.iofree         = bcm4390x_ohci_iofree;
      rhport->drvr.ctrlin         = bcm4390x_ohci_ctrlin;
      rhport->drvr.ctrlout        = bcm4390x_ohci_ctrlout;
      rhport->drvr.transfer       = bcm4390x_ohci_transfer;
      rhport->drvr.disconnect     = bcm4390x_ohci_disconnect;
    }

  /* Wait 50MS then perform hardware reset */

  up_mdelay(50);

  bcm4390x_putreg(0, BCM4390X_OHCI_CTRL);        /* Hardware reset */
  bcm4390x_putreg(0, BCM4390X_OHCI_CTRLHEADED);  /* Initialize control list head to Zero */
  bcm4390x_putreg(0, BCM4390X_OHCI_BULKHEADED);  /* Initialize bulk list head to Zero */

  /* Software reset */

  bcm4390x_putreg(OHCI_CMDST_HCR, BCM4390X_OHCI_CMDST);

  /* Write Fm interval (FI), largest data packet counter (FSMPS), and
   * periodic start.
   */

  bcm4390x_putreg(DEFAULT_FMINTERVAL, BCM4390X_OHCI_FMINT);
  bcm4390x_putreg(DEFAULT_PERSTART, BCM4390X_OHCI_PERSTART);

  /* Put HC in operational state */

  regval  = bcm4390x_getreg(BCM4390X_OHCI_CTRL);
  regval &= ~OHCI_CTRL_HCFS_MASK;
  regval |= OHCI_CTRL_HCFS_OPER;
  bcm4390x_putreg(regval, BCM4390X_OHCI_CTRL);

  /* Set global power in HcRhStatus */


  hc_rh_des_a = bcm4390x_getreg(BCM4390X_OHCI_RHDESCA);
  hc_rh_des_b = bcm4390x_getreg(BCM4390X_OHCI_RHDESCB);

  /* Po wer switching is allowed */
  if (! (hc_rh_des_a & OHCI_RHDESCA_NPS))
  {
      port_num = hc_rh_des_a & OHCI_RHDESCA_NDP_MASK;

      /* Each port is powered individually */
      if (hc_rh_des_a & OHCI_RHDESCA_PSM)
      {
          for (i = 0 ; i < port_num; i ++)
          {
              /* Port power is controlled by gloabal power switch */
              if (((hc_rh_des_b >> OHCI_RHDESCB_PPCM_SHIFT ) & (1 << (i+1 ))) == 0)
              {
                  bcm4390x_putreg(OHCI_RHSTATUS_SGP, BCM4390X_OHCI_RHSTATUS);
//                  break;
              }
              /* Port power is controlled by per-port power control*/
              else {
                  port_status = bcm4390x_getreg(BCM4390X_OHCI_RHPORTST(i + 1));
                  port_status |= OHCI_RHPORTST_PPS;
                  bcm4390x_putreg(port_status, BCM4390X_OHCI_RHPORTST(i + 1));
              }
          }

      }
      /* All ports are powered at the same time */
      else
      {
          dbg("All port are powered at the same time\n");
          bcm4390x_putreg(OHCI_RHSTATUS_SGP, BCM4390X_OHCI_RHSTATUS);
      }

  }


  /* Set HCCA base address */

  physaddr = bcm4390x_physramaddr((uintptr_t)&g_hcca);
  bcm4390x_putreg(physaddr, BCM4390X_OHCI_HCCA);

  /* Clear pending interrupts */

  regval = bcm4390x_getreg(BCM4390X_OHCI_INTST);
  bcm4390x_putreg(regval, BCM4390X_OHCI_INTST);

  /* Enable OHCI interrupts */

  bcm4390x_putreg((BCM4390X_ALL_INTS | OHCI_INT_MIE), BCM4390X_OHCI_INTEN);

  usbhost_vtrace1(OHCI_VTRACE1_INITIALIZED, 0);

  /* Initialize and return the connection interface */

  g_ohciconn.wait      = bcm4390x_ohci_wait;
  g_ohciconn.enumerate = bcm4390x_ohci_enumerate;
  return &g_ohciconn;
}

/*******************************************************************************
 * Name: bcm4390x_ohci_isr
 *
 * Description:
 *   OHCI "Top Half" interrupt handler.  If both EHCI and OHCI are enabled, then
 *   EHCI will manage the common UHPHS interrupt and will forward the interrupt
 *   event to this function.
 *
 *******************************************************************************/

int bcm4390x_ohci_isr(int irq, FAR void *context)
{
  uint32_t intst;
  uint32_t inten;
  uint32_t pending;

  /* Read Interrupt Status and mask out interrupts that are not enabled. */

  intst = bcm4390x_getreg(BCM4390X_OHCI_INTST);
  inten = bcm4390x_getreg(BCM4390X_OHCI_INTEN);
  usbhost_vtrace1(OHCI_VTRACE1_INTRPENDING, intst & inten);

#ifdef CONFIG_BCM4390X_EHCI

  /* Check the Master Interrupt Enable bit (MIE).  It this function is called
   * from the common UHPHS interrupt handler, there might be pending interrupts
   * but with the overall interstate disabled.  This could never happen if only
   * OHCI were enabled because we would never get here.
   */

  if ((inten & OHCI_INT_MIE) != 0)
#endif
    {
      /* Mask out the interrupts that are not enabled */
      pending = intst & inten;
      if (pending != 0)
        {
          /* Schedule interrupt handling work for the high priority worker
           * thread so that we are not pressed for time and so that we can
           * interrupt with other USB threads gracefully.
           *
           * The worker should be available now because we implement a
           * handshake by controlling the OHCI interrupts.
           */

          DEBUGASSERT(work_available(&g_ohci.work));
          DEBUGVERIFY(work_queue(HPWORK, &g_ohci.work, bcm4390x_ohci_bottomhalf,
                                 (FAR void *)pending, 0));

          /* Disable further OHCI interrupts so that we do not overrun the
           * work queue.
           */

          bcm4390x_putreg(OHCI_INT_MIE, BCM4390X_OHCI_INTDIS);

          /* Clear all pending status bits by writing the value of the
           * pending interrupt bits back to the status register.
           */

          bcm4390x_putreg(intst, BCM4390X_OHCI_INTST);
        }
    }

  return OK;
}

#endif /* CONFIG_BCM4390X_OHCI */
