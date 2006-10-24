#!/bin/sh
#	$Id: GMT_coverlogo.sh,v 1.7 2006-10-24 01:53:19 remko Exp $
#
# Creates the cover page GMT logo
#
#	Logo is 5.276" wide and 2.622" high and origin is lower left
#

dpi=`gmtget DOTS_PR_INCH`
gmtset GRID_PEN_PRIMARY thinnest DOTS_PR_INCH 1200
psxy -R0/1/0/1 -Jx1i -P -K -X0 -Y0.15 /dev/null > GMT_coverlogo.ps
gmtlogo 0 0 2.580645 >> GMT_coverlogo.ps
cat << EOF >> GMT_coverlogo.ps
%%Trailer
%%BoundingBox: 0 0 372 186
% Reset translations and scale and call showpage
S 0 showpage

end
EOF
gmtset DOTS_PR_INCH $dpi FRAME_PEN thick
