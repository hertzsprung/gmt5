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
 * Brief synopsis: grdlandmask defines a grid based on region and xinc/yinc values,
 * reads a shoreline data base, and sets the grid nodes inside, on the
 * boundary, and outside of the polygons to the user-defined values
 * <in>, <on>, and <out>.  These may be any number, including NaN.
 *
 * Author:	P. Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */
 
#define THIS_MODULE k_mod_grdlandmask /* I am grdlandmask */

#include "gmt_dev.h"

#define GMT_PROG_OPTIONS "-VRr" GMT_OPT("F")

#define GRDLANDMASK_N_CLASSES	(GSHHS_MAX_LEVEL + 1)	/* Number of bands separated by the levels */

struct GRDLANDMASK_CTRL {	/* All control options for this program (except common args) */
	/* ctive is true if the option has been activated */
	struct A {	/* -A<min_area>[/<min_level>/<max_level>] */
		bool active;
		struct GMT_SHORE_SELECT info;
	} A;
	struct D {	/* -D<resolution> */
		bool active;
		bool force;	/* if true, select next highest level if current set is not avaialble */
		char set;	/* One of f, h, i, l, c */
	} D;
	struct E {	/* -E */
		bool active;
		unsigned int inside;	/* if 2, then a point exactly on a polygon boundary is considered OUTSIDE, else 1 */
	} E;
	struct G {	/* -G<maskfile> */
		bool active;
		char *file;
	} G;
	struct I {	/* -Idx[/dy] */
		bool active;
		double inc[2];
	} I;
	struct N {	/* -N<maskvalues>[o] */
		bool active;
		unsigned int mode;	/* 1 if dry/wet only, 0 if 5 mask levels */
		double mask[GRDLANDMASK_N_CLASSES];	/* values for each level */
	} N;
};

void *New_grdlandmask_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDLANDMASK_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct GRDLANDMASK_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
	
	C->A.info.high = GSHHS_MAX_LEVEL;				/* Include all GSHHS levels */
	C->D.set = 'l';							/* Low-resolution coastline data */
	C->E.inside = GMT_ONEDGE;					/* Default is that points on a boundary are inside */
	GMT_memset (C->N.mask, GRDLANDMASK_N_CLASSES, double);		/* Default "wet" value = 0 */
	C->N.mask[1] = C->N.mask[3] = 1.0;				/* Default for "dry" areas = 1 (inside) */
	
	return (C);
}

void Free_grdlandmask_Ctrl (struct GMT_CTRL *GMT, struct GRDLANDMASK_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->G.file) free (C->G.file);	
	GMT_free (GMT, C);	
}

int GMT_grdlandmask_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: grdlandmask -G<outgrid> %s %s\n", GMT_I_OPT, GMT_Rgeo_OPT);
	GMT_message (GMT, "\t[%s] [-D<resolution>][+] [-E]\n\t[-N<maskvalues>] [%s] [%s]\n\n", GMT_A_OPT, GMT_V_OPT, GMT_r_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t-G Specify file name for output mask grid file.\n");
	GMT_inc_syntax (GMT, 'I', 0);
	GMT_Option (C, "R");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_GSHHS_syntax (GMT, 'A', "Place limits on coastline features from the GSHHS data base.");
	GMT_message (GMT, "\t-D Choose one of the following resolutions:\n");
	GMT_message (GMT, "\t   f - full resolution (may be very slow for large regions).\n");
	GMT_message (GMT, "\t   h - high resolution (may be slow for large regions).\n");
	GMT_message (GMT, "\t   i - intermediate resolution.\n");
	GMT_message (GMT, "\t   l - low resolution [Default].\n");
	GMT_message (GMT, "\t   c - crude resolution, for tasks that need crude continent outlines only.\n");
	GMT_message (GMT, "\t   Append + to use a lower resolution should the chosen one not be available [abort].\n");
	GMT_message (GMT, "\t-E Indicate that nodes exactly on a polygon boundary are outside [inside].\n");
	GMT_message (GMT, "\t-N Give values to use if a node is outside or inside a feature.\n");
	GMT_message (GMT, "\t   Specify this information using 1 of 2 formats:\n");
	GMT_message (GMT, "\t   -N<wet>/<dry>.\n");
	GMT_message (GMT, "\t   -N<ocean>/<land>/<lake>/<island>/<pond>.\n");
	GMT_message (GMT, "\t   NaN is a valid entry.  Default values are 0/1/0/1/0 (i.e., 0/1).\n");
	GMT_Option (C, "V,r,.");
	
	return (EXIT_FAILURE);
}

int GMT_grdlandmask_parse (struct GMTAPI_CTRL *C, struct GRDLANDMASK_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to grdlandmask and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, j, pos, n_files = 0;
	char line[GMT_TEXT_LEN256], ptr[GMT_BUFSIZ];
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Input files */
				n_files++;
				break;

			/* Processes program-specific parameters */

			case 'A':	/* Restrict GSHHS features */
				Ctrl->A.active = true;
				GMT_set_levels (GMT, opt->arg, &Ctrl->A.info);
				break;
			case 'D':	/* Set GSHHS resolution */
				Ctrl->D.active = true;
				Ctrl->D.set = opt->arg[0];
				Ctrl->D.force = (opt->arg[1] == '+');
				break;
			case 'E':	/* On-boundary setting */
				Ctrl->E.active = true;
				Ctrl->E.inside = GMT_INSIDE;
				break;
			case 'G':	/* OUtput filename */
				Ctrl->G.active = true;
				Ctrl->G.file = strdup (opt->arg);
				break;
			case 'I':	/* Grid spacings */
				if (GMT_getinc (GMT, opt->arg, Ctrl->I.inc)) {
					GMT_inc_syntax (GMT, 'I', 1);
					n_errors++;
				}
				Ctrl->I.active = true;
				break;
			case 'N':	/* Mask values */
				Ctrl->N.active = true;
				strncpy (line, opt->arg,  GMT_TEXT_LEN256);
#ifdef GMT_COMPAT
				if (line[strlen(line)-1] == 'o') { /* Edge is considered outside */
					GMT_report (GMT, GMT_MSG_COMPAT, "Warning: Option -N...o is deprecated; use -E instead\n");
					Ctrl->E.active = true;
					Ctrl->E.inside = GMT_INSIDE;
					line[strlen(line)-1] = 0;
				}
#endif
				j = pos = 0;
				while (j < 5 && (GMT_strtok (line, "/", &pos, ptr))) {
					Ctrl->N.mask[j] = (ptr[0] == 'N' || ptr[0] == 'n') ? GMT->session.f_NaN : (float)atof (ptr);
					j++;
				}
				if (!(j == 2 || j == 5)) {
					GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -N option: Specify 2 or 5 arguments\n");
					n_errors++;
				}
				Ctrl->N.mode = (j == 2);
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	GMT_check_lattice (GMT, Ctrl->I.inc, &GMT->common.r.registration, &Ctrl->I.active);

	n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error: Must specify -R option\n");
	n_errors += GMT_check_condition (GMT, Ctrl->I.inc[GMT_X] <= 0.0 || Ctrl->I.inc[GMT_Y] <= 0.0, "Syntax error -I option: Must specify positive increment(s)\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->G.file, "Syntax error -G: Must specify an output file\n");
	n_errors += GMT_check_condition (GMT, n_files, "Syntax error: Use -G to specify output an file\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grdlandmask_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grdlandmask (void *V_API, int mode, void *args)
{
	bool temp_shift = false, wrap, used_polygons;
	unsigned int base = 3, k, bin, np, side, np_new;
	int row, row_min, row_max, ii, col, col_min, col_max, i, direction, err, ind, nx1, ny1, error = 0;
	
	uint64_t ij;

	char line[GMT_TEXT_LEN256];
	char *shore_resolution[5] = {"full", "high", "intermediate", "low", "crude"};

	double xmin, xmax, ymin, ymax, west_border, east_border, i_dx_inch, i_dy_inch;
	double dummy, *x = NULL, *y = NULL;

	struct GMT_SHORE c;
	struct GMT_GRID *Grid = NULL;
	struct GMT_GSHHS_POL *p = NULL;
	struct GRDLANDMASK_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_NOT_A_SESSION);
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdlandmask_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdlandmask_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, GMT_PROG_OPTIONS, options)) Return (API->error);
	Ctrl = New_grdlandmask_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdlandmask_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the grdlandmask main code ----------------------------*/

	/* We know coastline data are geographic so we hardwire this here: */
	GMT->current.io.col_type[GMT_IN][GMT_X] = GMT_IS_LON;	GMT->current.io.col_type[GMT_IN][GMT_Y] = GMT_IS_LAT;

	/* Create the empty grid and allocate space */
	if ((Grid = GMT_Create_Data (API, GMT_IS_GRID, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, NULL, Ctrl->I.inc, \
		GMT_GRID_DEFAULT_REG, GMTAPI_NOTSET, Ctrl->G.file)) == NULL) Return (API->error);
	
	if (Grid->header->wesn[XLO] < 0.0 && Grid->header->wesn[XHI] < 0.0) {	/* Shift longitudes */
		temp_shift = true;
		Grid->header->wesn[XLO] += 360.0;
		Grid->header->wesn[XHI] += 360.0;
	}

	if (Ctrl->D.force) Ctrl->D.set = GMT_shore_adjust_res (GMT, Ctrl->D.set);
	base = GMT_set_resolution (GMT, &Ctrl->D.set, 'D');
	
	if (Ctrl->N.mode) {
		Ctrl->N.mask[3] = Ctrl->N.mask[1];
		Ctrl->N.mask[2] = Ctrl->N.mask[4] = Ctrl->N.mask[0];
	}

	if (GMT_init_shore (GMT, Ctrl->D.set, &c, Grid->header->wesn, &Ctrl->A.info)) {
		GMT_report (GMT, GMT_MSG_NORMAL, "%s resolution shoreline data base not installed\n", shore_resolution[base]);
		Return (EXIT_FAILURE);
	}
	if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) {
		GMT_report (GMT, GMT_MSG_VERBOSE, "GSHHG version %s\n%s\n%s\n", c.version, c.title, c.source);

		sprintf (line, "%s\n", GMT->current.setting.format_float_out);
		if (Ctrl->N.mode) {
			GMT_report (GMT, GMT_MSG_VERBOSE, "Nodes in water will be set to ");
			(GMT_is_dnan (Ctrl->N.mask[0])) ? GMT_message (GMT, "NaN\n") : GMT_message (GMT, line, Ctrl->N.mask[0]);
			GMT_report (GMT, GMT_MSG_VERBOSE, "Nodes on land will be set to ");
			(GMT_is_dnan (Ctrl->N.mask[1])) ? GMT_message (GMT, "NaN\n") : GMT_message (GMT, line, Ctrl->N.mask[1]);
		}
		else {
			GMT_report (GMT, GMT_MSG_VERBOSE, "Nodes in the oceans will be set to ");
			(GMT_is_dnan (Ctrl->N.mask[0])) ? GMT_message (GMT, "NaN\n") : GMT_message (GMT, line, Ctrl->N.mask[0]);
			GMT_report (GMT, GMT_MSG_VERBOSE, "Nodes on land will be set to ");
			(GMT_is_dnan (Ctrl->N.mask[1])) ? GMT_message (GMT, "NaN\n") : GMT_message (GMT, line, Ctrl->N.mask[1]);
			GMT_report (GMT, GMT_MSG_VERBOSE, "Nodes in lakes will be set to ");
			(GMT_is_dnan (Ctrl->N.mask[2])) ? GMT_message (GMT, "NaN\n") : GMT_message (GMT, line, Ctrl->N.mask[2]);
			GMT_report (GMT, GMT_MSG_VERBOSE, "Nodes in islands will be set to ");
			(GMT_is_dnan (Ctrl->N.mask[3])) ? GMT_message (GMT, "NaN\n") : GMT_message (GMT, line, Ctrl->N.mask[3]);
			GMT_report (GMT, GMT_MSG_VERBOSE, "Nodes in ponds will be set to ");
			(GMT_is_dnan (Ctrl->N.mask[4])) ? GMT_message (GMT, "NaN\n") : GMT_message (GMT, line, Ctrl->N.mask[4]);
		}
	}

	/* All data nodes are thus initialized to 0 */
	x = GMT_memory (GMT, NULL, Grid->header->nx, double);
	y = GMT_memory (GMT, NULL, Grid->header->ny, double);

	nx1 = Grid->header->nx - 1;	ny1 = Grid->header->ny - 1;

	GMT_parse_common_options (GMT, "J", 'J', "x1d");	/* Fake linear projection so the shore machinery will work */
	GMT_err_fail (GMT, GMT_map_setup (GMT, Grid->header->wesn), "");
	GMT->current.map.parallel_straight = GMT->current.map.meridian_straight = 2;	/* No resampling along bin boundaries */
	wrap = GMT->current.map.is_world = GMT_grd_is_global (GMT, Grid->header);
	/* Using -Jx1d means output is Cartesian but we want to force geographic */
	GMT->current.io.col_type[GMT_OUT][GMT_X] = GMT_IS_LON;	GMT->current.io.col_type[GMT_OUT][GMT_Y] = GMT_IS_LAT;

	/* Fill out gridnode coordinates and apply the implicit linear projection */

	for (col = 0; col <= nx1; col++) GMT_geo_to_xy (GMT, GMT_grd_col_to_x (GMT, col, Grid->header), 0.0, &x[col], &dummy);
	for (row = 0; row <= ny1; row++) GMT_geo_to_xy (GMT, 0.0, GMT_grd_row_to_y (GMT, row, Grid->header), &dummy, &y[row]);
	i_dx_inch = 1.0 / fabs (x[1] - x[0]);
	i_dy_inch = 1.0 / fabs (y[1] - y[0]);

	west_border = floor (GMT->common.R.wesn[XLO] / c.bsize) * c.bsize;
	east_border =  ceil (GMT->common.R.wesn[XHI] / c.bsize) * c.bsize;
	for (ind = 0; ind < c.nb; ind++) {	/* Loop over necessary bins only */

		bin = c.bins[ind];
		GMT_report (GMT, GMT_MSG_VERBOSE, "Working on block # %5ld\r", bin);

		if ((err = GMT_get_shore_bin (GMT, ind, &c))) {
			GMT_report (GMT, GMT_MSG_NORMAL, "%s [%s resolution shoreline]\n", GMT_strerror(err), shore_resolution[base]);
			Return (EXIT_FAILURE);
		}

		/* Use polygons, if any.  Go in both directions to cover both land and sea */

		used_polygons = false;

		for (direction = -1; c.ns > 0 && direction < 2; direction += 2) {

			/* Assemble one or more segments into polygons */

			np = GMT_assemble_shore (GMT, &c, direction, true, west_border, east_border, &p);

			/* Get clipped polygons in x,y inches that can be processed */

			np_new = GMT_prep_shore_polygons (GMT, &p, np, false, 0.0, -1);

			for (k = 0; k < np_new; k++) {

				if (p[k].n == 0) continue;

				used_polygons = true;	/* At least som points made it to here */

				/* Find min/max of polygon in inches */

				xmin = xmax = p[k].lon[0];
				ymin = ymax = p[k].lat[0];
				for (i = 1; i < p[k].n; i++) {
					if (p[k].lon[i] < xmin) xmin = p[k].lon[i];
					if (p[k].lon[i] > xmax) xmax = p[k].lon[i];
					if (p[k].lat[i] < ymin) ymin = p[k].lat[i];
					if (p[k].lat[i] > ymax) ymax = p[k].lat[i];
				}
				col_min = MAX (0, lrint (ceil (xmin * i_dx_inch - Grid->header->xy_off - GMT_CONV_LIMIT)));
				if (col_min > nx1) col_min = 0;
				/* So col_min is in range [0,nx1] */
				col_max = MIN (nx1, lrint (floor (xmax * i_dx_inch - Grid->header->xy_off + GMT_CONV_LIMIT)));
				if (col_max <= 0 || col_max < col_min) col_max = nx1;
				/* So col_max is in range [1,nx1] */
				row_min = MAX (0, lrint (ceil ((GMT->current.proj.rect[YHI] - ymax) * i_dy_inch - Grid->header->xy_off - GMT_CONV_LIMIT)));
				/* So row_min is in range [0,?] */
				row_max = MIN (ny1, lrint (floor ((GMT->current.proj.rect[YHI] - ymin) * i_dy_inch - Grid->header->xy_off + GMT_CONV_LIMIT)));
				/* So row_max is in range [?,ny1] */

				for (row = row_min; row <= row_max; row++) {
					assert (row >= 0);	/* Just in case we have a logic bug somewhere */
					for (col = col_min; col <= col_max; col++) {

						if ((side = GMT_non_zero_winding (GMT, x[col], y[row], p[k].lon, p[k].lat, p[k].n)) < Ctrl->E.inside) continue;	/* Outside */

						/* Here, point is inside, we must assign value */

						ij = GMT_IJP (Grid->header, row, col);
						if (p[k].level > Grid->data[ij]) Grid->data[ij] = (float)p[k].level;
					}
				}
			}

			GMT_free_shore_polygons (GMT, p, np_new);
			GMT_free (GMT, p);
		}

		if (!used_polygons) {	/* Lack of polygons or clipping etc resulted in no polygons after all, must deal with background */

			k = INT_MAX;	/* Initialize to outside range of levels (4 is highest) */
			/* Visit each of the 4 nodes, test if it is inside -R, and if so update lowest level found so far */

			if (!GMT_map_outside (GMT, c.lon_sw, c.lat_sw)) k = MIN (k, c.node_level[0]);				/* SW */
			if (!GMT_map_outside (GMT, c.lon_sw + c.bsize, c.lat_sw)) k = MIN (k, c.node_level[1]);			/* SE */
			if (!GMT_map_outside (GMT, c.lon_sw + c.bsize, c.lat_sw - c.bsize)) k = MIN (k, c.node_level[2]);	/* NE */
			if (!GMT_map_outside (GMT, c.lon_sw, c.lat_sw - c.bsize)) k = MIN (k, c.node_level[3]);			/* NW */

			/* If k is still INT_MAX we must assume this patch should have the min level of the bin */

			if (k == INT_MAX) k = MIN (MIN (c.node_level[0], c.node_level[1]) , MIN (c.node_level[2], c.node_level[3]));

			/* Determine nodes to initialize */

			row_min = MAX (0, lrint (ceil ((Grid->header->wesn[YHI] - c.lat_sw - c.bsize) * Grid->header->r_inc[GMT_Y] - Grid->header->xy_off)));
			row_max = MIN (ny1, lrint (floor ((Grid->header->wesn[YHI] - c.lat_sw) * Grid->header->r_inc[GMT_Y] - Grid->header->xy_off)));
			col_min = lrint (ceil (fmod (c.lon_sw - Grid->header->wesn[XLO], 360.0) * Grid->header->r_inc[GMT_X] - Grid->header->xy_off));
			col_max = lrint (floor (fmod (c.lon_sw + c.bsize - Grid->header->wesn[XLO], 360.0) * Grid->header->r_inc[GMT_X] - Grid->header->xy_off));
			if (wrap) {	/* Handle jumps */
				if (col_max < col_min) col_max += Grid->header->nx;
			}
			else {	/* Make sure we are inside our grid */
				if (col_min < 0) col_min = 0;
				if (col_max > nx1) col_max = nx1;
			}
			for (row = row_min; row <= row_max; row++) {
				for (col = col_min; col <= col_max; col++) {
					ii = (wrap) ? col % (int)Grid->header->nx : col;
					if (ii < 0 || ii > nx1) continue;
					ij = GMT_IJP (Grid->header, row, ii);
					Grid->data[ij] = (float)k;
				}
			}
		}

		GMT_free_shore (GMT, &c);
	}

	GMT_shore_cleanup (GMT, &c);
	GMT_free (GMT, x);
	GMT_free (GMT, y);

	GMT_grd_loop (GMT, Grid, row, col, ij) {	/* Turn levels into mask values */
		k = lrint (Grid->data[ij]);
		Grid->data[ij] = (float)Ctrl->N.mask[k];
	}

	if (wrap && Grid->header->registration == GMT_GRID_NODE_REG) { /* Copy over values to the repeating right column */
		unsigned int row_l;
		for (row_l = 0, ij = GMT_IJP (Grid->header, row_l, 0); row_l < Grid->header->ny; row_l++, ij += Grid->header->mx) Grid->data[ij+nx1] = Grid->data[ij];
	}
	
	if (temp_shift) {
		Grid->header->wesn[XLO] -= 360.0;
		Grid->header->wesn[XHI] -= 360.0;
	}

	sprintf (line, "Derived from the %s resolution shorelinen", shore_resolution[base]);
	if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_REMARK, line, Grid)) return (API->error);
	if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->G.file, Grid) != GMT_OK) {
		Return (API->error);
	}

	GMT_report (GMT, GMT_MSG_VERBOSE, "Done!\n");

	Return (GMT_OK);
}
