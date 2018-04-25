/*
 * Copyright (C) 2018 HummingLab. - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the
 * written license agreement between Telesign and its licensee.
 */
#pragma once

typedef struct {
	int fail;		/* fail code */
	wiced_bool_t usb;	/* charging or not */
	int voltage;		/* voltage level */
	int load;		/* load current */
	wiced_bool_t allow_fast_charge;	/* allow fast charge */
} charger_state_t;

typedef struct {
	char hostname[128];
	char path[256];
	char md5[16 * 2];
	int port;
} update_info_t;

const char * a_fw_version(void);
const char * a_fw_model(void);
charger_state_t * a_get_charger_state(void);
void a_set_allow_fast_charge(wiced_bool_t enable);
wiced_result_t a_upgrade_request(char* json_data, size_t len);
wiced_result_t a_asynchronous_event(event_handler_t function, void* arg);
