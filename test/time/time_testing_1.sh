#!/bin/sh
#
#	$Id: time_testing_1.sh,v 1.6 2007-12-20 18:22:44 remko Exp $
#
# This script runs some simple test to verify the that new time scheme
# has been implemented successfully

# Test 1:
# Get the epochs (which now decodes to a rata die number and a day fraction
# which is 0.0 unless the epoch occurs during a day) from gmt_time_system.h
# and convert to relative time using the new TIME_SYSTEM rata.  The values
# should match the new rata die + day fraction for each epoch.

. ../functions.sh
header "Test time conversions (rata die)"

sed -e 's/"//g' ../../src/gmt_time_systems.h | awk -F, '{if (NR < 8) print $2, 1, $4, $5 }' | \
	gmtconvert --TIME_SYSTEM=rata -fi0T -fo0t --D_FORMAT=%.12g | awk '{if ($1 != ($3+$4)) print $0}' > fail

passfail time_testing_1
