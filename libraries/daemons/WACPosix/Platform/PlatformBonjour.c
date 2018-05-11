/*
 * Platform Bonjour
 *
 * $Copyright (C) 2014 Broadcom Corporation All Rights Reserved.$
 *
 * $Id: PlatformBonjour.c $
 */
#include <stdio.h>

#include "PlatformBonjour.h"
#include "plat_helper.h"
#include "Debug.h"

#define plat_bonjour_log(M, ...) custom_log("PlatformBonjour", M, ##__VA_ARGS__)

OSStatus PlatformInitializemDNSResponder(void)
{
	OSStatus err = kNoErr;
	plat_bonjour_log("%s()+", __FUNCTION__);
	plat_helper_mdnsd_start();
	plat_bonjour_log("%s()-", __FUNCTION__);
	return err;
}

OSStatus PlatformMayStopmDNSResponder(void)
{
	OSStatus err = kNoErr;
	plat_bonjour_log("%s()+", __FUNCTION__);
	plat_helper_mdnsd_stop();
	plat_bonjour_log("%s()-", __FUNCTION__);
	return err;
}
