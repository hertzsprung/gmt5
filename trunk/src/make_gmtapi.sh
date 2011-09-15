#!/bin/sh
#	$Id$
#
# Script that creates gmt_${suppl}.h.
# It takes a list of all GMT programs as arguments. To use with the GMT
# supplements give the supplement name with a leading underscore as the
# first program argument (e.g., _dbase).
#
#	Paul Wessel, Dec 2010
#
first=`echo $1 | awk '{print substr($1,1,1)}'`
pwd=`pwd`
pwd=`basename $pwd|sed -e s/src// -e s/progs//`
case $pwd in
	"")	suppl=modules
		SPL=
		kind="standard" ;;
	*)	suppl=$pwd
		SPL=`echo $suppl | tr 'a-z' 'A-Z'`
		kind="supplemental" ;;
esac
year=`date '+%Y'`
now=`date '+%d-%b-%Y'`

cat << EOF > gmt_${suppl}.h
/*--------------------------------------------------------------------
 *	gmt_${suppl}.h	[Automatically generated by make]
 *
 *	Copyright (c) 1991-$year by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 or any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 * Include file for the $kind GMT_${suppl}_* functions; see man page for details.
 *
 * Build:	$now
 * Version:	5
 *
 */
#ifndef _GMTAPI${SPL}_INC_H
#define _GMTAPI${SPL}_INC_H

#include "gmt.h"

EOF

for file in $*; do
	prog=`basename $file '_func.o'`
	cat <<- EOF >> gmt_${suppl}.h
	EXTERN_MSC GMT_LONG GMT_${prog} (struct GMTAPI_CTRL *API, GMT_LONG mode, void *args);
	EOF
done
echo "#endif /* _GMTAPI${SPL}_INC_H */" >> gmt_${suppl}.h
