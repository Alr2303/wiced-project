#!/bin/sh

VER=`grep "^#define VERSION" apps/telesign/awair/awair.c | awk '{print $3};'`
# REV=`grep "^#define REVISION" apps/letus/awair/awair.c | awk '{print $3+0};'`
# STG=$(( (VER * 100) + REV))
DATE=`date +%y%m%d`

BLANK_COLOR='\033[0m'
GREEN_COLOR='\033[1;32m'
RED_COLOR='\033[1;31m'
YELLOW_COLOR='\033[1;33m'

set -e

clean() {
	rm -f build/*.bin
	rm -f ./*.bin
	rm -f ./*.zip
	./make clean
}

build_stg () { 
	echo "#####################################";
	echo "### build STAGING version        ###";
	echo "#####################################";

    echo $VER

	./make telesign.awair-OMNI-NetX STAGING=1 2>&1 | tee temp.log
	echo "$YELLOW_COLOR [WARNING LIST] $BLANK_COLOR"
	export GREP_COLOR="1;33"; cat temp.log | grep --color=always "warning:" || echo "$YELLOW_COLOR ========= No Warnings ========= $BLANK_COLOR"
	echo "$RED_COLOR [ERROR LIST] $BLANK_COLOR"
	export GREP_COLOR="1;31"; cat temp.log | grep --color=always "error:" || echo "$RED_COLOR ========== No Errors ========= $BLANK_COLOR"

    cp ./build/telesign.awair-OMNI-NetX/binary/telesign.awair-OMNI-NetX.bin ./omni.STG.${VER}.bin
}


build_prd () {
	echo "#####################################";
	echo "### build PRODUCT  version       ###";
	echo "#####################################";

	./make telesign.awair-OMNI-NetX 2>&1 | tee temp.log
	echo "$YELLOW_COLOR [WARNING LIST] $BLANK_COLOR"
	export GREP_COLOR="1;33"; cat temp.log | grep --color=always "warning:" || echo "$YELLOW_COLOR ========= No Warnings ========= $BLANK_COLOR"
	echo "$RED_COLOR [ERROR LIST] $BLANK_COLOR"
	export GREP_COLOR="1;31"; cat temp.log | grep --color=always "error:" || echo "$RED_COLOR ========== No Errors ========= $BLANK_COLOR"

    cp ./build/telesign.awair-OMNI-NetX/binary/telesign.awair-OMNI-NetX.bin ./omni.PRD.${VER}.bin
}



# build staging version
rm -f build/wifi.*.bin
rm -f build/wifi.*.factory




if [ "$1" = "clean" ]; then
clean
exit
fi


usage() {
	echo "Usage : ./build.sh
	 -t : choose target Staging or Production. [stg | prd]
	 -c : clean option.  (optional) [0 | 1]. default: 0"
}

while getopts ":t:c:" o; do
    case "${o}" in
        t)
            opt_target=${OPTARG}
            ;;
        c)
            opt_clean=${OPTARG}
            ;;
        *)
            usage
			exit
            ;;
    esac
done
shift $((OPTIND-1))

if [ -z "${opt_target}" ]; then
    usage
	exit
fi


if [ ${opt_target} = "stg" ]; then
	if [ ${opt_clean} = "1" ]; then
		clean
		build_stg
	else
		build_stg
	fi

elif [ ${opt_target} = 'prd' ]; then
	if [ ${opt_clean} = "1" ]; then
		clean
		build_prd
	else
    	build_prd
	fi

else
	usage
	exit
fi

