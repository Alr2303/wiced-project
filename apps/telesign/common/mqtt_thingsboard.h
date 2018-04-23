/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

#define TOPIC_TELEMETRY "v1/devices/me/telemetry"
#define TOPIC_ATTRIBUTES "v1/devices/me/attributes"
#define TOPIC_SUBSCRIBE_RPC "v1/devices/me/rpc/request/+"

wiced_result_t a_sys_mqtt_tb_publish_version(sys_mqtt_t *s);
wiced_result_t a_sys_mqtt_tb_subscribe(sys_mqtt_t *s);
wiced_bool_t a_sys_mqtt_tb_is_rpc_topic(wiced_mqtt_topic_msg_t *msg);
wiced_bool_t a_sys_mqtt_tb_is_attribute_topic(wiced_mqtt_topic_msg_t *msg);
