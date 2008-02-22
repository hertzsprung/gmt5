#!/bin/sh
#		GMT EXAMPLE 05
#		$Id: job05.sh,v 1.6 2008-02-22 21:10:42 remko Exp $
#
# Purpose:	Generate grid and show monochrome 3-D perspective
# GMT progs:	grdgradient, grdmath, grdview, pstext
# Unix progs:	echo, rm
#
ps=example_05.ps
grdmath -R-15/15/-15/15 -I0.3 X Y HYPOT DUP 2 MUL PI MUL 8 DIV COS EXCH NEG 10 DIV EXP MUL = \
	sombrero.grd
echo '-5 128 5 128' > gray.cpt
grdgradient sombrero.grd -A225 -Gintensity.grd -Nt0.75
grdview sombrero.grd -JX6i -JZ2i -B5/5/0.5SEwnZ -N-1/white -Qs -Iintensity.grd -X1.5i -Cgray.cpt \
	-R-15/15/-15/15/-1/1 -K -E120/30 -U/-1.25i/-0.75i/"Example 5 in Cookbook" > $ps
echo "4.1 5.5 50 0 33 BC z(r) = cos (2@~p@~r/8) * e@+-r/10@+" | pstext -R0/11/0/8.5 -Jx1i -O >> $ps
rm -f gray.cpt sombrero.grd intensity.grd .gmt*
