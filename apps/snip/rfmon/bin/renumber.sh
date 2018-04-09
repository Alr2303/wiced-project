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

#Normalize data samples from rfmon.
#Used by create_plot.sh script
# Always use the last N samples (default $NUM_LINES).
# Renumber the time (0th column) to start at 1 by subtracting the first time val from all following time values.
# Each line is usually 2 secs apart so 200 lines ~= 400 secs.

TMP_DATA=c:/Temp/last_200
NUM_LINES=200

cat $1 | head -$NUM_LINES > $TMP_DATA       # Store the first N lines
first=`cat $TMP_DATA | head -1 | awk '{print $1}'` # Extract first time val
first=$((10#$first))                           # force decimal (base 10) since data may start with leading 0 meaning octal.

# Pass $first into awk, subtract it from column 1 and print rest of columns.
cat $TMP_DATA | awk -v first="$first" \
       '{printf "%d  ", $1-first; for (i=2; i <=NF; i++) {printf "%s  ", $i} printf "\n"}'