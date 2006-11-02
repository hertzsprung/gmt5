REM		GMT EXAMPLE 08
REM
REM		$Id: job08.bat,v 1.11 2006-11-02 19:42:41 pwessel Exp $
REM
REM Purpose:	Make a 3-D bar plot
REM GMT progs:	grd2xyz, pstext, psxyz
REM DOS calls:	echo, del
REM
echo GMT EXAMPLE 08
set master=y
if exist job08.bat set master=n
if %master%==y cd ex08
grd2xyz guinea_bay.nc > tmp.txt
psxyz tmp.txt -B1/1/1000:"Topography (m)"::.ETOPO5:WSneZ+ -R-0.1/5.1/-0.1/5.1/-5000/0 -JM5i -JZ6i -E200/30 -So0.0833333ub-5000 -P -U"Example 8 in Cookbook" -Wthinnest -Glightgray -K > example_08.ps
echo 0.1 4.9 24 0 1 9 This is the surface of cube | pstext -R -J -JZ -Z0 -E200/30 -O >> example_08.ps
del .gmt*
del tmp.txt
if %master%==y cd ..
