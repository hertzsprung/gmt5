#!/bin/bash
#	$Id$
#
gmt psxy "${tut:-../tutorial}"/data -R0/6/0/6 -Jx1i -Baf -P -K -Wthinner > GMT_tut_8.ps
gmt psxy "${tut:-../tutorial}"/data -R -J -O -W -Si0.2i >> GMT_tut_8.ps
