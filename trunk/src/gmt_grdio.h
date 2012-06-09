/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis and F. Wobbe
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
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/

/*
 * Include file for grd i/o
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#ifndef GMT_GRDIO_H
#define GMT_GRDIO_H

/* Constants for *.img grids */

#define GMT_IMG_MINLON		0.0
#define GMT_IMG_MAXLON		360.0
#define GMT_IMG_MINLAT_72	-72.0059773539
#define GMT_IMG_MAXLAT_72	+72.0059773539
#define GMT_IMG_MINLAT_80	-80.7380086280
#define GMT_IMG_MAXLAT_80	+80.7380086280

enum GMT_enum_img {GMT_IMG_NLON_1M 	= 21600U,	/* At 1 min resolution */
	GMT_IMG_NLON_2M			= 10800U,	/* At 2 min resolution */
	GMT_IMG_NLAT_1M_72		= 12672U,	/* At 1 min resolution */
	GMT_IMG_NLAT_1M_80		= 17280U,	/* At 1 min resolution */
	GMT_IMG_NLAT_2M_72		= 6336U,	/* At 1 min resolution */
	GMT_IMG_NLAT_2M_80		= 8640U,	/* At 1 min resolution */
	GMT_IMG_ITEMSIZE		= 2U};		/* Size of 2 byte short ints */

/* Special grid format IDs */

#include "gmt_grdkeys.h"

#define GMT_GRD_IS_GOLDEN7	GMT_GRD_IS_SD
#define GMT_GRD_IS_GDAL		GMT_GRD_IS_GD

#include "gmt_customio.h"

struct GMT_GRD_INFO {	/* Holds any -R -I -F settings passed indirectly via -R<grdfile> */
	struct GRD_HEADER grd;	/* Header of grid file passed via -R */
	bool active;		/* true if initialized via -R */
};

struct GMT_GRID {	/* To hold a GMT float grid and its header in one container */
	unsigned int id;			/* The internal number of the grid */
	enum GMT_enum_alloc alloc_mode;	/* Allocation info [0] */
	struct GRD_HEADER *header;	/* Pointer to full GMT header for the grid */
	float *data;			/* Pointer to the float grid */
};

struct GMT_GRDFILE {
	size_t size;		/* Bytes per item */
	size_t n_byte;		/* Number of bytes for row */
	unsigned int row;	/* Current row */
	bool check;		/* true if we must replace NaNs with another representation on i/o */
	bool auto_advance;	/* true if we want to read file sequentially */

	int fid;		/* NetCDF file number */
	size_t edge[2];		/* Dimension arrays for netCDF files */
	size_t start[2];	/* same */

	double scale;		/* scale to use for i/o */
	double offset;		/* offset to use for i/o */
	
	FILE *fp;		/* File pointer for native files */
	
	void *v_row;		/* Void Row pointer for any format */
	
	struct GRD_HEADER header;	/* Full GMT header for the file */
};

#endif /* GMT_GRDIO_H */
