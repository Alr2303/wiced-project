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

NAME := airplay_posix

$(NAME)_SOURCES := airplay_posix.c \
                   airplay_posix_dct.c \
                   airplay_posix_button_monitor.c \
                   plat_helper.c

$(NAME)_COMPONENTS += \
                      mfi/mfi_sap \
                      nuttx/dhcpc \
                      nuttx/dhcpd \
                      nuttx/netlib \
                      daemons/mDNSResponder \
                      daemons/AirPlayPosix \
                      daemons/WACPosix

APPLICATION_DCT    := airplay_posix_dct.c
WIFI_CONFIG_DCT_H  := wifi_config_dct.h

#Large stack needed for printf in debug mode
NuttX_START_STACK := 4000

# Enable powersave features
GLOBAL_DEFINES    += PLATFORM_POWERSAVE_DEFAULT=1 PLATFORM_WLAN_POWERSAVE_STATS=1

# Define pool sizes
GLOBAL_DEFINES    += TX_PACKET_POOL_SIZE=16
GLOBAL_DEFINES    += RX_PACKET_POOL_SIZE=32

# Define path to local headers
$(NAME)_CFLAGS    += -I$(SOURCE_ROOT)/apps/demo/airplay_posix \
                     -I$(SOURCE_ROOT)/libraries/daemons/WACPosix/Sources \
                     -I$(SOURCE_ROOT)/libraries/daemons/WACPosix/Platform/AirPlayNuttX

# No GMAC for now
PLATFORM_NO_GMAC  := 1

GLOBAL_DEFINES    += WICED_USE_AUDIO
GLOBAL_DEFINES    += USE_APPLE_WAC WAC_AIRPLAY_NUTTX


VALID_OSNS_COMBOS := NuttX-NuttX_NS

VALID_BUILD_TYPES := release

VALID_PLATFORMS   := BCM943909WCD1_3 BCM943907WAE2_1
INVALID_PLATFORMS :=

NUTTX_APP         := airplay_posix

$(NAME)_DEFINES   += TARGET_RT_LITTLE_ENDIAN=1
