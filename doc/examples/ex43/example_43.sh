#!/bin/bash
#               GMT EXAMPLE 43
#               $Id$
#
# Purpose:      Illustrate regression and outlier detection
# GMT progs:    gmtregress, psbasemap, pstext, psxy
# Unix progs:   grep, paste, awk, sed
#

# Data from Table 7 in Rousseeuw and Leroy, 1984.
ps=example_43.ps

gmt regress -Ey -Nw -i0-1l bb_weights.txt > model.txt
gmt regress -Ey -Nw -i0-1l bb_weights.txt -Fxmc -T-2/6/0.1 > rls_line.txt
gmt regress -Ey -N2 -i0-1l bb_weights.txt -Fxm -T-2/6/8 > ls_line.txt
grep -v '^>' model.txt > A
grep -v '^#' bb_weights.txt > B
awk '{if ($7 == 0) printf "%dp\n", NR}' A > sed.txt
echo 0 lightred > t.cpt
echo 1 green >> t.cpt
gmt psbasemap -R0.01/1e6/0.1/1e5 -JX6il -P -Ba1pf3 -Bx+l"Log@-10@- body weight (kg)" -By+l"Log@-10@- brain weight (g)" -BWSne+glightblue -K -X1.5i -Y4i > $ps
gmt psxy -R-2/6/-1/5 -JX6i -O -K rls_line.txt -L+yt -Glightgreen >> $ps
sed -n -f sed.txt B | gmt pstext -R0.01/1e6/0.1/1e5 -JX6il -O -K -F+f12p+jRM -Dj0.15i >> $ps
gmt psxy -R-2/6/-1/5 -JX6i -O -K -L+d+p0.25p,- -Gcornsilk1 rls_line.txt >> $ps
gmt psxy -R -J -O -K rls_line.txt -W3p >> $ps
gmt psxy -R -J -O -K ls_line.txt -W1p,- >> $ps
gmt psxy -R -J -O -K -Sc0.15i -Ct.cpt -Wfaint -i0,1,6 model.txt >> $ps
awk '{print $1, $2, NR}' A | gmt pstext -R -J -O -K -F+f8p+jCM  -B0 >> $ps
gmt psbasemap -R0.5/28.5/-10/4 -JX6i/2i -O -K -Y-2.9i -B+glightgreen >> $ps
gmt psxy -R -J -O -K -Gcornsilk1 -W0.25p,- << EOF >> $ps
>
0	-2.5
30	-2.5
30	2.5
0	2.5
> -Glightblue
0	-10
30	-10
30	-2.5
0	-2.5

EOF
awk '{print NR, $6, $7}' A | gmt psxy -R -J -O -K -Sb1ub0 -W0.25p -Ct.cpt >> $ps
gmt psbasemap -R -J -O -K -Bafg100 -Bx+l"Index number" -By+l"z-zcore" -BWSne >> $ps
gmt psxy -R -J -O -T >> $ps
