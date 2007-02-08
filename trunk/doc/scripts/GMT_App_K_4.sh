#!/bin/sh
#	$Id: GMT_App_K_4.sh,v 1.5 2007-02-08 21:46:27 remko Exp $
#
pscoast `./getbox.sh -JE130.35/-0.2/1i -100 100 -100 100` -JE130.35/-0.2/3.5i -P -Dh -A1 \
	-Glightgray -Wthinnest -N1/thinnest,- -B30mg10mWSne -K > GMT_App_K_4.ps
./getrect.sh -JE130.35/-0.2/1i -20 20 -20 20 | psxy -R -JE130.35/-0.2/3.5i -O -Wthicker -L -A \
	>> GMT_App_K_4.ps
