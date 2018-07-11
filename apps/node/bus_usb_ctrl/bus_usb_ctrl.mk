NAME := App_usb_usb_ctrl

COMMON := ../../eventloop/common
LOCAL_COMMON := ../common

$(NAME)_SOURCES    := main.c \
			ctrl_console.c \
			$(COMMON)/console.c \
			$(COMMON)/device.c

GLOBAL_INCLUDES += $(COMMON)
$(NAME)_INCLUDES   += . \
		      $(COMMON) $(LOCAL_COMMON)

$(NAME)_COMPONENTS += \
		      utilities/wiced_log \
		      utilities/command_console \
	              utilities/command_console/dct

GLOBAL_DEFINES	   += TARGET_BUS_GW \
			WICED_DISABLE_MCU_POWERSAVE

WIFI_CONFIG_DCT_H := $(COMMON)/wifi_config_dct.h

APPLICATION_DCT := $(COMMON)/app_dct.c
