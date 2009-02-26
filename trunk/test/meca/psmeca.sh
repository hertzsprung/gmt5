#!/bin/sh
#	$Id: psmeca.sh,v 1.1 2009-02-26 03:54:56 remko Exp $
#
# Check psmeca for plotting beach balls

. ../functions.sh
header "Test psmeca for plotting focal mechanisms"

ps=psmeca.ps

# Right lateral Strike Slip
echo 0.0 5.0 0.0 0 90 0 5 0 0   Right Strike Slip | psmeca -Sa2.5c -Gblack -R-1/4/0/6 -JM14c -P -B2 -K > $ps

# Left lateral Strike Slip
echo 2.0 5.0 0.0 0 90 180 5 0 0 Left Strike Slip | psmeca -Sa2.5c -Gblack -R -J -K -O >> $ps

# Thrust
echo 0.0 3.0 0.0 0 45 90 5 0 0  Thrust | psmeca -Sa2.5c -Gblack -R -J -K -O >> $ps
echo 2.0 3.0 0.0 45 45 90 5 0 0 Thrust | psmeca -Sa2.5c -Gblack -R -J -K -O >> $ps

# Normal
echo 0.0 1.0 0.0 0 45 -90 5 0 0  Normal | psmeca -Sa2.5c -Gblack -R -J -K -O >> $ps
echo 2.0 1.0 0.0 45 45 -90 5 0 0 Normal | psmeca -Sa2.5c -Gblack -R -J -K -O >> $ps

# Mixed
echo 3.4 0.6 0.0 10 35 129 5 0 0 Mixed  | psmeca -Sa2.5c -Gblack -R -J -O >> $ps

rm -f .gmtcommands4

pscmp
