REM		GMT EXAMPLE 06
REM
REM		$Id: job06.bat,v 1.1.1.1 2000-12-28 01:23:45 gmt Exp $
REM
REM Purpose:	Make standard and polar histograms
REM GMT progs:	pshistogram, psrose
REM DOS calls:	del
REM
echo GMT EXAMPLE 06
set master=y
if exist job06.bat set master=n
if %master%==y cd ex06
psrose fractures.d -A10r -S1.8in -U/-2.25i/-0.75i/"Example 6 in Cookbook" -P -G0 -R0/1/0/360 -X2.5i -K -B0.2g0.2/30g30 > example_06.ps
pshistogram -Ba2000f1000:"Topography (m)":/a10f5:"Frequency"::,%%::."Two types of histograms":WSne v3206.t -R-6000/0/0/30 -JX4.8i/2.4i -G200 -O -Y5.5i -X-0.5i -L0.5p -Z1 -W250 >> example_06.ps
del .gmt*
if %master%==y cd ..
