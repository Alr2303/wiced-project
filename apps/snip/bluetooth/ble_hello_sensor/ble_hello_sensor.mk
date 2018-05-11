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

NAME := Bluetooth_Low_Energy_Hello_Sensor_Application

$(NAME)_SOURCES    := ble_hello_sensor.c \
                      wiced_bt_cfg.c

$(NAME)_INCLUDES   := .
$(NAME)_COMPONENTS := utilities/command_console

PLATFORMS_FOR_POWER_SAVE := CYW943907WAE3 BCM94343WWCD1
#ENABLE_APP_POWERSAVE macro will be used only for platforms mentioned in PLATFORM_FOR_POWER_SAVE_FLAG.
ifneq ($(filter $(PLATFORM),$(PLATFORMS_FOR_POWER_SAVE)),)
$(info "Power Save enabled")
GLOBAL_DEFINES   := ENABLE_APP_POWERSAVE

ifeq ($(PLATFORM), CYW943907WAE3)
GLOBAL_DEFINES   += PLATFORM_POWERSAVE_DEFAULT=1
GLOBAL_DEFINES   += PLATFORM_MCU_POWERSAVE_MODE_INIT=PLATFORM_MCU_POWERSAVE_MODE_DEEP_SLEEP
#GLOBAL_DEFINES   += PLATFORM_MCU_POWERSAVE_MODE_INIT=PLATFORM_MCU_POWERSAVE_MODE_SLEEP
# GLOBAL_DEFINES   += WICED_DISABLE_WATCHDOG
GLOBAL_DEFINES   += APPLICATION_WATCHDOG_TIMEOUT_SECONDS=60
GLOBAL_DEFINES   += WICED_NO_WIFI
GLOBAL_DEFINES   += NO_WIFI_FIRMWARE
endif

endif

# Set the below flag to 1, if application needs to be executed on Embedded Stack (CYW943907WAE3 only).
USE_BT_EMBED_MODE := 0

ifeq ($(USE_BT_EMBED_MODE), 1)
$(info "Embedded Stack")
$(NAME)_COMPONENTS += libraries/drivers/bluetooth/wiced_hci_bt \
                      libraries/protocols/wiced_hci

GLOBAL_DEFINES     += USE_WICED_HCI
EMBEDDED_APP_NAME      := headset
# The below BT chip related config is required for Fly-wire setup only, as this is not defined in the BCM943907WCD2 platform file.
ifeq ($(PLATFORM), BCM943907WCD2)
BT_CHIP            := 20706
BT_CHIP_REVISION   := A2
BT_CHIP_XTAL_FREQUENCY := 20MHz
endif

VALID_PLATFORMS    = CYW943907WAE3 BCM943907WCD2
INVALID_PLATFORMS  += BCM9WCD1AUDIO BCM943909*

else
$(info "Host Stack")
$(NAME)_COMPONENTS += libraries/drivers/bluetooth/low_energy

VALID_PLATFORMS += BCM9WCDPLUS114 \
                   BCM943909WCD* \
                   BCM943340WCD1 \
                   BCM9WCD1AUDIO \
                   BCM943438WLPTH_2 \
                   BCM943907WAE_1 \
                   BCM943340WCD1 \
                   BCM94343WWCD1 \
                   BCM943438WCD1 \
                   BCM920739B0_EVAL \
                   BCM920739B0_HB2 \
                   CYW943907WAE3 \
                   BCM943907AEVAL2F

INVALID_PLATFORMS  += BCM943907WCD2*
endif

# Enable this flag to get bluetooth protocol traces
#GLOBAL_DEFINES     += ENABLE_BT_PROTOCOL_TRACES

