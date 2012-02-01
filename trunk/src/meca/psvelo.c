/*--------------------------------------------------------------------
 *    $Id$
 *
 *    Copyright (c) 1996-2012 by G. Patau
 *    Distributed under the GNU Public Licence
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/*

psvelo will read <x,y> pairs (or <lon,lat>) from inputfile and
plot symbols on a map. Velocity ellipses, strain
crosses, or strain wedges, may be specified, some of which require
additional columns of data.  Only one symbol may be plotted at a time.
PostScript code is written to stdout.


 Author:	Kurt Feigl
 Date:		7 July 1998
 Version:	4
 Roots:		based on psxy.c
 Adapted to version 3.3 by Genevieve Patau (25 June 1999)
 Last modified : 18 February 2000

*/

#include "pslib.h"	/* to have pslib environment */
#include "gmt_meca.h"	/* to have gmt_meca supplements */

#include "meca.h"
#include "utilmeca.h"

#define CINE 1
#define ANISO 2
#define WEDGE 3
#define CROSS 4

#define DEFAULT_FONTSIZE	9.0	/* In points */

#define READ_ELLIPSE	0
#define READ_ROTELLIPSE	1
#define READ_ANISOTROPY	2
#define READ_WEDGE	4
#define READ_CROSS	8

/* parameters for writing text */
#define ANGLE		0.0
#define FORM		0

/* Control structure for psvelo */

struct PSVELO_CTRL {
	struct A {	/* -A */
		GMT_LONG active;
		struct GMT_SYMBOL S;
	} A;
	struct D {	/* -D */
		GMT_LONG active;
		double scale;
	} D;
 	struct E {	/* -E<fill> */
		GMT_LONG active;
		struct GMT_FILL fill;
	} E;
 	struct G {	/* -G<fill> */
		GMT_LONG active;
		struct GMT_FILL fill;
	} G;
	struct L {	/* -L */
		GMT_LONG active;
	} L;
	struct N {	/* -N */
		GMT_LONG active;
	} N;
	struct S {	/* -r<fill> */
		GMT_LONG active;
		GMT_LONG symbol;
		GMT_LONG readmode;
		double scale, wedge_amp, conrad;
		double fontsize, confidence;
		struct GMT_FILL fill;
	} S;
	struct W {	/* -W<pen> */
		GMT_LONG active;
		struct GMT_PEN pen;
	} W;
};

void *New_psvelo_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct PSVELO_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct PSVELO_CTRL);

	/* Initialize values whose defaults are not 0/FALSE/NULL */

	C->A.S.v.h_length = C->A.S.size_x = VECTOR_HEAD_LENGTH * GMT->session.u2u[GMT_PT][GMT_INCH];	/* 9p */
	C->A.S.v.v_angle = 30.0;
	C->A.S.v.status = GMT_VEC_END + GMT_VEC_FILL + GMT_VEC_OUTLINE;
	C->A.S.v.pen = GMT->current.setting.map_default_pen;
#ifdef GMT_COMPAT
	GMT->current.setting.map_vector_shape = 0.4;	/* Historical reasons */
#endif
	C->D.scale = 1.0;
	GMT_init_fill (GMT, &C->E.fill, 1.0, 1.0, 1.0);
	GMT_init_fill (GMT, &C->G.fill, 0.0, 0.0, 0.0);
	C->S.wedge_amp = 1.e7;
	C->S.conrad = 1.0;
	C->S.fontsize = DEFAULT_FONTSIZE;
	C->W.pen = GMT->current.setting.map_default_pen;
	return (C);
}

void Free_psvelo_Ctrl (struct GMT_CTRL *GMT, struct PSVELO_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	GMT_free (GMT, C);
}

GMT_LONG GMT_psvelo_usage (struct GMTAPI_CTRL *C, GMT_LONG level)
{
	struct GMT_CTRL *GMT = C->GMT;

	/* This displays the psvelo synopsis and optionally full usage information */

	GMT_message (GMT,"psvelo %s - Plot velocity vectors, crosses, and wedges on maps\n\n", GMT_VERSION);
	GMT_message (GMT,"usage: psvelo [<table>] %s %s [-A<awidth>/<alength>/<hwidth>] [%s]\n", GMT_J_OPT, GMT_Rgeo_OPT, GMT_B_OPT);
	GMT_message (GMT, "\t[-G<fill>] [-K] [-L] [-N] [-O]\n");
	GMT_message (GMT, "\t[-P] [-S<symbol><scale><fontsize>] [%s] [-V] [-W<pen>] [%s]\n", GMT_U_OPT, GMT_X_OPT);
	GMT_message (GMT, "\t[%s] [%s] [%s] [%s] [%s]\n\n", GMT_Y_OPT, GMT_c_OPT, GMT_h_OPT, GMT_i_OPT, GMT_colon_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_explain_options (GMT, "jR");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "<b");
	GMT_message (GMT, "\t-A Specify arrow head attributes:");
	GMT_vector_syntax (GMT, 15);
	GMT_message (GMT, "\t   Default is %gp+gblack+p1p\n", VECTOR_HEAD_LENGTH);
	GMT_message (GMT, "\t-D Multiply uncertainties by sigscale. (Se and Sw only)i\n");
	GMT_message (GMT, "\t-E Set color used for uncertainty wedges in -Sw option.\n");
	GMT_message (GMT, "\t-G Specify color (for symbols/polygons) or pattern (for polygons). fill can be either\n");
	GMT_message (GMT, "\t   1) <r/g/b> (each 0-255) for color or <gray> (0-255) for gray-shade [0].\n");
	GMT_message (GMT, "\t   2) p[or P]<iconsize>/<pattern> for predefined patterns (0-90).\n");
	GMT_explain_options (GMT, "K");
	GMT_message (GMT, "\t-L Draw line or symbol outline using the current pen (see -W).\n");
	GMT_message (GMT, "\t-N Do Not skip/clip symbols that fall outside map border [Default will ignore those outside].\n");
	GMT_explain_options (GMT, "OP");
	GMT_message (GMT, "\t-S Select symbol type and scale. Choose between:\n");
	GMT_message (GMT, "\t    (e) Velocity ellipses: in X,Y,Vx,Vy,SigX,SigY,CorXY,name format.\n");
	GMT_message (GMT, "\t    (r) Velocity ellipses: in X,Y,Vx,Vy,a,b,theta,name format.\n");
	GMT_message (GMT, "\t    (n) Anisotropy : in X,Y,Vx,Vy.\n");
	GMT_message (GMT, "\t    (w) Rotational wedges: in X,Y,Spin,Spinsig.\n");
	GMT_message (GMT, "\t    (x) Strain crosses : in X,Y,Eps1,Eps2,Theta.\n");
	GMT_explain_options (GMT, "UV");
	GMT_message (GMT,  "\t-W Set pen attributes [%s].\n", GMT_putpen (GMT, GMT->current.setting.map_default_pen));
	GMT_explain_options (GMT, "Xchi:.");
	
	return (EXIT_FAILURE);
}

GMT_LONG GMT_psvelo_parse (struct GMTAPI_CTRL *C, struct PSVELO_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to psvelo and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	GMT_LONG n_errors = 0, n, no_size_needed, n_set, got_A = FALSE;
	char txt[GMT_TEXT_LEN256], txt_b[GMT_TEXT_LEN256];
#ifdef GMT_COMPAT
	char txt_c[GMT_TEXT_LEN256];
#endif
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {

			case '<':	/* Skip input files */
				break;

			/* Processes program-specific parameters */

			case 'A':	/* Change size of arrow head */
				got_A = TRUE;
#ifdef GMT_COMPAT
				if (strchr (opt->arg, '/') && !strchr (opt->arg, '+')) {	/* Old-style args */
					sscanf (&opt->arg[1], "%[^/]/%[^/]/%s", txt, txt_b, txt_c);
					Ctrl->A.S.v.pen.width = GMT_to_points (GMT, txt);
					Ctrl->A.S.v.h_length = GMT_to_inch (GMT, txt_b);
					Ctrl->A.S.v.h_width = GMT_to_inch (GMT, txt_c);
					Ctrl->A.S.v.v_angle = atand (0.5 * Ctrl->A.S.v.h_width / Ctrl->A.S.v.h_length);
					Ctrl->A.S.v.status |= GMT_VEC_OUTLINE2;
				}
				else {
#endif
					if (opt->arg[0] == '+') {	/* No size (use default), just attributes */
						n_errors += GMT_parse_vector (GMT, opt->arg, &Ctrl->A.S);
					}
					else {	/* Size, plus possible attributes */
						n = sscanf (opt->arg, "%[^+]%s", txt, txt_b);	/* txt_a should be symbols size with any +<modifiers> in txt_b */
						if (n == 1) txt_b[0] = 0;	/* No modifiers present, set txt_b to empty */
						Ctrl->A.S.size_x = GMT_to_inch (GMT, txt);	/* Length of vector */
						n_errors += GMT_parse_vector (GMT, txt_b, &Ctrl->A.S);
					}
#ifdef GMT_COMPAT
				}
#endif
				break;
			case 'D':	/* Rescale Sigmas */
				Ctrl->D.active = TRUE;
				sscanf (opt->arg, "%lf",&Ctrl->D.scale);
				break;
			case 'E':	/* Set color for error ellipse  */
				GMT_getfill (GMT, opt->arg, &Ctrl->E.fill);
				Ctrl->E.active = TRUE;
				break;
			case 'G':	/* Set Gray shade for polygon */
				Ctrl->G.active = TRUE;
				GMT_getfill (GMT, opt->arg, &Ctrl->G.fill);
				break;
			case 'L':	/* Draw the outline */
				Ctrl->L.active = TRUE;
				break;
			case 'N':	/* Do not skip points outside border */
				Ctrl->N.active = TRUE;
				break;
			case 'S':	/* Get symbol [and size] */
 				txt_b[0] = '\0';
 				if (opt->arg[0] == 'e' || opt->arg[0] == 'r') {
					strcpy (txt, &opt->arg[1]);
					n = 0; while (txt[n] && txt[n] != '/') n++; txt[n] = 0;
					Ctrl->S.scale = GMT_to_inch (GMT, txt);
					sscanf (strchr(&opt->arg[1],'/')+1, "%lf/%s", &Ctrl->S.confidence, txt_b);
					/* confidence scaling */
					Ctrl->S.conrad = sqrt (-2.0 * log (1.0 - Ctrl->S.confidence));
					if (txt_b[0]) Ctrl->S.fontsize = GMT_convert_units (GMT, txt_b, GMT_PT, GMT_PT);
				}
				if (opt->arg[0] == 'n' || opt->arg[0] == 'x' ) Ctrl->S.scale = GMT_to_inch (GMT, &opt->arg[1]);
				if (opt->arg[0] == 'w' && strlen(opt->arg) > 3) {
					strcpy(txt, &opt->arg[1]);
					n=0; while (txt[n] && txt[n] != '/') n++; txt[n]=0;
					Ctrl->S.scale = GMT_to_inch (GMT, txt);
					sscanf(strchr(&opt->arg[1],'/')+1, "%lf", &Ctrl->S.wedge_amp);
				}
				switch (opt->arg[0]) {
					case 'e':
						Ctrl->S.symbol = CINE;
						Ctrl->S.readmode = READ_ELLIPSE;
						break;
					case 'r':
						Ctrl->S.symbol = CINE;
						Ctrl->S.readmode = READ_ROTELLIPSE;
						break;
					case 'n':
						Ctrl->S.symbol = ANISO;
						Ctrl->S.readmode = READ_ANISOTROPY;
						break;
					case 'w':
						Ctrl->S.symbol = WEDGE;
						Ctrl->S.readmode = READ_WEDGE;
						break;
					case 'x':
						Ctrl->S.symbol = CROSS;
						Ctrl->S.readmode = READ_CROSS;
						break;
					default:
						n_errors++;
						break;
				}
				break;
			case 'W':	/* Set line attributes */
				Ctrl->W.active = TRUE;
				if (opt->arg && GMT_getpen (GMT, opt->arg, &Ctrl->W.pen)) {
					GMT_pen_syntax (GMT, 'W', " ");
					n_errors++;
				}
				break;

			/* Illegal options */

		}
	}

	no_size_needed = (Ctrl->S.readmode == READ_ELLIPSE || Ctrl->S.readmode == READ_ROTELLIPSE || Ctrl->S.readmode == READ_ANISOTROPY || Ctrl->S.readmode == READ_CROSS || Ctrl->S.readmode == READ_WEDGE );
        /* Only one allowed */
	n_set = (Ctrl->S.readmode == READ_ELLIPSE) + (Ctrl->S.readmode == READ_ROTELLIPSE) + (Ctrl->S.readmode == READ_ANISOTROPY) + (Ctrl->S.readmode == READ_CROSS) + (Ctrl->S.readmode == READ_WEDGE);
	n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error: Must specify -R option\n");
	n_errors += GMT_check_condition (GMT, n_set > 1, "Syntax error: Only one -S setting is allowed.\n");
	n_errors += GMT_check_condition (GMT, !no_size_needed && (Ctrl->S.symbol > 1 && Ctrl->S.scale <= 0.0), "Syntax error: Must specify symbol size.\n");
	n_errors += GMT_check_condition (GMT, Ctrl->D.active && ! (Ctrl->S.readmode == READ_ELLIPSE || Ctrl->S.readmode == READ_WEDGE), "Syntax error: -D requres -Se|w.\n");

	if (!got_A && Ctrl->W.active) Ctrl->A.S.v.pen = Ctrl->W.pen;	/* Set vector pen to that given by -W  */
	if (Ctrl->A.S.v.status & GMT_VEC_OUTLINE2 && Ctrl->W.active) GMT_rgb_copy (Ctrl->A.S.v.pen.rgb, Ctrl->W.pen.rgb);	/* Set vector pen color from -W but not thickness */
	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_psvelo_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

GMT_LONG GMT_psvelo (struct GMTAPI_CTRL *API, GMT_LONG mode, void *args)
{
	GMT_LONG ix = 0, iy = 1, n_rec = 0, error = FALSE, old_is_world, justify;
	GMT_LONG greenwich, des_ellipse = TRUE, des_arrow = TRUE;

	double xy[2], plot_x, plot_y, vxy[2], plot_vx, plot_vy, dim[7];
	double eps1 = 0.0, eps2 = 0.0, spin = 0.0, spinsig = 0.0, theta = 0.0;
	double direction = 0, small_axis = 0, great_axis = 0, sigma_x, sigma_y, corr_xy;
	double t11 = 1.0, t12 = 0.0, t21 = 0.0, t22 = 1.0, hl, hw, vw, ssize;

	char *station_name;
	char *line, col[12][GMT_TEXT_LEN64];

	struct PSVELO_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;		/* General GMT interal parameters */
	struct GMT_OPTION *options = NULL;
	struct PSL_CTRL *PSL = NULL;		/* General PSL interal parameters */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_psvelo_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_psvelo_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments; return if errors are encountered */

	GMT = GMT_begin_module (API, "GMT_psvelo", &GMT_cpy);	/* Save current state */
	if (GMT_Parse_Common (API, "-VJR:", "BHKOPUVXhixYyc>", options)) Return (API->error);
	Ctrl = New_psvelo_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_psvelo_parse (API, Ctrl, options))) Return (error);
 	PSL = GMT->PSL;		/* This module also needs PSL */

	/*---------------------------- This is the psvelo main code ----------------------------*/

	greenwich = (GMT->common.R.wesn[XLO] < 0.0 || GMT->common.R.wesn[XHI] <= 0.0);

	if (GMT_err_pass (GMT, GMT_map_setup (GMT, GMT->common.R.wesn), "")) Return (GMT_RUNTIME_ERROR);

	GMT_plotinit (GMT, options);
	GMT_plotcanvas (GMT);	/* Fill canvas if requested */

	GMT_setpen (GMT, &Ctrl->W.pen);
	PSL_setfont (PSL, GMT->current.setting.font_annot[0].id);
	if (Ctrl->E.active) Ctrl->L.active = TRUE;

	if (!Ctrl->N.active) GMT_map_clip_on (GMT, GMT->session.no_rgb, 3);
	GMT_init_vector_param (GMT, &Ctrl->A.S);

	old_is_world = GMT->current.map.is_world;
	station_name = GMT_memory (GMT, NULL, 64, char);

	ix = (GMT->current.setting.io_lonlat_toggle[0]);	iy = 1 - ix;

	if (GMT_Init_IO (API, GMT_IS_TEXTSET, GMT_IS_POINT, GMT_IN, GMT_REG_DEFAULT, options) != GMT_OK) {	/* Register data input */
		Return (API->error);
	}
	if (GMT_Begin_IO (API, GMT_IS_TEXTSET, GMT_IN) != GMT_OK) {	/* Enables data input and sets access mode */
		Return (API->error);
	}

	if (Ctrl->S.readmode == READ_ELLIPSE || Ctrl->S.readmode == READ_ROTELLIPSE) GMT_report (GMT, GMT_MSG_NORMAL, "psvelo: 2-D confidence interval and scaling factor %f %f\n", Ctrl->S.confidence, Ctrl->S.conrad);

	Ctrl->A.S.v.v_width = Ctrl->A.S.v.pen.width * GMT->session.u2u[GMT_PT][GMT_INCH];

	do {	/* Keep returning records until we reach EOF */
		if ((line = GMT_Get_Record (API, GMT_READ_TEXT, NULL)) == NULL) {	/* Read next record, get NULL if special case */
			if (GMT_REC_IS_ERROR (GMT)) 		/* Bail if there are any read errors */
				Return (GMT_RUNTIME_ERROR);
			if (GMT_REC_IS_ANY_HEADER (GMT)) 	/* Skip all table and segment headers */
				continue;
			if (GMT_REC_IS_EOF (GMT)) 		/* Reached end of file */
				break;
		}

		/* Data record to process */

		n_rec++;
		if (Ctrl->S.readmode == READ_ELLIPSE || Ctrl->S.readmode == READ_ROTELLIPSE) {
			sscanf (line, "%s %s %s %s %s %s %s %[^\n]\n",
				col[0], col[1], col[2], col[3], col[4], col[5], col[6], station_name);
		}
		else {
			sscanf (line, "%s %s %s %s %s %s %s %s %s %[^\n]\n",
				col[0], col[1], col[2], col[3], col[4], col[5], col[6], col[7], col[8], station_name);
		}

		if ((GMT_scanf (GMT, col[GMT_X], GMT->current.io.col_type[GMT_IN][GMT_X], &xy[ix]) == GMT_IS_NAN) || (GMT_scanf (GMT, col[GMT_Y], GMT->current.io.col_type[GMT_IN][GMT_Y], &xy[iy]) == GMT_IS_NAN)) {
			GMT_report (GMT, GMT_MSG_FATAL, "Record %ld had bad x and/or y coordinates, must exit)\n", n_rec);
			GMT_exit (EXIT_FAILURE);
		}

		if (Ctrl->S.readmode == READ_ELLIPSE) {
			vxy[ix] = atof (col[2]);
			vxy[iy] = atof (col[3]);
			sigma_x = atof (col[4]);
			sigma_y = atof (col[5]);
			corr_xy = atof (col[6]);
			/* rescale uncertainties if necessary */
			if (Ctrl->D.active) {
				sigma_x = Ctrl->D.scale * sigma_x;
				sigma_y = Ctrl->D.scale * sigma_y;
			}
			if (fabs (sigma_x) < EPSIL && fabs (sigma_y) < EPSIL)
				des_ellipse = FALSE;
			else {
				des_ellipse = TRUE;
				ellipse_convert (sigma_x, sigma_y, corr_xy, Ctrl->S.conrad, &small_axis, &great_axis, &direction);

				/* convert to degrees */
				direction = direction * R2D;
			}
		}
		else if (Ctrl->S.readmode == READ_ROTELLIPSE) {
			vxy[ix] = atof (col[2]);
			vxy[iy] = atof (col[3]);
			great_axis = Ctrl->S.conrad*atof (col[4]);
			small_axis = Ctrl->S.conrad*atof (col[5]);
			direction = atof (col[6]);
			if (fabs (great_axis) < EPSIL && fabs (small_axis) < EPSIL)
				des_ellipse = FALSE;
			else
				des_ellipse = TRUE;
		}
		else if (Ctrl->S.readmode == READ_ANISOTROPY) {
			vxy[ix] = atof (col[2]);
			vxy[iy] = atof (col[3]);
		}
		else if (Ctrl->S.readmode == READ_CROSS) {
			eps1 = atof (col[2]);
			eps2 = atof (col[3]);
			theta = atof (col[4]);
		}
		else if (Ctrl->S.readmode == READ_WEDGE) {
			spin = atof (col[2]);
			spinsig = atof (col[3]);
			if (Ctrl->D.active) spinsig = spinsig * Ctrl->D.scale;
		}

		GMT_map_outside (GMT, xy[GMT_X], xy[GMT_Y]);
		if (GMT_abs (GMT->current.map.this_x_status) > 1 || GMT_abs (GMT->current.map.this_y_status) > 1) continue;

		GMT_geo_to_xy (GMT, xy[GMT_X], xy[GMT_Y], &plot_x, &plot_y);

		switch (Ctrl->S.symbol) {
			case CINE:
				des_arrow = hypot (vxy[0], vxy[1]) < 1.e-8 ? FALSE : TRUE;
				trace_arrow (GMT, xy[GMT_X], xy[GMT_Y], vxy[0], vxy[1], Ctrl->S.scale, &plot_x, &plot_y, &plot_vx, &plot_vy);
				get_trans (GMT, xy[GMT_X], xy[GMT_Y], &t11, &t12, &t21, &t22);
				if (des_ellipse) {
					if (Ctrl->E.active)
						paint_ellipse (PSL, plot_vx, plot_vy, direction, great_axis, small_axis, Ctrl->S.scale,
							t11,t12,t21,t22, Ctrl->E.active, Ctrl->E.fill.rgb, Ctrl->L.active);
					else
						paint_ellipse (PSL, plot_vx, plot_vy, direction, great_axis, small_axis, Ctrl->S.scale,
							t11,t12,t21,t22, Ctrl->E.active, Ctrl->G.fill.rgb, Ctrl->L.active);
				}
				if (des_arrow) {	/* verify that arrow is not ridiculously small */
					if (hypot (plot_x-plot_vx, plot_y-plot_vy) <= 1.5 * Ctrl->A.S.v.h_length) {
						hl = hypot (plot_x-plot_vx,plot_y-plot_vy) * 0.6;
						hw = hl * Ctrl->A.S.v.h_width/Ctrl->A.S.v.h_length;
						vw = hl * Ctrl->A.S.v.v_width/Ctrl->A.S.v.h_length;
						if (vw < 2.0/PSL_DOTS_PER_INCH) vw = 2./PSL_DOTS_PER_INCH;
					}
					else {
						hw = Ctrl->A.S.v.h_width;
						hl = Ctrl->A.S.v.h_length;
						vw = Ctrl->A.S.v.v_width;
					}
					dim[0] = plot_vx, dim[1] = plot_vy;
					dim[2] = vw, dim[3] = hl, dim[4] = hw;
					dim[5] = GMT->current.setting.map_vector_shape, dim[6] = Ctrl->A.S.v.status;
					GMT_setfill (GMT, &Ctrl->G.fill, Ctrl->L.active);
					if (Ctrl->A.S.v.status & GMT_VEC_OUTLINE2) GMT_setpen (GMT, &Ctrl->A.S.v.pen);
					PSL_plotsymbol (PSL, plot_x, plot_y, dim, PSL_VECTOR);
					if (Ctrl->A.S.v.status & GMT_VEC_OUTLINE2) GMT_setpen (GMT, &Ctrl->W.pen);
					
					justify = plot_vx - plot_x > 0. ? PSL_MR : PSL_ML;
					if (Ctrl->S.fontsize > 0.0 && strlen(station_name) > 0)	/* 1 inch = 2.54 cm */
						PSL_plottext (PSL, plot_x + (6 - justify) / 25.4 , plot_y, Ctrl->S.fontsize, station_name, ANGLE, justify, FORM);
				}
				else {
					GMT_setfill (GMT, &Ctrl->G.fill, 1);
					ssize = GMT_DOT_SIZE;
					PSL_plotsymbol (PSL, plot_x, plot_y, &ssize, GMT_SYMBOL_CIRCLE);
					justify = PSL_TC;
					if (Ctrl->S.fontsize > 0.0 && strlen (station_name) > 0) {
						PSL_plottext (PSL, plot_x, plot_y - 1. / 25.4, Ctrl->S.fontsize, station_name, ANGLE, justify, FORM);
					}
					/*  1 inch = 2.54 cm */
				}
				break;
			case ANISO:
				trace_arrow (GMT, xy[GMT_X], xy[GMT_Y], vxy[0], vxy[1], Ctrl->S.scale, &plot_x, &plot_y, &plot_vx, &plot_vy);
				PSL_plotsegment (PSL, plot_x, plot_y, plot_vx, plot_vy);
				break;
			case CROSS:
				/* triangular arrowheads */
				trace_cross (GMT, xy[GMT_X],xy[GMT_Y],eps1,eps2,theta,Ctrl->S.scale,Ctrl->A.S.v.v_width,Ctrl->A.S.v.h_length,Ctrl->A.S.v.h_width,0.1,Ctrl->L.active,Ctrl->W.pen);
				break;
			case WEDGE:
				PSL_comment (PSL, "begin wedge number %li", n_rec);
				GMT_geo_to_xy (GMT, xy[GMT_X], xy[GMT_Y], &plot_x, &plot_y);
				get_trans (GMT, xy[GMT_X], xy[GMT_Y], &t11, &t12, &t21, &t22);
				paint_wedge (PSL, plot_x, plot_y, spin, spinsig, Ctrl->S.scale, Ctrl->S.wedge_amp, t11,t12,t21,t22,
					Ctrl->G.active, Ctrl->G.fill.rgb, Ctrl->E.active, Ctrl->E.fill.rgb, Ctrl->L.active);
				break;
		}
	} while (TRUE);
	
	if (GMT_End_IO (API, GMT_IN, 0) != GMT_OK) {	/* Disables further data input */
		Return (API->error);
	}

	GMT_free (GMT, station_name);

	GMT_report (GMT, GMT_MSG_NORMAL, "Number of records read: %li\n", n_rec);

	if (Ctrl->D.active)  GMT_report (GMT, GMT_MSG_NORMAL, "Rescaling uncertainties by a factor of %f\n", Ctrl->D.scale);

	if (!Ctrl->N.active) GMT_map_clip_off (GMT);

	if (Ctrl->W.pen.style) PSL_setdash (PSL, CNULL, 0);

	GMT_map_basemap (GMT);

	GMT_plotend (GMT);

	Return (GMT_OK);
}
