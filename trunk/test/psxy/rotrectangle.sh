#!/bin/sh
#	$Id: rotrectangle.sh,v 1.5 2007-05-31 02:51:31 pwessel Exp $
#
# Test that psxy properly plots rotatable rectangles -Sj and -SJ

echo -n "$0: Test psxy and the rotated rectangle option:		"

# Bottom case tests -SJ with azimuths and dimensions in km
cat << EOF > $$.rects.d
-65 15 90 500 200
-70 20 0 500 200
-80 25 70 500 300
EOF
psxy -R270/20/305/25r -JOc280/25.5/22/69/4 -P -B10g5WSne -SJ $$.rects.d -Gred -W0.25p,green -K > rect.ps
psxy -R -J -O -K -Sc0.05 $$.rects.d -G0 >> rect.ps
# Middle case tests -Sj with angles and dimensions in inches
cat << EOF > $$.rects.d
-75 15 0 1 0.5
-70 20 30 1 0.5
-80 25 90 0.5 0.2
EOF
psxy -R -J -O -K -B10g5WSne  -Sj $$.rects.d -Gblue -W0.25p,green -Y3i >> rect.ps
psxy -R -J -O -K -Sc0.05 $$.rects.d -G0 >> rect.ps
# Top case is just Cartesian case where we pass angle and dimensions in -R units
cat << EOF > $$.rects.d
0 0 30 5 2
10 10 70 7 1
20 0 90 6 3
EOF
psxy -R-10/25/-5/15 -Jx0.15 -O -K -B10g5WSne -SJ $$.rects.d -Gbrown -W0.25p,green -Y3i >> rect.ps
psxy -R -J -O -Sc0.05 $$.rects.d -G0 >> rect.ps
compare -density 100 -metric PSNR rect_orig.ps rect.ps rect_diff.png > log
grep inf log > fail
if [ ! -s fail ]; then
        echo "[FAIL]"
	echo $0 >> ../fail_count.d
else
        echo "[PASS]"
        rm -f fail rect_diff.png log
fi
rm -f $$.*
