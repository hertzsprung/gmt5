#!/bin/sh
#	$Id: GMT_-J.sh,v 1.1 2001-03-21 04:10:21 pwessel Exp $
#

pstext -R0/5/0/3 -Jx1 -P -K -W200 -C0.01/0.035 << EOF > GMT_-J.ps
0	0.9	8	0	0	BL	Mercator [C]
1.1	1.35	8	0	0	BL	Albers [E]
2	1.05	8	0	0	BL	Orthographic
3	1.35	8	0	0	BL	Eckert IV + VI [E]
4	1.35	8	0	0	BL	Linear
4	1.2	8	0	0	BL	Logarithmic
4	1.05	8	0	0	BL	Exponential
EOF

pstext -R -Jx -O -K << EOF >> GMT_-J.ps
2.5	2.8	16	0	1	BC	GMT PROJECTIONS
2	2.25	12	0	1	BC	GEOGRAPHIC PROJECTIONS
0	1.75	11	0	0	BL	CYLINDRICAL
1.1	1.75	11	0	0	BL	CONICAL
2	1.75	11	0	0	BL	AZIMUTHAL
3	1.75	11	0	0	BL	THEMATIC
4	1.75	11	0	0	BL	OTHER
#
0	1.35	8	0	0	BL	Basic [E]
0	1.2	8	0	0	BL	Cassini
0	1.05	8	0	0	BL	Equidistant
#0	0.9	8	0	0	BL	Mercator [C]
0	0.75	8	0	0	BL	Miller
0	0.6	8	0	0	BL	Oblique Mercator [C]
0	0.45	8	0	0	BL	Transverse Mercator [C]
0	0.3	8	0	0	BL	UTM [C]
#
#1.1	1.35	8	0	0	BL	Albers [E]
1.1	1.2	8	0	0	BL	Lambert [C]
1.1	1.05	8	0	0	BL	Equidistant
#
2	1.35	8	0	0	BL	Equidistant
2	1.2	8	0	0	BL	Gnomonic
#2	1.05	8	0	0	BL	Orthographic
2	0.9	8	0	0	BL	Lambert [E]
2	0.75	8	0	0	BL	Stereographic [C]
#
#3	1.35	8	0	0	BL	Eckert IV + VI [E]
3	1.2	8	0	0	BL	Hammer [E]
3	1.05	8	0	0	BL	Mollweide [E]
3	0.9	8	0	0	BL	Robinson
3	0.75	8	0	0	BL	Sinusoidal [E]
3	0.6	8	0	0	BL	Winkel Tripel
3	0.45	8	0	0	BL	Van der Grinten
#
#4	1.35	8	0	0	BL	Linear
#4	1.2	8	0	0	BL	Logarithmic
#4	1.05	8	0	0	BL	Exponential
4	0.9	8	0	0	BL	Polar
#
0.05	2.75	8	0	0	BL	C = Conformal
0.05	2.6	8	0	0	BL	E = Equal Area
EOF

psxy -R -Jx -O -K -M -W0.5p << EOF >> GMT_-J.ps
>
2.3	2.75
2	2.4
>
2.7	2.75
4.2	1.9
>
1.7	2.2
0.2	1.9
>
1.9	2.2
1.3	1.9
>
2.1	2.2
2.2	1.9
>
2.3	2.2
3.2	1.9
>
0.2	1.7
0.2	1.5
>
1.3	1.7
1.3	1.5
>
2.2	1.7
2.2	1.5
>
3.2	1.7
3.2	1.5
>
4.2	1.7
4.2	1.5
>
0	2.55
0.85	2.55
0.85	2.87
0	2.87
0	2.55
EOF
