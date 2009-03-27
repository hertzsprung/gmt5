#!/bin/sh
#	$Id: sph_ex_3.sh,v 1.3 2009-03-27 23:04:08 guru Exp $
# Example of computing distances with sphdistance
PS=`basename $0 '.sh'`.ps
# Get the crude GSHHS data, select GMT -M format, and decimate to ~20%:
# gshhs $GMTHOME/src/coast/gshhs/gshhs_c.b -M | awk '{if ($1 == ">" || NR%5 == 0) print $0}' > gshhs_c.txt
# Get Voronoi polygons
sphtriangulate gshhs_c.txt -Qv -M -D > $$.pol
# Compute distances in km
sphdistance -Rg -I1 -Q$$.pol -G$$.nc -Lk -M
# Make a basic contour plot and overlay voronoi polygons and coastlines
grdcontour $$.nc -JG-140/30/7i -P -B30g30:"Distances from GSHHS crude": -K -C500 -A1000 -X0.75i -Y2i > $PS
psxy -R -J -O -K -M $$.pol -W0.25p,red >> $PS
pscoast -R -J -O -K -W1p -Glightgray -A0/1/1 >> $PS
psxy -Rg -J -O /dev/null >> $PS
gv $PS &
rm -f *$$*
