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
 * API functions to support the grdcut application.
 *
 * Author:	Walter Smith
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 * Brief synopsis: Reads a grid file and writes a portion within it
 * to a new file.
 */
 
#include "gmt.h"

/* Control structure for grdcontour */

struct GRDCUT_CTRL {
	struct In {
		BOOLEAN active;
		char *file;
	} In;
	struct G {	/* -G<output_grdfile> */
		BOOLEAN active;
		char *file;
	} G;
	struct Z {	/* -Z[min/max] */
		BOOLEAN active;
		COUNTER_MEDIUM mode;	/* 1 means NaN */
		double min, max;
	} Z;
};

#define NAN_IS_INSIDE	0
#define NAN_IS_OUTSIDE	1

void *New_grdcut_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDCUT_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct GRDCUT_CTRL);

	/* Initialize values whose defaults are not 0/FALSE/NULL */

	C->Z.min = -DBL_MAX;	C->Z.max = DBL_MAX;			/* No limits on z-range */
	return (C);
}

void Free_grdcut_Ctrl (struct GMT_CTRL *GMT, struct GRDCUT_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->In.file) free (C->In.file);	
	if (C->G.file) free (C->G.file);	
	GMT_free (GMT, C);	
}

GMT_LONG GMT_grdcut_usage (struct GMTAPI_CTRL *C, GMT_LONG level)
{
	struct GMT_CTRL *GMT = C->GMT;

	GMT_message (GMT, "grdcut %s [API] - Extract subregion from a grid\n\n", GMT_VERSION);
	GMT_message (GMT, "usage: grdcut <ingrid> -G<outgrid> %s [%s]\n\t[-Z[n][min/max]] [%s]\n", GMT_Rgeo_OPT, GMT_V_OPT, GMT_f_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<ingrid> is file to extract a subset from.\n");
	GMT_message (GMT, "\t-G Specify output grid file\n");
	GMT_explain_options (GMT, "R");
	GMT_message (GMT, "\t   Obviously, the WESN you specify must be within the WESN of the input grid.\n");
	GMT_message (GMT, "\t   If in doubt, run grdinfo first and check range of old file.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "V");
	GMT_message (GMT, "\t-Z Specify a range and determine the corresponding rectangular region so that\n");
	GMT_message (GMT, "\t   all values outside this region are outside the range [-inf/+inf].\n");
	GMT_message (GMT, "\t   Use -Zn to consider NaNs outside as well [Default just ignores NaNs].\n");
	GMT_explain_options (GMT, "f.");
	
	return (EXIT_FAILURE);
}

GMT_LONG GMT_grdcut_parse (struct GMTAPI_CTRL *C, struct GRDCUT_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to grdcut and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	GMT_LONG n_errors = 0, k, n_files = 0;
	char za[GMT_TEXT_LEN64], zb[GMT_TEXT_LEN64];
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Input files */
				Ctrl->In.active = TRUE;
				if (n_files++ == 0) Ctrl->In.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */
			
 			case 'G':	/* Output file */
				Ctrl->G.active = TRUE;
				Ctrl->G.file = strdup (opt->arg);
				break;
 			case 'Z':	/* Detect region via z-range */
				Ctrl->Z.active = TRUE;
				k = 0;
				if (opt->arg[k] == 'n') {
					Ctrl->Z.mode = NAN_IS_OUTSIDE;
					k = 1;
				}
				if (sscanf (&opt->arg[k], "%[^/]/%s", za, zb) == 2) {
					if (!(za[0] == '-' && za[1] == '\0')) n_errors += GMT_verify_expectations (GMT, GMT->current.io.col_type[GMT_IN][GMT_Z], GMT_scanf_arg (GMT, za, GMT->current.io.col_type[GMT_IN][GMT_Z], &Ctrl->Z.min), za);
					if (!(zb[0] == '-' && zb[1] == '\0')) n_errors += GMT_verify_expectations (GMT, GMT->current.io.col_type[GMT_IN][GMT_Z], GMT_scanf_arg (GMT, zb, GMT->current.io.col_type[GMT_IN][GMT_Z], &Ctrl->Z.max), zb);
				}
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}
	
	n_errors += GMT_check_condition (GMT, (GMT->common.R.active + Ctrl->Z.active) != 1, "Syntax error: Must specify either the -R or the -Z options\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->G.file, "Syntax error -G option: Must specify output grid file\n");
	n_errors += GMT_check_condition (GMT, n_files != 1, "Syntax error: Must specify one input grid file\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grdcut_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

GMT_LONG GMT_grdcut (struct GMTAPI_CTRL *API, GMT_LONG mode, void *args)
{
	GMT_LONG error = 0;
	unsigned int nx_old, ny_old;

	double wesn_new[4], wesn_old[4];

	struct GRD_HEADER test_header;
	struct GRDCUT_CTRL *Ctrl = NULL;
	struct GMT_GRID *G = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdcut_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdcut_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_module (API, "GMT_grdcut", &GMT_cpy);	/* Save current state */
	if (GMT_Parse_Common (API, "-VRf:", "", options)) Return (API->error);
	Ctrl = New_grdcut_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdcut_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the grdcut main code ----------------------------*/

	if (Ctrl->Z.active) {	/* Must determine new region via -Z, so get entire grid first */
		COUNTER_MEDIUM row0 = 0, row1 = 0, col0 = 0, col1 = 0, row, col;
		COUNTER_LARGE ij;
		BOOLEAN go;
		
		if ((G = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, NULL, GMT_GRID_ALL, Ctrl->In.file, NULL)) == NULL) {
			Return (API->error);	/* Get entire grid */
		}
		
		for (row = 0, go = TRUE; go && row < G->header->nx; row++) {	/* Scan from xmin towards xmax */
			for (col = 0, ij = GMT_IJP (G->header, 0, row); go && col < G->header->ny; col++, ij += G->header->mx) {
				if (GMT_is_fnan (G->data[ij])) {
					if (Ctrl->Z.mode == NAN_IS_OUTSIDE) go = FALSE;	/* Must stop since this NaN value defines the inner box */
				}
				else if (G->data[ij] >= Ctrl->Z.min && G->data[ij] <= Ctrl->Z.max)
					go = FALSE;
				if (!go) row0 = row;	/* Found starting column */
			}
		}
		if (go) {
			GMT_report (GMT, GMT_MSG_FATAL, "The sub-region implied by -Z is empty!\n");
			Return (EXIT_FAILURE);
		}
		for (row = G->header->nx-1, go = TRUE; go && row > row0; row--) {	/* Scan from xmax towards xmin */
			for (col = 0, ij = GMT_IJP (G->header, 0, row); go && col < G->header->ny; col++, ij += G->header->mx) {
				if (GMT_is_fnan (G->data[ij])) {
					if (Ctrl->Z.mode == NAN_IS_INSIDE) go = FALSE;	/* Must stop since this value defines the inner box */
				}
				else if (G->data[ij] >= Ctrl->Z.min && G->data[ij] <= Ctrl->Z.max)
					go = FALSE;
				if (!go) row1 = row;	/* Found stopping column */
			}
		}
		for (col = 0, go = TRUE; go && col < G->header->ny; col++) {	/* Scan from ymin towards ymax */
			for (row = row0, ij = GMT_IJP (G->header, col, row0); go && row < row1; row++, ij++) {
				if (GMT_is_fnan (G->data[ij])) {
					if (Ctrl->Z.mode == NAN_IS_INSIDE) go = FALSE;	/* Must stop since this value defines the inner box */
				}
				else if (G->data[ij] >= Ctrl->Z.min && G->data[ij] <= Ctrl->Z.max)
					go = FALSE;
				if (!go) col0 = col;	/* Found starting row */
			}
		}
		for (col = G->header->ny-1, go = TRUE; go && col >= col0; col--) {	/* Scan from ymax towards ymin */
			for (row = row0, ij = GMT_IJP (G->header, col, row0); go && row < row1; row++, ij++) {
				if (GMT_is_fnan (G->data[ij])) {
					if (Ctrl->Z.mode == NAN_IS_INSIDE) go = FALSE;	/* Must stop since this value defines the inner box */
				}
				else if (G->data[ij] >= Ctrl->Z.min && G->data[ij] <= Ctrl->Z.max)
					go = FALSE;
				if (!go) col1 = col;	/* Found starting row */
			}
		}
		if (row0 == 0 && col0 == 0 && row1 == (G->header->nx-1) && col1 == (G->header->ny-1)) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Your -Z limits produced no subset - output grid is identical to input grid\n");
			GMT_memcpy (wesn_new, G->header->wesn, 4, double);
		}
		else {	/* Adjust boundaries inwards */
			wesn_new[XLO] = G->header->wesn[XLO] + row0 * G->header->inc[GMT_X];
			wesn_new[XHI] = G->header->wesn[XHI] - (G->header->nx - 1 - row1) * G->header->inc[GMT_X];
			wesn_new[YLO] = G->header->wesn[YLO] + (G->header->ny - 1 - col1) * G->header->inc[GMT_Y];
			wesn_new[YHI] = G->header->wesn[YHI] - col0 * G->header->inc[GMT_Y];
		}
		GMT_free (GMT, G->data);	/* Free the grid array only as we need the header below */
	}
	else {	/* Just the usual subset selection via -R */
		if ((G = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, NULL, GMT_GRID_HEADER, Ctrl->In.file, NULL)) == NULL) {
			Return (API->error);	/* Get header only */
		}
		GMT_memcpy (wesn_new, GMT->common.R.wesn, 4, double);
	}
	
	if (wesn_new[YLO] < G->header->wesn[YLO] || wesn_new[YLO] > G->header->wesn[YHI]) error++;
	if (wesn_new[YHI] < G->header->wesn[YLO] || wesn_new[YHI] > G->header->wesn[YHI]) error++;

	if (GMT_is_geographic (GMT, GMT_IN)) {	/* Geographic data */
		if (wesn_new[XLO] < G->header->wesn[XLO] && wesn_new[XHI] < G->header->wesn[XLO]) {
			G->header->wesn[XLO] -= 360.0;
			G->header->wesn[XHI] -= 360.0;
		}
		if (wesn_new[XLO] > G->header->wesn[XHI] && wesn_new[XHI] > G->header->wesn[XHI]) {
			G->header->wesn[XLO] += 360.0;
			G->header->wesn[XHI] += 360.0;
		}
		if (!GMT_grd_is_global (GMT, G->header) && (wesn_new[XLO] < G->header->wesn[XLO] || wesn_new[XHI] > G->header->wesn[XHI])) error++;
	}
	else if (wesn_new[XLO] < G->header->wesn[XLO] || wesn_new[XHI] > G->header->wesn[XHI])
		error++;

	if (error) {
		GMT_report (GMT, GMT_MSG_FATAL, "Subset exceeds data domain!\n");
		Return (GMT_RUNTIME_ERROR);
	}

	/* Make sure output grid is kosher */

	GMT_adjust_loose_wesn (GMT, wesn_new, G->header);

	GMT_memcpy (test_header.wesn, wesn_new, 4, double);
	GMT_memcpy (test_header.inc, G->header->inc, 2, double);
	GMT_err_fail (GMT, GMT_grd_RI_verify (GMT, &test_header, 1), Ctrl->G.file);

	/* OK, so far so good. Check if new wesn differs from old wesn by integer dx/dy */

	if (GMT_minmaxinc_verify (GMT, G->header->wesn[XLO], wesn_new[XLO], G->header->inc[GMT_X], GMT_SMALL) == 1) {
		GMT_report (GMT, GMT_MSG_FATAL, "Old and new x_min do not differ by N * dx\n");
		Return (GMT_RUNTIME_ERROR);
	}
	if (GMT_minmaxinc_verify (GMT, wesn_new[XHI], G->header->wesn[XHI], G->header->inc[GMT_X], GMT_SMALL) == 1) {
		GMT_report (GMT, GMT_MSG_FATAL, "Old and new x_max do not differ by N * dx\n");
		Return (GMT_RUNTIME_ERROR);
	}
	if (GMT_minmaxinc_verify (GMT, G->header->wesn[YLO], wesn_new[YLO], G->header->inc[GMT_Y], GMT_SMALL) == 1) {
		GMT_report (GMT, GMT_MSG_FATAL, "Old and new y_min do not differ by N * dy\n");
		Return (GMT_RUNTIME_ERROR);
	}
	if (GMT_minmaxinc_verify (GMT, wesn_new[YHI], G->header->wesn[YHI], G->header->inc[GMT_Y], GMT_SMALL) == 1) {
		GMT_report (GMT, GMT_MSG_FATAL, "Old and new y_max do not differ by N * dy\n");
		Return (GMT_RUNTIME_ERROR);
	}

	GMT_grd_init (GMT, G->header, options, TRUE);

	GMT_memcpy (wesn_old, G->header->wesn, 4, double);
	nx_old = G->header->nx;		ny_old = G->header->ny;
	
	if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, wesn_new, GMT_GRID_DATA, Ctrl->In.file, G) == NULL) {	/* Get subset */
		Return (API->error);
	}

	if (GMT_is_verbose (GMT, GMT_MSG_NORMAL)) {
		char format[GMT_BUFSIZ];
		sprintf (format, "\t%s\t%s\t%s\t%s\t%s\t%s\t%%ld\t%%ld\n", GMT->current.setting.format_float_out, GMT->current.setting.format_float_out,
			GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out);
		GMT_report (GMT, GMT_MSG_NORMAL, "File spec:\tW E S N dx dy nx ny:\n");
		GMT_report (GMT, GMT_MSG_NORMAL, "Old:");
		GMT_report (GMT, GMT_MSG_NORMAL, format, wesn_old[XLO], wesn_old[XHI], wesn_old[YLO], wesn_old[YHI], G->header->inc[GMT_X], G->header->inc[GMT_Y], nx_old, ny_old);
		GMT_report (GMT, GMT_MSG_NORMAL, "New:");
		GMT_report (GMT, GMT_MSG_NORMAL, format, wesn_new[XLO], wesn_new[XHI], wesn_new[YLO], wesn_new[YHI], G->header->inc[GMT_X], G->header->inc[GMT_Y], G->header->nx, G->header->ny);
	}

	/* Send the subset of the grid to the destination. */
	
	if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, NULL, 0, Ctrl->G.file, G) != GMT_OK) {
		Return (API->error);
	}

	Return (GMT_OK);
}
