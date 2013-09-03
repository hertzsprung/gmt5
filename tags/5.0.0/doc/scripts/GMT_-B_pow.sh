#!/bin/bash
#	$Id$
#
. ./functions.sh

gmtset MAP_GRID_PEN_PRIMARY thinnest,.
psbasemap -R0/100/0/0.9 -JX3ip0.5/0.25i -Ba3f2g1p:"Axis Label":S -K -P > GMT_-B_pow.ps
psbasemap -R -J -Ba20f10g5:"Axis Label":S -O -Y0.85i >> GMT_-B_pow.ps