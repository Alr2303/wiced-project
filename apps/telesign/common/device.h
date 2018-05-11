/*
 * Copyright (C) 2018 HummingLab - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */
#pragma once

wiced_result_t a_dev_temp_humid_get(float *temp, float *humid);

void a_dev_set_usb_tester(int index);

uint16_t a_dev_adc_get(int adc);
wiced_result_t a_dev_adc_init(void);

wiced_result_t a_dev_temp_humid_init(void);
