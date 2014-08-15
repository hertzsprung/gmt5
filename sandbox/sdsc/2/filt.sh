#!/bin/sh
# Filter topo.nc using both convolution and spatial filters
# First we use a 100 km diameter Gaussian filter.
# Next we use a 100 km diameter median filter.
ps=filt.ps	# Output PS name
# Smooth the topography using a 100 km diameter Gaussian filter
gmt grdfilter topo.nc -Fg100k -D2 -I2m -V -Gtopo_filt_g.nc
# Get data gradients to simulate illuminations
gmt grdgradient topo_filt_g.nc -A45 -Nt1 -Gtopo_filt_g_int.nc
# Get a rainbow color table scaled to data range
gmt makecpt -Crainbow -T-5400/1500/500 -Z > t.cpt
# Plot filtered result
gmt grdimage topo_filt_g.nc -Itopo_filt_g_int.nc -Ct.cpt -JM6i -P -Baf -BWSne -K -X1i > $ps
# Overlay coastline
gmt pscoast -R -J -O -K -W0.25p -Dh >> $ps
# Smooth the topography using a 100 km diameter median filter
gmt grdfilter topo.nc -Fm100k -D2 -I2m -V -Gtopo_filt_m.nc
# Get data gradients to simulate illuminations
gmt grdgradient topo_filt_m.nc -A45 -Nt1 -Gtopo_filt_m_int.nc
# Plot filtered result
gmt grdimage topo_filt_m.nc -Itopo_filt_m_int.nc -Ct.cpt -J -Baf -BWSne -O -K -Y5i >> $ps
# Overlay coastline
gmt pscoast -R -J -O -K -W0.25p -Dh >> $ps
# Place color bar
gmt psscale -Ct.cpt -D6.2i/4.5i/8i/0.1i -Baf -O -Y-5i >> $ps
# Convert to High-Res PDF
gmt ps2raster -Tf -P $ps
# Clean up
rm -f topo_filt_g_int.nc topo_filt_m_int.nc t.cpt $ps
rm -f topo_filt_g.nc topo_filt_m.nc
echo "filt.sh: Finished, see filt.pdf"
open filt.pdf
