#!/bin/bash
#	$Id$
#
# Test the output of gmt grdflexure for single Gaussian seamount on elastic plate
ps=flexure_e.ps
m=g
f=0.2
gmt gmtset MAP_FRAME_TYPE plain
cat << EOF > t.txt
#lon lat azimuth, semi-major, semi-minor, height
300	200	70	70	30	5000
EOF
grdseamount -Rk0/600/0/400 -I1000 -Gsmt.nc t.txt -Dk -E -F$f -C$m
grdcontour smt.nc+Uk -Jx0.01i -Xc -P -A1 -GlLM/RM -Bafg -K -Z0.001 > $ps
grdflexure smt.nc -D3300/2700/2400/1030 -E5k -Gflex_e.nc -V
grdcontour flex_e.nc+Uk -J -O -K -C0.2 -A1 -Z0.001 -GlLM/RM -Bafg -BWsNE+t"Elastic Plate Flexure, T@-e@- = 5 km" -Y4.4i >> $ps
psxy -R -J -O -T >> $ps
