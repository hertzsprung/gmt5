/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
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
 * Brief synopsis: psmask tries to achieve masking using one of two different approaches:
 * The default way of operation is: Instead of painting tiles where there's no
 * data [or where there is data] psmask uses contouring to find the polygons
 * that contain the data [or contain the regions with no data].  For many types
 * of data coverage, this results in a manageable path instead of thousands of
 * tiles.  So, instead of painting polygons, psmask sets up actual clip paths.
 * As an option, the user may specify a rgb combination to fill the clipped areas.
 * To avoid having to deal with the problems that arise if the data distribution
 * is such that part of the map boundary should be part of the clip paths, we
 * internally enlarge the grid by one gridsize unit so no nodes along the edges
 * have data.  Before using the clippaths we move points outside the real region
 * onto the boundary.
 * Should the paths be too long for PostScript to handle, the user may override
 * the default with the -T switch.  Then, masking is achieved using tiling
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#include "pslib.h"
#include "gmt.h"

struct PSMASK_CTRL {
	struct C {	/* -C */
		BOOLEAN active;
	} C;
	struct D {	/* -D<dumpfile> */
		BOOLEAN active;
		char *file;
#ifdef DEBUG
		BOOLEAN debug;
#endif
	} D;
	struct F {	/* -F */
		BOOLEAN active;
	} F;
	struct G {	/* -G<fill> */
		BOOLEAN active;
		struct GMT_FILL fill;
	} G;
	struct I {	/* -Idx[/dy] */
		BOOLEAN active;
		double inc[2];
	} I;
	struct N {	/* -N */
		BOOLEAN active;
	} N;
	struct Q {	/* -Q<cut> */
		BOOLEAN active;
		COUNTER_MEDIUM min;
	} Q;
	struct S {	/* -S[-|=|+]<radius>[d|e|f|k|m|M|n|s] */
		BOOLEAN active;
		GMT_LONG mode;	/* May be negative */
		double radius;
		char unit;
	} S;
	struct T {	/* -T */
		BOOLEAN active;
	} T;
};

struct PSMASK_INFO {
	GMT_LONG first_dump;
	GMT_LONG p[5], i_off[5], j_off[5], k_off[5], offset;
	unsigned int bit[32];
};

void draw_clip_contours (struct GMT_CTRL *GMT, struct PSL_CTRL *PSL, double *xx, double *yy, GMT_LONG nn, double rgb[], GMT_LONG id, GMT_LONG flag)
{
	GMT_LONG i;
	double x, y;

	if (nn < 2 && flag < 2) return;

	for (i = 0; i < nn; i++) {
		x = xx[i];	y = yy[i];
		GMT_geo_to_xy (GMT, x, y, &xx[i], &yy[i]);
	}
	nn = GMT_compact_line (GMT, xx, yy, nn, FALSE, 0);

	if (nn > 0) PSL_comment (PSL, "Start of clip path sub-segment %d\n", id);
	PSL_beginclipping (PSL, xx, yy, nn, rgb, flag);
	if (nn > 0) PSL_comment (PSL, "End of clip path sub-segment %d\n", id);
}

GMT_LONG trace_clip_contours (struct GMT_CTRL *GMT, struct PSMASK_INFO *info, char *grd, COUNTER_MEDIUM *edge, struct GRD_HEADER *h, double inc2[], double **xx, double **yy, GMT_LONG i, GMT_LONG j, GMT_LONG kk, COUNTER_LARGE *max)
{
	GMT_LONG k, k0, n_cuts, kk_opposite, first_k, more;
	COUNTER_LARGE n = 1, edge_word, edge_bit, ij, ij0, m;
	double xk[4], yk[4], x0, y0;

	m = *max - 2;
	
	more = TRUE;
	do {
		ij = GMT_IJP (h, j, i);
		x0 = GMT_grd_col_to_x (GMT, i, h);
		y0 = GMT_grd_row_to_y (GMT, j, h);
		n_cuts = 0;
		k0 = kk;

		for (k = 0; k < 4; k++) {	/* Loop over box sides */

			/* Skip where we already have a cut (k == k0) */

			if (k == k0) continue;

			/* Skip edge already has been used */

			ij0 = GMT_IJ0 (h, j + info->j_off[k], i + info->i_off[k]);
			edge_word = ij0 / 32 + info->k_off[k] * info->offset;
			edge_bit = ij0 % 32;
			if (edge[edge_word] & info->bit[edge_bit]) continue;

			/* Skip if no zero-crossing on this edge */

			if ((grd[ij+info->p[k+1]] + grd[ij+info->p[k]]) != 1) continue;

			/* Here we have a crossing */

			if (k%2) {	/* Cutting a S-N line */
				if (k == 1) {
					xk[1] = x0 + h->inc[GMT_X];
					yk[1] = y0 + 0.5*h->inc[GMT_Y];
				}
				else {
					xk[3] = x0;
					yk[3] = y0 + 0.5*h->inc[GMT_Y];
				}
			}
			else {	/* Cutting a E-W line */
				if (k == 0) {
					xk[0] = x0 + 0.5*h->inc[GMT_X];
					yk[0] = y0;
				}
				else {
					xk[2] = x0 + 0.5*h->inc[GMT_X];
					yk[2] = y0 + h->inc[GMT_Y];
				}
			}
			kk = k;
			n_cuts++;
		}

		if (n > m) {	/* Must try to allocate more memory */
			*max = (*max == 0) ? GMT_CHUNK : ((*max) << 1);
			m = (m == 0) ? GMT_CHUNK : (m << 1);
			*xx = GMT_memory (GMT, *xx, *max, double);
			*yy = GMT_memory (GMT, *yy, *max, double);
		}
		if (n_cuts == 0) {	/* Close interior contour and return */
			/* if (fmod ((*xx[0] - inc2[GMT_X]), h->inc[GMT_X]) == 0.0) */	/* On side 1 or 3 */
			if (GMT_IS_ZERO (fmod ((*xx[0] - inc2[GMT_X]), h->inc[GMT_X])))	/* On side 1 or 3 */
				/* first_k = ((*xx[0] - x0) == 0.0) ? 3 : 1; */
				first_k = doubleAlmostEqualZero (*xx[0], x0) ? 3 : 1;
			else 	/* On side 0 or 2 */
				/* first_k = ((*yy[0] - y0) == 0.0) ? 0 : 2; */
				first_k = doubleAlmostEqualZero (*yy[0], y0) ? 0 : 2;
			kk_opposite = (first_k + 2) % 4;
			if (k0 != kk_opposite) {
				(*xx)[n] = x0 + 0.5*h->inc[GMT_X];
				(*yy)[n] = y0 + 0.5*h->inc[GMT_Y];
				n++;
			}
			(*xx)[n] = (*xx)[0];
			(*yy)[n] = (*yy)[0];
			n++;
			more = FALSE;
		}
		else if (n_cuts == 1) {	/* Draw a line to this point and keep tracing */
			/* Add center of box if this and previous cut NOT on opposite edges */
			kk_opposite = (k0 + 2) % 4;
			if (kk != kk_opposite) {
				(*xx)[n] = x0 + 0.5*h->inc[GMT_X];
				(*yy)[n] = y0 + 0.5*h->inc[GMT_Y];
				n++;
			}
			(*xx)[n] = xk[kk];
			(*yy)[n] = yk[kk];
			n++;
		}
		else {	/* Saddle point, we decide to connect to the point nearest previous point */
			kk = (k0 + 1)%4;	/* Pick next edge since it is arbitrarily where we go */
			/* First add center of box */
			(*xx)[n] = x0 + 0.5*h->inc[GMT_X];
			(*yy)[n] = y0 + 0.5*h->inc[GMT_Y];
			n++;
			(*xx)[n] = xk[kk];
			(*yy)[n] = yk[kk];
			n++;
		}
		if (more) {	/* Mark this edge as used */
			ij0 = GMT_IJ0 (h, j + info->j_off[kk], i + info->i_off[kk]);
			edge_word = ij0 / 32 + info->k_off[kk] * info->offset;
			edge_bit = ij0 % 32;
			edge[edge_word] |= info->bit[edge_bit];
		}

		/* Get next box (i,j,kk) */

		i -= (kk-2)%2;
		j -= (kk-1)%2;
		kk = (kk+2)%4;

	} while (more);
	return (n);
}

GMT_LONG clip_contours (struct GMT_CTRL *GMT, struct PSMASK_INFO *info, char *grd, struct GRD_HEADER *h, double inc2[], COUNTER_MEDIUM *edge, GMT_LONG first, double **x, double **y, COUNTER_LARGE *max)
{
	/* The routine finds the zero-contour in the grd dataset.  it assumes that
	 * no node has a value exactly == 0.0.  If more than max points are found
	 * trace_clip_contours will try to allocate more memory in blocks of GMT_CHUNK points
	 */
	 
	COUNTER_MEDIUM n_edges, edge_word, edge_bit, i, j, n = 0;
	static COUNTER_MEDIUM i0, j0, side;
	COUNTER_LARGE ij;
	BOOLEAN go_on = TRUE;
	 
	 
	n_edges = h->ny * lrint (ceil (h->nx / 16.0));
	 
	 /* Reset edge-flags to zero, if necessary */
	 if (first) {
		info->offset = n_edges / 2;
	 	i0 = 0;	/* Begin with upper left bin which is i = 0 and j = 1 */
	 	j0 = 1;
		side = 4;	/* Vertical interior gridlines */
		info->p[0] = info->p[4] = 0;	info->p[1] = 1;	info->p[2] = 1 - h->nx;	info->p[3] = -h->nx;
		info->i_off[0] = info->i_off[2] = info->i_off[3] = info->i_off[4] = 0;	info->i_off[1] =  1;
		info->j_off[0] = info->j_off[1] = info->j_off[3] = info->j_off[4] = 0;	info->j_off[2] = -1;
		info->k_off[0] = info->k_off[2] = info->k_off[4] = 0;	info->k_off[1] = info->k_off[3] = 1;
		for (i = 1, info->bit[0] = 1; i < 32; i++) info->bit[i] = info->bit[i-1] << 1;
	 }

	/* Loop over interior boxes */

	if (side == 4) {
		for (j = j0; go_on && j < h->ny; j++) {
			ij = GMT_IJP (h, j, i0);
			for (i = i0; go_on && i < h->nx-1; i++, ij++) {	/* nx-1 since the last bin starts at nx-2 and ends at nx-1 */
				edge_word = ij / 32 + info->offset;
				edge_bit = ij % 32;
				if (!(edge[edge_word] & info->bit[edge_bit]) && ((grd[ij]+grd[ij-h->nx]) == 1)) { /* Start tracing contour */
					*x[0] = GMT_grd_col_to_x (GMT, i, h);
					*y[0] = GMT_grd_row_to_y (GMT, j, h);
					edge[edge_word] |= info->bit[edge_bit];
					n = trace_clip_contours (GMT, info, grd, edge, h, inc2, x, y, i, j, 3, max);
					go_on = FALSE;
					i0 = i + 1;
					j0 = j;	/* Return to finish this row later */
				}
			}
			if (go_on) i0 = 0;	/* Go to start of next row unless we found something */
		}
		if (n == 0) {
			side = 5;
			i0 = 0;
			j0 = 1;
		}
	}
	if (n == 0 && side == 5) {
		for (j = j0; go_on && j < h->ny; j++) {
			ij = GMT_IJP (h, j, i0);
			for (i = i0; go_on && i < h->nx-1; i++, ij++) {
				edge_word = ij / 32 + info->offset;
				edge_bit = ij % 32;
				if (!(edge[edge_word] & info->bit[edge_bit]) && ((grd[ij]+grd[ij+1]) == 1)) { /* Start tracing contour */
					*x[0] = GMT_grd_col_to_x (GMT, i, h);
					*y[0] = GMT_grd_row_to_y (GMT, j, h);
					edge[edge_word] |= info->bit[edge_bit];
					n = trace_clip_contours (GMT, info, grd, edge, h, inc2, x, y, i, j, 2, max);
					go_on = FALSE;
					i0 = i + 1;
					j0 = j;	/* Return to finish this row later */
				}
				if (go_on) i0 = 1;
			}
		}
	}	

	return (n);
}

void shrink_clip_contours (struct GMT_CTRL *GMT, double *x, double *y, GMT_LONG n, double w, double e)
{
	/* Moves outside points to boundary */
	GMT_LONG i;

	for (i = 0; i < n; i++) {
		if (x[i] < w)
			x[i] = w;
		if (x[i] > e)
			x[i] = e;
		if (y[i] < GMT->common.R.wesn[YLO])
			y[i] = GMT->common.R.wesn[YLO];
		if (y[i] > GMT->common.R.wesn[YHI])
			y[i] = GMT->common.R.wesn[YHI];
	}
}

void *New_psmask_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct PSMASK_CTRL *C = NULL;
	
	C = GMT_memory (GMT, NULL, 1, struct PSMASK_CTRL);
	
	/* Initialize values whose defaults are not 0/FALSE/NULL */
	GMT_init_fill (GMT, &C->G.fill, -1.0, -1.0, -1.0);
		
	return (C);
}

void Free_psmask_Ctrl (struct GMT_CTRL *GMT, struct PSMASK_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->D.file) free (C->D.file);	
	GMT_free (GMT, C);	
}

GMT_LONG GMT_psmask_usage (struct GMTAPI_CTRL *C, GMT_LONG level)
{
	struct GMT_CTRL *GMT = C->GMT;

	GMT_message (GMT, "psmask %s [API] - Use data tables to clip or mask map areas with no coverage\n\n", GMT_VERSION);
	GMT_message (GMT, "usage: psmask <table> %s %s %s\n", GMT_I_OPT, GMT_J_OPT, GMT_Rgeoz_OPT);
	GMT_message (GMT, "\t[%s] [-C] [-D<template>] [-G<fill>] [%s] [-K] [-N] [-O] [-P] [-Q<min>]\n", GMT_B_OPT, GMT_Jz_OPT);
	GMT_message (GMT, "\t[-S%s] [-T] [%s] [%s] [%s]\n", GMT_RADIUS_OPT, GMT_U_OPT, GMT_V_OPT, GMT_X_OPT);
	GMT_message (GMT, "\t[%s] [%s] [%s] [%s] [%s]\n", GMT_Y_OPT, GMT_b_OPT, GMT_c_OPT, GMT_h_OPT, GMT_i_OPT);
	GMT_message (GMT, "\t[%s] [%s] [%s] [%s]\n\n", GMT_p_OPT, GMT_r_OPT, GMT_t_OPT, GMT_colon_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_inc_syntax (GMT, 'I', 0);
	GMT_explain_options (GMT, "jR");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "<b");
	GMT_message (GMT, "\t-C Terminate existing clip-path.  No other options required.\n");
	GMT_message (GMT, "\t-D Dump clip polygons as data polygons; no plotting takes place.\n");
	GMT_message (GMT, "\t   Append filename template which may contain a C-format specifier.\n");
	GMT_message (GMT, "\t   If no filename template is given we write all polygons to stdout.\n");
	GMT_message (GMT, "\t   If filename has no specifiers then we write all polygons to a single file.\n");
	GMT_message (GMT, "\t   If an integer format (e.g., %%6.6d) is found we substitute a running segment count\n");
	GMT_message (GMT, "\t   and write all polygons to individual files; see manual page for more examples.\n");
	GMT_message (GMT, "\t   Cannot be used with -T; see -Q to eliminate small polygons.\n");
	GMT_fill_syntax (GMT, 'G', "Select fill color/pattern [Default is no fill].");
	GMT_explain_options (GMT, "ZK");
	GMT_message (GMT, "\t-N Invert the sense of the clipping [or tiling].\n");
	GMT_explain_options (GMT, "OP");
	GMT_message (GMT, "\t-Q Do not dump contours with less than <cut> points [Dump all contours].\n");
	GMT_message (GMT, "\t   Ignored unless -D is set.\n");
	GMT_dist_syntax (GMT, 'S', "Set search radius to identify inside points.");
	GMT_message (GMT, "\t   This means nodes inside circles of <radius> centered on the input\n");
	GMT_message (GMT, "\t   data points are considered to be reliable estimates of the surface.\n");
	GMT_message (GMT, "\t   Default is -S0, i.e., only the nearest node is considered reliable.\n");
	GMT_message (GMT, "\t-T Paint tiles [Default will trace data outline].\n");
	GMT_message (GMT, "\t   If set you must also specify a color/fill with -G.\n");
	GMT_explain_options (GMT, "UVXC2D0chipFt:.");
	
	return (EXIT_FAILURE);
}

GMT_LONG GMT_psmask_parse (struct GMTAPI_CTRL *C, struct PSMASK_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to psmask and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

#ifdef GMT_COMPAT
	GMT_LONG n_plus, k;
#endif
	GMT_LONG n_errors = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {

			case '<':	/* Skip input files */
				break;

			/* Processes program-specific parameters */

			case 'C':
				Ctrl->C.active = TRUE;
				break;
			case 'D':	/* Dump the polygons to files */
				Ctrl->D.active = TRUE;

#ifdef GMT_COMPAT
				for (n_plus = -1, k = 0; opt->arg[k]; k++) {
					if (opt->arg[k] == '+' && opt->arg[k+1] == 'n') {
						GMT_report (GMT, GMT_MSG_COMPAT, "Warning: Option -D..+n<min> is deprecated; use -Q instead.\n");
						Ctrl->Q.min = atoi (&opt->arg[k + 2]);
						Ctrl->Q.active = TRUE;
						n_plus = k;
						break;
					}
				}

				if (n_plus >= 0) opt->arg[n_plus] = '\0';	/* If extra option rip it before check if there is a prefix */
#endif
				Ctrl->D.file = strdup (opt->arg);
#ifdef GMT_COMPAT
				if (n_plus >= 0) opt->arg[n_plus] = '+';	/* Restore it */
#endif
				break;
			case 'F':
				Ctrl->F.active = TRUE;
				break;
			case 'G':
				Ctrl->G.active = TRUE;
				if (GMT_getfill (GMT, opt->arg, &Ctrl->G.fill)) {
					GMT_fill_syntax (GMT, 'G', " ");
					n_errors++;
				}
				break;
			case 'I':
				Ctrl->I.active = TRUE;
				if (GMT_getinc (GMT, opt->arg, Ctrl->I.inc)) {
					GMT_inc_syntax (GMT, 'I', 1);
					n_errors++;
				}
				break;
			case 'N':
				Ctrl->N.active = TRUE;
				break;
			case 'Q':
				Ctrl->Q.active = TRUE;
				Ctrl->Q.min = atoi (opt->arg);
				break;
			case 'S':	/* Radius of influence */
				Ctrl->S.active = TRUE;
				Ctrl->S.mode = GMT_get_distance (GMT, opt->arg, &(Ctrl->S.radius), &(Ctrl->S.unit));
				break;
			case 'T':
				Ctrl->T.active = TRUE;
				break;
#ifdef DEBUG
			case 'd':
				Ctrl->D.debug = TRUE;
				break;
#endif

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	GMT_check_lattice (GMT, Ctrl->I.inc, &Ctrl->F.active, &Ctrl->I.active);

	if (!Ctrl->C.active) {
		n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error: Must specify -R option\n");
		n_errors += GMT_check_condition (GMT, !GMT->common.J.active && !Ctrl->D.active, "Syntax error: Must specify a map projection with the -J option\n");
		n_errors += GMT_check_condition (GMT, !Ctrl->I.active, "Syntax error: Must specify -I option\n");
		n_errors += GMT_check_condition (GMT, Ctrl->T.active && !GMT_IS_RECT_GRATICULE(GMT), "Syntax error -T option: Only available with Linear, Mercator, or basic cylindrical projections\n");
		n_errors += GMT_check_condition (GMT, Ctrl->T.active && !(Ctrl->G.fill.rgb[0] >= 0 || Ctrl->G.fill.use_pattern), "Syntax error -T option: Must also specify a tile fill with -G\n");
		n_errors += GMT_check_condition (GMT, Ctrl->I.active && (Ctrl->I.inc[GMT_X] <= 0.0 || Ctrl->I.inc[GMT_Y] <= 0.0), "Syntax error -I option: Must specify positive increments\n");
		n_errors += GMT_check_condition (GMT, Ctrl->S.mode == -1, "Syntax error -S: Unrecognized unit\n");
		n_errors += GMT_check_condition (GMT, Ctrl->S.mode == -2, "Syntax error -S: Unable to decode radius\n");
		n_errors += GMT_check_condition (GMT, Ctrl->S.mode == -3, "Syntax error -S: Radius is negative\n");
		n_errors += GMT_check_condition (GMT, Ctrl->D.active && Ctrl->T.active, "Syntax error: -D cannot be used with -T\n");
		n_errors += GMT_check_binary_io (GMT, 2);
	}

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_psmask_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

GMT_LONG GMT_psmask (struct GMTAPI_CTRL *API, GMT_LONG mode, void *args)
{
	COUNTER_MEDIUM section, k, row, col, n_edges, *d_col = NULL, d_row = 0;
	COUNTER_MEDIUM io_mode = 0, max_d_col = 0, ii, jj, i_start, j_start;
	COUNTER_MEDIUM fmt[3] = {0, 0, 0}, cont_counts[2] = {0, 0}, *edge = NULL;
	BOOLEAN error = FALSE, first = TRUE, node_only, make_plot, closed;
	
	COUNTER_LARGE ij, n_points, n_seg = 0, n_read, n;
	
	size_t n_seg_alloc = 0;

	char *grd = NULL;

	double *in = NULL, distance, x0, y0;
	double inc2[2], *x = NULL, *y = NULL, *grd_x0 = NULL, *grd_y0 = NULL;

	struct GMT_DATASET *D = NULL;
	struct GMT_LINE_SEGMENT *S = NULL;
	struct GMT_GRID *Grid = NULL;
	struct PSMASK_INFO info;
	struct PSMASK_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;		/* General GMT interal parameters */
	struct GMT_OPTION *options = NULL;
	struct PSL_CTRL *PSL = NULL;		/* General PSL interal parameters */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_psmask_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_psmask_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments; return if errors are encountered */

	GMT = GMT_begin_module (API, "GMT_psmask", &GMT_cpy);	/* Save current state */
	if (GMT_Parse_Common (API, "-VJRb:", "BKOPUXxYychipst>" GMT_OPT("E"), options)) Return (API->error);
	Ctrl = New_psmask_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_psmask_parse (API, Ctrl, options))) Return (error);
	PSL = GMT->PSL;		/* This module also needs PSL */

	/*---------------------------- This is the psmask main code ----------------------------*/

	make_plot = !Ctrl->D.active;	/* Turn off plotting if -D was used */

	/* Now test also if we must provide a default -J */
	if (Ctrl->D.active && GMT->current.proj.projection < 0) {	/* Is this the right way of testing it? */
		GMT_parse_common_options (GMT, "J", 'J', "x1d");	/* Fake linear projection */
	}

	if (!Ctrl->C.active && make_plot && GMT_err_pass (GMT, GMT_map_setup (GMT, GMT->common.R.wesn), "")) Return (GMT_RUNTIME_ERROR);

	if (Ctrl->D.active) {	/* Want to dump the x-y contour lines of the mask */
		COUNTER_LARGE dim[4] = {1, 0, 2, 0};
		if (!Ctrl->D.file[0] || !strchr (Ctrl->D.file, '%'))	/* No file given or filename without C-format specifiers means a single output file */
			io_mode = GMT_WRITE_SET;
		else {	/* Must determine the kind of output organization */
			k = 0;
			while (Ctrl->D.file[k]) {
				if (Ctrl->D.file[k++] == '%') {	/* Start of format */
					while (Ctrl->D.file[k] && !strchr ("d", Ctrl->D.file[k])) k++;	/* Scan past any format modifiers, like in %4.4d */
					if (Ctrl->D.file[k] == 'd') fmt[1] = k;
					k++;
				}
			}
			if (fmt[1]) io_mode = GMT_WRITE_SEGMENTS;	/* d: Want individual files with running numbers */
		}
		if ((D = GMT_Create_Data (API, GMT_IS_DATASET, dim)) == NULL) Return (API->error);	/* An empty table */
		if ((error = GMT_set_cols (GMT, GMT_OUT, 2))) Return (error);
	}
	
	if (Ctrl->C.active)
		GMT->current.ps.nclip = -1;	/* Signal that this program terminates clipping that initiated prior to this process */
	else if (!Ctrl->T.active)
		GMT->current.ps.nclip = +1;	/* Signal that this program initiates clipping that will outlive this process */

	if (make_plot) GMT_plotinit (GMT, options);

	if (Ctrl->C.active) {	/* Just undo previous polygon clip-path */
		PSL_endclipping (PSL, 1);
		GMT_map_basemap (GMT);
		GMT_report (GMT, GMT_MSG_NORMAL, "clipping off!\n");
	}
	else {	/* Start new clip_path */
		GMT_memset (inc2, 2, double);
		GMT_memset (&info, 1, struct PSMASK_INFO);
		info.first_dump = TRUE;

		if ((Grid = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);
		GMT_setnval (GMT->current.io.pad, 4, 1);		/* Change default pad to 1 only */
		GMT_init_newgrid (GMT, Grid, GMT->common.R.wesn, Ctrl->I.inc, Ctrl->F.active);
		
		if (Ctrl->S.active) {	/* Need distance calculations in correct units, and the d_row/d_col machinery */
			GMT_init_distaz (GMT, Ctrl->S.unit, Ctrl->S.mode, GMT_MAP_DIST);
			d_col = GMT_prep_nodesearch (GMT, Grid, Ctrl->S.radius, Ctrl->S.mode, &d_row, &max_d_col);
		}
		grd_x0 = GMT_memory (GMT, NULL, Grid->header->nx, double);
		grd_y0 = GMT_memory (GMT, NULL, Grid->header->ny, double);
		for (col = 0; col < Grid->header->nx; col++) grd_x0[col] = GMT_grd_col_to_x (GMT, col, Grid->header);
		for (row = 0; row < Grid->header->ny; row++) grd_y0[row] = GMT_grd_row_to_y (GMT, row, Grid->header);

		inc2[GMT_X] = 0.5 * Grid->header->inc[GMT_X];
		inc2[GMT_Y] = 0.5 * Grid->header->inc[GMT_Y];
		
		if (make_plot) {
			GMT_plane_perspective (GMT, GMT->current.proj.z_project.view_plane, GMT->current.proj.z_level);
			GMT_plotcanvas (GMT);	/* Fill canvas if requested */
		}

		GMT_report (GMT, GMT_MSG_NORMAL, "Allocate memory, read and process data file\n");

		/* Enlarge region by 1 row/column */

		Grid->header->wesn[XLO] -= Grid->header->inc[GMT_X];	Grid->header->wesn[XHI] += Grid->header->inc[GMT_X];	Grid->header->wesn[YLO] -= Grid->header->inc[GMT_Y];	Grid->header->wesn[YHI] += Grid->header->inc[GMT_Y];
		GMT_setnval (GMT->current.io.pad, 4, 0);		/* Change default pad to 0 only */
		GMT_grd_setpad (GMT, Grid->header, GMT->current.io.pad);	/* Change pad to 0 */
		GMT_set_grddim (GMT, Grid->header);
		grd = GMT_memory (GMT, NULL, Grid->header->size, char);

		/* Add GMT_CONV_LIMIT to ensure that special case radius = inc --> lrint(0.5) actually rounds to 1 */
		
		node_only = (max_d_col == 0 && d_row == 0);
		if (node_only && Ctrl->S.radius > 0.0) {
			GMT_report (GMT, GMT_MSG_FATAL, "Warning: Your search radius is too small to have any effect and is ignored.\n");
		}
		
		if ((error = GMT_set_cols (GMT, GMT_IN, 2)) != GMT_OK) {
			Return (error);
		}
		if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_IN, GMT_REG_DEFAULT, 0, options) != GMT_OK) {	/* Establishes data input */
			Return (API->error);
		}
		if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_IN) != GMT_OK) {	/* Enables data input and sets access mode */
			Return (API->error);
		}

		n_read = 0;
		do {	/* Keep returning records until we reach EOF */
			n_read++;
			if ((in = GMT_Get_Record (API, GMT_READ_DOUBLE, NULL)) == NULL) {	/* Read next record, get NULL if special case */
				if (GMT_REC_IS_ERROR (GMT)) 		/* Bail if there are any read errors */
					Return (GMT_RUNTIME_ERROR);
				if (GMT_REC_IS_ANY_HEADER (GMT)) 	/* Skip all table and segment headers */
					continue;
				if (GMT_REC_IS_EOF (GMT)) 		/* Reached end of file */
					break;
			}

			if (GMT_y_is_outside (GMT, in[GMT_Y], Grid->header->wesn[YLO], Grid->header->wesn[YHI])) continue;	/* Outside y-range */
			if (GMT_x_is_outside (GMT, &in[GMT_X], Grid->header->wesn[XLO], Grid->header->wesn[XHI])) continue;	/* Outside x-range (or longitude) */

			/* Data record to process */

			/* Determine the node closest to the data point */

			if (GMT_row_col_out_of_bounds (GMT, in, Grid->header, &row, &col)) continue;			/* Outside grid node range */

			if (node_only) {
				ij = GMT_IJP (Grid->header, row, col);
				grd[ij] = 1;
			}
			else {	/* Set coordinate of this node */

				x0 = GMT_grd_col_to_x (GMT, col, Grid->header);
				y0 = GMT_grd_row_to_y (GMT, row, Grid->header);

				/* Set this and all nodes within radius distance to 1 */

				j_start = (row > d_row) ? row - d_row : 0;
				for (jj = j_start; jj <= row + d_row; jj++) {
					if (jj >= Grid->header->ny) continue;
					i_start = (col > d_col[jj]) ? col - d_col[jj] : 0;
					for (ii = i_start; ii <= col + d_col[jj]; ii++) {
						if (ii >= Grid->header->nx) continue;
						distance = GMT_distance (GMT, x0, y0, grd_x0[ii], grd_y0[jj]);
						if (distance > Ctrl->S.radius) continue;
						ij = GMT_IJP (Grid->header, jj, ii);
						grd[ij] = 1;
					}
				}
			}
		} while (TRUE);
		
		if (GMT_End_IO (API, GMT_IN, 0) != GMT_OK) {	/* Disables further data input */
			Return (API->error);
		}

		GMT_report (GMT, GMT_MSG_NORMAL, "Read %" PRIu64 " data points\n", n_read);

		if (Ctrl->N.active) for (ij = 0; ij < Grid->header->nm; ij++) grd[ij] = 1 - grd[ij];	/* Reverse sense of test */

		/* Force perimeter nodes to be FALSE; thus all contours will be closed */

		for (col = 0, ij = (Grid->header->ny-1) * Grid->header->nx; col < Grid->header->nx; col++) grd[col] = grd[col+ij] = FALSE;
		for (row = 0; row < Grid->header->ny; row++) grd[row*Grid->header->nx] = grd[(row+1)*Grid->header->nx-1] = FALSE;

#ifdef DEBUG
		if (Ctrl->D.debug) {	/* Save a copy of the grid to psmask.nc */
			float *z = GMT_memory (GMT, NULL, Grid->header->nm, float);
			for (ij = 0; ij < Grid->header->nm; ij++) z[ij] = (float)grd[ij];
			GMT_write_grd (GMT, "psmask.nc", Grid->header, z, NULL, GMT->current.io.pad, FALSE);
			GMT_free (GMT, z);
		}
#endif
		if (!Ctrl->T.active) {	/* Must trace the outline of ON/OFF values in grd */
			/* Arrays holding the contour xy values */
			x = GMT_memory (GMT, NULL, GMT_CHUNK, double);
			y = GMT_memory (GMT, NULL, GMT_CHUNK, double);

			n_edges = Grid->header->ny * lrint (ceil (Grid->header->nx / 16.0));
			edge = GMT_memory (GMT, NULL, n_edges, GMT_LONG);

			if (make_plot) GMT_map_basemap (GMT);

			GMT_report (GMT, GMT_MSG_NORMAL, "Tracing the clip path\n");

			section = 0;
			first = TRUE;
			while ((n = clip_contours (GMT, &info, grd, Grid->header, inc2, edge, first, &x, &y, &n_points)) > 0) {
				closed = FALSE;
				shrink_clip_contours (GMT, x, y, n, Grid->header->wesn[XLO], Grid->header->wesn[XHI]);
				if (Ctrl->D.active && n > (COUNTER_LARGE)Ctrl->Q.min) {	/* Save the contour as output data */
					S = GMT_dump_contour (GMT, x, y, n, GMT->session.d_NaN);
					/* Select which table this segment should be added to */
					if (n_seg == n_seg_alloc) {
						size_t n_old_alloc = n_seg_alloc;
						D->table[0]->segment = GMT_memory (GMT, D->table[0]->segment, (n_seg_alloc += GMT_SMALL_CHUNK), struct GMT_LINE_SEGMENT *);
						GMT_memset (&(D->table[0]->segment[n_old_alloc]), n_seg_alloc - n_old_alloc, struct GMT_LINE_SEGMENT *);	/* Set to NULL */
					}
					D->table[0]->segment[n_seg++] = S;
					D->table[0]->n_segments++;	D->n_segments++;
					D->table[0]->n_records += n;	D->n_records += n;
					/* Generate a file name and increment cont_counts, if relevant */
					if (io_mode == GMT_WRITE_TABLES && !D->table[0]->file[GMT_OUT])
						D->table[0]->file[GMT_OUT] = GMT_make_filename (GMT, Ctrl->D.file, fmt, GMT->session.d_NaN, closed, cont_counts);
					else if (io_mode == GMT_WRITE_SEGMENTS)
						S->file[GMT_OUT] = GMT_make_filename (GMT, Ctrl->D.file, fmt, GMT->session.d_NaN, closed, cont_counts);
				}
				if (make_plot) draw_clip_contours (GMT, PSL, x, y, n, Ctrl->G.fill.rgb, section, first);
				first = FALSE;
				section++;
			}

			if (make_plot) draw_clip_contours (GMT, PSL, x, y, 0, Ctrl->G.fill.rgb, section, 2);	/* Activate clip-path */

			GMT_free (GMT, edge);
			GMT_free (GMT, x);
			GMT_free (GMT, y);
			if (Ctrl->D.active) {	/* Write the clip polygon file(s) */
				D->table[0]->segment = GMT_memory (GMT, D->table[0]->segment, n_seg, struct GMT_LINE_SEGMENT *);
				if (GMT_Write_Data (API, GMT_IS_DATASET, GMT_IS_FILE, GMT_IS_POLY, io_mode, NULL, Ctrl->D.file, D) != GMT_OK) {
					Return (API->error);
				}
			}
		}
		else {	/* Just paint tiles */
			COUNTER_LARGE start, n_use, np, plot_n;
			double y_bot, y_top, *xx = NULL, *yy = NULL, *xp = NULL, *yp = NULL;
			GMT_report (GMT, GMT_MSG_NORMAL, "Tiling...\n");

			for (row = 0; row < Grid->header->ny; row++) {
				y_bot = grd_y0[row] - inc2[GMT_Y];
				y_top = grd_y0[row] + inc2[GMT_Y];
				ij = GMT_IJP (Grid->header, row, 1);
				for (col = 0; col < Grid->header->nx; col++, ij++) {
					if (grd[ij] == 0) continue;

					np = GMT_graticule_path (GMT, &xx, &yy, 1, grd_x0[col] - inc2[GMT_X], grd_x0[col] + inc2[GMT_X], y_bot, y_top);
					plot_n = GMT_clip_to_map (GMT, xx, yy, np, &xp, &yp);
					GMT_free (GMT, xx);
					GMT_free (GMT, yy);
					if (plot_n == 0) continue;	/* Outside */
					
					GMT_setfill (GMT, &Ctrl->G.fill, FALSE);
					if ((*GMT->current.map.will_it_wrap) (GMT, xp, yp, plot_n, &start)) {	/* Polygon wraps */

						/* First truncate against left border */

						GMT->current.plot.n = GMT_map_truncate (GMT, xp, yp, plot_n, start, -1);
						n_use = GMT_compact_line (GMT, GMT->current.plot.x, GMT->current.plot.y, GMT->current.plot.n, FALSE, 0);
						PSL_plotpolygon (PSL, GMT->current.plot.x, GMT->current.plot.y, n_use);

						/* Then truncate against right border */

						GMT->current.plot.n = GMT_map_truncate (GMT, xp, yp, plot_n, start, +1);
						n_use = GMT_compact_line (GMT, GMT->current.plot.x, GMT->current.plot.y, GMT->current.plot.n, FALSE, 0);
						PSL_plotpolygon (PSL, GMT->current.plot.x, GMT->current.plot.y, n_use);
					}
					else
						PSL_plotpolygon (PSL, xp, yp, plot_n);
					GMT_free (GMT, xp);
					GMT_free (GMT, yp);
				}
			}

			GMT_map_basemap (GMT);
		}

		if (make_plot) GMT_plane_perspective (GMT, -1, 0.0);

		GMT_free (GMT, grd);
		GMT_free_grid (GMT, &Grid, FALSE);
		if (Ctrl->S.active) GMT_free (GMT, d_col);
		GMT_free (GMT, grd_x0);
		GMT_free (GMT, grd_y0);
		if (!Ctrl->T.active) GMT_report (GMT, GMT_MSG_NORMAL, "clipping on!\n");
	}

	if (make_plot) GMT_plotend (GMT);

	Return (GMT_OK);
}
