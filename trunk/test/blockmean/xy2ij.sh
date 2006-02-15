#!/bin/sh
#	$Id: xy2ij.sh,v 1.1 2006-02-15 23:47:41 pwessel Exp $
#
# Test to make sure the (x,y) <--> (i,j) macros work correctly
# We use -R0/5/0/5 -I1 for pixel and gridline registrations
# Points are prepared that will land in various tiles, some will
# fall exactly on tile boundaries.  One point will fall outside -R
# but should be considered for the gridline registrated case.
cat << EOF > pixel.d
0.65	0.7	1
1.33	1.8	2
0.25	2.25	3
0.69	3.33	4
2.0	3.3	5
4.67	4.0	6
3.0	2.0	7
5.05	3.35	8
3.0	1.3	9
EOF
# blockman uses GMT_x|y_to_i|j to determine which tiles
# Using -C gives tile centers via (i,j) -> (x,y)
blockmean -R0/5/0/5 -I1 -F -C pixel.d > pixel_ij.d
blockmean -R0/5/0/5 -I1 -F pixel.d > pixel_xy.d
# Connect the original point and the corresponding tile center
paste pixel_xy.d pixel_ij.d | awk '{if (NF == 6) printf ">\n%s\t%s\n%s\t%s\n", $1, $2, $4, $5}' \
	| psxy -R -JX4.5 -P -B1g1WSne -Wdefault -K -M -Y0.5i -X2i > xy2ij.ps
# Plot and label the points
psxy -R -J pixel.d -Sc0.125 -Gwhite -Wfaint -O -K -N >> xy2ij.ps
awk '{print $1, $2, 8, 0, 0, "CM", $3}' pixel.d | pstext -R -J -O -K -N >> xy2ij.ps
psxy -R -J pixel_ij.d -Sc0.15 -Gblack -O -K -N >> xy2ij.ps
awk '{print $1, $2, 8, 0, 0, "CM", $3}' pixel_ij.d | pstext -R -J -O -K -Gwhite -N >> xy2ij.ps
rm -f pixel*
# Do gridline registration with a similar data set (mostly offset by 0.5)
cat << EOF > grid.d
1.15	0.2	1
1.83	1.3	2
0.75	2.75	3
1.19	3.83	4
2.5	3.8	5
4.167	3.5	6
3.5	2.5	7
5.05	3.35	8
1.5	2.8	9
EOF
blockmean -R0/5/0/5 -I1 -C grid.d > grid_ij.d
blockmean -R0/5/0/5 -I1 grid.d > grid_xy.d
psbasemap -R0.5/5.5/0.5/5.5 -J -O -B0g1 -K -Y4.7 >> xy2ij.ps
paste grid_xy.d grid_ij.d | awk '{if (NF == 6) printf ">\n%s\t%s\n%s\t%s\n", $1, $2, $4, $5}' \
	| psxy -R0/5/0/5 -J -O -B1Wsne -Wdefault -K -M >> xy2ij.ps
psxy -R -J grid.d -Sc0.125 -Gwhite -Wfaint -O -K -N >> xy2ij.ps
awk '{print $1, $2, 8, 0, 0, "CM", $3}' grid.d | pstext -R -J -O -K -N >> xy2ij.ps
psxy -R -J grid_ij.d -Sc0.15 -Gblack -O -K -N >> xy2ij.ps
awk '{print $1, $2, 8, 0, 0, "CM", $3}' grid_ij.d | pstext -R -J -O -K -Gwhite -N >> xy2ij.ps
psxy -R -J -O /dev/null >> xy2ij.ps
rm -f grid*
gv xy2ij.ps &
