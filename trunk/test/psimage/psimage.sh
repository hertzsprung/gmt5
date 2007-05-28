#!/bin/sh
ps=psimage.ps
echo -n "GMT: Test psimage with different pattern options:		"
cat > t.in <<%
0 0
1 0 :FwhiteBblack
0 1 :FblackB-
1 1 :F-Bblack
0 2 :FwhiteB-
1 2 :F-Bwhite
0 3 :FredB-
1 3 :F-Bred
0 4 :FredByellow
1 4 :FyellowBred
%
psxy -R0/3/0/5 -JX4.5i/7.5i -Gp128/vegetative_fog.ras -P -K > $ps <<%
0 0
2 0
3 1
3 4
2 5
0 5
%
awk '{ x0=$1;x1=x0+1;y0=$2;y1=y0+1;c=$3; \
	printf "> -Gp80/10%s\n%i %i\n%i %i\n%i %i\n",c,x0,y0,x1,y1,x0,y1 ; \
	printf "> -GP80/10%s\n%i %i\n%i %i\n%i %i\n",c,x0,y0,x1,y1,x1,y0}' < t.in \
	| psxy -R -J -M -O -K >> $ps
awk '{ x0=$1+0.5;y0=$2+0.5;c=$3; \
	printf "%g %g 7 0 1 BR p%s\n",x0,y0,c ; \
	printf "%g %g 7 0 1 TL P%s\n",x0,y0,c}' < t.in \
	| pstext -Gpurple -R -J -O -K >> $ps
psimage -E80 -C3i/3i/BL ../../share/pattern/ps_pattern_10.ras -Gfred -Gb- -O -K >> $ps
psimage -E80 -C3i/3i/TL ../../share/pattern/ps_pattern_10.ras -O >> $ps
rm -f t.in .gmtcommands4

compare -density 100 -metric PSNR psimage_orig.ps $ps psimage_diff.png > log
grep inf log > fail
if [ ! -s fail ]; then
        echo "[FAILED]"
else
        echo "[OK"]
        rm -f fail psimage_diff.png log
fi

