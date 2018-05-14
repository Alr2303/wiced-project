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
#include "ping/command_console_ping.h"

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
#include "upgrade.h"
#include "assert_macros.h"

#include "bus_usb_charger_main.h"
#include "bus_usb_console.h"
#include "coap_daemon.h"

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define VERSION 0.0.3

const char* fw_version = EXPAND_AND_QUOTE(VERSION);
const char* fw_model = "USB-CHARGER";

#define COMMAND_HISTORY_LENGTH  (5)
#define MAX_COMMAND_LENGTH      (180)
static char command_buffer[MAX_COMMAND_LENGTH];
static char command_history_buffer[MAX_COMMAND_LENGTH * COMMAND_HISTORY_LENGTH];
static const command_t cons_commands[] = {
        WIFI_COMMANDS
	/* PING_COMMANDS */
	BUS_USB_COMMANDS
        EVENTLOOP_COMMANDS
        CMD_TABLE_END
};

#define DEF_SENSING_INTERVAL	(2 * 1000)
#define CHARGING_TEST_INTERVAL	(3600*1000)

#define EVENT_USB_DET		(1 << 0)
#define EVENT_CHANGE_INTERVAL	(1 << 1)

#define PWM_LED_MAX		100

#define VALID_VOLTAGE_ADC	500
#define VALID_IDLE_CURRNET_ADC	50
#define VALID_TEST_CURRENT_ADC	390

#define ERR_LOW_VOLTAGE		(1 << 0)
#define ERR_USB_TEST		(1 << 1)

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
static wiced_worker_thread_t worker_thread;
static eventloop_event_node_t event_update_interval;
static eventloop_timer_node_t timer_sensor_node;
static eventloop_timer_node_t timer_usb_detect_node;

static charger_state_t state;

static void update_interval(void *arg);

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

static wiced_result_t usb_post(void *arg)
{
	if (a_network_is_up()) {
		require_noerr(coap_post_int("usb", state.usb), _err);
	}
_err:
	return WICED_SUCCESS;
}

static void sensor_process(void *arg);
static void usb_detect_delayed(void *arg)
{
	int val = !wiced_gpio_input_get(GPIO_BUTTON_USB_DETECT); /* low active */

	a_eventloop_deregister_timer(&evt, &timer_usb_detect_node);
	if (state.test_process)
		return;

	if (state.usb == val)
		return;

	state.usb = val;
	if (val) {
		a_sys_pwm_level(&pwm_green, 0);
		a_sys_pwm_level(&pwm_red, PWM_LED_MAX);
	} else {
		a_sys_pwm_level(&pwm_green, PWM_LED_MAX);
		a_sys_pwm_level(&pwm_red, 0);
	}

	if (a_network_is_up()) {
		wiced_rtos_send_asynchronous_event(&worker_thread, usb_post, 0);
		sensor_process(0);
	}
}
static void usb_detect_fn(void *arg, wiced_bool_t on)
{
	const uint32_t delay = 100;
	a_eventloop_register_timer(&evt, &timer_usb_detect_node, usb_detect_delayed, delay, 0);
}

static wiced_result_t sensor_post(void *arg)
{
	if (a_network_is_up()) {
		require_noerr(coap_post_alive(false), _err);
		require_noerr(coap_post_int("voltage", state.voltage), _err);
		require_noerr(coap_post_int("current", state.current), _err);
		require_noerr(coap_post_int("fail", state.fail), _err);
		usb_post(0);
	}
_err:
	return WICED_SUCCESS;
}

static void charging_test(void)
{
	uint16_t current;

	state.test_process = WICED_TRUE;

	wiced_gpio_output_high(GPO_LOAD_TEST);
	wiced_rtos_delay_milliseconds(5);
	current = a_dev_adc_get(WICED_ADC_2);
	wiced_gpio_output_low(GPO_LOAD_TEST);
	wiced_rtos_delay_milliseconds(1);

	state.test_process = WICED_FALSE;

	if (current < VALID_TEST_CURRENT_ADC) {
		state.fail |= ERR_USB_TEST;
	} else {
		state.fail &= ~ERR_USB_TEST;
	}

	/* recheck usb charging */
	usb_detect_fn(0, 0);
}

static void sensor_process(void *arg)
{
	uint16_t voltage, current;

	voltage = a_dev_adc_get(WICED_ADC_1);
	current = a_dev_adc_get(WICED_ADC_2);

	state.voltage = (int)voltage;
	state.current = (int)current;
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "ADC Voltage: %d, Current: %d\n", state.voltage, state.current);

	if (state.voltage < VALID_VOLTAGE_ADC) {
		state.fail |= ERR_LOW_VOLTAGE;
	} else {
		state.fail &= ~ERR_LOW_VOLTAGE;
	}

	if (state.current < VALID_IDLE_CURRNET_ADC) {
		state.test_timeout += DEF_SENSING_INTERVAL;
		if (state.test_timeout >= CHARGING_TEST_INTERVAL) {
			state.test_timeout = 0;
			charging_test();
		}
	} else {
		state.test_timeout = 0;
	}

	if (a_network_is_up()) {
		wiced_rtos_send_asynchronous_event(&worker_thread, sensor_post, 0);
	}
}

static void init_state_info(void) {
	int len;
	app_dct_t* dct;

	memset(&state, 0, sizeof(state));
	wiced_dct_read_lock((void**) &dct, WICED_FALSE, DCT_APP_SECTION,
			    0, sizeof(app_dct_t));
	strncpy(state.server, dct->server, sizeof(state.server));
	strncpy(state.id, dct->device_id, sizeof(state.id));
	wiced_dct_read_unlock(dct, WICED_FALSE);
	len = strlen(state.id);
	if (len <= 0 || len >= sizeof(state.id) - 1) { /* empty or full */
		platform_dct_wifi_config_t* dct_wifi_config;
		wiced_dct_read_lock((void**)&dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION,
				    0, sizeof(platform_dct_wifi_config_t));
		sprintf(state.id, "%02x%02x%02x%02x%02x%02x",
			dct_wifi_config->mac_address.octet[0],
			dct_wifi_config->mac_address.octet[1],
			dct_wifi_config->mac_address.octet[2],
			dct_wifi_config->mac_address.octet[3],
			dct_wifi_config->mac_address.octet[4],
			dct_wifi_config->mac_address.octet[5]);
		wiced_dct_read_unlock((void*)dct_wifi_config, WICED_TRUE);

		wiced_dct_read_lock((void**) &dct, WICED_TRUE, DCT_APP_SECTION,
			    0, sizeof(app_dct_t));
		memset(dct->device_id, 0, sizeof(dct->device_id));
		strcpy(dct->device_id, state.id);
		wiced_dct_write((const void*)dct, DCT_APP_SECTION, 0, sizeof(app_dct_t));
		wiced_dct_read_unlock(dct, WICED_TRUE);
	}
}

void application_start(void) {
	uint32_t ms;
	wiced_result_t result;
	WICED_LOG_LEVEL_T level = WICED_LOG_DEBUG0;

	wiced_init();
	a_init_srand();

	init_state_info();
	a_set_allow_fast_charge(true);

	wiced_log_init(level, log_output_handler, NULL);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "USB Charger Version: v%s\n", fw_version);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Device ID: %s\n", state.id);

	result = command_console_init(STDIO_UART, sizeof(command_buffer), command_buffer,
				      COMMAND_HISTORY_LENGTH, command_history_buffer, " ");
	wiced_assert("Console Init Error", result == WICED_SUCCESS);
	console_add_cmd_table(cons_commands);

	a_dev_adc_init();
	a_eventloop_init(&evt);
	a_sys_led_init(&led, &evt, 500, gpio_table, N_ELEMENT(gpio_table));
	a_sys_pwm_init(&pwm_green, &evt, WICED_PWM_1, 10, 100);
	a_sys_pwm_init(&pwm_red, &evt, WICED_PWM_2, 10, 100);

	usb_detect_fn(0, 0);
	sensor_process(0);
	charging_test();
	a_sys_button_init(&button, PLATFORM_BUTTON_1, &evt, EVENT_USB_DET, usb_detect_fn, (void*)0);

	wiced_rtos_create_worker_thread(&worker_thread, WICED_DEFAULT_WORKER_PRIORITY, 4096, 2);
	a_eventloop_register_event(&evt, &event_update_interval, update_interval, EVENT_CHANGE_INTERVAL, 0);
	a_eventloop_register_timer(&evt, &timer_sensor_node, sensor_process, DEF_SENSING_INTERVAL, 0);
		
	ms = a_random_time_window(2000);
	printf("Start USB Charger after %dms\n", (int)ms);
	wiced_rtos_delay_milliseconds(ms);
	net_init();
	coap_daemon_init();

	while (1) {
		a_eventloop(&evt, WICED_WAIT_FOREVER);
	}
}

const char * a_fw_version(void)
{
	return fw_version;
}

const char * a_fw_model(void)
{
	return fw_model;
}

charger_state_t * a_get_charger_state(void)
{
	return &state;
}

void a_set_allow_fast_charge(wiced_bool_t enable)
{
	if (state.allow_fast_charge == enable)
		return;

	state.allow_fast_charge = enable;
	if (enable) {
		wiced_gpio_output_high(GPO_ENABLE_FAST_CHARGE);
	} else {
		wiced_gpio_output_low(GPO_ENABLE_FAST_CHARGE);
	}

	/* reset charger device */
	wiced_gpio_output_high(GPO_CHARGE_CONTROL);
	wiced_rtos_delay_milliseconds(100);
	wiced_gpio_output_low(GPO_CHARGE_CONTROL);
}

static void update_interval(void *arg)
{
	a_eventloop_deregister_timer(&evt, &timer_sensor_node);
	a_eventloop_register_timer(&evt, &timer_sensor_node, sensor_process, state.interval, 0);
}

void a_set_update_interval(int ms)
{
	if (ms < 1000)
		ms = 1000;
	else if (ms > 60 * 1000)
		ms = 60 * 1000;

	if (state.interval != ms) {
		state.interval = ms;
		a_eventloop_set_flag(&evt, EVENT_CHANGE_INTERVAL);
	}
}

static wiced_result_t upgrade_worker(void *arg)
{
	ota_result_t res;
	update_info_t* info = arg;

	if (!a_network_is_up())
		return WICED_SUCCESS;

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Start Upgrade\n");
	res = a_upgrade_try(WICED_FALSE, info->hostname, info->port, info->path, info->md5, WICED_TRUE);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Upgrade Result: %d\n", res);

	free(info);
	return WICED_SUCCESS;
}

wiced_result_t a_upgrade_request(char* json_data, size_t len)
{
	a_json_t json;
	const char *p;
	int entry[14];
	update_info_t* info = NULL;
	char* buf = NULL;

	info = malloc(sizeof(update_info_t));
	buf = malloc(len + 1);
	memset(info, 0, sizeof(update_info_t));

	a_json_init(&json, (char*)buf, len + 1, entry, N_ELEMENT(entry), '\0');
	a_json_append_str_sized(&json, (char*)json_data, len);
	
	p = a_json_get_prop(&json, "hostname");
	require(p, _bad);
	strcpy(info->hostname, p);
	p = a_json_get_prop(&json, "path");
	require(p, _bad);
	strcpy(info->path, p);
	p = a_json_get_prop(&json, "md5");
	require(p, _bad);
	strcpy(info->md5, p);
	info->port = a_json_get_prop_int(&json, "port", 0, 65535);
	require(info->port, _bad);
	free(buf);

	require(a_network_is_up(), _bad);
	wiced_rtos_send_asynchronous_event(&worker_thread, upgrade_worker, info);

	return WICED_SUCCESS;
_bad:
	if (buf)
		free(buf);

	if (info)
		free(info);
	return WICED_ERROR;
}

wiced_result_t a_asynchronous_event(event_handler_t function, void* arg)
{
	return wiced_rtos_send_asynchronous_event(&worker_thread, function, arg);
}
