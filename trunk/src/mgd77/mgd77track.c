/*--------------------------------------------------------------------
 *	$Id$
 *
 *    Copyright (c) 2004-2012 by P. Wessel
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/*
 * mgd77track will read *.mgd77-files and and write PostScript code
 * that will create a navigation plot on a basemap using the
 * projection specified by the user. WESN must be specified on the 
 * command line along with other options for scaling, annotation etc.
*
 * To select a sub-section of the track, specify the start/endpoints by:
 *	1) Start-time (yyyy-mm-ddT[hh:mm:ss]) OR start-distance (km)
 *	2) Stop-time  (yyyy-mm-ddT[hh:mm:ss]) OR stop-distance (km)
 *
 * Author:	Paul Wessel
 * Date:	19-AUG-2004
 * Version:	1.0 Based alot on the old gmttrack.c
 *
 *
 */
 
#define THIS_MODULE k_mod_mgd77track /* I am mgd77track */

#include "gmt_dev.h"
#include "mgd77.h"

#define GMT_PROG_OPTIONS "->BJKOPRUVXYcptxy"

#define MGD77TRACK_ANSIZE 0.125
#define MGD77TRACK_MARK_NEWDAY	0
#define MGD77TRACK_MARK_SAMEDAY	1
#define MGD77TRACK_MARK_DIST	2

#define ANNOT	0
#define LABEL	1

#define GAP_D	0	/* Indices into Ctrl.G */
#define GAP_T	1

struct MGD77TRACK_ANNOT {
	double annot_int_dist;
	double tick_int_dist;
	double annot_int_time;
	double tick_int_time;
};

struct MGD77TRACK_LEG_ANNOT {	/* Structure used to annotate legs after clipping is terminated */
	double x, y;
	double lon, lat;
	double angle;
	char text[16];
};

struct MGD77TRACK_MARKER {
	double marker_size, font_size;
	struct GMT_FILL f, s;	/* Font and symbol colors */
	struct GMT_FONT font;
};

struct MGD77TRACK_CTRL {	/* All control options for this program (except common args) */
	/* active is true if the option has been activated */
	struct A {	/* -A */
		bool active;
		int mode;	/* May be negative */
		double size;
		struct MGD77TRACK_ANNOT info;
	} A;
	struct C {	/* -C */
		bool active;
		unsigned int mode;
	} C;
	struct D {	/* -D */
		bool active;
		bool mode;	/* true to skip recs with time == NaN */
		double start;	/* Start time */
		double stop;	/* Stop time */
	} D;
	struct F {	/* -F */
		bool active;
		int mode;
	} F;
	struct G {	/* -G */
		bool active[2];
		unsigned int value[2];
	} G;
	struct I {	/* -I */
		bool active;
		unsigned int n;
		char code[3];
	} I;
	struct L {	/* -L */
		bool active;
		struct MGD77TRACK_ANNOT info;
	} L;
	struct N {	/* -N */
		bool active;
	} N;
	struct S {	/* -S */
		bool active;
		double start;	/* Start dist */
		double stop;	/* Stop dist */
	} S;
	struct T {	/* -T */
		bool active;
		struct MGD77TRACK_MARKER marker[3];
	} T;
	struct W {	/* -W<pen> */
		bool active;
		struct GMT_PEN pen;
	} W;
};

void *New_mgd77track_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct MGD77TRACK_CTRL *C = NULL;
	
	C = GMT_memory (GMT, NULL, 1, struct MGD77TRACK_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
	
	C->A.size = MGD77TRACK_ANSIZE;
	C->D.stop = C->S.stop = DBL_MAX;
	C->W.pen = GMT->current.setting.map_default_pen;
	GMT_init_fill (GMT, &C->T.marker[MGD77TRACK_MARK_SAMEDAY].s, 1.0, 1.0, 1.0);	/* White color for other time markers in same day */
	if (GMT->current.setting.proj_length_unit == GMT_CM) {
		C->T.marker[MGD77TRACK_MARK_NEWDAY].marker_size = C->T.marker[MGD77TRACK_MARK_SAMEDAY].marker_size = 0.1 / 2.54;	/* 1 mm */
		C->T.marker[MGD77TRACK_MARK_DIST].marker_size = 0.15 / 2.54;	/* 1.5 mm */
	}
	else {	/* Assume we think in inches */
		C->T.marker[MGD77TRACK_MARK_NEWDAY].marker_size = C->T.marker[MGD77TRACK_MARK_SAMEDAY].marker_size = 0.04;
		C->T.marker[MGD77TRACK_MARK_DIST].marker_size = 0.06;
	}
	C->T.marker[MGD77TRACK_MARK_NEWDAY].font = C->T.marker[MGD77TRACK_MARK_SAMEDAY].font = C->T.marker[MGD77TRACK_MARK_DIST].font = GMT->current.setting.font_annot[0];
	GMT_getfont (GMT, "Times-BoldItalic", &C->T.marker[MGD77TRACK_MARK_NEWDAY].font);
	GMT_getfont (GMT, "Times-Italic", &C->T.marker[MGD77TRACK_MARK_SAMEDAY].font);
	GMT_getfont (GMT, "Times-Roman", &C->T.marker[MGD77TRACK_MARK_DIST].font);

	return (C);
}

void Free_mgd77track_Ctrl (struct GMT_CTRL *GMT, struct MGD77TRACK_CTRL *C) {	/* Deallocate control structure */
	GMT_free (GMT, C);	
}

int GMT_mgd77track_usage (struct GMTAPI_CTRL *C, int level, struct MGD77TRACK_CTRL *Ctrl)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: mgd77track cruise(s) %s %s [-A[c][<size>]][,<inc><unit>] [%s]\n", GMT_Rgeo_OPT, GMT_J_OPT, GMT_B_OPT);
	GMT_message (GMT, "\t[-Cf|g|e] [-Da<startdate>] [-Db<stopdate>] [-F] [-Gt|d<gap>] [-I<code>] [-K] [-L<trackticks>] [-N] [-O] [-P] [-Sa<startdist>[<unit>]]\n");
	GMT_message (GMT, "\t[-Sb<stopdist>[<unit>]] [-TT|t|d<ms,mc,mfs,mf,mfc>] [%s] [%s] [-W<pen>] [%s]\n", GMT_U_OPT, GMT_V_OPT, GMT_X_OPT);
	GMT_message (GMT, "\t[%s] [%s] [%s] [%s]\n\n", GMT_Y_OPT, GMT_c_OPT, GMT_p_OPT, GMT_t_OPT);
     
	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);
             
	MGD77_Cruise_Explain (GMT);
	GMT_Option (C, "J-,R");
	GMT_message (GMT, "\tOPTIONS:\n\n");
	GMT_message (GMT, "\t-A Annotate legs when they enter the grid. Append c for cruise ID [Default is file prefix];\n");
	GMT_message (GMT, "\t   <size> is optional text size in points [9].  The font used is controlled by FONT_LABEL.\n");
	GMT_message (GMT, "\t   Optionally, append ,<inc>[unit] to place label every <inc> units apart; <unit> may be\n");
	GMT_message (GMT, "\t   k (km) or n (nautical miles), or d (days), h (hours).\n");
	GMT_Option (C, "B-");
	GMT_message (GMT, "\t-C Select procedure for along-track distance calculations:\n");
	GMT_message (GMT, "\t   f Flat Earth\n");
	GMT_message (GMT, "\t   g Great circle [Default]\n");
	GMT_message (GMT, "\t   e Ellipsoidal (geodesic) using current ellipsoid\n");
	GMT_message (GMT, "\t-D Plot from a<startdate> (given as yyyy-mm-ddT[hh:mm:ss]) [Start of cruise]\n");
	GMT_message (GMT, "\t   up to b<stopdate> (given as yyyy-mm-ddT[hh:mm:ss]) [End of cruise]\n");
	GMT_message (GMT, "\t-F Do NOT apply bitflags to MGD77+ cruises [Default applies error flags stored in the file].\n");
	GMT_message (GMT, "\t-G Consider point separations exceeding d<gap> (km) or t<gap> (minutes) to indicate a gap (do not draw) [0].\n");
	GMT_message (GMT, "\t-I Ignore certain data file formats from consideration. Append combination of act to ignore\n");
	GMT_message (GMT, "\t   (a) MGD77 ASCII, (c) MGD77+ netCDF, (m) MGD77T ASCII, or (t) plain table files. [Default ignores none].\n");
	GMT_Option (C, "K");
	GMT_message (GMT, "\t-L Put time/distance log marks on the track. E.g. a500ka24ht6h means (a)nnotate\n");
	GMT_message (GMT, "\t   every 500 km (k) and 24 h(ours), with (t)ickmarks every 500 km and 6 (h)ours.\n");
	GMT_message (GMT, "\t   Units of n(autical miles) and d(ays) are also recognized.\n");
	GMT_message (GMT, "\t-N Do Not clip leg name annotation that fall outside map border [Default will clip].\n");
	GMT_Option (C, "O,P");
	GMT_message (GMT, "\t-S Plot from a<startdist>[<unit>], with <unit> from %s [meter] [Start of cruise]\n", GMT_LEN_UNITS2_DISPLAY);
	GMT_message (GMT, "\t   up to b<stopdist> [End of cruise].\n");
	GMT_message (GMT, "\t-T Set attributes of marker items. Append T for new day marker, t for same\n");
	GMT_message (GMT, "\t   day marker, and d for distance marker.  Then, append 5 comma-separated items:\n");
	GMT_message (GMT, "\t   <markersize>[unit],<markercolor>,<markerfontsize,<markerfont>,<markerfontcolor>\n");
	GMT_message (GMT, "\t   Default settings for the three marker types are:\n");
	GMT_message (GMT, "\t     -TT%g,black,%g,%d,black\n", Ctrl->T.marker[MGD77TRACK_MARK_NEWDAY].marker_size, Ctrl->T.marker[MGD77TRACK_MARK_NEWDAY].font.size, Ctrl->T.marker[MGD77TRACK_MARK_NEWDAY].font.id);
	GMT_message (GMT, "\t     -Tt%g,white,%g,%d,black\n", Ctrl->T.marker[MGD77TRACK_MARK_SAMEDAY].marker_size, Ctrl->T.marker[MGD77TRACK_MARK_SAMEDAY].font.size, Ctrl->T.marker[MGD77TRACK_MARK_SAMEDAY].font.id);
	GMT_message (GMT, "\t     -Td%g,black,%g,%d,black\n", Ctrl->T.marker[MGD77TRACK_MARK_DIST].marker_size, Ctrl->T.marker[MGD77TRACK_MARK_DIST].font.size, Ctrl->T.marker[MGD77TRACK_MARK_DIST].font.id);
	GMT_Option (C, "U,V");
	GMT_message (GMT, "\t-W Set track pen attributes [%s].\n", GMT_putpen (GMT, Ctrl->W.pen));
	GMT_Option (C, "X,c,p,t,.");
	
	return (EXIT_FAILURE);
}

int get_annotinfo (char *args, struct MGD77TRACK_ANNOT *info)
{
	int i1, i2, flag1, flag2, type;
	bool error = false;
	double value;
	
	info->annot_int_dist = info->tick_int_dist = 0;
	info->annot_int_time = info->tick_int_time = 0;

	i1 = 0;
	while (args[i1]) {
		flag1 = 'a';
		if (isalpha ((int)args[i1])) {
			flag1 = args[i1];
			if (isupper((int)flag1)) flag1 = tolower ((int)flag1);
			i1++;
		}
		i2 = i1;
		while (args[i2] && strchr ("athkmnd", (int)args[i2]) == NULL) i2++;
		value = atof (&args[i1]);
		flag2 = args[i2];
		if (isupper((int)flag2)) flag2 = tolower ((int)flag2);
		if (flag2 == 'd') {		/* Days */
			value *= GMT_DAY2SEC_F;
			type = 't';
		}
		else if (flag2 == 'h') {		/* Hours */
			value *= GMT_HR2SEC_F;
			type = 't';
		}
		else if (flag2 == 'k') {	/* kilometers */
			value *= 1000;
			type = 'd';
		}
		else if (flag2 == 'n') {	/* Nautical miles */
			value *= MGD77_METERS_PER_NM;
			type = 'd';
		}
		else if (flag2 == 'm') {	/* Minutes */
			value *= GMT_MIN2SEC_F;
			type = 't';
		}
		else				/* Default is seconds */
			type = 't';
		i2++;
		if (flag1 == 'a') {	/* Annotation interval */
			if (type == 'd')	/* Distance */
				info->annot_int_dist = (int)value;
			else
				info->annot_int_time = (int)value;
		}
		else {			/* Tickmark interval */
			if (type == 'd')	/* Distance */
				info->tick_int_dist = (int)value;
			else
				info->tick_int_time = (int)value;
		}
		i1 = i2;
	}
	if (info->annot_int_dist <= 0 && info->tick_int_dist <= 0 && info->annot_int_time <= 0 && info->tick_int_time <= 0)
		error = true;
	if (info->annot_int_dist <= 0)
		info->annot_int_dist = info->tick_int_dist;
	else if (info->tick_int_dist <= 0)
		info->tick_int_dist = info->annot_int_dist;
	if (info->annot_int_time <= 0)
		info->annot_int_time = info->tick_int_time;
	else if (info->tick_int_time <= 0)
		info->tick_int_time = info->annot_int_time;
	return (error);
}

int GMT_mgd77track_parse (struct GMTAPI_CTRL *C, struct MGD77TRACK_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to mgd77track and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, mrk = 0;
	bool error = false;
	int j;
	char ms[GMT_TEXT_LEN64], mc[GMT_TEXT_LEN64], tmp[GMT_TEXT_LEN64], mfs[GMT_TEXT_LEN64], mf[GMT_TEXT_LEN64];
	char comment[GMT_BUFSIZ], mfc[GMT_TEXT_LEN64], *t = NULL;
	double dist_scale;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Skip input files */
			case '#':	/* Skip input files confused as numbers (e.g. 123456) */
				break;

			/* Processes program-specific parameters */

			case 'A':
				Ctrl->A.mode = 1;
				j = 0;
				if (opt->arg[0] == 'c') j++, Ctrl->A.mode = 2;
				if (opt->arg[j] && opt->arg[j] != ',') Ctrl->A.size = atof (&opt->arg[j]) * GMT->session.u2u[GMT_PT][GMT_INCH];
				if ((t = strchr (&opt->arg[2], ','))) {	/* Want label at regular spacing */
					t++;	/* Skip the comma */
					error += get_annotinfo (t, &Ctrl->A.info);
					Ctrl->A.mode = -Ctrl->A.mode;	/* Flag to tell machinery not to annot at entry */
				}
				break;

			case 'C':	/* Distance calculation method */
				Ctrl->C.active = true;
				if (opt->arg[0] == 'f') Ctrl->C.mode = 1;
				if (opt->arg[0] == 'g') Ctrl->C.mode = 2;
				if (opt->arg[0] == 'e') Ctrl->C.mode = 3;
				if (Ctrl->C.mode < 1 || Ctrl->C.mode > 3) {
					GMT_report (GMT, GMT_MSG_NORMAL, "Error -C: Flag must be f, g, or e\n");
					n_errors++;
				}
				break;
			case 'D':		/* Assign start/stop times for sub-section */
				Ctrl->D.active = true;
				switch (opt->arg[0]) {
				 	case 'A':		/* Start date, skip records with time = NaN */
						Ctrl->D.mode = true;
				 	case 'a':		/* Start date */
						t = &opt->arg[1];
						if (t && GMT_verify_expectations (GMT, GMT_IS_ABSTIME, GMT_scanf (GMT, t, GMT_IS_ABSTIME, &Ctrl->D.start), t)) {
							GMT_report (GMT, GMT_MSG_NORMAL, "Error -Da: Start time (%s) in wrong format\n", t);
							n_errors++;
						}
						break;
					case 'B':		/* Stop date, skip records with time = NaN */
						Ctrl->D.mode = true;
					case 'b':		/* Stop date */
						t = &opt->arg[1];
						if (t && GMT_verify_expectations (GMT, GMT_IS_ABSTIME, GMT_scanf (GMT, t, GMT_IS_ABSTIME, &Ctrl->D.stop), t)) {
							GMT_report (GMT, GMT_MSG_NORMAL, "Error -Db : Stop time (%s) in wrong format\n", t);
							n_errors++;
						}
						break;
					default:
						n_errors++;
						break;
				}
				break;

			case 'F':	/* Do NOT apply bitflags */
				Ctrl->F.active = true;
				switch (opt->arg[0]) {
					case '\0':	/* Both sets */
						Ctrl->F.mode = MGD77_NOT_SET;
						break;
					case 'm':	/* MGD77 set */
						Ctrl->F.mode = MGD77_M77_SET;
						break;
					case 'e':	/* extra CDF set */
						Ctrl->F.mode = MGD77_CDF_SET;
						break;
					default:
						GMT_report (GMT, GMT_MSG_NORMAL, "Error -T: append m, e, or neither\n");
						n_errors++;
						break;
				}
				break;

			case 'G':
				switch (opt->arg[0]) {
					case 'd':	/* Distance gap in km */
					Ctrl->G.active[GAP_D] = true;
						Ctrl->G.value[GAP_D] = lrint (atof (&opt->arg[1]) * 1000.0);	/* Gap converted to m from km */
						break;
					case 't':	/* Distance gap in minutes */
						Ctrl->G.active[GAP_T] = true;
						Ctrl->G.value[GAP_T] = lrint (atof (&opt->arg[1]) * 60.0);	/* Gap converted to seconds from minutes */
						break;
					default:
						GMT_report (GMT, GMT_MSG_NORMAL, "Error -G: Requires t|d and a positive value in km (d) or minutes (t)\n");
						n_errors++;
						break;
				}
				break;

			case 'I':
				Ctrl->I.active = true;
				if (Ctrl->I.n < 3) {
					if (strchr ("act", (int)opt->arg[0]))
						Ctrl->I.code[Ctrl->I.n++] = opt->arg[0];
					else {
						GMT_report (GMT, GMT_MSG_NORMAL, "Option -I Bad modifier (%c). Use -Ia|c|t!\n", opt->arg[0]);
						n_errors++;
					}
				}
				else {
					GMT_report (GMT, GMT_MSG_NORMAL, "Option -I: Can only be applied 0-2 times\n");
					n_errors++;
				}
				break;
			case 'L':
				Ctrl->L.active = true;
				error += get_annotinfo (opt->arg, &Ctrl->L.info);
				break;

			case 'N':
				Ctrl->N.active = true;
				break;

			case 'S':		/* Assign start/stop position for sub-section (in meters) */
				Ctrl->S.active = true;
				if (opt->arg[0] == 'a') {		/* Start position */
					MGD77_Set_Unit (GMT, &opt->arg[1], &dist_scale, 1);
					Ctrl->S.start = atof (&opt->arg[1]) * dist_scale;
				}
				else if (opt->arg[0] == 'b') {	/* Stop position */
					MGD77_Set_Unit (GMT, &opt->arg[1], &dist_scale, 1);
					Ctrl->S.stop = atof (&opt->arg[1]) * dist_scale;
				}
				else
					n_errors++;
				break;

			case 'T':	/* Marker attributes */
				Ctrl->T.active = true;
				switch (opt->arg[0]) {
					case 'T':	/* New day marker */
						mrk = MGD77TRACK_MARK_NEWDAY;
						break;
					case 't':	/* Same day marker */
						mrk = MGD77TRACK_MARK_SAMEDAY;
						break;
					case 'd':	/* Distance marker */
						mrk = MGD77TRACK_MARK_DIST;
						break;
					default:
						error = true;
						break;
				}
				if (error) {
					GMT_report (GMT, GMT_MSG_NORMAL, "Error: Unrecognized modifier %c given to -T\n", opt->arg[0]);
					n_errors++;
				}
				strncpy (comment, &opt->arg[1], GMT_BUFSIZ);
				for (j = 0; j < (int)strlen (comment); j++) if (comment[j] == ',') comment[j] = ' ';	/* Replace commas with spaces */
				j = sscanf (comment, "%s %s %s %s %s", ms, mc, mfs, mf, mfc);
				if (j != 5) {
					GMT_report (GMT, GMT_MSG_NORMAL, "Error: -TT|t|d takes 5 arguments\n");
					n_errors++;
				}
				Ctrl->T.marker[mrk].marker_size = GMT_to_inch (GMT, ms);
				if (GMT_getfill (GMT, mc, &Ctrl->T.marker[mrk].s)) {
					GMT_report (GMT, GMT_MSG_NORMAL, "Error: Bad fill specification for -T\n");
					GMT_fill_syntax (GMT, 'T', " ");
					n_errors++;
				}
				sprintf (tmp, "%s,%s,", mfs, mf);	/* Put mfs and mf together in order to be used by GMT_getpen */
				GMT_getfont (GMT, tmp, &Ctrl->T.marker[mrk].font);
				if (GMT_getfill (GMT, mfc, &Ctrl->T.marker[mrk].f)) {
					GMT_report (GMT, GMT_MSG_NORMAL, "Error: Bad fill specification for -T\n");
					GMT_fill_syntax (GMT, 'T', " ");
					n_errors++;
				}
				break;

			case 'W':
				Ctrl->W.active = true;
				if (GMT_getpen (GMT, opt->arg, &Ctrl->W.pen)) {
					GMT_pen_syntax (GMT, 'W', " ");
					n_errors++;
				}
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, Ctrl->D.start > 0.0 && Ctrl->S.start > 0.0, "Syntax error: Cannot specify both start time AND start distance\n");
	n_errors += GMT_check_condition (GMT, Ctrl->D.stop < DBL_MAX && Ctrl->S.stop < DBL_MAX, "Syntax error: Cannot specify both stop time AND stop distance\n");
	n_errors += GMT_check_condition (GMT, Ctrl->S.start > Ctrl->S.stop, "Syntax error -S: Start distance exceeds stop distance!\n");
	n_errors += GMT_check_condition (GMT, Ctrl->D.start > Ctrl->D.stop, "Syntax error -D: Start time exceeds stop time!\n");
	n_errors += GMT_check_condition (GMT, Ctrl->G.active[GAP_D] && Ctrl->G.value[GAP_D] <= 0.0, "Syntax error -Gd: Must specify a positive gap distance in km!\n");
	n_errors += GMT_check_condition (GMT, Ctrl->G.active[GAP_T] && Ctrl->G.value[GAP_T] <= 0.0, "Syntax error -Gt: Must specify a positive gap distance in minutes!\n");
	n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error: Region is not set\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

double get_heading (struct GMT_CTRL *GMT, int rec, double *lon, double *lat, int n_records)
{
	int i1, i2, j;
	double angle, x1, x0, y1, y0, sum_x2 = 0.0, sum_xy = 0.0, sum_y2 = 0.0, dx, dy;
	
	i1 = rec - 10;
	if (i1 < 0) i1 = 0;
	i2 = i1 + 10;
	if (i2 > (n_records-1)) i2 = n_records - 1;
	GMT_geo_to_xy (GMT, lon[rec], lat[rec], &x0, &y0);
	for (j = i1; j <= i2; j++) {	/* L2 fit for slope over this range of points */
		GMT_geo_to_xy (GMT, lon[j], lat[j], &x1, &y1);
		dx = x1 - x0;
		dy = y1 - y0;
		sum_x2 += dx * dx;
		sum_y2 += dy * dy;
		sum_xy += dx * dy;
	}
	if (sum_y2 < GMT_CONV_LIMIT)	/* Line is horizontal */
		angle = 0.0;
	else if (sum_x2 < GMT_CONV_LIMIT)	/* Line is vertical */
		angle = 90.0;
	else
		angle = (GMT_IS_ZERO (sum_xy)) ? 90.0 : d_atan2d (sum_xy, sum_x2);
	if (angle > 90.0)
		angle -= 180;
	else if (angle < -90.0)
		angle += 180.0;
	return (angle);
}

void annot_legname (struct GMT_CTRL *GMT, struct PSL_CTRL *PSL, double x, double y, double lon, double lat, double angle, char *text, double size)
{
	int just, form;
	
	if (lat < GMT->common.R.wesn[YLO])
		just = (angle >= 0.0) ? 1 : 3;
	else if (lat > GMT->common.R.wesn[YHI])
		just = (angle >= 0.0) ? 11 : 9;
	else if (lon < GMT->common.R.wesn[XLO])
		just = (angle >= 0.0) ? 9 : 1;
	else
		just = (angle >= 0.0) ? 3 : 11;
	form = GMT_setfont (GMT, &GMT->current.setting.font_label);
	GMT_smart_justify (GMT, just, angle, GMT->session.u2u[GMT_PT][GMT_INCH] * 0.15 * size, GMT->session.u2u[GMT_PT][GMT_INCH] * 0.15 * size, &x, &y, 1);
	PSL_plottext (PSL, x, y, size, text, angle, just, form);
}

int bad_coordinates (double lon, double lat) {
	return (GMT_is_dnan (lon) || GMT_is_dnan (lat));
}

extern void GMT_gcal_from_dt (struct GMT_CTRL *C, double t, struct GMT_gcal *cal);	/* Break internal time into calendar and clock struct info  */

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_mgd77track_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_mgd77track (void *V_API, int mode, void *args)
{
	uint64_t rec, first_rec, last_rec, i, n_id = 0, mrk = 0, dist_flag = 2, use, n_paths, argno, n_cruises = 0;
	int this_julian = 0, last_julian, error = 0;
	bool first, form, both = false;
	unsigned int annot_tick[2] = {0, 0}, draw_tick[2] = {0, 0};
	
	size_t n_alloc_c = GMT_SMALL_CHUNK;
	
       	char label[GMT_TEXT_LEN256], date[GMT_TEXT_LEN64], clock[GMT_TEXT_LEN64];
	char name[GMT_TEXT_LEN64], **list = NULL;

	double x, y, annot_dist[2] = {0, 0}, tick_dist[2] = {0, 0}, annot_time[2] = {0, 0};
	double *track_dist = NULL, angle, plot_x, plot_y, *lon = NULL, *lat = NULL, *track_time = NULL;
	double factor = 0.001, c_angle, tick_time[2] = {0, 0};
	
	struct MGD77_CONTROL M;
	struct MGD77_DATASET *D = NULL;
	struct MGD77TRACK_LEG_ANNOT *cruise_id = NULL;
	struct GMT_gcal calendar;
	struct MGD77TRACK_ANNOT *info[2] = {NULL, NULL};
	struct MGD77TRACK_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct PSL_CTRL *PSL = NULL;		/* General PSL interal parameters */
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_NOT_A_SESSION);
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	Ctrl = New_mgd77track_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_mgd77track_usage (API, GMTAPI_USAGE, Ctrl));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_mgd77track_usage (API, GMTAPI_SYNOPSIS, Ctrl));	/* Return the synopsis */

	/* Parse the command-line arguments */

	if (GMT_Parse_Common (API, GMT_PROG_OPTIONS, options)) Return (API->error);
	if ((error = GMT_mgd77track_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the mgd77track main code ----------------------------*/
	
	/* Initialize MGD77 output order and other parameters*/
	
	MGD77_Init (GMT, &M);			/* Initialize MGD77 Machinery */
	if (Ctrl->I.active) MGD77_Process_Ignore (GMT, 'I', Ctrl->I.code);
	info[LABEL] = &(Ctrl->L.info);	info[ANNOT] = &(Ctrl->A.info);

	if (Ctrl->F.active) {	/* Turn off automatic corrections */
		if (Ctrl->F.mode == MGD77_NOT_SET)	/* Both sets */
			M.use_corrections[MGD77_M77_SET] = M.use_corrections[MGD77_CDF_SET] = false;
		else if (Ctrl->F.mode == MGD77_M77_SET) /* MGD77 set */
			M.use_corrections[MGD77_M77_SET] = false;
		else	/* extra CDF set */
			M.use_corrections[MGD77_CDF_SET] = false;
	}
	
	/* Check that the options selected are mutually consistent */
	
	n_paths = MGD77_Path_Expand (GMT, &M, options, &list);	/* Get list of requested IDs */

	if (n_paths == 0) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Error: No cruises given\n");
		Return (EXIT_FAILURE);
	}

	use = (M.original) ? MGD77_ORIG : MGD77_REVISED;
		
	if (GMT_err_pass (GMT, GMT_map_setup (GMT, GMT->common.R.wesn), "")) Return (GMT_RUNTIME_ERROR);
	
	PSL = GMT_plotinit (GMT, options);
	GMT_plane_perspective (GMT, GMT->current.proj.z_project.view_plane, GMT->current.proj.z_level);
	GMT_plotcanvas (GMT);	/* Fill canvas if requested */
	
	GMT_map_clip_on (GMT, GMT->session.no_rgb, 3);
	GMT_setpen (GMT, &Ctrl->W.pen);
	both = (Ctrl->L.info.annot_int_time && Ctrl->L.info.annot_int_dist);
	
	if (Ctrl->N.active) cruise_id = GMT_memory (GMT, NULL, n_alloc_c, struct MGD77TRACK_LEG_ANNOT);

	MGD77_Select_Columns (GMT, "time,lon,lat", &M, MGD77_SET_ALLEXACT);	/* This sets up which columns to read */

	for (argno = 0; argno < n_paths; argno++) {		/* Process each ID */
	
		D = MGD77_Create_Dataset (GMT);	/* Get data structure w/header */
		if (MGD77_Open_File (GMT, list[argno], &M, MGD77_READ_MODE)) continue;

		GMT_report (GMT, GMT_MSG_VERBOSE, "Now processing cruise %s\n", list[argno]);
		
		if (MGD77_Read_Header_Record (GMT, list[argno], &M, &D->H)) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Error reading header sequence for cruise %s\n", list[argno]);
			Return (EXIT_FAILURE);
		}
		rec = 0;
		last_julian = -1;
		
		if (abs (Ctrl->A.mode) == 2)	/* Use MGD77 cruise ID */
			strncpy (name, D->H.mgd77[use]->Survey_Identifier, GMT_TEXT_LEN64);
		else {			/* Use file name prefix */
			strncpy (name, list[argno], GMT_TEXT_LEN64);
			for (i = 0; i < strlen (name); i++) if (name[i] == '.') name[i] = '\0';
		}
	
		/* Start reading data from file */
	
		if (MGD77_Read_Data (GMT, list[argno], &M, D)) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Error reading data set for cruise %s\n", list[argno]);
			Return (EXIT_FAILURE);
		}
		MGD77_Close_File (GMT, &M);
		track_time = (double*)D->values[0];
		lon = (double*)D->values[1];
		lat = (double*)D->values[2];
		if ((track_dist = GMT_dist_array_2 (GMT, lon, lat, D->H.n_records, 1.0, dist_flag)) == NULL) GMT_err_fail (GMT, GMT_MAP_BAD_DIST_FLAG, "");	/* Work internally in meters */
		for (rec = 0; rec < D->H.n_records && bad_coordinates (lon[rec], lat[rec]) && track_time[rec] < Ctrl->D.start && track_dist[rec] < Ctrl->S.start; rec++);	/* Find first record of interest */
		first_rec = rec;
		for (rec = D->H.n_records - 1; rec && track_time[rec] > Ctrl->D.stop && bad_coordinates (lon[rec], lat[rec]) && track_dist[rec] > Ctrl->S.stop; rec--);	/* Find last record of interest */
		last_rec = rec;
		GMT_report (GMT, GMT_MSG_VERBOSE, "mgd77track: Plotting %s [%s]\n", list[argno], D->H.mgd77[use]->Survey_Identifier);
		PSL_comment (PSL, "Tracking %s", list[argno]);
		
		/* First draw the track line, clip segments outside the area */
		
		if (Ctrl->G.active[GAP_D] || Ctrl->G.active[GAP_T]) {
			uint64_t start, stop;
			start = first_rec;
			while (start < last_rec && ((Ctrl->G.active[GAP_D] && (track_dist[start+1] - track_dist[start]) > Ctrl->G.value[GAP_D]) || (Ctrl->G.active[GAP_T] && (track_time[start+1] - track_time[start]) > Ctrl->G.value[GAP_T]))) {	/* First start of first segment */
				lon[start] = GMT->session.d_NaN;	/* Flag to make sure we do not plot this gap later */
				start++;
			}
			while (start <= last_rec) {
				stop = start;
				while (stop < last_rec && ((Ctrl->G.active[GAP_D] && (track_dist[stop+1] - track_dist[stop]) < Ctrl->G.value[GAP_D]) || (Ctrl->G.active[GAP_T] && (track_time[stop+1] - track_time[stop]) < Ctrl->G.value[GAP_T]))) stop++;	/* stop will be last point in segment */
				GMT_geo_line (GMT, &lon[start], &lat[start], stop-start+1);
				start = stop + 1;
				while (start < last_rec && ((Ctrl->G.active[GAP_D] && (track_dist[start+1] - track_dist[start]) > Ctrl->G.value[GAP_D]) || (Ctrl->G.active[GAP_T] && (track_time[start+1] - track_time[start]) > Ctrl->G.value[GAP_T]))) {	/* First start of first segment */
					lon[start] = GMT->session.d_NaN;	/* Flag to make sure we do not plot this gap later */
					start++;
				}
			}
		}
		else {	/* Plot the whole shabang */
			GMT_geo_line (GMT, lon, lat, D->H.n_records);
		}

		first = true;
		for (rec = first_rec; rec <= last_rec; rec++) {
			if (bad_coordinates (lon[rec], lat[rec]) || GMT_map_outside (GMT, lon[rec], lat[rec])) {
				first = true;
				continue;
			}
			GMT_geo_to_xy (GMT, lon[rec], lat[rec], &x, &y);
			if (first) {
				if (Ctrl->A.mode > 0) {
					c_angle = get_heading (GMT, rec, lon, lat, D->H.n_records);
					if (Ctrl->N.active) {	/* Keep these in a list to plot after clipping is turned off */
						cruise_id[n_id].x = x;
						cruise_id[n_id].y = y;
						cruise_id[n_id].lon = lon[rec];
						cruise_id[n_id].lat = lat[rec];
						cruise_id[n_id].angle = c_angle;

						strncpy (cruise_id[n_id].text, name, 16U);
						n_id++;
						if (n_id == n_alloc_c) {
							size_t old_n_alloc = n_alloc_c;
							n_alloc_c <<= 1;
							cruise_id = GMT_memory (GMT, cruise_id, n_alloc_c, struct MGD77TRACK_LEG_ANNOT);
							GMT_memset (&(cruise_id[old_n_alloc]), n_alloc_c - old_n_alloc,  struct MGD77TRACK_LEG_ANNOT);	/* Set to NULL/0 */
						}
					}
					else
						annot_legname (GMT, PSL, x, y, lon[rec], lat[rec], c_angle, name, GMT->session.u2u[GMT_INCH][GMT_PT] * 1.25 * Ctrl->A.size);
				}
				first = false;
				for (i = 0; i < 2; i++) {
					if (info[i]->annot_int_dist > 0) annot_dist[i] = (track_dist[rec] / info[i]->annot_int_dist + 1) * info[i]->annot_int_dist;
					if (info[i]->tick_int_dist > 0) tick_dist[i] = (track_dist[rec] / info[i]->tick_int_dist + 1) * info[i]->tick_int_dist;
					if (info[i]->annot_int_time > 0) annot_time[i] = ceil (track_time[rec] / info[i]->annot_int_time) * info[i]->annot_int_time;
					if (info[i]->tick_int_time > 0) tick_time[i] = ceil (track_time[rec] / info[i]->tick_int_time) * info[i]->tick_int_time;
				}
			}
			
			/* See if we need to annotate/tick the trackline for time/km and/or ID marks */
			
			for (i = 0; i < 2; i++) {
				if (info[i]->annot_int_time && (track_time[rec] >= annot_time[i])) {
					annot_time[i] += info[i]->annot_int_time;
					annot_tick[i] = 1;
				}
				if (info[i]->annot_int_dist && (track_dist[rec] >= annot_dist[i])) {
					annot_dist[i] += info[i]->annot_int_dist;
					annot_tick[i] += 2;
				}
				if (info[i]->tick_int_time && (track_time[rec] >= tick_time[i])) {
					tick_time[i] += info[i]->tick_int_time;
					draw_tick[i] = 1;
				}
				if (info[i]->tick_int_dist && (track_dist[rec] >= tick_dist[i])) {
					tick_dist[i] += info[i]->tick_int_dist;
					draw_tick[i] += 2;
				}
			}
			if (annot_tick[ANNOT]) {
				angle = get_heading (GMT, rec, lon, lat, D->H.n_records);
				if (angle < 0.0)
					angle += 90.0;
				else
					angle -= 90.0;
				if (annot_tick[ANNOT] & 1) {	/* Time mark */
					GMT_gcal_from_dt (GMT, annot_time[ANNOT], &calendar);			/* Convert t to a complete calendar structure */
					GMT_format_calendar (GMT, date, clock, &GMT->current.plot.calclock.date, &GMT->current.plot.calclock.clock, false, 1, annot_time[ANNOT]);
					this_julian = calendar.day_y;
					if (this_julian != last_julian) {
						mrk = MGD77TRACK_MARK_NEWDAY;
						sprintf (label, "%s+%s", date, clock);
					}
					else {
						mrk = MGD77TRACK_MARK_SAMEDAY;
						sprintf (label, "+%s", clock);
					}
					GMT_setfill (GMT, &Ctrl->T.marker[mrk].s, true);
					PSL_plotsymbol (PSL, x, y, &(Ctrl->T.marker[mrk].marker_size), GMT_SYMBOL_CIRCLE);
					form = GMT_setfont (GMT, &Ctrl->T.marker[mrk].font);
					plot_x = x;	plot_y = y;
					GMT_smart_justify (GMT, 5, angle, 0.5 * Ctrl->T.marker[mrk].font_size, 0.5 * Ctrl->T.marker[mrk].font_size, &plot_x, &plot_y, 1);
					PSL_plottext (PSL, plot_x, plot_y, GMT->session.u2u[GMT_INCH][GMT_PT] * Ctrl->T.marker[mrk].font_size, label, angle, 5, form);
					last_julian = calendar.day_y;
				}
				if (annot_tick[ANNOT] & 2) {	/* Distance mark */
					mrk = MGD77TRACK_MARK_DIST;
					sprintf (label, "%d km  ", (int)((annot_dist[ANNOT] - Ctrl->L.info.annot_int_dist) * factor));
					GMT_setfill (GMT, &Ctrl->T.marker[mrk].s, true);
					PSL_plotsymbol (PSL, x, y, &(Ctrl->T.marker[mrk].marker_size), GMT_SYMBOL_SQUARE);
					form = GMT_setfont (GMT, &Ctrl->T.marker[mrk].font);
					plot_x = x;	plot_y = y;
					GMT_smart_justify (GMT, 7, angle, 0.5 * Ctrl->T.marker[mrk].font_size, 0.5 * Ctrl->T.marker[mrk].font_size, &plot_x, &plot_y, 1);
					PSL_plottext (PSL, plot_x, plot_y, GMT->session.u2u[GMT_INCH][GMT_PT] * Ctrl->T.marker[mrk].font_size, label, angle, 7, form);
				}
			}
			if (both && !(annot_tick[ANNOT] & 1) && (draw_tick[ANNOT] & 1)) {
				mrk = (this_julian != last_julian) ? MGD77TRACK_MARK_NEWDAY : MGD77TRACK_MARK_SAMEDAY;
				GMT_setfill (GMT, &Ctrl->T.marker[mrk].s, true);
				PSL_plotsymbol (PSL, x, y, &(Ctrl->T.marker[mrk].marker_size), GMT_SYMBOL_CIRCLE);
			}
			if (both && !(annot_tick[ANNOT] & 2) && (draw_tick[ANNOT] & 2)) {
				mrk = (this_julian != last_julian) ? MGD77TRACK_MARK_NEWDAY : MGD77TRACK_MARK_SAMEDAY;
				GMT_setfill (GMT, &Ctrl->T.marker[mrk].s, true);
				PSL_plotsymbol (PSL, x, y, &(Ctrl->T.marker[mrk].marker_size), GMT_SYMBOL_SQUARE);
			}
			if (draw_tick[ANNOT]) {
				mrk = MGD77TRACK_MARK_DIST;
				PSL_setcolor (PSL, Ctrl->T.marker[mrk].s.rgb, PSL_IS_STROKE);
				PSL_plotsymbol (PSL, x, y, &(Ctrl->T.marker[mrk].marker_size), GMT_SYMBOL_CROSS);
			}
			if (annot_tick[ANNOT] || draw_tick[ANNOT]) annot_tick[ANNOT] = draw_tick[ANNOT] = false;
			if (annot_tick[LABEL]) {
				angle = get_heading (GMT, rec, lon, lat, D->H.n_records);
				if (angle < 0.0)
					angle += 90.0;
				else
					angle -= 90.0;
				annot_legname (GMT, PSL, x, y, lon[rec], lat[rec], angle, name, GMT->session.u2u[GMT_INCH][GMT_PT] * 1.25 * Ctrl->A.size);
				annot_tick[LABEL] = false;
			}
		}
		MGD77_Free_Dataset (GMT, &D);	/* Free memory allocated by MGD77_Read_File */
		GMT_free (GMT, track_dist);
		n_cruises++;
	}
		
	GMT_map_clip_off (GMT);

	GMT_map_basemap (GMT);
	
	if (Ctrl->A.mode > 0 && Ctrl->N.active) {	/* Plot leg names after clipping is terminated ( see -N) */
		unsigned int id;
		double size;
		size = GMT->session.u2u[GMT_INCH][GMT_PT] * 1.25 * Ctrl->A.size;
		for (id = 0; id < n_id; id++) annot_legname (GMT, PSL, cruise_id[id].x, cruise_id[id].y, cruise_id[id].lon, cruise_id[id].lat, cruise_id[id].angle, cruise_id[id].text, size);
		GMT_free (GMT, cruise_id);
	}

	GMT_plane_perspective (GMT, -1, 0.0);
	GMT_plotend (GMT);
	
	GMT_report (GMT, GMT_MSG_VERBOSE, "Plotted %d cruises\n", n_cruises);

	MGD77_Path_Free (GMT, n_paths, list);
	MGD77_end (GMT, &M);
	
	Return (GMT_OK);
}
