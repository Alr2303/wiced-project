NAME := App_bus_usb_charger

COMMON := ../../eventloop/common
LOCAL_COMMON := ../common

$(NAME)_SOURCES    := bus_usb_charger_main.c \
			bus_usb_console.c \
			coap_daemon.c \
			$(COMMON)/network.c \
			$(COMMON)/util.c $(COMMON)/eventloop.c \
			$(COMMON)/console.c \
			$(COMMON)/sys_led.c \
			$(COMMON)/sys_pwm.c \
			$(COMMON)/sys_button.c \
			$(COMMON)/sys_worker.c \
			$(COMMON)/json_parser.c \
			$(COMMON)/device.c

GLOBAL_INCLUDES += $(COMMON)
$(NAME)_INCLUDES   += . \
		      $(COMMON)

$(NAME)_COMPONENTS += protocols/HTTP \
		      protocols/COAP \
		      daemons/Gedday \
		      utilities/wiced_log \
		      utilities/command_console \
	              utilities/command_console/wifi \
		      utilities/command_console/platform \
	              utilities/command_console/dct

GLOBAL_DEFINES     += MAC_ADDRESS_SET_BY_HOST

GLOBAL_DEFINES	   += TARGET_BUS_USB_CHARGER


WIFI_CONFIG_DCT_H := $(COMMON)/wifi_config_dct.h

APPLICATION_DCT := $(COMMON)/app_dct.c
