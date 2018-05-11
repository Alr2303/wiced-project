/*
 * Copyright 2017, Cypress Semiconductor Corporation or a subsidiary of 
 * Cypress Semiconductor Corporation. All Rights Reserved.
 * 
 * This software, associated documentation and materials ("Software"),
 * is owned by Cypress Semiconductor Corporation
 * or one of its subsidiaries ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products. Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

#include <nuttx/config.h>

#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <nuttx/arch.h>
#include "arch/chip/wifi.h"

#include "netlib.h"
#include "dhcpc.h"

#include "wiced.h"
#include "airplay_posix_dct.h"
#include "wiced_framework.h"

#include "plat_helper.h"

#define DEBUG 1
#ifdef  DEBUG
  #define SHORT_FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
  #define custom_log(N, M, ...) printf("[%s: %s:%4d] " M "\n", N, SHORT_FILE, __LINE__, ##__VA_ARGS__)
  #define plat_helper_log(M, ...) custom_log("plat_helper", M, ##__VA_ARGS__)
#else
  #define custom_log(N, M, ...)
#endif

#define NET_DEVNAME "eth0"

#define MDNSRESPONDER_NAME      "mdnsr"
#define MDNSRESPONDER_PRIORITY  50
#define MDNSRESPONDER_STACKSIZE 16384

#define AIRPLAY_NAME            "AirPlay"
#define AIRPLAY_PRIORITY        50
#define AIRPLAY_STACKSIZE       16384

#define SOFTAP_IPADDR           "192.168.0.1"
#define SOFTAP_NETMASK          "255.255.255.0"
#define SOFTAP_GATEWAY          "192.168.0.1"

#define PLAT_HELPER_MAX_IFACE_NAME_LENGTH (8)
#define PLAT_HELPER_MAX_IFACE_STA_COUNT   (2)

#define PLAT_HELPER_IFACE_SOFTAP          "eth0"
#define PLAT_HELPER_IFACE_STA             "eth0"

#define WIFI_SSID_LENGTH_OCTETS           32
#define WIFI_PASSPHRASE_LENGTH_OCTETS     64
#define PLAY_PASSWORD_LENGTH_OCTETS       32

#define AIRPLAY_SPEAKER_NAME_SIZE              ( AIRPLAY_SPEAKER_NAME_STRING_LENGTH  + 1 )
#define AIRPLAY_SPEAKER_NAME_OFFSET            ( DCT_OFFSET(airplay_dct_t, device_name) )

#define AIRPLAY_PLAY_PASSWORD_SIZE             ( AIRPLAY_PLAY_PASSWORD_STRING_LENGTH  + 1 )
#define AIRPLAY_PLAY_PASSWORD_OFFSET           ( DCT_OFFSET(airplay_dct_t, play_password) )

#define AIRPLAY_DEVICE_CONFIGURED_SIZE         ( sizeof( uint32_t) )
#define AIRPLAY_DEVICE_CONFIGURED_OFFSET       ( DCT_OFFSET(airplay_dct_t, device_configured) )

#define AIRPLAY_APP_DCT_ACCESS_LOCK                pthread_mutex_lock(&s_wac_dct_mutex)
#define AIRPLAY_APP_DCT_ACCESS_UNLOCK              pthread_mutex_unlock(&s_wac_dct_mutex)


//static const char *s_wac_iface_list[]    = { PLAT_HELPER_IFACE_SOFTAP, NULL };
static const char s_wac_iface_softap[PLAT_HELPER_MAX_IFACE_NAME_LENGTH + 1];
static const char s_wac_iface_sta_list[PLAT_HELPER_MAX_IFACE_NAME_LENGTH + 1][PLAT_HELPER_MAX_IFACE_STA_COUNT];
static pid_t s_wac_mdnsd_pid;
static pid_t s_wac_airplayd_pid;
static char s_wac_name_storage[AIRPLAY_SPEAKER_NAME_SIZE];
static char s_wac_play_password_storage[AIRPLAY_PLAY_PASSWORD_SIZE];
static uint32_t s_wac_device_configured = 0xdeadbeef;
static platform_dct_wifi_config_t s_wac_platform_dct_wifi_config;
static pthread_mutex_t s_wac_dct_mutex = PTHREAD_MUTEX_INITIALIZER;

static pid_t exec_app(const char *name, int priority, int stacksize, main_t entry,
           char * const argv[], const char *redirfile, int oflags);
static int ipconfig_dhcp(void);
static int set_devicename(void);
static int plat_helper_save_wifi_config( const char *ssid, wiced_security_t security_type, const uint8_t *passphrase, size_t keylen );
static int plat_helper_retrieve_wifi_config( platform_dct_wifi_config_t** wifi_dct );


extern int mdns_responder(int argc, char **argv);
extern int AirPlayMain( int argc, char **argv);

int plat_helper_get_mac_addr(uint8_t *mac, uint32_t maclen)
{
    int rc = 0;
    wiced_result_t result;
    wiced_mac_t wmac;

    if (maclen < sizeof(wiced_mac_t)) {
        plat_helper_log("maclen smaller than 6 !");
        rc = -1;
        goto _mac_exit;
    }

    result = wiced_wifi_get_mac_address( &wmac );
    if (result != WICED_SUCCESS)
    {
        plat_helper_log("failed to get WiFi interface MAC address\n");
        rc = -1;
        goto _mac_exit;
    }

    memcpy(mac, &wmac.octet, maclen);

_mac_exit:
    return rc;
}

int plat_helper_airplayd_start(void)
{
    s_wac_airplayd_pid = exec_app(AIRPLAY_NAME, AIRPLAY_PRIORITY, AIRPLAY_STACKSIZE, AirPlayMain,  NULL, NULL, 0);
    return s_wac_airplayd_pid;
}

int plat_helper_airplayd_stop(void)
{
    if (s_wac_airplayd_pid > 0)
    {
        task_delete(s_wac_airplayd_pid);
        s_wac_airplayd_pid = 0;
    }
    return 0;
}

int plat_helper_mdnsd_start(void)
{
    s_wac_mdnsd_pid = exec_app(MDNSRESPONDER_NAME, MDNSRESPONDER_PRIORITY, MDNSRESPONDER_STACKSIZE, mdns_responder,  NULL, NULL, 0);
    usleep(500 * 1000);
    return s_wac_mdnsd_pid;
}

int plat_helper_mdnsd_stop(void)
{
    if (s_wac_mdnsd_pid > 0)
    {
        task_delete(s_wac_mdnsd_pid);
        s_wac_mdnsd_pid = 0;
    }
    return 0;
}

OSStatus plat_helper_join_wifi(const char * const inSSID,
                   const uint8_t * const inWiFiPSK,
                   size_t inWiFiPSKLen, bool bSave)
{
  wiced_result_t err;
  wiced_security_t secType;
  wiced_scan_result_t ap_info;

  if (bSave)
  {
     /* Identify the security type for the SSID */
     err = wiced_wifi_find_ap( inSSID, &ap_info, NULL );
     if (err != 0)
     {
       plat_helper_log("Failed to find given Access Point\n" );
       goto exit;
     }
     secType = ap_info.security;

     plat_helper_log("SSID: %s, security: %d, passfrase len=%d\n", inSSID, secType, inWiFiPSKLen);

     /* Save the configuration */
     plat_helper_save_wifi_config(inSSID, secType, inWiFiPSK, inWiFiPSKLen );
  }
  /* Bring up WiFi interface */
  err = wifi_driver_up_by_interface(WICED_STA_INTERFACE);
  if (err != 0)
  {
    plat_helper_log("failed to bring up WiFi interface\n");
    goto exit;
  }

  /* Setup IP address */
  ipconfig_dhcp();

  /* set host name */
  set_devicename();

exit:
   return (int) err;
}

static int ipconfig_dhcp(void)
{
  void *handle;
  uint8_t mac[IFHWADDRLEN];
  int ret = -1;

  /* Get the MAC address of the NIC */
  netlib_getmacaddr(NET_DEVNAME, mac);

  /* Set up the DHCPC modules */
  handle = dhcpc_open(&mac, IFHWADDRLEN);

  /* Get an IP address.  Note that there is no logic for renewing the IP address in this
   * example.  The address should be renewed in ds.lease_time/2 seconds.
   */

  if (handle)
    {
        struct dhcpc_state ds;
        ret = dhcpc_request(NET_DEVNAME, handle, &ds);
        if (ret)
        {
            plat_helper_log("dhcpc_request() returns err=%d\n", errno);
        }
        else
        {
          plat_helper_log("DHCP response:");
          plat_helper_log("  IP addr: %s", inet_ntoa(ds.ipaddr));
          plat_helper_log("  Netmask: %s", inet_ntoa(ds.netmask));
          plat_helper_log("  Gateway: %s", inet_ntoa(ds.default_router));

          if (ds.netmask.s_addr != 0)
            {
              netlib_set_ipv4netmask(NET_DEVNAME, &ds.netmask);
            }

          netlib_set_ipv4addr(NET_DEVNAME, &ds.ipaddr);

          if (ds.default_router.s_addr != 0)
            {
              netlib_set_dripv4addr(NET_DEVNAME, &ds.default_router);
            }

#if defined(CONFIG_NETDB_DNSCLIENT)
          if (ds.dnsaddr.s_addr != 0)
            {
              netlib_set_ipv4dnsaddr(&ds.dnsaddr);
            }
#endif
          ret = 0;
        }
        dhcpc_close(handle);
    }
    else
    {
        plat_helper_log("dhcpc_open returns err=%d\n", errno);
    }
  return ret;
}

static int set_devicename(void)
{
  char * devname;
  uint8_t mac[IFHWADDRLEN];
  char hostname[HOST_NAME_MAX];
  char mac_str[16];

  plat_helper_retrieve_name(&devname);
  if (strlen(devname) > 0)
  {
      strlcpy(hostname, devname, HOST_NAME_MAX);
  }
  else
  {
      /* create a unique name based on default host name, plue MAC address */
      netlib_getmacaddr(NET_DEVNAME, mac);
      snprintf(mac_str, sizeof(mac_str), "_%02x:%02x:%02x", mac[3], mac[4], mac[5]);

      gethostname(hostname, sizeof(hostname));
      hostname[HOST_NAME_MAX - 1] = '\0';
      strncat(hostname, mac_str, sizeof(hostname));
      hostname[HOST_NAME_MAX - 1] = '\0';
  }

  sethostname(hostname, strlen(hostname));

  return 0;
}

static pid_t exec_app(const char *name, int priority, int stacksize, main_t entry,  char * const argv[],
                      const char *redirfile, int oflags)
{
  posix_spawnattr_t attr;
  struct sched_param param;
  posix_spawn_file_actions_t file_actions;
  pid_t pid;
  int ret;

  /* Initialize attributes for task_spawn(). */

  ret = posix_spawnattr_init(&attr);
  if (ret != 0)
    {
      goto errout_with_errno;
    }

  ret = posix_spawn_file_actions_init(&file_actions);
  if (ret != 0)
    {
      goto errout_with_attrs;
    }

  /* Set the correct task size and priority */

  param.sched_priority = priority;
  ret = posix_spawnattr_setschedparam(&attr, &param);
  if (ret != 0)
    {
      goto errout_with_actions;
    }

  ret = task_spawnattr_setstacksize(&attr, stacksize);
  if (ret != 0)
    {
      goto errout_with_actions;
    }

   /* If robin robin scheduling is enabled, then set the scheduling policy
    * of the new task to SCHED_RR before it has a chance to run.
    */

  ret = posix_spawnattr_setschedpolicy(&attr, SCHED_RR);
  if (ret != 0)
    {
      goto errout_with_actions;
    }

  ret = posix_spawnattr_setflags(&attr,
                                 POSIX_SPAWN_SETSCHEDPARAM |
                                 POSIX_SPAWN_SETSCHEDULER);
  if (ret != 0)
    {
      goto errout_with_actions;
    }

  /* Is output being redirected? */

  if (redirfile)
    {
      /* Set up to close open redirfile and set to stdout (1) */

      ret = posix_spawn_file_actions_addopen(&file_actions, 1,
                                             redirfile, O_WRONLY, 0644);
      if (ret != 0)
        {
          plat_helper_log("ERROR: posix_spawn_file_actions_addopen failed: %d\n", ret);
          goto errout_with_actions;
        }
    }

  /* Start the built-in */

  ret = task_spawn(&pid, name, entry, &file_actions,
                   &attr, (argv) ? &argv[1] : (char * const *)NULL,
                   (char * const *)NULL);
  if (ret != 0)
    {
      plat_helper_log("ERROR: task_spawn failed: %d\n", ret);
      goto errout_with_actions;
    }

  /* Free attibutes and file actions.  Ignoring return values in the case
   * of an error.
   */

  /* Return the task ID of the new task if the task was sucessfully
   * started.  Otherwise, ret will be ERROR (and the errno value will
   * be set appropriately).
   */

  (void)posix_spawn_file_actions_destroy(&file_actions);
  (void)posix_spawnattr_destroy(&attr);
  return pid;

errout_with_actions:
  (void)posix_spawn_file_actions_destroy(&file_actions);

errout_with_attrs:
  (void)posix_spawnattr_destroy(&attr);

errout_with_errno:
  set_errno(ret);
  return ERROR;
}

static int plat_helper_retrieve_wifi_config( platform_dct_wifi_config_t** wifi_dct )
{
    wiced_result_t err;
    platform_dct_wifi_config_t* local_wifi_dct;
    err = wiced_dct_read_lock( (void**)&local_wifi_dct, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
    if (err != 0)
    {
        plat_helper_log("WiFi DCT read fail\n");
        goto exit;
    }
    memcpy((void *)&s_wac_platform_dct_wifi_config, local_wifi_dct, sizeof(platform_dct_wifi_config_t));
    if (wifi_dct)
    {
        *wifi_dct = &s_wac_platform_dct_wifi_config;
    }
exit:
    return 0;
}

static int plat_helper_save_wifi_config( const char *ssid, wiced_security_t security_type, const uint8_t *passphrase, size_t keylen )
{
    wiced_result_t err;
    platform_dct_wifi_config_t* local_wifi_dct;

    /* Read the current DCT data */
    err = plat_helper_retrieve_wifi_config(&local_wifi_dct);
    if (err != 0)
    {
        plat_helper_log("DCT read fail\n");
        goto exit;
    }

    /* Set the SSID */
    strlcpy( (char *)local_wifi_dct->stored_ap_list[0].details.SSID.value, ssid, WIFI_SSID_LENGTH_OCTETS );
    local_wifi_dct->stored_ap_list[0].details.SSID.length = strnlen( ssid, WIFI_SSID_LENGTH_OCTETS );

    /* Set the security type */
    local_wifi_dct->stored_ap_list[0].details.security = security_type;

    /* Set the passphrase */
    if ( ( security_type != WICED_SECURITY_OPEN ) && ( passphrase != NULL ) && ( keylen != 0 ))
    {
        if( ( security_type == WICED_SECURITY_WEP_PSK ) || ( security_type == WICED_SECURITY_WEP_SHARED ))
        {
            /* WICED WEP Pass-Phrase format is INDEX + KEY_LENGTH + PASSPHRASE
             * Actual pass phrase length is KEY_LENGTH + 2. */
            if (( keylen + 2) > WIFI_PASSPHRASE_LENGTH_OCTETS)
            {
                plat_helper_log("passphrase is too long(%d)\n", keylen);
                goto exit;
            }
            local_wifi_dct->stored_ap_list[0].security_key[0] = 0; // index 0
            local_wifi_dct->stored_ap_list[0].security_key[1] = (uint8_t) keylen; // length
            memcpy(&(local_wifi_dct->stored_ap_list[0].security_key[2]),passphrase,keylen );
            local_wifi_dct->stored_ap_list[0].security_key_length = (uint8_t)keylen + 2;
        }
        else
        {
            if ( keylen + 2 > WIFI_PASSPHRASE_LENGTH_OCTETS)
            {
                plat_helper_log("passphrase is too long(%d)\n", keylen);
                goto exit;
            }
            memcpy(local_wifi_dct->stored_ap_list[0].security_key, passphrase, keylen );
            local_wifi_dct->stored_ap_list[0].security_key_length = keylen;
        }
    }

    /* Save updated config details */
    local_wifi_dct->device_configured = WICED_TRUE;

    err = wiced_dct_write( (const void*) local_wifi_dct, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
    if (err != 0)
    {
        plat_helper_log("DCT write fail\n");
    }

exit:
    return (int) err;
}

int plat_helper_set_wac_configured(bool is_configured)
{
    wiced_result_t err;
    AIRPLAY_APP_DCT_ACCESS_LOCK;
    s_wac_device_configured = is_configured ? 1 : 0;
    err = wiced_dct_write( (void *)&s_wac_device_configured, DCT_APP_SECTION, AIRPLAY_DEVICE_CONFIGURED_OFFSET, AIRPLAY_DEVICE_CONFIGURED_SIZE );
    if (err != 0)
    {
       plat_helper_log("DCT write fail\n");
    }
    AIRPLAY_APP_DCT_ACCESS_UNLOCK;
    return err;
}

int plat_helper_get_wac_configured(bool *p_is_configured)
{
    wiced_result_t err = 0;
    AIRPLAY_APP_DCT_ACCESS_LOCK;
    if (s_wac_device_configured == 0xdeadbeef)
    {
        uint32_t *configured;
        err = wiced_dct_read_lock( ( void ** )&configured, WICED_FALSE, DCT_APP_SECTION, AIRPLAY_DEVICE_CONFIGURED_OFFSET, AIRPLAY_DEVICE_CONFIGURED_SIZE );
        if (err != 0)
        {
           plat_helper_log("wac configured DCT read fail\n");
           goto exit;
        }
        memcpy(&s_wac_device_configured, configured, AIRPLAY_DEVICE_CONFIGURED_SIZE);
    }

    *p_is_configured = (bool) s_wac_device_configured;

 exit:
    AIRPLAY_APP_DCT_ACCESS_UNLOCK;
    return (int) err;
}

int plat_helper_save_play_password( const char * password)
{
    wiced_result_t err = 0;
    AIRPLAY_APP_DCT_ACCESS_LOCK;
    if (password)
    {
        memset(s_wac_play_password_storage, 0, AIRPLAY_PLAY_PASSWORD_SIZE);
        strlcpy(s_wac_play_password_storage, password, AIRPLAY_PLAY_PASSWORD_SIZE);
        err = wiced_dct_write( (void *)s_wac_play_password_storage, DCT_APP_SECTION, AIRPLAY_PLAY_PASSWORD_OFFSET, AIRPLAY_PLAY_PASSWORD_SIZE );
        if (err != 0)
        {
           plat_helper_log("DCT write fail\n");
        }
    }
    AIRPLAY_APP_DCT_ACCESS_UNLOCK;
    return err;
}

int plat_helper_retrieve_play_password(char **wac_play_password)
{
    wiced_result_t err = 0;
    char *buf;

    AIRPLAY_APP_DCT_ACCESS_LOCK;
    if (wac_play_password)
    {
        err = wiced_dct_read_lock( ( void ** )&buf, WICED_FALSE, DCT_APP_SECTION, AIRPLAY_PLAY_PASSWORD_OFFSET, AIRPLAY_PLAY_PASSWORD_SIZE );
        if (err != 0)
        {
           plat_helper_log("play password DCT read fail\n");
           goto exit;
        }
        memcpy(s_wac_play_password_storage, buf, AIRPLAY_PLAY_PASSWORD_SIZE);
        *wac_play_password = s_wac_play_password_storage;
    }
exit:
    AIRPLAY_APP_DCT_ACCESS_UNLOCK;
    return err;
}


int plat_helper_save_name(const char * name)
{
    wiced_result_t err = 0;
    AIRPLAY_APP_DCT_ACCESS_LOCK;
    if (name)
    {
        memset(s_wac_name_storage, 0, AIRPLAY_SPEAKER_NAME_SIZE);
        strlcpy(s_wac_name_storage, name, AIRPLAY_SPEAKER_NAME_SIZE);
        err = wiced_dct_write( (void *)s_wac_name_storage, DCT_APP_SECTION, AIRPLAY_SPEAKER_NAME_OFFSET, AIRPLAY_SPEAKER_NAME_SIZE );
        if (err != 0)
        {
           plat_helper_log("device name DCT write fail\n");
           goto exit;
        }
    }
exit:
    AIRPLAY_APP_DCT_ACCESS_UNLOCK;
    // set to hostname
    sethostname(s_wac_name_storage, strlen(s_wac_name_storage));
    return err;
}

int plat_helper_retrieve_name(char **wac_name)
{
    wiced_result_t err = 0;
    char *buf;
    AIRPLAY_APP_DCT_ACCESS_LOCK;
    if (wac_name)
    {
        err = wiced_dct_read_lock( ( void ** )&buf, WICED_FALSE, DCT_APP_SECTION, AIRPLAY_SPEAKER_NAME_OFFSET, AIRPLAY_SPEAKER_NAME_SIZE );
        if (err != 0)
        {
           plat_helper_log("device name DCT read fail\n");
           goto exit;
        }
        memcpy(s_wac_name_storage, buf, AIRPLAY_SPEAKER_NAME_SIZE);
        if (strlen(s_wac_name_storage) == 0)
        {
            gethostname(s_wac_name_storage, sizeof(s_wac_name_storage));
        }
        *wac_name = s_wac_name_storage;
    }
exit:
    AIRPLAY_APP_DCT_ACCESS_UNLOCK;
    return err;
}

void plat_helper_platform_reboot()
{
    wiced_waf_reboot();
}
