#!/bin/sh
#	$Id: clipping2.sh,v 1.2 2009-04-17 00:31:30 remko Exp $
#
# Check clipping of multisegment lines crossing over the horizon

. ../functions.sh
header "Test psxy -JG for clipping line crossing horizon (S pole)"

ps=clipping2.ps

pscoast -X1i -Y1i -K -P -A10/1 -Di -Wthin -B60 -JG0/-90/7i -R-180/180/-90/-40 --BASEMAP_TYPE=plain > $ps
psxy clipline2.xy -m -O -P -W0.5p,blue -J -R >> $ps 

rm -f .gmtcommands4

pscmp
