#!/bin/bash
#	$Id$
#
gmt psxy "${tut:-../tutorial}"/data -R0/6/0/6 -Jx1i -P -Baf -Wthinner > GMT_tut_7.ps
