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
#include "platform_stdio.h"

#include "assert_macros.h"
#include "device.h"

#include <math.h>

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define VERSION 0.0.1

const char* fw_version = EXPAND_AND_QUOTE(VERSION);
const char* fw_model = "BUS-USB-CTRL";

#define UART_RX_BUFFER_SIZE 256

#define CONSOLE_UART	APP_UART
#define MAX_PORT	5
#define AVG_ALPHA	0.33 	/* N=5, smoothing factor = 2 / (1 + N)  */

#define COMMAND_HISTORY_LENGTH  (5)
#define MAX_COMMAND_LENGTH      (180)

static int cmd_hello(int argc, char* argv[]);
static int cmd_version(int argc, char* argv[]);
static int cmd_set(int argc, char* argv[]);
static int cmd_read(int argc, char* argv[]);

static char command_buffer[MAX_COMMAND_LENGTH];
static char command_history_buffer[MAX_COMMAND_LENGTH * COMMAND_HISTORY_LENGTH];
static const command_t cons_commands[] =
{
	{ "hello", cmd_hello, 0, NULL, NULL, NULL, "Hello" },
	{ "version", cmd_version, 0, NULL, NULL, NULL, "Read Version" },
	{ "read", cmd_read, 0, NULL, NULL, NULL, "Read All Value" } ,
	{ "set", cmd_set, 5, NULL, NULL, "each port", "Set Power Control" },
	CMD_TABLE_END
};

const static int port[MAX_PORT] = {
	BUSGW_GPO_P1_POWER,
	BUSGW_GPO_P2_POWER,
	BUSGW_GPO_P3_POWER,
	BUSGW_GPO_P4_POWER,
	BUSGW_GPO_USB_POWER
};

static wiced_uart_config_t uart_app_config =
{
	.baud_rate    = 115200,
	.data_width   = DATA_WIDTH_8BIT,
	.parity       = NO_PARITY,
	.stop_bits    = STOP_BITS_1,
	.flow_control = FLOW_CONTROL_DISABLED
};
static wiced_ring_buffer_t rx_buffer;
static uint8_t rx_data[UART_RX_BUFFER_SIZE];


static int adc_avg[WICED_ADC_MAX];
static int on[MAX_PORT];
static int temperature;
static int humidity;

static int cmd_hello(int argc, char* argv[])
{
	char* buf = "world\r\n";
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(CONSOLE_UART, buf, strlen(buf));
	platform_stdio_tx_unlock();
	return ERR_CMD_OK;
}

static int cmd_version(int argc, char* argv[])
{
	int len;
	char buf[128];
	len = sprintf(buf, "%s\r\n", fw_version);
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(CONSOLE_UART, buf, len);
	platform_stdio_tx_unlock();
	return ERR_CMD_OK;
}

static int cmd_set(int argc, char* argv[])
{
	int i;
	int len;
	char buf[128];
	if (argc < 6)
		return ERR_INSUFFICENT_ARGS;

	for (i = 0; i < MAX_PORT; i++) {
		if (argv[i+1][0] == '1') {
			on[i] = 1;
			wiced_gpio_output_high(port[i]);
		} else {
			on[i] = 0;
			wiced_gpio_output_low(port[i]);
		}
	}

	len = sprintf(buf, "%d %d %d %d %d\r\n", on[0], on[1], on[2], on[3], on[4]);
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(CONSOLE_UART, buf, len);
	platform_stdio_tx_unlock();
	return ERR_CMD_OK;
}

static int cmd_read(int argc, char* argv[])
{
	int len;
	char buf[256];

	len = sprintf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\r\n",
		      temperature, humidity, on[0], on[1], on[2], on[3], on[4],
		      adc_avg[0], adc_avg[1], adc_avg[2], adc_avg[3],
		      adc_avg[4], adc_avg[5], adc_avg[6], adc_avg[7]);
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(CONSOLE_UART, buf, len);
	platform_stdio_tx_unlock();
	return ERR_CMD_OK;
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
}

void application_start(void)
{
	int i;
	uint32_t cnt;

	wiced_core_init();

	ring_buffer_init(&rx_buffer, rx_data, UART_RX_BUFFER_SIZE);
	wiced_uart_init(APP_UART, &uart_app_config, &rx_buffer);

	command_console_init(CONSOLE_UART, sizeof(command_buffer), command_buffer,
			     COMMAND_HISTORY_LENGTH, command_history_buffer, " ");
	console_add_cmd_table(cons_commands);

	a_dev_temp_humid_init();
	a_dev_adc_init();
	fill_adc();

	/* turn on */
	for (i = 0; i < MAX_PORT; i++) {
		wiced_gpio_output_high(port[i]);
		on[i] = WICED_TRUE;
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

