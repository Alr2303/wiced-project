NAME := Lib_MQTTPacket

$(NAME)_SOURCES := 	src/MQTTConnectClient.c \
			src/MQTTConnectServer.c \
			src/MQTTDeserializePublish.c \
			src/MQTTFormat.c \
			src/MQTTPacket.c \
			src/MQTTSerializePublish.c \
			src/MQTTSubscribeClient.c \
			src/MQTTSubscribeServer.c \
			src/MQTTUnsubscribeClient.c \
			src/MQTTUnsubscribeServer.c

#make it visible for the applications which take advantage of this lib
GLOBAL_INCLUDES := .


$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS) -Wno-error=sign-conversion -Wno-error=conversion -Wno-error=unused-parameter -Wno-error=missing-prototypes -Wno-error=unused-but-set-variable
