#!/bin/sh
#	$Id: GMT_App_O_2.sh,v 1.5 2010-06-21 23:42:55 guru Exp $
#
#	Makes Fig 2 for Appendix O (labeled lines)
#

pscoast -R50/160/-15/15 -JM5.3i -Glightgray -A500 -K -P > GMT_App_O_2.ps
grdcontour geoid.nc -J -O -B20f10WSne -C10 -A20+s8 -Gn1/1i -S10 -T:LH >> GMT_App_O_2.ps
