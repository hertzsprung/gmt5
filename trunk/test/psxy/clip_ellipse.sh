#!/bin/sh
#	$Id: clip_ellipse.sh,v 1.2 2010-01-12 17:04:23 remko Exp $
#
# Check clipping of ellipses that should wrap in a global projection

. ../functions.sh
header "Test psxy global projections for clipping ellipses"

ps=clip_ellipse.ps
echo 180 0 0 4000 4000 > ellipse.d
psxy ellipse.d -P -K -JM3i -R-180/180/-60/60 -B0g180 -Wthin -SE -Gred -X1i -Y0.75i --BASEMAP_TYPE=plain > $ps 
psxy ellipse.d -O -K -JH0/3i -Rg -B0g180 -Wthin -SE -Gred -X3.5i --BASEMAP_TYPE=plain >> $ps
psxy ellipse.d -O -K -JW0/3i -Rg -B0g180 -Wthin -SE -Gred -X-3.5i -Y1.8i --BASEMAP_TYPE=plain >> $ps
psxy ellipse.d -O -K -JI0/3i -Rg -B0g180 -Wthin -SE -Gred -X3.5i --BASEMAP_TYPE=plain >> $ps
psxy ellipse.d -O -K -JN0/3i -Rg -B0g180 -Wthin -SE -Gred -X-3.5i -Y1.8i --BASEMAP_TYPE=plain >> $ps
psxy ellipse.d -O -K -JR0/3i -Rg -B0g180 -Wthin -SE -Gred -X3.5i --BASEMAP_TYPE=plain >> $ps
psxy ellipse.d -O -K -JKf0/3i -Rg -B0g180 -Wthin -SE -Gred -X-3.5i -Y2i --BASEMAP_TYPE=plain >> $ps
psxy ellipse.d -O -K -JKs0/3i -Rg -B0g180 -Wthin -SE -Gred -X3.5i --BASEMAP_TYPE=plain >> $ps
psxy ellipse.d -O -K -JV0/2i -Rg -B0g180 -Wthin -SE -Gred -X-3i -Y1.8i --BASEMAP_TYPE=plain >> $ps
psxy ellipse.d -O -K -JY0/45/3i -Rg -B0g180 -Wthin -SE -Gred -X3i --BASEMAP_TYPE=plain >> $ps
psxy -R -J -O /dev/null >> $ps
rm -f .gmtcommands4 ellipse.d

pscmp
