#!/bin/sh
#
#	$Id: rendering.sh,v 1.1 2007-09-11 22:54:05 remko Exp $

ps=rendering.ps

echo -n "$0: Test grdimage for rendering issues:			"
xyz2grd -R0/360/0/90 -F -I60/30 -Gt.grd <<%
#30 45 -1
90 15 -1
30 75 0
90 45 0
150 15 0
90 75 1
150 45 1
210 15 1
150 75 0
210 45 0
270 15 0
210 75 -1
270 45 -1
330 15 -1
270 75 0
330 45 0
#
30 15 0
330 75 -1
%

render () {
grdimage $VERBOSE -R -JW0/6i -B60/30 t.grd -Ct.cpt -E100 -O -K $*
echo 180 35 16 0 1 BL $1 | pstext -R -J -O -K -N
}

makecpt -Cpolar -T-1/1/0.5 -Z -D > t.cpt

psscale -D3i/-0.4i/6i/0.4ih -Ct.cpt -P -K -Y2i > $ps
render -Sn >> $ps
render -Sl -Y2i >> $ps
render -Sb -Y2i >> $ps
render -Sc -Y2i >> $ps
psxy -R -J /dev/null -O >> $ps

rm -f t.grd t.cpt .gmtcommands4

compare -density 100 -metric PSNR {,orig/}$ps rendering_diff.png > log 2>&1
grep inf log > fail
if [ ! -s fail ]; then
        echo "[FAIL]"
	echo $0 >> ../fail_count.d
else
        echo "[PASS]"
        rm -f fail rendering_diff.png log
fi
