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
 * API functions to support the gmtselect application.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 * Brief synopsis: gmtselect is a general-purpose spatial filter.  Data pass
 * or fail basedon one or more conditions.  Six conditions may be set:
 *
 *	1. Only data inside a rectangular area may pass
 *	2. Only data within a certain distance from given points may pass
 *	3. Only data within a certain distance from given lines may pass
 *	4. Only data within given polygons may pass
 *	5. Only data within the coastline may pass
 *	6. Only data with z-values within specified range may pass
 *
 * Distances are calculated in the users units using Euclidian geometry
 * unless a map projection and region (-R -J) are used.  Then, distances
 * are calculated using spherical geometry and converted to km, and any
 * distances given in options or via headers are assumed to be in km.
 *
 * Any one of these conditions may be negated for the opposite result
 * Both binary and ASCII data files are accommodated
 */
 
#define THIS_MODULE k_mod_gmtselect /* I am gmtselect */

#include "gmt_dev.h"

#define GMT_PROG_OPTIONS "-:>JRVabfghios" GMT_OPT("HMm")

#define GMTSELECT_N_TESTS	6				/* Number of specific tests available */
#define GMTSELECT_N_CLASSES	(GSHHS_MAX_LEVEL + 1)	/* Number of bands separated by the levels */

#define F_ITEM	0
#define N_ITEM	1

struct GMTSELECT_DATA {	/* Used for temporary storage when sorting data on x coordinate */
	double x, y, d;
};

struct GMTSELECT_CTRL {	/* All control options for this program (except common args) */
	/* active is true if the option has been activated */
	struct A {	/* -A<min_area>[/<min_level>/<max_level>] */
		bool active;
		struct GMT_SHORE_SELECT info;
	} A;
	struct C {	/* [-C[-|=|+]<dist>[unit]/<ptfile>] */
		bool active;
		int mode;	/* Form of distance calculation (can be negative) */
		double dist;	/* Radius of influence for each point */
		char unit;	/* Unit name */
		char *file;	/* Name of file with points */
	} C;
	struct D {	/* -D<resolution> */
		bool active;
		bool force;	/* if true, select next highest level if current set is not avaialble */
		char set;	/* One of f, h, i, l, c */
	} D;
	struct E {	/* -E<operators> , <op> = combination or f,n */
		bool active;
		unsigned int inside[2];	/* if 2, then a point exactly on a polygon boundary is considered OUTSIDE, else 1 */
	} E;
	struct L {	/* -L[p][-|=|+]<dist>[unit]/<lfile> */
		bool active;
		unsigned int end_mode;	/* Controls what happens beyond segment endpoints */
		int mode;	/* Form of distance calculation (can be negative) */
		double dist;	/* Distance of influence for each line */
		char unit;	/* Unit name */
		char *file;	/* Name of file with lines */
	} L;
	struct F {	/* -F<polygon> */
		bool active;
		char *file;	/* Name of file with polygons */
	} F;
	struct I {	/* -Icflrsz */
		bool active;
		bool pass[GMTSELECT_N_TESTS];	/* One flag for each setting */
	} I;
	struct N {	/* -N<maskvalues> */
		bool active;
		unsigned int mode;	/* 1 if dry/wet only, 0 if 5 mask levels */
		bool mask[GMTSELECT_N_CLASSES];	/* Mask for each level */
	} N;
	struct Z {	/* -Z<min>/<max> */
		bool active;
		double min;	/* Smallest z-value to pass through */
		double max;	/* Largest z-value to pass through */
	} Z;
	struct dbg {	/* -+step */
		bool active;
		double step;
	} dbg;
};

void *New_gmtselect_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	int i;
	struct GMTSELECT_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct GMTSELECT_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
	
	C->A.info.high = GSHHS_MAX_LEVEL;				/* Include all GSHHS levels */
	C->D.set = 'l';							/* Low-resolution coastline data */
	C->E.inside[F_ITEM] = C->E.inside[N_ITEM] = GMT_ONEDGE;		/* Default is that points on a boundary are inside */
	for (i = 0; i < GMTSELECT_N_TESTS; i++) C->I.pass[i] = true;	/* Default is to pass if we are inside */
	GMT_memset (C->N.mask, GMTSELECT_N_CLASSES, bool);		/* Default for "wet" areas = false (outside) */
	C->N.mask[1] = C->N.mask[3] = true;				/* Default for "dry" areas = true (inside) */
	C->Z.min = -DBL_MAX;	C->Z.max = DBL_MAX;			/* No limits on z-range */
	
	return (C);
}

void Free_gmtselect_Ctrl (struct GMT_CTRL *GMT, struct GMTSELECT_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->C.file) free (C->C.file);	
	if (C->F.file) free (C->F.file);	
	if (C->L.file) free (C->L.file);	
	GMT_free (GMT, C);	
}

int compare_x (const void *point_1, const void *point_2)
{
	const struct GMTSELECT_DATA *p1 = point_1, *p2 = point_2;

	if (p1->x < p2->x) return (-1);
	if (p1->x > p2->x) return (1);
	return (0);
}

int GMT_gmtselect_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: gmtselect [<table>] [%s]\n", GMT_A_OPT);
	GMT_message (GMT, "\t[-C%s/<ptfile>] [-D<resolution>][+] [-E[f][n]] [-F<polygon>] [%s]\n", GMT_DIST_OPT, GMT_J_OPT);
	GMT_message (GMT, "\t[-I[cflrsz] [-L[p]%s/<lfile>] [-N<info>] [%s]\n\t[%s] [%s] [-Z<min>/<max>] [%s]\n\t[%s] [%s] [%s]\n\t[%s] [%s] [%s] [%s]\n\n",
		GMT_DIST_OPT, GMT_Rgeo_OPT, GMT_V_OPT, GMT_a_OPT, GMT_b_OPT, GMT_f_OPT, GMT_g_OPT, GMT_h_OPT, GMT_i_OPT, GMT_o_OPT, GMT_s_OPT, GMT_colon_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "<");
	GMT_GSHHS_syntax (GMT, 'A', "Place limits on coastline features from the GSHHS data base (ignored  unless -N is set).");
	GMT_dist_syntax (GMT, 'C', "Pass locations that are within <dist> of any point in the ASCII <ptfile>.");
	GMT_message (GMT, "\t   Give distance as 0 if 3rd column of <ptfile> has individual distances.\n");
	GMT_message (GMT, "\t   Use -R -J to compute mapped Cartesian distances in cm, inch, m, or points [%s].\n", GMT->session.unit_name[GMT->current.setting.proj_length_unit]);
	GMT_message (GMT, "\t-D Choose one of the following resolutions: (Ignored unless -N is set).\n");
	GMT_message (GMT, "\t   f - full resolution (may be very slow for large regions).\n");
	GMT_message (GMT, "\t   h - high resolution (may be slow for large regions).\n");
	GMT_message (GMT, "\t   i - intermediate resolution.\n");
	GMT_message (GMT, "\t   l - low resolution [Default].\n");
	GMT_message (GMT, "\t   c - crude resolution, for tasks that need crude continent outlines only.\n");
	GMT_message (GMT, "\t   Append + to use a lower resolution should the chosen one not be available [abort].\n");
	GMT_message (GMT, "\t-E Indicate if points exactly on a polygon boundary are inside or outside.\n");
	GMT_message (GMT, "\t   Append f and/or n to modify the -F option or -N option, respectively,\n");
	GMT_message (GMT, "\t   to consider such points to be outside the feature [inside].\n");
	GMT_message (GMT, "\t   Give distance as 0 if 2nd column of segment headers have individual distances.\n");
	GMT_message (GMT, "\t   Use -R -J to compute mapped Cartesian distances in cm, inch, m, or points [%s].\n", GMT->session.unit_name[GMT->current.setting.proj_length_unit]);
	GMT_message (GMT, "\t   Optionally, use -Lp to exclude points projecting beyond a line's endpoints.\n");
	GMT_message (GMT, "\t-F Pass locations that are inside the polygons in the ASCII <polygon> file.\n");
	GMT_message (GMT, "\t-I Reverse the tests, i.e., pass locations outside the region.\n");
	GMT_message (GMT, "\t   Supply a combination of cflrz where each flag means:\n");
	GMT_message (GMT, "\t   c will pass locations beyond the minimum distance to the points in -C.\n");
	GMT_message (GMT, "\t   f will pass locations outside the polygons in -F.\n");
	GMT_message (GMT, "\t   l will pass locations beyond the minimum distance to the lines in -L.\n");
	GMT_message (GMT, "\t   r will pass locations outside the region given in -R [and -J].\n");
	GMT_message (GMT, "\t   s will pass locations that otherwise would be skipped in -N.\n");
	GMT_message (GMT, "\t   z will pass locations outside the range given in -Z.\n");
	GMT_explain_options (GMT, "J");
	GMT_dist_syntax (GMT, 'L', "Pass locations that are within <dist> of any line in ASCII <linefile>.");
	GMT_message (GMT, "\t-N Set if a point outside or inside a geographic feature should be s(kipped) or k(ept).\n");
	GMT_message (GMT, "\t   Append o to let feature boundary be considered outside [Default is inside].\n");
	GMT_message (GMT, "\t   Specify this information with s or k using 1 of 2 formats:\n");
	GMT_message (GMT, "\t   -N<wet>/<dry>.\n");
	GMT_message (GMT, "\t   -N<ocean>/<land>/<lake>/<island>/<pond>.\n");
	GMT_message (GMT, "\t   k means keep and s means skip [Default is s/k/s/k/s (i.e., s/k)].\n");
	GMT_explain_options (GMT, "RV");
	GMT_message (GMT, "\t-Z Assume the 3rd data column contains z-values and we want to keep records with\n");
	GMT_message (GMT, "\t   <min> <= z <= <max>.  Use - for <min> or <max> if there is no lower/upper limit.\n");
	GMT_explain_options (GMT, "aC0");
	GMT_message (GMT, "\t   Default is 2 input columns (3 if -Z is used).\n");
	GMT_explain_options (GMT, "D0fghios:.");
	
	return (EXIT_FAILURE);
}

int GMT_gmtselect_parse (struct GMTAPI_CTRL *C, struct GMTSELECT_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to gmtselect and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, pos, j, k;
	char ptr[GMT_BUFSIZ], buffer[GMT_BUFSIZ], za[GMT_TEXT_LEN64], zb[GMT_TEXT_LEN64];
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;
#ifdef GMT_COMPAT
	bool fix = false;
#endif

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Skip input files */
				break;

			/* Processes program-specific parameters */

			case 'A':	/* Limit GSHHS features */
				Ctrl->A.active = true;
				GMT_set_levels (GMT, opt->arg, &Ctrl->A.info);
				break;
			case 'C':	/* Near a point test */
				Ctrl->C.active = true;
#ifdef GMT_COMPAT
				if (opt->arg[0] == 'f') {
					GMT_report (GMT, GMT_MSG_COMPAT, "Warning: Option -Cf is deprecated; use -C- instead\n");
					opt->arg[0] = '-';
					fix = true;
				}
#endif
				for (j = 0; opt->arg[j] && opt->arg[j] != '/'; j++);
				if (opt->arg[j]) {
					Ctrl->C.file = strdup (&opt->arg[j+1]);
					opt->arg[j] = '\0';	/* Chop off the /filename part */
					Ctrl->C.mode = GMT_get_distance (GMT, opt->arg, &(Ctrl->C.dist), &(Ctrl->C.unit));
					opt->arg[j] = '/';	/* Restore the /filename part */
				}
				else {
					GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -C option: Expects -C%s/<file>\n", GMT_DIST_OPT);
					n_errors++;
				}
#ifdef GMT_COMPAT
				if (fix) opt->arg[0] = 'f';
#endif
				break;
			case 'D':	/* Set GSHHS resolution */
				Ctrl->D.active = true;
				Ctrl->D.set = opt->arg[0];
				if (opt->arg[1] == '+') Ctrl->D.force = true;
				break;
			case 'E':	/* On-boundary selection */
				Ctrl->E.active = true;
				for (j = 0; opt->arg[j]; j++) {
					switch (opt->arg[j]) {
						case 'f':
							Ctrl->E.inside[F_ITEM] = GMT_INSIDE;
							break;
						case 'n':
							Ctrl->E.inside[N_ITEM] = GMT_INSIDE;
							break;
						default:
							GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -E option: Expects -Ef, -En, or -Efn\n");
							n_errors++;
							break;
					}
				}
				break;
			case 'F':	/* Inside/outside polygon test */
				Ctrl->F.active = true;
				Ctrl->F.file = strdup (opt->arg);
				break;
			case 'I':	/* Invert these tests */
				Ctrl->I.active = true;
				for (j = 0; opt->arg[j]; j++) {
					switch (opt->arg[j]) {
						case 'r':
							Ctrl->I.pass[0] = false;
							break;
						case 'c':
							Ctrl->I.pass[1] = false;
							break;
						case 'l':
							Ctrl->I.pass[2] = false;
							break;
						case 'f':
							Ctrl->I.pass[3] = false;
							break;
						case 's':
							Ctrl->I.pass[4] = false;
							break;
						case 'z':
							Ctrl->I.pass[5] = false;
							break;
						default:
							GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -I option: Expects -Icflrsz\n");
							n_errors++;
							break;
					}
				}
				break;
			case 'L':	/* Near a line test */
				if (opt->arg[0]) {	/* Set line options */
					Ctrl->L.active = true;
					k = 0;
					if (opt->arg[k] == 'p') {	/* Disallow points beyond endpoints */
						Ctrl->L.end_mode = 10;
						k++;
					}
					for (j = k; opt->arg[j] && opt->arg[j] != '/'; j++);
					if (!opt->arg[j]) {
						GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -L option: Expects -L[p]%s/<file>\n", GMT_DIST_OPT);
						n_errors++;
					}
					else {
						Ctrl->L.file = strdup (&opt->arg[j+1]);
						opt->arg[j] = '\0';	/* Chop off the /filename part */
						Ctrl->L.mode = GMT_get_distance (GMT, &opt->arg[k], &(Ctrl->L.dist), &(Ctrl->L.unit));
						opt->arg[j] = '/';	/* Restore the /filename part */
					}
				}
				break;
			case 'N':	/* Inside/outside GSHHS land */
				Ctrl->N.active = true;
				strncpy (buffer, opt->arg, GMT_BUFSIZ);
#ifdef GMT_COMPAT
				if (buffer[strlen(buffer)-1] == 'o') { /* Edge is considered outside */
					GMT_report (GMT, GMT_MSG_COMPAT, "Warning: Option -N...o is deprecated; use -E instead\n");
					Ctrl->E.active = true;
					Ctrl->E.inside[N_ITEM] = GMT_INSIDE;
					buffer[strlen(buffer)-1] = 0;
				}
#endif
				j = pos = 0;
				while (j < GMTSELECT_N_CLASSES && (GMT_strtok (buffer, "/", &pos, ptr))) {
					switch (ptr[0]) {
						case 's':	/* Skip points in this level */
							Ctrl->N.mask[j] = false;
							break;
						case 'k':
							Ctrl->N.mask[j] = true;
							break;
						default:
							GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -N option: Bad modifier (use s or k)\n");
							n_errors++;
					}
					j++;
				}
				if (!(j == 2 || j == GMTSELECT_N_CLASSES)) {
					GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -N option: Specify 2 or 5 arguments\n");
					n_errors++;
				}
				Ctrl->N.mode = (j == 2);
				break;
			case 'Z':	/* Test z-range */
				Ctrl->Z.active = true;
				j = sscanf (opt->arg, "%[^/]/%s", za, zb);
				if (j != 2) {
					GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -Z option: Specify z_min and z_max\n");
					n_errors++;
				}
				if (!(za[0] == '-' && za[1] == '\0')) n_errors += GMT_verify_expectations (GMT, GMT->current.io.col_type[GMT_IN][GMT_Z], GMT_scanf_arg (GMT, za, GMT->current.io.col_type[GMT_IN][GMT_Z], &Ctrl->Z.min), za);
				if (!(zb[0] == '-' && zb[1] == '\0')) n_errors += GMT_verify_expectations (GMT, GMT->current.io.col_type[GMT_IN][GMT_Z], GMT_scanf_arg (GMT, zb, GMT->current.io.col_type[GMT_IN][GMT_Z], &Ctrl->Z.max), zb);
				break;
#ifdef DEBUG
			case '+':	/* Undocumented option to increase path-fix resolution */
				Ctrl->dbg.active = true;
				Ctrl->dbg.step = atof (opt->arg);
				break;
#endif
			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, Ctrl->C.mode == -1, "Syntax error -C: Unrecognized distance unit\n");
	n_errors += GMT_check_condition (GMT, Ctrl->C.mode == -2, "Syntax error -C: Unable to decode distance\n");
	n_errors += GMT_check_condition (GMT, Ctrl->C.mode == -3, "Syntax error -C: Distance is negative\n");
	n_errors += GMT_check_condition (GMT, Ctrl->C.active && GMT_access (GMT, Ctrl->C.file, R_OK), "Syntax error -C: Cannot read file %s!\n", Ctrl->C.file);
	n_errors += GMT_check_condition (GMT, Ctrl->F.active && GMT_access (GMT, Ctrl->F.file, R_OK), "Syntax error -F: Cannot read file %s!\n", Ctrl->F.file);
	n_errors += GMT_check_condition (GMT, Ctrl->L.active && GMT_access (GMT, Ctrl->L.file, R_OK), "Syntax error -L: Cannot read file %s!\n", Ctrl->L.file);
	n_errors += GMT_check_condition (GMT, Ctrl->L.mode == -1, "Syntax error -L: Unrecognized distance unit\n");
	n_errors += GMT_check_condition (GMT, Ctrl->L.mode == -2, "Syntax error -L: Unable to decode distance\n");
	n_errors += GMT_check_condition (GMT, Ctrl->L.mode == -3, "Syntax error -L: Distance is negative\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->N.active && (Ctrl->A.active || Ctrl->D.active), "Syntax error: -A and -D requires -N!\n");
	n_errors += GMT_check_condition (GMT, Ctrl->Z.active && Ctrl->Z.max <= Ctrl->Z.min, "Syntax error: -Z must have zmax > zmin!\n");
	n_errors += GMT_check_condition (GMT, Ctrl->L.active && Ctrl->C.active && !(Ctrl->C.mode == Ctrl->L.mode && Ctrl->C.unit == Ctrl->L.unit), "Syntax error: If both -C and -L are used they must use the same distance unit and calculation mode\n");
	n_errors += GMT_check_binary_io (GMT, 2 + Ctrl->Z.active);
	
	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

/* Must free allocated memory before returning */
#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_gmtselect_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_gmtselect (void *V_API, int mode, void *args)
{
	int err;	/* Required by GMT_err_fail */
	unsigned int base = 3, np[2] = {0, 0}, r_mode;
	unsigned int side, col, id;
	int n_fields, ind, wd[2] = {0, 0}, n_minimum = 2, bin, last_bin = INT_MAX, error = 0;
	bool inside, need_header = false, shuffle, just_copy_record = false, pt_cartesian = false;
	bool output_header = false, do_project = false, no_resample = false;

	uint64_t k, row, seg, n_read = 0, n_pass = 0;

	double xx, yy, *in = NULL;
	double west_border = 0.0, east_border = 0.0, xmin, xmax, ymin, ymax, lon;

	char *shore_resolution[5] = {"full", "high", "intermediate", "low", "crude"};

	struct GMT_DATATABLE *pol = NULL, *line = NULL, *point = NULL;
	struct GMT_GSHHS_POL *p[2] = {NULL, NULL};
	struct GMT_SHORE c;
	struct GMT_DATASET *Cin = NULL, *Lin = NULL, *Fin = NULL;
	struct GMTSELECT_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_gmtselect_usage (API, GMTAPI_USAGE));/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_gmtselect_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, GMT_PROG_OPTIONS, options)) Return (API->error);
	Ctrl = New_gmtselect_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_gmtselect_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the gmtselect main code ----------------------------*/

	GMT_report (GMT, GMT_MSG_VERBOSE, "Processing input table data\n");

	if (Ctrl->C.active && !GMT_is_geographic (GMT, GMT_IN)) pt_cartesian = true;

	shuffle = (GMT->current.setting.io_lonlat_toggle[GMT_IN] != GMT->current.setting.io_lonlat_toggle[GMT_OUT]);	/* Must rewrite output record */
	n_minimum = (Ctrl->Z.active) ? 3 : 2;	/* Minimum number of columns in ASCII input */
	
	if (!GMT->common.R.active && Ctrl->N.active) {	/* If we use coastline data or used -fg but didnt give -R we implicitly set -Rg */
		GMT->common.R.active = true;
		GMT->common.R.wesn[XLO] = 0.0;	GMT->common.R.wesn[XHI] = 360.0;	GMT->common.R.wesn[YLO] = -90.0;	GMT->common.R.wesn[YHI] = +90.0;
		GMT->current.io.col_type[GMT_IN][GMT_X] = GMT_IS_LON;
		GMT->current.io.col_type[GMT_IN][GMT_Y] = GMT_IS_LAT;
	}
	if (GMT->common.R.active) {	/* -R was set directly or indirectly; hence must set -J if not supplied */
		if (!GMT->common.J.active) {	/* -J not specified, set one implicitly */
			/* Supply dummy linear proj */
			GMT->current.proj.projection = GMT->current.proj.xyz_projection[GMT_X] = GMT->current.proj.xyz_projection[GMT_Y] = GMT_LINEAR;
			GMT->current.proj.pars[0] = GMT->current.proj.pars[1] = 1.0;
			GMT->common.J.active = no_resample = true;
		}
		else
			do_project = true;	/* Only true when the USER selected -J, not when we supply a dummy -Jx1d */
		Ctrl->dbg.step = 0.01;
		if (GMT_is_geographic (GMT, GMT_IN)) {
			while (GMT->common.R.wesn[XLO] < 0.0 && GMT->common.R.wesn[XHI] < 0.0) {	/* Make all-negative longitude range positive instead */
				GMT->common.R.wesn[XLO] += 360.0;
				GMT->common.R.wesn[XHI] += 360.0;
			}
		}
		GMT_err_fail (GMT, GMT_map_setup (GMT, GMT->common.R.wesn), "");
		if (no_resample) GMT->current.map.parallel_straight = GMT->current.map.meridian_straight = 2;	/* No resampling along bin boundaries */
	}

	if (do_project) GMT_report (GMT, GMT_MSG_VERBOSE, "Warning: -J means all data will be projected before tests are applied\n");
	 
	if (Ctrl->N.active) {	/* Set up GSHHS */
		if (Ctrl->D.force) Ctrl->D.set = GMT_shore_adjust_res (GMT, Ctrl->D.set);
		if (Ctrl->D.active) base = GMT_set_resolution (GMT, &Ctrl->D.set, 'D');
		if (Ctrl->N.mode) {	/* Post-process -N choice */
			Ctrl->N.mask[3] = Ctrl->N.mask[1];
			Ctrl->N.mask[2] = Ctrl->N.mask[4] = Ctrl->N.mask[0];
		}
		if (GMT_init_shore (GMT, Ctrl->D.set, &c, GMT->common.R.wesn, &Ctrl->A.info)) {
			GMT_report (GMT, GMT_MSG_NORMAL, "%s resolution shoreline data base not installed\n", shore_resolution[base]);
			Return (EXIT_FAILURE);
		}
		GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "GSHHG version %s\n%s\n%s\n", c.version, c.title, c.source);
		west_border = floor (GMT->common.R.wesn[XLO] / c.bsize) * c.bsize;
		east_border = ceil (GMT->common.R.wesn[XHI] / c.bsize) * c.bsize;
		wd[0] = 1;	wd[1] = -1;
		np[0] = np[1] = 0;
	}

	just_copy_record = (GMT_is_ascii_record (GMT) && !shuffle);

	/* Initiate pointer to distance calculation function */
	if (GMT_is_geographic (GMT, GMT_IN) && !do_project) {	/* Geographic data and no -R -J conversion */
		if (Ctrl->C.active)
			GMT_init_distaz (GMT, Ctrl->C.unit, Ctrl->C.mode, GMT_MAP_DIST);
		else if (Ctrl->L.active)
			GMT_init_distaz (GMT, Ctrl->L.unit, Ctrl->L.mode, GMT_MAP_DIST);
	}
	else if (do_project)	/* Lon/lat projected via -R -J */
		GMT_init_distaz (GMT, 'Z', 0, GMT_MAP_DIST);	/* Compute r-squared instead of r after projection to avoid hypot */
	else	/* Cartesian data */
		GMT_init_distaz (GMT, 'R', 0, GMT_MAP_DIST);	/* Compute r-squared instead of r to avoid hypot  */
	
	if (Ctrl->C.active) { 	/* Initialize point structure used in test for proximity to points [use Ctrl->C.dist ]*/
		if ((Cin = GMT_Read_Data (API, GMT_IS_DATASET, GMT_IS_FILE, GMT_IS_POINT, GMT_IO_ASCII, NULL, Ctrl->C.file, NULL)) == NULL) {
			Return (API->error);
		}
		if (Cin->n_columns < 2) {	/* Trouble */
			GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -C option: %s does not have at least 2 columns with coordinates\n", Ctrl->C.file);
			Return (EXIT_FAILURE);
		}
		if (Ctrl->C.dist == 0.0 && Cin->n_columns <= 2) {	/* Trouble */
			GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -C option: %s does not have a 3rd column with distances, yet -C0/<file> was given\n", Ctrl->C.file);
			Return (EXIT_FAILURE);
		}
		point = Cin->table[0];	/* Can only be one table since we read a single file */
		for (seg = 0; seg < point->n_segments; seg++) {
			if (Cin->n_columns == 2) point->segment[seg]->dist = Ctrl->C.dist;
			if (do_project) {	/* Convert all the points using the map projection */
				for (row = 0; row < point->segment[seg]->n_rows; row++) {
					GMT_geo_to_xy (GMT, point->segment[seg]->coord[GMT_X][row], point->segment[seg]->coord[GMT_Y][row], &xx, &yy);
					point->segment[seg]->coord[GMT_X][row] = xx;
					point->segment[seg]->coord[GMT_Y][row] = yy;
				}
				pt_cartesian = true;	/* Well, now it is */
			}
		}
		if (pt_cartesian) {	/* Speed up testing by sorting points on the x-coordinate first */
			struct GMTSELECT_DATA *data = NULL;	/* Used for temporary storage when sorting data on x coordinate */

			/* Copy xp into struct data, sort, and copy back */

			data = GMT_memory (GMT, NULL, point->n_records, struct GMTSELECT_DATA);

			for (seg = k = 0; seg < point->n_segments; seg++) {
				for (row = 0; row < point->segment[seg]->n_rows; row++, k++) {
					data[k].x = point->segment[seg]->coord[GMT_X][row];
					data[k].y = point->segment[seg]->coord[GMT_Y][row];
					data[k].d = (Ctrl->C.dist == 0.0) ? point->segment[seg]->coord[GMT_Z][row] : Ctrl->C.dist;
				}
			}
			
			/* Sort on x to speed up inside testing */
			qsort (data, point->n_records, sizeof (struct GMTSELECT_DATA), compare_x);
			
			for (seg = k = 0; seg < point->n_segments; seg++) {	/* Put back the new order */
				for (row = 0; row < point->segment[seg]->n_rows; row++, k++) {
					point->segment[seg]->coord[GMT_X][row] = data[k].x;
					point->segment[seg]->coord[GMT_Y][row] = data[k].y;
					if (Ctrl->C.dist == 0.0) point->segment[seg]->coord[GMT_Z][row] = data[k].d ;
				}
			}
			GMT_free (GMT, data);
		}
	}

	if (Ctrl->L.active) {	/* Initialize lines structure used in test for proximity to lines [use Ctrl->L.dist, ] */
		if ((Lin = GMT_Read_Data (API, GMT_IS_DATASET, GMT_IS_FILE, GMT_IS_LINE, GMT_IO_ASCII, NULL, Ctrl->L.file, NULL)) == NULL) {
			Return (API->error);
		}
		if (Lin->n_columns < 2) {	/* Trouble */
			GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -L option: %s does not have at least 2 columns with coordinates\n", Ctrl->L.file);
			Return (EXIT_FAILURE);
		}
		line = Lin->table[0];	/* Can only be one table since we read a single file */
		for (seg = 0; seg < line->n_segments; seg++) {
			if (Ctrl->L.dist > 0.0) line->segment[seg]->dist = Ctrl->L.dist;	/* Only override when nonzero */
			if (do_project) {	/* Convert all the line points using the map projection */
				for (row = 0; row < line->segment[seg]->n_rows; row++) {
					GMT_geo_to_xy (GMT, line->segment[seg]->coord[GMT_X][row], line->segment[seg]->coord[GMT_Y][row], &xx, &yy);
					line->segment[seg]->coord[GMT_X][row] = xx;
					line->segment[seg]->coord[GMT_Y][row] = yy;
				}
			}
		}
	}
	if (Ctrl->F.active) {	/* Initialize polygon structure used in test for polygon in/out test */
		GMT_skip_xy_duplicates (GMT, true);	/* Avoid repeating x/y points in polygons */
		if ((Fin = GMT_Read_Data (API, GMT_IS_DATASET, GMT_IS_FILE, GMT_IS_POLY, GMT_IO_ASCII, NULL, Ctrl->F.file, NULL)) == NULL) {
			Return (API->error);
		}
		GMT_skip_xy_duplicates (GMT, false);	/* Reset */
		if (Fin->n_columns < 2) {	/* Trouble */
			GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -F option: %s does not have at least 2 columns with coordinates\n", Ctrl->F.file);
			Return (EXIT_FAILURE);
		}
		pol = Fin->table[0];	/* Can only be one table since we read a single file */
		if (do_project) {	/* Convert all the polygons points using the map projection */
			for (seg = 0; seg < pol->n_segments; seg++) {
				for (row = 0; row < pol->segment[seg]->n_rows; row++) {
					GMT_geo_to_xy (GMT, pol->segment[seg]->coord[GMT_X][row], pol->segment[seg]->coord[GMT_Y][row], &xx, &yy);
					pol->segment[seg]->coord[GMT_X][row] = xx;
					pol->segment[seg]->coord[GMT_Y][row] = yy;
				}
			}
		}
	}
	
	/* Gather input/output  file names (or stdin/out) and enable i/o */
	
	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_IN,  GMT_ADD_DEFAULT, 0, options) != GMT_OK) {	/* Establishes data input */
		Return (API->error);
	}
	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_OUT, GMT_ADD_DEFAULT, 0, options) != GMT_OK) {	/* Establishes data output */
		Return (API->error);
	}
	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_IN, GMT_HEADER_ON) != GMT_OK) {	/* Enables data input and sets access mode */
		Return (API->error);
	}
	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_OUT, GMT_HEADER_ON) != GMT_OK) {	/* Enables data output and sets access mode */
		Return (API->error);
	}

	/* Now we are ready to take on some input values */

	GMT->common.b.ncol[GMT_OUT] = UINT_MAX;	/* Flag to have it reset to GMT->common.b.ncol[GMT_IN] when writing */
	r_mode = (just_copy_record) ? GMT_READ_MIXED : GMT_READ_DOUBLE;
	GMT_set_segmentheader (GMT, GMT_OUT, false);	/* Since processing of -C|L|F files might have turned it on [should be determined below] */
	
	do {	/* Keep returning records until we reach EOF */
		if ((in = GMT_Get_Record (API, r_mode, &n_fields)) == NULL) {	/* Read next record, get NULL if special case */
			if (GMT_REC_IS_ERROR (GMT)) 		/* Bail if there are any read errors */
				Return (GMT_RUNTIME_ERROR);
			if (GMT_REC_IS_TABLE_HEADER (GMT)) {	/* Echo table headers */
				GMT_Put_Record (API, GMT_WRITE_TABLE_HEADER, NULL);
				continue;
			}
			if (GMT_REC_IS_EOF (GMT)) 		/* Reached end of file */
				break;
			else if (GMT_REC_IS_SEGMENT_HEADER (GMT)) {
				output_header = true;
				need_header = GMT->current.io.multi_segments[GMT_OUT];	/* Only need to break up segments */
				continue;
			}
		}
		
		/* Data record to process */

		n_read++;
		if (n_read%1000 == 0) GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "Read %ld records, passed %ld records\r", n_read, n_pass);

		if (n_fields < n_minimum) {	/* Bad number of columns */
			if (Ctrl->Z.active)
				GMT_report (GMT, GMT_MSG_NORMAL, "-Z requires a data file with at least 3 columns; this file only has %ld near line %ld. Exiting.\n", n_fields, n_read);
			else
				GMT_report (GMT, GMT_MSG_NORMAL, "Data file must have at least 2 columns; this file only has %ld near line %ld. Exiting.\n", n_fields, n_read);
			Return (EXIT_FAILURE);
		}

		if (Ctrl->Z.active) {	/* Apply z-range test */
			if (GMT_is_dnan (in[GMT_Z])) { output_header = need_header; continue;}	/* cannot keep when no z */
			inside = (in[GMT_Z] >= Ctrl->Z.min && in[GMT_Z] <= Ctrl->Z.max); 
			if (inside != Ctrl->I.pass[5]) { output_header = need_header; continue;}
		}

		lon = in[GMT_X];	/* Use copy since we may have to wrap 360 */
		if (GMT->common.R.active) {	/* Apply region test */
			inside = !GMT_map_outside (GMT, lon, in[GMT_Y]);
			if (inside != Ctrl->I.pass[0]) { output_header = need_header; continue;}
		}

		if (do_project)	/* First project the input point */
			GMT_geo_to_xy (GMT, lon, in[GMT_Y], &xx, &yy);
		else {
			xx = lon;
			yy = in[GMT_Y];
		}
		
		if (Ctrl->C.active) {	/* Check for distance to points */
			inside = GMT_near_a_point (GMT, xx, yy, point, Ctrl->C.dist); 
			if (inside != Ctrl->I.pass[1]) { output_header = need_header; continue;}
		}

		if (Ctrl->L.active) {	/* Check for distance to lines */
			inside = GMT_near_lines (GMT, xx, yy, line, Ctrl->L.end_mode, NULL, NULL, NULL);
			if (inside != Ctrl->I.pass[2]) { output_header = need_header; continue;}
		}
		if (Ctrl->F.active) {	/* Check if we are in/out-side polygons */
			if (do_project) {	/* Projected lon/lat; temporary reset input type for GMT_inonout to do Cartesian mode */
				GMT->current.io.col_type[GMT_IN][GMT_X] = GMT->current.io.col_type[GMT_IN][GMT_Y] = GMT_IS_FLOAT;
			}
			inside = 0;
			for (seg = 0; seg < pol->n_segments && !inside; seg++) {	/* Check each polygon until we find that our point is inside */
				if (GMT_polygon_is_hole (pol->segment[seg])) continue;	/* Holes are handled within GMT_inonout */
				inside = (GMT_inonout (GMT, xx, yy, pol->segment[seg]) >= Ctrl->E.inside[F_ITEM]);
			}
			if (do_project) {	/* Reset input type for GMT_inonout to do Cartesian mode */
				GMT->current.io.col_type[GMT_IN][GMT_X] = GMT_IS_LON;
				GMT->current.io.col_type[GMT_IN][GMT_Y] = GMT_IS_LAT;
			}
			if (inside != Ctrl->I.pass[3]) { output_header = need_header; continue;}
		}

		if (Ctrl->N.active) {	/* Check if on land or not */
			int brow, i, this_node;
			xx = lon;
			while (xx < 0.0) xx += 360.0;
			brow = lrint (floor ((90.0 - in[GMT_Y]) / c.bsize));
			if (brow >= c.bin_ny) brow = c.bin_ny - 1;	/* Presumably only kicks in for south pole */
			col = lrint (floor (xx / c.bsize));
			bin = brow * c.bin_nx + col;
			if (bin != last_bin) {	/* Do this upon entering new bin */
				ind = 0;
				while (ind < c.nb && c.bins[ind] != bin) ind++;	/* Set ind to right bin */
				if (ind == c.nb) continue;			/* Bin not among the chosen ones */
				last_bin = bin;
				GMT_free_shore (GMT, &c);	/* Free previously allocated arrays */
				if ((err = GMT_get_shore_bin (GMT, ind, &c))) {
					GMT_report (GMT, GMT_MSG_NORMAL, "%s [%s resolution shoreline]\n", GMT_strerror(err), shore_resolution[base]);
					Return (EXIT_FAILURE);
				}

				/* Must use polygons.  Go in both directions to cover both land and sea */
				for (id = 0; id < 2; id++) {
					GMT_free_shore_polygons (GMT, p[id], np[id]);
					if (np[id]) GMT_free (GMT, p[id]);
					np[id] = GMT_assemble_shore (GMT, &c, wd[id], true, west_border, east_border, &p[id]);
					np[id] = GMT_prep_shore_polygons (GMT, &p[id], np[id], !no_resample, Ctrl->dbg.step, -1);
				}
			}

			if (c.ns == 0) {	/* No lines go through, check node level */
				this_node = MIN (MIN (c.node_level[0], c.node_level[1]) , MIN (c.node_level[2], c.node_level[3]));
			}
			else {
				this_node = 0;
				GMT_geo_to_xy (GMT, lon, in[GMT_Y], &xx, &yy);
				for (id = 0; id < 2; id++) {

					for (k = 0; k < np[id]; k++) {

						if (p[id][k].n == 0) continue;

						/* Find min/max of polygon */

						xmin = xmax = p[id][k].lon[0];
						ymin = ymax = p[id][k].lat[0];

						for (i = 1; i < p[id][k].n; i++) {
							if (p[id][k].lon[i] < xmin) xmin = p[id][k].lon[i];
							if (p[id][k].lon[i] > xmax) xmax = p[id][k].lon[i];
							if (p[id][k].lat[i] < ymin) ymin = p[id][k].lat[i];
							if (p[id][k].lat[i] > ymax) ymax = p[id][k].lat[i];
						}

						if (yy < ymin || yy > ymax) continue;
						if (xx < xmin || xx > xmax) continue;

						/* Must compare with polygon; holes are handled explicitly via the levels */

						if ((side = GMT_non_zero_winding (GMT, xx, yy, p[id][k].lon, p[id][k].lat, p[id][k].n)) < Ctrl->E.inside[N_ITEM]) continue;	/* Outside polygon */

						/* Here, point is inside, we must assign value */

						if (p[id][k].level > this_node) this_node = p[id][k].level;
					}
				}
			}
			inside = Ctrl->N.mask[this_node];
			if (inside != Ctrl->I.pass[4]) { output_header = need_header; continue;}
		}

		/* Here, we have passed all active test and the point is to be output */

		if (output_header) {	/* First output point for this segment - write the header */
			GMT_Put_Record (API, GMT_WRITE_SEGMENT_HEADER, NULL);
			output_header = false;
		}

		if (just_copy_record)
			GMT_Put_Record (API, GMT_WRITE_TEXT, NULL);
		else
			GMT_Put_Record (API, GMT_WRITE_DOUBLE, in);
		n_pass++;
	} while (true);
	
	if (GMT_End_IO (API, GMT_IN,  0) != GMT_OK) {	/* Disables further data input */
		Return (API->error);
	}
	if (GMT_End_IO (API, GMT_OUT, 0) != GMT_OK) {	/* Disables further data output */
		Return (API->error);
	}

	GMT_report (GMT, GMT_MSG_VERBOSE, "Read %" PRIu64 " records, passed %" PRIu64" records\n", n_read, n_pass);

	if (Ctrl->N.active) {
		GMT_free_shore (GMT, &c);
		GMT_shore_cleanup (GMT, &c);
		for (id = 0; id < 2; id++) {
			GMT_free_shore_polygons (GMT, p[id], np[id]);
			if (np[id]) GMT_free (GMT, p[id]);
		}
	}
	
	Return (GMT_OK);
}
