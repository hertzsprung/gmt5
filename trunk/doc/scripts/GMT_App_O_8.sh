#!/bin/bash
#	$Id$
#
#	Makes Fig 8 for Appendix O (labeled lines)
#
gmtconvert -i0,1,4 -Em150 transect.d | $AWK '{print $1,$2,int($3)}' > fix2.d
pscoast -R50/160/-15/15 -JM5.3i -Gburlywood -Sazure -A500 -K -P > GMT_App_O_8.ps
grdcontour geoid.nc -J -O -K -B20f10 -BWSne -C10 -A20+d+u" m"+f8p -Gl50/10S/160/10S -S10 \
	-T:-+ >> GMT_App_O_8.ps
psxy -R -J -O -Sqffix2.d:+g+an+p+Lf+u" m"+f8p -Wthick transect.d >> GMT_App_O_8.ps
