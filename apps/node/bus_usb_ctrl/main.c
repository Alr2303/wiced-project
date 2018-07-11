/*
 * Copyright (C) 2018 HummingLab. - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */
#include "wiced.h"
#include "wiced_log.h"
#include "command_console.h"

#include "assert_macros.h"
#include "device.h"
#include "main.h"
#include "ctrl_console.h"

#include <unistd.h>
#include <math.h>

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define VERSION 0.0.4

const char* fw_version = EXPAND_AND_QUOTE(VERSION);
const char* fw_model = "BUS-USB-CTRL";

#define UART_RX_BUFFER_SIZE 256

#define AVG_ALPHA	0.33 	/* N=5, smoothing factor = 2 / (1 + N)  */


const static int port[MAX_SWITCH] = {
	BUSGW_GPO_P1_POWER,
	BUSGW_GPO_P2_POWER,
	BUSGW_GPO_P3_POWER,
	BUSGW_GPO_P4_POWER,
	BUSGW_GPO_AP_POWER
};

int adc_avg[WICED_ADC_MAX];
float voltage[MAX_PORT];
float current[MAX_PORT];
int on_mask[MAX_SWITCH];
int on[MAX_SWITCH];
int temperature;
int humidity;
wiced_bool_t low_battery;

void apply_switch(wiced_bool_t slow)
{
	int i;
	for (i = 0; i < MAX_SWITCH; i++) {
		if (low_battery || on_mask[i] || !on[i])
			wiced_gpio_output_low(port[i]);
		else
			wiced_gpio_output_high(port[i]);

		if (slow)
			wiced_rtos_delay_milliseconds(200);
	}
}

static void fill_adc(void)
{
	int i;
	for (i = 0; i < WICED_ADC_MAX; i++) {
		adc_avg[i] = (int)a_dev_adc_get(WICED_ADC_1 + i);
	}
}

static void read_adc(void)
{
	int i;
	for (i = 0; i < WICED_ADC_MAX; i++) {
		int v = (int)a_dev_adc_get(WICED_ADC_1 + i);
		adc_avg[i] = (int)(v * AVG_ALPHA + adc_avg[i] * (1 - AVG_ALPHA));
	}

	for (i = 0; i < MAX_PORT; i++) {
		voltage[i] = (float)adc_avg[i] / 112;
		current[i] = (float)adc_avg[i + MAX_PORT] / 260;
	}
}

static void limit_abnormal_power(void)
{
	int i;
	wiced_bool_t changed = WICED_FALSE;
	float max_voltage = -1;
	wiced_time_t now;

	for (i = 0; i < MAX_PORT; i++) {
		if (voltage[i] > max_voltage)
			max_voltage = voltage[i];
	}

#if 1
	/* start checking after 60s */
	wiced_time_get_time(&now);
	if (now > 60 * 1000 && !low_battery && max_voltage < 21.) {
		/* low battery, power off all port */
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Low Battery: %.1f V\n", max_voltage);
		low_battery = WICED_TRUE;
		changed = WICED_TRUE;
	}
#endif

	for (i = 0; i < MAX_PORT; i++) {
		if (current[i] > 9.) {
			/* over current */
			wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Over current port %d: %.2f A\n", i, current[i]);
			on_mask[i] = WICED_TRUE;
			changed = WICED_TRUE;
		}
	}

	if (changed)
		apply_switch(WICED_FALSE);
}

static int log_output_handler(WICED_LOG_LEVEL_T level, char *log_msg)
{
	write(STDOUT_FILENO, log_msg, strlen(log_msg));
	return 0;
}

void application_start(void)
{
	int i;
	uint32_t cnt;

	wiced_core_init();
	wiced_log_init(WICED_LOG_INFO, log_output_handler, NULL);

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "USB Gateway Controller: v%s\n", fw_version);
	usb_console_init();

	a_dev_temp_humid_init();
	a_dev_adc_init();
	fill_adc();

	wiced_rtos_delay_milliseconds(1000);

	/* turn on */
	for (i = 0; i < MAX_SWITCH; i++) {
		on[i] = WICED_TRUE;
		on_mask[i] = WICED_FALSE;
	}
	apply_switch(WICED_TRUE);

	for (cnt = 0; ; cnt++) {
		/* read adc 4 times per sec */
		read_adc();
		limit_abnormal_power();
		if (low_battery)
			break;

		/* read temperature and humidy every 2 sec */
		if (cnt % 8 == 0) {
			float t, h;
			int v;
			a_dev_temp_humid_get(&t, &h);

			temperature = (int)round(t);
			v = (int)round(h);
			if (v < 0)
				v = 0;
			else if (v > 100)
				v = 100;
			humidity = v;
		}
		wiced_rtos_delay_milliseconds(260);
	}

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Stop all sensing...\n");
	while (1) {
		wiced_rtos_delay_milliseconds(10000);
	}
}

