#!/bin/sh
#		GMT EXAMPLE 19
#
#		$Id: job19.sh,v 1.2 2002-09-05 18:40:58 pwessel Exp $
#
# Purpose:	Illustrates various color pattern effects for maps
# GMT progs:	gmtset, grdimage, grdmath, makecpt, pscoast, pstext
# Unix progs:	rm
#
# First make a worldmap with graded blue oceans and rainbow continents

gmtset COLOR_MODEL rgb
grdmath -R-180/180/-90/90 -I1 -F Y COSD 2 POW = lat.grd
grdmath -R-180/180/-90/90 -I1 -F X = lon.grd
echo "0 255 255 255 1 0 0 255" > lat.cpt
makecpt -Crainbow -T-180/180/60 -Z > lon.cpt
grdimage lat.grd -JI0/6.5i -Clat.cpt -P -K -Y7.5i -B0 > example_19.ps
pscoast -R -JI -O -K -Dc -A5000 -Gc >> example_19.ps
grdimage lon.grd -JI -Clon.cpt -O -K >> example_19.ps
pscoast -R -JI -O -K -Q >> example_19.ps
pscoast -R -JI -O -K -Dc -A5000 -W0.25p >> example_19.ps
echo "0 20 32 0 1 CM 2ND INTERNATIONAL" | pstext -R -JI -O -K -G255/0/0 -S0.5p >> example_19.ps
echo "0 -10 32 0 1 CM GMT CONFERENCE" | pstext -R -JI -O -K -G255/0/0 -S0.5p >> example_19.ps
echo "0 -30 18 0 1 CM Honolulu, Hawaii, April 1, 2004" | pstext -R -JI -O -K -G0/255/50 -S0.25p >> example_19.ps

# Then show example of color patterns

pscoast -R -JI -O -K -Dc -A5000 -Gp100/86:F255/0/0B255/255/0 -Sp100/7:F255/0/0B0/0/0 -B0 -Y-3.25i >> example_19.ps
echo "0 15 32 0 1 CM SILLY USES OF" | pstext -R -JI -O -K -G50/255/50 -S0.5p >> example_19.ps
echo "0 -15 32 0 1 CM GMT COLOR PATTERNS" | pstext -R -JI -O -K -G255/0/255 -S0.5p >> example_19.ps

# Finally repeat 1st plot but exchange the patterns

grdimage lon.grd -JI -Clon.cpt -O -K -Y-3.25i -B0 -U"Example 19 in Cookbook" >> example_19.ps
pscoast -R -JI -O -K -Dc -A5000 -Gc >> example_19.ps
grdimage lat.grd -JI -Clat.cpt -O -K >> example_19.ps
pscoast -R -JI -O -K -Q >> example_19.ps
pscoast -R -JI -O -K -Dc -A5000 -W0.25p >> example_19.ps
echo "0 20 32 0 1 CM 2ND INTERNATIONAL" | pstext -R -JI -O -K -G255/0/0 -S0.5p >> example_19.ps
echo "0 -10 32 0 1 CM GMT CONFERENCE" | pstext -R -JI -O -K -G255/0/0 -S0.5p >> example_19.ps
echo "0 -30 18 0 1 CM Honolulu, Hawaii, April 1, 2004" | pstext -R -JI -O -G0/255/50 -S0.25p >> example_19.ps

\rm -f l*.grd l*.cpt .gmt*
