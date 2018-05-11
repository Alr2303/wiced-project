/*
 * Platform Helper functions/methods
 *
 * $Copyright (C) 2014 Broadcom Corporation All Rights Reserved.$
 *
 * $Id: plat_helper.c $
 */

#include "plat_helper.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>

#define plat_helper_log(M, ...) custom_log("plat_helper", M, ##__VA_ARGS__)

#define PLAT_HELPER_CMD_AIRPLAY_START     "/usr/sbin/airplayd &"
#define PLAT_HELPER_CMD_AIRPLAY_STOP      "killall -SIGTERM airplayd"
#define PLAT_HELPER_CMD_MDNS_START        "/usr/sbin/mdnsd &"
#define PLAT_HELPER_CMD_MDNS_STOP         "killall -SIGTERM mdnsd"

#define PLAT_HELPER_MAX_IFACE_NAME_LENGTH (8)
#define PLAT_HELPER_MAX_IFACE_STA_COUNT   (2)

#ifdef _PLAT_IFACE_SOFTAP
#define PLAT_HELPER_IFACE_SOFTAP           _PLAT_IFACE_SOFTAP
#else
#define PLAT_HELPER_IFACE_SOFTAP           "wlan0"
#endif

#ifdef _PLAT_IFACE_STA
#define PLAT_HELPER_IFACE_STA              _PLAT_IFACE_STA
#else
#define PLAT_HELPER_IFACE_STA              "wlan0"
#endif

static const char *s_wac_iface_list[]    = { PLAT_HELPER_IFACE_SOFTAP, NULL };

static const char s_wac_iface_softap[PLAT_HELPER_MAX_IFACE_NAME_LENGTH + 1];
static const char s_wac_iface_sta_list[PLAT_HELPER_MAX_IFACE_NAME_LENGTH + 1][PLAT_HELPER_MAX_IFACE_STA_COUNT];

static inline void plat_helper_init_struct_ifreq(struct ifreq *preq, const char *ifname)
{
	memset(preq, 0, sizeof(*preq));
	strcpy(preq->ifr_name, ifname);
}

int plat_helper_get_mac_addr(uint8_t *mac, uint32_t maclen)
{
	int rc = 0;
	struct if_nameindex *if_ni = NULL, *if_iter = NULL;
	int fd = -1;

	if (maclen < 6) {
		plat_helper_log("maclen smaller than 6 !");
		rc = -1;
		goto _mac_exit;
	}
	
	if_ni = if_nameindex();
	if (!if_ni) {
		plat_helper_log("if_nameindex() failed !");
		rc = -1;
		goto _mac_exit;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		plat_helper_log("WAC error: socket(SOCK_DGRAM) failed !");
		rc = -1;
		goto _mac_exit;
	}

	for (if_iter = if_ni; ! (if_iter->if_index == 0 && if_iter->if_name == NULL); if_iter++) {
		struct ifreq req;
		int if_flags;
		char **str_iter;
		size_t sa_data_len = sizeof(req.ifr_hwaddr.sa_data);

		plat_helper_log("%u->%s", if_iter->if_index, if_iter->if_name);
		
		for (str_iter = (char **)s_wac_iface_list; *str_iter != NULL; str_iter++) {
			if (!strcasecmp(if_iter->if_name, *str_iter)) {
				break;
			}
		}
		if (*str_iter == NULL) {
			continue;
		}
		plat_helper_init_struct_ifreq(&req, if_iter->if_name);
		rc = ioctl(fd, SIOCGIFFLAGS, &req);
		if (rc == -1) {
			plat_helper_log("ioctl SIOCGIFFLAGS failed !");
			goto _mac_exit;
		}
		if_flags = req.ifr_flags;
		plat_helper_init_struct_ifreq(&req, if_iter->if_name);
		rc = ioctl(fd, SIOCGIFHWADDR, &req);
		if (rc == -1) {
			plat_helper_log("ioctl SIOCGIFFLAGS failed !");
			goto _mac_exit;
		}
		memcpy(mac, req.ifr_hwaddr.sa_data,
		       (maclen < sa_data_len) ? maclen : sa_data_len);

		plat_helper_log("%s,mac=%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx,flags=0x%lx",
				if_iter->if_name,
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
				(unsigned long)if_flags);
	}

 _mac_exit:
	if (fd != -1) {
		close(fd);
	}
	if (if_ni) {
		if_freenameindex(if_ni);
	}
	return rc;
}

int plat_helper_airplayd_start(void)
{
	return system(PLAT_HELPER_CMD_AIRPLAY_START);
}

int plat_helper_airplayd_stop(void)
{
	return system(PLAT_HELPER_CMD_AIRPLAY_STOP);
}

int plat_helper_mdnsd_start(void)
{
	return system(PLAT_HELPER_CMD_MDNS_START);
}

int plat_helper_mdnsd_stop(void)
{
	return system(PLAT_HELPER_CMD_MDNS_STOP);
}
