#!/bin/bash
BASEDIR=/opt/wfd/dm
MDNSRESPONDERDIR=${BASEDIR}/airp/mDNSResponder-320.10.80
TOOLCHAIN_DIR=${BASEDIR}/WS/opt/cross
CROSS_PREFIX=armv7l-meego-linux-gnueabi-
make platform_makefile=Platform/Platform.include.mk TOOLCHAIN=${TOOLCHAIN_DIR} CROSS_COMPILE=${CROSS_PREFIX} MDNSROOT=${MDNSRESPONDERDIR} debug=1 $1
