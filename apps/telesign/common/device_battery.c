/*
 * Copyright (C) 2018 HummingLab - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */

#include "wiced.h"
#include "device.h"
#include "device_battery.h"
#include <math.h>

#define BAT_DISP_MIN	3030.
#define BAT_DISP_MAX	3710.

#define AVG_BETA	0.9

static double bat_avg = 0.;
static uint bat_cnt = 0;

void  a_dev_battery_update(void)
{
	uint16_t bat;
	bat = a_dev_battery_get();

	if (bat_cnt < 0xffff)
		bat_cnt++;

	bat_avg = AVG_BETA * bat_avg + (1. - AVG_BETA) * (double)bat;
}

uint16_t a_dev_battery_get_filter(void)
{
	double avg;
	double bias_correction;

	bias_correction = 1. - pow(AVG_BETA, (double)bat_cnt);
	avg = bat_avg /bias_correction;

	return (uint16_t)avg;
}

int a_dev_battery_get_percent(void)
{
	uint16_t avg = a_dev_battery_get_filter();
	if (avg  <= BAT_DISP_MIN) {
		return 0;
	} else if (avg >= BAT_DISP_MAX) {
		return 100;
	} else {
		return (int)((avg - BAT_DISP_MIN) / (BAT_DISP_MAX - BAT_DISP_MIN) * 100.);
	}
}
