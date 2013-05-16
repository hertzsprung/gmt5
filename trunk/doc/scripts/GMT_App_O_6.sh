#!/bin/bash
#	$Id$
#
#	Makes Fig 6 for Appendix O (labeled lines)
#
gmt pscoast -R50/160/-15/15 -JM5.3i -Gburlywood -Sazure -A500 -K -P > GMT_App_O_6.ps
gmt grdcontour geoid.nc -J -O -K -B20f10 -BWSne -C10 -A20+d+f8p -Gl50/10S/160/10S -S10 \
	-T:'-+' >> GMT_App_O_6.ps
gmt psxy -R -J -O -SqD1000k:+g+LD+an+p -Wthick transect.d >> GMT_App_O_6.ps
