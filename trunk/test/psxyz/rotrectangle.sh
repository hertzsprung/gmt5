#!/bin/sh
#	$Id: rotrectangle.sh,v 1.6 2007-06-05 14:02:37 remko Exp $
#
# Test that psxyz properly plots rotatable rectangles -Sj and -SJ

echo -n "$0: Test psxyz and the rotated rectangle option:		"

# Bottom case tests -SJ with azimuths and dimensions in km
ps=rect.ps
cat << EOF > $$.rects.d
-65 15 0 90 500 200
-70 20 0 0 500 200
-80 25 0 70 500 300
EOF
psxyz -JZ2 -E135/30 -R270/20/305/25/0/3r -JOc280/25.5/22/69/5i -P -B10g5WSne -SJ $$.rects.d -Gred -W0.25p,green -K > $ps
psxyz -JZ -E135/30 -R -J -O -K -Sc0.05i $$.rects.d -Gblack >> $ps
# Middle case tests -Sj with angles and dimensions in inches (hence trailing i)
cat << EOF > $$.rects.d
-75 15 0 0 1 0.5
-70 20 0 30 1 0.5
-80 25 0 90 0.5 0.2
EOF
psxyz -JZ -E135/30 -R -J -O -K -B10g5WSne -Sji $$.rects.d -Gblue -W0.25p,green -Y3i >> $ps
psxyz -JZ -E135/30 -R -J -O -K -Sc0.05i $$.rects.d -Gblack >> $ps
# Top case is just Cartesian case where we pass angle and dimensions in -R units
cat << EOF > $$.rects.d
0 0 0 30 5 2
10 10 0 70 7 1
20 0 0 90 6 3
EOF
psxyz -JZ -E135/30 -R-10/25/-5/15/0/4 -Jx0.15i -O -K -B10g5WSne -SJ $$.rects.d -Gbrown -W0.25p,green -Y3i >> $ps
psxyz -JZ -E135/30 -R -J -O -Sc0.05i $$.rects.d -Gblack >> $ps
compare -density 100 -metric PSNR {,orig/}$ps rect_diff.png > log
grep inf log > fail
if [ ! -s fail ]; then
        echo "[FAIL]"
	echo $0 >> ../fail_count.d
else
        echo "[PASS]"
        rm -f fail rect_diff.png log
fi
rm -f $$.*
