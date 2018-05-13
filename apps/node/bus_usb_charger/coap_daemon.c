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
#include "server/coap_server.h"
#include "client/coap_client.h"
#include "gedday.h"

#include "bus_usb_charger_main.h"
#include "assert_macros.h"
#include "network.h"
#include "coap_daemon.h"

#ifdef DTLS_ENABLE
#define COAP_TARGET_PORT ( 5684 )
#else
#define COAP_TARGET_PORT ( 5683 )
#endif
#define WICED_COAP_TIMEOUT       (10000)

#define MDNS_SERVICE_COAP	"_coap._udp.local"
#define TEXT_PLAIN		WICED_COAP_CONTENTTYPE_TEXT_PLAIN

static wiced_semaphore_t semaphore;
static wiced_coap_client_event_type_t expected_event;

static wiced_ip_address_t host_ip;
static wiced_coap_client_t coap_client;
static wiced_coap_server_t coap_server;
static wiced_coap_server_service_t service[7];

static wiced_result_t handle_version(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_fail(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_usb(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_voltage(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_current(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_fastcharge(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_update(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_interval(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);

wiced_result_t coap_post_alive(wiced_bool_t reset)
{
	int len;
	char buf[128];
	charger_state_t *state = a_get_charger_state();
	len = sprintf(buf, "%s%s", state->id, reset ? ",reset" : "");
	return coap_post_data("alive", buf, len);
}

wiced_result_t coap_post_int(char* endpoint, int data)
{
	char buf[20];
	sprintf(buf, "%s:%d", endpoint, data);
	return coap_post_data(NULL, buf, 0);
}

wiced_result_t coap_post_str(char* endpoint, char* data)
{
	char buf[128];
	sprintf(buf, "%s:%s", endpoint, data);
	return coap_post_data(NULL, buf, 0);
}

static wiced_result_t coap_wait_for(wiced_coap_client_event_type_t event, uint32_t timeout)
{
	if (wiced_rtos_get_semaphore(&semaphore, timeout) != WICED_SUCCESS) {
		return WICED_ERROR;
	} else {
		if (event != expected_event)
			return WICED_ERROR;
	}
	return WICED_SUCCESS;
}

wiced_result_t coap_post_data(char* path, char* data, size_t len)
{
	wiced_coap_client_request_t request;

	memset(&request, 0, sizeof(request));
	request.payload_type = WICED_COAP_CONTENTTYPE_TEXT_PLAIN;
	request.payload.data = (uint8_t*)data;
	request.payload.len = len ? len : strlen(data);

	wiced_coap_set_uri_path(&request.options, path ? path : "server");

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Sent Post: %s - %s\n", path ? path : "server" , data);
	require_noerr(wiced_coap_client_post(&coap_client, &request, WICED_COAP_MSGTYPE_CON, host_ip, COAP_TARGET_PORT), _error);
	require_noerr(coap_wait_for(WICED_COAP_CLIENT_EVENT_TYPE_POSTED, WICED_COAP_TIMEOUT), _error);
	/* wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Ack OK\n"); */
	return WICED_SUCCESS;

_error:
	return WICED_ERROR;
}

static wiced_result_t coap_client_callback(wiced_coap_client_event_info_t event_info)
{
	switch(event_info.type) {
        case WICED_COAP_CLIENT_EVENT_TYPE_POSTED:
		/* wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Posted\n"); */
		expected_event = event_info.type;
		wiced_rtos_set_semaphore(&semaphore);
		break;
        case WICED_COAP_CLIENT_EVENT_TYPE_GET_RECEIVED:
		/* wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Data Receved\n"); */
		expected_event = event_info.type;
		wiced_rtos_set_semaphore(&semaphore);
		break;
        case WICED_COAP_CLIENT_EVENT_TYPE_OBSERVED:
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Observed\n");
		expected_event = event_info.type;
		wiced_rtos_set_semaphore(&semaphore);
		break;
        case WICED_COAP_CLIENT_EVENT_TYPE_NOTIFICATION:
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Notification\n");
		break;
	case WICED_COAP_CLIENT_EVENT_TYPE_DELETED:
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Deleted\n");
		break;
        default:
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Wrong Event\n");
		break;
	}
	return WICED_SUCCESS;
}

static wiced_result_t coap_init(void *arg)
{
	int i;
	wiced_result_t res;
	gedday_service_t service_result;
	charger_state_t *state;

	wiced_rtos_init_semaphore(&semaphore);

#if 0
	res = gedday_init(WICED_STA_INTERFACE, "CoAP Discovery");
	require_noerr(res, _error);

	res = gedday_discover_service(MDNS_SERVICE_COAP, &service_result);
	require_action(res == WICED_SUCCESS, _error, gedday_deinit());

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Sever: %s\n", service_result.hostname);

	if (service_result.ip[0].version == WICED_IPV4)
		host_ip = service_result.ip[0];
	else
		host_ip = service_result.ip[1];

	/* must be swap it */
	host_ip.ip.v4 = ntohl(host_ip.ip.v4);
	gedday_deinit();
#else
	state = a_get_charger_state();
	wiced_hostname_lookup(state->server, &host_ip, 10000, WICED_STA_INTERFACE);
#endif
	require(host_ip.version == WICED_IPV4, _error);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "IP: " IPV4_PRINT_FORMAT "\n",
		      IPV4_SPLIT_TO_PRINT(host_ip));

	require_noerr(wiced_coap_client_init(&coap_client, WICED_STA_INTERFACE, coap_client_callback), _coap_err);
	require_noerr(wiced_coap_server_init(&coap_server), _coap_err);
	require_noerr(wiced_coap_server_start(&coap_server, WICED_STA_INTERFACE, COAP_TARGET_PORT, NULL), _coap_err);

	require_noerr(wiced_coap_server_add_service(&coap_server, &service[0], "version", handle_update, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[1], "fail", handle_fail, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[2], "usb", handle_usb, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[3], "voltage", handle_voltage, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[4], "current", handle_current, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[5], "fastcharge", handle_fastcharge, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[6], "interval", handle_interval, TEXT_PLAIN), _coap_err);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Sever Ready\n");
	
	for (i = 0; i < 3; i++) {
		res = coap_post_alive(WICED_TRUE);
		if (res == WICED_SUCCESS) {
			break;
		}
		check(res == WICED_SUCCESS);
	}
	return WICED_SUCCESS;

_error:
	wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to discover CoAP server\n");
	memset(&host_ip, 0, sizeof(host_ip));
	wiced_rtos_deinit_semaphore(&semaphore);
_coap_err:
	wiced_framework_reboot();
	return WICED_ERROR;
}

static void _net_event(wiced_bool_t up, void *arg)
{
	if (!up || host_ip.version)
		return;

	a_asynchronous_event(coap_init, 0);
}

wiced_result_t coap_daemon_init(void)
{
	a_network_register_callback(_net_event, 0);
	if (a_network_is_up())
		_net_event(WICED_TRUE, 0);
	return WICED_SUCCESS;
}

#define HANDLE_RESPONSE(data) handle_response(context, service, request, data)
static wiced_result_t handle_response(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request,
				      const char* data)
{
	wiced_coap_server_response_t response;
	wiced_coap_notification_type type = WICED_COAP_NOTIFICATION_TYPE_NONE;
	memset(&response, 0, sizeof(response));
	response.payload.data = (uint8_t*)data;
	response.payload.len = strlen(data);
	return wiced_coap_server_send_response(context, service, request->req_handle, &response, type);
}


static wiced_result_t handle_version(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	return HANDLE_RESPONSE(a_fw_version());
}

static wiced_result_t handle_fail(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	static char buf[8];
	charger_state_t *state = a_get_charger_state();
	sprintf(buf, "%d", (int)state->fail);
	return HANDLE_RESPONSE(buf);
}

static wiced_result_t handle_usb(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	charger_state_t *state = a_get_charger_state();
	return HANDLE_RESPONSE(state->usb ? "1" : "0");
}

static wiced_result_t handle_voltage(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	static char buf[8];
	charger_state_t *state = a_get_charger_state();
	sprintf(buf, "%d", state->voltage);
	return HANDLE_RESPONSE(buf);
}

static wiced_result_t handle_current(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	static char buf[8];
	charger_state_t *state = a_get_charger_state();
	sprintf(buf, "%d", state->current);
	return HANDLE_RESPONSE(buf);
}

static wiced_result_t handle_fastcharge(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	charger_state_t *state = a_get_charger_state();
	if (request->method == WICED_COAP_METHOD_PUT) {
		int en = (request->payload.data && request->payload.data[0] == '1') ? 1 : 0;
		wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Change Fast Charge Enable : %d\n", en);
		a_set_allow_fast_charge(en);
	}
	return HANDLE_RESPONSE(state->allow_fast_charge ? "1" : "0");
}

static wiced_result_t handle_update(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	if (request->method == WICED_COAP_METHOD_PUT) {
		a_upgrade_request((char*)request->payload.data, request->payload.len);
		return HANDLE_RESPONSE("OK");
	} else {
		return handle_version(context, service, request);
	}
}

static wiced_result_t handle_interval(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	static char buf[12];
	charger_state_t *state = a_get_charger_state();
	if (request->method == WICED_COAP_METHOD_PUT) {
		int val = atol((char*)request->payload.data);
		a_set_update_interval(val);
	}
	sprintf(buf, "%d", state->interval);
	return HANDLE_RESPONSE(buf);
}
