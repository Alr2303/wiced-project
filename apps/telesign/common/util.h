/*
 * Copyright (C) 2018 HummingLab - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */
#pragma once

#define N_ELEMENT(tbl)	(sizeof(tbl)/sizeof((tbl)[0]))
#define N_OFFSET(type, member)  ((uint32_t)(&(((type*)0)->member)))
#define N_SIZE(type, member)   ((sizeof)(((type*)0)->member))

const char* a_get_wlan_sec_name(wiced_security_t security);
wiced_result_t a_set_wlan(char *ssid, char *security, char *key);
void a_stack_check(void);

const char * a_fw_version(void);
