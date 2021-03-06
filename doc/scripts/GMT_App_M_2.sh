#!/bin/bash
#	$Id$
#
ps=GMT_App_M_2.ps

gmt gmtset FONT_ANNOT_PRIMARY 10p PROJ_LENGTH_UNIT cm

# Set up color palette with named annotations

cat > ages.cpt <<END
#COLOR_MODEL = RGB
#
0	197	0	255	23	197	0	255	;Neogene
23	81	0	255	66	81	0	255	;Paleogene
66	0	35	255	146	0	35	255	;Cretaceous
146	0	151	255	200	0	151	255	;Jurassic
200	0	255	244	251	0	255	244	;Triassic
251	0	255	127	299	0	255	127	;Permian
299	0	255	11	359	0	255	11	;Carboniferous
359	104	255	0	416	104	255	0	;Devonian
416	220	255	0	444	220	255	0	;Silurian
444	255	174	0	488	255	174	0	;Ordovician
488	255	58	0	542	255	58	0	;Cambrian
B	black
F	white
END

# Top row, left to right. Using names.
gmt psscale -Ef -Cages.cpt  -D00/13/-8/0.5    -K         > $ps
gmt psscale -Ef -Cages.cpt  -D04/13/-8/0.5 -O -K -L     >> $ps
gmt psscale -Ef -Cages.cpt  -D08/13/-8/0.5 -O -K -L0.0  >> $ps
gmt psscale -Ef -Cages.cpt  -D12/13/-8/0.5 -O -K -L0.1  >> $ps
gmt psscale -Ef -Cages.cpt  -D16/13/+8/0.5 -O -K -L     >> $ps
gmt psscale -Ef -Cages.cpt  -D20/13/+8/0.5 -O -K -L0.1  >> $ps

# Bottom row, left to right. Using numbers.
sed 's/;.*$//' ages.cpt > years.cpt
gmt psscale -Ef -Cyears.cpt -D00/04/+8/0.5 -O -K        >> $ps
gmt psscale -Ef -Cyears.cpt -D04/04/-8/0.5 -O -K -L     >> $ps
gmt psscale -Ef -Cyears.cpt -D08/04/-8/0.5 -O -K -L0.0  >> $ps
gmt psscale -Ef -Cyears.cpt -D12/04/-8/0.5 -O -K -L0.1  >> $ps
gmt psscale -Ef -Cyears.cpt -D16/04/-8/0.5 -O -K -Li    >> $ps
gmt psscale -Ef -Cyears.cpt -D20/04/-8/0.5 -O    -Li0.1 >> $ps
