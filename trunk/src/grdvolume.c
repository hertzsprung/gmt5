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
 * Brief synopsis: grdvolume reads a 2d binary gridded grid file, and calculates the volume
 * under the surface using exact integration of the bilinear interpolating
 * surface.  As an option, the user may supply a contour value; then the
 * volume is only integrated inside the chosen contour.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#define THIS_MODULE k_mod_grdvolume /* I am grdvolume */

#include "gmt_dev.h"

#define GMT_PROG_OPTIONS "-RVfho"

struct GRDVOLUME_CTRL {
	struct In {
		bool active;
		char *file;
	} In;
	struct C {	/* -C */
		bool active;
		double low, high, inc;
	} C;
	struct L {	/* -L<base> */
		bool active;
		double value;
	} L;
	struct S {	/* -S */
		bool active;
		char unit;
	} S;
	struct T {	/* -T[c|z] */
		bool active;
		unsigned int mode;
	} T;
	struct Z {	/* Z<fact>[/<shift>] */
		bool active;
		double scale, offset;
	} Z;
};

/* This function returns the volume bounded by a trapezoid based on two vertical
 * lines x0 and x1 and two horizontal lines y0 = ax +b and y1 = cx + d
 */

double vol_prism_frac_x (struct GMT_GRID *G, uint64_t ij, double x0, double x1, double a, double b, double c, double d)
{
	double dzdx, dzdy, dzdxy, ca, db, c2a2, d2b2, cdab, v, x02, x12, x03, x04, x13, x14;

	dzdx  = (G->data[ij+1] - G->data[ij]);
	dzdy  = (G->data[ij-G->header->mx] - G->data[ij]);
	dzdxy = (G->data[ij-G->header->mx+1] + G->data[ij] - G->data[ij+1] - G->data[ij-G->header->mx]);

	ca = c - a;
	db = d - b;
	c2a2 = c * c - a * a;
	d2b2 = d * d - b * b;
	cdab = c * d - a * b;
	x02 = x0 * x0;	x03 = x02 * x0;	x04 = x02 * x02;
	x12 = x1 * x1;	x13 = x12 * x1;	x14 = x12 * x12;

	v = (3.0 * dzdxy * c2a2 * (x14 - x04) +
	     4.0 * (2.0 * dzdx * ca + dzdy * c2a2 + 2.0 * dzdxy * cdab) * (x13 - x03) +
	     6.0 * (2.0 * G->data[ij] * ca + 2.0 * dzdx * db + 2.0 * dzdy * cdab + dzdxy * d2b2) * (x12 - x02) +
	     12.0 * (2.0 * G->data[ij] * db + dzdy * d2b2) * (x1 - x0)) / 24.0;

	return (v);
}

/* This function returns the volume bounded by a trapezoid based on two horizontal
 * lines y0 and y1 and two vertical lines x0 = ay +b and x1 = cy + d
 */

double vol_prism_frac_y (struct GMT_GRID *G, uint64_t ij, double y0, double y1, double a, double b, double c, double d)
{
	double dzdx, dzdy, dzdxy, ca, db, c2a2, d2b2, cdab, v, y02, y03, y04, y12, y13, y14;

	dzdx = (G->data[ij+1] - G->data[ij]);
	dzdy = (G->data[ij-G->header->mx] - G->data[ij]);
	dzdxy = (G->data[ij-G->header->mx+1] + G->data[ij] - G->data[ij+1] - G->data[ij-G->header->mx]);

	ca = c - a;
	db = d - b;
	c2a2 = c * c - a * a;
	d2b2 = d * d - b * b;
	cdab = c * d - a * b;
	y02 = y0 * y0;	y03 = y02 * y0;	y04 = y02 * y02;
	y12 = y1 * y1;	y13 = y12 * y1;	y14 = y12 * y12;

	v = (3.0 * dzdxy * c2a2 * (y14 - y04) +
	     4.0 * (2.0 * dzdy * ca + dzdx * c2a2 + 2.0 * dzdxy * cdab) * (y13 - y03) +
	     6.0 * (2.0 * G->data[ij] * ca + 2.0 * dzdy * db + 2.0 * dzdx * cdab + dzdxy * d2b2) * (y12 - y02) +
	     12.0 * (2.0 * G->data[ij] * db + dzdx * d2b2) * (y1 - y0)) / 24.0;

	return (v);
}

void SW_triangle (struct GMT_GRID *G, uint64_t ij, bool triangle, double *dv, double *da)
{	/* Calculates area of a SW-corner triangle */
	/* triangle = true gets triangle, false gives the complementary area */
	double x1, y0, frac;

	x1 = G->data[ij] / (G->data[ij] - G->data[ij+1]);
	y0 = G->data[ij] / (G->data[ij] - G->data[ij-G->header->mx]);
	frac = (x1 == 0.0) ? 0.0 : vol_prism_frac_x (G, ij, 0.0, x1, 0.0, 0.0, -y0 / x1, y0);
	if (triangle) {
		*dv += frac;
		*da += 0.5 * x1 * y0;
	}
	else {
		*dv += 0.25 * (G->data[ij] + G->data[ij+1] + G->data[ij-G->header->mx] + G->data[ij-G->header->mx+1]) - frac;
		*da += 1.0 - 0.5 * x1 * y0;
	}
}

void NE_triangle (struct GMT_GRID *G, uint64_t ij, bool triangle, double *dv, double *da)
{	/* Calculates area of a NE-corner triangle */
	/* triangle = true gets triangle, false gives the complementary area */
	double x0, y1, a, x0_1, y1_1, frac = 0.0;

	x0 = G->data[ij-G->header->mx] / (G->data[ij-G->header->mx] - G->data[ij+1-G->header->mx]);
	y1 = G->data[ij+1] / (G->data[ij+1] - G->data[ij+1-G->header->mx]);
	x0_1 = 1.0 - x0;
	y1_1 = y1 - 1.0;
	if (x0_1 != 0.0) {
		a = y1_1 / x0_1;
		frac = vol_prism_frac_x (G, ij, x0, 1.0, a, 1.0 - a * x0, 0.0, 0.0);
	}
	if (triangle) {
		*dv += frac;
		*da -= 0.5 * x0_1 * y1_1;	/* -ve because we need 1 - y1 */
	}
	else {
		*dv += 0.25 * (G->data[ij] + G->data[ij+1] + G->data[ij-G->header->mx] + G->data[ij-G->header->mx+1]) - frac;
		*da += 1.0 + 0.5 * x0_1 * y1_1;	/* +ve because we need 1 - y1 */
	}
}

void SE_triangle (struct GMT_GRID *G, uint64_t ij, bool triangle, double *dv, double *da)
{	/* Calculates area of a SE-corner triangle */
	/* triangle = true gets triangle, false gives the complementary area */
	double x0, y1, c, x0_1, frac = 0.0;

	x0 = G->data[ij] / (G->data[ij] - G->data[ij+1]);
	y1 = G->data[ij+1] / (G->data[ij+1] - G->data[ij+1-G->header->mx]);
	x0_1 = 1.0 - x0;
	if (x0_1 != 0.0) {
		c = y1 / x0_1;
		frac = vol_prism_frac_x (G, ij, x0, 1.0, 0.0, 0.0, c, -c * x0);
	}
	if (triangle) {
		*dv += frac;
		*da += 0.5 * x0_1 * y1;
	}
	else {
		*dv += 0.25 * (G->data[ij] + G->data[ij+1] + G->data[ij-G->header->mx] + G->data[ij-G->header->mx+1]) - frac;
		*da += 1.0 - 0.5 * x0_1 * y1;
	}
}

void NW_triangle (struct GMT_GRID *G, uint64_t ij, bool triangle, double *dv, double *da)
{	/* Calculates area of a NW-corner triangle */
	/* triangle = true gets triangle, false gives the complementary area */
	double x1, y0, y0_1, frac;

	x1 = G->data[ij-G->header->mx] / (G->data[ij-G->header->mx] - G->data[ij+1-G->header->mx]);
	y0 = G->data[ij] / (G->data[ij] - G->data[ij-G->header->mx]);
	y0_1 = 1.0 - y0;
	frac = (x1 == 0.0) ? 0.0 : vol_prism_frac_x (G, ij, 0.0, x1, y0_1 / x1, y0, 0.0, 1.0);
	if (triangle) {
		*dv += frac;
		*da += 0.5 * x1 * y0_1;
	}
	else {
		*dv += 0.25 * (G->data[ij] + G->data[ij+1] + G->data[ij-G->header->mx] + G->data[ij-G->header->mx+1]) - frac;
		*da += 1.0 - 0.5 * x1 * y0_1;
	}
}

void NS_trapezoid (struct GMT_GRID *G, uint64_t ij, bool right, double *dv, double *da)
{	/* Calculates area of a NS trapezoid */
	/* right = true gets the right trapezoid, false gets the left */
	double x0, x1;

	x0 = G->data[ij] / (G->data[ij] - G->data[ij+1]);
	x1 = G->data[ij-G->header->mx] / (G->data[ij-G->header->mx] - G->data[ij+1-G->header->mx]);
	if (right) {	/* Need right piece */
		*dv += vol_prism_frac_y (G, ij, 0.0, 1.0, x1 - x0, x0, 0.0, 1.0);
		*da += 0.5 * (2.0 - x0 - x1);
	}
	else {
		*dv += vol_prism_frac_y (G, ij, 0.0, 1.0, 0.0, 0.0, x1 - x0, x0);
		*da += 0.5 * (x0 + x1);
	}
}

void EW_trapezoid (struct GMT_GRID *G, uint64_t ij, bool top, double *dv, double *da)
{	/* Calculates area of a EW trapezoid */
	/* top = true gets the top trapezoid, false gets the bottom */
	double y0, y1;

	y0 = G->data[ij] / (G->data[ij] - G->data[ij-G->header->mx]);
	y1 = G->data[ij+1] / (G->data[ij+1] - G->data[ij+1-G->header->mx]);
	if (top) {	/* Need top piece */
		*dv += vol_prism_frac_x (G, ij, 0.0, 1.0, y1 - y0, y0, 0.0, 1.0);
		*da += 0.5 * (2.0 - y0 - y1);
	}
	else {
		*dv += vol_prism_frac_x (G, ij, 0.0, 1.0, 0.0, 0.0, y1 - y0, y0);
		*da += 0.5 * (y0 + y1);
	}
}

double median3 (double x[])
{	/* Returns the median of the three points in x */
	if (x[0] < x[1]) {
		if (x[2] > x[1]) return (x[1]);
		if (x[2] > x[0]) return (x[2]);
		return (x[0]);
	}
	else {
		if (x[2] > x[0]) return (x[0]);
		if (x[2] < x[1]) return (x[1]);
		return (x[2]);
	}
}

int ors_find_kink (struct GMT_CTRL *GMT, double y[], unsigned int n, unsigned int mode)
{	/* mode: 0 = find value maximum, 1 = find curvature maximum */
	unsigned int i, im;
	double *c = NULL, *f = NULL;

	if (mode == 0) {	/* Find maximum value */
		for (i = im = 0; i < n; i++) if (y[i] > y[im]) im = i;
		return (im);
	}

	/* Calculate curvatures */

	c = GMT_memory (GMT, NULL, n, double);

	for (i = 1; i < (n-1); i++) c[i] = y[i+1] - 2.0 * y[i] + y[i-1];
	c[0] = c[1];
	if (n > 1) c[n-1] = c[n-2];

	/* Apply 3-point median filter to curvatures to mitigate noisy values */

	f = GMT_memory (GMT, NULL, n, double);
	for (i = 1; i < (n-1); i++) f[i] = median3 (&c[i-1]);

	/* Find maximum negative filtered curvature */

	for (i = im = 1; i < (n-1); i++) if (f[i] < f[im]) im = i;

	GMT_free (GMT, c);
	GMT_free (GMT, f);

	return (im);
}

void *New_grdvolume_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDVOLUME_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct GRDVOLUME_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
	C->L.value = GMT->session.d_NaN;
	C->Z.scale = 1.0;
	return (C);
}

void Free_grdvolume_Ctrl (struct GMT_CTRL *GMT, struct GRDVOLUME_CTRL *C) {	/* Deallocate control structure */
	if (C->In.file) free (C->In.file);	
	GMT_free (GMT, C);	
}

int GMT_grdvolume_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: grdvolume <ingrid> [-C<cval> or -C<low>/<high>/<delta>] [-L<base>] [-S<unit>]\n");
	GMT_message (GMT, "\t[-T[c|h]]\n\t[%s] [%s] [-Z<fact>[/<shift>]] [%s] [%s] [%s]\n",
		GMT_Rgeo_OPT, GMT_V_OPT, GMT_f_OPT, GMT_ho_OPT, GMT_o_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<ingrid> is the name of the grid file.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t-C Find area, volume, and mean height inside the given <cval> contour,\n");
	GMT_message (GMT, "\t   OR search using all contours from <low> to <high> in steps of <delta>.\n");
	GMT_message (GMT, "\t   [Default returns area, volume and mean height of entire grid].\n");
	GMT_message (GMT, "\t-L Add volume from <base> up to contour [Default is from contour and up only].\n");
	GMT_message (GMT, "\t-S Convert degrees to distances, append a unit from %s [Default is Cartesian].\n", GMT_LEN_UNITS2_DISPLAY);
	GMT_message (GMT, "\t-T (or -Th): Find the contour value that yields max average height (volume/area).\n");
	GMT_message (GMT, "\t   Use -Tc to find contour that yields the max curvature of height vs contour.\n");
	GMT_explain_options (GMT, "RV");
	GMT_message (GMT, "\t-Z Subtract <shift> and then multiply data by <fact> before processing [1/0].\n");
	GMT_explain_options (GMT, "fho.");
	
	return (EXIT_FAILURE);
}

int GMT_grdvolume_parse (struct GMTAPI_CTRL *C, struct GRDVOLUME_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to grdvolume and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_files = 0;
	int n = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {
			case '<':	/* Input file (only one is accepted) */
				Ctrl->In.active = true;
				if (n_files++ == 0) Ctrl->In.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'C':
				Ctrl->C.active = true;
				n = sscanf (opt->arg, "%lf/%lf/%lf", &Ctrl->C.low, &Ctrl->C.high, &Ctrl->C.inc);
				if (n == 3) {
					n_errors += GMT_check_condition (GMT, Ctrl->C.low >= Ctrl->C.high || Ctrl->C.inc <= 0.0, "Syntax error -C option: high must exceed low and delta must be positive\n");
				}
				else
					Ctrl->C.high = Ctrl->C.low, Ctrl->C.inc = 1.0;	/* So calculation of ncontours will yield 1 */
				break;
			case 'L':
				Ctrl->L.active = true;
				if (opt->arg[0]) Ctrl->L.value = atof (opt->arg);
				break;
			case 'S':
				Ctrl->S.active = true;
				Ctrl->S.unit = opt->arg[0];
				break;
			case 'T':
				Ctrl->T.active = true;
				switch (opt->arg[0]) {
					case 'c':
						Ctrl->T.mode = 1;	/* Find maximum in height curvatures */
						break;
					case '\0':
					case 'h':
						Ctrl->T.mode = 0;	/* Find maximum in height values */
						break;
					default:
						n_errors++;
						GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -T option: Append c or h [Default].\n");
				}
				break;
			case 'Z':
				Ctrl->Z.active = true;
				n_errors += GMT_check_condition (GMT, sscanf (opt->arg, "%lf/%lf", &Ctrl->Z.scale, &Ctrl->Z.offset) < 1, "Syntax error option -Z: Must specify <fact> and optionally <shift>\n");
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, !Ctrl->In.file, "Syntax error: Must specify input grid file\n");
	n_errors += GMT_check_condition (GMT, Ctrl->C.active && !(n == 1 || n == 3), "Syntax error option -C: Must specify 1 or 3 arguments\n");
	n_errors += GMT_check_condition (GMT, Ctrl->S.active && !(strchr (GMT_LEN_UNITS2, Ctrl->S.unit)), "Syntax error option -S: Must append one of %s\n", GMT_LEN_UNITS2_DISPLAY);
	n_errors += GMT_check_condition (GMT, Ctrl->L.active && GMT_is_dnan (Ctrl->L.value), "Syntax error option -L: Must specify base\n");
	n_errors += GMT_check_condition (GMT, Ctrl->T.active && !Ctrl->C.active, "Syntax error option -T: Must also specify -Clow/high/delta\n");
	n_errors += GMT_check_condition (GMT, Ctrl->T.active && Ctrl->C.active && doubleAlmostEqualZero (Ctrl->C.high, Ctrl->C.low), "Syntax error option -T: Must specify -Clow/high/delta\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grdvolume_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grdvolume (void *V_API, int mode, void *args)
{
	bool bad, cut[4];
	int error = 0, ij_inc[5];
	unsigned int row, col, c, k, pos, neg, nc, n_contours;
	
	uint64_t ij;

	double take_out, dv, da, cval = 0.0, cellsize, fact, dist_pr_deg, sum, out[4];
	double *area = NULL, *vol = NULL, *height = NULL, this_base, small, wesn[4];

	struct GRDVOLUME_CTRL *Ctrl = NULL;
	struct GMT_GRID *Grid = NULL, *Work = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_NOT_A_SESSION);
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdvolume_usage (API, GMTAPI_USAGE));/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdvolume_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, GMT_PROG_OPTIONS, options)) Return (API->error);
	Ctrl = New_grdvolume_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdvolume_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the grdvolume main code ----------------------------*/

	GMT_report (GMT, GMT_MSG_VERBOSE, "Processing input grid\n");
	if ((Grid = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER_ONLY, NULL, Ctrl->In.file, NULL)) == NULL) {	/* Get header only */
		Return (API->error);
	}
	if (Ctrl->L.active && Ctrl->L.value >= Grid->header->z_min) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Selected base value exceeds the minimum grid z value - aborting\n");
		Return (EXIT_FAILURE);
	}
	
	if (!GMT->common.R.active) GMT_memcpy (GMT->common.R.wesn, Grid->header->wesn, 4, double);	/* No -R, use grid domain */
	GMT_memcpy (wesn, GMT->common.R.wesn, 4, double);

	if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA_ONLY, wesn, Ctrl->In.file, Grid) == NULL) {
		Return (API->error);
	}

	(void) GMT_set_outgrid (GMT, Ctrl->In.file, Grid, &Work);	/* true if input is a read-only array */
	GMT_grd_init (GMT, Work->header, options, true);

	/* Set node increments relative to the lower-left node of a 4-point box */
	GMT_grd_set_ij_inc (GMT, Work->header->mx, ij_inc);
	ij_inc[4] = ij_inc[0];	/* Repeat for convenience */
	cellsize = Work->header->inc[GMT_X] * Work->header->inc[GMT_Y];
	if (Ctrl->S.active) {
		GMT_init_distaz (GMT, Ctrl->S.unit, 1, GMT_MAP_DIST);	/* Flat Earth mode */
		dist_pr_deg = GMT->current.proj.DIST_M_PR_DEG;
		dist_pr_deg *= GMT->current.map.dist[GMT_MAP_DIST].scale;	/* Scales meters to desired unit */
		cellsize *= dist_pr_deg * dist_pr_deg;
	}

	n_contours = (Ctrl->C.active) ? lrint ((Ctrl->C.high - Ctrl->C.low) / Ctrl->C.inc) + 1 : 1;

	height = GMT_memory (GMT, NULL, n_contours, double);
	vol    = GMT_memory (GMT, NULL, n_contours, double);
	area   = GMT_memory (GMT, NULL, n_contours, double);

	if (!(Ctrl->Z.scale == 1.0 && Ctrl->Z.offset == 0.0)) {
		GMT_report (GMT, GMT_MSG_VERBOSE, "Subtracting %g and multiplying by %g\n", Ctrl->Z.offset, Ctrl->Z.scale);
		GMT_scale_and_offset_f (GMT, Work->data, Work->header->size, Ctrl->Z.scale, Ctrl->Z.offset);
		Work->header->z_min = (Work->header->z_min - Ctrl->Z.offset) * Ctrl->Z.scale;
		Work->header->z_max = (Work->header->z_max - Ctrl->Z.offset) * Ctrl->Z.scale;
		if (Ctrl->Z.scale < 0.0) double_swap (Work->header->z_min, Work->header->z_max);
	}

	this_base = (Ctrl->L.active) ? Ctrl->L.value : 0.0;
	small = Ctrl->C.inc * 1.0e-6;

	for (c = 0; Ctrl->C.active && c < n_contours; c++) {	/* Trace contour, only count volumes inside contours */

		cval = Ctrl->C.low + c * Ctrl->C.inc;
		take_out = (c == 0) ? cval : Ctrl->C.inc;	/* Take out start contour the first time and just the increment subsequent times */

		GMT_report (GMT, GMT_MSG_VERBOSE, "Compute volume, area, and average height for contour = %g\n", cval);
		
		for (ij = 0; ij < Work->header->size; ij++) {
			Work->data[ij] -= (float)take_out;		/* Take out the zero value */
			if (Work->data[ij] == 0.0) Work->data[ij] = (float)small;	/* But we dont want exactly zero, just + or - */
		}
		if (Ctrl->L.active) this_base -= take_out;

		if (Ctrl->L.active && this_base >= 0.0) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Base exceeds the current contour value - contour is ignored.\n");
			continue;
		}

		for (row = 1; row < Work->header->ny; row++) {

			dv = da = 0.0;	/* Reset these for each row */

			for (col = 0, ij = GMT_IJP (Work->header, row, 0); col < (Work->header->nx-1); col++, ij++) {

				/* Find if a contour goes through this bin */

				for (k = neg = pos = 0, bad = false; !bad && k < 4; k++) {
					(Work->data[ij+ij_inc[k]] <= (float)small) ? neg++ : pos++;
					if (GMT_is_fnan (Work->data[ij+ij_inc[k]])) bad = true;
				}

				if (bad || neg == 4) continue;	/* Contour not crossing, go to next bin */

				if (pos == 4) {	/* Need entire prism */
					for (k = 0, sum = 0.0; k < 4; k++) sum += Work->data[ij+ij_inc[k]];
					dv += 0.25 * sum;
					da += 1.0;
				}
				else {	/* Need partial prisms */

					for (k = nc = 0; k < 4; k++) {	/* Check the 4 sides for crossings */
						cut[k] = false;
						if ((Work->data[ij+ij_inc[k]] * Work->data[ij+ij_inc[k+1]]) < 0.0) nc++, cut[k] = true;	/* Crossing this border */
					}
					if (nc < 2) continue;	/* Can happen if some nodes were 0 and then reset to small, thus passing the test */

					if (nc == 4) {	/* Saddle scenario */
						if (Work->data[ij] > 0.0) {	/* Need both SW and NE triangles */
							SW_triangle (Work, ij, true, &dv, &da);
							NE_triangle (Work, ij, true, &dv, &da);
						}
						else {			/* Need both SE and NW corners */
							SE_triangle (Work, ij, true, &dv, &da);
							NW_triangle (Work, ij, true, &dv, &da);
						}

					}
					else if (cut[0]) {	/* Contour enters at S border ... */
						if (cut[1])	/* and exits at E border */
							SE_triangle (Work, ij, (Work->data[ij+ij_inc[1]] > 0.0), &dv, &da);
						else if (cut[2])	/* or exits at N border */
							NS_trapezoid (Work, ij, Work->data[ij] < 0.0, &dv, &da);
						else			/* or exits at W border */
							SW_triangle (Work, ij, (Work->data[ij] > 0.0), &dv, &da);
					}
					else if (cut[1]) {	/* Contour enters at E border */
						if (cut[2])	/* exits at N border */
							NE_triangle (Work, ij, (Work->data[ij+ij_inc[2]] > 0.0), &dv, &da);
						else			/* or exits at W border */
							EW_trapezoid (Work, ij, Work->data[ij] < 0.0, &dv, &da);
					}
					else			/* Contours enters at N border and exits at W */
						NW_triangle (Work, ij, (Work->data[ij+ij_inc[3]] > 0.0), &dv, &da);
				}
			}
			ij++;

			fact = cellsize;
			/* Allow for shrinking of longitudes with latitude */
			if (Ctrl->S.active) fact *= cosd (Work->header->wesn[YHI] - (row+0.5) * Work->header->inc[GMT_Y]);

			vol[c]  += dv * fact;
			area[c] += da * fact;
		}

		/* Adjust for lower starting base */
		if (Ctrl->L.active) vol[c] -= area[c] * this_base;
	}
	if (!Ctrl->C.active) {	/* Since no contours we can use columns with bilinear tops to get the volume */
		for (row = 0; row < Work->header->ny; row++) {
			dv = da = 0.0;
			for (col = 0, ij = GMT_IJP (Work->header, row, 0); col < Work->header->nx; col++, ij++) {
				if (GMT_is_fnan (Work->data[ij])) continue;

				/* Half the leftmost and rightmost cell */
				if (Work->header->registration == GMT_GRID_NODE_REG && (col == 0 || col == Work->header->nx-1)) {
					dv += 0.5 * Work->data[ij];
					da += 0.5;
				}
				else {
					dv += Work->data[ij];
					da += 1.0;
				}
			}

			fact = cellsize;
			/* Allow for shrinking of longitudes with latitude */
			if (Ctrl->S.active) fact *= cosd (Work->header->wesn[YHI] - row * Work->header->inc[GMT_Y]);
			/* Half the top and bottom row */
			if (Work->header->registration == GMT_GRID_NODE_REG && (row == 0 || row == Work->header->ny-1)) fact *= 0.5;

			vol[0]  += dv * fact;
			area[0] += da * fact;
		}

		/* Adjust for lower starting base */
		if (Ctrl->L.active) vol[0] -= area[0] * this_base;
	}

	/* Compute average heights */

	for (c = 0; c < n_contours; c++) height[c] = (area[c] > 0.0) ? vol[c] / area[c] : GMT->session.d_NaN;

	/* Find the best contour that gives largest height */

	/* Print out final estimates */

	if ((error = GMT_set_cols (GMT, GMT_OUT, 4)) != GMT_OK) {
		Return (error);
	}
	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_NONE, GMT_OUT, GMT_ADD_DEFAULT, 0, options) != GMT_OK) {	/* Registers default output destination, unless already set */
		Return (API->error);
	}
	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_OUT, GMT_HEADER_ON) != GMT_OK) {	/* Enables data output and sets access mode */
		Return (API->error);
	}

	if (Ctrl->T.active) {	/* Determine the best contour value and return the corresponding information for that contour only */
		c = ors_find_kink (GMT, height, n_contours, Ctrl->T.mode);
		out[0] = Ctrl->C.low + c * Ctrl->C.inc;	out[1] = area[c];	out[2] = vol[c];	out[3] = height[c];
		GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);	/* Write this to output */
	}
	else {			/* Return information for all contours (possibly one if -C<val> was used) */
		for (c = 0; c < n_contours; c++) {
			out[0] = Ctrl->C.low + c * Ctrl->C.inc;	out[1] = area[c];	out[2] = vol[c];	out[3] = height[c];
			GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);	/* Write this to output */
		}
	}
	if (GMT_End_IO (API, GMT_OUT, 0) != GMT_OK) {	/* Disables further data output */
		Return (API->error);
	}

	GMT_free (GMT, area);
	GMT_free (GMT, vol);
	GMT_free (GMT, height);

	Return (EXIT_SUCCESS);
}
