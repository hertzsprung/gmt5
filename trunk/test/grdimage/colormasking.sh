#!/bin/sh
#
#	$Id: colormasking.sh,v 1.10 2010-06-21 23:55:22 guru Exp $

. ../functions.sh
header "Test grdimage for use of color masking"

ps=colormasking.ps
#grdmath -R0/3/0/3 -I1 X Y DIV = t.nc
xyz2grd -R-0.5/2.5/-0.5/2.5 -I1 -F -Gt.nc <<%
0 0 0.0
0 1 0.2
0 2 0.4
1 0 1.0
1 1 NaN
1 2 1.4
2 0 2.0
2 1 2.2
2 2 2.4
%
makecpt -Crainbow -T-0.1/2.5/0.2 > t.cpt
grdimage t.nc -JX1c -Ct.cpt -K -P -X8c -Y8c > $ps
grdimage t.nc -JX3c -Ct.cpt -K -O -X-1c -Y-1c -Q >> $ps
grdimage t.nc -JX9c -Ct.cpt -K -O -X-3c -Y-3c -Q >> $ps
psxy -R -J -Gp50/10:FwhiteB- -O >> $ps <<%
1 0
2 0
2 1
1 1
%
rm -f t.nc t.cpt .gmtcommands4

pscmp
