#!/bin/sh
#
#	$Id: time_testing_5.sh,v 1.7 2007-11-15 04:20:42 remko Exp $
#
# This script runs some simple test to verify the that new time scheme
# has been implemented successfully

# Test 5:
# We plot a data set with psxy in two ways:  The bottom plot is based on
# calendar data whereas the upper plot is based on time relative to the
# users epoch.  We use the same data as before to generate the absolute
# coordinates using the Apollo 11 epoch.

. ../functions.sh
header "Test time conversions (abs & rel time)"

ps=time_testing_5.ps

sample1d -I0.5 << EOF > $$.d
0	0
4	1
EOF
gmtconvert $$.d -fi0t -fo0T --TIME_EPOCH=1969-07-21T02:56:00 --TIME_UNIT=d \
  | psxy -R1969-07-18T/1969-07-28T/-0.1/1.1 -JX9T/6 -Bpa7Rf1d/0.2 -Bsa1O/0.2 -Sc0.15 -Gred --PLOT_DATE_FORMAT="-o yyyy" --ANNOT_FONT_SIZE_PRIMARY="+9p" -K > $ps
psxy $$.d -R -JX9t/6 --TIME_EPOCH=1969-07-21T02:56:00 -Sx0.25 --TIME_UNIT=d --PLOT_DATE_FORMAT="-o yyyy" --ANNOT_FONT_SIZE_PRIMARY="+9p" -O >> $ps

rm -f $$.* .gmtcommands*

pscmp
