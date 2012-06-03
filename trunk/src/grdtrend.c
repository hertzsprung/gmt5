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
 * Brief synopsis: Reads a grid file and fits a trend surface.  Trend surface
 * is defined by:
 * 
 * m1 +m2*x + m3*y + m4*xy + m5*x*x + m6*y*y + m7*x*x*x
 * 	+ m8*x*x*y + m9*x*y*y + m10*y*y*y.
 * 
 * n_model is set by the user to be an integer in [1,10]
 * which sets the number of model coefficients to fit.
 * 
 * Author:		W. H. F. Smith
 * Date:		1 JAN, 2010
 * Version:	5 API
 * 
 * Explanations:
 * 
 * Thus:
 * n_model = 1 gives the mean value of the surface,
 * n_model = 3 fits a plane,
 * n_model = 4 fits a bilinear surface,
 * n_model = 6 fits a biquadratic,
 * n_model = 10 fits a bicubic surface.
 * 
 * The user may write out grid files of the fitted surface
 * [-T<trend.grd>] and / or of the residuals (input data
 * minus fitted trend) [-D<differences.grd] and / or of
 * the weights used in iterative fitting [-W<weight.grd].
 * This last option applies only when the surface is fit
 * iteratively [-N<n>[r]].
 * 
 * A robust fit may be achieved by iterative fitting of
 * a weighted least squares problem, where the weights
 * are set according to a scale length based on the 
 * Median absolute deviation (MAD: Huber, 1982).  The
 * -N<n>r option achieves this.
 * 
 * Calls:		uses the QR solution of the Normal
 * 		equations furnished by Wm. Menke's
 * 		C routine "gauss".  We gratefully
 * 		acknowledge this contribution, now
 * 		as GMT_gauss in gmt_vector.c
 * 
 * Remarks:
 * 
 * We adopt a translation and scaling of the x,y coordinates.
 * We choose x,y such that they are in [-1,1] over the range
 * of the grid file.  If the problem is unweighted, all input
 * values are filled (no "holes" or NaNs in the input grid file),
 * and n_model <= 4 (bilinear or simpler), then the normal
 * equations matrix (G'G in Menke notation) is diagonal under
 * this change of coordinates, and the solution is trivial.
 * In this case, it would be dangerous to try to accumulate
 * the sums which are the elements of the normal equations;
 * while they analytically cancel to zero, the addition errors
 * would likely prevent this.  Therefore we have written a
 * routine, grd_trivial_model(), to handle this case.
 * 
 * If the problem is more complex than the above trivial case,
 * (missing values, weighted problem, or n_model > 4), then
 * G'G is not trivial and we just naively accumulate sums in
 * the G'G matrix.  We hope that the changed coordinates in
 * [-1,1] will help the accuracy of the problem.  We also use
 * Legendre polynomials in this case so that the matrix elements
 * are conveniently sized near 0 or 1.
 */

#define THIS_MODULE k_mod_grdtrend /* I am grdtrend */

#include "gmt.h"

struct GRDTREND_CTRL {	/* All control options for this program (except common args) */
	struct In {
		bool active;
		char *file;
	} In;
	struct D {	/* -D<diffgrid> */
		bool active;
		char *file;
	} D;
	struct N {	/* -N[r]<n_model> */
		bool active;
		bool robust;
		unsigned int value;
	} N;
	struct T {	/* -T<trend.grd> */
		bool active;
		char *file;
	} T;
	struct W {	/* -W<weight.grd> */
		bool active;
		char *file;
	} W;
};

void *New_grdtrend_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDTREND_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct GRDTREND_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
		
	return (C);
}

void Free_grdtrend_Ctrl (struct GMT_CTRL *GMT, struct GRDTREND_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->In.file) free (C->In.file);	
	if (C->D.file) free (C->D.file);	
	if (C->T.file) free (C->T.file);	
	if (C->W.file) free (C->W.file);	
	GMT_free (GMT, C);	
}

int GMT_grdtrend_usage (struct GMTAPI_CTRL *C, int level) {

	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: grdtrend <ingrid> -N<n_model>[r] [-D<diffgrid>]\n");
	GMT_message (GMT, "\t[%s] [-T<trendgrid>] [%s] [-W<weightgrid>]\n\n", GMT_Rgeo_OPT, GMT_V_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<ingrid> is name of grid file to fit trend to.\n");
	GMT_message (GMT, "\t-N Fit a [robust] model with <n_model> terms.  <n_model> in [1,10].  E.g., robust planar = -N3r.\n");
	GMT_message (GMT, "\t   Model parameters order is given as follows:\n");
	GMT_message (GMT, "\t   z = m1 + m2*x + m3*y + m4*x*y + m5*x^2 + m6*y^2 + m7*x^3 + m8*x^2*y + m9*x*y^2 + m10*y^3.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t-D Supply filename to write grid file of differences (input - trend).\n");
	GMT_explain_options (GMT, "R");
	GMT_message (GMT, "\t-T Supply filename to write grid file of trend.\n");
	GMT_explain_options (GMT, "V");
	GMT_message (GMT, "\t-W Supply filename if you want to [read and] write grid file of weights.\n");
	GMT_message (GMT, "\t   If <weightgrid> can be read at run, and if robust = false, weighted problem will be solved.\n");
	GMT_message (GMT, "\t   If robust = true, weights used for robust fit will be written to <weightgrid>.\n");
	GMT_explain_options (GMT, ".");
	
	return (EXIT_FAILURE);
}

int GMT_grdtrend_parse (struct GMTAPI_CTRL *C, struct GRDTREND_CTRL *Ctrl, struct GMT_OPTION *options) {

	/* This parses the options provided to grdtrend and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_files = 0, j;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */
		switch (opt->option) {
			/* Common parameters */

			case '<':	/* Input file (only one is accepted) */
				Ctrl->In.active = true;
				if (n_files++ == 0) Ctrl->In.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'D':
				Ctrl->D.active = true;
				if (opt->arg[0])
					Ctrl->D.file = strdup (opt->arg);
				else {
					GMT_report (GMT, GMT_MSG_FATAL, "Syntax error -D option: Must specify file name\n");
					n_errors++;
				}
				break;
			case 'N':
				/* Must check for both -N[r]<n_model> and -N<n_model>[r] due to confusion */
				Ctrl->N.active = true;
				if (strchr (opt->arg, 'r')) Ctrl->N.robust = true;
				j = (opt->arg[0] == 'r') ? 1 : 0;
				if (opt->arg[j]) Ctrl->N.value = atoi(&opt->arg[j]);
				break;
			case 'T':
				Ctrl->T.active = true;
				if (opt->arg[0])
					Ctrl->T.file = strdup (opt->arg);
				else {
					GMT_report (GMT, GMT_MSG_FATAL, "Syntax error -T option: Must specify file name\n");
					n_errors++;
				}
				break;
			case 'W':
				Ctrl->W.active = true;
				if (opt->arg[0])
					Ctrl->W.file = strdup (opt->arg);
				else {
					GMT_report (GMT, GMT_MSG_FATAL, "Syntax error -W option: Must specify file name\n");
					n_errors++;
				}
				/* OK if this file doesn't exist */
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, n_files != 1, "Syntax error: Must specify an input grid file\n");
	n_errors += GMT_check_condition (GMT, Ctrl->N.value <= 0 || Ctrl->N.value > 10, "Syntax error -N option: Specify 1-10 model parameters\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

void set_up_vals (double *val, unsigned int nval, double vmin, double vmax, double dv, unsigned int pixel_reg)
{	/* Store x[i], y[j] once for all to save time  */
	unsigned int i;
	double v, middle, drange, true_min, true_max;

	true_min = (pixel_reg) ? vmin + 0.5 * dv : vmin;
	true_max = (pixel_reg) ? vmax - 0.5 * dv : vmax;

	middle = 0.5 * (true_min + true_max);
	drange = 2.0 / (true_max - true_min);
	for (i = 0; i < nval; i++) {
		v = true_min + i * dv;
		val[i] = (v - middle) * drange;
	}
	/* Just to be sure no rounding outside */
	val[0] = -1.0;
	val[nval - 1] = 1.0;
	return;
}

void load_pstuff (double *pstuff, unsigned int n_model, double x, double y, unsigned int newx, unsigned int newy)
{	/* Compute Legendre polynomials of x[i],y[j] as needed  */
	/* If either x or y has changed, compute new Legendre polynomials as needed  */

	if (newx) {
		if (n_model >= 2) pstuff[1] = x;
		if (n_model >= 5) pstuff[4] = 0.5*(3.0*pstuff[1]*pstuff[1] - 1.0);
		if (n_model >= 7) pstuff[6] = (5.0*pstuff[1]*pstuff[4] - 2.0*pstuff[1])/3.0;
	}
	if (newy) {
		if (n_model >= 3) pstuff[2] = y;
		if (n_model >= 6) pstuff[5] = 0.5*(3.0*pstuff[2]*pstuff[2] - 1.0);
		if (n_model >= 10) pstuff[9] = (5.0*pstuff[2]*pstuff[5] - 2.0*pstuff[2])/3.0;
	}
	/* In either case, refresh cross terms */

	if (n_model >= 4) pstuff[3] = pstuff[1]*pstuff[2];
	if (n_model >= 8) pstuff[7] = pstuff[4]*pstuff[2];
	if (n_model >= 9) pstuff[8] = pstuff[1]*pstuff[5];

	return;
}

void compute_trend (struct GMT_CTRL *GMT, struct GMT_GRID *T, double *xval, double *yval, double *gtd, unsigned int n_model, double *pstuff)
{	/* Find trend from a model  */
	unsigned int row, col, k;
	uint64_t ij;

	GMT_grd_loop (GMT, T, row, col, ij) {
		load_pstuff (pstuff, n_model, xval[col], yval[row], 1, (!(col)));
		T->data[ij] = 0.0;
		for (k = 0; k < n_model; k++) T->data[ij] += (float)(pstuff[k]*gtd[k]);
	}
}

void compute_resid (struct GMT_CTRL *GMT, struct GMT_GRID *D, struct GMT_GRID *T, struct GMT_GRID *R)
{	/* Find residuals from a trend  */
	unsigned int row, col;
	uint64_t ij;

	GMT_grd_loop (GMT, T, row, col, ij) R->data[ij] = D->data[ij] - T->data[ij];
}

void grd_trivial_model (struct GMT_CTRL *GMT, struct GMT_GRID *G, double *xval, double *yval, double *gtd, unsigned int n_model)
{
	/* Routine to fit up elementary polynomial model of grd data, 
	model = gtd[0] + gtd[1]*x + gtd[2]*y + gtd[3] * x * y,
	where x,y are normalized to range [-1,1] and there are no
	NaNs in grid file, and problem is unweighted least squares.  */

	unsigned int row, col;
	uint64_t ij;
	double x2, y2, sumx2 = 0.0, sumy2 = 0.0, sumx2y2 = 0.0;

	/* First zero the model parameters to use for sums */

	GMT_memset (gtd, n_model, double);

	/* Now accumulate sums */

	GMT_row_loop (GMT, G, row) {
		y2 = yval[row] * yval[row];
		GMT_col_loop (GMT, G, row, col, ij) {
			x2 = xval[col] * xval[col];
			sumx2 += x2;
			sumy2 += y2;
			sumx2y2 += (x2 * y2);
			gtd[0] += G->data[ij];
			if (n_model >= 2) gtd[1] += G->data[ij] * xval[col];
			if (n_model >= 3) gtd[2] += G->data[ij] * yval[row];
			if (n_model == 4) gtd[3] += G->data[ij] * xval[col] * yval[row];
		}
	}

	/* See how trivial it is?  */

	gtd[0] /= G->header->nm;
	if (n_model >= 2) gtd[1] /= sumx2;
	if (n_model >= 3) gtd[2] /= sumy2;
	if (n_model == 4) gtd[3] /= sumx2y2;

	return;
}

double compute_chisq (struct GMT_CTRL *GMT, struct GMT_GRID *R, struct GMT_GRID *W, double scale)
{	/* Find Chi-Squared from weighted residuals  */
	unsigned int row, col;
	uint64_t ij;
	double tmp, chisq = 0.0;

	GMT_grd_loop (GMT, R, row, col, ij) {
		if (GMT_is_fnan (R->data[ij])) continue;
		tmp = R->data[ij];
		if (scale != 1.0) tmp /= scale;
		tmp *= tmp;
		if (W->data[ij] != 1.0) tmp *= W->data[ij];
		chisq += tmp;
	}
	return (chisq);
}

double compute_robust_weight (struct GMT_CTRL *GMT, struct GMT_GRID *R, struct GMT_GRID *W)
{	/* Find weights from residuals  */
	unsigned int row, col;
	uint64_t j = 0, j2, ij;
	double r, mad, scale;

	GMT_grd_loop (GMT, R, row, col, ij) {
		if (GMT_is_fnan (R->data[ij])) continue;
		W->data[j++] = (float)fabs((double)R->data[ij]);
	}

	GMT_sort_array (GMT, R->data, j, GMTAPI_FLOAT);

	j2 = j / 2;
	mad = (j%2) ? W->data[j2] : 0.5 *(W->data[j2] + W->data[j2 - 1]);

	/* Adjust mad to equal Gaussian sigma */

	scale = 1.4826 * mad;

	/* Use weight according to Huber (1981), but squared */

	GMT_grd_loop (GMT, R, row, col, ij) {
		if (GMT_is_fnan (R->data[ij])) {
			W->data[ij] = R->data[ij];
			continue;
		}
		r = fabs (R->data[ij]) / scale;

		W->data[ij] = (float)((r <= 1.5) ? 1.0 : (3.0 - 2.25/r) / r);
	}
	return (scale);
}

void write_model_parameters (struct GMT_CTRL *GMT, double *gtd, unsigned int n_model)
{	/* Do reports if gmtdefs.verbose = NORMAL or above  */
	unsigned int i;
	char pbasis[10][16], format[GMT_BUFSIZ];

	sprintf (pbasis[0], "Mean");
	sprintf (pbasis[1], "X");
	sprintf (pbasis[2], "Y");
	sprintf (pbasis[3], "X*Y");
	sprintf (pbasis[4], "P2(x)");
	sprintf (pbasis[5], "P2(y)");
	sprintf (pbasis[6], "P3(x)");
	sprintf (pbasis[7], "P2(x)*P1(y)");
	sprintf (pbasis[8], "P1(x)*P2(y)");
	sprintf (pbasis[9], "P3(y)");

	sprintf(format, "Coefficient fit to %%s: %s\n", GMT->current.setting.format_float_out);
	for (i = 0; i < n_model; i++) GMT_message (GMT, format, pbasis[i], gtd[i]);

	return;
}

void load_gtg_and_gtd (struct GMT_CTRL *GMT, struct GMT_GRID *G, double *xval, double *yval, double *pstuff, double *gtg, double *gtd, unsigned int n_model, struct GMT_GRID *W, bool weighted)
{
	/* Routine to load the matrix G'G (gtg) and vector G'd (gtd)
	for the normal equations.  Routine uses indices i,j to refer
	to the grid file of data, and k,l to refer to the k_row, l_col
	of the normal equations matrix.  We need sums of [weighted]
	data and model functions in gtg and gtd.  We save time by
	loading only lower triangular part of gtg and then filling
	by symmetry after i,j loop.  */

	unsigned int row, col, k, l, n_used = 0;
	uint64_t ij;

	/* First zero things out to start */

	GMT_memset (gtd, n_model, double);
	GMT_memset (gtg, n_model * n_model, double);

	/* Now get going.  Have to load_pstuff separately in i and j,
	   because it is possible that we skip data when i = 0.
	   Loop over all data */

	GMT_row_loop (GMT, G, row) {
		load_pstuff (pstuff, n_model, xval[0], yval[row], 0, 1);
		GMT_col_loop (GMT, G, row, col, ij) {

			if (GMT_is_fnan (G->data[ij]))continue;

			n_used++;
			load_pstuff (pstuff, n_model, xval[col], yval[row], 1, 0);

			if (weighted) {
				/* Loop over all gtg and gtd elements */
				for (k = 0; k < n_model; k++) {
					gtd[k] += (G->data[ij] * W->data[ij] * pstuff[k]);
					gtg[k] += (W->data[ij] * pstuff[k]);
					for (l = k; l < n_model; l++) gtg[k + l*n_model] += (pstuff[k]*pstuff[l]*W->data[ij]);
				}
			}
			else {	/* If !weighted  */
				/* Loop over all gtg and gtd elements */
				for (k = 0; k < n_model; k++) {
					gtd[k] += (G->data[ij] * pstuff[k]);
					gtg[k] += pstuff[k];
					for (l = k; l < n_model; l++) gtg[k + l*n_model] += (pstuff[k]*pstuff[l]);
				}
			}	/* End if  */
		}
	}	/* End of loop over data i,j  */

	/* Now if !weighted, use more accurate sum for gtg[0], and set symmetry */

	if (!weighted) gtg[0] = (double)n_used;

	for (k = 0; k < n_model; k++) {
		for (l = 0; l < k; l++) gtg[l + k*n_model] = gtg[k + l*n_model];
	}
	/* That is all there is to it!  */

	return;
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grdtrend_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grdtrend (struct GMTAPI_CTRL *API, int mode, void *args) {
	/* High-level function that implements the grdcontour task */

	bool trivial, weighted,iterations, set_ones = true;
	int error = 0;
	unsigned int row, col;
	
	uint64_t ij;
	
	char format[GMT_BUFSIZ];

	double chisq, old_chisq, zero_test = 1.0e-08, scale = 1.0, dv;
	double *xval = NULL;	/* Pointer for array of change of variable: x[i]  */
	double *yval = NULL;	/* Pointer for array of change of variable: y[j]  */
	double *gtg = NULL;	/* Pointer for array for matrix G'G normal equations  */
	double *gtd = NULL;	/* Pointer for array for vector G'd normal equations  */
	double *old = NULL;	/* Pointer for array for old model, used for robust sol'n  */
	double *pstuff = NULL;	/* Pointer for array for Legendre polynomials of x[i],y[j]  */
	double wesn[4];		/* For optional subset specification */

	struct GRDTREND_CTRL *Ctrl = NULL;
	struct GMT_GRID *G = NULL, *R = NULL, *T = NULL, *W = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdtrend_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdtrend_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-VR", "", options)) Return (API->error);
	Ctrl = New_grdtrend_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdtrend_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the grdtrend main code ----------------------------*/

	weighted = (Ctrl->N.robust || Ctrl->W.active);
	trivial = (Ctrl->N.value < 5 && !weighted);

	GMT_memcpy (wesn, GMT->common.R.wesn, 4, double);	/* Current -R setting, if any */
	if ((G = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER, NULL, Ctrl->In.file, NULL)) == NULL) {
		Return (API->error);
	}
	if (GMT_is_subset (GMT, G->header, wesn)) GMT_err_fail (GMT, GMT_adjust_loose_wesn (GMT, wesn, G->header), "");	/* Subset requested; make sure wesn matches header spacing */
	if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, wesn, Ctrl->In.file, G) == NULL) {	/* Get subset */
		Return (API->error);
	}

	/* Check for NaNs (we include the pad for simplicity)  */
	ij = 0;
	while (trivial && ij < G->header->size) if (GMT_is_fnan (G->data[ij++])) trivial = false;

	/* Allocate other required arrays */

	if ((T = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);	/* Pointer for grid with array containing fitted surface  */
	GMT_memcpy (T->header, G->header, 1, struct GRD_HEADER);
	GMT_grd_init (GMT, T->header, options, true);
	T->data = GMT_memory (GMT, NULL, G->header->size, float);
	if (Ctrl->D.active || Ctrl->N.robust) {	/* If !D but robust, we would only need to allocate the data array */
		if ((R = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);	/* Pointer for grid with array containing residual surface  */
		GMT_memcpy (R->header, G->header, 1, struct GRD_HEADER);
		R->data = GMT_memory (GMT, NULL, G->header->size, float);
	}
	xval = GMT_memory (GMT, NULL, G->header->nx, double);
	yval = GMT_memory (GMT, NULL, G->header->ny, double);
	gtg = GMT_memory (GMT, NULL, Ctrl->N.value*Ctrl->N.value, double);
	gtd = GMT_memory (GMT, NULL, Ctrl->N.value, double);
	old = GMT_memory (GMT, NULL, Ctrl->N.value, double);
	pstuff = GMT_memory (GMT, NULL, Ctrl->N.value, double);
	pstuff[0] = 1.0; /* This is P0(x) = 1, which is not altered in this program. */

	/* If a weight array is needed, get one */

	if ((W = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);	/* Pointer for grid with array containing data weights  */
	GMT_grd_init (GMT, W->header, options, true);
	if (weighted) {
		if (!GMT_access (GMT, Ctrl->W.file, R_OK)) {	/* We have weights on input  */
			if ((W = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER, NULL, Ctrl->W.file, NULL)) == NULL) {	/* Get header only */
				Return (API->error);
			}
			if (W->header->nx != G->header->nx || W->header->ny != G->header->ny)
				GMT_report (GMT, GMT_MSG_FATAL, "Error: Input weight file does not match input data file.  Ignoring.\n");
			else {
				if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->W.file, W) == NULL) {	/* Get data */
					Return (API->error);
				}
				set_ones = false;
			}
		}
		if (set_ones) {
			W->data = GMT_memory (GMT, NULL, G->header->size, float);
			GMT_setnval (W->data, G->header->size, 1.0);
		}
	}

	/* End of weight set up.  */

	/* Set up xval and yval lookup tables */

	dv = 2.0 / (double)(G->header->nx - 1);
	for (col = 0; col < G->header->nx - 1; col++) xval[col] = -1.0 + col * dv;
	dv = 2.0 / (double)(G->header->ny - 1);
	for (row = 0; row < G->header->ny - 1; row++) yval[row] = -1.0 + row * dv;
	xval[G->header->nx - 1] = yval[G->header->ny - 1] = 1.0;

	/* Do the problem */

	if (trivial) {
		grd_trivial_model (GMT, G, xval, yval, gtd, Ctrl->N.value);
		compute_trend (GMT, T, xval, yval, gtd, Ctrl->N.value, pstuff);
		if (Ctrl->D.active) compute_resid (GMT, G, T, R);
	}
	else {	/* Problem is not trivial  !!  */
		int ierror;
		load_gtg_and_gtd (GMT, G, xval, yval, pstuff, gtg, gtd, Ctrl->N.value, W, weighted);
		ierror = GMT_gauss (GMT, gtg, gtd, Ctrl->N.value, Ctrl->N.value, zero_test, true);
		if (ierror) {
			GMT_report (GMT, GMT_MSG_FATAL, "Gauss returns error code %d\n", ierror);
			return (EXIT_FAILURE);
		}
		compute_trend (GMT, T, xval, yval, gtd, Ctrl->N.value, pstuff);
		if (Ctrl->D.active || Ctrl->N.robust) compute_resid (GMT, G, T, R);

		if (Ctrl->N.robust) {
			chisq = compute_chisq (GMT, R, W, scale);
			iterations = 1;
			sprintf (format, "Robust iteration %%d:  Old Chi Squared: %s  New Chi Squared: %s\n", GMT->current.setting.format_float_out, GMT->current.setting.format_float_out);
			do {
				old_chisq = chisq;
				GMT_memcpy (old, gtd, Ctrl->N.value, double);
				scale = compute_robust_weight (GMT, R, W);
				load_gtg_and_gtd (GMT, G, xval, yval, pstuff, gtg, gtd, Ctrl->N.value, W, weighted);
				ierror = GMT_gauss (GMT, gtg, gtd, Ctrl->N.value, Ctrl->N.value, zero_test, true);
				if (ierror) {
					GMT_report (GMT, GMT_MSG_FATAL, "Gauss returns error code %d\n", ierror);
					return (EXIT_FAILURE);
				}
				compute_trend (GMT, T, xval, yval, gtd, Ctrl->N.value, pstuff);
				compute_resid (GMT, G, T, R);
				chisq = compute_chisq (GMT, R, W, scale);
				GMT_report (GMT, GMT_MSG_NORMAL, format, gmt_module_name(GMT), iterations, old_chisq, chisq);
				iterations++;
			} while (old_chisq / chisq > 1.0001);

			/* Get here when new model not significantly better; use old one */

			GMT_memcpy (gtd, old, Ctrl->N.value, double);
			compute_trend (GMT, T, xval, yval, gtd, Ctrl->N.value, pstuff);
			compute_resid (GMT, G, T, R);
		}
	}

	/* End of do the problem section.  */

	/* Get here when ready to do output */

	if (GMT_is_verbose (GMT, GMT_MSG_NORMAL)) write_model_parameters (GMT, gtd, Ctrl->N.value);
	if (Ctrl->T.file) {
		strcpy (T->header->title, "trend surface");
		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->T.file, T) != GMT_OK) {
			Return (API->error);
		}
	}
	else
		GMT_free_grid (GMT, &T, true);	/* Not written out */
	if (Ctrl->D.file) {
		strcpy (R->header->title, "trend residuals");
		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->D.file, R) != GMT_OK) {
			Return (API->error);
		}
	}
	else if (Ctrl->D.active || Ctrl->N.robust)
		GMT_free_grid (GMT, &R, true);
	if (Ctrl->W.file && Ctrl->N.robust) {
		strcpy (W->header->title, "trend weights");
		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->W.file, W) != GMT_OK) {
			Return (API->error);
		}
	}
	else if (set_ones)
		GMT_free_grid (GMT, &W, true);

	/* That's all, folks!  */


	GMT_free (GMT, pstuff);
	GMT_free (GMT, gtd);
	GMT_free (GMT, gtg);
	GMT_free (GMT, old);
	GMT_free (GMT, yval);
	GMT_free (GMT, xval);

	GMT_report (GMT, GMT_MSG_NORMAL, "Done!\n");

	Return (EXIT_SUCCESS);
}
