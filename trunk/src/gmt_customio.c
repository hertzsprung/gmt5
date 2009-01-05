/*--------------------------------------------------------------------
 *	$Id: gmt_customio.c,v 1.71 2009-01-05 04:55:49 guru Exp $
 *
 *	Copyright (c) 1991-2008 by P. Wessel and W. H. F. Smith
 *	See COPYING file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it wi1552ll be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 *
 *	G M T _ C U S T O M I O . C   R O U T I N E S
 *
 * Takes care of all custom format gridfile input/output.  The
 * industrious user may supply his/her own code to read specific data
 * formats.  Five functions must be supplied, and they must conform
 * to the GMT standard and return the same arguments as the generic
 * GMT grdio functions.  See gmt_cdf.c for details.
 *
 * To add another data format:
 *
 *	1. Write the five required routines (see below).
 *	2. increment parameter GMT_N_GRD_FORMATS in file gmt_grdio.h
 *	3. Append another entry in the gmt_customio.h file.
 *	4. Provide another entry in the $GMT_SHAREDIR/conf/gmt_formats.conf file
 *
 * Author:	Paul Wessel
 * Date:	9-SEP-1992
 * Modified:	06-APR-2006
 * Version:	4.1.2
 *
 * Functions include:
 *
 *	GMT_*_read_grd_info :	Read header from file
 *	GMT_*_read_grd :	Read header and data set from file
 *	GMT_*_update_grd_info :	Update header in existing file
 *	GMT_*_write_grd_info :	Write header to new file
 *	GMT_*_write_grd :	Write header and data set to new file
 *
 * where * is a tag specific to a particular data format
 *
 * NOTE:  1. GMT assumes that GMT_read_grd_info has been called before calls
 *	     to GMT_read_grd.
 *	  2. Some formats may permit pipes to be used.  In that case GMT
 *	     expects the filename to be "=" (the equal sign).  It is the
 *	     responsibility of the custom routines to test for "=" and
 *	     give error messages if piping is not supported for this format
 *	     (e.g., netcdf uses fseek and can therefore not use pipes; other
 *	     formats may have similar limitations).
 *	  3. For most formats the write_grd_info and update_grd_info is the
 *	     same function (but netCDF is one exception)
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define GMT_WITH_NO_PS
#include "gmt.h"

int GMT_read_rasheader (FILE *fp, struct rasterfile *h);
int GMT_write_rasheader (FILE *fp, struct rasterfile *h);
int GMT_native_read_grd_header (FILE *fp, struct GRD_HEADER *header);
int GMT_native_write_grd_header (FILE *fp, struct GRD_HEADER *header);
int GMT_native_skip_grd_header (FILE *fp, struct GRD_HEADER *header);
int GMT_native_read_grd_info (struct GRD_HEADER *header);
int GMT_native_write_grd_info (struct GRD_HEADER *header);
int GMT_native_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);
int GMT_native_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);

void GMT_grdio_init (void) {
	int id;

	/* FORMAT # 0: DEFAULT: GMT netCDF-based (float) grdio, same as # 18 */

	id = 0;
	GMT_io_readinfo[id]   = (PFI) GMT_nc_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_nc_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_nc_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_nc_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_nc_write_grd;

	/* FORMAT # 1: GMT native binary (float) grdio [No loop over 1 &2 due to MS bug]*/

	id = 1;
	GMT_io_readinfo[id]   = (PFI) GMT_native_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_native_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_native_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_native_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_native_write_grd;

	/* FORMAT # 2: GMT native binary (short) grdio */

	id = 2;
	GMT_io_readinfo[id]   = (PFI) GMT_native_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_native_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_native_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_native_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_native_write_grd;

	/* FORMAT # 3: SUN 8-bit standard rasterfile grdio */

	id = 3;
	GMT_io_readinfo[id]   = (PFI) GMT_ras_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_ras_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_ras_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_ras_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_ras_write_grd;

	/* FORMAT # 4: GMT native binary (byte) grdio */

	id = 4;
	GMT_io_readinfo[id]   = (PFI) GMT_native_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_native_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_native_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_native_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_native_write_grd;

	/* FORMAT # 5: GMT native binary (bit) grdio */

	id = 5;
	GMT_io_readinfo[id]   = (PFI) GMT_native_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_native_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_native_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_bit_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_bit_write_grd;

	/* FORMAT # 6: GMT native binary (float) grdio (Surfer format) */

	id = 6;
	GMT_io_readinfo[id]   = (PFI) GMT_srf_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_srf_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_srf_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_srf_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_srf_write_grd;

	/* FORMAT # 7: GMT netCDF-based (byte) grdio */
 
	id = 7;
	GMT_io_readinfo[id]   = (PFI) GMT_cdf_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_cdf_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_cdf_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_cdf_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_cdf_write_grd;

	/* FORMAT # 8: GMT netCDF-based (short) grdio */
 
	id = 8;
	GMT_io_readinfo[id]   = (PFI) GMT_cdf_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_cdf_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_cdf_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_cdf_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_cdf_write_grd;

	/* FORMAT # 9: GMT netCDF-based (int) grdio */
 
	id = 9;
	GMT_io_readinfo[id]   = (PFI) GMT_cdf_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_cdf_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_cdf_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_cdf_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_cdf_write_grd;

	/* FORMAT # 10: GMT netCDF-based (float) grdio */
 
	id = 10;
	GMT_io_readinfo[id]   = (PFI) GMT_cdf_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_cdf_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_cdf_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_cdf_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_cdf_write_grd;

	/* FORMAT # 11: GMT netCDF-based (double) grdio */
 
	id = 11;
	GMT_io_readinfo[id]   = (PFI) GMT_cdf_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_cdf_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_cdf_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_cdf_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_cdf_write_grd;

	/* FORMAT # 12: NOAA NGDC MGG grid format */

	id = 12; 
	GMT_io_readinfo[id]   = (PFI) mgg2_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) mgg2_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) mgg2_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) mgg2_read_grd;
	GMT_io_writegrd[id]   = (PFI) mgg2_write_grd;

	/* FORMAT # 13: GMT native binary (int) grdio */

	id = 13;
	GMT_io_readinfo[id]   = (PFI) GMT_native_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_native_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_native_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_native_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_native_write_grd;

	/* FORMAT # 14: GMT native binary (double) grdio */

	id = 14;
	GMT_io_readinfo[id]   = (PFI) GMT_native_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_native_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_native_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_native_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_native_write_grd;

	/* FORMAT # 15: GMT netCDF-based (byte) grdio (COARDS compliant) */

	id = 15;
	GMT_io_readinfo[id]   = (PFI) GMT_nc_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_nc_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_nc_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_nc_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_nc_write_grd;

	/* FORMAT # 16: GMT netCDF-based (short) grdio (COARDS compliant) */

	id = 16;
	GMT_io_readinfo[id]   = (PFI) GMT_nc_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_nc_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_nc_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_nc_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_nc_write_grd;

	/* FORMAT # 17: GMT netCDF-based (int) grdio (COARDS compliant) */

	id = 17;
	GMT_io_readinfo[id]   = (PFI) GMT_nc_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_nc_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_nc_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_nc_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_nc_write_grd;

	/* FORMAT # 18: GMT netCDF-based (float) grdio (COARDS compliant) */

	id = 18;
	GMT_io_readinfo[id]   = (PFI) GMT_nc_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_nc_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_nc_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_nc_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_nc_write_grd;

	/* FORMAT # 19: GMT netCDF-based (double) grdio (COARDS compliant) */

	id = 19;
	GMT_io_readinfo[id]   = (PFI) GMT_nc_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_nc_update_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_nc_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_nc_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_nc_write_grd;

	/* FORMAT # 20: GMT native binary (double) grdio (Surfer format) */

	id = 20;
	GMT_io_readinfo[id]   = (PFI) GMT_srf_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_srf_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_srf_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_srf_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_srf_write_grd;

	/* FORMAT # 21: GMT native binary (float) grdio (AGC format) */

	id = 21;
	GMT_io_readinfo[id]   = (PFI) GMT_agc_read_grd_info;
	GMT_io_updateinfo[id] = (PFI) GMT_agc_write_grd_info;
	GMT_io_writeinfo[id]  = (PFI) GMT_agc_write_grd_info;
	GMT_io_readgrd[id]    = (PFI) GMT_agc_read_grd;
	GMT_io_writegrd[id]   = (PFI) GMT_agc_write_grd;

	/*
	 * ----------------------------------------------
	 * ADD CUSTOM FORMATS BELOW AS THEY ARE NEEDED */
}

/* CUSTOM I/O FUNCTIONS FOR GRIDDED DATA FILES */

/*-----------------------------------------------------------
 * Format # :	3
 * Type :	Standard 8-bit Sun rasterfiles
 * Prefix :	GMT_ras_
 * Author :	Paul Wessel, SOEST
 * Date :	17-JUN-1999
 * Purpose:	Rasterfiles may often be used to store
 *		datasets of limited dynamic range where
 *		8-bits is all that is needed.  Since the
 *		usual information like w,e,s,n is not part
 *		of a rasterfile's header, we assign default
 *		values as follows:
 *			w = s = 0.
 *			e = ras_width;
 *			n = ras_height;
 *			dx = dy = 1
 *		Such files are always pixel registered
 *
 * Functions :	GMT_ras_read_grd_info,
 *		GMT_ras_write_grd_info, GMT_ras_read_grd, GMT_ras_write_grd
 *-----------------------------------------------------------*/

#define	RAS_MAGIC	0x59a66a95

int GMT_is_ras_grid (struct GRD_HEADER *header)
{	/* Determine if file is a Sun rasterfile */
	FILE *fp;
	struct rasterfile h;
	if (!strcmp(header->name, "=")) return (GMT_GRDIO_PIPE_CODECHECK);	/* Cannot check on pipes */
	if ((fp = GMT_fopen (header->name, "rb")) == NULL) return (GMT_GRDIO_OPEN_FAILED);
	if (GMT_read_rasheader (fp, &h)) return (GMT_GRDIO_READ_FAILED);
	if (h.magic != RAS_MAGIC) return (GMT_GRDIO_NOT_RAS);
	if (h.type != 1 || h.depth != 8) return (GMT_GRDIO_NOT_8BIT_RAS);
	header->type = GMT_grd_format_decoder ("rb");
	return (header->type);
}

int GMT_ras_read_grd_info (struct GRD_HEADER *header)
{
	FILE *fp;
	struct rasterfile h;
	unsigned char u;
	int i;

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_IN);
#endif
		fp = GMT_stdin;
	}
	else if ((fp = GMT_fopen (header->name, "rb")) == NULL)
		return (GMT_GRDIO_OPEN_FAILED);

	if (GMT_read_rasheader (fp, &h)) return (GMT_GRDIO_READ_FAILED);
	if (h.magic != RAS_MAGIC) return (GMT_GRDIO_NOT_RAS);
	if (h.type != 1 || h.depth != 8) return (GMT_GRDIO_NOT_8BIT_RAS);

	for (i = 0; i < h.maplength; i++) {
		if (GMT_fread ((void *)&u, sizeof (unsigned char *), (size_t)1, fp) < (size_t)1) return (GMT_GRDIO_READ_FAILED);	/* Skip colormap by reading since fp could be stdin */
	}
	if (fp != GMT_stdin) GMT_fclose (fp);

	/* Since we have no info on boundary values, just use integer size and steps = 1 */

	header->x_min = header->y_min = 0.0;
	header->x_max = header->nx = h.width;
	header->y_max = header->ny = h.height;
	header->x_inc = header->y_inc = 1.0;
	header->node_offset = 1;	/* Pixel format */
	header->z_scale_factor = 1;	header->z_add_offset = 0;

	return (GMT_NOERROR);
}

int GMT_ras_write_grd_info (struct GRD_HEADER *header)
{
	FILE *fp;
	struct rasterfile h;

	if (!strcmp (header->name, "="))
	{
#ifdef SET_IO_MODE
		GMT_setmode (GMT_OUT);
#endif
		fp = GMT_stdout;
	}
	else if ((fp = GMT_fopen (header->name, "rb+")) == NULL && (fp = GMT_fopen (header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);

	h.magic = RAS_MAGIC;
	h.width = header->nx;
	h.height = header->ny;
	h.depth = 8;
	h.length = header->ny * (int) ceil (header->nx/2.0) * 2;
	h.type = 1;
	h.maptype = 0;
	h.maplength = 0;

	if (GMT_write_rasheader (fp, &h)) return (GMT_GRDIO_WRITE_FAILED);

	if (fp != GMT_stdout) GMT_fclose (fp);

	return (GMT_NOERROR);
}

int GMT_ras_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:	grid structure header */
	/* grid:	array with final grid */
	/* w,e,s,n:	Sub-region to extract  [Use entire file if 0,0,0,0] */
	/* padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively */
	/* complex:	TRUE if array is to hold real and imaginary parts (read in real only) */
	/*		Note: The file has only real values, we simply allow space in the array */
	/*		for imaginary parts when processed by grdfft etc. */

	GMT_LONG first_col, last_col, first_row, last_row, inc = 1;
	GMT_LONG i, j, j2, width_in, width_out, height_in, i_0_out, n2;
	GMT_LONG kk, ij;
	FILE *fp;
	BOOLEAN piping = FALSE, check;
	unsigned char *tmp;
	struct rasterfile h;
	int *k;

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_IN);
#endif
		fp = GMT_stdin;
		piping = TRUE;
	}
	else if ((fp = GMT_fopen (header->name, "rb")) != NULL) {	/* Skip header */
		if (GMT_read_rasheader (fp, &h)) return (GMT_GRDIO_READ_FAILED);
		if (h.maplength && GMT_fseek (fp, (long) h.maplength, SEEK_CUR)) return (GMT_GRDIO_SEEK_FAILED);
	}
	else
		return (GMT_GRDIO_OPEN_FAILED);

	n2 = (GMT_LONG) ceil (header->nx / 2.0) * 2;	/* Sun 8-bit rasters are stored using 16-bit words */
	tmp = (unsigned char *) GMT_memory (VNULL, (size_t)n2, sizeof (unsigned char), "GMT_ras_read_grd");

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;

	check = !GMT_is_dnan (header->nan_value);

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	width_out = width_in;		/* Width of output array */
	if (pad[0] > 0) width_out += pad[0];
	if (pad[1] > 0) width_out += pad[1];

	i_0_out = pad[0];		/* Edge offset in output */
	if (complex) {	/* Need twice as much output space since we load every 2nd cell */
		width_out *= 2;
		i_0_out *= 2;
		inc = 2;
	}

	if (piping) {	/* Skip data by reading it */
		for (j = 0; j < first_row; j++) {
			if (GMT_fread ((void *) tmp, sizeof (unsigned char), (size_t)n2, fp) < (size_t)n2) return (GMT_GRDIO_READ_FAILED);
		}
	}
	else {/* Simply seek by it */
		if (GMT_fseek (fp, (long) (first_row * n2 * sizeof (unsigned char)), SEEK_CUR)) return (GMT_GRDIO_SEEK_FAILED);
	}
	for (j = first_row, j2 = 0; j <= last_row; j++, j2++) {
		ij = (j2 + pad[3]) * width_out + i_0_out;	/* Already has factor of 2 in it if complex */
		if (GMT_fread ((void *) tmp, sizeof (unsigned char), (size_t)n2, fp) < (size_t)n2) return (GMT_GRDIO_READ_FAILED);
		for (i = 0; i < width_in; i++) {
			kk = ij + inc * i;
			grid[kk] = (float) tmp[k[i]];
			if (check && grid[kk] == header->nan_value) grid[kk] = GMT_f_NaN;
			if (GMT_is_fnan (grid[kk])) continue;
			header->z_min = MIN (header->z_min, (double)grid[kk]);
			header->z_max = MAX (header->z_max, (double)grid[kk]);
		}
	}
	if (piping) {	/* Skip data by reading it */
		for (j = last_row + 1; j < header->ny; j++) if (GMT_fread ((void *) tmp, sizeof (unsigned char), (size_t)n2, fp) < (size_t)n2) return (GMT_GRDIO_READ_FAILED);
	}
	header->nx = width_in;
	header->ny = height_in;
	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	if (fp != GMT_stdin) GMT_fclose (fp);

	GMT_free ((void *)k);
	GMT_free ((void *)tmp);
	return (GMT_NOERROR);
}

int GMT_ras_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:	grid structure header */
	/* grid:	array with final grid */
	/* w,e,s,n:	Sub-region to write  [Use entire file if 0,0,0,0] */
	/* padding:	# of empty rows/columns to remove on w, e, s, n of grid, respectively */
	/* complex:	Must be FALSE for rasterfiles.    If 64 is added we write no header */

	GMT_LONG i, i2, inc = 1;
	GMT_LONG j, j2, width_in, width_out, height_out, n2;
	GMT_LONG first_col, last_col, first_row, last_row;
	int *k;
	GMT_LONG kk, ij;

	BOOLEAN check, do_header = TRUE;

	unsigned char *tmp;

	FILE *fp;

	struct rasterfile h;

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_OUT);
#endif
		fp = GMT_stdout;
	}
	else if ((fp = GMT_fopen (header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);

	h.magic = RAS_MAGIC;
	h.width = header->nx;
	h.height = header->ny;
	h.depth = 8;
	h.length = header->ny * (int) ceil (header->nx/2.0) * 2;
	h.type = 1;
	h.maptype = 0;
	h.maplength = 0;

	n2 = (GMT_LONG) ceil (header->nx / 2.0) * 2;
	tmp = (unsigned char *) GMT_memory (VNULL, (size_t)n2, sizeof (unsigned char), "GMT_ras_write_grd");

	check = !GMT_is_dnan (header->nan_value);

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	if (complex >= 64) {	/* Want no header, adjust complex */
		complex %= 64;
		do_header = FALSE;
	}
	if (complex) inc = 2;

	width_in = width_out;		/* Physical width of input array */
	if (pad[0] > 0) width_in += pad[0];
	if (pad[1] > 0) width_in += pad[1];

	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	h.width = header->nx;
	h.height = header->ny;
	h.length = header->ny * (int) ceil (header->nx/2.0) * 2;

	/* store header information and array */

	if (do_header && GMT_write_rasheader (fp, &h)) return (GMT_GRDIO_WRITE_FAILED);

	i2 = first_col + pad[0];
	for (j = 0, j2 = first_row + pad[3]; j < height_out; j++, j2++) {
		ij = j2 * width_in + i2;
		for (i = 0; i < width_out; i++) {
			kk = inc * (ij + k[i]);
			if (check && GMT_is_fnan (grid[kk])) grid[kk] = (float)header->nan_value;
			tmp[i] = (unsigned char) grid[kk];
		}
		if (GMT_fwrite ((void *)tmp, sizeof (unsigned char), (size_t)width_out, fp) < (size_t)width_out) return (GMT_GRDIO_WRITE_FAILED);
	}
	if (fp != GMT_stdout) GMT_fclose (fp);

	GMT_free ((void *)k);
	GMT_free ((void *)tmp);

	return (GMT_NOERROR);

}

int GMT_read_rasheader (FILE *fp, struct rasterfile *h)
{
	/* Reads the header of a Sun rasterfile byte by byte
	   since the format is defined as the byte order on the
	   PDP-11.
	 */

	unsigned char byte[4];
	GMT_LONG i, j, value, in[4];

	for (i = 0; i < 8; i++) {

		if (GMT_fread ((void *)byte, sizeof (unsigned char), (size_t)4, fp) != 4) return (GMT_GRDIO_READ_FAILED);

		for (j = 0; j < 4; j++) in[j] = (GMT_LONG)byte[j];

		value = (in[0] << 24) + (in[1] << 16) + (in[2] << 8) + in[3];

		switch (i) {
			case 0:
				h->magic = value;
				break;
			case 1:
				h->width = value;
				break;
			case 2:
				h->height = value;
				break;
			case 3:
				h->depth = value;
				break;
			case 4:
				h->length = value;
				break;
			case 5:
				h->type = value;
				break;
			case 6:
				h->maptype = value;
				break;
			case 7:
				h->maplength = value;
				break;
		}
	}

	if (h->type == RT_OLD && h->length == 0) h->length = 2 * irint (ceil (h->width * h->depth / 16.0)) * h->height;

	return (GMT_NOERROR);
}

int GMT_write_rasheader (FILE *fp, struct rasterfile *h)
{
	/* Writes the header of a Sun rasterfile byte by byte
	   since the format is defined as the byte order on the
	   PDP-11.
	 */

	unsigned char byte[4];
	GMT_LONG i, value;

	if (h->type == RT_OLD && h->length == 0) {
		h->length = 2 * irint (ceil (h->width * h->depth / 16.0)) * h->height;
		h->type = RT_STANDARD;
	}

	for (i = 0; i < 8; i++) {

		switch (i) {
			case 0:
				value = h->magic;
				break;
			case 1:
				value = h->width;
				break;
			case 2:
				value = h->height;
				break;
			case 3:
				value = h->depth;
				break;
			case 4:
				value = h->length;
				break;
			case 5:
				value = h->type;
				break;
			case 6:
				value = h->maptype;
				break;
			default:
				value = h->maplength;
				break;
		}
		byte[0] = (unsigned char)((value >> 24) & 0xFF);
		byte[1] = (unsigned char)((value >> 16) & 0xFF);
		byte[2] = (unsigned char)((value >> 8) & 0xFF);
		byte[3] = (unsigned char)(value & 0xFF);

		if (GMT_fwrite ((void *)byte, sizeof (unsigned char), (size_t)4, fp) != 4) return (GMT_GRDIO_WRITE_FAILED);
	}

	return (GMT_NOERROR);
}

/*-----------------------------------------------------------
 * Format # :	5
 * Type :	Native binary (bit) C file
 * Prefix :	GMT_bit_
 * Author :	Paul Wessel, SOEST
 * Date :	27-JUN-1994
 * Purpose:	The native binary bit format is used
 *		primarily for piped i/o between programs
 *		that otherwise would use temporary, large
 *		intermediate grdfiles.  Note that not all
 *		programs can support piping (they may have
 *		to re-read the file or accept more than one
 *		grdfile).  Datasets containing ON/OFF information
 *		like bitmasks can be stored using bits
 *		We use 4-byte integers to store 32 bits at the time
 * Functions :	GMT_bit_read_grd, GMT_bit_write_grd
 *-----------------------------------------------------------*/

int GMT_is_native_grid (struct GRD_HEADER *header)
{
	struct STAT buf;
	GMT_LONG nm, mx, status, size;
	double item_size;
	struct GRD_HEADER t_head;
	
	if (!strcmp(header->name, "=")) return (GMT_GRDIO_PIPE_CODECHECK);	/* Cannot check on pipes */
	if (STAT (header->name, &buf)) return (GMT_GRDIO_STAT_FAILED);		/* Inquiry about file failed somehow */
	strcpy (t_head.name, header->name);
	if ((status = GMT_native_read_grd_info (&t_head))) return (GMT_GRDIO_READ_FAILED);	/* Failed to read header */
	if (t_head.nx <= 0 || t_head.ny <= 0) return (GMT_GRDIO_BAD_VAL);		/* Garbage for nx or ny */
	nm = ((GMT_LONG)t_head.nx) * ((GMT_LONG)t_head.ny);
	if (nm <= 0) return (GMT_GRDIO_BAD_VAL);		/* Overflow for nx * ny? */
	item_size = (buf.st_size - GRD_HEADER_SIZE) / nm;	/* Estimate size of elements */
	size = irint (item_size);
	if (!GMT_IS_ZERO(item_size - (double)size)) return (GMT_GRDIO_BAD_VAL);	/* Size not an integer */
	
	switch (size) {
		case 0:	/* Possibly bit map; check some more */
			mx = (GMT_LONG) ceil (t_head.nx / 32.0);
			nm = mx * ((GMT_LONG)t_head.ny);
			if ((buf.st_size - GRD_HEADER_SIZE) == nm)	/* yes it was a bit mask file */
				header->type = GMT_grd_format_decoder ("bm");
			else	/* No, junk data */
				return (GMT_GRDIO_BAD_VAL);
			break;
		case 1:	/* 1-byte elements */
			header->type = GMT_grd_format_decoder ("bb");
			break;
		case 2:	/* 2-byte short int elements */
			header->type = GMT_grd_format_decoder ("bs");
			break;
		case 4:	/* 4-byte elements - could be int or float */
			/* See if we can decide it is a float grid */
			if ((t_head.z_scale_factor == 1.0 && t_head.z_add_offset == 0.0) || fabs((t_head.z_min/t_head.z_scale_factor) - rint(t_head.z_min/t_head.z_scale_factor)) > GMT_CONV_LIMIT || fabs((t_head.z_max/t_head.z_scale_factor) - rint(t_head.z_max/t_head.z_scale_factor)) > GMT_CONV_LIMIT)
				header->type = GMT_grd_format_decoder ("bf");
			else
				header->type = GMT_grd_format_decoder ("bi");
			break;
		case 8:	/* 8-byte elements */
			header->type = GMT_grd_format_decoder ("bd");
			break;
		default:	/* Garbage */
			return (GMT_GRDIO_BAD_VAL);
			break;
	}
	return (header->type);
}

int GMT_bit_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:	grid structure header */
	/* grid:	array with final grid */
	/* w,e,s,n:	Sub-region to extract  [Use entire file if 0,0,0,0] */
	/* padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively */
	/* complex:	TRUE if array is to hold real and imaginary parts (read in real only) */
	/*		Note: The file has only real values, we simply allow space in the array */
	/*		for imaginary parts when processed by grdfft etc. */

	GMT_LONG first_col, last_col, first_row, last_row, word, bit, err;
	GMT_LONG i, j, j2, width_in, width_out, height_in, i_0_out, inc = 1, mx;
	int *k;
	GMT_LONG kk, ij;
	FILE *fp;
	BOOLEAN piping = FALSE, check = FALSE;
	unsigned int *tmp, ival;

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_IN);
#endif
		fp = GMT_stdin;
		piping = TRUE;
	}
	else if ((fp = GMT_fopen (header->name, "rb")) != NULL) {	/* Skip header */
		GMT_err_trap (GMT_native_skip_grd_header (fp, header));
	}
	else
		return (GMT_GRDIO_OPEN_FAILED);

	check = !GMT_is_dnan (header->nan_value);
	mx = (GMT_LONG) ceil (header->nx / 32.0);

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	width_out = width_in;		/* Width of output array */
	if (pad[0] > 0) width_out += pad[0];
	if (pad[1] > 0) width_out += pad[1];

	i_0_out = pad[0];		/* Edge offset in output */
	if (complex) {	/* Need twice as much output space since we load every 2nd cell */
		width_out *= 2;
		i_0_out *= 2;
		inc = 2;
	}
	tmp = (unsigned int *) GMT_memory (VNULL, (size_t)mx, sizeof (unsigned int), "GMT_bit_read_grd");

	if (piping) {	/* Skip data by reading it */
		for (j = 0; j < first_row; j++) if (GMT_fread ((void *) tmp, sizeof (unsigned int), (size_t)mx, fp) < (size_t)mx) return (GMT_GRDIO_READ_FAILED);
	}
	else {		/* Simply seek by it */
		if (GMT_fseek (fp, (long) (first_row * mx * sizeof (unsigned int)), SEEK_CUR)) return (GMT_GRDIO_SEEK_FAILED);
	}
	for (j = first_row, j2 = 0; j <= last_row; j++, j2++) {
		if (GMT_fread ((void *) tmp, sizeof (unsigned int), (size_t)mx, fp) < (size_t)mx) return (GMT_GRDIO_READ_FAILED);	/* Get one row */
		ij = (j2 + pad[3]) * width_out + i_0_out;	/* Already has factor of 2 in it if complex */
		for (i = 0; i < width_in; i++) {
			kk = ij + inc * i;
			word = k[i] / 32;
			bit = k[i] % 32;
			ival = (tmp[word] >> bit) & 1;
			grid[kk] = (float) ival;
			if (check && grid[kk] == header->nan_value) grid[kk] = GMT_f_NaN;
		}
	}
	if (piping) {	/* Skip data by reading it */
		for (j = last_row + 1; j < header->ny; j++) if (GMT_fread ((void *) tmp, sizeof (unsigned int), (size_t)mx, fp) < (size_t)mx) return (GMT_GRDIO_READ_FAILED);
	}

	header->nx = width_in;
	header->ny = height_in;
	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = 0; j < header->ny; j++) {
		for (i = 0; i < header->nx; i++) {
			ij = inc * ((j + pad[3]) * width_out + i + pad[0]);
			if (GMT_is_fnan (grid[ij])) continue;
			header->z_min = MIN (header->z_min, (double)grid[ij]);
			header->z_max = MAX (header->z_max, (double)grid[ij]);
		}
	}
	if (fp != GMT_stdin) GMT_fclose (fp);

	GMT_free ((void *)k);
	GMT_free ((void *)tmp);
	return (GMT_NOERROR);
}

int GMT_bit_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:	grid structure header */
	/* grid:	array with final grid */
	/* w,e,s,n:	Sub-region to extract  [Use entire file if 0,0,0,0] */
	/* padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively */
	/* complex:	TRUE if array is to hold real and imaginary parts (read in real only) */
	/*		Note: The file has only real values, we simply allow space in the array */
	/*		for imaginary parts when processed by grdfft etc.   If 64 is added we write no header*/

	int *k;
	GMT_LONG i, i2;
	GMT_LONG j, j2, width_in, width_out, height_out, mx, word, bit, err, inc = 1;
	GMT_LONG first_col, last_col, first_row, last_row;
	GMT_LONG kk, ij;
	BOOLEAN check = FALSE, do_header = TRUE;
	unsigned int *tmp, ival;

	FILE *fp;

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_OUT);
#endif
		fp = GMT_stdout;
	}
	else if ((fp = GMT_fopen (header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);

	check = !GMT_is_dnan (header->nan_value);

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	if (complex >= 64) {	/* Want no header, adjust complex */
		complex %= 64;
		do_header = FALSE;
	}
	if (complex) inc = 2;

	width_in = width_out;		/* Physical width of input array */
	if (pad[0] > 0) width_in += pad[0];
	if (pad[1] > 0) width_in += pad[1];

	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	/* Find z_min/z_max */

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = first_row, j2 = pad[3]; j <= last_row; j++, j2++) {
		for (i = first_col, i2 = pad[0]; i <= last_col; i++, i2++) {
			ij = inc * (j2 * width_in + i2);
			if (GMT_is_fnan (grid[ij])) {
				if (check) grid[ij] = (float)header->nan_value;
			}
			else {
				ival = (unsigned int) irint ((double)grid[ij]);
				if (ival > 1) ival = 1;	/* Truncate to 1 */
				header->z_min = MIN (header->z_min, (double)ival);
				header->z_max = MAX (header->z_max, (double)ival);
			}
		}
	}

	/* Store header information and array */

	if (do_header) {
		GMT_err_trap (GMT_native_write_grd_header (fp, header));
	}

	mx = (GMT_LONG) ceil (width_out / 32.0);
	tmp = (unsigned int *) GMT_memory (VNULL, (size_t)mx, sizeof (unsigned int), "GMT_bit_write_grd");

	i2 = first_col + pad[0];
	for (j = 0, j2 = first_row + pad[3]; j < height_out; j++, j2++) {
		memset ((void *)tmp, 0, (size_t)(mx * sizeof (unsigned int)));
		ij = j2 * width_in + i2;
		for (i = 0; i < width_out; i++) {
			kk = inc * (ij + k[i]);
			word = i / 32;
			bit = i % 32;
			ival = (unsigned int) irint ((double)grid[kk]);
			if (ival > 1) ival = 1;	/* Truncate to 1 */
			tmp[word] |= (ival << bit);
		}
		if (GMT_fwrite ((void *)tmp, sizeof (unsigned int), (size_t)mx, fp) < (size_t)mx) return (GMT_GRDIO_WRITE_FAILED);
	}

	if (fp != GMT_stdout) GMT_fclose (fp);

	GMT_free ((void *)k);
	GMT_free ((void *)tmp);

	return (GMT_NOERROR);
}

/*-----------------------------------------------------------
 * Format # :	1, 2, 4, 13, 14
 * Type :	Native binary C file
 * Prefix :	GMT_native_
 * Author :	Paul Wessel, SOEST
 * Date :	17-JUN-1999
 * Purpose:	The native binary output format is used
 *		primarily for piped i/o between programs
 *		that otherwise would use temporary, large
 *		intermediate grdfiles.  Note that not all
 *		programs can support piping (they may have
 *		to re-read the file or accept more than one
 *		grdfile).  Datasets with limited range may
 *		be stored using 1-, 2-, or 4-byte integers
 *		which will reduce storage space and improve
 *		throughput.
 *-----------------------------------------------------------*/

/* GMT 64-bit Modification:
 *
 * Read/write GRD header structure from native binary file.  This is
 * used by all the native binary formats in GMT.  We isolate the I/O of
 * the header structure here because of 32/64 bit issues of alignment.
 * The GRD header is 892 bytes long, three 4-byte integers followed
 * by ten 8-byte doubles and six character strings. This created a
 * problem on 64-bit systems, where the GRD_HEADER structure was
 * automatically padded with 4-bytes before the doubles. Taking
 * advantage of the code developed to deal with the 32/64-bit anomaly
 * (below), we have permanently added a 4-byte integer to the
 * GRD_HEADER structure, but skip it when reading or writing the
 * header.
 */

int GMT_native_read_grd_header (FILE *fp, struct GRD_HEADER *header)
{
	int err = GMT_NOERROR;
	/* Because GRD_HEADER is not 64-bit aligned we must read it in parts */
	if (GMT_fread ((void *)&header->nx, 3*sizeof (int), (size_t)1, fp) != 1 || GMT_fread ((void *)&header->x_min, sizeof (struct GRD_HEADER) - ((long)&header->x_min - (long)&header->nx), (size_t)1, fp) != 1)
                err = GMT_GRDIO_READ_FAILED;
	return (err);
}

int GMT_native_write_grd_header (FILE *fp, struct GRD_HEADER *header)
{
	int err = GMT_NOERROR;
	/* Because GRD_HEADER is not 64-bit aligned we must write it in parts */

	if (GMT_fwrite ((void *)&header->nx, 3*sizeof (int), (size_t)1, fp) != 1 || GMT_fwrite ((void *)&header->x_min, sizeof (struct GRD_HEADER) - ((long)&header->x_min - (long)&header->nx), (size_t)1, fp) != 1)
                err = GMT_GRDIO_WRITE_FAILED;
	return (err);
}

int GMT_native_skip_grd_header (FILE *fp, struct GRD_HEADER *header)
{
	int err = GMT_NOERROR;
	/* Because GRD_HEADER is not 64-bit aligned we must estimate the # of bytes in parts */
	
	if (GMT_fseek (fp, (long)(3*sizeof (int) + sizeof (struct GRD_HEADER) - ((long)&header->x_min - (long)&header->nx)), SEEK_SET))
                err = GMT_GRDIO_SEEK_FAILED;
	return (err);
}

int GMT_native_read_grd_info (struct GRD_HEADER *header)
{
	/* Read GRD header structure from native binary file.  This is used by
	 * all the native binary formats in GMT */

	int err;
	FILE *fp;

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_IN);
#endif
		fp = GMT_stdin;
	}
	else if ((fp = GMT_fopen (header->name, "rb")) == NULL)
		return (GMT_GRDIO_OPEN_FAILED);
	
	GMT_err_trap (GMT_native_read_grd_header (fp, header));

	if (fp != GMT_stdin) GMT_fclose (fp);

	return (GMT_NOERROR);
}

int GMT_native_write_grd_info (struct GRD_HEADER *header)
{
	/* Write GRD header structure to native binary file.  This is used by
	 * all the native binary formats in GMT */

	int err;
	FILE *fp;

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_OUT);
#endif
		fp = GMT_stdout;
	}
	else if ((fp = GMT_fopen (header->name, "rb+")) == NULL && (fp = GMT_fopen (header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);
	
	GMT_err_trap (GMT_native_write_grd_header (fp, header));

	if (fp != GMT_stdout) GMT_fclose (fp);

	return (GMT_NOERROR);
}

int GMT_native_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:	grid structure header */
	/* grid:	array with final grid */
	/* w,e,s,n:	Sub-region to extract  [Use entire file if 0,0,0,0] */
	/* padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively */
	/* complex:	TRUE if array is to hold real and imaginary parts (read in real only) */
	/*		Note: The file has only real values, we simply allow space in the array */
	/*		for imaginary parts when processed by grdfft etc. */

	GMT_LONG first_col, last_col;	/* First and last column to deal with */
	GMT_LONG first_row, last_row;	/* First and last row to deal with */
	GMT_LONG height_in;			/* Number of columns in subregion */
	int inc = 1;			/* Step in array: 1 for ordinary data, 2 for complex (skipping imaginary) */
	int err, i, j, j2, i_0_out;	/* Misc. counters */
	int *k;				/* Array with indices */
	int type;			/* Data type */
	size_t size;			/* Length of data type */
	GMT_LONG width_in;			/* Number of items in one row of the subregion */
	GMT_LONG kk, ij;
	GMT_LONG width_out;			/* Width of row as return (may include padding) */
	FILE *fp;			/* File pointer to data or pipe */
	BOOLEAN piping = FALSE;		/* TRUE if we read input pipe instead of from file */
	BOOLEAN check = FALSE;		/* TRUE if nan-proxies are used to signify NaN (for non-floating point types) */
	void *tmp;			/* Array pointer for reading in rows of data */

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_IN);
#endif
		fp = GMT_stdin;
		piping = TRUE;
	}
	else if ((fp = GMT_fopen (header->name, "rb")) != NULL)	{	/* Skip header */
		GMT_err_trap (GMT_native_skip_grd_header (fp, header));
	}
	else
		return (GMT_GRDIO_OPEN_FAILED);

	type = GMT_grdformats[header->type][1];
	size = GMT_grd_data_size (header->type, &header->nan_value);
	check = !GMT_is_dnan (header->nan_value);

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	width_out = width_in;		/* Width of output array */
	if (pad[0] > 0) width_out += pad[0];
	if (pad[1] > 0) width_out += pad[1];

	i_0_out = pad[0];		/* Edge offset in output */

	if (complex) {	/* Need twice as much output space since we load every 2nd cell */
		width_out *= 2;
		i_0_out *= 2;
		inc = 2;
	}

	/* Allocate memory for one row of data (for reading purposes) */

	tmp = (void *) GMT_memory (VNULL, (size_t)header->nx, size, "GMT_native_read_grd");

	/* Now deal with skipping */

	if (piping) {	/* Skip data by reading it */
		for (j = 0; j < first_row; j++) if (GMT_fread (tmp, size, (size_t)header->nx, fp) < (size_t)header->nx) return (GMT_GRDIO_READ_FAILED);
	}
	else {		/* Simply seek over it */
		if (GMT_fseek (fp, (long) (first_row * header->nx * size), SEEK_CUR)) return (GMT_GRDIO_SEEK_FAILED);
	}

	for (j = first_row, j2 = 0; j <= last_row; j++, j2++) {
		if (GMT_fread (tmp, (size_t)size, (size_t)header->nx, fp) < (size_t)header->nx) return (GMT_GRDIO_READ_FAILED);	/* Get one row */
		ij = (j2 + pad[3]) * width_out + i_0_out;	/* Already has factor of 2 in it if complex */
		for (i = 0; i < width_in; i++) {
			kk = ij + inc * i;
			grid[kk] = GMT_decode (tmp, k[i], type);	/* Convert whatever to float */
			if (check && grid[kk] == header->nan_value) grid[kk] = GMT_f_NaN;
		}
	}
	if (piping) {	/* Skip remaining data by reading it */
		for (j = last_row + 1; j < header->ny; j++) if (GMT_fread (tmp, (size_t)size, (size_t)header->nx, fp) < (size_t)header->nx) return (GMT_GRDIO_READ_FAILED);
	}

	header->nx = width_in;
	header->ny = height_in;
	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	/* Update z_min, z_max */

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = 0; j < header->ny; j++) {
		for (i = 0; i < header->nx; i++) {
			ij = inc * ((j + pad[3]) * width_out + i + pad[0]);
			if (GMT_is_fnan (grid[ij])) continue;
			header->z_min = MIN (header->z_min, (double)grid[ij]);
			header->z_max = MAX (header->z_max, (double)grid[ij]);
		}
	}
	if (fp != GMT_stdin) GMT_fclose (fp);

	GMT_free ((void *)k);
	GMT_free (tmp);

	return (GMT_NOERROR);
}

int GMT_native_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:	grid structure header */
	/* grid:	array with final grid */
	/* w,e,s,n:	Sub-region to write out  [Use entire file if 0,0,0,0] */
	/* padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively */
	/* complex:	TRUE if array is to hold real and imaginary parts (read in real only) */
	/*		Note: The file has only real values, we simply allow space in the array */
	/*		for imaginary parts when processed by grdfft etc.  If 64 is added we write no header */

	GMT_LONG first_col, last_col;	/* First and last column to deal with */
	GMT_LONG first_row, last_row;	/* First and last row to deal with */
	GMT_LONG width_out;			/* Width of row as return (may include padding) */
	GMT_LONG height_out;			/* Number of columns in subregion */
	int inc = 1;			/* Step in array: 1 for ordinary data, 2 for complex (skipping imaginary) */
	int i, j, i2, j2, err;	/* Misc. counters */
	int *k;				/* Array with indices */
	int type;			/* Data type */
	size_t size;			/* Length of data type */
	GMT_LONG width_in;			/* Number of items in one row of the subregion */
	GMT_LONG ij;
	FILE *fp;			/* File pointer to data or pipe */
	BOOLEAN check = FALSE;		/* TRUE if nan-proxies are used to signify NaN (for non-floating point types) */
	BOOLEAN do_header = TRUE;	/* TRUE if we should write the header first */
	void *tmp;			/* Array pointer for writing in rows of data */

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_OUT);
#endif
		fp = GMT_stdout;
	}
	else if ((fp = GMT_fopen (header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);

	type = GMT_grdformats[header->type][1];
	size = GMT_grd_data_size (header->type, &header->nan_value);
	check = !GMT_is_dnan (header->nan_value);

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	width_in = width_out;		/* Physical width of input array */
	if (pad[0] > 0) width_in += pad[0];
	if (pad[1] > 0) width_in += pad[1];
	if (complex >= 64) {	/* Want no header, adjust complex */
		complex %= 64;
		do_header = FALSE;
	}
	if (complex) inc = 2;

	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	/* Find z_min/z_max */

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = first_row, j2 = pad[3]; j <= last_row; j++, j2++) {
		for (i = first_col, i2 = pad[0]; i <= last_col; i++, i2++) {
			ij = (j2 * width_in + i2) * inc;
			if (GMT_is_fnan (grid[ij])) {
				if (check) grid[ij] = (float)header->nan_value;
			}
			else {
				header->z_min = MIN (header->z_min, (double)grid[ij]);
				header->z_max = MAX (header->z_max, (double)grid[ij]);
			}
		}
	}

	/* Round off to chosen type */

	if (type != 'f' && type != 'd') {
		header->z_min = irint (header->z_min);
		header->z_max = irint (header->z_max);
	}

	/* Store header information and array */

	if (do_header) {
		GMT_err_trap (GMT_native_write_grd_header (fp, header));
	}

	/* Allocate memory for one row of data (for writing purposes) */

	tmp = (void *) GMT_memory (VNULL, (size_t)header->nx, size, "GMT_native_write_grd");

	i2 = first_col + pad[0];
	for (j = 0, j2 = first_row + pad[3]; j < height_out; j++, j2++) {
		ij = j2 * width_in + i2;
		for (i = 0; i < width_out; i++) GMT_encode (tmp, i, grid[inc*(ij+k[i])], type);
		if (GMT_fwrite ((void *)tmp, (size_t)size, (size_t)header->nx, fp) < (size_t)header->nx) return (GMT_GRDIO_WRITE_FAILED);
	}

	GMT_free ((void *)k);
	GMT_free ((void *)tmp);

	if (fp != GMT_stdout) GMT_fclose (fp);

	return (GMT_NOERROR);
}

void GMT_encode (void *vptr, int k, float z, int type)
{

	switch (type) {
		case 'b':
			((char *)vptr)[k] = (char)irint (z);
			break;
		case 's':
			((short int *)vptr)[k] = (short int)irint (z);
			break;
		case 'i':
		case 'm':
			((int *)vptr)[k] = (int)irint (z);
			break;
		case 'f':
			((float *)vptr)[k] = z;
			break;
		case 'd':
			((double *)vptr)[k] = (double)z;
			break;
		default:
			fprintf (stderr, "GMT: Bad call to GMT_encode (gmt_customio.c)\n");
			break;
	}
}


float GMT_decode (void *vptr, int k, int type)
{
	float fval;

	switch (type) {
		case 'b':
			fval = (float)(((char *)vptr)[k]);
			break;
		case 's':
			fval = (float)(((short int *)vptr)[k]);
			break;
		case 'i':
		case 'm':
			fval = (float)(((int *)vptr)[k]);
			break;
		case 'f':
			fval = ((float *)vptr)[k];
			break;
		case 'd':
			fval = (float)(((double *)vptr)[k]);
			break;
		default:
			fprintf (stderr, "GMT: Bad call to GMT_decode (gmt_customio.c)\n");
			fval = GMT_f_NaN;
			break;
	}

	return (fval);
}

/*-----------------------------------------------------------
 * Format # :	6, 20
 * Type :	Native binary (float) C file
 * Prefix :	GMT_srf_
 * Author :	Joaquim Luis
 * Date :	09-SEP-1999
 * 		27-AUG-2005	Added minimalist support to GS format 7
 * 				Type :	Native binary (double) C file
 * Purpose:	to transform to/from Surfer grid file format
 * Functions :	GMT_srf_read_grd_info, GMT_srf_write_grd_info,
 *		GMT_srf_write_grd_info, GMT_srf_read_grd, GMT_srf_write_grd
 *-----------------------------------------------------------*/
 
struct srf_header6 {	/* Surfer 6 file header structure */
	char id[4];		/* ASCII Binary identifier (DSBB) */
	short int nx;		/* Number of columns */
	short int ny;		/* Number of rows */
	double x_min;		/* Minimum x coordinate */
	double x_max;		/* Maximum x coordinate */
	double y_min;		/* Minimum y coordinate */
	double y_max;		/* Maximum y coordinate */
	double z_min;		/* Minimum z value */
	double z_max;		/* Maximum z value */
};

/* The format 7 is rather more complicated. It has now headers that point to "sections"
   that may either be also headers or have real data. Besides that, is follows also the
   stupidity of storing the grid using doubles (I would bat that there are no more than 0-1
   Surfer users that really need to save their grids in doubles). The following header
   does not strictly follow the GS format description, but its enough for reading grids
   that do not contain break-lines (again my estimate is that it covers (100 - 1e4)% users).

   Note: I had significant troubles to be able to read correctly the Surfer 7 format. 
   In its basic and mostly used form (that is, without break-lines info) what we normally
   call a header, can be described by the srf_header7 structure bellow (but including
   the three commented lines). This would make that the header is composed of 2 char[4] and
   and 5 ints followed by doubles. The problem was that after the ints the doubles were not
   read correctly. It looked like everything was displaced by 4 bytes.
   I than found the note about the GMT 64-bit Modification and tried the same trick.
   While that worked with gcc compiled code, it crashed whith the VC6 compiler. 
   Since of the first 3 variables, only the first is important to find out which Surfer
   grid format we are dealing with, I removed them from the header definition (and jump
   12 bytes before reading the header). As a result the header has now one 4 byte char +
   trhee 4-bytes ints followed by 8-bytes doubles. With this organization the header is
   read correctly by GMT_read_srfheader7. Needless to say that I don't understand why the
   even number of 4-bytes variables before the 8-bytes caused that the doubles we incorrectly read. 

   Joaquim Luis 08-2005. */

struct srf_header7 {	/* Surfer 7 file header structure */
	/*char id[4];		 * ASCII Binary identifier (DSRB) */
	/*int idumb1;		 * Size of Header in bytes (is == 1) */
	/*int idumb2;		 * Version number of the file format. Currently must be set to 1*/
	char id2[4];		/* Tag ID indicating a grid section (GRID) */
	int len_g;		/* Length in bytes of the grid section (72) */
	int ny;			/* Number of rows */
	int nx;			/* Number of columns */
	double x_min;		/* Minimum x coordinate */
	double y_min;		/* Minimum y coordinate */
	double x_inc;		/* Spacing between columns */
	double y_inc;		/* Spacing between rows */
	double z_min;		/* Minimum z value */
	double z_max;		/* Maximum z value */
	double rotation;	/* not currently used */
	double no_value;	/* If GS were cleverer this would be NaN */
	char id3[4];		/* Tag ID idicating a data section (DATA) */
	int len_d;		/* Length in bytes of the DATA section */
};

int GMT_is_srf_grid (struct GRD_HEADER *header)
{
	FILE *fp;
	char id[5];
	if (!strcmp(header->name, "=")) return (GMT_GRDIO_PIPE_CODECHECK);	/* Cannot check on pipes */
	if ((fp = GMT_fopen (header->name, "rb")) == NULL) return (GMT_GRDIO_OPEN_FAILED);
	if (GMT_fread (id, sizeof (char), (size_t)4, fp) < (size_t)4) return (GMT_GRDIO_READ_FAILED);
	GMT_fclose (fp); 
	if (!strncmp (id, "DSBB", (size_t)4))
		header->type = GMT_grd_format_decoder ("sf");
	else if (!strncmp (id, "DSRB", (size_t)4))
		header->type = GMT_grd_format_decoder ("sd");
	else
		return (GMT_GRDIO_BAD_VAL);	/* Neither */
	return (header->type);
}

int GMT_srf_read_grd_info (struct GRD_HEADER *header)
{
	FILE *fp;
	struct srf_header6 h6;
	struct srf_header7 h7;
	int GMT_read_srfheader6 (FILE *fp, struct srf_header6 *h);
	int GMT_read_srfheader7 (FILE *fp, struct srf_header7 *h);
	char id[5];

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_IN);
#endif
		fp = GMT_stdin;
	}
	else if ((fp = GMT_fopen (header->name, "rb")) == NULL)
		return (GMT_GRDIO_OPEN_FAILED);

	if (GMT_fread (id, sizeof (char), (size_t)4, fp) < (size_t)4) return (GMT_GRDIO_READ_FAILED); 
	if (GMT_fseek(fp, 0L, SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);
	if (strncmp (id, "DSBB", (size_t)4) && strncmp (id, "DSRB", (size_t)4)) return (GMT_GRDIO_NOT_SURFER);

	if (!strncmp (id, "DSBB", (size_t)4)) {		/* Version 6 format */
		if (GMT_read_srfheader6 (fp, &h6)) return (GMT_GRDIO_READ_FAILED);
		header->type = 6;
	}
	else {					/* Version 7 format */
		if (GMT_read_srfheader7 (fp, &h7))  return (GMT_GRDIO_READ_FAILED);
		if ( (h7.len_d != (h7.nx * h7.ny * 8)) || (!strcmp (h7.id2, "GRID")) ) return (GMT_GRDIO_SURF7_UNSUPPORTED);
		header->type = 20;
	}

	if (fp != GMT_stdin) GMT_fclose (fp);

	header->node_offset = 0;	/* Grid node registration */
	if (header->type == 6) {
		strcpy (header->title, "Grid originally in Surfer 6 format");
		header->nx = (int)h6.nx;	header->ny = (int)h6.ny;
		header->x_min = h6.x_min;	header->x_max = h6.x_max;
		header->y_min = h6.y_min;	header->y_max = h6.y_max;
		header->z_min = h6.z_min;	header->z_max = h6.z_max;
		header->x_inc = GMT_get_inc (h6.x_min, h6.x_max, h6.nx, header->node_offset);
		header->y_inc = GMT_get_inc (h6.y_min, h6.y_max, h6.ny, header->node_offset);
	}
	else {			/* Format 7 */
		strcpy (header->title, "Grid originally in Surfer 7 format");
		header->nx = h7.nx;		header->ny = h7.ny;
		header->x_min = h7.x_min;	header->y_min = h7.y_min;
		header->x_max = h7.x_min + h7.x_inc * (h7.nx - 1);
		header->y_max = h7.y_min + h7.y_inc * (h7.ny - 1);
		header->z_min = h7.z_min;	header->z_max = h7.z_max;
		header->x_inc = h7.x_inc;	header->y_inc = h7.y_inc;
	}
	header->z_scale_factor = 1;	header->z_add_offset = 0;

	return (GMT_NOERROR);
}

int GMT_srf_write_grd_info (struct GRD_HEADER *header)
{
	FILE *fp;
	struct srf_header6 h;
	int GMT_write_srfheader (FILE *fp, struct srf_header6 *h);

	if (!strcmp (header->name, "="))
	{
#ifdef SET_IO_MODE
	GMT_setmode (GMT_OUT);
#endif
		fp = GMT_stdout;
	}
	else if ((fp = GMT_fopen (header->name, "rb+")) == NULL && (fp = GMT_fopen (header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);

	strcpy (h.id,"DSBB");
	h.nx = (short int)header->nx;	 h.ny = (short int)header->ny;
	if (header->node_offset) {
		h.x_min = header->x_min + header->x_inc/2;	 h.x_max = header->x_max - header->x_inc/2;
		h.y_min = header->y_min + header->y_inc/2;	 h.y_max = header->y_max - header->y_inc/2;
	}
	else {
		h.x_min = header->x_min;	 h.x_max = header->x_max;
		h.y_min = header->y_min;	 h.y_max = header->y_max;
	}
	h.z_min = header->z_min;	 h.z_max = header->z_max;

	if (GMT_write_srfheader (fp, &h)) return (GMT_GRDIO_WRITE_FAILED);

	if (fp != GMT_stdout) GMT_fclose (fp);

	return (GMT_NOERROR);
}

int GMT_read_srfheader6 (FILE *fp, struct srf_header6 *h)
{
	/* Reads the header of a Surfer 6 gridfile */

	if (GMT_fread ((void *)h, sizeof (struct srf_header6), (size_t)1, fp) < (size_t)1) return (GMT_GRDIO_READ_FAILED); 
	return (GMT_NOERROR);
}

int GMT_read_srfheader7 (FILE *fp, struct srf_header7 *h)
{
	/* Reads the header of a Surfer 7 gridfile */

	if (GMT_fseek (fp, 3*sizeof(int), SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);	/* skip the first 12 bytes */
	if (GMT_fread ((void *)h, sizeof (struct srf_header7), (size_t)1, fp) < (size_t)1) return (GMT_GRDIO_READ_FAILED);
	return (GMT_NOERROR);
}

int GMT_write_srfheader (FILE *fp, struct srf_header6 *h)
{
	if (GMT_fwrite ((void *)h, sizeof (struct srf_header6), (size_t)1, fp) < (size_t)1) return (GMT_GRDIO_WRITE_FAILED); 
	return (GMT_NOERROR);
}

int GMT_srf_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:     	grid structure header */
	/* grid:	array with final grid */
	/* w,e,s,n:	Sub-region to extract  [Use entire file if 0,0,0,0] */
	/* padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively */
	/* complex:	TRUE if array is to hold real and imaginary parts (read in real only) */
	/*		Note: The file has only real values, we simply allow space in the array */
	/*		for imaginary parts when processed by grdfft etc. */

	GMT_LONG first_col, last_col;	/* First and last column to deal with */
	GMT_LONG first_row, last_row;	/* First and last row to deal with */
	GMT_LONG width_in;			/* Number of items in one row of the subregion */
	GMT_LONG height_in;			/* Number of columns in subregion */
	int inc = 1;			/* Step in array: 1 for ordinary data, 2 for complex (skipping imaginary) */
	int i, j, j2, i_0_out; 	/* Misc. counters */
	int *k;				/* Array with indices */
	int type;			/* Data type */
	size_t size;			/* Length of data type */
	GMT_LONG kk, ij;
	GMT_LONG width_out;			/* Width of row as return (may include padding) */
	FILE *fp;			/* File pointer to data or pipe */
	BOOLEAN piping = FALSE;		/* TRUE if we read input pipe instead of from file */
	void *tmp;			/* Array pointer for reading in rows of data */
	header->nan_value = 0.1701410e39;	/* Test value in Surfer grids */

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_IN);
#endif
		fp = GMT_stdin;
		piping = TRUE;
	}
	else if ((fp = GMT_fopen (header->name, "rb")) != NULL) {	/* Skip header */
		if (header->type == 6) {	/* Version 6 */
			if (GMT_fseek (fp, (long) sizeof (struct srf_header6), SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);
		}
		else {			/* Version 7  (skip also the first 12 bytes) */
			if (GMT_fseek (fp, (long) (3*sizeof(int) + sizeof (struct srf_header7)), SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);
		}
	}
	else
		return (GMT_GRDIO_OPEN_FAILED);

	type = GMT_grdformats[header->type][1];
	size = GMT_grd_data_size (header->type, &header->nan_value);

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row, &k), header->name);;

	width_out = width_in;		/* Width of output array */
	if (pad[0] > 0) width_out += pad[0];
	if (pad[1] > 0) width_out += pad[1];

	i_0_out = pad[0];		/* Edge offset in output */

	if (complex) {	/* Need twice as much output space since we load every 2nd cell */
		width_out *= 2;
		i_0_out *= 2;
		inc = 2;
	}
	if ( (last_row - first_row + 1) != header->ny) {    /* We have a sub-region */
		/* Surfer grids are stored starting from Lower Left, which is contrary to
		   the rest of GMT grids that start at Top Left. So we must do a shift here */
		first_row = header->ny - height_in - first_row;
		last_row = first_row + height_in - 1;
	}

	/* Allocate memory for one row of data (for reading purposes) */

	tmp = (void *) GMT_memory (VNULL, (size_t)header->nx, size, "GMT_srf_read_grd");

	/* Now deal with skipping */

	if (piping) {	/* Skip data by reading it */
		for (j = 0; j < first_row; j++) if (GMT_fread (tmp, size, (size_t)header->nx, fp) < (size_t)header->nx) return (GMT_GRDIO_READ_FAILED);
	}
	else {		/* Simply seek over it */
		if (GMT_fseek (fp, (long) (first_row * header->nx * size), SEEK_CUR)) return (GMT_GRDIO_SEEK_FAILED);
	}

	for (j = first_row, j2 = height_in-1; j <= last_row; j++, j2--) {
		if (GMT_fread (tmp, size, (size_t)header->nx, fp) < (size_t)header->nx) return (GMT_GRDIO_READ_FAILED);	/* Get one row */
		ij = (j2 + pad[3]) * width_out + i_0_out;
		for (i = 0; i < width_in; i++) {
			kk = ij + inc * i;
			grid[kk] = GMT_decode (tmp, k[i], type);	/* Convert whatever to float */
			if (grid[kk] >= header->nan_value) grid[kk] = GMT_f_NaN;
		}
	}
	if (piping) {	/* Skip remaining data by reading it */
		for (j = last_row + 1; j < header->ny; j++) if (GMT_fread (tmp, size, (size_t)header->nx, fp) < (size_t)header->nx) return (GMT_GRDIO_READ_FAILED);
	}

	header->nx = width_in;
	header->ny = height_in;
	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	/* Update z_min, z_max */

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = 0; j < header->ny; j++) {
		for (i = 0; i < header->nx; i++) {
			ij = (j + pad[3]) * width_out + i + pad[0];
			if (GMT_is_fnan (grid[ij])) continue;
			header->z_min = MIN (header->z_min, (double)grid[ij]);
			header->z_max = MAX (header->z_max, (double)grid[ij]);
		}
	}

	if (fp != GMT_stdin) GMT_fclose (fp);

	GMT_free ((void *)k);
	GMT_free (tmp);

	return (GMT_NOERROR);
}

int GMT_srf_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:	grid structure header */
	/* grid:	array with final grid */
	/* w,e,s,n:	Sub-region to write out  [Use entire file if 0,0,0,0] */
	/* padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively */
	/* complex:	TRUE if array is to hold real and imaginary parts (read in real only) */
	/*		Note: The file has only real values, we simply allow space in the array */
	/*		for imaginary parts when processed by grdfft etc. */

	GMT_LONG first_col, last_col;	/* First and last column to deal with */
	GMT_LONG first_row, last_row;	/* First and last row to deal with */
	GMT_LONG width_out;			/* Width of row as return (may include padding) */
	GMT_LONG height_out;			/* Number of columns in subregion */
	int inc = 1;			/* Step in array: 1 for ordinary data, 2 for complex (skipping imaginary) */
	int i, j, i2, j2;		/* Misc. counters */
	int *k;				/* Array with indices */
	int type;			/* Data type */
	size_t size;			/* Length of data type */
	GMT_LONG ij;
	GMT_LONG width_in;			/* Number of items in one row of the subregion */
	FILE *fp;			/* File pointer to data or pipe */
	void *tmp;			/* Array pointer for writing in rows of data */
	struct srf_header6 h;

	header->nan_value = 0.1701410e39;	/* Test value in Surfer grids */

	if (!strcmp (header->name, "=")) {
#ifdef SET_IO_MODE
		GMT_setmode (GMT_OUT);
#endif
		fp = GMT_stdout;
	}
	else if ((fp = GMT_fopen (header->name, "wb")) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);

	type = GMT_grdformats[header->type][1];
	size = GMT_grd_data_size (header->type, &header->nan_value);

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	width_in = width_out;		/* Physical width of input array */
	if (pad[0] > 0) width_in += pad[0];
	if (pad[1] > 0) width_in += pad[1];
	complex %= 64;
	if (complex) inc = 2;

	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	/* Find z_min/z_max */

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = first_row, j2 = pad[3]; j <= last_row; j++, j2++) {
		for (i = first_col, i2 = pad[0]; i <= last_col; i++, i2++) {
			ij = (j2 * width_in + i2);
			if (GMT_is_fnan (grid[ij])) 
				grid[ij] = (float)header->nan_value;
			else {
				header->z_min = MIN (header->z_min, (double)grid[ij]);
				header->z_max = MAX (header->z_max, (double)grid[ij]);
			}
		}
	}

	/* store header information and array */

	strcpy (h.id,"DSBB");
	h.nx = (short int)header->nx;	 h.ny = (short int)header->ny;
	if (header->node_offset) {
		h.x_min = header->x_min + header->x_inc/2;	 h.x_max = header->x_max - header->x_inc/2;
		h.y_min = header->y_min + header->y_inc/2;	 h.y_max = header->y_max - header->y_inc/2;
	}
	else {
		h.x_min = header->x_min;	 h.x_max = header->x_max;
		h.y_min = header->y_min;	 h.y_max = header->y_max;
	}
	h.z_min = header->z_min;	 h.z_max = header->z_max;

	if (GMT_fwrite ((void *)&h, sizeof (struct srf_header6), (size_t)1, fp) != 1) return (GMT_GRDIO_WRITE_FAILED);

	/* Allocate memory for one row of data (for writing purposes) */

	tmp = (void *) GMT_memory (VNULL, (size_t)header->nx, size, "GMT_srf_write_grd");

	i2 = first_col + pad[0];
	for (j = 0, j2 = last_row + pad[3]; j < height_out; j++, j2--) {
		ij = j2 * width_in + i2;
		for (i = 0; i < width_out; i++) GMT_encode (tmp, i, grid[inc*(ij+k[i])], type);
		if (GMT_fwrite ((void *)tmp, (size_t)size, (size_t)header->nx, fp) < (size_t)header->nx) return (GMT_GRDIO_WRITE_FAILED);
	}

	GMT_free ((void *)k);
	GMT_free ((void *)tmp);

	if (fp != GMT_stdout) GMT_fclose (fp);

	return (GMT_NOERROR);
}

/* Add custom code here */

/* 12: NOAA NGDC MGG Format */
#include "gmt_mgg_header2.c"

/* 21: Atlantic Geoscience Center format */
#include "gmt_agc_io.c"
