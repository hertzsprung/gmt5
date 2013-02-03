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
 * Brief synopsis: grdpaste.c reads two grid files and writes a new file with
 * the first two pasted together along their common edge.
 *
 * Author:	Walter Smith
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#define THIS_MODULE k_mod_grdpaste /* I am grdpaste */

#include "gmt.h"

struct GRDPASTE_CTRL {
	struct In {
		bool active;
		char *file[2];
	} In;
	struct G {	/* -G<output_grdfile> */
		bool active;
		char *file;
	} G;
};

void *New_grdpaste_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDPASTE_CTRL *C = NULL;

	C = GMT_memory (GMT, NULL, 1, struct GRDPASTE_CTRL);

	/* Initialize values whose defaults are not 0/false/NULL */

	return (C);
}

void Free_grdpaste_Ctrl (struct GMT_CTRL *GMT, struct GRDPASTE_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->G.file) free (C->G.file);	
	if (C->In.file[0]) free (C->In.file[0]);	
	if (C->In.file[1]) free (C->In.file[1]);	
	GMT_free (GMT, C);	
}

int GMT_grdpaste_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: grdpaste <grid1> <grid2> -G<outgrid> [%s] [%s]\n\n", GMT_V_OPT, GMT_f_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\twhere grids <grid1> and <grid2> are to be combined into <outgrid>.\n");
	GMT_message (GMT, "\t<grid1> and <grid2> must have same dx,dy and one edge in common.\n");
	GMT_message (GMT, "\tIf in doubt, run grdinfo first and check your files.\n");
	GMT_message (GMT, "\tUse grdpaste and/or grdsample to adjust files as necessary.\n");
	GMT_message (GMT, "\t-G Specify file name for output grid file.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "Vf.");

	return (EXIT_FAILURE);
}

int GMT_grdpaste_parse (struct GMTAPI_CTRL *C, struct GRDPASTE_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to grdpaste and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_in = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Input files */
				if (n_in == 0)
					Ctrl->In.file[n_in++] = strdup (opt->arg);
				else if (n_in == 1)
					Ctrl->In.file[n_in++] = strdup (opt->arg);
				else {
					n_errors++;
					GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error: Only two files may be pasted\n");
				}
				break;

			/* Processes program-specific parameters */

 			case 'G':
				Ctrl->G.active = true;
				Ctrl->G.file = strdup (opt->arg);
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, !Ctrl->In.file[0] || !Ctrl->In.file[1], "Syntax error: Must specify two input files\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->G.file, "Syntax error -G: Must specify output file\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grdpaste_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

/* True if grid is a COARDS/CF netCDF file */
static inline bool is_nc_grid (struct GMT_GRID *grid) {
	return
		grid->header->type == GMT_GRD_IS_NB ||
		grid->header->type == GMT_GRD_IS_NS ||
		grid->header->type == GMT_GRD_IS_NI ||
		grid->header->type == GMT_GRD_IS_NF ||
		grid->header->type == GMT_GRD_IS_ND;
}

int GMT_grdpaste (struct GMTAPI_CTRL *API, int mode, void *args)
{
	bool error = false;
	int way;
	unsigned int one_or_zero;

	char format[GMT_BUFSIZ];

	double x_noise, y_noise;

	struct GMT_GRID *A = NULL, *B = NULL, *C = NULL;
	struct GRDPASTE_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdpaste_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdpaste_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-Vf", "", options)) Return (API->error);
	Ctrl = New_grdpaste_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdpaste_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the grdpaste main code ----------------------------*/

	GMT_set_pad (GMT, 0); /* No padding */

	/* Try to find a common side to join on  */

	if ((C = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);
	GMT_grd_init (GMT, C->header, options, false);

	if ((A = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER, NULL, Ctrl->In.file[0], NULL)) == NULL) {	/* Get header only */
		Return (API->error);
	}
	if ((B = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER, NULL, Ctrl->In.file[1], NULL)) == NULL) {	/* Get header only */
		Return (API->error);
	}

	if (A->header->registration != B->header->registration)
		error++;
	if ((A->header->z_scale_factor != B->header->z_scale_factor) || (A->header->z_add_offset != B->header->z_add_offset)) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Scale/offset not compatible!\n");
		Return (EXIT_FAILURE);
	}

	if (fabs (A->header->inc[GMT_X] - B->header->inc[GMT_X]) < 1.0e-6 && fabs (A->header->inc[GMT_Y] - B->header->inc[GMT_Y]) < 1.0e-6) {
		C->header->inc[GMT_X] = A->header->inc[GMT_X];
		C->header->inc[GMT_Y] = A->header->inc[GMT_Y];
	}
	else {
		GMT_report (GMT, GMT_MSG_NORMAL, "Grid intervals do not match!\n");
		Return (EXIT_FAILURE);
	}

	one_or_zero = A->header->registration == GMT_GRIDLINE_REG;
	x_noise = GMT_SMALL * C->header->inc[GMT_X];
	y_noise = GMT_SMALL * C->header->inc[GMT_Y];

	if (GMT_is_geographic (GMT, GMT_IN)) {	/* Must be careful in determining a match */
		double del;
		del = A->header->wesn[XLO] - B->header->wesn[XHI];
		if (fabs (del - 360.0) < GMT_CONV_LIMIT) {	/* A 360-degree offset between grids */
			B->header->wesn[XLO] += 360.0;	B->header->wesn[XHI] += 360.0;
		}
		else if (fabs (del + 360.0) < GMT_CONV_LIMIT) {	/* A -360-degree offset between grids */
			A->header->wesn[XLO] += 360.0;	A->header->wesn[XHI] += 360.0;
		}
		else {
			del = A->header->wesn[XHI] - B->header->wesn[XLO];
			if (fabs (del - 360.0) < GMT_CONV_LIMIT) {	/* A 360-degree offset between grids */
				B->header->wesn[XLO] += 360.0;	B->header->wesn[XHI] += 360.0;
			}
			else if (fabs (del + 360.0) < GMT_CONV_LIMIT) {	/* A -360-degree offset between grids */
				A->header->wesn[XLO] += 360.0;	A->header->wesn[XHI] += 360.0;
			}
		}
	}

	GMT_memcpy (C->header->wesn, A->header->wesn, 4, double);	/* Output region is set as the same as A... */
	if (fabs (A->header->wesn[XLO] - B->header->wesn[XLO]) < x_noise && fabs (A->header->wesn[XHI] - B->header->wesn[XHI]) < x_noise) {

		C->header->nx = A->header->nx;

		if (fabs (A->header->wesn[YHI] - B->header->wesn[YLO]) < y_noise) {			/* B is exactly on top of A */
			way = 1;
			C->header->ny = A->header->ny + B->header->ny - one_or_zero;
			C->header->wesn[YHI] = B->header->wesn[YHI];			/* ...but not for north */
		}
		else if (fabs (A->header->wesn[YLO] - B->header->wesn[YHI]) < y_noise) {	/* A is exactly on top of B */
			way = 2;
			C->header->ny = A->header->ny + B->header->ny - one_or_zero;
			C->header->wesn[YLO] = B->header->wesn[YLO];			/* ...but not for south */
		}
		else if ((fabs (A->header->wesn[YHI] - B->header->wesn[YLO]) < (C->header->inc[GMT_Y] + y_noise)) ) {
			/* B is on top of A but their pixel|grid reg limits under|overlap by one cell */
			if (one_or_zero)        /* Grid registration - underlap */
				way = 10;
			else                    /* Pixel registration - overlap */
				way = 11;
			C->header->ny = A->header->ny + B->header->ny - !one_or_zero;
			C->header->wesn[YHI] = B->header->wesn[YHI];			/* ...but not for north */
		}
		else if ((fabs (A->header->wesn[YLO] - B->header->wesn[YHI]) < (C->header->inc[GMT_Y] + y_noise)) ) {
			/* A is on top of B but their pixel|grid reg limits under|overlap by one cell */
			if (one_or_zero)        /* Grid registration - underlap */
				way = 21;
			else                    /* Pixel registration - overlap */
				way = 22;
			C->header->ny = A->header->ny + B->header->ny - !one_or_zero;
			C->header->wesn[YLO] = B->header->wesn[YLO];			/* ...but not for south */
		}
		else {
			GMT_report (GMT, GMT_MSG_NORMAL, "Grids do not share a common edge!\n");
			Return (EXIT_FAILURE);
		}
	}
	else if (fabs (A->header->wesn[YLO] - B->header->wesn[YLO]) < y_noise && fabs (A->header->wesn[YHI] - B->header->wesn[YHI]) < y_noise) {

		C->header->ny = A->header->ny;

		if (fabs (A->header->wesn[XLO] - B->header->wesn[XHI]) < x_noise) {			/* A is on the right of B */
			way = 3;
			C->header->nx = A->header->nx + B->header->nx - one_or_zero;
			C->header->wesn[XLO] = B->header->wesn[XLO];			/* ...but not for west */
		}
		else if (fabs (A->header->wesn[XHI] - B->header->wesn[XLO]) < x_noise) {	/* A is on the left of B */
			way = 4;
			C->header->nx = A->header->nx + B->header->nx - one_or_zero;
			C->header->wesn[XHI] = B->header->wesn[XHI];			/* ...but not for east */
		}
		else if ((fabs (A->header->wesn[XLO] - B->header->wesn[XHI]) < (C->header->inc[GMT_X] + x_noise)) ) {
			/* A is on right of B but their pixel|grid reg limits under|overlap by one cell */
			if (one_or_zero)        /* Grid registration - underlap */
				way = 32;
			else                    /* Pixel registration - overlap */
				way = 33;
			C->header->nx = A->header->nx + B->header->nx - !one_or_zero;
			C->header->wesn[XLO] = B->header->wesn[XLO];			/* ...but not for west */
		}
		else if ((fabs (A->header->wesn[XHI] - B->header->wesn[XLO]) < (C->header->inc[GMT_X] + x_noise)) ) {
			/* A is on left of B but their pixel|grid reg limits under|overlap by one cell */
			if (one_or_zero)        /* Grid registration - underlap */
				way = 43;
			else                    /* Pixel registration - overlap */
				way = 44;
			C->header->nx = A->header->nx + B->header->nx - !one_or_zero;
			C->header->wesn[XHI] = B->header->wesn[XHI];			/* ...but not for east */
		}
		else {
			GMT_report (GMT, GMT_MSG_NORMAL, "Grids do not share a common edge!\n");
			Return (EXIT_FAILURE);
		}
	}
	else {
		GMT_report (GMT, GMT_MSG_NORMAL, "Grids do not share a common edge!\n");
		Return (EXIT_FAILURE);
	}
	if (GMT_is_geographic (GMT, GMT_IN) && C->header->wesn[XHI] > 360.0) {	/* Must be careful in determining a match */
		C->header->wesn[XLO] -= 360.0;
		C->header->wesn[XHI] -= 360.0;
	}

	/* Now we can do it  */

	if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) {
		sprintf (format, "\t%s\t%s\t%s\t%s\t%s\t%s\t%%d\t%%d\n", GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out, GMT->current.setting.format_float_out);
		GMT_report (GMT, GMT_MSG_VERBOSE, "File spec:\tW E S N dx dy nx ny:\n");
		GMT_report (GMT, GMT_MSG_VERBOSE, format, Ctrl->In.file[0], A->header->wesn[XLO], A->header->wesn[XHI], A->header->wesn[YLO], A->header->wesn[YHI], A->header->inc[GMT_X], A->header->inc[GMT_Y], A->header->nx, A->header->ny);
		GMT_report (GMT, GMT_MSG_VERBOSE, format, Ctrl->In.file[1], B->header->wesn[XLO], B->header->wesn[XHI], B->header->wesn[YLO], B->header->wesn[YHI], B->header->inc[GMT_X], B->header->inc[GMT_Y], B->header->nx, B->header->ny);
		GMT_report (GMT, GMT_MSG_VERBOSE, format, Ctrl->G.file, C->header->wesn[XLO], C->header->wesn[XHI], C->header->wesn[YLO], C->header->wesn[YHI], C->header->inc[GMT_X], C->header->inc[GMT_Y], C->header->nx, C->header->ny);
	}

	C->header->registration = A->header->registration;
	GMT_set_grddim (GMT, C->header);
	C->data = GMT_memory_aligned (GMT, NULL, C->header->size, float);
	A->data = B->data = C->data;	/* A and B share the same final matrix declared for C */
	A->header->size = B->header->size = C->header->size;	/* Set A & B's size to the same as C */
	A->header->no_BC = B->header->no_BC = true;	/* We must disable the BC machinery */

	switch (way) {    /* How A and B are positioned relative to each other */
		case 1:         /* B is on top of A */
		case 10:		/* B is on top of A but their grid reg limits underlap by one cell */
		case 11:        /* B is on top of A but their pixel reg limits overlap by one cell */
			if (is_nc_grid(A)) {
				A->header->data_offset = B->header->nx * (B->header->ny - one_or_zero);
				if (way == 11)
					A->header->data_offset -= B->header->nx;
				else if (way == 10)
					A->header->data_offset += B->header->nx;
			}
			else {
				GMT->current.io.pad[YHI] = B->header->ny - one_or_zero;
				if (way == 11)
					GMT->current.io.pad[YHI]--;
				else if (way == 10)
					GMT->current.io.pad[YHI]++;
				GMT_grd_setpad (GMT, A->header, GMT->current.io.pad);
			}
			A->header->BB_my = C->header->my;		/* Needed if grid is read by GMT_gdal_read_grd */
			if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->In.file[0], A) == NULL) {  /* Get data from A */
				Return (API->error);
			}
			if (is_nc_grid(B)) {
				GMT_set_pad (GMT, 0); /* Reset padding */
			}
			else {
				GMT->current.io.pad[YHI] = 0;
				GMT->current.io.pad[YLO] = A->header->ny - one_or_zero;
				if (way == 11)
					GMT->current.io.pad[YLO]--;
				else if (way == 10)
					GMT->current.io.pad[YLO]++;
				GMT_grd_setpad (GMT, B->header, GMT->current.io.pad);
			}
			B->header->BB_my = C->header->my;		/* Needed if grid is read by GMT_gdal_read_grd */
			if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->In.file[1], B) == NULL) {  /* Get data from B */
				Return (API->error);
			}
			break;
		case 2:         /* A is on top of B */
		case 21:        /* A is on top of B but their grid reg limits underlap by one cell */
		case 22:        /* A is on top of B but their pixel reg limits overlap by one cell */
			if (!is_nc_grid(A)) {
				GMT->current.io.pad[YLO] = B->header->ny - one_or_zero;
				if (way == 22)
					GMT->current.io.pad[YLO]--;
				else if (way == 21)
					GMT->current.io.pad[YLO]++;
				GMT_grd_setpad (GMT, A->header, GMT->current.io.pad);
			}
			A->header->BB_my = C->header->my;		/* Needed if grid is read by GMT_gdal_read_grd */
			if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->In.file[0], A) == NULL) {  /* Get data from A */
				Return (API->error);
			}
			if (is_nc_grid(B)) {
				GMT_set_pad (GMT, 0); /* Reset padding */
				B->header->data_offset = A->header->nx * (A->header->ny - one_or_zero);
				if (way == 22)
					B->header->data_offset -= A->header->nx;
				else if (way == 21)
					B->header->data_offset += A->header->nx;
			}
			else {
				GMT->current.io.pad[YLO] = 0;
				GMT->current.io.pad[YHI] = A->header->ny - one_or_zero;
				if (way == 22)
					GMT->current.io.pad[YHI]--;
				else if (way == 21)
					GMT->current.io.pad[YHI]++;
				GMT_grd_setpad (GMT, B->header, GMT->current.io.pad);
			}
			B->header->BB_my = C->header->my;		/* Needed if grid is read by GMT_gdal_read_grd */
			if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->In.file[1], B) == NULL) {  /* Get data from B */
				Return (API->error);
			}
			break;
		case 3:         /* A is on the right of B */
		case 32:        /* A is on right of B but their grid reg limits underlap by one cell */
		case 33:        /* A is on right of B but their pixel reg limits overlap by one cell */
			if (is_nc_grid(A)) {
				A->header->stride = C->header->nx;
				A->header->data_offset = B->header->nx - one_or_zero;
				if (way == 33)
					A->header->data_offset--;
				else if (way == 32)
					A->header->data_offset++;
			}
			else {
				GMT->current.io.pad[XLO] = B->header->nx - one_or_zero;
				if (way == 33)
					GMT->current.io.pad[XLO]--;
				else if (way == 32)
					GMT->current.io.pad[XLO]++;
				GMT_grd_setpad (GMT, A->header, GMT->current.io.pad);
			}
			A->header->BB_mx = C->header->mx;		/* Needed if grid is read by GMT_gdal_read_grd */
			if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->In.file[0], A) == NULL) {  /* Get data from A */
				Return (API->error);
			}
			if (is_nc_grid(B)) {
				GMT_set_pad (GMT, 0); /* Reset padding */
				B->header->stride = C->header->nx;
			}
			else {
				GMT->current.io.pad[XLO] = 0; GMT->current.io.pad[XHI] = A->header->nx - one_or_zero;
				if (way == 33)
					GMT->current.io.pad[XHI]--;
				else if (way == 32)
					GMT->current.io.pad[XHI]++;
				GMT_grd_setpad (GMT, B->header, GMT->current.io.pad);
			}
			B->header->BB_mx = C->header->mx;		/* Needed if grid is read by GMT_gdal_read_grd */
			if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->In.file[1], B) == NULL) {  /* Get data from B */
				Return (API->error);
			}
			break;
		case 4:         /* A is on the left of B */
		case 43:        /* A is on left of B but their grid reg limits underlap by one cell */
		case 44:        /* A is on left of B but their pixel reg limits overlap by one cell */
			if (is_nc_grid(A)) {
				A->header->stride = C->header->nx;
			}
			else {
				GMT->current.io.pad[XHI] = B->header->nx - one_or_zero;
				if (way == 44)
					GMT->current.io.pad[XHI]--;
				else if (way == 43)
					GMT->current.io.pad[XHI]++;
				GMT_grd_setpad (GMT, A->header, GMT->current.io.pad);
			}
			A->header->BB_mx = C->header->mx;		/* Needed if grid is read by GMT_gdal_read_grd */
			if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->In.file[0], A) == NULL) {  /* Get data from A */
				Return (API->error);
			}
			if (is_nc_grid(B)) {
				GMT_set_pad (GMT, 0); /* Reset padding */
				B->header->stride = C->header->nx;
				B->header->data_offset = A->header->nx - one_or_zero;
				if (way == 44)
					B->header->data_offset--;
				else if (way == 43)
					B->header->data_offset++;
			}
			else {
				GMT->current.io.pad[XHI] = 0;
				GMT->current.io.pad[XLO] = A->header->nx - one_or_zero;
				if (way == 44)
					GMT->current.io.pad[XLO]--;
				else if (way == 43)
					GMT->current.io.pad[XLO]++;
				GMT_grd_setpad (GMT, B->header, GMT->current.io.pad);
			}
			B->header->BB_mx = C->header->mx;		/* Needed if grid is read by GMT_gdal_read_grd */
			if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA, NULL, Ctrl->In.file[1], B) == NULL) {  /* Get data from B */
				Return (API->error);
			}
			break;
	}

	GMT_set_pad (GMT, 0); /* Reset padding */
	if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->G.file, C) != GMT_OK) {
		Return (API->error);
	}
	A->data = B->data = NULL; /* Since these were never actually allocated */

	GMT_set_pad (GMT, 2); /* Restore to GMT Defaults */
	Return (GMT_OK);
}
