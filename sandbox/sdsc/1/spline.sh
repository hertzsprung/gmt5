#!/bin/sh
# Grid topo.xyz using Green's functions for cubic splines
# First we solve the exact interpolation.  Next we solve
# via SVD and select enough eigenvalues to explain 99.99%
# of data variance. This exercises both our Gauss-Jordan
# solver and our SVD decomposition
ps=spline.ps	# Output PS file
# Do exact cubic spline interpolation
gmt greenspline ../topo.xyz -R80W/64W/34N/42N -I2m -fg -D2 -Sc -V -Gtopo_spline_gj.nc
# Get data gradients to simulate illuminations
gmt grdgradient topo_spline_gj.nc -A45 -Nt1 -Gtopo_spline_gj_int.nc
# Get a rainbow color table scaled to data range
gmt makecpt -Crainbow -T-5400/1500/500 -Z > t.cpt
# Plot splined result
gmt grdimage topo_spline_gj.nc -Itopo_spline_gj_int.nc -Ct.cpt -JM6i -P -Baf -BWSne -K -X1i > $ps
# Overlay coastline
gmt pscoast -R -J -O -K -W0.25p -Dh >> $ps
# Do approximate cubic spline interpolation
gmt greenspline ../topo.xyz -R80W/64W/34N/42N -I2m -fg -D2 -Sc -Cv99.99 -V -Gtopo_spline_svd.nc
# Get data gradients to simulate illuminations
gmt grdgradient topo_spline_svd.nc -A45 -Nt1 -Gtopo_spline_svd_int.nc
# Plot splined result
gmt grdimage topo_spline_svd.nc -Itopo_spline_svd_int.nc -Ct.cpt -J -Baf -BWSne -O -K -Y5i >> $ps
# Overlay coastline
gmt pscoast -R -J -O -K -W0.25p -Dh >> $ps
# Place color bar
gmt psscale -Ct.cpt -D6.2i/4.5i/8i/0.1i -Baf -O -Y-5i >> $ps
# Convert to High-Res PDF
gmt ps2raster -Tf -P $ps
# Clean up
rm -f topo_spline_gj_int.nc topo_spline_svd_int.nc t.cpt $ps
rm -f topo_spline_gj.nc topo_spline_svd.nc
echo "spline.sh: Finished, see spline.pdf"
open spline.pdf
