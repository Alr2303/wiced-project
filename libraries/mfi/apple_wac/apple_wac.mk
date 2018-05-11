#
# $ Copyright Broadcom Corporation $
#
NAME := Lib_Apple_WAC

$(NAME)_HEADERS := apple_wac.h

GLOBAL_INCLUDES += .

$(NAME)_SOURCES := wac.c

ifeq ($(BUILD_TYPE),debug)
$(NAME)_DEFINES := DEBUG
endif

$(NAME)_ALWAYS_OPTIMISE := 1

GLOBAL_DEFINES += AUTO_IP_ENABLED

$(NAME)_COMPONENTS += daemons/Gedday \
                      mfi/apple_device_ie \
                      mfi/mfi_sap \
                      daemons/HTTP_server
