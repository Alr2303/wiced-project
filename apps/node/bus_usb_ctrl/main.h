/*
 * Copyright (C) 2018 HummingLab. - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */
#pragma once

#define MAX_PORT	4
#define MAX_SWITCH	5

extern const char* fw_version;
extern const char* fw_model;

extern int adc_avg[WICED_ADC_MAX];
extern float voltage[MAX_PORT];
extern float current[MAX_PORT];
extern int on_mask[MAX_SWITCH];
extern int on[MAX_SWITCH];
extern int temperature;
extern int humidity;
extern wiced_bool_t low_battery;

void apply_switch(wiced_bool_t slow);
