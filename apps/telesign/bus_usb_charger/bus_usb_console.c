/*
 * Copyright (C) 2018 HummingLab - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */
#include "wiced.h"
#include "command_console.h"
#include "wwd_debug.h"
#include "wiced_log.h"

#include "console.h"
#include "app_dct.h"
#include "util.h"
#include "sysloop.h"
#include "device.h"

int cmd_led(int argc, char* argv[])
{
	int val;
	if (argc < 2)
		return ERR_INSUFFICENT_ARGS;

	val = !!atoi(argv[1]);
	printf("Power LED: %d\n", val);
	if (val)
		wiced_gpio_output_low(USBCG_LED_POWER);
	else
		wiced_gpio_output_high(USBCG_LED_POWER);
	return ERR_CMD_OK;
}

int cmd_fast_charge(int argc, char* argv[])
{
	int val;
	if (argc < 2)
		return ERR_INSUFFICENT_ARGS;

	val = !!atoi(argv[1]);
	printf("Fast Charge: %s (%d)\n", val ? "Enabled" : "Disabled", val);
	if (val)
		wiced_gpio_output_high(GPO_ENABLE_FAST_CHARGE);
	else
		wiced_gpio_output_low(GPO_ENABLE_FAST_CHARGE);
	return ERR_CMD_OK;
}

int cmd_usb_detect(int argc, char* argv[])
{
	int val = wiced_gpio_input_get(GPIO_BUTTON_USB_DETECT);
	printf("USB Detect: %s (%d)\n", val ? "IDLE" : "CHARGING", val);
	return ERR_CMD_OK;
}
