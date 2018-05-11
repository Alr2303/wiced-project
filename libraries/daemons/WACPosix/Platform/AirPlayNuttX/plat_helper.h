/*
 * Platform Helper functions/methods
 *
 * $Copyright (C) 2014 Broadcom Corporation All Rights Reserved.$
 *
 * $Id: plat_helper.h $
 */

#ifndef __PLAT_HELPER_H__
#define __PLAT_HELPER_H__

#include "Common.h"

int plat_helper_get_mac_addr(uint8_t *mac, uint32_t maclen);
int plat_helper_set_wac_configured(bool is_configured);
int plat_helper_get_wac_configured(bool *p_is_configured);
int plat_helper_retrieve_play_password(char **wac_play_password);
int plat_helper_retrieve_name(char **wac_name);
int plat_helper_save_play_password(const char *wac_play_password);
int plat_helper_save_name(const char *wac_name);

OSStatus plat_helper_join_wifi(const char * const inSSID,
			       const uint8_t * const inWiFiPSK,
			       size_t inWiFiPSKLen, bool bSave);

int plat_helper_airplayd_start(void);
int plat_helper_airplayd_stop(void);
int plat_helper_mdnsd_start(void);
int plat_helper_mdnsd_stop(void);

int plat_helper_retrieve_wac_ssid(char **wac_nvram_ssid);
int plat_helper_retrieve_wac_psk(char **wac_nvram_psk);
int plat_helper_save_wac_ssid(const char *wac_nvram_ssid);
int plat_helper_save_wac_psk(const char *wac_nvram_psk);

void plat_helper_platform_reboot(void);


#endif /* __PLAT_HELPER_H__ */

