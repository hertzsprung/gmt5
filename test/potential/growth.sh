#!/bin/bash
#	$Id$
#
# Test the output of gmt grdseamount for Gaussian shapes
ps=growth.ps
gmt gmtset MAP_FRAME_TYPE plain
cat << EOF > t.txt
#lon lat azimuth, semi-major, semi-minor, height
50	100	110	60	30	5000	6	2
150	50	30	50	40	4000	10	4
EOF
echo "100 75 0 50 50 5000 10 0" > t.txt
#echo "100 75 50 5000 10 0" > t.txt
#grdseamount -Rk0/200/0/150 -I1000 -Gsmt_%3.1f.nc t.txt -T10/0/1 -V -Cg -Qc/g -Dk
grdseamount -Rk0/200/0/150 -I100 -Gsmt_%4.2f.nc t.txt -T10/0/0.01 -Vl -Cc -Qc/g -Dk -E
grdseamount -Rk0/200/0/150 -I100 -Gsmt.nc t.txt -Vl -Cc -Dk -E
#grdseamount -Rk0/200/0/150 -I1000 -Gsmt_0.0.nc t.txt -Vl -Cg -Dk -E
grdcontour smt_0.00.nc+Uk -Jx0.03i -Xc -P -C250 -A1000 -Bafg -K > $ps
grdtrack -Gsmt_0.00.nc+Uk -E0/75/200/75 -o0,2 -nn | psxy -R0/200/0/6000 -JX6i/4i -O -K -Y5.2i -Baf >> $ps
grdtrack -Gsmt.nc+Uk -E0/75/200/75 -o0,2 -nn | psxy -R0/200/0/6000 -JX6i/4i -O -K -W0.25p,red >> $ps
psxy -R -J -O -T >> $ps
ps2raster -Tf $ps
open growth.pdf
rm -f smt_*.??.nc
