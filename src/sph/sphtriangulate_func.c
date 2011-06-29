/*--------------------------------------------------------------------
 *	$Id: sphtriangulate_func.c,v 1.19 2011-06-29 07:09:37 guru Exp $
 *
 *	Copyright (c) 2008-2011 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 or any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 * Spherical triangulation - with Delaunay or Voronoi output.
 * Relies on STRIPACK Fortran F77 library (Renka, 1997). Reference:
 * Renka, R, J,, 1997, Algorithm 772: STRIPACK: Delaunay Triangulation
 *  and Voronoi Diagram on the Surface of a Sphere, AMC Trans. Math.
 *  Software, 23 (3), 416-434.
 * We translate to C using f2c and link with -lf2c
 *
 * Author:      Paul Wessel
 * Date:        13-FEB-2008
 *
 */
 
#include "gmt_sph.h"
#include "sph.h"

struct SPHTRIANGULATE_CTRL {
	struct A {	/* -A */
		GMT_LONG active;
	} A;
	struct C {	/* -C */
		GMT_LONG active;
	} C;
	struct D {	/* -D */
		GMT_LONG active;
	} D;
	struct G {	/* -G<output_grdfile> */
		GMT_LONG active;
		char *file;
	} G;
	struct L {	/* -L<unit>] */
		GMT_LONG active;
		char unit;
	} L;
	struct N {	/* -N */
		GMT_LONG active;
		char *file;
	} N;
	struct Q {	/* -Q */
		GMT_LONG active;
		GMT_LONG mode;	/* 0 is Delaunay, 1 is Voronoi */
	} Q;
	struct T {	/* -T */
		GMT_LONG active;
	} T;
};

void stripack_delaunay_output (struct GMT_CTRL *GMT, double *lon, double *lat, struct STRIPACK_DELAUNAY *D, GMT_LONG get_arcs, GMT_LONG get_area, GMT_LONG nodes, struct GMT_DATASET ***DD)
{	/* Prints out the Delaunay triangles either as polygons (for filling) or arcs (lines). */
	GMT_LONG i, ij, k, error, ID, dim[4] = {1, 0, 0, 0};
	double area_sphere = 0.0, area_triangle = GMT->session.d_NaN, V[3][3], R2 = 6371007.1810 * 6371007.1810, dist = GMT->session.d_NaN;
	char segment_header[GMT_BUFSIZ];
	struct GMT_DATASET *Dout[2] = {NULL, NULL};
	struct GMT_LINE_SEGMENT *S[2] = {NULL, NULL};
	if (get_area == 2) /* Steradians */
		R2 = GMT->current.map.dist[GMT_MAP_DIST].scale = 1.0;
	else
		R2 *= (GMT->current.map.dist[GMT_MAP_DIST].scale * GMT->current.map.dist[GMT_MAP_DIST].scale);	/* Get final measure unit for area */

	if (!get_arcs) {	/* Output polygons of three points.  Not explicitly closed (use -L or -G in psxy) */
		dim[1] = D->n;	/* segments */
		dim[2] = 2;	/* Columns */
		dim[3] = 3;	/* Rows */
		if ((error = GMT_Create_Data (GMT->parent, GMT_IS_DATASET, dim, (void **)&Dout[0], -1, &ID))) {
			GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a data set for sphtriangulate\n");
			GMT_exit (EXIT_FAILURE);
		}
		if (nodes) {	/* Want Voronoi node and area information via Dout[2] */
			dim[1] = 1;	/* segments */
			dim[2] = 4;	/* Columns */
			dim[3] = D->n;	/* Rows */
			if ((error = GMT_Create_Data (GMT->parent, GMT_IS_DATASET, dim, (void **)&Dout[1], -1, &ID))) {
				GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a data set for sphtriangulate nodes\n");
				GMT_exit (EXIT_FAILURE);
			}
			S[1] = Dout[1]->table[0]->segment[0];
		}
		GMT_report (GMT, GMT_MSG_NORMAL, "%s: Output %d unique triangle polygons\n", GMT->init.progname, D->n);
		for (k = ij = 0; k < D->n; k++, ij += TRI_NROW) {	/* For each triangle */
			S[0] = Dout[0]->table[0]->segment[k];
			/* Write segment header with triangle # and the three node numbers */
			if (get_area) {
				for (i = 0; i < 3; i++) GMT_geo_to_cart (GMT, lon[D->tri[ij+i]], lat[D->tri[ij+i]], V[i], TRUE);
				area_triangle = stripack_areas (V[0], V[1], V[2]);
				area_sphere += area_triangle;
			}
			if (nodes) {
				S[1]->coord[GMT_X][k] = D->tri[ij];
				S[1]->coord[GMT_Y][k] = D->tri[ij+1];
				S[1]->coord[GMT_Z][k] = D->tri[ij+2];
				S[1]->coord[3][k] = area_triangle * R2;
				sprintf (segment_header, "Triangle: %ld %d-%d-%d Area: %g", k, D->tri[ij], D->tri[ij+1], D->tri[ij+2], area_triangle * R2);
				S[0]->header = strdup (segment_header);
			}
			else {
				sprintf (segment_header, "Triangle: %ld", k);
				S[0]->header = strdup (segment_header);
			}
			for (i = 0; i < 3; i++) {	/* Write out the three vertices */
				S[0]->coord[GMT_X][i] = lon[D->tri[ij+i]];
				S[0]->coord[GMT_Y][i] = lat[D->tri[ij+i]];
			}
		}
		if (get_area) GMT_report (GMT, GMT_MSG_NORMAL, "Total surface area = %g\n", area_sphere * R2);
	}
	else {	/* Want just the arcs (to draw then, probably).  This avoids repeating arcs */
		GMT_LONG j, ij1, ij2, ij3, n_arcs;
		struct STRPACK_ARC *arc = NULL;
		
		n_arcs = 3 * D->n;
		arc = GMT_memory (GMT, NULL, n_arcs, struct STRPACK_ARC);
		for (k = ij = ij1 = 0, ij2 = 1, ij3 = 2; k < D->n; k++, ij1 += TRI_NROW, ij2 += TRI_NROW, ij3 += TRI_NROW) {	/* For each triangle */
			arc[ij].begin = D->tri[ij1];	arc[ij].end = D->tri[ij2];	ij++;
			arc[ij].begin = D->tri[ij2];	arc[ij].end = D->tri[ij3];	ij++;
			arc[ij].begin = D->tri[ij1];	arc[ij].end = D->tri[ij3];	ij++;
		}
		for (k = 0; k < n_arcs; k++) if (arc[k].begin > arc[k].end) i_swap (arc[k].begin, arc[k].end);

		qsort ((void *)arc, (size_t)n_arcs, sizeof (struct STRPACK_ARC), compare_arc);
		for (i = 1, j = 0; i < n_arcs; i++) {
			if (arc[i].begin != arc[j].begin || arc[i].end != arc[j].end) j++;
			arc[j] = arc[i];
		}
		n_arcs = j + 1;
		GMT_report (GMT, GMT_MSG_NORMAL, "Output %ld unique triangle arcs\n", n_arcs);

		dim[1] = n_arcs;	/* segments */
		dim[2] = 2;		/* Columns */
		dim[3] = 2;		/* Rows */
		if ((error = GMT_Create_Data (GMT->parent, GMT_IS_DATASET, dim, (void **)&Dout[0], -1, &ID))) {
			GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a data set for sphtriangulate arcs\n");
			GMT_exit (EXIT_FAILURE);
		}
		for (i = 0; i < n_arcs; i++) {
			S[0] = Dout[0]->table[0]->segment[i];
			S[0]->coord[GMT_X][0] = lon[arc[i].begin];	S[0]->coord[GMT_Y][0] = lat[arc[i].begin];
			S[0]->coord[GMT_X][1] = lon[arc[i].end];	S[0]->coord[GMT_Y][1] = lat[arc[i].end];
			if (get_area) dist = GMT_distance (GMT, S[0]->coord[GMT_X][0], S[0]->coord[GMT_Y][0], S[0]->coord[GMT_X][1], S[0]->coord[GMT_Y][1]);
			sprintf (segment_header, "Arc: %d-%d Length: %g", arc[i].begin, arc[i].end, dist);
			S[0]->header = strdup (segment_header);
		}
		GMT_free (GMT, arc);
	}
	*DD = Dout;
}

void stripack_voronoi_output (struct GMT_CTRL *GMT, GMT_LONG n, double *lon, double *lat, struct STRIPACK_VORONOI *V, GMT_LONG get_arcs, GMT_LONG get_area, GMT_LONG nodes, struct GMT_DATASET ***DD)
{	/* Prints out the Voronoi polygons either as polygons (for filling) or arcs (lines) */
	GMT_LONG i, j, k, node, vertex, node_stop, node_new, vertex_new, node_last, vertex_last, n_arcs = 0;
	GMT_LONG n_alloc = GMT_CHUNK, p_alloc = GMT_TINY_CHUNK, error, ID, dim[4] = {1, 0, 0, 0};
	
	char segment_header[GMT_BUFSIZ];
	
	double area_sphere = 0.0, area_polygon, area_triangle, area_km2 = GMT->session.d_NaN, dist = GMT->session.d_NaN, V1[3], V2[3], V3[3];
	double *plat = NULL, *plon = NULL, R2 = 6371007.1810 * 6371007.1810;

	struct GMT_DATASET *Dout[2] = {NULL, NULL};
	struct GMT_LINE_SEGMENT *S[2] = {NULL, NULL};
	struct STRPACK_ARC *arc = NULL;

	if (get_area == 2) /* Steradians */
		R2 = GMT->current.map.dist[GMT_MAP_DIST].scale = 1.0;
	else
		R2 *= (GMT->current.map.dist[GMT_MAP_DIST].scale * GMT->current.map.dist[GMT_MAP_DIST].scale);	/* Get final measure unit for area */
	if (get_arcs) arc = GMT_memory (GMT, NULL, n_alloc, struct STRPACK_ARC);
	plon = GMT_memory (GMT, NULL, p_alloc, double);
	plat = GMT_memory (GMT, NULL, p_alloc, double);
	
	dim[1] = n;	/* segments */
	dim[2] = 2;	/* Columns */
	dim[3] = (get_arcs) ? 2 : 0;	/* Rows (unknown length if polygons; 2 if arc) */
	if ((error = GMT_Create_Data (GMT->parent, GMT_IS_DATASET, dim, (void **)&Dout[0], -1, &ID))) {
		GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a data set for sphtriangulate\n");
		GMT_exit (EXIT_FAILURE);
	}
	if (nodes) {	/* Want Voronoi node and area information via Dout[2] */
		dim[1] = 1;	/* segments */
		dim[2] = 3;	/* Columns */
		dim[3] = n;	/* Rows */
		if ((error = GMT_Create_Data (GMT->parent, GMT_IS_DATASET, dim, (void **)&Dout[1], -1, &ID))) {
			GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a data set for sphtriangulate nodes\n");
			GMT_exit (EXIT_FAILURE);
		}
		S[1] = Dout[1]->table[0]->segment[0];
	}

	for (node = 0; node < n; node++) {

		area_polygon = 0.0;

		node_new = node_stop = V->lend[node];
		vertex_new = V->listc[node_new];

		/* Each iteration of this DO walks along one side of the polygon,
		   considering the subtriangle NODE --> VERTEX_LAST --> VERTEX. */

		vertex = 0;
		do {
			node_last = node_new;
			node_new = V->lptr[node_last];
			vertex_last = vertex_new;
			vertex_new = V->listc[node_new];

			if (get_arcs) {
				arc[n_arcs].begin = (int)vertex_last;	arc[n_arcs].end = (int)vertex_new;
				n_arcs++;
				if (n_arcs == n_alloc) {
					n_alloc <<= 1;
					arc = GMT_memory (GMT, arc, n_alloc, struct STRPACK_ARC);
				}
				vertex++;
			}
			else {
				plon[vertex] = V->lon[vertex_last];
				plat[vertex] = V->lat[vertex_last];
				if (get_area) {	/* Convert three corners to Cartesian */
					GMT_geo_to_cart (GMT, V->lon[node], V->lat[node], V1, TRUE);
					GMT_geo_to_cart (GMT, V->lon[vertex_last], V->lat[vertex_last], V2, TRUE);
					GMT_geo_to_cart (GMT, V->lon[vertex_new], V->lat[vertex_new], V3, TRUE);
					area_triangle = stripack_areas (V1, V2, V3);
					area_polygon += area_triangle;
				}
				vertex++;
				if (vertex == p_alloc) {
					p_alloc <<= 1;
					plon = GMT_memory (GMT, plon, p_alloc, double);
					plat = GMT_memory (GMT, plat, p_alloc, double);
				}
			}

			/* When we reach the vertex where we started, we are done with this polygon */
		} while (node_new != node_stop);

		if (!get_arcs) {
			S[0] = Dout[0]->table[0]->segment[node];
			if (get_area) area_km2 = area_polygon * R2;
			if (nodes) {
				S[1]->coord[GMT_X][node] = lon[node];
				S[1]->coord[GMT_Y][node] = lat[node];
				S[1]->coord[GMT_Z][node] = area_km2;
				sprintf (segment_header, "Pol: %ld", node);
				S[0]->header = strdup (segment_header);
			}
			else {
				sprintf (segment_header, "Pol: %ld %g %g Area: %g", node, lon[node], lat[node], area_km2);
				S[0]->header = strdup (segment_header);
			}
			
			GMT_alloc_segment (GMT, S[0], vertex, 2, FALSE);
			GMT_memcpy (S[0]->coord[GMT_X], plon, vertex, double);
			GMT_memcpy (S[0]->coord[GMT_Y], plat, vertex, double);
	
			if (get_area) area_sphere += area_polygon;
		}
	}
	if (get_arcs) {	/* Process arcs */
		for (k = 0; k < n_arcs; k++) if (arc[k].begin > arc[k].end) i_swap (arc[k].begin, arc[k].end);

		qsort ((void *)arc, (size_t)n_arcs, sizeof (struct STRPACK_ARC), compare_arc);
		for (i = 1, j = 0; i < n_arcs; i++) {
			if (arc[i].begin != arc[j].begin || arc[i].end != arc[j].end) j++;
			arc[j] = arc[i];
		}
		n_arcs = j + 1;
		dim[1] = n_arcs;	/* segments */
		dim[2] = 2;		/* Columns */
		dim[3] = 2;		/* Rows */
		if ((error = GMT_Create_Data (GMT->parent, GMT_IS_DATASET, dim, (void **)&Dout[0], -1, &ID))) {
			GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a data set for sphtriangulate Voronoi nodes\n");
			GMT_exit (EXIT_FAILURE);
		}
		GMT_report (GMT, GMT_MSG_NORMAL, "Output %ld unique Voronoi arcs\n", n_arcs);

		for (i = 0; i < n_arcs; i++) {
			S[0] = Dout[0]->table[0]->segment[i];
			S[0]->coord[GMT_X][0] = V->lon[arc[i].end];	S[1]->coord[GMT_Y][0] = V->lat[arc[i].end];
			S[0]->coord[GMT_X][1] = V->lon[arc[i].begin];	S[1]->coord[GMT_Y][1] = V->lat[arc[i].begin];
			if (get_area) dist = GMT_distance (GMT, S[0]->coord[GMT_X][0], S[0]->coord[GMT_Y][0], S[0]->coord[GMT_X][1], S[0]->coord[GMT_Y][1]);
			sprintf (segment_header, "Arc: %d-%d Length: %g", arc[i].begin, arc[i].end, dist);
			S[0]->header = strdup (segment_header);
		}
		GMT_free (GMT, arc);
	}
	else {
		GMT_free (GMT, plon);
		GMT_free (GMT, plat);
		if (nodes) *DD[1] = Dout[1];
		if (get_area) GMT_report (GMT, GMT_MSG_NORMAL, "Total surface area = %g\n", area_sphere * R2);
	}
	*DD = Dout;
}

char *unit_name (char unit, GMT_LONG arc) {
	char *name;
	switch (unit) {
		case 'k':	/* km */
			name = (arc) ? "km" : "km^2";
			break;
		case 'M':	/* Miles */
			name = (arc) ? "miles" : "sq. miles";
			break;
		case 'n':	/* Nautical miles */
			name = (arc) ? "nautical miles" : "sq. nautical miles";
			break;
		case 'd':	/* Degrees */
			name = (arc) ? "degrees" : "steradians";
			break;
		case 'f':	/* Feet */
			name = (arc) ? "feet" : "sq. ft.";
			break;
		default:
			name = (arc) ? "m" : "m^2";
			break;
	}
	return (name);
}

void *New_sphtriangulate_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct SPHTRIANGULATE_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct SPHTRIANGULATE_CTRL);
	C->L.unit = 'e';	/* Default is meter distances */
	
	return ((void *)C);
}

void Free_sphtriangulate_Ctrl (struct GMT_CTRL *GMT, struct SPHTRIANGULATE_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->G.file) free ((void *)C->G.file);	
	if (C->N.file) free ((void *)C->N.file);	
	GMT_free (GMT, C);	
}

GMT_LONG GMT_sphtriangulate_usage (struct GMTAPI_CTRL *C, GMT_LONG level)
{
	struct GMT_CTRL *GMT = C->GMT;
	GMT_message (GMT, "sphtriangulate %s - Delaunay or Voronoi construction of spherical lon,lat data\n\n", GMT_VERSION);
	GMT_message (GMT, "==> The hard work is done by algorithms 772 (STRIPACK) & 773 (SSRFPACK) by R. J. Renka [1997] <==\n\n");
	GMT_message (GMT, "usage: sphtriangulate [<table>] [-A] [-C] [-D] [-L<unit>] [-N<nodetable>]\n");
	GMT_message (GMT, "\t[-Qd|v] [-T] [-V] [%s] [%s] [%s]\n", GMT_b_OPT, GMT_h_OPT, GMT_i_OPT);
	GMT_message (GMT, "\t[%s]\n\n", GMT_colon_OPT);
               
	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);
               
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t<table> is one or more data file (in ASCII, binary, netCDF) with (x,y,z[,w]).\n");
	GMT_message (GMT, "\t   If no files are given, standard input is read.\n");
	GMT_message (GMT, "\t-A Compute and print triangle or polygon areas in header records (see -L for units).\n");
	GMT_message (GMT, "\t   If -T is selected we print arc lengths instead.\n");
	GMT_message (GMT, "\t   Cannot be used with the binary output option.\n");
	GMT_message (GMT, "\t-C Conserve memory (Converts lon/lat <--> x/y/z when needed) [store both in memory].\n");
	GMT_message (GMT, "\t-D Skip repeated input vertex at the end of a closed segment.\n");
	GMT_message (GMT, "\t-L Set distance unit arc (d)egree, m(e)ter, (f)eet, (k)m, arc (m)inute, (M)ile, (n)autical mile, or arc (s)econd [e].\n");
	GMT_message (GMT, "\t   PROJ_ELLIPSOID determines if geodesic or gerat-circle distances are used.\n");
	GMT_message (GMT, "\t-N Output filename for Delaunay or Voronoi polygon information [Store in output segment headers].\n");
	GMT_message (GMT, "\t   Delaunay: output is the node triplets and area (i, j, k, area).\n");
	GMT_message (GMT, "\t   Voronoi: output is the node coordinates and polygon area (lon, lat, area).\n");
	GMT_message (GMT, "\t   Cannot be used with -T.\n");
	GMT_message (GMT, "\t-Q Append d for Delaunay triangles or v for Voronoi polygons [Delaunay].\n");
	GMT_message (GMT, "\t   If -bo is used then -N may be used to specify a separate file where the\n");
	GMT_message (GMT, "\t   polygon information normally is written.\n");
	GMT_message (GMT, "\t-T Write arcs [Default writes polygons].\n");
	GMT_explain_options (GMT, "VC2D0hi:.");

	return (EXIT_FAILURE);
}

GMT_LONG GMT_sphtriangulate_parse (struct GMTAPI_CTRL *C, struct SPHTRIANGULATE_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to sphtriangulate and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	GMT_LONG n_errors = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Skip input files */
				break;

			/* Processes program-specific parameters */

			case 'A':
				Ctrl->A.active = TRUE;
				break;
			case 'C':
				Ctrl->C.active = TRUE;
				break;
			case 'D':
				Ctrl->D.active = TRUE;
				break;
			case 'L':
				Ctrl->L.active = TRUE;
				if (!(opt->arg && strchr (GMT_LEN_UNITS, opt->arg[0]))) {
					GMT_report (GMT, GMT_MSG_FATAL, "Syntax error: Expected -L%s\n", GMT_LEN_UNITS_DISPLAY);
					n_errors++;
				}
				else
					Ctrl->L.unit = toupper (opt->arg[0]);
				break;
			case 'N':
				Ctrl->N.active = TRUE;
				Ctrl->N.file = strdup (opt->arg);
				break;
			case 'Q':
				Ctrl->Q.active = TRUE;
				Ctrl->Q.mode = (opt->arg[0] == 'v') ? VORONOI : DELAUNAY;
				break;
			case 'T':
				Ctrl->T.active = TRUE;
				break;
			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}
	
	if (GMT->common.b.active[GMT_IN] && GMT->common.b.ncol[GMT_IN] == 0) GMT->common.b.ncol[GMT_IN] = 3;
	n_errors += GMT_check_condition (GMT, GMT->common.b.active[GMT_IN] && GMT->common.b.ncol[GMT_IN] < 3, "Syntax error: Binary input data (-bi) must have at least 3 columns\n");
	n_errors += GMT_check_condition (GMT, GMT->common.b.active[GMT_OUT] && Ctrl->A.active && !Ctrl->N.active, "Syntax error: Binary output does not support storing areas unless -N is used\n");
	n_errors += GMT_check_condition (GMT, Ctrl->N.active && Ctrl->T.active, "Syntax error -N: Cannot be used with -T.\n");
	n_errors += GMT_check_condition (GMT, Ctrl->N.active && !Ctrl->N.file, "Syntax error -N: Must specify output file\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define Return(code) {Free_sphtriangulate_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); return (code);}

GMT_LONG GMT_sphtriangulate (struct GMTAPI_CTRL *API, struct GMT_OPTION *options)
{
	char *mode[2] = {"Delaunay", "Voronoi"}, header[GMT_BUFSIZ];

	GMT_LONG n = 0, n_alloc, n_dup = 0, n_fields;
	GMT_LONG error = FALSE, first = FALSE, steradians = FALSE;

	double first_x = 0.0, first_y = 0.0, X[3], *in = NULL;
	double *xx = NULL, *yy = NULL, *zz = NULL, *lon = NULL, *lat = NULL;

	struct SPHTRIANGULATE_CTRL *Ctrl = NULL;
	struct STRIPACK T;
	struct GMT_DATASET **Dout = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));

	if (!options || options->option == GMTAPI_OPT_USAGE) return (GMT_sphtriangulate_usage (API, GMTAPI_USAGE));/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) return (GMT_sphtriangulate_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_module (API, "GMT_sphtriangulate", &GMT_cpy);			/* Save current state */
	if ((error = GMT_Parse_Common (API, "-VRb:", "hims", options))) Return (error);
	Ctrl = (struct SPHTRIANGULATE_CTRL *) New_sphtriangulate_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_sphtriangulate_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the sphtriangulate main code ----------------------------*/

	GMT_init_distaz (GMT, Ctrl->L.unit, 1 + GMT_sph_mode (GMT), GMT_MAP_DIST);
	
	steradians = (Ctrl->L.unit == 'D');	/* Flag so we can do steradians */

	/* Now we are ready to take on some input values */

	if ((error = GMT_set_cols (GMT, GMT_IN, 2))) Return (error);
	if ((error = GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_IN, GMT_REG_DEFAULT, options))) Return (error);	/* Registers default input sources, unless already set */
	if ((error = GMT_Begin_IO (API, GMT_IS_DATASET, GMT_IN, GMT_BY_REC))) Return (error);				/* Enables data input and sets access mode */

	if (!Ctrl->C.active) (void)GMT_malloc2 (GMT, lon, lat, 0, 0, double);
	n_alloc = GMT_malloc3 (GMT, xx, yy, zz, 0, 0, double);
	
	n = 0;
	while ((n_fields = GMT_Get_Record (API, GMT_READ_DOUBLE, (void **)&in)) != EOF) {	/* Keep returning records until we reach EOF */

		if (GMT_REC_IS_ERROR (GMT)) Return (GMT_RUNTIME_ERROR);
		if (GMT_REC_IS_TBL_HEADER (GMT)) continue;	/* Skip table headers */

		while (GMT_REC_IS_SEG_HEADER (GMT)) {	/* Segment header, get next record */
			n_fields = GMT_Get_Record (API, GMT_READ_DOUBLE, (void **)&in);	
			first_x = in[GMT_X];	first_y = in[GMT_Y];
			first = TRUE;
		}
		if (Ctrl->D.active && !first) {	/* Look for duplicate point at end of segments that replicate start point */
			if (in[GMT_X] == first_x && in[GMT_Y] == first_y) {	/* If any point after the first matches the first */
				n_dup++;
				continue;
			}
		}
		/* Convert lon,lat in degrees to Cartesian x,y,z triplets */
		GMT_geo_to_cart (GMT, in[GMT_Y], in[GMT_X], X, TRUE);
		xx[n] = X[0];	yy[n] = X[1];	zz[n] = X[2];
		if (!Ctrl->C.active) {
			lon[n] = in[GMT_X];
			lat[n] = in[GMT_Y];
		}
		
		n++;
		if (n == n_alloc) {	/* Get more memory */
			if (!Ctrl->C.active) (void)GMT_malloc2 (GMT, lon, lat, n, n_alloc, double);
			n_alloc = GMT_malloc3 (GMT, xx, yy, zz, n, n_alloc, double);
		}
		first = FALSE;
	}
	if ((error = GMT_End_IO (API, GMT_IN, 0))) Return (error);				/* Disables further data input */

	if (!Ctrl->C.active) (void)GMT_malloc2 (GMT, lon, lat, 0, n, double);
	n_alloc = GMT_malloc3 (GMT, xx, yy, zz, 0, n, double);

	if (Ctrl->D.active && n_dup) GMT_report (GMT, GMT_MSG_NORMAL, "Skipped %ld duplicate points in segments\n", n_dup);
	GMT_report (GMT, GMT_MSG_NORMAL, "Do Voronoi construction using %ld points\n", n);

	GMT_memset (&T, 1, struct STRIPACK);
	T.mode = (int)Ctrl->Q.mode;
	stripack_lists (GMT, n, xx, yy, zz, &T);	/* Do the basic triangulation */
	if (Ctrl->C.active) {	/* Recompute lon,lat and set pointers */
		cart_to_geo (GMT, n, xx, yy, zz, xx, yy);	/* Revert to lon, lat */
		lon = xx;
		lat = yy;
	}
	GMT_free (GMT,  zz);
	
	if (Ctrl->Q.mode == VORONOI) {
		stripack_voronoi_output (GMT, n, lon, lat, &T.V, Ctrl->T.active, Ctrl->A.active + steradians, Ctrl->N.active, &Dout);
		GMT_free (GMT, T.V.lon);
		GMT_free (GMT, T.V.lat);
		GMT_free (GMT, T.V.lend);
		GMT_free (GMT, T.V.listc);
		GMT_free (GMT, T.V.lptr);
	}
	else {
		stripack_delaunay_output (GMT, lon, lat, &T.D, Ctrl->T.active, Ctrl->A.active + steradians, Ctrl->N.active, &Dout);
	}
	Dout[0]->table[0]->header = GMT_memory (GMT, NULL, 1, char *);
	sprintf (header, "# sphtriangulate %s output via STRPACK.", mode[Ctrl->Q.mode]);
	if (Ctrl->T.active)
		strcat (header, "  Arc lengths in ");
	else
		strcat (header, "  Areas in ");
	strcat (header, unit_name (Ctrl->L.unit, Ctrl->T.active));
	Dout[0]->table[0]->n_headers = 1;
	Dout[0]->table[0]->header[0] = strdup (header);
	
	if ((error = GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_OUT, GMT_REG_DEFAULT, options))) Return (error);	/* Registers default output sources, unless already set */
	if ((error = GMT_Begin_IO (API, GMT_IS_DATASET, GMT_OUT, GMT_BY_SET))) Return (error);					/* Enables data output and sets access mode */

	if (Ctrl->T.active) GMT->current.io.multi_segments[GMT_OUT] = TRUE;	/* Must produce multisegment output files */
	
	if ((error = GMT_Put_Data (API, GMT_IS_DATASET, GMT_IS_FILE, GMT_IS_POINT, NULL, Dout[0]->io_mode, NULL, (void *)Dout[0]))) Return (error);
	if (Ctrl->N.active) {
		sprintf (header, "# sphtriangulate nodes output\n");
		Dout[1]->table[0]->header = GMT_memory (GMT, NULL, 1, char *);
		Dout[1]->table[0]->n_headers = 1;
		Dout[1]->table[0]->header[0] = strdup (header);
		if ((error = GMT_Put_Data (API, GMT_IS_DATASET, GMT_IS_FILE, GMT_IS_POINT, NULL, Dout[1]->io_mode, (void **)&Ctrl->N.file, (void *)Dout[1]))) Return (error);
	}
	
	GMT_free (GMT, T.D.tri);
	if (!Ctrl->C.active) {
		GMT_free (GMT, lon);
		GMT_free (GMT, lat);
	}
	GMT_free (GMT, xx);
	GMT_free (GMT, yy);

	GMT_report (GMT, GMT_MSG_NORMAL, "Triangularization completed\n");

	Return (GMT_OK);
}
