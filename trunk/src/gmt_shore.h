/*--------------------------------------------------------------------
 *	$Id: gmt_shore.h,v 1.2 2001-02-20 17:48:32 pwessel Exp $
 *
 *	Copyright (c) 1991-2001 by P. Wessel and W. H. F. Smith
 *	See COPYING file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: www.soest.hawaii.edu/gmt
 *--------------------------------------------------------------------*/

#ifndef _GMT_SHORE_H
#define _GMT_SHORE_H

/*
 * Include file for gmt_shore.c
 *
 * Author:	Paul Wessel
 * Date:	12-AUG-1995
 * Revised:	15-AUG-1998
 * Version:	3.3.6
 */

#include "netcdf.h"

#define MAX_DELTA 65535	/* Largest value to store in a unsigned short, used as largest dx or dy in bin  */
#define MAX_LEVEL	4	/* Highest hierarchical level of coastlines */

#define N_BLEVELS	3	/* Number of levels for borders */
#define N_RLEVELS	10	/* Number of levels for rivers */

struct GMT_SHORE {

	/* Global variables that remain fixed for all bins */
	
	int nb;			/* Number of bins to use */
	int *bins;		/* Array with the nb bin numbers to use */
	double scale;		/* Multiplier to convert dx, dy back to dlon, dlat in degrees */
	
	/* Variables associated with the current bin */
	
	int ns;			/* Number of segments to use in current bin */
	unsigned char node_level[4];
	struct GMT_SHORE_SEGMENT *seg;	/* Array of these segments */
	struct SIDE *side[4];		/* Has position & id for each side exit/entry */
	int nside[4];			/* Number of entries per side, including corner */
	int n_entries;
	BOOLEAN leftmost_bin;		/* TRUE if current bin is at left edge of map */
	double bsize;			/* Size of square bins in degrees */
	double lon_sw;			/* Longitude of SW corner */
	double lat_sw;			/* Latitude of SW corner */
	double lon_corner[4];		/* Longitudes of 4 corners (depends on direction) */
	double lat_corner[4];		/* Latitudes of 4 corners (depends on direction) */

	/* Data variables associated with shoreline database */
	
	int bin_size;	/* Size of square bins in minutes */
	int bin_nx;		/* Number of bins in 360 degrees of longitude */
	int bin_ny;		/* Number of bins in 180 degrees of latitude */
	int n_bin;		/* Number of bins present in the data set */
	int n_seg;		/* Number of segments present in the data set */
	int n_pt;		/* Number of points present in the data set */
	
	int *bin_firstseg;	/* Array with ids of first segment per bin */
	short *bin_info;	/* Array with levels of all 4 nodes per bin */
	short *bin_nseg;	/* Array with number of segments per bin */
	
	char units[80];		/* Units of lon/lat */
	char title[80];		/* Title of data set */
	char source[80];	/* Source of data set */

	/* Netcdf ID variables */
	
	int cdfid;		/* File id for coastbin file */
	
	int bin_size_id;	/* Id for variable bin_size */
	int bin_nx_id;		/* Id for variable bin_nx */
	int bin_ny_id;		/* Id for variable bin_ny */
	int n_bin_id;		/* Id for variable n_bin */
	int n_seg_id;		/* Id for variable n_seg */
	int n_pt_id;		/* Id for variable n_pt */
	int bin_firstseg_id;	/* Id for variable bin_firstseg */
	int bin_info_id;	/* Id for variable bin_info */
	int bin_nseg_id;	/* Id for variable bin_nseg */
	
	int seg_info_id;	/* Id for variable seg_info */
	int seg_area_id;	/* Id for variable seg_area */
	int seg_start_id;	/* Id for variable seg_start */
	
	int pt_dx_id;		/* Id for variable pt_dx */
	int pt_dy_id;		/* Id for variable pt_dy */
	
};

struct GMT_SHORE_SEGMENT {
	unsigned char level;	/* Level of polygon segment (1 i ocean/land, 2 = land/lake, 3 = lake/island, etc) */
	unsigned char entry;	/* Side (0-3) the segment starts on, or 4 for closed segments */
	unsigned char exit;	/* Side (0-3) the segment ends on, or 4 for closed segments */
	unsigned short n;	/* Number of points in segment */
	short *dx;		/* Array of scaled longitudes relative to SW corner */
	short *dy;		/* Array of scaled latitudes relative to SW corner */
};

struct SIDE {
	unsigned short pos;	/* Position along side in 0-65535 range */
	short id;	/* Local segment id */
};

struct GMT_BR {	/* Structure for Borders and Rivers */

	/* Global variables that remain fixed for all bins */
	
	int nb;			/* Number of bins to use */
	int *bins;		/* Array with the nb bin numbers to use */
	double scale;		/* Multiplier to convert dx, dy back to dlon, dlat in degrees */
	
	/* Variables associated with the current bin */
	
	int ns;			/* Number of segments to use in current bin */
	struct GMT_BR_SEGMENT *seg;	/* Array of these segments */
	double lon_sw;		/* Longitude of SW corner */
	double lat_sw;		/* Latitude of SW corner */
	double bsize;		/* Size of square bins in degrees */

	/* Data variables associated with shoreline database */
	
	int bin_size;	/* Size of square bins in minutes */
	int bin_nx;		/* Number of bins in 360 degrees of longitude */
	int bin_ny;		/* Number of bins in 180 degrees of latitude */
	int n_bin;		/* Number of bins present in the data set */
	int n_seg;		/* Number of segments present in the data set */
	int n_pt;		/* Number of points present in the data set */
	
	int *bin_firstseg;	/* Array with ids of first segment per bin */
	short *bin_nseg;	/* Array with number of segments per bin */
	
	char units[80];		/* Units of lon/lat */
	char title[80];		/* Title of data set */
	char source[80];	/* Source of data set */

	/* Netcdf ID variables */
	
	int cdfid;		/* File id for coastbin file */
	
	int bin_size_id;	/* Id for variable bin_size */
	int bin_nx_id;		/* Id for variable bin_nx */
	int bin_ny_id;		/* Id for variable bin_ny */
	int n_bin_id;		/* Id for variable n_bin */
	int n_seg_id;		/* Id for variable n_seg */
	int n_pt_id;		/* Id for variable n_pt */
	int bin_firstseg_id;	/* Id for variable bin_firstseg */
	int bin_nseg_id;	/* Id for variable bin_nseg */
	
	int seg_n_id;		/* Id for variable seg_n */
	int seg_level_id;	/* Id for variable seg_level */
	int seg_start_id;	/* Id for variable seg_start */
	
	int pt_dx_id;		/* Id for variable pt_dx */
	int pt_dy_id;		/* Id for variable pt_dy */
	
};

struct GMT_BR_SEGMENT {
	unsigned short n;	/* Number of points in segment */
	unsigned short level;	/* Hierarchial level of segment */
	short *dx;		/* Array of scaled longitudes relative to SW corner */
	short *dy;		/* Array of scaled latitudes relative to SW corner */
};

struct POL {
	int n;
	int interior;	/* TRUE if polygon is inside bin */
	int level;
	double *lon;
	double *lat;
};

/* Public functions */

EXTERN_MSC void GMT_get_shore_bin (int b, struct GMT_SHORE *c, double min_area, int min_level, int max_level);
EXTERN_MSC void GMT_get_br_bin (int b, struct GMT_BR *c, int *level, int n_levels);
EXTERN_MSC void GMT_free_polygons (struct POL *p, int n);
EXTERN_MSC void GMT_free_shore (struct GMT_SHORE *c);
EXTERN_MSC void GMT_free_br (struct GMT_BR *c);
EXTERN_MSC void GMT_shore_cleanup (struct GMT_SHORE *c);
EXTERN_MSC void GMT_br_cleanup (struct GMT_BR *c);
EXTERN_MSC int GMT_init_shore (char res, struct GMT_SHORE *c, double w, double e, double s, double n);
EXTERN_MSC int GMT_init_br (char which, char res, struct GMT_BR *c, double w, double e, double s, double n);
EXTERN_MSC int GMT_assemble_shore (struct GMT_SHORE *c, int dir, int first_level, BOOLEAN assemble, BOOLEAN shift, double west, double east, struct POL **pol);
EXTERN_MSC int GMT_assemble_br (struct GMT_BR *c, BOOLEAN shift, double edge, struct POL **pol);
EXTERN_MSC int GMT_prep_polygons (struct POL **p, int np, BOOLEAN greenwich, BOOLEAN sample, double step, int anti_bin);
EXTERN_MSC int GMT_set_resolution (char *res, char opt);

#endif /* _GMT_SHORE_H */
