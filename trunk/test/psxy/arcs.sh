#!/bin/sh
#	$Id: arcs.sh,v 1.2 2009-01-14 22:52:12 guru Exp $
#
# Plot all the symbols on a 1x1 inch grid pattern

. ../functions.sh
header "Test psxy with various circles, ellipses and wedges"

ps=arcs.ps
psxy -R0/4/0/4 -Jx1i -P -M -W5p -S1i -X2i -Y2i << EOF > $ps
1 1 1 c
1 3.2 0 0.8 0.4 e
> -W3p,blue -Ggreen
1 2 -30 210 1 w
> -Gp100/7:Fgreen -W5p,red
1 3 30 90 1 w
> -Gred -W5p
3 1 -45 1 0.5 e
> -Gp100/12:FredByellow -W-
3 3 45 1 0.5 e
EOF

pscmp
