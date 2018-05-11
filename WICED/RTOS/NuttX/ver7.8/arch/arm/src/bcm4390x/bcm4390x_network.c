/*
 * $ Copyright Broadcom Corporation $
 */

#include <nuttx/config.h>

#include <assert.h>

#include "up_internal.h"

#include "bcm4390x_wwd.h"

void up_netinitialize(void)
{
#ifdef CONFIG_BCM4390X_WWD
  int err = bcm4390x_wwd_init();
  DEBUGASSERT(err == OK);
#endif /* CONFIG_BCM4390X_WWD */
}
