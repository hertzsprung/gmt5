 /*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis and F. Wobbe
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
 * Brief synopsis: triangulate reads one or more files (or stdin) with x,y[,whatever] and
 * outputs the indices of the vertices of the optimal Delaunay triangulation
 * using the method by Watson, D. F., ACORD: Automatic contouring of raw data,
 * Computers & Geosciences, 8, 97-101, 1982.  Optionally, the output may take
 * the form of (1) a multi-segment file with the vertex coordinates needed to
 * draw the triangles, or (2) a grid file based on gridding the plane estimates.
 * PS. Instead of Watson's method you may choose to link with the triangulate
 * routine written by Jonathan Shewchuk.  See the file TRIANGLE.HOWTO for
 * details.  That function is far faster than Watson's method and also allows
 * for Voronoi polygon output.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */
 
#define THIS_MODULE k_mod_triangulate /* I am triangulate */

#include "gmt.h"

struct TRIANGULATE_CTRL {
	struct D {	/* -Dx|y */
		bool active;
		unsigned int dir;
	} D;
	struct E {	/* -E<value> */
		bool active;
		double value;
	} E;
	struct G {	/* -G<output_grdfile> */
		bool active;
		char *file;
	} G;
	struct I {	/* -Idx[/dy] */
		bool active;
		double inc[2];
	} I;
	struct M {	/* -M */
		bool active;
	} M;
	struct Q {	/* -Q */
		bool active;
	} Q;
	struct S {	/* -S */
		bool active;
	} S;
	struct Z {	/* -Z */
		bool active;
	} Z;
};

struct TRIANGULATE_EDGE {
	unsigned int begin, end;
};

int compare_edge (const void *p1, const void *p2)
{
	const struct TRIANGULATE_EDGE *a = p1, *b = p2;

	if (a->begin < b->begin) return (-1);
	if (a->begin > b->begin) return (+1);
	if (a->end < b->end) return (-1);
	if (a->end > b->end) return (+1);
	return (0);
}

void *New_triangulate_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct TRIANGULATE_CTRL *C = NULL;
	
	C = GMT_memory (GMT, NULL, 1, struct TRIANGULATE_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
	C->D.dir = 2;	/* No derivatives */
	return (C);
}

void Free_triangulate_Ctrl (struct GMT_CTRL *GMT, struct TRIANGULATE_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->G.file) free (C->G.file);	
	GMT_free (GMT, C);	
}

int GMT_triangulate_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: triangulate [<table>] [-Dx|y] [-E<empty>] [-G<outgrid>]\n");
	GMT_message (GMT, "\t[%s] [%s] [-M[z]] [-Q]", GMT_I_OPT, GMT_J_OPT);
	GMT_message (GMT, "\n\t[%s] [-S] [%s] [-Z] [%s] [%s]\n\t[%s] [%s] [%s] [%s]\n\n",
		GMT_Rgeo_OPT, GMT_V_OPT, GMT_b_OPT, GMT_f_OPT, GMT_h_OPT, GMT_i_OPT, GMT_r_OPT, GMT_colon_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "<");   
	GMT_message (GMT, "\t-D Take derivative in the x- or y-direction (only with -G) [Default is z value].\n");
	GMT_message (GMT, "\t-E Value to use for empty nodes [Default is NaN].\n");
	GMT_message (GMT, "\t-G Grid data. Give name of output grid file and specify -R -I.\n");
	GMT_message (GMT, "\t   Cannot be used with -Q.\n");
	GMT_inc_syntax (GMT, 'I', 0);
	GMT_explain_options (GMT, "J");   
	GMT_message (GMT, "\t-M Output triangle edges as multiple segments separated by segment headers.\n");
	GMT_message (GMT, "\t   [Default is to output the indices of vertices for each Delaunay triangle].\n");
	GMT_message (GMT, "\t-Q Compute Voronoi polygon edges instead (requires -R and Shewchuk algorithm) [Delaunay triangulation].\n");
	GMT_message (GMT, "\t-S Output triangle polygons as multiple segments separated by segment headers.\n");
	GMT_message (GMT, "\t   Cannot be used with -Q.\n");
	GMT_message (GMT, "\t-Z Expect (x,y,z) data on input (and output); automatically set if -G is used [Expect (x,y) data].\n");
	GMT_explain_options (GMT, "RVC2");
	GMT_message (GMT, "\t-bo Write binary (double) index table [Default is ASCII i/o].\n");
	GMT_explain_options (GMT, "fhiF:.");
	
	return (EXIT_FAILURE);
}

int GMT_triangulate_parse (struct GMTAPI_CTRL *C, struct TRIANGULATE_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to triangulate and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Skip input files */
				break;

			/* Processes program-specific parameters */

			case 'D':
				Ctrl->D.active = true;
				switch (opt->arg[0]) {
					case 'x': case 'X':
						Ctrl->D.dir = GMT_X; break;
					case 'y': case 'Y':
						Ctrl->D.dir = GMT_Y; break;
					default:
						GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error: Give -Dx or -Dy\n");
						n_errors++; break;
				}
				break;
			case 'E':
				Ctrl->E.active = true;
				Ctrl->E.value = (opt->arg[0] == 'N' || opt->arg[0] == 'n') ? GMT->session.d_NaN : atof (opt->arg);
				break;
			case 'G':
				Ctrl->G.active = true;
				Ctrl->G.file = strdup (opt->arg);
				break;
			case 'I':
				Ctrl->I.active = true;
				if (GMT_getinc (GMT, opt->arg, Ctrl->I.inc)) {
					GMT_inc_syntax (GMT, 'I', 1);
					n_errors++;
				}
				break;
#ifdef GMT_COMPAT
			case 'm':
				GMT_report (GMT, GMT_MSG_COMPAT, "Warning: -m option is deprecated and reverted back to -M.\n");
#endif
			case 'M':
				Ctrl->M.active = true;
				break;
			case 'Q':
				Ctrl->Q.active = true;
				break;
			case 'S':
				Ctrl->S.active = true;
				break;
			case 'Z':
				Ctrl->Z.active = true;
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	GMT_check_lattice (GMT, Ctrl->I.inc, &GMT->common.r.active, &Ctrl->I.active);

	n_errors += GMT_check_binary_io (GMT, 2);
	n_errors += GMT_check_condition (GMT, Ctrl->I.active && (Ctrl->I.inc[GMT_X] <= 0.0 || Ctrl->I.inc[GMT_Y] <= 0.0), "Syntax error -I option: Must specify positive increment(s)\n");
	n_errors += GMT_check_condition (GMT, Ctrl->G.active && !Ctrl->G.file, "Syntax error -G option: Must specify file name\n");
	n_errors += GMT_check_condition (GMT, Ctrl->G.active && (Ctrl->I.active + GMT->common.R.active) != 2, "Syntax error: Must specify -R, -I, -G for gridding\n");
	n_errors += GMT_check_condition (GMT, Ctrl->G.active && Ctrl->Q.active, "Syntax error -G option: Cannot be used with -Q\n");
	n_errors += GMT_check_condition (GMT, Ctrl->S.active && Ctrl->Q.active, "Syntax error -G option: Cannot be used with -S\n");
	n_errors += GMT_check_condition (GMT, Ctrl->Q.active && !GMT->common.R.active, "Syntax error -Q option: Requires -R\n");
	n_errors += GMT_check_condition (GMT, Ctrl->Q.active && GMT->current.setting.triangulate == GMT_TRIANGLE_WATSON, "Syntax error -Q option: Requires Shewchck triangulation algorithm\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_triangulate_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_triangulate (struct GMTAPI_CTRL *API, int mode, void *args)
{
	int *link = NULL;	/* Must remain int and not int due to triangle function */
	
	uint64_t ij, ij1, ij2, ij3, np, i, j, k, n_edge, p, n = 0;
	unsigned int n_input, n_output;
	int row, col, col_min, col_max, row_min, row_max;
	bool triplets[2] = {false, false}, error = false, map_them = false;
	
	size_t n_alloc;
	
	double zj, zk, zl, zlj, zkj, xp, yp, a, b, c, f;
	double xkj, xlj, ykj, ylj, out[3], vx[4], vy[4];
	double *xx = NULL, *yy = NULL, *zz = NULL, *in = NULL;
	double *xe = NULL, *ye = NULL;

	char *tri_algorithm[2] = {"Watson", "Shewchuk"};

	struct GMT_GRID *Grid = NULL;

	struct TRIANGULATE_EDGE *edge = NULL;
	struct TRIANGULATE_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_triangulate_usage (API, GMTAPI_USAGE));/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_triangulate_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-VJRbf:", "hirs>" GMT_OPT("FHm"), options)) Return (API->error);
	Ctrl = New_triangulate_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_triangulate_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the triangulate main code ----------------------------*/

	GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "%s triangulation algoritm selected\n", tri_algorithm[GMT->current.setting.triangulate]);
	
	if (Ctrl->G.active) {
		if ((Grid = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);
		GMT_grd_init (GMT, Grid->header, options, false);
		/* Completely determine the header for the new grid; croak if there are issues.  No memory is allocated here. */
		GMT_err_fail (GMT, GMT_init_newgrid (GMT, Grid, GMT->common.R.wesn, Ctrl->I.inc, GMT->common.r.active), Ctrl->G.file);
	}
	if (Ctrl->Q.active && Ctrl->Z.active) GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "Warning: We will read (x,y,z), but only (x,y) will be output when -Q is used\n");
	n_output = ((Ctrl->M.active || Ctrl->S.active) && (Ctrl->Q.active || !Ctrl->Z.active)) ? 2 : 3;
	triplets[GMT_OUT] = (n_output == 3);
	if ((error = GMT_set_cols (GMT, GMT_OUT, n_output))) Return (error);
	
	if (GMT->common.R.active && GMT->common.J.active) { /* Gave -R -J */
		map_them = true;
		GMT_err_fail (GMT, GMT_map_setup (GMT, Grid->header->wesn), "");
	}

	/* Now we are ready to take on some input values */

	n_input = (Ctrl->G.active || Ctrl->Z.active) ? 3 : 2;
	if ((error = GMT_set_cols (GMT, GMT_IN, n_input)) != GMT_OK) {
		Return (error);
	}

	/* Initialize the i/o since we are doing record-by-record reading/writing */
	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_IN, GMT_REG_DEFAULT, 0, options) != GMT_OK) {	/* Establishes data input */
		Return (API->error);
	}
	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_IN) != GMT_OK) {	/* Enables data input and sets access mode */
		Return (API->error);
	}

	triplets[GMT_IN] = (n_input == 3);
	n_alloc = GMT_CHUNK;
	xx = GMT_memory (GMT, NULL, n_alloc, double);
	yy = GMT_memory (GMT, NULL, n_alloc, double);
	if (triplets[GMT_IN]) zz = GMT_memory (GMT, NULL, n_alloc, double);

	n = 0;
	do {	/* Keep returning records until we reach EOF */
		if ((in = GMT_Get_Record (API, GMT_READ_DOUBLE, NULL)) == NULL) {	/* Read next record, get NULL if special case */
			if (GMT_REC_IS_ERROR (GMT)) 		/* Bail if there are any read errors */
				Return (GMT_RUNTIME_ERROR);
			if (GMT_REC_IS_ANY_HEADER (GMT)) 	/* Skip all headers */
				continue;
			if (GMT_REC_IS_EOF (GMT)) 		/* Reached end of file */
				break;
		}

		/* Data record to process */
	
		xx[n] = in[GMT_X];	yy[n] = in[GMT_Y];
		if (triplets[GMT_IN]) zz[n] = in[GMT_Z];
		n++;

		if (n == n_alloc) {	/* Get more memory */
			n_alloc <<= 1;
			xx = GMT_memory (GMT, xx, n_alloc, double);
			yy = GMT_memory (GMT, yy, n_alloc, double);
			if (triplets[GMT_IN]) zz = GMT_memory (GMT, zz, n_alloc, double);
		}
		if (n == INT_MAX) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Error: Cannot triangulate more than %d points\n", INT_MAX);
			GMT_free (GMT, xx);
			GMT_free (GMT, yy);
			if (triplets[GMT_IN]) GMT_free (GMT, zz);
			Return (EXIT_FAILURE);
		}
	} while (true);
	
	if (GMT_End_IO (API, GMT_IN, 0) != GMT_OK) {	/* Disables further data input */
		Return (API->error);
	}

	xx = GMT_memory (GMT, xx, n, double);
	yy = GMT_memory (GMT, yy, n, double);
	if (triplets[GMT_IN]) zz = GMT_memory (GMT, zz, n, double);

	if (map_them) {	/* Must make parallel arrays for projected x/y */
		double *xxp = NULL, *yyp = NULL;

		xxp = GMT_memory (GMT, NULL, n, double);
		yyp = GMT_memory (GMT, NULL, n, double);
		for (i = 0; i < n; i++) GMT_geo_to_xy (GMT, xx[i], yy[i], &xxp[i], &yyp[i]);

		GMT_report (GMT, GMT_MSG_VERBOSE, "Do Delaunay optimal triangulation on projected coordinates\n");

		if (Ctrl->Q.active) {
			double we[2];
			we[0] = GMT->current.proj.rect[XLO];	we[1] = GMT->current.proj.rect[XHI];
			np = GMT_voronoi (GMT, xxp, yyp, n, we, &xe, &ye);
		}
		else
			np = GMT_delaunay (GMT, xxp, yyp, n, &link);

		GMT_free (GMT, xxp);
		GMT_free (GMT, yyp);
	}
	else {
		GMT_report (GMT, GMT_MSG_VERBOSE, "Do Delaunay optimal triangulation on given coordinates\n");

		if (Ctrl->Q.active) {
			double we[2];
			we[0] = GMT->common.R.wesn[XLO];	we[1] = GMT->common.R.wesn[XHI];
			np = GMT_voronoi (GMT, xx, yy, n, we, &xe, &ye);
		}
		else
			np = GMT_delaunay (GMT, xx, yy, n, &link);
	}

	if (Ctrl->Q.active)
		GMT_report (GMT, GMT_MSG_VERBOSE, "%" PRIu64 " Voronoi edges found\n", np);
	else
		GMT_report (GMT, GMT_MSG_VERBOSE, "%" PRIu64 " Delaunay triangles found\n", np);
	

	if (Ctrl->G.active) {	/* Grid via planar triangle segments */
		int nx = Grid->header->nx, ny = Grid->header->ny;	/* Signed versions */
		Grid->data = GMT_memory (GMT, NULL, Grid->header->size, float);
		if (!Ctrl->E.active) Ctrl->E.value = GMT->session.d_NaN;
		for (p = 0; p < Grid->header->size; p++) Grid->data[p] = (float)Ctrl->E.value;	/* initialize grid */

		for (k = ij = 0; k < np; k++) {

			/* Find equation for the plane as z = ax + by + c */

			vx[0] = vx[3] = xx[link[ij]];	vy[0] = vy[3] = yy[link[ij]];	zj = zz[link[ij++]];
			vx[1] = xx[link[ij]];	vy[1] = yy[link[ij]];	zk = zz[link[ij++]];
			vx[2] = xx[link[ij]];	vy[2] = yy[link[ij]];	zl = zz[link[ij++]];

			xkj = vx[1] - vx[0];	ykj = vy[1] - vy[0];
			zkj = zk - zj;	xlj = vx[2] - vx[0];
			ylj = vy[2] - vy[0];	zlj = zl - zj;

			f = 1.0 / (xkj * ylj - ykj * xlj);
			a = -f * (ykj * zlj - zkj * ylj);
			b = -f * (zkj * xlj - xkj * zlj);
			c = -a * vx[1] - b * vy[1] + zk;

			/* Compute grid indices the current triangle may cover, assuming all triangles are
			   in the -R region (Grid->header->wesn[XLO]/x_max etc.)  Always, col_min <= col_max, row_min <= row_max.
			 */

			xp = MIN (MIN (vx[0], vx[1]), vx[2]);	col_min = GMT_grd_x_to_col (GMT, xp, Grid->header);
			xp = MAX (MAX (vx[0], vx[1]), vx[2]);	col_max = GMT_grd_x_to_col (GMT, xp, Grid->header);
			yp = MAX (MAX (vy[0], vy[1]), vy[2]);	row_min = GMT_grd_y_to_row (GMT, yp, Grid->header);
			yp = MIN (MIN (vy[0], vy[1]), vy[2]);	row_max = GMT_grd_y_to_row (GMT, yp, Grid->header);

			/* Adjustments for triangles outside -R region. */
			/* Triangle to the left or right. */
			if ((col_max < 0) || (col_min >= nx)) continue;
			/* Triangle Above or below */
			if ((row_max < 0) || (row_min >= ny)) continue;

			/* Triangle covers boundary, left or right. */
			if (col_min < 0) col_min = 0;       if (col_max >= nx) col_max = Grid->header->nx - 1;
			/* Triangle covers boundary, top or bottom. */
			if (row_min < 0) row_min = 0;       if (row_max >= ny) row_max = Grid->header->ny - 1;

			for (row = row_min; row <= row_max; row++) {
				yp = GMT_grd_row_to_y (GMT, row, Grid->header);
				p = GMT_IJP (Grid->header, row, col_min);
				for (col = col_min; col <= col_max; col++, p++) {
					xp = GMT_grd_col_to_x (GMT, col, Grid->header);

					if (!GMT_non_zero_winding (GMT, xp, yp, vx, vy, 4)) continue;	/* Outside */

					if (Ctrl->D.dir == GMT_X)
						Grid->data[p] = (float)a;
					else if (Ctrl->D.dir == GMT_Y)
						Grid->data[p] = (float)b;
					else
						Grid->data[p] = (float)(a * xp + b * yp + c);
				}
			}
		}
		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->G.file, Grid) != GMT_OK) {
			Return (API->error);
		}
	}
	
	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_OUT, GMT_REG_DEFAULT, 0, options) != GMT_OK) {	/* Establishes data output */
		Return (API->error);
	}
	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_OUT) != GMT_OK) {	/* Enables data output and sets access mode */
		Return (API->error);
	}
	if (Ctrl->M.active) {	/* Must find unique edges to output only once */
		if (Ctrl->Q.active) {	/* Voronoi edges */
			for (i = j = 0; i < np; i++) {
				fprintf (GMT->session.std[GMT_OUT], "%c Edge %" PRIu64 "\n", GMT->current.setting.io_seg_marker[GMT_OUT], i);
				out[GMT_X] = xe[j];	out[GMT_Y] = ye[j++];
				GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);
				out[GMT_X] = xe[j];	out[GMT_Y] = ye[j++];
				GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);
			}
			GMT_free (GMT, xe);
			GMT_free (GMT, ye);
		}
		else {	/* Triangle edges */
			n_edge = 3 * np;
			edge = GMT_memory (GMT, NULL, n_edge, struct TRIANGULATE_EDGE);
			for (i = ij1 = 0, ij2 = 1, ij3 = 2; i < np; i++, ij1 += 3, ij2 += 3, ij3 += 3) {
				edge[ij1].begin = link[ij1];	edge[ij1].end = link[ij2];
				edge[ij2].begin = link[ij2];	edge[ij2].end = link[ij3];
				edge[ij3].begin = link[ij1];	edge[ij3].end = link[ij3];
			}
			for (i = 0; i < n_edge; i++) if (edge[i].begin > edge[i].end) int_swap (edge[i].begin, edge[i].end);

			qsort (edge, n_edge, sizeof (struct TRIANGULATE_EDGE), compare_edge);
			for (i = 1, j = 0; i < n_edge; i++) {
				if (edge[i].begin != edge[j].begin || edge[i].end != edge[j].end) j++;
				edge[j] = edge[i];
			}
			n_edge = j + 1;

			GMT_report (GMT, GMT_MSG_VERBOSE, "%" PRIu64 " unique triangle edges\n", n_edge);

			for (i = 0; i < n_edge; i++) {
				fprintf (GMT->session.std[GMT_OUT], "%c Edge %d-%d\n", GMT->current.setting.io_seg_marker[GMT_OUT], edge[i].begin, edge[i].end);
				out[GMT_X] = xx[edge[i].begin];	out[GMT_Y] = yy[edge[i].begin];	if (triplets[GMT_OUT]) out[GMT_Z] = zz[edge[i].begin];
				GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);
				out[GMT_X] = xx[edge[i].end];	out[GMT_Y] = yy[edge[i].end];	if (triplets[GMT_OUT]) out[GMT_Z] = zz[edge[i].end];
				GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);
			}
			GMT_free (GMT, edge);
		}
	}
	else if (Ctrl->S.active)  {	/* Write triangle polygons */
		for (i = ij = 0; i < np; i++, ij += 3) {
			fprintf (GMT->session.std[GMT_OUT], "%c Polygon %d-%d-%d\n", GMT->current.setting.io_seg_marker[GMT_OUT], link[ij], link[ij+1], link[ij+2]);
			for (k = 0; k < 3; k++) {	/* Three vertices */
				out[GMT_X] = xx[link[ij+k]];	out[GMT_Y] = yy[link[ij+k]];	if (triplets[GMT_OUT]) out[GMT_Z] = zz[link[ij+k]];
				GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);	/* Write this to output */
			}
		}
	}
	else {	/* Write table of indices */
		for (i = ij = 0; i < np; i++, ij += 3) {
			for (k = 0; k < 3; k++) out[k] = (double)link[ij+k];
			GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);	/* Write this to output */
		}
	}
	if (GMT_End_IO (API, GMT_OUT, 0) != GMT_OK) {	/* Disables further data output */
		Return (API->error);
	}

	GMT_free (GMT, xx);
	GMT_free (GMT, yy);
	if (triplets[GMT_IN]) GMT_free (GMT, zz);
#ifdef TRIANGLE_D
#ifdef MEMDEBUG
	/* Shewchuk's function allocated the memory separately */
	if (GMT->current.setting.triangulate == GMT_TRIANGLE_SHEWCHUK)
		GMT_memtrack_off (GMT, &g_mem_keeper);
#endif
#endif
	if (!Ctrl->Q.active) GMT_free (GMT, link);
#ifdef TRIANGLE_D
#ifdef MEMDEBUG
	if (GMT->current.setting.triangulate == GMT_TRIANGLE_SHEWCHUK)
		GMT_memtrack_on (GMT, &g_mem_keeper);
#endif
#endif

	GMT_report (GMT, GMT_MSG_VERBOSE, "Done!\n");

	Return (GMT_OK);
}
