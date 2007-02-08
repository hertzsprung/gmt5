#!/bin/sh
#	$Id: GMT_App_O_7.sh,v 1.6 2007-02-08 21:46:28 remko Exp $
#
#	Makes Fig 7 for Appendix O (labeled lines)
#

pscoast -R50/160/-15/15 -JM5.3i -Glightgray -A500 -K -P > GMT_App_O_7.ps
grdcontour geoid.grd -J -O -K -B20f10WSne -C10 -A20+d+um+s8 -Gl50/10S/160/10S -S10 \
	-T:-+ >> GMT_App_O_7.ps
psxy -R -J -O -SqD15d:+gblack+kwhite+Ld+o+u-\\260 -Wthick transect.d >> GMT_App_O_7.ps
