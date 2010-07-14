#!/bin/sh
#	$Id: east_map_2.sh,v 1.3 2010-07-14 15:11:30 remko Exp $

. ../functions.sh
header "Test -JG (US East Coast 160 km w/tilt+view)"

EARTH_MODEL=e
DEBUG=
X0=-Xc
Y0=-Yc
REGION=-Rg
PSFILE=east_map_2.${EARTH_MODEL}
TITLE=:.${PSFILE}:
latitude=41.5
longitude=-74.0
altitude=160.0
tilt=55
azimuth=210
twist=0
Width=20.0
Height=20.0

PROJ=-JG${DEBUG}${EARTH_MODEL}${longitude}/${latitude}/${altitude}/${azimuth}/${tilt}/${twist}/${Width}/${Height}/7i+

pscoast ${GMT_VERBOSE} $REGION $PROJ -P -Yc -Xc -B5g5/5g5${TITLE} -G128/255/128 -S128/128/255 -W -Ia -Di -Na --ANNOT_MIN_SPACING=0.5i > $PSFILE.ps

pscmp $PSFILE
