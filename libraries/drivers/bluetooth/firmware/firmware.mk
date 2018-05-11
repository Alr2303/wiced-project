#
# $ Copyright Broadcom Corporation $
#
NAME := Lib_WICED_Bluetooth_Firmware_Driver_for_BCM$(BT_CHIP)$(BT_CHIP_REVISION)

# Use Firmware images which are already present

ifneq ($(EMBEDDED_APP_NAME),)
$(NAME)_SOURCES := $(BT_CHIP)$(BT_CHIP_REVISION)/$(BT_CHIP_XTAL_FREQUENCY)/$(EMBEDDED_APP_NAME)/$(EMBEDDED_APP_NAME)_bcm$(BT_CHIP)$(BT_CHIP_REVISION)_$(BT_CHIP_XTAL_FREQUENCY)_wiced_firmware_image.c
else
ifeq ( $(BT_CHIP_XTAL_FREQUENCY), )
$(NAME)_SOURCES := $(BT_CHIP)$(BT_CHIP_REVISION)/bt_firmware_image.c
else
$(NAME)_SOURCES := $(BT_CHIP)$(BT_CHIP_REVISION)/$(BT_CHIP_XTAL_FREQUENCY)/bt_firmware_image.c
endif
endif