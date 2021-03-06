/*      $Id$
 *
 *	Copyright (c) 1991-2015 by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis and F. Wobbe
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

int GMT_is_agc_grid (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header)
int GMT_agc_read_grd_info (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header)
int GMT_agc_write_grd_info (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header)
int GMT_agc_read_grd (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header, float *grid, double wesn[], unsigned int *pad, unsigned int complex_mode)
int GMT_agc_write_grd (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header, float *grid, double wesn[], unsigned int *pad, unsigned int complex_mode)

Private Functions used by the public functions:

int ReadRecord (FILE *fpi, float z[ZBLOCKWIDTH][ZBLOCKHEIGHT])
int WriteRecord (FILE *file, float *outz[ZBLOCKWIDTH][ZBLOCKHEIGHT], float *prerec, float *postrec)
void packAGCheader (float *prez, float *postz, struct GMT_GRID_HEADER *header)
void SaveAGCHeader (char *remark, float *agchead)

*/

# define ZBLOCKWIDTH 	40U
# define ZBLOCKHEIGHT 	40U
# define PREHEADSIZE	12U
# define POSTHEADSIZE	2U
# define HEADINDSIZE	6U
# define BUFFHEADSIZE	(HEADINDSIZE + POSTHEADSIZE)
# define RECORDLENGTH 	(ZBLOCKWIDTH*ZBLOCKHEIGHT + PREHEADSIZE + POSTHEADSIZE)

# define AGCHEADINDICATOR	"agchd:"
# define PARAMSIZE		((GMT_GRID_REMARK_LEN160 - HEADINDSIZE) / BUFFHEADSIZE)

int ReadRecord (FILE *fpi, float z[ZBLOCKWIDTH][ZBLOCKHEIGHT])
{	/* Reads one block of data, including pre- and post-headers */
	size_t nitems;
	float garbage[PREHEADSIZE];

	if (GMT_fread (garbage, sizeof(float), PREHEADSIZE, fpi) < PREHEADSIZE)
		return (GMT_GRDIO_READ_FAILED);
	nitems = GMT_fread (z, sizeof(float), ZBLOCKWIDTH * ZBLOCKHEIGHT, fpi);

	if (nitems != ZBLOCKWIDTH * ZBLOCKHEIGHT && !feof(fpi))
		return (GMT_GRDIO_READ_FAILED);	/* Bad stuff */
	if (GMT_fread (garbage, sizeof(float), POSTHEADSIZE, fpi) < POSTHEADSIZE)
		return (GMT_GRDIO_READ_FAILED);
	return (GMT_NOERROR);
}

int WriteRecord (FILE *file, float rec[ZBLOCKWIDTH][ZBLOCKHEIGHT], float *prerec, float *postrec)
{	/* Writes one block of data, including pre- and post-headers */
	if (GMT_fwrite (prerec, sizeof(float), PREHEADSIZE, file) < PREHEADSIZE)
		return (GMT_GRDIO_WRITE_FAILED);
	if (GMT_fwrite (rec, sizeof(float), ZBLOCKWIDTH * ZBLOCKHEIGHT, file) < ZBLOCKWIDTH * ZBLOCKHEIGHT)
		return (GMT_GRDIO_WRITE_FAILED);
	if (GMT_fwrite (postrec, sizeof(float), POSTHEADSIZE, file) < POSTHEADSIZE)
		return (GMT_GRDIO_WRITE_FAILED);
	return (GMT_NOERROR);
}

void packAGCheader (float *prez, float *postz, struct GMT_GRID_HEADER *header)
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
	unsigned int i;
	size_t j;

	strcpy (remark, AGCHEADINDICATOR);
	for (i = 0; i < BUFFHEADSIZE; i++) {
		sprintf (floatvalue, "%f", agchead[i]);
		for (j = strlen (floatvalue); j < PARAMSIZE; j++) strcat (floatvalue, " ");
		strcat (remark, floatvalue);
	}
}

int GMT_is_agc_grid (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header) {
	/* Determine if file is an AGC grid file. */
	FILE *fp = NULL;
	int nx, ny;
	off_t predicted_size;
	float recdata[RECORDLENGTH], x_min, x_max, y_min, y_max, x_inc, y_inc;
	struct stat buf;

	if (!strcmp (header->name, "="))
		return (GMT_GRDIO_PIPE_CODECHECK);		/* Cannot check on pipes */
	if (stat (header->name, &buf))
		return (GMT_GRDIO_STAT_FAILED);			/* Inquiry about file failed somehow */
	if ((fp = GMT_fopen (GMT, header->name, "rb")) == NULL)
		return (GMT_GRDIO_OPEN_FAILED);			/* Opening the file failed somehow */
	if (GMT_fread (recdata, sizeof(float), RECORDLENGTH, fp) < RECORDLENGTH)
		return (GMT_GRDIO_READ_FAILED);

	y_min = recdata[0];	y_max = recdata[1];
	if (y_min >= y_max)
		return (GMT_GRDIO_BAD_VAL);
	x_min = recdata[2];	x_max = recdata[3];
	if (x_min >= x_max)
		return (GMT_GRDIO_BAD_VAL);
	y_inc = recdata[4];	x_inc = recdata[5];
	if (x_inc <= 0.0 || y_inc <= 0.0)
		return (GMT_GRDIO_BAD_VAL);
	nx = (int)GMT_get_n (GMT, x_min, x_max, x_inc, 0);
	if (nx <= 0)
		return (GMT_GRDIO_BAD_VAL);
	ny = (int)GMT_get_n (GMT, y_min, y_max, y_inc, 0);
	if (ny <= 0)
		return (GMT_GRDIO_BAD_VAL);
	/* OK so far; see if file size matches the predicted size given the header info */
	predicted_size = lrint (ceil ((double)ny /ZBLOCKHEIGHT) * ceil ((double)nx / ZBLOCKWIDTH)) * (ZBLOCKHEIGHT * ZBLOCKWIDTH + PREHEADSIZE + POSTHEADSIZE) * sizeof (float);
	if (predicted_size == buf.st_size) {
		/* Yes, appears to be an AGC grid */
		header->type = GMT_GRID_IS_AF;
		header->nan_value = 0.0f; /* NaN value for AGC format */
		return GMT_NOERROR;
	}
	return GMT_GRDIO_BAD_VAL;
}

int GMT_agc_read_grd_info (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header)
{	/* Read header info. NOTE: All AGC files are assumed to be gridline-registered */
	unsigned int i;
	FILE *fp = NULL;
	float recdata[RECORDLENGTH], agchead[BUFFHEADSIZE];

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT, GMT_IN);
#endif
		fp = GMT->session.std[GMT_IN];
	}
	else if ((fp = GMT_fopen (GMT, header->name, "rb")) == NULL)
		return (GMT_GRDIO_OPEN_FAILED);

	if (GMT_fread (recdata, sizeof(float), RECORDLENGTH, fp) < RECORDLENGTH) return (GMT_GRDIO_READ_FAILED);
	
	header->registration = GMT_GRID_NODE_REG;	/* Hardwired since no info about this in the header */
	header->wesn[XLO]  = recdata[2];
	header->wesn[XHI]  = recdata[3];
	header->wesn[YLO]  = recdata[0];
	header->wesn[YHI]  = recdata[1];
	header->inc[GMT_Y] = recdata[4];
	header->inc[GMT_X] = recdata[5];
	header->nx = GMT_grd_get_nx (GMT, header);
	header->ny = GMT_grd_get_ny (GMT, header);
	header->z_scale_factor = 1.0;
	header->z_add_offset = 0.0;
	for (i = 6; i < PREHEADSIZE; i++) agchead[i-6] = recdata[i];
	agchead[BUFFHEADSIZE-2] = recdata[RECORDLENGTH-2];
	agchead[BUFFHEADSIZE-1] = recdata[RECORDLENGTH-1];
	SaveAGCHeader (header->remark, agchead);
	
	GMT_fclose (GMT, fp);

	return (GMT_NOERROR);
}

int GMT_agc_write_grd_info (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header)
{	/* Write grd header info to file */
	FILE *fp = NULL;
	float prez[PREHEADSIZE], postz[POSTHEADSIZE];

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT, GMT_OUT);
#endif
		fp = GMT->session.std[GMT_OUT];
	}
	else if ((fp = GMT_fopen (GMT, header->name, "rb+")) == NULL && (fp = GMT_fopen (GMT, header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);
	
	packAGCheader (prez, postz, header);	/* Stuff header info into the AGC arrays */

	if (GMT_fwrite (prez, sizeof(float), PREHEADSIZE, fp) < PREHEADSIZE) return (GMT_GRDIO_WRITE_FAILED);

	GMT_fclose (GMT, fp);

	return (GMT_NOERROR);
}

int GMT_agc_read_grd (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header, float *grid, double wesn[], unsigned int *pad, unsigned int complex_mode)
{	/* header:     	grid structure header
	 * grid:	array with final grid
	 * wesn:	Sub-region to extract  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex_mode:	&4 | &8 if complex array is to hold real (4) and imaginary (8) parts (otherwise read as real only)
	 *		Note: The file has only real values, we simply allow space in the complex array
	 *		for real and imaginary parts when processed by grdfft etc.
	 */

	int first_col, last_col, j, col;		/* First and last column to deal with */
	int first_row, last_row, j_gmt, colend;		/* First and last row to deal with */
	unsigned int width_in;			/* Number of items in one row of the subregion */
	unsigned int width_out;			/* Width of row as return (may include padding) */
	unsigned int height_in;			/* Number of columns in subregion */
	unsigned int i;				/* Misc. counters */
	unsigned int *k = NULL;			/* Array with indices */
	unsigned int block, n_blocks, n_blocks_x, n_blocks_y;	/* Misc. counters */
	unsigned int datablockcol, datablockrow, rowstart, rowend, colstart, row;
	uint64_t ij, imag_offset;
	float z[ZBLOCKWIDTH][ZBLOCKHEIGHT];
	FILE *fp = NULL;			/* File pointer to data or pipe */
	
	if (!strcmp (header->name, "=")) {	/* Read from pipe */
#ifdef SET_IO_MODE
		GMT_setmode (GMT, GMT_IN);
#endif
		fp = GMT->session.std[GMT_IN];
	}
	else if ((fp = GMT_fopen (GMT, header->name, "rb")) == NULL)
		return (GMT_GRDIO_OPEN_FAILED);

	GMT_err_pass (GMT, GMT_grd_prep_io (GMT, header, wesn, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row, &k), header->name);
	(void)GMT_init_complex (header, complex_mode, &imag_offset);	/* Set offset for imaginary complex component */

	width_out = width_in;		/* Width of output array */
	if (pad[XLO] > 0) width_out += pad[XLO];
	if (pad[XHI] > 0) width_out += pad[XHI];

	/* Because of the 40x40 blocks we read the entire file and only use what we need */

	/* Rows are read south to north */
	
	GMT_memset (z, ZBLOCKWIDTH * ZBLOCKHEIGHT, float); /* Initialize buffer to zero */

	header->z_min = +DBL_MAX;	header->z_max = -DBL_MAX;
	
	n_blocks_y = urint (ceil ((double)header->ny / (double)ZBLOCKHEIGHT));
	n_blocks_x = urint (ceil ((double)header->nx / (double)ZBLOCKWIDTH));
	n_blocks = n_blocks_x * n_blocks_y;
	datablockcol = datablockrow = 0;
	for (block = 0; block < n_blocks; block++) {
		if (ReadRecord (fp, z)) return (GMT_GRDIO_READ_FAILED);
		rowstart = datablockrow * ZBLOCKHEIGHT;
		rowend = MIN (rowstart + ZBLOCKHEIGHT, header->ny);
		for (i = 0, row = rowstart; row < rowend; i++, row++) {
			j_gmt = header->ny - 1 - row;	/* GMT internal row number */
			if (j_gmt < first_row || j_gmt > last_row) continue;
			colstart = datablockcol * ZBLOCKWIDTH;
			colend = MIN (colstart + ZBLOCKWIDTH, header->nx);
			for (j = 0, col = colstart; col < colend; j++, col++) {
				if (col < first_col || col > last_col) continue;
				ij = imag_offset + (((j_gmt - first_row) + pad[YHI]) * width_out + col - first_col) + pad[XLO];
				grid[ij] = (z[j][i] == 0.0) ? GMT->session.f_NaN : z[j][i];	/* AGC uses exact zero as NaN flag */
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
	GMT_free (GMT, k);

	header->nx = width_in;	header->ny = height_in;
	GMT_memcpy (header->wesn, wesn, 4, double);

	GMT_fclose (GMT, fp);
	
	return (GMT_NOERROR);
}

int GMT_agc_write_grd (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header, float *grid, double wesn[], unsigned int *pad, unsigned int complex_mode)
{	/* header:	grid structure header
	 * grid:	array with final grid
	 * wesn:	Sub-region to write out  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex_mode:	&4 | &8 if complex array is to hold real (4) and imaginary (8) parts (otherwise read as real only)
	 *		Note: The file has only real values, we simply allow space in the complex array
	 *		for real and imaginary parts when processed by grdfft etc.
	 */

	int first_col, last_col, col, colend = 0;		/* First and last column to deal with */
	int j_gmt, i, j, first_row, last_row;		/* First and last row to deal with */
	unsigned int width_in;			/* Number of items in one row of the subregion */
	unsigned int width_out;			/* Width of row as return (may include padding) */
	unsigned int height_out;			/* Number of columns in subregion */
	unsigned int i2, j2;			/* Misc. counters */
	unsigned int *k = NULL;			/* Array with indices */
	unsigned int block, n_blocks, n_blocks_x, n_blocks_y;	/* Misc. counters */
	unsigned int row, rowstart, rowend, colstart, datablockcol, datablockrow;
	uint64_t kk, ij, imag_offset;
	float prez[PREHEADSIZE], postz[POSTHEADSIZE];
	float outz[ZBLOCKWIDTH][ZBLOCKHEIGHT];
	FILE *fp = NULL;			/* File pointer to data or pipe */

	if (!strcmp (header->name, "=")) {	/* Write to pipe */
#ifdef SET_IO_MODE
		GMT_setmode (GMT, GMT_OUT);
#endif
		fp = GMT->session.std[GMT_OUT];
	}
	else if ((fp = GMT_fopen (GMT, header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);
	
	GMT_err_pass (GMT, GMT_grd_prep_io (GMT, header, wesn, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row, &k), header->name);
	(void)GMT_init_complex (header, complex_mode, &imag_offset);	/* Set offset for imaginary complex component */

	width_in = width_out;		/* Physical width of input array */
	if (pad[XLO] > 0) width_in += pad[XLO];
	if (pad[XHI] > 0) width_in += pad[XHI];

	GMT_memcpy (header->wesn, wesn, 4, double);

	/* Find z_min/z_max */

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = first_row, j2 = pad[YHI]; j <= last_row; j++, j2++) {
		ij = imag_offset + j2 * width_in;
		for (i = first_col, i2 = pad[XLO]; i <= last_col; i++, i2++) {
			kk = ij + i2;
			if (GMT_is_fnan (grid[kk]))	/* in AGC, NaN <--> 0.0 */
				grid[ij] = 0.0f;
			else {
				header->z_min = MIN (header->z_min, (double)grid[kk]);
				header->z_max = MAX (header->z_max, (double)grid[kk]);
			}
		}
	}

	/* Since AGC files are always gridline-registered we must change -R when a pixel grid is to be written */
	if (header->registration == GMT_GRID_PIXEL_REG) {
		GMT_change_grdreg (GMT, header, GMT_GRID_NODE_REG);
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Warning: AGC grids are always gridline-registered.  Your pixel-registered grid will be converted.\n");
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Warning: AGC grid region in file %s reset to %g/%g/%g/%g\n", header->name, header->wesn[XLO], header->wesn[XHI], header->wesn[YLO], header->wesn[YHI]);
	}

	packAGCheader (prez, postz, header);	/* Stuff header info into the AGC arrays */

	GMT_memset (outz, ZBLOCKWIDTH * ZBLOCKHEIGHT, float); /* or WriteRecord (fp, outz, prez, postz); points to uninitialised buffer */

	n_blocks_y = urint (ceil ((double)header->ny / (double)ZBLOCKHEIGHT));
	n_blocks_x = urint (ceil ((double)header->nx / (double)ZBLOCKWIDTH));
	n_blocks = n_blocks_x * n_blocks_y;
	datablockcol = datablockrow = 0;
	for (block = 0; block < n_blocks; block++) {
		rowstart = datablockrow * ZBLOCKHEIGHT;
		rowend = MIN (rowstart + ZBLOCKHEIGHT, header->ny);
		for (i = 0, row = rowstart; row < rowend; i++, row++) {
			j_gmt = header->ny - 1 - row;	/* GMT internal row number */
			if (j_gmt < first_row || j_gmt > last_row) continue;
			colstart = datablockcol * ZBLOCKWIDTH;
			colend = MIN (colstart + ZBLOCKWIDTH, header->nx);
			for (j = 0, col = colstart; col < colend; j++, col++) {
				if (col < first_col || col > last_col) continue;
				ij = imag_offset + ((j_gmt - first_row) + pad[YHI]) * width_in + (col - first_col) + pad[XLO];
				outz[j][i] = grid[ij];
			}
		}

		if (WriteRecord (fp, outz, prez, postz)) return (GMT_GRDIO_WRITE_FAILED);

		if (++datablockrow >= n_blocks_y) {
			datablockrow = 0;
			datablockcol++;
		}
	}
	GMT_free (GMT, k);

	GMT_fclose (GMT, fp);

	return (GMT_NOERROR);
}
