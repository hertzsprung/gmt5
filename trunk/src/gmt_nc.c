/*--------------------------------------------------------------------
 *	$Id: gmt_nc.c,v 1.60 2007-02-24 00:10:05 remko Exp $
 *
 *	Copyright (c) 1991-2007 by P. Wessel and W. H. F. Smith
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
 *
 *	G M T _ N C . C   R O U T I N E S
 *
 * Takes care of all grd input/output built on NCAR's NetCDF routines.
 * This version is intended to provide more general support for reading
 * NetCDF files that were not generated by GMT. At the same time, the grids
 * written by these routines are intended to be more conform COARDS conventions.
 * These routines are to eventually replace the older gmt_cdf_ routines.
 *
 * Most functions will exit with error message if an internal error is returned.
 * There functions are only called indirectly via the GMT_* grdio functions.
 *
 * Author:	Remko Scharroo
 * Date:	04-AUG-2005
 * Version:	1
 *
 * Functions include:
 *
 *	GMT_nc_read_grd_info :		Read header from file
 *	GMT_nc_read_grd :		Read data set from file
 *	GMT_nc_update_grd_info :	Update header in existing file
 *	GMT_nc_write_grd_info :		Write header to new file
 *	GMT_nc_write_grd :		Write header and data set to new file
 *
 *	void check_nc_status (int status)	Used for misc checks
 *	void nc_nopipe (char *file)		---"---
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define GMT_WITH_NO_PS
#define GMT_CDF_CONVENTION    "COARDS"	/* grd files are COARDS-compliant */
#include "gmt.h"

EXTERN_MSC int GMT_cdf_grd_info (int ncid, struct GRD_HEADER *header, char job);
char nc_file[BUFSIZ];
int GMT_nc_grd_info (struct GRD_HEADER *header, char job);
int GMT_nc_get_att_text (int ncid, int varid, char *name, char *text, size_t textlen);
void GMT_nc_get_units (int ncid, int varid, char *name_units);
void GMT_nc_put_units (int ncid, int varid, char *name_units);
void GMT_nc_check_step (int n, double *x, char *varname);

int GMT_is_nc_grid (char *file)
{	/* Returns type 18 (=nf) for new NetCDF grid,
	   type 10 (=cf) for old NetCDF grids and -1 upon error */
	int ncid, z_id = -1, i = 0, j = 0, id = 13, nvars, ndims;
	nc_type z_type;
	char filename[GMT_LONG_TEXT];

	/* Extract variable name from filename */
	strcpy (filename, file);
	while (filename[i] && filename[i] != '?') i++;
	if (filename[i]) {
		filename[i] = '\0';
		j = i + 1;
		while (filename[j] && filename[j] != '[' && filename[j] != '(') j++;
		if (filename[j]) filename[j] = '\0';
	}
	else
		i = -1;
	nc_nopipe (filename);

	/* Open the file and look for the required variable */
	if (nc_open (filename, NC_NOWRITE, &ncid)) return (-1);
	if (!nc_inq_dimid (ncid, "xysize", &z_id)) {
		id = 5;
		nc_inq_varid (ncid, "z", &z_id);
	}
	else if (i < 0) {
		nc_inq_nvars (ncid, &nvars);
		while (j < nvars && z_id < 0) {
			check_nc_status (nc_inq_varndims (ncid, j, &ndims));
			if (ndims == 2) z_id = j;
			j++;
		}
	}
	else
		nc_inq_varid (ncid, &filename[i+1], &z_id);

	/* When variable can not be found */
	if (z_id < 0) {
		if (i < 0)
			fprintf (stderr, "%s: no 2-D variable in file [%s]\n", GMT_program, filename);
		else
			fprintf (stderr, "%s: named variable (%s) does not exist in file [%s]\n", GMT_program, &filename[i+1], filename);
		exit (EXIT_FAILURE);
	}
	check_nc_status (nc_inq_vartype (ncid, z_id, &z_type));
	id += ((z_type == NC_BYTE) ? 2 : z_type);
	nc_close (ncid);
	return (id);
}

int GMT_nc_read_grd_info (struct GRD_HEADER *header)
{
	GMT_nc_grd_info (header, 'r');
	return (0);
}

int GMT_nc_update_grd_info (struct GRD_HEADER *header)
{
	GMT_nc_grd_info (header, 'u');
	return (0);
}

int GMT_nc_write_grd_info (struct GRD_HEADER *header)
{
	GMT_nc_grd_info (header, 'w');
	return (0);
}

int GMT_nc_grd_info (struct GRD_HEADER *header, char job)
{
	int i, j;
	double dummy[2], *xy = VNULL;
	char varname[GRD_UNIT_LEN];
	nc_type z_type;
	double t_value[3];

	/* Dimension ids, variable ids, etc.. */
	int ncid, z_id = -1, ids[5] = {-1,-1,-1,-1,-1}, dims[5], nvars, ndims;
	size_t lens[5], item[2];

	for (i = 0; i < 3; i++) header->t_index[i] = -1, t_value[i] = GMT_d_NaN;

	/* If the file name contains a ?, extract the name of the variable following it */

	i = 0;
	varname[0] = '\0';
	while (header->name[i] && header->name[i] != '?') i++;
	if (header->name[i]) {
		strcpy (varname, &header->name[i+1]);
		header->name[i] = '\0';
		i = 0;
		while (varname[i] && varname[i] != '(' && varname[i] != '[') i++;
		if (varname[i] == '(') {
			sscanf (&varname[i+1], "%lf,%lf,%lf)", &t_value[0], &t_value[1], &t_value[2]);
			varname[i] = '\0';
		}
		else if (varname[i] == '[') {
			sscanf (&varname[i+1], "%d,%d,%d]", &header->t_index[0], &header->t_index[1], &header->t_index[2]);
			varname[i] = '\0';
		}
	}

	/* Open NetCDF file */

	nc_nopipe (header->name);
	switch (job) {
		case 'r':
			check_nc_status (nc_open (header->name, NC_NOWRITE, &ncid)); break;
		case 'u':
			check_nc_status (nc_open (header->name, NC_WRITE + NC_NOFILL, &ncid)); break;
		default:
			check_nc_status (nc_create (header->name, NC_CLOBBER + NC_NOFILL, &ncid)); break;
	}

	/* Retrieve or define dimensions and variables */

	if (job == 'r' || job == 'u') {
		/* First see if this is an old NetCDF formatted file */
		if (!nc_inq_dimid (ncid, "xysize", &i)) return (GMT_cdf_grd_info (ncid, header, job));

		/* Find first 2-dimensional (z) variable or specified variable */
		if (!varname[0]) {
			check_nc_status (nc_inq_nvars (ncid, &nvars));
			i = 0;
			while (i < nvars && z_id < 0) {
				check_nc_status (nc_inq_varndims (ncid, i, &ndims));
				if (ndims == 2) z_id = i;
				i++;
			}
		}
		else if (nc_inq_varid (ncid, varname, &z_id) == NC_NOERR) {
			check_nc_status (nc_inq_varndims (ncid, z_id, &ndims));
			if (ndims < 2 || ndims > 5) {
				fprintf (stderr, "%s: named variable (%s) is %d-D, not 2-, 3-, 4- or 5-D [%s]\n", GMT_program, varname, ndims, header->name);
				exit (EXIT_FAILURE);
			}
		}
		else {
			fprintf (stderr, "%s: named variable (%s) does not exist in file [%s]\n", GMT_program, varname, header->name);
			exit (EXIT_FAILURE);
		}
		if (z_id < 0) {
			fprintf (stderr, "%s: no 2-D variable in file [%s]\n", GMT_program, header->name);
			exit (EXIT_FAILURE);
		}

		/* Get the z data type and determine its dimensions */
		check_nc_status (nc_inq_vartype (ncid, z_id, &z_type));
		check_nc_status (nc_inq_vardimid (ncid, z_id, dims));
		header->type = ((z_type == NC_BYTE) ? 2 : z_type) + 13;

		/* Get the ids of the x and y (and depth and time) variables */
		for (i = 0; i < ndims; i++) {
			check_nc_status (nc_inq_dim (ncid, dims[i], varname, &lens[i]));
			if (nc_inq_varid (ncid, varname, &ids[i])) ids[i] = -1;
		}
		header->nx = (int) lens[ndims-1];
		header->ny = (int) lens[ndims-2];
	}
	else {
		/* Define dimensions of z variable */
#define NC_FLOAT_OR_DOUBLE(xmin, xmax, dx) ((dx) < GMT_SMALL * MAX(fabs(xmin),fabs(xmax)) ? NC_DOUBLE : NC_FLOAT)
		ndims = 2;
		check_nc_status (nc_def_dim (ncid, "x", (size_t) header->nx, &dims[1]));
		check_nc_status (nc_def_var (ncid, "x", NC_FLOAT_OR_DOUBLE(header->x_min, header->x_max, header->x_inc), 1, &dims[1], &ids[1]));
		check_nc_status (nc_def_dim (ncid, "y", (size_t) header->ny, &dims[0]));
		check_nc_status (nc_def_var (ncid, "y", NC_FLOAT_OR_DOUBLE(header->y_min, header->y_max, header->y_inc), 1, &dims[0], &ids[0]));

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

		check_nc_status (nc_def_var (ncid, "z", z_type, 2, dims, &z_id));
	}
	header->z_id = z_id;
	header->ncid = ncid;

	/* Query or assign attributes */

	if (job == 'u') check_nc_status (nc_redef (ncid));

	if (job == 'r') {
		/* Get global information */
		if (GMT_nc_get_att_text (ncid, NC_GLOBAL, "title", header->title, GRD_TITLE_LEN))
		    GMT_nc_get_att_text (ncid, z_id, "long_name", header->title, GRD_TITLE_LEN);
		if (GMT_nc_get_att_text (ncid, NC_GLOBAL, "history", header->command, GRD_COMMAND_LEN))
		    GMT_nc_get_att_text (ncid, NC_GLOBAL, "source", header->command, GRD_COMMAND_LEN);
		GMT_nc_get_att_text (ncid, NC_GLOBAL, "description", header->remark, GRD_REMARK_LEN);
		nc_get_att_int (ncid, NC_GLOBAL, "node_offset", &header->node_offset);

		/* Create enough memory to store the x- and y-coordinate values */
		xy = (double *)GMT_memory (VNULL, (size_t)(MAX(header->nx,header->ny)), sizeof(double), GMT_program);

		/* Get information about x variable */
		GMT_nc_get_units (ncid, ids[ndims-1], header->x_units);
		if (!(j = nc_get_var_double (ncid, ids[ndims-1], xy))) GMT_nc_check_step (header->nx, xy, header->x_units);
		if (!nc_get_att_double (ncid, ids[ndims-1], "actual_range", dummy))
			header->x_min = dummy[0], header->x_max = dummy[1];
		else if (!j) {
			header->x_min = xy[0], header->x_max = xy[header->nx-1];
			header->node_offset = 0;
		}
		else {
			header->x_min = 0.0, header->x_max = (double) header->nx-1;
			header->node_offset = 0;
		}
		header->x_inc = GMT_get_inc (header->x_min, header->x_max, header->nx, header->node_offset);
		if (GMT_is_dnan(header->x_inc)) header->x_inc = 1.0;

		/* Get information about y variable */
		GMT_nc_get_units (ncid, ids[ndims-2], header->y_units);
		if (!(j = nc_get_var_double (ncid, ids[ndims-2], xy))) GMT_nc_check_step (header->ny, xy, header->y_units);
		if (!nc_get_att_double (ncid, ids[ndims-2], "actual_range", dummy))
			header->y_min = dummy[0], header->y_max = dummy[1];
		else if (!j) {
			header->y_min = xy[0], header->y_max = xy[header->ny-1];
			header->node_offset = 0;
		}
		else {
			header->y_min = 0.0, header->y_max = (double) header->ny-1;
			header->node_offset = 0;
		}
		/* Check for reverse order of y-coordinate */
		if (header->y_min > header->y_max) {
			header->y_order = -1;
			dummy[0] = header->y_max, dummy[1] = header->y_min;
			header->y_min = dummy[0], header->y_max = dummy[1];
		}
		else
			header->y_order = 1;
		header->y_inc = GMT_get_inc (header->y_min, header->y_max, header->ny, header->node_offset);
		if (GMT_is_dnan(header->y_inc)) header->y_inc = 1.0;

		GMT_free ((void *)xy);

		/* Get information about z variable */
		GMT_nc_get_units (ncid, z_id, header->z_units);
		if (nc_get_att_double (ncid, z_id, "scale_factor", &header->z_scale_factor)) header->z_scale_factor = 1.0;
		if (nc_get_att_double (ncid, z_id, "add_offset", &header->z_add_offset)) header->z_add_offset = 0.0;
		if (nc_get_att_double (ncid, z_id, "_FillValue", &header->nan_value))
		    nc_get_att_double (ncid, z_id, "missing_value", &header->nan_value);
		if (nc_get_att_double (ncid, z_id, "actual_range", dummy) &&
		    nc_get_att_double (ncid, z_id, "valid_range", dummy)) dummy[0] = 0, dummy [1] = 0;
		header->z_min = dummy[0], header->z_max = dummy[1];

		/* Get grid buffer */
		item[0] = 0;
		for (i = 0; i < ndims-2; i++) {
			if (header->t_index[i] > -1) { /* Do nothing */ }
			else if (GMT_is_dnan(t_value[i])) { header->t_index[i] = 0; }
			else {
				item[1] = lens[i]-1;
				if (nc_get_att_double (ncid, ids[i], "actual_range", dummy)) {
					check_nc_status (nc_get_var1_double (ncid, ids[i], &item[0], &dummy[0]));
					check_nc_status (nc_get_var1_double (ncid, ids[i], &item[1], &dummy[1]));
				}
				header->t_index[i] = irint((t_value[i] - dummy[0]) / (dummy[1] - dummy[0]) * item[1]);
			}
		}
	}
	else {
		/* Store global attributes */
		check_nc_status (nc_put_att_text (ncid, NC_GLOBAL, "Conventions", strlen (GMT_CDF_CONVENTION) + 1, (const char *) GMT_CDF_CONVENTION));
		if (header->title[0]) check_nc_status (nc_put_att_text (ncid, NC_GLOBAL, "title", GRD_TITLE_LEN, header->title));
		if (header->command[0]) check_nc_status (nc_put_att_text (ncid, NC_GLOBAL, "history", GRD_COMMAND_LEN, header->command));
		if (header->remark[0]) check_nc_status (nc_put_att_text (ncid, NC_GLOBAL, "description", GRD_REMARK_LEN, header->remark));
		check_nc_status (nc_put_att_int (ncid, NC_GLOBAL, "node_offset", NC_LONG, 1, &header->node_offset));

		/* Define x variable */
		GMT_nc_put_units (ncid, ids[ndims-1], header->x_units);
		dummy[0] = header->x_min, dummy[1] = header->x_max;
		check_nc_status (nc_put_att_double (ncid, ids[ndims-1], "actual_range", NC_DOUBLE, 2, dummy));

		/* Define y variable */
		GMT_nc_put_units (ncid, ids[ndims-2], header->y_units);
		header->y_order = 1;
		dummy[(1-header->y_order)/2] = header->y_min, dummy[(1+header->y_order)/2] = header->y_max;
		check_nc_status (nc_put_att_double (ncid, ids[ndims-2], "actual_range", NC_DOUBLE, 2, dummy));

		/* Define z variable */
		GMT_nc_put_units (ncid, z_id, header->z_units);
		if (header->z_scale_factor != 1.0) check_nc_status (nc_put_att_double (ncid, z_id, "scale_factor", NC_DOUBLE, 1, &header->z_scale_factor));
		if (header->z_add_offset != 0.0) check_nc_status (nc_put_att_double (ncid, z_id, "add_offset", NC_DOUBLE, 1, &header->z_add_offset));
		if (z_type == NC_FLOAT || z_type == NC_DOUBLE)
			check_nc_status (nc_put_att_double (ncid, z_id, "_FillValue", z_type, 1, &header->nan_value));
		else {
			i = irint (header->nan_value);
			check_nc_status (nc_put_att_int (ncid, z_id, "_FillValue", z_type, 1, &i));
		}
		if (header->z_min <= header->z_max)
			dummy[0] = header->z_min, dummy[1] = header->z_max;
		else
			dummy[0] = 0.0, dummy[1] = 0.0;
		check_nc_status (nc_put_att_double (ncid, z_id, "actual_range", z_type, 2, dummy));

		/* Store values along x and y axes */
		check_nc_status (nc_enddef (ncid));
		xy = (double *) GMT_memory (VNULL, (size_t) MAX (header->nx,header->ny), sizeof (double), "GMT_nc_grd_info");
		for (i = 0; i < header->nx; i++) xy[i] = (double) GMT_i_to_x (i, header->x_min, header->x_max, header->x_inc, 0.5 * header->node_offset, header->nx);
		check_nc_status (nc_put_var_double (ncid, ids[ndims-1], xy));
		if (header->y_order > 0) {
			for (i = 0; i < header->ny; i++) xy[i] = (double) GMT_i_to_x (i, header->y_min, header->y_max, header->y_inc, 0.5 * header->node_offset, header->ny);
		}
		else {
			for (i = 0; i < header->ny; i++) xy[i] = (double) GMT_j_to_y (i, header->y_min, header->y_max, header->y_inc, 0.5 * header->node_offset, header->ny);
		}
		check_nc_status (nc_put_var_double (ncid, ids[ndims-2], xy));
		GMT_free ((void *)xy);
	}

	/* Close NetCDF file, unless job == 'W' */

	if (job != 'W') check_nc_status (nc_close (ncid));
	return (0);
}

int GMT_nc_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:	grid structure header
	 * grid:	array with final grid
	 * w,e,s,n:	Sub-region to extract  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex:	TRUE if array is to hold real and imaginary parts (read in real only)
	 *		Note: The file has only real values, we simply allow space in the array
	 *		for imaginary parts when processed by grdfft etc.
	 *
	 * Reads a subset of a grdfile and optionally pads the array with extra rows and columns
	 * header values for nx and ny are reset to reflect the dimensions of the logical array,
	 * not the physical size (i.e., the padding is not counted in nx and ny)
	 */
	 
	int  ncid;
	size_t start[5] = {0,0,0,0,0}, edge[5] = {1,1,1,1,1};
	int first_col, last_col, first_row, last_row;
	int i, j, width_in, width_out, height_in, i_0_out, inc = 1;
	int *k, ndims;
	size_t ij, kk;	/* To allow 64-bit addressing on 64-bit systems */
	BOOLEAN check;
	float *tmp = VNULL;

	/* Check type: is file in old NetCDF format or not at all? */

	if (GMT_grdformats[header->type][0] == 'c')
		return (GMT_cdf_read_grd (header, grid, w, e, s, n, pad, complex));
	else if (GMT_grdformats[header->type][0] != 'n') {
		fprintf (stderr, "%s: File is not in NetCDF format [%s]\n", GMT_program, header->name);
		exit (EXIT_FAILURE);
	}

	k = GMT_grd_prep_io (header, &w, &e, &s, &n, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row);

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

	nc_nopipe (header->name);
 	check_nc_status (nc_open (header->name, NC_NOWRITE, &ncid));
	check = !GMT_is_dnan (header->nan_value);
	check_nc_status (nc_inq_varndims (ncid, header->z_id, &ndims));

	tmp = (float *) GMT_memory (VNULL, (size_t)header->nx, sizeof (float), "GMT_nc_read_grd");

	/* Load the data row by row. The data in the file is stored either "upside down"
	 * (y_order < 0, the first row is the top row) or "upside up" (y_order > 0, the first
	 * row is the bottom row). GMT will store the data in "upside down" mode. */

	for (i = 0; i < ndims-2; i++) {
		start[i] = header->t_index[i];
	}
	edge[ndims-1] = header->nx;
	if (header->y_order < 0)
		ij = (size_t)pad[3] * (size_t)width_out + (size_t)i_0_out;
	else {		/* Flip around the meaning of first and last row */
		ij = ((size_t)last_row - (size_t)first_row + (size_t)pad[3]) * (size_t)width_out + (size_t)i_0_out;
		j = first_row;
		first_row = header->ny - 1 - last_row;
		last_row = header->ny - 1 - j;
	}
	header->z_min =  DBL_MAX;
	header->z_max = -DBL_MAX;

	for (j = first_row; j <= last_row; j++, ij -= ((size_t)header->y_order * (size_t)width_out)) {
		start[ndims-2] = j;
		check_nc_status (nc_get_vara_float (ncid, header->z_id, start, edge, tmp));	/* Get one row */
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

	check_nc_status (nc_close (ncid));

	GMT_free ((void *)k);
	GMT_free ((void *)tmp);
	return (0);
}

int GMT_nc_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex)
{	/* header:	grid structure header
	 * grid:	array with final grid
	 * w,e,s,n:	Sub-region to write out  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex:	TRUE if array is to hold real and imaginary parts (read in real only)
	 *		Note: The file has only real values, we simply allow space in the array
	 *		for imaginary parts when processed by grdfft etc.
	 */

	size_t start[2] = {0,0}, edge[2] = {1,1};
	int i, inc = 1, *k, nr_oor = 0;
	int j, width_in, width_out, height_out;
	int first_col, last_col, first_row, last_row;
	size_t ij;	/* To allow 64-bit addressing on 64-bit systems */
	float *tmp_f = VNULL;
	int *tmp_i = VNULL;
	double limit[2] = {FLT_MIN, FLT_MAX}, value;
	nc_type z_type;

	/* Determine the value to be assigned to missing data, if not already done so */

	switch (GMT_grdformats[header->type][1]) {
		case 'b':
			if (GMT_is_dnan (header->nan_value)) header->nan_value = CHAR_MIN;
			limit[0] = CHAR_MIN - 0.5, limit[1] = CHAR_MAX + 0.5;
			z_type = NC_BYTE; break;
		case 's':
			if (GMT_is_dnan (header->nan_value)) header->nan_value = SHRT_MIN;
			limit[0] = SHRT_MIN - 0.5, limit[1] = SHRT_MAX + 0.5;
			z_type = NC_SHORT; break;
		case 'i':
			if (GMT_is_dnan (header->nan_value)) header->nan_value = INT_MIN;
			limit[0] = INT_MIN - 0.5, limit[1] = INT_MAX + 0.5;
			z_type = NC_INT; break;
		case 'f':
			z_type = NC_FLOAT; break;
		case 'd':
			z_type = NC_DOUBLE; break;
		default:
			z_type = NC_NAT;
	}

	k = GMT_grd_prep_io (header, &w, &e, &s, &n, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row);

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

	/* Write grid header without closing file afterwards */

	GMT_nc_grd_info (header, 'W');

	/* Set start position for writing grid */

	edge[1] = width_out;
	ij = (size_t)first_col + (size_t)pad[0] + ((size_t)last_row + (size_t)pad[3]) * (size_t)width_in;
	header->z_min =  DBL_MAX;
	header->z_max = -DBL_MAX;

	/* Store z-variable. Distinguish between floats and integers */

	if (z_type == NC_FLOAT || z_type == NC_DOUBLE) {
		tmp_f = (float *) GMT_memory (VNULL, (size_t)width_in, sizeof (float), "GMT_nc_write_grd");
		for (j = 0; j < height_out; j++, ij -= (size_t)width_in) {
			start[0] = j;
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
			check_nc_status (nc_put_vara_float (header->ncid, header->z_id, start, edge, tmp_f));
		}
		GMT_free ((void *)tmp_f);
	}

	else {
		tmp_i = (int *) GMT_memory (VNULL, (size_t)width_in, sizeof (int), "GMT_nc_write_grd");
		for (j = 0; j < height_out; j++, ij -= (size_t)width_in) {
			start[0] = j;
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
			check_nc_status (nc_put_vara_int (header->ncid, header->z_id, start, edge, tmp_i));
		}
		GMT_free ((void *)tmp_i);
	}

	if (nr_oor > 0) fprintf (stderr, "%s: Warning: %d out-of-range grid values converted to _FillValue [%s]\n", GMT_program, nr_oor, header->name);

	GMT_free ((void *)k);

	if (header->z_min <= header->z_max)
		limit[0] = header->z_min, limit[1] = header->z_max;
	else {
		fprintf (stderr, "%s: Warning: No valid values in grid [%s]\n", GMT_program, header->name);
		limit[0] = 0.0, limit[1] = 0.0;
	}
	check_nc_status (nc_put_att_double (header->ncid, header->z_id, "actual_range", z_type, 2, limit));

	/* Close grid */

	check_nc_status (nc_close (header->ncid));

	return (0);
}

void check_nc_status (int status)
{	/* This function checks the return status of a netcdf function and takes
	 * appropriate action if the status != NC_NOERR
	 */
	if (status != NC_NOERR) {
		fprintf (stderr, "%s: %s [%s]\n", GMT_program, nc_strerror (status), nc_file);
		exit (EXIT_FAILURE);
	}
}

void nc_nopipe (char *file)
{	/* This function checks if file was called as a pipe */
	if (!strcmp (file,"=")) {
		fprintf (stderr, "%s: GMT Fatal Error: NetCDF-based I/O does not support piping - Exiting\n", GMT_program);
		exit (EXIT_FAILURE);
	}
	strcpy (nc_file, file);
}

int GMT_nc_get_att_text (int ncid, int varid, char *name, char *text, size_t textlen)
{	/* This function is a replacement for nc_get_att_text that avoids overflow of text
	 * ncid, varid, name, text	: as in nc_get_att_text
	 * textleni			: maximum number of characters to copy to string text
	 */
	int err;
	size_t attlen;
	char *tmp;

	if ((err = nc_inq_attlen (ncid, varid, name, &attlen))) return (err);
	tmp = (char *) GMT_memory (VNULL, attlen, sizeof (char), "GMT_nc_get_att_text");
	nc_get_att_text (ncid, varid, name, tmp);
	strncpy (text, tmp, textlen);
	if (attlen < textlen) text[attlen] = 0;
	GMT_free (tmp);
	return (0);
}

void GMT_nc_get_units (int ncid, int varid, char *name_units)
{	/* Get attributes long_name and units for given variable ID
	 * and assign variable name if attributes are not available.
	 * ncid, varid		: as in nc_get_att_text
	 * nameunit		: long_name and units in form "long_name [units]"
	 */
	char units[GRD_UNIT_LEN];
	if (GMT_nc_get_att_text (ncid, varid, "long_name", name_units, GRD_UNIT_LEN)) nc_inq_varname (ncid, varid, name_units);
	if (!GMT_nc_get_att_text (ncid, varid, "units", units, GRD_UNIT_LEN) && units[0]) sprintf (name_units, "%s [%s]", name_units, units);
}

void GMT_nc_put_units (int ncid, int varid, char *name_units)
{	/* Put attributes long_name and units for given variable ID based on
	 * string name_unit in the form "long_name [units]".
	 * ncid, varid		: as is nc_put_att_text
	 * name_units		: string in form "long_name [units]"
	 */
	int i = 0;
	char name[GRD_UNIT_LEN], units[GRD_UNIT_LEN];

	strcpy (name, name_units);
	units[0] = '\0';
	while (name[i] && name[i] != '[') i++;
	if (name[i]) {
		strcpy (units, &name[i+1]);
		name[i]='\0';
		if (name[i-1] == ' ') name[i-1]='\0';
	}
	i = 0;
	while (units[i] && units[i] != ']') i++;
	if (units[i]) units[i]='\0';
	if (name[0]) check_nc_status (nc_put_att_text (ncid, varid, "long_name", strlen(name), name));
	if (units[0]) check_nc_status (nc_put_att_text (ncid, varid, "units", strlen(units), units));
}

void GMT_nc_check_step (int n, double *x, char *varname)
{	/* Check if all steps in range are the same (within 2%) */
	double step, step_min, step_max;
	int i;
	if (n < 2) return;
	step_min = step_max = x[1]-x[0];
	for (i = 2; i < n; i++) {
		step = x[i]-x[i-1];
		if (step < step_min) step_min = step;
		if (step > step_max) step_max = step;
	}
	if (fabs(step_min-step_max)/(fabs(step_min)+fabs(step_max)) > 0.05) {
		fprintf (stderr, "%s: WARNING: The step size of coordinate (%s) in grid %s is not constant.\n", GMT_program, varname, nc_file);
		fprintf (stderr, "%s: WARNING: GMT will use a constant step size of %g; the original ranges from %g to %g.\n", GMT_program, (x[n-1]-x[0])/(n-1), step_min, step_max);
	}
}
