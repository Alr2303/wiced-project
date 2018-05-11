#
# Copyright 2017, Cypress Semiconductor Corporation or a subsidiary of 
 # Cypress Semiconductor Corporation. All Rights Reserved.
 # This software, including source code, documentation and related
 # materials ("Software"), is owned by Cypress Semiconductor Corporation
 # or one of its subsidiaries ("Cypress") and is protected by and subject to
 # worldwide patent protection (United States and foreign),
 # United States copyright laws and international treaty provisions.
 # Therefore, you may use this Software only as provided in the license
 # agreement accompanying the software package from which you
 # obtained this Software ("EULA").
 # If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 # non-transferable license to copy, modify, and compile the Software
 # source code solely for use in connection with Cypress's
 # integrated circuit products. Any reproduction, modification, translation,
 # compilation, or representation of this Software except as specified
 # above is prohibited without the express written permission of Cypress.
 #
 # Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 # EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 # WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 # reserves the right to make changes to the Software without notice. Cypress
 # does not assume any liability arising out of the application or use of the
 # Software or any product or circuit described in the Software. Cypress does
 # not authorize its products for use in any products where a malfunction or
 # failure of the Cypress product may reasonably be expected to result in
 # significant property damage, injury or death ("High Risk Product"). By
 # including Cypress's product in a High Risk Product, the manufacturer
 # of such system or application assumes all risk of such use and in doing
 # so agrees to indemnify Cypress against all liability.
#

NAME := Lib_HomeKit_IP_Library

HOMEKIT_LIB_TYPE := ip
HOMEKIT_DIR ?= ..

ifdef USE_MFI
APPLE_HOMEKIT_LIBRARY_NAME := $(HOMEKIT_DIR)/Homekit_$(HOMEKIT_LIB_TYPE).$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a
else
APPLE_HOMEKIT_LIBRARY_NAME := $(HOMEKIT_DIR)/Homekit_$(HOMEKIT_LIB_TYPE).without_mfi.$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a
endif

ifneq ($(wildcard $(CURDIR)$(APPLE_HOMEKIT_LIBRARY_NAME)),)
$(NAME)_PREBUILT_LIBRARY := $(APPLE_HOMEKIT_LIBRARY_NAME)
else
# Build from source (internal)
include $(CURDIR)/$(HOMEKIT_DIR)/apple_homekit_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(APPLE_HOMEKIT_LIBRARY_NAME)),)

$(NAME)_COMPONENTS := utilities/JSON_parser \
                      utilities/linked_list \
                      daemons/HTTP_server

# Add the source file for the services defined here
$(NAME)_SOURCES +=  $(HOMEKIT_DIR)/apple_homekit_characteristics.c \
                    $(HOMEKIT_DIR)/apple_homekit_services.c

GLOBAL_INCLUDES += $(HOMEKIT_DIR)/.
GLOBAL_INCLUDES += $(HOMEKIT_DIR)/pairing_profile_accessory

#HomeKit must be build with this option, as it is part of the Apple Bonjour Conformance Test requirement
GLOBAL_DEFINES     += AUTO_IP_ENABLED

$(NAME)_SOURCES    += $(HOMEKIT_DIR)/homekit_gedday_host.c
$(NAME)_COMPONENTS += daemons/Gedday
