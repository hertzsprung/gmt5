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
 * Brief synopsis: grdproject reads a geographical grid file and evaluates the grid at new grid positions
 * specified by the map projection and new dx/dy values using a weighted average of all
 * points within the search radius. Optionally, grdproject may perform the inverse
 * transformation, going from rectangular coordinates to geographical.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Ver:		5 API
 */

#define THIS_MODULE k_mod_grdproject /* I am grdproject */

#include "gmt_dev.h"

struct GRDPROJECT_CTRL {
	struct In {	/* Input grid */
		bool active;
		char *file;
	} In;
	struct A {	/* -A[k|m|n|i|c|p] */
		bool active;
		char unit;
	} A;
	struct C {	/* -C[<dx/dy>] */
		bool active;
		double easting, northing;
	} C;
	struct D {	/* -Ddx[/dy] */
		bool active;
		double inc[2];
	} D;
	struct E {	/* -E<dpi> */
		bool active;
		int dpi;
	} E;
	struct G {	/* -G */
		bool active;
		char *file;
	} G;
	struct I {	/* -I */
		bool active;
	} I;
	struct M {	/* -Mc|i|m */
		bool active;
		char unit;
	} M;
};

void *New_grdproject_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDPROJECT_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct GRDPROJECT_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
		
	return (C);
}

void Free_grdproject_Ctrl (struct GMT_CTRL *GMT, struct GRDPROJECT_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->In.file) free (C->In.file);	
	if (C->G.file) free (C->G.file);	
	GMT_free (GMT, C);	
}

int GMT_grdproject_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: grdproject <ingrid> -G<outgrid> %s\n", GMT_J_OPT);
	GMT_message (GMT, "\t[-A[%s|%s]] [-C[<dx>/<dy>]] [-D%s] [-E<dpi>]\n", GMT_LEN_UNITS2_DISPLAY, GMT_DIM_UNITS_DISPLAY, GMT_inc_OPT);
	GMT_message (GMT, "\t[-I] [-M%s] [%s]\n", GMT_DIM_UNITS_DISPLAY, GMT_Rgeo_OPT);
	GMT_message (GMT, "\t[%s] [%s] [%s]\n\n", GMT_V_OPT, GMT_n_OPT, GMT_r_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<ingrid> is data set to be projected.\n");
	GMT_message (GMT, "\t-G Set name of output grid\n");
	GMT_explain_options (GMT, "J");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t-A Force projected values to be in actual distance units [Default uses the given map scale].\n");
	GMT_message (GMT, "\t   Specify unit by appending e (meter), f (foot) k (km), M (mile), n (nautical mile), u (survey foot),\n");
	GMT_message (GMT, "\t   or i (inch), c (cm), or p (points) [e].\n");
	GMT_message (GMT, "\t-C Coordinates are relative to projection center [Default is relative to lower left corner].\n");
	GMT_message (GMT, "\t   Optionally append dx/dy to add (or subtract if -I) (i.e., false easting & northing) [0/0].\n");
	GMT_inc_syntax (GMT, 'D', 0);
	GMT_message (GMT, "\t-E Set dpi for output grid.\n");
	GMT_message (GMT, "\t-I Inverse transformation from rectangular to geographical.\n");
	GMT_message (GMT, "\t-M Temporarily reset PROJ_LENGTH_UNIT to be c (cm), i (inch), or p (point).\n");
	GMT_message (GMT, "\t   Cannot be used if -A is set.\n");
	GMT_explain_options (GMT, "R");
	GMT_explain_options (GMT, "VnF.");

	return (EXIT_FAILURE);
}

int GMT_grdproject_parse (struct GMTAPI_CTRL *C, struct GRDPROJECT_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to grdproject and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_files = 0;
	int sval;
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

			case 'A':	/* Force meters */
				Ctrl->A.active = true;
				Ctrl->A.unit = opt->arg[0];
				break;
			case 'C':	/* Coordinates relative to origin */
				Ctrl->C.active = true;
				if (opt->arg[0]) 	/* Also gave shifts */
					n_errors += GMT_check_condition (GMT, sscanf (opt->arg, "%lf/%lf", &Ctrl->C.easting, &Ctrl->C.northing) != 2,
						 "Syntax error: Expected -C[<false_easting>/<false_northing>]\n");
				break;
			case 'D':	/* Grid spacings */
				Ctrl->D.active = true;
				if (GMT_getinc (GMT, opt->arg, Ctrl->D.inc)) {
					GMT_inc_syntax (GMT, 'D', 1);
					n_errors++;
				}
				break;
			case 'E':	/* Set dpi of grid */
				Ctrl->E.active = true;
				sval = atoi (opt->arg);
				n_errors += GMT_check_condition (GMT, sval <= 0, "Syntax error -E option: Must specify positive dpi\n");
				Ctrl->E.dpi = sval;
				break;
			case 'G':	/* Output file */
				Ctrl->G.file = strdup (opt->arg);
				break;
			case 'I':	/* Inverse projection */
				Ctrl->I.active = true;
				break;
			case 'M':	/* Directly specify units */
				Ctrl->M.active = true;
				Ctrl->M.unit = opt->arg[0];
				break;
#ifdef GMT_COMPAT
			case 'N':	/* Backwards compatible.  nx/ny can now be set with -D */
				GMT_report (GMT, GMT_MSG_COMPAT, "Warning: -N option is deprecated; use -D instead.\n");
				Ctrl->D.active = true;
				sscanf (opt->arg, "%d/%d", &ii, &jj);
				if (jj == 0) jj = ii;
				sprintf (format, "%d+/%d+", ii, jj);
				if (GMT_getinc (GMT, format, Ctrl->D.inc)) {
					GMT_inc_syntax (GMT, 'D', 1);
					n_errors++;
				}
				break;
#endif
			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	GMT_check_lattice (GMT, Ctrl->D.inc, &GMT->common.r.registration, &Ctrl->D.active);

	n_errors += GMT_check_condition (GMT, !Ctrl->In.file, "Syntax error: Must specify input file\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->G.file, "Syntax error -G option: Must specify output file\n");
	n_errors += GMT_check_condition (GMT, !GMT->common.J.active, "Syntax error: Must specify a map projection with the -J option\n");
	n_errors += GMT_check_condition (GMT, (Ctrl->M.active + Ctrl->A.active) == 2, "Syntax error: Can specify only one of -A and -M\n");
	n_errors += GMT_check_condition (GMT, (Ctrl->D.active + Ctrl->E.active) > 1, "Syntax error: Must specify only one of -D or -E\n");
	n_errors += GMT_check_condition (GMT, Ctrl->D.active && (Ctrl->D.inc[GMT_X] <= 0.0 || Ctrl->D.inc[GMT_Y] < 0.0), "Syntax error -D option: Must specify positive increment(s)\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grdproject_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grdproject (void *V_API, int mode, void *args)
{
	bool set_n = false, shift_xy = false;
	unsigned int use_nx = 0, use_ny = 0, offset, k, unit = 0;
	int error = 0;

	char format[GMT_BUFSIZ], unit_name[GMT_GRID_UNIT_LEN80], scale_unit_name[GMT_GRID_UNIT_LEN80];

	double wesn[4];
	double xmin, xmax, ymin, ymax, inch_to_unit, unit_to_inch, fwd_scale, inv_scale;

	struct GMT_GRID *Geo = NULL, *Rect = NULL;
	struct GRDPROJECT_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdproject_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdproject_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-VJR", "nr" GMT_OPT("FS"), options)) Return (API->error);
	Ctrl = New_grdproject_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdproject_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the grdproject main code ----------------------------*/

	GMT_report (GMT, GMT_MSG_VERBOSE, "Processing input grid\n");
	if ((Ctrl->D.active + Ctrl->E.active) == 0) set_n = true;
	if (Ctrl->M.active) GMT_err_fail (GMT, GMT_set_measure_unit (GMT, Ctrl->M.unit), "-M");
	shift_xy = !(Ctrl->C.easting == 0.0 && Ctrl->C.northing == 0.0);
	
	unit = GMT_check_scalingopt (GMT, 'A', Ctrl->A.unit, scale_unit_name);
	GMT_init_scales (GMT, unit, &fwd_scale, &inv_scale, &inch_to_unit, &unit_to_inch, unit_name);

	if (Ctrl->I.active) {	/* Must flip the column types since in is Cartesian and out is geographic */
		GMT->current.io.col_type[GMT_OUT][GMT_X] = GMT_IS_LON;	GMT->current.io.col_type[GMT_OUT][GMT_Y] = GMT_IS_LAT;	/* Inverse projection expects x,y and gives lon, lat */
		GMT->current.io.col_type[GMT_IN][GMT_X] = GMT->current.io.col_type[GMT_IN][GMT_Y] = GMT_IS_FLOAT;
	}
	
	if (GMT->common.R.active)	/* Load the w/e/s/n from -R */
		GMT_memcpy (wesn, GMT->common.R.wesn, 4, double);
	else {	/* If -R was not given we infer the option via the input grid */
		char opt_R[GMT_BUFSIZ];
		struct GMT_GRID *G = NULL;
		if ((G = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER_ONLY, NULL, Ctrl->In.file, NULL)) == NULL) {	/* Get header only */
			Return (API->error);
		}
		GMT_memcpy (wesn, G->header->wesn, 4, double);
		if (!Ctrl->I.active) {
			sprintf (opt_R, "%.12f/%.12f/%.12f/%.12f", wesn[XLO], wesn[XHI], wesn[YLO], wesn[YHI]);
			GMT_parse_common_options (GMT, "R", 'R', opt_R);
			if (GMT_map_setup (GMT, GMT->common.R.wesn)) Return (GMT_RUNTIME_ERROR);
		}
		else {			/* Do inverse transformation */
			double x_c, y_c, lon_t, lat_t, ww, ee, ss, nn;
			/* Obtain a first crude estimation of the good -R */
			x_c = (wesn[XLO] + wesn[XHI]) / 2.0; 		/* mid point of projected coords */
			y_c = (wesn[YLO] + wesn[YHI]) / 2.0; 
			if (GMT->current.proj.projection == GMT_UTM && !GMT->current.proj.north_pole && y_c > 0) y_c *= -1;
			if (y_c > 0)
				GMT_parse_common_options (GMT, "R", 'R', "-180/180/0/80");
			else
				GMT_parse_common_options (GMT, "R", 'R', "-180/180/-80/0");
			if (GMT->current.proj.projection == GMT_UTM && !GMT->current.proj.north_pole && y_c < 0) y_c *= -1;	/* Undo the *-1 (only for the UTM case) */ 
			if (shift_xy) {
				x_c -= Ctrl->C.easting;
				y_c -= Ctrl->C.northing;
			}
			/* Convert from 1:1 scale */ 
			if (unit) {
				x_c *= fwd_scale;
				y_c *= fwd_scale;
			}

			if (GMT_map_setup (GMT, GMT->common.R.wesn)) Return (GMT_RUNTIME_ERROR);

			x_c *= GMT->current.proj.scale[GMT_X];
			y_c *= GMT->current.proj.scale[GMT_Y];

			if (Ctrl->C.active) {	/* Then correct so lower left corner is (0,0) */
				x_c += GMT->current.proj.origin[GMT_X];
				y_c += GMT->current.proj.origin[GMT_Y];
			}
			GMT_xy_to_geo (GMT, &lon_t, &lat_t, x_c, y_c);
			sprintf (opt_R, "%.12f/%.12f/%.12f/%.12f", lon_t-1, lon_t+1, lat_t-1, lat_t+1);
			if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "First opt_R\t %s\t%g\t%g\n", opt_R, x_c, y_c);
			GMT->common.R.active = false;	/* We need to reset this to not fall into non-wanted branch deeper down */
			GMT_parse_common_options (GMT, "R", 'R', opt_R);
			if (GMT_map_setup (GMT, GMT->common.R.wesn)) Return (GMT_RUNTIME_ERROR);

			/* Finally obtain the good limits */
			if (shift_xy) {
				wesn[XLO] -= Ctrl->C.easting;	wesn[XHI] -= Ctrl->C.easting;
				wesn[YLO] -= Ctrl->C.northing;	wesn[YHI] -= Ctrl->C.northing;
			}
			if (unit) for (k = 0; k < 4; k++) wesn[k] *= fwd_scale;
			
			wesn[XLO] *= GMT->current.proj.scale[GMT_X];	wesn[XHI] *= GMT->current.proj.scale[GMT_X];
			wesn[YLO] *= GMT->current.proj.scale[GMT_Y];	wesn[YHI] *= GMT->current.proj.scale[GMT_Y];

			if (Ctrl->C.active) {
				wesn[XLO] += GMT->current.proj.origin[GMT_X];	wesn[XHI] += GMT->current.proj.origin[GMT_X];
				wesn[YLO] += GMT->current.proj.origin[GMT_Y];	wesn[YHI] += GMT->current.proj.origin[GMT_Y];
			}

			GMT_xy_to_geo (GMT, &ww, &ss, wesn[XLO], wesn[YLO]);		/* SW corner */
			GMT_xy_to_geo (GMT, &ee, &nn, wesn[XHI], wesn[YHI]);		/* NE corner */
			sprintf (opt_R, "%.12f/%.12f/%.12f/%.12fr", ww, ss, ee, nn);
			if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) GMT_message (GMT, "Second opt_R\t %s\n", opt_R);
			GMT->common.R.active = false;
			GMT_parse_common_options (GMT, "R", 'R', opt_R);
		}
		if (GMT_Destroy_Data (API, GMT_ALLOCATED, &G) != GMT_OK) {
			Return (API->error);
		}
	}

	if (GMT_map_setup (GMT, GMT->common.R.wesn)) Return (GMT_RUNTIME_ERROR);

	xmin = (Ctrl->C.active) ? GMT->current.proj.rect[XLO] - GMT->current.proj.origin[GMT_X] : GMT->current.proj.rect[XLO];
	xmax = (Ctrl->C.active) ? GMT->current.proj.rect[XHI] - GMT->current.proj.origin[GMT_X] : GMT->current.proj.rect[XHI];
	ymin = (Ctrl->C.active) ? GMT->current.proj.rect[YLO] - GMT->current.proj.origin[GMT_Y] : GMT->current.proj.rect[YLO];
	ymax = (Ctrl->C.active) ? GMT->current.proj.rect[YHI] - GMT->current.proj.origin[GMT_Y] : GMT->current.proj.rect[YHI];
	if (Ctrl->A.active) {	/* Convert to chosen units */
		strncpy (unit_name, scale_unit_name, GMT_GRID_UNIT_LEN80);
		xmin /= GMT->current.proj.scale[GMT_X];
		xmax /= GMT->current.proj.scale[GMT_X];
		ymin /= GMT->current.proj.scale[GMT_Y];
		ymax /= GMT->current.proj.scale[GMT_Y];
		if (unit) {	/* Change the 1:1 unit used */
			xmin *= fwd_scale;
			xmax *= fwd_scale;
			ymin *= fwd_scale;
			ymax *= fwd_scale;
		}
	}
	else {	/* Convert inches to chosen MEASURE */
		xmin *= inch_to_unit;
		xmax *= inch_to_unit;
		ymin *= inch_to_unit;
		ymax *= inch_to_unit;
	}
	if (shift_xy) {
		xmin += Ctrl->C.easting;
		xmax += Ctrl->C.easting;
		ymin += Ctrl->C.northing;
		ymax += Ctrl->C.northing;
	}

	sprintf (format, "(%s/%s/%s/%s)", GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out);

	if (Ctrl->I.active) {	/* Transforming from rectangular projection to geographical */

		/* if (GMT->common.R.oblique) double_swap (s, e); */  /* Got w/s/e/n, make into w/e/s/n */

		if ((Rect = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->In.file, NULL)) == NULL) {
			Return (API->error);
		}

		if ((Geo = GMT_Duplicate_Data (API, GMT_IS_GRID, GMT_DUPLICATE_NONE, Rect)) == NULL) Return (API->error);	/* Just to get a header we can change */

		GMT_memcpy (Geo->header->wesn, wesn, 4, double);

		offset = Rect->header->registration;	/* Same as input */
		if (GMT->common.r.active) offset = !offset;	/* Toggle */
		if (set_n) {
			use_nx = Rect->header->nx;
			use_ny = Rect->header->ny;
		}
		GMT_err_fail (GMT, GMT_project_init (GMT, Geo->header, Ctrl->D.inc, use_nx, use_ny, Ctrl->E.dpi, offset), Ctrl->G.file);
		GMT_set_grddim (GMT, Geo->header);
		if (GMT_Create_Data (API, GMT_IS_GRID, GMT_GRID_DATA_ONLY, NULL, NULL, NULL, 0, 0, Geo) == NULL) Return (API->error);
		GMT_grd_init (GMT, Geo->header, options, true);

		if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) {
			GMT_report (GMT, GMT_MSG_VERBOSE, "Transform ");
			GMT_message (GMT, format, Geo->header->wesn[XLO], Geo->header->wesn[XHI], Geo->header->wesn[YLO], Geo->header->wesn[YHI]);
			GMT_message (GMT, " <-- ");
			GMT_message (GMT, format, xmin, xmax, ymin, ymax);
			GMT_message (GMT, " [%s]\n", unit_name);
		}

		/* Modify input rect header if -A, -C, -M have been set */

		if (shift_xy) {
			Rect->header->wesn[XLO] -= Ctrl->C.easting;
			Rect->header->wesn[XHI] -= Ctrl->C.easting;
			Rect->header->wesn[YLO] -= Ctrl->C.northing;
			Rect->header->wesn[YHI] -= Ctrl->C.northing;

		}
		if (Ctrl->A.active) {	/* Convert from 1:1 scale */
			if (unit) for (k = 0; k < 4; k++) Rect->header->wesn[k] *= inv_scale;	/* Undo the 1:1 unit used */
			Rect->header->wesn[XLO] *= GMT->current.proj.scale[GMT_X];
			Rect->header->wesn[XHI] *= GMT->current.proj.scale[GMT_X];
			Rect->header->wesn[YLO] *= GMT->current.proj.scale[GMT_Y];
			Rect->header->wesn[YHI] *= GMT->current.proj.scale[GMT_Y];
		}
		else if (GMT->current.setting.proj_length_unit != GMT_INCH) {	/* Convert from inch to whatever */
			for (k = 0; k < 4; k++) Rect->header->wesn[k] *= unit_to_inch;
		}
		if (Ctrl->C.active) {	/* Then correct so lower left corner is (0,0) */
			Rect->header->wesn[XLO] += GMT->current.proj.origin[GMT_X];
			Rect->header->wesn[XHI] += GMT->current.proj.origin[GMT_X];
			Rect->header->wesn[YLO] += GMT->current.proj.origin[GMT_Y];
			Rect->header->wesn[YHI] += GMT->current.proj.origin[GMT_Y];
		}
		GMT_set_grdinc (GMT, Rect->header);	/* Update inc and r_inc given changes to wesn */
		
		sprintf (Geo->header->x_units, "longitude [degrees_east]");
		sprintf (Geo->header->y_units, "latitude [degrees_north]");

		GMT_grd_project (GMT, Rect, Geo, true);

		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->G.file, Geo) != GMT_OK) {
			Return (API->error);
		}
	}
	else {	/* Forward projection from geographical to rectangular grid */

		if ((Geo = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->In.file, NULL)) == NULL) {
			Return (API->error);
		}

		if ((Rect = GMT_Duplicate_Data (API, GMT_IS_GRID, GMT_DUPLICATE_NONE, Geo)) == NULL) Return (API->error);	/* Just to get a header we can change */
		GMT_memcpy (Rect->header->wesn, GMT->current.proj.rect, 4, double);
		if (Ctrl->A.active) {	/* Convert from 1:1 scale */
			if (unit) {	/* Undo the 1:1 unit used */
				Ctrl->D.inc[GMT_X] *= inv_scale;
				Ctrl->D.inc[GMT_Y] *= inv_scale;
			}
			Ctrl->D.inc[GMT_X] *= GMT->current.proj.scale[GMT_X];
			Ctrl->D.inc[GMT_Y] *= GMT->current.proj.scale[GMT_Y];
		}
		else if (GMT->current.setting.proj_length_unit != GMT_INCH) {	/* Convert from inch to whatever */
			Ctrl->D.inc[GMT_X] *= unit_to_inch;
			Ctrl->D.inc[GMT_Y] *= unit_to_inch;
		}
		if (set_n) {
			use_nx = Geo->header->nx;
			use_ny = Geo->header->ny;
		}

		if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) {
			GMT_report (GMT, GMT_MSG_VERBOSE, "Transform ");
			GMT_message (GMT, format, Geo->header->wesn[XLO], Geo->header->wesn[XHI], Geo->header->wesn[YLO], Geo->header->wesn[YHI]);
			GMT_message (GMT, " --> ");
			GMT_message (GMT, format, xmin, xmax, ymin, ymax);
			GMT_message (GMT, " [%s]\n", unit_name);
		}

		offset = Geo->header->registration;	/* Same as input */
		if (GMT->common.r.active) offset = !offset;	/* Toggle */

		GMT_err_fail (GMT, GMT_project_init (GMT, Rect->header, Ctrl->D.inc, use_nx, use_ny, Ctrl->E.dpi, offset), Ctrl->G.file);
		GMT_set_grddim (GMT, Rect->header);
		if (GMT_Create_Data (API, GMT_IS_GRID, GMT_GRID_DATA_ONLY, NULL, NULL, NULL, 0, 0, Rect) == NULL) Return (API->error);
		GMT_grd_project (GMT, Geo, Rect, false);
		GMT_grd_init (GMT, Rect->header, options, true);

		/* Modify output rect header if -A, -C, -M have been set */

		if (Ctrl->C.active) {	/* Change origin from lower left to projection center */
			Rect->header->wesn[XLO] -= GMT->current.proj.origin[GMT_X];
			Rect->header->wesn[XHI] -= GMT->current.proj.origin[GMT_X];
			Rect->header->wesn[YLO] -= GMT->current.proj.origin[GMT_Y];
			Rect->header->wesn[YHI] -= GMT->current.proj.origin[GMT_Y];
		}
		if (Ctrl->A.active) {	/* Convert to 1:1 scale */
			Rect->header->wesn[XLO] /= GMT->current.proj.scale[GMT_X];
			Rect->header->wesn[XHI] /= GMT->current.proj.scale[GMT_X];
			Rect->header->wesn[YLO] /= GMT->current.proj.scale[GMT_Y];
			Rect->header->wesn[YHI] /= GMT->current.proj.scale[GMT_Y];
			if (unit) for (k = 0; k < 4; k++) Rect->header->wesn[k] *= fwd_scale;	/* Change the 1:1 unit used */
		}
		else if (GMT->current.setting.proj_length_unit != GMT_INCH) {	/* Convert from inch to whatever */
			for (k = 0; k < 4; k++) Rect->header->wesn[k] /= unit_to_inch;
		}
		if (shift_xy) {
			Rect->header->wesn[XLO] += Ctrl->C.easting;
			Rect->header->wesn[XHI] += Ctrl->C.easting;
			Rect->header->wesn[YLO] += Ctrl->C.northing;
			Rect->header->wesn[YHI] += Ctrl->C.northing;

		}
		GMT_set_grdinc (GMT, Rect->header);	/* Update inc and r_inc given changes to wesn */
		strncpy (Rect->header->x_units, unit_name, GMT_GRID_UNIT_LEN80);
		strncpy (Rect->header->y_units, unit_name, GMT_GRID_UNIT_LEN80);

		/* rect xy values are here in GMT projected units chosen by user */

		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->G.file, Rect) != GMT_OK) {
			Return (API->error);
		}
	}

	Return (GMT_OK);
}
