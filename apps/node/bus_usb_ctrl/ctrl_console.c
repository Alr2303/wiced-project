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
#include "assert_macros.h"
#include "platform_stdio.h"
#include "command_console.h"

#include "console.h"
#include "util.h"
#include "main.h"
#include "ctrl_console.h"

#define DEV_UART APP_UART
#define MAX_CONSOLE_LINE  30
#define MAX_ARGC  10
#define UART_RX_BUFFER_SIZE 64

#define DEV_CONSOLE_THREAD_STACK_SIZE (3*1024)

typedef struct
{
	const char* name; /* The command name matched at the command line. */
	command_function_t command; /* Functionction that runs the command. */
	int arg_count;		    /* Minimum number of arguments. */
} dev_command_t;

static wiced_ring_buffer_t rx_buffer;
static uint8_t rx_data[UART_RX_BUFFER_SIZE];
static wiced_thread_t dev_console_thread;
static wiced_mutex_t mutex;
static char console_buf[MAX_CONSOLE_LINE+1];
static int console_pos = 0;

#define COMMAND_HISTORY_LENGTH  (5)
#define MAX_COMMAND_LENGTH      (180)
static char command_buffer[MAX_COMMAND_LENGTH];
static char command_history_buffer[MAX_COMMAND_LENGTH * COMMAND_HISTORY_LENGTH];

static void puts_err(int uart) {
	static const char* err = "ERR\r\n";
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(uart, err, strlen(err));
	platform_stdio_tx_unlock();
}

static void puts_ok(int uart) {
	static const char* ok = "OK\r\n";
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(uart, ok, strlen(ok));
	platform_stdio_tx_unlock();
}

static wiced_uart_config_t uart_app_config =
{
	.baud_rate    = 115200,
	.data_width   = DATA_WIDTH_8BIT,
	.parity       = NO_PARITY,
	.stop_bits    = STOP_BITS_1,
	.flow_control = FLOW_CONTROL_DISABLED
};

static void _hello(int uart)
{
	const char* buf = "world\r\n";
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(uart, buf, strlen(buf));
	platform_stdio_tx_unlock();
}

static int cmd_hello(int argc, char* argv[])
{
	_hello(STDIO_UART);
	return ERR_CMD_OK;
}

static int dev_cmd_hello(int argc, char* argv[])
{
	_hello(DEV_UART);
	return ERR_CMD_OK;
}

static void _version(int uart)
{
	int len;
	char buf[128];
	len = sprintf(buf, "%s\r\n", fw_version);
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(uart, buf, len);
	platform_stdio_tx_unlock();
}

static int cmd_version(int argc, char* argv[])
{
	_version(STDIO_UART);
	return ERR_CMD_OK;
}

static int dev_cmd_version(int argc, char* argv[])
{
	_version(DEV_UART);
	return ERR_CMD_OK;
}

static wiced_result_t _switch_set(int uart, int argc, char* argv[])
{
	int i;
	if (argc < 6) {
		puts_err(uart);
		return ERR_INSUFFICENT_ARGS;
	}

	for (i = 0; i < MAX_PORT; i++) {
		on[i] = (argv[i+1][0] == '1') ? 1 : 0;
	}
	apply_switch(WICED_FALSE);
	puts_ok(uart);
	return ERR_CMD_OK;
}

static int cmd_switch_set(int argc, char* argv[])
{
	return _switch_set(STDIO_UART, argc, argv);
}

static int dev_switch_set(int argc, char* argv[])
{
	return _switch_set(DEV_UART, argc, argv);
}

static wiced_result_t _switch_mask(int uart, int argc, char* argv[])
{
	int i;
	if (argc < 6) {
		puts_err(uart);
		return ERR_INSUFFICENT_ARGS;
	}

	for (i = 0; i < MAX_PORT; i++) {
		on_mask[i] = (argv[i+1][0] == '1') ? 1 : 0;
	}
	apply_switch(WICED_FALSE);
	puts_ok(uart);
	return ERR_CMD_OK;
}

static int cmd_switch_mask(int argc, char* argv[])
{
	return _switch_mask(STDIO_UART, argc, argv);
}

static int cmd_read(int argc, char* argv[])
{
	printf("Voltage: %.1f %.1f %.1f %.1f\n", voltage[0], voltage[1], voltage[2], voltage[3]);
	printf("Current: %.2f %.2f %.2f %.2f\n", current[0], current[1], current[2], current[3]);
	printf("Voltage Raw: %d %d %d %d\n", adc_avg[0], adc_avg[1], adc_avg[2], adc_avg[3]);
	printf("Current Raw: %d %d %d %d\n", adc_avg[4], adc_avg[5], adc_avg[6], adc_avg[7]);
	printf("Temperature: %d\n", temperature);
	printf("Humidity: %d\n", humidity);
	printf("On: %d %d %d %d %d\n", on[0], on[1], on[2], on[3], on[4]);
	printf("On Mask: %d %d %d %d %d\n", on_mask[0], on_mask[1], on_mask[2], on_mask[3], on_mask[4]);
	return ERR_CMD_OK;
}

static int dev_read(int argc, char* argv[])
{
	char buf[64];
	sprintf(buf, "%.1f %.1f %.1f %.1f %.2f %.2f %.2f %.2f\r\n",
		voltage[0], voltage[1], voltage[2], voltage[3],
		current[0], current[1], current[2], current[3]);
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(DEV_UART, buf, strlen(buf));
	platform_stdio_tx_unlock();
	return ERR_CMD_OK;
}

static int dev_read_raw(int argc, char* argv[])
{
	char buf[64];
	sprintf(buf, "%d %d %d %d %d %d %d %d\r\n",
		adc_avg[0], adc_avg[1], adc_avg[2], adc_avg[3], adc_avg[4], adc_avg[5], adc_avg[6], adc_avg[7]);
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(DEV_UART, buf, strlen(buf));
	platform_stdio_tx_unlock();
	return ERR_CMD_OK;
}

static int dev_port(int argc, char* artv[])
{
	char buf[64];
	sprintf(buf, "%d %d %d %d %d %d %d %d %d %d\r\n",
		on_mask[0], on_mask[1], on_mask[2], on_mask[3], on_mask[4], on[0], on[1] ,on[2], on[3], on[4]);
	platform_stdio_tx_lock();
        wiced_uart_transmit_bytes(DEV_UART, buf, strlen(buf));
	platform_stdio_tx_unlock();
	return ERR_CMD_OK;
	
}

static void _reset_ap(int uart)
{
	on_mask[MAX_SWITCH-1] = 1;
	apply_switch(WICED_FALSE);
	wiced_rtos_delay_milliseconds(1000);
	on_mask[MAX_SWITCH-1] = 0;
	apply_switch(WICED_FALSE);
	puts_ok(uart);
}

static int cmd_reset_ap(int argc, char* argv[])
{
	_reset_ap(STDIO_UART);
	return ERR_CMD_OK;
}

static int dev_reset_ap(int argc, char* argv[])
{
	_reset_ap(DEV_UART);
	return ERR_CMD_OK;
}

static const command_t cons_commands[] = {
	{"hello", cmd_hello, 0, NULL, NULL, NULL, "Hello:"},
	{"version", cmd_version, 0, NULL, NULL, NULL, "Read Version"},
	{"read", cmd_read, 0, NULL, NULL, NULL, "Read All Value"} ,
	{"mask", cmd_switch_mask, 5, NULL, NULL, "each port", "Set make of switch"},
	{"set", cmd_switch_set, 5, NULL, NULL, "each port", "Set Power Control"},
	{"reset_ap", cmd_reset_ap, 0, NULL, NULL, NULL, "Reset AP"},
	{"reboot", cmd_reboot, 0, NULL, NULL, NULL, "Reboot"},
	CMD_TABLE_END
};

static const dev_command_t dev_cons_commands[] = {
	{"hello", dev_cmd_hello, 0},
	{"version", dev_cmd_version, 0},
	{"set", dev_switch_set, 5},
	{"reset_ap", dev_reset_ap, 0},
	{"read", dev_read, 0},
	{"read_raw", dev_read_raw, 0},
	{"port", dev_port, 0},
};

/*
 * Very simple console parser
 *  - not accept extra space and any control character excpet return
 */
static void run_command(void)
{
	int i;
	int argc_pos;
	char* argv[MAX_ARGC+1];

	memset(argv, 0, sizeof(argv));
	check(console_pos <= MAX_CONSOLE_LINE);
	console_buf[console_pos] = '\0';

	argc_pos = 0;
	argv[argc_pos++] = console_buf;
	for (i = 0; i < console_pos; i++) {
		if (console_buf[i] == ' ') {
			console_buf[i] = '\0';
			argv[argc_pos++] = &console_buf[i+1];
			if (argc_pos >= MAX_ARGC)
				break;
		}
	}

	for (i = 0; i < N_ELEMENT(dev_cons_commands); i++) {
		if (strcmp(argv[0], dev_cons_commands[i].name) == 0) {
			require_string(argc_pos >= dev_cons_commands[i].arg_count + 1, bad_param,
				       "Too short parameter for device command");
			dev_cons_commands[i].command(argc_pos, argv);
			break;
		}
	}
	require_string(i < N_ELEMENT(dev_cons_commands), bad_param, "Unknown device command");
	console_pos = 0;
	return;
		
bad_param:
	puts_err(DEV_UART);
	console_pos = 0;
	return;
}

static void console_process_char(char c)
{
	if ((c > 31) && (c < 127)) {
		if (console_pos < MAX_CONSOLE_LINE) {
			console_buf[console_pos++] = c;
		} else {
			wiced_log_msg(WLF_DEF, WICED_LOG_WARNING, "Exceed console buffer length\n");
		}
	} else if (c == '\r') {
		if (console_pos > 0)
			run_command();
		console_pos = 0;
	}
}

static void dev_console_thread_func(wiced_thread_arg_t arg)
{
	wiced_result_t result;
	uint8_t received_character;
	uint32_t expected_transfer_size;
	
	while (1) {
		expected_transfer_size = 1;
		result = wiced_uart_receive_bytes(DEV_UART, &received_character, &expected_transfer_size, 1000);
		if (result == WICED_SUCCESS) {
			console_process_char(received_character);
		}
        }
}

wiced_result_t usb_console_init(void)
{
	wiced_result_t res = WICED_SUCCESS;
	/* maintenance console */
	res = command_console_init(STDIO_UART, sizeof(command_buffer), command_buffer,
				   COMMAND_HISTORY_LENGTH, command_history_buffer, " ");
	require_noerr(res, exit);
	res = console_add_cmd_table(cons_commands);
	require_noerr(res, exit);

	/* console for main logger board */
	res = ring_buffer_init(&rx_buffer, rx_data, UART_RX_BUFFER_SIZE);
	require_noerr(res, exit);
	res = wiced_uart_init(DEV_UART, &uart_app_config, &rx_buffer);
	require_noerr(res, exit);

	/* init thread */
	res = wiced_rtos_init_mutex(&mutex);
	require_noerr(res, exit);
	
	res = wiced_rtos_create_thread(&dev_console_thread, WICED_DEFAULT_LIBRARY_PRIORITY,
				       "dev_console", dev_console_thread_func, DEV_CONSOLE_THREAD_STACK_SIZE, NULL);
	require_noerr(res, exit);
	
exit:
	return res;
}

