#!/bin/sh
#	$Id: GMT_App_K_5.sh,v 1.3 2004-04-12 04:01:58 pwessel Exp $
#
pscoast `./getbox -JE130.35/-0.2/1i -20 20 -20 20` -JE130.35/-0.2/3.5i -P -Df -Glightgray -W0.25p \
   -N1/0.25tap -B10mg2mWSne > GMT_App_K_5.ps
