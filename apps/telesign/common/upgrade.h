/*
 * Copyright (C) 2018 HummingLab - All Rights Reserved.
 *
 * This software is confidential and proprietary to Telesign.
 * No Part of this software may be reproduced, stored, transmitted, disclosed
 * or used in any form or by any means other than as expressly provide by the 
 * written license agreement between Telesign and its licensee.
 */
#pragma once

typedef enum {
	OTA_SUCCESS, OTA_FAILURE,
	OTA_NO_UPGRADE, OTA_NO_UPGRADE_204, OTA_NO_UPGRADE_500, OTA_NO_UPGRADE_4XX,
	OTA_FAIL_TO_LOOKUP_HOSTNAME,
	OTA_FAIL_TO_CREATE_SOCKET,
	OTA_FAIL_TO_CONNECT_TO_SERVER,
	OTA_FAIL_TO_CREATE_TCP_STREAM,
	OTA_FAIL_TO_SEND_HTTP_REQUEST,
	OTA_FAIL_BY_BAD_HTTP_RESPONSE,
	OTA_FAIL_BY_CONTENT_LENGTH_OVER_MAXSIZE,
	OTA_FAIL_TO_FIND_EMPTY_LINE,
	OTA_FAIL_BY_BINARY_OVER_FLASHSIZE,
	OTA_FAIL_TO_RECV_BINARY,
	OTA_FAIL_MD5_VALIDATION,
	OTA_FAIL_MD5_VALIDATION_WRITING,
} ota_result_t;

ota_result_t a_upgrade_try(wiced_bool_t no_reboot);
ota_result_t a_upgrade_from_shell(const char *host, uint16_t port);

