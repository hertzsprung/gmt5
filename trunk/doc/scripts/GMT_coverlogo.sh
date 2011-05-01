#!/bin/bash
#	$Id: GMT_coverlogo.sh,v 1.12 2011-05-01 18:06:37 remko Exp $
#
# Creates the cover page GMT logo
#
#	Logo is 5.458" wide and 2.729" high and origin is lower left
#
. ./functions.sh

scale=2.729
psxy -R0/2/0/1 -Jx${scale}i -P -K -X0 -Y0 /dev/null > GMT_coverlogo.ps
gmtlogo 0 0 $scale >> GMT_coverlogo.ps
psxy -R -J -O /dev/null >> GMT_coverlogo.ps
