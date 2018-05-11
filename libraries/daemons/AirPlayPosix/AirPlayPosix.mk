#
# $ Copyright Broadcom Corporation $
#

NAME := Lib_AirPlayPosix

mDNSResponder_DIR = ../mDNSResponder

GLOBAL_INCLUDES += .

# AirPlay
$(NAME)_SOURCES := Sources/AirPlayMain.c \
                   Sources/AirPlayReceiverPOSIX.c \
                   Sources/AirPlayReceiverServer.c \
                   Sources/AirPlayReceiverSession.c \
                   Sources/AirPlaySettings.c \
                   Sources/AirPlayUtils.c \
                   Sources/AirTunesClock.c \
                   Sources/AirTunesDACP.c \
                   Sources/AirTunesServer.cpp

# Apple Lossless
$(NAME)_SOURCES += External/AppleLossless/ag_dec.c \
                   External/AppleLossless/AppleLosslessDecoder.c \
                   External/AppleLossless/BitUtilities.c \
                   External/AppleLossless/dp_dec.c \
                   External/AppleLossless/matrix_dec.c

# GladmanAES
$(NAME)_SOURCES += External/GladmanAES/aes_modes.c \
                   External/GladmanAES/aescrypt.c \
                   External/GladmanAES/aeskey.c \
                   External/GladmanAES/aestab.c \
                   External/GladmanAES/gcm.c \
                   External/GladmanAES/gf128mul.c

# Support
$(NAME)_SOURCES += Support/AESUtils.c \
                   Support/AsyncConnection.c \
                   Support/AudioConverterLite.c \
                   Support/AudioUtils.c \
                   Support/Base64Utils.c \
                   Support/CFCompat.c \
                   Support/CFLite.c \
                   Support/CFLiteBinaryPlist.c \
                   Support/CFLitePreferencesFile.c \
                   Support/CFLiteRunLoopSelect.c \
                   Support/CFUtils.c \
                   Support/CommandLineUtils.c \
                   Support/DataBufferUtils.c \
                   Support/DebugIPCUtils.c \
                   Support/DebugServices.c \
                   Support/DispatchLite.c \
                   Support/DMAP.c \
                   Support/DMAPUtils.c \
                   Support/HTTPClient.c \
                   Support/HTTPMessage.c \
                   Support/HTTPNetUtils.c \
                   Support/HTTPServerCPP.cpp \
                   Support/HTTPUtils.c \
                   Support/IEEE80211Utils.c \
                   Support/MathUtils.c \
                   Support/MD5Utils.c \
                   Support/MiscUtils.c \
                   Support/NetPerf.c \
                   Support/NetUtils.c \
                   Support/NTPUtils.c \
                   Support/PIDUtils.c \
                   Support/RandomNumberUtils.c \
                   Support/SDPUtils.c \
                   Support/SHAUtils.c \
                   Support/StringUtils.c \
                   Support/SystemUtils.c \
                   Support/ThreadUtils.c \
                   Support/TickUtils.c \
                   Support/TimeUtils.c \
                   Support/URLUtils.c \
                   Support/utfconv.c \
                   Support/UUIDUtils.c

#                  Support/MFiSAP.c \

# curve25519
$(NAME)_SOURCES += External/Curve25519/curve25519-donna.c \

# Platform
$(NAME)_SOURCES += Platform/PlatformNuttX/NuttXPlatformImplementation.c \
                   Platform/PlatformNuttX/AudioUtils.c \
                   Platform/PlatformNuttX/PlatformDACPClient.c

$(NAME)_CXXFLAGS = -Wno-cast-align -Wno-unused-result -Wnon-virtual-dtor
$(NAME)_CFLAGS   = -Wno-cast-align -Wno-unused-result -std=gnu99

$(NAME)_INCLUDES := Sources \
                    Support \
                    External/AppleLossless \
                    External/Curve25519 \
                    External/GladmanAES \
                    Platform/PlatformNuttX \
                    $(mDNSResponder_DIR)//mDNSShared

GLOBAL_DEFINES += _GNU_SOURCE AIRPLAY_ALAC=1 AUDIO_CONVERTER_ALAC=1 AIRPLAY_MFI=1 CFCOMPAT_NOTIFICATIONS_ENABLED=0 CFL_BINARY_PLISTS=1 CFL_XML=0 CFLITE_ENABLED=1 DEBUG_CF_OBJECTS_ENABLED=1 DEBUG_EXPORT_ERROR_STRINGS=1 AIRPLAY_THREADED_MAIN=1 AIRPLAY_AUDIO_INPUT=0 AIRPLAY_EASY_CONFIG=1 AIRPLAY_WEB_SERVER=1
GLOBAL_DEFINES += NETUTILS_HAVE_GETADDRINFO=0 NEW=new AIRPLAY_META_DATA=0
GLOBAL_DEFINES += AES_UTILS_HAS_GLADMAN_GCM=1 AES_UTILS_USE_GLADMAN_AES=1 TARGET_HAS_MD5_UTILS=1 TARGET_HAS_SHA_UTILS=1 TARGET_NO_OPENSSL=1 USE_VIA_ACE_IF_PRESENT=0
GLOBAL_DEFINES += CommonServices_PLATFORM_HEADER=\"PlatformCommonServices.h\"
