#!/bin/bash
#	$Id$
#

ps=seis.ps

gmt gmtset FONT_ANNOT_PRIMARY 10p PROJ_LENGTH_UNIT cm

gmt makecpt -T-6/6/1 -Cseis -D > tmp.cpt
gmt psscale -Ctmp.cpt -D00/04/8/0.5 -K           > $ps
gmt psscale -Ctmp.cpt -D00/13/8/0.5 -O -K -I -N100 >> $ps
gmt makecpt -T-6/6/1 -Cseis -D | grep -v ^0 | sed 's/^1/0/' > tmp.cpt
gmt psscale -Ctmp.cpt -D04/04/8/0.5 -O -K       >> $ps
gmt psscale -Ctmp.cpt -D04/13/8/0.5 -O -K -I -N100 >> $ps
gmt psscale -Ctmp.cpt -D08/04/8/0.5 -O -K -L    >> $ps
gmt psscale -Ctmp.cpt -D08/13/8/0.5 -O -K -L -I -N100 >> $ps
gmt psscale -Ctmp.cpt -D12/04/8/0.5 -O -K -L0.1    >> $ps
gmt psscale -Ctmp.cpt -D12/13/8/0.5 -O -K -L0.1 -I -N100 >> $ps
gmt makecpt -T-6/6/1 -Cseis -D -Z > tmp.cpt
gmt psscale -Ctmp.cpt -D16/04/8/0.5 -O -K       >> $ps
gmt psscale -Ctmp.cpt -D16/13/8/0.5 -O -K -I -N100 >> $ps
gmt makecpt -T-6/6/1 -Cseis -D -Z | grep -v ^0 | sed 's/^1/0/' > tmp.cpt
gmt psscale -Ctmp.cpt -D20/04/8/0.5 -O -K       >> $ps
gmt psscale -Ctmp.cpt -D20/13/8/0.5 -O    -I -N100 >> $ps

