/*
 * Copyright (C) 2018 HummingLab. - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the
 * written license agreement between Telesign and its licensee.
 */
#pragma once

wiced_result_t coap_daemon_init(void);

wiced_result_t coap_post_alive(wiced_bool_t reset);
wiced_result_t coap_post_int(char* endpoint, int data);
wiced_result_t coap_post_data(char* endpoint, void* data, size_t len);

