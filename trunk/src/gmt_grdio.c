/*--------------------------------------------------------------------
 *	$Id: gmt_grdio.c,v 1.140 2011-03-15 02:06:36 guru Exp $
 *
 *	Copyright (c) 1991-2011 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be u237seful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 *
 *	G M T _ G R D I O . C   R O U T I N E S
 *
 * Generic routines that take care of all gridfile input/output.
 * These are the only PUBLIC grd io functions to be used by developers
 *
 * Author:	Paul Wessel
 * Date:	9-SEP-1992
 * Modified:	1-JAN-2010
 * Version:	5
 * 64-bit Compliant: Yes
 *
 * Functions include:
 *
 *	GMT_grd_get_format :	Get format id, scale, offset and missing value for grdfile
 *
 *	GMT_read_grd_info :	Read header from file
 *	GMT_read_grd :		Read data set from file (must be preceded by GMT_read_grd_info)
 *	GMT_update_grd_info :	Update header in existing file (must be preceded by GMT_read_grd_info)
 *	GMT_write_grd_info :	Write header to new file
 *	GMT_write_grd :		Write header and data set to new file
 *
 *	For programs that must access on row at the time, you must use:
 *	GMT_open_grd :		Opens the grdfile for reading or writing
 *	GMT_read_grd_row :	Reads a single row of data from grdfile
 *	GMT_write_grd_row :	Writes a single row of data from grdfile
 *	GMT_close_grd :		Close the grdfile
 *
 * Additional supporting grid routines:
 *
 *	GMT_grd_init 		Initialize grd header structure
 *	GMT_grd_shift 		Rotates grdfiles in x-direction
 *	GMT_grd_setregion 	Determines subset coordinates for grdfiles
 *	GMT_grd_is_global	Determine whether grid is "global", i.e. longitudes are periodic
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define GMT_WITH_NO_PS
#include "gmt.h"
#include "gmt_internals.h"

GMT_LONG GMT_grdformats[GMT_N_GRD_FORMATS][2] = {
#include "gmt_grdformats.h"
};

struct GRD_PAD {
	double wesn[4];
	GMT_LONG pad[4];
	GMT_LONG expand;
};

EXTERN_MSC GMT_LONG GMT_is_nc_grid (struct GMT_CTRL *C, struct GRD_HEADER *header);
EXTERN_MSC GMT_LONG GMT_is_native_grid (struct GMT_CTRL *C, struct GRD_HEADER *header);
EXTERN_MSC GMT_LONG GMT_is_ras_grid (struct GMT_CTRL *C, struct GRD_HEADER *header);
EXTERN_MSC GMT_LONG GMT_is_srf_grid (struct GMT_CTRL *C, struct GRD_HEADER *header);
EXTERN_MSC GMT_LONG GMT_is_mgg2_grid (struct GMT_CTRL *C, struct GRD_HEADER *header);
EXTERN_MSC GMT_LONG GMT_is_agc_grid (struct GMT_CTRL *C, struct GRD_HEADER *header);
EXTERN_MSC GMT_LONG GMT_is_esri_grid (struct GMT_CTRL *C, struct GRD_HEADER *header);

/* GENERIC I/O FUNCTIONS FOR GRIDDED DATA FILES */

/* Routines to see if a particular grd file format is specified as part of filename. */

void GMT_expand_filename (struct GMT_CTRL *C, char *file, char *fname)
{
	GMT_LONG i, length, f_length, found, start;

	if (C->current.setting.io_gridfile_shorthand) {	/* Look for matches */
		f_length = (GMT_LONG) strlen (file);
		for (i = found = 0; !found && i < C->session.n_shorthands; i++) {
			length = (GMT_LONG) strlen (C->session.shorthand[i].suffix);
			start = f_length - length;
			found = (start < 0) ? FALSE : !strncmp (&file[start], C->session.shorthand[i].suffix, (size_t)length);
		}
		if (found) {	/* file ended in a recognized shorthand extension */
			i--;
			sprintf (fname, "%s=%ld/%g/%g/%g", file, C->session.shorthand[i].id, C->session.shorthand[i].scale, C->session.shorthand[i].offset, C->session.shorthand[i].nan);
		}
		else
			strcpy (fname, file);
	}
	else	/* Simply copy the full name */
		strcpy (fname, file);
}

GMT_LONG GMT_grd_get_format (struct GMT_CTRL *C, char *file, struct GRD_HEADER *header, GMT_LONG magic)
{
	/* This functions does a couple of things:
	 * 1. It tries to determine what kind of grid file this is. If a file is openeed for
	 *    reading we see if (a) a particular format has been specified with
	 *    the =<code> suffix, or (b) we are able to guess the format based on known
	 *    characteristics of various formats, or (c) assume the default grid format.
	 *    If a file is opened for writing, only option (a) and (c) apply.
	 *    If we cannot obtain the format we return an error.
	 * 2. We strip the suffix off. The relevant info is stored in the header struct.
	 * 3. In case of netCDF grids, the optional ?<varname> is stripped off as well.
	 *    The info is stored in header->varname.
	 * 4. If a file is open for reading, we set header->name to the full path of the file
	 *    by seaching in current dir and the various GMT_*DIR paths.
	 */
	
	GMT_LONG i = 0, val, j;
	char code[GMT_TEXT_LEN], tmp[BUFSIZ];

	GMT_expand_filename (C, file, header->name);	/* May append a suffix to header->name */

	/* Set default values */
	header->z_scale_factor = C->session.d_NaN, header->z_add_offset = 0.0, header->nan_value = C->session.d_NaN;

	while (header->name[i] && header->name[i] != '=') i++;

	if (header->name[i]) {	/* Reading or writing when =suffix is present: get format type, scale, offset and missing value */
		i++;
		sscanf (&header->name[i], "%[^/]/%lf/%lf/%lf", code, &header->z_scale_factor, &header->z_add_offset, &header->nan_value);
		val = GMT_grd_format_decoder (C, code);
		if (val < 0) return (val);
		header->type = val;
		if ( val == 22 && header->name[i+2] && header->name[i+2] == '?' ) {	/* A SUBDATASET request for GDAL */
			char *pch;
			pch = strstr(&header->name[i+3], "::");
			if ( pch ) {		/* The file name was omited within the SUBDATASET. Must put it there for GDAL */
				tmp[0] = '\0';
				strncpy(tmp, &header->name[i+3], pch - &header->name[i+3] + 1);
				strcat(tmp, "\"");	strncat(tmp, header->name, i-1);	strcat(tmp, "\"");
				strcat(tmp, &pch[1]);
				strcpy (header->name, tmp);
			}
			else
				strcpy (header->name, &header->name[i+3]);
			magic = 0;	/* We don't want it to try to prepend any path */
		}
		else {
			j = (i == 1) ? i : i - 1;
			header->name[j] = 0;
		}
		sscanf (header->name, "%[^?]?%s", tmp, header->varname);    /* Strip off variable name */
		if (magic) {	/* Reading: possibly prepend a path from GMT_[GRID|DATA|IMG]DIR */
			if (!GMT_getdatapath (C, tmp, header->name)) return (GMT_GRDIO_FILE_NOT_FOUND);
		}
		else		/* Writing: store truncated pathname */
			strcpy (header->name, tmp);
	}
	else if (magic) {	/* Reading: determine file format automatically based on grid content */
		sscanf (header->name, "%[^?]?%s", tmp, header->varname);    /* Strip off variable name */
		if (!GMT_getdatapath (C, tmp, header->name)) return (GMT_GRDIO_FILE_NOT_FOUND);	/* Possibly prepended a path from GMT_[GRID|DATA|IMG]DIR */
		/* First check if we have a netCDF grid. This MUST be first, because ?var needs to be stripped off. */
		if ((val = GMT_is_nc_grid (C, header)) >= 0) return (GMT_NOERROR);
		/* Continue only when file was a pipe or when nc_open didn't like the file. */
		if (val != GMT_GRDIO_NC_NO_PIPE && val != GMT_GRDIO_OPEN_FAILED) return (val);
		/* Then check for native GMT grid */
		if (GMT_is_native_grid (C, header) >= 0) return (GMT_NOERROR);
		/* Next check for Sun raster GMT grid */
		if (GMT_is_ras_grid (C, header) >= 0) return (GMT_NOERROR);
		/* Then check for Golden Software surfer grid */
		if (GMT_is_srf_grid (C, header) >= 0) return (GMT_NOERROR);
		/* Then check for NGDC GRD98 GMT grid */
		if (GMT_is_mgg2_grid (C, header) >= 0) return (GMT_NOERROR);
		/* Then check for AGC GMT grid */
		if (GMT_is_agc_grid (C, header) >= 0) return (GMT_NOERROR);
		/* Then check for ESRI GMT grid */
		if (GMT_is_esri_grid (C, header) >= 0) return (GMT_NOERROR);
		return (GMT_GRDIO_UNKNOWN_FORMAT);	/* No supported format found */
	}
	else {			/* Writing: get format type, scale, offset and missing value from C->current.setting.io_gridfile_format */
		if (sscanf (header->name, "%[^?]?%s", tmp, header->varname) > 1) strcpy (header->name, tmp);    /* Strip off variable name */
		sscanf (C->current.setting.io_gridfile_format, "%[^/]/%lf/%lf/%lf", code, &header->z_scale_factor, &header->z_add_offset, &header->nan_value);
		val = GMT_grd_format_decoder (C, code);
		if (val < 0) return (val);
		header->type = val;
	}
	if (header->type == GMT_grd_format_decoder (C, "af")) header->nan_value = 0.0;	/* 0 is NaN in the AGC format */
	return (GMT_NOERROR);
}

void GMT_grd_set_units (struct GMT_CTRL *C, struct GRD_HEADER *header)
{
	/* Set unit strings for grid coordinates x, y and z based on
	   output data types for columns 0, 1, and 2.
	*/
	GMT_LONG i;
	char *string[3] = {NULL, NULL, NULL}, unit[GRD_UNIT_LEN], date[GMT_CALSTRING_LENGTH], clock[GMT_CALSTRING_LENGTH];

	/* Copy pointers to unit strings */
	string[0] = header->x_units;
	string[1] = header->y_units;
	string[2] = header->z_units;

	/* Use input data type as backup fr output data type */
	for (i = 0; i < 3; i++) 
		if (C->current.io.col_type[GMT_OUT][i] == GMT_IS_UNKNOWN) C->current.io.col_type[GMT_OUT][i] = C->current.io.col_type[GMT_IN][i];

	/* Catch some anomalies */
	if (C->current.io.col_type[GMT_OUT][GMT_X] == GMT_IS_LAT) {
		GMT_report (C, GMT_MSG_FATAL, "Output type for X-coordinate of grid %s is LAT. Replaced by LON.\n", header->name);
		C->current.io.col_type[GMT_OUT][GMT_X] = GMT_IS_LON;
	}
	if (C->current.io.col_type[GMT_OUT][GMT_Y] == GMT_IS_LON) {
		GMT_report (C, GMT_MSG_FATAL, "Output type for Y-coordinate of grid %s is LON. Replaced by LAT.\n", header->name);
		C->current.io.col_type[GMT_OUT][GMT_Y] = GMT_IS_LAT;
	}

	/* Set unit strings one by one based on output type */
	for (i = 0; i < 3; i++) {
		switch (C->current.io.col_type[GMT_OUT][i]) {
		case GMT_IS_LON:
			strcpy (string[i], "longitude [degrees_east]"); break;
		case GMT_IS_LAT:
			strcpy (string[i], "latitude [degrees_north]"); break;
		case GMT_IS_ABSTIME:
		case GMT_IS_RELTIME:
		case GMT_IS_RATIME:
			/* Determine time unit */
			switch (C->current.setting.time_system.unit) {
			case 'y':
				strcpy (unit, "years"); break;
			case 'o':
				strcpy (unit, "months"); break;
			case 'd':
				strcpy (unit, "days"); break;
			case 'h':
				strcpy (unit, "hours"); break;
			case 'm':
				strcpy (unit, "minutes"); break;
			default:
				strcpy (unit, "seconds"); break;
			}
			GMT_format_calendar (C, date, clock, &C->current.io.date_output, &C->current.io.clock_output, FALSE, 1, 0.0);
			sprintf (string[i], "time [%s since %s %s]", unit, date, clock);
			/* Warning for non-double grids */
			if (i == 2 && GMT_grdformats[header->type][1] != 'd') GMT_report (C, GMT_MSG_FATAL, "Warning: Use double precision output grid to avoid loss of significance of time coordinate.\n");
			break;
		}
	}
}

void GMT_grd_get_units (struct GMT_CTRL *C, struct GRD_HEADER *header)
{
	/* Set input data types for columns 0, 1 and 2 based on unit strings for
	   grid coordinates x, y and z.
	   When "Time": transform the data scale and offset to match the current time system.
	*/
	GMT_LONG i;
	char string[3][GMT_LONG_TEXT], *units = NULL;
	double scale = 1.0, offset = 0.0;
	struct GMT_TIME_SYSTEM time_system;

	/* Copy unit strings */
	strcpy (string[0], header->x_units);
	strcpy (string[1], header->y_units);
	strcpy (string[2], header->z_units);

	/* Parse the unit strings one by one */
	for (i = 0; i < 3; i++) {
		/* Skip parsing when input data type is already set */
		if (C->current.io.col_type[GMT_IN][i] & GMT_IS_GEO) continue;
		if (C->current.io.col_type[GMT_IN][i] & GMT_IS_RATIME) {
			C->current.proj.xyz_projection[i] = GMT_TIME;
			continue;
		}

		/* Change name of variable and unit to lower case for comparison */
		GMT_str_tolower (string[i]);

		if ((!strncmp (string[i], "longitude", (size_t)9) || strstr (string[i], "degrees_e")) && (header->wesn[XLO] > -360.0 && header->wesn[XHI] <= 360.0)) {
			/* Input data type is longitude */
			C->current.io.col_type[GMT_IN][i] = GMT_IS_LON;
		}
		else if ((!strncmp (string[i], "latitude", (size_t)8) || strstr (string[i], "degrees_n")) && (header->wesn[YLO] >= -90.0 && header->wesn[YHI] <= 90.0)) {
			/* Input data type is latitude */
			C->current.io.col_type[GMT_IN][i] = GMT_IS_LAT;
		}
		else if (!strcmp (string[i], "time") || !strncmp (string[i], "time [", (size_t)6)) {
			/* Input data type is time */
			C->current.io.col_type[GMT_IN][i] = GMT_IS_RELTIME;
			C->current.proj.xyz_projection[i] = GMT_TIME;

			/* Determine coordinates epoch and units (default is internal system) */
			GMT_memcpy (&time_system, &C->current.setting.time_system, 1, struct GMT_TIME_SYSTEM);
			units = strchr (string[i], '[') + 1;
			if (!units || GMT_get_time_system (C, units, &time_system) || GMT_init_time_system_structure (C, &time_system))
				GMT_report (C, GMT_MSG_FATAL, "Warning: Time units [%s] in grid not recognised, defaulting to gmt.conf.\n", units);

			/* Determine scale between grid and internal time system, as well as the offset (in internal units) */
			scale = time_system.scale * C->current.setting.time_system.i_scale;
			offset = (time_system.rata_die - C->current.setting.time_system.rata_die) + (time_system.epoch_t0 - C->current.setting.time_system.epoch_t0);
			offset *= GMT_DAY2SEC_F * C->current.setting.time_system.i_scale;

			/* Scale data scale and extremes based on scale and offset */
			if (i == 0) {
				header->wesn[XLO] = header->wesn[XLO] * scale + offset;
				header->wesn[XHI] = header->wesn[XHI] * scale + offset;
				header->inc[GMT_X] *= scale;
			}
			else if (i == 1) {
				header->wesn[YLO] = header->wesn[YLO] * scale + offset;
				header->wesn[YHI] = header->wesn[YHI] * scale + offset;
				header->inc[GMT_Y] *= scale;
			}
			else {
				header->z_add_offset = header->z_add_offset * scale + offset;
				header->z_scale_factor *= scale;
			}
		}
	}
}

GMT_LONG GMT_grd_pad_status (struct GRD_HEADER *header, GMT_LONG *pad)
{	/* Determines if this grid has padding at all (pad = NULL) OR
	 * if pad is given, determines if the pads are different.
	 * Return codes are:
	 * 1) If pad == NULL:
	 *    FALSE: Grid has zero padding.
	 *    TRUE:  Grid has non-zero padding.
	 * 2) If pad contains the desired pad:
	 *    TRUE:  Grid padding matches pad exactly.
	 *    FALSE: Grid padding failed to match pad exactly.
	 */
	GMT_LONG side;
	
	if (pad) {	/* Determine if the grid's pad differ from given pad (FALSE) or not (TRUE) */
		for (side = 0; side < 4; side++) if (header->pad[side] != pad[side]) return (FALSE);	/* Pads differ */
		return (TRUE);	/* Pads match */
	}
	else {	/* We just want to determine if the grid has padding already (TRUE) or not (FALSE) */
		for (side = 0; side < 4; side++) if (header->pad[side]) return (TRUE);	/* Grid has a pad */
		return (FALSE);	/* Grid has no pad */
	}
}

GMT_LONG GMT_padspace (struct GRD_HEADER *header, double *wesn, GMT_LONG *pad, struct GRD_PAD *P)
{	/* When padding is requested it is usually used to set boundary conditions based on
	 * two extra rows/columns around the domain of interest.  BCs like natural or periodic
	 * can then be used to fill in the pad.  However, if the domain is taken from a grid
	 * whose full domain exceeds the region of interest we are better off using the extra
	 * data to fill those pad rows/columns.  Thus, this function tries to determine if the
	 * input grid has the extra data we need to fill the BC pad with observations. */
	double wesn2[4];
	
	/* First copy over original settings to the Pad structure */
	GMT_memset (P, 1, struct GRD_PAD);						/* Initialize to zero */
	GMT_memcpy (P->pad, pad, 4, GMT_LONG);						/* Duplicate the pad */
	if (!wesn || (wesn[XLO] == wesn[XHI] && wesn[YLO] == wesn[YHI])) return (FALSE);	/* No subset requested */
	GMT_memcpy (P->wesn, wesn, 4, double);						/* Copy the subset boundaries */
	if (pad[XLO] == 0 && pad[XHI] == 0 && pad[YLO] == 0 && pad[YHI] == 0) return (FALSE);	/* No padding requested */
	
	/* Determine if data exist for a pad on all four sides.  If not we give up */
	if ((wesn2[XLO] = wesn[XLO] - pad[XLO] * header->inc[GMT_X]) < header->wesn[XLO]) return (FALSE);
	if ((wesn2[XHI] = wesn[XHI] + pad[XHI] * header->inc[GMT_X]) > header->wesn[XHI]) return (FALSE);
	if ((wesn2[YLO] = wesn[YLO] - pad[YLO] * header->inc[GMT_Y]) < header->wesn[YLO]) return (FALSE);
	if ((wesn2[YHI] = wesn[YHI] + pad[YHI] * header->inc[GMT_Y]) > header->wesn[YHI]) return (FALSE);
	
	/* Here we know that there is enough input data to fill the BC pad with actual data values */
	
	/* Temporarily enlarge the region so it now includes the padding we need */
	GMT_memcpy (P->wesn, wesn2, 4, double);
	/* Temporarily set padding to zero (since the pad is now part of the region) */
	GMT_memset (P->pad, 4, GMT_LONG);
	
	return (TRUE);	/* Return TRUE so the calling function can take appropriate action */
}

GMT_LONG GMT_read_grd_info (struct GMT_CTRL *C, char *file, struct GRD_HEADER *header)
{	/* file:	File name
	 * header:	grid structure header
	 * Note: The header reflects what is actually in the file, and all the dimensions
	 * reflect the number of rows, cols, size, pads etc.  However, if GMT_read_grd is
	 * called requesting a subset then these will be reset accordingly.
	 */

	GMT_LONG err;
	double scale, offset, nan_value;

	/* Initialize grid information */
	GMT_grd_init (C, header, NULL, FALSE);

	/* Save parameters on file name suffix before issuing C->session.readinfo */
 	GMT_err_trap (GMT_grd_get_format (C, file, header, TRUE));
	scale = header->z_scale_factor, offset = header->z_add_offset, nan_value = header->nan_value;

	GMT_err_trap ((*C->session.readinfo[header->type]) (C, header));
	GMT_grd_get_units (C, header);
	if (!GMT_is_dnan(scale)) header->z_scale_factor = scale, header->z_add_offset = offset;
	if (!GMT_is_dnan(nan_value)) header->nan_value = nan_value;
	if (header->z_scale_factor == 0.0) GMT_report (C, GMT_MSG_FATAL, "GMT Warning: scale_factor should not be 0.\n");
	GMT_err_pass (C, GMT_grd_RI_verify (C, header, 0), file);
	GMT_set_grddim (C, header);	/* Set all integer dimensions and xy_off */

	header->z_min = header->z_min * header->z_scale_factor + header->z_add_offset;
	header->z_max = header->z_max * header->z_scale_factor + header->z_add_offset;

	return (GMT_NOERROR);
}

GMT_LONG GMT_write_grd_info (struct GMT_CTRL *C, char *file, struct GRD_HEADER *header)
{	/* file:	File name
	 * header:	grid structure header
	 */

	GMT_LONG err;

 	GMT_err_trap (GMT_grd_get_format (C, file, header, FALSE));

	if (GMT_is_dnan(header->z_scale_factor))
		header->z_scale_factor = 1.0;
	else if (header->z_scale_factor == 0.0) {
		header->z_scale_factor = 1.0;
		GMT_report (C, GMT_MSG_FATAL, "GMT Warning: scale_factor should not be 0. Reset to 1.\n");
	}
	header->z_min = (header->z_min - header->z_add_offset) / header->z_scale_factor;
	header->z_max = (header->z_max - header->z_add_offset) / header->z_scale_factor;
	GMT_grd_set_units (C, header);
	return ((*C->session.writeinfo[header->type]) (C, header));
}

GMT_LONG GMT_update_grd_info (struct GMT_CTRL *C, char *file, struct GRD_HEADER *header)
{	/* file:	- IGNORED -
	 * header:	grid structure header
	 */

	header->z_min = (header->z_min - header->z_add_offset) / header->z_scale_factor;
	header->z_max = (header->z_max - header->z_add_offset) / header->z_scale_factor;
	GMT_grd_set_units (C, header);
	return ((*C->session.updateinfo[header->type]) (C, header));
}

GMT_LONG GMT_read_grd (struct GMT_CTRL *C, char *file, struct GRD_HEADER *header, float *grid, double *wesn, GMT_LONG *pad, GMT_LONG complex)
{	/* file:	- IGNORED -
	 * header:	grid structure header
	 * grid:	array with final grid
	 * wesn:	Sub-region to extract  [Use entire file if NULL or contains 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex:	TRUE if array is to hold real and imaginary parts (read in real only)
	 *		Note: The file has only real values, we simply allow space in the array
	 *		for imaginary parts when processed by grdfft etc.
	 */

	GMT_LONG expand, err;
	struct GRD_PAD P;

	expand = GMT_padspace (header, wesn, pad, &P);	/* TRUE if we can extend the region by the pad-size to obtain real data for BC */

	GMT_err_trap ((*C->session.readgrd[header->type]) (C, header, grid, P.wesn, P.pad, complex));
	
	if (expand) {	/* Must undo the region extension and reset nx, ny */
		header->nx -= (int)(pad[XLO] + pad[XHI]);
		header->ny -= (int)(pad[YLO] + pad[YHI]);
		GMT_memcpy (header->wesn, wesn, 4, double);
		header->nm = GMT_get_nm (header->nx, header->ny);
		GMT_setnval (header->BC, 4, GMT_BC_IS_DATA);
	}
	if (header->z_scale_factor == 0.0) GMT_report (C, GMT_MSG_FATAL, "GMT Warning: scale_factor should not be 0.\n");
	GMT_grd_setpad (header, pad);		/* Copy the pad to the header */
	GMT_set_grddim (C, header);		/* Update all dimensions */
	GMT_grd_do_scaling (grid, header->size, header->z_scale_factor, header->z_add_offset);
	header->z_min = header->z_min * header->z_scale_factor + header->z_add_offset;
	header->z_max = header->z_max * header->z_scale_factor + header->z_add_offset;
	return (GMT_NOERROR);
}

GMT_LONG GMT_write_grd (struct GMT_CTRL *C, char *file, struct GRD_HEADER *header, float *grid, double *wesn, GMT_LONG *pad, GMT_LONG complex)
{	/* file:	File name
	 * header:	grid structure header
	 * grid:	array with final grid
	 * wesn:	Sub-region to write out  [Use entire file if NULL or contains 0,0,0,0]
	 * padding:	# of empty rows/columns to add on w, e, s, n of grid, respectively
	 * complex:	TRUE if array is to hold real and imaginary parts (read in real only)
	 *		Note: The file has only real values, we simply allow space in the array
	 *		for imaginary parts when processed by grdfft etc.
	 */

	GMT_LONG err;

	GMT_err_trap (GMT_grd_get_format (C, file, header, FALSE));
	if (GMT_is_dnan (header->z_scale_factor))
		header->z_scale_factor = 1.0;
	else if (header->z_scale_factor == 0.0) {
		header->z_scale_factor = 1.0;
		GMT_report (C, GMT_MSG_FATAL, "GMT Warning: scale_factor should not be 0. Reset to 1.\n");
	}
	GMT_grd_set_units (C, header);
	
	GMT_grd_do_scaling (grid, header->size, 1.0/header->z_scale_factor, -header->z_add_offset/header->z_scale_factor);
	return ((*C->session.writegrd[header->type]) (C, header, grid, wesn, pad, complex));
}

GMT_LONG GMT_grd_data_size (struct GMT_CTRL *C, GMT_LONG format, double *nan_value)
{
	/* Determine size of data type and set NaN value, if not yet done so (integers only) */

	switch (GMT_grdformats[format][1]) {
		case 'b':
			if (GMT_is_dnan (*nan_value)) *nan_value = CHAR_MIN;
			return (sizeof (char));
			break;
		case 's':
			if (GMT_is_dnan (*nan_value)) *nan_value = SHRT_MIN;
			return (sizeof (short int));
			break;
		case 'i':
			if (GMT_is_dnan (*nan_value)) *nan_value = INT_MIN;
		case 'm':
			return (sizeof (int));
			break;
		case 'f':
			return (sizeof (float));
			break;
		case 'd':
			return (sizeof (double));
			break;
		default:
			GMT_report (C, GMT_MSG_FATAL, "Unknown grid data type: %c\n", (int)GMT_grdformats[format][1]);
			return (GMT_GRDIO_UNKNOWN_TYPE);
	}
}

GMT_LONG GMT_grd_format_decoder (struct GMT_CTRL *C, const char *code)
{
	/* Returns the integer grid format ID that goes with the specified 2-character code */

	GMT_LONG id;

	if (isdigit ((int)code[0])) {	/* File format number given, convert directly */
		id = atoi (code);
 		if (id < 0 || id >= GMT_N_GRD_FORMATS) return (GMT_GRDIO_UNKNOWN_ID);
	}
	else {	/* Character code given */
		GMT_LONG i, group;
		for (i = group = 0, id = -1; id < 0 && i < GMT_N_GRD_FORMATS; i++) {
			if (GMT_grdformats[i][0] == (short)code[0]) {
				group = code[0];
				if (GMT_grdformats[i][1] == (short)code[1]) id = i;
			}
		}

		if (id == -1) return (GMT_GRDIO_UNKNOWN_ID);
	}

	return (id);
}

void GMT_grd_do_scaling (float *grid, GMT_LONG nm, double scale, double offset)
{
	/* Routine that scales and offsets the data if specified.
	 * Note: The loop includes the pad which we also want scaled as well. */
	GMT_LONG i;

	if (GMT_is_dnan (scale) || GMT_is_dnan (offset)) return;	/* Sanity check */
	if (scale == 1.0 && offset == 0.0) return;			/* No work needed */

	if (scale == 1.0)
		for (i = 0; i < nm; i++) grid[i] += (float)offset;
	else if (offset == 0.0)
		for (i = 0; i < nm; i++) grid[i] *= (float)scale;
	else
		for (i = 0; i < nm; i++) grid[i] = grid[i] * ((float)scale) + (float)offset;
}

/* gmt_grd_RI_verify -- routine to check grd R and I compatibility
 *
 * Author:	W H F Smith
 * Date:	20 April 1998
 */

GMT_LONG GMT_grd_RI_verify (struct GMT_CTRL *C, struct GRD_HEADER *h, GMT_LONG mode)
{
	/* mode - 0 means we are checking an existing grid, mode = 1 means we test a new -R -I combination */

	GMT_LONG error = 0;

	if (!strcmp (C->init.progname, "grdedit")) return (GMT_NOERROR);	/* Separate handling in grdedit to allow grdedit -A */

	switch (GMT_minmaxinc_verify (C, h->wesn[XLO], h->wesn[XHI], h->inc[GMT_X], GMT_SMALL)) {
		case 3:
			GMT_report (C, GMT_MSG_FATAL, "GMT ERROR: grid x increment <= 0.0\n");
			error++;
			break;
		case 2:
			GMT_report (C, GMT_MSG_FATAL, "GMT ERROR: grid x range <= 0.0\n");
			error++;
			break;
		case 1:
			GMT_report (C, GMT_MSG_FATAL, "GMT ERROR: (x_max-x_min) must equal (NX + eps) * x_inc), where NX is an integer and |eps| <= %g.\n", GMT_SMALL);
			error++;
		default:
			/* Everything is OK */
			break;
	}

	switch (GMT_minmaxinc_verify (C, h->wesn[YLO], h->wesn[YHI], h->inc[GMT_Y], GMT_SMALL)) {
		case 3:
			GMT_report (C, GMT_MSG_FATAL, "GMT ERROR: grid y increment <= 0.0\n");
			error++;
			break;
		case 2:
			GMT_report (C, GMT_MSG_FATAL, "GMT ERROR: grid y range <= 0.0\n");
			error++;
			break;
		case 1:
			GMT_report (C, GMT_MSG_FATAL, "GMT ERROR: (y_max-y_min) must equal (NY + eps) * y_inc), where NY is an integer and |eps| <= %g.\n", GMT_SMALL);
			error++;
		default:
			/* Everything is OK */
			break;
	}
	if (error) return ((mode == 0) ? GMT_GRDIO_RI_OLDBAD : GMT_GRDIO_RI_NEWBAD);
	return (GMT_NOERROR);
}

GMT_LONG GMT_grd_prep_io (struct GMT_CTRL *C, struct GRD_HEADER *header, double wesn[], GMT_LONG *width, GMT_LONG *height, GMT_LONG *first_col, GMT_LONG *last_col, GMT_LONG *first_row, GMT_LONG *last_row, GMT_LONG **index)
{
	/* Determines which rows and columns to extract to extract from a grid, based on w,e,s,n.
	 * This routine first rounds the w,e,s,n boundaries to the nearest gridlines or pixels,
	 * then determines the first and last columns and rows, and the width and height of the subset (in cells).
	 * The routine also returns and array of the x-indices in the source grid to be used in the target (subset) grid.
	 */

	GMT_LONG one_or_zero, i, geo = FALSE, *k = NULL;
	double small = 0.1, half_or_zero, x;

	half_or_zero = (header->registration == GMT_PIXEL_REG) ? 0.5 : 0.0;

	if (!GMT_is_subset (header, wesn)) {	/* Get entire file */
		*width  = header->nx;
		*height = header->ny;
		*first_col = *first_row = 0;
		*last_col  = header->nx - 1;
		*last_row  = header->ny - 1;
		GMT_memcpy (wesn, header->wesn, 4, double);
	}
	else {				/* Must deal with a subregion */
		if (C->current.io.col_type[GMT_IN][GMT_X] == GMT_IS_LON)
			geo = TRUE;	/* Geographic data for sure */
		else if (wesn[XLO] < header->wesn[XLO] || wesn[XHI] > header->wesn[XHI])
			geo = TRUE;	/* Probably dealing with periodic grid */

		if (wesn[YLO] < header->wesn[YLO] || wesn[YHI] > header->wesn[YHI]) return (GMT_GRDIO_DOMAIN_VIOLATION);	/* Calling program goofed... */

		one_or_zero = (header->registration == GMT_PIXEL_REG) ? 0 : 1;

		/* Make sure w,e,s,n are proper multiples of x_inc,y_inc away from x_min,y_min */

		GMT_err_pass (C, GMT_adjust_loose_wesn (C, wesn, header), header->name);

		/* Get dimension of subregion */

		*width  = irint ((wesn[XHI] - wesn[XLO]) / header->inc[GMT_X]) + one_or_zero;
		*height = irint ((wesn[YHI] - wesn[YLO]) / header->inc[GMT_Y]) + one_or_zero;

		/* Get first and last row and column numbers */

		*first_col = (GMT_LONG)floor ((wesn[XLO] - header->wesn[XLO]) / header->inc[GMT_X] + small);
		*last_col  = (GMT_LONG)ceil  ((wesn[XHI] - header->wesn[XLO]) / header->inc[GMT_X] - small) - 1 + one_or_zero;
		*first_row = (GMT_LONG)floor ((header->wesn[YHI] - wesn[YHI]) / header->inc[GMT_Y] + small);
		*last_row  = (GMT_LONG)ceil  ((header->wesn[YHI] - wesn[YLO]) / header->inc[GMT_Y] - small) - 1 + one_or_zero;
	}

	k = GMT_memory (C, NULL, *width, GMT_LONG);
	if (geo) {
		small = 0.1 * header->inc[GMT_X];
		for (i = 0; i < (*width); i++) {
			x = GMT_col_to_x (i, wesn[XLO], wesn[XHI], header->inc[GMT_X], half_or_zero, *width);
			if (header->wesn[XLO] - x > small)
				x += 360.0;
			else if (x - header->wesn[XHI] > small)
				x -= 360.0;
			k[i] = GMT_grd_x_to_col (x, header);
		}
	}
	else {	/* Normal ordering */
		for (i = 0; i < (*width); i++) k[i] = (*first_col) + i;
	}

	*index = k;
	
	return (GMT_NOERROR);
}

void GMT_decode_grd_h_info (struct GMT_CTRL *C, char *input, struct GRD_HEADER *h) {

/*	Given input string, copy elements into string portions of h.
	By default use "/" as the field separator. However, if the first and
	last character of the input string is the same AND that character
	is non-alpha-numeric, use the first character as a separator. This
	is to allow "/" as part of the fields.
	If a field has an equals sign, skip it.
	This routine is usually called if -D<input> was given by user,
	and after GMT_grd_init() has been called.
*/
	char ptr[BUFSIZ], sep[] = "/";
	GMT_LONG entry = 0, pos = 0;

	if (input[0] != input[strlen(input)-1]) {}
	else if (input[0] == '=') {}
	else if (input[0] >= 'A' && input[0] <= 'Z') {}
	else if (input[0] >= 'a' && input[0] <= 'z') {}
	else if (input[0] >= '0' && input[0] <= '9') {}
	else {
		sep[0] = input[0];
		pos = 1;
	}

	while ((GMT_strtok (input, sep, &pos, ptr))) {
		if (ptr[0] != '=') {
			switch (entry) {
				case 0:
					GMT_memset (h->x_units, GRD_UNIT_LEN, char);
					if (strlen(ptr) >= GRD_UNIT_LEN) GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: X unit string exceeds upper length of %d characters (truncated)\n", GRD_UNIT_LEN);
					strncpy (h->x_units, ptr, (size_t)GRD_UNIT_LEN);
					break;
				case 1:
					GMT_memset (h->y_units, GRD_UNIT_LEN, char);
					if (strlen(ptr) >= GRD_UNIT_LEN) GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: Y unit string exceeds upper length of %d characters (truncated)\n", GRD_UNIT_LEN);
					strncpy (h->y_units, ptr, (size_t)GRD_UNIT_LEN);
					break;
				case 2:
					GMT_memset (h->z_units, GRD_UNIT_LEN, char);
					if (strlen(ptr) >= GRD_UNIT_LEN) GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: Z unit string exceeds upper length of %d characters (truncated)\n", GRD_UNIT_LEN);
					strncpy (h->z_units, ptr, (size_t)GRD_UNIT_LEN);
					break;
				case 3:
					h->z_scale_factor = atof (ptr);
					break;
				case 4:
					h->z_add_offset = atof (ptr);
					break;
				case 5:
					if (strlen(ptr) >= GRD_TITLE_LEN) GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: Title string exceeds upper length of %d characters (truncated)\n", GRD_TITLE_LEN);
					strncpy (h->title, ptr, (size_t)GRD_TITLE_LEN);
					break;
				case 6:
					if (strlen(ptr) >= GRD_REMARK_LEN) GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: Remark string exceeds upper length of %d characters (truncated)\n", GRD_REMARK_LEN);
					strncpy (h->remark, ptr, (size_t)GRD_REMARK_LEN);
					break;
				default:
					break;
			}
		}
		entry++;
	}
	return;
}

GMT_LONG GMT_open_grd (struct GMT_CTRL *C, char *file, struct GMT_GRDFILE *G, char mode)
{
	/* Assumes header contents is already known.  For writing we
	 * assume that the header has already been written.  We fill
	 * the GRD_FILE structure with all the required information.
	 * mode can be w or r.  Upper case W or R refers to headerless
	 * grdraster-type files.
	 */

	GMT_LONG r_w, err, header = TRUE, magic = TRUE;
	int cdf_mode[3] = { NC_NOWRITE, NC_WRITE, NC_WRITE};
	char *bin_mode[3] = { "rb", "rb+", "wb"};
	EXTERN_MSC GMT_LONG GMT_nc_grd_info (struct GMT_CTRL *C, struct GRD_HEADER *header, char job);

	if (mode == 'r' || mode == 'R') {	/* Open file for reading */
		if (mode == 'R') header = FALSE;
		r_w = 0;
	}
	else if (mode == 'W') {
		r_w = 2;
		header = magic = FALSE;
	}
	else
		r_w = 1;
 	GMT_err_trap (GMT_grd_get_format (C, file, &G->header, magic));
	if (GMT_is_dnan(G->header.z_scale_factor))
		G->header.z_scale_factor = 1.0;
	else if (G->header.z_scale_factor == 0.0) {
		G->header.z_scale_factor = 1.0;
		GMT_report (C, GMT_MSG_FATAL, "GMT Warning: scale_factor should not be 0. Reset to 1.\n");
	}
	if (GMT_grdformats[G->header.type][0] == 'c') {		/* Open netCDF file, old format */
		GMT_err_trap (nc_open (G->header.name, cdf_mode[r_w], &G->fid));
		if (header) GMT_nc_grd_info (C, &G->header, mode);
		G->edge[0] = G->header.nx;
		G->start[0] = G->start[1] = G->edge[1] = 0;
	}
	else if (GMT_grdformats[G->header.type][0] == 'n') {		/* Open netCDF file, COARDS-compliant format */
		GMT_err_trap (nc_open (G->header.name, cdf_mode[r_w], &G->fid));
		if (header) GMT_nc_grd_info (C, &G->header, mode);
		G->edge[0] = 1;
		G->edge[1] = G->header.nx;
		G->start[0] = G->header.ny-1;
		G->start[1] = 0;
	}
	else {				/* Regular binary file with/w.o standard GMT header */
		if (r_w == 0 && (G->fp = GMT_fopen (C, G->header.name, bin_mode[0])) == NULL)
			return (GMT_GRDIO_OPEN_FAILED);
		else if ((G->fp = GMT_fopen (C, G->header.name, bin_mode[r_w])) == NULL)
			return (GMT_GRDIO_CREATE_FAILED);
		if (header && GMT_fseek (G->fp, (long)GRD_HEADER_SIZE, SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);
	}

	G->size = GMT_grd_data_size (C, G->header.type, &G->header.nan_value);
	G->check = !GMT_is_dnan (G->header.nan_value);
	G->scale = G->header.z_scale_factor, G->offset = G->header.z_add_offset;

	if (GMT_grdformats[G->header.type][1] == 'm')	/* Bit mask */
		G->n_byte = irint (ceil (G->header.nx / 32.0)) * G->size;
	else if (GMT_grdformats[G->header.type][0] == 'r' && GMT_grdformats[G->header.type][1] == 'b')	/* Sun Raster */
		G->n_byte = irint (ceil (G->header.nx / 2.0)) * 2 * G->size;
	else	/* All other */
		G->n_byte = G->header.nx * G->size;

	G->v_row =  GMT_memory (C, NULL, G->n_byte, char);

	G->row = 0;
	G->auto_advance = TRUE;	/* Default is to read sequential rows */
	return (GMT_NOERROR);
}

void GMT_close_grd (struct GMT_CTRL *C, struct GMT_GRDFILE *G)
{
	GMT_free (C, G->v_row);
	if (GMT_grdformats[G->header.type][0] == 'c' || GMT_grdformats[G->header.type][0] == 'n')
		nc_close (G->fid);
	else
		GMT_fclose (C, G->fp);
}

GMT_LONG GMT_read_grd_row (struct GMT_CTRL *C, struct GMT_GRDFILE *G, GMT_LONG row_no, float *row)
{	/* Reads the entire row vector form the grdfile
	 * If row_no is negative it is interpreted to mean that we want to
	 * fseek to the start of the abs(row_no) record and no reading takes place.
	 */

	GMT_LONG i, err;

	if (GMT_grdformats[G->header.type][0] == 'c') {		/* Get one NetCDF row, old format */
		if (row_no < 0) {	/* Special seek instruction */
			G->row = GMT_abs (row_no);
			G->start[0] = G->row * G->edge[0];
			return (GMT_NOERROR);
		}
		GMT_err_trap (nc_get_vara_float (G->fid, G->header.z_id, G->start, G->edge, row));
		if (G->auto_advance) G->start[0] += G->edge[0];
	}
	else if (GMT_grdformats[G->header.type][0] == 'n') {	/* Get one NetCDF row, COARDS-compliant format */
		if (row_no < 0) {	/* Special seek instruction */
			G->row = GMT_abs (row_no);
			G->start[0] = G->header.ny - 1 - G->row;
			return (GMT_NOERROR);
		}
		GMT_err_trap (nc_get_vara_float (G->fid, G->header.z_id, G->start, G->edge, row));
		if (G->auto_advance) G->start[0] --;
	}
	else {			/* Get a binary row */
		if (row_no < 0) {	/* Special seek instruction */
			G->row = GMT_abs (row_no);
			if (GMT_fseek (G->fp, (long)(GRD_HEADER_SIZE + G->row * G->n_byte), SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);
			return (GMT_NOERROR);
		}
		if (!G->auto_advance && GMT_fseek (G->fp, (long)(GRD_HEADER_SIZE + G->row * G->n_byte), SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);

		if (GMT_fread (G->v_row, (size_t)G->size, (size_t)G->header.nx, G->fp) != (size_t)G->header.nx)  return (GMT_GRDIO_READ_FAILED);	/* Get one row */
		for (i = 0; i < G->header.nx; i++) {
			row[i] = GMT_decode (C, G->v_row, i, GMT_grdformats[G->header.type][1]);	/* Convert whatever to float */
			if (G->check && row[i] == G->header.nan_value) row[i] = C->session.f_NaN;
		}
	}
	GMT_grd_do_scaling (row, (GMT_LONG)G->header.nx, G->scale, G->offset);
	G->row++;
	return (GMT_NOERROR);
}

GMT_LONG GMT_write_grd_row (struct GMT_CTRL *C, struct GMT_GRDFILE *G, GMT_LONG row_no, float *row)
{	/* Writes the entire row vector to the grdfile */

	GMT_LONG i, size, err;
	void *tmp = NULL;

	size = GMT_grd_data_size (C, G->header.type, &G->header.nan_value);
	tmp = GMT_memory (C, NULL, G->header.nx * size, char);

	GMT_grd_do_scaling (row, (GMT_LONG)G->header.nx, G->scale, G->offset);
	for (i = 0; i < G->header.nx; i++) if (GMT_is_fnan (row[i]) && G->check) row[i] = (float)G->header.nan_value;

	switch (GMT_grdformats[G->header.type][0]) {
		case 'c':
			GMT_err_trap (nc_put_vara_float (G->fid, G->header.z_id, G->start, G->edge, row));
			if (G->auto_advance) G->start[0] += G->edge[0];
			break;
		case 'n':
			GMT_err_trap (nc_put_vara_float (G->fid, G->header.z_id, G->start, G->edge, row));
			if (G->auto_advance) G->start[0] --;
			break;
		default:
			for (i = 0; i < G->header.nx; i++) GMT_encode (C, tmp, i, row[i], GMT_grdformats[G->header.type][1]);
			if (GMT_fwrite (tmp, (size_t)size, (size_t)G->header.nx, G->fp) < (size_t)G->header.nx) return (GMT_GRDIO_WRITE_FAILED);
	}

	GMT_free (C, tmp);
	return (GMT_NOERROR);
}

void GMT_set_grddim (struct GMT_CTRL *C, struct GRD_HEADER *h)
{	/* Assumes pad is set and then computes nx, ny, mx, my, nm, size, xy_off based on w/e/s/n.  */
	h->nx = GMT_grd_get_nx (h);		/* Set nx, ny based on w/e/s/n and offset */
	h->ny = GMT_grd_get_ny (h);
	h->mx = GMT_grd_get_nxpad (h, h->pad);	/* Set mx, my based on h->{nx,ny} and the current pad */
	h->my = GMT_grd_get_nypad (h, h->pad);
	h->nm = GMT_grd_get_nm (h);		/* Sets the number of actual data items */
	h->size = GMT_grd_get_size (C, h);	/* Sets the nm items needed to hold this array */
	h->xy_off = 0.5 * h->registration;
}

void GMT_grd_init (struct GMT_CTRL *C, struct GRD_HEADER *header, struct GMT_OPTION *options, GMT_LONG update)
{	/* GMT_grd_init initializes a grd header to default values and copies the
	 * options to the header variable command.
	 * update = TRUE if we only want to update command line */
	GMT_LONG i, len;

	if (update)	/* Only clean the command history */
		GMT_memset (header->command, GRD_COMMAND_LEN, char);
	else {		/* Wipe the slate clean */
		GMT_memset (header, 1, struct GRD_HEADER);

		/* Set the variables that are not initialized to 0/FALSE/NULL */
		header->z_scale_factor		= 1.0;
		header->type			= -1;
		header->y_order			= 1;
		header->z_id			= -1;
		header->z_min			= C->session.d_NaN;
		header->z_max			= C->session.d_NaN;
		header->nan_value		= C->session.d_NaN;
		strcpy (header->x_units, "x");
		strcpy (header->y_units, "y");
		strcpy (header->z_units, "z");
		for (i = 0; i < 3; i++) header->t_index[i] = -1;
		GMT_grd_setpad (header, C->current.io.pad);	/* Assign default pad */
	}

	/* Always update command line history, if given */

	if (options) {
		struct GMTAPI_CTRL *API = C->parent;
		GMT_LONG argc; char **argv;
		
		GMT_Create_Args (API, &argc, &argv, options);
		strcpy (header->command, C->init.progname);
		len = strlen (header->command);
		for (i = 0; len < GRD_COMMAND_LEN && i < argc; i++) {
			len += strlen (argv[i]) + 1;
			if (len > GRD_COMMAND_LEN) continue;
			strcat (header->command, " ");
			strcat (header->command, argv[i]);
		}
		header->command[len] = 0;
		GMT_Destroy_Args (API, argc, argv);
	}
}

void GMT_grd_shift (struct GMT_CTRL *C, struct GMT_GRID *G, double shift)
{
	/* Rotate geographical, global grid in e-w direction
	 * This function will shift a grid by shift degrees */

	GMT_LONG col, row, k, ij, n_shift, width, n_warn = 0;
	float *tmp = NULL;

	n_shift = irint (shift / G->header->inc[GMT_X]);
	width = irint (360.0 / G->header->inc[GMT_X]);
	if (width > G->header->nx) {
		GMT_report (C, GMT_MSG_FATAL, "Error: Cannot rotate grid, width is too small\n");
		return;
	}

	tmp = GMT_memory (C, NULL, G->header->nx, float);

	for (row = 0; row < G->header->ny; row++) {
		ij = GMT_IJP (G->header, row, 0);
		if (width < G->header->nx && G->data[ij] != G->data[ij+width]) n_warn++;
		for (col = 0; col < G->header->nx; col++) {
			k = (col - n_shift) % width;
			if (k < 0) k += width;
			tmp[k] = G->data[ij+col];
		}
		GMT_memcpy (&G->data[ij], tmp, G->header->nx, float);
	}
	GMT_free (C, tmp);

	/* Shift boundaries */

	G->header->wesn[XLO] += shift;
	G->header->wesn[XHI] += shift;
	if (G->header->wesn[XHI] < 0.0) {
		G->header->wesn[XLO] += 360.0;
		G->header->wesn[XHI] += 360.0;
	}
	else if (G->header->wesn[XHI] > 360.0) {
		G->header->wesn[XLO] -= 360.0;
		G->header->wesn[XHI] -= 360.0;
	}

	if (n_warn) GMT_report (C, GMT_MSG_FATAL, "Gridline-registered global grid has inconsistent values at repeated node for %ld rows\n", n_warn);
}

GMT_LONG GMT_grd_is_global (struct GMT_CTRL *C, struct GRD_HEADER *h)
{	/* Determine if grid could be global */

	if (GMT_x_is_lon (C, GMT_IN)) {
		if (fabs (h->wesn[XHI] - h->wesn[XLO] - 360.0) < GMT_SMALL) {
			GMT_report (C, GMT_MSG_VERBOSE, "GMT_grd_is_global: yes, longitudes span exactly 360\n");
			return (TRUE);
		}
		else if (fabs (h->nx * h->inc[GMT_X] - 360.0) < GMT_SMALL) {
			GMT_report (C, GMT_MSG_VERBOSE, "GMT_grd_is_global: yes, longitude cells span exactly 360\n");
			return (TRUE);
		}
		else if ((h->wesn[XHI] - h->wesn[XLO]) > 360.0) {
			GMT_report (C, GMT_MSG_VERBOSE, "GMT_grd_is_global: yes, longitudes span more than 360\n");
			return (TRUE);
		}
	}
	else if (h->wesn[YLO] >= -90.0 && h->wesn[YHI] <= 90.0) {
		if (fabs (h->wesn[XHI] - h->wesn[XLO] - 360.0) < GMT_SMALL) {
			GMT_report (C, GMT_MSG_FATAL, "GMT_grd_is_global: probably, x spans exactly 360 and -90 <= y <= 90\n");
			GMT_report (C, GMT_MSG_FATAL, "     To make sure the grid is recognized as geographical and global, use the -fg option\n");
			return (FALSE);
		}
		else if (fabs (h->nx * h->inc[GMT_X] - 360.0) < GMT_SMALL) {
			GMT_report (C, GMT_MSG_FATAL, "GMT_grd_is_global: probably, x cells span exactly 360 and -90 <= y <= 90\n");
			GMT_report (C, GMT_MSG_FATAL, "     To make sure the grid is recognized as geographical and global, use the -fg option\n");
			return (FALSE);
		}
	}
	GMT_report (C, GMT_MSG_NORMAL, "GMT_grd_is_global: no!\n");
	return (FALSE);
}

GMT_LONG GMT_grd_setregion (struct GMT_CTRL *C, struct GRD_HEADER *h, double *wesn, GMT_LONG interpolant)
{
	/* GMT_grd_setregion determines what w,e,s,n should be passed to GMT_read_grd.
	 * It does so by using C->common.R.wesn which have been set correctly by map_setup.
	 * Use interpolant to indicate if (and how) the grid is interpolated after this call.
	 * This determines possible extension of the grid to allow interpolation (without padding).
	 *
	 * Here are some considerations about the boundary we need to match, assuming the grid is gridline oriented:
	 * - When the output is to become pixels, the outermost point has to be beyond 0.5 cells inside the region
	 * - When linear interpolation is needed afterwards, the outermost point needs to be on the region edge or beyond
	 * - When the grid is pixel oriented, the limits need to go outward by another 0.5 cells
	 * - When the region is global, do not extend the longitudes outward (otherwise you create wrap-around issues)
	 * So to determine the boundary, we go inward from there.
	 */

	GMT_LONG grid_global;
	double shift_x, x_range, off;

	/* First make an educated guess whether the grid and region are geographical and global */
	grid_global = GMT_grd_is_global (C, h);

	switch (interpolant) {
		case BCR_BILINEAR:
			off = 0.0;
			break;
		case BCR_BSPLINE:
		case BCR_BICUBIC:
			off = 1.5;
			break;
		default:
			off = -0.5;
			break;
	}
	if (h->registration == GMT_PIXEL_REG) off += 0.5;
	/* Initial assignment of wesn */
	wesn[YLO] = C->common.R.wesn[YLO] - off * h->inc[GMT_Y], wesn[YHI] = C->common.R.wesn[YHI] + off * h->inc[GMT_Y];
	if (GMT_360_RANGE (C->common.R.wesn[XLO], C->common.R.wesn[XHI]) && GMT_x_is_lon (C, GMT_IN)) off = 0.0;
	wesn[XLO] = C->common.R.wesn[XLO] - off * h->inc[GMT_X], wesn[XHI] = C->common.R.wesn[XHI] + off * h->inc[GMT_X];

	if (C->common.R.oblique && !GMT_IS_RECT_GRATICULE (C)) {	/* Used -R... with oblique boundaries - return entire grid */
		if (wesn[XHI] < h->wesn[XLO])	/* Make adjustments so C->current.proj.[w,e] jives with h->wesn */
			shift_x = 360.0;
		else if (wesn[XLO] > h->wesn[XHI])
			shift_x = -360.0;
		else
			shift_x = 0.0;

		wesn[XLO] = h->wesn[XLO] + irint ((wesn[XLO] - h->wesn[XLO] + shift_x) / h->inc[GMT_X]) * h->inc[GMT_X];
		wesn[XHI] = h->wesn[XHI] + irint ((wesn[XHI] - h->wesn[XLO] + shift_x) / h->inc[GMT_X]) * h->inc[GMT_X];
		wesn[YLO] = h->wesn[YLO] + irint ((wesn[YLO] - h->wesn[YLO]) / h->inc[GMT_Y]) * h->inc[GMT_Y];
		wesn[YHI] = h->wesn[YHI] + irint ((wesn[YHI] - h->wesn[YLO]) / h->inc[GMT_Y]) * h->inc[GMT_Y];

		/* Make sure we do not exceed grid domain (which can happen if C->common.R.wesn exceeds the grid limits) */
		if (wesn[XLO] < h->wesn[XLO] && !grid_global) wesn[XLO] = h->wesn[XLO];
		if (wesn[XHI] > h->wesn[XHI] && !grid_global) wesn[XHI] = h->wesn[XHI];
		if (wesn[YLO] < h->wesn[YLO]) wesn[YLO] = h->wesn[YLO];
		if (wesn[YHI] > h->wesn[YHI]) wesn[YHI] = h->wesn[YHI];

		/* If North or South pole are within the map boundary, we need all longitudes but restrict latitudes */
		if (!C->current.map.outside (C, 0.0, +90.0)) wesn[XLO] = h->wesn[XLO], wesn[XHI] = h->wesn[XHI], wesn[YHI] = h->wesn[YHI];
		if (!C->current.map.outside (C, 0.0, -90.0)) wesn[XLO] = h->wesn[XLO], wesn[XHI] = h->wesn[XHI], wesn[YLO] = h->wesn[YLO];
		return (0);
	}

	/* First set and check latitudes since they have no complications */
	wesn[YLO] = MAX (h->wesn[YLO], h->wesn[YLO] + floor ((wesn[YLO] - h->wesn[YLO]) / h->inc[GMT_Y] + GMT_SMALL) * h->inc[GMT_Y]);
	wesn[YHI] = MIN (h->wesn[YHI], h->wesn[YLO] + ceil  ((wesn[YHI] - h->wesn[YLO]) / h->inc[GMT_Y] - GMT_SMALL) * h->inc[GMT_Y]);

	if (wesn[YHI] <= wesn[YLO]) {	/* Grid must be outside chosen -R */
		if (C->current.setting.verbose) GMT_report (C, GMT_MSG_FATAL, "Your grid y's or latitudes appear to be outside the map region and will be skipped.\n");
		return (1);
	}

	/* Periodic grid with 360 degree range is easy */

	if (grid_global) {
		wesn[XLO] = h->wesn[XLO] + floor ((wesn[XLO] - h->wesn[XLO]) / h->inc[GMT_X] + GMT_SMALL) * h->inc[GMT_X];
		wesn[XHI] = h->wesn[XLO] + ceil  ((wesn[XHI] - h->wesn[XLO]) / h->inc[GMT_X] - GMT_SMALL) * h->inc[GMT_X];
		/* For the odd chance that xmin or xmax are outside the region: bring them in */
		if (wesn[XHI] - wesn[XLO] >= 360.0) {
			while (wesn[XLO] < C->common.R.wesn[XLO]) wesn[XLO] += h->inc[GMT_X];
			while (wesn[XHI] > C->common.R.wesn[XHI]) wesn[XHI] -= h->inc[GMT_X];
		}
		return (0);
	}

	/* Shift a geographic grid 360 degrees up or down to maximize the amount of longitude range */

	if (C->current.io.col_type[GMT_IN][GMT_X] == GMT_IS_LON) {
		x_range = MIN (wesn[XLO], h->wesn[XHI]) - MAX (wesn[XHI], h->wesn[XLO]);
		if (MIN (wesn[XLO], h->wesn[XHI] + 360.0) - MAX (wesn[XHI], h->wesn[XLO] + 360.0) > x_range)
			shift_x = 360.0;
		else if (MIN (wesn[XLO], h->wesn[XHI] - 360.0) - MAX (wesn[XHI], h->wesn[XLO] - 360.0) > x_range)
			shift_x = -360.0;
		else
			shift_x = 0.0;
		h->wesn[XLO] += shift_x;
		h->wesn[XHI] += shift_x;
	}

	wesn[XLO] = MAX (h->wesn[XLO], h->wesn[XLO] + floor ((wesn[XLO] - h->wesn[XLO]) / h->inc[GMT_X] + GMT_SMALL) * h->inc[GMT_X]);
	wesn[XHI] = MIN (h->wesn[XHI], h->wesn[XLO] + ceil  ((wesn[XHI] - h->wesn[XLO]) / h->inc[GMT_X] - GMT_SMALL) * h->inc[GMT_X]);

	if (wesn[XHI] <= wesn[XLO]) {	/* Grid is outside chosen -R in longitude */
		if (C->current.setting.verbose) GMT_report (C, GMT_MSG_FATAL, "Your grid x's or longitudes appear to be outside the map region and will be skipped.\n");
		return (1);
	}
	return (0);
}

GMT_LONG GMT_adjust_loose_wesn (struct GMT_CTRL *C, double wesn[], struct GRD_HEADER *header)
{
	/* Used to ensure that sloppy w,e,s,n values are rounded to the gridlines or pixels in the referenced grid.
	 * Upon entry, the boundaries w,e,s,n are given as a rough approximation of the actual subset needed.
	 * The routine will limit the boundaries to the grids region and round w,e,s,n to the nearest gridline or
	 * pixel boundaries (depending on the grid orientation).
	 * Warnings are produced when the w,e,s,n boundaries are adjusted, so this routine is currently not
	 * intended to throw just any values at it (although one could).
	 */
	
	GMT_LONG global, error = FALSE;
	double half_or_zero, val, dx, small;
	
	half_or_zero = (header->registration == GMT_PIXEL_REG) ? 0.5 : 0.0;

	switch (GMT_minmaxinc_verify (C, wesn[XLO], wesn[XHI], header->inc[GMT_X], GMT_SMALL)) {	/* Check if range is compatible with x_inc */
		case 3:
			return (GMT_GRDIO_BAD_XINC);
			break;
		case 2:
			return (GMT_GRDIO_BAD_XRANGE);
			break;
		default:
			/* Everything is seemingly OK */
			break;
	}
	switch (GMT_minmaxinc_verify (C, wesn[YLO], wesn[YHI], header->inc[GMT_Y], GMT_SMALL)) {	/* Check if range is compatible with y_inc */
		case 3:
			return (GMT_GRDIO_BAD_YINC);
			break;
		case 2:
			return (GMT_GRDIO_BAD_YRANGE);
			break;
		default:
			/* Everything is OK */
			break;
	}
	global = GMT_grd_is_global (C, header);

	if (!global) {
		if (wesn[XLO] < header->wesn[XLO]) { wesn[XLO] = header->wesn[XLO]; error = TRUE; }
		if (wesn[XHI] > header->wesn[XHI]) { wesn[XHI] = header->wesn[XHI]; error = TRUE; }
	}
	if (wesn[YLO] < header->wesn[YLO]) { wesn[YLO] = header->wesn[YLO]; error = TRUE; }
	if (wesn[YHI] > header->wesn[YHI]) { wesn[YHI] = header->wesn[YHI]; error = TRUE; }
	if (error) GMT_report (C, GMT_MSG_FATAL, "Warning: Subset exceeds data domain. Subset reduced to common region.\n");
	error = FALSE;

	if (!(C->current.io.col_type[GMT_IN][GMT_X] == GMT_IS_LON && GMT_360_RANGE (wesn[XLO], wesn[XHI]) && global)) {    /* Do this unless a 360 longitude wrap */
		small = GMT_SMALL * header->inc[GMT_X];

		val = header->wesn[XLO] + irint ((wesn[XLO] - header->wesn[XLO]) / header->inc[GMT_X]) * header->inc[GMT_X];
		dx = fabs (wesn[XLO] - val);
		if (C->current.io.col_type[GMT_IN][GMT_X] == GMT_IS_LON) dx = fmod (dx, 360.0);
		if (dx > small) {
			wesn[XLO] = val;
			GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: (w - x_min) must equal (NX + eps) * x_inc), where NX is an integer and |eps| <= %g.\n", GMT_SMALL);
			GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: w reset to %g\n", wesn[XLO]);
		}

		val = header->wesn[XLO] + irint ((wesn[XHI] - header->wesn[XLO]) / header->inc[GMT_X]) * header->inc[GMT_X];
		dx = fabs (wesn[XHI] - val);
		if (C->current.io.col_type[GMT_IN][GMT_X] == GMT_IS_LON) dx = fmod (dx, 360.0);
		if (dx > GMT_SMALL) {
			wesn[XHI] = val;
			GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: (e - x_min) must equal (NX + eps) * x_inc), where NX is an integer and |eps| <= %g.\n", GMT_SMALL);
			GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: e reset to %g\n", wesn[XHI]);
		}
	}

	/* Check if s,n are a multiple of y_inc offset from y_min - if not adjust s, n */
	small = GMT_SMALL * header->inc[GMT_Y];

	val = header->wesn[YLO] + irint ((wesn[YLO] - header->wesn[YLO]) / header->inc[GMT_Y]) * header->inc[GMT_Y];
	if (fabs (wesn[YLO] - val) > small) {
		wesn[YLO] = val;
		GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: (s - y_min) must equal (NY + eps) * y_inc), where NY is an integer and |eps| <= %g.\n", GMT_SMALL);
		GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: s reset to %g\n", wesn[YLO]);
	}

	val = header->wesn[YLO] + irint ((wesn[YHI] - header->wesn[YLO]) / header->inc[GMT_Y]) * header->inc[GMT_Y];
	if (fabs (wesn[YHI] - val) > small) {
		wesn[YHI] = val;
		GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: (n - y_min) must equal (NY + eps) * y_inc), where NY is an integer and |eps| <= %g.\n", GMT_SMALL);
		GMT_report (C, GMT_MSG_FATAL, "GMT WARNING: n reset to %g\n", wesn[YHI]);
	}
	return (GMT_NOERROR);
}

GMT_LONG GMT_read_img (struct GMT_CTRL *C, char *imgfile, struct GMT_GRID *Grid, double *in_wesn, double scale, GMT_LONG mode, double lat, GMT_LONG init)
{
	/* Function that reads an entire Sandwell/Smith Mercator grid and stores it like a regular
	 * GMT grid.  If init is TRUE we also initialize the Mercator projection.  Lat should be 0.0
	 * if we are dealing with standard 72 or 80 img latitude; else it must be specified.
	 */

	GMT_LONG min, i, j, k, ij, first_i, n_skip, n_cols, status;
	short int *i2 = NULL;
	char file[BUFSIZ];
	struct STAT buf;
	FILE *fp = NULL;
	double wesn[4], wesn_all[4];

	if (!GMT_getdatapath (C, imgfile, file)) return (GMT_GRDIO_FILE_NOT_FOUND);
	if (STAT (file, &buf)) return (GMT_GRDIO_STAT_FAILED);	/* Inquiry about file failed somehow */

	switch (buf.st_size) {	/* Known sizes are 1 or 2 min at lat_max = ~72 or ~80 */
		case GMT_IMG_NLON_1M*GMT_IMG_NLAT_1M_80*GMT_IMG_ITEMSIZE:
			if (lat == 0.0) lat = GMT_IMG_MAXLAT_80;
		case GMT_IMG_NLON_1M*GMT_IMG_NLAT_1M_72*GMT_IMG_ITEMSIZE:
			if (lat == 0.0) lat = GMT_IMG_MAXLAT_72;
			min = 1;
			break;
		case GMT_IMG_NLON_2M*GMT_IMG_NLAT_2M_80*GMT_IMG_ITEMSIZE:
			if (lat == 0.0) lat = GMT_IMG_MAXLAT_80;
		case GMT_IMG_NLON_2M*GMT_IMG_NLAT_2M_72*GMT_IMG_ITEMSIZE:
			if (lat == 0.0) lat = GMT_IMG_MAXLAT_72;
			min = 2;
			break;
		default:
			if (lat == 0.0) return (GMT_GRDIO_BAD_IMG_LAT);
			min = (buf.st_size > GMT_IMG_NLON_2M*GMT_IMG_NLAT_2M_80*GMT_IMG_ITEMSIZE) ? 1 : 2;
			GMT_report (C, GMT_MSG_FATAL, "img file %s has unusual size - grid increment defaults to %ld min\n", file, min);
			break;
	}

	wesn_all[XLO] = GMT_IMG_MINLON;	wesn_all[XHI] = GMT_IMG_MAXLON;
	wesn_all[YLO] = -lat;		wesn_all[YHI] = lat;
	if (!in_wesn || (in_wesn[XLO] == in_wesn[XHI] && in_wesn[YLO] == in_wesn[YHI])) {	/* Default is entire grid */
		GMT_memcpy (wesn, wesn_all, 4, double);
	}
	else	/* Use specified subset */
		GMT_memcpy (wesn, in_wesn, 4, double);


	if ((fp = GMT_fopen (C, file, "rb")) == NULL) return (GMT_GRDIO_OPEN_FAILED);
	
	GMT_grd_init (C, Grid->header, NULL, FALSE);
	Grid->header->inc[GMT_X] = Grid->header->inc[GMT_Y] = min / 60.0;
	
	if (init) {
		/* Select plain Mercator on a sphere with -Jm1 -R0/360/-lat/+lat */
		C->current.setting.proj_ellipsoid = GMT_get_ellipsoid (C, "Sphere");
		C->current.proj.units_pr_degree = TRUE;
		C->current.proj.pars[0] = 180.0;
		C->current.proj.pars[1] = 0.0;
		C->current.proj.pars[2] = 1.0;
		C->current.proj.projection = GMT_MERCATOR;
		C->current.io.col_type[GMT_IN][GMT_X] = GMT_IS_LON;
		C->current.io.col_type[GMT_IN][GMT_Y] = GMT_IS_LAT;
		C->common.J.active = TRUE;

		GMT_err_pass (C, GMT_map_setup (C, wesn_all), file);
	}

	if (wesn[XLO] < 0.0 && wesn[XHI] < 0.0) wesn[XLO] += 360.0, wesn[XHI] += 360.0;

	/* Project lon/lat boundaries to Mercator units */
	GMT_geo_to_xy (C, wesn[XLO], wesn[YLO], &Grid->header->wesn[XLO], &Grid->header->wesn[YLO]);
	GMT_geo_to_xy (C, wesn[XHI], wesn[YHI], &Grid->header->wesn[XHI], &Grid->header->wesn[YHI]);

	/* Adjust boundaries to multiples of increments, making sure we are inside bounds */
	Grid->header->wesn[XLO] = MAX (GMT_IMG_MINLON, floor (Grid->header->wesn[XLO] / Grid->header->inc[GMT_X]) * Grid->header->inc[GMT_X]);
	Grid->header->wesn[XHI] = MIN (GMT_IMG_MAXLON, ceil (Grid->header->wesn[XHI] / Grid->header->inc[GMT_X]) * Grid->header->inc[GMT_X]);
	if (Grid->header->wesn[XLO] > Grid->header->wesn[XHI]) Grid->header->wesn[XLO] -= 360.0;
	Grid->header->wesn[YLO] = MAX (0.0, floor (Grid->header->wesn[YLO] / Grid->header->inc[GMT_Y]) * Grid->header->inc[GMT_Y]);
	Grid->header->wesn[YHI] = MIN (C->current.proj.rect[YHI], ceil (Grid->header->wesn[YHI] / Grid->header->inc[GMT_Y]) * Grid->header->inc[GMT_Y]);
	/* Allocate grid memory */

	Grid->header->registration = GMT_PIXEL_REG;	/* These are always pixel grids */
	if ((status = GMT_grd_RI_verify (C, Grid->header, 1))) return (status);	/* Final verification of -R -I; return error if we must */
	GMT_grd_setpad (Grid->header, C->current.io.pad);			/* Assign default pad */
	GMT_set_grddim (C, Grid->header);					/* Set all dimensions before returning */
	Grid->data = GMT_memory (C, NULL, Grid->header->size, float);

	n_cols = (min == 1) ? GMT_IMG_NLON_1M : GMT_IMG_NLON_2M;		/* Number of columns (10800 or 21600) */
	first_i = (GMT_LONG)floor (Grid->header->wesn[XLO] / Grid->header->inc[GMT_X]);				/* first tile partly or fully inside region */
	if (first_i < 0) first_i += n_cols;
	n_skip = (GMT_LONG)floor ((C->current.proj.rect[YHI] - Grid->header->wesn[YHI]) / Grid->header->inc[GMT_Y]);	/* Number of rows clearly above y_max */
	if (GMT_fseek (fp, (long)(n_skip * n_cols * GMT_IMG_ITEMSIZE), SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);

	i2 = GMT_memory (C, NULL, n_cols, short int);
	for (j = 0; j < Grid->header->ny; j++) {	/* Read all the rows, offset by 2 boundary rows and cols */
		if (GMT_fread ((void *)i2, sizeof (short int), (size_t)n_cols, fp) != (size_t)n_cols)  return (GMT_GRDIO_READ_FAILED);	/* Get one row */
#if defined(_WIN32) || WORDS_BIGENDIAN == 0
		for (i = 0; i < n_cols; i++) i2[i] = GMT_swab2 (i2[i]);
#endif
		ij = GMT_IJP (Grid->header, j, 0);
		for (i = 0, k = first_i; i < Grid->header->nx; i++) {	/* Process this row's values */
			switch (mode) {
				case 0:	/* No encoded track flags, do nothing */
					break;
				case 1:	/* Remove the track flag on odd (constrained) points */
					if (i2[k]%2) i2[k]--;
					break;
				case 2:	/* Remove the track flag on odd (constrained) points and set unconstrained to NaN */
					i2[k] = (i2[k]%2) ? i2[k] - 1 : SHRT_MIN;
					break;
				case 3:	/* Set odd (constrained) points to 1 and set unconstrained to 0 */
					i2[k] %= 2;
					break;
			}
			Grid->data[ij+i] = (float)((mode == 3) ? i2[k] : (i2[k] * scale));
			if (++k == n_cols) k = 0;	/* Wrapped around 360 */
		}
	}
	GMT_free (C, i2);
	GMT_fclose (C, fp);
	if (init) {
		GMT_memcpy (C->common.R.wesn, wesn, 4, double);
		C->common.J.active = FALSE;
	}
	return (GMT_NOERROR);
}

void GMT_grd_pad_off (struct GMT_CTRL *C, struct GMT_GRID *G)
{	/* Shifts the grid contents so there is no pad.  The remainder of
 	 * the array is not reset and should not be addressed.
	 * If pad is zero then we do nothing.
	 */
	GMT_LONG row, ijp, ij0;
	
	if (!GMT_grd_pad_status (G->header, NULL)) return;	/* No pad so nothing to do */
	/* Here, G has a pad which we need to eliminate */
	for (row = 0; row < G->header->ny; row++) {
		ijp = GMT_IJP (G->header, row, 0);	/* Index of start of this row's first column in padded grid  */
		ij0 = GMT_IJ0 (G->header, row, 0);	/* Index of start of this row's first column in unpadded grid */
		GMT_memcpy (&(G->data[ij0]), &(G->data[ijp]), G->header->nx, float);	/* Only copy the nx data values */
	}
	GMT_memset (G->header->pad, 4, GMT_LONG);	/* Pad is no longer active */
}

void GMT_grd_pad_on (struct GMT_CTRL *C, struct GMT_GRID *G, GMT_LONG *pad)
{	/* Shift grid content from a non-padded (or differently padded) to a padded organization.
 	 * We check that the grid size can handle this and allocate more space if needed.
	 * If pad matches the grid's pad then we do nothing.
	 */
	GMT_LONG row, ijp, ij0, size;
	struct GRD_HEADER *h = NULL;
	
	if (GMT_grd_pad_status (G->header, pad)) return;	/* Already padded as requested so nothing to do */
	/* Here the pads differ (or G has no pad at all) */
	size = GMT_grd_get_nxpad (G->header, pad) * GMT_grd_get_nypad (G->header, pad);
	if (size > G->header->size) {	/* Must allocate more space */
		G->data = GMT_memory (C, G->data, size, float);
		G->header->size = size;
	}
	/* Because G may have a pad that is nonzero (but different from pad) we need a different header structure in the macros below */
	h = GMT_duplicate_gridheader (C, G->header);
	
	GMT_grd_setpad (G->header, pad);		/* Pad is now active and set to specified dimensions */
	GMT_set_grddim (C, G->header);			/* Update all dimensions to reflect the padding */
	for (row = G->header->ny-1; row >= 0; row--) {
		ijp = GMT_IJP (G->header, row, 0);	/* Index of start of this row's first column in padded grid  */
		ij0 = GMT_IJ0 (h, row, 0);		/* Index of start of this row's first column in unpadded grid */
		GMT_memcpy (&(G->data[ijp]), &(G->data[ij0]), G->header->nx, float);
	}
	GMT_free (C, h);	/* Done with this header */
}

void GMT_grd_pad_zero (struct GMT_CTRL *C, struct GMT_GRID *G)
{	/* Sets all boundary row/col nodes to zero and sets
	 * the header->BC to GMT_IS_NOTSET.
	 */
	GMT_LONG row, kf, kl, k, nx1;
	
	if (!GMT_grd_pad_status (G->header, NULL)) return;	/* No pad so nothing to do */
	if (G->header->BC[XLO] == GMT_BC_IS_NOTSET && G->header->BC[XHI] == GMT_BC_IS_NOTSET && G->header->BC[YLO] == GMT_BC_IS_NOTSET && G->header->BC[YHI] == GMT_BC_IS_NOTSET) return;	/* No BCs set so nothing to do */			/* No pad so nothing to do */
	/* Here, G has a pad with BCs which we need to reset */
	if (G->header->pad[YHI]) GMT_memset (G->data, G->header->pad[YHI] * G->header->mx, float);		/* Zero the top pad */
	nx1 = G->header->nx - 1;	/* Last column */
	GMT_row_loop (G, row) {
		kf = GMT_IJP (G->header, row,   0);				/* Index of first column this row  */
		kl = GMT_IJ0 (G->header, row, nx1);				/* Index of last column this row */
		for (k = 1; k <= G->header->pad[XLO]; k++) G->data[kf-k] = 0.0;	/* Zero the left pad at this row*/
		for (k = 1; k <= G->header->pad[XHI]; k++) G->data[kl+k] = 0.0;	/* Zero the left pad at this row */
	}
	if (G->header->pad[YLO]) {
		kf = GMT_IJP (G->header, G->header->ny, -G->header->pad[XLO]);		/* Index of first column of bottom pad  */
		GMT_memset (&(G->data[kf]), G->header->pad[YLO] * G->header->mx, float);	/* Zero the bottom pad */
	}
	GMT_memset (G->header->BC, 4, GMT_LONG);				/* BCs no longer set for this grid */
}

struct GMT_GRID *GMT_create_grid (struct GMT_CTRL *C)
{	/* Allocates space for a new grid container.  No space allocated for the float grid itself */
	struct GMT_GRID *G = NULL;

	G = GMT_memory (C, NULL, 1, struct GMT_GRID);
	G->header = GMT_memory (C, NULL, 1, struct GRD_HEADER);

	return (G);
}

struct GRD_HEADER *GMT_duplicate_gridheader (struct GMT_CTRL *C, struct GRD_HEADER *h)
{	/* Duplicates a grid header. */
	struct GRD_HEADER *hnew = NULL;

	hnew = GMT_memory (C, NULL, 1, struct GRD_HEADER);
	GMT_memcpy (hnew, h, 1, struct GRD_HEADER);
	return (hnew);
}

struct GMT_GRID *GMT_duplicate_grid (struct GMT_CTRL *C, struct GMT_GRID *G, GMT_LONG alloc_data)
{	/* Duplicates an entire grid, including data. */
	struct GMT_GRID *Gnew = NULL;

	Gnew = GMT_create_grid (C);
	GMT_memcpy (Gnew->header, G->header, 1, struct GRD_HEADER);
	if (alloc_data) {	/* ALso allocate and duplicate data array */
		Gnew->data = GMT_memory (C, NULL, G->header->size, float);
		GMT_memcpy (Gnew->data, G->data, G->header->size, float);
	}
	return (Gnew);
}

void GMT_free_grid (struct GMT_CTRL *C, struct GMT_GRID **G, GMT_LONG free_grid)
{	/* By taking a reference to the grid pointer we can set it to NULL when done */
	if (!(*G)) return;	/* Nothing to deallocate */
	if ((*G)->data && free_grid) GMT_free (C, (*G)->data);
	if ((*G)->header) GMT_free (C, (*G)->header);
	GMT_free (C, *G);
	*G = NULL;
}

GMT_LONG GMT_set_outgrid (struct GMT_CTRL *C, struct GMT_GRID *G, struct GMT_GRID **Out)
{	/* When the input grid is a read-only memory location then we cannot use
	 * the same grid to hold the output results but must allocate a separate
	 * grid.  To avoid wasting memory we try to reuse the input array when
	 * it is possible. We return TRUE when new memory had to be allocated.
	 * Note we duplicate the grid if we must so that Out always has the input
	 * data in it (directly or via the pointer).  */
	
	if (G->alloc_mode == GMT_READONLY) {	/* Cannot store results in the read-only input array */
		*Out = GMT_duplicate_grid (C, G, TRUE);
		(*Out)->alloc_mode = GMT_ALLOCATED;
		return (TRUE);
	}
	/* Here we may overwrite the input grid and just pass the pointer back */
	(*Out) = G;
	return (FALSE);
}

GMT_LONG GMT_init_newgrid (struct GMT_CTRL *C, struct GMT_GRID *Grid, double wesn[], double xinc, double yinc, GMT_LONG registration)
{	/* Does the dirty work of initializing the Grid header and make sure all is correct:
 	 * Make sure -R -I is compatible.
	 * Set all the dimension parameters and pad info. Programs that need to set up a grid from
	 * scratch should use this function to simplify the procedure. */
	GMT_LONG status;
	
	GMT_memcpy (Grid->header->wesn, wesn, 4, double);
	Grid->header->inc[GMT_X] = xinc;	Grid->header->inc[GMT_Y] = yinc;
	Grid->header->registration = (int)registration;
	GMT_RI_prepare (C, Grid->header);	/* Ensure -R -I consistency and set nx, ny in case of meter units etc. */
	if ((status = GMT_grd_RI_verify (C, Grid->header, 1))) return (status);	/* Final verification of -R -I; return error if we must */
	GMT_grd_setpad (Grid->header, C->current.io.pad);	/* Assign default pad */
	GMT_set_grddim (C, Grid->header);	/* Set all dimensions before returning */
	return (GMT_NOERROR);
}

GMT_LONG GMT_change_grdreg (struct GMT_CTRL *C, struct GRD_HEADER *header, GMT_LONG registration)
{
	GMT_LONG old_registration;
	double F;
	/* Adjust the grid header to the selected registration, if different.
	 * In all cases we return the original registration. */
	
	old_registration = header->registration;
	if (old_registration == registration) return (old_registration);	/* Noting to do */
	
	F = (header->registration == GMT_PIXEL_REG) ? 0.5 : -0.5;	/* Pixel will shrink w/e/s/n, gridline will extend */
	header->wesn[XLO] += F * header->inc[GMT_X];
	header->wesn[XHI] -= F * header->inc[GMT_X];
	header->wesn[YLO] += F * header->inc[GMT_Y];
	header->wesn[YHI] -= F * header->inc[GMT_Y];
	
	header->registration = (int)registration;
	header->xy_off = 0.5 * header->registration;
	return (old_registration);
}

void GMT_grd_zminmax (struct GMT_CTRL *C, struct GMT_GRID *G)
{	/* Reset the xmin/zmax values in the header */
	GMT_LONG row, col, node, n = 0;
	
	G->header->z_min = DBL_MAX;	G->header->z_max = -DBL_MAX;
	GMT_grd_loop (G, row, col, node) {
		if (GMT_is_fnan (G->data[node])) continue;
		/* Update z_min, z_max */
		G->header->z_min = MIN (G->header->z_min, (double)G->data[node]);
		G->header->z_max = MAX (G->header->z_max, (double)G->data[node]);
		n++;
	}
	if (n == 0) G->header->z_min = G->header->z_max = C->session.d_NaN;
}
