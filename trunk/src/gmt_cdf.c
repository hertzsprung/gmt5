/*--------------------------------------------------------------------
 *	$Id: gmt_cdf.c,v 1.55 2009-09-09 23:27:01 guru Exp $
 *
 *	Copyright (c) 1991-2009 by P. Wessel and W. H. F. Smith
 *	See LICENSE.TXT file for copying and redistribution conditions.
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
 *
 *	G M T _ C D F . C   R O U T I N E S
 *
 * Takes care of all grd input/output built on NCAR's netCDF routines (which is
 * an XDR implementation)
 * Most functions will exit with error message if an internal error is returned.
 * There functions are only called indirectly via the GMT_* grdio functions.
 *
 * Author:	Paul Wessel
 * Date:	9-SEP-1992
 * Modified:	06-DEC-2001
 * Version:	4
 *
 * Functions include:
 *
 *	GMT_cdf_read_grd_info :		Read header from file
 *	GMT_cdf_read_grd :		Read header and data set from file
 *	GMT_cdf_update_grd_info :	Update header in existing file
 *	GMT_cdf_write_grd_info :	Write header to new file
 *	GMT_cdf_write_grd :		Write header and data set to new file
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define GMT_WITH_NO_PS
#include "gmt.h"

GMT_LONG GMT_cdf_grd_info (int ncid, struct GRD_HEADER *header, char job);

GMT_LONG GMT_cdf_read_grd_info (struct GRD_HEADER *header)
{
	int ncid;
	GMT_LONG err;
	if (!strcmp (header->name,"=")) return (GMT_GRDIO_NC_NO_PIPE);
	GMT_err_trap (nc_open (header->name, NC_NOWRITE, &ncid));
	GMT_err_trap (GMT_cdf_grd_info (ncid, header, 'r'));
	GMT_err_trap (nc_close (ncid));
	return (GMT_NOERROR);
}

GMT_LONG GMT_cdf_update_grd_info (struct GRD_HEADER *header)
{
	int ncid;
	GMT_LONG err;
	if (!strcmp (header->name,"=")) return (GMT_GRDIO_NC_NO_PIPE);
	GMT_err_trap (nc_open (header->name, NC_WRITE + NC_NOFILL, &ncid));
	GMT_err_trap (GMT_cdf_grd_info (ncid, header, 'u'));
	GMT_err_trap (nc_close (ncid));
	return (GMT_NOERROR);
}

GMT_LONG GMT_cdf_write_grd_info (struct GRD_HEADER *header)
{
	int ncid;
	GMT_LONG err;
	if (!strcmp (header->name,"=")) return (GMT_GRDIO_NC_NO_PIPE);
	GMT_err_trap (nc_create (header->name, NC_CLOBBER + NC_NOFILL, &ncid));
	GMT_err_trap (GMT_cdf_grd_info (ncid, header, 'w'));
	GMT_err_trap (nc_close (ncid));
	return (GMT_NOERROR);
}

GMT_LONG GMT_cdf_grd_info (int ncid, struct GRD_HEADER *header, char job)
{
	GMT_LONG err;
	int i, nm[2];
	double dummy[2];
	char text[GRD_COMMAND_LEN+GRD_REMARK_LEN];
	nc_type z_type;

	/* Dimension ids, varibale ids, etc. */
	int side_dim, xysize_dim, x_range_id, y_range_id, z_range_id, inc_id, nm_id, z_id, dims[1];

	/* Define and get dimensions and variables */

	if (job == 'w') {
		GMT_err_trap (nc_def_dim (ncid, "side", (size_t)2, &side_dim));
		GMT_err_trap (nc_def_dim (ncid, "xysize", (size_t) (header->nx * header->ny), &xysize_dim));

		dims[0]	= side_dim;
		GMT_err_trap (nc_def_var (ncid, "x_range", NC_DOUBLE, 1, dims, &x_range_id));
		GMT_err_trap (nc_def_var (ncid, "y_range", NC_DOUBLE, 1, dims, &y_range_id));
		GMT_err_trap (nc_def_var (ncid, "z_range", NC_DOUBLE, 1, dims, &z_range_id));
		GMT_err_trap (nc_def_var (ncid, "spacing", NC_DOUBLE, 1, dims, &inc_id));
		GMT_err_trap (nc_def_var (ncid, "dimension", NC_LONG, 1, dims, &nm_id));

		switch (GMT_grdformats[header->type][1]) {
			case 'b':
				z_type = NC_BYTE; break;
			case 's':
				z_type = NC_SHORT; break;
			case 'i':
				z_type = NC_INT; break;
			case 'f':
				z_type = NC_FLOAT; break;
			case 'd':
				z_type = NC_DOUBLE; break;
			default:
				z_type = NC_NAT;
		}

		dims[0]	= xysize_dim;
		GMT_err_trap (nc_def_var (ncid, "z", z_type, 1, dims, &z_id));
	}
	else {
		GMT_err_trap (nc_inq_varid (ncid, "x_range", &x_range_id));
		GMT_err_trap (nc_inq_varid (ncid, "y_range", &y_range_id));
		GMT_err_trap (nc_inq_varid (ncid, "z_range", &z_range_id));
        	GMT_err_trap (nc_inq_varid (ncid, "spacing", &inc_id));
        	GMT_err_trap (nc_inq_varid (ncid, "dimension", &nm_id));
        	GMT_err_trap (nc_inq_varid (ncid, "z", &z_id));
		GMT_err_trap (nc_inq_vartype (ncid, z_id, &z_type));
		header->type = ((z_type == NC_BYTE) ? 2 : z_type) + 5;
	}
	header->z_id = z_id;

	/* Get or assign attributes */

	memset ((void *)text, 0, (size_t)(GRD_COMMAND_LEN+GRD_REMARK_LEN));

	if (job == 'u') GMT_err_trap (nc_redef (ncid));

	if (job == 'r') {
		GMT_err_trap (nc_get_att_text (ncid, x_range_id, "units", header->x_units));
        	GMT_err_trap (nc_get_att_text (ncid, y_range_id, "units", header->y_units));
		GMT_err_trap (nc_get_att_text (ncid, z_range_id, "units", header->z_units));
        	GMT_err_trap (nc_get_att_double (ncid, z_id, "scale_factor", &header->z_scale_factor));
        	GMT_err_trap (nc_get_att_double (ncid, z_id, "add_offset", &header->z_add_offset));
        	GMT_err_trap (nc_get_att_int (ncid, z_id, "node_offset", &header->node_offset));
        	nc_get_att_double (ncid, z_id, "_FillValue", &header->nan_value);
        	GMT_err_trap (nc_get_att_text (ncid, NC_GLOBAL, "title", header->title));
        	GMT_err_trap (nc_get_att_text (ncid, NC_GLOBAL, "source", text));
		strncpy (header->command, text, (size_t)GRD_COMMAND_LEN);
		strncpy (header->remark, &text[GRD_COMMAND_LEN], (size_t)GRD_REMARK_LEN);

		GMT_err_trap (nc_get_var_double (ncid, x_range_id, dummy));
		header->x_min = dummy[0];
		header->x_max = dummy[1];
		GMT_err_trap (nc_get_var_double (ncid, y_range_id, dummy));
		header->y_min = dummy[0];
		header->y_max = dummy[1];
		GMT_err_trap (nc_get_var_double (ncid, inc_id, dummy));
		header->x_inc = dummy[0];
		header->y_inc = dummy[1];
		GMT_err_trap (nc_get_var_int (ncid, nm_id, nm));
		header->nx = nm[0];
		header->ny = nm[1];
		GMT_err_trap (nc_get_var_double (ncid, z_range_id, dummy));
		header->z_min = dummy[0];
		header->z_max = dummy[1];
		header->y_order = -1;
	}
	else {
		strcpy (text, header->command);
		strcpy (&text[GRD_COMMAND_LEN], header->remark);
		GMT_err_trap (nc_put_att_text (ncid, x_range_id, "units", (size_t)GRD_UNIT_LEN, header->x_units));
		GMT_err_trap (nc_put_att_text (ncid, y_range_id, "units", (size_t)GRD_UNIT_LEN, header->y_units));
		GMT_err_trap (nc_put_att_text (ncid, z_range_id, "units", (size_t)GRD_UNIT_LEN, header->z_units));
		GMT_err_trap (nc_put_att_double (ncid, z_id, "scale_factor", NC_DOUBLE, (size_t)1, &header->z_scale_factor));
		GMT_err_trap (nc_put_att_double (ncid, z_id, "add_offset", NC_DOUBLE, (size_t)1, &header->z_add_offset));
		if (z_type == NC_FLOAT || z_type == NC_DOUBLE) {
			GMT_err_trap (nc_put_att_double (ncid, z_id, "_FillValue", z_type, (size_t)1, &header->nan_value));
		}
		else {
			i = irint (header->nan_value);
			GMT_err_trap (nc_put_att_int (ncid, z_id, "_FillValue", z_type, (size_t)1, &i));
		}
		GMT_err_trap (nc_put_att_int (ncid, z_id, "node_offset", NC_LONG, (size_t)1, &header->node_offset));
		GMT_err_trap (nc_put_att_text (ncid, NC_GLOBAL, "title", (size_t)GRD_TITLE_LEN, header->title));
		GMT_err_trap (nc_put_att_text (ncid, NC_GLOBAL, "source", (size_t)(GRD_COMMAND_LEN+GRD_REMARK_LEN), text));

		GMT_err_trap (nc_enddef (ncid));

		dummy[0] = header->x_min;
		dummy[1] = header->x_max;
		GMT_err_trap (nc_put_var_double (ncid, x_range_id, dummy));
		dummy[0] = header->y_min;
		dummy[1] = header->y_max;
		GMT_err_trap (nc_put_var_double (ncid, y_range_id, dummy));
		dummy[0] = header->x_inc;
		dummy[1] = header->y_inc;
		GMT_err_trap (nc_put_var_double (ncid, inc_id, dummy));
		nm[0] = header->nx;
		nm[1] = header->ny;
		GMT_err_trap (nc_put_var_int (ncid, nm_id, nm));
		if (header->z_min <= header->z_max) {
			dummy[0] = header->z_min; dummy[1] = header->z_max;
		}
		else {
			dummy[0] = 0.0; dummy[1] = 0.0;
		}
		GMT_err_trap (nc_put_var_double (ncid, z_range_id, dummy));
	}
	return (GMT_NOERROR);
}

GMT_LONG GMT_cdf_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, GMT_LONG *pad, BOOLEAN complex)
{	/* header:	grid structure header
	 * grid:	array with final grid
	 * w,e,s,n:	Sub-region to extract  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex:	TRUE if array is to hold real and imaginary parts (read in real only)
	 *		Note: The file has only real values, we simply allow space in the array
	 *		for imaginary parts when processed by grdfft etc.
	 *
	 * Reads a subset of a grid file and optionally pads the array with extra rows and columns
	 * header values for nx and ny are reset to reflect the dimensions of the logical array,
	 * not the physical size (i.e., the padding is not counted in nx and ny)
	 */
	 
	int  ncid;
	GMT_LONG err;
	size_t start[1], edge[1];
	GMT_LONG first_col, last_col, first_row, last_row;
	GMT_LONG i, j, width_in, height_in, i_0_out, inc = 1;
	GMT_LONG ij, kk, width_out;
	GMT_LONG *k;
	BOOLEAN check;
	float *tmp = VNULL;

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	width_out = width_in;		/* Width of output array */
	if (pad[0] > 0) width_out += pad[0];
	if (pad[1] > 0) width_out += pad[1];

	i_0_out = pad[0];		/* Edge offset in output */
	if (complex) {	/* Need twice as much space and load every 2nd cell */
		width_out *= 2;
		i_0_out *= 2;
		inc = 2;
	}

	/* Open the NetCDF file */

	if (!strcmp (header->name,"=")) return (GMT_GRDIO_NC_NO_PIPE);
 	GMT_err_trap (nc_open (header->name, NC_NOWRITE, &ncid));
	check = !GMT_is_dnan (header->nan_value);

	/* Load data row by row. The data in the file is stored in the same
	 * "upside down" fashion as within GMT. The first row is the top row */

	tmp = (float *) GMT_memory (VNULL, (GMT_LONG)header->nx, sizeof (float), "GMT_cdf_read_grd");

	edge[0] = header->nx;
	ij = pad[3] * width_out + i_0_out;
	header->z_min =  DBL_MAX;
	header->z_max = -DBL_MAX;

	for (j = first_row; j <= last_row; j++, ij += width_out) {
		start[0] = j * header->nx;
		GMT_err_trap (nc_get_vara_float (ncid, header->z_id, start, edge, tmp));	/* Get one row */
		for (i = 0; i < width_in; i++) {	/* Check for and handle NaN proxies */
			kk = ij+i*inc;
			grid[kk] = tmp[k[i]];
			if (check && grid[kk] == header->nan_value) grid[kk] = GMT_f_NaN;
			if (GMT_is_fnan (grid[kk])) continue;
			header->z_min = MIN (header->z_min, (double)grid[kk]);
			header->z_max = MAX (header->z_max, (double)grid[kk]);
		}
	}

	header->nx = width_in;
	header->ny = height_in;
	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	GMT_err_trap (nc_close (ncid));

	GMT_free ((void *)k);
	GMT_free ((void *)tmp);
	return (GMT_NOERROR);
}

GMT_LONG GMT_cdf_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, GMT_LONG *pad, BOOLEAN complex)
{	/* header:	grid structure header
	 * grid:	array with final grid
	 * w,e,s,n:	Sub-region to write out  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex:	TRUE if array is to hold real and imaginary parts (read in real only)
	 *		Note: The file has only real values, we simply allow space in the array
	 *		for imaginary parts when processed by grdfft etc.
	 */

	size_t start[1], edge[1];
	int ncid;
	GMT_LONG err;
	GMT_LONG i, inc = 1, nr_oor = 0;
	GMT_LONG j, width_out, height_out;
	GMT_LONG first_col, last_col, first_row, last_row;
	GMT_LONG ij, width_in;
	float *tmp_f = VNULL;
	int *tmp_i = VNULL;
	GMT_LONG *k;
	double limit[2] = {-FLT_MAX, FLT_MAX}, value;
	nc_type z_type;

	/* Determine the value to be assigned to missing data, if not already done so */

	switch (GMT_grdformats[header->type][1]) {
		case 'b':
			if (GMT_is_dnan (header->nan_value)) header->nan_value = CHAR_MIN;
			limit[0] = CHAR_MIN - 0.5; limit[1] = CHAR_MAX + 0.5;
			z_type = NC_BYTE; break;
		case 's':
			if (GMT_is_dnan (header->nan_value)) header->nan_value = SHRT_MIN;
			limit[0] = SHRT_MIN - 0.5; limit[1] = SHRT_MAX + 0.5;
			z_type = NC_SHORT; break;
		case 'i':
			if (GMT_is_dnan (header->nan_value)) header->nan_value = INT_MIN;
			limit[0] = INT_MIN - 0.5; limit[1] = INT_MAX + 0.5;
			z_type = NC_INT; break;
		case 'f':
			z_type = NC_FLOAT; break;
		case 'd':
			z_type = NC_DOUBLE; break;
		default:
			z_type = NC_NAT;
	}

	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	width_in = width_out;		/* Physical width of input array */
	if (pad[0] > 0) width_in += pad[0];
	if (pad[1] > 0) width_in += pad[1];

	complex %= 64;	/* grd Header is always written */
	if (complex) inc = 2;

	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;
	header->nx = width_out;
	header->ny = height_out;

	/* Write grid header */

	if (!strcmp (header->name,"=")) return (GMT_GRDIO_NC_NO_PIPE);
	GMT_err_trap (nc_create (header->name, NC_CLOBBER + NC_NOFILL, &ncid));
	GMT_err_trap (GMT_cdf_grd_info (ncid, header, 'w'));

	/* Set start position for writing grid */

	edge[0] = width_out;
	ij = first_col + pad[0] + (first_row + pad[3]) * width_in;
	header->z_min =  DBL_MAX;
	header->z_max = -DBL_MAX;

	/* Store z-variable */

	if (z_type == NC_FLOAT || z_type == NC_DOUBLE) {
		tmp_f = (float *) GMT_memory (VNULL, width_in, sizeof (float), "GMT_cdf_write_grd");
		for (j = 0; j < height_out; j++, ij += width_in) {
			start[0] = j * width_out;
			for (i = 0; i < width_out; i++) {
				value = grid[inc*(ij+k[i])];
				if (GMT_is_fnan (value))
					tmp_f[i] = (float)header->nan_value;
				else if (fabs(value) > FLT_MAX) {
					tmp_f[i] = (float)header->nan_value;
					nr_oor++;
				}
				else {
					tmp_f[i] = (float)value;
					header->z_min = MIN (header->z_min, (double)tmp_f[i]);
					header->z_max = MAX (header->z_max, (double)tmp_f[i]);
				}
			}
			GMT_err_trap (nc_put_vara_float (ncid, header->z_id, start, edge, tmp_f));
		}
		GMT_free ((void *)tmp_f);
	}

	else {
		tmp_i = (int *) GMT_memory (VNULL, width_in, sizeof (int), "GMT_nc_write_grd");
		for (j = 0; j < height_out; j++, ij += width_in) {
			start[0] = j * width_out;
			for (i = 0; i < width_out; i++) {
				value = grid[inc*(ij+k[i])];
				if (GMT_is_fnan (value))
					tmp_i[i] = irint (header->nan_value);
				else if (value <= limit[0] || value >= limit[1]) {
					tmp_i[i] = irint (header->nan_value);
					nr_oor++;
				}
				else {
					tmp_i[i] = irint (value);
					header->z_min = MIN (header->z_min, (double)tmp_i[i]);
					header->z_max = MAX (header->z_max, (double)tmp_i[i]);
				}
			}
			GMT_err_trap (nc_put_vara_int (ncid, header->z_id, start, edge, tmp_i));
		}
		GMT_free ((void *)tmp_i);
	}

	if (nr_oor > 0) fprintf (stderr, "%s: Warning: %ld out-of-range grid values converted to _FillValue [%s]\n", GMT_program, nr_oor, header->name);

	GMT_free ((void *)k);

	if (header->z_min <= header->z_max) {
		limit[0] = header->z_min; limit[1] = header->z_max;
	}
	else {
		fprintf (stderr, "%s: Warning: No valid values in grid [%s]\n", GMT_program, header->name);
		limit[0] = 0.0; limit[1] = 0.0;
	}
	GMT_err_trap (nc_put_var_double (ncid, header->z_id - 3, limit));

	/* Close grid */

	GMT_err_trap (nc_close (ncid));

	return (GMT_NOERROR);
}
