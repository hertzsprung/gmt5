REM		GMT EXAMPLE 15
REM
REM		$Id: job15.bat,v 1.5 2004-09-29 01:29:02 pwessel Exp $
REM
REM Purpose:	Gridding and clipping when data are missing
REM GMT progs:	blockmedian, gmtconvert, grdclip, grdcontour, grdinfo, minmax,
REM		nearneighbor, pscoast, psmask, pstext, surface
REM DOS calls:	echo, del, gawk
REM
echo GMT EXAMPLE 15
set master=y
if exist job15.bat set master=n
if %master%==y cd ex15
gmtconvert ship.xyz -bod > ship.b
set region=-R245/255/20/30
nearneighbor %region% -I10m -S40k -Gship.grd ship.b -bi3
grdinfo -C -M ship.grd | gawk "{print $12, $13}" > tmp
grdcontour ship.grd -JM3i -P -B2WSne -C250 -A1000 -G2i -K -U"Example 15 in Cookbook" > example_15.ps
REM
blockmedian %region% -I10m ship.b -bi3 -bod > ship_10m.b
surface %region% -I10m ship_10m.b -Gship.grd -bi3
psmask %region% -I10m ship.b -J -O -K -T -Glightgray -bi3 -X3.6i >> example_15.ps
grdcontour ship.grd -J -B2WSne -C250 -A1000 -L-8000/0 -G2i -O -K >> example_15.ps
REM
psmask %region% -I10m ship_10m.b -bi3 -J -B2WSne -O -K -X-3.6i -Y3.75i >> example_15.ps
grdcontour ship.grd -J -C250 -A1000 -G2i -L-8000/0 -O -K >> example_15.ps
psmask -C -O -K >> example_15.ps
REM
grdclip ship.grd -Sa-1/NaN -Gship_clipped.grd
grdcontour ship_clipped.grd -J -B2WSne -C250 -A1000 -L-8000/0 -G2i -O -K -X3.6i >> example_15.ps
pscoast %region% -J -O -K -Ggray -W0.25p >> example_15.ps
psxy tmp -R -J -O -K -Sa0.15i -W1p >> example_15.ps
echo -0.3 3.6 24 0 1 CB Gridding with missing data | pstext -R0/3/0/4 -Jx1i -O -N >> example_15.ps
del ship*.grd
del ship*.b
del .gmt*
del tmp
if %master%==y cd ..
