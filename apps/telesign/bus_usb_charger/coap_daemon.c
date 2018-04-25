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
#define WICED_COAP_TARGET_PORT ( 5684 )
#else
#define WICED_COAP_TARGET_PORT ( 5683 )
#endif

#define MDNS_SERVICE_COAP	"_coap._udp.local"
#define TEXT_PLAIN		WICED_COAP_CONTENTTYPE_TEXT_PLAIN
static wiced_ip_address_t host_ip;
static wiced_coap_client_t coap_client;
static wiced_coap_server_t coap_server;
static wiced_coap_server_service_t service[7];

static wiced_result_t handle_version(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_fail(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_usb(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_voltage(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_load(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);
static wiced_result_t handle_cutoff(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request);

wiced_result_t coap_client_callback(wiced_coap_client_event_info_t event_info)
{
	return WICED_SUCCESS;
}

static void _net_event(wiced_bool_t up, void *arg)
{
	wiced_result_t res;
	gedday_service_t service_result;

	if (!up || host_ip.version)
		return;

	res = gedday_init(WICED_STA_INTERFACE, "CoAP Discovery");
	require_noerr(res, _error);

	res = gedday_discover_service(MDNS_SERVICE_COAP, &service_result);
	require_action(res == WICED_SUCCESS, _error, gedday_deinit());

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Sever: %s\n", service_result.hostname);

	if (service_result.ip[0].version == WICED_IPV4)
		host_ip = service_result.ip[0];
	else
		host_ip = service_result.ip[1];
	gedday_deinit();

	/* must be swap it */
	host_ip.ip.v4 = ntohl(host_ip.ip.v4);
	require(host_ip.version == WICED_IPV4, _error);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "IP: " IPV4_PRINT_FORMAT "\n",
		      IPV4_SPLIT_TO_PRINT(host_ip));

	require_noerr(wiced_coap_client_init(&coap_client, WICED_STA_INTERFACE, coap_client_callback), _coap_err);
	require_noerr(wiced_coap_server_init(&coap_server), _coap_err);
	require_noerr(wiced_coap_server_start(&coap_server, WICED_STA_INTERFACE, WICED_COAP_TARGET_PORT, NULL), _coap_err);

	require_noerr(wiced_coap_server_add_service(&coap_server, &service[0], "version", handle_version, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[1], "fail", handle_fail, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[2], "usb", handle_usb, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[3], "voltage", handle_voltage, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[4], "load", handle_load, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[5], "cutoff", handle_cutoff, TEXT_PLAIN), _coap_err);
	require_noerr(wiced_coap_server_add_service(&coap_server, &service[6], "update", handle_cutoff, TEXT_PLAIN), _coap_err);

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "CoAP Sever Ready\n");
_coap_err:
	return;

_error:
	wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to discover CoAP server\n");
	memset(&host_ip, 0, sizeof(host_ip));
	return;
}

wiced_result_t coap_daemon_init(void)
{
	a_network_register_callback(_net_event, 0);
	if (a_network_is_up())
		_net_event(WICED_TRUE, 0);
	return WICED_SUCCESS;
}

static wiced_result_t handle_version(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	wiced_coap_server_response_t response;
	wiced_coap_notification_type type = WICED_COAP_NOTIFICATION_TYPE_NONE;
	memset(&response, 0, sizeof(response));
	require(request->method == WICED_COAP_METHOD_GET, _bad);

	response.payload.data = (uint8_t*)a_fw_version();
	response.payload.len = strlen(a_fw_version());
_bad:
	wiced_coap_server_send_response(context, service, request->req_handle, &response, type);
	return WICED_SUCCESS;
}

static wiced_result_t handle_fail(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	wiced_coap_server_response_t response;
	wiced_coap_notification_type type = WICED_COAP_NOTIFICATION_TYPE_NONE;
	memset(&response, 0, sizeof(response));
	require(request->method == WICED_COAP_METHOD_GET, _bad);

	response.payload.data = (uint8_t*)a_fw_version();
	response.payload.len = strlen(a_fw_version());
_bad:
	wiced_coap_server_send_response(context, service, request->req_handle, &response, type);
	return WICED_SUCCESS;
}

static wiced_result_t handle_usb(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	wiced_coap_server_response_t response;
	wiced_coap_notification_type type = WICED_COAP_NOTIFICATION_TYPE_NONE;
	memset(&response, 0, sizeof(response));
	require(request->method == WICED_COAP_METHOD_GET, _bad);

	response.payload.data = (uint8_t*)a_fw_version();
	response.payload.len = strlen(a_fw_version());
_bad:
	wiced_coap_server_send_response(context, service, request->req_handle, &response, type);
	return WICED_SUCCESS;
}

static wiced_result_t handle_voltage(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	wiced_coap_server_response_t response;
	wiced_coap_notification_type type = WICED_COAP_NOTIFICATION_TYPE_NONE;
	memset(&response, 0, sizeof(response));
	require(request->method == WICED_COAP_METHOD_GET, _bad);

	response.payload.data = (uint8_t*)a_fw_version();
	response.payload.len = strlen(a_fw_version());
_bad:
	wiced_coap_server_send_response(context, service, request->req_handle, &response, type);
	return WICED_SUCCESS;
}

static wiced_result_t handle_load(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	wiced_coap_server_response_t response;
	wiced_coap_notification_type type = WICED_COAP_NOTIFICATION_TYPE_NONE;
	memset(&response, 0, sizeof(response));
	require(request->method == WICED_COAP_METHOD_GET, _bad);

	response.payload.data = (uint8_t*)a_fw_version();
	response.payload.len = strlen(a_fw_version());
_bad:
	wiced_coap_server_send_response(context, service, request->req_handle, &response, type);
	return WICED_SUCCESS;
}

static wiced_result_t handle_cutoff(void* context, wiced_coap_server_service_t* service, wiced_coap_server_request_t* request)
{
	wiced_coap_server_response_t response;
	wiced_coap_notification_type type = WICED_COAP_NOTIFICATION_TYPE_NONE;
	memset(&response, 0, sizeof(response));
	require(request->method == WICED_COAP_METHOD_GET, _bad);

	response.payload.data = (uint8_t*)a_fw_version();
	response.payload.len = strlen(a_fw_version());
_bad:
	wiced_coap_server_send_response(context, service, request->req_handle, &response, type);
	return WICED_SUCCESS;
}

