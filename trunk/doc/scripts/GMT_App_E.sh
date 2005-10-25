#!/bin/sh
#	$Id: GMT_App_E.sh,v 1.5 2005-10-25 13:23:35 remko Exp $
#
#	This script makes the documentation in Appendix E.
#
#	Paul Wessel, v 1.1
#

trap 'rm -f $$.*; exit 1' 1 2 3 15

xwidth=0.45	# Width of each box (all units are in inches)
ywidth=0.45	# Height of each box
w=0.9		# Width of two adjacent boxes
dx=0.55		# Amont to translate to the right
dy=0.50		# Amount to translate up
y=0.05		# Initial offset in x
x=0.05		# Initial offset in y
back=-5.45	# Amount to translate to left after 1 row

cat << END > $$.App_E.d
0	0
$xwidth	0
$xwidth	$ywidth
0	$ywidth
END

psbasemap -R0/6.0/0/7.55 -Jx1i -P -B0 -K > GMT_App_E.ps
for iy in 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
do
	for ix in 1 2 3 4 5 6
	do
		p=`echo "$iy * 6 + $ix" | bc`
		psxy -R0/$xwidth/0/$ywidth -Jx1i -Gp0/$p -O -K $$.App_E.d -X${x}i -Y${y}i >> GMT_App_E.ps
		psxy -R -Jx -W0.5p -L -O -K $$.App_E.d >> GMT_App_E.ps
		psxy -R -Jx -GP0/$p -O -K $$.App_E.d -X${xwidth}i >> GMT_App_E.ps
		psxy -R -Jx -W0.5p -L -O -K $$.App_E.d >> GMT_App_E.ps
		echo "0 0.225" | psxy -R0/$w/0/$ywidth -Jx -O -K -N -Sc0.17i -W0.25p -Gwhite >> GMT_App_E.ps
		echo "0 0.225 9 0 1 CM $p" | pstext -R0/$w/0/$ywidth -Jx -O -K -N >> GMT_App_E.ps
		y=0.0
		x=$dx
	done
	y=$dy
	x=$back
done

rm -f $$.*
