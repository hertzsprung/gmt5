/*--------------------------------------------------------------------
 *	$Id: gmt_nc.c,v 1.98 2011-04-21 02:31:23 guru Exp $
 *
 *	Copyright (c) 1991-2011 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
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
 *	G M T _ N C . C   R O U T I N E S
 *
 * Takes care of all grd input/output built on NCAR's NetCDF routines.
 * This version is intended to provide more general support for reading
 * NetCDF files that were not generated by GMT. At the same time, the grids
 * written by these routines are intended to be more conform COARDS conventions.
 * These routines are to eventually replace the older gmt_cdf_ routines.
 *
 * Most functions will return with error message if an internal error is returned.
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
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define GMT_WITH_NO_PS
#define GMT_CDF_CONVENTION    "COARDS/CF-1.0"	/* grd files are COARDS-compliant */
#include "gmt.h"
#include "gmt_internals.h"

EXTERN_MSC GMT_LONG GMT_cdf_grd_info (struct GMT_CTRL *C, int ncid, struct GRD_HEADER *header, char job);

GMT_LONG GMT_is_nc_grid (struct GMT_CTRL *C, struct GRD_HEADER *header)
{	/* Returns type 18 (=nf) for new NetCDF grid,
	   type 10 (=cf) for old NetCDF grids and -1 upon error */
	int ncid, z_id = -1, j = 0, id = 13, nvars, ndims, err;
	nc_type z_type;
	char varname[GRD_VARNAME_LEN];

	/* Extract levels name from variable name */
	strcpy (varname, header->varname);
	if (varname[0]) {
		j = 0;
		while (varname[j] && varname[j] != '[' && varname[j] != '(') j++;
		if (varname[j]) varname[j] = '\0';
	}
	if (!strcmp (header->name, "=")) return (GMT_GRDIO_NC_NO_PIPE);

	/* Open the file and look for the required variable */
	if (GMT_access (C, header->name, F_OK)) return (GMT_GRDIO_FILE_NOT_FOUND);
	if (nc_open (header->name, NC_NOWRITE, &ncid)) return (GMT_GRDIO_OPEN_FAILED);
	if (!nc_inq_dimid (ncid, "xysize", &z_id)) {	/* Old style GMT netCDF grid */
		id = 5;
		if (nc_inq_varid (ncid, "z", &z_id)) return (GMT_GRDIO_NO_VAR);
	}
	else if (varname[0]) {	/* ?<varname> used */
		if (nc_inq_varid (ncid, varname, &z_id)) return (GMT_GRDIO_NO_VAR);
	}
	else {			/* Look for first 2D grid */
		nc_inq_nvars (ncid, &nvars);
		for (j = 0; j < nvars && z_id < 0; j++) {
			GMT_err_trap (nc_inq_varndims (ncid, j, &ndims));
			if (ndims == 2) z_id = j;
		}
		if (z_id < 0) return (GMT_GRDIO_NO_2DVAR);
	}

	GMT_err_trap (nc_inq_vartype (ncid, z_id, &z_type));
	id += ((z_type == NC_BYTE) ? 2 : z_type);
	nc_close (ncid);
	header->type = id;
	return (id);
}

void GMT_nc_get_units (struct GMT_CTRL *C, int ncid, int varid, char *name_units)
{	/* Get attributes long_name and units for given variable ID
	 * and assign variable name if attributes are not available.
	 * ncid, varid		: as in nc_get_att_text
	 * nameunit		: long_name and units in form "long_name [units]"
	 */
	char units[GRD_UNIT_LEN];
	if (GMT_nc_get_att_text (C, ncid, varid, "long_name", name_units, (size_t)GRD_UNIT_LEN)) nc_inq_varname (ncid, varid, name_units);
	if (!GMT_nc_get_att_text (C, ncid, varid, "units", units, (size_t)GRD_UNIT_LEN) && units[0]) sprintf (name_units, "%s [%s]", name_units, units);
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
		name[i] = '\0';
		if (name[i-1] == ' ') name[i-1] = '\0';
	}
	i = 0;
	while (units[i] && units[i] != ']') i++;
	if (units[i]) units[i] = '\0';
	if (name[0]) nc_put_att_text (ncid, varid, "long_name", strlen(name), name);
	if (units[0]) nc_put_att_text (ncid, varid, "units", strlen(units), units);
}

void GMT_nc_check_step (struct GMT_CTRL *C, GMT_LONG n, double *x, char *varname, char *file)
{	/* Check if all steps in range are the same (within 2%) */
	double step, step_min, step_max;
	GMT_LONG i;
	if (n < 2) return;
	step_min = step_max = x[1]-x[0];
	for (i = 2; i < n; i++) {
		step = x[i]-x[i-1];
		if (step < step_min) step_min = step;
		if (step > step_max) step_max = step;
	}
	if (fabs (step_min-step_max)/(fabs (step_min)+fabs (step_max)) > 0.05) {
		GMT_report (C, GMT_MSG_FATAL, "Warning: The step size of coordinate (%s) in grid %s is not constant.\n", varname, file);
		GMT_report (C, GMT_MSG_FATAL, "Warning: GMT will use a constant step size of %g; the original ranges from %g to %g.\n", (x[n-1]-x[0])/(n-1), step_min, step_max);
	}
}

GMT_LONG GMT_nc_grd_info (struct GMT_CTRL *C, struct GRD_HEADER *header, char job)
{
	GMT_LONG j, err;
	int old_fill_mode;
	double dummy[2], *xy = NULL;
	char dimname[GRD_UNIT_LEN], coord[8];
	nc_type z_type;
	double t_value[3];

	/* Dimension ids, variable ids, etc.. */
	int i, ncid, z_id = -1, ids[5] = {-1,-1,-1,-1,-1}, dims[5], nvars, ndims;
	size_t lens[5], item[2];

	for (i = 0; i < 3; i++) t_value[i] = C->session.d_NaN;

	/* If not yet determined, attempt to get the layer IDs from the variable name */

	if (header->t_index[0] >= 0) { /* Do nothing: already determined */ }
	else if (!header->varname[0])
		header->t_index[0] = 0;	/* No varname: use first layer */
	else {
		i = 0;
		while (header->varname[i] && header->varname[i] != '(' && header->varname[i] != '[') i++;
		if (header->varname[i] == '(') {
			sscanf (&header->varname[i+1], "%lf,%lf,%lf)", &t_value[0], &t_value[1], &t_value[2]);
			header->varname[i] = '\0';
		}
		else if (header->varname[i] == '[') {
			sscanf (&header->varname[i+1], "%d,%d,%d]", &header->t_index[0], &header->t_index[1], &header->t_index[2]);
			header->varname[i] = '\0';
		}
	}

	/* Open NetCDF file */

	if (!strcmp (header->name,"=")) return (GMT_GRDIO_NC_NO_PIPE);
	switch (job) {
		case 'r':
			GMT_err_trap (nc_open (header->name, NC_NOWRITE, &ncid));
			break;
		case 'u':
			GMT_err_trap (nc_open (header->name, NC_WRITE, &ncid)); 
			GMT_err_trap (nc_set_fill (ncid, NC_NOFILL, &old_fill_mode)); 
			break;
		default:
			GMT_err_trap (nc_create (header->name, NC_CLOBBER, &ncid));
			GMT_err_trap (nc_set_fill (ncid, NC_NOFILL, &old_fill_mode));
			break;
	}

	/* Retrieve or define dimensions and variables */

	if (job == 'r' || job == 'u') {
		/* First see if this is an old NetCDF formatted file */
		if (!nc_inq_dimid (ncid, "xysize", &i)) return (GMT_cdf_grd_info (C, ncid, header, job));

		/* Find first 2-dimensional (z) variable or specified variable */
		if (!header->varname[0]) {
			GMT_err_trap (nc_inq_nvars (ncid, &nvars));
			i = 0;
			while (i < nvars && z_id < 0) {
				GMT_err_trap (nc_inq_varndims (ncid, i, &ndims));
				if (ndims == 2) z_id = i;
				i++;
			}
		}
		else if (nc_inq_varid (ncid, header->varname, &z_id) == NC_NOERR) {
			GMT_err_trap (nc_inq_varndims (ncid, z_id, &ndims));
			if (ndims < 2 || ndims > 5) return (GMT_GRDIO_BAD_DIM);
		}
		else
			return (GMT_GRDIO_NO_VAR);
		if (z_id < 0) return (GMT_GRDIO_NO_2DVAR);

		/* Get the z data type and determine its dimensions */
		GMT_err_trap (nc_inq_vartype (ncid, z_id, &z_type));
		GMT_err_trap (nc_inq_vardimid (ncid, z_id, dims));
		header->type = ((z_type == NC_BYTE) ? 2 : z_type) + 13;

		/* Get the ids of the x and y (and depth and time) coordinate variables */
		for (i = 0; i < ndims; i++) {
			GMT_err_trap (nc_inq_dim (ncid, dims[i], dimname, &lens[i]));
			if (nc_inq_varid (ncid, dimname, &ids[i])) ids[i] = -1;
		}
		header->xy_dim[0] = ndims-1;
		header->xy_dim[1] = ndims-2;

		/* Check if LatLon variable exists, then we may need to flip x and y */
		if (nc_inq_varid (ncid, "LatLon", &i) == NC_NOERR) nc_get_var_int (ncid, i, header->xy_dim);
		header->nx = (int) lens[header->xy_dim[0]];
		header->ny = (int) lens[header->xy_dim[1]];
	}
	else {
		/* Define dimensions of z variable */
		ndims = 2;
		header->xy_dim[0] = 1;
		header->xy_dim[1] = 0;

		strcpy (coord, (C->current.io.col_type[GMT_OUT][GMT_X] == GMT_IS_LON) ? "lon" : (C->current.io.col_type[GMT_OUT][GMT_X] & GMT_IS_RATIME) ? "time" : "x");
		GMT_err_trap (nc_def_dim (ncid, coord, (size_t) header->nx, &dims[1]));
		GMT_err_trap (nc_def_var (ncid, coord, NC_DOUBLE, 1, &dims[1], &ids[1]));

		strcpy (coord, (C->current.io.col_type[GMT_OUT][GMT_Y] == GMT_IS_LAT) ? "lat" : (C->current.io.col_type[GMT_OUT][GMT_Y] & GMT_IS_RATIME) ? "time" : "y");
		GMT_err_trap (nc_def_dim (ncid, coord, (size_t) header->ny, &dims[0]));
		GMT_err_trap (nc_def_var (ncid, coord, NC_DOUBLE, 1, &dims[0], &ids[0]));

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

		/* Variable name is given, or defaults to "z" */
		if (!header->varname[0]) strcpy (header->varname, "z");
		GMT_err_trap (nc_def_var (ncid, header->varname, z_type, 2, dims, &z_id));
	}
	header->z_id = z_id;
	header->ncid = ncid;

	/* Query or assign attributes */

	if (job == 'u') GMT_err_trap (nc_redef (ncid));

	if (job == 'r') {
		/* Get global information */
		if (GMT_nc_get_att_text (C, ncid, NC_GLOBAL, "title", header->title, (size_t)GRD_TITLE_LEN))
		    GMT_nc_get_att_text (C, ncid, z_id, "long_name", header->title, (size_t)GRD_TITLE_LEN);
		if (GMT_nc_get_att_text (C, ncid, NC_GLOBAL, "history", header->command, (size_t)GRD_COMMAND_LEN))
		    GMT_nc_get_att_text (C, ncid, NC_GLOBAL, "source", header->command, (size_t)GRD_COMMAND_LEN);
		GMT_nc_get_att_text (C, ncid, NC_GLOBAL, "description", header->remark, (size_t)GRD_REMARK_LEN);
		nc_get_att_int (ncid, NC_GLOBAL, "node_offset", &header->registration);

		/* Create enough memory to store the x- and y-coordinate values */
		xy = GMT_memory (C, NULL, MAX(header->nx,header->ny), double);

		/* Get information about x variable */
		GMT_nc_get_units (C, ncid, ids[header->xy_dim[0]], header->x_units);
		if (!(j = nc_get_var_double (ncid, ids[header->xy_dim[0]], xy))) GMT_nc_check_step (C, header->nx, xy, header->x_units, header->name);
		if (!nc_get_att_double (ncid, ids[header->xy_dim[0]], "actual_range", dummy))
			header->wesn[XLO] = dummy[0], header->wesn[XHI] = dummy[1];
		else if (!j) {
			header->wesn[XLO] = xy[0], header->wesn[XHI] = xy[header->nx-1];
			header->registration = GMT_GRIDLINE_REG;
		}
		else {
			header->wesn[XLO] = 0.0, header->wesn[XHI] = (double) header->nx-1;
			header->registration = GMT_GRIDLINE_REG;
		}
		header->inc[GMT_X] = GMT_get_inc (header->wesn[XLO], header->wesn[XHI], header->nx, header->registration);
		if (GMT_is_dnan(header->inc[GMT_X])) header->inc[GMT_X] = 1.0;

		/* Get information about y variable */
		GMT_nc_get_units (C, ncid, ids[header->xy_dim[1]], header->y_units);
		if (!(j = nc_get_var_double (ncid, ids[header->xy_dim[1]], xy))) GMT_nc_check_step (C, header->ny, xy, header->y_units, header->name);
		if (!nc_get_att_double (ncid, ids[header->xy_dim[1]], "actual_range", dummy))
			header->wesn[YLO] = dummy[0], header->wesn[YHI] = dummy[1];
		else if (!j)
			header->wesn[YLO] = xy[0], header->wesn[YHI] = xy[header->ny-1];
		else
			header->wesn[YLO] = 0.0, header->wesn[YHI] = (double) header->ny-1;
		/* Check for reverse order of y-coordinate */
		if (header->wesn[YLO] > header->wesn[YHI]) {
			header->y_order = -1;
			dummy[0] = header->wesn[YHI], dummy[1] = header->wesn[YLO];
			header->wesn[YLO] = dummy[0], header->wesn[YHI] = dummy[1];
		}
		else
			header->y_order = 1;
		header->inc[GMT_Y] = GMT_get_inc (header->wesn[YLO], header->wesn[YHI], header->ny, header->registration);
		if (GMT_is_dnan(header->inc[GMT_Y])) header->inc[GMT_Y] = 1.0;

		GMT_free (C, xy);

		/* Get information about z variable */
		GMT_nc_get_units (C, ncid, z_id, header->z_units);
		if (nc_get_att_double (ncid, z_id, "scale_factor", &header->z_scale_factor)) header->z_scale_factor = 1.0;
		if (nc_get_att_double (ncid, z_id, "add_offset", &header->z_add_offset)) header->z_add_offset = 0.0;
		if (nc_get_att_double (ncid, z_id, "_FillValue", &header->nan_value))
		    nc_get_att_double (ncid, z_id, "missing_value", &header->nan_value);
		if (nc_get_att_double (ncid, z_id, "actual_range", dummy) && nc_get_att_double (ncid, z_id, "valid_range", dummy))
			{ /* Leave values to their defaults (NaN) */ }
		else {
			/* z-limits need to be converted from actual to internal grid units. */
			header->z_min = (dummy[0] - header->z_add_offset) / header->z_scale_factor;
			header->z_max = (dummy[1] - header->z_add_offset) / header->z_scale_factor;
		}

		/* Get grid buffer */
		item[0] = 0;
		for (i = 0; i < ndims-2; i++) {
			if (header->t_index[i] > -1) { /* Do nothing */ }
			else if (GMT_is_dnan(t_value[i]))
				header->t_index[i] = 0;
			else {
				item[1] = lens[i]-1;
				if (nc_get_att_double (ncid, ids[i], "actual_range", dummy)) {
					GMT_err_trap (nc_get_var1_double (ncid, ids[i], &item[0], &dummy[0]));
					GMT_err_trap (nc_get_var1_double (ncid, ids[i], &item[1], &dummy[1]));
				}
				header->t_index[i] = irint((t_value[i] - dummy[0]) / (dummy[1] - dummy[0]) * item[1]);
			}
		}
	}
	else {
		/* Store global attributes */
		GMT_err_trap (nc_put_att_text (ncid, NC_GLOBAL, "Conventions", strlen(GMT_CDF_CONVENTION), (const char *) GMT_CDF_CONVENTION));
		if (header->title[0]) {
			GMT_err_trap (nc_put_att_text (ncid, NC_GLOBAL, "title", strlen(header->title), header->title));
		}
		else {
			GMT_err_trap (nc_put_att_text (ncid, NC_GLOBAL, "title", strlen(header->name), header->name));
		}
		if (header->command[0]) GMT_err_trap (nc_put_att_text (ncid, NC_GLOBAL, "history", strlen(header->command), header->command));
		if (header->remark[0]) GMT_err_trap (nc_put_att_text (ncid, NC_GLOBAL, "description", strlen(header->remark), header->remark));
		GMT_err_trap (nc_put_att_text (ncid, NC_GLOBAL, "GMT_version", strlen(GMT_VERSION), (const char *) GMT_VERSION));
		if (header->registration == GMT_PIXEL_REG) GMT_err_trap (nc_put_att_int (ncid, NC_GLOBAL, "node_offset", NC_LONG, (size_t)1, &header->registration));

		/* Define x variable */
		GMT_nc_put_units (ncid, ids[header->xy_dim[0]], header->x_units);
		dummy[0] = header->wesn[XLO], dummy[1] = header->wesn[XHI];
		GMT_err_trap (nc_put_att_double (ncid, ids[header->xy_dim[0]], "actual_range", NC_DOUBLE, (size_t)2, dummy));

		/* Define y variable */
		GMT_nc_put_units (ncid, ids[header->xy_dim[1]], header->y_units);
		header->y_order = 1;
		dummy[(1-header->y_order)/2] = header->wesn[YLO], dummy[(1+header->y_order)/2] = header->wesn[YHI];
		GMT_err_trap (nc_put_att_double (ncid, ids[header->xy_dim[1]], "actual_range", NC_DOUBLE, (size_t)2, dummy));

		/* When varname is given, and z_units is default, overrule z_units with varname */
		if (header->varname[0] && !strcmp (header->z_units, "z")) strcpy (header->z_units, header->varname);

		/* Define z variable. Attempt to remove "scale_factor" or "add_offset" when no longer needed */
		GMT_nc_put_units (ncid, z_id, header->z_units);
		if (header->z_scale_factor != 1.0) {
			GMT_err_trap (nc_put_att_double (ncid, z_id, "scale_factor", NC_DOUBLE, (size_t)1, &header->z_scale_factor));
		}
		else if (job == 'u')
			nc_del_att (ncid, z_id, "scale_factor");
		if (header->z_add_offset != 0.0) {
			GMT_err_trap (nc_put_att_double (ncid, z_id, "add_offset", NC_DOUBLE, (size_t)1, &header->z_add_offset));
		}
		else if (job == 'u')
			nc_del_att (ncid, z_id, "add_offset");
		if (z_type == NC_FLOAT || z_type == NC_DOUBLE) {
			GMT_err_trap (nc_put_att_double (ncid, z_id, "_FillValue", z_type, (size_t) 1, &header->nan_value));
		}
		else {
			i = irint (header->nan_value);
			GMT_err_trap (nc_put_att_int (ncid, z_id, "_FillValue", z_type, (size_t)1, &i));
		}

		/* Limits need to be stored in actual, not internal grid, units */
		if (header->z_min <= header->z_max) {
			dummy[0] = header->z_min * header->z_scale_factor + header->z_add_offset;
			dummy[1] = header->z_max * header->z_scale_factor + header->z_add_offset;
		}
		else
			dummy[0] = 0.0, dummy[1] = 0.0;
		GMT_err_trap (nc_put_att_double (ncid, z_id, "actual_range", NC_DOUBLE, (size_t)2, dummy));

		/* Store values along x and y axes */
		GMT_err_trap (nc_enddef (ncid));
		xy = GMT_memory (C, NULL,  MAX (header->nx,header->ny), double);
		for (i = 0; i < header->nx; i++) xy[i] = GMT_grd_col_to_x (i, header);
		GMT_err_trap (nc_put_var_double (ncid, ids[header->xy_dim[0]], xy));
		if (header->y_order > 0) {
			for (i = 0; i < header->ny; i++) xy[i] = (double) GMT_col_to_x (i, header->wesn[YLO], header->wesn[YHI], header->inc[GMT_Y], 0.5 * header->registration, header->ny);
		}
		else {
			for (i = 0; i < header->ny; i++) xy[i] = (double) GMT_row_to_y (i, header->wesn[YLO], header->wesn[YHI], header->inc[GMT_Y], 0.5 * header->registration, header->ny);
		}
		GMT_err_trap (nc_put_var_double (ncid, ids[header->xy_dim[1]], xy));
		GMT_free (C, xy);
	}

	/* Close NetCDF file, unless job == 'W' */

	if (job != 'W') GMT_err_trap (nc_close (ncid));
	return (GMT_NOERROR);
}

GMT_LONG GMT_nc_read_grd_info (struct GMT_CTRL *C, struct GRD_HEADER *header)
{
	return (GMT_nc_grd_info (C, header, 'r'));
}

GMT_LONG GMT_nc_update_grd_info (struct GMT_CTRL *C, struct GRD_HEADER *header)
{
	return (GMT_nc_grd_info (C, header, 'u'));
}

GMT_LONG GMT_nc_write_grd_info (struct GMT_CTRL *C, struct GRD_HEADER *header)
{
	return (GMT_nc_grd_info (C, header, 'w'));
}

GMT_LONG GMT_nc_read_grd (struct GMT_CTRL *C, struct GRD_HEADER *header, float *grid, double wesn[], GMT_LONG *pad, GMT_LONG complex_mode)
{	/* header:	grid structure header
	 * grid:	array with final grid
	 * wesn:	Sub-region to extract  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex_mode:	1|2 if complex array is to hold real (1) and imaginary (2) parts (0 = read as real only)
	 *		Note: The file has only real values, we simply allow space in the complex array
	 *		for real and imaginary parts when processed by grdfft etc.
	 *
	 * Reads a subset of a grdfile and optionally pads the array with extra rows and columns
	 * header values for nx and ny are reset to reflect the dimensions of the logical array,
	 * not the physical size (i.e., the padding is not counted in nx and ny)
	 */
	 
	size_t start[5] = {0,0,0,0,0}, edge[5] = {1,1,1,1,1};
	int ncid, ndims;
	GMT_LONG first_col, last_col, first_row, last_row, check, *k = NULL;
	GMT_LONG i, j, width_in, width_out, height_in, i_0_out, inc, off, err;
	size_t ij, kk;	/* To allow 64-bit addressing on 64-bit systems */
	float *tmp = NULL;
	EXTERN_MSC GMT_LONG GMT_cdf_read_grd (struct GMT_CTRL *C, struct GRD_HEADER *header, float *grid, double wesn[], GMT_LONG *pad, GMT_LONG complex_mode);

	/* Check type: is file in old NetCDF format or not at all? */

	if (GMT_grdformats[header->type][0] == 'c')
		return (GMT_cdf_read_grd (C, header, grid, wesn, pad, complex_mode));
	else if (GMT_grdformats[header->type][0] != 'n')
		return (NC_ENOTNC);

	GMT_err_pass (C, GMT_grd_prep_io (C, header, wesn, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row, &k), header->name);
	(void)GMT_init_complex (C, complex_mode, &inc, &off);	/* Set stride and offset if complex */

	width_out = width_in;		/* Width of output array */
	if (pad[XLO] > 0) width_out += pad[XLO];
	if (pad[XHI] > 0) width_out += pad[XHI];

	width_out *= inc;			/* Possibly doubled if complex_mode is TRUE */
	i_0_out = inc * pad[XLO] + off;		/* Edge offset in output */

	/* Open the NetCDF file */

	if (!strcmp (header->name,"=")) return (GMT_GRDIO_NC_NO_PIPE);
 	GMT_err_trap (nc_open (header->name, NC_NOWRITE, &ncid));
	check = !GMT_is_dnan (header->nan_value);
	GMT_err_trap (nc_inq_varndims (ncid, header->z_id, &ndims));

	tmp = GMT_memory (C, NULL, header->nx, float);

	/* Load the data row by row. The data in the file is stored either "top down"
	 * (y_order < 0, the first row is the top row) or "bottom up" (y_order > 0, the first
	 * row is the bottom row). GMT will store the data in "top down" mode. */

	for (i = 0; i < ndims-2; i++) start[i] = header->t_index[i];

	edge[header->xy_dim[0]] = header->nx;
	if (header->y_order < 0)
		ij = (size_t)pad[YHI] * (size_t)width_out + (size_t)i_0_out;
	else {		/* Flip around the meaning of first and last row */
		ij = ((size_t)last_row - (size_t)first_row + (size_t)pad[YHI]) * (size_t)width_out + (size_t)i_0_out;
		j = first_row;
		first_row = header->ny - 1 - last_row;
		last_row = header->ny - 1 - j;
	}
	header->z_min =  DBL_MAX;	header->z_max = -DBL_MAX;

	for (j = first_row; j <= last_row; j++, ij -= ((size_t)header->y_order * (size_t)width_out)) {
		start[header->xy_dim[1]] = j;
		GMT_err_trap (nc_get_vara_float (ncid, header->z_id, start, edge, tmp));	/* Get one row */
		for (i = 0, kk = ij; i < width_in; i++, kk+=inc) {	/* Check for and handle NaN proxies */
			grid[kk] = tmp[k[i]];
			if (check && grid[kk] == header->nan_value) grid[kk] = C->session.f_NaN;
			if (GMT_is_fnan (grid[kk])) continue;
			header->z_min = MIN (header->z_min, (double)grid[kk]);
			header->z_max = MAX (header->z_max, (double)grid[kk]);
		}
	}

	header->nx = (int)width_in;
	header->ny = (int)height_in;
	GMT_memcpy (header->wesn, wesn, 4, double);

	GMT_err_trap (nc_close (ncid));

	GMT_free (C, k);
	GMT_free (C, tmp);
	return (GMT_NOERROR);
}

GMT_LONG GMT_nc_write_grd (struct GMT_CTRL *C, struct GRD_HEADER *header, float *grid, double wesn[], GMT_LONG *pad, GMT_LONG complex_mode)
{	/* header:	grid structure header
	 * grid:	array with final grid
	 * wesn:	Sub-region to write out  [Use entire file if 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex_mode:	1|2 if complex array is to hold real (1) and imaginary (2) parts (0 = read as real only)
	 *		Note: The file has only real values, we simply allow space in the complex array
	 *		for real and imaginary parts when processed by grdfft etc.
	 */

	size_t start[2] = {0,0}, edge[2] = {1,1};
	GMT_LONG i, j, inc, off, nr_oor = 0, err, width_in, width_out, height_out, node;
	GMT_LONG first_col, last_col, first_row, last_row, *k = NULL;
	size_t ij;	/* To allow 64-bit addressing on 64-bit systems */
	float *tmp_f = NULL;
	int *tmp_i = NULL;
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

	GMT_err_pass (C, GMT_grd_prep_io (C, header, wesn, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row, &k), header->name);
	(void)GMT_init_complex (C, complex_mode, &inc, &off);	/* Set stride and offset if complex */

	width_in = width_out;		/* Physical width of input array */
	if (pad[XLO] > 0) width_in += pad[XLO];
	if (pad[XHI] > 0) width_in += pad[XHI];

	GMT_memcpy (header->wesn, wesn, 4, double);
	header->nx = (int)width_out;
	header->ny = (int)height_out;

	/* Write grid header without closing file afterwards */

	GMT_err_trap (GMT_nc_grd_info (C, header, 'W'));

	/* Set start position for writing grid */

	edge[1] = width_out;
	ij = (size_t)first_col + (size_t)pad[XLO] + ((size_t)last_row + (size_t)pad[YHI]) * (size_t)width_in;
	header->z_min =  DBL_MAX;	header->z_max = -DBL_MAX;

	/* Store z-variable. Distinguish between floats and integers */

	if (z_type == NC_FLOAT || z_type == NC_DOUBLE) {
		tmp_f = GMT_memory (C, NULL, width_in, float);
		for (j = 0; j < height_out; j++, ij -= (size_t)width_in) {
			start[0] = j;
			for (i = 0; i < width_out; i++) {
				node = inc*(ij+k[i])+off;
				if (node < 0 || node > header->size) {
					fprintf (stderr, "Outside bounds\n");
				}
				value = grid[node];
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
			GMT_err_trap (nc_put_vara_float (header->ncid, header->z_id, start, edge, tmp_f));
		}
		GMT_free (C, tmp_f);
	}

	else {
		tmp_i = GMT_memory (C, NULL, width_in, int);
		for (j = 0; j < height_out; j++, ij -= (size_t)width_in) {
			start[0] = j;
			for (i = 0; i < width_out; i++) {
				value = grid[inc*(ij+k[i])+off];
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
			GMT_err_trap (nc_put_vara_int (header->ncid, header->z_id, start, edge, tmp_i));
		}
		GMT_free (C, tmp_i);
	}

	if (nr_oor > 0) GMT_report (C, GMT_MSG_FATAL, "Warning: %ld out-of-range grid values converted to _FillValue [%s]\n", nr_oor, header->name);

	GMT_free (C, k);

	/* Limits need to be written in actual, not internal grid, units */

	if (header->z_min <= header->z_max) {
		limit[0] = header->z_min * header->z_scale_factor + header->z_add_offset;
		limit[1] = header->z_max * header->z_scale_factor + header->z_add_offset;
	}
	else {
		GMT_report (C, GMT_MSG_FATAL, "Warning: No valid values in grid [%s]\n", header->name);
		limit[0] = limit[1] = 0.0;
	}
	GMT_err_trap (nc_put_att_double (header->ncid, header->z_id, "actual_range", NC_DOUBLE, (size_t)2, limit));

	/* Close grid */

	GMT_err_trap (nc_close (header->ncid));

	return (GMT_NOERROR);
}
