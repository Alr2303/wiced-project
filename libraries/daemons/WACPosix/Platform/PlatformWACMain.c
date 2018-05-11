/*
 * Platform WAC Main
 *
 * $Copyright (C) 2014 Broadcom Corporation All Rights Reserved.$
 *
 * $Id: PlatformWACMain.c $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>

#include "WACServerAPI.h"
#include "PlatformApplyConfiguration.h"
#include "plat_helper.h"

#define UNUSED_ARG(x) (void)x

#define PLAT_WAC_SSID_JOIN_CHECK_SECS (10)

static WACContext_t s_wac_ctx;
static WACPlatformParameters_t s_wac_params;
static WACCallbackMessage_t s_wac_msg;

static const char  s_wac_firmware_rev_str[] = "0.0.1_alpha";
static const char  s_wac_hardware_rev_str[] = "1.0.0";
static const char  s_wac_name_str[] = "cy_airplay_wac";
static const char  s_wac_model_str[] = "cygnus";
static const char  s_wac_manufacturer_str[] = "Cypress";
static const char *s_wac_supported_ext_protocols[] = { NULL };

static void wac_server_callback(WACContext_t *p_ctx, WACCallbackMessage_t msg)
{
	if (p_ctx) {
		int rc;
		s_wac_msg = msg;
		syslog(LOG_NOTICE, "WAC status: [%s]", WACCallbackMessageToString(msg));
		fprintf(stderr, "\nWAC status: [%s]\n", WACCallbackMessageToString(msg));
		if (msg == WACCallbackMessage_ConfigComplete) {
			rc = plat_helper_set_wac_configured(true);
			if (rc != 0) {
				syslog(LOG_NOTICE, "WAC status: couldn't store config complete status !");
			}
		}
	}
}

static int wac_server_shutdown(WACContext_t *p_ctx)
{
	int rc = 0;
	if (WACServerStop(p_ctx) != kNoErr) {
		rc = -1;
	}
	return rc;
}

static inline bool wac_is_configured(void)
{
	bool rc = false;
	if (plat_helper_get_wac_configured(&rc) != 0) {
		syslog(LOG_WARNING, "WAC status: unable to get configuration status !");
	}
	return rc;
}

static inline bool wac_supports_airplay(void)
{
	bool rc = true;
	return rc;
}

static inline bool wac_supports_airprint(void)
{
	bool rc = false;
	return rc;
}

static inline bool wac_supports_2_g(void)
{
	bool rc = true;
	return rc;
}

static inline bool wac_supports_5_g(void)
{
	bool rc = true;
	return rc;
}

static inline bool wac_supports_wol(void)
{
	bool rc = false;
	return rc;
}

static inline char* wac_get_firmware_rev(void)
{
	return (char *)s_wac_firmware_rev_str;
}

static inline char* wac_get_hardware_rev(void)
{
	return (char *)s_wac_hardware_rev_str;
}

static inline char* wac_get_name(void)
{
	char *wac_name = NULL;
	if (plat_helper_retrieve_name(&wac_name) != 0) {
		syslog(LOG_WARNING, "WAC status: unable to get device name !");
		wac_name = (char *)s_wac_name_str;
	}
	return wac_name;
}

static inline char* wac_get_model(void)
{
	return (char *)s_wac_model_str;
}

static inline char* wac_get_manufacturer(void)
{
	return (char *)s_wac_manufacturer_str;
}

static inline char** wac_get_supported_ext_protocols(void)
{
	return (char **)s_wac_supported_ext_protocols;
}

static inline void print_help(int argc, const char *argv[])
{
	UNUSED_ARG(argc);
	fprintf(stderr,
		"Usage: %s [OPTION]\n\n"
		"       --help           help\n"
		"       --daemon         run as a daemon and be silent\n"
		"       --daemon-verbose run as a daemon but be verbose\n"
		"       --reset-config   resets WAC configuration status\n",
		argv[0]);
}

static void wac_server_int_handler(int signum)
{
	syslog(LOG_NOTICE, "WAC server received %d signal !", signum);
	if (!wac_is_configured()) {
		wac_server_shutdown(&s_wac_ctx);
		usleep(500000);
		plat_helper_mdnsd_start();
		usleep(500000);
		plat_helper_airplayd_start();
	}
	exit(0);
}

int main(int argc, const char *argv[])
{
	int rc = 0;
	int arg_index = 0;
	OSStatus wac_status = kNoErr; 

	for (arg_index = 1; arg_index < argc; arg_index++) {
		if (!strcasecmp(argv[arg_index], "--daemon" )) {
			rc = daemon(0, 0);
			if (rc != 0) {
				syslog(LOG_ERR, "Run %s as daemon failed with %d !", argv[0], rc);
				goto  _main_error;
			}
		}
		else if (!strcasecmp(argv[arg_index], "--daemon-verbose" )) {
			rc = daemon(0, 1);
			if (rc != 0) {
				syslog(LOG_ERR, "Run %s as daemon failed with %d !", argv[0], rc);
				goto  _main_error;
			}
		}
		else if (!strcasecmp(argv[arg_index], "--reset-config" )) {
			rc = plat_helper_set_wac_configured(false);
			if (rc != 0) {
				syslog(LOG_WARNING, "WAC server: couldn't reset config !");
			}
			rc = plat_helper_save_play_password("");
			if (rc != 0) {
				syslog(LOG_WARNING, "WAC server: couldn't reset play password !");
			}
		}
		else if (!strncasecmp(argv[arg_index], "--softap-iface", strlen("--softap-iface") )) {

		}
		else if (!strncasecmp(argv[arg_index], "--sta-iface", strlen("--sta-iface") )) {

		}
		else if (!strcasecmp(argv[arg_index], "--help" )) {
			print_help(argc, argv);
			goto _main_error;
		}
		else if ( argc > 1 ) {
			syslog(LOG_WARNING, "WAC server: unknown option !");
			print_help(argc, argv);
			goto _main_error;
		}
	}

	signal(SIGINT,  wac_server_int_handler);
	signal(SIGQUIT, wac_server_int_handler);
	signal(SIGTERM, wac_server_int_handler);
	signal(SIGPIPE, SIG_IGN);

	do {

		if ( wac_is_configured() == false ) {
			s_wac_msg = WACCallbackMessage_Initializing;
			memset(&s_wac_ctx, 0, sizeof(s_wac_ctx));
			memset(&s_wac_params, 0, sizeof(s_wac_params));
			s_wac_ctx.platformParams = &s_wac_params;
			s_wac_params.isUnconfigured = true;

			rc = plat_helper_get_mac_addr(s_wac_params.macAddress, sizeof(s_wac_params.macAddress));
			if (rc != 0) {
				goto _main_error;
			}

			s_wac_params.supportsAirPlay = wac_supports_airplay();
			s_wac_params.supportsAirPrint = wac_supports_airprint();
			s_wac_params.supports2_4GHzWiFi = wac_supports_2_g();
			s_wac_params.supports5GHzWiFi = wac_supports_5_g();
			s_wac_params.supportsWakeOnWireless = wac_supports_wol();
			s_wac_params.firmwareRevision = wac_get_firmware_rev();
			s_wac_params.hardwareRevision = wac_get_hardware_rev();
			s_wac_params.name = wac_get_name();
			s_wac_params.model = wac_get_model();
			s_wac_params.manufacturer = wac_get_manufacturer();
			s_wac_params.supportedExternalAccessoryProtocols = wac_get_supported_ext_protocols();

			/* stop airplayd and mdnsd */
			plat_helper_airplayd_stop();
			usleep(500000);
			plat_helper_mdnsd_stop();
			usleep(500000);

			wac_status = WACServerStart(&s_wac_ctx, wac_server_callback);
			if (wac_status != kNoErr) {
				syslog(LOG_WARNING, "WAC status: WACServerStart() failed !\n");
				goto _main_error;
			}

			while ( s_wac_msg < WACCallbackMessage_ConfigComplete ) {
				sleep(1);
			}

			rc = wac_server_shutdown(&s_wac_ctx);
			usleep(500000);
			plat_helper_mdnsd_start();
			usleep(500000);
			plat_helper_airplayd_start();
		}

		if ( wac_is_configured() == true ) {
			char buf_ssid[256] = "";
			char buf_psk[256] = "";
			char *ssid = NULL, *psk = NULL;
			size_t bufpsklen = 0;

			plat_helper_retrieve_wac_ssid(&ssid);
			if (ssid) {
				snprintf(buf_ssid, sizeof(buf_ssid), "%s", ssid);
			}
			plat_helper_retrieve_wac_psk(&psk);
			if (psk) {
				snprintf(buf_psk, sizeof(buf_psk), "%s", psk);
				bufpsklen = strlen(buf_psk);
			}
			do {
				wac_status = plat_helper_join_wifi(buf_ssid, (uint8_t *)buf_psk,
								   bufpsklen, false, true);
				sleep(PLAT_WAC_SSID_JOIN_CHECK_SECS);
			} while ( wac_is_configured() == true );
		}

	} while (true);

 _main_error:
	return rc;
}
