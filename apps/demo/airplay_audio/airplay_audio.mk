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

NAME := App_airplay_audio

$(NAME)_SOURCES    := airplay_audio.c

$(NAME)_COMPONENTS := daemons/airplay_server \
                      daemons/Gedday \
                      utilities/command_console \
                      mfi/mfi_sap \
                      protocols/HTTP

ifdef USE_WIFI_COMMANDS
$(info *** Using WiFi console commands ***)
$(NAME)_COMPONENTS += utilities/command_console/wifi
GLOBAL_DEFINES     += USE_WIFI_CONSOLE_COMMANDS
endif

ifneq (,$(findstring mfi.apple_wac,$(COMPONENTS)))
# Enable WAC only on plaforms that are in the list below
PLATFORMS_SUPPORTING_WAC := BCM943909WCD1_3* BCM943907WAE_1* BCM943907APS* BCM943907WCD1* BCM943907WAE2_1* CYW943907WAE3*
$(eval PLATFORMS_SUPPORTING_WAC := $(call EXPAND_WILDCARD_PLATFORMS,$(PLATFORMS_SUPPORTING_WAC)))
ifneq ($(filter $(PLATFORM),$(PLATFORMS_SUPPORTING_WAC)),)
$(info *** Apple WAC is ENABLED ***)
#$(NAME)_COMPONENTS += mfi/apple_wac
$(NAME)_COMPONENTS += inputs/button_manager
GLOBAL_DEFINES     += USE_APPLE_WAC
endif
endif

APPLICATION_DCT    := airplay_audio_dct.c

ifeq (,$(findstring USE_APPLE_WAC,$(GLOBAL_DEFINES)))
WIFI_CONFIG_DCT_H  := wifi_config_dct.h
else
$(info *** NOT using wifi_config_dct.h ! ***)
# WiFi firmware may not accept all country codes; some might be locked out
GLOBAL_DEFINES     += WICED_COUNTRY_CODE=WICED_COUNTRY_UNITED_STATES
endif

# Platforms in the list below use more network buffers
PLATFORMS_WITH_EXTRA_BUFFERS := BCM943909WCD1_3* BCM943909B0FCBU BCM943907WAE_1* BCM943907APS* BCM943907WAE2_1* CYW943907WAE3*
$(eval PLATFORMS_WITH_EXTRA_BUFFERS := $(call EXPAND_WILDCARD_PLATFORMS,$(PLATFORMS_WITH_EXTRA_BUFFERS)))
ifneq ($(filter $(PLATFORM),$(PLATFORMS_WITH_EXTRA_BUFFERS)),)
GLOBAL_DEFINES     += TX_PACKET_POOL_SIZE=10
GLOBAL_DEFINES     += RX_PACKET_POOL_SIZE=28

ifdef WICED_ENABLE_TRACEX
$(info *** airplay_audio using TraceX ! ***)
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_DDR_OFFSET=0x0
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_SIZE=0x200000
$(NAME)_COMPONENTS += test/TraceX
endif

else
GLOBAL_DEFINES     += TX_PACKET_POOL_SIZE=5
GLOBAL_DEFINES     += RX_PACKET_POOL_SIZE=10
endif

GLOBAL_DEFINES     += WICED_USE_AUDIO
GLOBAL_DEFINES     += USE_DACP_CONSOLE
GLOBAL_DEFINES     += AUTO_IP_ENABLED

ifneq (,$(findstring BCM943907WAE_1,$(PLATFORM)))
# Option to use ssd1306 128x64 i2c display over WICED_I2C_2
$(info *** airplay_audio using SSD1306 display on WAE_1! ***)
GLOBAL_DEFINES     += USE_AUDIO_DISPLAY
$(NAME)_COMPONENTS += audio/display
endif

# Adjust stack size of hardware worker thread (increase to 4096 if thread is expected to call printf)
GLOBAL_DEFINES     += HARDWARE_IO_WORKER_THREAD_STACK_SIZE=1024

# For FR_APP use the BCM9WCD1AUDIO rather than BCM9WCD1AUDIO_EXT
FR_APP    := $(subst BCM9WCD1AUDIO_ext,BCM9WCD1AUDIO,$(OUTPUT_DIR))/binary/$(subst BCM9WCD1AUDIO_ext,BCM9WCD1AUDIO,$(CLEANED_BUILD_STRING)).stripped.elf
DCT_IMAGE := $(OUTPUT_DIR)/DCT.stripped.elf

VALID_OSNS_COMBOS  := ThreadX-NetX_Duo
VALID_PLATFORMS    := BCM9WCD1AUDIO BCM9WCD1AUDIO_ext BCM943909* BCM943907* CYW943907WAE3*
INVALID_PLATFORMS  := BCM943909QT BCM943907AEVAL*
