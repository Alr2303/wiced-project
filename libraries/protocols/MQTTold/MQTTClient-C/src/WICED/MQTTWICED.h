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
 *    YS Lee - convert to WICED
 *******************************************************************************/

#if !defined(MQTTWICED_H)
#define MQTTWICED_H

#include "wiced.h"

typedef wiced_time_t Timer;

typedef struct Network Network;

struct Network
{
	wiced_bool_t connected;
	wiced_tcp_socket_t socket;
	wiced_tcp_stream_t stream;
	wiced_bool_t use_tls;
	wiced_tls_context_t context;

	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};

void TimerInit(Timer*);
int TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

typedef wiced_mutex_t Mutex;

void MutexInit(Mutex*);
int MutexLock(Mutex*);
int MutexUnlock(Mutex*);

typedef wiced_thread_t Thread;

int ThreadStart(Thread*, void (*fn)(void*), void* arg);

void NetworkInit(Network*);
int NetworkConnect(Network*, const char*, int);
int NetworkConnectTLS(Network* n, const char* addr, int port, const char* peer_cn);

#endif
