REM		GMT EXAMPLE 16
REM
REM		$Id: job16.bat,v 1.4 2004-01-10 02:34:54 pwessel Exp $
REM
REM Purpose:	Illustrates interpolation methods using same data as Example 12.
REM GMT progs:	gmtset, grdview, grdfilter, pscontour, psscale, pstext, surface, triangulate
REM DOS calls:	echo, del
REM
REM First make a cpt file as in example 12:
REM
echo GMT EXAMPLE 16
set master=y
if exist job16.bat set master=n
if %master%==y cd ex16
REM makecpt -Crainbow -T675/975/25 to make raw ex16.cpt
REM Hand edit to add patterns and skips
REM
REM Now illustrate various means of contouring, using triangulate and surface.
REM
gmtset ANNOT_FONT_SIZE 9
REM
pscontour -R0/6.5/-0.2/6.5 -Jx0.45i -P -K -Y5.5i -Ba2f1WSne table_5.11 -Cex16.cpt -I > example_16.ps
echo 3.25 7 18 0 4 CB pscontour (triangulate) | pstext -R -J -O -K -N >> example_16.ps
REM
surface table_5.11 -R -I0.1 -Graws0.grd
grdview raws0.grd -R -J -Ba2f1WSne -Cex16.cpt -Qs -O -K -X3.5i >> example_16.ps
echo 3.25 7 18 0 4 CB surface (tension = 0) | pstext -R -J -O -K -N >> example_16.ps
REM
surface table_5.11 -R -I0.1 -Graws5.grd -T0.5
grdview raws5.grd -R -J -Ba2f1WSne -Cex16.cpt -Qs -O -K -Y-3.75i -X-3.5i >> example_16.ps
echo 3.25 7 18 0 4 CB surface (tension = 0.5) | pstext -R -J -O -K -N >> example_16.ps
REM
triangulate table_5.11 -Grawt.grd -R -I0.1 > NUL
grdfilter rawt.grd -Gfiltered.grd -D0 -Fc1
grdview filtered.grd -R -J -Ba2f1WSne -Cex16.cpt -Qs -O -K -X3.5i >> example_16.ps
echo 3.25 7 18 0 4 CB triangulate @~\256@~ grdfilter | pstext -R -J -O -K -N >> example_16.ps
echo 3.2125 7.5 32 0 4 CB Gridding of Data | pstext -R0/10/0/10 -Jx1i -O -K -N -X-3.5i >> example_16.ps
psscale -D3.25i/0.35i/5i/0.25ih -Cex16.cpt -O -U"Example 16 in Cookbook" -Y-0.75i >> example_16.ps
del *.grd
del .gmt*
if %master%==y cd ..
