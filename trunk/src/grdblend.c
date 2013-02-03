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
 * grdblend reads any number of grid files that may partly overlap and
 * creates a blend of all the files given certain criteria.  Each input
 * grid is considered to have an "outer" and "inner" region.  The outer
 * region is the extent of the grid; the inner region is provided as
 * input in the form of a -Rw/e/s/n statement.  Finally, each grid can
 * be assigned its separate weight.  This information is given to the
 * program in ASCII format, one line per grid file; each line looks like
 *
 * grdfile	-Rw/e/s/n	weight
 *
 * The blending will use a 2-D cosine taper between the inner and outer
 * regions.  The output at any node is thus a weighted average of the
 * values from any grid that occupies that grid node.  Because the out-
 * put grid can be really huge (say global grids at fine resolution),
 * all grid input/output is done row by row so memory should not be a
 * limiting factor in making large grid.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#define THIS_MODULE k_mod_grdblend /* I am grdblend */

#include "gmt.h"

#define BLEND_UPPER	0
#define BLEND_LOWER	1
#define BLEND_FIRST	2
#define BLEND_LAST	3

struct GRDBLEND_CTRL {
	struct In {	/* Input files */
		bool active;
		char **file;
		unsigned int n;	/* If n > 1 we probably got *.grd or something */
	} In;
	struct G {	/* -G<grdfile> */
		bool active;
		char *file;
	} G;
	struct C {	/* -C */
		bool active;
		unsigned int mode;
	} C;
	struct I {	/* -Idx[/dy] */
		bool active;
		double inc[2];
	} I;
	struct N {	/* -N<nodata> */
		bool active;
		double nodata;
	} N;
	struct Q {	/* -Q */
		bool active;
	} Q;
	struct Z {	/* -Z<scale> */
		bool active;
		double scale;
	} Z;
	struct W {	/* -W */
		bool active;
	} W;
};

struct GRDBLEND_INFO {	/* Structure with info about each input grid file */
	struct GMT_GRDFILE G;				/* I/O structure for grid files, including grd header */
	int in_i0, in_i1, out_i0, out_i1;		/* Indices of outer and inner x-coordinates (in output grid coordinates) */
	int in_j0, in_j1, out_j0, out_j1;		/* Indices of outer and inner y-coordinates (in output grid coordinates) */
	off_t offset;					/* grid offset when the grid extends beyond north */
	off_t skip;					/* Byte offset to skip in native binary files */
	bool ignore;					/* true if the grid is entirely outside desired region */
	bool outside;				/* true if the current output row is outside the range of this grid */
	bool invert;					/* true if weight was given as negative and we want to taper to zero INSIDE the grid region */
	bool open;					/* true if file is currently open */
	bool delete;					/* true if file was produced by grdsample to deal with different registration/increments */
	char file[GMT_TEXT_LEN256];			/* Name of grid file */
	double weight, wt_y, wxr, wxl, wyu, wyd;	/* Various weighting factors used for cosine-taper weights */
	double wesn[4];					/* Boundaries of inner region */
	float *z;					/* Row vector holding the current row from this file */
};

#define N_NOT_SUPPORTED	8

int found_unsupported_format (struct GMT_CTRL *GMT, struct GRD_HEADER *h, char *file)
{	/* Check that grid files are not among the unsupported formats that has no row-by-row io yet */
	unsigned int i;
	static char *not_supported[N_NOT_SUPPORTED] = {"rb", "rf", "sf", "sd", "af", "ei", "ef", "gd"};
	for (i = 0; i < N_NOT_SUPPORTED; i++) {	/* Only allow netcdf (both v3 and new) and native binary output */
		if (GMT_grd_format_decoder (GMT, not_supported[i], &h->type) != GMT_NOERROR) {
			/* no valid type id */
			GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "Grid format type %s for file %s is not directly supported\n", not_supported[i], file);
			return (1);
		}
	}
	return (GMT_NOERROR);
}

void decode_R (struct GMT_CTRL *GMT, char *string, double wesn[]) {
	unsigned int i, pos, error = 0;
	char text[GMT_BUFSIZ];

	/* Needed to decode the inner region -Rw/e/s/n string */

	i = pos = 0;
	while (!error && (GMT_strtok (string, "/", &pos, text))) {
		error += GMT_verify_expectations (GMT, GMT->current.io.col_type[GMT_IN][i/2], GMT_scanf_arg (GMT, text, GMT->current.io.col_type[GMT_IN][i/2], &wesn[i]), text);
		i++;
	}
	if (error || (i != 4) || GMT_check_region (GMT, wesn)) {
		GMT_syntax (GMT, 'R');
	}
}

bool out_of_phase (struct GRD_HEADER *g, struct GRD_HEADER *h)
{	/* Look for phase shifts in w/e/s/n between the two grids */
	unsigned int way, side;
	double a;
	for (side = 0; side < 4; side++) {
		way = side / 2;
		a = fabs (fmod (g->wesn[side] - h->wesn[side], h->inc[way]));
		if (a < GMT_CONV_LIMIT) continue;
		if (fabs (a - h->inc[way]) < GMT_CONV_LIMIT) continue;
		return true;
	}
	return false;
}

bool overlap_check (struct GMT_CTRL *GMT, struct GRDBLEND_INFO *B, struct GRD_HEADER *h, unsigned int mode)
{
	double w, e, shift = 720.0;
	char *type[2] = {"grid", "inner grid"};
	w = ((mode) ? B->wesn[XLO] : B->G.header.wesn[XLO]) - shift;	e = ((mode) ? B->wesn[XHI] : B->G.header.wesn[XHI]) - shift;
	while (e < h->wesn[XLO]) { w += 360.0; e += 360.0; shift -= 360.0; }
	if (w > h->wesn[XHI]) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Warning: File %s entirely outside longitude range of final grid region (skipped)\n", B->file);
		B->ignore = true;
		return true;
	}
	if (! (GMT_IS_ZERO (shift))) {	/* Must modify region */
		if (mode) {
			B->wesn[XLO] = w;	B->wesn[XHI] = e;
		}
		else {
			B->G.header.wesn[XLO] = w;	B->G.header.wesn[XHI] = e;
		}
		GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "File %s %s region needed longitude adjustment to fit final grid region\n", B->file, type[mode]);
	}
	return false;
}

int init_blend_job (struct GMT_CTRL *GMT, char **files, unsigned int n_files, struct GRD_HEADER *h, struct GRDBLEND_INFO **blend) {
	int type, status;
	bool do_sample, not_supported;
	unsigned int one_or_zero = !h->registration, n = 0, nr;
	struct GRDBLEND_INFO *B = NULL;
	char *sense[2] = {"normal", "inverse"}, buffer[GMT_BUFSIZ];
	char Targs[GMT_TEXT_LEN256], Iargs[GMT_TEXT_LEN256], Rargs[GMT_TEXT_LEN256], cmd[GMT_BUFSIZ];
	struct BLEND_LIST {
		char *file;
		char *region;
		double weight;
	} *L = NULL;

	if (n_files > 1) {	/* Got a bunch of grid files */
		L = GMT_memory (GMT, NULL, n_files, struct BLEND_LIST);
		for (n = 0; n < n_files; n++) {
			L[n].file = strdup (files[n]);
			L[n].region = strdup ("-");	/* inner == outer region */
			L[n].weight = 1.0;		/* Default weight */
		}
	}
	else {	/* Must read blend file */
		size_t n_alloc = 0;
		char *line = NULL, r_in[GMT_TEXT_LEN256], file[GMT_TEXT_LEN256];
		double weight;
		GMT_set_meminc (GMT, GMT_SMALL_CHUNK);
		do {	/* Keep returning records until we reach EOF */
			if ((line = GMT_Get_Record (GMT->parent, GMT_READ_TEXT, NULL)) == NULL) {	/* Read next record, get NULL if special case */
				if (GMT_REC_IS_ERROR (GMT)) 		/* Bail if there are any read errors */
					return (GMT_RUNTIME_ERROR);
				if (GMT_REC_IS_ANY_HEADER (GMT)) 	/* Skip all table and segment headers */
					continue;
				if (GMT_REC_IS_EOF (GMT)) 		/* Reached end of file */
					break;
			}
			
			/* Data record to process */

			nr = sscanf (line, "%s %s %lf", file, r_in, &weight);
			if (nr < 1) {
				GMT_report (GMT, GMT_MSG_NORMAL, "Read error for blending parameters near row %d\n", n);
				return (EXIT_FAILURE);
			}
			if (n == n_alloc) L = GMT_malloc (GMT, L, n, &n_alloc, struct BLEND_LIST);
			L[n].file = strdup (file);
			L[n].region = strdup (r_in);
			L[n].weight = (nr == 1) ? 1.0 : weight;	/* Default weight if not given */
			n++;
		} while (true);
		GMT_reset_meminc (GMT);
		n_files = n;
	}
	
	B = GMT_memory (GMT, NULL, n_files, struct GRDBLEND_INFO);
	
	for (n = 0; n < n_files; n++) {	/* Process each input grid */
		strncpy (B[n].file, L[n].file, GMT_TEXT_LEN256);
		GMT_err_fail (GMT, GMT_read_grd_info (GMT, B[n].file, &B[n].G.header), B[n].file);	/* Read header structure */
		not_supported = found_unsupported_format (GMT, &B[n].G.header, B[n].file);
		B[n].weight = L[n].weight;
		if (!strcmp (L[n].region, "-"))
			GMT_memcpy (B[n].wesn, B[n].G.header.wesn, 4, double);	/* Set inner = outer region */
		else
			decode_R (GMT, &L[n].region[2], B[n].wesn);	/* Must decode the -R string */
		/* Skip the file if its outer region does not lie within the final grid region */
		if (h->wesn[YLO] > B[n].wesn[YHI] || h->wesn[YHI] < B[n].wesn[YLO]) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Warning: File %s entirely outside y-range of final grid region (skipped)\n", B[n].file);
			B[n].ignore = true;
			continue;
		}
		if (GMT_is_geographic (GMT, GMT_IN)) {	/* Must carefully check the longitude overlap */
			if (overlap_check (GMT, &B[n], h, 0)) continue;	/* Check header for -+360 issues and overlap */
			if (overlap_check (GMT, &B[n], h, 1)) continue;	/* Check inner region for -+360 issues and overlap */
		}
		else if (h->wesn[XLO] > B[n].wesn[XHI] || h->wesn[XHI] < B[n].wesn[XLO] || h->wesn[YLO] > B[n].wesn[YHI] || h->wesn[YHI] < B[n].wesn[YLO]) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Warning: File %s entirely outside x-range of final grid region (skipped)\n", B[n].file);
			B[n].ignore = true;
			continue;
		}

		/* If input grids have different spacing or registration we must resample */

		Targs[0] = Iargs[0] = Rargs[0] = '\0';
		do_sample = 0;
		if (not_supported) {
			GMT_report (GMT, GMT_MSG_VERBOSE, "File %s not supported via row-by-row read - must reformat first\n", B[n].file);
			do_sample |= 2;
		}
		if (h->registration != B[n].G.header.registration) {
			strcpy (Targs, "-T");
			GMT_report (GMT, GMT_MSG_VERBOSE, "File %s has different registration than the output grid - must resample\n", B[n].file);
			do_sample |= 1;
		}
		if (!(doubleAlmostEqualZero (B[n].G.header.inc[GMT_X], h->inc[GMT_X])
					&& doubleAlmostEqualZero (B[n].G.header.inc[GMT_Y], h->inc[GMT_Y]))) {
			sprintf (Iargs, "-I%g/%g", h->inc[GMT_X], h->inc[GMT_Y]);
			GMT_report (GMT, GMT_MSG_VERBOSE, "File %s has different increments (%g/%g) than the output grid (%g/%g) - must resample\n",
				B[n].file, B[n].G.header.inc[GMT_X], B[n].G.header.inc[GMT_Y], h->inc[GMT_X], h->inc[GMT_Y]);
			do_sample |= 1;
		}
		if (out_of_phase (&(B[n].G.header), h)) {	/* Set explicit -R for resampling that is multiple of desired increments AND inside both original grid and desired grid */
			double wesn[4];
			wesn[XLO] = GMT_grd_col_to_x (GMT, GMT_grd_x_to_col (GMT, B[n].G.header.wesn[XLO], h), h);
			while ((wesn[XLO] + GMT_CONV_LIMIT) < MAX (h->wesn[XLO], B[n].G.header.wesn[XLO])) wesn[XLO] += h->inc[GMT_X];
			wesn[XHI] = GMT_grd_col_to_x (GMT, GMT_grd_x_to_col (GMT, B[n].G.header.wesn[XHI], h), h);
			while ((wesn[XHI] - GMT_CONV_LIMIT) > MIN (h->wesn[XHI], B[n].G.header.wesn[XHI])) wesn[XHI] -= h->inc[GMT_X];
			wesn[YLO] = GMT_grd_row_to_y (GMT, GMT_grd_y_to_row (GMT, B[n].G.header.wesn[YLO], h), h);
			while ((wesn[YLO] + GMT_CONV_LIMIT) < MAX (h->wesn[YLO], B[n].G.header.wesn[YLO])) wesn[YLO] += h->inc[GMT_Y];
			wesn[YHI] = GMT_grd_row_to_y (GMT, GMT_grd_y_to_row (GMT, B[n].G.header.wesn[YHI], h), h);
			while ((wesn[YHI] - GMT_CONV_LIMIT) > MIN (h->wesn[YHI], B[n].G.header.wesn[YHI])) wesn[YHI] -= h->inc[GMT_Y];
			sprintf (Rargs, "-R%.12g/%.12g/%.12g/%.12g", wesn[XLO], wesn[XHI], wesn[YLO], wesn[YHI]);
			GMT_report (GMT, GMT_MSG_VERBOSE, "File %s coordinates are phase-shifted w.r.t. the output grid - must resample\n", B[n].file);
			do_sample |= 1;
		}
		else if (do_sample) {	/* Set explicit -R to handle possible subsetting */
			double wesn[4];
			GMT_memcpy (wesn, h->wesn, 4, double);
			if (wesn[XLO] < B[n].G.header.wesn[XLO]) wesn[XLO] = B[n].G.header.wesn[XLO];
			if (wesn[XHI] > B[n].G.header.wesn[XHI]) wesn[XHI] = B[n].G.header.wesn[XHI];
			if (wesn[YLO] < B[n].G.header.wesn[YLO]) wesn[YLO] = B[n].G.header.wesn[YLO];
			if (wesn[YHI] > B[n].G.header.wesn[YHI]) wesn[YHI] = B[n].G.header.wesn[YHI];
			sprintf (Rargs, "-R%.12g/%.12g/%.12g/%.12g", wesn[XLO], wesn[XHI], wesn[YLO], wesn[YHI]);
			GMT_report (GMT, GMT_MSG_DEBUG, "File %s is sampled using region %s\n", B[n].file, Rargs);
		}
		if (do_sample) {	/* One or more reasons to call grdsample before using this grid */
			if (do_sample & 1) {	/* Resampling of the grid */
				sprintf (buffer, "/tmp/grdblend_resampled_%d_%d.nc", (int)getpid(), n);
				sprintf (cmd, "%s %s %s %s -G%s -V%d", B[n].file, Targs, Iargs, Rargs, buffer, GMT->current.setting.verbose);
				if (GMT_is_geographic (GMT, GMT_IN)) strcat (cmd, " -fg");
				GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "Resample %s via grdsample %s\n", B[n].file, cmd);
				if ((status = GMT_grdsample (GMT->parent, 0, cmd))) {	/* Resample the file */
					GMT_report (GMT, GMT_MSG_NORMAL, "Error: Unable to resample file %s - exiting\n", B[n].file);
					GMT_exit (EXIT_FAILURE);
				}
			}
			else {	/* Just reformat to netCDF so this grid may be used as well */
				sprintf (buffer, "/tmp/grdblend_reformatted_%d_%d.nc", (int)getpid(), n);
				sprintf (cmd, "%s %s %s -V%d", B[n].file, Rargs, buffer, GMT->current.setting.verbose);
				if (GMT_is_geographic (GMT, GMT_IN)) strcat (cmd, " -fg");
				GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "Reformat %s via grdreformat %s\n", B[n].file, cmd);
				if ((status = GMT_grdreformat (GMT->parent, 0, cmd))) {	/* Resample the file */
					GMT_report (GMT, GMT_MSG_NORMAL, "Error: Unable to resample file %s - exiting\n", B[n].file);
					GMT_exit (EXIT_FAILURE);
				}
			}
			strncpy (B[n].file, buffer, GMT_TEXT_LEN256);	/* Use the temporary file instead */
			B[n].delete = true;		/* Flag to delete this temporary file when done */
			GMT_err_fail (GMT, GMT_read_grd_info (GMT, B[n].file, &B[n].G.header), B[n].file);	/* Re-read header structure */
			if (overlap_check (GMT, &B[n], h, 0)) continue;	/* In case grdreformat changed the region */
		}
		if (B[n].weight < 0.0) {	/* Negative weight means invert sense of taper */
			B[n].weight = fabs (B[n].weight);
			B[n].invert = true;
		}

		GMT_err_fail (GMT, GMT_open_grd (GMT, B[n].file, &B[n].G, 'r'), B[n].file);	/* Open the grid for incremental row reading */

		/* Here, i0, j0 is the very first col, row to read, while i1, j1 is the very last col, row to read .
		 * Weights at the outside i,j should be 0, and reach 1 at the edge of the inside block */

		/* The following works for both pixel and grid-registered grids since we are here using the i,j to measure the width of the
		 * taper zone in units of dx, dy. */
		 
		B[n].out_i0 = lrint ((B[n].G.header.wesn[XLO] - h->wesn[XLO]) * h->r_inc[GMT_X]);
		B[n].in_i0  = lrint ((B[n].wesn[XLO] - h->wesn[XLO]) * h->r_inc[GMT_X]) - 1;
		B[n].in_i1  = lrint ((B[n].wesn[XHI] - h->wesn[XLO]) * h->r_inc[GMT_X]) + one_or_zero;
		B[n].out_i1 = lrint ((B[n].G.header.wesn[XHI] - h->wesn[XLO]) * h->r_inc[GMT_X]) - B[n].G.header.registration;
		B[n].out_j0 = lrint ((h->wesn[YHI] - B[n].G.header.wesn[YHI]) * h->r_inc[GMT_Y]);
		B[n].in_j0  = lrint ((h->wesn[YHI] - B[n].wesn[YHI]) * h->r_inc[GMT_Y]) - 1;
		B[n].in_j1  = lrint ((h->wesn[YHI] - B[n].wesn[YLO]) * h->r_inc[GMT_Y]) + one_or_zero;
		B[n].out_j1 = lrint ((h->wesn[YHI] - B[n].G.header.wesn[YLO]) * h->r_inc[GMT_Y]) - B[n].G.header.registration;

		B[n].wxl = M_PI * h->inc[GMT_X] / (B[n].wesn[XLO] - B[n].G.header.wesn[XLO]);
		B[n].wxr = M_PI * h->inc[GMT_X] / (B[n].G.header.wesn[XHI] - B[n].wesn[XHI]);
		B[n].wyu = M_PI * h->inc[GMT_Y] / (B[n].G.header.wesn[YHI] - B[n].wesn[YHI]);
		B[n].wyd = M_PI * h->inc[GMT_Y] / (B[n].wesn[YLO] - B[n].G.header.wesn[YLO]);

		if (B[n].out_j0 < 0) {	/* Must skip to first row inside the present -R */
			type = GMT->session.grdformat[B[n].G.header.type][0];
			if (type == 'c')	/* Old-style, 1-D netcdf grid */
				B[n].offset = B[n].G.header.nx * abs (B[n].out_j0);
			else if (type == 'n')	/* New, 2-D netcdf grid */
				B[n].offset = B[n].out_j0;
			else
				B[n].skip = (off_t)(B[n].G.n_byte * abs (B[n].out_j0));	/* do the fseek when we are ready to read first row */
		}

		/* Allocate space for one entire row */

		B[n].z = GMT_memory (GMT, NULL, B[n].G.header.nx, float);
		GMT_report (GMT, GMT_MSG_VERBOSE, "Blend file %s in %g/%g/%g/%g with %s weight %g [%d-%d]\n",
			B[n].G.header.name, B[n].wesn[XLO], B[n].wesn[XHI], B[n].wesn[YLO], B[n].wesn[YHI], sense[B[n].invert], B[n].weight, B[n].out_j0, B[n].out_j1);

		GMT_close_grd (GMT, &B[n].G);
	}

	for (n = 0; n < n_files; n++) {
		free (L[n].file);
		free (L[n].region);
	}
	GMT_free (GMT, L);
	*blend = B;

	return (n_files);
}

void sync_input_rows (struct GMT_CTRL *GMT, int row, struct GRDBLEND_INFO *B, unsigned int n_blend, double half) {
	unsigned int k;

	for (k = 0; k < n_blend; k++) {	/* Get every input grid ready for the new row */
		if (B[k].ignore) continue;
		if (row < B[k].out_j0 || row > B[k].out_j1) {	/* Either done with grid or haven't gotten to this range yet */
			B[k].outside = true;
			if (B[k].open) {
				GMT_close_grd (GMT, &B[k].G);	/* Done with this file */
				B[k].open = false;
				GMT_free (GMT, B[k].z);
				if (B[k].delete) remove (B[k].file);	/* Delete the temporary resampled file */
			}
			continue;
		}
		B[k].outside = false;
		if (row <= B[k].in_j0)		/* Top cosine taper weight */
			B[k].wt_y = 0.5 * (1.0 - cos ((row - B[k].out_j0 + half) * B[k].wyu));
		else if (row >= B[k].in_j1)	/* Bottom cosine taper weight */
			B[k].wt_y = 0.5 * (1.0 - cos ((B[k].out_j1 - row + half) * B[k].wyd));
		else				/* We are inside the inner region; y-weight = 1 */
			B[k].wt_y = 1.0;
		B[k].wt_y *= B[k].weight;

		if (!B[k].open) {
			GMT_err_fail (GMT, GMT_open_grd (GMT, B[k].file, &B[k].G, 'r'), B[k].file);	/* Open the grid for incremental row reading */
			if (B[k].skip) fseek (B[k].G.fp, B[k].skip, SEEK_CUR);	/* Position for native binary files */
			B[k].G.start[0] += B[k].offset;					/* Start position for netCDF files */
			B[k].open = true;
		}

		GMT_err_fail (GMT, GMT_read_grd_row (GMT, &B[k].G, 0, B[k].z), B[k].file);	/* Get one row from this file */
	}
}

void *New_grdblend_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDBLEND_CTRL *C = NULL;
	
	C = GMT_memory (GMT, NULL, 1, struct GRDBLEND_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
	
	C->N.nodata = GMT->session.d_NaN;
	C->Z.scale = 1.0;
	
	return (C);
}

void Free_grdblend_Ctrl (struct GMT_CTRL *GMT, struct GRDBLEND_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->G.file) free (C->G.file);	
	GMT_free (GMT, C);	
}

int GMT_grdblend_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: grdblend [<blendfile> | <grid1> <grid2> ...] -G<outgrid> %s\n", GMT_I_OPT);
	GMT_message (GMT, "\t%s [-Cf|l|o|u] [-N<nodata>] [-Q] [%s] [-W]\n\t[-Z<scale>] [%s] [%s]\n", GMT_Rgeo_OPT, GMT_V_OPT, GMT_f_OPT, GMT_r_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<blendfile> is an ASCII file (or stdin) with blending parameters for each input grid.\n");
	GMT_message (GMT, "\t   Each record has three items: filename -Rw/e/s/n weight.\n");
	GMT_message (GMT, "\t   Relative weights are <weight> inside the given -R and cosine taper to 0 at actual grid -R.\n");
	GMT_message (GMT, "\t   Give filename - weight if inner region should equal the actual region.\n");
	GMT_message (GMT, "\t   Give a negative weight to invert the sense of the taper (i.e., |<weight>| outside given R.\n");
	GMT_message (GMT, "\t   If only filename is given we interpret that as if filename - 1.0 was given.\n");
	GMT_message (GMT, "\t   Grids not in netCDF or native binary format will be converted first.\n");
	GMT_message (GMT, "\t   Grids not coregistered with the output -R -I will be resampled first.\n");
	GMT_message (GMT, "\tAlternatively, if all grids have the same weight (1) and inner region should equal the outer,\n");
	GMT_message (GMT, "\tthen you can instead list all the grid files on the command line (e.g., patches_*.nc).\n");
	GMT_message (GMT, "\tYou must have at least 2 input grids for this mechanism to work.\n");
	GMT_message (GMT, "\t-G <outgrid> is the name of the final 2-D grid.\n");
	GMT_message (GMT, "\t   Only netCDF and native binary grid formats are directly supported;\n");
	GMT_message (GMT, "\t   other formats will be converted via grdreformat when blending is complete.\n");
	GMT_inc_syntax (GMT, 'I', 0);
	GMT_explain_options (GMT, "R");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t-C Clobber modes; no blending takes places as output node is determinde by the mode:\n");
	GMT_message (GMT, "\t   f The first input grid determines the final value.\n");
	GMT_message (GMT, "\t   l The lowest input grid value determines the final value.\n");
	GMT_message (GMT, "\t   o The last input grid overrides any previous value.\n");
	GMT_message (GMT, "\t   u The highest input grid value determines the final value.\n");
	GMT_message (GMT, "\t-N Set value for nodes without constraints [Default is NaN].\n");
	GMT_message (GMT, "\t-Q Grdraster-compatible output without leading grd header [Default writes GMT grid file].\n");
	GMT_message (GMT, "\t   Output grid must be in one of the native binary formats.\n");
	GMT_explain_options (GMT, "V");
	GMT_message (GMT, "\t-W Write out weights only (only applies to a single input file) [make blend grid].\n");
	GMT_message (GMT, "\t-Z Multiply z-values by this scale before writing to file [1].\n");
	GMT_explain_options (GMT, "fF.");
	
	return (EXIT_FAILURE);
}

int GMT_grdblend_parse (struct GMTAPI_CTRL *C, struct GRDBLEND_CTRL *Ctrl, struct GMT_GRDFILE *S, struct GMT_OPTION *options)
{
	/* This parses the options provided to grdblend and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	int err;
 	unsigned int n_errors = 0;
	size_t n_alloc = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Skip input files */
				Ctrl->In.active = true;
				if (n_alloc <= Ctrl->In.n) Ctrl->In.file = GMT_memory (GMT, Ctrl->In.file, n_alloc += GMT_SMALL_CHUNK, char *);
				Ctrl->In.file[Ctrl->In.n++] = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'C':	/* Clobber mode */
				Ctrl->C.active = true;
				switch (opt->arg[0]) {
					case 'u': Ctrl->C.mode = BLEND_UPPER; break;
					case 'l': Ctrl->C.mode = BLEND_LOWER; break;
					case 'f': Ctrl->C.mode = BLEND_FIRST; break;
					case 'o': Ctrl->C.mode = BLEND_LAST; break;
					default:
						GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -C option: Modifiers are f|l|o|u only\n");
						n_errors++;
						break;
				}
				break;
			case 'G':	/* Output filename */
				Ctrl->G.file = strdup (opt->arg);
				Ctrl->G.active = true;
				break;
			case 'I':	/* Grid spacings */
				Ctrl->I.active = true;
				if (GMT_getinc (GMT, opt->arg, Ctrl->I.inc)) {
					GMT_inc_syntax (GMT, 'I', 1);
					n_errors++;
				}
				break;
			case 'N':	/* NaN-value */
				Ctrl->N.active = true;
				if (opt->arg[0])
					Ctrl->N.nodata = (opt->arg[0] == 'N' || opt->arg[0] == 'n') ? GMT->session.d_NaN : atof (opt->arg);
				else {
					GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -N option: Must specify value or NaN\n");
					n_errors++;
				}
				break;
			case 'Q':	/* No header on output */
				Ctrl->Q.active = true;
				break;
			case 'W':	/* Write weights instead */
				Ctrl->W.active = true;
				break;
			case 'Z':	/* z-multiplier */
				Ctrl->Z.active = true;
				Ctrl->Z.scale = atof (opt->arg);
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	GMT_check_lattice (GMT, Ctrl->I.inc, &GMT->common.r.active, &Ctrl->I.active);

	n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error -R option: Must specify region\n");
	n_errors += GMT_check_condition (GMT, Ctrl->I.inc[GMT_X] <= 0.0 || Ctrl->I.inc[GMT_Y] <= 0.0, "Syntax error -I option: Must specify positive dx, dy\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->G.file, "Syntax error -G option: Must specify output file\n");
	if ((err = GMT_grd_get_format (GMT, Ctrl->G.file, &(S->header), false)) != GMT_NOERROR){
		GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error: %s [%s]\n", GMT_strerror(err), Ctrl->G.file); n_errors++;
	}

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {for (k = 0; k < Ctrl->In.n; k++) free (Ctrl->In.file[k]); GMT_free (GMT, Ctrl->In.file); Free_grdblend_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grdblend (struct GMTAPI_CTRL *API, int mode, void *args)
{
	unsigned int col, row, nx_360 = 0, k, kk, m, n_blend, error;
	int status, pcol;
	bool reformat, wrap_x;
	
	uint64_t ij, n_fill, n_tot;
	
	double wt_x, w, wt;
	
	float *z = NULL, no_data_f;
	
	char wmode[2] = {'w', 'W'}, type;
	char *outfile = NULL, outtemp[GMT_BUFSIZ];
	
	struct GRDBLEND_INFO *blend = NULL;
	struct GMT_GRDFILE S;
	struct GMT_GRID *Grid = NULL;
	struct GRDBLEND_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdblend_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdblend_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-VfR:", "r", options)) Return (API->error);
	GMT_grd_init (GMT, &S.header, options, false);
	Ctrl = New_grdblend_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdblend_parse (API, Ctrl, &S, options))) Return (error);
	
	/*---------------------------- This is the grdblend main code ----------------------------*/

	/* Formats other than netcdf (both v3 and new) and native binary must be reformatted at the end */
	reformat = found_unsupported_format (GMT, &S.header, Ctrl->G.file);
	type = GMT->session.grdformat[S.header.type][0];
	if (Ctrl->Q.active && (reformat || (type == 'c' || type == 'n'))) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -Q option: Not supported for grid format %s\n", GMT->session.grdformat[S.header.type]);
		Return (EXIT_FAILURE);
	}
	
	n_fill = n_tot = 0;

	GMT_memcpy (S.header.inc, Ctrl->I.inc, 2, double);
	GMT_memcpy (S.header.wesn, GMT->common.R.wesn, 4, double);
	S.header.registration = (GMT->common.r.active) ? GMT_PIXEL_REG : GMT_GRIDLINE_REG;
	
	GMT_RI_prepare (GMT, &S.header);	/* Ensure -R -I consistency and set nx, ny */

	/* Process blend parameters and populate blend structure and open input files and seek to first row inside the output grid */

	if (Ctrl->In.n <= 1) {	/* Got a blend file (or stdin) */
		if (GMT_Init_IO (API, GMT_IS_TEXTSET, GMT_IS_TEXT, GMT_IN, GMT_REG_DEFAULT, 0, options) != GMT_OK) {	/* Register data input */
			Return (API->error);
		}
		if (GMT_Begin_IO (API, GMT_IS_TEXTSET, GMT_IN) != GMT_OK) {	/* Enables data input and sets access mode */
			Return (API->error);
		}
	}

	status = init_blend_job (GMT, Ctrl->In.file, Ctrl->In.n, &S.header, &blend);

	if (Ctrl->In.n <= 1 && GMT_End_IO (API, GMT_IN, 0) != GMT_OK) {	/* Disables further data input */
		Return (API->error);
	}

	if (status < 0) Return (EXIT_FAILURE);	/* Something went wrong in init_blend_job */
	n_blend = status;
	if (Ctrl->W.active && n_blend > 1) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -W option: Only applies when there is a single input grid file\n");
		Return (EXIT_FAILURE);
	}

	no_data_f = (float)Ctrl->N.nodata;

	/* Initialize header structure for output blend grid */

	S.header.z_min = S.header.z_max = 0.0;

	GMT_err_fail (GMT, GMT_grd_RI_verify (GMT, &S.header, 1), Ctrl->G.file);

	n_tot = GMT_get_nm (GMT, S.header.nx, S.header.ny);

	z = GMT_memory (GMT, NULL, S.header.nx, float);	/* Memory for one output row */

	if (GMT_File_Is_Memory (Ctrl->G.file)) {	/* GMT_grdblend is called by another module; must return as GMT_GRID */
		if ((Grid = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);
		GMT_grd_init (GMT, Grid->header, options, false);

		/* Completely determine the header for the new grid; croak if there are issues.  No memory is allocated here. */
		GMT_err_fail (GMT, GMT_init_newgrid (GMT, Grid, GMT->common.R.wesn, Ctrl->I.inc, S.header.registration), Ctrl->G.file);
		Grid->data = GMT_memory_aligned (GMT, NULL, Grid->header->size, float);	/* Memory for the entire padded grid */
	}
	else {
		if (reformat) {	/* Must use a temporary netCDF file then reformat it at the end */
			sprintf (outtemp, "/tmp/grdblend_temp_%" PRIu64 ".nc", (uint64_t)getpid());	/* Get temporary file name */
			outfile = outtemp;
		}
		else
			outfile = Ctrl->G.file;
		if (!Ctrl->Q.active) GMT_err_fail (GMT, GMT_write_grd_info (GMT, Ctrl->G.file, &S.header), Ctrl->G.file);

		GMT_err_fail (GMT, GMT_open_grd (GMT, Ctrl->G.file, &S, wmode[Ctrl->Q.active]), Ctrl->G.file);
	}
	
	if (Ctrl->Z.active) GMT_report (GMT, GMT_MSG_VERBOSE, "Output data will be scaled by %g\n", Ctrl->Z.scale);

	S.header.z_min = DBL_MAX;	S.header.z_max = -DBL_MAX;	/* These will be updated in the loop below */
	wrap_x = (GMT_is_geographic (GMT, GMT_OUT));	/* Periodic geographic grid */
	if (wrap_x) nx_360 = lrint (360.0 * S.header.r_inc[GMT_X]);

	for (row = 0; row < S.header.ny; row++) {	/* For every output row */

		GMT_memset (z, S.header.nx, float);	/* Start from scratch */

		sync_input_rows (GMT, row, blend, n_blend, S.header.xy_off);	/* Wind each input file to current record and read each of the overlapping rows */

		for (col = 0; col < S.header.nx; col++) {	/* For each output node on the current row */

			w = 0.0;	/* Reset weight */
			for (k = m = 0; k < n_blend; k++) {	/* Loop over every input grid; m will be the number of contributing grids to this node  */
				if (blend[k].ignore) continue;					/* This grid is entirely outside the s/n range */
				if (blend[k].outside) continue;					/* This grid is currently outside the s/n range */
				if (wrap_x) {	/* Special testing for periodic x coordinates */
					pcol = col + nx_360;
					while (pcol > blend[k].out_i1) pcol -= nx_360;
					if (pcol < blend[k].out_i0) continue;	/* This grid is currently outside the w/e range */
				}
				else {	/* Not periodic */
					pcol = col;
					if (pcol < blend[k].out_i0 || pcol > blend[k].out_i1) continue;	/* This grid is currently outside the xmin/xmax range */
				}
				kk = pcol - blend[k].out_i0;					/* kk is the local column variable for this grid */
				if (GMT_is_fnan (blend[k].z[kk])) continue;			/* NaNs do not contribute */
				if (Ctrl->C.active) {	/* Clobber; update z[col] according to selected mode */
					switch (Ctrl->C.mode) {
						case BLEND_FIRST: if (m) continue; break;	/* Already set */
						case BLEND_UPPER: if (m && blend[k].z[kk] <= z[col]) continue; break;	/* Already has a higher value; else set below */
						case BLEND_LOWER: if (m && blend[k].z[kk] >- z[col]) continue; break;	/* Already has a lower value; else set below */
						/* Last case BLEND_LAST is always true in that we always update z[col] */
					}
					z[col] = blend[k].z[kk];					/* Just pick this grid's value */
					w = 1.0;							/* Set weights to 1 */
					m = 1;								/* Pretend only one grid came here */
				}
				else {	/* Do the weighted blending */ 
					if (pcol <= blend[k].in_i0)					/* Left cosine-taper weight */
						wt_x = 0.5 * (1.0 - cos ((pcol - blend[k].out_i0 + S.header.xy_off) * blend[k].wxl));
					else if (pcol >= blend[k].in_i1)					/* Right cosine-taper weight */
						wt_x = 0.5 * (1.0 - cos ((blend[k].out_i1 - pcol + S.header.xy_off) * blend[k].wxr));
					else								/* Inside inner region, weight = 1 */
						wt_x = 1.0;
					wt = wt_x * blend[k].wt_y;					/* Actual weight is 2-D cosine taper */
					if (blend[k].invert) wt = blend[k].weight - wt;			/* Invert the sense of the tapering */
					z[col] += (float)(wt * blend[k].z[kk]);				/* Add up weighted z*w sum */
					w += wt;							/* Add up the weight sum */
					m++;								/* Add up the number of contributing grids */
				}
			}

			if (m) {	/* OK, at least one grid contributed to an output value */
				if (!Ctrl->W.active) {		/* Want output z blend */
					z[col] = (float)((w == 0.0) ? 0.0 : z[col] / w);	/* Get weighted average z */
					if (Ctrl->Z.active) z[col] *= (float)Ctrl->Z.scale;		/* Apply the global scale here */
				}
				else		/* Get the weight only */
					z[col] = (float)w;				/* Only interested in the weights */
				n_fill++;						/* One more cell filled */
				if (z[col] < S.header.z_min) S.header.z_min = z[col];	/* Update the extrema for output grid */
				if (z[col] > S.header.z_max) S.header.z_max = z[col];
			}
			else			/* No grids covered this node, defaults to the no_data value */
				z[col] = no_data_f;
		}
		if (Grid) {	/* Must copy entire row to grid */
			ij = GMT_IJP (Grid->header, row, 0);
			GMT_memcpy (&(Grid->data[ij]), z, S.header.nx, float);
		}
		else
			GMT_err_fail (GMT, GMT_write_grd_row (GMT, &S, z), Ctrl->G.file);

		if (row%10 == 0)  GMT_report (GMT, GMT_MSG_VERBOSE, "Processed row %7ld of %d\r", row, S.header.ny);

	}
	GMT_report (GMT, GMT_MSG_VERBOSE, "Processed row %7ld\n", row);

	if (Grid) {	/* Must write entire grid */
		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->G.file, Grid) != GMT_OK) {
			Return (API->error);
		}
	}
	else {	/* Finish the line-by-line writing */
		GMT_close_grd (GMT, &S);	/* Close the output gridfile */
		if (!Ctrl->Q.active) GMT_err_fail (GMT, GMT_update_grd_info (GMT, Ctrl->G.file, &S.header), Ctrl->G.file);
	}
	GMT_free (GMT, z);

	for (k = 0; k < n_blend; k++) if (blend[k].open) {
		GMT_free (GMT, blend[k].z);
		GMT_close_grd (GMT, &blend[k].G);	/* Close all input grd files still open */
	}

	if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) {
		char empty[GMT_TEXT_LEN64];
		GMT_report (GMT, GMT_MSG_VERBOSE, "Blended grid size of %s is %d x %d\n", Ctrl->G.file, S.header.nx, S.header.ny);
		if (n_fill == n_tot)
			GMT_report (GMT, GMT_MSG_VERBOSE, "All nodes assigned values\n");
		else {
			if (GMT_is_fnan (no_data_f))
				strcpy (empty, "NaN");
			else
				sprintf (empty, "%g", no_data_f);
			GMT_report (GMT, GMT_MSG_VERBOSE, "%" PRIu64 " nodes assigned values, %" PRIu64 " set to %s\n", n_fill, n_tot - n_fill, empty);
		}
	}

	GMT_free (GMT, blend);

	if (reformat) {	/* Must reformat the output grid to the non-supported format */
		int status;
		char cmd[GMT_BUFSIZ];
		sprintf (cmd, "%s %s -V%d", outfile, Ctrl->G.file, GMT->current.setting.verbose);
		GMT_report (GMT, GMT_MSG_LONG_VERBOSE, "Reformat %s via grdreformat %s\n", outfile, cmd);
		if ((status = GMT_grdreformat (GMT->parent, 0, cmd))) {	/* Resample the file */
			GMT_report (GMT, GMT_MSG_NORMAL, "Error: Unable to resample file %s.\n", outfile);
		}
	}

	Return (GMT_OK);
}
