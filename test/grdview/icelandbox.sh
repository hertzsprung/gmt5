#!/bin/bash
#
# Script to plot 3D model of Iceland, provided by Peter Schmidt
#
#	$Id$

ps=icelandbox.ps

### Plot options
popt=-p130/30

### Colormap
cpt="${src:-.}"/resid.cpt   

### Pivotal points
a="-24.4130658 63.0225642"
b="-17.49029 63.1705533"
c="-17.4678686 64.5788931"
d="-13.5105157 64.5134051"
e="-13.121354 66.7035842"
f="-25.3398981 66.6019215"

### Set up walls between 0 and 200 km depth
gmt psxyz $popt -R-24.4/63.0/-13.1/66.7/0/200r -JZ-5c -JT-17.927/64.463/15c -Bxa2f1 -Bya1f0.5 -Bz50 -BESZ -Wthinnest -K << EOF > $ps
> -G220
$a 200
$a 0
$b 0
$b 200
>
$c 200
$c 0
$d 0
$d 200
> -G180
$b 200
$b 0
$c 0
$c 200
>
$d 200
$d 0
$e 0
$e 200
EOF

gmt psxyz -p -R -J -JZ -Wthinnest -O -K << EOF >> $ps
$a 100
$b 100
$c 100
$d 100
$e 100
EOF
 
### Set clipping perimeter
gmt psclip $popt/0 -R -JZ -J -O -K << EOF >> $ps
$a
$b
$c
$d
$e
$f
EOF

### Start plotting gmt surface
gmt grdimage D3-25TV24-resid.nc -E100 -nl -p -R -J -JZ -C$cpt -O -K >> $ps

gmt grdcontour --PS_COMMENTS=1 D3-25TV24-resid.nc -p -R -J -JZ -Wthinner -A- -C$cpt -W -K -O >> $ps

gmt pscoast -p -R -J -JZ -Dh -A100 -Wthinnest -S135/190/240 -O -K >> $ps
gmt psclip -C -O >> $ps

