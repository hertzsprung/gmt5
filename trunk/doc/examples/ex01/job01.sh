#!/bin/sh
#
#	GMT Example 01  $Id: job01.sh,v 1.11 2006-10-22 14:26:49 remko Exp $
#
# Purpose:	Make two contour maps based on the data in the file osu91a1f_16.grd
# GMT progs:	gmtset, grdcontour, psbasemap, pscoast
# Unix progs:	rm
#
gmtset GRID_CROSS_SIZE 0 ANNOT_FONT_SIZE_PRIMARY 10
psbasemap -R0/6.5/0/9 -Jx1i -B0 -P -K -U"Example 1 in Cookbook" > example_01.ps
pscoast -Rg -JH0/6i -X0.25i -Y0.5i -O -K -Bg30 -Dc -Glightgray >> example_01.ps
grdcontour -R osu91a1f_16.grd -J -C10 -A50+s7 -Gd4i -L-1000/-1 -Wcthinnest,- -Wathin,- -O -K -T0.1i/0.02i >> example_01.ps
grdcontour -R osu91a1f_16.grd -J -C10 -A50+s7 -Gd4i -L-1/1000 -O -K -T0.1i/0.02i >> example_01.ps
pscoast -Rg -JH180/6i -Y4i -O -K -Bg30:."Low Order Geoid": -Dc -Glightgray >> example_01.ps
grdcontour osu91a1f_16.grd -J -C10 -A50+s7 -Gd4i -L-1000/-1 -Wcthinnest,- -Wathin,- -O -K -T0.1i/0.02i:-+ >> example_01.ps
grdcontour osu91a1f_16.grd -J -C10 -A50+s7 -Gd4i -L-1/1000 -O -T0.1i/0.02i:-+ >> example_01.ps
rm -f .gmtcommands4 .gmtdefaults4
