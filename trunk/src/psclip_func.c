/*--------------------------------------------------------------------
 *	$Id: psclip_func.c,v 1.10 2011-04-12 03:05:18 remko Exp $
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
 * Brief synopsis: psclip reads one or many xy-files and draws polygons just like psclip
 * with the exception that these polygons are set up to act as clip
 * paths for subsequent plotting.  psclip uses the even-odd winding
 * rule to decide what's inside and outside the path.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#include "pslib.h"
#include "gmt.h"

#define CLIP_POL	0	/* Terminate polygon clipping */
#define CLIP_TXT	1	/* Terminate textbased clipping */
#define CLIP_STEXT	2	/* Undo one level of textclipping and plot current straight text */
#define CLIP_CTEXT	3	/* Undo one level of textclipping and plot current curved text */

struct PSCLIP_CTRL {
	struct C {	/* -C */
		GMT_LONG active;
		GMT_LONG mode;	/* 1 if we are to plot text previously used to set a straight clip path; 2 for curved */
		GMT_LONG n;	/* Number of levels to undo [1] */
	} C;
	struct N {	/* -N */
		GMT_LONG active;
	} N;
	struct T {	/* -T */
		GMT_LONG active;
	} T;
};

void *New_psclip_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct PSCLIP_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct PSCLIP_CTRL);
	C->C.n = 1;	/* Default undoes one level of clipping */

	return ((void *)C);
}

void Free_psclip_Ctrl (struct GMT_CTRL *GMT, struct PSCLIP_CTRL *C) {	/* Deallocate control structure */
	GMT_free (GMT, C);
}

GMT_LONG GMT_psclip_usage (struct GMTAPI_CTRL *C, GMT_LONG level)
{
	struct GMT_CTRL *GMT = C->GMT;

	/* This displays the psclip synopsis and optionally full usage information */

	GMT_message (GMT, "psclip %s [API] - To set up polygonal clip paths\n\n", GMT_VERSION);
	GMT_message (GMT, "usage: psclip -C[s|c|t[<n>|a]|p[<n>|a]] [-K] [-O]  OR\n");
	GMT_message (GMT, "\tpsclip <xy-files> %s %s [%s]\n", GMT_J_OPT, GMT_Rgeoz_OPT, GMT_B_OPT);
	GMT_message (GMT, "\t[%s] [-K] [-N] [-O] [-P] [-T] [%s] [%s]\n", GMT_Jz_OPT, GMT_U_OPT, GMT_V_OPT);
	GMT_message (GMT, "\t[%s] [%s] [%s] [%s]\n", GMT_X_OPT, GMT_Y_OPT, GMT_bi_OPT, GMT_c_OPT);
	GMT_message (GMT, "\t[%s] [%s] [%s] [%s] [%s] [%s] [%s]\n\n", GMT_f_OPT, GMT_g_OPT, GMT_h_OPT, GMT_i_OPT, GMT_p_OPT, GMT_t_OPT, GMT_colon_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t-C Means undo existing clip-path; no file is needed.  -R, -J are not required (unless -B is used).\n");
	GMT_message (GMT, "\t   Append p to terminate polygon clipping; append how many clip levels to restore or a for all [1].\n");
	GMT_message (GMT, "\t   Append t to terminate text clipping; append how many clip levels to restore or a for all [1].\n");
	GMT_message (GMT, "\t   Append c to plot text previously used to build a curved clip path set (restores 1 level).\n");
	GMT_message (GMT, "\t   Append s to plot text previously used to build a straight-text clip path set (restores 1 level).\n");
	GMT_message (GMT, "\t<xy-files> is one or more polygon files.  If none, standard input is read\n");
	GMT_explain_options (GMT, "jR");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "bZK");
	GMT_message (GMT, "\t-N Use the outside of the polygons and the map boundary as clip paths\n");
	GMT_explain_options (GMT, "OP");
	GMT_message (GMT, "\t-T Set clip path for the entire map frame.  No input file is required\n");
	GMT_explain_options (GMT, "UVXC2cfghipt:.");
	
	return (EXIT_FAILURE);
}

GMT_LONG GMT_psclip_parse (struct GMTAPI_CTRL *C, struct PSCLIP_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to psclip and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	GMT_LONG n_errors = 0, n_files = 0, k = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {

			case '<':	/* Input files */
				n_files++;
				break;

			/* Processes program-specific parameters */

			case 'C':	/* Turn clipping off */
				Ctrl->C.active = TRUE;
				if (opt->arg[0] == 'p') { k = 1; Ctrl->C.mode = CLIP_POL; }	/* Default anyway */
				if (opt->arg[0] == 's') { k = 1; Ctrl->C.mode = CLIP_STEXT; }
				if (opt->arg[0] == 'c') { k = 1; Ctrl->C.mode = CLIP_CTEXT; }
				if (opt->arg[0] == 't') { k = 1; Ctrl->C.mode = CLIP_TXT; }
				if (opt->arg[k]) {	/* Gave argument for how many levels */
					if (isdigit ((int)opt->arg[k])) Ctrl->C.n = atoi (&opt->arg[k]);
					else if (opt->arg[k] == 'a') Ctrl->C.n = (Ctrl->C.mode == CLIP_POL) ? PSL_ALL_CLIP_POL : PSL_ALL_CLIP_TXT;
					else {
						GMT_report (GMT, GMT_MSG_FATAL, "Syntax error -C:  Correct syntax is -C[s|c|t|p[<n>|a]]\n");
						n_errors++;
					}
				}
				break;
			case 'N':	/* Use the outside of the polygons as clip area */
				Ctrl->N.active = TRUE;
				break;
			case 'T':	/* Select map clip */
				Ctrl->T.active = TRUE;
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	if (!Ctrl->C.active) {
		n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error:  Must specify -R option\n");
		n_errors += GMT_check_condition (GMT, !GMT->common.J.active, "Syntax error:  Must specify a map projection with the -J option\n");
	}
	if (Ctrl->T.active) Ctrl->N.active = TRUE;	/* -T implies -N */
	if (Ctrl->T.active && n_files) GMT_report (GMT, GMT_MSG_FATAL, "Warning: Option -T ignores all input files\n");

	if (Ctrl->N.active && GMT->current.map.frame.plot) {
		GMT_report (GMT, GMT_MSG_FATAL, "Warning: Option -B cannot be used in combination with Options -N or -T. -B is ignored.\n");
		GMT->current.map.frame.plot = FALSE;
	}

	n_errors += GMT_check_binary_io (GMT, 2);

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

void gmt_terminate_clipping (struct GMT_CTRL *C, struct PSL_CTRL *PSL, GMT_LONG mode, GMT_LONG n)
{
	if (mode == CLIP_POL) {
		PSL_endclipping (PSL, n);	/* Reduce polygon clipping by n levels [1] */
		if (n == PSL_ALL_CLIP_POL)
			GMT_report (C, GMT_MSG_NORMAL, "Restore ALL polygon clip levels\n");
		else
			GMT_report (C, GMT_MSG_NORMAL, "Restore %ld polygon clip levels\n", n);
	}
	else if (mode == CLIP_TXT) {
		PSL_endclipping (PSL, -n);	/* Reduce polygon clipping by n levels [1] */
		if (n == PSL_ALL_CLIP_TXT)
			GMT_report (C, GMT_MSG_NORMAL, "Restore ALL text clip levels\n");
		else
			GMT_report (C, GMT_MSG_NORMAL, "Restore %ld text clip levels\n", n);
	}
	else if (mode == CLIP_STEXT) {
		PSL_endclipping (PSL, -1);	/* Undo last text clipping levels */
		PSL_plottextclip (PSL, NULL, NULL, 0, 0.0, NULL, NULL, 0, NULL, 9);	/* This lays down the straight text */
		GMT_report (C, GMT_MSG_NORMAL, "Restore 1 text clip level and place delayed straight text\n");
	}
	else if (mode == CLIP_CTEXT) {
		PSL_endclipping (PSL, -1);	/* Undo last text clipping levels */
		PSL_plottextpath (PSL, NULL, NULL, 0, NULL, 0.0, NULL, 0, NULL, 0, NULL, 8);	/* Lay down the curved text */
		GMT_report (C, GMT_MSG_NORMAL, "Restore 1 text clip level and place delayed curved text\n");
	}
}

#define Return(code) {Free_psclip_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); return (code);}

GMT_LONG GMT_psclip (struct GMTAPI_CTRL *API, struct GMT_OPTION *options)
{
	GMT_LONG error = FALSE;

	double x0, y0;

	struct PSCLIP_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;		/* General GMT interal parameters */
	struct PSL_CTRL *PSL = NULL;		/* General PSL interal parameters */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));

	if (!options || options->option == GMTAPI_OPT_USAGE) return (GMT_psclip_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) return (GMT_psclip_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments; return if errors are encountered */

	GMT = GMT_begin_module (API, "GMT_psclip", &GMT_cpy);	/* Save current state */
	if ((error = GMT_Parse_Common (API, "-VJRbf:", "BKOPUXxYycghipst>" GMT_OPT("EZMm"), options))) Return (error);
	Ctrl = (struct PSCLIP_CTRL *)New_psclip_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_psclip_parse (API, Ctrl, options))) Return (error);
	PSL = GMT->PSL;		/* This module also needs PSL */

	/*---------------------------- This is the psclip main code ----------------------------*/

	if (!GMT->current.proj.x_off_supplied && GMT->common.O.active) GMT->current.setting.map_origin[GMT_X] = 0.0;
	if (!GMT->current.proj.y_off_supplied && GMT->common.O.active) GMT->current.setting.map_origin[GMT_Y] = 0.0;

	if (Ctrl->C.mode == CLIP_POL) {	/* Only keep track of polygon clip levels */
		if (Ctrl->C.active)
			GMT->current.ps.clip = -Ctrl->C.n;	/* Program terminates n levels of prior clipping */
		else
			GMT->current.ps.clip = +1;		/* Program adds one new level of clipping */
	}

	GMT_plotinit (API, PSL, options);

	if (Ctrl->C.active && !GMT->current.map.frame.plot) {	/* Just undo previous clip-path(s), no basemap needed so no -R -J parsing */
		gmt_terminate_clipping (GMT, PSL, Ctrl->C.mode, Ctrl->C.n);
		GMT_plotend (GMT, PSL);
		GMT_report (GMT, GMT_MSG_NORMAL, "Done!\n");

		Return (EXIT_SUCCESS);
	}

	/* Here we have -R -J etc to deal with */
	
	if (GMT_err_pass (GMT, GMT_map_setup (GMT, GMT->common.R.wesn), "")) Return (GMT_RUNTIME_ERROR);

	GMT_plane_perspective (GMT, PSL, GMT->current.proj.z_project.view_plane, GMT->current.proj.z_level);

	if (Ctrl->C.active) {	/* Undo previous clip-path(s) and draw basemap */
		gmt_terminate_clipping (GMT, PSL, Ctrl->C.mode, Ctrl->C.n);
		GMT_map_basemap (GMT, PSL);
	}
	else {	/* Start new clip_path */
		GMT_LONG tbl, seg, i, first = !Ctrl->N.active;
		double *x = NULL, *y = NULL;
		struct GMT_DATASET *D = NULL;
		struct GMT_LINE_SEGMENT *S = NULL;
		GMT_map_basemap (GMT, PSL);

		if (Ctrl->N.active) GMT_map_clip_on (GMT, PSL, GMT->session.no_rgb, 1);	/* Must clip map */

		if (!Ctrl->T.active) {
			if ((error = GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POLY, GMT_IN, GMT_REG_DEFAULT, options))) Return (error);	/* Register data input */
			if ((error = GMT_Begin_IO (API, GMT_IS_DATASET, GMT_IN, GMT_BY_SET))) Return (error);				/* Enables data input and sets access mode */
			if ((error = GMT_Get_Data (API, GMT_IS_DATASET, GMT_IS_FILE, 0, NULL, 0, NULL, (void **)&D))) Return (error);

			for (tbl = 0; tbl < D->n_tables; tbl++) {
				for (seg = 0; seg < D->table[tbl]->n_segments; seg++) {	/* For each segment in the table */
					S = D->table[tbl]->segment[seg];		/* Current segment */
					if (D->alloc_mode == GMT_READONLY) {	/* Cannot store results in the read-only input array */
						x = GMT_memory (GMT, NULL, S->n_rows, double);
						y = GMT_memory (GMT, NULL, S->n_rows, double);
					}
					else {	/* Reuse input arrays */
						x = S->coord[GMT_X];	y = S->coord[GMT_Y];	/* Short hand for x,y columns */
					}

					for (i = 0; i < S->n_rows; i++) {
						GMT_geo_to_xy (GMT, S->coord[GMT_X][i], S->coord[GMT_Y][i], &x0, &y0);
						x[i] = x0; y[i] = y0;
					}
					PSL_beginclipping (PSL, x, y, S->n_rows, GMT->session.no_rgb, first);
					first = 0;
					if (D->alloc_mode == GMT_READONLY) {	/* Free temp arrays */
						GMT_free (GMT, x);	GMT_free (GMT, y);
					}
				}
			}
			if ((error = GMT_End_IO (API, GMT_IN, 0))) Return (error);	/* Disables further data input */
			GMT_Destroy_Data (API, GMT_ALLOCATED, (void **)&D);
		}
		/* Finalize the composite polygon clip path */
		PSL_beginclipping (PSL, NULL, NULL, (GMT_LONG)0, GMT->session.no_rgb, 2 + first);
	}

	GMT_plane_perspective (GMT, PSL, -1, 0.0);
	GMT_plotend (GMT, PSL);

	GMT_report (GMT, GMT_MSG_NORMAL, "Done!\n");

	Return (GMT_OK);
}
