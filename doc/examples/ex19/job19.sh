#!/bin/sh
#		GMT EXAMPLE 19
#		$Id: job19.sh,v 1.16 2009-05-07 23:08:58 guru Exp $
#
# Purpose:	Illustrates various color pattern effects for maps
# GMT progs:	gmtset, grdimage, grdmath, makecpt, pscoast, pstext, psimage
# Unix progs:	rm
#
ps=example_19.ps

# First make a worldmap with graded blue oceans and rainbow continents

gmtset COLOR_MODEL rgb
grdmath -Rd -I1 Y COSD 2 POW = lat.grd
grdmath -Rd -I1 X Y ABS 90 NEQ MUL = lon.grd
echo "0 white 1 blue" > lat.cpt
makecpt -Crainbow -T-180/180/60 -Z > lon.cpt
grdimage lat.grd -Sl -JI0/6.5i -Clat.cpt -P -K -Y7.5i -B0 > $ps
pscoast -R -J -O -K -Dc -A5000 -Gc >> $ps
grdimage lon.grd -Sl -J -Clon.cpt -O -K >> $ps
pscoast -R -J -O -K -Q >> $ps
pscoast -R -J -O -K -Dc -A5000 -Wthinnest >> $ps
echo "0 20 32 0 1 CM 8TH INTERNATIONAL" | pstext -R -J -O -K -Gred -Sthinner >> $ps
echo "0 -10 32 0 1 CM GMT CONFERENCE" | pstext -R -J -O -K -Gred -Sthinner >> $ps
echo "0 -30 18 0 1 CM Honolulu, Hawaii, April 1, 2010" | pstext -R -J -O -K -Ggreen -Sthinnest >> $ps

# Then show example of color patterns and placing a PostScript image

pscoast -R -J -O -K -Dc -A5000 -Gp100/86:FredByellow -Sp100/circuit.ras -B0 -Y-3.25i >> $ps
echo "0 30 32 0 1 CM SILLY USES OF" | pstext -R -J -O -K -Glightgreen -Sthinner >> $ps
echo "0 -30 32 0 1 CM COLOR PATTERNS" | pstext -R -J -O -K -Gmagenta -Sthinner >> $ps
psimage -C3.25i/1.625i/CM -W3i GMT_covertext.eps -O -K >> $ps

# Finally repeat 1st plot but exchange the patterns

grdimage lon.grd -Sl -J -Clon.cpt -O -K -Y-3.25i -B0 -U"Example 19 in Cookbook" >> $ps
pscoast -R -J -O -K -Dc -A5000 -Gc >> $ps
grdimage lat.grd -Sl -J -Clat.cpt -O -K >> $ps
pscoast -R -J -O -K -Q >> $ps
pscoast -R -J -O -K -Dc -A5000 -Wthinnest >> $ps
echo "0 20 32 0 1 CM 8TH INTERNATIONAL" | pstext -R -J -O -K -Gred -Sthinner >> $ps
echo "0 -10 32 0 1 CM GMT CONFERENCE" | pstext -R -J -O -K -Gred -Sthinner >> $ps
echo "0 -30 18 0 1 CM Honolulu, Hawaii, April 1, 2010" | pstext -R -J -O -Ggreen -Sthinnest >> $ps

rm -f l*.grd l*.cpt .gmt*
