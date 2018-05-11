/*
 * $ Copyright Broadcom Corporation $
 */

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** USBX Component                                                        */
/**                                                                       */
/**   BCM4390X Controller Driver                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#ifndef UX_DCD_BCM4390X_H
#define UX_DCD_BCM4390X_H

#include "siutils.h"
#include "sbusbd.h"
#include "usbdev.h"


//#define UX_DCD_BCM4390X_DEBUG_TRACE_ENABLE
#define UX_DCD_BCM4390X_PRINT_MACRO_DEBUG(args) do {printf args;} while(0==1)

#ifdef UX_DCD_BCM4390X_DEBUG_TRACE_ENABLE
#define UX_DCD_BCM4390X_DEBUG(args)     UX_DCD_BCM4390X_PRINT_MACRO_DEBUG(args)
#else
#define UX_DCD_BCM4390X_DEBUG(args)
#endif


/* Define BCM4390X generic equivalences.  */

#define UX_DCD_BCM4390X_SLAVE_CONTROLLER                           0x80
#define UX_DCD_BCM4390X_MAX_ED                                     EP_MAX
#define UX_DCD_BCM4390X_MAX_ED_NUM_IN_ADDRESS                      USB_MAX_ENDPOINTS

#define UX_DCD_BCM4390X_SOFTCONN_TIMER                             10
#define UX_DCD_BCM4390X_RESET_BOUNCING_TIMEOUT                     30


#define UX_DCD_BCM4390X_MAX_PKT_TRANSFER                           PKTBUFSZ

#define UX_DCD_BCM4390X_HW_EP_UNSET                                0xFF


/* Define USB BCM4390X endpoint transfer status definition.  */

#define UX_DCD_BCM4390X_ED_TRANSFER_STATUS_IDLE                    0
#define UX_DCD_BCM4390X_ED_TRANSFER_STATUS_SETUP                   1
#define UX_DCD_BCM4390X_ED_TRANSFER_STATUS_IN_COMPLETION           2
#define UX_DCD_BCM4390X_ED_TRANSFER_STATUS_OUT_COMPLETION          3

/* Define USB BCM4390X endpoint transfer callback phase definition.  */

#define UX_DCD_BCM4390X_ED_TRANSFER_PHASE_SETUP                    0
#define UX_DCD_BCM4390X_ED_TRANSFER_PHASE_SET_CONFIGURATION        1
#define UX_DCD_BCM4390X_ED_TRANSFER_PHASE_SET_INTERFACE            2

#define UX_DCD_BCM4390X_ED_TRANSFER_PHASE_DMA_TX_DONE              11
#define UX_DCD_BCM4390X_ED_TRANSFER_PHASE_DMA_CTRL_RX_DONE         12
#define UX_DCD_BCM4390X_ED_TRANSFER_PHASE_DMA_NONCTRL_RX_DONE      13

/* Define USB BCM4390X physical endpoint status definition.  */

#define UX_DCD_BCM4390X_ED_STATUS_UNUSED                           0
#define UX_DCD_BCM4390X_ED_STATUS_USED                             1
#define UX_DCD_BCM4390X_ED_STATUS_TRANSFER                         2
#define UX_DCD_BCM4390X_ED_STATUS_STALLED                          4

/* Define USB BCM4390X physical endpoint state machine definition.  */

#define UX_DCD_BCM4390X_ED_STATE_IDLE                              0
#define UX_DCD_BCM4390X_ED_STATE_DATA_TX                           1
#define UX_DCD_BCM4390X_ED_STATE_DATA_RX                           2
#define UX_DCD_BCM4390X_ED_STATE_STATUS_TX                         3
#define UX_DCD_BCM4390X_ED_STATE_STATUS_RX                         4



/* Define USB BCM4390X physical endpoint structure.  */

typedef struct UX_DCD_BCM4390X_ED_STRUCT
{

    ULONG           ux_dcd_bcm4390x_ed_index;
    ULONG           ux_dcd_bcm4390x_ed_hw_ep;
    ULONG           ux_dcd_bcm4390x_ed_type;
    ULONG           ux_dcd_bcm4390x_ed_direction;
    ULONG           ux_dcd_bcm4390x_ed_status;
    ULONG           ux_dcd_bcm4390x_ed_state;
    ULONG           ux_dcd_bcm4390x_ed_payload_length;
    ULONG           ux_dcd_bcm4390x_ed_transfer_status;
    struct UX_SLAVE_ENDPOINT_STRUCT
                    *ux_dcd_bcm4390x_ed_endpoint;
} UX_DCD_BCM4390X_ED;


/* Define USB BCM4390X DCD structure definition.  */

typedef struct UX_DCD_BCM4390X_STRUCT
{
    struct UX_SLAVE_DCD_STRUCT
                    *ux_dcd_bcm4390x_dcd_owner;
    struct UX_DCD_BCM4390X_ED_STRUCT
                    ux_dcd_bcm4390x_ed[UX_DCD_BCM4390X_MAX_ED];                              /* Stored in hardware (chip level) physical endpoint number order.  */
    UCHAR           ux_dcd_bcm4390x_address_to_hw_ep[UX_DCD_BCM4390X_MAX_ED_NUM_IN_ADDRESS]; /* "USB stack endpoint number in address -> hardware physical endpoint number" mapping table.  */
    ULONG           ux_dcd_bcm4390x_base;
    ULONG           ux_dcd_bcm4390x_reset_timeout;
    ULONG           ux_dcd_bcm4390x_debug;
    VOID            *ux_dcd_bcm4390x_thread_stack;
    TX_THREAD       ux_dcd_bcm4390x_thread;
    TX_SEMAPHORE    ux_dcd_bcm4390x_semaphore;
    usbdev_ch_t     *ux_dcd_bcm4390x_chip_handle;

} UX_DCD_BCM4390X;



/* Define USB BCM4390X DCD prototypes.  */

UINT    _ux_dcd_bcm4390x_address_set(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG address);

ULONG   _ux_dcd_bcm4390x_endpoint_register_address_get(UX_DCD_BCM4390X_ED *ed);
UINT    _ux_dcd_bcm4390x_endpoint_create(UX_DCD_BCM4390X *dcd_bcm4390x, UX_SLAVE_ENDPOINT *endpoint);
UINT    _ux_dcd_bcm4390x_endpoint_destroy(UX_DCD_BCM4390X *dcd_bcm4390x, UX_SLAVE_ENDPOINT *endpoint);
UINT    _ux_dcd_bcm4390x_endpoint_reset(UX_DCD_BCM4390X *dcd_bcm4390x, UX_SLAVE_ENDPOINT *endpoint);
UINT    _ux_dcd_bcm4390x_endpoint_stall(UX_DCD_BCM4390X *dcd_bcm4390x, UX_SLAVE_ENDPOINT *endpoint);
UINT    _ux_dcd_bcm4390x_endpoint_status(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG endpoint_index);
ULONG   _ux_dcd_bcm4390x_endpoint_map_hw_ep(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG endpoint_index);

UINT    _ux_dcd_bcm4390x_frame_number_get(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG *frame_number);

UINT    _ux_dcd_bcm4390x_function(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter);
UINT    _ux_dcd_bcm4390x_initialize_complete(VOID);
VOID    _ux_dcd_bcm4390x_interrupt_handler(VOID);

VOID    _ux_dcd_bcm4390x_register_clear(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG bcm4390x_register, ULONG value);
ULONG   _ux_dcd_bcm4390x_register_read(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG bcm4390x_register);
VOID    _ux_dcd_bcm4390x_register_set(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG bcm4390x_register, ULONG value);
VOID    _ux_dcd_bcm4390x_register_write(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG bcm4390x_register, ULONG value);

UINT    _ux_dcd_bcm4390x_state_change(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG state);

UINT    _ux_dcd_bcm4390x_transfer_callback(UX_DCD_BCM4390X *dcd_bcm4390x, ULONG hw_endpoint_number, ULONG callback_phase, VOID *param_in_out);
UINT    _ux_dcd_bcm4390x_transfer_request(UX_DCD_BCM4390X *dcd_bcm4390x, UX_SLAVE_TRANSFER *transfer_request);

UINT    _ux_dcd_bcm4390x_initialize(ULONG dcd_io, ULONG parameter);
UINT    _ux_dcd_bcm4390x_uninitialize(VOID);
VOID    _ux_dcd_bcm4390x_interrupt_thread(ULONG dcd_pointer);

#define ux_dcd_bcm4390x_initialize                      _ux_dcd_bcm4390x_initialize
#define ux_dcd_bcm4390x_uninitialize                    _ux_dcd_bcm4390x_uninitialize
#define ux_dcd_bcm4390x_interrupt_handler               _ux_dcd_bcm4390x_interrupt_handler


/* Define USB BCM4390X DCD inter-communication APIs with BCM4390X hradware level chip handle functions.  */
bool    _ux_dcd_bcm4390x_ch_dpc(UX_DCD_BCM4390X *dcd_bcm4390x, usbdev_ch_t *ch);

uint    _ux_dcd_bcm4390x_ep_attach(UX_DCD_BCM4390X *dcd_bcm4390x, usbdev_ch_t *ch, UX_ENDPOINT_DESCRIPTOR *endpoint, int config, int interface, int alternate);
void    _ux_dcd_bcm4390x_ep_detach(UX_DCD_BCM4390X *dcd_bcm4390x, usbdev_ch_t *ch, int ep);

int     _ux_dcd_bcm4390x_ch_dma(UX_DCD_BCM4390X *dcd_bcm4390x, usbdev_ch_t *ch, int dma_ch);
int     _ux_dcd_bcm4390x_ch_devreq(UX_DCD_BCM4390X *dcd_bcm4390x, usbdev_ch_t *ch);
void    _ux_dcd_bcm4390x_ch_setcfg(UX_DCD_BCM4390X *dcd_bcm4390x, usbdev_ch_t *ch, int config_num);





#endif

