#
# $ Copyright Broadcom Corporation $
#

NAME := Lib_cJSON

GLOBAL_INCLUDES := .

$(NAME)_SOURCES := cJSON.c

$(NAME)_CFLAGS := -Wno-error=char-subscripts