#!/bin/bash
#		GMT EXAMPLE 28
#		$Id$
#
# Purpose:	Illustrates how to mix UTM data and UTM projection
# GMT progs:	makecpt, grdgradient, grdimage, grdinfo, grdmath, pscoast, pstext, mapproject
# Unix progs:	rm, echo
#
ps=example_28.ps

# Get intensity grid and set up a color table
grdgradient Kilauea.utm.nc -Nt1 -A45 -GKilauea.utm_i.nc
makecpt -Ccopper -T0/1500/100 -Z > Kilauea.cpt
# Lay down the UTM topo grid using a 1:16,000 scale
grdimage Kilauea.utm.nc -IKilauea.utm_i.nc -CKilauea.cpt -Jx1:160000 -P -K \
	-U"Example 28 in Cookbook" --FORMAT_FLOAT_OUT=%.10g --FONT_ANNOT_PRIMARY=9p \
	> $ps
# Overlay geographic data and coregister by using correct region and projection with the same scale
pscoast -RKilauea.utm.nc -Ju5Q/1:160000 -O -K -Df+ -Slightblue -W0.5p -B5mg5mNE \
	--FONT_ANNOT_PRIMARY=12p --FORMAT_GEO_MAP=ddd:mmF >> $ps
echo 155:16:20W 19:26:20N KILAUEA | pstext -R -J -O -K -F+f12p,Helvetica-Bold+jCB >> $ps
psbasemap -R -J -O -K --FONT_ANNOT_PRIMARY=9p -Lf155:07:30W/19:15:40N/19:23N/5k+l1:16,000+u \
	--FONT_LABEL=10p >> $ps
# Annotate in km but append ,000m to annotations to get customized meter labels
grdmath Kilauea.utm.nc = Kilauea.utm.km.nc+uk
psbasemap -RKilauea.utm.km.nc -Jx1:160 -B5g5:,"-@:8:000m":WSne -O --FONT_ANNOT_PRIMARY=10p \
	--MAP_GRID_CROSS_SIZE_PRIMARY=0.1i --FONT_LABEL=10p >> $ps
# Clean up
rm -f Kilauea.utm_i.nc Kilauea.cpt tmp.txt Kilauea.utm.km.nc
