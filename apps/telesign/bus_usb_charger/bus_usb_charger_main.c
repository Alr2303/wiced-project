/*
 * Copyright (C) 2018 HummingLab. - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the
 * written license agreement between Telesign and its licensee.
 */
#include <string.h>
#include <sys/unistd.h>

#include "wiced.h"
#include "wiced_log.h"
#include "command_console.h"
#include "dct/command_console_dct.h"
#include "wifi/command_console_wifi.h"
#include "platform/command_console_platform.h"

#include "network.h"
#include "app_dct.h"
#include "console.h"

#include "eventloop.h"
#include "sys_led.h"
#include "sys_pwm.h"
#include "sys_button.h"
#include "sys_worker.h"
#include "util.h"
#include "json_parser.h"
#include "device.h"

#include "bus_usb_charger_main.h"
#include "bus_usb_console.h"
#include "coap_daemon.h"

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define VERSION 0.0.1

const char* fw_version = EXPAND_AND_QUOTE(VERSION);
const char* fw_model = "USB-CHARGER";

#define COMMAND_HISTORY_LENGTH  (5)
#define MAX_COMMAND_LENGTH      (180)
static char command_buffer[MAX_COMMAND_LENGTH];
static char command_history_buffer[MAX_COMMAND_LENGTH * COMMAND_HISTORY_LENGTH];
static const command_t cons_commands[] = {
        /* WIFI_COMMANDS */
        /* PLATFORM_COMMANDS */
        /* DCT_CONSOLE_COMMANDS */
	BUS_USB_COMMANDS
        BUS_SHELTER_COMMANDS
        CMD_TABLE_END
};

#define EVENT_USB_DET		(1 << 0)

#define PWM_LED_MAX		100

typedef enum {
	LED_POWER,
} led_index_t;

static const int gpio_table[] = {
        [LED_POWER] = USBCG_LED_POWER,
};

static const uint8_t led_all_off[] = { 1 }; /* low active */
static const uint8_t led_all_on[] = { 0 };

static eventloop_t evt;
static sys_led_t led;
static sys_button_t button;
static sys_pwm_t pwm_green;
static sys_pwm_t pwm_red;

static eventloop_timer_node_t timer_node;

void net_init(void) {
	app_dct_t* dct;
        wiced_bool_t inited = WICED_FALSE;

	if (inited)
		return;

	inited = WICED_TRUE;

	wiced_dct_read_lock((void**) &dct, WICED_FALSE, DCT_APP_SECTION,
			    0, sizeof(app_dct_t));
	wiced_dct_read_unlock(dct, WICED_FALSE);

	a_network_init();
}

static int log_output_handler(WICED_LOG_LEVEL_T level, char *log_msg)
{
	write(STDOUT_FILENO, log_msg, strlen(log_msg));
	return 0;
}

static void usb_detect_fn(void *arg, wiced_bool_t on)
{
	int val = !wiced_gpio_input_get(GPIO_BUTTON_USB_DETECT); /* low active */

	if (val) {
		a_sys_pwm_level(&pwm_green, 0);
		a_sys_pwm_level(&pwm_red, PWM_LED_MAX);
	} else {
		a_sys_pwm_level(&pwm_green, PWM_LED_MAX);
		a_sys_pwm_level(&pwm_red, 0);
	}
}

static void led_all(const uint8_t *tbl)
{
	int i;
	for (i = 0; i < N_ELEMENT(gpio_table); i++)
		a_sys_led_set(&led, i, tbl[i]);
}

static void initial_led_blink_cb(void *arg)
{
	static int cnt;
	if (++cnt > 6) {
		a_eventloop_deregister_timer(&evt, &timer_node);
		led_all(led_all_on);
		coap_daemon_init();
		return;
	}
	led_all((cnt % 2) ? led_all_off: led_all_on);
}

void application_start(void) {
	wiced_result_t result;
	WICED_LOG_LEVEL_T level = WICED_LOG_DEBUG0;

	wiced_init();

	wiced_log_init(level, log_output_handler, NULL);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "USB Charger Version: v%s\n", fw_version);


	result = command_console_init(STDIO_UART, sizeof(command_buffer), command_buffer,
				      COMMAND_HISTORY_LENGTH, command_history_buffer, " ");
	wiced_assert("Console Init Error", result == WICED_SUCCESS);
	console_add_cmd_table(cons_commands);

	a_dev_adc_init();

	net_init();
	a_eventloop_init(&evt);
	a_sys_led_init(&led, &evt, 500, gpio_table, N_ELEMENT(gpio_table));
	a_sys_button_init(&button, PLATFORM_BUTTON_1, &evt, EVENT_USB_DET, usb_detect_fn, (void*)0);
	a_sys_pwm_init(&pwm_green, &evt, WICED_PWM_1, 10, 100);
	a_sys_pwm_init(&pwm_red, &evt, WICED_PWM_2, 10, 100);

	a_sys_pwm_level(&pwm_green, PWM_LED_MAX);

	a_eventloop_register_timer(&evt, &timer_node, initial_led_blink_cb, 500, 0);

	printf("Start USB Charger\n");

	while (1) {
		a_eventloop(&evt, WICED_WAIT_FOREVER);
	}
}

const char * a_fw_version(void) {
	return fw_version;
}

const char * a_fw_model(void) {
	return fw_model;
}
