/*
 * Copyright (C) 2018 HummingLab - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */
#pragma once

typedef void (*sys_worker_fn)(void *arg);

typedef struct {
	eventloop_t *evt;

	uint32_t event_flag;
	int interval_ms;
	sys_worker_fn worker_fn;
	sys_worker_fn finish_fn;
	void *arg;

	eventloop_timer_node_t timer_node;
	eventloop_event_node_t event_node;
	wiced_worker_thread_t worker_thread;
} sys_worker_t;

wiced_result_t a_sys_worker_trigger(sys_worker_t *s);
wiced_result_t a_sys_worker_init(sys_worker_t *s, eventloop_t *e, uint32_t event_flag, int interval_ms,
				 sys_worker_fn worker_fn, sys_worker_fn finish_fn, void *arg);
