#!/bin/sh
#	$Id: sph_ex_2.sh,v 1.1 2009-03-16 18:47:31 myself Exp $
# Example of gridding with sphinterpolate
PS=`basename $0 '.sh'`.ps
PDF=`basename $0 '.sh'`.pdf
makecpt -Crainbow -T-9000/9000/1000 -Z > $$.cpt
sphinterpolate lun2.txt -Rg -I1 -Q0 -G$$.nc -V
grdimage $$.nc -JH0/4.5i -B30g30:."-Q0": -C$$.cpt -X0.8i -Y5.5i -K --HEADER_OFFSET=0i --HEADER_FONT_SIZE=18p > $PS
sphinterpolate lun2.txt -Rg -I1 -Q1 -G$$.nc -V
grdimage $$.nc -J -B30g30:."-Q1": -C$$.cpt -X4.9i -O -K  --HEADER_OFFSET=0i --HEADER_FONT_SIZE=18p >> $PS
sphinterpolate lun2.txt -Rg -I1 -Q2 -G$$.nc -V
grdimage $$.nc -J -B30g30:."-Q2": -C$$.cpt -X-4.9i -Y-5i -O -K  --HEADER_OFFSET=0i --HEADER_FONT_SIZE=18p >> $PS
sphinterpolate lun2.txt -Rg -I1 -Q3 -G$$.nc -V
grdimage $$.nc -J -B30g30:."-Q3": -C$$.cpt -X4.9i -O -K  --HEADER_OFFSET=0i --HEADER_FONT_SIZE=18p >> $PS
psxy -Rg -J -O -K lun2.txt -Sc0.02i -G0 -B30g30 -X-2.45 -Y2.5i >> $PS
psxy -Rg -J -O /dev/null >> $PS
ps2raster -Tf $PS
open $PDF
rm -f $PS *$$*
