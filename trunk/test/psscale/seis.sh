#!/bin/sh
#	$Id: seis.sh,v 1.10 2007-11-15 04:20:42 remko Exp $
#
. ../functions.sh

header "Test psscale and makecpt combinations"

ps=seis.ps
gmtset ANNOT_FONT_SIZE 10p MEASURE_UNIT cm

makecpt -T-6/6/1 -Cseis -D > tmp.cpt
psscale -Ctmp.cpt -D00/04/8/0.5 -K           > $ps
psscale -Ctmp.cpt -D00/13/8/0.5 -O -K -I -N100 >> $ps
makecpt -T-6/6/1 -Cseis -D | grep -v ^0 | sed 's/^1/0/' > tmp.cpt
psscale -Ctmp.cpt -D04/04/8/0.5 -O -K       >> $ps
psscale -Ctmp.cpt -D04/13/8/0.5 -O -K -I -N100 >> $ps
psscale -Ctmp.cpt -D08/04/8/0.5 -O -K -L    >> $ps
psscale -Ctmp.cpt -D08/13/8/0.5 -O -K -L -I -N100 >> $ps
psscale -Ctmp.cpt -D12/04/8/0.5 -O -K -L0.1    >> $ps
psscale -Ctmp.cpt -D12/13/8/0.5 -O -K -L0.1 -I -N100 >> $ps
makecpt -T-6/6/1 -Cseis -D -Z > tmp.cpt
psscale -Ctmp.cpt -D16/04/8/0.5 -O -K       >> $ps
psscale -Ctmp.cpt -D16/13/8/0.5 -O -K -I -N100 >> $ps
makecpt -T-6/6/1 -Cseis -D -Z | grep -v ^0 | sed 's/^1/0/' > tmp.cpt
psscale -Ctmp.cpt -D20/04/8/0.5 -O -K       >> $ps
psscale -Ctmp.cpt -D20/13/8/0.5 -O    -I -N100 >> $ps

rm -f tmp.cpt .gmtcommands4

pscmp seis
