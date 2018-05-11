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

NAME := avs_app

#WICED_ENABLE_TRACEX := 1

#GLOBAL_DEFINES     += WICED_DISABLE_WATCHDOG
#GLOBAL_DEFINES     += AVS_APP_ENABLE_WIFI_CMDS

# When using network debug logging, it's a good idea
# to bump up the TX packet pool.

#ENABLE_NETWORK_LOGGING := 1

ifdef ENABLE_NETWORK_LOGGING
GLOBAL_DEFINES     += AVS_APP_ENABLE_NETWORK_LOGGING
GLOBAL_DEFINES     += TX_PACKET_POOL_SIZE=30
else
GLOBAL_DEFINES     += TX_PACKET_POOL_SIZE=20
endif
GLOBAL_DEFINES     += RX_PACKET_POOL_SIZE=20
GLOBAL_DEFINES     += WICED_TCP_TX_DEPTH_QUEUE=20
GLOBAL_DEFINES     += WICED_TCP_RX_DEPTH_QUEUE=20
GLOBAL_DEFINES     += WICED_TCP_WINDOW_SIZE=20*1024

APPLICATION_DCT    := avs_app_dct.c

$(NAME)_SOURCES := avs_app.c \
                   avs_app_capture.c \
                   avs_app_config.c \
                   avs_app_console.c \
                   avs_app_dct.c \
                   avs_app_debug.c \
                   avs_app_keypad.c \
                   avs_app_playback.c \
                   avs_app_playlist.c

$(NAME)_COMPONENTS := protocols/SNTP \
                      audio/audio_client \
                      audio/avs_client \
                      utilities/command_console \
                      utilities/command_console/wifi \
                      utilities/command_console/dct \
                      utilities/bufmgr \
                      utilities/wiced_log \
                      inputs/button_manager

WIFI_CONFIG_DCT_H  := wifi_config_dct.h

ifdef WICED_ENABLE_TRACEX
$(info avs app using TraceX lib)

# Only use DDR on WCD1 boards
ifneq ($(filter $(PLATFORM),BCM943909WCD1_3),)
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_DDR_OFFSET=0x0
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_SIZE=0x200000
else
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_SIZE=0x60000
endif

$(NAME)_COMPONENTS += test/TraceX
$(NAME)_SOURCES    += avs_app_tracex.c
endif

GLOBAL_DEFINES     += WICED_USE_AUDIO

VALID_OSNS_COMBOS  := ThreadX-NetX_Duo
VALID_PLATFORMS    := BCM943909WCD* BCM943907* CYW943907WAE*
INVALID_PLATFORMS  := BCM943907AEVAL*
