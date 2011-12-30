/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2011 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
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
 * Public function prototypes for GMT API session manipulations and data i/o.
 *
 * Author: 	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	1.1
 *
 * The API consists of 16 functions plus all the GMT_<modules>.
 *
 * There are 2 public functions used for GMT API session handling.
 * This part of the API helps the developer create and delete GMT sessions:
 *
 * GMT_Create_Session	: Initialize a new GMT session
 * GMT_Destroy_Session	: Destroy a GMT session
 *
 * There is 1 public function for common error reporting.
 * Errors will be reported to stderr or selected log file:
 *
 * GMT_Report_Error	: Report an error given an error code
 *
 * There are 13 further public functions used for GMT i/o activities:
 *
 * GMT_Encode_ID	: Encode a resource ID in a file name
 * GMT_Register_IO	: Register a source or destination for i/o use
 * GMT_Init_IO		: Initialize i/o machinery before program use
 * GMT_Begin_IO		: Allow i/o to take place
 * GMT_End_IO		: Disallow further i/o
 * GMT_Read_Data	: Load data set into program memory from selected source
 * GMT_Get_Data		: Load data set into program memory from registered source
 * GMT_Retrieve_Data	: Retrieve pointer to registered container with data
 * GMT_Write_Data	: Place data set from program memory to selected destination
 * GMT_Put_Data		: Place data set from program memory to registered destination
 * GMT_Get_Record	: Get the next single data record from the source(s)
 * GMT_Put_Record	: Send the next output record to its destination
 * GMT_Create_Data	: Return an empty container for a data set
 * GMT_Destroy_Data	: Destroy a data set and its container
 *
 * The above functions deal with registration of input sources (files,
 * streams, file handles, or memory locations) and output destinations
 * (same flavors as input), the setup of the i/o, and generic functions
 * to access the data either in one go (GMT_Get|Put_Data) or on a
 * record-by-record basis (GMT_Get|Put_Record).  Finally, data sets that
 * are allocated can then be destroyed when no longer needed.
 * These 14 functions have Fortran bindings as well, provided you run
 * configure with --enable-fapi.
 *
 */

#include "pslib.h"
#include "gmt.h"
#include "gmt_internals.h"

#ifdef FORTRAN_API
static struct GMTAPI_CTRL *GMT_FORTRAN = NULL;	/* Global structure needed for FORTRAN-77 [PW: not tested yet - is it even needed?] */
#endif

static GMT_LONG GMTAPI_session_counter = 0;	/* Keeps track of the ID of new sessions for multi-session programs */
static char *GMTAPI_errstr[] = {
#include "gmtapi_errstr.h"
};

/* Macros that report error, then return NULL pointer, TRUE, or a value, respectively */
#define return_null(API,err) { GMT_Report_Error(API,err); return (NULL);}
#define return_error(API,err) { GMT_Report_Error(API,err); return (TRUE);}
#define return_value(API,err,val) { GMT_Report_Error(API,err); return (val);}

/* Misc. local text strings needed in this file only, used when debug verbose is on (-V4) */

static const char *GMT_method[GMT_N_METHODS] = {"File", "Stream", "File Descriptor", "Memory Copy", "Memory Reference", "Read-only Memory"};
static const char *GMT_family[GMT_N_FAMILIES] = {"Data Table", "Text Table", "GMT Grid", "CPT Table", "GMT Image"};
static const char *GMT_via[2] = {"User Vector", "User Matrix"};
static const char *GMT_direction[2] = {"Input", "Output"};
static const char *GMT_stream[2] = {"Standard", "User-supplied"};
static const char *GMT_status[3] = {"Unused", "In-use", "Used"};
static const char *GMT_geometry[GMT_N_GEOMETRIES] = {"None", "Point", "Line", "Polygon", "Surface"};

/*==================================================================================================
 *		PRIVATE FUNCTIONS ONLY USED BY THIS LIBRARY FILE
 *==================================================================================================
 */

/* We also need to return the pointer to an object given a void * address of that pointer.
 * This needs to be done on a per data-type basis, e.g., to cast that void * to a struct GMT_GRID **
 * so we may return the value at that address: */

struct GMT_PALETTE * gmt_get_cpt_ptr (struct GMT_PALETTE **ptr) {return (*ptr);}
struct GMT_DATASET * gmt_get_dataset_ptr (struct GMT_DATASET **ptr) {return (*ptr);}
struct GMT_TEXTSET * gmt_get_textset_ptr (struct GMT_TEXTSET **ptr) {return (*ptr);}
struct GMT_GRID    * gmt_get_grid_ptr (struct GMT_GRID **ptr) {return (*ptr);}
struct GMT_MATRIX  * gmt_get_matrix_ptr (struct GMT_MATRIX **ptr) {return (*ptr);}
struct GMT_VECTOR  * gmt_get_vector_ptr (struct GMT_VECTOR **ptr) {return (*ptr);}
#ifdef USE_GDAL
struct GMT_IMAGE   * gmt_get_image_ptr (struct GMT_IMAGE **ptr) {return (*ptr);}
#endif

/* return_address is a convenience function that, given type, calls the correct converter */
void *return_address (void *data, GMT_LONG type) {
	void *p = NULL;
	switch (type) {
		case GMT_IS_GRID:	p = gmt_get_grid_ptr (data);	break;
		case GMT_IS_DATASET:	p = gmt_get_dataset_ptr (data);	break;
		case GMT_IS_TEXTSET:	p = gmt_get_textset_ptr (data);	break;
		case GMT_IS_CPT:	p = gmt_get_cpt_ptr (data);	break;
		case GMT_IS_MATRIX:	p = gmt_get_matrix_ptr (data);	break;
		case GMT_IS_VECTOR:	p = gmt_get_vector_ptr (data);	break;
#ifdef USE_GDAL
		case GMT_IS_IMAGE:	p = gmt_get_image_ptr (data);	break;
#endif
	}
	return (p);
}

/* Note: Many/all of these do not need to check if API == NULL since they are called from functions that do. */
/* Private functions used by this library only.  These are not accessed outside this file. */

void GMT_io_banner (struct GMT_CTRL *C, GMT_LONG direction)
{	/* Write verbose message about binary record i/o format */
	char message[GMT_TEXT_LEN256], skip[GMT_TEXT_LEN64];
	char *letter = "chilfd", s[2] = {0, 0};
	GMT_LONG col;
	
	if (C->current.setting.verbose < GMT_MSG_NORMAL) return;	/* Not in verbose mode anyway */
	if (!C->common.b.active[direction]) return;	/* Not using binary i/o */
	GMT_memset (message, GMT_TEXT_LEN256, char);	/* Start with a blank message */
	for (col = 0; col < C->common.b.ncol[direction]; col++) {	/* For each binary column of data */
		if (C->current.io.fmt[direction][col].skip < 0) {	/* Must skip BEFORE reading this column */
			sprintf (skip, "%ldx", -C->current.io.fmt[direction][col].skip);
			strcat (message, skip);
		}
		s[0] = letter[C->current.io.fmt[direction][col].type];	/* Get data type code */
		strcat (message, s);
		if (C->current.io.fmt[direction][col].skip > 0) {	/* Must skip AFTER reading this column */
			sprintf (skip, "%ldx", C->current.io.fmt[direction][col].skip);
			strcat (message, skip);
		}
	}
	GMT_report (C, GMT_MSG_NORMAL, "%s %ld columns via binary records using format %s\n", GMT_direction[direction], C->common.b.ncol[direction], message);
}

double GMTAPI_get_val (struct GMTAPI_CTRL *API, union GMT_UNIVECTOR *u, GMT_LONG row, GMT_LONG type)
{	/* Returns a double value from the <type> column array pointed to by the union pointer *u, at row position row.
 	 * Used in GMTAPI_Import_Dataset and GMTAPI_Import_Grid. */
	double val;
	
	switch (type) {	/* Use type to select the correct array from which to extract a value */
		case GMTAPI_UCHAR:	val = u->uc1[row];	break;
		case GMTAPI_CHAR:	val = u->sc1[row];	break;
		case GMTAPI_USHORT:	val = u->ui2[row];	break;
		case GMTAPI_SHORT:	val = u->si2[row];	break;
		case GMTAPI_UINT:	val = u->ui4[row];	break;
		case GMTAPI_INT:	val = u->si4[row];	break;
		case GMTAPI_ULONG:	val = u->ui8[row];	break;
		case GMTAPI_LONG:	val = u->si8[row];	break;
		case GMTAPI_FLOAT:	val = u->f4[row];	break;
		case GMTAPI_DOUBLE:	val = u->f8[row];	break;
		default:
			fprintf (stderr, "GMT API: Internal error in GMTAPI_get_val: Passed bad type (%" GMT_LL "d)\n", type);
			val = 0.0;
			API->error = GMT_NOT_A_VALID_TYPE;
			break;
	}
	return (val);
}

void GMTAPI_put_val (struct GMTAPI_CTRL *API, union GMT_UNIVECTOR *u, double val, GMT_LONG row, GMT_LONG type)
{	/* Places a double value in the <type> column array[i] pointed to by the union pointer *u, at row position row.
 	 * No check to see if the type can hold the value is performed, so truncation may result.
 	 * Used in GMTAPI_Export_Dataset and GMTAPI_Export_Grid. */
	
	switch (type) {/* Use type to select the correct array to which we will put a value */
		case GMTAPI_UCHAR:	u->uc1[row] = (unsigned char)val;	break;
		case GMTAPI_CHAR:	u->sc1[row] = (char)val;		break;
		case GMTAPI_USHORT:	u->ui2[row] = (unsigned short int)val;	break;
		case GMTAPI_SHORT:	u->si2[row] = (short int)val;		break;
		case GMTAPI_UINT:	u->ui4[row] = (unsigned int)val;	break;
		case GMTAPI_INT:	u->si4[row] = (int)val;			break;
		case GMTAPI_ULONG:	u->ui8[row] = (GMT_ULONG)val;		break;
		case GMTAPI_LONG:	u->si8[row] = (GMT_LONG)val;		break;
		case GMTAPI_FLOAT:	u->f4[row] = (float)val;		break;
		case GMTAPI_DOUBLE:	u->f8[row] = val;			break;
		default:
			fprintf (stderr, "GMT API: Internal error in GMTAPI_put_val: Passed bad type (%" GMT_LL "d)\n", type);
			API->error = GMT_NOT_A_VALID_TYPE;
			break;
	}
}

GMT_LONG GMTAPI_n_items (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG direction, GMT_LONG *first_ID)
{	/* Count how many data sets of the given family are currently registered and unused for the given direction (GMT_IN|GMT_OUT).
 	 * Also return the ID of the first data object for the given direction (GMTAPI_NOTSET if not found).
	 */
	GMT_LONG i, n;	
	
	*first_ID = GMTAPI_NOTSET;	/* Not found yet */
	API->error = GMT_OK;		/* No error yet */
	for (i = n = 0; i < API->n_objects; i++) {
		if (!API->object[i]) continue;				/* A freed object, skip */
		if (API->object[i]->direction != direction) continue;	/* Wrong direction */
		if (API->object[i]->status != GMT_IS_UNUSED) continue;	/* Already used */
		if (family != API->object[i]->family) continue;		/* Wrong data type */
		n++;
		if (*first_ID == GMTAPI_NOTSET) *first_ID = API->object[i]->ID;
	}
	return (n);
}

/* Mapping of internal [row][col] indices to a single 1-D index.
 * Internally, row and col starts at 0.  These will be accessed
 * via pointers to these functions, hence they are not macros.
 */

GMT_LONG GMTAPI_2D_to_index_C_normal (GMT_LONG row, GMT_LONG col, GMT_LONG dim)
{	/* Maps (row,col) to 1-D index for C normal grid */
	return ((row * dim) + col);	/* Normal grid */
}

GMT_LONG GMTAPI_2D_to_index_C_cplx_real (GMT_LONG row, GMT_LONG col, GMT_LONG dim)
{	/* Maps (row,col) to 1-D index for C complex grid, real component */
	return (2*(row * dim) + col);	/* Complex grid, real(1) component */
}

GMT_LONG GMTAPI_2D_to_index_C_cplx_imag (GMT_LONG row, GMT_LONG col, GMT_LONG dim)
{	/* Maps (row,col) to 1-D index for C complex grid, imaginary component */
	return (2*(row * dim) + col + 1);	/* Complex grid, imag(2) component */
}

GMT_LONG GMTAPI_2D_to_index_F_normal (GMT_LONG row, GMT_LONG col, GMT_LONG dim)
{	/* Maps (row,col) to 1-D index for Fortran */
	return ((col * dim) + row);
}

GMT_LONG GMTAPI_2D_to_index_F_cplx_real (GMT_LONG row, GMT_LONG col, GMT_LONG dim)
{	/* Maps (row,col) to 1-D index for Fortran complex grid, real component */
	return (2*(col * dim) + row);	/* Complex grid, real(1) */
}

GMT_LONG GMTAPI_2D_to_index_F_cplx_imag (GMT_LONG row, GMT_LONG col, GMT_LONG dim)
{	/* Maps (row,col) to 1-D index for Fortran complex grid, imaginary component  */
	return (2*(col * dim) + row + 1);	/* Complex grid, imag(2) component */
}

PFL GMTAPI_get_2D_to_index (GMT_LONG shape, GMT_LONG mode)
{
	/* Return pointer to the required 2D-index function above.  Here
	 * shape is either GMTAPI_ORDER_ROW (C) or GMTAPI_ORDER_COL (FORTRAN);
	 * mode is either 0 (regular grid), 1 (complex real) or 2 (complex imag)
	 */
	PFL p = NULL;
	
	switch (mode) {
		case GMT_IS_NORMAL:
			p = (shape == GMTAPI_ORDER_ROW) ? GMTAPI_2D_to_index_C_normal : GMTAPI_2D_to_index_F_normal;
			break;
		case GMT_IS_COMPLEX_REAL:
			p = (shape == GMTAPI_ORDER_ROW) ? GMTAPI_2D_to_index_C_cplx_real : GMTAPI_2D_to_index_F_cplx_real;
			break;
		case GMT_IS_COMPLEX_IMAG:
			p = (shape == GMTAPI_ORDER_ROW) ? GMTAPI_2D_to_index_C_cplx_imag : GMTAPI_2D_to_index_F_cplx_imag;
			break;
		default:
			fprintf (stderr, "GMTAPI_get_2D_to_index: Illegal mode passed - aborting\n");
			GMT_exit (-1);
	}
	return (p);
}

#if 0	/* Unused at the present time */
void GMTAPI_index_to_2D_C (GMT_LONG *row, GMT_LONG *col, GMT_LONG index, GMT_LONG dim, GMT_LONG mode)
{	/* Maps 1-D index to (row,col) for C */
	if (mode) index /= 2;
	*col = (index % dim);
	*row = (index / dim);
}

void GMTAPI_index_to_2D_F (GMT_LONG *row, GMT_LONG *col, GMT_LONG index, GMT_LONG dim, GMT_LONG mode)
{	/* Maps 1-D index to (row,col) for Fortran */
	if (mode) index /= 2;
	*col = (index / dim);
	*row = (index % dim);
}
#endif

void GMTAPI_grdheader_to_info (struct GRD_HEADER *h, struct GMT_MATRIX *M)
{	/* Packs the necessary items of the grid header into the matrix parameters */
	M->n_columns = h->nx;	/* Cast to GMT_LONG due to definition of GRD_HEADER */
	M->n_rows = h->ny;
	M->registration = h->registration;
	GMT_memcpy (M->limit, h->wesn, 4, double);
}

void GMTAPI_info_to_grdheader (struct GMT_CTRL *C, struct GRD_HEADER *h, struct GMT_MATRIX *M)
{	/* Unpacks the necessary items into the grid header from the matrix parameters */
	h->nx = (int)M->n_columns;	/* Cast to int due to definition of GRD_HEADER */
	h->ny = (int)M->n_rows;
	h->registration = (int)M->registration;
	GMT_memcpy (h->wesn, M->limit, 4, double);
	/* Compute xy_off and increments */
	h->xy_off = (h->registration == GMT_GRIDLINE_REG) ? 0.0 : 0.5;
	h->inc[GMT_X] = GMT_get_inc (C, h->wesn[XLO], h->wesn[XHI], h->nx, h->registration);
	h->inc[GMT_Y] = GMT_get_inc (C, h->wesn[YLO], h->wesn[YHI], h->ny, h->registration);
}

GMT_LONG GMTAPI_need_grdpadding (struct GRD_HEADER *h, GMT_LONG *pad)
{	/* Compares current grid pad status to output pad requested.  If we need
	 * to add a pad we return TRUE here, otherwise FALSE. */
	GMT_LONG k;
	
	for (k = 0; k < 4; k++) if (h->pad[k] < pad[k]) return (TRUE);
	return (FALSE);
}

GMT_LONG GMTAPI_set_grdarray_size (struct GMT_CTRL *C, struct GRD_HEADER *h, double *wesn)
{	/* Determines size of grid given grid spacing and grid domain in h.
 	 * However, if wesn is given and not empty we use that sub-region instead.
 	 * Finally, the current pad is used when calculating the grid size.
	 * NOTE: This function leaves h unchanged by testing on a temporary header. */
	struct GRD_HEADER *h_tmp = NULL;
	GMT_LONG size;
	
	/* Must duplicate header and possibly reset wesn, then set pad and recalculate the dims */
	h_tmp = GMT_memory (C, NULL, 1, struct GRD_HEADER);
	GMT_memcpy (h_tmp, h, 1, struct GRD_HEADER);
	
	if (wesn && !(wesn[XLO] == wesn[XHI] && wesn[YLO] == wesn[YHI])) GMT_memcpy (h_tmp->wesn, wesn, 4, double);	/* Use wesn instead of header info */
	GMT_grd_setpad (C, h_tmp, C->current.io.pad);	/* Use the system pad setting by default */
	GMT_set_grddim (C, h_tmp);			/* Computes all integer parameters */
	size = h_tmp->size;				/* This is the size needed to hold grid + padding */
	GMT_free (C, h_tmp);
	return (size);
}

GMT_LONG GMTAPI_Next_IO_Source (struct GMTAPI_CTRL *API, GMT_LONG direction)
{	/* Get ready for the next source/destination (open file, initialize counters, etc.).
	 * Note this is only a mechanism for dataset and textset files where it is common
	 * to give many files on the command line (e.g., *.txt) and we do rec-by-rec processing. */
	int *fd = NULL;	/* !!! Must be int* due to nature of Unix system function */
	GMT_LONG error = 0, kind, via = 0;
	static const char *dir[2] = {"from", "to"};
	static const char *operation[2] = {"Reading", "Writing"};
	char *mode = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMT_VECTOR *V = NULL;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	
	API->error = GMT_OK;		/* No error yet */
	S = API->object[API->current_item[direction]];		/* For shorthand purposes only */
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Next_IO_Source: Selected object %" GMT_LL "d\n", S->ID);
	mode = (direction == GMT_IN) ? API->GMT->current.io.r_mode : API->GMT->current.io.w_mode;	/* Reading or writing */
	S->close_file = FALSE;		/* Do not want to close file pointers passed to us unless WE open them below */
	/* Either use binary n_columns settings or initialize to unknown, i.e., GMT_MAX_COLUMNS */
	S->n_expected_fields = (API->GMT->common.b.ncol[direction]) ? API->GMT->common.b.ncol[direction] : GMT_MAX_COLUMNS;
	GMT_memset (API->GMT->current.io.curr_pos[direction], 3, GMT_LONG);	/* Reset file, seg, point counters */
	if (S->method >= GMT_VIA_VECTOR) via = (S->method / GMT_VIA_VECTOR) - 1;
	
	switch (S->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:	/* Filename given; we must open file ourselves */
			if (S->family == GMT_IS_GRID) return (GMT_Report_Error (API, GMT_NOT_A_VALID_TYPE));	/* Grids not allowed here */
			if ((S->fp = GMT_fopen (API->GMT, S->filename, mode)) == NULL) {	/* Trouble opening file */
				GMT_report (API->GMT, GMT_MSG_FATAL, "Unable to open file %s for %s\n", S->filename, GMT_direction[direction]);
				return (GMT_ERROR_ON_FOPEN);
			}
			S->close_file = TRUE;	/* We do want to close files we are opening, but later */
			strcpy (API->GMT->current.io.current_filename[direction], S->filename);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "%s %s %s file %s\n", 
				operation[direction], GMT_family[S->family], dir[direction], S->filename);
			if (GMT_binary_header (API->GMT, direction)) {
				GMT_io_binary_header (API->GMT, S->fp, direction);
				GMT_report (API->GMT, GMT_MSG_FATAL, "%s %ld bytes of header %s binary file %s\n",
					operation[direction], API->GMT->current.io.io_n_header_items, dir[direction], S->filename);
			}
			break;
			
		case GMT_IS_STREAM:	/* Given a stream; no need to open (or close) anything */
#ifdef SET_IO_MODE
			if (S->family == GMT_IS_DATASET && S->fp == API->GMT->session.std[direction]) 
				GMT_setmode (API->GMT, (int)direction);	/* Windows may need to have its read mode changed from text to binary */
#endif
			kind = (S->fp == API->GMT->session.std[direction]) ? 0 : 1;	/* 0 if stdin/out, 1 otherwise for user pointer */
			sprintf (API->GMT->current.io.current_filename[direction], "<%s %s>", GMT_stream[kind], GMT_direction[direction]);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "%s %s %s %s %s stream\n", 
				operation[direction], GMT_family[S->family], dir[direction], GMT_stream[kind], GMT_direction[direction]);
			if (GMT_binary_header (API->GMT, direction)) {
				GMT_io_binary_header (API->GMT, S->fp, direction);
				GMT_report (API->GMT, GMT_MSG_FATAL, "%s %ld bytes of header %s binary %s stream\n",
					operation[direction], API->GMT->current.io.io_n_header_items, dir[direction], GMT_stream[kind]);
			}
			break;
			
		case GMT_IS_FDESC:	/* Given a pointer to a file handle; otherwise same as stream */
			fd = (int *)S->fp;
			if ((S->fp = GMT_fdopen (*fd, mode)) == NULL) {	/* Reopen handle as stream */
				GMT_report (API->GMT, GMT_MSG_FATAL, "Unable to open file descriptor %d for %s\n", *fd, GMT_direction[direction]);
				return (GMT_ERROR_ON_FDOPEN);
			}
			kind = (S->fp == API->GMT->session.std[direction]) ? 0 : 1;	/* 0 if stdin/out, 1 otherwise for user pointer */
			sprintf (API->GMT->current.io.current_filename[direction], "<%s %s>", GMT_stream[kind], GMT_direction[direction]);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "%s %s %s %s %s stream via supplied file descriptor\n", 
				operation[direction], GMT_family[S->family], dir[direction], GMT_stream[kind], GMT_direction[direction]);
			if (GMT_binary_header (API->GMT, direction)) {
				GMT_io_binary_header (API->GMT, S->fp, direction);
				GMT_report (API->GMT, GMT_MSG_FATAL, "%s %ld bytes of header %s binary %s stream via supplied file descriptor\n",
					operation[direction], API->GMT->current.io.io_n_header_items, dir[direction], GMT_stream[kind]);
			}
			break;
			
	 	case GMT_IS_COPY:	/* Copy, nothing to do [PW: not tested] */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "%s %s %s memory copy supplied by pointer\n", 
				operation[direction], GMT_family[S->family], dir[direction]);
			break;

	 	case GMT_IS_REF:	/* Reference, nothing to do [PW: not tested] */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "%s %s %s memory reference supplied by pointer\n", 
				operation[direction], GMT_family[S->family], dir[direction]);
			break;

	 	case GMT_IS_COPY + GMT_VIA_MATRIX:	/* This means getting a dataset via a user matrix [PW: not tested] */
			if (S->family != GMT_IS_DATASET) return (GMT_Report_Error (API, GMT_NOT_A_VALID_TYPE));
			GMT_report (API->GMT, GMT_MSG_NORMAL, "%s %s %s %s memory location via %s\n", 
				operation[direction], GMT_family[S->family], dir[direction], GMT_direction[direction], GMT_via[via]);
			M = S->resource;
			if (direction == GMT_OUT && M->alloc_mode == 1) {	/* Must allocate output space */
				S->n_alloc = GMT_CHUNK * M->n_columns;
				/* S->n_rows is 0 which means we are allocating more space as we need it */
				if ((error = GMT_alloc_univector (API->GMT, &(M->data), M->type, S->n_alloc)) != GMT_OK) return (GMT_Report_Error (API, error));
			}
			else
				S->n_rows = M->n_rows;	/* Hard-wired limit as pass in from calling program */
			API->GMT->common.b.ncol[direction] = M->n_columns;	/* Basically, we are doing what GMT calls binary i/o */
			API->GMT->common.b.active[direction] = TRUE;
			strcpy (API->GMT->current.io.current_filename[direction], "<memory>");
			break;
			
		 case GMT_IS_READONLY + GMT_VIA_VECTOR:	/* These 3 means getting a data table via user vector arrays */
		 case GMT_IS_REF + GMT_VIA_VECTOR:
		 case GMT_IS_COPY + GMT_VIA_VECTOR:
			if (S->family != GMT_IS_DATASET) return (GMT_Report_Error (API, GMT_NOT_A_VALID_TYPE));
			GMT_report (API->GMT, GMT_MSG_NORMAL, "%s %s %s %s memory location via %s\n", 
					operation[direction], GMT_family[S->family], dir[direction], GMT_direction[direction], GMT_via[via]);
			V = S->resource;
			if (direction == GMT_OUT && V->alloc_mode == 1) {	/* Must allocate output space */
				S->n_alloc = GMT_CHUNK;
				/* S->n_rows is 0 which means we are allocating more space as we need it */
				if ((error = GMT_alloc_vectors (API->GMT, V, S->n_alloc)) != GMT_OK) return (GMT_Report_Error (API, error));
			}
			else
				S->n_rows = V->n_rows;	/* Hard-wired limit as passed in from calling program */
			S->n_columns = V->n_columns;
			API->GMT->common.b.ncol[direction] = V->n_columns;	/* Basically, we are doing what GMT calls binary i/o */
			API->GMT->common.b.active[direction] = TRUE;
			strcpy (API->GMT->current.io.current_filename[direction], "<memory>");
			break;

		default:
			GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: Internal error: GMTAPI_Next_IO_Source called with illegal method\n");
			break;
	}

	/* A few things pertaining only to data/text tables */
	API->GMT->current.io.rec_in_tbl_no = -1;	/* Start on new table */
	S->import = (S->family == GMT_IS_TEXTSET) ? (PFP)GMT_ascii_textinput : API->GMT->current.io.input;	/* The latter may point to ascii or binary input functions */

	return (GMT_OK);		
}

GMT_LONG GMTAPI_Next_Data_Object (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG direction)
{	/* Sets up current_item to be the next unused item of the required direction; or return EOF.
	 * When EOF is returned, API->current_item[direction] holds the last object ID used. */
	GMT_LONG found = FALSE, k;
	
	API->error = GMT_OK;		/* No error yet */
	k = API->current_item[direction] + 1;	/* Advance to next item, if possible */
	while (k < API->n_objects && !found) {
		if (API->object[k] && API->object[k]->status == GMT_IS_UNUSED && API->object[k]->direction == direction && family == API->object[k]->family)
			found = TRUE;	/* Got item that is unused, has correct direction and family */
		else
			k++;	/* No, keep looking */
	}
	if (found) {	/* Update to use next item */
		API->current_item[direction] = k;	/* The next item */
		return (GMTAPI_Next_IO_Source (API, direction));	/* Initialize the next source/destination */
	}
	else
		return (EOF);	/* No more objects available for this direction; return EOF */
}				

GMT_LONG GMTAPI_Add_Data_Object (struct GMTAPI_CTRL *API, struct GMTAPI_DATA_OBJECT *object)
{	/* Hook object to end of linked list and assign unique id (> 0) which is returned */
	
	/* Find the first entry in the API->object array which is unoccupied, and if
	 * they are all occupied then reallocate the array to make more space.
	 * We thus find and return the lowest available ID. */
	GMT_LONG object_ID;
	
	API->error = GMT_OK;		/* No error yet */
	API->n_objects++;		/* Add one more entry to the tally */
	if (API->n_objects == API->n_objects_alloc) {	/* Must allocate more space for more data descriptors */
		GMT_LONG old_n_alloc = API->n_objects_alloc;
		API->n_objects_alloc += GMT_SMALL_CHUNK;
		API->object = GMT_memory (API->GMT, API->object, API->n_objects_alloc, struct GMTAPI_DATA_OBJECT *);
		GMT_memset (&(API->object[old_n_alloc]), API->n_objects_alloc - old_n_alloc, struct GMTAPI_DATA_OBJECT *);	/* Set to NULL */
		if (!(API->object)) {	/* Failed to allocate more memory */
			API->n_objects--;	/* Undo our premature increment */
			return_value (API, GMT_MEMORY_ERROR, GMTAPI_NOTSET);
		}
	}
	object_ID = object->ID = API->unique_ID++;	/* Assign a unique object ID */
	API->object[API->n_objects-1] = object;		/* Hook the current object onto the end of the list */

	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Add_Data_Object: Added new object %" GMT_LL "d: method = %s geometry = %s direction = %s\n",
		object->ID, GMT_method[object->method], GMT_geometry[object->geometry], GMT_direction[object->direction]);
	
	return (object_ID);		
}

GMT_LONG GMTAPI_Get_Item (struct GMTAPI_CTRL *API, GMT_LONG object_ID, GMT_LONG direction)
{	/* Checks to see if the given object_ID is listed and of the right direction.  If so
 	 * we return the item number; otherwise return GMTAPI_NOTSET and set API->error to the error code. */
	GMT_LONG i, item;

	 /* Search for the object in the active list.  However, if object_ID == GMTAPI_NOTSET we pick the first in that direction */
	
	API->error = GMT_OK;		/* No error yet */
	for (i = 0, item = GMTAPI_NOTSET; item == GMTAPI_NOTSET && i < API->n_objects; i++) {
		if (!API->object[i]) continue;	/* Empty object */
		if (direction == GMTAPI_NOTSET && API->object[i]->ID == object_ID) item = i;	/* Pick the requested object regardless of direction */
		else if (API->object[i]->ID == object_ID) item = i;					/* Pick the requested object */
	}
	if (item == GMTAPI_NOTSET) { API->error = GMT_NOT_A_VALID_ID; return (GMTAPI_NOTSET); }		/* No such object found */

	/* OK, we found the object; is it the right kind (input or output)? */
	if (direction != GMTAPI_NOTSET && API->object[item]->direction != direction) {
		/* Passing an input object but it is listed as output, or vice versa */
		if (direction == GMT_IN)  { API->error = GMT_NOT_INPUT_OBJECT;  return (GMTAPI_NOTSET); }
		if (direction == GMT_OUT) { API->error = GMT_NOT_OUTPUT_OBJECT; return (GMTAPI_NOTSET); }
	}
	/* Here we have been successful in finding the right object */
	return (item);		
}

GMT_LONG GMTAPI_Validate_ID (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG object_ID, GMT_LONG direction)
{	/* Checks to see if the given object_ID is listed and of the right direction.  If so
 	 * we return the item number; otherwise return GMTAPI_NOTSET and set API->error to the error code. */
	GMT_LONG i, item;

	 /* Search for the object in the active list.  However, if object_ID == GMTAPI_NOTSET we pick the first in that direction */
	
	API->error = GMT_OK;		/* No error yet */
	for (i = 0, item = GMTAPI_NOTSET; item == GMTAPI_NOTSET && i < API->n_objects; i++) {
		if (!API->object[i]) continue;	/* Empty object */
		if (direction != GMTAPI_NOTSET && API->object[i]->status != GMT_IS_UNUSED) continue;	/* Already used this object */
		if (!(family == GMTAPI_NOTSET || family == API->object[i]->family)) continue;		/* Not the required data type */
		if (object_ID == GMTAPI_NOTSET && direction == API->object[i]->direction) item = i;	/* Pick the first object with the specified direction */
		else if (direction == GMTAPI_NOTSET && API->object[i]->ID == object_ID) item = i;	/* Pick the requested object regardless of direction */
		else if (API->object[i]->ID == object_ID) item = i;					/* Pick the requested object */
	}
	if (item == GMTAPI_NOTSET) { API->error = GMT_NOT_A_VALID_ID; return (GMTAPI_NOTSET); }		/* No such object found */

	/* OK, we found the object; is it the right kind (input or output)? */
	if (direction != GMTAPI_NOTSET && API->object[item]->direction != direction) {
		/* Passing an input object but it is listed as output, or vice versa */
		if (direction == GMT_IN)  { API->error = GMT_NOT_INPUT_OBJECT;  return (GMTAPI_NOTSET); }
		if (direction == GMT_OUT) { API->error = GMT_NOT_OUTPUT_OBJECT; return (GMTAPI_NOTSET); }
	}
	/* Here we have been successful in finding the right object */
	return (item);		
}

GMT_LONG GMTAPI_Decode_ID (char *filename)
{	/* Checking if filename contains a name with embedded GMTAPI Object ID.
	 * If found we return the ID, otherwise we return GMTAPI_NOTSET.
 	*/
	GMT_LONG object_ID = GMTAPI_NOTSET;
	
	if (GMT_File_Is_Memory (filename)) {	/* Passing ID of an already registered object */
		if (sscanf (&filename[9], "%" GMT_LL "d", &object_ID) != 1) return (GMTAPI_NOTSET);	/* Get the object ID unless we fail scanning */
	}
	return (object_ID);	/* Returns GMTAPI_NOTSET if no embedded ID was found */
}

GMT_LONG GMTAPI_ptr2id (struct GMTAPI_CTRL *API, void *ptr)
{	/* Returns the ID of the first object whose data pointer matches ptr.
 	 * This is necessary since many objects may have the same pointer
	 * but we only want to destroy the memory once.  This function is
	 * only used in GMT_Destroy_Data.
	 */
	GMT_LONG i, object_ID = GMTAPI_NOTSET;	/* Not found yet */
	void *data = NULL;
	
	for (i = 0; object_ID == GMTAPI_NOTSET && i < API->n_objects; i++) {	/* Loop over all objects */
		if (!API->object[i]) continue;	/* Skip freed objects */
		data = return_address (ptr, API->object[i]->family);	/* Get void* pointer to resource */
		if (API->object[i]->data == data && object_ID == GMTAPI_NOTSET) object_ID = API->object[i]->ID;	/* Found a matching data pointer */
	}
	return (object_ID);	/* Return ID or -1 if not found */	
}

GMT_LONG GMTAPI_is_registered (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG geometry, GMT_LONG direction, void *resource)
{	/* Checks to see if the given data pointer has already been registered.
 	 * This can happen for grids which first gets registered reading the header
 	 * and then is registered again when reading the whole grid.  In those cases
	 * we dont want to register them twice.
	 */
	GMT_LONG i, item;

	if (API->n_objects == 0) return (GMTAPI_NOTSET);	/* There are no known resources yet */
	
	 /* Search for the object in the active list.  However, if object_ID == GMTAPI_NOTSET we pick the first in that direction */
	
	for (i = 0, item = GMTAPI_NOTSET; item == GMTAPI_NOTSET && i < API->n_objects; i++) {
		if (!API->object[i]) continue;					/* Empty object */
		if (API->object[i]->status != GMT_IS_UNUSED) continue;		/* Finished with this one unless it is reset */
		if (API->object[i]->direction != direction) continue;		/* Wrong direction */
		if (API->object[i]->family != family) continue;			/* Wrong family */
		if (API->object[i]->geometry != geometry) continue;		/* Wrong geometry */
		if (resource && API->object[i]->resource == resource) item = API->object[i]->ID;	/* Yes: already registered */
	}
	return (item);		/* The ID of the object (or -1) */
}

GMT_LONG GMTAPI_Unregister_IO (struct GMTAPI_CTRL *API, GMT_LONG object_ID, GMT_LONG direction)
{	/* Remove specified object ID from active list of objects */
	GMT_LONG item;

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));		/* GMT_Create_Session has not been called */
	API->error = GMT_OK;		/* No error yet */
	if (API->n_objects == 0) return (GMT_Report_Error (API, GMT_NO_RESOURCES));	/* There are no known resources yet */

	/* Check if this is a valid ID and matches the direction */
	if ((item = GMTAPI_Validate_ID (API, GMTAPI_NOTSET, object_ID, direction)) == GMTAPI_NOTSET) return (GMT_Report_Error (API, API->error));

	/* OK, now it is safe to remove the object */

	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Unregister_IO: Unregistering object no %" GMT_LL "d\n", API->object[item]->ID);

	if (API->object[item]->method == GMT_IS_FILE && API->object[item]->filename) free (API->object[item]->filename);	/* Free any strdup-allocated filenames */
	GMT_free (API->GMT, API->object[item]);		/* Free the current data object */
	API->n_objects--;				/* Tally of how many data sets are left */
	while (item < API->n_objects) {
		API->object[item] = API->object[item+1];	/* Shuffle pointers down one entry */
		item++;
	}

	/* All active resources are found consecutively from 0 to (API->n_objects-1); those with status == 0 (GMT_IS_UNUSED) are available for use. */
	return (GMT_Report_Error (API, GMT_OK));
}

struct GMT_PALETTE * GMTAPI_Import_CPT (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode)
{	/* Does the actual work of loading in a CPT palette table.
 	 * The mode controls how the back-, fore-, NaN-color entries are handled.
	 * Note: Memory is allocated to hold the GMT_PALETTE structure except for method GMT_IS_REF.
	 */
	
	GMT_LONG item, kind;
	struct GMT_PALETTE *P = NULL;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_CPT: Passed ID = %" GMT_LL "d and mode = %" GMT_LL "d\n", ID, mode);
	
	if (ID == GMTAPI_NOTSET) return_null (API, GMT_NO_INPUT);
	API->error = GMT_OK;		/* No error yet */
	if ((item = GMTAPI_Validate_ID (API, GMT_IS_CPT, ID, GMT_IN)) == GMTAPI_NOTSET) return_null (API, API->error);
	
	S = API->object[item];	/* Use S as shorthand */
	if (S->status != GMT_IS_UNUSED) { /* Already read this resource before; are we allowed to re-read? */
		if (S->method == GMT_IS_STREAM || S->method == GMT_IS_FDESC) return_null (API, GMT_READ_ONCE); /* Not allowed to re-read streams */
		if (!(mode & GMT_IO_RESET)) return_null (API, GMT_READ_ONCE);	/* Not authorized to re-read */
	}

	switch (S->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
			/* GMT_read_cpt will report where it is reading from if level is GMT_MSG_NORMAL */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Reading CPT table from %s %s\n", GMT_method[S->method], S->filename);
			if ((P = GMT_read_cpt (API->GMT, S->filename, S->method, mode)) == NULL) return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_STREAM:
 			/* GMT_read_cpt will report where it is reading from if level is GMT_MSG_NORMAL */
			kind = (S->fp == API->GMT->session.std[GMT_OUT]) ? 0 : 1;	/* 0 if stdin/out, 1 otherwise for user pointer */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Reading CPT table from %s %s stream\n", GMT_method[S->method], GMT_stream[kind]);
			if ((P = GMT_read_cpt (API->GMT, S->fp, S->method, mode)) == NULL) return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_FDESC:
			/* GMT_read_cpt will report where it is reading from if level is GMT_MSG_NORMAL */
			kind = (*((int *)S->fp) == GMT_OUT) ? 0 : 1;	/* 0 if stdin/out, 1 otherwise for user pointer */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Reading CPT table from %s %s stream\n", GMT_method[S->method], GMT_stream[kind]);
			if ((P = GMT_read_cpt (API->GMT, S->fp, S->method, mode)) == NULL) return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_COPY:	/* Duplicate the input CPT palette */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating CPT table from GMT_PALETTE memory location\n");
			if (S->resource == NULL) return_null (API, GMT_PTR_IS_NULL);
			P = GMT_memory (API->GMT, NULL, 1, struct GMT_PALETTE);
			GMT_copy_palette (API->GMT, P, S->resource);
			break;
		case GMT_IS_REF:	/* Just pass memory location, so nothing is allocated */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing CPT table from GMT_PALETTE memory location\n");
			if ((P = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			P->alloc_mode = GMT_REFERENCE;	/* Tell GMT_* modules not to free this memory since we did not allocate */
			break;
		case GMT_IS_READONLY:	/* Just pass memory location, so nothing is allocated */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing read-only CPT table from GMT_PALETTE memory location\n");
			if ((P = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			P->alloc_mode = GMT_READONLY;	/* Tell GMT_* modules not to free this memory since we did not allocate */
			break;
		default:	/* Barking up the wrong tree here... */
			GMT_report (API->GMT, GMT_MSG_FATAL, "Wrong method used to import CPT tables\n");
			return_null (API, GMT_WRONG_KIND);
			break;		
	}
	S->status = GMT_IS_USED;	/* Mark as read */
	S->data = P;			/* Retain pointer to the allocated data so we use garbage collection later */

	return (P);	/* Pass back the palette */	
}

GMT_LONG GMTAPI_Export_CPT (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode, struct GMT_PALETTE *P)
{	/* Does the actual work of writing out the specified CPT to a destination.
	 * The mode controls how the back, for, NaN color entries are handled.
	 */
	GMT_LONG item, kind, error;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	struct GMT_PALETTE *P_copy = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Export_CPT: Passed ID = %" GMT_LL "d and mode = %" GMT_LL "d\n", ID, mode);
	
	API->error = GMT_OK;		/* No error yet */
	if (ID == GMTAPI_NOTSET) return (GMT_Report_Error (API, GMT_OUTPUT_NOT_SET));
	if ((item = GMTAPI_Validate_ID (API, GMT_IS_CPT, ID, GMT_OUT)) == GMTAPI_NOTSET) return (GMT_Report_Error (API, API->error));

	S = API->object[item];
	if (S->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET)) {	/* Only allow writing of a data set once, unless we override via mode */
		return (GMT_Report_Error (API, GMT_WRITTEN_ONCE));
	}
	
	switch (S->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
			/* GMT_write_cpt will report where it is writing from if level is GMT_MSG_NORMAL */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Write CPT table to %s %s\n", GMT_method[S->method], S->filename);
			if ((error = GMT_write_cpt (API->GMT, S->filename, S->method, mode, P))) return (GMT_Report_Error (API, error));
			break;
	 	case GMT_IS_STREAM:
			/* GMT_write_cpt will report where it is writing from if level is GMT_MSG_NORMAL */
			kind = (S->fp == API->GMT->session.std[GMT_OUT]) ? 0 : 1;	/* 0 if stdin/out, 1 otherwise for user pointer */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Write CPT table to %s %s output stream\n", GMT_method[S->method], GMT_stream[kind]);
			if ((error = GMT_write_cpt (API->GMT, S->fp, S->method, mode, P))) return (GMT_Report_Error (API, error));
			break;
	 	case GMT_IS_FDESC:
			/* GMT_write_cpt will report where it is writing from if level is GMT_MSG_NORMAL */
			kind = (*((int *)S->fp) == GMT_OUT) ? 0 : 1;	/* 0 if stdin/out, 1 otherwise for user pointer */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Write CPT table to %s %s output stream\n", GMT_method[S->method], GMT_stream[kind]);
			if ((error = GMT_write_cpt (API->GMT, S->fp, S->method, mode, P))) return (GMT_Report_Error (API, error));
			break;
		case GMT_IS_COPY:		/* Duplicate the input cpt */
			if (S->resource) return (GMT_Report_Error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating CPT table to GMT_PALETTE memory location\n");
			P_copy = GMT_memory (API->GMT, NULL, 1, struct GMT_PALETTE);
			GMT_copy_palette (API->GMT, P_copy, P);
			S->resource = P_copy;	/* Set resource pointer from object to this palette */
			break;
		case GMT_IS_REF:	/* Just pass memory location */
			if (S->resource) return (GMT_Report_Error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing CPT table to GMT_PALETTE memory location\n");
			P->alloc_mode = GMT_REFERENCE;	/* To avoid accidental freeing by GMT_* modules since nothing was allocated */
			S->resource = P;	/* Set resource pointer from object to this palette */
			break;
		default:
			GMT_report (API->GMT, GMT_MSG_FATAL, "Wrong method used to export CPT tables\n");
			return (GMT_Report_Error (API, GMT_WRONG_KIND));
			break;		
	}
	S->status = GMT_IS_USED;	/* Mark as written */
	S->data = P;			/* Retain pointer to the allocated data so we use garbage collection later */
	
	return (GMT_Report_Error (API, GMT_OK));
}

GMT_LONG col_check (struct GMT_TABLE *T, GMT_LONG *n_cols) {
	GMT_LONG seg;
	/* Checks that all segments in this table has the correct number of columns.
	 * If *n_cols == 0 we set it to the number of columns found in the first segment. */
	
	for (seg = 0; seg < T->n_segments; seg++) {
		if ((*n_cols) == 0 && seg == 0) *n_cols = T->segment[seg]->n_columns;
		if (T->segment[seg]->n_columns != (*n_cols)) return (TRUE);
	}
	return (FALSE);	/* All is well */
}

struct GMT_DATASET * GMTAPI_Import_Dataset (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode)
{	/* Does the actual work of loading in the entire virtual data set (possibly via many sources)
	 * If ID == GMTAPI_NOTSET we get all registered input tables, otherwise we just get the one requested.
	 * Note: Memory is allocated for the Dataset except for method GMT_IS_DATASET_REF.
	 */
	
	GMT_LONG item, col, row, seg, n_cols = 0, ij, first_item = 0, this_item = GMTAPI_NOTSET, last_item, geometry;
	GMT_LONG allocate = FALSE, update = FALSE, n_alloc, all_D, use_GMT_io, poly;
	PFL GMT_2D_to_index;
	struct GMT_DATASET *D = NULL, *Din = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMT_VECTOR *V = NULL;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Dataset: Passed ID = %" GMT_LL "d and mode = %" GMT_LL "d\n", ID, mode);
	
	API->error = GMT_OK;		/* No error yet */
	if (ID == GMTAPI_NOTSET) {	/* More than one source: Merge all registered data tables into a single virtual data set */
		last_item  = API->n_objects - 1;	/* Must check all objects */
		allocate = TRUE;
		n_alloc = GMT_TINY_CHUNK;
	}
	else {		/* Requested a single, specific data table */
		if ((first_item = GMTAPI_Validate_ID (API, GMT_IS_DATASET, ID, GMT_IN)) == GMTAPI_NOTSET) return_null (API, API->error);
		last_item  = first_item;
		n_alloc = 1;
	}
	
	/* Allocate data set and an initial list of tables */
	D = GMT_memory (API->GMT, NULL, 1, struct GMT_DATASET);
	D->table = GMT_memory (API->GMT, NULL, n_alloc, struct GMT_TABLE *);
	D->alloc_mode = GMT_ALLOCATED;		/* So GMT_* modules can free this memory (may override below) */
	use_GMT_io = !(mode & GMT_IO_ASCII);	/* FALSE if we insist on ASCII reading */
	
	for (item = first_item; item <= last_item; item++) {	/* Look through all sources for registered inputs (or just one) */
		S = API->object[item];	/* S is the current data object */
		if (!S) {	/* Probably not a good sign */
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Dataset: Skipped empty object (item = %ld)\n", item);
			continue;
		}
		if (S->direction == GMT_OUT) continue;	/* We're doing reading here, so skip output objects */
		if (S->family != GMT_IS_DATASET) continue;	/* We're doing datasets here, so skip other data types */
		if (S->status != GMT_IS_UNUSED) { 	/* Already read this resource before; are we allowed to re-read? */
			if (S->method == GMT_IS_STREAM || S->method == GMT_IS_FDESC) return_null (API, GMT_READ_ONCE);	/* Not allowed to re-read streams */
			if (!(mode & GMT_IO_RESET)) return_null (API, GMT_READ_ONCE);	/* Not authorized to re-read */
		}
		if (this_item == GMTAPI_NOTSET) this_item = item;	/* First item that worked */
		geometry = (API->GMT->common.a.output) ? API->GMT->common.a.geometry : S->geometry;	/* When reading GMT and writing OGR/GMT we must make sure we set this first */
		poly = (geometry == GMT_IS_POLY || geometry == GMT_IS_MULTIPOLYGON );	/* To enable polar cap assessment in i/o */
		switch (S->method) {	/* File, array, stream etc ? */
	 		case GMT_IS_FILE:	/* Import all the segments, then count total number of records */
#ifdef SET_IO_MODE
				if (item == first_item) GMT_setmode (API->GMT, GMT_IN);	/* Windows may need to switch read mode from text to binary */
#endif
				/* GMT_read_table will report where it is reading from if level is GMT_MSG_NORMAL */
				if (API->GMT->current.io.ogr == 1 && D->n_tables > 0)	/* Only single tables if GMT/OGR */
					return_null (API, GMT_OGR_ONE_TABLE_ONLY);
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Reading %s from %s %s\n", GMT_family[S->family], GMT_method[S->method], S->filename);
				if ((D->table[D->n_tables] = GMT_read_table (API->GMT, S->filename, S->method, FALSE, poly, use_GMT_io)) == NULL) continue;		/* Ran into an empty file (e.g., /dev/null or equivalent). Skip to next item, */
				D->table[D->n_tables]->id = D->n_tables;	/* Give sequential internal ID numbers to tables */
				D->n_tables++;	/* Since we just read one */
				update = TRUE;
				break;
				
			case GMT_IS_STREAM:	/* Import all the segments, then count total number of records */
	 		case GMT_IS_FDESC:
				/* GMT_read_table will report where it is reading from if level is GMT_MSG_NORMAL */
#ifdef SET_IO_MODE
				if (item == first_item) GMT_setmode (API->GMT, GMT_IN);	/* Windows may need to switch read mode from text to binary */
#endif
				if (API->GMT->current.io.ogr == 1 && D->n_tables > 0)	/* Only single tables if GMT/OGR */
					return_null (API, GMT_OGR_ONE_TABLE_ONLY);
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Reading %s from %s %lx\n", GMT_family[S->family], GMT_method[S->method], (GMT_LONG)S->fp);
				if ((D->table[D->n_tables] = GMT_read_table (API->GMT, S->fp, S->method, FALSE, poly, use_GMT_io)) == NULL) continue;		/* Ran into an empty file (e.g., /dev/null or equivalent). Skip to next item, */
				D->table[D->n_tables]->id = D->n_tables;	/* Give sequential internal ID numbers to tables */
				D->n_tables++;	/* Since we just read one */
				update = TRUE;
				break;

			case GMT_IS_COPY:	/* Duplicate the input dataset */
				if (n_alloc > 1) return_null (API, GMT_ONLY_ONE_ALLOWED);
				if ((Din = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating data table from GMT_DATASET memory location\n");
				GMT_free (API->GMT, D->table);	/* Free up what we allocated earlier since GMT_alloc_dataset does it all */
				GMT_free (API->GMT, D);
				D = GMT_duplicate_dataset (API->GMT, Din, Din->n_columns, GMT_ALLOC_NORMAL);
				break;
				
			case GMT_IS_REF:	/* Just pass memory location, so free up what we allocated first */
				if (n_alloc > 1) return_null (API, GMT_ONLY_ONE_ALLOWED);
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing data table from GMT_DATASET memory location\n");
				GMT_free (API->GMT, D->table);	/* Free up what we allocated up front since we just wish to pass the pointer */
				GMT_free (API->GMT, D);
				if ((D = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
				D->alloc_mode = GMT_REFERENCE;	/* So GMT_* modules wont free this memory */
				break;
				
	 		case GMT_IS_COPY + GMT_VIA_MATRIX:
				/* Each array source becomes a separate table with a single segment */
				if ((M = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating data table from user array location\n");
				D->table[D->n_tables] = GMT_memory (API->GMT, NULL, 1, struct GMT_TABLE);
				D->table[D->n_tables]->segment = GMT_memory (API->GMT, NULL, 1, struct GMT_LINE_SEGMENT *);
				D->table[D->n_tables]->segment[0] = GMT_memory (API->GMT, NULL, 1, struct GMT_LINE_SEGMENT);
				GMT_alloc_segment (API->GMT, D->table[D->n_tables]->segment[0], M->n_rows, M->n_columns, TRUE);
				GMT_2D_to_index = GMTAPI_get_2D_to_index (M->shape, GMT_IS_NORMAL);
				for (row = 0; row < M->n_rows; row++) {
					for (col = 0; col < M->n_columns; col++) {
						ij = GMT_2D_to_index (row, col, M->dim);
						D->table[D->n_tables]->segment[0]->coord[col][row] = GMTAPI_get_val (API, &(M->data), ij, M->type);
					}
				}
				D->table[D->n_tables]->segment[0]->n_rows = M->n_rows;
				D->table[D->n_tables]->segment[0]->n_columns = D->table[D->n_tables]->n_columns = M->n_columns;
				D->table[D->n_tables]->n_records += M->n_rows;
				D->table[D->n_tables]->n_segments = 1;
				D->n_tables++;	/* Since we just read one */
				update = TRUE;
				break;

	 		case GMT_IS_COPY + GMT_VIA_VECTOR:
				/* Each column array source becomes column arrays in a separate table with a single segment */
				if ((V = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating data table from user column arrays location\n");
				D->table[D->n_tables] = GMT_memory (API->GMT, NULL, 1, struct GMT_TABLE);
				D->table[D->n_tables]->segment = GMT_memory (API->GMT, NULL, 1, struct GMT_LINE_SEGMENT *);
				D->table[D->n_tables]->segment[0] = GMT_memory (API->GMT, NULL, 1, struct GMT_LINE_SEGMENT);
				GMT_alloc_segment (API->GMT, D->table[D->n_tables]->segment[0], V->n_rows, V->n_columns, TRUE);
				for (col = 0, all_D = TRUE; all_D && col < V->n_columns; col++) if (V->type[col] != GMTAPI_DOUBLE) all_D = FALSE;
				if (all_D) {	/* Can use fast memcpy */
					for (col = 0; col < V->n_columns; col++) {
						GMT_memcpy (D->table[D->n_tables]->segment[0]->coord[col], V->data[col].f8, V->n_rows, double);
					}
				}
				else {	/* Must copy items individually */
					for (row = 0; row < V->n_rows; row++) {
						for (col = 0; col < V->n_columns; col++) {
							D->table[D->n_tables]->segment[0]->coord[col][row] = GMTAPI_get_val (API, &(V->data[col]), row, V->type[col]);
						}
					}
				}
				D->table[D->n_tables]->segment[0]->n_rows = V->n_rows;
				D->table[D->n_tables]->segment[0]->n_columns = D->table[D->n_tables]->n_columns = V->n_columns;
				D->table[D->n_tables]->n_records += V->n_rows;
				D->table[D->n_tables]->n_segments = 1;
				D->n_tables++;	/* Since we just read one */
				update = TRUE;
				break;

		 	case GMT_IS_REF + GMT_VIA_VECTOR:
		 	case GMT_IS_READONLY + GMT_VIA_VECTOR:
				if ((V = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
				if (V->type[0] != GMTAPI_DOUBLE) return_null (API, GMT_NOT_A_VALID_TYPE);
				/* Each column array source becomes preallocated column arrays in a separate table with a single segment */
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating data table from user column arrays location\n");
				D->table[D->n_tables] = GMT_memory (API->GMT, NULL, 1, struct GMT_TABLE);
				D->table[D->n_tables]->segment = GMT_memory (API->GMT, NULL, 1, struct GMT_LINE_SEGMENT *);
				D->table[D->n_tables]->segment[0] = GMT_memory (API->GMT, NULL, 1, struct GMT_LINE_SEGMENT);
				GMT_alloc_segment (API->GMT, D->table[D->n_tables]->segment[0], 0, V->n_columns, TRUE);
				for (col = 0; col < V->n_columns; col++) {
					D->table[D->n_tables]->segment[0]->coord[col] = V->data[col].f8;
				}
				D->table[D->n_tables]->segment[0]->n_rows = V->n_rows;
				D->table[D->n_tables]->segment[0]->n_columns = D->table[D->n_tables]->n_columns = V->n_columns;
				D->table[D->n_tables]->n_records += V->n_rows;
				D->table[D->n_tables]->n_segments = 1;
				D->n_tables++;	/* Since we just read one */
				update = TRUE;
				break;

			default:	/* Barking up the wrong tree here... */
				GMT_report (API->GMT, GMT_MSG_FATAL, "Wrong method used to import data tables\n");
				GMT_free (API->GMT, D->table);
				GMT_free (API->GMT, D);
				return_null (API, GMT_WRONG_KIND);
				break;		
		}
		if (update) {
			D->n_segments += D->table[D->n_tables-1]->n_segments;	/* Sum up total number of segments across the data set */
			D->n_records += D->table[D->n_tables-1]->n_records;	/* Sum up total number of records across the data set */
			/* Update segment IDs so they are sequential across many tables (GMT_read_table sets the ids relative to current table). */
			if (D->n_tables > 1) 
				for (seg = 0; seg < D->table[D->n_tables-1]->n_segments; seg++) 
					D->table[D->n_tables-1]->segment[seg]->id += D->table[D->n_tables-2]->n_segments;
			if (allocate && D->n_tables == n_alloc) {	/* Must allocate space for more tables */
				GMT_LONG old_n_alloc = n_alloc;
				n_alloc += GMT_TINY_CHUNK;
				D->table = GMT_memory (API->GMT, D->table, n_alloc, struct GMT_TABLE *);
				GMT_memset (&(D->table[old_n_alloc]), n_alloc - old_n_alloc, struct GMT_TABLE *);	/* Set to NULL */
			}
		}
		S->alloc_mode = D->alloc_mode;	/* Clarify allocation mode for this entity */
		if (col_check (D->table[D->n_tables-1], &n_cols)) {	/* Different tables have different number of columns, which is not good */
			return_null (API, GMT_N_COLS_VARY);
		}
		S->status = GMT_IS_USED;	/* Mark as read */
	}
	if (D->n_tables == 0) {	/* Only found empty files (e.g., /dev/null) and we have nothing to show for our efforts.  Return an single empty table with no segments. */
		D->table = GMT_memory (API->GMT, D->table, 1, struct GMT_TABLE *);
		D->table[0] = GMT_memory (API->GMT, NULL, 1, struct GMT_TABLE);
		D->n_tables = 1;	/* But we must indicate we found one (empty) table */
	}
	else {	/* Found one or more tables */
		if (allocate && D->n_tables < n_alloc) D->table = GMT_memory (API->GMT, D->table, D->n_tables, struct GMT_TABLE *);
		D->n_columns = D->table[0]->n_columns;
		if (!D->min) D->min = GMT_memory (API->GMT, NULL, D->n_columns, double);
		if (!D->max) D->max = GMT_memory (API->GMT, NULL, D->n_columns, double);
	}

	API->object[this_item]->data = D;		/* Retain pointer to the allocated data so we use garbage collection later */
	return (D);		
}

GMT_LONG GMTAPI_Export_Dataset (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode, struct GMT_DATASET *D)
{	/* Does the actual work of writing out the specified data set to one destination.
	 * If ID == GMTAPI_NOTSET we use the first registered output destination, otherwise we just use the one requested.
	 * See the GMTAPI documentation for how mode is used to create multiple files from segments, etc.
	 */
	GMT_LONG tbl, item, seg, row, col, offset, ij, error, default_method;
	PFL GMT_2D_to_index;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	struct GMT_DATASET *D_copy = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMT_VECTOR *V = NULL;
	void *ptr = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Export_Dataset: Passed ID = %" GMT_LL "d and mode = %" GMT_LL "d\n", ID, mode);

	API->error = GMT_OK;		/* No error yet */
	if (ID == GMTAPI_NOTSET) return (GMT_Report_Error (API, GMT_OUTPUT_NOT_SET));
	if ((item = GMTAPI_Validate_ID (API, GMT_IS_DATASET, ID, GMT_OUT)) == GMTAPI_NOTSET) return (GMT_Report_Error (API, API->error));

	S = API->object[item];	/* S is the object whose data we will export */
	if (S->family != GMT_IS_DATASET) return (GMT_Report_Error (API, GMT_WRONG_KIND));			/* Called with wrong data type */
	if (mode > GMT_WRITE_DATASET && !S->filename) return (GMT_Report_Error (API, GMT_OUTPUT_NOT_SET));	/* Must have filename when segments are to be written */
	if (S->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET))	/* Only allow writing of a data set once unless overridden by mode */
		return (GMT_Report_Error (API, GMT_WRITTEN_ONCE));
	default_method = GMT_IS_FILE;
	if (S->filename)	/* Write to this file */
		ptr = S->filename;
	else {			/* No filename so we switch to writing to the stream or fdesc */
		default_method = (S->method == GMT_IS_FILE) ? GMT_IS_STREAM : S->method;
		ptr = S->fp;
#ifdef SET_IO_MODE
		GMT_setmode (API->GMT, GMT_OUT);	/* Windows may need to switch write mode from text to binary */
#endif
	}
	D->io_mode = mode;	/* Handles if tables or segments should be written to separate files */
	switch (S->method) {	/* File, array, stream etc ? */
	 	case GMT_IS_STREAM:
#ifdef SET_IO_MODE
			GMT_setmode (API->GMT, GMT_OUT);	/* Windows may need to switch write mode from text to binary */
#endif
		case GMT_IS_FILE:
	 	case GMT_IS_FDESC:
			/* GMT_write_dataset (or lower) will report where it is reading from if level is GMT_MSG_NORMAL */
			if ((error = GMT_write_dataset (API->GMT, ptr, default_method, D, TRUE, GMTAPI_NOTSET))) return (GMT_Report_Error (API, error));
			break;
			
		case GMT_IS_COPY:		/* Duplicate the input dataset */
			if (S->resource) return (GMT_Report_Error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating data table to GMT_DATASET memory location\n");
			D_copy = GMT_duplicate_dataset (API->GMT, D, D->n_columns, GMT_ALLOC_NORMAL);
			S->resource = D_copy;	/* Set resource pointer from object to this dataset */
			break;
			
		case GMT_IS_REF:	/* Just pass memory location */
			if (S->resource) return (GMT_Report_Error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing data table to GMT_DATASET memory location\n");
			D->alloc_mode = GMT_REFERENCE;	/* To avoid accidental freeing upstream */
			S->resource = D;		/* Set resource pointer from object to this dataset */
			break;
			
	 	case GMT_IS_COPY + GMT_VIA_MATRIX:
			if (S->resource == NULL) return (GMT_Report_Error (API, GMT_PTR_IS_NULL));	/* The output resource must initially have info needed to do the output */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating data table to user array location\n");
			M = GMT_duplicate_matrix (API->GMT, S->resource, FALSE);
			/* Must allocate output space */
			M->n_rows = D->n_records;	/* Number of rows needed to hold the data records */
			M->n_columns = D->n_columns;	/* Number of columns needed to hold the data records */
			if (API->GMT->current.io.multi_segments[GMT_OUT]) M->n_rows += D->n_segments;	/* Add one row for each segment header */
			if (M->shape == GMT_ROW_FORMAT) {	/* C-style matrix layout */
				if (M->dim == 0) M->dim = D->n_columns;
				if (M->dim < D->n_columns) return (GMT_Report_Error (API, GMT_DIM_TOO_SMALL));
				S->n_alloc = M->n_rows * M->dim;	/* Get total number of elements as n_rows * dim */
			}
			else {	/* Fortran style */
				if (M->dim == 0) M->dim = D->n_records;
				if (M->dim < D->n_records) return (GMT_Report_Error (API, GMT_DIM_TOO_SMALL));
				S->n_alloc = M->n_columns * M->dim;	/* Get total number of elements as n_columns * dim */
			}
			if ((error = GMT_alloc_univector (API->GMT, &(M->data), M->type, S->n_alloc)) != GMT_OK) return (GMT_Report_Error (API, error));
			GMT_2D_to_index = GMTAPI_get_2D_to_index (M->shape, GMT_IS_NORMAL);
				
			for (tbl = offset = 0; tbl < D->n_tables; tbl++) {
				for (seg = 0; seg < D->table[tbl]->n_segments; seg++) {
					for (row = 0; row < D->table[tbl]->segment[seg]->n_rows; row++) {
						for (col = 0; col < D->table[tbl]->segment[seg]->n_columns; col++) {
							ij = GMT_2D_to_index (row + offset, col, M->dim);
							GMTAPI_put_val (API, &(M->data), D->table[tbl]->segment[seg]->coord[col][row], ij, M->type);
						}
					}
					offset += D->table[tbl]->segment[seg]->n_rows;	/* Since row starts at 0 for each segment */
				}
			}
			S->resource = M;	/* Set resource pointer from object to this matrix */
			break;
			
		case GMT_IS_COPY + GMT_VIA_VECTOR:
		case GMT_IS_REF + GMT_VIA_VECTOR:
			if ((V = S->resource) == NULL) return (GMT_Report_Error (API, GMT_PTR_IS_NULL));	/* The output resource must initially have info needed to do the output */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating data table to user column arrays location\n");
			V = GMT_duplicate_vector (API->GMT, S->resource, FALSE);
			V->n_rows = D->n_records;
			if (API->GMT->current.io.multi_segments[GMT_OUT]) V->n_rows += D->n_segments;
			if ((error = GMT_alloc_vectors (API->GMT, V, V->n_rows)) != GMT_OK) return (GMT_Report_Error (API, error));
			for (tbl = ij = 0; tbl < D->n_tables; tbl++) {
				for (seg = 0; seg < D->table[tbl]->n_segments; seg++) {
					for (row = 0; row < D->table[tbl]->segment[seg]->n_rows; row++, ij++) {
						for (col = 0; col < D->table[tbl]->segment[seg]->n_columns; col++) {
							GMTAPI_put_val (API, &(V->data[col]), D->table[tbl]->segment[seg]->coord[col][row], ij, V->type[col]);
						}
					}
				}
			}
			S->resource = V;
			break;

		default:
			GMT_report (API->GMT, GMT_MSG_FATAL, "Wrong method used to export data tables\n");
			return (GMT_Report_Error (API, GMT_WRONG_KIND));
			break;		
	}
	S->alloc_mode = D->alloc_mode;	/* Clarify allocation mode for this entity */
	S->status = GMT_IS_USED;	/* Mark as written */
	S->data = D;		/* Retain pointer to the allocated data so we use garbage collection later */
	
	return (GMT_Report_Error (API, GMT_OK));
}

struct GMT_TEXTSET * GMTAPI_Import_Textset (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode)
{	/* Does the actual work of loading in the entire virtual text set (possibly via many sources)
	 * If ID == GMTAPI_NOTSET we get all registered input tables, otherwise we just get the one requested.
	 * Note: Memory is allocated for the Dataset except for GMT_IS_DATASET_REF.
	 */
	
	GMT_LONG item, row, seg, first_item = 0, last_item, n_alloc, update = FALSE, allocate = FALSE;
	char *t = NULL;
	struct GMT_TEXTSET *T = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Textset: Passed ID = %" GMT_LL "d and mode = %" GMT_LL "d\n", ID, mode);

	API->error = GMT_OK;		/* No error yet */
	T = GMT_memory (API->GMT, NULL, 1, struct GMT_TEXTSET);
	
	if (ID == GMTAPI_NOTSET) {	/* More than one source: Merge all registered data tables into a single virtual text set */
		last_item = API->n_objects - 1;	/* Must check all objects */
		allocate  = TRUE;
		n_alloc = GMT_TINY_CHUNK;
	}
	else {		/* Requested a single, specific data table */
		if ((first_item = GMTAPI_Validate_ID (API, GMT_IS_TEXTSET, ID, GMT_IN)) == GMTAPI_NOTSET) return_null (API, API->error);
		last_item  = first_item;
		n_alloc = 1;
	}
	T->table = GMT_memory (API->GMT, NULL, n_alloc, struct GMT_TEXT_TABLE *);
	T->alloc_mode = GMT_ALLOCATED;	/* So GMT_* modules can free this memory (may override below) */
	
	for (item = first_item; item <= last_item; item++) {	/* Look through all sources for registered inputs (or just one) */
		S = API->object[item];	/* Current object */
		if (!S) {	/* Probably not a good sign */
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Textset: Skipped empty object (item = %ld)\n", item);
			continue;
		}
		if (S->direction == GMT_OUT) continue;	/* We're doing reading here, so bugger off! */
		if (S->family != GMT_IS_TEXTSET) continue;	/* We're doing textsets here, so skip other things */
		if (S->status != GMT_IS_UNUSED) {	/* Already read this resource before; are we allowed to re-read? */
			if (S->method == GMT_IS_STREAM || S->method == GMT_IS_FDESC) return_null (API, GMT_READ_ONCE);	/* Not allowed to re-read streams */
			if (!(mode & GMT_IO_RESET)) return_null (API, GMT_READ_ONCE);	/* Not authorized to re-read */
		}
		switch (S->method) {	/* File, array, stream etc ? */
			case GMT_IS_FILE:	/* Import all the segments, then count total number of records */
#ifdef SET_IO_MODE
				if (item == first_item) GMT_setmode (API->GMT, GMT_IN);
#endif
				/* GMT_read_texttable will report where it is reading from if level is GMT_MSG_NORMAL */
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Reading %s from %s %s\n", GMT_family[S->family], GMT_method[S->method], S->filename);
				if ((T->table[T->n_tables] = GMT_read_texttable (API->GMT, S->filename, S->method)) == NULL) continue;	/* Ran into an empty file (e.g., /dev/null or equivalent). Skip to next item, */
				T->table[T->n_tables]->id = T->n_tables;	/* Give internal ID numbers to tables */
				update = TRUE;
				break;
	 		case GMT_IS_STREAM:
	 		case GMT_IS_FDESC:
#ifdef SET_IO_MODE
				if (item == first_item) GMT_setmode (API->GMT, GMT_IN);
#endif
				/* GMT_read_texttable will report where it is reading from if level is GMT_MSG_NORMAL */
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Reading %s from %s %lx\n", GMT_family[S->family], GMT_method[S->method], (GMT_LONG)S->fp);
				if ((T->table[T->n_tables] = GMT_read_texttable (API->GMT, S->fp, S->method)) == NULL) continue;	/* Ran into an empty file (e.g., /dev/null or equivalent). Skip to next item, */
				T->table[T->n_tables]->id = T->n_tables;	/* Give internal ID numbers to tables */
				update = TRUE;
				break;
			case GMT_IS_COPY:	/* Duplicate the input dataset */
				if (n_alloc > 1) return_null (API, GMT_ONLY_ONE_ALLOWED);
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating text table from GMT_TEXTSET memory location\n");
				GMT_free (API->GMT, T->table);	/* Free up what we allocated since GMT_alloc_dataset does it all */
				GMT_free (API->GMT, T);
				if (S->resource == NULL) return_null (API, GMT_PTR_IS_NULL);
				T = GMT_duplicate_textset (API->GMT, S->resource, GMT_ALLOC_NORMAL);
				break;
			case GMT_IS_REF:	/* Just pass memory location, so free up what we allocated first */
				if (n_alloc > 1) return_null (API, GMT_ONLY_ONE_ALLOWED);
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing data table from GMT_TEXTSET memory location\n");
				GMT_free (API->GMT, T->table);	/* Free up what we allocated since GMT_alloc_textset does it all */
				GMT_free (API->GMT, T);
				if ((T = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
				T->alloc_mode = GMT_REFERENCE;	/* So GMT_* modules wont free this memory */
				break;
	 		case GMT_IS_COPY + GMT_VIA_MATRIX:
				/* Each matrix source becomes a separate table with one segment */
			 	if ((M = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating text table from user matrix location\n");
				T->table[T->n_tables] = GMT_memory (API->GMT, NULL, 1, struct GMT_TEXT_TABLE);
				T->table[T->n_tables]->segment = GMT_memory (API->GMT, NULL, 1, struct GMT_TEXT_SEGMENT *);
				T->table[T->n_tables]->segment[0] = GMT_memory (API->GMT, NULL, 1, struct GMT_TEXT_SEGMENT);
				T->table[T->n_tables]->segment[0]->record = GMT_memory (API->GMT, NULL, M->n_rows, char *);
				t = M->data.sc1;
				for (row = 0; row < M->n_rows; row++) {
					T->table[T->n_tables]->segment[0]->record[row] = strdup (&t[row*M->dim]);
				}
				T->table[T->n_tables]->segment[0]->n_rows = M->n_rows;
				T->table[T->n_tables]->n_records += M->n_rows;
				T->table[T->n_tables]->n_segments = 1;
				update = TRUE;
				break;
			default:	/* Barking up the wrong tree here... */
				GMT_report (API->GMT, GMT_MSG_FATAL, "Wrong method used to import data tables\n");
				GMT_free (API->GMT, T->table);
				GMT_free (API->GMT, T);
				return_null (API, GMT_WRONG_KIND);
				break;		
		}
		if (update) {
			T->n_segments += T->table[T->n_tables]->n_segments;	/* Sum up total number of segments across the data set */
			T->n_records += T->table[T->n_tables]->n_records;	/* Sum up total number of records across the data set */
			/* Update segment IDs so they are sequential across many tables (GMT_read_table sets the ids relative to current table). */
			if (T->n_tables > 0) 
				for (seg = 0; seg < T->table[T->n_tables]->n_segments; seg++) 
					T->table[T->n_tables]->segment[seg]->id += T->table[T->n_tables-1]->n_segments;
			T->n_tables++;
		}
		if (allocate && T->n_tables == n_alloc) {	/* Must allocate space for more tables */
			GMT_LONG old_n_alloc = n_alloc;
			n_alloc += GMT_TINY_CHUNK;
			T->table = GMT_memory (API->GMT, T->table, n_alloc, struct GMT_TEXT_TABLE *);
			GMT_memset (&(T->table[old_n_alloc]), n_alloc - old_n_alloc, struct GMT_TEXT_TABLE *);	/* Set to NULL */
		}
		S->alloc_mode = T->alloc_mode;	/* Clarify allocation mode for this entity */
		S->status = GMT_IS_USED;	/* Mark as read */
	}

	if (T->n_tables == 0) {	/* Only found empty files (e.g., /dev/null) and we have nothing to show for our efforts.  Return an single empty table with no segments. */
		T->table = GMT_memory (API->GMT, T->table, 1, struct GMT_TEXT_TABLE *);
		T->table[0] = GMT_memory (API->GMT, NULL, 1, struct GMT_TEXT_TABLE);
		T->n_tables = 1;	/* But we must indicate we found one (empty) table */
	}
	else {	/* Found one or more tables */
		if (allocate && T->n_tables < n_alloc) T->table = GMT_memory (API->GMT, T->table, T->n_tables, struct GMT_TEXT_TABLE *);
	}
	API->object[first_item]->data = T;		/* Retain pointer to the allocated data so we use garbage collection later */

	return (T);		
}

GMT_LONG GMTAPI_Export_Textset (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode, struct GMT_TEXTSET *T)
{	/* Does the actual work of writing out the specified text set to one destination.
	 * If ID == GMTAPI_NOTSET we use the first registered output destination, otherwise we just use the one requested.
	 * Again, see GMTAPI documentation for the meaning of mode.
	 */
	GMT_LONG tbl, item, seg, row, offset, error, default_method;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	struct GMT_TEXTSET *T_copy = NULL;
	struct GMT_MATRIX *M = NULL;
	char *ptr = NULL;
	void *dest = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Export_Textset: Passed ID = %" GMT_LL "d and mode = %" GMT_LL "d\n", ID, mode);

	API->error = GMT_OK;		/* No error yet */
	if (ID == GMTAPI_NOTSET) return (GMT_Report_Error (API, GMT_OUTPUT_NOT_SET));
	if ((item = GMTAPI_Validate_ID (API, GMT_IS_TEXTSET, ID, GMT_OUT)) == GMTAPI_NOTSET) return (GMT_Report_Error (API, API->error));

	default_method = (mode > GMT_WRITE_DATASET) ? GMT_IS_FILE : GMT_IS_STREAM;
	S = API->object[item];
	if (S->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET))	/* Only allow writing of a data set once, unless overridden by mode */
		return (GMT_Report_Error (API, GMT_WRITTEN_ONCE));
	default_method = GMT_IS_FILE;
	if (S->filename)	/* Write to this file */
		dest = S->filename;
	else {			/* No filename so we switch to writing to the stream */
		default_method = (S->method == GMT_IS_FILE) ? GMT_IS_STREAM : S->method;
		dest = S->fp;
	}
	T->io_mode = mode;
	switch (S->method) {	/* File, array, stream etc ? */
	 	case GMT_IS_STREAM:
#ifdef SET_IO_MODE
			GMT_setmode (API->GMT, GMT_OUT);
#endif
		case GMT_IS_FILE:
	 	case GMT_IS_FDESC:
			/* GMT_write_textset (or lower) will report where it is reading from if level is GMT_MSG_NORMAL */
			if ((error = GMT_write_textset (API->GMT, dest, default_method, T, GMTAPI_NOTSET))) return (GMT_Report_Error (API, error));
			break;
		case GMT_IS_COPY:		/* Duplicate the input textset */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating data table to GMT_TEXTSET memory location\n");
			if (S->resource) return (GMT_Report_Error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			T_copy = GMT_duplicate_textset (API->GMT, T, GMT_ALLOC_NORMAL);
			S->resource = T_copy;	/* Set resource pointer from object to this textset */
			break;
		case GMT_IS_REF:	/* Just pass memory location */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing data table to GMT_TEXTSET memory location\n");
			if (S->resource) return (GMT_Report_Error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			T->alloc_mode = GMT_REFERENCE;	/* To avoid accidental freeing */
			S->resource = T;		/* Set resource pointer from object to this textset */
			break;
	 	case GMT_IS_COPY + GMT_VIA_MATRIX:
			if ((M = S->resource) == NULL) return (GMT_Report_Error (API, GMT_PTR_IS_NULL));	/* The output resource cannot be NULL for Matrix */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating text table to user array location\n");
			M = GMT_duplicate_matrix (API->GMT, S->resource, FALSE);
			/* Must allocate output space */
			M->n_rows = T->n_records;	/* Number of rows needed to hold the data records */
			M->n_columns = 1;		/* Number of columns needed to hold the data records */
			if (API->GMT->current.io.multi_segments[GMT_OUT]) M->n_rows += T->n_segments;	/* Add one row for each segment header */
			S->n_alloc = M->n_rows * M->dim;	/* Get total number of elements as n_rows * dim */
			if ((error = GMT_alloc_univector (API->GMT, &(M->data), M->type, S->n_alloc)) != GMT_OK) return (GMT_Report_Error (API, error));
				
			ptr = M->data.sc1;
			for (tbl = offset = 0; tbl < T->n_tables; tbl++) {
				for (seg = 0; seg < T->table[tbl]->n_segments; seg++) {
					if (API->GMT->current.io.multi_segments[GMT_OUT]) {
						strncpy (&ptr[offset*M->dim], T->table[tbl]->segment[seg]->header, M->dim);
						offset++;
					}
					for (row = 0; row < T->table[tbl]->segment[seg]->n_rows; row++, offset++)
						strncpy (&ptr[offset*M->dim], T->table[tbl]->segment[seg]->record[row], M->dim);
				}
			}
			S->resource = M;	/* Set resource pointer from object to this matrix */
			break;
		default:
			GMT_report (API->GMT, GMT_MSG_FATAL, "Wrong method used to export text tables\n");
			return (GMT_Report_Error (API, GMT_WRONG_KIND));
			break;		
	}
	S->alloc_mode = T->alloc_mode;	/* Clarify allocation mode for this entity */
	S->status = GMT_IS_USED;	/* Mark as read */
	S->data = T;		/* Retain pointer to the allocated data so we use garbage collection later */
	
	return (GMT_Report_Error (API, GMT_OK));
}

#ifdef USE_GDAL
struct GMT_IMAGE * GMTAPI_Import_Image (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode, struct GMT_IMAGE *image)
{	/* Handles the reading of a 2-D grid given in one of several ways.
	 * Get the entire image:
 	 * 	mode = GMT_GRID_ALL reads both header and image;
	 * Get a subset of the image:  Call GMTAPI_Import_Image twice:
	 * 	1. first with mode = GMT_GRID_HEADER which reads header only.  Then, pass
	 *	   the new S-> wesn to match your desired subregion
	 *	2. 2nd with mode = GMT_GRID_DATA, which reads image based on header's settings
	 * If the image->data array is NULL it will be allocated for you.
	 */
	
	GMT_LONG item, row, col, i0, i1, j0, j1, ij, ij_orig, complex_mode, reset, size, done = 1;
	double dx, dy;
	PFL GMT_2D_to_index;
	struct GMT_IMAGE *I = NULL, *I_orig = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Image: Passed ID = %" GMT_LL "d and mode = %" GMT_LL "d\n", ID, mode);

	API->error = GMT_OK;		/* No error yet */
	if ((item = GMTAPI_Validate_ID (API, GMT_IS_IMAGE, ID, GMT_IN)) == GMTAPI_NOTSET) return_null (API, API->error);
	
	S = API->object[item];		/* Current data object */
	reset = mode & GMT_IO_RESET;	/* Get GMT_IO_RESET bit, if set */
	if (S->status != GMT_IS_UNUSED && !reset) return_null (API, GMT_READ_ONCE);	/* Already read this resources before, so fail unless overridden by mode */
	S->alloc_mode = TRUE;
	mode -= reset;			/* Remove GMT_IO_RESET bit, if set */
	complex_mode = mode >> 2;	/* Yields 0 for normal data, 1 if real complex, and 2 if imag complex */
	mode &= 3;			/* Knock off any complex mode codes */
	
	switch (S->method) {
		case GMT_IS_FILE:	/* Name of a image file on disk */

			if (image == NULL) {	/* Only allocate image struct when not already allocated */
				if (mode == GMT_GRID_DATA) return_null (API, GMT_NO_GRDHEADER);		/* For mode = GMT_GRID_DATA grid must already be allocated */	
				I = GMT_create_image (API->GMT);
			}
			else
				I = image;	/* We are passing in a image already allocated */
			I->header->complex_mode = complex_mode;		/* Set the complex mode */
			done = (mode == GMT_GRID_HEADER) ? 0 : 1;	/* Not done until we read grid */
			if (mode != GMT_GRID_DATA) {		/* Must init header and read the header information from file */
				GMT_grd_init (API->GMT, I->header, NULL, FALSE);
				if (GMT_err_pass (API->GMT, GMT_read_image_info (API->GMT, S->filename, I), S->filename)) 
					return_null (API, GMT_BAD_PERMISSION);
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header, get out of here */
			}
			/* Here we will read the grid data themselves. */
			/* To get a subset we use wesn that is not NULL or contain 0/0/0/0.
			 * Otherwise we extract the entire file domain */
			if (!I->data) {	/* Array is not allocated yet, do so now. We only expect header (and possibly w/e/s/n subset) to have been set correctly */
				I->data = GMT_memory (API->GMT, NULL, I->header->size * I->header->n_bands, unsigned char);
			}
			else {	/* Already have allocated space; check that it is enough */
				size = GMTAPI_set_grdarray_size (API->GMT, I->header, S->wesn);	/* Get array dimension only, which includes padding. DANGER DANGER JL*/
				if (size > I->header->size) return_null (API, GMT_IMAGE_READ_ERROR);
			}
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Reading image from file %s\n", S->filename);
			if (GMT_err_pass (API->GMT, GMT_read_image (API->GMT, S->filename, I, S->wesn, 
				API->GMT->current.io.pad, complex_mode), S->filename)) 
				return_null (API, GMT_IMAGE_READ_ERROR);
			if (GMT_err_pass (API->GMT, GMT_image_BC_set (API->GMT, I), S->filename)) return_null (API, GMT_IMAGE_BC_ERROR);	/* Set boundary conditions */
			I->alloc_mode = GMT_ALLOCATED;
			break;
			
	 	case GMT_IS_COPY:	/* GMT grid and header in a GMT_GRID container object. */
			if ((I_orig = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (image == NULL) {	/* Only allocate when not already allocated */
				if (mode == GMT_GRID_DATA) return_null (API, GMT_NO_GRDHEADER);		/* For mode = 2 grid must already be allocated */	
				I = GMT_create_image (API->GMT);
			}
			else
				I = image;	/* We are passing in an image already */
			done = (mode == GMT_GRID_HEADER) ? 0 : 1;	/* Not done until we read grid */
			if (mode != GMT_GRID_DATA) {	/* Must init header and copy the header information from the existing grid */
				GMT_memcpy (I->header, I_orig->header, 1, struct GRD_HEADER);
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header, get out of here */
			}
			/* Here we will read grid data. */
			/* To get a subset we use wesn that is not NULL or contain 0/0/0/0.
			 * Otherwise we use everything passed in */
			if (!I->data) {	/* Array is not allocated, do so now. We only expect header (and possibly subset w/e/s/n) to have been set correctly */
				I->header->size = GMTAPI_set_grdarray_size (API->GMT, I->header, S->wesn);	/* Get array dimension only, which may include padding */
				I->data = GMT_memory (API->GMT, NULL, I->header->size * I->header->n_bands, unsigned char);
			}
			I->alloc_mode = GMT_ALLOCATED;
			if (!S->region && GMT_grd_pad_status (API->GMT, I->header, API->GMT->current.io.pad)) {	/* Want an exact copy with no subset and same padding */
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating image data from GMT_IMAGE memory location\n");
				GMT_memcpy (I->data, I_orig->data, I_orig->header->size * I_orig->header->n_bands, char);
				break;		/* Done with this image */
			}
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Extracting subset image data from GMT_IMAGE memory location\n");
			/* Here we need to do more work: Either extract subset or add/change padding, or both. */
			/* Get start/stop row/cols for subset (or the entire domain) */
			dx = I->header->inc[GMT_X] * I->header->xy_off;	dy = I->header->inc[GMT_Y] * I->header->xy_off;
			j1 = GMT_grd_y_to_row (API->GMT, I->header->wesn[YLO]+dy, I_orig->header);
			j0 = GMT_grd_y_to_row (API->GMT, I->header->wesn[YHI]-dy, I_orig->header);
			i0 = GMT_grd_x_to_col (API->GMT, I->header->wesn[XLO]+dx, I_orig->header);
			i1 = GMT_grd_x_to_col (API->GMT, I->header->wesn[XHI]-dx, I_orig->header);
			GMT_memcpy (I->header->pad, API->GMT->current.io.pad, 4, GMT_LONG);	/* Set desired padding */
			for (row = j0; row <= j1; row++) {
				for (col = i0; col <= i1; col++, ij++) {
					ij_orig = GMT_IJP (I_orig->header, row, col);	/* Position of this (row,col) in original grid organization */
					ij = GMT_IJP (I->header, row, col);		/* Position of this (row,col) in output grid organization */
					I->data[ij] = I_orig->data[ij_orig];
				}
			}
			break;
			
	 	case GMT_IS_REF:	/* GMT grid and header in a GMT_GRID container object by reference */
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing image data from GMT_IMAGE memory location\n");
			if ((I = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			done = (mode == GMT_GRID_HEADER) ? 0 : 1;	/* Not done until we read image */
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Image: Change alloc mode\n");
			I->alloc_mode = GMT_REFERENCE;	/* So we dont accidentally free this memory */
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Image: Check pad\n");
			if (!GMTAPI_need_grdpadding (I->header, API->GMT->current.io.pad)) break;	/* Pad is correct so we are done */
			/* Here we extend G->data to allow for padding, then rearrange rows */
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Image: Add pad\n");
			/*GMT_grd_pad_on (API->GMT, I, API->GMT->current.io.pad);*/
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Image: Return from GMT_IS_REF\n");
			break;
			
	 	case GMT_IS_READONLY:	/* GMT grid and header in a GMT_GRID container object by exact reference (no changes allowed to grid values) */
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			if ((I = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			done = (mode == GMT_GRID_HEADER) ? 0 : 1;	/* Not done until we read grid */
			if (GMTAPI_need_grdpadding (I->header, API->GMT->current.io.pad)) return_null (API, GMT_PADDING_NOT_ALLOWED);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing image data from read-only GMT_IMAGE memory location\n");
			I->alloc_mode = GMT_READONLY;	/* So we dont accidentally free this memory */
			break;
			
	 	case GMT_IS_COPY + GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			I = (image == NULL) ? GMT_create_image (API->GMT) : image;	/* Only allocate when not already allocated */
			I->header->complex_mode = complex_mode;	/* Set the complex mode */
			if (mode != GMT_GRID_DATA) {
				GMT_grd_init (API->GMT, I->header, NULL, FALSE);
				GMTAPI_info_to_grdheader (API->GMT, I->header, M);	/* Populate a GRD header structure */
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header */
			}
			I->alloc_mode = GMT_ALLOCATED;
			/* Must convert to new array */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Importing image data from user memory location\n");
			GMT_set_grddim (API->GMT, I->header);	/* Set all dimensions */
			I->data = GMT_memory (API->GMT, NULL, I->header->size, unsigned char);
			GMT_2D_to_index = GMTAPI_get_2D_to_index (M->shape, complex_mode);
			GMT_grd_loop (API->GMT, I, row, col, ij) {
				ij_orig = GMT_2D_to_index (row, col, M->dim);
				I->data[ij] = (char)GMTAPI_get_val (API, &(M->data), ij_orig, M->type);
			}
			break;
			
	 	case GMT_IS_REF + GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			I = (image == NULL) ? GMT_create_image (API->GMT) : image;	/* Only allocate when not already allocated */
			I->header->complex_mode = complex_mode;	/* Set the complex mode */
			if (mode != GMT_GRID_DATA) {
				GMT_grd_init (API->GMT, I->header, NULL, FALSE);
				GMTAPI_info_to_grdheader (API->GMT, I->header, M);	/* Populate a GRD header structure */
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header */
			}
			if (!(M->shape == GMTAPI_ORDER_ROW && M->type == GMTAPI_FLOAT && M->alloc_mode == 0 && !complex_mode)) 
				return_null (API, GMT_NOT_A_VALID_IO_ACCESS);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing image data from user memory location\n");
			I->data = (unsigned char *)(M->data.sc1);
			S->alloc_mode = FALSE;	/* No memory needed to be allocated (so none should be freed later */
			I->alloc_mode = GMT_REFERENCE;	/* So we dont accidentally free this memory */
			if (!GMTAPI_need_grdpadding (I->header, API->GMT->current.io.pad)) break;	/* Pad is correct so we are done */
			/* Here we extend I->data to allow for padding, then rearrange rows */
			/*GMT_grd_pad_on (API->GMT, I, API->GMT->current.io.pad);*/
			break;
			
	 	case GMT_IS_READONLY + GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			I = (image == NULL) ? GMT_create_image (API->GMT) : image;	/* Only allocate when not already allocated */
			I->header->complex_mode = complex_mode;	/* Set the complex mode */
			if (mode != GMT_GRID_DATA) {
				GMT_grd_init (API->GMT, I->header, NULL, FALSE);
				GMTAPI_info_to_grdheader (API->GMT, I->header, M);	/* Populate a GRD header structure */
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header */
			}
			if (!(M->shape == GMTAPI_ORDER_ROW && M->type == GMTAPI_FLOAT && M->alloc_mode == 0 && !complex_mode)) 
				return_null (API, GMT_NOT_A_VALID_IO_ACCESS);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing image data from user read-only memory location\n");
			I->data = (unsigned char *)(M->data.sc1);
			S->alloc_mode = FALSE;	/* No memory needed to be allocated (so none should be freed later */
			I->alloc_mode = GMT_READONLY;	/* So we dont accidentally free this memory */
			break;
			
		default:
			GMT_report (API->GMT, GMT_MSG_FATAL, "Wrong method used to import image\n");
			return_null (API, GMT_NOT_A_VALID_METHOD);
			break;
	}

	if (done) S->status = GMT_IS_USED;	/* Mark as read (unless we just got the header) */
	S->data = I;		/* Retain pointer to the allocated data so we use garbage collection later */
	
	return ((mode == GMT_GRID_DATA) ? NULL : I);	/* Pass back out what we have so far */
}
#endif

struct GMT_GRID * GMTAPI_Import_Grid (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode, struct GMT_GRID *grid)
{	/* Handles the reading of a 2-D grid given in one of several ways.
	 * Get the entire grid:
 	 * 	mode = GMT_GRID_ALL reads both header and grid;
	 * Get a subset of the grid:  Call GMTAPI_Import_Grid twice:
	 * 	1. first with mode = GMT_GRID_HEADER which reads header only.  Then, pass
	 *	   the new S-> wesn to match your desired subregion
	 *	2. 2nd with mode = GMT_GRID_DATA, which reads grid based on header's settings
	 * If the grid->data array is NULL it will be allocated for you.
	 */
	
	GMT_LONG item, row, col, i0, i1, j0, j1, ij, ij_orig, complex_mode, reset, size, done = 1;
	double dx, dy;
	PFL GMT_2D_to_index;
	struct GMT_GRID *G = NULL, *G_orig = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Grid: Passed ID = %" GMT_LL "d and mode = %" GMT_LL "d\n", ID, mode);

	API->error = GMT_OK;		/* No error yet */
	if ((item = GMTAPI_Validate_ID (API, GMT_IS_GRID, ID, GMT_IN)) == GMTAPI_NOTSET) return_null (API, API->error);
	
	S = API->object[item];		/* Current data object */
	reset = mode & GMT_IO_RESET;	/* Get GMT_IO_RESET bit, if set */
	if (S->status != GMT_IS_UNUSED && !reset) return_null (API, GMT_READ_ONCE);	/* Already read this resources before, so fail unless overridden by mode */
	S->alloc_mode = TRUE;
	mode -= reset;			/* Remove GMT_IO_RESET bit, if set */
	complex_mode = mode >> 2;	/* Yields 0 for normal data, 1 if real complex, and 2 if imag complex */
	mode &= 3;			/* Knock off any complex mode codes */
	
	switch (S->method) {
		case GMT_IS_FILE:	/* Name of a grid file on disk */
			if (grid == NULL) {	/* Only allocate grid struct when not already allocated */
				if (mode == GMT_GRID_DATA) return_null (API, GMT_NO_GRDHEADER);		/* For mode = GMT_GRID_DATA grid must already be allocated */	
				G = GMT_create_grid (API->GMT);
			}
			else
				G = grid;	/* We are working on a grid already allocated */
			G->header->complex_mode = complex_mode;		/* Set the complex mode */
			done = (mode == GMT_GRID_HEADER) ? 0 : 1;	/* Not done until we read grid */
			if (mode != GMT_GRID_DATA) {		/* Must init header and read the header information from file */
				GMT_grd_init (API->GMT, G->header, NULL, FALSE);
				if (GMT_err_pass (API->GMT, GMT_read_grd_info (API->GMT, S->filename, G->header), S->filename)) 
					return_null (API, GMT_BAD_PERMISSION);
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header, get out of here */
			}
			/* Here we will read the grid data themselves. */
			/* To get a subset we use wesn that is not NULL or contain 0/0/0/0.
			 * Otherwise we extract the entire file domain */
			size = GMTAPI_set_grdarray_size (API->GMT, G->header, S->wesn);	/* Get array dimension only, which includes padding */
			if (!G->data) {	/* Array is not allocated yet, do so now. We only expect header (and possibly w/e/s/n subset) to have been set correctly */
				G->header->size = size;
				G->data = GMT_memory (API->GMT, NULL, G->header->size, float);
			}
			else {	/* Already have allocated space; check that it is enough */
				if (size > G->header->size) return_null (API, GMT_GRID_READ_ERROR);
			}
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Reading grid from file %s\n", S->filename);
			if (GMT_err_pass (API->GMT, GMT_read_grd (API->GMT, S->filename, G->header, G->data, S->wesn, 
							API->GMT->current.io.pad, complex_mode), S->filename)) 
				return_null (API, GMT_GRID_READ_ERROR);
			if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G), S->filename)) return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			G->alloc_mode = GMT_ALLOCATED;
			break;
			
	 	case GMT_IS_COPY:	/* GMT grid and header in a GMT_GRID container object. */
			if ((G_orig = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (grid == NULL) {	/* Only allocate when not already allocated */
				if (mode == GMT_GRID_DATA) return_null (API, GMT_NO_GRDHEADER);		/* For mode = 2 grid must already be allocated */	
				G = GMT_create_grid (API->GMT);
			}
			else
				G = grid;	/* We are passing in a grid already */
			done = (mode == GMT_GRID_HEADER) ? 0 : 1;	/* Not done until we read grid */
			if (mode != GMT_GRID_DATA) {	/* Must init header and copy the header information from the existing grid */
				GMT_memcpy (G->header, G_orig->header, 1, struct GRD_HEADER);
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header, get out of here */
			}
			/* Here we will read grid data. */
			/* To get a subset we use wesn that is not NULL or contain 0/0/0/0.
			 * Otherwise we use everything passed in */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating grid data from GMT_GRID memory location\n");
			if (!G->data) {	/* Array is not allocated, do so now. We only expect header (and possibly subset w/e/s/n) to have been set correctly */
				G->header->size = GMTAPI_set_grdarray_size (API->GMT, G->header, S->wesn);	/* Get array dimension only, which may include padding */
				G->data = GMT_memory (API->GMT, NULL, G->header->size, float);
			}
			G->alloc_mode = GMT_ALLOCATED;
			if (!S->region && GMT_grd_pad_status (API->GMT, G->header, API->GMT->current.io.pad)) {	/* Want an exact copy with no subset and same padding */
				GMT_memcpy (G->data, G_orig->data, G_orig->header->size, float);
				break;		/* Done with this grid */
			}
			/* Here we need to do more work: Either extract subset or add/change padding, or both. */
			/* Get start/stop row/cols for subset (or the entire domain) */
			dx = G->header->inc[GMT_X] * G->header->xy_off;	dy = G->header->inc[GMT_Y] * G->header->xy_off;
			j1 = GMT_grd_y_to_row (API->GMT, G->header->wesn[YLO]+dy, G_orig->header);
			j0 = GMT_grd_y_to_row (API->GMT, G->header->wesn[YHI]-dy, G_orig->header);
			i0 = GMT_grd_x_to_col (API->GMT, G->header->wesn[XLO]+dx, G_orig->header);
			i1 = GMT_grd_x_to_col (API->GMT, G->header->wesn[XHI]-dx, G_orig->header);
			GMT_memcpy (G->header->pad, API->GMT->current.io.pad, 4, GMT_LONG);	/* Set desired padding */
			for (row = j0; row <= j1; row++) {
				for (col = i0; col <= i1; col++, ij++) {
					ij_orig = GMT_IJP (G_orig->header, row, col);	/* Position of this (row,col) in original grid organization */
					ij = GMT_IJP (G->header, row, col);		/* Position of this (row,col) in output grid organization */
					G->data[ij] = G_orig->data[ij_orig];
				}
			}
			GMT_BC_init (API->GMT, G->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G), "Grid memory")) return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			break;
			
	 	case GMT_IS_REF:	/* GMT grid and header in a GMT_GRID container object by reference */
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing grid data from GMT_GRID memory location\n");
			if ((G = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			done = (mode == GMT_GRID_HEADER) ? 0 : 1;	/* Not done until we read grid */
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Grid: Change alloc mode\n");
			G->alloc_mode = GMT_REFERENCE;	/* So we dont accidentally free this memory */
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Grid: Check pad\n");
			GMT_BC_init (API->GMT, G->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G), "Grid memory")) return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			if (!GMTAPI_need_grdpadding (G->header, API->GMT->current.io.pad)) break;	/* Pad is correct so we are done */
			/* Here we extend G->data to allow for padding, then rearrange rows */
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Grid: Add pad\n");
			GMT_grd_pad_on (API->GMT, G, API->GMT->current.io.pad);
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Import_Grid: Return from GMT_IS_REF\n");
			break;
			
	 	case GMT_IS_READONLY:	/* GMT grid and header in a GMT_GRID container object by exact reference (no changes allowed to grid values) */
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			if ((G = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			done = (mode == GMT_GRID_HEADER) ? 0 : 1;	/* Not done until we read grid */
			GMT_BC_init (API->GMT, G->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G), "Grid memory")) return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			if (GMTAPI_need_grdpadding (G->header, API->GMT->current.io.pad)) return_null (API, GMT_PADDING_NOT_ALLOWED);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing grid data from read-only GMT_GRID memory location\n");
			G->alloc_mode = GMT_READONLY;	/* So we dont accidentally free this memory */
			break;
			
	 	case GMT_IS_COPY + GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			G = (grid == NULL) ? GMT_create_grid (API->GMT) : grid;	/* Only allocate when not already allocated */
			G->header->complex_mode = complex_mode;	/* Set the complex mode */
			if (mode != GMT_GRID_DATA) {
				GMT_grd_init (API->GMT, G->header, NULL, FALSE);
				GMTAPI_info_to_grdheader (API->GMT, G->header, M);	/* Populate a GRD header structure */
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header */
			}
			G->alloc_mode = GMT_ALLOCATED;
			/* Must convert to new array */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Importing grid data from user memory location\n");
			GMT_set_grddim (API->GMT, G->header);	/* Set all dimensions */
			G->data = GMT_memory (API->GMT, NULL, G->header->size, float);
			GMT_2D_to_index = GMTAPI_get_2D_to_index (M->shape, complex_mode);
			GMT_grd_loop (API->GMT, G, row, col, ij) {
				ij_orig = GMT_2D_to_index (row, col, M->dim);
				G->data[ij] = (float)GMTAPI_get_val (API, &(M->data), ij_orig, M->type);
			}
			GMT_BC_init (API->GMT, G->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G), "Grid memory")) return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			break;
			
	 	case GMT_IS_REF + GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			G = (grid == NULL) ? GMT_create_grid (API->GMT) : grid;	/* Only allocate when not already allocated */
			G->header->complex_mode = complex_mode;	/* Set the complex mode */
			if (mode != GMT_GRID_DATA) {
				GMT_grd_init (API->GMT, G->header, NULL, FALSE);
				GMTAPI_info_to_grdheader (API->GMT, G->header, M);	/* Populate a GRD header structure */
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header */
			}
			if (!(M->shape == GMTAPI_ORDER_ROW && M->type == GMTAPI_FLOAT && M->alloc_mode == 0 && !complex_mode)) 
				 return_null (API, GMT_NOT_A_VALID_IO_ACCESS);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing grid data from user memory location\n");
			G->data = M->data.f4;
			S->alloc_mode = FALSE;	/* No memory needed to be allocated (so none should be freed later */
			G->alloc_mode = GMT_REFERENCE;	/* So we dont accidentally free this memory */
			GMT_BC_init (API->GMT, G->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G), "Grid memory")) return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			if (!GMTAPI_need_grdpadding (G->header, API->GMT->current.io.pad)) break;	/* Pad is correct so we are done */
			/* Here we extend G->data to allow for padding, then rearrange rows */
			GMT_grd_pad_on (API->GMT, G, API->GMT->current.io.pad);
			break;
			
	 	case GMT_IS_READONLY + GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M = S->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			G = (grid == NULL) ? GMT_create_grid (API->GMT) : grid;	/* Only allocate when not already allocated */
			G->header->complex_mode = complex_mode;	/* Set the complex mode */
			if (mode != GMT_GRID_DATA) {
				GMT_grd_init (API->GMT, G->header, NULL, FALSE);
				GMTAPI_info_to_grdheader (API->GMT, G->header, M);	/* Populate a GRD header structure */
				if (mode == GMT_GRID_HEADER) break;	/* Just needed the header */
			}
			if (!(M->shape == GMTAPI_ORDER_ROW && M->type == GMTAPI_FLOAT && M->alloc_mode == 0 && !complex_mode)) 
				return_null (API, GMT_NOT_A_VALID_IO_ACCESS);
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing grid data from user read-only memory location\n");
			G->data = M->data.f4;
			S->alloc_mode = FALSE;	/* No memory needed to be allocated (so none should be freed later */
			G->alloc_mode = GMT_READONLY;	/* So we dont accidentally free this memory */
			GMT_BC_init (API->GMT, G->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G), "Grid memory")) return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			break;
			
		default:
			GMT_report (API->GMT, GMT_MSG_FATAL, "Wrong method used to import grid\n");
			return_null (API, GMT_NOT_A_VALID_METHOD);
			break;
	}

	if (done) S->status = GMT_IS_USED;	/* Mark as read (unless we just got the header) */
	S->data = G;		/* Retain pointer to the allocated data so we use garbage collection later */
	
	return (G);	/* Pass back out what we have so far */	
}

GMT_LONG GMTAPI_Export_Grid (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode, struct GMT_GRID *G)
{	/* Writes out a single grid to destination */
	GMT_LONG item, row, col, ij, ijp, ij_orig, error, i0, i1, j0, j1, complex_mode, size, done = 1;
	double dx, dy;
	PFL GMT_2D_to_index;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	struct GMT_GRID *G_copy = NULL;
	struct GMT_MATRIX *M = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Export_Grid: Passed ID = %" GMT_LL "d and mode = %" GMT_LL "d\n", ID, mode);

	API->error = GMT_OK;		/* No error yet */
	if (ID == GMTAPI_NOTSET) return (GMT_Report_Error (API, GMT_OUTPUT_NOT_SET));
	if ((item = GMTAPI_Validate_ID (API, GMT_IS_GRID, ID, GMT_OUT)) == GMTAPI_NOTSET) return (GMT_Report_Error (API, API->error));

	S = API->object[item];	/* The current object whose data we will export */
	if (S->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET)) return (GMT_Report_Error (API, GMT_WRITTEN_ONCE));	/* Only allow writing of a data set once, unless overridden by mode */
	mode &= (GMT_IO_RESET - 1);	/* Remove GMT_IO_RESET bit, if set */
	complex_mode = mode >> 2;	/* Yields 0 for normal data, 1 if real complex, and 2 if imag complex */
	mode &= 3;			/* Knock off any complex mode codes */
	switch (S->method) {
		case GMT_IS_FILE:	/* Name of a grid file on disk */
			if (mode == GMT_GRID_HEADER) {	/* Update header structure only */
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Updating grid header for file %s\n", S->filename);
				if (GMT_update_grd_info (API->GMT, NULL, G->header)) return (GMT_Report_Error (API, GMT_GRID_WRITE_ERROR));
				done = 0;	/* Since we are not done with writing */
			}
			else {
				GMT_report (API->GMT, GMT_MSG_NORMAL, "Writing grid to file %s\n", S->filename);
				if (GMT_err_pass (API->GMT, GMT_write_grd (API->GMT, S->filename, G->header, G->data, S->wesn, API->GMT->current.io.pad, complex_mode), S->filename)) return (GMT_Report_Error (API, GMT_GRID_WRITE_ERROR));
			}
			break;
			
	 	case GMT_IS_COPY:	/* Duplicate GMT grid and header to a GMT_GRID container object. Subset allowed */
			if (S->resource) return (GMT_Report_Error (API, GMT_PTR_NOT_NULL));	/* The ouput resource pointer must be NULL */
			if (mode == GMT_GRID_HEADER) return (GMT_Report_Error (API, GMT_NOT_A_VALID_MODE));
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Duplicating grid data to GMT_GRID memory location\n");
			if (!S->region) {	/* No subset, possibly same padding */
				G_copy = GMT_duplicate_grid (API->GMT, G, TRUE);
				if (GMTAPI_need_grdpadding (G_copy->header, API->GMT->current.io.pad)) GMT_grd_pad_on (API->GMT, G_copy, API->GMT->current.io.pad);
				GMT_BC_init (API->GMT, G_copy->header);	/* Initialize grid interpolation and boundary condition parameters */
				if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G_copy), "Grid memory")) return (GMT_Report_Error (API, GMT_GRID_BC_ERROR));	/* Set boundary conditions */
				S->resource = G_copy;	/* Set resource pointer to the grid */
				break;		/* Done with this grid */
			}
			/* Here we need to extract subset, and possibly change padding. */
			/* Get start/stop row/cols for subset (or the entire domain) */
			G_copy = GMT_create_grid (API->GMT);
			GMT_memcpy (G_copy->header, G->header, 1, struct GRD_HEADER);
			GMT_memcpy (G_copy->header->wesn, S->wesn, 4, double);
			dx = G->header->inc[GMT_X] * G->header->xy_off;	dy = G->header->inc[GMT_Y] * G->header->xy_off;
			j1 = GMT_grd_y_to_row (API->GMT, G->header->wesn[YLO]+dy, G->header);
			j0 = GMT_grd_y_to_row (API->GMT, G->header->wesn[YHI]-dy, G->header);
			i0 = GMT_grd_x_to_col (API->GMT, G->header->wesn[XLO]+dx, G->header);
			i1 = GMT_grd_x_to_col (API->GMT, G->header->wesn[XHI]-dx, G->header);
			GMT_memcpy (G->header->pad, API->GMT->current.io.pad, 4, GMT_LONG);		/* Set desired padding */
			G_copy->header->size = GMTAPI_set_grdarray_size (API->GMT, G->header, S->wesn);	/* Get array dimension only, which may include padding */
			G_copy->data = GMT_memory (API->GMT, NULL, G_copy->header->size, float);
			G_copy->header->z_min = DBL_MAX;	G_copy->header->z_max = -DBL_MAX;	/* Must set zmin/zmax since we are not writing */
			for (row = j0; row <= j1; row++) {
				for (col = i0; col <= i1; col++, ij++) {
					ij_orig = GMT_IJP (G->header, row, col);	/* Position of this (row,col) in original grid organization */
					ij = GMT_IJP (G_copy->header, row, col);	/* Position of this (row,col) in output grid organization */
					G_copy->data[ij] = G->data[ij_orig];
					if (GMT_is_fnan (G_copy->data[ij])) continue;
					/* Update z_min, z_max */
					G_copy->header->z_min = MIN (G_copy->header->z_min, (double)G_copy->data[ij]);
					G_copy->header->z_max = MAX (G_copy->header->z_max, (double)G_copy->data[ij]);
				}
			}
			GMT_BC_init (API->GMT, G_copy->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G_copy), "Grid memory")) return (GMT_Report_Error (API, GMT_GRID_BC_ERROR));	/* Set boundary conditions */
			S->resource = G_copy;	/* Set resource pointer to the grid */
			break;
			
	 	case GMT_IS_REF:	/* GMT grid and header in a GMT_GRID container object - just pass the reference */
			if (S->resource) return (GMT_Report_Error (API, GMT_PTR_NOT_NULL));	/* The output resource pointer must be NULL */
			if (S->region) return (GMT_Report_Error (API, GMT_SUBSET_NOT_ALLOWED));
			if (mode == GMT_GRID_HEADER) return (GMT_Report_Error (API, GMT_NOT_A_VALID_MODE));
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing grid data to GMT_GRID memory location\n");
			if (GMTAPI_need_grdpadding (G->header, API->GMT->current.io.pad)) GMT_grd_pad_on (API->GMT, G, API->GMT->current.io.pad);	/* Adjust pad */
			G->alloc_mode = GMT_REFERENCE;	/* So we dont accidentally free this later */
			GMT_grd_zminmax (API->GMT, G->header, G->data);	/* Must set zmin/zmax since we are not writing */
			GMT_BC_init (API->GMT, G->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (GMT_err_pass (API->GMT, GMT_grd_BC_set (API->GMT, G), "Grid memory")) return (GMT_Report_Error (API, GMT_GRID_BC_ERROR));	/* Set boundary conditions */
			S->resource = G;	/* Set resource pointer to the grid */
			break;
			
	 	case GMT_IS_COPY + GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT FULLY TESTED] */
			if (S->resource == NULL) return (GMT_Report_Error (API, GMT_PTR_IS_NULL));	/* The output resource pointer cannot be NULL for matrix */
			if (mode == GMT_GRID_HEADER) return (GMT_Report_Error (API, GMT_NOT_A_VALID_MODE));
			M = GMT_duplicate_matrix (API->GMT, S->resource, FALSE);
			GMTAPI_grdheader_to_info (G->header, M);	/* Populate an array with GRD header information */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Exporting grid data to user memory location\n");
			size = GMT_get_nm (API->GMT, G->header->nx, G->header->ny);
			if ((error = GMT_alloc_univector (API->GMT, &(M->data), M->type, size)) != GMT_OK) return (error);
			GMT_2D_to_index = GMTAPI_get_2D_to_index (M->shape, complex_mode);
			GMT_grd_loop (API->GMT, G, row, col, ijp) {
				ij = GMT_2D_to_index (row, col, M->dim);
				GMTAPI_put_val (API, &(M->data), (double)G->data[ijp], ij, M->type);
			}
			S->resource = M;	/* Set resource pointer to the matrix */
			break;

	 	case GMT_IS_REF + GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT FULLY TESTED] */
			if (S->resource == NULL) return (GMT_Report_Error (API, GMT_PTR_IS_NULL));	/* The output resource pointer cannot be NULL for matrix */
			if (mode == GMT_GRID_HEADER) return (GMT_Report_Error (API, GMT_NOT_A_VALID_MODE));
			M = GMT_duplicate_matrix (API->GMT, S->resource, FALSE);
			if (!(M->shape == GMTAPI_ORDER_ROW && M->type == GMTAPI_FLOAT && M->alloc_mode == 0 && !complex_mode)) 
				return (GMT_Report_Error (API, GMT_NOT_A_VALID_IO_ACCESS));
			GMTAPI_grdheader_to_info (G->header, M);	/* Populate an array with GRD header information */
			GMT_report (API->GMT, GMT_MSG_NORMAL, "Referencing grid data to user memory location\n");
			M->data.f4 = G->data;
			S->resource = M;	/* Set resource pointer to the matrix */
			break;
			
		default:
			GMT_report (API->GMT, GMT_MSG_FATAL, "Wrong method used to export grids\n");
			return (GMT_Report_Error (API, GMT_WRONG_KIND));
			break;
	}

	if (done) S->status = GMT_IS_USED;	/* Mark as written (unless we only updated header) */
	S->data = G;		/* Retain pointer to the allocated data so we use garbage collection later */

	return (GMT_OK);		
}

void * GMTAPI_Import_Data (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG ID, GMT_LONG mode, void *data)
{
	/* Function that will import the data object referred to by the ID (or all registered inputs if ID == GMTAPI_NOTSET).
	 * This is a wrapper functions for CPT, Dataset, Textset, Grid and Image imports; see the specific functions
	 * for details on the arguments, in particular the mode setting (or see the GMT API documentation).
	 */
	GMT_LONG item;
	void *new = NULL;
	
	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);			/* GMT_Create_Session has not been called */
	API->error = GMT_OK;		/* No error yet */
	if (!API->registered[GMT_IN]) return_null (API, GMT_NO_INPUT);		/* No sources registered yet */

	/* Get information about this resource first */
	if ((item = GMTAPI_Validate_ID (API, family, ID, GMT_IN)) == GMTAPI_NOTSET) return_null (API, API->error);
	
	/* The case where ID is not set but a virtual (memory) file is found is a special case: we must supply the correct ID */
	if (ID == GMTAPI_NOTSET && item && API->object[item]->method != GMT_IS_FILE) ID = API->object[item]->ID;	/* Found virtual file; set actual ID */
	
	switch (family) {	/* CPT, Dataset, or Grid */
		case GMT_IS_CPT:
			new = GMTAPI_Import_CPT (API, ID, mode);		/* Try to import a CPT */
			break;
		case GMT_IS_DATASET:
			new = GMTAPI_Import_Dataset (API, ID, mode);		/* Try to import data tables */
			break;
		case GMT_IS_TEXTSET:
			new = GMTAPI_Import_Textset (API, ID, mode);		/* Try to import text tables */
			break;
		case GMT_IS_GRID:
			new = GMTAPI_Import_Grid (API, ID, mode, data);		/* Try to import a grid */
			break;
#ifdef USE_GDAL
		case GMT_IS_IMAGE:
			new = GMTAPI_Import_Image (API, ID, mode, data);	/* Try to import a image */
			break;
#endif
		default:
			API->error = GMT_NOT_A_VALID_METHOD;
			new = NULL;
			break;
	}
	if (new == NULL) return_null (API, API->error);	/* Return NULL as something went wrong */
	return (new);	/* Successful, return pointer */
}

GMT_LONG GMTAPI_Export_Data (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG ID, GMT_LONG mode, void *data)
{
	/* Function that will export the single data object referred to by the ID as registered by GMT_Register_IO.
	 */
	GMT_LONG error, item;
	
	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));			/* GMT_Create_Session has not been called */
	API->error = GMT_OK;		/* No error yet */
	if (!API->registered[GMT_OUT]) return (GMT_Report_Error (API, GMT_NO_OUTPUT));		/* No destination registered yet */

	/* Get information about this resource first */
	if ((item = GMTAPI_Validate_ID (API, family, ID, GMT_OUT)) == GMTAPI_NOTSET) return (GMT_Report_Error (API, API->error));

	/* The case where ID is not set but a virtual (memory) file is found is a special case: we must supply the correct ID */
	if (ID == GMTAPI_NOTSET && item && API->object[item]->method != GMT_IS_FILE) ID = API->object[item]->ID;	/* Found virtual file; set actual ID */

	switch (family) {	/* CPT, Dataset, Textfile, or Grid */
		case GMT_IS_CPT:	/* Export a CPT */
			error = GMTAPI_Export_CPT (API, ID, mode, data);
			break;
		case GMT_IS_DATASET:	/* Export a Data set */
			error = GMTAPI_Export_Dataset (API, ID, mode, data);
			break;
		case GMT_IS_TEXTSET:	/* Export a Text set */
			error = GMTAPI_Export_Textset (API, ID, mode, data);
			break;
		case GMT_IS_GRID:	/* Export a GMT grid */
			error = GMTAPI_Export_Grid (API, ID, mode, data);
			break;
		default:
			error = GMT_NOT_A_VALID_METHOD;
			break;
	}
	return (GMT_Report_Error (API, error));	/* Return status */
}

GMT_LONG GMTAPI_Init_Import (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG geometry, GMT_LONG mode, struct GMT_OPTION *head)
{	/* Handle registration of data files given with option arguments and/or stdin as input sources.
	 * These are the possible actions taken:
	 * 1. If (mode | GMT_REG_FILES_IF_NONE) is TRUE and NO resources have previously been registered, then we scan the option list for files (option == '<' (input)).
	 *    For each file found we register the item as a resource.
	 * 2. If (mode | GMT_REG_FILES_ALWAYS) is TRUE then we always scan the option list for files (option == '<' (input)).
	 *    For each file found we register the item as a resource.
	 * 3. If (mode & GMT_REG_STD_IF_NONE) is TRUE we will register stdin as an input source only if there are NO input items registered.
	 * 4. If (mode & GMT_REG_STD_ALWAYS) is TRUE we will register stdin as an input source, regardless of other items already registered.
	 */
	
	GMT_LONG n_reg, n_new = 0, object_ID, first_ID;
	struct GMT_OPTION *current = NULL;
	double *wesn = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Init_Import: Passed family = %s and geometry = %s\n", GMT_family[family], GMT_geometry[geometry]);

	API->error = GMT_OK;		/* No error yet */
	n_reg = GMTAPI_n_items (API, family, GMT_IN, &first_ID);	/* Count unread datasets in this family that have already been registered */
	
	if ((mode & GMT_REG_FILES_ALWAYS) || ((mode & GMT_REG_FILES_IF_NONE) && n_reg == 0)) {	/* Wish to register all input file args as sources */
		current = head;
		while (current) {		/* Loop over the list and look for input files */
			if (current->option == GMTAPI_OPT_INFILE) {	/* File given, register it */
				if (geometry == GMT_IS_SURFACE) {	/* Grids may require a subset */
					if (API->GMT->common.R.active) {	/* Global subset may have been specified (it might also match the grids domain) */
						wesn = GMT_memory (API->GMT, NULL, 4, double);
						GMT_memcpy (wesn, API->GMT->common.R.wesn, 4, double);
					}
				}
				if ((object_ID = GMT_Register_IO (API, family, GMT_IS_FILE, geometry, GMT_IN, current->arg, wesn)) == GMTAPI_NOTSET) return_value (API, API->error, GMTAPI_NOTSET);	/* Failure to register */
				n_new++;	/* Count of new items registered */
				if (wesn) GMT_free (API->GMT, wesn);
				if (first_ID == GMTAPI_NOTSET) first_ID = object_ID;	/* Found our first ID */
			}
			current = current->next;	/* Go to next option */
		}
		n_reg += n_new;	/* Total registration count so far */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Init_Import: Added %ld new sources\n", n_new);
	}
	
	/* Note that n_reg can have changed if we added file args above */
	
	if ((mode & GMT_REG_STD_ALWAYS) || ((mode & GMT_REG_STD_IF_NONE) && n_reg == 0)) {	/* Wish to register stdin pointer as a source */
		if ((object_ID = GMT_Register_IO (API, family, GMT_IS_STREAM, geometry, GMT_IN, API->GMT->session.std[GMT_IN], NULL)) == GMTAPI_NOTSET) return_value (API, API->error, GMTAPI_NOTSET);	/* Failure to register stdin */
		n_reg++;		/* Add the single item */
		if (first_ID == GMTAPI_NOTSET) first_ID = object_ID;	/* Found our first ID */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Init_Import: Added stdin to registered sources\n");
	}
	return (first_ID);		
}

GMT_LONG GMTAPI_Init_Export (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG geometry, GMT_LONG mode, struct GMT_OPTION *head)
{	/* Handle registration of output file given with option arguments and/or stdout as output destinations.
	 * Only a single output may be considered.  These are the possible actions taken:
	 * 1. If (mode | GMT_REG_FILES_IF_NONE) is TRUE and NO destinations have previously been registered, 
	 *    then we scan the option list for files (option == '>' (output)).
	 *    Only one file can be registered as a destination; finding more than one results in an error.
	 * 2. If (mode | GMT_REG_FILES_ALWAYS) is TRUE then we always scan the option list for files (option == '>' (output)).
	 *    Only one file can be registered as a destination; finding more than one results in an error.
	 * 3. If (mode & GMT_REG_STD_IF_NONE) is TRUE we will register stdout as the only destination if there is NO output item registered.
	 * 4. If (mode & GMT_REG_STD_ALWAYS) is TRUE we will register stdout as an destination, 
	 *    and give error if other output items have already been registered.
	 */
	
	GMT_LONG n_reg, object_ID;
	struct GMT_OPTION *current = NULL;
	
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Init_Export: Passed family = %s and geometry = %s\n", GMT_family[family], GMT_geometry[geometry]);

	API->error = GMT_OK;		/* No error yet */
	n_reg = GMTAPI_n_items (API, family, GMT_OUT, &object_ID);	/* Are there outputs registered already? */
	if (n_reg == 1) {						/* There is a destination registered already */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Init_Export: Found one registered destination (object ID = %ld); skip further registrations\n", object_ID);
		return (object_ID);
	}
	if (n_reg > 1) return_value (API, GMT_ONLY_ONE_ALLOWED, GMTAPI_NOTSET);	/* Only one output destination allowed at once */
	
	/* Here nothing has been registered (n_reg = 0)  */
	
	if ((mode & GMT_REG_FILES_ALWAYS) || (mode & GMT_REG_FILES_IF_NONE)) {	/* Wish to register a single output file arg as destination */
		current = head;
		while (current) {		/* Loop over the list and look for input files */
			if (current->option == GMTAPI_OPT_OUTFILE) n_reg++;	/* File given, count it */
			current = current->next;				/* Go to next option */
		}
		if (n_reg > 1) return_value (API, GMT_ONLY_ONE_ALLOWED, GMTAPI_NOTSET);	/* Only one output allowed */
	
		if (n_reg == 1) {	/* Register the single output file found above */
			current = head;
			while (current) {		/* Loop over the list and look for output files (we know there is only one) */
				if (current->option == GMTAPI_OPT_OUTFILE) {	/* File given, register it */
					fprintf (stderr, "%c %s\n", current->option, current->arg);
					if ((object_ID = GMT_Register_IO (API, family, GMT_IS_FILE, geometry, GMT_OUT, current->arg, NULL)) == GMTAPI_NOTSET) return_value (API, API->error, GMTAPI_NOTSET);	/* Failure to register */
					GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Init_Import: Added 1 new destination\n");
				}
				current = current->next;	/* Go to next option */
			}
		}
	}
	/* Note that n_reg can have changed if we added file arg */
	
	if ((mode & GMT_REG_STD_ALWAYS) && n_reg == 1) return_value (API, GMT_ONLY_ONE_ALLOWED, GMTAPI_NOTSET);	/* Only one output destination allowed at once */
	
	if (n_reg == 0 && ((mode & GMT_REG_STD_ALWAYS) || (mode & GMT_REG_STD_IF_NONE))) {	/* Wish to register stdout pointer as a destination */
		if ((object_ID = GMT_Register_IO (API, family, GMT_IS_STREAM, geometry, GMT_OUT, API->GMT->session.std[GMT_OUT], NULL)) == GMTAPI_NOTSET) return_value (API, API->error, GMTAPI_NOTSET);	/* Failure to register stdout?*/
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Init_Export: Added stdout to registered destinations\n");
		n_reg = 1;	/* Only have one item */
	}
	if (n_reg == 0) return_value (API, GMT_OUTPUT_NOT_SET, GMTAPI_NOTSET);	/* No output set */
	return (object_ID);		
}

GMT_LONG GMTAPI_Begin_IO (struct GMTAPI_CTRL *API, GMT_LONG direction)
{
	/* Initializes the i/o mechanism for either input or output (given by direction).
	 * GMTAPI_Begin_IO must be called before any bulk data i/o is allowed.
	 * direction:	Either GMT_IN or GMT_OUT.
	 * Returns:	FALSE if successfull, TRUE if error.
	 */
	
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	if (!(direction == GMT_IN || direction == GMT_OUT)) return_error (API, GMT_NOT_A_VALID_DIRECTION);
	if (!API->registered[direction]) GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Begin_IO: Warning: No %s resources registered\n", GMT_direction[direction]);
	
	API->io_mode[direction] = GMT_BY_SET;
	API->io_enabled[direction] = TRUE;	/* OK to access resources */
	API->GMT->current.io.ogr = GMTAPI_NOTSET;
	API->GMT->current.io.segment_header[0] = API->GMT->current.io.current_record[0] = 0;
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Begin_IO: %s resource access is now enabled [container]\n", GMT_direction[direction]);

	return (API->error = GMT_OK);	/* No error encountered */
}

#ifdef USE_GDAL
GMT_LONG GMTAPI_Destroy_Image (struct GMTAPI_CTRL *API, GMT_LONG mode, struct GMT_IMAGE **I)
{
	/* Delete the given image resource.
	 * Mode 0 means we don't free objects whose allocation mode flag == GMT_REFERENCE */

	API->error = GMT_OK;		/* No error yet */
	if (!(*I)) {	/* Probably not a good sign */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Destroy_Image: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*I)->alloc_mode == GMT_REFERENCE && mode == GMT_ALLOCATED) return (GMT_MEMORY_MODE_ERROR);	/* Not allowed to free here */
	
	GMT_free_image (API->GMT, I, TRUE);
	return (GMT_Report_Error (API, GMT_OK));
}
#endif

GMT_LONG GMTAPI_Destroy_Grid (struct GMTAPI_CTRL *API, GMT_LONG mode, struct GMT_GRID **G)
{
	/* Delete the given grid resource.
	 * Mode 0 means we don't free objects whose allocation mode flag == GMT_REFERENCE */

	API->error = GMT_OK;		/* No error yet */
	if (!(*G)) {	/* Probably not a good sign */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Destroy_Grid: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*G)->alloc_mode == GMT_REFERENCE && mode == GMT_ALLOCATED) return (GMT_MEMORY_MODE_ERROR);	/* Not allowed to free here */
	
	GMT_free_grid (API->GMT, G, TRUE);
	return (GMT_Report_Error (API, GMT_OK));
}

GMT_LONG GMTAPI_Destroy_Dataset (struct GMTAPI_CTRL *API, GMT_LONG mode, struct GMT_DATASET **D)
{
	/* Delete the given dataset resource.
	 * Mode 0 means we don't free objects whose allocation mode flag == GMT_REFERENCE */

	API->error = GMT_OK;		/* No error yet */
	if (!(*D)) {	/* Probably not a good sign */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Destroy_Dataset: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*D)->alloc_mode == GMT_REFERENCE && mode == GMT_ALLOCATED) return (GMT_MEMORY_MODE_ERROR);	/* Not allowed to free here */

	GMT_free_dataset (API->GMT, D);
	return (GMT_Report_Error (API, GMT_OK));
}

GMT_LONG GMTAPI_Destroy_Textset (struct GMTAPI_CTRL *API, GMT_LONG mode, struct GMT_TEXTSET **D)
{
	/* Delete the given textset resource.
	 * Mode 0 means we don't free things whose allocation mode flag == GMT_REFERENCE */

	API->error = GMT_OK;		/* No error yet */
	if (!(*D)) {	/* Probably not a good sign */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Destroy_Textset: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*D)->alloc_mode == GMT_REFERENCE && mode == GMT_ALLOCATED) return (GMT_MEMORY_MODE_ERROR);	/* Not allowed to free here */

	GMT_free_textset (API->GMT, D);
	return (GMT_Report_Error (API, GMT_OK));
}

GMT_LONG GMTAPI_Destroy_CPT (struct GMTAPI_CTRL *API, GMT_LONG mode, struct GMT_PALETTE **P)
{
	/* Delete the given CPT resource.
	 * Mode 0 means we don't free objects whose allocation mode flag == GMT_REFERENCE */

	API->error = GMT_OK;		/* No error yet */
	if (!(*P)) {	/* Probably not a good sign */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Destroy_CPT: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*P)->alloc_mode == GMT_REFERENCE && mode == GMT_ALLOCATED) return (GMT_MEMORY_MODE_ERROR);	/* Not allowed to free here */

	GMT_free_palette (API->GMT, P);
	return (GMT_Report_Error (API, GMT_OK));
}

GMT_LONG GMTAPI_Destroy_Matrix (struct GMTAPI_CTRL *API, GMT_LONG mode, struct GMT_MATRIX **M)
{
	/* Delete the given Matrix resource.
	 * Mode 0 means we don't free objects whose allocation mode flag == GMT_REFERENCE */

	API->error = GMT_OK;		/* No error yet */
	if (!(*M)) {	/* Probably not a good sign */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Destroy_Matrix: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*M)->alloc_mode == GMT_REFERENCE && mode == GMT_ALLOCATED) return (GMT_MEMORY_MODE_ERROR);	/* Not allowed to free here */

	GMT_free_matrix (API->GMT, M, TRUE);
	return (GMT_Report_Error (API, GMT_OK));
}

GMT_LONG GMTAPI_Destroy_Vector (struct GMTAPI_CTRL *API, GMT_LONG mode, struct GMT_VECTOR **V)
{
	/* Delete the given Matrix resource.
	 * Mode 0 means we don't free objects whose allocation mode flag == GMT_REFERENCE */

	API->error = GMT_OK;		/* No error yet */
	if (!(*V)) {	/* Probably not a good sign */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMTAPI_Destroy_Vector: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*V)->alloc_mode == GMT_REFERENCE && mode == GMT_ALLOCATED) return (GMT_MEMORY_MODE_ERROR);	/* Not allowed to free here */

	GMT_free_vector (API->GMT, V, TRUE);
	return (GMT_Report_Error (API, GMT_OK));
}

struct GMTAPI_DATA_OBJECT * GMTAPI_Make_DataObject (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG method, GMT_LONG geometry, void *resource, GMT_LONG direction)
{	/* Simply the creation and initialization of this DATA_OBJECT structure */
	struct GMTAPI_DATA_OBJECT *S = NULL;

	S = GMT_memory (API->GMT, NULL, 1, struct GMTAPI_DATA_OBJECT);

	S->family = family;
	S->method = method;
	S->geometry = geometry;
	S->resource = resource;
	S->direction = direction;

	return (S);
}

GMT_LONG GMT_destroy_data_ptr (struct GMTAPI_CTRL *API, GMT_LONG family, void *X)
{
	/* Like GMT_Destroy_Data but takes pointer to data rather than address of pointer.
	 * We pass GMT_CLOBBER to make sure we free the memory.
	 */

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	API->error = GMT_OK;		/* No error yet */
	if (!X) return (GMT_OK);	/* Null pointer */
	
	switch (family) {	/* dataset, cpt, text table or grid */
		case GMT_IS_GRID:	/* GMT grid */
			GMT_free_grid_ptr (API->GMT, X, GMT_CLOBBER);
			break;
		case GMT_IS_DATASET:
			GMT_free_dataset_ptr (API->GMT, X);
			break;
		case GMT_IS_TEXTSET:
			GMT_free_textset_ptr (API->GMT, X);
			break;
		case GMT_IS_CPT:
			GMT_free_cpt_ptr (API->GMT, X);
			break;
#ifdef USE_GDAL
		case GMT_IS_IMAGE:
			GMT_free_image_ptr (API->GMT, X, GMT_CLOBBER);
			break;
#endif
			
		/* Also allow destoying of intermediate vector and matrix containers */
		case GMT_IS_MATRIX:
			GMT_free_matrix_ptr (API->GMT, X, GMT_CLOBBER);
			break;
		case GMT_IS_VECTOR:
			GMT_free_vector_ptr (API->GMT, X, GMT_CLOBBER);
			break;
		default:
			return (GMT_Report_Error (API, GMT_WRONG_KIND));
			break;		
	}
	GMT_free (API->GMT, X);
	return (GMT_OK);	/* Null pointer */
}


void GMT_Garbage_Collection (struct GMTAPI_CTRL *API, GMT_LONG level)
{
	/* GMT_Garbage_Collection frees all registered memory associated with the current module level,
	 * or for the entire session if level == GMTAPI_NOTSET (-1) */
	
	GMT_LONG i, j, error, n_free;
	void *address = NULL;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	
	/* Free memory allocated during data registration (e.g., via GMT_Get|Put_Data).
	 * Because GMTAPI_Unregister_IO will delete an object and shuffle
	 * the API->object array, reducing API->n_objects by one we must
	 * be aware that API->n_objects changes in the loop below, hence the while loop */
	
	API->error = GMT_OK;		/* No error yet */
	i = n_free = 0;
	while (i < API->n_objects) {	/* While there are more objects to consider */
		S = API->object[i];	/* Shorthand for the the current object */
		if (!S) {		/* Skip empty object [Should not happen?] */
			GMT_report (API->GMT, GMT_MSG_FATAL, "GMT_Garbage_Collection found empty object number % " GMT_LL "d [Bug?]\n", i++);
			continue;
		}
		if (!(level == GMTAPI_NOTSET || S->level == level)) {	/* Not the right module level (or not end of session) */
			i++;	continue;
		}
		if (!S->data) {	/* No memory to free (probably freed earlier); handle trashing of object after this loop */
			i++;	continue;
		}
		/* Here we will try to free the memory pointed to by S->resource */
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Garbage_Collection: Destroying object: C=%ld A=%ld ID=%ld W=%s F=%s M=%s S=%s P=%lx D=%lx N=%s\n",
			S->close_file, S->alloc_mode, S->ID, GMT_direction[S->direction], GMT_family[S->family], GMT_method[S->method], GMT_status[S->status], (GMT_LONG)S->resource, (GMT_LONG)S->data, S->filename);
		address = S->data;	/* Keep a record of what the address was (since S->data will be set to NULL when freed) */
		error = GMT_destroy_data_ptr (API, S->family, API->object[i]->data);	/* Do the dirty deed */
		
		if (error < 0) {	/* Failed to destroy this memory; that cannot be a good thing... */
			GMT_report (API->GMT, GMT_MSG_FATAL, "GMT_Garbage_Collection failed to destroy memory for object % " GMT_LL "d [Bug?]\n", i++);
			/* Skip it for now; but this is possibly a fatal error [Bug]? */
		}
		else {	/* Successfully freed.  See if this address occurs more than once (e.g., both for in and output); if so just set repeated data pointer to NULL */
			S->data = NULL;
			for (j = i; j < API->n_objects; j++) if (API->object[j]->data == address) API->object[j]->data = NULL;	/* Yes, set to NULL so we don't try to free twice */
			n_free++;	/* Number of freed n_objects; do not increment i since GMT_Destroy_Data shuffled the array */
		}
		i++;	/* Go to next */
	}
 	if (n_free) GMT_report (API->GMT, GMT_MSG_VERBOSE, "GMTAPI_Garbage_Collection freed %ld memory objects\n", n_free);

	/* Deallocate all remaining objects associated with NULL pointers (e.g., rec-by-rec i/o or those set to NULL above) set during this module (or session) */
	i = 0;
	while (i < API->n_objects) {	/* While there are more objects to consider */
		S = API->object[i];	/* Shorthand for the the current object */
		if (S && (level == GMTAPI_NOTSET || S->level == level))	/* Yes, this object was added in this module (or we dont care), get rid of it; leave i where it is */
			GMTAPI_Unregister_IO (API, S->ID, GMTAPI_NOTSET);	/* This shuffles the object array and reduces n_objects */
		else
			i++;	/* Was allocated higher up, leave alone and go to next */
	}
}

GMT_LONG GMTAPI_Already_Registered (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG direction, void *resource) {
	/* Determine if resource is a filename and that it has already been registered */
	GMT_LONG object_ID, item;
	
	if ((object_ID = GMTAPI_Decode_ID (resource)) == GMTAPI_NOTSET) return (GMTAPI_NOTSET);	/* Not a registered resource */
	if ((item = GMTAPI_Validate_ID (API, family, object_ID, direction)) == GMTAPI_NOTSET) return (GMTAPI_NOTSET);	/* Not the right attributes */
	return (object_ID);	/* resource is a registered and valid item */
}

/*========================================================================================================
 *          HERE ARE THE PUBLIC GMT API UTILITY FUNCTIONS, WITH THEIR FORTRAN BINDINGS
 *========================================================================================================
 */

/*===>  Create a new GMT Session */

struct GMTAPI_CTRL * GMT_Create_Session (char *session, GMT_LONG mode)
{
	/* Initializes the GMT API for a new session. This is typically called once in a program,
	 * but programs that manage many threads might call it several times to create as many
	 * sessions as needed. [Note: There is of yet no thread support built into the GMT API
	 * but you could still manage many sessions at once].
	 * The mode argument controls if we want to initialize PSL (for PostScript plotting) or not:
	 *   mode = GMTAPI_GMT		: Initialize GMT only
	 *   mode = GMTAPI_GMTPSL	: Initialize both GMT and PSL (needed to plot things)
	 * The session argument is a textstring used when reporting errors or messages from activity
	 *   originating within this session.
	 * We return the pointer to the allocated API structure.
	 * If any error occurs we report the error, set the error code via API->error, and return NULL.
	 * We terminate each session with a call to GMT_Destroy_Session.
	 */
	
	struct GMTAPI_CTRL *G = NULL;

	if ((G = calloc (1, sizeof (struct GMTAPI_CTRL))) == NULL) return_null (NULL, GMT_MEMORY_ERROR);	/* Failed to allocate the structure */
	
	/* GMT_begin initializes, among onther things, the settings in the user's (or the system's) gmt.conf file */
	if ((G->GMT = GMT_begin (session, mode)) == NULL) {		/* Initializing GMT and possibly the PSL machinery failed */
		free (G);	/* Free G */
		return_null (G, GMT_MEMORY_ERROR);
	}
	G->GMT->parent = G;	/* So we know who's your daddy */
		
	/* Allocate memory to keep track of registered data resources */
	
	G->n_objects_alloc = GMT_SMALL_CHUNK;	/* Start small; this may grow as more resources are registered */
	G->object = GMT_memory (G->GMT, NULL, G->n_objects_alloc, struct GMTAPI_DATA_OBJECT *);
	
	/* Set the unique Session parameters */
	
	G->session_ID = GMTAPI_session_counter++;		/* Guarantees each session ID will be unique and sequential from 0 up */
	if (session) G->session_tag = strdup (session);		/* Only used in reporting and error messages */
	
	return (G);	/* Pass the structure back out */
}

#ifdef FORTRAN_API
/* Fortran binding [THESE MAY CHANGE ONCE WE ACTUALLY TRY TO USE THESE] */
struct GMTAPI_CTRL * GMT_Create_Session_ (char *tag, GMT_LONG *mode, int len)
{	/* Fortran version: We pass the hidden global GMT_FORTRAN structure */
	return (GMT_Create_Session (tag, *mode));
}
#endif

/*===>  Destroy a registered GMT Session */

GMT_LONG GMT_Destroy_Session (struct GMTAPI_CTRL **C)
{
	/* GMT_Destroy_Session terminates the information for the specified session and frees all memory.
	 * Returns FALSE if all is well and TRUE if there were errors. */
	
	GMT_LONG i;
	struct GMTAPI_CTRL *API = *C;
	
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);	/* GMT_Create_Session has not been called */
	
	GMT_Garbage_Collection (API, GMTAPI_NOTSET);	/* Free any remaining memory from data registration during the session */
	/* Deallocate all remaining objects associated with NULL pointers (e.g., rec-by-rec i/o) */
	for (i = 0; i < API->n_objects; i++) GMTAPI_Unregister_IO (API, API->object[i]->ID, GMTAPI_NOTSET);
	GMT_free (API->GMT, API->object);
	GMT_end (API->GMT);	/* Terminate GMT machinery */
	if (API->session_tag) free (API->session_tag);
 	free (API);		/* Not GMT_free since this item was allocated before GMT was initialized */
	*C = NULL;		/* Return this pointer to its NULL state */
	
	return (GMT_OK);
}

#ifdef FORTRAN_API
GMT_LONG GMT_Destroy_Session_ ()
{	/* Fortran version: We pass the hidden global GMT_FORTRAN structure*/
	return (GMT_Destroy_Session (&GMT_FORTRAN));
}
#endif

/*===>  Error message reporting */

GMT_LONG GMT_Report_Error (struct GMTAPI_CTRL *API, GMT_LONG error)
{	/* Write error message to log or stderr, then return error code back.
 	 * All functions can call this, even if API has not been initialized. */
	FILE *fp = NULL;
	if (error < GMT_OK) {	/* Report only negative numbers as errors: positive are warnings only */
		if (!API || !API->GMT || (fp = API->GMT->session.std[GMT_ERR]) == NULL) fp = stderr;
		if (API && API->session_tag) fprintf (fp, "[Session %s (%" GMT_LL "d)]: ", API->session_tag, API->session_ID);
		fprintf (fp, "Error returned from GMT API: %s (%" GMT_LL "d)\n", GMTAPI_errstr[-error], error);
	}
	else if (error)
		GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Report_Error: Warning returned from GMT API: %" GMT_LL "d\n", error);
	if (API) API->error = error;	/* Update API error value of API exists */
	return (error);
}

#ifdef FORTRAN_API
GMT_LONG GMT_Report_Error_ (GMT_LONG *error)
{	/* Fortran version: We pass the hidden global GMT_FORTRAN structure */
	return (GMT_Report_Error (GMT_FORTRAN, *error));
}
#endif

GMT_LONG GMT_Encode_ID (struct GMTAPI_CTRL *API, char *filename, GMT_LONG object_ID)
{
	/* Creates a filename with the embedded GMTAPI Object ID.  Space must exist */
	
	if (!filename) return_error (API, GMT_MEMORY_ERROR);	/* Oops, not allocated space */
	
	sprintf (filename, "@GMTAPI@-%6.6" GMT_LL "d", object_ID);	/* Place the object ID in the special GMT API format */
	return (API->error = GMT_OK);	/* No error encountered */
}

#ifdef FORTRAN_API
GMT_LONG GMT_Encode_ID_ (char *filename, GMT_LONG *object_ID, int len)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Encode_ID (GMT_FORTRAN, filename, *object_ID));
}
#endif

/* Data registration:  The main reason for data registration is the following:
 * Unlike GMT 4, GMT 5 may be used as modules by another calling program.  In
 * that case, the input data file may not be a file but a memory location (i.e.,
 * a data array).  To allow the program to pass such information we needed a
 * way to abstract things so that the modules have no idea of where things are
 * coming from (and were output is going as well).  
 * The API session maintains a single linked linear list of data objects; these
 * objects contain information about all the data resources (sources and destinations)
 * that it has been told about.  Because GMT programs (hence the GMT modules) must
 * be able to find data from stdin, command line files, and command options (e.g.,
 * -Gmyfile.txt) we must be flexible in how things are done.
 *
 * Source registration is done in one of several ways:
 *  1. Call GMT_Register_IO directly and specify the source.  The specifics about the
 *     source will be stored in a new data object which is added to the linked list.
 *     This is what top-level programs must do to allow a GMT module to read via a
 *     memory location.
 *  2. Give file names via the option list (this is what happens when stand-alone
 *     GMT programs process the command line argv list).  Depending on the GMT module,
 *     the module will call GMT_Init_IO to scan for such option arguments and then add
 *     each file found as a new data object.
 *  3. Again, depending on the GMT module, if no unused resources are found, the module
 *     will, via GMT_Init_IO, add stdin as an input resource.  This can be in addition
 *     to any other registered sources, but most often it is added because no other
 *     sources were found.
 *
 * The lower-level GMT i/o machinery will handle complications such as 0 (stdin), 1, or
 * many data files so that the modules themselves simply read the next record with
 * GMT_Get_Record until EOF (as if there is only one input source).  Modules that need
 * to store all the data in memory for futher processing will call GMT_Get_Data instead,
 * which will return a single entity (grid, dataset, cpt, etc).
 *
 * Destination registration is done in the same way, with the exception that for most
 * modules (those processing data tables, at least), only one output destination (e.g., file)
 * can be specified.  However, data sets such as tables with segments can, via mode
 * options, be specified to be written to separate table files or even segment files.
 * The actual writing is done by lower-level functions so that the GMT modules are simply
 * calling GMT_Put_Data (all in one go).  For record-by-record output the modules will use
 * GMT_Put_Record.  This keeps data i/o in the modules uniform and simple across GMT.
 */

GMT_LONG GMT_Register_IO (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG method, GMT_LONG geometry, GMT_LONG direction, void *resource, double wesn[])
{
	/* Adds a new data object to the list of registered objects and returns a unique object ID.
	 * Arguments are as listed for GMTAPI_Register_Im|Export (); see those for details.
	 * During the registration we make sure files exist and are readable.
	 *
	 * if direction == GMT_IN:
	 * A program uses this routine to pass information about input data to GMT.
	 * family:	Specifies the data type we are trying to import; select one of:
	 *   GMT_IS_CPT:	A GMT_PALETTE structure:
	 *   GMT_IS_DATASET:	A GMT_DATASET structure:
	 *   GMT_IS_TEXTSET:	A GMT_TEXTSET structure:
	 *   GMT_IS_GRID:	A GMT_GRID structure:
	 *   GMT_IS_IMAGE:	A GMT_IMAGE structure:
	 * method:	Specifies by what method we will import this data set:
	 *   GMT_IS_FILE:	A file name is given via input.  The program will read data from this file
	 *   GMT_IS_STREAM:	A file pointer to an open file is passed via input. --"--
	 *   GMT_IS_FDESC:	A file descriptor to an open file is passed via input. --"--
	 *   GMT_IS_COPY:	A pointer to a data set to be copied
	 *   GMT_IS_REF:	A pointer to a data set to be passed as is, but we may reallocate sizes
	 *   GMT_IS_READONLY:	A pointer to a data set to be passed as is
	 * The following approaches can be added to the method for all but CPT:
	 *   GMT_VIA_MATRIX:	A 2-D user matrix is passed via input as a source for copying.
	 *			The GMT_MATRIX structure must have parameters filled out.
	 *   GMT_VIA_VECTOR:	An array of user column vectors is passed via input as a source for copying.
	 *			The GMT_VECTOR structure must have parameters filled out.
	 * geometry:	One of GMT_TEXT, GMT_POINT, GMT_LINE, GMT_POLY, or GMT_SURF (for GMT grids)
	 * input:	Pointer to the source filename, stream, handle, array position, etc.
	 * wesn:	Grid subset defined by 4 doubles; otherwise use NULL
	 * RETURNED:	Unique ID assigned to this input resouce, or GMTAPI_NOTSET (-1) if error.
	 * 
	 * An error status is returned if problems are encountered via API->error [GMT_OK].
	 *
	 * GMT_IS_GRID & GMT_VIA_MATRIX: Since GMT internally uses floats in C arrangement, anything else will be converted to float.
	 * GMT_IS_DATASET & GMT_VIA_MATRIX: Since GMT internally uses doubles in C arrangement, anything else will be converted to double.
	 *
	 * GMTAPI_Register_Import will allocate and populate a GMTAPI_DATA_OBJECT structure which
	 * is appended to the data list maintained by the GMTAPI_CTRL API structure.
	 *
	 * if direction == GMT_OUT:
	 * The main program uses this routine to pass information about output data from GMT.
	 * family:	Specifies the data type we are trying to export; select one of:
	 *   GMT_IS_CPT:	A GMT_PALETTE structure:
	 *   GMT_IS_DATASET:	A GMT_DATASET structure:
	 *   GMT_IS_TEXTSET:	A GMT_TEXTSET structure:
	 *   GMT_IS_IMAGE:	A GMT_IMAGE structure:
	 *   GMT_IS_GRID:	A GMT_GRID structure:
	 * method:	Specifies by what method we will export this data set:
	 *   GMT_IS_FILE:	A file name is given via output.  The program will write data to this file
	 *   GMT_IS_STREAM:	A file pointer to an open file is passed via output. --"--
	 *   GMT_IS_FDESC:	A file descriptor to an open file is passed via output. --"--
	 *   GMT_IS_COPY:	A pointer to a data set to be copied
	 *   GMT_IS_REF:	A pointer to a data set to be passed as is; we may reallocate to size
	 *   GMT_IS_READONLY:	A pointer to a data set to be passed as is
	 * The following approaches can be added to the method for all but CPT:
	 *   GMT_VIA_MATRIX:	A 2-D user matrix is passed via input as a source for copying.
	 *			The GMT_MATRIX structure must have parameters filled out.
	 *   GMT_VIA_VECTOR:	An array of user column vectors is passed via input as a source for copying.
	 *			The GMT_VECTOR structure must have parameters filled out.
	 * geometry:	One of GMT_TEXT, GMT_POINT, GMT_LINE, GMT_POLY, or GMT_SURF (for GMT grids)
	 * output:	Pointer to the destination filename, stream, handle, array position, etc.
	 * wesn:	Grid subset defined by 4 doubles; otherwise use NULL
	 * RETURNED:	Unique ID assigned to this output resouce, or GMTAPI_NOTSET (-1) if error.
	 * 
	 * An error status is returned if problems are encountered via API->error [GMT_OK].
	 *
	 * GMTAPI_Register_Export will allocate and populate a GMTAPI_DATA_OBJECT structure which
	 * is appended to the data list maintained by the GMTAPI_CTRL API structure.
	 */
	GMT_LONG item, via = 0, m, object_ID;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMT_VECTOR *V = NULL;

	if (API == NULL) return_value (API, GMT_NOT_A_SESSION, GMTAPI_NOTSET);
	
	API->error = GMT_OK;		/* No error yet */
	/* Check if this filename is an embedded API Object ID passed via the filename and of the right kind.  */
	if ((object_ID = GMTAPI_Already_Registered (API, family, direction, resource)) != GMTAPI_NOTSET) return (object_ID);	/* OK, return the object ID */

	if ((object_ID = GMTAPI_is_registered (API, family, geometry, direction, resource)) != GMTAPI_NOTSET) {	/* Registered before */
		if ((item = GMTAPI_Validate_ID (API, GMTAPI_NOTSET, object_ID, direction)) == GMTAPI_NOTSET) return_value (API, API->error, GMTAPI_NOTSET);
		if ((family == GMT_IS_GRID || family == GMT_IS_IMAGE) && wesn) {	/* Update the subset region if given (for grids/images only) */
			S = API->object[item];	/* Use S as shorthand */
			GMT_memcpy (S->wesn, wesn, 4, double);
			S->region = 1;
		}
		return (object_ID);	/* Already registered so we are done */
	}
	
	if (method >= GMT_VIA_VECTOR) via = (method / GMT_VIA_VECTOR) - 1;
	m = method - (via + 1) * GMT_VIA_VECTOR;
	
	switch (method) {	/* Consider CPT, data, text, and grids, accessed via a variety of methods */
		case GMT_IS_FILE:	/* Registration via a single file name */
			/* No, so presumably it is a regular file name */
			if (direction == GMT_IN) {	/* For input we can check if the file exists and can be read. */
				char *p, *file = strdup (resource);
				if (family == GMT_IS_GRID && (p = strchr (file, '='))) *p = '\0';	/* Chop off any =<stuff> for grids so access can work */
				else if (family == GMT_IS_IMAGE && (p = strchr (file, '+'))) *p = '\0';	/* Chop off any +<stuff> for images so access can work */
				if (GMT_access (API->GMT, file, F_OK) && !GMT_check_url_name(file)) {	/* For input we can check if the file exists (except if via Web) */
					GMT_report (API->GMT, GMT_MSG_FATAL, "File %s not found\n", file);
					return_value (API, GMT_FILE_NOT_FOUND, GMTAPI_NOTSET);
				}
				if (GMT_access (API->GMT, file, R_OK) && !GMT_check_url_name(file)) {	/* Found it but we cannot read. */
					GMT_report (API->GMT, GMT_MSG_FATAL, "Not permitted to read file %s\n", file);
					return_value (API, GMT_BAD_PERMISSION, GMTAPI_NOTSET);
				}
				free (file);
			}
			else if (resource == NULL) {	/* No file given [should this mean stdin/stdout?] */
				return_value (API, GMT_OUTPUT_NOT_SET, GMTAPI_NOTSET);
			}
			/* Create a new data object and initialize variables */
			if ((S = GMTAPI_Make_DataObject (API, family, method, geometry, NULL, direction)) == NULL) {
				return_value (API, GMT_MEMORY_ERROR, GMTAPI_NOTSET);	/* No more memory */
			}
			if (strlen (resource)) S->filename = strdup (resource);
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Register_IO: Registered %s %s %s as an %s resource\n", GMT_family[family], GMT_method[method], S->filename, GMT_direction[direction]);
			break;

	 	case GMT_IS_STREAM:	/* Methods that indirectly involve a file */
	 	case GMT_IS_FDESC:
			if (resource == NULL) {	/* No file given [should this mean stdin/stdout?] */
				return_value (API, GMT_OUTPUT_NOT_SET, GMTAPI_NOTSET);
			}
			if ((S = GMTAPI_Make_DataObject (API, family, method, geometry, NULL, direction)) == NULL) {
				return_value (API, GMT_MEMORY_ERROR, GMTAPI_NOTSET);	/* No more memory */
			}
			S->fp = resource;	/* Pass the stream of fdesc onward */
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Register_IO: Registered %s %s %lx as an %s resource\n", GMT_family[family], GMT_method[method], (GMT_LONG)resource, GMT_direction[direction]);
			break;

		case GMT_IS_COPY:
		case GMT_IS_REF:
		case GMT_IS_READONLY:
			if (direction == GMT_OUT && resource != NULL) {
				return_value (API, GMT_PTR_NOT_NULL, GMTAPI_NOTSET);	/* Output registration of memory takes no resource */
			}
			else if (direction == GMT_IN && resource == NULL) {
				return_value (API, GMT_PTR_IS_NULL, GMTAPI_NOTSET);	/* Output registration of memory takes no resource */
			}
			if ((S = GMTAPI_Make_DataObject (API, family, method, geometry, resource, direction)) == NULL) {
				return_value (API, GMT_MEMORY_ERROR, GMTAPI_NOTSET);	/* No more memory */
			}
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Register_IO: Registered %s %s %lx as an %s resource\n", GMT_family[family], GMT_method[method], (GMT_LONG)resource, GMT_direction[direction]);
			break;

		 case GMT_IS_COPY + GMT_VIA_MATRIX:	/* Here, a data grid is passed via a GMT_MATRIX structure */
		 case GMT_IS_REF + GMT_VIA_MATRIX:
		 case GMT_IS_READONLY + GMT_VIA_MATRIX:
			if ((M = resource) == NULL) {
				return_value (API, GMT_PTR_IS_NULL, GMTAPI_NOTSET);	/* Matrix container must be given for both input and output */
			}
			if (direction == GMT_IN) {	/* For input we can check if the GMT_MATRIX structure has proper parameters. */
				if (M->n_rows <= 0 || M->n_columns <= 0 || GMT_check_region (API->GMT, M->limit)) {
					GMT_report (API->GMT, GMT_MSG_FATAL, "Error in GMT_Register_IO (%s): Matrix parameters not set.\n", GMT_direction[direction]);
					return_value (API, GMT_NO_PARAMETERS, GMTAPI_NOTSET);
				}
			}
			if ((S = GMTAPI_Make_DataObject (API, family, method, geometry, resource, direction)) == NULL) {
				return_value (API, GMT_MEMORY_ERROR, GMTAPI_NOTSET);	/* No more memory */
			}
			API->GMT->common.b.active[direction] = TRUE;
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Register_IO: Registered %s %s %lx via %s as an %s resource\n", GMT_family[family], GMT_method[m], (GMT_LONG)resource, GMT_via[via], GMT_direction[direction]);
			break;
		 case GMT_IS_COPY + GMT_VIA_VECTOR:	/* Here, some data vectors are passed via a GMT_VECTOR structure */
		 case GMT_IS_REF + GMT_VIA_VECTOR:
		 case GMT_IS_READONLY + GMT_VIA_VECTOR:
			if ((V = resource) == NULL) {
				return_value (API, GMT_PTR_IS_NULL, GMTAPI_NOTSET);	/* Vector container must be given for both input and output */
			}
			if (direction == GMT_IN) {	/* For input we can check if the GMT_VECTOR structure has proper parameters. */
				if (V->n_rows <= 0 || V->n_columns <= 0) {
					GMT_report (API->GMT, GMT_MSG_FATAL, "Error in GMT_Register_IO (%s): Vector parameters not set.\n", GMT_direction[direction]);
					return_value (API, GMT_NO_PARAMETERS, GMTAPI_NOTSET);
				}
			}
			if ((S = GMTAPI_Make_DataObject (API, family, method, geometry, resource, direction)) == NULL) {
				return_value (API, GMT_MEMORY_ERROR, GMTAPI_NOTSET);	/* No more memory */
			}
			API->GMT->common.b.active[direction] = TRUE;
			GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Register_IO: Registered %s %s %lx via %s as an %s resource\n", GMT_family[family], GMT_method[m], (GMT_LONG)resource, GMT_via[via], GMT_direction[direction]);
			break;

		default:
			GMT_report (API->GMT, GMT_MSG_FATAL, "Error in GMT_Register_IO (%s): Unrecognized method %" GMT_LL "d\n", GMT_direction[direction], method);
			return_value (API, GMT_NOT_A_VALID_METHOD, GMTAPI_NOTSET);
			break;
	}

	if (wesn) {	/* Copy the subset region if it was given (for grids) */
		GMT_memcpy (S->wesn, wesn, 4, double);
		S->region = 1;
	}
	
	S->level = API->GMT->hidden.func_level;	/* Object was allocated at this module nesting level */
	
	/* Here S is not NULL and no errors have occurred (yet) */
	
	API->registered[direction] = TRUE;	/* We have at least registered one item */
	object_ID = GMTAPI_Add_Data_Object (API, S);
	return_value (API, API->error, object_ID);
}

#ifdef FORTRAN_API
GMT_LONG GMT_Register_IO_ (GMT_LONG *family, GMT_LONG *method, void *input, GMT_LONG *geometry, double wesn[], GMT_LONG *direction)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Register_IO (GMT_FORTRAN, *family, *method, *geometry, *direction, input, wesn));
}
#endif

GMT_LONG GMT_Init_IO (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG geometry, GMT_LONG direction, GMT_LONG mode, struct GMT_OPTION *head)
{
	/* Registers program option file arguments as sources/destinations for the current module.
	 * All modules planning to use std* and/or command-line file args must call GMT_Init_IO to register these resources.
	 * family:	The kind of data (GMT_IS_DATASET|TEXTSET|CPT|GRID)
	 * geometry:	Either GMT_IS_TEXT|POINT|LINE|POLYGON|SURFACE
	 * direction:	Either GMT_IN or GMT_OUT
	 * mode:	Bitflags composed of 1 = add command line (option) files, 2 = add std* if no other input/output,
	 *		4 = add std* regardless.  mode must be > 0.
	 * head:	Linked list of program option arguments.
	 *
	 * Returns:	FALSE if successfull, TRUE if error.
	 */
	GMT_LONG object_ID;	/* ID of first object [only for debug purposes - not used in this function] */

	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	API->error = GMT_OK;		/* No error yet */
	if (geometry < GMT_IS_TEXT || geometry > GMT_IS_SURFACE) return_error (API, GMT_BAD_GEOMETRY);
	if (!(direction == GMT_IN || direction == GMT_OUT)) return_error (API, GMT_NOT_A_VALID_DIRECTION);
	if (mode < 1 || mode > 5) return_error (API, GMT_NOT_A_VALID_MODE);

	GMT_io_banner (API->GMT, direction);	/* Message for binary i/o */
	if (direction == GMT_IN)
		object_ID = GMTAPI_Init_Import (API, family, geometry, mode, head);
	else
		object_ID = GMTAPI_Init_Export (API, family, geometry, mode, head);
	return ((API->error) ? TRUE : FALSE);	/* Return TRUE if any error occured in the Init_* functions */
}

#ifdef FORTRAN_API
GMT_LONG GMT_Init_IO_ (GMT_LONG *family, GMT_LONG *geometry, GMT_LONG *direction, GMT_LONG *mode, struct GMT_OPTION *head)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Init_IO (GMT_FORTRAN, *family, *geometry, *direction, *mode, head));
	
}
#endif

GMT_LONG GMT_Begin_IO (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG direction)
{
	/* Initializes the rec-by-rec i/o mechanism for either input or output (given by direction).
	 * GMT_Begin_IO must be called before any data i/o is allowed.
	 * family:	The kind of data must be GMT_IS_DATASET or TEXTSET.
	 * direction:	Either GMT_IN or GMT_OUT.
	 * Returns:	FALSE if successfull, TRUE if error.
	 */
	GMT_LONG error;
	
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	if (!(direction == GMT_IN || direction == GMT_OUT)) return_error (API, GMT_NOT_A_VALID_DIRECTION);
	if (!(family == GMT_IS_DATASET || family == GMT_IS_TEXTSET)) return_error (API, GMT_NOT_A_VALID_IO_ACCESS);
	if (!API->registered[direction]) GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Begin_IO: Warning: No %s resources registered\n", GMT_direction[direction]);
	
	/* Must initialize record-by-record machinery for dataset or textset */
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Begin_IO: Initialize record-by-record access for %s\n", GMT_direction[direction]);
	API->current_item[direction] = -1;	/* GMTAPI_Next_Data_Object (below) will wind it to the first item >= 0 */
	if ((error = GMTAPI_Next_Data_Object (API, family, direction))) return_error (API, GMT_NO_RESOURCES);	/* Something went bad */
	API->io_mode[direction] = GMT_BY_REC;
	API->io_enabled[direction] = TRUE;	/* OK to access resources */
	API->GMT->current.io.ogr = GMTAPI_NOTSET;
	API->GMT->current.io.segment_header[0] = API->GMT->current.io.current_record[0] = 0;
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_Begin_IO: %s resource access is now enabled [record-by-record]\n", GMT_direction[direction]);

	return (API->error = GMT_OK);	/* No error encountered */
}

#ifdef FORTRAN_API
GMT_LONG GMT_Begin_IO_ (GMT_LONG *family, GMT_LONG *direction)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Begin_IO (GMT_FORTRAN, *family, *direction));
	
}
#endif

GMT_LONG GMT_End_IO (struct GMTAPI_CTRL *API, GMT_LONG direction, GMT_LONG mode)
{
	/* Terminates the i/o mechanism for either input or output (given by direction).
	 * GMT_End_IO must be called after all data i/o is completed.
	 * direction:	Either GMT_IN or GMT_OUT
	 * mode:	Either GMT_IO_DONE (nothing), GMT_IO_RESET (let all resources be accessible again), or GMT_IO_UNREG (unreg all accessed resources).
	 * NOTE: 	Mode not yet implemented until we see a use.
	 * Returns:	FALSE if successfull, TRUE if error.
	 */
	struct GMTAPI_DATA_OBJECT *S = NULL;
	
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	if (!(direction == GMT_IN || direction == GMT_OUT)) return_error (API, GMT_NOT_A_VALID_DIRECTION);
	if (mode < GMT_IO_DONE || mode > GMT_IO_UNREG) return_error (API, GMT_NOT_A_VALID_IO_MODE);
	
	GMT_free_ogr (API->GMT, &(API->GMT->current.io.OGR), 0);	/* Free segment-related array */
	API->io_enabled[direction] = FALSE;	/* No longer OK to access resources */
	API->current_rec[direction] = 0;	/* Reset for next use */
	if (direction == GMT_OUT) {		/* Finalize output issues */
		S = API->object[API->current_item[GMT_OUT]];	/* Shorthand for the data source we are working on */
		if (S) {	/* Dealt with file i/o */
			S->status = GMT_IS_USED;	/* Done writing to this destination */
			if (S->method == GMT_IS_COPY && API->io_mode[GMT_OUT] == GMT_BY_REC) {	/* GMT_Put_Record: Must realloc last segment and the tables segment array */
				if (S->family == GMT_IS_DATASET) {	/* Dataset type */
					struct GMT_DATASET *D = S->resource;
					if (D && D->table && D->table[0]) {
						struct GMT_TABLE *T = D->table[0];
						GMT_LONG *p = API->GMT->current.io.curr_pos[GMT_OUT];
						if (p[1] >= 0) GMT_alloc_segment (API->GMT, T->segment[p[1]], T->segment[p[1]]->n_rows, T->n_columns, FALSE);	/* Last segment */
						T->segment = GMT_memory (API->GMT, T->segment, T->n_segments, struct GMT_LINE_SEGMENT *);
						D->n_segments = T->n_segments;
						GMT_set_tbl_minmax (API->GMT, T);
					}
				}
				else {	/* Textset */
					struct GMT_TEXTSET *D = S->resource;
					if (D && D->table && D->table[0]) {
						struct GMT_TEXT_TABLE *T = D->table[0];
						GMT_LONG *p = API->GMT->current.io.curr_pos[GMT_OUT];
						if (p[1] >= 0) T->segment[p[1]]->record = GMT_memory (API->GMT, T->segment[p[1]]->record, T->segment[p[1]]->n_rows, char *);	/* Last segment */
						T->segment = GMT_memory (API->GMT, T->segment, T->n_segments, struct GMT_TEXT_SEGMENT *);
						D->n_segments = T->n_segments;
					}
				}
			}
			if (S->close_file) {	/* Close file that we opened earlier */
				GMT_fclose (API->GMT, S->fp);
				S->close_file = FALSE;
			}
		}
	}
			
	GMT_report (API->GMT, GMT_MSG_DEBUG, "GMT_End_IO: %s resource access is now disabled\n", GMT_direction[direction]);

	return (API->error = GMT_OK);	/* No error encountered */
}

#ifdef FORTRAN_API
GMT_LONG GMT_End_IO_ (GMT_LONG *direction, GMT_LONG *mode)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_End_IO (GMT_FORTRAN, *direction, *mode));
	
}
#endif

void * GMT_Retrieve_Data (struct GMTAPI_CTRL *API, GMT_LONG ID)
{
	/* Function to return pointer to the container for a registered data set.
	 * Typically used when we wish a module to "write" its results to a memory
	 * location that we wish to access from the calling program.  The procedure
	 * is to use GMT_Register_IO with GMT_REF|COPY|READONLY and GMT_OUT but use
	 * NULL as the source/destination.  Data are "written" by GMT allocating a
	 * output container and updating the objects->resource pointer to this container.
	 * GMT_Retrieve_Data simply returns that pointer given the registered ID.
	 */
	
	GMT_LONG item;
	
	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);
	
	/* Determine the item in the object list that matches this ID */
	if ((item = GMTAPI_Validate_ID (API, GMTAPI_NOTSET, ID, GMTAPI_NOTSET)) == GMTAPI_NOTSET) {
		return_null (API, API->error);
	}
	
	if (API->object[item]->resource == NULL) {
		return_null (API, GMT_PTR_IS_NULL);
	}
	
	API->error = GMT_OK;			/* No error encountered */
	return (API->object[item]->resource);	/* Return pointer to the container */
}

void * GMT_Get_Data (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode, void *data)
{
	/* Function to import registered data sources directly into program memory as a set (not record-by-record).
	 * data is pointer to an existing grid container when we read a grid in two steps, otherwise use NULL.
	 * ID is the registered resource from which to import.
	 * Return: Pointer to data container, or NULL if there were errors (passed back via API->error).
	 */
	GMT_LONG item, was_enabled;
	void *new = NULL;
	
	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);
	
	/* Determine the item in the object list that matches this ID and direction */
	if ((item = GMTAPI_Validate_ID (API, GMTAPI_NOTSET, ID, GMT_IN)) == GMTAPI_NOTSET) {
		return_null (API, API->error);
	}
	
	was_enabled = API->io_enabled[GMT_IN];
	if (!was_enabled && GMTAPI_Begin_IO (API, GMT_IN) != GMT_OK) {	/* Enables data input if not already set and sets access mode */
		return_null (API, API->error);
	}

	/* OK, try to do the importing */
	if ((new = GMTAPI_Import_Data (API, API->object[item]->family, ID, mode, data)) == NULL) {
		return_null (API, API->error);
	}

	if (!was_enabled && GMT_End_IO (API, GMT_IN, 0) != GMT_OK) {	/* Disables data input if we had to set it in this function */
		return_null (API, API->error);
	}
	API->error = GMT_OK;	/* No error encountered */
	return (new);		/* Return pointer to the data container */
}

#ifdef FORTRAN_API
void * GMT_Get_Data_ (GMT_LONG *ID, GMT_LONG *mode, void *data)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Data (GMT_FORTRAN, *ID, *mode, data));
	
}
#endif

void * GMT_Read_Data (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG method, GMT_LONG geometry, double wesn[], GMT_LONG mode, char *input, void *data)
{
	/* Function to read data files directly into program memory as a set (not record-by-record).
	 * We can combine the <register resource - import resource > sequence in
	 * one combined function.  See GMT_Register_IO for details on arguments.
	 * data is pointer to an existing grid container when we read a grid in two steps, otherwise use NULL.
	 * Case 1: input != NULL: Register input as the source and import data.
	 * Case 2: input == NULL: Register stdin as the source and import data.
	 * Case 3: geometry == 0: Loop over all previously registered AND unread sources and combine as virtual dataset [DATASET|TEXTSET only]
	 * Return: Pointer to data container, or NULL if there were errors (passed back via API->error).
	 */
	GMT_LONG in_ID = GMTAPI_NOTSET;
	void *new = NULL;
	
	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);
	
	if (input) {	/* Case 1: Load from a single, given source. Register it first. */
		if ((in_ID = GMT_Register_IO (API, family, method, geometry, GMT_IN, input, wesn)) == GMTAPI_NOTSET) return_null (API, API->error);
	}
	else if (input == NULL && geometry) {	/* Case 2: Load from stdin.  Register stdin first */
		if ((in_ID = GMT_Register_IO (API, family, GMT_IS_STREAM, geometry, GMT_IN, API->GMT->session.std[GMT_IN], wesn)) == GMTAPI_NOTSET) return_null (API, API->error);	/* Failure to register std??? */
	}
	else {	/* Case 3: input == NULL && geometry == 0, so use all previously registered sources (unless already used). */
		if (!(family == GMT_IS_DATASET || family == GMT_IS_TEXTSET)) return_null (API, GMT_ONLY_ONE_ALLOWED);	/* Virtual source only applies to data and text tables */
	}
	/* OK, try to do the importing */
	if ((new = GMT_Get_Data (API, in_ID, mode, data)) == NULL) return_null (API, API->error);

	API->error = GMT_OK;	/* No error encountered */
	return (new);		/* Return pointer to the data container */
}

#ifdef FORTRAN_API
void * GMT_Read_Data_ (GMT_LONG *family, GMT_LONG *method, GMT_LONG *geometry, double *wesn, GMT_LONG *mode, char *input, void *data)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Data (GMT_FORTRAN, *family, *method, *geometry, wesn, *mode, input, data));
	
}
#endif

GMT_LONG GMT_Write_Data (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG method, GMT_LONG geometry, double wesn[], GMT_LONG mode, char *output, void *data)
{
	/* Function to write data directly from program memory as a set (not record-by-record).
	 * We can combine the <register resource - export resource > sequence in
	 * one combined function.  See GMT_Register_IO for details on arguments.
	 * Here, *data is the pointer to the data object to save (CPT, dataset, textset, Grid)
	 * Case 1: output != NULL: Register this as the destination and export data.
	 * Case 2: output == NULL: Register stdout as the destination and export data.
	 * Case 3: geometry == 0: Use a previously registered single destination.
	 * While only one output destination is allowed, for DATA|TEXTSETS one can
	 * have the tables and even segments be written to individual files (see the mode
	 * description in the documentation for how to enable this feature.)
	 * Return: FALSE if all is well, TRUE if there was an error (and set API->error).
	 */
	GMT_LONG out_ID, n_reg;
	
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);

	if (output) {	/* Case 1: Save to a single specified destination.  Register it first. */
		if ((out_ID = GMT_Register_IO (API, family, method, geometry, GMT_OUT, output, wesn)) == GMTAPI_NOTSET) return_error (API, API->error);
	}
	else if (output == NULL && geometry) {	/* Case 2: Save to stdout.  Register stdout first. */
		if (family == GMT_IS_GRID) return_error (API, GMT_STREAM_NOT_ALLOWED);	/* Cannot write grids to stream */
		if ((out_ID = GMT_Register_IO (API, family, GMT_IS_STREAM, geometry, GMT_OUT, API->GMT->session.std[GMT_OUT], wesn)) == GMTAPI_NOTSET) return_error (API, API->error);	/* Failure to register std??? */
	}
	else {	/* Case 3: output == NULL && geometry == 0, so use the previously registered destination */
		if ((n_reg = GMTAPI_n_items (API, family, GMT_OUT, &out_ID)) != 1) return_error (API, GMT_NO_OUTPUT);	/* There is no registered output */
	}
	if (GMT_Put_Data (API, out_ID, mode, data) != GMT_OK) return_error (API, API->error);

	return (API->error = GMT_OK);	/* No error encountered */
}

#ifdef FORTRAN_API
GMT_LONG GMT_Write_Data_ (GMT_LONG *family, GMT_LONG *method, GMT_LONG *geometry, double *wesn, GMT_LONG *mode, char *output, void *data)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Put_Data (GMT_FORTRAN, *family, *method, *geometry, wesn, *mode, output, data));
	
}
#endif

GMT_LONG GMT_Put_Data (struct GMTAPI_CTRL *API, GMT_LONG ID, GMT_LONG mode, void *data)
{
	/* Function to write data directly from program memory as a set (not record-by-record).
	 * We can combine the <register resource - export resource > sequence in
	 * one combined function.  See GMT_Register_IO for details on arguments.
	 * Here, *data is the pointer to the data object to save (CPT, dataset, textset, Grid)
	 * ID is the registered destination.
	 * While only one output destination is allowed, for DATA|TEXTSETS one can
	 * have the tables and even segments be written to individual files (see the mode
	 * description in the documentation for how to enable this feature.)
	 * Return: FALSE if all is well, TRUE if there was an error (and set API->error).
	 */
	GMT_LONG item, was_enabled;
	
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	if (ID == GMTAPI_NOTSET) return_error (API, GMT_NOT_A_VALID_ID);

	/* Determine the item in the object list that matches this ID and direction */
	if ((item = GMTAPI_Validate_ID (API, GMTAPI_NOTSET, ID, GMT_OUT)) == GMTAPI_NOTSET) return_error (API, API->error);

	was_enabled = API->io_enabled[GMT_OUT];
	if (!was_enabled && GMTAPI_Begin_IO (API, GMT_OUT) != GMT_OK) {	/* Enables data output if not already set and sets access mode */
		return_error (API, API->error);
	}
	if (GMTAPI_Export_Data (API, API->object[item]->family, ID, mode, data) != GMT_OK) return_error (API, API->error);

	if (!was_enabled && GMT_End_IO (API, GMT_OUT, 0) != GMT_OK) {	/* Disables data output if we had to set it in this function */
		return_error (API, API->error);
	}
	return (API->error = GMT_OK);	/* No error encountered */
}

#ifdef FORTRAN_API
GMT_LONG GMT_Put_Data_ (GMT_LONG *ID, GMT_LONG *mode, void *data)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Put_Data (GMT_FORTRAN, *ID, *mode, data));
	
}
#endif

void * GMT_Get_Record (struct GMTAPI_CTRL *API, GMT_LONG mode, GMT_LONG *retval)
{
	/* Retrieves the next data record from the virtual input source and
	 * returns the number of columns found via *retval (unless retval == NULL).
	 * If current record is a segment header then we return 0.
	 * If we reach EOF then we return EOF.
	 * mode is either GMT_READ_DOUBLE (data columns), GMT_READ_TEXT (text string) or
	 *	GMT_READ_MIXED (expect data but tolerate read errors).
	 * Also, if (mode | GMT_FILE_BREAK) is TRUE then we will return empty-handed
	 *	when we get to the end of a file except the final file (which is EOF). 
	 *	The calling module can then take actions appropriate between data files.
	 * The double array OR text string is returned via the pointer *record.
	 * If not a data record we return NULL, and pass status via API->GMT->current.io.status.
	 */

	GMT_LONG get_next_record, status, col, n_nan, ij, i, n_fields = 0, *p = NULL;
	char *t_record = NULL;
	void *record = NULL;
	PFL GMT_2D_to_index;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	struct GMT_TEXTSET *DT = NULL;
	struct GMT_DATASET *DS = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMT_VECTOR *V = NULL;
	
	if (retval) *retval = 0;
	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);
	if (!API->io_enabled[GMT_IN]) return_null (API, GMT_ACCESS_NOT_ENABLED);

	S = API->object[API->current_item[GMT_IN]];	/* Shorthand for the current data source we are working on */
	
	do {	/* We do this until we can secure the next record or we run out of records (and return EOF) */
		get_next_record = FALSE;	/* We expect to read one data record and return */
		API->GMT->current.io.status = 0;	/* Initialize status to OK */
		if (S->status == GMT_IS_USED) {		/* Finished reading from this resource, go to next resource */
			if (API->GMT->current.io.ogr == 1) return_null (API, GMT_OGR_ONE_TABLE_ONLY);	/* Only allow single tables if GMT/OGR */
			if (GMTAPI_Next_Data_Object (API, S->family, GMT_IN) == EOF)	/* That was the last source, return */
				n_fields = EOF;
			else {
				S = API->object[API->current_item[GMT_IN]];	/* Shorthand for the next data source to work on */
				get_next_record = TRUE;				/* Since we haven't read the next record yet */
			}
			continue;
		}
		switch (S->method) {
			case GMT_IS_FILE:	/* File, stream, and fd are all the same for us, regardless of data or text input */
		 	case GMT_IS_STREAM:
		 	case GMT_IS_FDESC:
			 	record = S->import (API->GMT, S->fp, &(S->n_expected_fields), &(S->n_columns));	/* Get that next record */
				n_fields = S->n_columns;	/* Get that next record */
				if (API->GMT->current.io.status & GMT_IO_EOF) {			/* End-of-file in current file (but there may be many files) */
					S->status = GMT_IS_USED;	/* Mark as read */
					if (S->close_file) {	/* Close if it was a file that we opened earlier */
						GMT_fclose (API->GMT, S->fp);
						S->close_file = FALSE;
					}
					if (GMTAPI_Next_Data_Object (API, S->family, GMT_IN) == EOF)	/* That was the last source, return */
						n_fields = EOF;					/* EOF is ONLY returned when we reach the end of the LAST data file */
					else if (mode & GMT_FILE_BREAK) {			/* Return empty handed to indicate a break between files */
						n_fields = GMT_IO_NEXT_FILE;			/* We flag this situation with a special return value */
						API->GMT->current.io.status = GMT_IO_NEXT_FILE;
					}
					else {	/* Get ready to read the next data file */
						S = API->object[API->current_item[GMT_IN]];	/* Shorthand for the next data source to work on */
						get_next_record = TRUE;				/* Since we haven't read the next record yet */
					}
					API->GMT->current.io.tbl_no++;				/* Update number of tables we have processed */
				}
				else
					S->status = GMT_IS_USING;				/* Mark as being read */
				
				if (GMT_REC_IS_DATA (API->GMT) && S->n_expected_fields != GMT_MAX_COLUMNS) API->GMT->common.b.ncol[GMT_IN] = S->n_expected_fields;	/* Set the actual column count */
				break;
				
			case GMT_IS_COPY + GMT_VIA_MATRIX:	/* Here we copy/read from a user memory location */
			case GMT_IS_REF + GMT_VIA_MATRIX:
			case GMT_IS_READONLY + GMT_VIA_MATRIX:
				if (API->current_rec[GMT_IN] >= S->n_rows) {	/* Our only way of knowing we are done is to quit when we reach the number of rows that was registered */
					API->GMT->current.io.status = GMT_IO_EOF;
					S->status = GMT_IS_USED;	/* Mark as read */
					if (retval) *retval = EOF;
					return (NULL);	/* Done with this array */
				}
				else
					S->status = GMT_IS_USING;				/* Mark as being read */
				M = S->resource;
				GMT_2D_to_index = GMTAPI_get_2D_to_index (M->shape, GMT_IS_NORMAL);
				for (col = n_nan = 0; col < S->n_columns; col++) {	/* We know the number of columns from registration */
					ij = GMT_2D_to_index (API->current_rec[GMT_IN], col, M->dim);
					API->GMT->current.io.curr_rec[col] = GMTAPI_get_val (API, &(M->data), ij, M->type);
					if (GMT_is_dnan (API->GMT->current.io.curr_rec[col])) n_nan++;
				}
				if (n_nan == S->n_columns) {
					API->GMT->current.io.status = GMT_IO_SEG_HEADER;	/* Flag as segment header */
					record = NULL;
				}
				else
					record = API->GMT->current.io.curr_rec;
				n_fields = S->n_columns;
				break;

			 case GMT_IS_COPY + GMT_VIA_VECTOR:	/* Here we copy from a user memory location that points to an array of column vectors */
			 case GMT_IS_REF + GMT_VIA_VECTOR:
			 case GMT_IS_READONLY + GMT_VIA_VECTOR:
				if (API->current_rec[GMT_IN] >= S->n_rows) {	/* Our only way of knowing we are done is to quit when we reach the number or rows that was registered */
					API->GMT->current.io.status = GMT_IO_EOF;
					S->status = GMT_IS_USED;	/* Mark as read */
					if (retval) *retval = EOF;
					return (NULL);	/* Done with this array */
				}
				else
					S->status = GMT_IS_USING;				/* Mark as being read */
				V = S->resource;
				for (col = n_nan = 0; col < S->n_columns; col++) {	/* We know the number of columns from registration */
					API->GMT->current.io.curr_rec[col] = GMTAPI_get_val (API, &(V->data[col]), API->current_rec[GMT_IN], V->type[col]);
					if (GMT_is_dnan (API->GMT->current.io.curr_rec[col])) n_nan++;
				}
				if (n_nan == S->n_columns) {
					API->GMT->current.io.status = GMT_IO_SEG_HEADER;	/* Flag as segment header */
					record = NULL;
				}
				else
					record = API->GMT->current.io.curr_rec;
				n_fields = S->n_columns;
				break;

			case GMT_IS_REF:	/* Only for textsets and datasets */
			case GMT_IS_READONLY:
				p = API->GMT->current.io.curr_pos[GMT_IN];	/* Shorthand used below */
				if (S->family == GMT_IS_DATASET) {
					DS = S->resource;
					
					status = 0;
					if (p[2] == DS->table[p[0]]->segment[p[1]]->n_rows) {	/* Reached end of current segment */
						p[1]++, p[2] = 0;				/* Advance to next segments 1st row */
						status = GMT_IO_SEG_HEADER;			/* Indicates a segment boundary */
					}
					if (p[1] == DS->table[p[0]]->n_segments) {		/* Also the end of a table ("file") */
						p[0]++, p[1] = 0;
						if (mode & GMT_FILE_BREAK) {			/* Return empty handed to indicate a break between files */
							status = n_fields = GMT_IO_NEXT_FILE;
							record = NULL;
						}
					}
					if (p[0] == DS->n_tables) {	/* End of entire data set */
						status = GMT_IO_EOF;
						n_fields = EOF;
						record = NULL;
						S->status = GMT_IS_USED;	/* Mark as read */
					}
					if (!status) {	/* OK get the record */
						for (col = n_nan = 0; col < DS->n_columns; col++) {
							API->GMT->current.io.curr_rec[col] = DS->table[p[0]]->segment[p[1]]->coord[col][p[2]];
							if (GMT_is_dnan (API->GMT->current.io.curr_rec[col])) n_nan++;
						}
						p[2]++;
						n_fields = API->GMT->common.b.ncol[GMT_IN] = DS->n_columns;
						if (n_nan == S->n_columns) {
							API->GMT->current.io.status = GMT_IO_SEG_HEADER;	/* Flag as segment header */
							record = NULL;
						}
						else
							record = API->GMT->current.io.curr_rec;
						S->status = GMT_IS_USING;	/* Mark as read */
					}
					API->GMT->current.io.status = status;
				}
				if (S->family == GMT_IS_TEXTSET) {
					DT = S->resource;
					if (p[2] == DT->table[p[0]]->segment[p[1]]->n_rows) {p[1]++, p[2] = 0;}
					if (p[1] == DT->table[p[0]]->n_segments) {p[0]++, p[1] = 0;}
					if (p[0] == DT->n_tables) {
						n_fields = EOF;
						API->GMT->current.io.status = GMT_IO_EOF;
						record = NULL;
						S->status = GMT_IS_USED;	/* Mark as read */
					}
					else {
						t_record = DT->table[p[0]]->segment[p[1]]->record[p[2]++];
						API->GMT->current.io.status = 0;
						if (t_record[0] == API->GMT->current.setting.io_seg_marker[GMT_IN]) {	/* Segment header */
							i = GMT_trim_segheader (API->GMT, t_record);
							strcpy (API->GMT->current.io.segment_header, &t_record[i]);
							API->GMT->current.io.status = GMT_IO_SEG_HEADER;
							record = NULL;
						}
						else {	/* Regular record */
							strcpy (API->GMT->current.io.current_record, t_record);
							record = t_record;
						}
						n_fields = 1;
						S->status = GMT_IS_USING;	/* Mark as read */
					}
				}
				break;
			default:
				GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: Internal error: GMT_Get_Record called with illegal method\n");
				break;
		}
	} while (get_next_record);
	
	if (!(n_fields == EOF || n_fields == GMT_IO_NEXT_FILE)) API->current_rec[GMT_IN]++;	/* Increase record count, unless EOF */

	API->error = GMT_OK;	/* No error encountered */
	if (retval) *retval = n_fields;	/* Requested we return the number of fields found */
	return (record);	/* Return pointer to current record */		
}

#ifdef FORTRAN_API
void * GMT_Get_Record_ (GMT_LONG *mode, GMT_LONG *status)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Record (GMT_FORTRAN, *mode, status));
	
}
#endif

GMT_LONG GMT_Put_Record (struct GMTAPI_CTRL *API, GMT_LONG mode, void *record)
{	/* Writes a single data record to destimation.
	 * We use mode to signal the kind of record:
	 *   GMT_WRITE_TBLHEADER: Write an ASCII table header
	 *   GMT_WRITE_SEGHEADER: Write an ASCII or binary segment header
	 *   GMT_WRITE_DOUBLE:    Write an ASCII or binary data record
	 *   GMT_WRITE_TEXT:      Write an ASCII data record
	 * For text: If record == NULL use internal current record or header.
	 * Returns FALSE if a record was written successfully(See what -s[r] can do).
	 * If an error occurs we return TRUE and set API->error.
	 */
	GMT_LONG error = 0, col, ij, wrote = 1, *p = NULL;
	char *s = NULL;
	double *d = NULL;
	PFL GMT_2D_to_index;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMT_VECTOR *V = NULL;
	
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	API->error = GMT_OK;		/* No error yet */
	if (!API->io_enabled[GMT_OUT]) return_error (API, GMT_ACCESS_NOT_ENABLED);

	S = API->object[API->current_item[GMT_OUT]];	/* Shorthand for the data source we are working on */
	if (S->status == GMT_IS_USED) return_error (API, GMT_WRITTEN_ONCE);	/* Only allow writing of a data set once [unless we reset status] */
	switch (S->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
	 	case GMT_IS_STREAM:
	 	case GMT_IS_FDESC:
			switch (mode) {
				case GMT_WRITE_TBLHEADER:	/* Export a table header record; skip if binary */
					s = (record) ? record : API->GMT->current.io.current_record;	/* Default to last input record if NULL */
					GMT_write_tableheader (API->GMT, S->fp, s);
					break;
				case GMT_WRITE_SEGHEADER:	/* Export a segment header record; write NaNs if binary  */
					if (record) strcpy (API->GMT->current.io.segment_header, record);	/* Default to last segment record if NULL */
					GMT_write_segmentheader (API->GMT, S->fp, API->GMT->common.b.ncol[GMT_OUT]);
					break;
				case GMT_WRITE_DOUBLE:		/* Export either a formatted ASCII data record or a binary record */
					if (API->GMT->common.b.ncol[GMT_OUT] < 0) API->GMT->common.b.ncol[GMT_OUT] = API->GMT->common.b.ncol[GMT_IN];
					wrote = API->GMT->current.io.output (API->GMT, S->fp, API->GMT->common.b.ncol[GMT_OUT], record);
					break;
				case GMT_WRITE_TEXT:		/* Export the current text record; skip if binary */
					s = (record) ? record : API->GMT->current.io.current_record;
					GMT_write_textrecord (API->GMT, S->fp, s);
					break;
				default:
					GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: Internal error: GMT_Put_Record called with illegal mode\n");
					return_error (API, GMT_NOT_A_VALID_IO_MODE);	
					break;
			}
			break;
		
		case GMT_IS_COPY:	/* Fill in a DATASET structure with one table only */
			if (S->family == GMT_IS_DATASET) {
				struct GMT_DATASET *D = S->resource;
				struct GMT_TABLE *T = NULL;
				if (!D) {	/* First time allocation */
					D = GMT_create_dataset (API->GMT, 1, -GMT_TINY_CHUNK, 0, 0);	
					S->resource = D;
					API->GMT->current.io.curr_pos[GMT_OUT][1] = -1;	/* Start at seg = -1 */
					D->n_columns = D->table[0]->n_columns = API->GMT->common.b.ncol[GMT_OUT];
				}
				T = D->table[0];	/* GMT_Put_Record only writes one table */
				p = API->GMT->current.io.curr_pos[GMT_OUT];	/* Short hand to counters for table, segment, row */
				switch (mode) {
					case GMT_WRITE_TBLHEADER:	/* Export a table header record; skip if binary */
						s = (record) ? record : API->GMT->current.io.current_record;	/* Default to last input record if NULL */
						/* Hook into table header list */
						break;
					case GMT_WRITE_SEGHEADER:	/* Export a segment header record; write NaNs if binary  */
						p[1]++, p[2] = 0;	/* Go to next segment */
						if (p[1] > 0) GMT_alloc_segment (API->GMT, T->segment[p[1]-1], T->segment[p[1]-1]->n_rows, T->n_columns, FALSE);
						if (p[1] == T->n_alloc) T->segment = GMT_malloc (API->GMT, T->segment, p[1], &T->n_alloc, struct GMT_LINE_SEGMENT *);	
						if (!T->segment[p[1]]) T->segment[p[1]] = GMT_memory (API->GMT, NULL, 1, struct GMT_LINE_SEGMENT);	
						if (record) T->segment[p[1]]->header = strdup (record);	/* Default to last segment record if NULL */
						T->n_segments++;
						break;
					case GMT_WRITE_DOUBLE:		/* Export a segment row */
						if (p[1] == -1) { GMT_report (API->GMT, GMT_MSG_NORMAL, "GMTAPI: Internal Warning: GMT_Put_Record (double) called before any segments declared\n"); p[1] = 0; T->n_segments = 1;}
						if (API->GMT->common.b.ncol[GMT_OUT] < 0) API->GMT->common.b.ncol[GMT_OUT] = API->GMT->common.b.ncol[GMT_IN];
						if (!T->segment[p[1]]) T->segment[p[1]] = GMT_memory (API->GMT, NULL, 1, struct GMT_LINE_SEGMENT);	
						if (p[2] == T->segment[p[1]]->n_alloc) {
							T->segment[p[1]]->n_alloc = (T->segment[p[1]]->n_alloc == 0) ? GMT_CHUNK : T->segment[p[1]]->n_alloc << 1;
							GMT_alloc_segment (API->GMT, T->segment[p[1]], -T->segment[p[1]]->n_alloc, T->n_columns, T->segment[p[1]]->n_rows == 0);
						}
						for (col = 0; col < API->GMT->common.b.ncol[GMT_OUT]; col++) T->segment[p[1]]->coord[col][p[2]] = ((double *)record)[col];
						p[2]++;	T->segment[p[1]]->n_rows++;
						/* Copy from record to current row in Dd */
						break;
					default:
						GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: Internal error: GMT_Put_Record (double) called with illegal mode\n");
						return_error (API, GMT_NOT_A_VALID_IO_MODE);	
						break;
				}
			}
			else {	/* TEXTSET */
				struct GMT_TEXTSET *D = S->resource;
				struct GMT_TEXT_TABLE *T = NULL;
				if (!D) {	/* First time allocation of one table */
					D = GMT_create_textset (API->GMT, 1, -GMT_TINY_CHUNK, 0);	
					S->resource = D;
					API->GMT->current.io.curr_pos[GMT_OUT][1] = -1;	/* Start at seg = -1 */
				}
				T = D->table[0];	/* GMT_Put_Record only writes one table */
				p = API->GMT->current.io.curr_pos[GMT_OUT];	/* Short hand to counters for table, segment, row */
				switch (mode) {
					case GMT_WRITE_TBLHEADER:	/* Export a table header record; skip if binary */
						s = (record) ? record : API->GMT->current.io.current_record;	/* Default to last input record if NULL */
						/* Hook into table header list */
						break;
					case GMT_WRITE_SEGHEADER:	/* Export a segment header record; write NaNs if binary  */
						p[1]++, p[2] = 0;	/* Go to next segment */
						if (p[1] > 0) T->segment[p[1]-1]->record = GMT_memory (API->GMT, T->segment[p[1]-1]->record, T->segment[p[1]-1]->n_rows, char *);
						if (p[1] == T->n_alloc) T->segment = GMT_malloc (API->GMT, T->segment, p[1], &T->n_alloc, struct GMT_TEXT_SEGMENT *);	
						if (!T->segment[p[1]]) T->segment[p[1]] = GMT_memory (API->GMT, NULL, 1, struct GMT_TEXT_SEGMENT);	
						if (record) T->segment[p[1]]->header = strdup (record);	/* Default to last segment record if NULL */
						T->n_segments++;
						break;
					case GMT_WRITE_TEXT:		/* Export a record */
						if (p[1] == -1) { GMT_report (API->GMT, GMT_MSG_NORMAL, "GMTAPI: Internal Warning: GMT_Put_Record (text) called before any segments declared\n"); p[1] = 0; T->n_segments = 1;}
						if (!T->segment[p[1]]) T->segment[p[1]] = GMT_memory (API->GMT, NULL, 1, struct GMT_TEXT_SEGMENT);	/* Allocate new segment */
						if (p[2] == T->segment[p[1]]->n_alloc) {	/* Allocate more records */
							T->segment[p[1]]->n_alloc = (T->segment[p[1]]->n_alloc == 0) ? GMT_CHUNK : T->segment[p[1]]->n_alloc << 1;
							T->segment[p[1]]->record = GMT_memory (API->GMT, NULL, T->segment[p[1]]->n_alloc, char *);
						}
						T->segment[p[1]]->record[p[2]] = strdup (record);
						p[2]++;	T->segment[p[1]]->n_rows++;
						break;
					default:
						GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: Internal error: GMT_Put_Record (text) called with illegal mode\n");
						return_error (API, GMT_NOT_A_VALID_IO_MODE);	
						break;
				}
			}
			break;			
		
	 	case GMT_IS_COPY + GMT_VIA_MATRIX:	/* Data matrix only */
			d = record;
			if (!record) GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: GMT_Put_Record passed a NULL data pointer for method GMT_IS_COPY + GMT_VIA_MATRIX\n");
			if (S->n_rows && API->current_rec[GMT_OUT] >= S->n_rows)
				GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: GMT_Put_Record exceeding limits on rows(?)\n");
			if (mode == GMT_WRITE_SEGHEADER && API->GMT->current.io.multi_segments[GMT_OUT]) {	/* Segment header - flag in data as NaNs */
				for (col = 0; col < API->GMT->common.b.ncol[GMT_OUT]; col++) d[col] = API->GMT->session.d_NaN;
			}
			M = S->resource;
			if (!M) {	/* Was given a NULL pointer == First time allocation, default to double data type */
				if ((M = GMT_create_matrix (API->GMT)) == NULL) {
					return_error (API, GMT_MEMORY_ERROR);	
				}
				S->resource = M;
				M->type = GMTAPI_DOUBLE;
			}
			GMT_2D_to_index = GMTAPI_get_2D_to_index (M->shape, GMT_IS_NORMAL);
			for (col = 0; col < API->GMT->common.b.ncol[GMT_OUT]; col++) {	/* Place the output items */
				ij = GMT_2D_to_index (API->current_rec[GMT_OUT], col, M->dim);
				GMTAPI_put_val (API, &(M->data), d[col], ij, M->type);
			}
			M->n_rows++;
			break;
			
		case GMT_IS_COPY + GMT_VIA_VECTOR:	/* List of column arrays */
		case GMT_IS_REF + GMT_VIA_VECTOR:
			d = record;
			if (!record) GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: GMT_Put_Record passed a NULL data pointer for method GMT_IS_DATASET_ARRAY\n");
			if (S->n_rows && API->current_rec[GMT_OUT] >= S->n_rows)
				GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: GMT_Put_Record exceeding limits on rows(?)\n");
			if (mode == GMT_WRITE_SEGHEADER && API->GMT->current.io.multi_segments[GMT_OUT]) {	/* Segment header - flag in data as NaNs */
				for (col = 0; col < API->GMT->common.b.ncol[GMT_OUT]; col++) d[col] = API->GMT->session.d_NaN;
			}
			V = S->resource;
			if (!V) {	/* Was given a NULL pointer == First time allocation, default to double data type */
				if ((V = GMT_create_vector (API->GMT, API->GMT->common.b.ncol[GMT_OUT])) == NULL) {
					return_error (API, GMT_MEMORY_ERROR);	
				}
				S->resource = V;
				for (col = 0; col < S->n_columns; col++) V->type[col] = GMTAPI_DOUBLE;
			}
			for (col = 0; col < API->GMT->common.b.ncol[GMT_OUT]; col++) {	/* Place the output items */
				GMTAPI_put_val (API, &(V->data[col]), d[col], API->current_rec[GMT_OUT], V->type[col]);
			}
			V->n_rows++;
			break;

		default:
			GMT_report (API->GMT, GMT_MSG_FATAL, "GMTAPI: Internal error: GMT_Put_Record called with illegal method\n");
			return_error (API, GMT_NOT_A_VALID_METHOD);	
			break;
	}

	if (mode == GMT_WRITE_DOUBLE || mode == GMT_WRITE_TEXT) API->current_rec[GMT_OUT]++;	/* Only increment if we placed a data record on the output */
	
	if (S->n_alloc && API->current_rec[GMT_OUT] == S->n_alloc) {	/* Must allocate more memory */
		GMT_LONG size;
		S->n_alloc += GMT_CHUNK;
		size = S->n_alloc;
		size *= API->GMT->common.b.ncol[GMT_OUT];
		if (S->method == (GMT_IS_COPY + GMT_VIA_MATRIX)) {
			M = S->resource;
			if ((error = GMT_alloc_univector (API->GMT, &(M->data), M->type, size)) != GMT_OK) return (error);
		}
		else {
			V = S->resource;
			if ((error = GMT_alloc_vectors (API->GMT, V, size)) != GMT_OK) return (error);
		}
	}
	S->status = GMT_IS_USING;	/* Have started writing to this destination */
	
	return ((wrote) ? FALSE : TRUE);		
}

#ifdef FORTRAN_API
GMT_LONG GMT_Put_Record_ (GMT_LONG *mode, void *record)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Put_Record (GMT_FORTRAN, *mode, record));
	
}
#endif

void * GMT_Create_Data (struct GMTAPI_CTRL *API, GMT_LONG family, GMT_LONG par[])
{
	/* Create an empty container of the requested kind; no data are provided.
	 * The known kinds are GMT_IS_{DATASET,TEXTSET,GMTGRID,CPT}, but we also
	 * allow the creation of the containers for GMT_IS_{VECTOR,MATRIX}.
	 * The par array contains dimensions for tables (par[0] = number of tables,
	 *   par[1] = number of segments per table, par[2] = number of columns,
	 *   and par[3] = number of rows per segment). The array is ignored for
	 * CPT and GMT grids. For GMT_IS_VECTOR, par[0] holds the number of columns.
	 * Return: Pointer to resource, or NULL if an error (set via API->error).
	 */

	void *data = NULL;
	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);
	API->error = GMT_OK;		/* No error yet */
	
	switch (family) {	/* dataset, cpt, text, or grid */
		case GMT_IS_GRID:	/* GMT grid, allocate header but not data array */
		 	data = GMT_create_grid (API->GMT);
			break;
		case GMT_IS_DATASET:	/* GMT dataset, allocate the requested tables */
			data = GMT_create_dataset (API->GMT, par[0], par[1], par[2], par[3]);
			break;
		case GMT_IS_TEXTSET:	/* GMT text dataset, allocate the requested tables */
			data = GMT_create_textset (API->GMT, par[0], par[1], par[2]);
			break;
		case GMT_IS_CPT:	/* GMT CPT table, allocate one with no contents */
		 	data = GMT_create_palette (API->GMT, par[0]);
			break;
		case GMT_IS_MATRIX:	/* GMT matrix container, allocate one with no contents */
		 	data = GMT_create_matrix (API->GMT);
			break;
		case GMT_IS_VECTOR:	/* GMT vector container, allocate one with the requested number of columns but no contents */
		 	data = GMT_create_vector (API->GMT, par[0]);
			break;
		default:
			API->error = GMT_WRONG_KIND;
			break;		
	}
	if (API->error) return_null (API, API->error);
	GMT_report (API->GMT, GMT_MSG_VERBOSE, "Successfully created a new %s\n", GMT_family[family]);
	
	return (data);
}

#ifdef FORTRAN_API
void * GMT_Create_Data_ (GMT_LONG *family, GMT_LONG *par)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Create_Data (GMT_FORTRAN, *family, par));
	
}
#endif

char *ptrvoid (char ** p) { return *p; }	/* Handle as char ** just to determine if address is of a NULL pointer */

GMT_LONG GMT_Destroy_Data (struct GMTAPI_CTRL *API, GMT_LONG mode, void *X)
{
	/* Destroy a resource that is no longer needed.
	 * Mode GMTAPI_ALLOCATE (0) means we dont free objects whose allocation
	 * flag == GMT_REFERENCE (1), otherwise we try to free.
	 * Return: FALSE if all is well, otherwise TRUE and API->error is set.
	 */
	GMT_LONG error, item, direction, object_ID;

	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	API->error = GMT_OK;			/* No error yet */
	if (X == NULL) return (FALSE);		/* Null address, quietly skip */
	if (!ptrvoid(X)) return (FALSE);	/* Null pointer, quietly skip */
	if ((object_ID = GMTAPI_ptr2id (API, X)) == GMTAPI_NOTSET) return_error (API, GMT_OBJECT_NOT_FOUND);	/* Could not find it */
	if ((item = GMTAPI_Validate_ID (API, GMTAPI_NOTSET, object_ID, GMTAPI_NOTSET)) == GMTAPI_NOTSET) return_error (API, API->error);
	
	switch (API->object[item]->family) {	/* dataset, cpt, text table or grid */
		case GMT_IS_GRID:	/* GMT grid */
			error = GMTAPI_Destroy_Grid (API, mode, X);
			break;
		case GMT_IS_DATASET:
			error = GMTAPI_Destroy_Dataset (API, mode, X);
			break;
		case GMT_IS_TEXTSET:
			error = GMTAPI_Destroy_Textset (API, mode, X);
			break;
		case GMT_IS_CPT:
			error = GMTAPI_Destroy_CPT (API, mode, X);
			break;
#ifdef USE_GDAL
		case GMT_IS_IMAGE:
			error = GMTAPI_Destroy_Image (API, mode, X);
			break;
#endif
			
		/* Also allow destoying of intermediate vector and matrix containers */
		case GMT_IS_MATRIX:
			error = GMTAPI_Destroy_Matrix (API, mode, X);
			break;
		case GMT_IS_VECTOR:
			error = GMTAPI_Destroy_Vector (API, mode, X);
			break;
		default:
			return_error (API, GMT_WRONG_KIND);
			break;		
	}
	if (error == GMT_PTR_IS_NULL) return (FALSE);	/* Quietly ignore objects pointing to NULL */
	if (!error) {	/* We successfully freed the items, now remove from IO list */
		GMT_LONG j;
		void *address = API->object[item]->data;
		GMT_report (API->GMT, GMT_MSG_VERBOSE, "GMT_Destroy_Data: freed memory for a %s for object %ld\n", GMT_family[API->object[item]->family], object_ID);
		direction = API->object[item]->direction;		
		if ((error = GMTAPI_Unregister_IO (API, object_ID, GMTAPI_NOTSET))) return_error (API, error);	/* Did not find object */
		for (j = 0; j < API->n_objects; j++) if (API->object[j]->data == address) API->object[j]->data = NULL;	/* Set repeated entries to NULL so we don't try to free twice */
	}
	
	return (FALSE);	/* Returns number of items freed or an error */	
}

#ifdef FORTRAN_API
GMT_LONG GMT_Destroy_Data_ (GMT_LONG *mode, void *X)
{	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Destroy_Data (GMT_FORTRAN, *mode, X));
	
}
#endif
