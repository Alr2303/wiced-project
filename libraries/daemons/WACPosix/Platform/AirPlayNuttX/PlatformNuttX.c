#include <nuttx/config.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/prctl.h>

#include "Common.h"     // For OSStatus

#include "PlatformRandomNumber.h"
#include "PlatformMFiAuth.h"
#include "PlatformBonjour.h"
#include "PlatformLogging.h"
#include "PlatformApplyConfiguration.h"
#include "PlatformSoftwareAccessPoint.h"

#include "MFiSAP.h"

#include "netlib.h"
#include "wiced.h"

#include "plat_helper.h"


#include "dhcpc.h"
#include "dhcpd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define WIFI_SSID_LENGTH_OCTETS           32

#define NET_DEVNAME "eth0"
#define SOFTAP_IPADDR           "192.168.0.1"
#define SOFTAP_NETMASK          "255.255.255.0"
#define SOFTAP_GATEWAY          "192.168.0.1"

// OUI
#define kAppleOUI                               ((uint8_t*)"\x00\xA0\x40")
#define kAppleOUISubType                        0x00


static int ipconfig_static(char * ipaddr_str, char *netmask_str, char * gateway_str);
extern int wifi_driver_ip_up(wiced_interface_t);
extern int wifi_driver_ip_down(wiced_interface_t);
//=============================================================================
//	PlatformMFiAuthInitialize
//=============================================================================
OSStatus PlatformMFiAuthInitialize( void )
{
    return MFiPlatform_Initialize( 1 );
}

//=============================================================================
//	PlatformMFiAuthFinalize
//=============================================================================
void PlatformMFiAuthFinalize( void )
{
    return MFiPlatform_Finalize();
}

//=============================================================================
//	MFiPlatform_CreateSignature
//=============================================================================
OSStatus PlatformMFiAuthCreateSignature( const void *inDigestPtr,
                                         size_t     inDigestLen,
                                         uint8_t    **outSignaturePtr,
                                         size_t     *outSignatureLen )
{
    return MFiPlatform_CreateSignature( inDigestPtr, inDigestLen,
                                        outSignaturePtr, outSignatureLen );
}

//=============================================================================
//	PlatformMFiAuthCopyCertificate
//=============================================================================
OSStatus PlatformMFiAuthCopyCertificate( uint8_t **outCertificatePtr, size_t *outCertificateLen )
{
    return MFiPlatform_CopyCertificate( outCertificatePtr, outCertificateLen );
}

//=============================================================================
//	PlatformCryptoStrongRandomBytes
//=============================================================================

OSStatus	PlatformCryptoStrongRandomBytes( void *inBuffer, size_t inByteCount )
{
	static int		sRandomFD = -1;
	uint8_t *		dst;
	ssize_t			n;

	if( sRandomFD < 0 )
	{
		sRandomFD = open( "/dev/random", O_RDONLY );
		if( sRandomFD < 0 )
        {
           plat_log( "open random device error: ");
           return kOpenErr;
        }
	}
	dst = (uint8_t *) inBuffer;
	while( inByteCount > 0 )
	{
		n = read( sRandomFD, dst, inByteCount );
		if( n < 0 )
        {
            plat_log( "read urandom error: ");
            return kReadErr;
        }
		dst += n;
		inByteCount -= n;
	}
	return( kNoErr );
}

//=============================================================================
//	PlatformMFiAuthCopyCertificate
//=============================================================================

OSStatus PlatformSoftwareAccessPointStart(const uint8_t *inIE, size_t inIELen)
{
    wiced_mac_t mac;
    wiced_custom_ie_info_t custom_ie;
    wiced_result_t result;
    OSStatus ret;
    wiced_ssid_t SSID;
    int channel = 6;
    uint8_t * buf;

    wiced_wifi_get_mac_address( &mac );

    /* get a buffer */
    buf = (uint8_t *)malloc(inIELen);
    if (buf == NULL)
    {
        plat_log("Failed to allocate buffer");
        return kNoMemoryErr;
    }

#define IE_VENDOR_PREFIX_LENGTH 6

    memcpy(buf, inIE + IE_VENDOR_PREFIX_LENGTH, inIELen - IE_VENDOR_PREFIX_LENGTH);

    /* set unique SSID */
    snprintf((void *)&SSID.value, WIFI_SSID_LENGTH_OCTETS,
             "Unconfigured Device %02x%02x%02x",
             mac.octet[3], mac.octet[4], mac.octet[5]);
    SSID.length = strlen( (char *) &SSID.value );

    /* Start softAP */
    memset(&custom_ie, 0, sizeof(custom_ie));
    memcpy(&custom_ie.oui, kAppleOUI, WIFI_IE_OUI_LENGTH);
    custom_ie.subtype = kAppleOUISubType;
    custom_ie.data    = buf;
    custom_ie.length  = inIELen - IE_VENDOR_PREFIX_LENGTH;
    custom_ie.which_packets = VENDOR_IE_BEACON | VENDOR_IE_PROBE_RESPONSE;

    result = wiced_wifi_start_ap_with_custom_ie( &SSID,
                                    WICED_SECURITY_OPEN,
                                    NULL,
                                    channel,
                                    &custom_ie);

    plat_log("Start softAP: SSID=%s, channel=%d", SSID.value, channel);

    if (buf)
    {
        free(buf);
        buf = NULL;
    }

    if (result != OK)
    {
        plat_log("Failed to start softAP, result=%d", result);
        ret = kInternalErr;
        goto _Return;
    }
    /* Bring up interface */
    result = wifi_driver_ip_up(WICED_AP_INTERFACE);
    if (result != OK)
    {
        plat_log("Failed to bring up WiFi interface, result=%d", result);
        ret = kInternalErr;
        goto _Return;
    }

    /* configure static IP address */
    if (ipconfig_static(SOFTAP_IPADDR, SOFTAP_NETMASK, SOFTAP_GATEWAY) != 0)
    {
        plat_log("Failed to configure IP static IP address ret=%d", result);
        ret = kInternalErr;
        goto _Return;
    }

    usleep(50*1000);

    /* start DHCP server */
    if (start_dhcpd() != 0)
    {
        plat_log("Failed to start DHCP server");
        ret = kInternalErr;
        goto _Return;
    }

    usleep(100 * 1000);

    ret = kNoErr;

_Return:
    return ret;
}


OSStatus PlatformSoftwareAccessPointStop(void)
{
    wiced_result_t result;
    OSStatus ret;

    /* Stop DHCP server */
    if (stop_dhcpd() != 0)
    {
        plat_log("Failed to stop DHCP server");
        ret = kInternalErr;
        goto _Return;
    }

    /* Bring down interface */
    result = wifi_driver_ip_down(WICED_AP_INTERFACE);
    if (result != OK)
    {
        plat_log("Failed to bring down WiFi interface, result=%d", result);
        ret = kInternalErr;
        goto _Return;
    }

    /* Stop AP */
    result = wiced_stop_ap();
    if (result != OK)
    {
        plat_log("Failed to stop softAP, result=%d", result);
        ret = kInternalErr;
        goto _Return;
    }

    ret = kNoErr;

_Return:
    return ret;
}

OSStatus PlatformApplyAirPlayPlayPassword(const char * const inPlayPassword)
{
    (void) inPlayPassword;
    return plat_helper_save_play_password(inPlayPassword);
 }

OSStatus PlatformApplyName(const char * const inName)
{
    (void) inName;
    return plat_helper_save_name(inName);
}

OSStatus PlatformJoinDestinationWiFiNetwork(const char * const inSSID,
                                            const uint8_t * const inWiFiPSK,
                                            size_t inWiFiPSKLen)
{
    return plat_helper_join_wifi(inSSID, inWiFiPSK, inWiFiPSKLen, true);
}


OSStatus PlatformInitializemDNSResponder(void)
{
   plat_helper_mdnsd_start();
   return kNoErr;
}

OSStatus PlatformMayStopmDNSResponder(void)
{
   plat_helper_mdnsd_stop();
   return kNoErr;
}


static int ipconfig_static(char * ipaddr_str, char *netmask_str, char * gateway_str)
{
  int ret = -1;
  struct in_addr ipaddr;
  struct in_addr netmask;
  struct in_addr gateway;

  uint8_t mac[IFHWADDRLEN];
  uint8_t iff;

  /* Get the MAC address of the NIC */
  netlib_getmacaddr(NET_DEVNAME, mac);

  /* get ip addr */
  ret = inet_aton(ipaddr_str, &ipaddr);
  if (ret <= 0)
  {
      plat_log("wrong IP address format: %s\n", ipaddr_str);
      ret = -1;
      goto ipc_static_err;
  }

  /* get netmask */
  ret = inet_aton(netmask_str, &netmask);
  if (ret <= 0)
  {
      plat_log("wrong netmask format: %s\n", netmask_str);
      ret = -1;
      goto ipc_static_err;
  }

  /* get gateway*/
  ret = inet_aton(gateway_str, &gateway);
  if (ret <= 0)
  {
      plat_log("wrong IP address: %s\n", gateway_str);
      ret = -1;
      goto ipc_static_err;
  }

  /* Get interface status */
  netlib_getifstatus(NET_DEVNAME, &iff);

  plat_log("IP address: %s\t netmask: %s", ipaddr_str, netmask_str);
  netlib_set_ipv4addr(NET_DEVNAME, &ipaddr);
  netlib_set_ipv4netmask(NET_DEVNAME, &netmask);

  ret = 0;

ipc_static_err:
  return ret;
}
