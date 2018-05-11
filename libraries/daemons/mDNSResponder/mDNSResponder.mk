#
# $ Copyright Broadcom Corporation $
#

NAME := Lib_mDNSResponder

GLOBAL_INCLUDES += .

$(NAME)_SOURCES := mDNSCore/DNSCommon.c \
                   mDNSCore/DNSDigest.c \
                   mDNSCore/mDNS.c \
                   mDNSCore/uDNS.c \
                   mDNSPosix/mDNSPosix.c \
                   mDNSPosix/mDNSUNP.c \
                   mDNSPosix/PosixDaemon.c \
                   mDNSShared/GenLinkedList.c \
                   mDNSShared/mDNSDebug.c \
                   mDNSShared/PlatformCommon.c \
                   mDNSShared/uds_daemon.c

 # libdns_sd client library
$(NAME)_SOURCES += mDNSShared/dnssd_clientlib.c \
                   mDNSShared/dnssd_clientstub.c \
                   mDNSShared/dnssd_ipc.c

$(NAME)_CFLAGS = -Wno-array-bounds -Wno-pointer-sign -Wno-unused -Wno-unused-parameter -Werror=implicit-function-declaration

$(NAME)_INCLUDES := mDNSCore \
                    mDNSShared

$(NAME)_DEFINES += _GNU_SOURCE NOT_HAVE_SA_LEN USE_TCP_LOOPBACK MDNS_DEBUGMSGS=0 PLATFORM_NO_RLIMIT APPLE_OSX_mDNSResponder=0 PID_FILE=\"/var/run/mdnsd.pid\" MDNS_UDS_SERVERPATH=\"/var/run/mdnsd\"
                   