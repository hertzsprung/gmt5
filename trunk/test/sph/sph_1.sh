#!/bin/bash
#
#       $Id$

ps=sph_1.ps

gmt makecpt -Crainbow -T-7000/15000/1000 -Z > tt.cpt
gmt sphinterpolate mars370.txt -Rg -I1 -Q0 -Gtt.nc
gmt grdimage tt.nc -JH0/4.5i -B30g30 -B+t"-Q0" -Ctt.cpt -X0.8i -Y5.5i -K --MAP_TITLE_OFFSET=0i --FONT_TITLE=18p > $ps
gmt sphinterpolate mars370.txt -Rg -I1 -Q1 -Gtt.nc
gmt grdimage tt.nc -J -B30g30 -B+t"-Q1" -Ctt.cpt -X4.9i -O -K  --MAP_TITLE_OFFSET=0i --FONT_TITLE=18p >> $ps
gmt sphinterpolate mars370.txt -Rg -I1 -Q2 -Gtt.nc
gmt grdimage tt.nc -J -B30g30 -B+t"-Q2" -Ctt.cpt -X-4.9i -Y-5i -O -K  --MAP_TITLE_OFFSET=0i --FONT_TITLE=18p >> $ps
gmt sphinterpolate mars370.txt -Rg -I1 -Q3 -Gtt.nc
gmt grdimage tt.nc -J -B30g30 -B+t"-Q3" -Ctt.cpt -X4.9i -O -K  --MAP_TITLE_OFFSET=0i --FONT_TITLE=18p >> $ps
gmt psxy -Rg -J -O -K mars370.txt -Sc0.05i -G0 -B30g30 -X-2.45i -Y2.5i >> $ps
gmt psxy -Rg -J -O -T >> $ps

