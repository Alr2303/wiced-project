#
#	Copyright (C) 2012-2013 Apple Inc. All Rights Reserved.
#
#	Build Targets
#	-------------
#	airplayd	-- 	Build AirPlay executable
#	airplaylib	-- 	Build AirPlay Library
#	clean		-- 	Clean AirPlay 
#	help		-- 	Help info	[default]
#
#
#	Build options
#	-------------
#	openssl		-- 	1=Use OpenSSL for AES, SHA*, etc. 
#					0=Compile in AES, SHA*, etc. code directly.
#
#	debug       -- 	1=Compile in debug code, asserts, etc. 
#					0=Strip out debug code for a release build.
#

ifneq ($(platform_makefile), )
include $(platform_makefile)
endif

##################################################################
#
# Platform independent stuff
#
##################################################################


#
# VPaths
#
VPATH			+= $(SRCROOT)/External/AppleLossless
VPATH			+= $(SRCROOT)/External/Curve25519
VPATH			+= $(SRCROOT)/External/Ed25519
ifneq ($(openssl),1)
VPATH			+= $(SRCROOT)/External/GladmanAES
endif
VPATH			+= $(SRCROOT)/Sources
VPATH			+= $(SRCROOT)/Support

# Append vpaths from platform
VPATH		+= $(PLATFORM_VPATH)


#
# Include Paths
#
INCLUDES		+= -I$(SRCROOT)/External
INCLUDES		+= -I$(SRCROOT)/External/AppleLossless
INCLUDES		+= -I$(SRCROOT)/External/Curve25519
ifneq ($(openssl),1)
INCLUDES		+= -I$(SRCROOT)/External/GladmanAES
endif
INCLUDES		+= -I$(SRCROOT)/Sources
INCLUDES		+= -I$(SRCROOT)/Support

# Append includes from platform
INCLUDES		+= $(PLATFORM_INCLUDES)


#
# Object File List
#

# AirPlay
OBJS			+= AirPlayReceiverPOSIX.o
OBJS			+= AirPlayReceiverServer.o
OBJS			+= AirPlayReceiverSession.o
OBJS			+= AirPlaySettings.o
OBJS			+= AirPlayUtils.o
OBJS			+= AirTunesClock.o
OBJS			+= AirTunesDACP.o
OBJS			+= AirTunesServer.o

# Apple Lossless
OBJS			+= ag_dec.o
OBJS			+= AppleLosslessDecoder.o
OBJS			+= BitUtilities.o
OBJS			+= dp_dec.o
OBJS			+= matrix_dec.o

ifneq ($(openssl),1)
# GladmanAES
OBJS			+= aes_modes.o
OBJS			+= aescrypt.o
OBJS			+= aeskey.o
OBJS			+= aestab.o
OBJS			+= gcm.o
OBJS			+= gf128mul.o
endif

# Support
OBJS			+= AESUtils.o
OBJS			+= AsyncConnection.o
OBJS			+= AudioConverterLite.o
OBJS			+= AudioUtils.o
OBJS			+= Base64Utils.o
OBJS			+= CFCompat.o
OBJS			+= CFLite.o
OBJS			+= CFLiteBinaryPlist.o
OBJS			+= CFLitePreferencesFile.o
OBJS			+= CFLiteRunLoopSelect.o
OBJS			+= CFUtils.o
OBJS			+= CommandLineUtils.o
OBJS			+= curve25519-donna.o
OBJS			+= DataBufferUtils.o
OBJS			+= DebugIPCUtils.o
OBJS			+= DebugServices.o
OBJS			+= DispatchLite.o
OBJS			+= DMAP.o
OBJS			+= DMAPUtils.o
OBJS			+= HTTPClient.o
OBJS			+= HTTPMessage.o
OBJS			+= HTTPNetUtils.o
OBJS			+= HTTPServerCPP.o
OBJS			+= HTTPUtils.o
OBJS			+= IEEE80211Utils.o
OBJS			+= MathUtils.o
OBJS			+= MD5Utils.o
OBJS			+= MFiSAP.o
OBJS			+= MiscUtils.o
OBJS			+= NetPerf.o
OBJS			+= NetUtils.o
OBJS			+= NTPUtils.o
OBJS			+= PIDUtils.o
OBJS			+= RandomNumberUtils.o
OBJS			+= SDPUtils.o
OBJS			+= SHAUtils.o
OBJS			+= StringUtils.o
OBJS			+= SystemUtils.o
OBJS			+= ThreadUtils.o
OBJS			+= TickUtils.o
OBJS			+= TimeUtils.o
OBJS			+= URLUtils.o
OBJS			+= utfconv.o
OBJS			+= UUIDUtils.o

# Add Platform Objs
OBJS			+= $(PLATFORM_OBJS)

# Link flags 
LINKFLAGS		+= 

# Link flags - add platform specified library paths 
LINKFLAGS		+= $(PLATFORM_LIBRARY_PATHS)


# Libraries 
LIBS			+= 

# Libraries - Add platform specified libraries
LIBS			+= $(PLATFORM_LIBS)


# Flags
COMMONFLAGS		+= -D__STDC_LIMIT_MACROS=1
COMMONFLAGS		+= -D_GNU_SOURCE
# Maybe the platform should decide whether or not it needs ALAC ?
# Why is this turned off by default anyway ?
#COMMONFLAGS		+= -DAIRPLAY_ALAC=0
#COMMONFLAGS		+= -DAUDIO_CONVERTER_ALAC=0
COMMONFLAGS		+= -DAIRPLAY_MFI=1
COMMONFLAGS		+= -DCFCOMPAT_NOTIFICATIONS_ENABLED=0
COMMONFLAGS		+= -DCFL_BINARY_PLISTS=1
COMMONFLAGS		+= -DCFL_XML=0
COMMONFLAGS		+= -DCFLITE_ENABLED=1
COMMONFLAGS		+= -DDEBUG_CF_OBJECTS_ENABLED=1
COMMONFLAGS		+= -DDEBUG_EXPORT_ERROR_STRINGS=1

ifeq ($(debug),1)
COMMONFLAGS		+= -DDEBUG=1 -g
else
COMMONFLAGS		+= -DNDEBUG -Os
endif

ifneq ($(openssl),1)
COMMONFLAGS		+= -DAES_UTILS_HAS_GLADMAN_GCM=1
COMMONFLAGS		+= -DAES_UTILS_USE_GLADMAN_AES=1
COMMONFLAGS		+= -DTARGET_HAS_MD5_UTILS=1
COMMONFLAGS		+= -DTARGET_HAS_SHA_UTILS=1
COMMONFLAGS		+= -DTARGET_NO_OPENSSL=1
COMMONFLAGS		+= -DUSE_VIA_ACE_IF_PRESENT=0
endif


COMMONFLAGS		+= -DAIRPLAY_AUDIO_INPUT=0
COMMONFLAGS		+= -DAIRPLAY_EASY_CONFIG=1
COMMONFLAGS		+= -DAIRPLAY_WEB_SERVER=1


# Add Platform header file(s) to include
COMMONFLAGS		+= $(PLATFORM_HEADERS)


COMMON_WARNINGS	+= -W
COMMON_WARNINGS	+= -Wall
# Let's not stop the show because of warnings
# COMMON_WARNINGS	+= -Werror
COMMON_WARNINGS	+= -Wextra
COMMON_WARNINGS	+= -Wformat
COMMON_WARNINGS	+= -Wmissing-braces
COMMON_WARNINGS	+= -Wno-cast-align
COMMON_WARNINGS	+= -Wparentheses
# C header "net/if.h" will trigger -Wshadow because
# of struct if_nameindex and function if_nameindex() sharing the same name
COMMON_WARNINGS	+= -Wshadow
COMMON_WARNINGS	+= -Wsign-compare
COMMON_WARNINGS	+= -Wswitch
COMMON_WARNINGS	+= -Wuninitialized
COMMON_WARNINGS	+= -Wunknown-pragmas
COMMON_WARNINGS	+= -Wunused-function
COMMON_WARNINGS	+= -Wunused-label
COMMON_WARNINGS	+= -Wunused-parameter
COMMON_WARNINGS	+= -Wno-unused-result
COMMON_WARNINGS	+= -Wunused-value
COMMON_WARNINGS	+= -Wunused-variable
CWARNS			+= $(COMMON_WARNINGS)
CWARNS			+= -Wmissing-prototypes
CWARNS			+= -fgnu89-inline # Workaround for buggy system headers.
CXXWARNS		+= $(COMMON_WARNINGS)
CXXWARNS		+= -Wnon-virtual-dtor
CXXWARNS		+= -Woverloaded-virtual


# Archive flags to create airplay library
# Use platform ar flag if set by platform
ARFLAGS			?= -rv
ifneq ($(PLATFORM_ARFLAGS), )
ARFLAGS			= $(PLATFORM_ARFLAGS)
endif


# Target specific flags
airplaylib: COMMONFLAGS 	+= -DAIRPLAY_THREADED_MAIN



CFLAGS			+= $(INCLUDES) $(COMMONFLAGS) $(CWARNS)
CFLAGS			+= -std=gnu99
CXXFLAGS		+= $(INCLUDES) $(COMMONFLAGS) $(CXXWARNS)


#
# Rules
#
.PHONY : all
all: help


#
# Target to build airplay daemon.
#
airplayd: check $(OBJS) AirPlayMain.o 
	$(CC) $(LINKFLAGS) -o $@ $(filter-out check,$^) $(LIBS)
ifneq ($(debug), 1)
	@echo "------>"
	@echo "------> Stripping"
	@echo "------>"
	$(STRIP) $@
endif
	@echo "--------------"
	@echo "BUILD COMPLETE: $@"
	@echo "--------------"


#
# Target to build airplay library.
#
airplaylib: check $(OBJS) AirPlayMain.o
	@echo "======> AR=$(AR)"
	@echo "======> ARFLAGS=$(ARFLAGS)"
	@echo "======> PLATFORM_ARFLAGS=$(PLATFORM_ARFLAGS)"
	$(AR) $(ARFLAGS) $@.a $(filter-out check,$^)
ifneq ($(debug), 1)
	@echo "------>"
	@echo "------> Stripping"
	@echo "------>"
	$(STRIP) $@.a
endif
	@echo "--------------"
	@echo "BUILD COMPLETE: $@.a"
	@echo "--------------"


#
# Target to build airplayutil.
#
airplayutil: check $(OBJS) airplayutil.o
	$(CC) $(LINKFLAGS) -o $@ $(filter-out check,$^) $(LIBS)
ifneq ($(debug), 1)
	@echo "------>"
	@echo "------> Stripping"
	@echo "------>"
	$(STRIP) $@
endif
	@echo "--------------"
	@echo "BUILD COMPLETE: $@"
	@echo "--------------"


#
# Target to cleanup 
#
.PHONY : clean
clean: 
	-rm -rf *.o $(OBJS) airplayd AirPlayMain.o airplayutil.o airplayutil airplaylib.a


#
# Target to check that the caller of this makefile has passed in the needed information
#
.PHONY: check
check:
ifeq ($(SRCROOT), )
	# Stop and print error - caller has not specified the required makefile variables.
	$(error Error: Caller needs to specify necessary makefile variables. See example, Platform/PlatformMakefileExample.)
endif
# ifeq ($(platform_makefile),)
#	Stop and print error if the caller has not included the platform specific makefile
#	$(error Caller needs to specify platform specific include makefile. See example, Platform/PlatformMakefileExample. Usage: platform_makefile=<path>/<makefile>)
# endif


#
# Target to print out help info
#
.PHONY : help
help:
	@echo "--------- AirPlay Make Help -----------"
	@echo " To build debug mode AirPlay Executable:"
	@echo " 	make  platform_makefile=<<path_to_Patform.Include.mk> debug=1 airplayd"
	@echo 
	@echo " To build non-debug mode AirPlay Library:"
	@echo " 	make  platform_makefile=<<path_to_Patform.Include.mk> airplaylib"
	@echo 
	@echo " To clean AirPlay:"
	@echo " 	make  clean"
	@echo "---------------------------------------"

