#
# $ Copyright Broadcom Corporation $
#

NAME := Lib_WACPosix

GLOBAL_INCLUDES += .

# WAC
$(NAME)_SOURCES := Sources/WACServer.c \
                   Sources/WACBonjour.c

# External
# GladmanAES
# Duplicated code with AirPlay
#$(NAME)_SOURCES += External/GladmanAES/aes_modes.c \
#                   External/GladmanAES/aescrypt.c \
#                   External/GladmanAES/aeskey.c \
#                   External/GladmanAES/aestab.c \
#                   External/GladmanAES/gcm.c \
#                   External/GladmanAES/gf128mul.c

# curve25519
#$(NAME)_SOURCES += External/Curve25519/curve25519-donna.c \

# Support
#$(NAME)_SOURCES += Support/AESUtils.c \


$(NAME)_SOURCES += Support/AppleDeviceIE.c \
                   Support/HTTPServer.c \
                   Support/HTTPUtils.c \
                   Support/SecurityUtils.c \
                   Support/SHAUtils.c \
                   Support/TimeUtils.c \
                   Support/StringUtils.c \
                   Support/TLVUtils.c

                   
#                  Support/MFiSAPServer.c \


# Platform
$(NAME)_SOURCES += Platform/AirPlayNuttX/PlatformNuttX.c \
                   Platform/AirPlayNuttX/PlatformAirPlayWACNuttX.c

$(NAME)_INCLUDES := Sources \
                    Support \
                    External/Curve25519 \
                    External/GladmanAES \
                    Platform/ \
                    Platform/AirPlayNuttX/ \
                    $(mDNSResponder_DIR)/mDNSShared \
                    $(mDNSResponder_DIR)/mDNSPosix \
                    $(mDNSResponder_DIR)/mDNSCore \


#                    $(AirPlayPosix_DIR)/Support \

$(NAME)_DEFINES += _GNU_SOURCE DEBUG_EXPORT_ERROR_STRINGS=1 TIME_PLATFORM=1 TARGET_RT_LITTLE_ENDIAN=1 DEBUG=1
$(NAME)_DEFINES += AES_UTILS_HAS_GLADMAN_GCM=1 AES_UTILS_USE_GLADMAN_AES=1 TARGET_HAS_MD5_UTILS=1 TARGET_HAS_SHA_UTILS=1 TARGET_NO_OPENSSL=1 USE_VIA_ACE_IF_PRESENT=0
                   