#!/bin/sh
#		GMT EXAMPLE 09
#
#		$Id: job09.sh,v 1.7 2005-12-11 06:23:08 pwessel Exp $
#
# Purpose:	Make wiggle plot along track from geoid deflections
# GMT progs:	pswiggle, pstext, psxy
# Unix progs:	$AWK, ls, paste, tail, rm
#
pswiggle track_*.xys -R185/250/-68/-42 -U"Example 9 in Cookbook" -K -Jm0.13i -Ba10f5 -Gblack -Z2000 -W0.25p -S240/-67/500/@~m@~rad > example_09.ps
psxy -R -J -O -K ridge.xy -W1.75p >> example_09.ps
psxy -R -J -O -K -M fz.xy -W0.5pta >> example_09.ps
if [ -e ./tmp ]; then
	rm -f tmp
fi
# Make label file
for file in track_*.xys
do
	tail -n 1 $file >> tmp
done
ls -1 track_*.xys | $AWK -F. '{print $2}' > tracks.lis
paste tmp tracks.lis | $AWK '{print $1, $2, 10, 50, 1, "RM", $4}' | pstext -R -J -D-0.05i/-0.05i -O >> example_09.ps
rm -f tmp tracks.lis .gmt*
