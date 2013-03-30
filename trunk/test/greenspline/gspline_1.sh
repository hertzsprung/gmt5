#!/bin/bash
#
#       $Id$

ps=gspline_1.ps

# Figure 2 in Wessel, P. (2009), A general-purpose Green's function-based
#	interpolator, Computers & Geosciences, 35, 1247–1254.


R=-2000/25000/0/11
T=Table_4.2.txt
psbasemap -R-2/25/0/11 -JX6i/3i -P -K -B5f1:"Distance (km)":/2f1:"Mg (ppm)":WSne -X1.25i -Y2i --FONT_LABEL=18p > $ps
psxy -R$R -J -O -K $T -Sc0.075i -Gblack >> $ps
greenspline -R-2000/25000 -I100 $T -Sl -D0 | psxy -R$R -JX -O -K -Wthin,. >> $ps
greenspline -R-2000/25000 -I100 $T -Sc -D0 | psxy -R$R -JX -O -K -Wthin,- >> $ps
greenspline -R-2000/25000 -I100 $T -St0.25 -D0 | psxy -R$R -J -O -K -Wthin >> $ps
pslegend -R$R -JX -O -K -F -Dx5.9i/2.9i/2.05i/TR --FONT_ANNOT_PRIMARY=12p << EOF >> $ps
S 0.2i - 0.35i - 0.5p 0.5i Tension (@%6%t@%% = 0.25)
S 0.2i - 0.35i - 0.5p,- 0.5i No tension
S 0.2i - 0.35i - 0.5p,. 0.5i Linear interpolation
EOF
psxy -R$R -JX -O -T >> $ps

