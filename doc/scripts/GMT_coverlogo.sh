#!/bin/sh
#	$Id: GMT_coverlogo.sh,v 1.1 2001-03-21 04:10:21 pwessel Exp $
#
# Creates the cover page GMT logo
#
#	Logo is 5.276" wide and 2.622" high and origin is lower left
#

dpi=`gmtget DOTS_PR_INCH`
gmtset GRID_PEN 0.25p DOTS_PR_INCH 1200
psxy -R0/1/0/1 -Jx1i -P -K -X0 -Y0 /dev/null > GMT_coverlogo.ps
gmtlogo 0 0 2.580645 >> GMT_coverlogo.ps
cat << EOF >> GMT_coverlogo.ps
%%Trailer
%%BoundingBox: 0 0 372 186
% Reset translations and scale and call showpage
S 0 showpage

end
EOF
gmtset DOTS_PR_INCH $dpi FRAME_PEN 1.2p
