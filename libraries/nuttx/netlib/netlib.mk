#
# $ Copyright Broadcom Corporation $
#

NAME := Lib_netlib

$(NAME)_SOURCES := \
                   netlib.c \
                   netlib_setmacaddr.c \
                   netlib_setipv4addr.c \
                   netlib_getifstatus.c \
                   netlib_setifstatus.c \
                   netlib_ipv6netmask2prefix.c \
                   netlib_setdripv4addr.c \
                   netlib_setipv4netmask.c \
                   netlib_getmacaddr.c \
                   netlib_autoconfig.c \
                   netlib_getipv4addr.c \
                   netlib_setipv4dnsaddr.c \
                   netlib_setarp.c

GLOBAL_INCLUDES := .
