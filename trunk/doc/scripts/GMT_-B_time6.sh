#!/bin/bash
#	$Id: GMT_-B_time6.sh,v 1.7 2011-03-15 02:06:29 guru Exp $
#
. functions.sh
gmtset FORMAT_DATE_MAP "o yy" FORMAT_TIME_PRIMARY_MAP Abbreviated
psbasemap -R1996T/1996-6T/0/1 -JX5i/0.2i -Ba1Of1dS -P > GMT_-B_time6.ps
