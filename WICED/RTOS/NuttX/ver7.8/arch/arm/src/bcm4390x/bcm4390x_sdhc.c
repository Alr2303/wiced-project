/*
 * $ Copyright Broadcom Corporation $
 */

/****************************************************************************
 *
 *   Copyright (C) 2011-2012, 2014 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#define CONFIG_SDIO_DMA
#define CONFIG_SDIO_ADMA2

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <string.h>
#include <assert.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/wdog.h>
#include <nuttx/clock.h>
#include <nuttx/arch.h>
#include <nuttx/sdio.h>
#include <nuttx/wqueue.h>
#include <nuttx/mmcsd.h>
#include <nuttx/irq.h>
#include <arch/board/board.h>
#include "chip.h"
#include "up_arch.h"
#include <typedefs.h>
#include <bcmutils.h>
#include <osl.h>
#include <sdioh.h>
#include <hndsoc.h>
#include <sdioh.h>
#include "bcm4390x_sdhc.h"
#include "platform_appscr4.h"
#include <platform_mcu_peripheral.h>

#define enter_critical_section irqsave
#define leave_critical_section irqrestore

#ifdef CONFIG_BCM4390X_SDHC

#ifdef CONFIG_SDIO_DMA

/* System will crash if calling DMA_MAP! WAR is using a DMA buffer for tx/rx and do
 * copy between DMA buffer and application buffer
 */
#define WAR_USE_DMA_MEMORY

#ifdef WAR_USE_DMA_MEMORY
#define MAX_MIDDLE_DMA_SIZE     32768
void *p_dmabuf_va_start;
ulong dmabuf_phy_start;
void *p_dmabuf_va;
ulong dmabuf_phy;
uint32_t dmabuf_len;
#endif

#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#if !defined(CONFIG_SCHED_WORKQUEUE) || !defined(CONFIG_SCHED_HPWORK)
#error "Callback support requires CONFIG_SCHED_WORKQUEUE and CONFIG_SCHED_HPWORK"
#endif

#ifndef CONFIG_BCM4390X_SDHC_PRIO
#define CONFIG_BCM4390X_SDHC_PRIO NVIC_SYSH_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_BCM4390X_SDHC_DMAPRIO
#define CONFIG_BCM4390X_SDHC_DMAPRIO DMA_CCR_PRIMED
#endif

#if !defined(CONFIG_DEBUG_FS) || !defined(CONFIG_DEBUG_VERBOSE)
#undef CONFIG_SDIO_XFRDEBUG
#endif

/* SDCLK frequencies corresponding to various modes of operation.  These
 * values may be provided in either the NuttX configuration file or in
 * the board.h file
 */

#ifndef CONFIG_BCM4390X_IDMODE_FREQ
#define CONFIG_BCM4390X_IDMODE_FREQ 400000    /* 400 KHz, ID mode */
#endif
#ifndef CONFIG_BCM4390X_MMCXFR_FREQ
#define CONFIG_BCM4390X_MMCXFR_FREQ 20000000  /* 20MHz MMC, normal clocking */
#endif
#ifndef CONFIG_BCM4390X_SD1BIT_FREQ
#define CONFIG_BCM4390X_SD1BIT_FREQ 20000000  /* 20MHz SD 1-bit, normal clocking */
#endif
#ifndef CONFIG_BCM4390X_SD4BIT_FREQ
#define CONFIG_BCM4390X_SD4BIT_FREQ 25000000  /* 25MHz SD 4-bit, normal clocking */
#endif


/* Timing */

#define SDHC_CMDTIMEOUT         (100000)
#define SDHC_LONGTIMEOUT        (0x7fffffff)

/* Big DVS setting.  Range is 0=SDCLK*213 through 14=SDCLK*227 */

#define SDHC_DVS_MAXTIMEOUT     (14)
#define SDHC_DVS_DATATIMEOUT    (14)

/* Data transfer / Event waiting interrupt mask bits */

#define SDHC_RESPERR_INTS  (SDHC_INT_CCE|SDHC_INT_CTOE|SDHC_INT_CEBE|SDHC_INT_CIE)
#define SDHC_RESPDONE_INTS (SDHC_RESPERR_INTS|SDHC_INT_CC)

#define SCHC_XFRERR_INTS   (SDHC_INT_DCE|SDHC_INT_DTOE|SDHC_INT_DEBE)
#define SDHC_RCVDONE_INTS  (SCHC_XFRERR_INTS|SDHC_INT_BRR|SDHC_INT_TC)
#define SDHC_SNDDONE_INTS  (SCHC_XFRERR_INTS|SDHC_INT_BWR|SDHC_INT_TC)
#define SDHC_XFRDONE_INTS  (SCHC_XFRERR_INTS|SDHC_INT_BRR|SDHC_INT_BWR|SDHC_INT_TC)

#define SCHC_DMAERR_INTS   (SDHC_INT_DCE|SDHC_INT_DTOE|SDHC_INT_DEBE|SDHC_INT_DMAE)
#define SDHC_DMADONE_INTS  (SCHC_DMAERR_INTS|SDHC_INT_DINT|SDHC_INT_TC)

#define SDHC_WAITALL_INTS  (SDHC_RESPDONE_INTS|SDHC_XFRDONE_INTS|SDHC_DMADONE_INTS)

/* Register logging support */

#ifdef CONFIG_SDIO_XFRDEBUG
#define SAMPLENDX_BEFORE_SETUP  0
#define SAMPLENDX_AFTER_SETUP   1
#define SAMPLENDX_END_TRANSFER  2
#define DEBUG_NSAMPLES          3
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA
#define DMA_MODE_NONE   0
#define DMA_MODE_SDMA   1
#define DMA_MODE_ADMA1  2
#define DMA_MODE_ADMA2  3
#define DMA_MODE_ADMA2_64 4
#define DMA_MODE_AUTO   -1

#ifndef CONFIG_SDIO_ADMA2
#error We only support ADMA2 for now
#endif
#endif

struct bcm4390x_dev_s
{
  struct sdio_dev_s  dev;

  /* Cypress-specific extensions */
  /* Event support */

  sem_t              waitsem;    /* Implements event waiting */
  sdio_eventset_t    waitevents; /* Set of events to be waited for */
  uint32_t           waitints;   /* Interrupt enables for event waiting */
  volatile sdio_eventset_t wkupevent; /* The event that caused the wakeup */
  WDOG_ID            waitwdog;   /* Watchdog that handles event timeouts */

  /* Callback support */

  uint8_t            cdstatus;   /* Card status */
  sdio_eventset_t    cbevents;   /* Set of events to be cause callbacks */
  worker_t           callback;   /* Registered callback function */
  void              *cbarg;      /* Registered callback argument */
  struct work_s      cbwork;     /* Callback work queue structure */

  /* Interrupt mode data transfer support */

  uint32_t          *buffer;     /* Address of current R/W buffer */
  size_t             remaining;  /* Number of bytes remaining in the transfer */
  uint32_t           xfrints;    /* Interrupt enables for data transfer */

  /* DMA data transfer support */

#ifdef CONFIG_SDIO_DMA
  volatile uint8_t   xfrflags;    /* Used to synchronize SDIO and DMA completion events */

  void        *adma2_dscr_buf;    /* ADMA2 Descriptor Buffer virtual address */
  ulong       adma2_dscr_phys;    /* ADMA2 Descriptor Buffer physical address */
  void        *adma2_dscr_start_buf;
  ulong       adma2_dscr_start_phys;
  uint        alloced_adma2_dscr_size;
  uint        max_num_dma_dscr;
  uint8       sd_dma_mode;
#endif

  /* SDHC info */

  uint32_t      caps;        /* cached value of capabilities reg */
  uint32_t      caps3;
  uint32_t      max_blk_len;
  uint32_t      sd_base_clk;
  uint8_t       version;     /* Host Controller Spec Compliance Version */

  uint8_t       xfr_write;
};

/* Register logging support */

#ifdef CONFIG_SDIO_XFRDEBUG
struct bcm4390x_sdhcregs_s
{
  /* All read-able SDHC registers */

  uint32_t dsaddr;    /* DMA System Address Register */
  uint32_t blkattr;   /* Block Attributes Register */
  uint32_t cmdarg;    /* Command Argument Register */
  uint32_t xferty;    /* Transfer Type Register */
  uint32_t cmdrsp0;   /* Command Response 0 */
  uint32_t cmdrsp1;   /* Command Response 1 */
  uint32_t cmdrsp2;   /* Command Response 2 */
  uint32_t cmdrsp3;   /* Command Response 3 */
  uint32_t prsstat;   /* Present State Register */
  uint32_t proctl;    /* Protocol Control Register */
  uint32_t sysctl;    /* System Control Register */
  uint32_t irqstat;   /* Interrupt Status Register */
  uint32_t irqstaten; /* Interrupt Status Enable Register */
  uint32_t irqsigen;  /* Interrupt Signal Enable Register */
  uint32_t ac12err;   /* Auto CMD12 Error Status Register */
  uint32_t htcapblt;  /* Host Controller Capabilities */
  uint32_t cap3;      /* Capabilities of version 3 */
  uint32_t admaes;    /* ADMA Error Status Register */
  uint32_t adsaddr;   /* ADMA System Address Register */
  uint32_t vendor;    /* Vendor Specific Register */
  uint32_t mmcboot;   /* MMC Boot Register */
  uint32_t hostver;   /* Host Controller Version */
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Low-level helpers ********************************************************/

static void bcm4390x_takesem(struct bcm4390x_dev_s *priv);
#define     bcm4390x_givesem(priv) (sem_post(&priv->waitsem))
static void bcm4390x_configwaitints(struct bcm4390x_dev_s *priv, uint32_t waitints,
              sdio_eventset_t waitevents, sdio_eventset_t wkupevents);
static void bcm4390x_configxfrints(struct bcm4390x_dev_s *priv, uint32_t xfrints);

/* Debug Helpers ************************************************************/

#ifdef CONFIG_SDIO_XFRDEBUG
static void bcm4390x_sampleinit(void);
static void bcm4390x_sdhcsample(struct bcm4390x_sdhcregs_s *regs);
static void bcm4390x_sample(struct bcm4390x_dev_s *priv, int index);
static void bcm4390x_dumpsample(struct bcm4390x_dev_s *priv,
              struct bcm4390x_sdhcregs_s *regs, const char *msg);
static void bcm4390x_dumpsamples(struct bcm4390x_dev_s *priv);
static void bcm4390x_showregs(struct bcm4390x_dev_s *priv, const char *msg);
#else
#define   bcm4390x_sampleinit()
#define   bcm4390x_sample(priv,index)
#define   bcm4390x_dumpsamples(priv)
#define   bcm4390x_showregs(priv,msg)
#endif

/* Data Transfer Helpers ****************************************************/

static void bcm4390x_dataconfig(struct bcm4390x_dev_s *priv, bool bwrite,
                               unsigned int blocksize, unsigned int nblocks,
                               unsigned int timeout);
static void bcm4390x_datadisable(void);
#ifndef CONFIG_SDIO_DMA
static void bcm4390x_transmit(struct bcm4390x_dev_s *priv);
static void bcm4390x_receive(struct bcm4390x_dev_s *priv);
#endif
static void bcm4390x_eventtimeout(int argc, uint32_t arg);
static void bcm4390x_endwait(struct bcm4390x_dev_s *priv, sdio_eventset_t wkupevent);
static void bcm4390x_endtransfer(struct bcm4390x_dev_s *priv, sdio_eventset_t wkupevent);

/* Interrupt Handling *******************************************************/

static int  bcm4390x_interrupt(int irq, void *context);

/* SDIO interface methods ***************************************************/

/* Mutual exclusion */

#ifdef CONFIG_SDIO_MUXBUS
static int bcm4390x_lock(FAR struct sdio_dev_s *dev, bool lock);
#endif

/* Initialization/setup */

static void bcm4390x_reset(FAR struct sdio_dev_s *dev);
static uint8_t bcm4390x_status(FAR struct sdio_dev_s *dev);
static void bcm4390x_widebus(FAR struct sdio_dev_s *dev, bool enable);
static void bcm4390x_frequency(FAR struct sdio_dev_s *dev, uint32_t frequency);
static void bcm4390x_clock(FAR struct sdio_dev_s *dev,
              enum sdio_clock_e rate);
static int  bcm4390x_attach(FAR struct sdio_dev_s *dev);

/* Command/Status/Data Transfer */

static int  bcm4390x_sendcmd(FAR struct sdio_dev_s *dev, uint32_t cmd,
              uint32_t arg);
#ifndef CONFIG_SDIO_DMA
static int  bcm4390x_recvsetup(FAR struct sdio_dev_s *dev, FAR uint8_t *buffer,
              size_t nbytes);
static int  bcm4390x_sendsetup(FAR struct sdio_dev_s *dev,
              FAR const uint8_t *buffer, uint32_t nbytes);
#endif
static int  bcm4390x_cancel(FAR struct sdio_dev_s *dev);

static int  bcm4390x_waitresponse(FAR struct sdio_dev_s *dev, uint32_t cmd);
static int  bcm4390x_recvshortcrc(FAR struct sdio_dev_s *dev, uint32_t cmd,
              uint32_t *rshort);
static int  bcm4390x_recvlong(FAR struct sdio_dev_s *dev, uint32_t cmd,
              uint32_t rlong[4]);
static int  bcm4390x_recvshort(FAR struct sdio_dev_s *dev, uint32_t cmd,
              uint32_t *rshort);
static int  bcm4390x_recvnotimpl(FAR struct sdio_dev_s *dev, uint32_t cmd,
              uint32_t *rnotimpl);

/* EVENT handler */

static void bcm4390x_waitenable(FAR struct sdio_dev_s *dev,
              sdio_eventset_t eventset);
static sdio_eventset_t
            bcm4390x_eventwait(FAR struct sdio_dev_s *dev, uint32_t timeout);
static void bcm4390x_callbackenable(FAR struct sdio_dev_s *dev,
              sdio_eventset_t eventset);
static int  bcm4390x_registercallback(FAR struct sdio_dev_s *dev,
              worker_t callback, void *arg);

/* DMA */

#ifdef CONFIG_SDIO_DMA
static bool bcm4390x_dmasupported(FAR struct sdio_dev_s *dev);
static int  bcm4390x_dmarecvsetup(FAR struct sdio_dev_s *dev,
              FAR uint8_t *buffer, size_t buflen);
static int  bcm4390x_dmasendsetup(FAR struct sdio_dev_s *dev,
              FAR const uint8_t *buffer, size_t buflen);
#endif

/* Initialization/uninitialization/reset ************************************/

static void bcm4390x_callback(void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct bcm4390x_dev_s g_sdhcdev =
{
  .dev =
  {
#ifdef CONFIG_SDIO_MUXBUS
    .lock             = bcm4390x_lock,
#endif
    .reset            = bcm4390x_reset,
    .status           = bcm4390x_status,
    .widebus          = bcm4390x_widebus,
    .clock            = bcm4390x_clock,
    .attach           = bcm4390x_attach,
    .sendcmd          = bcm4390x_sendcmd,
#ifndef CONFIG_SDIO_DMA
    .recvsetup        = bcm4390x_recvsetup,
    .sendsetup        = bcm4390x_sendsetup,
#else
    .recvsetup        = bcm4390x_dmarecvsetup,
    .sendsetup        = bcm4390x_dmasendsetup,
#endif
    .cancel           = bcm4390x_cancel,
    .waitresponse     = bcm4390x_waitresponse,
    .recvR1           = bcm4390x_recvshortcrc,
    .recvR2           = bcm4390x_recvlong,
    .recvR3           = bcm4390x_recvshort,
    .recvR4           = bcm4390x_recvnotimpl,
    .recvR5           = bcm4390x_recvnotimpl,
    .recvR6           = bcm4390x_recvshortcrc,
    .recvR7           = bcm4390x_recvshort,
    .waitenable       = bcm4390x_waitenable,
    .eventwait        = bcm4390x_eventwait,
    .callbackenable   = bcm4390x_callbackenable,
    .registercallback = bcm4390x_registercallback,
#ifdef CONFIG_SDIO_DMA
    .dmasupported     = bcm4390x_dmasupported,
    .dmarecvsetup     = bcm4390x_dmarecvsetup,
    .dmasendsetup     = bcm4390x_dmasendsetup,
#endif
  },
};

/* Register logging support */

#ifdef CONFIG_SDIO_XFRDEBUG
static struct bcm4390x_sdhcregs_s g_sampleregs[DEBUG_NSAMPLES];
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Low-level Helpers
 ****************************************************************************/
/****************************************************************************
 * Name: bcm4390x_takesem
 *
 * Description:
 *   Take the wait semaphore (handling false alarm wakeups due to the receipt
 *   of signals).
 *
 * Input Parameters:
 *   dev - Instance of the SDIO device driver state structure.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_takesem(struct bcm4390x_dev_s *priv)
{
  /* Take the semaphore (perhaps waiting) */

  while (sem_wait(&priv->waitsem) != 0)
    {
      /* The only case that an error should occr here is if the wait was
       * awakened by a signal.
       */

      ASSERT(errno == EINTR);
    }
}

/****************************************************************************
 * Name: bcm4390x_configwaitints
 *
 * Description:
 *   Enable/disable SDIO interrupts needed to suport the wait function
 *
 * Input Parameters:
 *   priv       - A reference to the SDIO device state structure
 *   waitints   - The set of bits in the SDIO MASK register to set
 *   waitevents - Waited for events
 *   wkupevent  - Wake-up events
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_configwaitints(struct bcm4390x_dev_s *priv, uint32_t waitints,
                                 sdio_eventset_t waitevents,
                                 sdio_eventset_t wkupevent)
{
  irqstate_t flags;

  /* Save all of the data and set the new interrupt mask in one, atomic
   * operation.
   */

  flags = enter_critical_section();
  priv->waitevents = waitevents;
  priv->wkupevent  = wkupevent;
  priv->waitints   = waitints;
#ifdef CONFIG_SDIO_DMA
  priv->xfrflags   = 0;
#endif
  putreg32(priv->xfrints | priv->waitints | SDHC_INT_CINT,
           BCM4390X_SDHC_IRQSIGEN);
  leave_critical_section(flags);
}

/****************************************************************************
 * Name: bcm4390x_configxfrints
 *
 * Description:
 *   Enable SDIO interrupts needed to support the data transfer event
 *
 * Input Parameters:
 *   priv    - A reference to the SDIO device state structure
 *   xfrints - The set of bits in the SDIO MASK register to set
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_configxfrints(struct bcm4390x_dev_s *priv, uint32_t xfrints)
{
  irqstate_t flags;
  flags = enter_critical_section();
  priv->xfrints = xfrints;

  putreg32(priv->xfrints | priv->waitints | SDHC_INT_CINT,
           BCM4390X_SDHC_IRQSIGEN);

  leave_critical_section(flags);
}

/****************************************************************************
 * DMA Helpers
 ****************************************************************************/

/****************************************************************************
 * Name: bcm4390x_sampleinit
 *
 * Description:
 *   Setup prior to collecting DMA samples
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_XFRDEBUG
static void bcm4390x_sampleinit(void)
{
    memset(g_sampleregs, 0xff, DEBUG_NSAMPLES * sizeof(struct bcm4390x_sdhcregs_s));
}
#endif

/****************************************************************************
 * Name: bcm4390x_sdhcsample
 *
 * Description:
 *   Sample SDIO registers
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_XFRDEBUG
static void bcm4390x_sdhcsample(struct bcm4390x_sdhcregs_s *regs)
{
    //regs->dsaddr    = getreg32(BCM4390X_SDHC_DSADDR);    /* DMA System Address Register */
    regs->blkattr   = getreg32(BCM4390X_SDHC_BLKATTR);   /* Block Attributes Register */
    regs->cmdarg    = getreg32(BCM4390X_SDHC_CMDARG);    /* Command Argument Register */
    regs->xferty    = getreg32(BCM4390X_SDHC_XFERTYP);   /* Transfer Type Register */
    //regs->cmdrsp0   = getreg32(BCM4390X_SDHC_CMDRSP0);   /* Command Response 0 */
    //regs->cmdrsp1   = getreg32(BCM4390X_SDHC_CMDRSP1);   /* Command Response 1 */
    //regs->cmdrsp2   = getreg32(BCM4390X_SDHC_CMDRSP2);   /* Command Response 2 */
    //regs->cmdrsp3   = getreg32(BCM4390X_SDHC_CMDRSP3);   /* Command Response 3 */
    regs->prsstat   = getreg32(BCM4390X_SDHC_PRSSTAT);   /* Present State Register */
    regs->proctl    = getreg32(BCM4390X_SDHC_PROCTL);    /* Protocol Control Register */
    regs->sysctl    = getreg32(BCM4390X_SDHC_SYSCTL);    /* System Control Register */
    regs->irqstat   = getreg32(BCM4390X_SDHC_IRQSTAT);   /* Interrupt Status Register */
    regs->irqstaten = getreg32(BCM4390X_SDHC_IRQSTATEN); /* Interrupt Status Enable Register */
    regs->irqsigen  = getreg32(BCM4390X_SDHC_IRQSIGEN);  /* Interrupt Signal Enable Register */
    regs->ac12err   = getreg32(BCM4390X_SDHC_AC12ERR);   /* Auto CMD12 Error Status Register */
    regs->htcapblt  = getreg32(BCM4390X_SDHC_HTCAPBLT);  /* Host Controller Capabilities */
    regs->cap3      = getreg32(BCM4390X_SDHC_HTCAPBLT3_OFFSET);       /* Cap Info Register */
    regs->hostver   = getreg32(BCM4390X_SDHC_HOSTVER);   /* Host Controller Version */
}
#endif

/****************************************************************************
 * Name: bcm4390x_sample
 *
 * Description:
 *   Sample SDIO/DMA registers
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_XFRDEBUG
static void bcm4390x_sample(struct bcm4390x_dev_s *priv, int index)
{
  bcm4390x_sdhcsample(&g_sampleregs[index]);
}
#endif

/****************************************************************************
 * Name: bcm4390x_dumpsample
 *
 * Description:
 *   Dump one register sample
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_XFRDEBUG
static void bcm4390x_dumpsample(struct bcm4390x_dev_s *priv,
                               struct bcm4390x_sdhcregs_s *regs, const char *msg)
{
  dbg("SDHC Registers: %s\n", msg);
  dbg("  BLKATTR[%08x]: %08x\n", BCM4390X_SDHC_BLKATTR,   regs->blkattr);
  dbg("   CMDARG[%08x]: %08x\n", BCM4390X_SDHC_CMDARG,    regs->cmdarg);
  dbg("   XFERTY[%08x]: %08x\n", BCM4390X_SDHC_XFERTYP,   regs->xferty);
  dbg("  PRSSTAT[%08x]: %08x\n", BCM4390X_SDHC_PRSSTAT,   regs->prsstat);
  dbg("   PROCTL[%08x]: %08x\n", BCM4390X_SDHC_PROCTL,    regs->proctl);
  dbg("   SYSCTL[%08x]: %08x\n", BCM4390X_SDHC_SYSCTL,    regs->sysctl);
  dbg("  IRQSTAT[%08x]: %08x\n", BCM4390X_SDHC_IRQSTAT,   regs->irqstat);
  dbg("IRQSTATEN[%08x]: %08x\n", BCM4390X_SDHC_IRQSTATEN, regs->irqstaten);
  dbg(" IRQSIGEN[%08x]: %08x\n", BCM4390X_SDHC_IRQSIGEN,  regs->irqsigen);
  dbg("  AC12ERR[%08x]: %08x\n", BCM4390X_SDHC_AC12ERR,   regs->ac12err);
  dbg("      CAP[%08x]: %08x\n", BCM4390X_SDHC_HTCAPBLT,  regs->htcapblt);
  dbg("     CAP3[%08x]: %08x\n", BCM4390X_SDHC_HTCAPBLT3, regs->cap3);
  dbg("  HOSTVER[%08x]: %08x\n", BCM4390X_SDHC_HOSTVER,   regs->hostver);
}
#endif

/****************************************************************************
 * Name: bcm4390x_dumpsamples
 *
 * Description:
 *   Dump all sampled register data
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_XFRDEBUG
static void  bcm4390x_dumpsamples(struct bcm4390x_dev_s *priv)
{
    return;
  bcm4390x_dumpsample(priv, &g_sampleregs[SAMPLENDX_BEFORE_SETUP], "Before setup");
  bcm4390x_dumpsample(priv, &g_sampleregs[SAMPLENDX_AFTER_SETUP], "After setup");
  bcm4390x_dumpsample(priv, &g_sampleregs[SAMPLENDX_END_TRANSFER], "End of transfer");
}
#endif

/****************************************************************************
 * Name: bcm4390x_showregs
 *
 * Description:
 *   Dump the current state of all registers
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_XFRDEBUG
static void bcm4390x_showregs(struct bcm4390x_dev_s *priv, const char *msg)
{
  struct bcm4390x_sdhcregs_s regs;

  bcm4390x_sdhcsample(&regs);
  bcm4390x_dumpsample(priv, &regs, msg);
}
#endif

/****************************************************************************
 * Data Transfer Helpers
 ****************************************************************************/
#ifdef CONFIG_SDIO_DMA
static int bcm4390x_create_dma_desc(struct bcm4390x_dev_s *priv, FAR uint8_t *buffer, size_t buf_len,
        int direction)
{
#define ADMA2_MAX_LEN_OF_BUFFER     16384

    uint32_t num_dscr, idx;
    ulong buf_phy_addr;
    uint32_t length, remain_len;
    uint32_t flags;
    adma2_dscr_32b_t *adma2_dscr_table;

    if (priv->sd_dma_mode != DMA_MODE_ADMA2){
        dbg("ERROR: DMA mode %d is unsupported\n", priv->sd_dma_mode);
        return -ERROR;
    }

    num_dscr = buf_len / ADMA2_MAX_LEN_OF_BUFFER;
    num_dscr += (buf_len % ADMA2_MAX_LEN_OF_BUFFER ? 1 : 0);
    if (num_dscr > priv->max_num_dma_dscr) {
        dbg("ERROR: too many dma dscr (%d) requested! Max is %d\n", num_dscr, priv->max_num_dma_dscr);
        return -ERROR;
    }

#ifdef WAR_USE_DMA_MEMORY
    if (buf_len > dmabuf_len) {
        dbg("ERROR: not support buffer size %d bytes, (max is %d bytes)\n", buf_len, dmabuf_len);
        return ERROR;
    }

    if (direction == DMA_TX) {
        memcpy(p_dmabuf_va, buffer, buf_len);
    }
    buf_phy_addr = (ulong) p_dmabuf_va;
#else
    if ((ulong)buffer & 0x3) {
        dbg("Spec suggests to use 4-byte alignment address, input=0x%x\n", (ulong)buffer);
    }

    buf_phy_addr = (ulong) DMA_MAP(NULL, buffer, buf_len, direction, 0, 0);
#endif

    adma2_dscr_table = priv->adma2_dscr_buf;

    remain_len = buf_len;
    for( idx=0; remain_len && idx<num_dscr; idx++ ) {
        if (remain_len > ADMA2_MAX_LEN_OF_BUFFER) {
            length = ADMA2_MAX_LEN_OF_BUFFER;
            flags = ADMA2_ATTRIBUTE_VALID | ADMA2_ATTRIBUTE_ACT_TRAN;
        } else {
            length = remain_len;
            flags = ADMA2_ATTRIBUTE_VALID | ADMA2_ATTRIBUTE_END | ADMA2_ATTRIBUTE_INT | ADMA2_ATTRIBUTE_ACT_TRAN;
        }

        adma2_dscr_table[idx].phys_addr = buf_phy_addr;
        adma2_dscr_table[idx].len_attr = (length & 0xffff) << 16;
        adma2_dscr_table[idx].len_attr |= flags;

        buf_phy_addr += length;
        remain_len -= length;

        fvdbg("Adam2 dscr, addr=0x%08x, attr=0x%04x, len=%d, remain=%d\n",
                adma2_dscr_table[idx].phys_addr,
                adma2_dscr_table[idx].len_attr & 0xffff,
                (adma2_dscr_table[idx].len_attr & 0xffff0000) >> 16,
                remain_len);
    }

    return OK;
}
#endif

/****************************************************************************
 * Name: bcm4390x_dataconfig
 *
 * Description:
 *   Configure the SDIO data path for the next data transfer
 *
 ****************************************************************************/
static void bcm4390x_dataconfig(struct bcm4390x_dev_s *priv, bool bwrite,
                               unsigned int blocksize, unsigned int nblocks,
                               unsigned int timeout)
{
    uint32_t sz_blk;

    fvdbg("Configure data block size to %d for %d blocks\n", blocksize, nblocks);

    if (blocksize > priv->max_blk_len) {
        /* ttn: this block will be called if requesting for large size of read/write */
        uint32_t data_len;
        data_len = blocksize;
        blocksize = priv->max_blk_len;
        nblocks = data_len / blocksize + data_len % blocksize;
    }

    priv->xfr_write = bwrite;

    sz_blk = getreg32(BCM4390X_SDHC_BLKATTR);
    sz_blk = (sz_blk & ~(SDHC_BLKATTR_SIZE_MASK|SDHC_BLKATTR_CNT_MASK)) |
            (uint32_t)blocksize |
            ((uint32_t)nblocks << SDHC_BLKATTR_CNT_SHIFT);
    putreg32(sz_blk, BCM4390X_SDHC_BLKATTR);
}

/****************************************************************************
 * Name: bcm4390x_datadisable
 *
 * Description:
 *   Disable the SDIO data path setup by bcm4390x_dataconfig() and
 *   disable DMA.
 *
 ****************************************************************************/

static void bcm4390x_datadisable(void)
{
    /*
     * ttnote: here requests to disable DMA, but even the original code doesn't do
     *         it (disable DMA enabled bit @ ??? reg).
     *         Original code sets timeout_clk @ 0x2c reg here, but I didn't see where the
     *         timeout_clk was restored. So, remove it. Tested and didn't see a problem.
     */

  /* Set the block size to zero (no transfer) */

  putreg32(0, BCM4390X_SDHC_BLKATTR);
}

#ifndef CONFIG_SDIO_DMA
/****************************************************************************
 * Name: bcm4390x_transmit
 *
 * Description:
 *   Send SDIO data in interrupt mode
 *
 * Input Parameters:
 *   priv - An instance of the SDIO device interface
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_transmit(struct bcm4390x_dev_s *priv)
{
  union
  {
    uint32_t w;
    uint8_t  b[4];
  } data;

  fvdbg("Entry: remaining: %d PRESENT: %08x\n",
          priv->remaining, getreg32(BCM4390X_SDHC_PRSSTAT));

  while (priv->remaining > 0 &&
         (getreg32(BCM4390X_SDHC_PRSSTAT) & SDHC_PRSSTAT_BWEN) != 0)
    {
      /* Is there a full word remaining in the user buffer? */

      if (priv->remaining >= sizeof(uint32_t))
        {
          /* Yes, transfer the word to the TX FIFO */

          data.w           = *priv->buffer++;
          priv->remaining -= sizeof(uint32_t);
        }
      else
        {
          /* No.. transfer just the bytes remaining in the user buffer,
           * padding with zero as necessary to extend to a full word.
           */

          uint8_t *ptr = (uint8_t *)priv->remaining;
          int i;

          data.w = 0;
          for (i = 0; i < priv->remaining; i++)
            {
               data.b[i] = *ptr++;
            }

          /* Now the transfer is finished */

          priv->remaining = 0;
        }

      /* Put the word in the FIFO */

      putreg32(data.w, BCM4390X_SDHC_DATPORT);
    }

  fvdbg("Exit remaining %d bytes after transmitting, IRQEN=0x%x, PRESENT=0x%x\n", priv->remaining,
          getreg32(BCM4390X_SDHC_IRQSTATEN), getreg32(BCM4390X_SDHC_PRSSTAT));

}

/****************************************************************************
 * Name: bcm4390x_receive
 *
 * Description:
 *   Receive SDIO data in interrupt mode
 *
 * Input Parameters:
 *   priv - An instance of the SDIO device interface
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_receive(struct bcm4390x_dev_s *priv)
{
  union
  {
    uint32_t w;
    uint8_t  b[4];
  } data;

  fvdbg("Entry remaining: %d PRESENT: %08x\n",
          priv->remaining, getreg32(BCM4390X_SDHC_PRSSTAT));

  while (priv->remaining > 0 &&
         (getreg32(BCM4390X_SDHC_PRSSTAT) & SDHC_PRSSTAT_BREN) != 0)
    {
      /* Read the next word from the RX buffer */

      data.w = getreg32(BCM4390X_SDHC_DATPORT);
      if (priv->remaining >= sizeof(uint32_t))
        {
          /* Transfer the whole word to the user buffer */

          *priv->buffer++  = data.w;
          priv->remaining -= sizeof(uint32_t);
        }
      else
        {
          /* Transfer any trailing fractional word */

          uint8_t *ptr = (uint8_t *)priv->buffer;
          int i;

          for (i = 0; i < priv->remaining; i++)
            {
               *ptr++ = data.b[i];
            }

          /* Now the transfer is finished */

          priv->remaining = 0;
        }
    }

  fvdbg("Exit remaining %d bytes after receiving, IRQEN=0x%x, PRESENT=0x%x\n", priv->remaining,
          getreg32(BCM4390X_SDHC_IRQSTATEN), getreg32(BCM4390X_SDHC_PRSSTAT));
}
#endif /* CONFIG_SDIO_DMA */

/****************************************************************************
 * Name: bcm4390x_eventtimeout
 *
 * Description:
 *   The watchdog timeout setup when the event wait start has expired without
 *   any other waited-for event occurring.
 *
 * Input Parameters:
 *   argc   - The number of arguments (should be 1)
 *   arg    - The argument (state structure reference cast to uint32_t)
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Always called from the interrupt level with interrupts disabled.
 *
 ****************************************************************************/

static void bcm4390x_eventtimeout(int argc, uint32_t arg)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)arg;

  DEBUGASSERT(argc == 1 && priv != NULL);
  DEBUGASSERT((priv->waitevents & SDIOWAIT_TIMEOUT) != 0);

  /* Is a data transfer complete event expected? */

  if ((priv->waitevents & SDIOWAIT_TIMEOUT) != 0)
    {
      /* Yes.. Sample registers at the time of the timeout */
      dbg("Timeout: remaining=%d, irqstat=0x%x, irqen=0x%x\n", priv->remaining, getreg32(BCM4390X_SDHC_IRQSTAT), getreg32(BCM4390X_SDHC_IRQSIGEN));

      dbg("Host ctrl reg = 0x%08x, sig=0x%08x 0x%08x\n", getreg32(BCM4390X_SDHC_PROCTL),
              getreg32(BCM4390X_SDHC_IRQSTATEN),
              getreg32(BCM4390X_SDHC_IRQSIGEN));

      bcm4390x_sample(priv, SAMPLENDX_END_TRANSFER);

      /* Wake up any waiting threads */

      bcm4390x_endwait(priv, SDIOWAIT_TIMEOUT);
    }
}

/****************************************************************************
 * Name: bcm4390x_endwait
 *
 * Description:
 *   Wake up a waiting thread if the waited-for event has occurred.
 *
 * Input Parameters:
 *   priv      - An instance of the SDIO device interface
 *   wkupevent - The event that caused the wait to end
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Always called from the interrupt level with interrupts disabled.
 *
 ****************************************************************************/

static void bcm4390x_endwait(struct bcm4390x_dev_s *priv, sdio_eventset_t wkupevent)
{
  /* Cancel the watchdog timeout */

  (void)wd_cancel(priv->waitwdog);

  /* Disable event-related interrupts */

  bcm4390x_configwaitints(priv, 0, 0, wkupevent);

  /* Wake up the waiting thread */

  bcm4390x_givesem(priv);
}

/****************************************************************************
 * Name: bcm4390x_endtransfer
 *
 * Description:
 *   Terminate a transfer with the provided status.  This function is called
 *   only from the SDIO interrupt handler when end-of-transfer conditions
 *   are detected.
 *
 * Input Parameters:
 *   priv   - An instance of the SDIO device interface
 *   wkupevent - The event that caused the transfer to end
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Always called from the interrupt level with interrupts disabled.
 *
 ****************************************************************************/

static void bcm4390x_endtransfer(struct bcm4390x_dev_s *priv, sdio_eventset_t wkupevent)
{
  /* Disable all transfer related interrupts */

  bcm4390x_configxfrints(priv, 0);

  /* Clearing pending interrupt status on all transfer related interrupts */

  putreg32(SDHC_XFRDONE_INTS, BCM4390X_SDHC_IRQSTAT);

  /* Mark the transfer finished */

  priv->remaining = 0;

  /* Debug instrumentation */

  bcm4390x_sample(priv, SAMPLENDX_END_TRANSFER);

  /* Is a thread wait for these data transfer complete events? */

  if ((priv->waitevents & wkupevent) != 0)
    {
      /* Yes.. wake up any waiting threads */

      bcm4390x_endwait(priv, wkupevent);
    }
}

/****************************************************************************
 * Interrupt Handling
 ****************************************************************************/

/****************************************************************************
 * Name: bcm4390x_interrupt
 *
 * Description:
 *   SDIO interrupt handler
 *
 * Input Parameters:
 *   dev - An instance of the SDIO device interface
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static int bcm4390x_interrupt(int irq, void *context)
{
  struct bcm4390x_dev_s *priv = &g_sdhcdev;
  uint32_t enabled;
  uint32_t pending;
  uint32_t regval;

  /* Check the SDHC IRQSTAT register.  Mask out all bits that don't
   * correspond to enabled interrupts.  (This depends on the fact that bits
   * are ordered the same in both the IRQSTAT and IRQSIGEN registers).  If
   * there are non-zero bits remaining, then we have work to do here.
   */

  regval  = getreg32(BCM4390X_SDHC_IRQSIGEN);
  enabled = getreg32(BCM4390X_SDHC_IRQSTAT) & regval;

  fllvdbg("IRQSTAT: %08x IRQSIGEN %08x enabled: %08x\n",
          getreg32(BCM4390X_SDHC_IRQSTAT), regval, enabled);

  /* Disable card interrupts to clear the card interrupt to the host system. */

  regval &= ~SDHC_INT_CINT;
  putreg32(regval, BCM4390X_SDHC_IRQSIGEN);

  /* Clear all pending interrupts */

  putreg32(enabled, BCM4390X_SDHC_IRQSTAT);

  /* Handle in progress, interrupt driven data transfers ********************/

  pending  = enabled & priv->xfrints;

  if (pending != 0)
    {
#ifndef CONFIG_SDIO_DMA
      if ((pending & SDHC_INT_BRR) != 0 && (pending & SDHC_INT_BWR) != 0) {
          dbg("WARNING: Both Tx and Rx interrupts happened, intr=0x%08x\n", pending)
      }

      /* Is the RX buffer read ready?  Is so then we must be processing a
       * non-DMA receive transaction.
       */

      if ((pending & SDHC_INT_BRR) != 0)
        {
          /* Receive data from the RX buffer */
          bcm4390x_receive(priv);
        }

      /* Otherwise, Is the TX buffer write ready? If so we must
       * be processing a non-DMA send transaction.
       */

      else if ((pending & SDHC_INT_BWR) != 0)
        {
          /* Send data via the TX FIFO */
          bcm4390x_transmit(priv);
        }
#else /* ndef CONFIG_SDIO_DMA */
      if ( (pending & SDHC_INT_BRR) || (pending & SDHC_INT_BWR) ) {
          dbg("ERROR: Getting BRR/BWR intr (IRQSTAT=0x%08x) when using DMA.\n", pending);
      }
#endif

      /* Handle transfer complete events */

      if ((pending & SDHC_INT_TC) != 0)
        {
          /* Terminate the transfer */
#ifdef CONFIG_SDIO_DMA
#ifdef WAR_USE_DMA_MEMORY
          if (!priv->xfr_write) {
              /* if it is a receiving operation, copy received data to application buffer */
              memcpy(priv->buffer, p_dmabuf_va, priv->remaining);
          }

#endif
#endif
          bcm4390x_endtransfer(priv, SDIOWAIT_TRANSFERDONE);
        }

      /* Handle data block send/receive CRC failure */

      else if ((pending & SDHC_INT_DCE) != 0)
        {
          /* Terminate the transfer with an error */

          flldbg("ERROR: Data block CRC failure, remaining: %d\n", priv->remaining);
          bcm4390x_endtransfer(priv, SDIOWAIT_TRANSFERDONE | SDIOWAIT_ERROR);
        }

      /* Handle data timeout error */

      else if ((pending & SDHC_INT_DTOE) != 0)
        {
          /* Terminate the transfer with an error */

          flldbg("ERROR: Data timeout, remaining: %d\n", priv->remaining);
          bcm4390x_endtransfer(priv, SDIOWAIT_TRANSFERDONE | SDIOWAIT_TIMEOUT);
        }
    }

  /* Handle wait events *****************************************************/

  pending  = enabled & priv->waitints;
  if (pending != 0)
    {
      /* Is this a response completion event? */

      if ((pending & SDHC_RESPDONE_INTS) != 0)
        {
          /* Yes.. Is their a thread waiting for response done? */

          if ((priv->waitevents & (SDIOWAIT_CMDDONE | SDIOWAIT_RESPONSEDONE)) != 0)
            {
              /* Yes.. mask further interrupts and wake the thread up */

              regval = getreg32(BCM4390X_SDHC_IRQSIGEN);
              regval &= ~SDHC_RESPDONE_INTS;
              putreg32(regval, BCM4390X_SDHC_IRQSIGEN);

              bcm4390x_endwait(priv, SDIOWAIT_RESPONSEDONE);
            }
        }
    }

  /* Re-enable card interrupts */

  regval  = getreg32(BCM4390X_SDHC_IRQSIGEN);
  regval |= SDHC_INT_CINT;
  putreg32(regval, BCM4390X_SDHC_IRQSIGEN);

  return OK;
}

/****************************************************************************
 * SDIO Interface Methods
 ****************************************************************************/

/****************************************************************************
 * Name: bcm4390x_lock
 *
 * Description:
 *   Locks the bus. Function calls low-level multiplexed bus routines to
 *   resolve bus requests and acknowledgement issues.
 *
 * Input Parameters:
 *   dev    - An instance of the SDIO device interface
 *   lock   - TRUE to lock, FALSE to unlock.
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_MUXBUS
static int bcm4390x_lock(FAR struct sdio_dev_s *dev, bool lock)
{
  /* Single SDIO instance so there is only one possibility.  The multiplex
   * bus is part of board support package.
   */

  bcm4390x_muxbus_sdio_lock(lock);
  return OK;
}
#endif

/****************************************************************************
 * Name: bcm4390x_reset
 *
 * Description:
 *   Reset the SD controller.  Undo all setup and initialization.
 *
 * Input Parameters:
 *   dev - An instance of the SD device interface
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_reset(FAR struct sdio_dev_s *dev)
{
  FAR struct bcm4390x_dev_s *priv = (FAR struct bcm4390x_dev_s *)dev;
  uint32_t regval;

  /* Disable all interrupts so that nothing interferes with the following. */

  putreg32(0, BCM4390X_SDHC_IRQSIGEN);

  /* Reset the SDHC block, putting registers in their default, reset state.
   * Initiate the reset by setting the RSTA bit in the SYSCTL register.
   */

  regval  = getreg32(BCM4390X_SDHC_SYSCTL);
  regval |= SDHC_SYSCTL_RSTA;
  putreg32(regval, BCM4390X_SDHC_SYSCTL);

  /* The SDHC will reset the RSTA bit to 0 when the capabilities
   * registers are valid and the host driver can read them.
   */

  while ((getreg32(BCM4390X_SDHC_SYSCTL) & SDHC_SYSCTL_RSTA) != 0);

  /* Make sure that all clocking is disabled */

  bcm4390x_clock(dev, CLOCK_SDIO_DISABLED);

  /* Enable all status bits (these could not all be potential sources of
   * interrupts.
   */

  putreg32(SDHC_INT_ALL, BCM4390X_SDHC_IRQSTATEN);

  fvdbg("SYSCTL: %08x PRSSTAT: %08x IRQSTATEN: %08x\n",
        getreg32(BCM4390X_SDHC_SYSCTL), getreg32(BCM4390X_SDHC_PRSSTAT),
        getreg32(BCM4390X_SDHC_IRQSTATEN));

  /* The next phase of the hardware reset would be to set the SYSCTRL INITA
   * bit to send 80 clock ticks for card to power up and then reset the card
   * with CMD0.  This is done elsewhere.
   */

  /* Reset state data */

  priv->waitevents = 0;      /* Set of events to be waited for */
  priv->waitints   = 0;      /* Interrupt enables for event waiting */
  priv->wkupevent  = 0;      /* The event that caused the wakeup */
#ifdef CONFIG_SDIO_DMA
  priv->xfrflags   = 0;      /* Used to synchronize SDIO and DMA completion events */
#endif

  wd_cancel(priv->waitwdog); /* Cancel any timeouts */

  /* Interrupt mode data transfer support */

  priv->buffer     = 0;      /* Address of current R/W buffer */
  priv->remaining  = 0;      /* Number of bytes remaining in the transfer */
  priv->xfrints    = 0;      /* Interrupt enables for data transfer */

  /* checking DMA mode */
#ifdef CONFIG_SDIO_DMA
  if (priv->sd_dma_mode == DMA_MODE_ADMA2)
  {
    /* Because bcm4390x_reset() will also reset host ctrl reg, so set dma mode after it. */
    uint32_t host_ctrl;
    host_ctrl = getreg32(BCM4390X_SDHC_PROCTL);
    host_ctrl |= SDHC_PROCTL_DMAS_ADMA2;
    putreg32(host_ctrl, BCM4390X_SDHC_PROCTL);
  }
#endif
}

/****************************************************************************
 * Name: bcm4390x_status
 *
 * Description:
 *   Get SDIO status.
 *
 * Input Parameters:
 *   dev   - Device-specific state data
 *
 * Returned Value:
 *   Returns a bitset of status values (see bcm4390x_status_* defines)
 *
 ****************************************************************************/

static uint8_t bcm4390x_status(FAR struct sdio_dev_s *dev)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;
  return priv->cdstatus;
}

/****************************************************************************
 * Name: bcm4390x_widebus
 *
 * Description:
 *   Called after change in Bus width has been selected (via ACMD6).  Most
 *   controllers will need to perform some special operations to work
 *   correctly in the new bus mode.
 *
 * Input Parameters:
 *   dev  - An instance of the SDIO device interface
 *   wide - true: wide bus (4-bit) bus mode enabled
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_widebus(FAR struct sdio_dev_s *dev, bool wide)
{
  uint32_t regval;

  /* Set the Data Transfer Width (DTW) field in the PROCTL register */

  regval = getreg32(BCM4390X_SDHC_PROCTL);
  regval &= ~SDHC_PROCTL_DTW_MASK;
  if (wide)
    {
      regval |= SDHC_PROCTL_DTW_4BIT;
    }
  else
    {
      regval |= SDHC_PROCTL_DTW_1BIT;
    }
  putreg32(regval, BCM4390X_SDHC_PROCTL);
}

/****************************************************************************
 * Name: bcm4390x_frequency
 *
 * Description:
 *   Set the SD clock frequency
 *
 * Input Parameters:
 *   dev       - An instance of the SDIO device interface
 *   frequency - The frequency to use
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

/*
 * tttodo: we are able to support SDR50, DDR50 and SDR104. It means we are able to work on
 *         50 Mhz (50 MB/s) and also 104? Mhz. If we want to implement this, needs to add
 *         codes to check capabilities of SD card.
 */
static void bcm4390x_frequency(FAR struct sdio_dev_s *dev, uint32_t frequency)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;
  uint32_t req_div = 0;
  uint32_t div_bit = 0;
  uint32_t base_clk = 0;
  uint32_t timeout_clk = 0;
  uint32_t regval;

  base_clk = priv->sd_base_clk;

  req_div = (base_clk / frequency);
  req_div += ((base_clk % frequency) ? 1 : 0);
  req_div += (req_div & 0x1 ? 1 : 0);
  req_div /= 2;
  if (req_div > 0x3ff) { // ttn: according to spec, max of 10-bit clk mode is 0x3ff (1/2046).
      fvdbg("Original clk divider is %d. Per spec., limit it to be 1023\n", req_div);
      req_div = 0x3ff;
  }

  div_bit = ((req_div & 0x00ff) << 8) | ((req_div & 0x0300) >> 2);

  regval = getreg32(BCM4390X_SDHC_SYSCTL);

  /* stop clock before changing clock */
  regval &= 0xfffffff8;
  putreg32(regval, BCM4390X_SDHC_SYSCTL);

  fvdbg ("Set clk, frequeycy=%d, div=%d, div_bit=0x%x, clkctl=0x%08x\n",
          frequency, req_div, div_bit, regval);
  regval &= ~(0xffc0);
  regval |= (div_bit | 1);
  putreg32(regval, BCM4390X_SDHC_SYSCTL);
  while((getreg32(BCM4390X_SDHC_SYSCTL) & 0x2) == 0);

  /*
   * decide timeout clock control. The first value, 7, is from bcmsdstd.
   * Tested with 25 Mhz (real clk is 22857 KMhz), 7 also works.
   * bcmsdstd sets timeout when starting clock.
   */
  timeout_clk = 0x70000;

  regval  = getreg32(BCM4390X_SDHC_SYSCTL);
  regval |= 0x4;
  regval |= timeout_clk;
  putreg32(regval, BCM4390X_SDHC_SYSCTL);

  fvdbg("ABSFREQ SYSCTRL: %08x (div=%d, clk=%dKhz)\n", regval,
          ((regval & 0xff00)>>8) | ((regval & 0x00c0)<<2),
          base_clk/((((regval & 0xff00)>>8) | ((regval & 0x00c0)<<2))*2*1000));

}

/****************************************************************************
 * Name: bcm4390x_clock
 *
 * Description:
 *   Enable/disable SDIO clocking
 *
 * Input Parameters:
 *   dev  - An instance of the SDIO device interface
 *   rate - Specifies the clocking to use (see enum sdio_clock_e)
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_clock(FAR struct sdio_dev_s *dev, enum sdio_clock_e rate)
{
  uint32_t frequency;
  uint32_t regval;

  /* The SDCLK must be disabled before its frequency can be changed: "SDCLK
   * frequency can be changed when this bit is 0. Then, the host controller
   * shall maintain the same clock frequency until SDCLK is stopped (stop at
   * SDCLK = 0).
   */

  regval  = getreg32(BCM4390X_SDHC_SYSCTL);
  regval &= ~SDHC_SYSCTL_SDCLKEN;
  putreg32(regval, BCM4390X_SDHC_SYSCTL);
  fvdbg("SYSCTRL: %08x\n", getreg32(BCM4390X_SDHC_SYSCTL));

  switch (rate)
    {
      default:
      case CLOCK_SDIO_DISABLED :     /* Clock is disabled */
        {
          /* Clear the prescaler and divisor settings and other clock
           * enables as well.
           */

          regval &= ~(SDHC_SYSCTL_IPGEN | SDHC_SYSCTL_HCKEN | SDHC_SYSCTL_PEREN |
                      SDHC_SYSCTL_SDCLKFS_MASK | SDHC_SYSCTL_DVS_MASK);
          putreg32(regval, BCM4390X_SDHC_SYSCTL);
          fvdbg("SYSCTRL: %08x\n", getreg32(BCM4390X_SDHC_SYSCTL));
          return;
        }

      case CLOCK_IDMODE :            /* Initial ID mode clocking (<400KHz) */
        frequency = CONFIG_BCM4390X_IDMODE_FREQ;
        break;

      case CLOCK_MMC_TRANSFER :      /* MMC normal operation clocking */
        frequency = CONFIG_BCM4390X_MMCXFR_FREQ;
        break;

      case CLOCK_SD_TRANSFER_1BIT :  /* SD normal operation clocking (narrow 1-bit mode) */
#ifndef CONFIG_SDIO_WIDTH_D1_ONLY
        frequency = CONFIG_BCM4390X_SD1BIT_FREQ;
        break;
#endif

      case CLOCK_SD_TRANSFER_4BIT :  /* SD normal operation clocking (wide 4-bit mode) */
        frequency = CONFIG_BCM4390X_SD4BIT_FREQ;
        break;
    }

  /* Then set the selected frequency */

  bcm4390x_frequency(dev, frequency);
}

/****************************************************************************
 * Name: bcm4390x_attach
 *
 * Description:
 *   Attach and prepare interrupts
 *
 * Input Parameters:
 *   dev - An instance of the SDIO device interface
 *
 * Returned Value:
 *   OK on success; A negated errno on failure.
 *
 ****************************************************************************/

static int bcm4390x_attach(FAR struct sdio_dev_s *dev)
{
  int ret = OK;

  /* Attach the SDIO interrupt handler */

  ret = irq_attach(SDIO_REMAPPED_ExtIRQn, bcm4390x_interrupt);
  fvdbg(" IRQ Attached, ret=%d\n", ret);
  if (ret == OK)
    {

      /* Disable all interrupts at the SDIO controller and clear all pending
       * interrupts.
       */

      putreg32(0,            BCM4390X_SDHC_IRQSIGEN);
      putreg32(SDHC_INT_ALL, BCM4390X_SDHC_IRQSTAT);

#ifdef CONFIG_ARCH_IRQPRIO
      /* Set the interrupt priority */

      up_prioritize_irq(BCM4390X_IRQ_SDHC, CONFIG_BCM4390X_SDHC_PRIO);
#endif

      /* Enable SDIO interrupts at the NVIC.  They can now be enabled at
       * the SDIO controller as needed.
       */
 
      fvdbg("ROUTE SDHC IRQ\n");
      platform_irq_remap_sink( OOB_AOUT_SDIO_HOST_INTR, SDIO_REMAPPED_ExtIRQn );

      up_enable_irq(SDIO_REMAPPED_ExtIRQn);
    }

  return ret;
}

/****************************************************************************
 * Name: bcm4390x_sendcmd
 *
 * Description:
 *   Send the SDIO command
 *
 * Input Parameters:
 *   dev  - An instance of the SDIO device interface
 *   cmd  - The command to send (32-bits, encoded)
 *   arg  - 32-bit argument required with some commands
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static int bcm4390x_sendcmd(FAR struct sdio_dev_s *dev, uint32_t cmd, uint32_t arg)
{
  uint32_t regval;
  uint32_t cmdidx;
  int32_t  timeout;

  /* Initialize the command index */

  cmdidx = (cmd & MMCSD_CMDIDX_MASK) >> MMCSD_CMDIDX_SHIFT;
  regval = cmdidx << SDHC_XFERTYP_CMDINX_SHIFT;

  /* Does a data transfer accompany the command? */

  if ((cmd & MMCSD_DATAXFR) != 0)
    {
      /* Yes.. Configure the data transfer */

      switch (cmd & MMCSD_DATAXFR_MASK)
        {
          default:
          case MMCSD_NODATAXFR : /* No.. no data transfer */
            break;

          /* The following two cases are probably missing some setup logic */

          case MMCSD_RDSTREAM :  /* Yes.. streaming read data transfer */
            regval |= (SDHC_XFERTYP_DPSEL | SDHC_XFERTYP_DTDSEL);
            break;

          case MMCSD_WRSTREAM :  /* Yes.. streaming write data transfer */
            regval |= SDHC_XFERTYP_DPSEL;
            break;

          case MMCSD_RDDATAXFR : /* Yes.. normal read data transfer */
            regval |= (SDHC_XFERTYP_DPSEL | SDHC_XFERTYP_DTDSEL);
            break;

          case MMCSD_WRDATAXFR : /* Yes.. normal write data transfer */
            regval |= SDHC_XFERTYP_DPSEL;
            break;
        }

      /* Is it a multi-block transfer? */

      if ((cmd & MMCSD_MULTIBLOCK) != 0)
        {
          /* Yes.. should the transfer be stopped with ACMD12? */

          if ((cmd & MMCSD_STOPXFR) != 0)
            {
              /* Yes.. Indefinite block transfer */

              regval |= (SDHC_XFERTYP_MSBSEL | SDHC_XFERTYP_AC12EN);
            }
          else
            {
              /* No.. Fixed block transfer */

              regval |= (SDHC_XFERTYP_MSBSEL | SDHC_XFERTYP_BCEN);
            }
        }
    }

  /* Configure response type bits */

  switch (cmd & MMCSD_RESPONSE_MASK)
    {
    case MMCSD_NO_RESPONSE:                /* No response */
      regval |= SDHC_XFERTYP_RSPTYP_NONE;
      break;

    case MMCSD_R1B_RESPONSE:              /* Response length 48, check busy & cmdindex */
      regval |= (SDHC_XFERTYP_RSPTYP_LEN48BSY | SDHC_XFERTYP_CICEN |
                 SDHC_XFERTYP_CCCEN);
      break;

    case MMCSD_R1_RESPONSE:              /* Response length 48, check cmdindex */
    case MMCSD_R5_RESPONSE:
    case MMCSD_R6_RESPONSE:
    case MMCSD_R7_RESPONSE:
      regval |= (SDHC_XFERTYP_RSPTYP_LEN48 | SDHC_XFERTYP_CICEN |
                 SDHC_XFERTYP_CCCEN);
      break;

    case MMCSD_R2_RESPONSE:              /* Response length 136, check CRC */
     /* XXX: asic team confirm this is an issue in chip, R2 CRC check doesn't work.
      *      It is better to check chip revision for enabling this war.
      */

      regval |= (SDHC_XFERTYP_RSPTYP_LEN136);
      break;

    case MMCSD_R3_RESPONSE:              /* Response length 48 */
    case MMCSD_R4_RESPONSE:
      regval |= SDHC_XFERTYP_RSPTYP_LEN48;
      break;
    }

  /* Enable DMA */

#ifdef CONFIG_SDIO_DMA
  /* Internal DMA is used */
  /* ttn: didn't see a problem related to here. But I think it is reasonable if only
   * enable DMA when DAT lines will be used
   * */
    if (regval & SDHC_XFERTYP_DPSEL) {
        regval |= SDHC_XFERTYP_DMAEN;
    }
#endif

  /* Other bits? What about CMDTYP? */

  dbg("[CMD%02d] cmd: %08x arg: %08x regval: %08x\n", cmdidx, cmd, arg, regval);

  /* The Command Inhibit (CIHB) bit is set in the PRSSTAT bit immediately
   * after the transfer type register is written.  This bit is cleared when
   * the command response is received.  If this status bit is 0, it
   * indicates that the CMD line is not in use and the SDHC can issue a
   * SD/MMC Command using the CMD line.
   *
   * CIHB should always be set when this function is called.
   */

  timeout = SDHC_CMDTIMEOUT;
  while ((getreg32(BCM4390X_SDHC_PRSSTAT) & SDHC_PRSSTAT_CIHB) != 0)
    {
      if (--timeout <= 0)
        {
          dbg("ERROR: Timeout cmd: %08x PRSSTAT: %08x\n",
               cmd, getreg32(BCM4390X_SDHC_PRSSTAT));

          return -EBUSY;
        }
    }

  /* Set the SDHC Argument value */

  putreg32(arg, BCM4390X_SDHC_CMDARG);

  /* Clear interrupt status and write the SDHC CMD */

  putreg32(SDHC_RESPDONE_INTS, BCM4390X_SDHC_IRQSTAT);

  putreg32(regval, BCM4390X_SDHC_XFERTYP);

  return OK;
}

/****************************************************************************
 * Name: bcm4390x_recvsetup
 *
 * Description:
 *   Setup hardware in preparation for data transfer from the card in non-DMA
 *   (interrupt driven mode).  This method will do whatever controller setup
 *   is necessary.  This would be called for SD memory just BEFORE sending
 *   CMD13 (SEND_STATUS), CMD17 (READ_SINGLE_BLOCK), CMD18
 *   (READ_MULTIPLE_BLOCKS), ACMD51 (SEND_SCR), etc.  Normally, SDIO_WAITEVENT
 *   will be called to receive the indication that the transfer is complete.
 *
 * Input Parameters:
 *   dev    - An instance of the SDIO device interface
 *   buffer - Address of the buffer in which to receive the data
 *   nbytes - The number of bytes in the transfer
 *
 * Returned Value:
 *   Number of bytes sent on success; a negated errno on failure
 *
 ****************************************************************************/

#ifndef CONFIG_SDIO_DMA
static int bcm4390x_recvsetup(FAR struct sdio_dev_s *dev, FAR uint8_t *buffer,
                             size_t nbytes)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;

  DEBUGASSERT(priv != NULL && buffer != NULL && nbytes > 0);
  DEBUGASSERT(((uint32_t)buffer & 3) == 0);

  /* Reset the DPSM configuration */

  bcm4390x_datadisable();
  bcm4390x_sampleinit();
  bcm4390x_sample(priv, SAMPLENDX_BEFORE_SETUP);

  /* Save the destination buffer information for use by the interrupt handler */

  priv->buffer    = (uint32_t *)buffer;
  priv->remaining = nbytes;

  /* Then set up the SDIO data path */

  bcm4390x_dataconfig(priv, false, nbytes, 1, SDHC_DVS_DATATIMEOUT);

  /* And enable interrupts */

  bcm4390x_configxfrints(priv, SDHC_RCVDONE_INTS);
  bcm4390x_sample(priv, SAMPLENDX_AFTER_SETUP);
  return OK;
}
#endif

/****************************************************************************
 * Name: bcm4390x_sendsetup
 *
 * Description:
 *   Setup hardware in preparation for data transfer from the card.  This method
 *   will do whatever controller setup is necessary.  This would be called
 *   for SD memory just AFTER sending CMD24 (WRITE_BLOCK), CMD25
 *   (WRITE_MULTIPLE_BLOCK), ... and before SDIO_SENDDATA is called.
 *
 * Input Parameters:
 *   dev    - An instance of the SDIO device interface
 *   buffer - Address of the buffer containing the data to send
 *   nbytes - The number of bytes in the transfer
 *
 * Returned Value:
 *   Number of bytes sent on success; a negated errno on failure
 *
 ****************************************************************************/

#ifndef CONFIG_SDIO_DMA
static int bcm4390x_sendsetup(FAR struct sdio_dev_s *dev, FAR const uint8_t *buffer,
                           size_t nbytes)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;

  DEBUGASSERT(priv != NULL && buffer != NULL && nbytes > 0);
  DEBUGASSERT(((uint32_t)buffer & 3) == 0);

  /* Reset the DPSM configuration */

  bcm4390x_datadisable();
  bcm4390x_sampleinit();
  bcm4390x_sample(priv, SAMPLENDX_BEFORE_SETUP);

  /* Save the source buffer information for use by the interrupt handler */

  priv->buffer    = (uint32_t *)buffer;
  priv->remaining = nbytes;

  /* Then set up the SDIO data path */

  bcm4390x_dataconfig(priv, true, nbytes, 1, SDHC_DVS_DATATIMEOUT);

  /* Enable TX interrupts */

  bcm4390x_configxfrints(priv, SDHC_SNDDONE_INTS);
  bcm4390x_sample(priv, SAMPLENDX_AFTER_SETUP);
  return OK;
}
#endif

/****************************************************************************
 * Name: bcm4390x_cancel
 *
 * Description:
 *   Cancel the data transfer setup of SDIO_RECVSETUP, SDIO_SENDSETUP,
 *   SDIO_DMARECVSETUP or SDIO_DMASENDSETUP.  This must be called to cancel
 *   the data transfer setup if, for some reason, you cannot perform the
 *   transfer.
 *
 * Input Parameters:
 *   dev  - An instance of the SDIO device interface
 *
 * Returned Value:
 *   OK is success; a negated errno on failure
 *
 ****************************************************************************/

static int bcm4390x_cancel(FAR struct sdio_dev_s *dev)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;
#ifdef CONFIG_SDIO_DMA
  uint32_t regval;
#endif

  /* Disable all transfer- and event- related interrupts */

  bcm4390x_configxfrints(priv, 0);
  bcm4390x_configwaitints(priv, 0, 0, 0);

  /* Clearing pending interrupt status on all transfer- and event- related
   * interrupts
   */

  putreg32(SDHC_WAITALL_INTS, BCM4390X_SDHC_IRQSTAT);

  /* Cancel any watchdog timeout */

  (void)wd_cancel(priv->waitwdog);

  /* If this was a DMA transfer, make sure that DMA is stopped */

#ifdef CONFIG_SDIO_DMA
  /* Stop the DMA by resetting the data path */

  regval = getreg32(BCM4390X_SDHC_SYSCTL);
  regval |= SDHC_SYSCTL_RSTD;
  putreg32(regval, BCM4390X_SDHC_SYSCTL);
#endif

  /* Mark no transfer in progress */

  priv->remaining = 0;
  return OK;
}

/****************************************************************************
 * Name: bcm4390x_waitresponse
 *
 * Description:
 *   Poll-wait for the response to the last command to be ready.  This
 *   function should be called even after sending commands that have no
 *   response (such as CMD0) to make sure that the hardware is ready to
 *   receive the next command.
 *
 * Input Parameters:
 *   dev  - An instance of the SDIO device interface
 *   cmd  - The command that was sent.  See 32-bit command definitions above.
 *
 * Returned Value:
 *   OK is success; a negated errno on failure
 *
 ****************************************************************************/

static int bcm4390x_waitresponse(FAR struct sdio_dev_s *dev, uint32_t cmd)
{
  uint32_t errors;
  int32_t  timeout;
  int      ret = OK;

  switch (cmd & MMCSD_RESPONSE_MASK)
    {
    case MMCSD_NO_RESPONSE:
      timeout = SDHC_CMDTIMEOUT;
      errors  = 0;
      return OK;

    case MMCSD_R1_RESPONSE:
    case MMCSD_R1B_RESPONSE:
    case MMCSD_R2_RESPONSE:
    case MMCSD_R6_RESPONSE:
      timeout = SDHC_LONGTIMEOUT;
      errors  = SDHC_RESPERR_INTS;
      break;

    case MMCSD_R4_RESPONSE:
    case MMCSD_R5_RESPONSE:
      return -ENOSYS;

    case MMCSD_R3_RESPONSE:
    case MMCSD_R7_RESPONSE:
      timeout = SDHC_CMDTIMEOUT;
      errors  = SDHC_RESPERR_INTS;
      break;

    default:
      return -EINVAL;
    }

  /* Then wait for the Command Complete (CC) indication (or timeout).  The
   * CC bit is set when the end bit of the command response is received
   * (except Auto CMD12).
   */

  while ((getreg32(BCM4390X_SDHC_IRQSTAT) & SDHC_INT_CC) == 0)
    {
      if (--timeout <= 0)
        {
          dbg("ERROR: Timeout cmd: %08x IRQSTAT: %08x\n",
               cmd, getreg32(BCM4390X_SDHC_IRQSTAT));

          return -ETIMEDOUT;
        }
    }

  /* Check for hardware detected errors */

  if ((getreg32(BCM4390X_SDHC_IRQSTAT) & errors) != 0)
    {
      dbg("ERROR: cmd: %08x (idx=%d) errors: %08x IRQSTAT: %08x\n",
           cmd, ((cmd & MMCSD_CMDIDX_MASK) >> MMCSD_CMDIDX_SHIFT),
           errors, getreg32(BCM4390X_SDHC_IRQSTAT));
      ret = -EIO;
    }

  /* Clear the response wait status bits */

  putreg32(SDHC_RESPDONE_INTS, BCM4390X_SDHC_IRQSTAT);
  return ret;
}

/****************************************************************************
 * Name: bcm4390x_recvRx
 *
 * Description:
 *   Receive response to SDIO command.  Only the critical payload is
 *   returned -- that is 32 bits for 48 bit status and 128 bits for 136 bit
 *   status.  The driver implementation should verify the correctness of
 *   the remaining, non-returned bits (CRCs, CMD index, etc.).
 *
 * Input Parameters:
 *   dev    - An instance of the SDIO device interface
 *   Rx - Buffer in which to receive the response
 *
 * Returned Value:
 *   Number of bytes sent on success; a negated errno on failure.  Here a
 *   failure means only a faiure to obtain the requested reponse (due to
 *   transport problem -- timeout, CRC, etc.).  The implementation only
 *   assures that the response is returned intacta and does not check errors
 *   within the response itself.
 *
 ****************************************************************************/

static int bcm4390x_recvshortcrc(FAR struct sdio_dev_s *dev, uint32_t cmd,
                                uint32_t *rshort)
{
  uint32_t regval;
  int ret = OK;

  /* R1  Command response (48-bit)
   *     47        0               Start bit
   *     46        0               Transmission bit (0=from card)
   *     45:40     bit5   - bit0   Command index (0-63)
   *     39:8      bit31  - bit0   32-bit card status
   *     7:1       bit6   - bit0   CRC7
   *     0         1               End bit
   *
   * R1b Identical to R1 with the additional busy signalling via the data
   *     line.
   *
   * R6  Published RCA Response (48-bit, SD card only)
   *     47        0               Start bit
   *     46        0               Transmission bit (0=from card)
   *     45:40     bit5   - bit0   Command index (0-63)
   *     39:8      bit31  - bit0   32-bit Argument Field, consisting of:
   *                               [31:16] New published RCA of card
   *                               [15:0]  Card status bits {23,22,19,12:0}
   *     7:1       bit6   - bit0   CRC7
   *     0         1               End bit
   */


#ifdef CONFIG_DEBUG
  if (!rshort)
    {
      fdbg("ERROR: rshort=NULL\n");
      ret = -EINVAL;
    }

  /* Check that this is the correct response to this command */

  else if ((cmd & MMCSD_RESPONSE_MASK) != MMCSD_R1_RESPONSE &&
           (cmd & MMCSD_RESPONSE_MASK) != MMCSD_R1B_RESPONSE &&
           (cmd & MMCSD_RESPONSE_MASK) != MMCSD_R6_RESPONSE)
    {
      fdbg("ERROR: Wrong response CMD=%08x\n", cmd);
      ret = -EINVAL;
    }
  else
#endif
    {
      /* Check if a timeout or CRC error occurred */

      regval = getreg32(BCM4390X_SDHC_IRQSTAT);
      if ((regval & SDHC_INT_CTOE) != 0)
        {
          fdbg("ERROR: Command timeout: %08x\n", regval);
          ret = -ETIMEDOUT;
        }
      else if ((regval & SDHC_INT_CCE) != 0)
        {
          fdbg("ERROR: CRC failure: %08x\n", regval);
          ret = -EIO;
        }
    }

  /* Return the R1/R1b/R6 response.  These responses are returned in
   * CDMRSP0.  NOTE: This is not true for R1b (Auto CMD12 response) which
   * is returned in CMDRSP3.
   */

  *rshort = getreg32(BCM4390X_SDHC_CMDRSP0);
  return ret;
}

static int bcm4390x_recvlong(FAR struct sdio_dev_s *dev, uint32_t cmd, uint32_t rlong[4])
{
  uint32_t regval;
  int ret = OK;

  /* R2  CID, CSD register (136-bit)
   *     135       0               Start bit
   *     134       0               Transmission bit (0=from card)
   *     133:128   bit5   - bit0   Reserved
   *     127:1     bit127 - bit1   127-bit CID or CSD register
   *                               (including internal CRC)
   *     0         1               End bit
   */

#ifdef CONFIG_DEBUG
  /* Check that R1 is the correct response to this command */

  if ((cmd & MMCSD_RESPONSE_MASK) != MMCSD_R2_RESPONSE)
    {
      fdbg("ERROR: Wrong response CMD=%08x\n", cmd);
      ret = -EINVAL;
    }
  else
#endif
    {
      /* Check if a timeout or CRC error occurred */

      regval = getreg32(BCM4390X_SDHC_IRQSTAT);
      if (regval & SDHC_INT_CTOE)
        {
          fdbg("ERROR: Timeout IRQSTAT: %08x\n", regval);
          ret = -ETIMEDOUT;
        }
      else if (regval & SDHC_INT_CCE)
        {
          fdbg("ERROR: CRC fail IRQSTAT: %08x\n", regval);
          ret = -EIO;
        }
    }

  /* Return the long response in CMDRSP3..0 */

  if (rlong)
    {
      uint32_t idx;
      uint8_t lb;

      rlong[0] = getreg32(BCM4390X_SDHC_CMDRSP3); /* b127:b96 */
      rlong[1] = getreg32(BCM4390X_SDHC_CMDRSP2); /* b95:b64 */
      rlong[2] = getreg32(BCM4390X_SDHC_CMDRSP1); /* b63:b32 */
      rlong[3] = getreg32(BCM4390X_SDHC_CMDRSP0); /* b31:b00 */

      /*
       * Nuttx mmcsd driver expects that R2 includes CRC bits at bit7-0 (This is also CID definition
       * in SD spec 5.2. But chip stores b127:8 of response at b119:0 of registers. So the received
       * data needs to be shifted 8 bits.
       */
      fvdbg("R2 (Original): 0x%08x 0x%08x 0x%08x 0x%08x\n", rlong[0], rlong[1], rlong[2], rlong[3]);

      for(idx=0; idx<4; idx++) {
          lb = (idx==3? 0 : (uint8_t)(rlong[idx+1] >> 24));
          rlong[idx] = ((rlong[idx] << 8) & 0xffffff00) | lb;
      }

      fvdbg("R2 (Shifted) : 0x%08x 0x%08x 0x%08x 0x%08x\n", rlong[0], rlong[1], rlong[2], rlong[3]);
    }
  return ret;
}

static int bcm4390x_recvshort(FAR struct sdio_dev_s *dev, uint32_t cmd, uint32_t *rshort)
{
  uint32_t regval;
  int ret = OK;

  /* R3  OCR (48-bit)
   *     47        0               Start bit
   *     46        0               Transmission bit (0=from card)
   *     45:40     bit5   - bit0   Reserved
   *     39:8      bit31  - bit0   32-bit OCR register
   *     7:1       bit6   - bit0   Reserved
   *     0         1               End bit
   */

  /* Check that this is the correct response to this command */

#ifdef CONFIG_DEBUG
  if ((cmd & MMCSD_RESPONSE_MASK) != MMCSD_R3_RESPONSE &&
      (cmd & MMCSD_RESPONSE_MASK) != MMCSD_R7_RESPONSE)
    {
      fdbg("ERROR: Wrong response CMD=%08x\n", cmd);
      ret = -EINVAL;
    }
  else
#endif
    {
      /* Check if a timeout occurred (Apparently a CRC error can terminate
       * a good response)
       */

      regval = getreg32(BCM4390X_SDHC_IRQSTAT);
      if (regval & SDHC_INT_CTOE)
        {
          fdbg("ERROR: Timeout IRQSTAT: %08x\n", regval);
          ret = -ETIMEDOUT;
        }
    }

  /* Return the short response in CMDRSP0 */

  if (rshort)
    {
      *rshort = getreg32(BCM4390X_SDHC_CMDRSP0);
    }

  return ret;
}

/* MMC responses not supported */

static int bcm4390x_recvnotimpl(FAR struct sdio_dev_s *dev, uint32_t cmd, uint32_t *rnotimpl)
{
  /* Just return an error */

  return -ENOSYS;
}

/****************************************************************************
 * Name: bcm4390x_waitenable
 *
 * Description:
 *   Enable/disable of a set of SDIO wait events.  This is part of the
 *   the SDIO_WAITEVENT sequence.  The set of to-be-waited-for events is
 *   configured before calling bcm4390x_eventwait.  This is done in this way
 *   to help the driver to eliminate race conditions between the command
 *   setup and the subsequent events.
 *
 *   The enabled events persist until either (1) SDIO_WAITENABLE is called
 *   again specifying a different set of wait events, or (2) SDIO_EVENTWAIT
 *   returns.
 *
 * Input Parameters:
 *   dev      - An instance of the SDIO device interface
 *   eventset - A bitset of events to enable or disable (see SDIOWAIT_*
 *              definitions). 0=disable; 1=enable.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_waitenable(FAR struct sdio_dev_s *dev,
                             sdio_eventset_t eventset)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;
  uint32_t waitints;

  DEBUGASSERT(priv != NULL);

  /* Disable event-related interrupts */

  bcm4390x_configwaitints(priv, 0, 0, 0);

  /* Select the interrupt mask that will give us the appropriate wakeup
   * interrupts.
   */

  waitints = 0;
  if ((eventset & (SDIOWAIT_CMDDONE | SDIOWAIT_RESPONSEDONE)) != 0)
    {
      waitints |= SDHC_RESPDONE_INTS;
    }

  if ((eventset & SDIOWAIT_TRANSFERDONE) != 0)
    {
      waitints |= SDHC_XFRDONE_INTS;
    }

  /* Enable event-related interrupts */

  bcm4390x_configwaitints(priv, waitints, eventset, 0);
}

/****************************************************************************
 * Name: bcm4390x_eventwait
 *
 * Description:
 *   Wait for one of the enabled events to occur (or a timeout).  Note that
 *   all events enabled by SDIO_WAITEVENTS are disabled when bcm4390x_eventwait
 *   returns.  SDIO_WAITEVENTS must be called again before bcm4390x_eventwait
 *   can be used again.
 *
 * Input Parameters:
 *   dev     - An instance of the SDIO device interface
 *   timeout - Maximum time in milliseconds to wait.  Zero means immediate
 *             timeout with no wait.  The timeout value is ignored if
 *             SDIOWAIT_TIMEOUT is not included in the waited-for eventset.
 *
 * Returned Value:
 *   Event set containing the event(s) that ended the wait.  Should always
 *   be non-zero.  All events are disabled after the wait concludes.
 *
 ****************************************************************************/

static sdio_eventset_t bcm4390x_eventwait(FAR struct sdio_dev_s *dev,
                                       uint32_t timeout)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;
  sdio_eventset_t wkupevent = 0;
  int ret;

  DEBUGASSERT((priv->waitevents != 0 && priv->wkupevent == 0) ||
              (priv->waitevents == 0 && priv->wkupevent != 0));

  /* Check if the timeout event is specified in the event set */

  if ((priv->waitevents & SDIOWAIT_TIMEOUT) != 0)
    {
      int delay;

      /* Yes.. Handle a corner case */

      if (!timeout)
        {
          return SDIOWAIT_TIMEOUT;
        }

      /* Start the watchdog timer */

      delay = MSEC2TICK(timeout);
      ret   = wd_start(priv->waitwdog, delay, (wdentry_t)bcm4390x_eventtimeout,
                       1, (uint32_t)priv);
      if (ret != OK)
        {
          fdbg("ERROR: wd_start failed: %d\n", ret);
        }
    }

  /* Loop until the event (or the timeout occurs). Race conditions are avoided
   * by calling bcm4390x_waitenable prior to triggering the logic that will cause
   * the wait to terminate.  Under certain race conditions, the waited-for
   * may have already occurred before this function was called!
   */

  for (; ; )
    {
      /* Wait for an event in event set to occur.  If this the event has already
       * occurred, then the semaphore will already have been incremented and
       * there will be no wait.
       */

      bcm4390x_takesem(priv);
      wkupevent = priv->wkupevent;

      /* Check if the event has occurred.  When the event has occurred, then
       * evenset will be set to 0 and wkupevent will be set to a non-zero value.
       */

      if (wkupevent != 0)
        {
          /* Yes... break out of the loop with wkupevent non-zero */

          break;
        }
    }

  /* Disable event-related interrupts */

  bcm4390x_configwaitints(priv, 0, 0, 0);
#ifdef CONFIG_SDIO_DMA
  priv->xfrflags   = 0;
#endif

  bcm4390x_dumpsamples(priv);
  return wkupevent;
}

/****************************************************************************
 * Name: bcm4390x_callbackenable
 *
 * Description:
 *   Enable/disable of a set of SDIO callback events.  This is part of the
 *   the SDIO callback sequence.  The set of events is configured to enabled
 *   callbacks to the function provided in bcm4390x_registercallback.
 *
 *   Events are automatically disabled once the callback is performed and no
 *   further callback events will occur until they are again enabled by
 *   calling this method.
 *
 * Input Parameters:
 *   dev      - An instance of the SDIO device interface
 *   eventset - A bitset of events to enable or disable (see SDIOMEDIA_*
 *              definitions). 0=disable; 1=enable.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void bcm4390x_callbackenable(FAR struct sdio_dev_s *dev,
                                 sdio_eventset_t eventset)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;

  fvdbg("eventset: %02x\n", eventset);
  DEBUGASSERT(priv != NULL);

  priv->cbevents = eventset;
  bcm4390x_callback(priv);
}

/****************************************************************************
 * Name: bcm4390x_registercallback
 *
 * Description:
 *   Register a callback that that will be invoked on any media status
 *   change.  Callbacks should not be made from interrupt handlers, rather
 *   interrupt level events should be handled by calling back on the work
 *   thread.
 *
 *   When this method is called, all callbacks should be disabled until they
 *   are enabled via a call to SDIO_CALLBACKENABLE
 *
 * Input Parameters:
 *   dev -      Device-specific state data
 *   callback - The function to call on the media change
 *   arg -      A caller provided value to return with the callback
 *
 * Returned Value:
 *   0 on success; negated errno on failure.
 *
 ****************************************************************************/

static int bcm4390x_registercallback(FAR struct sdio_dev_s *dev,
                                  worker_t callback, void *arg)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;

  /* Disable callbacks and register this callback and its argument */

  fvdbg("Register %p(%p)\n", callback, arg);
  DEBUGASSERT(priv != NULL);

  priv->cbevents = 0;
  priv->cbarg    = arg;
  priv->callback = callback;
  return OK;
}

#ifdef CONFIG_SDIO_DMA

#define SD_PAGE_BITS    12
#define SD_PAGE     (1 << SD_PAGE_BITS)

static int bcm4390x_alloc_dma_resource(struct sdio_dev_s *dev)
{
    struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;
    uint alloced;
    void *va;

#ifdef WAR_USE_DMA_MEMORY
    dmabuf_len = 0;
    p_dmabuf_va_start = DMA_ALLOC_CONSISTENT(NULL, MAX_MIDDLE_DMA_SIZE, SD_PAGE_BITS, &dmabuf_len, &dmabuf_phy_start, 0x12);
    if (p_dmabuf_va_start == NULL) {
        dbg("ERROR: failed to alloc DMA middle buffer\n");
        return ERROR;
    }
    p_dmabuf_va = (void *)ROUNDUP((uintptr)p_dmabuf_va_start, SD_PAGE);
    dmabuf_phy = ROUNDUP((dmabuf_phy_start), SD_PAGE);
    fdbg("Middle buf for DMA, va=0x%08x / 0x%08x, phy=0x%08x / 0x%08x, sz=%d\n",
            (uint32_t)p_dmabuf_va_start, (uint32_t)p_dmabuf_va,
            dmabuf_phy_start, dmabuf_phy,
            dmabuf_len);
#endif

    alloced = 0;
    if ((va = DMA_ALLOC_CONSISTENT(NULL, SD_PAGE, SD_PAGE_BITS, &alloced,
            &priv->adma2_dscr_start_phys, 0x12)) == NULL) {
#ifdef WAR_USE_DMA_MEMORY
        DMA_FREE_CONSISTENT(NULL, p_dmabuf_va_start, dmabuf_len, dmabuf_phy_start, 0x12);
#endif
        priv->max_num_dma_dscr = 0;
        dbg("ERROR: failed to alloc DMA resources\n");
        return ERROR;
    } else {
        priv->adma2_dscr_start_buf = va;
        priv->adma2_dscr_buf = (void *)ROUNDUP((uintptr)va, SD_PAGE);
        priv->adma2_dscr_phys = ROUNDUP((priv->adma2_dscr_start_phys), SD_PAGE);
        priv->alloced_adma2_dscr_size = alloced;
        memset((char *)priv->adma2_dscr_buf, 0, SD_PAGE);
        priv->max_num_dma_dscr = alloced / sizeof(struct adma2_dscr_32b);
        fdbg("Alloced ADMA2 Descriptor %d bytes @virt/phys: %p/0x%lx(start@ 0x%lx)\n",
                priv->alloced_adma2_dscr_size, priv->adma2_dscr_buf,
                priv->adma2_dscr_phys, priv->adma2_dscr_start_phys);
        fvdbg("Max number of dscr is %d\n", priv->max_num_dma_dscr);
    }

    return OK;
}

static void bcm4390x_free_dma_resource(struct sdio_dev_s *dev)
{
    struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;

#ifdef WAR_USE_DMA_MEMORY
    if (p_dmabuf_va_start) {
        DMA_FREE_CONSISTENT(NULL, p_dmabuf_va_start, dmabuf_len, dmabuf_phy_start, 0x12);
    }
#endif

    if (priv->adma2_dscr_start_buf) {
        DMA_FREE_CONSISTENT(NULL, priv->adma2_dscr_start_buf, priv->alloced_adma2_dscr_size,
                                    priv->adma2_dscr_start_phys, 0x12);
    }
}
#endif

/****************************************************************************
 * Name: bcm4390x_dmasupported
 *
 * Description:
 *   Return true if the hardware can support DMA
 *
 * Input Parameters:
 *   dev - An instance of the SDIO device interface
 *
 * Returned Value:
 *   true if DMA is supported.
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA
static bool bcm4390x_dmasupported(FAR struct sdio_dev_s *dev)
{
  return true;
}
#endif

/****************************************************************************
 * Name: bcm4390x_dmarecvsetup
 *
 * Description:
 *   Setup to perform a read DMA.  If the processor supports a data cache,
 *   then this method will also make sure that the contents of the DMA memory
 *   and the data cache are coherent.  For read transfers this may mean
 *   invalidating the data cache.
 *
 * Input Parameters:
 *   dev    - An instance of the SDIO device interface
 *   buffer - The memory to DMA from
 *   buflen - The size of the DMA transfer in bytes
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA

static int bcm4390x_dmarecvsetup(FAR struct sdio_dev_s *dev, FAR uint8_t *buffer,
                              size_t buflen)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;

  DEBUGASSERT(priv != NULL && buffer != NULL && buflen > 0);
#ifndef WAR_USE_DMA_MEMORY
  /*
   * We can ignore this since following codes copy data in between application buffer
   * and a middle buffer for DMA
   */
  DEBUGASSERT(((uint32_t)buffer & 3) == 0);
#endif

  /* Reset the DPSM configuration */

  bcm4390x_datadisable();

  /* Begin sampling register values */

  bcm4390x_sampleinit();
  bcm4390x_sample(priv, SAMPLENDX_BEFORE_SETUP);

  /* Save the destination buffer information for use by the interrupt handler */

  priv->buffer    = (uint32_t *)buffer;
  priv->remaining = buflen;

#ifdef CONFIG_SDIO_ADMA2
  if (bcm4390x_create_dma_desc(priv, buffer, buflen, DMA_RX)) {
      dbg("ERROR: failed to create DMA DESC. Required size %d bytes\n", buflen);
      return ERROR;
  }
#endif

  /* Then set up the SDIO data path */

  bcm4390x_dataconfig(priv, false, buflen, 1, SDHC_DVS_DATATIMEOUT);

  /* Configure the RX DMA */

  bcm4390x_configxfrints(priv, SDHC_DMADONE_INTS);

#ifdef CONFIG_SDIO_ADMA2
  putreg32((uint32_t)priv->adma2_dscr_phys, BCM4390X_SDHC_ADSADDR);
#else
  putreg32((uint32_t)buffer, BCM4390X_SDHC_DSADDR);
#endif

  /* Sample the register state */

  bcm4390x_sample(priv, SAMPLENDX_AFTER_SETUP);
  return OK;
}
#endif

/****************************************************************************
 * Name: bcm4390x_dmasendsetup
 *
 * Description:
 *   Setup to perform a write DMA.  If the processor supports a data cache,
 *   then this method will also make sure that the contents of the DMA memory
 *   and the data cache are coherent.  For write transfers, this may mean
 *   flushing the data cache.
 *
 * Input Parameters:
 *   dev    - An instance of the SDIO device interface
 *   buffer - The memory to DMA into
 *   buflen - The size of the DMA transfer in bytes
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 ****************************************************************************/

#ifdef CONFIG_SDIO_DMA
static int bcm4390x_dmasendsetup(FAR struct sdio_dev_s *dev,
                              FAR const uint8_t *buffer, size_t buflen)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;

  DEBUGASSERT(priv != NULL && buffer != NULL && buflen > 0);
#ifndef WAR_USE_DMA_MEMORY
  /*
   * We can ignore this since following codes copy data in between buffer and
   * a middle buffer for DMA
   */
  DEBUGASSERT(((uint32_t)buffer & 3) == 0);
#endif

  /* Reset the DPSM configuration */

  bcm4390x_datadisable();

  /* Begin sampling register values */

  bcm4390x_sampleinit();
  bcm4390x_sample(priv, SAMPLENDX_BEFORE_SETUP);

  /* Save the source buffer information for use by the interrupt handler */

  priv->buffer    = (uint32_t *)buffer;
  priv->remaining = buflen;

#ifdef CONFIG_SDIO_ADMA2
  if (bcm4390x_create_dma_desc(priv, (FAR uint8_t *)buffer, buflen, DMA_TX)) {
      dbg("ERROR: failed to create DMA DESC. Required size %d bytes\n", buflen);
      return ERROR;
  }
#endif /* CONFIG_SDIO_DMA */

  /* Then set up the SDIO data path */

  bcm4390x_dataconfig(priv, true, buflen, 1, SDHC_DVS_DATATIMEOUT);

  /* Enable TX interrupts */

  bcm4390x_configxfrints(priv, SDHC_DMADONE_INTS);

  /* Configure the TX DMA */

#ifdef CONFIG_SDIO_ADMA2
  putreg32((uint32_t)priv->adma2_dscr_phys, BCM4390X_SDHC_ADSADDR);
#else
  putreg32((uint32_t)buffer, BCM4390X_SDHC_DSADDR);
#endif

  /* Sample the register state */

  bcm4390x_sample(priv, SAMPLENDX_AFTER_SETUP);

  return OK;
}
#endif

/****************************************************************************
 * Initialization/uninitialization/reset
 ****************************************************************************/
/****************************************************************************
 * Name: bcm4390x_callback
 *
 * Description:
 *   Perform callback.
 *
 * Assumptions:
 *   This function does not execute in the context of an interrupt handler.
 *   It may be invoked on any user thread or scheduled on the work thread
 *   from an interrupt handler.
 *
 ****************************************************************************/

static void bcm4390x_callback(void *arg)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)arg;

  /* Is a callback registered? */

  DEBUGASSERT(priv != NULL);
  fvdbg("Callback %p(%p) cbevents: %02x cdstatus: %02x\n",
        priv->callback, priv->cbarg, priv->cbevents, priv->cdstatus);

  if (priv->callback)
    {
      /* Yes.. Check for enabled callback events */

      if ((priv->cdstatus & SDIO_STATUS_PRESENT) != 0)
        {
          /* Media is present.  Is the media inserted event enabled? */

          if ((priv->cbevents & SDIOMEDIA_INSERTED) == 0)
            {
              /* No... return without performing the callback */

              return;
            }
        }
      else
        {
          /* Media is not present.  Is the media eject event enabled? */

          if ((priv->cbevents & SDIOMEDIA_EJECTED) == 0)
            {
              /* No... return without performing the callback */

              return;
            }
        }

      /* Perform the callback, disabling further callbacks.  Of course, the
       * the callback can (and probably should) re-enable callbacks.
       */

      priv->cbevents = 0;

      /* Callbacks cannot be performed in the context of an interrupt handler.
       * If we are in an interrupt handler, then queue the callback to be
       * performed later on the work thread.
       */

      if (up_interrupt_context())
        {
          /* Yes.. queue it */

           fvdbg("Queuing callback to %p(%p)\n", priv->callback, priv->cbarg);
          (void)work_queue(HPWORK, &priv->cbwork, (worker_t)priv->callback, priv->cbarg, 0);
        }
      else
        {
          /* No.. then just call the callback here */

          fvdbg("Callback to %p(%p)\n", priv->callback, priv->cbarg);
          priv->callback(priv->cbarg);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sdhc_mediachange
 *
 * Description:
 *   Called by board-specific logic -- possible from an interrupt handler --
 *   in order to signal to the driver that a card has been inserted or
 *   removed from the slot
 *
 * Input Parameters:
 *   dev        - An instance of the SDIO driver device state structure.
 *   cardinslot - true is a card has been detected in the slot; false if a
 *                card has been removed from the slot.  Only transitions
 *                (inserted->removed or removed->inserted should be reported)
 *
 * Returned Values:
 *   None
 *
 ****************************************************************************/

void sdhc_mediachange(FAR struct sdio_dev_s *dev, bool cardinslot)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;
  uint8_t cdstatus;
  irqstate_t flags;

  /* Update card status */

  flags = enter_critical_section();
  cdstatus = priv->cdstatus;
  if (cardinslot)
    {
      priv->cdstatus |= SDIO_STATUS_PRESENT;
    }
  else
    {
      priv->cdstatus &= ~SDIO_STATUS_PRESENT;
    }

  fvdbg("cdstatus OLD: %02x NEW: %02x\n", cdstatus, priv->cdstatus);

  /* Perform any requested callback if the status has changed */

  if (cdstatus != priv->cdstatus)
    {
      bcm4390x_callback(priv);
    }

  leave_critical_section(flags);
}

/****************************************************************************
 * Name: sdio_wrprotect
 *
 * Description:
 *   Called by board-specific logic to report if the card in the slot is
 *   mechanically write protected.
 *
 * Input Parameters:
 *   dev       - An instance of the SDIO driver device state structure.
 *   wrprotect - true is a card is writeprotected.
 *
 * Returned Values:
 *   None
 *
 ****************************************************************************/

void sdhc_wrprotect(FAR struct sdio_dev_s *dev, bool wrprotect)
{
  struct bcm4390x_dev_s *priv = (struct bcm4390x_dev_s *)dev;
  irqstate_t flags;

  /* Update card status */

  flags = enter_critical_section();
  if (wrprotect)
    {
      priv->cdstatus |= SDIO_STATUS_WRPROTECTED;
    }
  else
    {
      priv->cdstatus &= ~SDIO_STATUS_WRPROTECTED;
    }

  fvdbg("cdstatus: %02x\n", priv->cdstatus);
  leave_critical_section(flags);
}

/****************************************************************************
 * Name: sdhc_initialize
 *
 * Description:
 *   Initialize SDIO for operation.
 *
 * Input Parameters:
 *   slotno - Not used.
 *
 * Returned Values:
 *   A reference to an SDIO interface structure.  NULL is returned on failures.
 *
 ****************************************************************************/

FAR struct sdio_dev_s *sdhc_initialize(int slotno)
{
  /* There is only one slot */
  struct bcm4390x_dev_s *priv = &g_sdhcdev;
  uint32_t blk_bits;
  uint32_t blk_sz[] = {512, 1024, 2048}; // per sdhc spec

  DEBUGASSERT(slotno == 0);

  /* Initialize the SDHC slot structure data structure */

  sem_init(&priv->waitsem, 0, 0);
  priv->waitwdog = wd_create();
  DEBUGASSERT(priv->waitwdog);

  osl_core_enable(SDIOH_CORE_ID);

  priv->caps = getreg32(BCM4390X_SDHC_HTCAPBLT);
  priv->caps3 = getreg32(BCM4390X_SDHC_HTCAPBLT3);
  priv->version = getreg16(BCM4390X_SDHC_HOSTVER) & 0xff;

  blk_bits = (priv->caps >> 16) & 0x3;
  priv->max_blk_len = (blk_bits > 2 ? 512 : blk_sz[blk_bits]);

  /* ttq: not really sure if sd clk source is from CPU. but based on my tests, it is from 320 Mhz and it is the same with cpu clk speed */
  priv->sd_base_clk = platform_reference_clock_get_freq( PLATFORM_REFERENCE_CLOCK_CPU );

#ifdef CONFIG_SDIO_DMA
  /* XXX we only support ADAM2 for now */
  priv->sd_dma_mode = DMA_MODE_ADMA2;
  fvdbg("ADMA2 is enabled\n" );
#else
  fvdbg("PIO mode is enabled\n");
#endif

  fvdbg("Max blk sz=%d, caps=0x%x, caps3=0x%x, spec ver=%d, base clk=%d\n",
          priv->max_blk_len,
          priv->caps,
          priv->caps3,
          priv->version+1,
          priv->sd_base_clk);

  /* ttn: Below "if block" is from bcmsdstd. We also use sd_base_clk to store clk speed. */
  if (GFIELD(priv->caps, CAP_BASECLK) == 0) {
      /* XXX: base clock freq is 0, gets the HT clock as the base clock */
#if 0
      dbg("Freq check: cpu=%dK bp=%dK alp=%dK ilp=%dK\n",
              platform_reference_clock_get_freq( PLATFORM_REFERENCE_CLOCK_CPU )/1000,        /* 320 Mhz */
              platform_reference_clock_get_freq( PLATFORM_REFERENCE_CLOCK_BACKPLANE )/1000,  /* 160 Mhz */
              platform_reference_clock_get_freq( PLATFORM_REFERENCE_CLOCK_ALP )/1000,
              platform_reference_clock_get_freq( PLATFORM_REFERENCE_CLOCK_ILP )/1000);
#endif
      uint32_t ht_clk = platform_reference_clock_get_freq( PLATFORM_REFERENCE_CLOCK_BACKPLANE );
      priv->caps |= (ht_clk / 1000000) << 8;
  }

  /* Reset the card and assure that it is in the initial, unconfigured
   * state.
   */

#ifdef CONFIG_SDIO_DMA
  if (bcm4390x_alloc_dma_resource(&priv->dev) != OK) {
      return NULL;
  }
#endif

  bcm4390x_reset(&priv->dev);

  return &g_sdhcdev.dev;
}

int bcm4390x_sdhc_archinitialize(void)
{
  int ret;

  if (!sdhc_initialize(0)) {
      dbg("ERROR: Failed to init sdhc\n");
      return -1;
  }

  ret = mmcsd_slotinitialize(CONFIG_NSH_MMCSDMINOR, &g_sdhcdev.dev);
  if (ret != OK)
    {
      dbg("ERROR: Failed to bind SDHC to the MMC/SD driver: %d\n", ret);
      return ret;
    }

  /* Handle the initial card state */

  sdhc_mediachange(&g_sdhcdev.dev, 1);

  /* todo: Enable CD interrupts to handle subsequent media changes */

  return OK;
}

#endif /* CONFIG_BCM4390X_SDHC */
