/*******************************************************************************
 * Copyright (c) 2015 YS Lee <aqwerf@gmail.com>
 * All right reserved.
 *
 * This software is confidential and proprietary information.
. * You shall use it only in accordance with the terms of the
 * license agreement you entered into with the copyright holder.
 ******************************************************************************/
#include <sys/unistd.h>

#include "wiced.h"
#include "resources.h"

#include "spi_flash.h"
#include "wiced_apps_common.h"

static app_header_t headers[DCT_MAX_APP_COUNT];
static app_header_t headers_tmp[DCT_MAX_APP_COUNT];

#define SFLASH_SECTOR_SIZE		( 4096UL )
#define SECTORS(size)			(((size) + SFLASH_SECTOR_SIZE - 1) / SFLASH_SECTOR_SIZE)

#define WIFI_FIRMWARE_SIZE		(512 * 1024)		//128 Sectors
#define APP_IMAGE_SIZE			(768 * 1024)		//192 Sectors
#define RESERVED_SPACE_SIZE		(1 * 1024 * 1024)	//256 Sectors
#define DATA_SAPCE_SIZE			(12 * 1024 * 1024)	//3072 Sectors : FIXED up to 12MB

#define FIRMWARE_SIZE	(400 * 1024)
#define IMAGE_SIZE	(500 * 1024)

#define SFLASH_SIZE	(1 * 1024 * 1024)

static void build_headers(void)
{
	int start = 1;
	int size;

	memset(&headers, 0, sizeof(headers));

	size = SECTORS(WIFI_FIRMWARE_SIZE);		//512KB, 128 Sectors
	headers[DCT_WIFI_FIRMWARE_INDEX].count = 1;
	headers[DCT_WIFI_FIRMWARE_INDEX].sectors[0].start = start;
	headers[DCT_WIFI_FIRMWARE_INDEX].sectors[0].count = size;

	start += size;
	size = SECTORS(APP_IMAGE_SIZE);			//768KB, 192 Sectors
	headers[DCT_FR_APP_INDEX].count = 1;
	headers[DCT_FR_APP_INDEX].sectors[0].start = start;
	headers[DCT_FR_APP_INDEX].sectors[0].count = size;

	start += size;
	size = SECTORS(APP_IMAGE_SIZE);			//768KB, 192 Sectors
	headers[DCT_APP0_INDEX].count = 1;
	headers[DCT_APP0_INDEX].sectors[0].start = start;
	headers[DCT_APP0_INDEX].sectors[0].count = size;

	start += size;
	size = SECTORS(APP_IMAGE_SIZE);			//768KB, 192 Sectors
	headers[DCT_APP1_INDEX].count = 1;
	headers[DCT_APP1_INDEX].sectors[0].start = start;
	headers[DCT_APP1_INDEX].sectors[0].count = size;

	start += size;
	size = SECTORS(RESERVED_SPACE_SIZE);	//1MB, 256 Sectors
	headers[DCT_APP2_INDEX].count = 1;
	headers[DCT_APP2_INDEX].sectors[0].start = start;
	headers[DCT_APP2_INDEX].sectors[0].count = size;
	
	start += size;
	size = SECTORS(DATA_SAPCE_SIZE);		//12MB, 3072 Sectors
	headers[DCT_FILESYSTEM_IMAGE_INDEX].count = 1;
	headers[DCT_FILESYSTEM_IMAGE_INDEX].sectors[0].start = start;
	headers[DCT_FILESYSTEM_IMAGE_INDEX].sectors[0].count = size;	
}

static wiced_result_t is_same_wifi(void)
{
	int i;
	uint8_t *buf;
	wiced_app_t app;

	if (wiced_framework_app_open(DCT_WIFI_FIRMWARE_INDEX, &app) != WICED_SUCCESS)
		return WICED_ERROR;

	buf = malloc(4096);

	for (i = 0; i < wifi_firmware_image.size / 4096; i++) {
		wiced_framework_app_read_chunk(&app, 4096*i, buf, 4096);

		if (memcmp(&wifi_firmware_image.val.mem.data[i*4096], buf, 4096) != 0) {
			printf("Error offset: %d\n", i);
			goto _error;
		}
	}
	if ((wifi_firmware_image.size % 4096) != 0) {
		wiced_framework_app_read_chunk(&app, 4096*i, buf, 4096);
		if (memcmp(&wifi_firmware_image.val.mem.data[i*4096], buf,
			   wifi_firmware_image.size % 4096) != 0) {
			printf("Error offset: %d\n", i);
			goto _error;
		}
	}

	wiced_framework_app_close(&app);
	free(buf);
	return WICED_SUCCESS;

_error:
	wiced_framework_app_close(&app);
	free(buf);
	return WICED_ERROR;
}

static wiced_result_t check_header(const sflash_handle_t* sflash_handle)
{
	wiced_result_t result;
	result = sflash_read(sflash_handle, 0, &headers_tmp, sizeof(headers_tmp));

	if (result != WICED_SUCCESS || memcmp(headers, headers_tmp, sizeof(headers_tmp)) != 0) {
		return WICED_ERROR;
	}
	return WICED_SUCCESS;
}

extern void application_start(void)
{
	wiced_result_t result;
	sflash_handle_t sflash_handle;
	unsigned long size;
	wiced_app_t app;
	uint32_t i;

	/* Initialise the device */
	result = wiced_core_init();
	printf("Init System(%d)\n", result);

	platform_led_set_state(WICED_LED_INDEX_1, WICED_LED_ON);
	/**************************************************************/
	/* build sflash header  */
        /* Initialise the serial flash driver */
        if ( init_sflash( &sflash_handle, 0, SFLASH_WRITE_ALLOWED ) != 0 )
        {
            /* return from main in order to try rebooting... */
		printf("#### Fail to init sflahs\n");
		return;
        }
	printf("Sflash init ok = 0x%0x\n", (unsigned int)sflash_handle.device_id);

	result  = sflash_get_size(&sflash_handle, &size);
	printf("Sflash size(%d) = %ludBytes\n", result, size);

	build_headers();
	/* modify wifi header size */
	headers[DCT_WIFI_FIRMWARE_INDEX].sectors[0].count = SECTORS(wifi_firmware_image.size);

	if (check_header(&sflash_handle) == WICED_SUCCESS && is_same_wifi() == WICED_SUCCESS) {
		printf("Already Upgraded\n");
		platform_led_set_state(WICED_LED_INDEX_1, WICED_LED_OFF);
		platform_led_set_state(WICED_LED_INDEX_2, WICED_LED_ON);
		while (1) {
			wiced_rtos_delay_milliseconds(1000);
		}
	}

	result = sflash_chip_erase(&sflash_handle);
	printf("Sflash erase all = %d\n", result);

	result = sflash_write(&sflash_handle, 0, &headers, sizeof(headers));
	printf("Sflash build header = %d\n", result);

	/**************************************************************/
	/* sflash write */
	wiced_framework_app_open(DCT_WIFI_FIRMWARE_INDEX, &app);
	wiced_framework_app_get_size(&app, &i);
	printf("FW Area = %ud(%d)\n", (unsigned int)i, (int)wifi_firmware_image.size);

	printf("writing...\n");
	if (wifi_firmware_image.size % 2)
		i = wifi_firmware_image.size + 1;
	else
		i = wifi_firmware_image.size;

	wiced_framework_app_write_chunk(&app, (uint8_t*)wifi_firmware_image.val.mem.data, i);
	wiced_framework_app_close(&app);

	printf("verifying...\n");

	if (is_same_wifi() == WICED_SUCCESS) {
		printf("verifying... ok\n");
	} else {
		printf("verifying... FAIL\n");
	}

	platform_led_set_state(WICED_LED_INDEX_1, WICED_LED_OFF);
	platform_led_set_state(WICED_LED_INDEX_2, WICED_LED_ON);

	while (1) {
		wiced_rtos_delay_milliseconds(1000);
	}
}
