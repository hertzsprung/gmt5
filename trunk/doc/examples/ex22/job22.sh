#!/bin/sh
#
#	GMT Example 22  $Id: job22.sh,v 1.2 2004-04-25 21:23:23 pwessel Exp $
#
# Purpose:	Automatic map of last 7 days of worldwide seismicity
# GMT progs:	gmtset, pscoast, psxy, pslegend
# Unix progs:	cat, sed, awk, wget
#
gmtset ANNOT_FONT_SIZE 10p HEADER_FONT_SIZE 18p PLOT_DEGREE_FORMAT ddd:mm:ssF

# Get the data (-q quietly) from USGS using the wget (comment out in case
# your system does not have wget)

#wget http://neic.usgs.gov/neis/gis/bulletin.asc -q -O neic_quakes.d

# Count the number of events (to be used in title later. one less due to header)

n=`cat neic_quakes.d | wc -l`
n=`expr $n - 1`

# Pull out the first and last timestamp to use in legend title

first=`sed -n 2p neic_quakes.d | awk -F, '{printf "%s %s\n", $1, $2}'`
last=`sed -n '$p' neic_quakes.d | awk -F, '{printf "%s %s\n", $1, $2}'`

# Assign a string that contains the current user @ the current computer node.
# Note that two @@ is needed to print a single @ in pstext:

#set me = "$user@@`hostname`"
me="GMT guru @@ GMTbox"

# Create standard seismicity color table

cat << EOF > neis.cpt
0	red	100	red
100	green	300	green
300	blue	10000	blue
EOF

# Start plotting. First lay down map, then plot quakes with size = magintude/50":

pscoast -Rg -JK180/9i -B45g30:."Worldwide earthquake activity": -Glightbrown -Slightblue \
  -Dc -A1000 -K -U/-0.75i/-2.5i/"Example 22 in Cookbook" -Y2.75i > example_22.ps
awk -F, '{ print $4, $3, $6, $5*0.02}' neic_quakes.d | psxy -R -JK -O -K -Cneis.cpt -Sci -Wthin -H >> example_22.ps

# Create legend input file for NEIS quake plot

cat << EOF > neis.legend
H 16 1 $n events during $first to $last
D 0 1p
N 3
V 0 1p
S 0.1i c 0.1i red 0.25p 0.2i Shallow depth (0-100 km)
S 0.1i c 0.1i green 0.25p 0.2i Intermediate depth (100-300 km)
S 0.1i c 0.1i blue 0.25p 0.2i Very deep (> 300 km)
V 0 1p
D 0 1p
N 7
V 0 1p
S 0.1i c 0.06i - 0.25p 0.3i M 3
S 0.1i c 0.08i - 0.25p 0.3i M 4
S 0.1i c 0.10i - 0.25p 0.3i M 5
S 0.1i c 0.12i - 0.25p 0.3i M 6
S 0.1i c 0.14i - 0.25p 0.3i M 7
S 0.1i c 0.16i - 0.25p 0.3i M 8
S 0.1i c 0.18i - 0.25p 0.3i M 9
V 0 1p
D 0 1p
N 1
>
EOF

# Put together a reasonable legend text, and add logo and user's name:

cat << EOF >> neis.legend
T USGS/NEIS most recent earthquakes for the last seven days.  The data were
T obtained automatically from the USGS Earthquake Hazards Program page at
T @_http://neic/usgs.gov @_.  Interested users may also receive email alerts
T from the USGS.
T This script can be called daily to update the latest information.
G 0.4i
# Add USGS logo
I USGS.ras 1i RT
G -0.3i
L 12 6 LB $me
EOF

# OK, now we can actually run pslegend.  We center the legend below the map.
# Trial and error shows that 1.7i is a good legend height:

pslegend -Dx4.5i/-0.4i/7i/1.7i/TC -Jx1i -R0/8/0/8 -O -F neis.legend -Glightyellow >> example_22.ps

# Clean up after ourselves:

rm -f neis.* .gmtcommands4 .gmtdefaults4
