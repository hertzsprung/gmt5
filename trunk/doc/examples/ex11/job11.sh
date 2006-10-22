#!/bin/sh
#		GMT EXAMPLE 11
#
#		$Id: job11.sh,v 1.8 2006-10-22 14:26:49 remko Exp $
#
# Purpose:	Create a 3-D RGB Cube
# GMT progs:	gmtset, grdimage, grdmath, pstext, psxy
# Unix progs:	$AWK, rm
#
# First create a Plane from (0,0,0) to (255,255,255).
# Only needs to be done once, and is used on each of the 6 faces of the cube.
#

grdmath -I1 -R0/255/0/255 Y 256 MUL X ADD = rgb_cube.grd

#
# For each of the 6 faces, create a color palette with one color (r,g,b) fixed
# at either the min. of 0 or max. of 255, and the other two components
# varying smoothly across the face from 0 to 255.
#
# This uses $AWK script "rgb_cube.awk", with arguments specifying which color
# (r,g,b) is held constant at 0 or 255, which color varies in the x-direction
# of the face, and which color varies in the y-direction.  If the color is to
# increase in x (y), a lower case x (y) is indicated; if the color is to 
# decrease in the x (y) direction, an upper case X (Y) is used.
#
# Use grdimage to paint the faces and psxy to add "cut-along-the-dotted" lines.
#

gmtset TICK_LENGTH 0 COLOR_MODEL rgb

pstext -R0/8/0/11 -Jx1i < /dev/null -P -U"Example 11 in Cookbook" -K > example_11.ps
$AWK -f rgb_cube.awk r=x g=y b=255 < /dev/null > rgb_cube.cpt
grdimage rgb_cube.grd -Crgb_cube.cpt -JX2.5i/2.5i -R0/255/0/255 -K -O -X2i -Y4.5i -B256wesn >> example_11.ps

$AWK -f rgb_cube.awk r=255 g=y b=X < /dev/null > rgb_cube.cpt
grdimage rgb_cube.grd -Crgb_cube.cpt -J -K -O -X2.5i -B256wesn >> example_11.ps

$AWK -f rgb_cube.awk r=x g=255 b=Y < /dev/null > rgb_cube.cpt
grdimage rgb_cube.grd -Crgb_cube.cpt -J -K -O -X-2.5i -Y2.5i -B256wesn >> example_11.ps

psxy -Wthinnest,. -J -R -K -O -X2.5i << END >> example_11.ps
0 0
20 20
20 235
0 255
END

psxy -Wthinnest,. -J -R -K -O -X-2.5i -Y2.5i << END >> example_11.ps
0 0
20 20
235 20
255 0
END

psxy -Wthinnest,. -J -R -K -O -X-2.5i -Y-2.5i << END >> example_11.ps
255 0
235 20
235 235
255 255
END

$AWK -f rgb_cube.awk r=0 g=y b=x < /dev/null > rgb_cube.cpt
grdimage rgb_cube.grd -Crgb_cube.cpt -J -K -O -Y-2.5i -B256wesn >> example_11.ps

$AWK -f rgb_cube.awk r=x g=0 b=y < /dev/null > rgb_cube.cpt
grdimage rgb_cube.grd -Crgb_cube.cpt -J -K -O -X2.5i -Y-2.5i -B256wesn >> example_11.ps


echo "10 10 14 0 Times-BoldItalic BL GMT 4" | pstext -J -R -Gwhite -K -O >> example_11.ps

psxy -Wthinnest,. -J -R -K -O -X2.5i << END >> example_11.ps
0 0
20 20
20 235
0 255
END

psxy -Wthinnest,. -J -R -K -O -X-5i << END >> example_11.ps
255 0
235 20
235 235
255 255
END

$AWK -f rgb_cube.awk r=x g=Y b=0 < /dev/null > rgb_cube.cpt
grdimage rgb_cube.grd -Crgb_cube.cpt -J -K -O -X2.5i -Y-2.5i -B256wesn >> example_11.ps

psxy -Wthinnest,. -J -R -K -O -X2.5i << END >> example_11.ps
0 0
20 20
20 235
0 255
END

psxy -Wthinnest,. -J -R -O -X-5i << END >> example_11.ps
255 0
235 20
235 235
255 255
END

rm -f rgb_cube.cpt rgb_cube.grd .gmtcommands4 .gmtdefaults4
