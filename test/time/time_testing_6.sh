#!/bin/sh
#
#	$Id: time_testing_6.sh,v 1.1 2007-05-28 19:40:30 pwessel Exp $
#
# This script runs some simple test to verify the that new time scheme
# has been implemented successfully

# Test 6:
# We create a grid with grdmath and then make a contour map
# and associate x with days relative to the start of 2004.

echo -n "GMT: Test time conversions, part 6 (plotting abs & rel time):		"

grdmath -R0/90/0/90 -I1 X Y ADD = $$.grd
grdcontour $$.grd -JX6t/6 -C5 -A10 --TIME_EPOCH=2004-01-01T --TIME_UNIT=d  -Bpa7Rf1d/10 -Bsa1O/10 -P --PLOT_DATE_FORMAT="-o-yy" --ANNOT_FONT_SIZE_PRIMARY="+9p" > T6.ps
compare -density 100 -metric PSNR T6_orig.ps T6.ps T6_diff.png > log
grep inf log > fail
if [ ! -s fail ]; then
        echo "[FAILED]"
else
        echo "[OK"]
        rm -f fail T6_diff.png log
fi

rm -f $$.* .gmtcommands*
