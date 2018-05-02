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
#include "device.h"

int cmd_fast_charge(int argc, char* argv[])
{
	int val;
	if (argc < 2)
		return ERR_INSUFFICENT_ARGS;

	val = !!atoi(argv[1]);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Fast Charge: %s (%d)\n", val ? "Enabled" : "Disabled", val);
	if (val)
		wiced_gpio_output_high(GPO_ENABLE_FAST_CHARGE);
	else
		wiced_gpio_output_low(GPO_ENABLE_FAST_CHARGE);

	/* reset charger device */
	wiced_gpio_output_high(GPO_CHARGE_CONTROL);
	wiced_rtos_delay_milliseconds(100);
	wiced_gpio_output_low(GPO_CHARGE_CONTROL);
	return ERR_CMD_OK;
}

int cmd_usb_detect(int argc, char* argv[])
{
	int val = wiced_gpio_input_get(GPIO_BUTTON_USB_DETECT);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "USB Detect: %s (%d)\n", val ? "IDLE" : "CHARGING", val);
	return ERR_CMD_OK;
}

static void update_sensor(uint16_t* v, uint16_t* i)
{
	*v = a_dev_adc_get(WICED_ADC_1);
	*i = a_dev_adc_get(WICED_ADC_2);
}

int cmd_usb_test(int argc, char* argv[])
{
	int cnt;
	uint16_t v, i;

	update_sensor(&v, &i);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Default V=%d, I=%d\n", v, i);
	wiced_gpio_output_high(GPO_LOAD_TEST);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Turn On Load\n");
	for (cnt = 0; cnt < 10; cnt++) {
		update_sensor(&v, &i);
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "%d: Default V=%d, I=%d\n", cnt+1, v, i);
		wiced_rtos_delay_milliseconds(500);
	}
	wiced_gpio_output_low(GPO_LOAD_TEST);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Turn Off Load\n");
	for (cnt = 0; cnt < 4; cnt++) {
		update_sensor(&v, &i);
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "%d: Default V=%d, I=%d\n", cnt+1, v, i);
		wiced_rtos_delay_milliseconds(500);
	}
	return ERR_CMD_OK;
}
