#
# $ Copyright Broadcom Corporation $
#

NAME := Lib_airplay_server

GLOBAL_INCLUDES += .

$(NAME)_SOURCES := 
$(NAME)_INCLUDES := support \
                    platform \
                    libraries/mfi/mfi_sap

AIRPLAY_LIBRARY_NAME :=airplay.$(RTOS).$(NETWORK).$(HOST_ARCH).release.a

ifneq ($(wildcard $(CURDIR)$(AIRPLAY_LIBRARY_NAME)),)
$(NAME)_PREBUILT_LIBRARY := $(AIRPLAY_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)airplay_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(AIRPLAY_LIBRARY_NAME)),)

$(NAME)_SOURCES += platform/airplay_audio_platform_interface.c
$(NAME)_SOURCES += platform/airplay_time_platform_interface.c
