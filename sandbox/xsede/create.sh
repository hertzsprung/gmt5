#!/bin/sh
# I used this to prep the data sets.  We get a chunk of
# Etopo1m topo over eastern US and then randomly sample
# 1000 points for use in gridding.
exit
N=1010
#grdcut -R80W/64W/34N/42N etopo1m.nc -Gtopo.nc
#grdgradient topo.nc -A45 -Nt1 -Gint.nc
makecpt -Crainbow -T-5400/1500/500 -Z > t.cpt
gmtmath -T1/$N/1 -80 -64 RAND = x
sleep 1
gmtmath -T1/$N/1 34 42 RAND = y
paste x y | grdtrack -Gtopo.nc -i1,3 | blockmean -R -I2m -fg | awk '{if (NR <= 1000) print $0}' > topo.xyz
grdimage topo.nc -Iint.nc -Ct.cpt -JM6i -P -Baf -BWSne -K -X1i > t.ps
pscoast -R -J -O -K -W0.25p -Dh >> t.ps
psxy -R -J -O -K -Baf -BWSne -Sc0.1c -Ct.cpt -Y5i topo.xyz >> t.ps
psscale -Ct.cpt -D6.2i/4.5i/8i/0.1i -Baf -O -K -Y-5i >> t.ps
psxy -R -J -O -T >> t.ps
ps2raster -Tf -P t.ps
open t.pdf
rm -f x y t.cpt t.ps int.nc
wc topo.xyz
