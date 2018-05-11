#
# Copyright 2017, Cypress Semiconductor Corporation or a subsidiary of 
 # Cypress Semiconductor Corporation. All Rights Reserved.
 # This software, including source code, documentation and related
 # materials ("Software"), is owned by Cypress Semiconductor Corporation
 # or one of its subsidiaries ("Cypress") and is protected by and subject to
 # worldwide patent protection (United States and foreign),
 # United States copyright laws and international treaty provisions.
 # Therefore, you may use this Software only as provided in the license
 # agreement accompanying the software package from which you
 # obtained this Software ("EULA").
 # If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 # non-transferable license to copy, modify, and compile the Software
 # source code solely for use in connection with Cypress's
 # integrated circuit products. Any reproduction, modification, translation,
 # compilation, or representation of this Software except as specified
 # above is prohibited without the express written permission of Cypress.
 #
 # Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 # EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 # WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 # reserves the right to make changes to the Software without notice. Cypress
 # does not assume any liability arising out of the application or use of the
 # Software or any product or circuit described in the Software. Cypress does
 # not authorize its products for use in any products where a malfunction or
 # failure of the Cypress product may reasonably be expected to result in
 # significant property damage, injury or death ("High Risk Product"). By
 # including Cypress's product in a High Risk Product, the manufacturer
 # of such system or application assumes all risk of such use and in doing
 # so agrees to indemnify Cypress against all liability.
#

NAME := App_Apple_HomeKit_Certification

$(NAME)_SOURCES    := homekit_certification_test.c

$(NAME)_COMPONENTS := utilities/command_console\
                      libraries/protocols/HTTP2

WIFI_CONFIG_DCT_H  := wifi_config_dct.h

VALID_BUILD_TYPES  := release

APPLICATION_DCT := homekit_certification_test_dct.c

ifdef OTA2_SUPPORT
$(NAME)_COMPONENTS += filesystems/ota2
endif

ifdef USE_MFI
$(NAME)_COMPONENTS += mfi/apple_wac
else
GLOBAL_DEFINES += WICED_HOMEKIT_DO_NOT_USE_MFI
endif

ifdef USE_FIXED_SETUP_CODE
GLOBAL_DEFINES += USE_FIXED_SETUP_CODE
endif

GLOBAL_DEFINES += PLATFORM_POWERSAVE_DEFAULT=1

#Note: Only ThreadX-NetX_Duo OS has been internally tested against the Bonjour Conformance Test (with a pass )
VALID_OSNS_COMBOS  := ThreadX-NetX_Duo ThreadX-NetX
VALID_PLATFORMS    := BCM9WCDPLUS114 BCM943909WCD* BCM943438WLPTH_2 BCM943907AEVAL2F* BCM943907AEVAL1F*

ifeq ($(PLATFORM),$(filter $(PLATFORM),BCM943909WCD1_3 BCM943907WCD1 BCM943907WCD2 BCM943907AEVAL1F BCM943907AEVAL2F ))
GLOBAL_DEFINES += IOS9_ENABLED_SERVICES
GLOBAL_DEFINES += HOMEKIT_ICLOUD_ENABLE
$(NAME)_COMPONENTS += mfi/apple_homekit/icloud
else
$(NAME)_COMPONENTS += mfi/apple_homekit/ip
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=5
GLOBAL_DEFINES += RX_PACKET_POOL_SIZE=5
endif

ifeq ($(PLATFORM),$(filter $(PLATFORM),BCM943909WCD1_3 BCM943907WCD1 BCM943907WCD2 BCM943907AEVAL1F ))
#Uncomment below line to run app on Ethernet interface
#GLOBAL_DEFINES += RUN_ON_ETHERNET_INTERFACE
endif

#Set factory reset application (original OTA style) to be this same application.
ifneq (1, $(OTA2_SUPPORT))
$(info NOT OTA2, include FR_APP)
FR_APP    := $(OUTPUT_DIR)/binary/$(CLEANED_BUILD_STRING).stripped.elf
DCT_IMAGE := $(OUTPUT_DIR)/DCT.stripped.elf
else
$(info OTA2 Support, include ota2_extract as OTA_APP)
OTA_APPLICATION	:= snip.ota2_extract-$(PLATFORM)
OTA_APP    := build/$(OTA_APPLICATION)/binary/$(OTA_APPLICATION).stripped.elf
endif

