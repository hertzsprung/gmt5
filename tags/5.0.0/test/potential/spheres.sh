#! /bin/bash
# $Id$
#
# Computes the gravity anomaly of a sphere both analytical and descrete triangles

. ../functions.sh
header "Compute gravity anom of a sphere with Okabe method"

r=10; z0=-15; ro=1000;
ps=spheres.ps

echo -50 0 > li
echo  50 0 >> li
sample1d li -Fl -I1 > li1.dat

#solidos -S$rad/$z0/10/48 > sphere.raw
xyzokb -Trsphere.raw -C$ro -Fli1.dat > ptodos_g.dat

# xyzokb solution
awk '{print $1, $3}' ptodos_g.dat | psxy -R-50/50/0/0.125 -JX14c/10c -B10f5/.01WSne:."Anomaly (mGal)": -W1p -P -K > $ps
awk '{print $1, $3}' ptodos_g.dat | psxy -R -JX -Sc.1c -G0 -O -K >> $ps


# Profile of analytic anomaly
gmtmath -T-50/50/1 T $z0 HYPOT 3 POW INV 6.674e-6 MUL 4 MUL 3 DIV PI MUL $r 3 POW MUL $ro MUL $z0 ABS MUL = ztmp.dat
psxy ztmp.dat -R -JX -W1p,200/0/0 -O >> $ps

rm -f li li1.dat ptodos_g.dat ztmp.dat .gmtcom*

pscmp