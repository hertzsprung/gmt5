/*-----------------------------------------------------------------
 *	$Id: x2sys_cross_func.c,v 1.10 2011-05-11 09:48:21 guru Exp $
 *
 *      Copyright (c) 1999-2011 by P. Wessel
 *      See LICENSE.TXT file for copying and redistribution conditions.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; version 2 or any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      Contact info: www.soest.hawaii.edu/pwessel
 *--------------------------------------------------------------------*/
/* x2sys_cross will calculate crossovers generated by the
 * intersections of two tracks.  Optionally, it will also evaluate
 * the interpolated datafields at the crossover locations.
 *
 * Author:	Paul Wessel
 * Date:	15-JUN-2004
 * Version:	1.0, based on the spirit of the old xsystem code,
 *		but with a smarter algorithm based on the book
 *		"Algorithms in C" by R. Sedgewick.
 *		31-MAR-2006: Changed -O to -L to avoid clash with GMT.
 *
 */

#include "x2sys.h"

/* Control structure for x2sys_cross */

#define HHI	0
#define VLO	1
#define	VHI	2

struct X2SYS_CROSS_CTRL {
	struct A {	/* -A */
		GMT_LONG active;
		char *file;
	} A;
	struct I {	/* -I */
		GMT_LONG active;
		GMT_LONG mode;
	} I;
	struct S {	/* -S */
		GMT_LONG active[2];
		double limit[3];
	} S;
	struct T {	/* -T */
		GMT_LONG active;
		char *TAG;
	} T;
	struct W {	/* -W */
		GMT_LONG active;
		GMT_LONG width;
	} W;
	struct Q {	/* -Q */
		GMT_LONG active;
		GMT_LONG mode;
	} Q;
	struct Z {	/* -Z */
		GMT_LONG active;
	} Z;
};

struct PAIR {				/* Used with -Kkombinations.lis option */
	char *id1, *id2;
};

void *New_x2sys_cross_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct X2SYS_CROSS_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct X2SYS_CROSS_CTRL);

	/* Initialize values whose defaults are not 0/FALSE/NULL */

	C->S.limit[VHI] = DBL_MAX;	/* Ignore crossovers on segments that implies speed higher than this */
	C->W.width = 3;			/* Number of points on either side in the interpolation */
	return ((void *)C);
}

void Free_x2sys_cross_Ctrl (struct GMT_CTRL *GMT, struct X2SYS_CROSS_CTRL *C) {	/* Deallocate control structure */
	if (C->A.file) free ((void *)C->A.file);
	if (C->T.TAG) free ((void *)C->T.TAG);
	GMT_free (GMT, C);
}

GMT_LONG GMT_x2sys_cross_usage (struct GMTAPI_CTRL *C, GMT_LONG level) {
	struct GMT_CTRL *GMT = C->GMT;
	
	GMT_message (GMT, "%s %s - calculate crossovers\n\n", GMT->init.progname, X2SYS_VERSION);
	GMT_message (GMT, "usage: x2sys_cross <files> -T<TAG> [-A<combi.lis>] [-Il|a|c] [%s]\n", GMT_J_OPT);
	GMT_message (GMT, "\t[-Qe|i] [%s] [-Sl|h|u<speed>] [%s] [-W<size>] [%s] [-Z]\n\n", GMT_Rgeo_OPT, GMT_V_OPT, GMT_bo_OPT);

	GMT_message (GMT, "\tOutput is x y t1 t2 d1 d2 az1 az2 v1 v2 xval1 xmean1 xval2 xmean2 ...\n");
	GMT_message (GMT, "\tIf time is not selected (or present) we use record numbers as proxies i1 i2\n");
	
	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<files> is one or more datafiles, or give =<files.lis> for a file with a list of datafiles\n");
	GMT_message (GMT, "\t-T <TAG> is the system tag for the data set.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t-A Gives list of file pairs that are ok to compare [Default is all combinations]\n");
	GMT_message (GMT, "\t-I sets the interpolation mode.  Choose among:\n");
	GMT_message (GMT, "\t   l Linear interpolation [Default]\n");
	GMT_message (GMT, "\t   a Akima spline interpolation\n");
	GMT_message (GMT, "\t   c Acubic spline interpolation\n");
	GMT_explain_options (GMT, "J");
	GMT_message (GMT, "\t-Q Append e for external crossovers\n");
	GMT_message (GMT, "\t   Append i for internal crossovers [Default is all crossovers]\n");
	GMT_explain_options (GMT, "R");
	GMT_message (GMT, "\t-S Sets limits on lower and upper speeds (units determined by -Ns):\n");
	GMT_message (GMT, "\t   -Sl sets lower speed [Default is 0]\n");
	GMT_message (GMT, "\t   -Sh no headings should be computed if velocity drops below this value [0]\n");
	GMT_message (GMT, "\t   -Su sets upper speed [Default is Infinity]\n");
	GMT_explain_options (GMT, "V");
	GMT_message (GMT, "\t-W sets maximum points on either side of xover to use in interpolation [Default is 3]\n");
	GMT_message (GMT, "\t-Z Return z-values for each track [Default is crossover and mean value]\n");
	GMT_explain_options (GMT, "D");
	
	return (EXIT_FAILURE);
}

GMT_LONG GMT_x2sys_cross_parse (struct GMTAPI_CTRL *C, struct X2SYS_CROSS_CTRL *Ctrl, struct GMT_OPTION *options) {

	/* This parses the options provided to grdcut and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	GMT_LONG n_errors = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {
			/* Common parameters */

			case '<':	/* Skip input files */
				break;

			/* Processes program-specific parameters */
			
			case 'A':	/* Get list of approved filepair combinations to check */
				Ctrl->A.active = TRUE;
				Ctrl->A.file = strdup (opt->arg);
				break;
			case 'I':
				Ctrl->I.active = TRUE;
				switch (opt->arg[0]) {
					case 'l':
						Ctrl->I.mode = 0;
						break;
					case 'a':
						Ctrl->I.mode = 1;
						break;
					case 'c':
						Ctrl->I.mode = 2;
						break;
					case 'n':
						Ctrl->I.mode = 3;
						break;
					default:
						n_errors++;
						break;
				}
				break;
			case 'S':	/* Speed checks */
				switch (opt->arg[0]) {
					case 'L':
					case 'l':	/* Lower cutoff speed */
						Ctrl->S.limit[VLO] = atof (&opt->arg[1]);
						Ctrl->S.active[VLO] = TRUE;
						break;
					case 'U':
					case 'u':	/* Upper cutoff speed */
						Ctrl->S.limit[VHI] = atof (&opt->arg[1]);
						Ctrl->S.active[VLO] = TRUE;
						break;
					case 'H':
					case 'h':	/* Heading calculation cutoff speed */
						Ctrl->S.limit[HHI] = atof (&opt->arg[1]);
						Ctrl->S.active[HHI] = TRUE;
						break;
					default:
						GMT_report (GMT, GMT_MSG_FATAL, "Syntax error: -S<l|h|u><speed>\n");
						n_errors++;
						break;
				}
				break;
			case 'T':
				Ctrl->T.active = TRUE;
				Ctrl->T.TAG = strdup (opt->arg);
				break;
			case 'W':	/* Get new window half-width as number of points */
				Ctrl->W.active = TRUE;
				Ctrl->W.width = atoi (opt->arg);
				break;
			case 'Q':	/* Specify internal or external only */
				Ctrl->Q.active = TRUE;
				if (opt->arg[0] == 'e') Ctrl->Q.mode = 1;
				else if (opt->arg[0] == 'i') Ctrl->Q.mode = 2;
				else Ctrl->Q.mode = 3;
				break;
			case 'Z':	/* Return z1, z1 rather than (z1-z1) and 0.5 * (z1 + z2) */
				Ctrl->Z.active = TRUE;
				break;
			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, !Ctrl->T.active || !Ctrl->T.TAG, "Syntax error: -T must be used to set the TAG\n");
	n_errors += GMT_check_condition (GMT, Ctrl->W.width < 1, "Syntax error: Error -W: window must be at least 1\n");
	n_errors += GMT_check_condition (GMT, Ctrl->S.limit[VLO] > Ctrl->S.limit[VHI], "Syntax error: Error -S: lower speed cutoff higher than upper cutoff!\n");
	n_errors += GMT_check_condition (GMT, Ctrl->Q.mode == 3, "Syntax error: Error -Q: Only one of -Qe -Qi can be specified!\n");


	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

GMT_LONG combo_ok (char *name_1, char *name_2, struct PAIR *pair, int n_pairs)
{
	int i;

	/* Return TRUE if this particular combination is found in the list of pairs */

	for (i = 0; i < n_pairs; i++) {
		if (!(strcmp (name_1, pair[i].id1) || strcmp (name_2, pair[i].id2))) return (TRUE);
		if (!(strcmp (name_2, pair[i].id1) || strcmp (name_1, pair[i].id2))) return (TRUE);
	}
	return (FALSE);
}

#define Return(code) {Free_x2sys_cross_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); return (code);}

GMT_LONG GMT_x2sys_cross (struct GMTAPI_CTRL *API, struct GMT_OPTION *options)
{
	char **trk_name = NULL;			/* Name of tracks */
	char line[GMT_BUFSIZ];			/* buffer */
	char item[GMT_BUFSIZ];			/* buffer */
	char t_or_i;				/* t = time, i = dummy node time */
	char name1[80], name2[80];		/* Name of two files to be examined */
	char *x2sys_header = "> %s %ld %s %ld %s\n";

	GMT_LONG n_rec[2];			/* Number of data records for both files */
	GMT_LONG window_width;			/* Max number of points to use in the interpolation */
	GMT_LONG n_tracks = 0;			/* Total number of data sets to compare */
	GMT_LONG nx;				/* Number of crossovers found for this pair */
	GMT_LONG *col_number = NULL;		/* Array with the column numbers of the data fields */
	GMT_LONG n_output;			/* Number of columns on output */
	GMT_LONG n_pairs = 0;			/* Number of acceptable combinations */
	GMT_LONG A, B, i, j, col, k, start, n_bad;	/* Misc. counters and local variables */
	GMT_LONG end, first, n_ok, n_alloc = 1;
	GMT_LONG n_data_col, left[2], t_left;
	GMT_LONG n_left, right[2], t_right, n_right;
	GMT_LONG n_duplicates, n_errors;
	GMT_LONG add_chunk;

	GMT_LONG xover_locations_only = FALSE;	/* TRUE if only x,y (and possible indices) to be output */
	GMT_LONG internal = TRUE;		/* FALSE if only external xovers are needed */
	GMT_LONG external = TRUE;		/* FALSE if only internal xovers are needed */
	GMT_LONG error = FALSE;			/* TRUE for invalid arguments */
	GMT_LONG do_project = FALSE;		/* TRUE if we must mapproject first */
	GMT_LONG got_time = FALSE;		/* TRUE if there is a time column */
	GMT_LONG first_header = TRUE;		/* TRUE for very first crossover */
	GMT_LONG first_crossover;		/* TRUE for first crossover between two data sets */
	GMT_LONG same = FALSE;			/* TRUE when the two cruises we compare have the same name */
	GMT_LONG has_time[2];			/* TRUE for each cruises that actually has a time column */
	GMT_LONG *duplicate = NULL;		/* Array, TRUE for any cruise that is already listed */
	GMT_LONG *ok = NULL;
	GMT_LONG cmdline_files = FALSE;		/* TRUE if files where given directly on the command line */

	double dt;				/* Time between crossover and previous node */
	double dist_x[2];			/* Distance(s) along track at the crossover point */
	double time_x[2];			/* Time(s) along track at the crossover point */
	double deld, delt;			/* Differences in dist and time across xpoint */
	double speed[2];			/* speed across the xpoint ( = deld/delt) */
	double **data[2] = {NULL, NULL};	/* Data matrices for the two data sets to be checked */
	double *xdata[2] = {NULL, NULL};	/* Data vectors with estimated values at crossover points */
	double *dist[2] = {NULL, NULL};		/* Data vectors with along-track distances */
	double *time[2] = {NULL, NULL};		/* Data vectors with along-track times (or dummy node indices) */
	double *t = NULL, *y = NULL;		/* Interpolation y(t) arrays */
	double *out = NULL;			/* Output record array */
	double X2SYS_NaN;			/* Value to write out when result is NaN */
	double xx, yy;				/* Temporary projection variables */
	double dist_scale;			/* Scale to give selected distance units */
	double vel_scale;			/* Scale to give selected velocity units */


	struct X2SYS_INFO *s = NULL;			/* Data format information  */
	struct GMT_XSEGMENT *ylist_A = NULL, *ylist_B = NULL;		/* y-indices sorted in increasing order */
	struct GMT_XOVER XC;				/* Structure with resulting crossovers */
	struct X2SYS_FILE_INFO data_set[2];		/* File information */
	struct X2SYS_BIX Bix;
	struct PAIR *pair = NULL;		/* Used with -Akombinations.lis option */
	struct GMT_OPTION *opt = NULL;
	FILE *fp = NULL;
	struct X2SYS_CROSS_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;

/*----------------------------------END OF VARIBLE DECLARATIONS-----------------------------------------------*/

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));

	if (!options || options->option == GMTAPI_OPT_USAGE) return (GMT_x2sys_cross_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) return (GMT_x2sys_cross_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_module (API, "GMT_x2sys_cross", &GMT_cpy);	/* Save current state */
	if ((error = GMT_Parse_Common (API, "-VfRJb", ">", options))) Return (error);
	Ctrl = (struct X2SYS_CROSS_CTRL *)New_x2sys_cross_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_x2sys_cross_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the x2sys_cross main code ----------------------------*/

	x2sys_err_fail (GMT, x2sys_set_system (GMT, Ctrl->T.TAG, &s, &Bix, &GMT->current.io), Ctrl->T.TAG);
	if (!s->geographic) GMT->current.io.col_type[GMT_IN][GMT_X] = GMT->current.io.col_type[GMT_IN][GMT_Y] = GMT->current.io.col_type[GMT_OUT][GMT_X] = GMT->current.io.col_type[GMT_OUT][GMT_Y] = GMT_IS_UNKNOWN;

	if (s->x_col == -1 || s->y_col == -1) {
		GMT_report (GMT, GMT_MSG_FATAL, "Error: lon,lat or x,y are not among data columns!\n");
		Return (EXIT_FAILURE);
	}
	
	if ((n_tracks = x2sys_get_tracknames (GMT, options, &trk_name, &cmdline_files)) == 0) {
		GMT_report (GMT, GMT_MSG_FATAL, "Error: Must give at least one data set!\n");
		Return (EXIT_FAILURE);		
	}
	
	GMT->current.setting.interpolant = Ctrl->I.mode;
	if (Ctrl->Q.active) {
		if (Ctrl->Q.mode == 1) internal = FALSE;
		if (Ctrl->Q.mode == 2) external = FALSE;
	}

	GMT_report (GMT, GMT_MSG_NORMAL, "Files found: %ld\n", n_tracks);

	duplicate = GMT_memory (GMT, NULL, n_tracks, GMT_LONG);

	GMT_report (GMT, GMT_MSG_NORMAL, "Checking for duplicates : ");
	/* Make sure there are no duplicates */
	for (A = n_duplicates = 0; A < n_tracks; A++) {	/* Loop over all files */
		if (duplicate[A]) continue;
		for (B = A + 1; B < n_tracks; B++) {
			if (duplicate[B]) continue;
			same = !strcmp (trk_name[A], trk_name[B]);
			if (same) {
				GMT_report (GMT, GMT_MSG_FATAL, "File %s repeated on command line - skipped\n", trk_name[A]);
				duplicate[B] = TRUE;
				n_duplicates++;
			}
		}
	}
	GMT_report (GMT, GMT_MSG_NORMAL, "%ld found\n", n_duplicates);
	
	if (Ctrl->A.active) {	/* Read list of acceptable trk_name combinations */

		GMT_report (GMT, GMT_MSG_NORMAL, "Explicit combinations found: ");
		if ((fp = fopen (Ctrl->A.file, "r")) == NULL) {
			GMT_report (GMT, GMT_MSG_FATAL, "Error: Could not open combinations file %s!\n", Ctrl->A.file);
			Return (EXIT_FAILURE);
		}

		n_alloc = add_chunk = GMT_CHUNK;
		pair = GMT_memory (GMT, NULL, n_alloc, struct PAIR);

		while (fgets (line, GMT_BUFSIZ, fp)) {

			if (line[0] == '#' || line[0] == '\n') continue;	/* Skip comments and blanks */
			GMT_chop (line);	/* Get rid of CR, LF stuff */

			if (sscanf (line, "%s %s", name1, name2) != 2) {
				GMT_report (GMT, GMT_MSG_FATAL, "Error: Error decoding combinations file for pair %ld!\n", n_pairs);
				Return (EXIT_FAILURE);
			}
			pair[n_pairs].id1 = strdup (name1);
			pair[n_pairs].id2 = strdup (name2);
			n_pairs++;
			if (n_pairs == n_alloc) {
				add_chunk *= 2;
				n_alloc += add_chunk;
				pair = GMT_memory (GMT, pair, n_alloc, struct PAIR);
			}
		}
		fclose (fp);

		if (!n_pairs) {
			GMT_report (GMT, GMT_MSG_FATAL, "Error: No combinations found in file %s!\n", Ctrl->A.file);
			Return (EXIT_FAILURE);
		}
		pair = GMT_memory (GMT, pair, n_pairs, struct PAIR);
		GMT_report (GMT, GMT_MSG_NORMAL, "%ld\n", n_pairs);
	}

	X2SYS_NaN = GMT->session.d_NaN;

	if (GMT->current.setting.interpolant == 0) Ctrl->W.width = 1;
	window_width = 2 * Ctrl->W.width;
	n_data_col = x2sys_n_data_cols (GMT, s);
	got_time = (s->t_col >= 0);
	if (!got_time) Ctrl->S.active[VLO] = FALSE;	/* Cannot check speed if there is no time */

	n_output = 10 + 2 * n_data_col;
	GMT->current.io.col_type[GMT_OUT][GMT_X] = (!strcmp (s->info[s->x_col].name, "lon")) ? GMT_IS_LON : GMT_IS_FLOAT;
	GMT->current.io.col_type[GMT_OUT][GMT_Y] = (!strcmp (s->info[s->x_col].name, "lat")) ? GMT_IS_LAT : GMT_IS_FLOAT;
	GMT->current.io.col_type[GMT_OUT][GMT_Z] = GMT->current.io.col_type[GMT_OUT][3] = (got_time) ? GMT_IS_ABSTIME : GMT_IS_FLOAT;
	for (i = 0; i < n_data_col+2; i++) GMT->current.io.col_type[GMT_OUT][4+2*i] = GMT->current.io.col_type[GMT_OUT][5+2*i] = GMT_IS_FLOAT;

	if (n_data_col == 0) {
		xover_locations_only = TRUE;
		n_output = 2;
	}
	else {	/* Set the actual column numbers with data fields */
		t = GMT_memory (GMT, NULL, window_width, double);
		y = GMT_memory (GMT, NULL, window_width, double);
		col_number = GMT_memory (GMT, NULL, n_data_col, GMT_LONG);
		ok = GMT_memory (GMT, NULL, n_data_col, GMT_LONG);
		for (col = k = 0; col < s->n_out_columns; col++) {
			if (col == s->x_col || col == s->y_col || col == s->t_col) continue;
			col_number[k++] = col;
		}
		if (s->t_col < 0) GMT_report (GMT, GMT_MSG_NORMAL, "No time column, use dummy times\n");
	}

	out = GMT_memory (GMT, NULL, n_output, double);
	xdata[0] = GMT_memory (GMT, NULL, s->n_out_columns, double);
	xdata[1] = GMT_memory (GMT, NULL, s->n_out_columns, double);

	if (GMT->common.R.active && GMT->current.proj.projection != GMT_NO_PROJ) {
		do_project = TRUE;
		s->geographic = FALSE;	/* Since we then have x,y projected coordinates, not lon,lat */
		s->dist_flag = 0;
		GMT_err_fail (GMT, GMT_map_setup (GMT, GMT->common.R.wesn), "");
	}

	GMT_init_distaz (GMT, s->dist_flag ? GMT_MAP_DIST_UNIT : 'X', s->dist_flag, GMT_MAP_DIST);
		
	MGD77_Set_Unit (GMT, s->unit[X2SYS_DIST_SELECTION], &dist_scale, -1);	/* Gets scale which multiplies meters to chosen distance unit */
	MGD77_Set_Unit (GMT, s->unit[X2SYS_SPEED_SELECTION], &vel_scale, -1);	/* Sets output scale for distances using in velocities */
	switch (s->unit[X2SYS_SPEED_SELECTION][0]) {
		case 'c':
			vel_scale = 1.0;
			break;
		case 'e':
			vel_scale /= dist_scale;			/* Must counteract any distance scaling to get meters. dt is in sec so we get m/s */
			break;
		case 'f':
			vel_scale /= (METERS_IN_A_FOOT * dist_scale);		/* Must counteract any distance scaling to get feet. dt is in sec so we get ft/s */
			break;
		case 'k':
			vel_scale *= (3600.0 / dist_scale);		/* Must counteract any distance scaling to get km. dt is in sec so 3600 gives km/hr */
			break;
		case 'm':
			vel_scale *= (3600.0 / dist_scale);		/* Must counteract any distance scaling to get miles. dt is in sec so 3600 gives miles/hr */
			break;
		case 'n':
			vel_scale *= (3600.0 / dist_scale);		/* Must counteract any distance scaling to get miles. dt is in sec so 3600 gives miles/hr */
			break;
		default:	/*Cartesian */
			break;
	}

	if ((error = GMT_set_cols (GMT, GMT_OUT, n_output))) Return (error);
	if ((error = GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_OUT, GMT_REG_DEFAULT, options))) Return (error);	/* Registers default output destination, unless already set */
	if ((error = GMT_Begin_IO (API, GMT_IS_DATASET, GMT_OUT, GMT_BY_REC))) Return (error);				/* Enables data output and sets access mode */

	for (A = 0; A < n_tracks; A++) {	/* Loop over all files */
		if (duplicate[A]) continue;

		if (s->x_col < 0 || s->x_col < 0) {
			GMT_report (GMT, GMT_MSG_FATAL, "Error: x and/or y column not found for track %s!\n", trk_name[A]);
			Return (EXIT_FAILURE);
		}

		x2sys_err_fail (GMT, (s->read_file) (trk_name[A], &data[0], s, &data_set[0], &GMT->current.io, &n_rec[0]), trk_name[A]);

		has_time[0] = FALSE;
		if (got_time) {	/* Check to make sure we do in fact have time */
			for (i = n_bad = 0; i < n_rec[0]; i++) n_bad += GMT_is_dnan (data[0][s->t_col][i]);
			if (n_bad < n_rec[0]) has_time[0] = TRUE;
		}

		if (do_project) {	/* Convert all the coordinates */
			for (i = 0; i < n_rec[0]; i++) {
				GMT_geo_to_xy (GMT, data[0][s->x_col][i], data[0][s->y_col][i], &xx, &yy);
				data[0][s->x_col][i] = xx;
				data[0][s->y_col][i] = yy;
			}
		}

		GMT_err_fail (GMT, GMT_dist_array (GMT, data[0][s->x_col], data[0][s->y_col], n_rec[0], dist_scale, s->dist_flag, &dist[0]), "");

		time[0] = (has_time[0]) ? data[0][s->t_col] : x2sys_dummytimes (GMT, n_rec[0]) ;

		GMT_init_track (GMT, data[0][s->y_col], n_rec[0], &ylist_A);

		for (B = A; B < n_tracks; B++) {
			if (duplicate[B]) continue;

			same = !strcmp (trk_name[A], trk_name[B]);
			if (same && !(A == B)) {
				GMT_report (GMT, GMT_MSG_FATAL, "File %s repeated on command line - skipped\n", trk_name[A]);
				continue;
			}
			if (!internal &&  same) continue;	/* Only do external errors */
			if (!external && !same) continue;	/* Only do internal errors */

			if (Ctrl->A.active && !combo_ok (trk_name[A], trk_name[B], pair, (int)n_pairs)) continue;	/* Do not want this combo */
			
			GMT_report (GMT, GMT_MSG_NORMAL, "Processing %s - %s : ", trk_name[A], trk_name[B]);

			if (same) {	/* Just set pointers */
				data[1] = data[0];
				dist[1] = dist[0];
				time[1] = time[0];
				has_time[1] = has_time[0];
				n_rec[1] = n_rec[0];
				ylist_B = ylist_A;
				data_set[1] = data_set[0];
			}
			else {	/* Must read a second file */

				x2sys_err_fail (GMT, (s->read_file) (trk_name[B], &data[1], s, &data_set[1], &GMT->current.io, &n_rec[1]), trk_name[B]);

				has_time[1] = FALSE;
				if (got_time) {	/* Check to make sure we do in fact have time */
					for (i = n_bad = 0; i < n_rec[1]; i++) n_bad += GMT_is_dnan (data[1][s->t_col][i]);
					if (n_bad < n_rec[1]) has_time[1] = TRUE;
				}
				
				if (do_project) {	/* Convert all the coordinates */
					for (i = 0; i < n_rec[0]; i++) {
						GMT_geo_to_xy (GMT, data[1][s->x_col][i], data[1][s->y_col][i], &xx, &yy);
						data[1][s->x_col][i] = xx;
						data[1][s->y_col][i] = yy;
					}
				}

				GMT_err_fail (GMT, GMT_dist_array (GMT, data[1][s->x_col], data[1][s->y_col], n_rec[1], dist_scale, s->dist_flag, &dist[1]), "");

				time[1] = (has_time[1]) ? data[1][s->t_col] : x2sys_dummytimes (GMT, n_rec[1]);

				GMT_init_track (GMT, data[1][s->y_col], n_rec[1], &ylist_B);
			}

			/* Calculate all possible crossover locations */

			nx = GMT_crossover (GMT, data[0][s->x_col], data[0][s->y_col], data_set[0].ms_rec, ylist_A, n_rec[0], data[1][s->x_col], data[1][s->y_col], data_set[1].ms_rec, ylist_B, n_rec[1], (A == B), &XC);

			if (nx && xover_locations_only) {	/* Report crossover locations only */
				sprintf (line, "%s - %s", trk_name[A], trk_name[B]);
				GMT_Put_Record (API, GMT_WRITE_SEGHEADER, (void *)line);
				for (i = 0; i < nx; i++) {
					out[0] = XC.x[i];
					out[1] = XC.y[i];
					if (s->geographic) GMT_lon_range_adjust (GMT, s->geodetic, &out[0]);
					GMT_Put_Record (API, GMT_WRITE_DOUBLE, (void *)out);	/* Write this to output */
				}
				GMT_x_free (GMT, &XC);
			}
			else if (nx) {	/* Got crossovers, now estimate crossover values */

				first_crossover = TRUE;

				for (i = 0; i < nx; i++) {	/* For each potential crossover */

					GMT_memset (ok, n_data_col, GMT_LONG);
					n_ok = 0;

					for (k = 0; k < 2; k++) {	/* For each of the two data sets involved */

						/* Get node number to each side of crossover location */

				/*	--o----------o--------o------X-------o-------o----------o-- ----> time
							      ^      ^       ^
							    left   xover   right			*/

						left[k]  = (GMT_LONG) floor (XC.xnode[k][i]);
						right[k] = (GMT_LONG) ceil  (XC.xnode[k][i]);
						
						if (left[k] == right[k]) {	/* Crosses exactly on a node; move left or right so interpolation will work */
							if (left[k] > 0)
								left[k]--;	/* Move back so cross occurs at right[k] */
							else
								right[k]++;	/* Move forward so cross occurs at left[k] */
						}

						deld = dist[k][right[k]] - dist[k][left[k]];
						delt = time[k][right[k]] - time[k][left[k]];

						/* Check if speed is outside accepted domain */

						speed[k] = (delt == 0.0) ? GMT->session.d_NaN : vel_scale * (deld / delt);
						if (Ctrl->S.active[VLO] && !GMT_is_dnan (speed[k]) && (speed[k] < Ctrl->S.limit[VLO] || speed[k] > Ctrl->S.limit[VHI])) continue;

						/* Linearly estimate the crossover times and distances */

						dt = XC.xnode[k][i] - left[k];
						time_x[k] = time[k][left[k]];
						dist_x[k] = dist[k][left[k]];
						if (dt > 0.0) {
							time_x[k] += dt * delt;
							dist_x[k] += dt * deld;
						}


						for (j = 0; j < n_data_col; j++) {	/* Evaluate each field at the crossover */

							col = col_number[j];

							start = t_right = left[k];
							end = t_left = right[k];
							n_left = n_right = 0;

							xdata[k][col] = GMT->session.d_NaN;	/* In case of nuthin' */

							/* First find the required <window> points to the left of the xover */

							while (start >= 0 && n_left < Ctrl->W.width) {
								if (!GMT_is_dnan (data[k][col][start])) {
									n_left++;
									if (t_left > left[k]) t_left = start;
									y[Ctrl->W.width-n_left] = data[k][col][start];
									t[Ctrl->W.width-n_left] = time[k][start];
								}
								start--;
							}

							if (!n_left) continue;
							if (got_time && ((time_x[k] - time[k][t_left]) > Bix.time_gap)) continue;
							if (!got_time && ((dist_x[k] - dist[k][t_left]) > Bix.dist_gap)) continue;

							/* Ok, that worked.  Now for the right side: */

							while (end < n_rec[k] && n_right < Ctrl->W.width) {
								if (!GMT_is_dnan (data[k][col][end])) {
									y[Ctrl->W.width+n_right] = data[k][col][end];
									t[Ctrl->W.width+n_right] = time[k][end];
									n_right++;
									if (t_right < right[k]) t_right = end;
								}
								end++;
							}

							if (!n_right) continue;
							if (got_time && ((time[k][t_right] - time_x[k]) > Bix.time_gap)) continue;
							if (!got_time && ((dist[k][t_right] - dist_x[k]) > Bix.dist_gap)) continue;

							/* Ok, got enough data to interpolate at xover */

							first = Ctrl->W.width - n_left;
							n_errors = GMT_intpol (GMT, &t[first], &y[first], (n_left + n_right), (GMT_LONG)1, &time_x[k], &xdata[k][col], GMT->current.setting.interpolant);
							if (n_errors == 0) {	/* OK */
								ok[j]++;
								n_ok++;
							}
						}
					}

					/* Only output crossover if there are any data there */

					if (n_ok == 0) continue;
					for (j = n_ok = 0; j < n_data_col; j++) if (ok[j] == 2) n_ok++;
					if (n_ok == 0) continue;

					/* OK, got something to report */

					/* Load the out array */

					out[0] = XC.x[i];	/* Crossover location */
					out[1] = XC.y[i];

					for (k = 0; k < 2; k++) {	/* Get times, distances, headings, and velocities */

						/* Get time */

						out[2+k] = (got_time && !has_time[k]) ? X2SYS_NaN : time_x[k];

						/* Get cumulative distance at crossover */

						out[k+4] = dist_x[k];

						/* Estimate heading there */

						j = k + 6;
						out[j] = (!GMT_is_dnan (speed[k]) && (!Ctrl->S.active[HHI] || speed[k] > Ctrl->S.limit[HHI])) ? (*GMT->current.map.azimuth_func) (data[k][s->x_col][right[k]], data[k][s->y_col][right[k]], data[k][s->x_col][left[k]], data[k][s->y_col][left[k]], FALSE) : X2SYS_NaN;

						/* Estimate velocities there */

						j = k + 8;
						out[j] = (has_time[k]) ? speed[k] : X2SYS_NaN;
					}

					/* Calculate crossover and mean value */

					for (k = 0, j = 10; k < n_data_col; k++) {
						if (Ctrl->Z.active) {
							col = col_number[k];
							out[j++] = xdata[0][col];
							out[j++] = xdata[1][col];
						}
						else {
							if (ok[k] == 2) {
								col = col_number[k];
								out[j++] = xdata[0][col] - xdata[1][col];
								out[j++] = 0.5 * (xdata[0][col] + xdata[1][col]);
							}
							else {
								out[j] = out[j+1] = X2SYS_NaN;
								j += 2;
							}
						}
					}

					if (first_header) {	/* Write the header record */
						t_or_i = (got_time) ? 't' : 'i';
						sprintf (line, "# Tag: %s", Ctrl->T.TAG);
						GMT_Put_Record (API, GMT_WRITE_TBLHEADER, (void *)line);
						sprintf (line, "# Command: %s", GMT->init.progname);
						if (cmdline_files) strcat (line, " [tracks]");
						for (opt = options; opt; opt = opt->next) if (opt->option == GMTAPI_OPT_INFILE) {strcat (line, " "); strcat (line, opt->arg);}
						GMT_Put_Record (API, GMT_WRITE_TBLHEADER, (void *)line);
						sprintf (line, "# %s\t%s\t%c_1\t%c_2\tdist_1\tdist_2\thead_1\thead_2\tvel_1\tvel_2",
							s->info[s->out_order[s->x_col]].name, s->info[s->out_order[s->y_col]].name, t_or_i, t_or_i);
						for (j = 0; j < n_data_col; j++) {
							col = col_number[j];
							if (Ctrl->Z.active)
								sprintf (item, "\t%s_1\t%s_2", s->info[s->out_order[col]].name, s->info[s->out_order[col]].name);
							else
								sprintf (item, "\t%s_X\t%s_M", s->info[s->out_order[col]].name, s->info[s->out_order[col]].name);
							strcat (line, item);
						}
						GMT_Put_Record (API, GMT_WRITE_TBLHEADER, (void *)line);
						first_header = FALSE;
					}

					if (first_crossover) {
						char info[GMT_BUFSIZ], start[2][GMT_TEXT_LEN64], stop[2][GMT_TEXT_LEN64];
						GMT_memset (info, GMT_BUFSIZ, char);
						for (k = 0; k < 2; k++) {
							if (has_time[k]) {	/* Find first and last record times */
								for (j = 0; j < n_rec[k] && GMT_is_dnan (time[k][j]); j++);	/* Find first non-NaN time */
								GMT_ascii_format_col (GMT, start[k], time[k][j], 2);
								for (j = n_rec[k]-1; j > 0 && GMT_is_dnan (time[k][j]); j--);	/* Find last non-NaN time */
								GMT_ascii_format_col (GMT, stop[k], time[k][j], 3);
							}
							else {
								(void)strcpy (start[k], "NaN");
								(void)strcpy (stop[k], "NaN");
							}
						}
						sprintf (info, "%s/%s/%g %s/%s/%g", start[0], stop[0], dist[0][n_rec[0]-1], start[1], stop[1], dist[1][n_rec[1]-1]);
						sprintf (line, x2sys_header, trk_name[A], data_set[0].year, trk_name[B], data_set[1].year, info);
						GMT_Put_Record (API, GMT_WRITE_SEGHEADER, (void *)line);
						first_crossover = FALSE;
					}

					if (s->geographic) GMT_lon_range_adjust (GMT, s->geodetic, &out[0]);
					GMT_Put_Record (API, GMT_WRITE_DOUBLE, (void *)out);	/* Write this to output */
				}

				GMT_x_free (GMT, &XC);
			}

			if (!same) {	/* Must free up memory for B */
				x2sys_free_data (GMT, data[1], s->n_out_columns, &data_set[1]);
				GMT_free (GMT, dist[1]);
				if (!got_time) GMT_free (GMT, time[1]);
				GMT_free (GMT, ylist_B);
			}
			GMT_report (GMT, GMT_MSG_NORMAL, "%ld\n", nx);
		}

		/* Must free up memory for A */

		x2sys_free_data (GMT, data[0], s->n_out_columns, &data_set[0]);
		GMT_free (GMT, dist[0]);
		if (!got_time) GMT_free (GMT, time[0]);
		GMT_free (GMT, ylist_A);
	}
	if ((error = GMT_End_IO (API, GMT_OUT, 0))) Return (error);	/* Disables further data output */

	/* Free up other arrays */

	GMT_free (GMT, xdata[0]);
	GMT_free (GMT, xdata[1]);
	GMT_free (GMT, out);
	GMT_free (GMT, duplicate);
	if (n_data_col) {
		GMT_free (GMT, t);
		GMT_free (GMT, y);
		GMT_free (GMT, col_number);
		GMT_free (GMT, ok);
	}
	x2sys_free_list (GMT, trk_name, n_tracks);

	x2sys_end (GMT, s);

	Return (GMT_OK);
}
