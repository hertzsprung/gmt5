/*--------------------------------------------------------------------
 *	$Id: gmtapi.h,v 1.4 2006-03-28 01:37:39 pwessel Exp $
 *
 *	Copyright (c) 1991-2006 by P. Wessel and W. H. F. Smith
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
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
 
/*
 * The single include file for users who which to develop applications
 * that require building blocks from the GMT Application Program Interface
 * library (the GMT API).
 * GMT developers who whish to add new GMT core applications also need to
 * include gmtapi_devel.h.
 *
 * Author: 	Paul Wessel
 * Date:	25-MAR-2006
 * Version:	0.1
 */

#ifndef _GMTAPI_H
#define _GMTAPI_H

#include "gmt.h"

/*=====================================================================================
 *	GMT API CONSTANTS DEFINITIONS
 *=====================================================================================
 */

#define GMTAPI_N_ARRAY_ARGS	8	/* Minimum size of information array used to specify array parameters */
#define GMTAPI_N_GRID_ARGS	12	/* Minimum size of information array used to specify grid parameters */

	/* Misc GMTAPI error codes; can be passed to GMT_Error_Message */
	
#define GMTAPI_OK		0
#define GMTAPI_ERROR		1
#define GMTAPI_NOT_A_SESSION	2
#define GMTAPI_NOT_A_VALID_ID	3
#define GMTAPI_FILE_NOT_FOUND	4
#define GMTAPI_BAD_PERMISSION	5
#define GMTAPI_GRID_READ_ERROR	6
#define GMTAPI_GRID_WRITE_ERROR	7
#define GMTAPI_DATA_READ_ERROR	8
#define GMTAPI_DATA_WRITE_ERROR	9
#define GMTAPI_N_COLS_VARY	10
#define GMTAPI_NO_INPUT		11
#define GMTAPI_NO_OUTPUT	12

	/* Index parameters used to access the information arrays */

#define GMTAPI_TYPE		0	/* arg[0] = data type (GMTAPI_{BYTE|SHORT|FLOAT|INT|DOUBLE}) */
#define GMTAPI_NDIM		1	/* arg[1] = dimensionality of data (1, 2, or 3) (GMT grids = 2 yet stored internally as 1D) */
#define GMTAPI_NROW		2	/* arg[2] = number_of_rows (or length of 1-D array) */
#define GMTAPI_NCOL		3	/* arg[3] = number_of_columns (1 for 1-D array) */
#define GMTAPI_KIND		4	/* arg[4] = arrangment of rows/col (0 = rows (C), 1 = columns (Fortran)) */
#define GMTAPI_DIML		5	/* arg[5] = length of dimension for row (C) or column (Fortran) */
#define GMTAPI_FREE		6	/* arg[6] = 1 to free array after use, 0 to leave alone */
#define GMTAPI_NODE		7	/* arg[7] = 1 for pixel registration, 0 for node */
#define GMTAPI_XMIN		8	/* arg[8] = x_min (west) of grid */
#define GMTAPI_XMAX		9	/* arg[9] = x_max (east) of grid */
#define GMTAPI_YMIN		10	/* arg[10] = y_min (south) of grid */
#define GMTAPI_YMAX		11	/* arg[11] = y_max (north) of grid */

	/* Data primitive identifiers */

#define GMTAPI_N_TYPES		5	/* The number of supported data types (below) */
#define GMTAPI_BYTE		0	/* The 1-byte data integer type */
#define GMTAPI_SHORT		1	/* The 2-byte data integer type */
#define GMTAPI_INT		2	/* The 4-byte data integer type */
#define GMTAPI_FLOAT		3	/* The 4-byte data float type */
#define GMTAPI_DOUBLE		4	/* The 8-byte data float type */

	/* Array ordering constants */
	
#define GMTAPI_ORDER_ROW	0	/* C-style array order: as index increase we move across rows */
#define GMTAPI_ORDER_COL	1	/* Fortran-style array order: as index increase we move down columns */

/*=====================================================================================
 *	GMT API STRUCTURE DEFINITIONS
 *=====================================================================================
 */

struct GMTAPI_CTRL {
	/* Master controller which holds all GMT API related information at run-time.
	 * It is expected that users can run several GMT sessions concurrently when
	 * GMT 5 moves from using global data to passing a GMT structure. */
	 
	int n_data;				/* Number of currently active data objects */
	int n_alloc;				/* Allocation counter */
	int GMTAPI_size[GMTAPI_N_TYPES];	/* Size of various data types in bytes */
	struct GMTAPI_DATA_OBJECT **data;	/* List of registered data objects */
	PFI GMT_2D_to_index[2];			/* Pointers to the row or column-order index functions */
	PFV GMT_index_to_2D[2];			/* Pointers to the inverse index functions */
};

/*=====================================================================================
 *	GMT API LIBRARY_SPECIFIC FUNCTION PROTOTYPES
 *=====================================================================================
 */

extern int GMT_Create_Session  (struct GMTAPI_CTRL **GMT, int flags);
extern int GMT_Destroy_Session (struct GMTAPI_CTRL *GMT);
extern int GMT_Register_Import  (struct GMTAPI_CTRL *GMT, int method, void **source,   double parameters[]);
extern int GMT_Register_Export (struct GMTAPI_CTRL *GMT, int method, void **receiver, double parameters[]);
extern void GMT_Error (struct GMTAPI_CTRL *GMT, int error);

/*=====================================================================================
 *	GMT API GMT FUNCTION PROTOTYPES
 *=====================================================================================
 */

extern int GMT_read_all_write_all_records (struct GMTAPI_CTRL *GMT, char *command, int inarg[], int outarg);
extern int GMT_read_one_write_one_record  (struct GMTAPI_CTRL *GMT, char *command, int inarg[], int outarg);

#endif /* _GMTAPI_H */
