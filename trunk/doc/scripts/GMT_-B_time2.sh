#!/bin/bash
#	$Id: GMT_-B_time2.sh,v 1.11 2011-05-01 18:06:37 remko Exp $
#
. ./functions.sh
gmtset FORMAT_DATE_MAP "o dd" FORMAT_CLOCK_MAP hh:mm FONT_ANNOT_PRIMARY +9p
psbasemap -R1969-7-21T/1969-7-23T/0/1 -JX5i/0.2i -Bpa6Hf1h -Bsa1KS -P -K > GMT_-B_time2.ps
psbasemap -R -J -Bpa6Hf1h -Bsa1DS -O -Y0.65i >> GMT_-B_time2.ps
