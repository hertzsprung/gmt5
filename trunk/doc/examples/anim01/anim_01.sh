#!/bin/sh
#               GMT ANIMATION 01
#               $Id: anim_01.sh,v 1.1 2008-05-30 04:27:08 guru Exp $
#
# Purpose:      Make web page with simple animated GIF of sine function
# GMT progs:    gmtset, gmtmath, psbasemap, pstext, psxy, ps2raster
# Unix progs:   awk, mkdir, rm, mv, echo, convert, cat
#
# 1. Initialization
# 1a) Assign movie parameters
. gmt_shell_functions.sh
width=4i
height=2i
dpi=125
n_frames=18
name=`basename $0 '.sh'`
# 1b) Do frame-independent calculations and setup
angle_step=`gmtmath -Q 360 $n_frames DIV =`
angle_inc=`gmtmath -Q $angle_step 10 DIV =`
gmtset DOTS_PR_INCH $dpi
psbasemap -R0/360/-1.2/1.6 -JX3.5i/1.65i -P -K -X0.35i -Y0.25i \
	-Ba90g90f30:,-\\312:/a0.5f0.1g1WSne -Glightgreen \
	--PAPER_MEDIA=Custom_${width}x${height} --ANNOT_FONT_SIZE=+9p > $$.map.ps
# 2. Main frame loop
mkdir -p $$
frame=0
while [ $frame -le $n_frames ]; do
	# Create file name using a name_##.tif format
	file=`gmt_set_framename $name $frame`
	cp -f $$.map.ps $$.ps
	angle=`gmtmath -Q $frame $angle_step MUL =`
	if [ $frame -gt 0 ]; then	# First plot has no curves
#		Plot smooth blue curve and dark red dots at all angle steps so far
		gmtmath -T0/$angle/$angle_inc T SIND = $$.sin.d
		psxy -R -J -O -K -W1p,blue $$.sin.d >> $$.ps
		gmtmath -T0/$angle/$angle_step T SIND = $$.sin.d
		psxy -R -J -O -K -Sc0.1i -Gdarkred $$.sin.d >> $$.ps
	fi
	#	Plot red dot at current angle and annotate
	sin=`gmtmath -Q $angle SIND =`
	echo $angle $sin | psxy -R -J -O -K -Sc0.1i -Gred >> $$.ps
	echo $angle | awk '{printf "0 1.6 14 0 1 LT a = %3.3d\n", $1}' \
		| pstext -R -J -O -K -N -Dj0.1i/0.05i >> $$.ps
	psxy -R -J -O /dev/null >> $$.ps
	if [ $# -eq 0 ]; then
		mv $$.ps $name.ps
		gmt_cleanup .gmt
		gmt_abort "$0: First frame plotted to $name.ps"
	fi
#	RIP to TIFF at specified dpi
	ps2raster -E$dpi -Tt $$.ps
	mv -f $$.tif $$/$file.tif
	echo "Frame $file completed"
	frame=`gmt_set_framenext $frame`
done
# 3. Create animated GIF file and HTML for web page
convert -delay 20 -loop 0 $$/*.tif $name.gif
cat << EOF > $name.html
<HTML>
<TITLE>GMT Trigonometry: The sine movie</TITLE>
<BODY bgcolor="#ffffff">
<CENTER>
<H1>GMT Trigonometry: The sine movie</H1>
<IMG src="$name.gif">
</CENTER>
<HR>
We demonstrate how the sine function <I>y = sin(a)</I> varies with <I>a</I> over
the full 360-degree interval.  We plot a bright red circle at each
new angle, letting previous circles turn dark red.  The underlying
sine curve is sampled at 10 times the frame sampling rate in order to reproduce
a smooth curve.  Our animation uses Imagemagick's convert tool to make an animated GIF file
with a 0.2 second pause between frames, set to repeat forever.
<HR>
<I>$name.sh: Created by $USER on `date`</I>
</BODY>
</HTML>
EOF
# 4. Clean up temporary files
gmtset DOTS_PR_INCH 300
gmt_cleanup .gmt
