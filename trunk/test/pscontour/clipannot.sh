#!/bin/bash
#
#	$Id: clipannot.sh,v 1.2 2011-03-22 18:41:32 guru Exp $

. ../functions.sh
header "Test pscontour for clip path set by annotations"

ps=clipannot.ps
# Make a grid, annotate every 20 and use those as clip paths
# Then draw some lines intersection the annotations and draw
# a dense gridline set.  Text should not have lines through them.
grdmath -R0/10/0/10 -I0.1 X Y MUL = tmp.nc
grd2xyz tmp.nc > tmp.xyz
pscontour -R0/10/0/10 -A20+e+c0.05i+s24 -C10 tmp.xyz -JX6i -P -K -W0.5p > $ps
psxy -R -J -O -K -W1p,red << EOF >> $ps
> line through 20 label
0	0
10	8.5
> line through 40 label
0	8.2
10	8.2
EOF
psbasemap -R -J -O -K -B0g0.2 >> $ps
psclip -Cs -O -K  >> $ps
psbasemap -R -J -O -B2WSne:."Delayed Annotations": >> $ps
rm -f tmp.nc tmp.xyz

pscmp
