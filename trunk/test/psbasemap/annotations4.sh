#!/bin/bash
#
#	$Id$

ps=annotations4.ps

psbasemap -R-180/180/-90/90 -B60g60 -JX14cd/7cd --MAP_FRAME_TYPE=inside --FONT_ANNOT_PRIMARY=10p -P -K > $ps
psbasemap -R-180/180/-90/90 -B60g60 -JX14cd/7cd --MAP_FRAME_TYPE=plain  --FONT_ANNOT_PRIMARY=10p -O >> $ps
