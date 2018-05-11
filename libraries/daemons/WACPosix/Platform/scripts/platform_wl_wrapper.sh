#!/bin/sh

# The name of the softap interface below and
# the one in udhcpd.softap.conf needs to stay in sync !
IFACE_SOFTAP="wlan0"

# That's the name of the sta interface
IFACE_STA_LIST="wlan0"

# Name of the DHCP client available on the platform
# Either udhcpc or dhcpcd
DHCP_CLIENT="dhcpcd"

# Period during which the DHCP client will try to obtain a lease
DHCP_CLIENT_GET_LEASE_TIMEOUT="60"

# SotfAP configuration details
SOFTAP_CHANNEL="6"
SOFTAP_SSID_PREFIX="BRCM_Unconfigured"
SOFTAP_IP="192.168.16.1"

#################################################################################
# DHCPCD start
# Args:
#       $1 - Interface name
#       $2 - Name of the script/executable to run at dhcp events
plat_dhcpcd_start() {
    iface=$1
    run_script=$2

    echo "plat_dhcpcd_start: ${iface} ${run_script}"

# Assuming dhcpcd was not running on a specific iface
# and launch it for that specific iface.
# Note that dhcpcd might be already lauched in "master" mode,
# and already working on all available interfaces,
# however, we're *not* handling that situation.

# Drop DHCP lease (if any)
    dhcpcd -k $iface
    echo "plat_dhcpdc_start: dropping current lease..."
    sleep 1
# Renew DHCP lease
    dhcpcd -w -n -t "${DHCP_CLIENT_GET_LEASE_TIMEOUT}" "${iface}"
    echo "plat_dhcpcd_start: renewing lease..."
}

#################################################################################
# UDHCPC start
# Args:
#       $1 - Interface name
#       $2 - Name of the script/executable to run at dhcp events
plat_udhcpc_start() {
    iface=$1
    run_script=$2

    echo "plat_udhcpc_start: ${iface} ${run_script}"

    pid_file="/var/run/udhcpc-${iface}.pid"
    pid_of_udhcpc=""
# Look for PID file in /var/run
    if [ -r "$pid_file" ]; then
	pid_of_udhcpc=$(cat ${pid_file})
	echo "plat_udhcpc_start: found PID=${pid_of_udhcpc} in ${pid_file}"
    fi
# Look for PID in process table
    if [ -z "$pid_of_udhcpc" ]; then
	pid_of_udhcpc=$(ps axw | grep udhcpc | grep ${iface} | cut -d ' ' -f 3)
	echo "plat_udhcpc_start: found PID=${pid_of_udhcpc} in process table"
    fi

# Bring interface up
    ifconfig "${iface}" up
    sleep 1

# Assuming udhcpc was not running; launch it
    if [ -z "$pid_of_udhcpc" ]; then
	if [ -z "$run_script" ]; then
	    run_script="/tmp/ldhclnt"
	fi
	udhcpc -i "${iface} -p "${pid_file}" -s "${run_script}
	echo "plat_udhcpc_start: udhcpc -i ${iface} -p ${pid_file} -s ${run_script}"
    else
# Drop DHCP lease (if any)
	kill -SIGUSR2 "${pid_of_udhcpc}"
	echo "plat_udhcpc_start: dropping current lease..."
# Renew DHCP lease
	kill -SIGUSR1 "${pid_of_udhcpc}"
	echo "plat_udhcpc_start: renewing lease..."
    fi
}

#################################################################################
# DHCP client start
# Args:
#       $1 - Interface name
#       $2 - Name of the script/executable to run at dhcp events
plat_dhcp_client_start() {
    iface=$1
    run_script=$2

    echo "plat_dhcp_client_start: ${iface} ${run_script}"

    if [ "${DHCP_CLIENT}" == "dhcpcd" ]; then
	plat_dhcpcd_start "${iface}" "${run_script}"
    else
	if [ "${DHCP_CLIENT}" == "udhcpc" ]; then
	    plat_udhcpc_start "${iface}" "${run_script}"
	else
	    echo "plat_dhcp_client_start: ${DHCP_CLIENT} is not supported !"
	    return 1
	fi
    fi
}

#################################################################################
# Link-local IP assignment (for WiFi interface)
# Args:
#       $1 - WiFi Interface name
plat_assign_link_local_ip() {
    iface=$1

    echo "plat_assign_link_local_ip: ${iface}"

    iface_mac=$(wl -i "${iface}" cur_etheraddr | cut -d ' ' -f 2)
    iface_mac_5=$(echo "${iface_mac}" | cut -d ":" -f 5)
    iface_mac_6=$(echo "${iface_mac}" | cut -d ":" -f 6)
    iface_mac_5_dec=$(printf "%d" 0x${iface_mac_5})
    iface_mac_6_dec=$(printf "%d" 0x${iface_mac_6})
    if [ "$iface_mac_5_dec" -lt "2" ]; then
	iface_mac_5_dec=`expr $iface_mac_5_dec + 237`
    elif [ "$iface_mac_5_dec" -gt "253" ]; then
	iface_mac_5_dec=`expr $iface_mac_5_dec - 137`
    fi
    ip_addr="169.254.${iface_mac_5_dec}.${iface_mac_6_dec}"

    ifconfig "${iface}" "${ip_addr}"

    echo "plat_assign_link_local_ip: assigned ${ip_addr} to ${iface}."
}

#################################################################################
# Joins given SSID with PSK (if any) and interface name
# Args:
#       $1 - Interface name
#       $2 - Check for existing association with $3 if "true"
#       $3 - SSID
#       $4 - PSK may or may not be supplied
plat_join_ssid() {
    iface=$1
    b_check_for_assoc=$2
    orig_ssid=$3
    orig_psk=$4

    echo "plat_join_ssid: calling args [${iface}] [${b_check_for_assoc}] [${orig_ssid}] [${orig_psk}]"

# Do we have an interface to use?
    if [ "$iface" == "" ]; then
	echo "plat_join_ssid: Interface Name is an empty string; bailing out !"
	return 1
    fi
# Do we have an SSID to join?
    if [ "$orig_ssid" == "" ]; then
	echo "plat_join_ssid: SSID is an empty string; bailing out !"
	return 1
    fi

# Check whether or not we're already associated
    if [ "$b_check_for_assoc" == "true" ]; then
	curr_ssid=$(wl -i "${iface}" ssid | cut -d ':' -f 2 | cut -d '"' -f 2)
	if [ "$curr_ssid" == "$orig_ssid" ]; then
	    echo "plat_join_ssid: already associated with $orig_ssid !"
	    return 0
	fi
    fi

# Bring interface down
    ifconfig "${iface}" 0.0.0.0
    ifconfig "${iface}" down
# Make sure we're not in AP mode
    wl -i "${iface}" down
    wl -i "${iface}" apsta 0
    wl -i "${iface}" ap 0
    wl -i "${iface}" band auto
    wl -i "${iface}" up
# Make sure interface is actually up
    wl_isup=$(wl -i "${iface}" isup)
    test_count=0
    while [ "$wl_isup" == "0" ] && [ "$test_count" != "10" ]; do
	sleep 1
	wl_isup=$(wl -i "${iface}" isup)
	echo "plat_join_ssid: try ${test_count}; wl isup returns ${wl_isup}"
	test_count=`expr $test_count + 1`
    done

# Scanning for SSID / AP
    echo "plat_join_ssid: scanning for ${orig_ssid} . . ."
    wl -i "${iface}" scan "${orig_ssid}"

# Wait for results
    wl_scanresults=""
    scanned_ssid=""
    test_count=0
    until [ "$scanned_ssid" == "$orig_ssid" ] || [ "$test_count" == "5" ]; do
	sleep 2
	wl_scanresults=$(wl -i "$iface" scanresults)
	scanned_ssid=$(echo "${wl_scanresults}" | grep -m 1 "SSID: \"" | cut -d ':' -f 2 | cut -d '"' -f 2)
	test_count=`expr $test_count + 1`
    done

# Let's verify whether or not scanning was successful
    if [ "$scanned_ssid" != "$orig_ssid" ]; then
# Scan failed; AP is down?
	echo "plat_join_ssid: scan for ${orig_ssid} returns nothing !"
	return 1
    else
	echo "plat_join_ssid: scan for ${orig_ssid} was successful !"
    fi

# Parse auth mode and encryption scheme
    orig_amode=$(echo "${wl_scanresults}" | grep -m 1 "AKM Suites" | cut -d ':' -f 2 | cut -d ' ' -f 2)
    orig_wsec=$(echo "${wl_scanresults}" | grep -m 1 "ciphers" | cut -d ':' -f 2 | cut -d ' ' -f 2)

    echo "plat_join_ssid: parsed AKM=${orig_amode} ciphers=${orig_wsec}"

# Do we have a PSK?
    if [ "$orig_psk" == "" ] || [ "$orig_amode" == "" ]; then
# No, we don't have a PSK; assuming open link
	curr_ssid=$(wl -i "${iface}" ssid | cut -d ':' -f 2 | cut -d '"' -f 2)
	test_count=0
# Limit wl join attempts in case AP is now offline
	while [ "$curr_ssid" != "$orig_ssid" ] && [ "$test_count" != "10" ]; do
	    echo "plat_join_ssid: joining ${orig_ssid} . . ."
	    sleep 2
	    wl -i "${iface}" join "${orig_ssid}" imode bss amode open prescanned
	    sleep 2
	    wl_status=$(wl -i "${iface}" status)
	    curr_ssid=$(echo "${wl_status}" | grep -m 1 "SSID: \"" | cut -d ':' -f 2 | cut -d '"' -f 2)
	    echo "plat_join_ssid: . . . current SSID is [${curr_ssid}]."
	    test_count=`expr $test_count + 1`
	done

	if [ "$curr_ssid" != "$orig_ssid" ]; then
# Join failed; AP is down?
	    echo "plat_join_ssid: joining ${orig_ssid} failed !"
	    return 1
	fi

    else
# Yes, we have a PSK, we only support WPA PSK or WPA2 PSK

	case $orig_wsec in
	    AES*) wl_wsec=4 ;;
	    TKIP*) wl_wsec=2 ;;
	    *)
	    echo "Unsupported encryption scheme: ${orig_wsec} !"
	    return 1
	    ;;
	esac

	case $orig_amode in
	    WPA-PSK)
	    if [ "$wl_wsec" == "4" ]; then
		wl_amode="wpa2psk"
		wl_wpa_auth=128
	    else
		wl_amode="wpapsk"
		wl_wpa_auth=4
	    fi
	    ;;
            WPA2-PSK)
	    wl_amode="wpa2psk"
	    wl_wpa_auth=128
	    ;;
            *)
	    echo "Unsupported authorization scheme: ${orig_amode} !"
	    return 1
	    ;;
	esac

	echo "plat_join_ssid: wsec=${wl_wsec} wpa_auth=${wl_wpa_auth}"

	wl -i "${iface}" sup_wpa 1
	wl -i "${iface}" wsec $wl_wsec
	wl -i "${iface}" wpa_auth $wl_wpa_auth
	wl -i "${iface}" set_pmk "${orig_psk}"

	curr_ssid=$(wl -i "${iface}" ssid | cut -d ':' -f 2 | cut -d '"' -f 2)
	test_count=0
# Limit wl join attempts in case AP is now offline
	while [ "$curr_ssid" != "$orig_ssid" ] && [ "$test_count" != "10" ]; do
	    echo "plat_join_ssid: joining ${orig_ssid} . . ."
	    sleep 2
	    wl -i "${iface}" join "${orig_ssid}" imode bss amode "${wl_amode}" prescanned
	    sleep 2
	    wl_status=$(wl -i "${iface}" status)
	    curr_ssid=$(echo "${wl_status}" | grep -m 1 "SSID: \"" | cut -d ':' -f 2 | cut -d '"' -f 2)
	    echo "plat_join_ssid: . . . current SSID is [${curr_ssid}]."
	    test_count=`expr $test_count + 1`
	done
	
	if [ "$curr_ssid" != "$orig_ssid" ]; then
# Join failed; AP is down?
	    echo "plat_join_ssid: joining ${orig_ssid} failed !"
	    return 1
	fi

    fi

# Let's make sure we have an IP for the interface
    plat_dhcp_client_start "${iface}"
    ip_addr=$(ifconfig "${iface}" | grep "inet addr" | cut -d ':' -f 2 | cut -d ' ' -f 1)
    test_count=0
    while [ "$ip_addr" == "" ] && [ "$test_count" != "10" ]; do
	echo "plat_join_ssid: waiting for IP addr on ${iface} . . ."
	sleep 2
	ip_addr=$(ifconfig "${iface}" | grep "inet addr" | cut -d ':' -f 2 | cut -d ' ' -f 1)
	test_count=`expr $test_count + 1`
    done
# Let's use link-local if we didn't get an address from udhcpc
    if [ "$ip_addr" == "" ] && [ "$DHCP_CLIENT" == "udhcpc" ]; then
	echo "plat_join_ssid: still no IP; let's assign a  link-local IP..."
	plat_assign_link_local_ip "${iface}"
    fi

# Success
    return 0
}

#################################################################################
# Tries to join given SSID with PSK (if any)
# Goes through an hard-coded list of interfaces
# Args:
#       $1 - Check for existing association with $2 if "true"
#       $2 - SSID
#       $3 - PSK may or may not be supplied
plat_try_join_wifi() {
    iface_list=$IFACE_STA_LIST
    b_check_for_assoc=$1
    orig_ssid=$2
    orig_psk=$3

    echo "plat_try_join_wifi: calling args [${b_check_for_assoc}] [${orig_ssid}] [${orig_psk}]"

    success_count=0
    join_result=1

    for iface in $iface_list
      do
      if [ "$success_count" == "0" ]; then
	  plat_join_ssid "${iface}" $b_check_for_assoc "${orig_ssid}" "${orig_psk}"
	  join_result="$?"
      else
	  join_result=1
      fi
      if [ "$join_result" == "0" ]; then
	  echo "plat_try_join_wifi: successfully joined ${orig_ssid} on interface ${iface} !"
	  success_count=`expr $success_count + 1`
      else
	  curr_ssid=$(wl -i "${iface}" ssid | cut -d ':' -f 2 | cut -d '"' -f 2)
	  if [ "$curr_ssid" == "" ]; then
	      echo "plat_try_join_wifi: issuing \"ifconfig ${iface} down\" since it's not associated with any SSID"
	      ifconfig "${iface}" down
	  fi
      fi
    done

    if [ "$success_count" == "0" ]; then 
	echo "plat_try_join_wifi: failed to join ${orig_ssid} !"
	return 1
    fi
# Success
    return 0
}

#################################################################################
# Sets provided IE data on specified interface
# Args:
#       $1 - Interface name
#       $2 - IE length (OUI + following payload)
#       $3 - OUI
#       $4 - payload (hexadecimal)
plat_add_ie() {
    iface=$1
    ie_length=$2
    ie_OUI=$3
    ie_payload=$4

    if [ "$ie_length" != "" ]; then
	wl -i "${iface}" add_ie 3 "${ie_length}" "${ie_OUI}" "${ie_payload}"
	return 0
    fi

    return 1
}

#################################################################################
# DHCP server start
# Args: 
#       $1 - Interface Name
#       $2 - IP address
#       $3 - Config file
plat_dhcp_server_start() {
    iface=$1
    ip_addr=$2
    udhcpd_conf_file=$3

    echo "plat_dhcp_server_start: $iface $ip_addr $udhcpd_conf_file"

    ifconfig $iface $ip_addr up
# make sure the interface is up
    ifcfg=$(ifconfig "${iface}" | grep "inet addr" | cut -d ':' -f 2 | cut -d ' ' -f 1)
    if [ "$ifcfg" == "$ip_addr" ]; then
	echo "interface ${iface} ${ip_addr} is up!"
	pid_file="/var/run/udhcpd-${iface}.pid"
	udhcpd_pid=""
	if [ -r "$pid_file" ]; then
	    udhcpd_pid=$(cat ${pid_file})
	fi
	if [ -z "$udhcpd_pid" ]; then
# for /var/lib/misc/udhcpd-${iface}.leases 
	    mkdir -p /var/lib/misc
	    echo "plat_dhcp_server_start: udhcpd ${udhcpd_conf_file}"
	    udhcpd $udhcpd_conf_file
	else
	    echo "plat_dhcp_server_start: udhcpd already running at PID=${udhcpd_pid}"
	fi
    fi
}

#################################################################################
# DHCP server stop
# Args:
#       $1 - Interface Name
plat_dhcp_server_stop() {
    iface=$1
    
    echo "plat_dhcp_server_stop: $iface"
    
    pid_file="/var/run/udhcpd-${iface}.pid"
    udhcpd_pid=""
    if [ -r "$pid_file" ]; then
	udhcpd_pid=$(cat ${pid_file})
    fi
    if [ -n "$udhcpd_pid" ]; then
	echo "plat_dhcp_server_stop: kill PID ${udhcpd_pid}"
	kill ${udhcpd_pid}
    fi
}

#################################################################################
# Starts a soft AP with provided IE data
# Args:
#       $1 - Config file for UDHCPD
#       $2 - IE length (OUI + following payload)
#       $3 - OUI
#       $4 - payload (hexadecimal)
plat_soft_ap_start() {
    iface=$IFACE_SOFTAP
    iface_ip=$SOFTAP_IP
    udhcpd_conf_file=$1
    ie_length=$2
    ie_OUI=$3
    ie_payload=$4

    echo "plat_soft_ap_start: on ${iface}, ie_length=${ie_length} ie_OUI=${ie_OUI}"
    echo "plat_soft_ap_start: ie_payload=${ie_payload}"
    echo "plat_soft_ap_start: udhcpd_conf_file=${udhcpd_conf_file}"

    wl -i "${iface}" disassoc
    wl -i "${iface}" wsec 0
    ifconfig "${iface}" 0.0.0.0
    ifconfig "${iface}" down
    sleep 1
    wl -i "${iface}" down
    sleep 1
    wl -i "${iface}" apsta 0
    wl -i "${iface}" ap 1
    wl -i "${iface}" channel $SOFTAP_CHANNEL
    wl -i "${iface}" up
# Make sure interface is actually up
    wl_isup=$(wl -i "${iface}" isup)
    test_count=0
    while [ "$wl_isup" == "0" ] && [ "$test_count" != "10" ]; do
	sleep 1
	wl_isup=$(wl -i "${iface}" isup)
	echo "plat_soft_ap_start: try ${test_count}; wl isup returns ${wl_isup}"
	test_count=`expr $test_count + 1`
    done

    plat_add_ie $iface $ie_length $ie_OUI $ie_payload

    iface_mac=$(wl -i "${iface}" cur_etheraddr | cut -d ' ' -f 2)
    iface_mac_4=$(echo "${iface_mac}" | cut -d ":" -f 4)
    iface_mac_5=$(echo "${iface_mac}" | cut -d ":" -f 5)
    iface_mac_6=$(echo "${iface_mac}" | cut -d ":" -f 6)
    ap_ssid=$(echo "${SOFTAP_SSID_PREFIX}_${iface_mac_4}${iface_mac_5}${iface_mac_6}")

    wl -i "${iface}" ssid "${ap_ssid}"

    plat_dhcp_server_start "${iface}" ${iface_ip} "${udhcpd_conf_file}"
}

#################################################################################
# Stops soft AP
# Args: none
plat_soft_ap_stop() {
    iface=$IFACE_SOFTAP

    sta_list=$(wl -i "${iface}" authe_sta_list)
    test_count=0
    while [ "$sta_list" != "" ] && [ "$test_count" != "10" ]; do
	sleep 1
	sta_list=$(wl -i "${iface}" authe_sta_list)
	echo "plat_soft_ap_stop: try ${test_count}; wl authe_sta_list returns ${sta_list}"
	test_count=`expr $test_count + 1`
    done

    plat_dhcp_server_stop "${iface}"
    wl -i "${iface}" down
    wl -i "${iface}" apsta 0
    wl -i "${iface}" ap 0
    wl -i "${iface}" up
    ifconfig "${iface}" 0.0.0.0
    ifconfig "${iface}" down
}

