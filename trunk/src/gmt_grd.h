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
 * grd.h contains the definition for a GMT-SYSTEM Version >= 2 grd file
 *
 * grd is stored in rows going from west (xmin) to east (xmax)
 * first row in file has yvalue = north (ymax).  
 * This is SCANLINE orientation.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#ifndef _GMT_GRD_H
#define _GMT_GRD_H

/* netcdf convention */
#define GMT_NC_CONVENTION "COARDS, CF-1.5"

/* Nodes that are unconstrained are assumed to be set to NaN */

enum GMT_enum_reg {
	GMT_GRIDLINE_REG = 0,
	GMT_PIXEL_REG};

/* These lengths (except GRD_VARNAME_LEN80) must NOT be changed as they are part of grd definition */
enum GMT_enum_grdlen {
	GRD_UNIT_LEN80     = 80U,
	GRD_TITLE_LEN80    = 80U,
	GRD_VARNAME_LEN80  = 80U,
	GRD_COMMAND_LEN320 = 320U,
	GRD_REMARK_LEN160  = 160U,
	GRD_HEADER_SIZE	   = 892U};
	
/* Note: GRD_HEADER_SIZE is 4 less than sizeof (struct GRD_HEADER) for 64 bit systems due to alignment.
   Since the GRD_HEADER was designed during 32-bit era its sizeof was 892.  Bof backwards compatibility
   we continue to enforce this header size by not writing the structure components separately. */

enum GMT_enum_grdtype {
	/* Special cases of geographic grids with periodicity */
	GMT_GRD_CARTESIAN=0,			/* Cartesian data, no periodicity involved */
	GMT_GRD_GEOGRAPHIC_LESS360,		/* x is longitude, but range is < 360 degrees */
	GMT_GRD_GEOGRAPHIC_EXACT360_NOREPEAT,	/* x is longitude, range is 360 degrees, no repeat node */
	GMT_GRD_GEOGRAPHIC_EXACT360_REPEAT,	/* x is longitude, range is 360 degrees, gridline registered and repeat node at 360*/
	GMT_GRD_GEOGRAPHIC_MORE360		/* x is longitude, and range exceeds 360 degrees */
};

/*
 * GMT's internal representation of grids is north-up, i.e., the index of the
 * least dimension (aka y or lat) increases to the south. NetCDF files are
 * usually written bottom-up, i.e., the index of the least dimension increases
 * from south to north (k_nc_start_south):
 *
 * k_nc_start_north:  k_nc_start_south:
 *
 * y ^ 0 1 2            -------> x
 *   | 3 4 5            | 0 1 2
 *   | 6 7 8            | 3 4 5
 *   -------> x       y V 6 7 8
 */

enum Netcdf_row_order {
	/* Order of rows in z variable */
	k_nc_start_north = -1, /* The least dimension (i.e., lat or y) decreases */
	k_nc_start_south = 1   /* The least dimension (i.e., lat or y) increases */
};

enum Netcdf_chunksize {
	k_netcdf_io_classic = 0, /* netCDF classic format */
	k_netcdf_io_chunked_auto /* netCDF 4 auto-determined optimal chunk size */
};

struct GRD_HEADER {
/* ===== Do not change the first three items. They are copied verbatim to the native grid header */
	unsigned int nx;                /* Number of columns */
	unsigned int ny;                /* Number of rows */
	unsigned int registration;      /* GMT_GRIDLINE_REG (0) for node grids, GMT_PIXEL_REG (1) for pixel grids */
/* This section is flexible. It is not copied to any grid header or stored in the file */
	unsigned int type;              /* Grid format */
	unsigned int bits;              /* Bits per data value (e.g., 32 for ints/floats; 8 for bytes) */
	unsigned int complex_mode;      /* 0 = normal, GMT_GRID_COMPLEX_REAL = real part of complex grid, GMT_GRID_COMPLEX_IMAG = imag part of complex grid */
	unsigned int mx, my;            /* Actual dimensions of the grid in memory, allowing for the padding */
	unsigned int BB_mx, BB_my;      /* Actual dimensions of a mosaicked grid, allowing for the padding */
	size_t nm;                      /* Number of data items in this grid (nx * ny) [padding is excluded] */
	size_t size;                    /* Actual number of items (not bytes) required to hold this grid (= mx * my) */
	unsigned int n_bands;           /* Number of bands [1]. Used with IMAGE containers and macros to get ij index from row,col, band */
	unsigned int pad[4];            /* Padding on west, east, south, north sides [2,2,2,2] */
	unsigned int BC[4];             /* Boundary condition applied on each side via pad [0 = not set, 1 = natural, 2 = periodic, 3 = data] */
	unsigned int grdtype;           /* 0 for Cartesian, > 0 for geographic and depends on 360 periodicity [see GMT_enum_grdtype above] */
	char name[GMT_TEXT_LEN256];     /* Actual name of the file after any ?<varname> and =<stuff> has been removed */
	char varname[GRD_VARNAME_LEN80];/* NetCDF: variable name */
	int row_order;                  /* NetCDF: k_nc_start_south if S->N, k_nc_start_north if N->S */
	int z_id;                       /* NetCDF: id of z field */
	int ncid;                       /* NetCDF: file ID */
	int xy_dim[2];                  /* NetCDF: dimension order of x and y; normally {1, 0} */
	size_t t_index[3];              /* NetCDF: index of higher coordinates */
	size_t data_offset;             /* NetCDF: distance from the beginning of the in-memory grid */
	size_t stride;                  /* NetCDF: distance between two rows in the in-memory grid */
	double nan_value;               /* Missing value as stored in grid file */
	double xy_off;                  /* 0.0 (registration == GMT_GRIDLINE_REG) or 0.5 ( == GMT_PIXEL_REG) */
	double r_inc[2];                /* Reciprocal incs, i.e. 1/inc */
	char flags[4];                  /* Flags used for ESRI grids */
	char *pocket;                   /* GDAL: A working variable handy to transmit info between funcs e.g. +b<band_info> to gdalread */
	double bcr_threshold;           /* sum of cardinals must >= threshold in bilinear; else NaN */
	unsigned int bcr_interpolant;   /* Interpolation function used (0, 1, 2, 3) */
	unsigned int bcr_n;             /* Width of the interpolation function */
	unsigned int nxp;               /* if X periodic, nxp > 0 is the period in pixels  */
	unsigned int nyp;               /* if Y periodic, nxp > 0 is the period in pixels  */
	bool no_BC;                     /* If true we skip BC stuff entirely */
	bool gn;                        /* true if top    edge will be set as N pole  */
	bool gs;                        /* true if bottom edge will be set as S pole  */
	bool is_netcdf4;                /* true if netCDF-4/HDF5 format */
	unsigned int z_chunksize[2];    /* chunk size (lat,lon) */
	bool z_shuffle;                 /* if shuffle filter is turned on */
	unsigned int z_deflate_level;   /* if deflate filter is in use */
	bool z_scale_autoadust;         /* if z_scale_factor should be auto-detected */
	bool z_offset_autoadust;        /* if z_add_offset should be auto-detected */

/* ===== The following elements must not be changed. They are copied verbatim to the native grid header */
	double wesn[4];                   /* Min/max x and y coordinates */
	double z_min;                     /* Minimum z value */
	double z_max;                     /* Maximum z value */
	double inc[2];                    /* x and y increment */
	double z_scale_factor;            /* grd values must be multiplied by this */
	double z_add_offset;              /* After scaling, add this */
	char x_units[GRD_UNIT_LEN80];     /* units in x-direction */
	char y_units[GRD_UNIT_LEN80];     /* units in y-direction */
	char z_units[GRD_UNIT_LEN80];     /* grid value units */
	char title[GRD_TITLE_LEN80];      /* name of data set */
	char command[GRD_COMMAND_LEN320]; /* name of generating command */
	char remark[GRD_REMARK_LEN160];   /* comments re this data set */
};

/*-----------------------------------------------------------------------------------------
 *	Notes on registration:

	Assume x_min = y_min = 0 and x_max = y_max = 10 and x_inc = y_inc = 1.
	For a normal node grid we have:
		(1) nx = (x_max - x_min) / x_inc + 1 = 11
		    ny = (y_max - y_min) / y_inc + 1 = 11
		(2) node # 0 is at (x,y) = (x_min, y_max) = (0,10) and represents the surface
		    value in a box with dimensions (1,1) centered on the node.
	For a pixel grid we have:
		(1) nx = (x_max - x_min) / x_inc = 10
		    ny = (y_max - y_min) / y_inc = 10
		(2) node # 0 is at (x,y) = (x_min + 0.5*x_inc, y_max - 0.5*y_inc) = (0.5, 9.5)
		    and represents the surface value in a box with dimensions (1,1)
		    centered on the node.
-------------------------------------------------------------------------------------------*/

/* The array wesn in the header has a name that indicates the order (west, east, south, north).
 * However, to avoid using confusing indices 0-3 we define very brief constants XLO, XHI, YLO, YHI
 * that should be used instead: */
enum GMT_enum_wesnIDs {
	XLO = 0, /* Index for west or xmin value */
	XHI,     /* Index for east or xmax value */
	YLO,     /* Index for south or ymin value */
	YHI,     /* Index for north or ymax value */
	ZLO,     /* Index for zmin value */
	ZHI      /* Index for zmax value */
};

/* These macros should be used to convert between (column,row) and (x,y).  It will eliminate
 * one source of typos and errors, and since macros are done at compilation time there is no
 * overhead.  Note: gmt_x_to_col does not need nx but we included it for symmetry reasons.
 * gmt_y_to_row must first compute j', the number of rows in the increasing y-direction (to
 * match the sense of truncation used for x) then we revert to row number increasing down
 * by flipping: j = ny - 1 - j'.
 * Note that input col, row _may_ be negative, hence we do the cast to (int) here. */

#define gmt_x_to_col(x,x0,dx,off,nx) (lrint((((x) - (x0)) / (dx)) - (off)))
#define gmt_y_to_row(y,y0,dy,off,ny) ((ny) - 1 - lrint(((((y) - (y0)) / (dy)) - (off))))
#define GMT_col_to_x(C,col,x0,x1,dx,off,nx) (((int)(col) == (int)((nx)-1)) ? (x1) - (off) * (dx) : (x0) + ((col) + (off)) * (dx))
#define GMT_row_to_y(C,row,y0,y1,dy,off,ny) (((int)(row) == (int)((ny)-1)) ? (y0) + (off) * (dy) : (y1) - ((row) + (off)) * (dy))

/* The follow macros simplify using the 4 above macros when all info is in the struct header h. */

#define GMT_grd_col_to_x(C,col,h) GMT_col_to_x(C,col,h->wesn[XLO],h->wesn[XHI],h->inc[GMT_X],h->xy_off,h->nx)
#define GMT_grd_row_to_y(C,row,h) GMT_row_to_y(C,row,h->wesn[YLO],h->wesn[YHI],h->inc[GMT_Y],h->xy_off,h->ny)
#define GMT_grd_x_to_col(C,x,h) gmt_x_to_col(x,h->wesn[XLO],h->inc[GMT_X],h->xy_off,h->nx)
#define GMT_grd_y_to_row(C,y,h) gmt_y_to_row(y,h->wesn[YLO],h->inc[GMT_Y],h->xy_off,h->ny)

/* These macros calculate the number of nodes in x or y for the increment dx, dy */

#define GMT_get_n(C,min,max,inc,off) (lrint (((max) - (min)) / (inc)) + 1 - (off))
#define GMT_get_inc(C,min,max,n,off) (((max) - (min)) / ((n) + (off) - 1))

/* The follow macros simplify using the 2 above macros when all info is in the struct header */

#define GMT_grd_get_nx(C,h) GMT_get_n(C,h->wesn[XLO],h->wesn[XHI],h->inc[GMT_X],h->registration)
#define GMT_grd_get_ny(C,h) GMT_get_n(C,h->wesn[YLO],h->wesn[YHI],h->inc[GMT_Y],h->registration)

/* The follow macros gets the full length or rows and columns when padding is considered (i.e., mx and my) */

#define gmt_grd_get_nxpad(h,pad) ((h->nx) + pad[XLO] + pad[XHI])
#define gmt_grd_get_nypad(h,pad) ((h->ny) + pad[YLO] + pad[YHI])

/* 64-bit-safe macros to return the number of points in the grid given its dimensions */

#define GMT_get_nm(C,nx,ny) (((uint64_t)(nx)) * ((uint64_t)(ny)))
#define gmt_grd_get_nm(h) (((uint64_t)(h->nx)) * ((uint64_t)(h->ny)))

/* GMT_grd_setpad copies the given pad into the header */

#define GMT_grd_setpad(C,h,newpad) memcpy ((h)->pad, newpad, 4*sizeof(unsigned int))

/* gmt_grd_get_size computes grid size including the padding, and doubles it if complex values */

#define gmt_grd_get_size(h) ((((h->complex_mode & GMT_GRID_COMPLEX_MASK) > 0) + 1) * h->mx * h->my)

/* Calculate 1-D index a[ij] corresponding to 2-D array a[row][col], with 64-bit precision.
 * Use GMT_IJP when array is padded by BC rows/cols, else use GMT_IJ0.  In both cases
 * we pass the column dimension as padding is added by the macro. */

/* New IJP macro using h and the pad info */
#define GMT_IJP(h,row,col) (((uint64_t)(row)+(uint64_t)h->pad[YHI])*((uint64_t)h->mx)+(uint64_t)(col)+(uint64_t)h->pad[XLO])
/* New IJPR|C macros using h and the pad info to get the real or imag component of a complex array*/
#define GMT_IJPR(h,row,col) (2*(((uint64_t)(row)+(uint64_t)h->pad[YHI])*((uint64_t)h->mx)+(uint64_t)(col)+(uint64_t)h->pad[XLO]))
#define GMT_IJPC(h,row,col) (GMT_IJPR(h,row,col)+1)
/* New IJ0 macro using h but ignores the pad info */
#define GMT_IJ0(h,row,col) (((uint64_t)(row))*((uint64_t)h->nx)+(uint64_t)(col))
/* New IJPGI macro using h and the pad info that works for either grids (n_bands = 1) or images (n_bands = 1,3,4) */
#define GMT_IJPGI(h,row,col) (((uint64_t)(row)+(uint64_t)h->pad[YHI])*((uint64_t)h->mx*(uint64_t)h->n_bands)+(uint64_t)(col)+(uint64_t)h->pad[XLO]*(uint64_t)h->n_bands)

/* Obtain row and col from index */
#define GMT_col(h,ij) (((ij) % h->mx) - h->pad[XLO])
#define GMT_row(h,ij) (((ij) / h->mx) - h->pad[YHI])

/* To set up a standard double for-loop over rows and columns to visit all nodes in a padded array by computing the node index, use GMT_grd_loop */
/* Note: All arguments must be actual variables and not expressions.
 * Note: that input col, row _may_ be signed, hence we do the cast to (int) here. */

#define GMT_row_loop(C,G,row) for (row = 0; (int)row < (int)G->header->ny; row++)
#define GMT_col_loop(C,G,row,col,ij) for (col = 0, ij = GMT_IJP (G->header, row, 0); (int)col < (int)G->header->nx; col++, ij++)
#define GMT_grd_loop(C,G,row,col,ij) GMT_row_loop(C,G,row) GMT_col_loop(C,G,row,col,ij)
/* Just a loop over columns */
#define GMT_col_loop2(C,G,col) for (col = 0; (int)col < (int)G->header->nx; col++)
/* Loop over all nodes including the pad */
#define GMT_row_padloop(C,G,row,ij) for (row = ij = 0; (int)row < (int)G->header->my; row++)
#define GMT_col_padloop(C,G,col,ij) for (col = 0; (int)col < (int)G->header->mx; col++, ij++)
#define GMT_grd_padloop(C,G,row,col,ij) GMT_row_padloop(C,G,row,ij) GMT_col_padloop(C,G,col,ij)

/* The usage could be:
	GMT_grd_loop (GMT, Grid, row, col, node) fprintf (stderr, "Value at row = %d and col = %d is %g\n", row, col, Grid->data[node]);
*/
/* The GMT_y_is_outside macro returns true if y is outside the given domain.
 * For GMT_x_is_outside, see the function in gmt_support.c since we must also deal with longitude periodicity.
 */

/* GMT_is_subset is true if wesn is set and wesn cuts through the grid region */
#define GMT_is_subset(C,h,R) (R[XHI] > R[XLO] && R[YHI] > R[YLO] && (R[XLO] > h->wesn[XLO] || R[XHI] < h->wesn[XHI] || R[YLO] > h->wesn[YLO] || R[YHI] < h->wesn[YHI]))
/* GMT_grd_same_region is true if two grids have the exact same regions */
#define GMT_grd_same_region(C,G1,G2) (G1->header->wesn[XLO] == G2->header->wesn[XLO] && G1->header->wesn[XHI] == G2->header->wesn[XHI] && G1->header->wesn[YLO] == G2->header->wesn[YLO] && G1->header->wesn[YHI] == G2->header->wesn[YHI])
/* GMT_grd_same_inc is true if two grids have the exact same grid increments */
#define GMT_grd_same_inc(C,G1,G2) (G1->header->inc[GMT_X] == G2->header->inc[GMT_X] && G1->header->inc[GMT_Y] == G2->header->inc[GMT_Y])
/* GMT_grd_same_dim is true if two grids have the exact same dimensions and registrations */
#define GMT_grd_same_shape(C,G1,G2) (G1->header->nx == G2->header->nx && G1->header->ny == G2->header->ny && G1->header->registration == G2->header->registration)
/* GMT_y_is_outside is true if y is outside the given range */
#define GMT_y_is_outside(C,y,bottom,top) ((GMT_is_dnan(y) || (y) < bottom || (y) > top) ? true : false)
/* GMT_grd_is_global is true for a geographic grid with exactly 360-degree range (with or without repeating column) */
#define GMT_grd_is_global(C,h) (h->grdtype == GMT_GRD_GEOGRAPHIC_EXACT360_NOREPEAT || h->grdtype == GMT_GRD_GEOGRAPHIC_EXACT360_REPEAT)

/* GMT_grd_duplicate_column is true for geographical global grid where first and last data columns are identical */
#define GMT_grd_duplicate_column(C,h,way) (C->current.io.col_type[way][GMT_X] == GMT_IS_LON && GMT_360_RANGE (h->wesn[XHI], h->wesn[XLO]) && h->registration == GMT_GRIDLINE_REG)

#endif /* _GMT_GRD_H */
