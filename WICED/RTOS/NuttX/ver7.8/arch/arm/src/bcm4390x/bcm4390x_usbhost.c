/*
 * $ Copyright Broadcom Corporation $
 */

/********************************************************************************************
 * arch/arm/src/bcm4390x/bcm4390x_usbhost.c
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
 ********************************************************************************************/

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <nuttx/usb/usbhost.h>

#include <nuttx/usb/usbhost_trace.h>

#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "bcm4390x_usbhost.h"
#include "platform_usb.h"
#include "bcm4390x_ehci.h"
#include "bcm4390x_ohci.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/
#define BCM4390x_PORT_NUM 1
#define HCI_THREAD_STACK_SIZE        (1024 + 1400)
#define HCI_THREAD_PRIORITY   SCHED_PRIORITY_MAX


/*******************************************************************************
 * Private Types
 *******************************************************************************/
typedef struct usb_host_enum {
        bool ehci_controller;
        bool connected[BCM4390x_PORT_NUM];
        struct usbhost_connection_s *xhci_conn;
}usb_host_enum_t;

static host_thread_type_t    ehci_thread;
static host_thread_type_t    ohci_thread;
/*******************************************************************************
 * Private Function Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Public Data
 *******************************************************************************/

/*******************************************************************************
 * Private Functions
 *******************************************************************************/

/******************************************************
 *            Static Function Definitions
 ******************************************************/
static void xhci_enum_thread(wwd_thread_arg_t arg);
/*******************************************************************************
 * Public Functions
 *******************************************************************************/


static usb_host_enum_t ehci_usbh, ohci_usbh;

static void xhci_enum_thread( wwd_thread_arg_t arg )
{
    usb_host_enum_t *context = (usb_host_enum_t *)arg;
    struct usbhost_connection_s *conn = context->xhci_conn;
    int port_num;
    int ret;

    memset( context->connected, 0, sizeof(context->connected) );

    while (1)
    {
        port_num = conn->wait( conn, context->connected );
        context->connected[port_num] = context->connected[port_num] ? 0 : 1;

        /* Device connected */
        if ( context->connected[port_num] )
        {

            udbg( "%s, %s port: %d conncted, calling enum function\n", __FUNCTION__, context->ehci_controller ? "EHCI" : "OHCI", port_num s);
            ret = conn->enumerate( conn, port_num );
            if (ret != OK)
                 udbg("%s: Enumerate failed\n", __FUNCTION__);
        }
        else /* Device disconnected */
            udbg("%s, device is disconnected\n", __FUNCTION__);

    }

}

int bcm4390x_usb_host_initialize( void )
{
    platform_result_t status;
    wwd_result_t retval;
    int ret;

    status = platform_usb_host_init();

    if (status != PLATFORM_SUCCESS)
    {
        dbg( "USB20 Host HW init failed. status=%d\n", status );
        goto exit;
    }

    ehci_usbh.ehci_controller = 1;
    ehci_usbh.xhci_conn = bcm4390x_ehci_initialize();
    ohci_usbh.xhci_conn = bcm4390x_ohci_initialize();
    ohci_usbh.ehci_controller = 0;

    platform_usb_host_post_init();

#ifdef CONFIG_USBHOST_MSC
    ret = usbhost_storageinit();
    if (ret != OK)
    {
        dbg( "ERROR: Failed to register the mass storage class: %d\n", ret );
        goto exit;
    }
#endif

#ifdef CONFIG_USBHOST_HIDMOUSE

    ret = usbhost_mouse_init();
    if (ret != OK)
    {
        dbg("ERROR: Failed to register the mouse class: %d\n", ret);
        goto exit;
    }
#endif
    /* EHCI HOST thread */
    retval = host_rtos_create_thread_with_arg( &ehci_thread, xhci_enum_thread, "EHCI", NULL, (uint32_t) HCI_THREAD_STACK_SIZE, (uint32_t) HCI_THREAD_PRIORITY, (wwd_thread_arg_t)&ehci_usbh );
    if ( retval != WWD_SUCCESS )
    {
        dbg("%s, Crate EHCI thread failed\n", __FUNCTION__);

        goto exit;

    }

    /* OHCI HOST thread */
    retval = host_rtos_create_thread_with_arg( &ohci_thread, xhci_enum_thread, "OHCI", NULL, (uint32_t) HCI_THREAD_STACK_SIZE, (uint32_t) HCI_THREAD_PRIORITY, (wwd_thread_arg_t)&ohci_usbh  );
    if ( retval != WWD_SUCCESS )
    {

        dbg( "%s, Crate OHCI thread failed\n", __FUNCTION__ );

        goto exit;
    }

    return OK;


    exit:
    dbg( "Error ret: %d\n", ret );
    return -ENODEV;
}
