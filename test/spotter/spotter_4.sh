#!/bin/bash
#
#       $Id: spotter_4.sh,v 1.1 2011-05-26 18:50:39 guru Exp $

. ../functions.sh
header "Test spotter:grdrotater for grid rotation"

ps=grdrot.ps

# Build a grid
grdmath -R0/30/0/30 -I0.1 X Y MUL = t.nc
# Design a multi-polygon area of interest
cat << EOF > P.txt
>
5	5
15	5
15	20
5	20
5	5
>
20	20
28	20
28	28
20	28
20	20
EOF
# This is original grid outline, for reference
cat << EOF > G.txt
0	0
30	0
30	30
0	30
0	0
EOF
# Rotation without constraining polygons
grdrotater t.nc -e0/53/60 -Grot.nc > box.txt
grdcontour -R-10/80/-5/56 -JM5i -P -B10WSne t.nc -A100 -C25 -K -Xc > $ps
grdcontour -R -J rot.nc -A100 -C25 -O -K -Wa0.75p,red, -Wc0.25p,red >> $ps
psxy -R -J -O -K G.txt -W1p,blue -A >> $ps
psxy -R -J -O -K box.txt -W1p,red >> $ps
echo 0 53 | psxy -R -J -O -K -Sc0.15i -Gred -N -W0.25p >> $ps
echo 0 53 -90 -30 | psxy -R -J -O -K -Sml0.4i -N -W2p >> $ps
# Rotating with constraining polygons
grdrotater t.nc -FP.txt -e0/53/60 -Grot.nc > R.txt
grdcontour -R-10/80/-5/56 -JM5i -B10WSne t.nc -A100 -C25 -O -K -Y5i >> $ps
grdcontour -R -J rot.nc -A100 -C25 -O -K -Wa0.75p,red, -Wc0.25p,red >> $ps
psxy -R -J -O -K G.txt -W1p,blue -A >> $ps
psxy -R -J -O -K box.txt -W1p,red >> $ps
psxy -R -J -O -K P.txt -W1p >> $ps
psxy -R -J -O -K R.txt -W1p >> $ps
echo 0 53 | psxy -R -J -O -K -Sc0.15i -Gred -N -W0.25p >> $ps
echo 0 53 -90 -30 | psxy -R -J -O -K -Sml0.4i -N -W2p >> $ps
psxy -R -J -O -T >> $ps
rm -f *.txt t.nc rot.nc 

pscmp

