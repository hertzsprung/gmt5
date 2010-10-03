#!/bin/sh
#		GMT Appendix P, example 2
#		$Id: GMT_App_P_2.sh,v 1.4 2010-10-03 22:06:00 remko Exp $
#
# Purpose:	Illustrates the use of isolation mode
# GMT progs:	gmtset, grdimage, grdmath, makecpt, pscoast
# GMT funcs:	gmt_init_tmpdir, gmt_remove_tmpdir
#
ps=GMT_App_P_2.ps

# Make GMT shell functions accessible the the script
. gmt_shell_functions.sh

# Create a temporary directory. $GMT_TMPDIR will be set to its pathname.
gmt_init_tmpdir

# These settings will be local to this script only since it writes to
# $GMT_TMPDIR/.gmtdefaults4
gmtset ANNOT_FONT_SIZE_PRIMARY 14p

# Make grid file and color map in temporary directory
grdmath -Rd -I1 Y = $GMT_TMPDIR/lat.nc
makecpt -Crainbow -T-90/90/60 -Z > $GMT_TMPDIR/lat.cpt

# The grdimage command creates the history file $GMT_TMPDIR/.gmtcommands4
grdimage $GMT_TMPDIR/lat.nc -Sl -JK6.5i -C$GMT_TMPDIR/lat.cpt -P -K > $ps
pscoast -R -J -O -Dc -A5000 -Gwhite -B60g30/30g30 >> $ps

# Clean up all temporary files and the temporary directory
gmt_remove_tmpdir
