/*
 * Copyright (C) 2018 HummingLab - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the
 * written license agreement between Telesign and its licensee.
 */
#pragma once

#include "mqtt_api.h"

struct _sys_mqtt_t;
typedef void (*mqtt_subscribe_cb)(struct _sys_mqtt_t *s, wiced_mqtt_topic_msg_t *msg, void *arg);
typedef void (*mqtt_net_event_cb)(wiced_bool_t net, wiced_bool_t mqtt, void *arg);

#define A_EVENT_NET_CHANGE		(1 << 31)
#define A_EVENT_MQTT_CONNECT_REQ	(1 << 30)
#define A_EVENT_MQTT_PUBLISHED		(1 << 29)
#define A_EVENT_MQTT_SUBSCRIBED		(1 << 28)
#define A_EVENT_MQTT_DISCONNECTED	(1 << 27)

#define TOPIC_TELEMETRY "v1/devices/me/telemetry"
#define TOPIC_ATTRIBUTES "v1/devices/me/attributes"
#define TOPIC_SUBSCRIBE_RPC "v1/devices/me/rpc/request/+"

typedef struct _sys_mqtt_t {
	char mqtt_obj[WICED_MQTT_OBJECT_MEMORY_SIZE_REQUIREMENT];

	eventloop_t *evt;
	const char *hostname;
	wiced_bool_t use_tls;
	const char *peer_cn;
	const char *user_token;
	mqtt_subscribe_cb subscribe_cb;
	mqtt_net_event_cb net_event_cb;
	void *arg;


	eventloop_timer_node_t activity_timer_node;
	eventloop_timer_node_t retry_timer_node;

	eventloop_event_node_t net_event_node;
	eventloop_event_node_t mqtt_con_event_node;
	eventloop_event_node_t mqtt_pub_event_node;
	eventloop_event_node_t mqtt_sub_event_node;
	eventloop_event_node_t mqtt_discon_event_node;

	wiced_bool_t mqtt_connected;
	wiced_mutex_t mutex;

	wiced_mqtt_event_type_t mqtt_pending_event_bitmap;
	uint32_t mqtt_interruptable_event_bitmap;


} sys_mqtt_t;

wiced_result_t a_sys_mqtt_init(sys_mqtt_t* s, eventloop_t *e, const char* hostname, wiced_bool_t use_tls,
			       const char *user_token, const char *peer_cn, mqtt_subscribe_cb subscribe_cb,
			       mqtt_net_event_cb net_event_cb, void *arg);
wiced_result_t a_sys_mqtt_publish(sys_mqtt_t *s, char *topic, char* data, uint32_t data_len, int qos,
				  wiced_bool_t retain);

wiced_bool_t a_sys_mqtt_is_rpc_topic(wiced_mqtt_topic_msg_t *msg);
wiced_bool_t a_sys_mqtt_is_attribute_topic(wiced_mqtt_topic_msg_t *msg);
