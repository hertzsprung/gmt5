/*	$Id$
 *
 * Include file defining structures used in gshhs.c
 *
 * Paul Wessel, SOEST
 *
 *	Copyright (c) 1996-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU Lesser General Public License as published by
 *	the Free Software Foundation; version 3 or any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU Lesser General Public License for more details.
 *
 *	Contact info: www.soest.hawaii.edu/pwessel
 *
 *	14-SEP-2004.  PW: Version 1.3.  Header is now n * 8 bytes (n = 5)
 *			  For use with version 1.3 of GSHHS
 *	2-MAY-2006.  PW: Version 1.4.  Header is now 32 bytes (all int 4)
 *			  For use with version 1.4 of GSHHS
 *	31-MAR-2007.  PW: Version 1.5.  no format change
 *			  For use with version 1.5 of GSHHS
 *	28-AUG-2007.  PW: Version 1.6.  no format change
 *			  For use with version 1.6 of GSHHS which now has WDBII
 *			  borders and rivers.
 *	03-JUL-2008.  PW: Version 1.11. New -I<id> option to pull out a single pol
 *	27-MAY-2009.  PW: Version 1.12. Now includes container polygon ID in header,
 *			  an ancestor ID, and area of the reduced polygon. Works on
 *			  GSHHS 2.0 data.
 *			  Header is now 44 bytes (all 4-byte integers)
 *	24-MAY-2010.  PW: Data version is now 2.1.0. [no change to format]
 *	15-JUL-2011.   PW: Data version is now 2.2.0. [Change in header format to store
 *			  area magnitude and let greenwich be 2-bit flag (0-3)].  Also
 *			  flag WDBII riverlakes with the river flag as used for GSHHS.
 */

#ifndef _GSHHS
#define _GSHHS
#include "gmt_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "gmt_notposix.h"

#include "common_byteswap.h"

#ifndef M_PI
#define M_PI          3.14159265358979323846
#endif

#ifndef SEEK_CUR	/* For really ancient systems */
#define SEEK_CUR 1
#endif

#define GSHHS_DATA_RELEASE	9		/* For v2.2.0 data set */
#define GSHHS_PROG_VERSION	"1.13"

#define GSHHS_MAXPOL	200000	/* Should never need to allocate more than this many polygons */
#define GSHHS_SCL	1.0e-6	/* Convert micro-degrees to degrees */

struct GSHHS {	/* Global Self-consistent Hierarchical High-resolution Shorelines */
	unsigned int id;		/* Unique polygon id number, starting at 0 */
	unsigned int n;		/* Number of points in this polygon */
	unsigned int flag;	/* = level + version << 8 + greenwich << 16 + source << 24 + river << 25 + p << 26 */
	/* flag contains 6 items, as follows:
	 * low byte:	level = flag & 255: Values: 1 land, 2 lake, 3 island_in_lake, 4 pond_in_island_in_lake
	 * 2nd byte:	version = (flag >> 8) & 255: Values: Should be 9 for GSHHS release 9.
 	 * 3rd byte:	greenwich = (flag >> 16) & 3: Values: 0 if Greenwich nor Dateline are crossed,
	 *		1 if Greenwich is crossed, 2 if Dateline is crossed, 3 if both is crossed.
	 * 4th byte:	source = (flag >> 24) & 1: Values: 0 = CIA WDBII, 1 = WVS
	 * 4th byte:	river = (flag >> 25) & 1: Values: 0 = not set, 1 = river-lake and GSHHS level = 2 (or WDBII class 0)
	 * 4th byte:	area magnitude scale p (as in 10^p) = flag >> 26.  We divide area by 10^p.
	 */
	int west, east, south, north;	/* min/max extent in micro-degrees */
	unsigned int area;	/* Area of polygon in km^2 * 10^p for this resolution file */
	unsigned int area_full;	/* Area of corresponding full-resolution polygon in km^2 * 10^p */
	int container;	/* Id of container polygon that encloses this polygon (-1 if none) */
	int ancestor;	/* Id of ancestor polygon in the full resolution set that was the source of this polygon (-1 if none) */
};

/* byteswap all members of GSHHS struct */
#define GSHHS_STRUCT_N_MEMBERS 11
static inline void bswap_GSHHS_struct (struct GSHHS *h) {
	unsigned int unsigned32[GSHHS_STRUCT_N_MEMBERS];
	unsigned n;

	/* since all members are 32 bit words: */
	memcpy (&unsigned32, h, sizeof(struct GSHHS));

	for (n = 0; n < GSHHS_STRUCT_N_MEMBERS; ++n)
		unsigned32[n] = bswap32 (unsigned32[n]);

	memcpy (h, &unsigned32, sizeof(struct GSHHS));
}

struct	POINT {	/* Each lon, lat pair is stored in micro-degrees in 4-byte integer format */
	int x;
	int y;
};

/* byteswap members of POINT struct */
static inline void bswap_POINT_struct (struct POINT *p) {
	unsigned int unsigned32;
	memcpy (&unsigned32, &p->x, sizeof(unsigned int));
	unsigned32 = bswap32 (unsigned32);
	memcpy (&p->x, &unsigned32, sizeof(unsigned int));
	memcpy (&unsigned32, &p->y, sizeof(unsigned int));
	unsigned32 = bswap32 (unsigned32);
	memcpy (&p->y, &unsigned32, sizeof(unsigned int));
}

#endif	/* _GSHHS */
