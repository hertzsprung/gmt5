#!/bin/sh
# I used this to prep the data sets.  We get a chunk of
# Etopo1m topo over eastern US and then randomly sample
# 5000 points for use in gridding.
#exit
N=5100
gmt grdcut -R80W/64W/34N/42N etopo1m.nc -Gtopo.nc
gmt grdgradient topo.nc -A45 -Nt1 -Gint.nc
gmt makecpt -Crainbow -T-5400/1500/500 -Z > t.cpt
gmt math -T1/$N/1 -80 -64 RAND = x
sleep 1
gmt math -T1/$N/1 34 42 RAND = y
paste x y | gmt grdtrack -Gtopo.nc -i1,3 | gmt blockmean -R -I1m -fg | awk '{if (NR <= 5000) print $0}' > topo.xyz
gmt grdimage topo.nc -Iint.nc -Ct.cpt -JM6i -P -Baf -BWSne -K -X1i > t.ps
gmt pscoast -R -J -O -K -W0.25p -Dh >> t.ps
gmt psxy -R -J -O -K -Baf -BWSne -Sc0.1c -Ct.cpt -Y5i topo.xyz >> t.ps
gmt psscale -Ct.cpt -D6.2i/4.5i/8i/0.1i -Baf -O -K -Y-5i >> t.ps
gmt psxy -R -J -O -T >> t.ps
gmt psconvert -Tf -P t.ps
open t.pdf
rm -f x y t.cpt t.ps int.nc
wc topo.xyz
