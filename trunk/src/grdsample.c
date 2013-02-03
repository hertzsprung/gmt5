/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2013 by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis and F. Wobbe
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
 * API functions to support the grdsample application.
 *
 * Brief synopsis: grdsample reads a grid file and evaluates the grid at new grid
 * positions specified by new dx/dy values using a 2-D Taylor expansion of order 3.
 * In order to evaluate derivatives along the edges of the surface, I assume 
 * natural bicubic spline conditions, i.e. both the second and third normal 
 * derivatives are zero, and that the dxdy derivative in the corners are zero, too.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#define THIS_MODULE k_mod_grdsample /* I am grdsample */

#include "gmt.h"

struct GRDSAMPLE_CTRL {
	struct In {
		bool active;
		char *file;
	} In;
	struct G {	/* -G<grdfile> */
		bool active;
		char *file;
	} G;
	struct I {	/* -Idx[/dy] */
		bool active;
		double inc[2];
	} I;
	struct T {	/* -T */
		bool active;
	} T;
};

void *New_grdsample_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDSAMPLE_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct GRDSAMPLE_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
	return (C);
}

void Free_grdsample_Ctrl (struct GMT_CTRL *GMT, struct GRDSAMPLE_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->In.file) free (C->In.file);	
	if (C->G.file) free (C->G.file);	
	GMT_free (GMT, C);	
}

int GMT_grdsample_usage (struct GMTAPI_CTRL *C, int level) {

	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: grdsample <ingrid> -G<outgrid> [%s]\n", GMT_I_OPT);
	GMT_message (GMT, "\t[%s] [-T] [%s] [%s]\n\t[%s] [%s]\n", GMT_Rgeo_OPT, GMT_V_OPT, GMT_f_OPT, GMT_n_OPT, GMT_r_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<ingrid> is data set to be resampled.\n");
	GMT_message (GMT, "\t-G Set the name of the interpolated output grid file.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_inc_syntax (GMT, 'I', 0);
	GMT_message (GMT, "\t   When omitted: grid spacing is copied from input grid.\n");
	GMT_message (GMT, "\t-R Specify a subregion [Default is old region].\n");
	GMT_message (GMT, "\t-T Toggle between grid registration and pixel registration.\n");
	GMT_explain_options (GMT, "VfnF.");

	return (EXIT_FAILURE);
}

int GMT_grdsample_parse (struct GMTAPI_CTRL *C, struct GRDSAMPLE_CTRL *Ctrl, struct GMT_OPTION *options) {

	/* This parses the options provided to grdsample and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	int n_errors = 0, n_files = 0;
#ifdef GMT_COMPAT
	int ii = 0, jj = 0;
	char format[GMT_BUFSIZ];
#endif
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Input files */
				Ctrl->In.active = true;
				if (n_files++ == 0) Ctrl->In.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'G':	/* Output file */
				Ctrl->G.active = true;
				Ctrl->G.file = strdup (opt->arg);
				break;
			case 'I':	/* Grid spacings */
				Ctrl->I.active = true;
				if (GMT_getinc (GMT, opt->arg, Ctrl->I.inc)) {
					GMT_inc_syntax (GMT, 'I', 1);
					n_errors++;
				}
				break;
#ifdef GMT_COMPAT
			case 'L':	/* BCs */
				GMT_report (GMT, GMT_MSG_COMPAT, "Warning: Option -L is deprecated; -n+b%s was set instead, use this in the future.\n", opt->arg);
				strncpy (GMT->common.n.BC, opt->arg, 4U);
				/* We turn on geographic coordinates if -Lg is given by faking -fg */
				/* But since GMT_parse_f_option is private to gmt_init and all it does */
				/* in this case are 2 lines bellow we code it here */
				if (!strcmp (GMT->common.n.BC, "g")) {
					GMT->current.io.col_type[GMT_IN][GMT_X] = GMT->current.io.col_type[GMT_OUT][GMT_X] = GMT_IS_LON;
					GMT->current.io.col_type[GMT_IN][GMT_Y] = GMT->current.io.col_type[GMT_OUT][GMT_Y] = GMT_IS_LAT;
				}
				break;
			case 'N':	/* Backwards compatible.  nx/ny can now be set with -I */
				GMT_report (GMT, GMT_MSG_COMPAT, "Warning: Option -N<nx>/<ny> is deprecated; use -I<nx>+/<ny>+ instead.\n");
				Ctrl->I.active = true;
				sscanf (opt->arg, "%d/%d", &ii, &jj);
				if (jj == 0) jj = ii;
				sprintf (format, "%d+/%d+", ii, jj);
				if (GMT_getinc (GMT, format, Ctrl->I.inc)) {
					GMT_inc_syntax (GMT, 'I', 1);
					n_errors++;
				}
				break;
#endif
			case 'T':	/* Convert from pixel file <-> gridfile */
				Ctrl->T.active = true;
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	GMT_check_lattice (GMT, Ctrl->I.inc, &GMT->common.r.active, &Ctrl->I.active);

	n_errors += GMT_check_condition (GMT, n_files != 1, "Syntax error: Must specify a single input grid file\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->G.file, "Syntax error -G: Must specify output file\n");
	n_errors += GMT_check_condition (GMT, GMT->common.r.active && Ctrl->T.active, 
					"Syntax error: Only one of -r, -T may be specified\n");
	n_errors += GMT_check_condition (GMT, Ctrl->I.active && (Ctrl->I.inc[GMT_X] <= 0.0 || Ctrl->I.inc[GMT_Y] <= 0.0), 
					"Syntax error -I: Must specify positive increments\n");
	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grdsample_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grdsample (struct GMTAPI_CTRL *API, int mode, void *args) {

	int error = 0;
	unsigned int row, col;
	
	uint64_t ij;
	
	char format[GMT_BUFSIZ];
	
	double *lon = NULL, lat;

	struct GRDSAMPLE_CTRL *Ctrl = NULL;
	struct GMT_GRID *Gin = NULL, *Gout = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdsample_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdsample_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-VfR", "nr" GMT_OPT("FQ"), options)) Return (API->error);
	Ctrl = New_grdsample_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdsample_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the grdsample main code ----------------------------*/

	if ((Gin = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER, NULL, Ctrl->In.file, NULL)) == NULL) {	/* Get header only */
		Return (API->error);
	}

	if ((Gout = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);
	GMT_memcpy (Gout->header->wesn, (GMT->common.R.active ? GMT->common.R.wesn : Gin->header->wesn), 4, double);

	if (Ctrl->I.active)
		GMT_memcpy (Gout->header->inc, Ctrl->I.inc, 2, double);
	else
		GMT_memcpy (Gout->header->inc, Gin->header->inc, 2, double);

	if (Ctrl->T.active)
		Gout->header->registration = !Gin->header->registration;
	else if (GMT->common.r.active)
		Gout->header->registration = GMT_PIXEL_REG;
	else
		Gout->header->registration = Gin->header->registration;

	GMT_RI_prepare (GMT, Gout->header);	/* Ensure -R -I consistency and set nx, ny */
	GMT_set_grddim (GMT, Gout->header);

	if (GMT->common.R.active) {	/* Make sure input grid and output -R has an overlap */
		if (Gout->header->wesn[YLO] < Gin->header->wesn[YLO] - GMT_SMALL || Gout->header->wesn[YHI] > Gin->header->wesn[YHI] + GMT_SMALL) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Error: Selected region exceeds the Y-boundaries of the grid file!\n");
			Return (EXIT_FAILURE);
		}
		if (GMT_is_geographic (GMT, GMT_IN)) {	/* Must carefully check the longitude overlap */
			int shift = 0;
			if (Gin->header->wesn[XHI] < Gout->header->wesn[XLO]) shift += 360;
			if (Gin->header->wesn[XLO] > Gout->header->wesn[XHI]) shift -= 360;
			if (shift) {	/* Must modify header */
				Gin->header->wesn[XLO] += shift, Gin->header->wesn[XHI] += shift;
				GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "File %s region needed longitude adjustment to fit final grid region\n", Ctrl->In.file);
			}
		}
		else if (Gout->header->wesn[XLO] < Gin->header->wesn[XLO] - GMT_SMALL || Gout->header->wesn[XHI] > Gin->header->wesn[XHI] + GMT_SMALL) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Error: Selected region exceeds the X-boundaries of the grid file!\n");
			return (EXIT_FAILURE);
		}
	}

	if (!Ctrl->I.active) {
		Gout->header->inc[GMT_X] = GMT_get_inc (GMT, Gout->header->wesn[XLO], Gout->header->wesn[XHI], Gout->header->nx, Gout->header->registration);
		Gout->header->inc[GMT_Y] = GMT_get_inc (GMT, Gout->header->wesn[YLO], Gout->header->wesn[YHI], Gout->header->ny, Gout->header->registration);
	}

	GMT_err_fail (GMT, GMT_grd_RI_verify (GMT, Gout->header, 1), Ctrl->G.file);

	Gout->data = GMT_memory_aligned (GMT, NULL, Gout->header->size, float);

	GMT_grd_init (GMT, Gin->header, options, true);

	sprintf (format, "Input  grid (%s/%s/%s/%s) nx = %%d ny = %%d dx = %s dy = %s registration = %%d\n", 
		GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, 
		GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out);

	GMT_report (GMT, GMT_MSG_VERBOSE, format, Gin->header->wesn[XLO], Gin->header->wesn[XHI], 
		Gin->header->wesn[YLO], Gin->header->wesn[YHI], Gin->header->nx, Gin->header->ny,
		Gin->header->inc[GMT_X], Gin->header->inc[GMT_Y], Gin->header->registration);

	memcpy (&format, "Output", 6);

	GMT_report (GMT, GMT_MSG_VERBOSE, format, Gout->header->wesn[XLO], Gout->header->wesn[XHI], 
		Gout->header->wesn[YLO], Gout->header->wesn[YHI], Gout->header->nx, Gout->header->ny,
		Gout->header->inc[GMT_X], Gout->header->inc[GMT_Y], Gout->header->registration);

	if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->In.file, Gin) == NULL) {	/* Get subset */
		Return (API->error);
	}

	if (Gout->header->inc[GMT_X] > Gin->header->inc[GMT_X]) GMT_report (GMT, GMT_MSG_VERBOSE, "Warning: Output sampling interval in x exceeds input interval and may lead to aliasing.\n");
	if (Gout->header->inc[GMT_Y] > Gin->header->inc[GMT_Y]) GMT_report (GMT, GMT_MSG_VERBOSE, "Warning: Output sampling interval in y exceeds input interval and may lead to aliasing.\n");

	/* Precalculate longitudes */

	lon = GMT_memory (GMT, NULL, Gout->header->nx, double);
	for (col = 0; col < Gout->header->nx; col++) {
		lon[col] = GMT_grd_col_to_x (GMT, col, Gout->header);
		if (!Gin->header->nxp)
			/* Nothing */;
		else if (lon[col] > Gin->header->wesn[XHI])
			lon[col] -= Gin->header->inc[GMT_X] * Gin->header->nxp;
		else if (lon[col] < Gin->header->wesn[XLO])
			lon[col] += Gin->header->inc[GMT_X] * Gin->header->nxp;
	}

	/* Loop over input point and estinate output values */
	
	GMT_row_loop (GMT, Gout, row) {
		lat = GMT_grd_row_to_y (GMT, row, Gout->header);
		if (!Gin->header->nyp)
			/* Nothing */;
		else if (lat > Gin->header->wesn[YHI])
			lat -= Gin->header->inc[GMT_Y] * Gin->header->nyp;
		else if (lat < Gin->header->wesn[YLO])
			lat += Gin->header->inc[GMT_Y] * Gin->header->nyp;
		GMT_col_loop (GMT, Gout, row, col, ij) Gout->data[ij] = (float)GMT_get_bcr_z (GMT, Gin, lon[col], lat);
	}

	if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->G.file, Gout) != GMT_OK) {
		Return (API->error);
	}

	GMT_free (GMT, lon);

	Return (GMT_OK);
}
