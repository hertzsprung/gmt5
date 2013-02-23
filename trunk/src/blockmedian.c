/*--------------------------------------------------------------------
 *    $Id$
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
 * API functions to support the blockmedian application.
 *
 * Author:	Walter H.F. Smith
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 * Brief synopsis: reads records of x, y, data, [weight] and writes out median
 * value per cell, where cellular region is bounded by West East South North
 * and cell dimensions are delta_x, delta_y.
 */

#define BLOCKMEDIAN	/* Since mean, median, mode share near-similar macros we require this setting */

#define THIS_MODULE k_mod_blockmedian /* I am blockmedian */

#include "gmt_dev.h"
#include "block_subs.h"

int GMT_blockmedian_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: blockmedian [<table>] %s %s\n", GMT_I_OPT, GMT_Rgeo_OPT);
	GMT_message (GMT, "\t[-C] [-E[b]] [-Er|s[-]] [-Q] [-T<q>] [%s] [-W[i][o]] [%s]\n\t[%s] [%s] [%s]\n\t[%s] [%s] [%s]\n\n",
		GMT_V_OPT, GMT_b_OPT, GMT_f_OPT, GMT_h_OPT, GMT_i_OPT, GMT_o_OPT, GMT_r_OPT, GMT_colon_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_inc_syntax (GMT, 'I', 0);
	GMT_explain_options (GMT, "R");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "<");
	GMT_message (GMT, "\t-C Output center of block as location [Default is (median x, median y), but see -Q].\n");
	GMT_message (GMT, "\t-E Extend output with L1 scale (s), low (l), and high (h) value per block, i.e.,\n");
	GMT_message (GMT, "\t   output (x,y,z,s,l,h[,w]) [Default outputs (x,y,z[,w])]; see -W regarding w.\n");
	GMT_message (GMT, "\t   Use -Eb for box-and-whisker output (x,y,z,l,25%%q,75%%q,h[,w]).\n");
	GMT_message (GMT, "\t   Use -Er to report record number of the median value per block,\n");
	GMT_message (GMT, "\t   or -Es to report an unsigned integer source id (sid) taken from the x,y,z[,w],sid input.\n");
	GMT_message (GMT, "\t   For ties, report record number (or sid) of largest value; append - for smallest.\n");
	GMT_message (GMT, "\t-Q Quicker; get median z and x,y at that z [Default gets median x, median y, median z].\n");
	GMT_message (GMT, "\t-T Set quantile (0 < q < 1) to report [Default is 0.5 which is the median of z].\n");
	GMT_explain_options (GMT, "V");
	GMT_message (GMT, "\t-W Set Weight options.\n");
	GMT_message (GMT, "\t   -Wi reads Weighted Input (4 cols: x,y,z,w) but skips w on output.\n");
	GMT_message (GMT, "\t   -Wo reads unWeighted Input (3 cols: x,y,z) but weight sum on output.\n");
	GMT_message (GMT, "\t   -W with no modifier has both weighted Input and Output; Default is no weights used.\n");
	GMT_explain_options (GMT, "C0");
	GMT_message (GMT, "\t    Default is 3 columns (or 4 if -W is set).\n");
	GMT_explain_options (GMT, "D0fhioF:.");
	
	return (EXIT_FAILURE);
}

int GMT_blockmedian_parse (struct GMTAPI_CTRL *C, struct BLOCKMEDIAN_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to blockmedian and sets parameters in CTRL.
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

			case 'C':	/* Report center of block instead */
				Ctrl->C.active = true;
				break;
			case 'E':	/* Report extended statistics */
				Ctrl->E.active = true;			/* Report standard deviation, min, and max in cols 4-6 */
				if (opt->arg[0] == 'b')
					Ctrl->E.mode = BLK_DO_EXTEND4;		/* Report min, 25%, 75% and max in cols 4-7 */
				else if (opt->arg[0] == 'r' || opt->arg[0] == 's') {
					Ctrl->E.mode = (opt->arg[1] == '-') ? BLK_DO_INDEX_LO : BLK_DO_INDEX_HI;	/* Report row number or sid of median */
					if (opt->arg[0] == 's') /* report sid */
						Ctrl->E.mode |= BLK_DO_SRC_ID;
				}
				else if (opt->arg[0] == '\0')
					Ctrl->E.mode = BLK_DO_EXTEND3;		/* Report L1scale, low, high in cols 4-6 */
				else
					n_errors++;
				break;
			case 'I':	/* Get block dimensions */
				Ctrl->I.active = true;
				if (GMT_getinc (GMT, opt->arg, Ctrl->I.inc)) {
					GMT_inc_syntax (GMT, 'I', 1);
					n_errors++;
				}
				break;
			case 'Q':	/* Quick mode for median z */
				Ctrl->Q.active = true;		/* Get median z and (x,y) of that point */
				break;
			case 'T':	/* Select a particular quantile [0.5 (median)] */
				Ctrl->T.active = true;		
				Ctrl->T.quantile = atof (opt->arg);
				break;
			case 'W':	/* Use in|out weights */
				Ctrl->W.active = true;
				switch (opt->arg[0]) {
					case '\0':
						Ctrl->W.weighted[GMT_IN] = Ctrl->W.weighted[GMT_OUT] = true; break;
					case 'i': case 'I':
						Ctrl->W.weighted[GMT_IN] = true; break;
					case 'o': case 'O':
						Ctrl->W.weighted[GMT_OUT] = true; break;
					default:
						n_errors++; break;
				}
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}
	
	GMT_check_lattice (GMT, Ctrl->I.inc, &GMT->common.r.registration, &Ctrl->I.active);	/* If -R<grdfile> was given we may get incs unless -I was used */

	n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error: Must specify -R option\n");
	n_errors += GMT_check_condition (GMT, Ctrl->T.quantile <= 0.0 || Ctrl->T.quantile >= 1.0,
			"Syntax error: 0 < q < 1 for quantile in -T [0.5]\n");
	n_errors += GMT_check_condition (GMT, Ctrl->I.inc[GMT_X] <= 0.0 || Ctrl->I.inc[GMT_Y] <= 0.0,
			"Syntax error -I option: Must specify positive increment(s)\n");
	n_errors += GMT_check_binary_io (GMT, (Ctrl->W.weighted[GMT_IN]) ? 4 : 3);

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

void median_output (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *h, uint64_t first_in_cell, uint64_t first_in_new_cell, double weight_sum, double *out, double *extra,
	unsigned int go_quickly, unsigned int emode, double *quantile, unsigned int n_quantiles, struct BLK_DATA *data)
{
	double weight_half, weight_count;
	uint64_t node, n_in_cell, node1;
	unsigned int k, k_for_xy;
	int way;

	/* Remember: Data are already sorted on z for each cell */

	/* Step 1: Find the n_quantiles requested (typically only one unless -Eb was used) */

	n_in_cell = first_in_new_cell - first_in_cell;
	node = first_in_cell;
	weight_count = data[first_in_cell].a[BLK_W];
	k_for_xy = (n_quantiles == 3) ? 1 : 0;	/* If -Eb is set get get median location, else same as for z (unless -Q) */
	for (k = 0; k < n_quantiles; k++) {

		weight_half = quantile[k] * weight_sum;	/* Normally, quantile will be 0.5 (i.e., median), hence the name of the variable */

		/* Determine the point where we hit the desired quantile */

		while (weight_count < weight_half) weight_count += data[++node].a[BLK_W];	/* Wind up until weight_count hits the mark */

		if (weight_count == weight_half) {
			node1 = node + 1;
			extra[k] = 0.5 * (data[node].a[BLK_Z] + data[node1].a[BLK_Z]);
			if (k == k_for_xy && go_quickly == 1) {	/* Only get x,y at the z-quantile if requested [-Q] */
				out[GMT_X] = 0.5 * (data[node].a[GMT_X] + data[node1].a[GMT_X]);
				out[GMT_Y] = 0.5 * (data[node].a[GMT_Y] + data[node1].a[GMT_Y]);
			}
			if (k == 0 && emode) {	/* Return record number of median, selecting the high or the low value since it is a tie */
				way = (data[node].a[BLK_Z] >= data[node1].a[BLK_Z]) ? +1 : -1;
				if (emode & BLK_DO_INDEX_HI) extra[3] = (way == +1) ? data[node].src_id : data[node1].src_id;
				else extra[3] = (way == +1) ? data[node1].src_id : data[node].src_id;
			}
		}
		else {
			extra[k] = data[node].a[BLK_Z];
			if (k == k_for_xy && go_quickly == 1) {	/* Only get x,y at the z-quantile if requested [-Q] */
				out[GMT_X] = data[node].a[GMT_X];
				out[GMT_Y] = data[node].a[GMT_Y];
			}
			if (k == 0 && emode) extra[3] = data[node].src_id;	/* Return record number of median */
		}
	}
	out[GMT_Z] = extra[k_for_xy];	/* The desired quantile is passed via z */

	if (go_quickly == 1) return;	/* Already have everything requested so we return */

	if (go_quickly == 2) {	/* Return center of block instead of computing a representative location */
		unsigned int row, col;
		row = GMT_row (h, data[node].ij);
		col = GMT_col (h, data[node].ij);
		out[GMT_X] = GMT_grd_col_to_x (GMT, col, h);
		out[GMT_Y] = GMT_grd_row_to_y (GMT, row, h);
		return;
	}

	/* We get here when we need separate quantile calculations for both x and y locations */

	weight_half = quantile[k_for_xy] * weight_sum;	/* We want the same quantile for locations as was used for z */

	if (n_in_cell > 2) qsort(&data[first_in_cell], n_in_cell, sizeof (struct BLK_DATA), BLK_compare_x);
	node = first_in_cell;
	weight_count = data[first_in_cell].a[BLK_W];
	while (weight_count < weight_half) weight_count += data[++node].a[BLK_W];
	out[GMT_X] = (weight_count == weight_half) ?  0.5 * (data[node].a[GMT_X] + data[node + 1].a[GMT_X]) : data[node].a[GMT_X];

	if (n_in_cell > 2) qsort (&data[first_in_cell], n_in_cell, sizeof (struct BLK_DATA), BLK_compare_y);
	node = first_in_cell;
	weight_count = data[first_in_cell].a[BLK_W];
	while (weight_count < weight_half) weight_count += data[++node].a[BLK_W];
	out[GMT_Y] = (weight_count == weight_half) ? 0.5 * (data[node].a[GMT_Y] + data[node + 1].a[GMT_Y]) : data[node].a[GMT_Y];
}

/* Must free allocated memory before returning */
#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {GMT_free_grid (GMT, &Grid, false); Free_blockmedian_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_blockmedian (void *V_API, int mode, void *args)
{
	uint64_t n_lost, node, first_in_cell, first_in_new_cell;
	uint64_t n_read, nz, n_pitched, n_cells_filled;
	
	size_t n_alloc = 0, nz_alloc = 0;
	
	bool do_extra;
	
	int error = 0;
	unsigned int row, col, w_col, i_col = 0, sid_col, emode = 0, n_input, n_output, n_quantiles = 1, go_quickly = 0;
	
	double out[8], wesn[4], quantile[3] = {0.25, 0.5, 0.75}, extra[8], weight, *in = NULL, *z_tmp = NULL;

	char format[GMT_BUFSIZ], *old_format = NULL;

	struct GMT_OPTION *options = NULL;
	struct GMT_GRID *Grid = NULL;
	struct BLK_DATA *data = NULL;
	struct BLOCKMEDIAN_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_blockmedian_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_blockmedian_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-VfRb:", "ghior>" GMT_OPT("FH"), options)) Return (API->error);
	Ctrl = New_blockmedian_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_blockmedian_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the blockmedian main code ----------------------------*/

	GMT_report (GMT, GMT_MSG_VERBOSE, "Processing input table data\n");

	if ((Grid = GMT_Create_Data (API, GMT_IS_GRID, GMT_GRID_HEADER_ONLY, NULL, GMT->common.R.wesn, Ctrl->I.inc, \
		GMT->common.r.registration, 0, NULL)) == NULL) Return (API->error);

	go_quickly = (Ctrl->Q.active) ? 1 : 0;
	if (Ctrl->C.active && go_quickly == 1) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Warning: -C overrides -Q\n");
		go_quickly = 0;
	}
	if (Ctrl->C.active) go_quickly = 2;			/* Flag used in output calculation */
	n_input = 3 + Ctrl->W.weighted[GMT_IN] + ((Ctrl->E.mode & BLK_DO_SRC_ID) ? 1 : 0);	/* 3 columns on output, plus 1 extra if -W and another if -Es  */
	n_output = (Ctrl->W.weighted[GMT_OUT]) ? 4 : 3;		/* 3 columns on output, plus 1 extra if -W  */
	if (Ctrl->E.active) {					/* One or more -E settings */
		if (Ctrl->E.mode & BLK_DO_EXTEND3) {	/* Add s,l,h cols */
			n_output += 3;
			do_extra = true;
		}
		else if (Ctrl->E.mode & BLK_DO_EXTEND4) {	/* Add l, 25%, 75%, h cols */
			n_output += 4;
			n_quantiles = 3;
			do_extra = true;
		}
		if (Ctrl->E.mode & BLK_DO_INDEX_LO || Ctrl->E.mode & BLK_DO_INDEX_HI) {	/* Add index */
			n_output++;
			emode = Ctrl->E.mode & (BLK_DO_INDEX_LO + BLK_DO_INDEX_HI);
		}
	}
	if (!(Ctrl->E.mode & BLK_DO_EXTEND4)) quantile[0] = Ctrl->T.quantile;	/* Just get the single quantile [median] */

	if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) {
		sprintf (format, "W: %s E: %s S: %s N: %s nx: %%d ny: %%d\n", GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out);
		GMT_report (GMT, GMT_MSG_VERBOSE, format, Grid->header->wesn[XLO], Grid->header->wesn[XHI], Grid->header->wesn[YLO], Grid->header->wesn[YHI], Grid->header->nx, Grid->header->ny);
	}

	GMT_set_xy_domain (GMT, wesn, Grid->header);	/* May include some padding if gridline-registered */

	/* Specify input and output expected columns */
	if ((error = GMT_set_cols (GMT, GMT_IN, n_input)) != GMT_OK) {
		Return (error);
	}
	if ((error = GMT_set_cols (GMT, GMT_OUT, n_output)) != GMT_OK) {
		Return (error);
	}

	/* Register likely data sources unless the caller has already done so */
	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_IN,  GMT_REG_DEFAULT, 0, options) != GMT_OK) {	/* Registers default input sources, unless already set */
		Return (API->error);
	}
	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_OUT, GMT_REG_DEFAULT, 0, options) != GMT_OK) {	/* Registers default output destination, unless already set */
		Return (API->error);
	}

	/* Initialize the i/o for doing record-by-record reading/writing */
	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_IN) != GMT_OK) {	/* Enables data input and sets access mode */
		Return (API->error);
	}

	sid_col = (Ctrl->W.weighted[GMT_IN]) ? 4 : 3;	/* Column with integer source id [if -Es is set] */
	n_read = n_pitched = 0;	/* Initialize counters */

	/* Read the input data */

	do {	/* Keep returning records until we reach EOF */
		if ((in = GMT_Get_Record (API, GMT_READ_DOUBLE, NULL)) == NULL) {	/* Read next record, get NULL if special case */
			if (GMT_REC_IS_ERROR (GMT)) 		/* Bail if there are any read errors */
				Return (GMT_RUNTIME_ERROR);
			if (GMT_REC_IS_ANY_HEADER (GMT)) 	/* Skip all table and segment headers */
				continue;
			if (GMT_REC_IS_EOF (GMT)) 		/* Reached end of file */
				break;
		}
		
		if (GMT_is_dnan (in[GMT_Z])) 		/* Skip if z = NaN */
			continue;

		/* Clean data record to process */

		n_read++;						/* Number of records read */

		if (GMT_y_is_outside (GMT, in[GMT_Y], wesn[YLO], wesn[YHI])) continue;		/* Outside y-range */
		if (GMT_x_is_outside (GMT, &in[GMT_X], wesn[XLO], wesn[XHI])) continue;		/* Outside x-range (or longitude) */

		/* We appear to be inside: Get row and col indices of this block */

		if (GMT_row_col_out_of_bounds (GMT, in, Grid->header, &row, &col)) continue;	/* Sorry, outside after all */

		/* OK, this point is definitively inside and will be used */

		node = GMT_IJP (Grid->header, row, col);	/* Bin node */

		if (n_pitched == n_alloc) data = GMT_malloc (GMT, data, n_pitched, &n_alloc, struct BLK_DATA);
		data[n_pitched].ij = node;
		data[n_pitched].src_id = (Ctrl->E.mode & BLK_DO_SRC_ID) ? (uint64_t)lrint (in[sid_col]) : n_read;
		data[n_pitched].a[BLK_W] = ((Ctrl->W.weighted[GMT_IN]) ? in[3] : 1.0);
		if (!Ctrl->C.active) {	/* Need to store (x,y) so we can compute median location later */
			data[n_pitched].a[GMT_X] = in[GMT_X];
			data[n_pitched].a[GMT_Y] = in[GMT_Y];
		}
		data[n_pitched].a[BLK_Z] = in[GMT_Z];

		n_pitched++;
	} while (true);
	
	if (GMT_End_IO (API, GMT_IN, 0) != GMT_OK) {	/* Disables further data input */
		Return (API->error);
	}
	
	if (n_read == 0) {	/* Blank/empty input files */
		GMT_report (GMT, GMT_MSG_VERBOSE, "No data records found; no output produced");
		Return (EXIT_SUCCESS);
	}
	if (n_pitched == 0) {	/* No points inside region */
		GMT_report (GMT, GMT_MSG_VERBOSE, "No data points found inside the region; no output produced");
		Return (EXIT_SUCCESS);
	}

	if (n_pitched < n_alloc) {
		n_alloc = n_pitched;
		data = GMT_malloc (GMT, data, 0, &n_alloc, struct BLK_DATA);
	}

	/* Ready to go. */

	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_OUT) != GMT_OK) {	/* Enables data output and sets access mode */
		Return (API->error);
	}

	w_col = GMT_get_cols (GMT, GMT_OUT) - 1;	/* Weights always reported in last output column */
	if (emode) {					/* Index column last, with weight col just before */
		i_col = w_col--;
		old_format = GMT->current.io.o_format[i_col];		/* Need to restore this at end */
		GMT->current.io.o_format[i_col] = strdup ("%.0f");	/* Integer format for src_id */
	}

	/* Sort on node and Z value */

	qsort (data, n_pitched, sizeof (struct BLK_DATA), BLK_compare_index_z);

	/* Find n_in_cell and write appropriate output  */

	first_in_cell = n_cells_filled = nz = 0;
	while (first_in_cell < n_pitched) {
		weight = data[first_in_cell].a[BLK_W];
		if (do_extra) {
			if (nz == nz_alloc) z_tmp = GMT_malloc (GMT, z_tmp, nz, &nz_alloc, double);
			z_tmp[0] = data[first_in_cell].a[BLK_Z];
			nz = 1;
		}
		first_in_new_cell = first_in_cell + 1;
		while ((first_in_new_cell < n_pitched) && (data[first_in_new_cell].ij == data[first_in_cell].ij)) {
			weight += data[first_in_new_cell].a[BLK_W];
			if (do_extra) {	/* Must get a temporary copy of the sorted z array */
				if (nz == nz_alloc) z_tmp = GMT_malloc (GMT, z_tmp, nz, &nz_alloc, double);
				z_tmp[nz++] = data[first_in_new_cell].a[BLK_Z];
			}
			first_in_new_cell++;
		}

		/* Now we have weight sum [and copy of z in case of -E]; now calculate the quantile(s): */

		median_output (GMT, Grid->header, first_in_cell, first_in_new_cell, weight, out, extra, go_quickly, emode, quantile, n_quantiles, data);
		/* Here, x,y,z are loaded into out */

		if (Ctrl->E.mode & BLK_DO_EXTEND4) {	/* Need 7 items: x, y, median, min, 25%, 75%, max [,weight] */
			out[3] = z_tmp[0];	/* 0% quantile (min value) */
			out[4] = extra[0];	/* 25% quantile */
			out[5] = extra[2];	/* 75% quantile */
			out[6] = z_tmp[nz-1];	/* 100% quantile (max value) */
		}
		else if (Ctrl->E.mode & BLK_DO_EXTEND3) {	/* Need 6 items: x, y, median, MAD, min, max [,weight] */
			out[4] = z_tmp[0];	/* Low value */
			out[5] = z_tmp[nz-1];	/* High value */
			/* Turn z_tmp into absolute deviations from the median (out[GMT_Z]) */
			if (nz > 1) {
				for (node = 0; node < nz; node++) z_tmp[node] = fabs (z_tmp[node] - out[GMT_Z]);
				GMT_sort_array (GMT, z_tmp, nz, GMT_DOUBLE);
				out[3] = (nz%2) ? z_tmp[nz/2] : 0.5 * (z_tmp[(nz-1)/2] + z_tmp[nz/2]);
				out[3] *= 1.4826;	/* This will be L1 MAD-based scale */
			}
			else
				out[3] = GMT->session.d_NaN;
		}
		if (Ctrl->W.weighted[GMT_OUT]) out[w_col] = weight;
		if (emode) out[i_col] = extra[3];

		GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);	/* Write this to output */

		n_cells_filled++;
		first_in_cell = first_in_new_cell;
	}
	if (GMT_End_IO (API, GMT_OUT, 0) != GMT_OK) {	/* Disables further data output */
		Return (API->error);
	}

	n_lost = n_read - n_pitched;	/* Number of points that did not get used */
	GMT_report (GMT, GMT_MSG_VERBOSE, "N read: %ld N used: %ld N outside_area: %ld N cells filled: %ld\n", n_read, n_pitched, n_lost, n_cells_filled);

	GMT_free (GMT, data);
	if (do_extra) GMT_free (GMT, z_tmp);

	GMT_set_pad (GMT, 2);			/* Restore to GMT padding defaults */

	if (emode) {
		free (GMT->current.io.o_format[i_col]);		/* Free the temporary integer format */
		GMT->current.io.o_format[i_col] = old_format;	/* Restore previous format */
	}

	Return (GMT_OK);
}
