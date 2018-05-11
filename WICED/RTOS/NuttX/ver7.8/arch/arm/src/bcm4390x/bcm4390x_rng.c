/*
 * arch/arm/src/bcm4390x/bcm4390x_rng.c
 *
 * $ Copyright Broadcom Corporation $
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>

#include "up_arch.h"
#include "up_internal.h"

#include "wiced_crypto.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t rng_read(struct file *filep, char *buffer, size_t);

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_rngops =
{
  . read = rng_read,
};

/****************************************************************************
 * Private functions
 ****************************************************************************/

/****************************************************************************
 * Name: rng_read
 *
 * Description:
 *   This is the standard, NuttX character driver read method
 *
 * Input Parameters:
 *   filep - The VFS file instance
 *   buffer - Buffer in which to return the random samples
 *   buflen - The length of the buffer
 *
 * Returned Value:
 *
 ****************************************************************************/

static ssize_t rng_read(struct file *filep, char *buffer, size_t buflen)
{
  ssize_t retval;
  int ret;
  (void) filep;

  if (buffer == NULL || buflen == 0)
    {
      retval = -EFAULT;
    }
  else
    {
      ret = wiced_crypto_get_random(buffer, buflen);
      if (ret == WICED_SUCCESS)
        {
          retval = buflen;
        }
      else
        {
          retval = -EIO;
        }
    }
  return retval;
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_rnginitialize
 *
 * Description:
 *   Register the /dev/randome driver.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void up_rnginitialize(void)
{
  int ret;

  ret = register_driver("/dev/random", &g_rngops, 0644, NULL);
  if (ret < 0)
    {
      dbg("ERROR: Failed to register /dev/random\n");
      return;
    }
}






