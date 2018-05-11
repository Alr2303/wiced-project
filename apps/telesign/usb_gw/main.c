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
#include "sys_mqtt.h"
#include "sys_button.h"
#include "sys_worker.h"
#include "util.h"
#include "json_parser.h"
#include "device.h"


#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define VERSION 0.0.1

const char* fw_version = EXPAND_AND_QUOTE(VERSION);
const char* fw_model = "USB-GW";

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

#define VALID_USB_INPUT_VOLTAGE_ADC_LEVEL 1000

#define MAX_USB_PORT		4

#define EVENT_SENSOR_FINISHED	(1 << 0)
#define EVENT_USB1_DET		(1 << 1)
#define EVENT_USB2_DET		(1 << 2)
#define EVENT_USB3_DET		(1 << 3)
#define EVENT_USB4_DET		(1 << 4)

#define SENSING_INTERVAL	(10 * 1000)

#define ADC_P1_V          WICED_ADC_1
#define ADC_P2_V          WICED_ADC_2
#define ADC_BAT           WICED_ADC_3

typedef enum {
	LED_SERVER_G,
	LED_SERVER_R,
	LED_WIFI_G,
	LED_WIFI_R,
	LED_SENSOR_G,
	LED_SENSOR_R,
	LED_BAT_R,
	LED_FAULT_R,
} led_index_t;

static const int gpio_table[] =
{
	[LED_SERVER_G] = USBGW_LED_CPU_G,
	[LED_SERVER_R] = USBGW_LED_CPU_Y,
	[LED_WIFI_G] = USBGW_LED_WIFI_G,
	[LED_WIFI_R] = USBGW_LED_WIFI_Y,
	[LED_SENSOR_G] = USBGW_LED_SENSOR_G,
	[LED_SENSOR_R] = USBGW_LED_SENSOR_Y,
	[LED_BAT_R] = USBGW_LED_BAT_Y,
	[LED_FAULT_R] = USBGW_LED_FAULT_R,
};

static const int usb_det[] = {
	USBGW_BUTTON_1,
	USBGW_BUTTON_2,
	USBGW_BUTTON_3,
	USBGW_BUTTON_4,
};

static const uint8_t led_all_off[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t led_g_on[] =    { 1, 0, 1, 0, 1, 0, 0, 0 };
static const uint8_t led_r_on[] =    { 0, 1, 0, 1, 0, 1, 1, 1 };

static eventloop_t evt;
static sys_led_t led;
static sys_button_t button;
static sys_mqtt_t mqtt;
static sys_worker_t worker;

static char server[MAX_SERVER_NAME];
static char device_token[MAX_DEVICE_TOKEN];

static eventloop_timer_node_t timer_node;

static int temp;
static int humid;
static int charging_bits;
static int fault_bits;

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

static void mqtt_subscribe_cb_fn(sys_mqtt_t *s, wiced_mqtt_topic_msg_t *msg, void *arg)
{
#if 0
	a_json_t json;
	int entry[10];
	const char* val;
	if (!a_sys_mqtt_is_rpc_topic(msg))
		return;
#endif
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
	sprintf(buf, "{\"activity\":1,\"temperature\":%d,\"humidity\":%d,\"fail\":%d,"
		"\"usbUse1\":%d,\"usbUse2\":%d,\"usbUse3\":%d,\"usbUse4\":%d,"
		"\"usbFail1\":%d,\"usbFail2\":%d,\"usbFail3\":%d,\"usbFail4\":%d}",
		temp, humid, !!fault_bits,
		!!(charging_bits & 0x01), !!(charging_bits & 0x02),
		!!(charging_bits & 0x04), !!(charging_bits & 0x08),
		!!(fault_bits & 0x01), !!(fault_bits & 0x02),
		!!(fault_bits & 0x04), !!(fault_bits & 0x08));
	a_sys_mqtt_publish(&mqtt, TOPIC_TELEMETRY, buf, strlen(buf), 0, 0);
}

static void sensor_process(void *arg)
{
	float f_temp, f_humid;
	a_dev_temp_humid_get(&f_temp, &f_humid);

	temp = (int)(f_temp + 0.5);
	humid = (int)(f_humid + 0.5);
}

static void usb_detect_fn(void *arg, wiced_bool_t on)
{
	int i;
	int new_charging_bits = 0;
	int new_fault_bits = 0;
	static uint32_t count = 0;

	for (i = 0; i < MAX_USB_PORT; i++) {
		if (!wiced_gpio_input_get(usb_det[i])) {
			new_charging_bits |= (1 << i);
		}
	}

        for (i = 0; i < 2; i++) {
		int mask = 0x3 << (i*2);
		uint16_t val = a_dev_adc_get(ADC_P1_V + i);
		if (count % 30 == 0)
			wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "USB Power %d ADC: 0x%x\n", i, val);

		if (val < VALID_USB_INPUT_VOLTAGE_ADC_LEVEL) {
			new_charging_bits &= ~mask;
			new_fault_bits |= mask;
		}
	}

	if (charging_bits != new_charging_bits || fault_bits != new_fault_bits) {
		charging_bits = new_charging_bits;
		fault_bits = new_fault_bits;
		a_sys_worker_trigger(&worker);

		if (fault_bits) {
			a_sys_led_set(&led, LED_SENSOR_G, LED_OFF);
			a_sys_led_set(&led, LED_SENSOR_R, LED_BLINK);
		} else {
			a_sys_led_set(&led, LED_SENSOR_G, LED_ON);
			a_sys_led_set(&led, LED_SENSOR_R, LED_OFF);
		}			
	}
}

void application_start(void)
{
	wiced_result_t result;
	WICED_LOG_LEVEL_T level = WICED_LOG_DEBUG0;

	wiced_init();

	wiced_log_init(level, log_output_handler, NULL);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "USB-GW Version: v%s\n", fw_version);

	result = command_console_init(STDIO_UART, sizeof(command_buffer), command_buffer,
				      COMMAND_HISTORY_LENGTH, command_history_buffer, " ");
	wiced_assert("Console Init Error", result == WICED_SUCCESS);
	console_add_cmd_table(cons_commands);

	a_dev_temp_humid_init();
	a_dev_adc_init();

	a_app_net_init();
	a_eventloop_init(&evt);

	a_sys_led_init(&led, &evt, 500, gpio_table, N_ELEMENT(gpio_table));
	a_sys_button_init(&button, PLATFORM_BUTTON_1, &evt, EVENT_USB1_DET, usb_detect_fn, (void*)0);
	a_sys_button_init(&button, PLATFORM_BUTTON_2, &evt, EVENT_USB2_DET, usb_detect_fn, (void*)1);
	a_sys_button_init(&button, PLATFORM_BUTTON_3, &evt, EVENT_USB3_DET, usb_detect_fn, (void*)2);
	a_sys_button_init(&button, PLATFORM_BUTTON_4, &evt, EVENT_USB4_DET, usb_detect_fn, (void*)3);
	a_sys_worker_init(&worker, &evt, EVENT_SENSOR_FINISHED, SENSING_INTERVAL,
			  sensor_process, send_telemetry_sensor, 0);
	a_eventloop_register_timer(&evt, &timer_node, initial_led_blink_cb, 500, 0);

	printf("Start USB Gateway\n");
	
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
