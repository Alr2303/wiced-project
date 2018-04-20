/*
 * Copyright (C) 2018 HummingLab. - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */
#include <sys/unistd.h>

#include "wiced.h"
#include "wiced_log.h"
#include "command_console.h"
#include "dct/command_console_dct.h"
#include "wifi/command_console_wifi.h"
#include "platform/command_console_platform.h"

/* #include "device.h" */
#include "network.h"
#include "app_dct.h"
#include "console.h"

#include "eventloop.h"
#include "sys_led.h"
#include "sys_pwm.h"
#include "sys_button.h"
#include "sys_mqtt.h"
#include "sys_worker.h"
#include "util.h"
#include "json_parser.h"
#include "device.h"
#include "upgrade.h"

#define MAX_FAULT_PORT		4

#define EVENT_SENSOR_FINISHED		(1 << 0)
#define EVENT_FAULT1_DET		(1 << 1)
#define EVENT_FAULT2_DET		(1 << 2)
#define EVENT_FAULT3_DET		(1 << 3)
#define EVENT_FAULT4_DET		(1 << 4)

#define SENSING_INTERVAL		(10 * 1000)

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define VERSION 0.0.1

const char* fw_version = EXPAND_AND_QUOTE(VERSION);
const char* fw_model = "LED-GW";

#define COMMAND_HISTORY_LENGTH  (5)
#define MAX_COMMAND_LENGTH      (180)
static char command_buffer[MAX_COMMAND_LENGTH];
static char command_history_buffer[MAX_COMMAND_LENGTH * COMMAND_HISTORY_LENGTH];
static const command_t cons_commands[] =
{
    /* WIFI_COMMANDS */
    /* PLATFORM_COMMANDS */
    /* DCT_CONSOLE_COMMANDS */
	BUS_SHELTER_COMMANDS
	CMD_TABLE_END
};

typedef enum {
	LED_SERVER_G,
	LED_SERVER_R,
	LED_WIFI_G,
	LED_WIFI_R,
	LED_P1_G,
	LED_P1_R,
	LED_P2_G,
	LED_P2_R,
	LED_P3_G,
	LED_P3_R,
	LED_P4_G,
	LED_P4_R
} led_index_t;

static const int gpio_table[] =
{
	[LED_SERVER_G] = LEDGW_SERVER_G,
	[LED_SERVER_R] = LEDGW_SERVER_R,
	[LED_WIFI_G] = LEDGW_WIFI_G,
	[LED_WIFI_R] = LEDGW_WIFI_R,
	[LED_P1_G] = LEDGW_P1_G,
	[LED_P1_R] = LEDGW_P1_R,
	[LED_P2_G] = LEDGW_P2_G,
	[LED_P2_R] = LEDGW_P2_R,
	[LED_P3_G] = LEDGW_P3_G,
	[LED_P3_R] = LEDGW_P3_R,
	[LED_P4_G] = LEDGW_P4_G,
	[LED_P4_R] = LEDGW_P4_R,
};

static const int led_fault_det[] = {
	GPIO_BUTTON_1,
	GPIO_BUTTON_2,
	GPIO_BUTTON_3,
	GPIO_BUTTON_4,
};

static const uint8_t led_all_off[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t led_g_on[] =    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
static const uint8_t led_r_on[] =    { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 };

static eventloop_t evt;
static sys_led_t led;
static sys_pwm_t pwm;
static sys_button_t button;
static sys_mqtt_t mqtt;
static sys_worker_t worker;
static wiced_worker_thread_t worker_thread;

static char server[MAX_SERVER_NAME];
static char device_token[MAX_DEVICE_TOKEN];

static wiced_bool_t state_led_emergency;
static int state_led_on_level;
static wiced_bool_t blink;
static wiced_bool_t led_fail[4];

static int temp;
static int humid;

static eventloop_timer_node_t timer_node;

void a_app_net_init(void) {
	app_dct_t* dct;
        wiced_bool_t inited = WICED_FALSE;

	if (inited)
		return;

	inited = WICED_TRUE;

	wiced_dct_read_lock((void**) &dct, WICED_FALSE, DCT_APP_SECTION,
			     0, sizeof(app_dct_t));
	wiced_dct_read_unlock(dct, WICED_FALSE);

	a_network_init();
	wiced_rtos_delay_milliseconds(1 * 1000);
}

static int log_output_handler(WICED_LOG_LEVEL_T level, char *logmsg)
{
	UNUSED_PARAMETER(level);
	write(STDOUT_FILENO, logmsg, strlen(logmsg));
	return 0;
}

static void indicate_led(led_mode_t mode)
{
	int i;
	for (i = 0; i < MAX_FAULT_PORT; i++) {
		if (led_fail[i]) {
			a_sys_led_set(&led, LED_P1_G + i*2, LED_OFF);
			a_sys_led_set(&led, LED_P1_R + i*2, LED_BLINK);
		} else {
			a_sys_led_set(&led, LED_P1_G + i*2, mode);
			a_sys_led_set(&led, LED_P1_R + i*2, LED_OFF);
		}
	}
}

static void emergency_blink_cb(void *arg)
{
	blink = !blink;
	a_sys_pwm_level(&pwm, blink ? 100 : 50);
}

static void update_led(void)
{
	if (state_led_emergency) {
		a_eventloop_register_timer(&evt, &timer_node, emergency_blink_cb, 500, 0);
		blink = WICED_TRUE;
		a_sys_pwm_level(&pwm, 100);
		indicate_led(LED_BLINK);
	} else {
		a_eventloop_deregister_timer(&evt, &timer_node);
		a_sys_pwm_level(&pwm, state_led_on_level);
		indicate_led(state_led_on_level ? LED_ON : LED_OFF);
	}

	a_sys_worker_trigger(&worker);
}

static void mqtt_subscribe_cb_fn(sys_mqtt_t *s, wiced_mqtt_topic_msg_t *msg, void *arg)
{
	a_json_t json;
	int entry[14];
	const char* val;

	if (!a_sys_mqtt_is_rpc_topic(msg))
		return;

	a_json_init(&json, (char*)msg->data, msg->data_len, entry, N_ELEMENT(entry), '\0');
	a_json_append_str_sized(&json, (char*)msg->data, msg->data_len);
	if (!a_json_is_good(&json))
		return;

	val = a_json_get_prop(&json, "method");
	if (!val)
		return;
	
	if (strcmp(val, "emergency") == 0) {
		int v = a_json_get_prop_int(&json, "params", 0, 1) ? WICED_TRUE : WICED_FALSE;
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Emergency: %s\n", v ? "ON" : "OFF");
		if (v != state_led_emergency) {
			state_led_emergency = v;
			update_led();
		}
	} else if (strcmp(val, "led") == 0) {
		int v = a_json_get_prop_int(&json, "params", 0, 100);
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "LED: %d\n", v);
		if (v != state_led_on_level) {
			state_led_on_level = v;
			update_led();
		}
	} else if (strcmp(val, "firmware") == 0) {
		const char* hostname;
		const char* path;
		const char* md5;
		int port;
		wiced_bool_t https;

		hostname = a_json_get_prop(&json, "hostname");
		path = a_json_get_prop(&json, "path");
		md5 = a_json_get_prop(&json, "md5");
		port = a_json_get_prop_int(&json, "port", 0, 65535);
		https = a_json_get_prop_int(&json, "https", 0, 1);

		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Upgrade: %s:%s:%d/%s\n",
			      https ? "https" : "http",
			      hostname, port, path);

		a_upgrade_try(https, hostname, port, path, md5, WICED_TRUE);
	}
}

static void update_net_state_fn(wiced_bool_t net, wiced_bool_t mqtt, void *arg)
{
	if (net) {
		a_sys_led_set(&led, LED_WIFI_G, LED_BLINK);
		a_sys_led_set(&led, LED_WIFI_R, LED_OFF);
	} else {
		a_sys_led_set(&led, LED_WIFI_G, LED_OFF);
		a_sys_led_set(&led, LED_WIFI_R, LED_BLINK);
	}		
	if (mqtt) {
		a_sys_led_set(&led, LED_SERVER_G, LED_BLINK);
		a_sys_led_set(&led, LED_SERVER_R, LED_OFF);
	} else {
		a_sys_led_set(&led, LED_SERVER_G, LED_OFF);
		a_sys_led_set(&led, LED_SERVER_R, LED_BLINK);
	}
}

static wiced_result_t init_mqtt()
{
	app_dct_t* dct;

	update_net_state_fn(WICED_FALSE, WICED_FALSE, NULL);

	wiced_dct_read_lock((void**) &dct, WICED_FALSE, DCT_APP_SECTION, 0, sizeof(app_dct_t));
	strncpy(server, dct->server, sizeof(server));
	strncpy(device_token, dct->device_token, sizeof(device_token));
	server[sizeof(server) - 1] = '\0';
	device_token[sizeof(device_token) - 1] = '\0';
	wiced_dct_read_unlock(dct, WICED_FALSE);
	a_sys_mqtt_init(&mqtt, &evt, server, WICED_FALSE, device_token,
			"*.humminglab.io", mqtt_subscribe_cb_fn, update_net_state_fn, &mqtt);
	return WICED_SUCCESS;
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
		led_all(led_all_off);
		init_mqtt();
		return;
	}
	led_all((cnt % 2) ? led_g_on: led_r_on);
}

static void send_telemetry_sensor(void *arg)
{
	char buf[256];
	sprintf(buf, "{\"activity\":1,\"temperature\":%d,\"humidity\":%d,"
		"\"ledFail1\":%d,\"ledFail2\":%d,\"ledFail3\":%d,\"ledFail4\":%d, \"fail\":0,"
		"\"ledLevel\":%d,\"emergency\":%d}",
		temp, humid,
		led_fail[0], led_fail[1], led_fail[2], led_fail[3],
		state_led_on_level, state_led_emergency);
	a_sys_mqtt_publish(&mqtt, TOPIC_TELEMETRY, buf, strlen(buf), 0, 0);
}

static void sensor_process(void *arg)
{
	float f_temp, f_humid;
	a_dev_temp_humid_get(&f_temp, &f_humid);

	temp = (int)(f_temp + 0.5);
	humid = (int)(f_humid + 0.5);
}

static void fault_detect_fn(void *arg, wiced_bool_t on)
{
	int i;
	for (i = 0; i < MAX_FAULT_PORT; i++) {
		led_fail[i] = !!wiced_gpio_input_get(led_fault_det[i]);
	}
	update_led();
}

void application_start(void)
{
	wiced_result_t result;
	WICED_LOG_LEVEL_T level = WICED_LOG_DEBUG0;

	wiced_init();

	wiced_log_init(level, log_output_handler, NULL);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "LED-GW Version: v%s\n", fw_version);

	result = command_console_init(STDIO_UART, sizeof(command_buffer), command_buffer,
				      COMMAND_HISTORY_LENGTH, command_history_buffer, " ");
	wiced_assert("Console Init Error", result == WICED_SUCCESS);
	console_add_cmd_table(cons_commands);

	a_dev_temp_humid_init();

	a_app_net_init();
	a_eventloop_init(&evt);
	a_sys_led_init(&led, &evt, 500, gpio_table, N_ELEMENT(gpio_table));
	a_sys_pwm_init(&pwm, &evt, WICED_PWM_1, 10, 50);

	a_sys_button_init(&button, PLATFORM_BUTTON_1, &evt, EVENT_FAULT1_DET, fault_detect_fn, (void*)0);
	a_sys_button_init(&button, PLATFORM_BUTTON_2, &evt, EVENT_FAULT2_DET, fault_detect_fn, (void*)1);
	a_sys_button_init(&button, PLATFORM_BUTTON_3, &evt, EVENT_FAULT3_DET, fault_detect_fn, (void*)2);
	a_sys_button_init(&button, PLATFORM_BUTTON_4, &evt, EVENT_FAULT4_DET, fault_detect_fn, (void*)3);

	wiced_rtos_create_worker_thread(&worker_thread, WICED_DEFAULT_WORKER_PRIORITY, 4096, 2);
	a_sys_worker_init(&worker, &worker_thread, &evt, EVENT_SENSOR_FINISHED, SENSING_INTERVAL,
			  sensor_process, send_telemetry_sensor, 0);
	a_eventloop_register_timer(&evt, &timer_node, initial_led_blink_cb, 500, 0);

	printf("Start LED Gateway\n");
	
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
