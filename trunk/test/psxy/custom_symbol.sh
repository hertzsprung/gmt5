#!/bin/bash
#       $Id$
#
# Check two custom symbols symbols with new variables and text capabilities

ps=custom_symbol.ps

sed -n 1p custom_data.txt | gmt psxy -R0/20/10/30 -JM6i -P -Bag -W0.5p,red -Skcomet/2.5i -K -Xc -Yc > $ps
sed -n 2p custom_data.txt | gmt psxy -R -J -W1p,green -Skdip/2.5i -O >> $ps

