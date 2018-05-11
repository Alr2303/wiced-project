/*
 * Platform Apply Configuration
 *
 * $Copyright (C) 2014 Broadcom Corporation All Rights Reserved.$
 *
 * $Id: PlatformApplyConfiguration.c $
 */

#include "PlatformApplyConfiguration.h"
#include "plat_helper.h"
#include "Debug.h"

#ifdef USE_BRCM_ROUTER_NVRAM
#define TYPEDEF_BOOL
#include "bcmnvram.h"
#endif

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define plat_apply_conf_log(M, ...) custom_log("PlatformApplyConf", M, ##__VA_ARGS__)

#ifdef _PLAT_SCRIPT_PATH
#define PLAT_WL_SCRIPT_PATH              _PLAT_SCRIPT_PATH
#else
#define PLAT_WL_SCRIPT_PATH              "/usr/share/wac"
#endif
#define PLAT_WL_SCRIPT_NAME              "platform_wl_wrapper.sh"
#define PLAT_WL_SCRIPT_FUNC              "plat_try_join_wifi"

#define PLAT_CMD_BUF_BASE_LENGTH (256)

#define PLAT_NVRAM_WAC_WIFI_SSID         "wac_wifi_ssid"
#define PLAT_NVRAM_WAC_WIFI_PSK          "wac_wifi_psk"
#define PLAT_NVRAM_WAC_AIRPLAY_PASSWORD  "wac_airp_pw"
#define PLAT_NVRAM_WAC_IS_CONFIGURED     "wac_is_configured"

#define PLAT_NVRAM_WAC_EXT               ".txt"

#define PLAT_NVRAM_WAC_FILE_READ_STORAGE (256)
#define PLAT_NVRAM_WAC_FILE_OPEN_MODE    (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH)

static const char empty_string[] = "";
static char wac_name_storage[HOST_NAME_MAX + 1];
static char file_read_storage[PLAT_NVRAM_WAC_FILE_READ_STORAGE];

OSStatus PlatformJoinDestinationWiFiNetwork(const char * const inSSID,
                                            const uint8_t * const inWiFiPSK,
                                            size_t inWiFiPSKLen)
{
	OSStatus err;
	plat_apply_conf_log("%s()+", __FUNCTION__);
	err = plat_helper_join_wifi(inSSID, inWiFiPSK, inWiFiPSKLen, true, false);
	plat_apply_conf_log("%s()-", __FUNCTION__);
	return err;
}

OSStatus PlatformApplyName(const char * const inName)
{
	OSStatus err = kParamErr;
	plat_apply_conf_log("%s()+ (with name=[%s])",
			    __FUNCTION__, inName ? inName : "NULL");
	if (inName && inName[0]) {
		int rc = plat_helper_save_name(inName);
		if (rc != 0) {
			plat_apply_conf_log("plat_helper_save_name() failed with %d !", rc);
		} else {
			err = kNoErr;
		}
	}
	plat_apply_conf_log("%s()-", __FUNCTION__);
	return err;
}


OSStatus PlatformApplyAirPlayPlayPassword(const char * const inPlayPassword)
{
	OSStatus err = kNoErr;
	char *play = NULL;
	int rc = -1;
	plat_apply_conf_log("%s()+ (with pw=[%s])",
			    __FUNCTION__,
			    inPlayPassword ? inPlayPassword : "NULL");
	if (!inPlayPassword) {
		play = (char *)empty_string;
	}
	else {
		play = (char *)inPlayPassword;
	}
	rc = plat_helper_save_play_password(play);
	if (rc != 0) {
		plat_apply_conf_log("plat_helper_save_play_password() failed !");
		err = kUnknownErr;
	}
	plat_apply_conf_log("%s()-", __FUNCTION__);
	return err;
}

static inline bool is_printable_ascii_string(const uint8_t * const buf, size_t buflen)
{
	bool rc = true;
	size_t i;

	for (i = 0; i < buflen; i++) {
		if ((buf[i] < 32) || (buf[i] > 126)) {
			rc = false;
			break;
		}
	}

	return rc;
}

OSStatus plat_helper_join_wifi(const char * const inSSID,
			       const uint8_t * const inWiFiPSK,
			       size_t inWiFiPSKLen,
			       bool b_store_to_nvram,
			       bool b_check_assoc)
{
	OSStatus err = kNoErr;
	char *cmd_buf = NULL, *cmd_buf_index = NULL;
	size_t bytes_to_output = PLAT_CMD_BUF_BASE_LENGTH + inWiFiPSKLen;
	int rc = 0;
	char *psk_str = NULL;

	plat_apply_conf_log("%s()+ (with ssid=[%s] and with %s PSK)",
			    __FUNCTION__,
			    inSSID ? inSSID : "NULL",
			    inWiFiPSKLen ? "" : "OUT");
	if (!inSSID || !inSSID[0]) {
		err = kParamErr;
		plat_apply_conf_log("Invalid SSID string !");
		goto _join_exit;
	}

	if (b_store_to_nvram) {
		rc = plat_helper_save_wac_ssid(inSSID);
		if (rc != 0) {
			plat_apply_conf_log("plat_helper_save_wac_ssid() failed !");
		}
	}

	cmd_buf = malloc(bytes_to_output);
	if (!cmd_buf) {
		plat_apply_conf_log("malloc() failed !");
		err = kNoMemoryErr;
		goto _join_exit;
	}
	cmd_buf_index = cmd_buf;
	rc = snprintf(cmd_buf_index, bytes_to_output,
		      ". %s/%s && %s",
		      PLAT_WL_SCRIPT_PATH, PLAT_WL_SCRIPT_NAME, PLAT_WL_SCRIPT_FUNC);
	if ((rc < 0) || (rc >= (int)bytes_to_output)) {
		plat_apply_conf_log("snprintf() failed !");
		err = kNoMemoryErr;
		goto _join_exit;
	}
	plat_apply_conf_log("cmd_buf_index=[%s]", cmd_buf_index);
	bytes_to_output -= rc;
	cmd_buf_index += rc;
	/* check on existing SSID association ? */
	rc = snprintf(cmd_buf_index, bytes_to_output, " %s",
		      b_check_assoc ? "true" : "false");
	if ((rc < 0) || (rc >= (int)bytes_to_output)) {
		plat_apply_conf_log("snprintf() failed !");
		err = kNoMemoryErr;
		goto _join_exit;
	}
	plat_apply_conf_log("cmd_buf_index=[%s]", cmd_buf_index);
	bytes_to_output -= rc;
	cmd_buf_index += rc;
	/* SSID */
	rc = snprintf(cmd_buf_index, bytes_to_output,
		      " \"%s\"", inSSID);
	if ((rc < 0) || (rc >= (int)bytes_to_output)) {
		plat_apply_conf_log("snprintf() failed !");
		err = kNoMemoryErr;
		goto _join_exit;
	}
	plat_apply_conf_log("cmd_buf_index=[%s]", cmd_buf_index);
	bytes_to_output -= rc;
	cmd_buf_index += rc;
	/* PSK is applicable */
	if (inWiFiPSKLen && inWiFiPSK) {
		size_t psk_index = 0;
		bool is_ascii_psk = is_printable_ascii_string(inWiFiPSK, inWiFiPSKLen);
		rc = snprintf(cmd_buf_index, bytes_to_output, " ");
		if ((rc < 0) || (rc >= (int)bytes_to_output)) {
			plat_apply_conf_log("snprintf() failed !");
			err = kNoMemoryErr;
			goto _join_exit;
		}
		plat_apply_conf_log("cmd_buf_index=[%s]", cmd_buf_index);
		bytes_to_output -= rc;
		cmd_buf_index += rc;

		psk_str = cmd_buf_index;

		if (is_ascii_psk) {
			rc = snprintf(cmd_buf_index, bytes_to_output, "%c", '"');
			if ((rc < 0) || (rc >= (int)bytes_to_output)) {
				plat_apply_conf_log("snprintf() failed !");
				err = kNoMemoryErr;
				goto _join_exit;
			}
			bytes_to_output -= rc;
			cmd_buf_index += rc;
		}

		for (psk_index = 0; psk_index < inWiFiPSKLen; psk_index++) {
			rc = snprintf(cmd_buf_index, bytes_to_output,
				      (is_ascii_psk ? "%c" : "%02hhx"),
				      inWiFiPSK[psk_index]);
			if ((rc < 0) || (rc >= (int)bytes_to_output)) {
				plat_apply_conf_log("snprintf() failed !");
				err = kNoMemoryErr;
				goto _join_exit;
			}
			bytes_to_output -= rc;
			cmd_buf_index += rc;
		}

		if (is_ascii_psk) {
			rc = snprintf(cmd_buf_index, bytes_to_output, "%c", '"');
			if ((rc < 0) || (rc >= (int)bytes_to_output)) {
				plat_apply_conf_log("snprintf() failed !");
				err = kNoMemoryErr;
				goto _join_exit;
			}
			bytes_to_output -= rc;
			cmd_buf_index += rc;
		}

		plat_apply_conf_log("cmd_buf_index=[%s]", psk_str);
	}

	if (b_store_to_nvram) {
		if (!psk_str) {
			psk_str = (char *)empty_string;
		}
		rc = plat_helper_save_wac_psk(psk_str);
		if (rc != 0) {
			plat_apply_conf_log("plat_helper_save_wac_psk failed !");
		}
	}

	rc = system(cmd_buf);
	if (rc == -1) {
		plat_apply_conf_log("%s:%s failed !", PLAT_WL_SCRIPT_NAME, PLAT_WL_SCRIPT_FUNC);
		err = kUnknownErr;
	}
	if (WIFSIGNALED(rc) &&
	    ((WTERMSIG(rc) == SIGINT) || (WTERMSIG(rc) == SIGQUIT))) {
		plat_apply_conf_log("%s:%s was interrupted by SIGINT or SIGQUIT !", PLAT_WL_SCRIPT_NAME, PLAT_WL_SCRIPT_FUNC);
		err = kEndingErr;
	}
 _join_exit:
	if (cmd_buf) {
		free(cmd_buf);
	}
	plat_apply_conf_log("%s()-", __FUNCTION__);
	return err;
}

int plat_helper_retrieve_name(char **wac_name)
{
	int rc = -1;
	if (wac_name) {
		wac_name_storage[0] = '\0';
		rc = gethostname(wac_name_storage, sizeof(wac_name_storage));
		*wac_name = wac_name_storage;
	}
	return rc;
}

int plat_helper_save_name(const char *wac_name)
{
	int rc = -1;
	if (wac_name) {
		rc = sethostname(wac_name, strlen(wac_name));
	}
	return rc;
}

#ifdef USE_BRCM_ROUTER_NVRAM

int plat_helper_retrieve_wac_ssid(char **wac_nvram_ssid)
{
	int rc = -1;
	if (wac_nvram_ssid) {
		char *ssid = nvram_get(PLAT_NVRAM_WAC_WIFI_SSID);
		if (!ssid || !ssid[0]) {
			*wac_nvram_ssid = NULL;
		} else {
			*wac_nvram_ssid = ssid;
			rc = 0;
		}
	}
	return rc;
}

int plat_helper_retrieve_wac_psk(char **wac_nvram_psk)
{
	int rc = -1;
	if (wac_nvram_psk) {
		char *psk = nvram_get(PLAT_NVRAM_WAC_WIFI_PSK);
		if (!psk || !psk[0]) {
			*wac_nvram_psk = NULL;
		} else {
			*wac_nvram_psk = psk;
			rc = 0;
		}
	}
	return rc;
}

int plat_helper_retrieve_play_password(char **wac_play_password)
{
	int rc = -1;
	if (wac_play_password) {
		char *play = nvram_get(PLAT_NVRAM_WAC_AIRPLAY_PASSWORD);
		if (!play || !play[0]) {
			*wac_play_password = NULL;
		} else {
			*wac_play_password = play;
			rc = 0;
		}
	}
	return rc;
}

int plat_helper_save_wac_ssid(const char *wac_nvram_ssid)
{
	int rc = -1;
	if (wac_nvram_ssid) {
		rc = nvram_set(PLAT_NVRAM_WAC_WIFI_SSID, wac_nvram_ssid);
		if (rc != 0) {
			plat_apply_conf_log("nvram_set() failed !");
		}
		/* nvram_commit() is going to be called while dealing with the PSK */
	}
	return rc;
}

int plat_helper_save_wac_psk(const char *wac_nvram_psk)
{
	int rc = -1;
	if (wac_nvram_psk) {
		rc = nvram_set(PLAT_NVRAM_WAC_WIFI_PSK, wac_nvram_psk);
		if (rc != 0) {
			plat_apply_conf_log("nvram_set() failed !");
		}
		rc = nvram_commit();
		if (rc != 0) {
			plat_apply_conf_log("nvram_commit() failed !");
		}
	}
	return rc;
}

int plat_helper_save_play_password(const char *wac_play_password)
{
	int rc = -1;
	if (wac_play_password) {
		rc = nvram_set(PLAT_NVRAM_WAC_AIRPLAY_PASSWORD, wac_play_password);
		if (rc != 0) {
			plat_apply_conf_log("nvram_set() failed !");
		}
		rc = nvram_commit();
		if (rc != 0) {
			plat_apply_conf_log("nvram_commit() failed !");
		}
	}
	return rc;
}

int plat_helper_set_wac_configured(bool is_configured)
{
	int rc = -1;

	rc = nvram_set(PLAT_NVRAM_WAC_IS_CONFIGURED, is_configured ? "1" : "0");
	if (rc != 0) {
		plat_apply_conf_log("nvram_set() failed !");
	}
	else {
		rc = nvram_commit();
		if (rc != 0) {
			plat_apply_conf_log("nvram_commit() failed !");
		}
	}

	return rc;
}

int plat_helper_get_wac_configured(bool *p_is_configured)
{
	int rc = -1;
	if (p_is_configured) {
		char *value = nvram_get(PLAT_NVRAM_WAC_IS_CONFIGURED);
		if (!value || !value[0] || !strcmp(value, "0")) {
			*p_is_configured = false;
		}
		else {
			*p_is_configured = true;
		}
		rc = 0;
	}
	return rc;
}

#else /* ! USE_BRCM_ROUTER_NVRAM */

static inline int plat_write_text(const char *filename, const char *text)
{
	int rc = -1;
	int fd = -1;
	ssize_t bytes_written = -1;
	ssize_t to_be_written = 0;

	fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC|O_SYNC, PLAT_NVRAM_WAC_FILE_OPEN_MODE);
	if (fd == -1) {
		plat_apply_conf_log("open() failed !");
		goto _exit;
	}
	to_be_written = strlen(text);
	bytes_written = write(fd, text, to_be_written);
	if (bytes_written < to_be_written) {
		plat_apply_conf_log("write() failed !");
	}
	else {
		rc = 0;
	}
	if (close(fd) == -1) {
		plat_apply_conf_log("write() failed !");
		rc = -1;
	}
 _exit:
	return rc;
}

static inline int plat_read_text(const char *filename, char **text)
{
	int rc = -1;
	int fd = -1;
	ssize_t bytes_read = -1;

	*text = NULL;
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		plat_apply_conf_log("open() failed !");
		goto _exit;
	}
	bytes_read = read(fd, file_read_storage, sizeof(file_read_storage));
	if (bytes_read >= 0) {
		file_read_storage[bytes_read] = '\0';
		*text = file_read_storage;
		rc = 0;
	}
	if (close(fd) == -1) {
		plat_apply_conf_log("write() failed !");
		rc = -1;
	}
 _exit:
	return rc;
}

int plat_helper_retrieve_wac_ssid(char **wac_nvram_ssid)
{
	int rc = -1;
	if (wac_nvram_ssid) {
		rc = plat_read_text( PLAT_WL_SCRIPT_PATH "/" PLAT_NVRAM_WAC_WIFI_SSID PLAT_NVRAM_WAC_EXT, wac_nvram_ssid );
	}
	return rc;
}

int plat_helper_retrieve_wac_psk(char **wac_nvram_psk)
{
	int rc = -1;
	if (wac_nvram_psk) {
		rc = plat_read_text( PLAT_WL_SCRIPT_PATH "/" PLAT_NVRAM_WAC_WIFI_PSK PLAT_NVRAM_WAC_EXT, wac_nvram_psk );
	}
	return rc;
}

int plat_helper_retrieve_play_password(char **wac_play_password)
{
	int rc = -1;
	if (wac_play_password) {
		rc = plat_read_text( PLAT_WL_SCRIPT_PATH "/" PLAT_NVRAM_WAC_AIRPLAY_PASSWORD PLAT_NVRAM_WAC_EXT, wac_play_password);
	}
	return rc;
}

int plat_helper_save_wac_ssid(const char *wac_nvram_ssid)
{
	int rc = -1;
	if (wac_nvram_ssid) {
		rc = plat_write_text( PLAT_WL_SCRIPT_PATH "/" PLAT_NVRAM_WAC_WIFI_SSID PLAT_NVRAM_WAC_EXT, wac_nvram_ssid );
	}
	return rc;
}

int plat_helper_save_wac_psk(const char *wac_nvram_psk)
{
	int rc = -1;
	if (wac_nvram_psk) {
		rc = plat_write_text( PLAT_WL_SCRIPT_PATH "/" PLAT_NVRAM_WAC_WIFI_PSK PLAT_NVRAM_WAC_EXT, wac_nvram_psk );
	}
	return rc;
}

int plat_helper_save_play_password(const char *wac_play_password)
{
	int rc = -1;
	if (wac_play_password) {
		rc = plat_write_text( PLAT_WL_SCRIPT_PATH "/" PLAT_NVRAM_WAC_AIRPLAY_PASSWORD PLAT_NVRAM_WAC_EXT, wac_play_password);
	}
	return rc;
}

int plat_helper_set_wac_configured(bool is_configured)
{
	int rc = -1;
	rc = plat_write_text( PLAT_WL_SCRIPT_PATH "/" PLAT_NVRAM_WAC_IS_CONFIGURED PLAT_NVRAM_WAC_EXT, is_configured ? "1" : "0");
	return rc;
}

int plat_helper_get_wac_configured(bool *p_is_configured)
{
	int rc = -1;
	if (p_is_configured) {
		char *value = NULL;
		rc = plat_read_text( PLAT_WL_SCRIPT_PATH "/" PLAT_NVRAM_WAC_IS_CONFIGURED PLAT_NVRAM_WAC_EXT, &value);
		if (!value || !value[0] || !strncmp(value, "0", 1)) {
			*p_is_configured = false;
		}
		else {
			*p_is_configured = true;
		}
		rc = 0;
	}
	return rc;
}

#endif /* USE_BRCM_ROUTER_NVRAM */
