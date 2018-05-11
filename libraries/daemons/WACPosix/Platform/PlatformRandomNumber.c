/*
 * Platform Random Number
 *
 * $Copyright (C) 2014 Broadcom Corporation All Rights Reserved.$
 *
 * $Id: PlatformRandomNumber.c $
 */

#include "PlatformRandomNumber.h"
#include "Debug.h"

#include <stdio.h>
#include <string.h>

#define plat_rand_log(M, ...) custom_log("PlatformRandomNumber", M, ##__VA_ARGS__)

#define MIN_OF(a, b) (a > b ? b : a)
/* Typical output is: 4732043b-5dd0-4afa-af93-9adc172ae27b */
#define PLAT_RANDOM_UUID "/proc/sys/kernel/random/uuid"
#define PLAT_UUID_FORMAT "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"


OSStatus PlatformCryptoStrongRandomBytes(void *inBuffer, size_t inByteCount)
{
	int rc = 0;
	OSStatus err = kNoErr;
	uint8_t uuid[16];
	size_t bytes_copied = 0, bytes_pass_cnt = 0;
	uint8_t *rand_array = (uint8_t *)inBuffer;
	FILE *file_uuid = NULL;

	plat_rand_log("%s()+ (asked for %u bytes)",
		      __FUNCTION__, inByteCount);
	if (!inBuffer || !inByteCount) {
		err = kParamErr;
		plat_rand_log("inBuffer is NULL or inByteCount is 0");
		goto _random_exit;
	}
	
	do {
		bytes_pass_cnt = MIN_OF(sizeof(uuid), inByteCount - bytes_copied);
		file_uuid = fopen(PLAT_RANDOM_UUID, "r");
		if (!file_uuid) {
			err = kNotFoundErr;
			plat_rand_log("fopen([%s]) failed !", PLAT_RANDOM_UUID);
			break;
		}

		rc = fscanf(file_uuid, PLAT_UUID_FORMAT,
			    &uuid[0], &uuid[1], &uuid[2], &uuid[3],
			    &uuid[4], &uuid[5], &uuid[6], &uuid[7],
			    &uuid[8], &uuid[9], &uuid[10], &uuid[11],
			    &uuid[12], &uuid[13], &uuid[14], &uuid[15]);
		if (rc != 16) {
			plat_rand_log("issue with fscanf(); returns %d !", rc);
		}
		plat_rand_log("%02x %02x %02x %02x %02x %02x %02x %02x",
			      uuid[0], uuid[1], uuid[2], uuid[3],
			      uuid[4], uuid[5], uuid[6], uuid[7]);
		plat_rand_log("%02x %02x %02x %02x %02x %02x %02x %02x",
			      uuid[8], uuid[9], uuid[10], uuid[11],
			      uuid[12], uuid[13], uuid[14], uuid[15]);
		memcpy((uint8_t *)(rand_array + bytes_copied),
		       uuid, bytes_pass_cnt);
		bytes_copied += bytes_pass_cnt;
		rc = fclose(file_uuid);
	} while (inByteCount - bytes_copied);
	
 _random_exit:
	plat_rand_log("%s()-", __FUNCTION__);
	return err;
}
