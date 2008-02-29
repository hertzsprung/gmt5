#!/bin/sh
#	$Id: GMT_App_O.sh,v 1.9 2008-02-29 03:24:30 remko Exp $
#
#	Makes the inserts for Appendix O (labeled lines)
#	This first script just gets the data ready and run the various scripts
#

gmtset FRAME_WIDTH 0.04i PLOT_DEGREE_FORMAT ddd:mm:ssF ANNOT_FONT_SIZE_PRIMARY +9p
grdcut ../../examples/ex01/osu91a1f_16.nc -R50/160/-15/15 -Ggeoid.grd
# fixed algorithm points
cat << EOF > fix.d
80	-8.5
55	-7.5
102	0
130	10.5
EOF
# Complex line algorithm points
cat << EOF > cross.d
> 1st branch
59	-12
62	-7
66	-3
71	-1
77	3
> 2nd branch
94	-11
100	-10.5
105	-9.5
109	-8.6
114	-6.5
119	-4
126	2
> 3rd branch
148	3
158	13
EOF
info=`grdinfo -M -C geoid.grd`
x0=`echo $info | cut -f12 -d ' '`
y0=`echo $info | cut -f13 -d ' '`
x1=`echo $info | cut -f14 -d ' '`
y1=`echo $info | cut -f15 -d ' '`
project -C$x0/$y0 -E$x1/$y1 -G10 -Q > $$.d
dist=`gmtconvert -I $$.d --D_FORMAT=%.0lf | head -1 | cut -f3`
R=`minmax -I1 $$.d`
echo "# Geoid Extrema Separation is $dist km" > transect.d
grdtrack $$.d -Ggeoid.grd | grdtrack -GApp_O.nc >> transect.d
rm -f $$.d

for n in 1 2 3 4 5 6 7 8 9; do
	sh GMT_App_O_$n.sh
done

rm -f fix.d fix2.d cross.d geoid.grd transect.d great_NY_*.d ttt.cpt topo5_int.nc
