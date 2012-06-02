REM		GMT EXAMPLE 06
REM
REM		$Id$
REM
REM Purpose:	Make standard and polar histograms
REM GMT progs:	pshistogram, psrose
REM DOS calls:	del
REM
echo GMT EXAMPLE 06
set ps=example_06.ps
psrose fractures.d -: -A10r -S1.8in -UL/-2.25i/-0.75i/"Example 6 in Cookbook" -P -Gorange -R0/1/0/360 -X2.5i -K -B0.2g0.2/30g30+glightblue -W1p > %ps%
pshistogram -Ba2000f1000:"Topography (m)":/a10f5:"Frequency"::,%%::."Histograms":WSne v3206.t -R-6000/0/0/30 -JX4.8i/2.4i -Gorange -O -Y5.5i -X-0.5i -L1p -Z1 -W250 >> %ps%
del .gmt*
