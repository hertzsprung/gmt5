#!/bin/bash
#               GMT ANIMATION 04
#               $Id$
#
# Purpose:      Make DVD-res Quicktime movie of NY to Miami flight
# GMT progs:    gmt gmtset, gmt gmtmath, gmt psbasemap, gmt pstext, gmt psxy, gmt ps2raster
# Unix progs:   awk, mkdir, rm, mv, echo, qt_export, cat
# Note:         Run with any argument to build movie; otherwise 1st frame is plotted only.
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
name=anim_04
ps=${name}.ps

# Set up flight path
gmt project -C-73.8333/40.75 -E-80.133/25.75 -G5 -Q > $$.path.d
frame=0
mkdir -p frames
gmt grdgradient USEast_Coast.nc -A90 -Nt1 -Gint_$$.nc
gmt makecpt -Cglobe -Z > globe_$$.cpt
function make_frame () {
	local frame file ID lon lat dist
	frame=$1; lon=$2; lat=$3; dist=$4
	file=`gmt_set_framename ${name} ${frame}`
	ID=`echo ${frame} | $AWK '{printf "%04d\n", $1}'`
	gmt grdimage -JG${lon}/${lat}/${altitude}/${azimuth}/${tilt}/${twist}/${Width}/${Height}/7i+ \
		${REGION} -P -Y0.1i -X0.1i USEast_Coast.nc -Iint_$$.nc -Cglobe_$$.cpt \
		--PS_MEDIA=${px}ix${py}i -K > ${file}_$$.ps
	gmt psxy -JG${lon}/${lat}/${altitude}/${azimuth}/${tilt}/${twist}/${Width}/${Height}/7i+ \
		${REGION} -O -K -W1p $$.path.d >> ${file}_$$.ps
	gmt pstext -R0/${px}/0/${py} -Jx1i -F+f14p,Helvetica-Bold+jTL -O >> ${file}_$$.ps <<< "0 4.6 ${ID}"
	[[ ${frame} -eq 0 ]] && cp ${file}_$$.ps ${ps}
	if [ $# -eq 0 ]; then
		gmt_cleanup .gmt
		gmt_abort "${0}: First frame plotted to ${name}.ps"
	fi
	gmt ps2raster ${file}_$$.ps -Tt -E${dpi}
	mv ${file}_$$.tif frames/${file}.tif
	rm -f ${file}_$$.ps
	echo "Frame ${file} completed"
}
n_jobs=0
max_jobs=8
while read lon lat dist; do
	make_frame ${frame} ${lon} ${lat} ${dist} &
	((++n_jobs))
	frame=`gmt_set_framenext ${frame}`
	if [ ${n_jobs} -ge ${max_jobs} ]; then
		wait
		n_jobs=0
	fi
done < $$.path.d
wait

file=`gmt_set_framename ${name} 0`

echo "anim_04.sh: Made ${frame} frames at 480x720 pixels placed in subdirectory frames"
# GIF animate every 10th frame
${GRAPHICSMAGICK-gm} convert -delay 40 -loop 0 +dither frames/${name}_*0.tif ${name}.gif
if type -ft ${FFMPEG-ffmpeg} >/dev/null 2>&1 ; then
	# create x264 video at 25fps
	${FFMPEG-ffmpeg} -loglevel warning -y -f image2 -r 25 -i frames/${name}_%6d.tif -vcodec libx264 -pix_fmt yuv420p ${name}.mp4
fi

# 4. Clean up temporary files
gmt_cleanup .gmt

# ex: noet ts=4 sw=4
