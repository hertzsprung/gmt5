#!/bin/sh
#	$Id: GMT_-R.sh,v 1.6 2004-04-13 21:32:27 pwessel Exp $
#

gmtset BASEMAP_TYPE PLAIN ANNOT_FONT_PRIMARY 0 ANNOT_FONT_SIZE_PRIMARY 8 TICK_LENGTH 0.05i \
	PLOT_DEGREE_FORMAT ddd:mm:ss CHAR_ENCODING ISOLatin1+
pscoast -R-90/-70/18/35.819 -JM2 -P -Dl -Glightgray -W1 -Ba10g5WSEn -K > GMT_-R.ps
pstext -R0/2/-0.5/2 -Jx1 -O -K -N -Y-0.5 << EOF >> GMT_-R.ps
1	-0.375	9	0	2	CT	@%0%a)@%% @%1%\035R@%%xmin/xmax/ymin/ymax
EOF
psxy -R -Jx -O -K -N -Sc0.1 -W0.5p << EOF >> GMT_-R.ps
0	1
1	0
1	2
2	1
EOF
psxy -R -Jx -O -K -N -M -W0.5p << EOF >> GMT_-R.ps
>
0	1
0.675	-0.35
>
1	0
1.28	-0.35
>
1	2
1.67	-0.35
>
2	1
1.05	-0.35
EOF
#
pscoast -R-90/20/-65.5327/29.4248r -JOc280/20/22/69/2 -O -Dl -Glightgray -W1 -Ba10g5WSEn -K -X2.75 -Y0.5 >> GMT_-R.ps
pstext -R0/2/-0.5/2 -Jx1 -O -K -N -Y-0.5 << EOF >> GMT_-R.ps
1	-0.375	9	0	2	CT	@%0%b)@%% @%1%\035R@%%xlleft/ylleft/xuright/yuright @%1%r@%%
EOF
psxy -R -Jx -O -K -N -Sc0.1 -W0.5p << EOF >> GMT_-R.ps
0	0
2	2
EOF
psxy -R -Jx -O -N -M -W0.5p << EOF >> GMT_-R.ps
>
0	0
0.56	-0.35
>
0	0
0.87	-0.35
>
2	2
1.2	-0.35
>
2	2
1.63	-0.35
EOF

