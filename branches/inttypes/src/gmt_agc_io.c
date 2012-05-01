/*      $Id$
 *
 *	Copyright (c) 1991-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
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
 * Rewritten from original code by Robert Helie.  That code was hard-wired
 * in two applications (gmt2agcgrd.c and agc2gmtgrd.c) based on GMT 3.4.
 * The following code is modified to fit within the gmt_customio style
 * and argument passing.  Note that AGC files are ASSUMED to be gridline-
 * oriented.  If a pixel grid is requested to be written in AGC format
 * we will shrink the region by 0.5 dx|dy to obtain gridline registration.
 * Finally, AGC uses 0.0 to represent NaNs; I know, not the smartest but
 * it is what it is.  P.Wessel.
 */
/*-----------------------------------------------------------
 * Format # :	21
 * Type :	Atlantic Geoscience Genter (AGC) format
 * Prefix :	GMT_agc_
 * Author :	Paul Wessel, by modifying code from Robert Helie
 * Date :	09-APR-2006
 * Purpose:	To transform to/from AGC grid file format
 * Functions :	GMT_agc_read_grd_info, GMT_agc_write_grd_info,
 *		GMT_agc_write_grd_info, GMT_agc_read_grd, GMT_agc_write_grd
 *-----------------------------------------------------------*/

/* Public Functions:

int GMT_is_agc_grid (struct GMT_CTRL *C, char *file)
int GMT_agc_read_grd_info (struct GMT_CTRL *C, struct GRD_HEADER *header)
int GMT_agc_write_grd_info (struct GMT_CTRL *C, struct GRD_HEADER *header)
int GMT_agc_read_grd (struct GMT_CTRL *C, struct GRD_HEADER *header, float *grid, double wesn[], GMT_LONG *pad, GMT_LONG complex_mode)
GMT_LONG GMT_agc_write_grd (struct GMT_CTRL *C, struct GRD_HEADER *header, float *grid, double wesn[], GMT_LONG *pad, GMT_LONG complex_mode)

Private Functions used by the public functions:

GMT_LONG ReadRecord (FILE *fpi, float z[ZBLOCKWIDTH][ZBLOCKHEIGHT])
GMT_LONG WriteRecord (FILE *file, float outz[ZBLOCKWIDTH][ZBLOCKHEIGHT], float *prerec, float *postrec)
void packAGCheader (float *prez, float *postz, struct GRD_HEADER *header)
void SaveAGCHeader (char *remark, float *agchead)

*/

# define ZBLOCKWIDTH 	40
# define ZBLOCKHEIGHT 	40
# define PREHEADSIZE	12
# define POSTHEADSIZE	2
# define HEADINDSIZE	6
# define BUFFHEADSIZE	(HEADINDSIZE + POSTHEADSIZE)
# define RECORDLENGTH 	(ZBLOCKWIDTH*ZBLOCKHEIGHT + PREHEADSIZE + POSTHEADSIZE)

# define AGCHEADINDICATOR	"agchd:"
# define PARAMSIZE		((GRD_REMARK_LEN160 - HEADINDSIZE) / BUFFHEADSIZE)

GMT_LONG ReadRecord (FILE *fpi, float z[ZBLOCKWIDTH][ZBLOCKHEIGHT])
{	/* Reads one block of data, including pre- and post-headers */
	size_t nitems;
	float garbage[PREHEADSIZE];

	if (GMT_fread (garbage, sizeof(float), PREHEADSIZE, fpi) < PREHEADSIZE) return (GMT_GRDIO_READ_FAILED);
	nitems = GMT_fread (z, sizeof(float), (ZBLOCKWIDTH * ZBLOCKHEIGHT), fpi);

	if (nitems != (ZBLOCKWIDTH * ZBLOCKHEIGHT) && !feof(fpi)) return (GMT_GRDIO_READ_FAILED);	/* Bad stuff */
	if (GMT_fread (garbage, sizeof(float), POSTHEADSIZE, fpi) < POSTHEADSIZE) return (GMT_GRDIO_READ_FAILED);
	return (GMT_NOERROR);
}

GMT_LONG WriteRecord (FILE *file, float rec[ZBLOCKWIDTH][ZBLOCKHEIGHT], float *prerec, float *postrec)
{	/* Writes one block of data, including pre- and post-headers */
	if (GMT_fwrite (prerec, sizeof(float), PREHEADSIZE, file) < (size_t)PREHEADSIZE) return (GMT_GRDIO_WRITE_FAILED);
	if (GMT_fwrite (rec, sizeof(float), (ZBLOCKWIDTH * ZBLOCKHEIGHT), file) < (size_t)(ZBLOCKWIDTH * ZBLOCKHEIGHT)) return (GMT_GRDIO_WRITE_FAILED);
	if (GMT_fwrite (postrec, sizeof(float), POSTHEADSIZE, file) < (size_t)POSTHEADSIZE)  return (GMT_GRDIO_WRITE_FAILED); 
	return (GMT_NOERROR);
}

void packAGCheader (float *prez, float *postz, struct GRD_HEADER *header)
{	/* Places grd header info in the AGC header array */
	GMT_memset (prez,  PREHEADSIZE,  float);
	GMT_memset (postz, POSTHEADSIZE, float);
	prez[0] = (float)header->wesn[YLO];
	prez[1] = (float)header->wesn[YHI];
	prez[2] = (float)header->wesn[XLO];
	prez[3] = (float)header->wesn[XHI];
	prez[4] = (float)header->inc[GMT_Y];
	prez[5] = (float)header->inc[GMT_X];
	prez[PREHEADSIZE-1] = (float)RECORDLENGTH;
}

void SaveAGCHeader (char *remark, float *agchead)
{	/* Place AGC header data in remark string */
	char floatvalue[PARAMSIZE+1];	/* Allow space for final \0 */
	COUNTER_MEDIUM i;
	size_t j;

	strcpy (remark, AGCHEADINDICATOR);
	for (i = 0; i < BUFFHEADSIZE; i++) {
		sprintf (floatvalue, "%f", agchead[i]);
		for (j = strlen (floatvalue); j < PARAMSIZE; j++) strcat (floatvalue, " ");
		strcat (remark, floatvalue);
	}
}

GMT_LONG GMT_is_agc_grid (struct GMT_CTRL *C, char *file)
{	/* Determine if file is an AGC grid file. */
	FILE *fp = NULL;
	GMT_LONG nx, ny;
	off_t predicted_size;
	float recdata[RECORDLENGTH], x_min, x_max, y_min, y_max, x_inc, y_inc;
	struct GMT_STAT buf;

	if (!strcmp (file, "=")) return (GMT_GRDIO_PIPE_CODECHECK);	/* Cannot check on pipes */
	if (GMT_STAT (file, &buf)) return (GMT_GRDIO_STAT_FAILED);	/* Inquiry about file failed somehow */
	if ((fp = GMT_fopen (C, file, "rb")) == NULL) return (GMT_GRDIO_OPEN_FAILED);
	if (GMT_fread (recdata, sizeof(float), RECORDLENGTH, fp) < RECORDLENGTH) return (GMT_GRDIO_READ_FAILED);
	
	y_min = recdata[0];	y_max = recdata[1];
	if (y_min >= y_max) return (GMT_GRDIO_BAD_VAL);
	x_min = recdata[2];	x_max = recdata[3];
	if (x_min >= x_max) return (GMT_GRDIO_BAD_VAL);
	y_inc = recdata[4];	x_inc = recdata[5];
	if (x_inc <= 0.0 || y_inc <= 0.0) return (GMT_GRDIO_BAD_VAL);
	nx = GMT_get_n (C, x_min, x_max, x_inc, 0);
	if (nx <= 0) return (GMT_GRDIO_BAD_VAL);
	ny = GMT_get_n (C, y_min, y_max, y_inc, 0);
	if (ny <= 0) return (GMT_GRDIO_BAD_VAL);
	/* OK so far; see if file size matches the predicted size given the header info */
	predicted_size = lrint (ceil ((double)ny /ZBLOCKHEIGHT) * ceil ((double)nx / ZBLOCKWIDTH)) * (ZBLOCKHEIGHT * ZBLOCKWIDTH + PREHEADSIZE + POSTHEADSIZE) * sizeof (float);
	if (predicted_size == buf.st_size) return (GMT_GRD_IS_AF);	/* Yes, appears to be an AGC grid */
	return (GMT_GRDIO_BAD_VAL);
}

GMT_LONG GMT_agc_read_grd_info (struct GMT_CTRL *C, struct GRD_HEADER *header)
{	/* Read header info. NOTE: All AGC files are assumed to be gridline-registered */
	COUNTER_MEDIUM i;
	FILE *fp = NULL;
	float recdata[RECORDLENGTH], agchead[BUFFHEADSIZE];

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (C, GMT_IN);
#endif
		fp = C->session.std[GMT_IN];
	}
	else if ((fp = GMT_fopen (C, header->name, "rb")) == NULL)
		return (GMT_GRDIO_OPEN_FAILED);

	if (GMT_fread (recdata, sizeof(float), RECORDLENGTH, fp) < RECORDLENGTH) return (GMT_GRDIO_READ_FAILED);
	
	header->registration = GMT_GRIDLINE_REG;	/* Hardwired since no info about this in the header */
	header->wesn[XLO] = (double)recdata[2];
	header->wesn[XHI] = (double)recdata[3];
	header->wesn[YLO] = (double)recdata[0];
	header->wesn[YHI] = (double)recdata[1];
	header->inc[GMT_Y] = (double)recdata[4];
	header->inc[GMT_X] = (double)recdata[5];
	header->nx = GMT_grd_get_nx (C, header);
	header->ny = GMT_grd_get_ny (C, header);
	header->z_scale_factor = 1.0;
	header->z_add_offset = 0.0;
	for (i = 6; i < PREHEADSIZE; i++) agchead[i-6] = recdata[i];
	agchead[BUFFHEADSIZE-2] = recdata[RECORDLENGTH-2];
	agchead[BUFFHEADSIZE-1] = recdata[RECORDLENGTH-1];
	SaveAGCHeader (header->remark, agchead);
	
	GMT_fclose (C, fp);

	return (GMT_NOERROR);
}

GMT_LONG GMT_agc_write_grd_info (struct GMT_CTRL *C, struct GRD_HEADER *header)
{	/* Write grd header info to file */
	FILE *fp = NULL;
	float prez[PREHEADSIZE], postz[POSTHEADSIZE];

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (C, GMT_OUT);
#endif
		fp = C->session.std[GMT_OUT];
	}
	else if ((fp = GMT_fopen (C, header->name, "rb+")) == NULL && (fp = GMT_fopen (C, header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);
	
	packAGCheader (prez, postz, header);	/* Stuff header info into the AGC arrays */

	if (GMT_fwrite (prez, sizeof(float), PREHEADSIZE, fp) < PREHEADSIZE) return (GMT_GRDIO_WRITE_FAILED);

	GMT_fclose (C, fp);

	return (GMT_NOERROR);
}

GMT_LONG GMT_agc_read_grd (struct GMT_CTRL *C, struct GRD_HEADER *header, float *grid, double wesn[], GMT_LONG *pad, GMT_LONG complex_mode)
{	/* header:     	grid structure header
	 * grid:	array with final grid
	 * wesn:	Sub-region to extract  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex_mode:	1|2 if complex array is to hold real (1) and imaginary (2) parts (0 = read as real only)
	 *		Note: The file has only real values, we simply allow space in the complex array
	 *		for real and imaginary parts when processed by grdfft etc.
	 */

	COUNTER_MEDIUM first_col, last_col;		/* First and last column to deal with */
	COUNTER_MEDIUM first_row, last_row;		/* First and last row to deal with */
	COUNTER_MEDIUM width_in;			/* Number of items in one row of the subregion */
	COUNTER_MEDIUM width_out;			/* Width of row as return (may include padding) */
	COUNTER_MEDIUM height_in;			/* Number of columns in subregion */
	COUNTER_MEDIUM inc, off;			/* Step in array: 1 for ordinary data, 2 for complex (skipping imaginary) */
	COUNTER_MEDIUM i, j, j_gmt, i_0_out;		/* Misc. counters */
	COUNTER_MEDIUM *k = NULL;			/* Array with indices */
	COUNTER_MEDIUM block, n_blocks, n_blocks_x, n_blocks_y;	/* Misc. counters */
	COUNTER_MEDIUM datablockcol, datablockrow, rowstart, rowend, colstart, colend, row, col;
	COUNTER_LARGE ij;
	float z[ZBLOCKWIDTH][ZBLOCKHEIGHT];
	FILE *fp = NULL;			/* File pointer to data or pipe */
	
	if (!strcmp (header->name, "=")) {	/* Read from pipe */
#ifdef SET_IO_MODE
		GMT_setmode (C, GMT_IN);
#endif
		fp = C->session.std[GMT_IN];
	}
	else if ((fp = GMT_fopen (C, header->name, "rb")) == NULL)
		return (GMT_GRDIO_OPEN_FAILED);

	GMT_err_pass (C, GMT_grd_prep_io (C, header, wesn, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row, &k), header->name);
	(void)GMT_init_complex (complex_mode, &inc, &off);	/* Set stride and offset if complex */

	width_out = width_in;		/* Width of output array */
	if (pad[XLO] > 0) width_out += pad[XLO];
	if (pad[XHI] > 0) width_out += pad[XHI];

	width_out *= inc;			/* Possibly twice if complex is TRUE */
	i_0_out = inc * pad[XLO] + off;		/* Edge offset in output */

	/* Because of the 40x40 blocks we read the entire file and only use what we need */

	/* Rows are read south to north */
	
	header->z_min = +DBL_MAX;	header->z_max = -DBL_MAX;
	
	n_blocks_y = lrint (ceil ((double)header->ny / (double)ZBLOCKHEIGHT));
	n_blocks_x = lrint (ceil ((double)header->nx / (double)ZBLOCKWIDTH));
	n_blocks = n_blocks_x * n_blocks_y;
	datablockcol = datablockrow = 0;
	for (block = 0; block < n_blocks; block++) {
		if (ReadRecord (fp, z)) return (GMT_GRDIO_READ_FAILED);
		rowstart = datablockrow * ZBLOCKHEIGHT;
		rowend = MIN (rowstart + ZBLOCKHEIGHT, (COUNTER_MEDIUM)header->ny);
		for (i = 0, row = rowstart; row < rowend; i++, row++) {
			j_gmt = header->ny - 1 - row;	/* GMT internal row number */
			if (j_gmt < first_row || j_gmt > last_row) continue;
			colstart = datablockcol * ZBLOCKWIDTH;
			colend = MIN (colstart + ZBLOCKWIDTH, (COUNTER_MEDIUM)header->nx);
			for (j = 0, col = colstart; col < colend; j++, col++) {
				if (col < first_col || col > last_col) continue;
				ij = (((j_gmt - first_row) + pad[YHI]) * width_out + inc * (col - first_col)) + i_0_out;
				grid[ij] = (z[j][i] == 0.0) ? C->session.f_NaN : z[j][i];	/* AGC uses exact zero as NaN flag */
				if (GMT_is_fnan (grid[ij])) continue;
				header->z_min = MIN (header->z_min, (double)grid[ij]);
				header->z_max = MAX (header->z_max, (double)grid[ij]);
			}
		}

		if (++datablockrow >= n_blocks_y) {
			datablockrow = 0;
			datablockcol++;
		}
	}
	GMT_free (C, k);

	header->nx = (int)width_in;	header->ny = (int)height_in;
	GMT_memcpy (header->wesn, wesn, 4, double);

	GMT_fclose (C, fp);
	
	return (GMT_NOERROR);
}

GMT_LONG GMT_agc_write_grd (struct GMT_CTRL *C, struct GRD_HEADER *header, float *grid, double wesn[], GMT_LONG *pad, GMT_LONG complex_mode)
{	/* header:	grid structure header
	 * grid:	array with final grid
	 * wesn:	Sub-region to write out  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex_mode:	1|2 if complex array is to hold real (1) and imaginary (2) parts (0 = read as real only)
	 *		Note: The file has only real values, we simply allow space in the complex array
	 *		for real and imaginary parts when processed by grdfft etc.
	 */


	COUNTER_MEDIUM first_col, last_col;		/* First and last column to deal with */
	COUNTER_MEDIUM first_row, last_row;		/* First and last row to deal with */
	COUNTER_MEDIUM width_in;			/* Number of items in one row of the subregion */
	COUNTER_MEDIUM width_out;			/* Width of row as return (may include padding) */
	COUNTER_MEDIUM height_out;			/* Number of columns in subregion */
	COUNTER_MEDIUM inc;				/* Step in array: 1 for ordinary data, 2 for complex (skipping imaginary) */
	COUNTER_MEDIUM off;				/* Complex array offset: 0 for real, 1 for imaginary */
	COUNTER_MEDIUM i, j, i2, j2;			/* Misc. counters */
	COUNTER_MEDIUM *k = NULL;			/* Array with indices */
	COUNTER_MEDIUM block, n_blocks, n_blocks_x, n_blocks_y;	/* Misc. counters */
	COUNTER_MEDIUM rowstart, rowend, colstart, colend = 0, datablockcol, datablockrow;
	COUNTER_MEDIUM j_gmt, row, col;
	COUNTER_LARGE ij;
	float prez[PREHEADSIZE], postz[POSTHEADSIZE];
	float outz[ZBLOCKWIDTH][ZBLOCKHEIGHT];
	FILE *fp = NULL;			/* File pointer to data or pipe */

	if (!strcmp (header->name, "=")) {	/* Write to pipe */
#ifdef SET_IO_MODE
		GMT_setmode (C, GMT_OUT);
#endif
		fp = C->session.std[GMT_OUT];
	}
	else if ((fp = GMT_fopen (C, header->name, "rb+")) == NULL && (fp = GMT_fopen (C, header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);
	
	GMT_err_pass (C, GMT_grd_prep_io (C, header, wesn, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row, &k), header->name);
	(void)GMT_init_complex (complex_mode, &inc, &off);	/* Set stride and offset if complex */

	width_in = width_out;		/* Physical width of input array */
	if (pad[XLO] > 0) width_in += pad[XLO];
	if (pad[XHI] > 0) width_in += pad[XHI];

	GMT_memcpy (header->wesn, wesn, 4, double);

	/* Find z_min/z_max */

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = first_row, j2 = pad[YHI]; j <= last_row; j++, j2++) {
		for (i = first_col, i2 = pad[XLO]; i <= last_col; i++, i2++) {
			ij = (j2 * width_in + i2) * inc + off;
			if (GMT_is_fnan (grid[ij]))	/* in AGC, NaN <--> 0.0 */
				grid[ij] = 0.0;
			else {
				header->z_min = MIN (header->z_min, (double)grid[ij]);
				header->z_max = MAX (header->z_max, (double)grid[ij]);
			}
		}
	}
	
	/* Since AGC files are always gridline-registered we must change -R when a pixel grid is to be written */
	if (header->registration == GMT_PIXEL_REG) {
		GMT_change_grdreg (C, header, GMT_GRIDLINE_REG);
		GMT_report (C, GMT_MSG_NORMAL, "Warning: AGC grids are always gridline-registered.  Your pixel-registered grid will be converted.\n");
		GMT_report (C, GMT_MSG_NORMAL, "Warning: AGC grid region in file %s reset to %g/%g/%g/%g\n", header->name, header->wesn[XLO], header->wesn[XHI], header->wesn[YLO], header->wesn[YHI]);
	}
	
	packAGCheader (prez, postz, header);	/* Stuff header info into the AGC arrays */

	n_blocks_y = lrint (ceil ((double)header->ny / (double)ZBLOCKHEIGHT));
	n_blocks_x = lrint (ceil ((double)header->nx / (double)ZBLOCKWIDTH));
	n_blocks = n_blocks_x * n_blocks_y;
	datablockcol = datablockrow = 0;
	for (block = 0; block < n_blocks; block++) {
		rowstart = datablockrow * ZBLOCKHEIGHT;
		rowend = MIN (rowstart + ZBLOCKHEIGHT, (COUNTER_MEDIUM)header->ny);
		for (i = 0, row = rowstart; row < rowend; i++, row++) {
			j_gmt = header->ny - 1 - row;	/* GMT internal row number */
			if (j_gmt < first_row || j_gmt > last_row) continue;
			colstart = datablockcol * ZBLOCKWIDTH;
			colend = MIN (colstart + ZBLOCKWIDTH, (COUNTER_MEDIUM)header->nx);
			for (j = 0, col = colstart; col < colend; j++, col++) {
				if (col < first_col || col > last_col) continue;
				ij = ((j_gmt - first_row) + pad[YHI]) * width_in + (col - first_col) + pad[XLO];
				outz[j][i] = grid[inc*ij+off];
			}
		} 

		if (WriteRecord (fp, outz, prez, postz)) return (GMT_GRDIO_WRITE_FAILED);

		if (++datablockrow >= n_blocks_y) {
			datablockrow = 0;
			datablockcol++;
		}
	}
	GMT_free (C, k);

	GMT_fclose (C, fp);

	return (GMT_NOERROR);
}
