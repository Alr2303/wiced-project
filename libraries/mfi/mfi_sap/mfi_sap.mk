#
# $ Copyright Broadcom Corporation $
#

NAME := Lib_MFi_SAP

GLOBAL_INCLUDES  += .
$(NAME)_INCLUDES += mfi/iauth_chip

$(NAME)_COMPONENTS += mfi/iauth_chip

$(NAME)_SOURCES := MFiSAP-WICED.c \
				   wiced_mfi_sap_adapter.c \
				   wiced_mfi.c


$(NAME)_ALWAYS_OPTIMISE := 1

