#!/bin/sh
#	$Id: meca_2b.sh,v 1.1 2010-01-24 20:30:58 remko Exp $

. ../functions.sh
header "Test psmeca for plotting focal mechanisms (2b)"

ps=meca_2b.ps

gmtset MEASURE_UNIT inch TICK_LENGTH 0.075i FRAME_WIDTH 0.1i X_ORIGIN 2.5c Y_ORIGIN 1.3i

# Plotting 2 mechanisms on map
psmeca -P -R128/130/10/11.1 -H1 -a0.1i/cc -JX2i -Sc0.4i -B1 -Y8.5i -K << EOF > $ps
lon   lat  dep str dip rake str dip rake m ex nx ny 
129.5  10.5 10  0   90   0  90   90 180  1 24  0  0 10km
128.5  10.5 40  0   45  90 180   45  90  1 24  0  0 40km
EOF
(echo 128 11; echo 130 11) | psxy -R -J -K -O -W0.25p/255/0/0 >> $ps
pstext -R -N -J -K -O << EOF >> $ps
128 11 14 0 1 5 P1
130 11 14 0 1 7 P2
EOF

y_offset=-2.5
x_offset=0
for a in 0 40 80 ; do
    pscoupe -R0/200/0/100 -JX1.5i/-1.5i -Ba100f10/a50f10WesN \
        -L -H1 -Sc0.4 -Ab128/10/$a/250/90/200/0/100f -G200 -a0.1i/cc \
        -Y$y_offset -X$x_offset -O -K << EOF >> $ps
lon   lat  dep str dip rake str dip rake m ex nx ny 
129.5 10.5  10  0   90   0  90   90 180  1 24  0  0 10-$a
128.5 10.5  40  0   45  90 180   45  90  1 24  0  0 40-$a
EOF
    pstext -R -J -O -K << EOF >> $ps
200 90 18 0 1 BR $a
EOF
    y_offset=0
    x_offset=2.5
done
y_offset=-2.5
x_offset=-5
for a in 120 160 200 ; do
    pscoupe -R -J -Ba100f10/a50f10WesN -E255/255/255 \
        -N -L -H1 -Sc0.4 -Ab128/11/$a/250/90/400/0/100f -G200 -a0.1i/cc \
        -Y$y_offset -X$x_offset -O -K << EOF >> $ps
lon   lat  dep str dip rake str dip rake m ex nx ny
129.5 10.5  10  0   90   0  90   90 180  1 24  0  0 10-$a
128.5 10.5  40  0   45  90 180   45  90  1 24  0  0 40-$a
EOF
    pstext -R -J -O -K << EOF >> $ps
200 90 18 0 1 BR $a
EOF
    y_offset=0
    x_offset=2.5
done
y_offset=-2.5
x_offset=-5
for a in 240 280 320 ; do
    pscoupe -R -J -Ba100f10/a50f10WesN -E255/255/255 \
        -N -L -H1 -Sc0.4 -Ab130/10.5/$a/250/90/200/0/100f -G200 -a0.1i/cc \
        -Y$y_offset -X$x_offset -O -K << EOF >> $ps
lon   lat  dep str dip rake str dip rake m ex nx ny
129.5 10.5  10  0   90   0  90   90 180  1 24  0  0 10-$a
128.5 10.5  40  0   45  90 180   45  90  1 24  0  0 40-$a
EOF
    pstext -R -J -O -K << EOF >> $ps
200 90 18 0 1 BR $a
EOF
    y_offset=0
    x_offset=2.5
done
pstext -X-5i -R0/10/0/15 -Jx1i -O << EOF >> $ps
3 8.5 24 0 1 1 Variation of azimuth
3 8.0 20 0 1 1 vertical cross-section
EOF

rm -f .gmtdefaults4 .gmtcommands4 Aa* Ab*

pscmp
