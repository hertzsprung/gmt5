#!/bin/sh
#	$Id: GMT_-B_time1.sh,v 1.5 2004-04-12 19:51:12 pwessel Exp $
#
gmtset PLOT_DATE_FORMAT -o ANNOT_FONT_SIZE +9p
psbasemap -R2000-4-1T/2000-5-25T/0/1 -JX6T/0.2 -Bpa1OS -Bsa7Rf1d -P > GMT_-B_time1.ps
