#!/bin/sh
#		GMT EXAMPLE 14
#
#		$Id: job14.sh,v 1.11 2006-10-22 14:26:49 remko Exp $
#
# Purpose:	Showing simple gridding, contouring, and resampling along tracks
# GMT progs:	blockmean, grdcontour, grdtrack, grdtrend, minmax, project,
#		pstext, psbasemap, psxy, surface
# Unix progs:	$AWK, rm
#
# First draw network and label the nodes
gmtset GRID_PEN_PRIMARY thinnest,-
psxy table_5.11 -R0/7/0/7 -JX3.06i/3.15i -B2f1WSNe -Sc0.05i -Gblack -P -K -Y6.45i > example_14.ps
$AWK '{printf "%g %s 6 0 0 LM %g\n", $1+0.08, $2, $3}' table_5.11 | pstext -R -J -O -K -N >> example_14.ps
blockmean table_5.11 -R0/7/0/7 -I1 > mean.xyz
# Then draw blockmean cells
psbasemap -R0.5/7.5/0.5/7.5 -J -O -K -B0g1 -X3.25i >> example_14.ps
psxy -R -J -B2f1eSNw mean.xyz -Ss0.05i -Gblack -O -K >> example_14.ps
$AWK '{printf "%g %s 6 0 0 LM %g\n", $1+0.1, $2, $3}' mean.xyz | pstext -R -J -O -K -Wwhiteo -C0.01i/0.01i -N >> example_14.ps
# Then surface and contour the data
surface mean.xyz -R -I1 -Gdata.grd
grdcontour data.grd -J -B2f1WSne -C25 -A50 -G3i/10 -S4 -O -K -X-3.25i -Y-3.55i >> example_14.ps
psxy -R -J mean.xyz -Ss0.05i -Gblack -O -K >> example_14.ps
# Fit bicubic trend to data and compare to gridded surface
grdtrend data.grd -N10 -Ttrend.grd
project -C0/0 -E7/7 -G0.1 > track
grdcontour trend.grd -J -B2f1wSne -C25 -A50 -Glct/cb -S4 -O -K -X3.25i >> example_14.ps
psxy -R -J track -Wthick,. -O -K >> example_14.ps
# Sample along diagonal
grdtrack track -Gdata.grd | cut -f3,4 > data.d
grdtrack track -Gtrend.grd | cut -f3,4 > trend.d
psxy `minmax data.d trend.d -I0.5/25` -JX6.3i/1.4i data.d -Wthick -O -K -X-3.25i -Y-1.9i -B1/50WSne >> example_14.ps
psxy -R -J trend.d -Wthinner,- -O -U"Example 14 in Cookbook" >> example_14.ps
rm -f mean.xyz track *.grd *.d .gmt*
