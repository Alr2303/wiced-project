#
# Local macros - not needed by AirPlay 
#
TOOLROOT		= /Volumes/AirPlayLinux
MDNSROOT		= /Users/ymustafa/projects/linux/SWrelease/mDNSResponder
PLATFORMSRCROOT	= $(SRCROOT)/Platform/PlatformLinux
CROSSBASE		= $(TOOLROOT)/x-tools/arm-unknown-linux-gnueabi
CROSSBASEREAL	= $(realpath $(CROSSBASE))
CROSSROOT		= $(CROSSBASEREAL)/arm-unknown-linux-gnueabi/sysroot
CROSSPREFIX		= $(CROSSBASEREAL)/bin/arm-unknown-linux-gnueabi-


##################################################################
#
# PLATFORM DEPENDENT MAKEFILE VARIABLES TO PASS TO AIRPLAY
#
# TO BE MODIFIED AS NEEDED BY CUSTOMER
#  |   |   |   |    |
#  |   |   |   |    |
#  V   V   V   V    V
#

##
# 1. Source root Path Information:
#
# 	SRCROOT  : Path to directory where AirPlay source release is located
#
SRCROOT=/Users/ymustafa/projects/posix/Release



##
# 2. Toolchain Information
#
# 	Information about toolset to be used for the platform being ported to
#
AR				= $(CROSSPREFIX)ar
CC				= $(CROSSPREFIX)gcc
CXX				= $(CROSSPREFIX)g++
LD				= $(CROSSPREFIX)gcc
NM				= $(CROSSPREFIX)nm
RANLIB			= $(CROSSPREFIX)ranlib
STRIP			= $(CROSSPREFIX)strip


##
# 3. Include Path(s) Information
#
# 	list of Inlcude directories for the platform being ported to:
# 			- standard header files like stdlib.h etc
# 			- dns_sd.h for Bonjour header
#
PLATFORM_INCLUDES	+= -I$(CROSSROOT)/opt/vc/include
PLATFORM_INCLUDES	+= -I$(CROSSROOT)/opt/vc/include/interface/vcos/pthreads
PLATFORM_INCLUDES	+= -I$(CROSSROOT)/opt/vc/include/interface/vmcs_host/linux
PLATFORM_INCLUDES	+= -I$(CROSSROOT)/opt/vc/src/hello_pi/libs/ilclient
PLATFORM_INCLUDES	+= -I$(PLATFORMSRCROOT)
PLATFORM_INCLUDES	+= -I$(MDNSROOT)/mDNSShared


##
# 4. Library Path(s) Information
#
# 	list of Library directories for the platform being ported to:
# 			- platform libraries listed below
# 			- libdns_sd for Bonjour library
#
PLATFORM_LIBRARY_PATHS		+= -L$(MDNSROOT)/mDNSPosix/build/prod


##
# 5. Libraries
#
# 	Libraries to be linked to. Only needed if building AirPlay as an executable
#
PLATFORM_LIBS			+= -lasound
PLATFORM_LIBS			+= -lc
ifeq ($(openssl),1)
PLATFORM_LIBS			+= -lcrypto
endif
PLATFORM_LIBS			+= -ldns_sd
PLATFORM_LIBS			+= -lm
PLATFORM_LIBS			+= -lpthread
PLATFORM_LIBS			+= -lrt
PLATFORM_LIBS			+= -lstdc++


##
# 6. Platform Source files Information
#
# 	list of platform implementation files to be compiled:
#
PLATFORM_OBJS		+= LinuxPlatformImplementation.o
PLATFORM_OBJS		+= AudioUtilsALSA.o
PLATFORM_OBJS		+= MFiServerPlatformLinux.o


##
# 7. Platform VPath Information
#
# 	path(s) to the platform source files listed above
#
PLATFORM_VPATH			+= $(PLATFORMSRCROOT)


##
# 8. Platform Headers to be hooked in
#
# 	List of Platform Header file(s) to be hooked into AirPlay during compilation
#
PLATFORM_HEADERS	+= -DCommonServices_PLATFORM_HEADER=\"PlatformCommonServices.h\"


##
# 9. Archiver Flags 
#
# 	Flags to provide to Archiver utility. Only needed if building AirPlay as a library.
#
PLATFORM_ARFLAGS			+= -rv


##
# 10. Flags/Conditionals to be enabled based on Platform requirements
#
#
PLATFORM_COMMONFLAGS		= 



#
#####################   END   ####################################

