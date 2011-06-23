#!/bin/bash
#	$Id: GMT_log.sh,v 1.9 2011-06-23 01:17:06 remko Exp $
#
. ./functions.sh

psxy -R1/100/0/10 -Jx1.5il/0.15i -B2g3/a2f1g2WSne+gbisque -Wthick,blue,- -P -K -h sqrt.d > GMT_log.ps
psxy -R -J -Ss0.1i -N -Gred -W -O -h sqrt.d10 >> GMT_log.ps
