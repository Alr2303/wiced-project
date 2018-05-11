/*
 * Copyright (C) 2018 HummingLab, Inc. - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */
#pragma once

typedef void (*a_network_callback_t)(wiced_bool_t, void*);

wiced_result_t a_network_register_callback(a_network_callback_t callback, void *arg);
extern wiced_bool_t a_network_is_assoc(void);
extern wiced_bool_t a_network_is_up(void);
extern void a_network_start(void);
extern void a_network_stop(void);
extern void a_network_restart(void);
extern void a_network_init(void);
