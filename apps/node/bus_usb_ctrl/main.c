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
#include "ctrl_console.h"

#include <unistd.h>
#include <math.h>

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define VERSION 0.0.3

const char* fw_version = EXPAND_AND_QUOTE(VERSION);
const char* fw_model = "BUS-USB-CTRL";

#define UART_RX_BUFFER_SIZE 256

#define MAX_PORT	5
#define AVG_ALPHA	0.33 	/* N=5, smoothing factor = 2 / (1 + N)  */


const static int port[MAX_PORT] = {
	BUSGW_GPO_P1_POWER,
	BUSGW_GPO_P2_POWER,
	BUSGW_GPO_P3_POWER,
	BUSGW_GPO_P4_POWER,
	BUSGW_GPO_AP_POWER
};

static int adc_avg[WICED_ADC_MAX];
static int on[MAX_PORT];
static int temperature;
static int humidity;

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
	for (i = 0; i < MAX_PORT; i++) {
		wiced_gpio_output_high(port[i]);
		on[i] = WICED_TRUE;
		wiced_rtos_delay_milliseconds(300);
	}

	for (cnt = 0; ; cnt++) {
		/* read adc 4 times per sec */
		read_adc();

		/* read temperature and humidy every sec */
		if (cnt % 4 == 0) {
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
}

