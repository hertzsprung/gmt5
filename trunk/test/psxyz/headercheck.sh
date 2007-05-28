#!/bin/sh
#	$Id: headercheck.sh,v 1.4 2007-05-28 23:25:36 pwessel Exp $
# Test that symbols pick up correct -W -G from command line or header

echo -n "$0: Test psxyz and operation of -W -G in headers:		"

psxyz -R-1/10/-1/10/0/1 -JX5/4 -JZ1 -E135/35 -P -B2g1 -Sc0.2i -Gyellow -W2.5p,cyan -M -K << EOF > headercheck.ps
> -Ggreen -W1p
0	0	0
1	1	0
> -Gred	-W-
2	2	0
3	3	0
> -Gblue -W+
4	4	0
5	5	0
> -G-
6	6	0
7	7	0
> -G+	-Wthin,-
8	8	0
9	9	0
EOF
#
# Now test that lines/polygons are OK
cat << EOF > $$.cpt
3	p100/9	6	-
6	cyan	9	yellow
EOF
psxyz -R -J -JZ -E135/35 -O -Y4.25i -Gred -L -M -B2g1 -C$$.cpt << EOF >> headercheck.ps
> -Ggreen -W+
0	0	0
2	2	0
0	2	0
> -G- -W1p,blue
2	0	0
4	4	0
3	3	0
2	0	0
> -G+ -W1p,-
0	4	0
3	6	0
2	7	0
> -Gp100/32 -W-
5	0	0
6	5	0
4	3	0
> -W- -Z8
7	4	0
9	5	0
8	7	0
> -W1p -Z5
8	7	0
9	9	0
6	9	0
EOF
compare -density 100 -metric PSNR headercheck_orig.ps headercheck.ps headercheck_diff.png > log
grep inf log > fail
if [ ! -s fail ]; then
        echo "[FAIL]"
else
        echo "[PASS]"
        rm -f fail headercheck_diff.png log
fi
rm -f $$.*
