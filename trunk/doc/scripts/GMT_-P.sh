#!/bin/sh
#	$Id: GMT_-P.sh,v 1.2 2003-04-14 21:56:56 pwessel Exp $
#

pstext -R0/2.5/0/1.7 -Jx1 -P -K -N << EOF > GMT_-P.ps
1.125	1.525	9	0	0	CB	leading
1.125	1.4	9	0	0	CB	paper edge
0.5	0.65	10	0	1	CM	\035P
1.75	0.65	10	0	1	CM	Default
0.8	0.15	9	0	2	BL	x
0.15	1.1	9	0	2	BL	y
2.1	1.1	9	90	2	BL	x
1.45	0.15	9	90	2	BL	y
EOF
psxy -R -Jx -O -K -Sv0.1/0.2/0.2 -G100 << EOF >> GMT_-P.ps
0.5	1.35	90	0.3
1.75	1.35	90	0.3
EOF
psxy -R -Jx -O -K -Sv0.005/0.025/0.015 -G0 << EOF >> GMT_-P.ps
0.1	0.1	0	0.7
0.1	0.1	90	1
2.15	0.1	180	0.7
2.15	0.1	90	1
EOF
psxy -R -Jx -O -M -L -N -W0.5p << EOF >> GMT_-P.ps
>
0	0
1	0
1	1.3
0	1.3
>
1.25	0
2.25	0
2.25	1.3
1.25	1.3
EOF
