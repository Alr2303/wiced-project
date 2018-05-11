NAME := Lib_MQTTClient-C

$(NAME)_SOURCES := src/MQTTClient.c \
	src/WICED/MQTTWICED.c

#make it visible for the applications which take advantage of this lib
GLOBAL_INCLUDES := .

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS) \
	-Wno-error=missing-prototypes -Wno-error=sign-conversion \
	-Wno-error=conversion \
	-Wno-error=unused-parameter \
	-Ilibraries/protocols/MQTTold/MQTTPacket/src \
	-DMQTTCLIENT_PLATFORM_HEADER="WICED/MQTTWICED.h"
