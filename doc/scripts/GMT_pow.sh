#!/bin/sh
#	$Id: GMT_pow.sh,v 1.3 2006-10-24 01:53:19 remko Exp $
#

psxy -R0/100/0/10 -Jx0.3ip0.5/0.15i -Ba1p/a2f1WSne -Wthick -P -K sqrt.d > GMT_pow.ps
psxy -R -J -Sc0.075i -Gwhite -W -O sqrt.d10 >> GMT_pow.ps
