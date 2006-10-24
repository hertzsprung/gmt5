#!/bin/sh 
#	$Id: GMT_App_B_1.sh,v 1.4 2006-10-24 01:53:19 remko Exp $
#

trap 'rm -f $$.*; exit 1' 1 2 3 15

psxy -R0/3/0/3 -JX2.5i/1.25i -B1g1wesn -P -K -Wthinner -L -Ggray << EOF > GMT_gridreg.ps
0.5	1.5
1.5	1.5
1.5	2.5
0.5	2.5
EOF
xyz2grd -R0/3/0/3 -I1 -N0 /dev/null -G$$.grd 1>/dev/null 2>&1
grd2xyz $$.grd | psxy -R0/3/0/3 -JX2.5i/1.25i -Sc0.12i -N -G0 -O >> GMT_gridreg.ps
rm -f $$.*
