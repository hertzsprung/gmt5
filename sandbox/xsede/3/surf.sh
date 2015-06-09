#!/bin/sh
# Grid topo_${inc}.xyz using minimum curvature cubic splines
# First we solve problem using surface default settings.
# Next we tighten the convergence limit to 1 cm and allow
# up to 5000 iterations per intermediate grid.
verbose=-V	# Comment this out for a quiet run
inc=2m		# Use the ~1000 points 2m data set
#inc=1m		# Use the ~5000 points 1m data set
ps=surf.ps	# Output PS name
e=`gmt math -Q 34 42 ADD 2 DIV COSD =`	# Set aspect ratio at this latitude
# Request time stamps with elapsed time in session
gmt set TIME_REPORT elapsed
# Run surface using default settings
gmt surface ../topo_${inc}.xyz -R80W/64W/34N/42N -I${inc} -fg -A$e $verbose -Gtopo_surf_1.nc
# Get data gradients to simulate illuminations
gmt grdgradient topo_surf_1.nc -A45 -Nt1 -Gtopo_surf_1_int.nc
# Get a rainbow color table scaled to data range
gmt makecpt -Crainbow -T-5400/1500/500 -Z > t.cpt
# Plot gridded result
gmt grdimage topo_surf_1.nc -Itopo_surf_1_int.nc -Ct.cpt -JM6i -P -Baf -BWSne -K -X1i -U"Default run" > $ps
# Overlay coastline
gmt pscoast -Rtopo_surf_1.nc -J -O -K -W0.25p -Dh >> $ps
# Run surface with tighter settings for convergence
gmt surface ../topo_${inc}.xyz -R80W/64W/34N/42N -I${inc} -fg -A$e -C0.01 -N5000 -Z1.4 $verbose -Gtopo_surf_2.nc
# Get data gradients to simulate illuminations
gmt grdgradient topo_surf_2.nc -A45 -Nt1 -Gtopo_surf_2_int.nc
# Plot gridded result
gmt grdimage topo_surf_2.nc -Itopo_surf_2_int.nc -Ct.cpt -J -Baf -BWSne -O -K -Y6i -U"1 cm convergence" >> $ps
# Overlay coastline
gmt pscoast -R -J -O -K -W0.25p -Dh >> $ps
# Place color bar
gmt psscale -Ct.cpt -D6.2i/-1.1i/9i/0.1i -Baf -O >> $ps
# Convert to High-Res PDF
gmt psconvert -Tf -P $ps
# Clean up
rm -f topo_surf_1_int.nc topo_surf_2_int.nc t.cpt gmt.conf gmt.history $ps
rm -f topo_surf_1.nc topo_surf_2.nc
echo "surf.sh: Finished, see surf.pdf"
# open surf.pdf
