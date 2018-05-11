/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *    Ian Craggs - convert to FreeRTOS
 *    YS Lee - convert to WICED
 *******************************************************************************/

#include "wiced.h"
#include "wiced_tls.h"

#include "../MQTTClient.h"

#define TCP_CONNECTION_TIMEOUT          ( 20000 )


int ThreadStart(wiced_thread_t* thread, void (*fn)(void*), void* arg)
{
	int rc;
	rc = wiced_rtos_create_thread(thread, /* thread handle */
				      WICED_DEFAULT_LIBRARY_PRIORITY,
				      "MQTTTask", /* thread name */
				      (wiced_thread_function_t)fn,	  /* thread function */
				      WICED_DEFAULT_APPLICATION_STACK_SIZE,	  /* stack size */
				      arg);
	return (rc == WICED_SUCCESS) ? MQTT_SUCCESS : MQTT_FAILURE;
}


void MutexInit(wiced_mutex_t* mutex)
{
	wiced_rtos_init_mutex(mutex);
}

int MutexLock(wiced_mutex_t* mutex)
{
	return (wiced_rtos_lock_mutex(mutex) == WICED_SUCCESS) ?
		MQTT_SUCCESS : MQTT_FAILURE;
}

int MutexUnlock(wiced_mutex_t* mutex)
{
	return (wiced_rtos_unlock_mutex(mutex) == WICED_SUCCESS) ? 
		MQTT_SUCCESS : MQTT_FAILURE;
}


void TimerCountdownMS(wiced_time_t* timer, unsigned int timeout_ms)
{
	wiced_time_t t;
	wiced_time_get_time(&t);
	*timer = t + timeout_ms;
}


void TimerCountdown(wiced_time_t* timer, unsigned int timeout) 
{
	TimerCountdownMS(timer, timeout * 1000);
}


int TimerLeftMS(wiced_time_t* timer) 
{
	int diff;
	wiced_time_t cur;
	wiced_time_get_time(&cur);
	diff = (int)(*timer - cur);
	return (diff < 0) ? 0 : diff;
}


int TimerIsExpired(wiced_time_t* timer)
{
	return !TimerLeftMS(timer);
}


void TimerInit(wiced_time_t* timer)
{
	*timer = 0;
}

#if 0
static void dump_bytes(const uint8_t* bptr, uint32_t len)
{
    uint32_t i = 0;

    for (i = 0; i < len; )
    {
        if ((i & 0x0f) == 0)
        {
            WPRINT_NETWORK_DEBUG( ( "\n" ) );
        }
        else if ((i & 0x07) == 0)
        {
            WPRINT_NETWORK_DEBUG( (" ") );
        }
        WPRINT_NETWORK_DEBUG( ( "%02x ", bptr[i++] ) );
    }
    WPRINT_NETWORK_DEBUG( ( "\n" ) );
}
#endif

extern int MQTT_read(Network* n, unsigned char* buffer, int len, int timeout_ms);
int MQTT_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	wiced_result_t result;

	if (!n->connected) {
		wiced_rtos_delay_milliseconds((uint32_t)timeout_ms);
		return MQTT_FAILURE;
	}

	result = wiced_tcp_stream_read(&n->stream, buffer, (uint16_t)len, (uint32_t)timeout_ms);
#if 0
#ifdef WPRINT_NETWORK_DEBUG
	if (result == 0) {
		WPRINT_NETWORK_DEBUG(("Recv(%d): %d B", result, len));
		dump_bytes(buffer, len);
	}
#endif
#endif
	if (result == WICED_SUCCESS)
		return len;
	else if (result == WICED_TIMEOUT || result == WICED_PENDING)
		return 0;
	else
		return MQTT_FAILURE;
}

extern int MQTT_write(Network* n, unsigned char* buffer, int len, int timeout_ms);
int MQTT_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	/* FIXEM: WICED tcp send api has not timeout */
	wiced_result_t result;

	if (!n->connected) {
		wiced_rtos_delay_milliseconds((uint32_t)timeout_ms);
		return MQTT_FAILURE;
	}

	result = wiced_tcp_send_buffer(&n->socket, buffer, (uint16_t)len);

#if 0
#ifdef WPRINT_NETWORK_DEBUG
	WPRINT_NETWORK_DEBUG(("Send(%d): %d B", result, len));
	if (result == 0) 
		dump_bytes(buffer, len);
#endif
#endif
	return (result == WICED_SUCCESS) ? len : MQTT_FAILURE;
}

extern void MQTT_disconnect(Network* n);
void MQTT_disconnect(Network* n)
{
	if (n->connected) {
		wiced_tcp_stream_deinit(&n->stream);
		wiced_tcp_disconnect(&n->socket);
		wiced_tcp_delete_socket(&n->socket);
		if (n->use_tls) {
			wiced_tls_deinit_context(&n->context);
		}
		n->connected = WICED_FALSE;
	}
}


void NetworkInit(Network* n)
{
	memset(n, 0, sizeof(Network));
	n->mqttread = MQTT_read;
	n->mqttwrite = MQTT_write;
	n->disconnect = MQTT_disconnect;
}

int NetworkConnectTLS(Network* n, const char* addr, int port, const char* peer_cn)
{
	int count = 0;
	wiced_result_t result;
	wiced_ip_address_t ip;

	while ((result = wiced_hostname_lookup(addr, &ip, 10000)) != WICED_SUCCESS) {
		if (++count >= 2) {
			WPRINT_NETWORK_ERROR(("Fail to MQTT addr lookup: %s\n", addr));
			return MQTT_FAILURE;
		}
	}
	WPRINT_NETWORK_DEBUG(("MQTT lookup: IPv%d, %d.%d.%d.%d\n", ip.version,
			      (int)(ip.ip.v4>>24)&0x0ff, (int)(ip.ip.v4>>16)&0x0ff, 
			      (int)(ip.ip.v4>>8)&0x0ff, (int)(ip.ip.v4>>0)&0x0ff));

	result = wiced_tcp_create_socket(&n->socket, WICED_STA_INTERFACE);
	if (result != WICED_SUCCESS) {
		WPRINT_NETWORK_ERROR(("Fail to create socket\n"));
		return MQTT_FAILURE;
	}

	if (peer_cn) {
		n->use_tls = WICED_TRUE;
		wiced_tls_init_context(&n->context, NULL, peer_cn);
		wiced_tcp_enable_tls(&n->socket, &n->context);
	} else {
		n->use_tls = WICED_FALSE;
	}
	result = wiced_tcp_connect(&n->socket, &ip, (uint16_t)port, TCP_CONNECTION_TIMEOUT);
	if (result != WICED_SUCCESS) {
		WPRINT_NETWORK_ERROR(("Fail to make tcp channel\n"));
		wiced_tcp_delete_socket(&n->socket);
		return MQTT_FAILURE;
	}

	n->connected = WICED_TRUE;
	result = wiced_tcp_stream_init(&n->stream, &n->socket);
	if (result != WICED_SUCCESS) {
		WPRINT_NETWORK_ERROR(("Fail to make tcp stream\n"));
		wiced_tcp_delete_socket(&n->socket);
		return MQTT_FAILURE;
	}

	WPRINT_NETWORK_DEBUG(("Connected MQTT TCP Channel\n"));
	return 0;
}

int NetworkConnect(Network* n, const char* addr, int port)
{
	return NetworkConnectTLS(n, addr, port, NULL);
}
