#!/bin/sh
#	$Id: GMT_App_B_2.sh,v 1.2 2004-04-10 17:19:14 pwessel Exp $
#

psxy -R0/3/0/3 -JX2.5i/1.25i -B1g1wesn -P -K -W0p -L -Ggray << EOF > GMT_pixelreg.ps
1	1
2	1
2	2
1	2
EOF
xyz2grd -R0/3/0/3 -I1 -F -N0 /dev/null -Gt.grd 2>&1 1>/dev/null
grd2xyz t.grd | psxy -R0/3/0/3 -JX2.5i/1.25i -Sc0.12i -Gblack -O >> GMT_pixelreg.ps
\rm -f t.grd
