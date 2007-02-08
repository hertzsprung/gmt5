#!/bin/sh
#	$Id: GMT_volcano.sh,v 1.5 2007-02-08 21:43:12 remko Exp $

ln -s ../../examples/ex20/bullseye.def .
echo "0 0" | psxy -R-0.5/0.5/-0.5/0.5 -JX2i -P -Ba0.25g0.05WSne -Wthick -Skvolcano/2i -K > GMT_volcano.ps
echo "0 0" | psxy -R -J -N -Ba0.25g0.05wSnE -Wthick -Skbullseye/2i -O -X2.5i >> GMT_volcano.ps
rm -f bullseye.def
