REM		GMT EXAMPLE 06
REM
REM		$Id: job06.bat,v 1.9 2011-03-15 02:06:31 guru Exp $
REM
REM Purpose:	Make standard and polar histograms
REM GMT progs:	pshistogram, psrose
REM DOS calls:	del
REM
echo GMT EXAMPLE 06
set ps=..\example_06.ps
psrose fractures.d -: -A10r -S1.8in -U/-2.25i/-0.75i/"Example 6 in Cookbook" -P -Gblack -R0/1/0/360 -X2.5i -K -B0.2g0.2/30g30 > %ps%
pshistogram -Ba2000f1000:"Topography (m)":/a10f5:"Frequency"::,%%::."Two types of histograms":WSne v3206.t -R-6000/0/0/30 -JX4.8i/2.4i -Ggray -O -Y5.5i -X-0.5i -Lthinner -Z1 -W250 >> %ps%
del .gmt*
