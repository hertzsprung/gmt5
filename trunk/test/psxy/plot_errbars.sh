#!/bin/sh
#	$Id: plot_errbars.sh,v 1.3 2007-11-15 04:20:42 remko Exp $
#
# Plot error bars and test [+|-]<epen>

. ../functions.sh
header "Test psxy error bar colors"

ps=plot_errbars.ps
cat << EOF > $$
1	1	1	1
2	2	2	1
3	3	3	1
4	4	4	1
5	5	5	1
EOF
makecpt -Crainbow -T0/6/1 > $$.cpt
psxy -R0/6/0/6 -JX3i -P -B0 -Sc0.2i -C$$.cpt -W0.25p -X1i -Y2i $$ -Ex/2p,red -K > $ps
psxy -R -J -O -B0 -Sc0.2i -C$$.cpt -W0.25p -X3.25i $$ -Ey/-1p -K >> $ps
psxy -R -J -O -B0 -Sc0.2i -C$$.cpt -W+5p -X-3.25i -Y3.5i $$ -Ey/+1p -K >> $ps
psxy -R -J -O -B0 -Sc0.2i -C$$.cpt -W0.25p,red -X3.25i $$ -Ex/-1p >> $ps

rm -f $$*

pscmp
