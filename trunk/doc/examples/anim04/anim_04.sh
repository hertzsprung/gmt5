#!/bin/sh
#               GMT ANIMATION 04
#               $Id: anim_04.sh,v 1.1 2008-06-03 01:44:49 guru Exp $
#
# Purpose:      Make DVD-res Quicktime movie of NY to Miami flight
# GMT progs:    gmtset, gmtmath, psbasemap, pstext, psxy, ps2raster
# Unix progs:   awk, mkdir, rm, mv, echo, convert, cat
#
# 1. Initialization
# 1a) Assign movie parameters
. gmt_shell_functions.sh
REGION=-Rg
altitude=160.0
tilt=55
azimuth=210
twist=0
Width=36.0
Height=34.0
px=7.2
py=4.8
dpi=100
name=`basename $0 '.sh'`

# Set up flight path
project -C-73.8333/40.75 -E-80.133/25.75 -G5 -Q > $$.path.d
cat << EOF >> $$.cities.d
-73.8333	40.75	0	5	5
-80.133		25.75	0	5	5
EOF
frame=0
mkdir -p $$
grdgradient USEast_Coast.nc -A90 -Nt1 -G$$_int.nc
makecpt -Cglobe -Z > $$.cpt
while read lon lat dist; do
	file=`gmt_set_framename $name $frame`
	ID=`echo $frame | awk '{printf "%4.4d\n", $1}'`
	grdimage ${VERBOSE} $REGION -JG${lon}/${lat}/${altitude}/${azimuth}/${tilt}/${twist}/${Width}/${Height}/7i+ \
		-P -Y0.1i -X0.1i USEast_Coast.nc -I$$_int.nc -C$$.cpt --PAPER_MEDIA=Custom_${px}ix${py}i -K > $$.ps
	psxy -R -J -O -K -W1p $$.path.d >> $$.ps
	echo 0 4.6 14 0 1 TL $ID | pstext -R0/$px/0/$py -Jx1i -O >> $$.ps
	if [ $# -eq 0 ]; then
		mv $$.ps $name.ps
		gmt_cleanup .gmt
		gmt_abort "$0: First frame plotted to $name.ps"
	fi
	ps2raster $$.ps -Tt -E$dpi
	mv $$.tif $$/$file.tif
        echo "Frame $file completed"
	frame=`gmt_set_framenext $frame`
done < $$.path.d
exit
echo "anim_04.sH: Made $frame frames at 480x720 pixels"
qt_export $$/anim_0_123456.tiff --video=h263,24,100, ${name}_movie.m4v
# 4. Clean up temporary files
gmtset DOTS_PR_INCH 300
gmt_cleanup .gmt
