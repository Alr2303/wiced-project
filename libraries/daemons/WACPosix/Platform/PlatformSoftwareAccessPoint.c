/*
 * Platform Soft AP
 *
 * $Copyright (C) 2014 Broadcom Corporation All Rights Reserved.$
 *
 * $Id: PlatformSoftwareAccessPoint.c $
 */

#include "PlatformSoftwareAccessPoint.h"
#include "Debug.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define plat_softap_log(M, ...) custom_log("PlatformSoftAP", M, ##__VA_ARGS__)

#if( defined( _PLAT_SCRIPT_PATH ) )
#define PLAT_WL_SCRIPT_PATH _PLAT_SCRIPT_PATH
#else
#define PLAT_WL_SCRIPT_PATH "/usr/share/wac"
#endif
#define PLAT_WL_SCRIPT_NAME       "platform_wl_wrapper.sh"
#define PLAT_WL_SCRIPT_FUNC_START "plat_soft_ap_start"
#define PLAT_WL_SCRIPT_FUNC_STOP  "plat_soft_ap_stop"

#define PLAT_UDHCPD_CONF_FILE     "udhcpd.softap.conf"

#define PLAT_CMD_BUF_BASE_LENGTH (256)

static const uint8_t s_plat_apple_ie_id     =   0xDD;
static const uint8_t s_plat_apple_ie_oui[3] = { 0x00, 0xA0, 0x40 };

OSStatus PlatformSoftwareAccessPointStart(const uint8_t *inIE, size_t inIELen)
{
	OSStatus err = kNoErr;
	char *cmd_buf = NULL, *cmd_buf_index = NULL;
	size_t bytes_to_output = PLAT_CMD_BUF_BASE_LENGTH + inIELen;
	int rc = 0;
	size_t ie_length = 0;
	uint8_t *ie_payload = NULL;

	plat_softap_log("%s()+", __FUNCTION__);
	cmd_buf = malloc(bytes_to_output);
	if (!cmd_buf) {
		plat_softap_log("malloc() failed !");
		err = kNoMemoryErr;
		goto _softap_start_exit;
	}
	cmd_buf_index = cmd_buf;
	rc = snprintf(cmd_buf_index, bytes_to_output,
		      ". %s/%s && %s",
		      PLAT_WL_SCRIPT_PATH, PLAT_WL_SCRIPT_NAME, PLAT_WL_SCRIPT_FUNC_START);
	if ((rc < 0) || (rc >= (int)bytes_to_output)) {
		plat_softap_log("snprintf() failed !");
		err = kNoMemoryErr;
		goto _softap_start_exit;
	}
	plat_softap_log("cmd_buf_index=[%s]", cmd_buf_index);
	bytes_to_output -= rc;
	cmd_buf_index += rc;
	
	rc = snprintf(cmd_buf_index, bytes_to_output,
		      " %s/%s",
		      PLAT_WL_SCRIPT_PATH, PLAT_UDHCPD_CONF_FILE);
	if ((rc < 0) || (rc >= (int)bytes_to_output)) {
		plat_softap_log("snprintf() failed !");
		err = kNoMemoryErr;
		goto _softap_start_exit;
	}
	plat_softap_log("cmd_buf_index=[%s]", cmd_buf_index);
	bytes_to_output -= rc;
	cmd_buf_index += rc;

	if (inIE && inIELen) {
		size_t ie_index = 0;
		char *ie_payload_str = NULL;

		if ((inIE[0] == s_plat_apple_ie_id) && (inIELen > 5)) {
			if (inIE[1] == (inIELen - 2)) {
				ie_length = inIELen - 2;
				ie_payload = (uint8_t *)&(inIE[5]); 
			} else {
				plat_softap_log("Provided IE length doesn't match value in IE byte array !");
				plat_softap_log("Provided IE len = %d; IE length from IE array = %d",
						(int)inIELen, (inIELen > 1) ? (int)inIE[1] : (int)0);
				err = kParamErr;
				goto _softap_start_exit;
			}
		} else if ((inIE[0] == s_plat_apple_ie_oui[0]) && (inIELen > 3)) {
			ie_length = inIELen;
			ie_payload = (uint8_t *)&(inIE[3]);
		} else {
			plat_softap_log("Can't validate IE data array !");
			err = kParamErr;
			goto _softap_start_exit;
		}

		rc = snprintf(cmd_buf_index, bytes_to_output,
			      " %zu", ie_length);
		if ((rc < 0) || (rc >= (int)bytes_to_output)) {
			plat_softap_log("snprintf() failed !");
			err = kNoMemoryErr;
			goto _softap_start_exit;
		}
		plat_softap_log("cmd_buf_index=[%s]", cmd_buf_index);
		bytes_to_output -= rc;
		cmd_buf_index += rc;
		
		rc = snprintf(cmd_buf_index, bytes_to_output,
			      " %02hhx:%02hhx:%02hhx ",
			      s_plat_apple_ie_oui[0],
			      s_plat_apple_ie_oui[1],
			      s_plat_apple_ie_oui[2]);
		if ((rc < 0) || (rc >= (int)bytes_to_output)) {
			plat_softap_log("snprintf() failed !");
			err = kNoMemoryErr;
			goto _softap_start_exit;
		}
		plat_softap_log("cmd_buf_index=[%s]", cmd_buf_index);
		bytes_to_output -= rc;
		cmd_buf_index += rc;
		ie_payload_str = cmd_buf_index;
		for (ie_index = 0; ie_index < (ie_length - 3); ie_index++) {
			rc = snprintf(cmd_buf_index, bytes_to_output,
				      "%02hhx", ie_payload[ie_index]);
			if ((rc < 0) || (rc >= (int)bytes_to_output)) {
				plat_softap_log("snprintf() failed !");
				err = kNoMemoryErr;
				goto _softap_start_exit;
			}
			bytes_to_output -= rc;
			cmd_buf_index += rc;
		}
		plat_softap_log("cmd_buf_index=[%s]", ie_payload_str);
	}

	rc = system(cmd_buf);
	if (rc != 0) {
		plat_softap_log("%s:%s failed !", PLAT_WL_SCRIPT_NAME, PLAT_WL_SCRIPT_FUNC_START);
		err = kUnknownErr;
	}
 _softap_start_exit:
	if (cmd_buf) {
		free(cmd_buf);
	}
	plat_softap_log("%s()-", __FUNCTION__);
	return err;
}

OSStatus PlatformSoftwareAccessPointStop(void)
{
	OSStatus err = kNoErr;
	char *cmd_buf = NULL, *cmd_buf_index = NULL;
	size_t bytes_to_output = PLAT_CMD_BUF_BASE_LENGTH;
	int rc = 0;

	plat_softap_log("%s()+", __FUNCTION__);
	cmd_buf = malloc(bytes_to_output);
	if (!cmd_buf) {
		plat_softap_log("malloc() failed !");
		err = kNoMemoryErr;
		goto _softap_stop_exit;
	}
	cmd_buf_index = cmd_buf;
	rc = snprintf(cmd_buf_index, bytes_to_output,
		      ". %s/%s && %s",
		      PLAT_WL_SCRIPT_PATH, PLAT_WL_SCRIPT_NAME, PLAT_WL_SCRIPT_FUNC_STOP);
	if ((rc < 0) || (rc >= (int)bytes_to_output)) {
		plat_softap_log("snprintf() failed !");
		err = kNoMemoryErr;
		goto _softap_stop_exit;
	}
	plat_softap_log("cmd_buf_index=[%s]", cmd_buf_index);

	rc = system(cmd_buf);
	if (rc != 0) {
		plat_softap_log("%s:%s failed !", PLAT_WL_SCRIPT_NAME, PLAT_WL_SCRIPT_FUNC_STOP);
		err = kUnknownErr;
	}
 _softap_stop_exit:
	if (cmd_buf) {
		free(cmd_buf);
	}
	plat_softap_log("%s()-", __FUNCTION__);
	return err;
}
