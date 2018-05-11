#!/bin/bash

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
 # so agrees to indemnify Cypress against all liability.$
# Create metadata file for gnuplot
# Used by create_plot.sh script

TMPFILE="c:/Temp/renumbered"
OUTPUT_DIR=plots

dfile=$1    # Data file
title=$2    # Title

# Create output dir if necessary
[ -d $OUTPUT_DIR ] || mkdir $OUTPUT_DIR

# Get list of peers from first line
peers=`cat $dfile | grep Time | awk '{print NF - 1}'`

# Read macaddrs into 'macs' array
IFS=' ' read -r -a macs <<< `cat $dfile | grep Time`

#First field is timestamp, renumber them so they start at 0 for a graph
#that starts at 0. (Maybe gnuplot can do this on its own?)
cat $dfile | sed 1d | sed '$d' | `dirname $0`/renumber.sh  > $TMPFILE

mean=`cat $TMPFILE | awk '{for (i=2; i <=NF; i++) tot += $i} END {printf "%1.2f\n", tot/(NR*(NF-1))}'`

>&2 echo "Mean: $mean"

#Create gnuplot metedata
echo "set term pngcairo size 1800, 480"
echo "set output \"${OUTPUT_DIR}/${dfile}.png\""
echo "set title \"$title, $peers Peers \($mean%\)\""
echo 'set ylabel "RTP PER Rate (4 PT Moving Avg)"'
echo 'set xlabel "Seconds"'
echo "set format y '%2.1f%%'"
echo 'set yrange [0:1.5]'
echo "set key font \",8\""

echo 'plot \'
for ((i = 1; i < $peers; i++))
do
    echo \"$TMPFILE\" u 1:$((i + 1)) lw 3 t \"${macs[i]}\" with lines, \\
done
#if not drawing 'sum' line below, remove comma from this line:
echo \"$TMPFILE\" u 1:$((i + 1)) lw 3 t \"${macs[i]}\" with lines, \\

#Add a line for the sum of all peers.
echo -n \"$TMPFILE\" u 1: \(\(
for ((i = 2; i < $((peers+1)); i++))
do
echo -n \$${i}+
done
echo \$${i}\)/$((peers))\) lw 1 dt 2 lc black t \"Mean of PERs\" with lines, \\

echo $mean lc black t \"Overall Mean \($mean%\)\"

