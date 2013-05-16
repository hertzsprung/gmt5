#!/bin/bash
#		GMT EXAMPLE 06
#		$Id$
#
# Purpose:	Make standard and polar histograms
# GMT progs:	pshistogram, psrose
# Unix progs:	rm
#
ps=example_06.ps
psrose fractures.d -: -A10r -S1.8in -UL/-2.25i/-0.75i/"Example 6 in Cookbook" -P -Gorange \
	-R0/1/0/360 -X2.5i -K -Bx0.2g0.2 -By30g30 -B+glightblue -W1p > $ps
pshistogram -Bxa2000f1000+l"Topography (m)" -Bya10f5+l"Frequency"+s" %" -BWSne+t"Histograms"+glightblue \
	v3206.t -R-6000/0/0/30 -JX4.8i/2.4i -Gorange -O -Y5.5i -X-0.5i -L1p -Z1 -W250 >> $ps
