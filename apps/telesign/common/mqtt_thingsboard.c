/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"

#include "eventloop.h"
#include "sys_mqtt.h"
#include "mqtt_thingsboard.h"

extern const char * a_fw_version(void);
extern const char * a_fw_model(void);

wiced_result_t a_sys_mqtt_tb_publish_version(sys_mqtt_t *s)
{
	char buf[128];

	sprintf(buf, "{\"version\":\"%s\",\"model\":\"%s\"}", a_fw_version(), a_fw_model());
	a_sys_mqtt_publish(s, TOPIC_ATTRIBUTES, buf, strlen(buf), 0, 0);
	return WICED_TRUE;
}

wiced_result_t a_sys_mqtt_tb_subscribe(sys_mqtt_t *s)
{
	if (a_sys_mqtt_app_subscribe(s, TOPIC_SUBSCRIBE_RPC, WICED_MQTT_QOS_DELIVER_AT_MOST_ONCE) != WICED_SUCCESS) {
		return WICED_ERROR;
	}
	return WICED_SUCCESS;
}

wiced_bool_t a_sys_mqtt_tb_is_rpc_topic(wiced_mqtt_topic_msg_t *msg)
{
	uint32_t cmp_size = strlen(TOPIC_SUBSCRIBE_RPC) - 1;
	if (msg && msg->topic && msg->topic_len > cmp_size && msg->data && msg->data_len > 0 &&
	    strncmp((char*)msg->topic, TOPIC_SUBSCRIBE_RPC, cmp_size) == 0)
		return WICED_TRUE;
	else
		return WICED_FALSE;
}

wiced_bool_t a_sys_mqtt_tb_is_attribute_topic(wiced_mqtt_topic_msg_t *msg)
{
	uint32_t cmp_size = strlen(TOPIC_ATTRIBUTES);
	if (msg && msg->topic && msg->topic_len > cmp_size && msg->data && msg->data_len > 0 &&
	    strncmp((char*)msg->topic, TOPIC_ATTRIBUTES, cmp_size) == 0)
		return WICED_TRUE;
	else
		return WICED_FALSE;
}
