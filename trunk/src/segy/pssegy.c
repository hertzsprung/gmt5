/*--------------------------------------------------------------------
 *	$Id$
 *
 *    Copyright (c) 1999-2012 by T. Henstock
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/* pssegy program to plot segy files in postscript with variable trace spacing option
 * uses the GMT pslib imagemask routines to plot a
 * 1-bit depth bitmap which will not obliterate stuff underneath!
 *
 * Author:	Tim Henstock (then@noc.soton.ac.uk)
 * Date:	1-July-1996
 * Version:	1.0
 *
 * Bug fixes:	1.1, 11-6-96 remove undesirable normalization introduced by -Z option. Add -U option for reduction.
 *
 * enhancements: 1.2, 7-20-98 add option to match location of traces from file
 *
 *               1.3, 1/7/99 check number of samples trace by trace to cope with SEGY with variable trace length
 *
 *               2.0, 5/7/99 update for GMT 3.3.1
 *
 *               2.1 10/4/2001 fix unreduce bug, modify to byte-swap if necessary, make 64-bit safe
 *
 *                   10/7/2009 fix bug when reading trace locations from arbitrary header locations,
 *                             8 bytes copied, should be 4 bytes
 *		2.2 7/30/2010 Ported to GMT 5 P. Wessel (global variables removed)
 *
 * This program is free software and may be copied or redistributed under the terms
 * of the GNU public license.
 */

#define THIS_MODULE k_mod_pssegy /* I am pssegy */

#include "gmt_dev.h"
#include "segy_io.h"

#define GMT_PROG_OPTIONS "->BJKOPRUVXYcpt"

#define B_ID	0	/* Indices into Q values */
#define U_ID	1
#define X_ID	2
#define Y_ID	3

#define PLOT_CDP	1
#define PLOT_OFFSET	2

struct PSSEGY_CTRL {
	struct In {	/* -In */
		bool active;
		char *file;
	} In;
	struct A {	/* -A */
		bool active;
	} A;
	struct C {	/* -C<cpt> */
		bool active;
		double value;
	} C;
	struct D {	/* -D */
		bool active;
		double value;
	} D;
	struct E {	/* -E */
		bool active;
		double value;
	} E;
	struct F {	/* -F<fill> */
		bool active;
		double rgb[4];
	} F;
	struct I {	/* -I */
		bool active;
	} I;
	struct L {	/* -L */
		bool active;
		uint32_t value;
	} L;
	struct M {	/* -M */
		bool active;
		uint32_t value;
	} M;
	struct N {	/* -N */
		bool active;
	} N;
	struct Q {	/* -Qb|u|x|y */
		bool active[4];
		double value[4];
	} Q;
	struct S {	/* -S */
		bool active;
		unsigned int mode;
		int value;
	} S;
	struct T {	/* -T */
		bool active;
		char *file;
	} T;
	struct W {	/* -W */
		bool active;
	} W;
	struct Z {	/* -Z */
		bool active;
	} Z;
};

void *New_pssegy_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct PSSEGY_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct PSSEGY_CTRL);

	/* Initialize values whose defaults are not 0/false/NULL */

	C->A.active = !GMT_BIGENDIAN;
	C->M.value = 10000;
	C->Q.value[X_ID] = 1.0; /* Ctrl->Q.value[X_ID], Ctrl->Q.value[Y_ID] are trace and sample interval */
	return (C);
}

void Free_pssegy_Ctrl (struct GMT_CTRL *GMT, struct PSSEGY_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C && C->In.file) free (C->In.file);
	if (C && C->T.file) free (C->T.file);
	GMT_free (GMT, C);
}

int GMT_pssegy_usage (struct GMTAPI_CTRL *API, int level)
{
	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_Message (API, GMT_TIME_NONE, "usage: pssegy [<segyfile>] -D<dev> -F<color> | -W %s %s \n", GMT_Jx_OPT, GMT_Rx_OPT);
	GMT_Message (API, GMT_TIME_NONE, "\t[-A] [-C<clip>] [-E<slop>] [-I] [-K] [-L<nsamp>] [-M<ntraces>] [-N]\n");
	GMT_Message (API, GMT_TIME_NONE, "\t[-O] [-P] [-Q<mode><value>] [-S<header>] [-T<tracefile>] [%s]\n", GMT_U_OPT);
	GMT_Message (API, GMT_TIME_NONE, "\t[%s] [-W] [%s] [%s] [-Z] [%s] [%s] [%s]\n\n", GMT_V_OPT, GMT_X_OPT, GMT_Y_OPT, GMT_c_OPT, GMT_p_OPT, GMT_t_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_Message (API, GMT_TIME_NONE, "\t<segyfile> is an IEEE SEGY file [or standard input] \n\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-D<dev> to give deviation in X units of plot for 1.0 on scaled trace\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-F<gray> to fill variable area with shade <gray>\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-F<color> to fill variable area with color\n");
	GMT_Message (API, GMT_TIME_NONE, "\t	only a single color for the bitmap though!\n");
	GMT_Option (API, "JX,R");
	GMT_Message (API, GMT_TIME_NONE, "\tNB units for y are s or km\n");
	GMT_Message (API, GMT_TIME_NONE, "\n\tOPTIONS:\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-A flips the default byte-swap state (default assumes data have a bigendian byte-order)\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-C<clip> to clip scaled trace excursions at <clip>, applied after bias\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-E<error> slop to allow for -T. recommended in case of arithmetic errors!\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-I to fill negative rather than positive excursions\n");
	GMT_Option (API, "K");
	GMT_Message (API, GMT_TIME_NONE, "\t-L<nsamp> to override number of samples\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-M<ntraces> to fix number of traces. Default reads all traces.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t  -M0 will read number in binary header, -Mn will attempt to read only n traces.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-N to trace normalize the plot\n");
	GMT_Message (API, GMT_TIME_NONE, "\t	order of operations: [normalize][bias][clip](deviation)\n");
	GMT_Option (API, "O,P");
	GMT_Message (API, GMT_TIME_NONE, "\t-Q<mode><value> can be used to change 4 different settings:\n");
	GMT_Message (API, GMT_TIME_NONE, "\t  -Qb<bias> to bias scaled traces (-B-0.1 subtracts 0.1 from values)\n");
	GMT_Message (API, GMT_TIME_NONE, "\t  -Qu<redvel> to apply reduction velocity (-ve removes reduction already present)\n");
	GMT_Message (API, GMT_TIME_NONE, "\t  -Qx<mult> to multiply trace locations by <mult>\n");
	GMT_Message (API, GMT_TIME_NONE, "\t  -Qy<dy> to override sample interval\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-S<header> to set variable spacing\n");
	GMT_Message (API, GMT_TIME_NONE, "\t	<header> is c for cdp or o for offset\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-T<filename> to look in filename for a list of locations to select traces\n");
	GMT_Message (API, GMT_TIME_NONE, "\t	(same units as header * X, ie values printed by previous -V run)\n");
	GMT_Option (API, "U,V");
	GMT_Message (API, GMT_TIME_NONE, "\t-W to plot wiggle trace (must specify either -W or -F)\n");
	GMT_Option (API, "X");
	GMT_Message (API, GMT_TIME_NONE, "\t-Z to suppress plotting traces whose rms amplitude is 0.\n");
	GMT_Option (API, "c,p,t,.");

	return (EXIT_FAILURE);
}

int GMT_pssegy_parse (struct GMTAPI_CTRL *C, struct PSSEGY_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to pssegy and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_files = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {

			case '<':	/* Input files */
				if (n_files++ == 0) Ctrl->In.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'A':	/* Swap data */
				Ctrl->A.active = !Ctrl->A.active;
				break;
			case 'C':	/* trace clip */
				Ctrl->C.active = true;
				Ctrl->C.value = (float) atof (opt->arg);
				break;
			case 'D':	/* trace scaling */
				Ctrl->D.active = true;
				Ctrl->D.value = atof (opt->arg);
				break;
			case 'E':
				Ctrl->E.active = true;
				Ctrl->E.value = atof (opt->arg);
				break;
			case 'F':
				Ctrl->F.active = true;
				if (GMT_getrgb (GMT, opt->arg, Ctrl->F.rgb)) {
					n_errors++;
					GMT_rgb_syntax (GMT, 'F', " ");
				}
				break;
			case 'I':
				Ctrl->I.active = true;
				break;
			case 'L':
				Ctrl->L.active = true;
				Ctrl->L.value = atoi (opt->arg);
				break;
			case 'M':
				Ctrl->M.active = true;
				Ctrl->M.value = atoi (opt->arg);
				break;
			case 'N':	/* trace norm. */
				Ctrl->N.active = true;
				break;
			case 'Q':
				switch (opt->arg[0]) {
					case 'b':	/* Trace bias */
						Ctrl->Q.active[B_ID] = true;
						Ctrl->Q.value[B_ID] = atof (opt->arg);
						break;
					case 'u':	/* reduction velocity application */
						Ctrl->Q.active[U_ID] = true;
						Ctrl->Q.value[U_ID] = atof (opt->arg);
						break;
					case 'x': /* over-rides of header info */
						Ctrl->Q.active[X_ID] = true;
						Ctrl->Q.value[X_ID] = atof (opt->arg);
						break;
					case 'y': /* over-rides of header info */
						Ctrl->Q.active[Y_ID] = true;
						Ctrl->Q.value[Y_ID] = atof (opt->arg);
						break;
				}
				break;
			case 'S':
				Ctrl->S.active = true;
				switch (opt->arg[0]) {
					case 'o':
						Ctrl->S.mode = PLOT_OFFSET;
						break;
					case 'c':
						Ctrl->S.mode = PLOT_CDP;
						break;
					case 'b':
						Ctrl->S.value = atoi (&opt->arg[1]);
						break;
				}
				break;
			case 'T':	/* plot traces only at listed locations */
				Ctrl->T.active = true;
				Ctrl->T.file = strdup (opt->arg);
				break;
			case 'W':
				Ctrl->W.active = true;
				break;
			case 'Z':
				Ctrl->Z.active = true;
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}
	n_errors += GMT_check_condition (GMT, Ctrl->T.active && !Ctrl->T.file, "Syntax error: Option -T requires a file name\n");
	n_errors += GMT_check_condition (GMT, Ctrl->T.active && Ctrl->T.file && !access (Ctrl->T.file, R_OK), "Syntax error: Cannot file file %s\n", Ctrl->T.file);
	n_errors += GMT_check_condition (GMT, Ctrl->E.value < 0.0, "Syntax error -E option: Slop cannot be negative\n");
	n_errors += GMT_check_condition (GMT, Ctrl->I.active && !Ctrl->F.active, "Syntax error: Must specify -F with -I\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->F.active && !Ctrl->W.active, "Syntax error: Must specify -F or -W\n");
	n_errors += GMT_check_condition (GMT, Ctrl->D.value <= 0.0, "Syntax error: Must specify a positive deviation\n");
	n_errors += GMT_check_condition (GMT, Ctrl->S.mode == PLOT_CDP && Ctrl->S.mode == PLOT_OFFSET, "Syntax error: Cannot specify more than one trace location key\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

float segy_rms (float *data, uint32_t n_samp)
{ /* function to return rms amplitude of n_samp values from the array data */
	uint32_t ix;
	double sumsq = 0.0;

	for (ix = 0; ix < n_samp; ix++)
		sumsq += ((double) data[ix])*((double) data[ix]);
	sumsq /= ((double) n_samp);
	sumsq = sqrt (sumsq);
	return (float) sumsq;
}

int segy_paint (int ix, int iy, unsigned char *bitmap, int bm_nx, int bm_ny)	/* pixel to paint */
{
	int byte, quot, rem;
	static unsigned char bmask[8]={128, 64, 32, 16, 8, 4, 2, 1};

	quot = ix / 8;
	rem = ix - quot * 8;

	if ((quot >= bm_nx-1) || (iy >= bm_ny-1) || (ix < 0) || (iy < 0)) return (-1);	/* outside bounds of plot array */

	byte = (bm_ny - iy - 1) * bm_nx + quot;	/* find byte to paint - flip vertical! */
	bitmap[byte] = bitmap[byte] | bmask[rem];
	return (0);
}

void segy_wig_bmap (struct GMT_CTRL *GMT, double x0, float data0, float data1, double y0, double y1, unsigned char *bitmap, int bm_nx, int bm_ny) /* apply current sample with all options to bitmap */
{
	int px0, px1, py0, py1, ix, iy;
	double xp0, xp1, yp0, yp1, slope;

	GMT_geo_to_xy (GMT, x0+ (double)data0, y0, &xp0, &yp0); /* returns 2 ends of line segment in plot coords */
	GMT_geo_to_xy (GMT, x0+ (double)data1, y1, &xp1, &yp1);
	slope = (yp1 - yp0) / (xp1 - xp0);

	px0 = lrint (xp0 * PSL_DOTS_PER_INCH);
	px1 = lrint (xp1 * PSL_DOTS_PER_INCH);
	py0 = lrint (yp0 * PSL_DOTS_PER_INCH);
	py1 = lrint (yp1 * PSL_DOTS_PER_INCH);

	/* now have the pixel locations for the two samples - join with a line..... */
	if (fabs (slope) <= 1.0) { /* more pixels needed in x direction */
		if (px0 < px1) {
			for (ix = px0; ix <= px1; ix++) {
				iy = py0 + lrint (slope * (float) (ix - px0));
				segy_paint (ix, iy, bitmap, bm_nx, bm_ny);
			}
		}
		else {
			for (ix = px1; ix <= px0; ix++) {
				iy = py0 + lrint (slope * (float) (ix - px0));
				segy_paint (ix, iy, bitmap, bm_nx, bm_ny);
			}

		}
	}
	else { /* more pixels needed in y direction */
		if (py0 < py1) {
			for (iy = py0; iy <= py1; iy++) {
				ix = px0 + lrint (((float) (iy - py0)) / slope);
				segy_paint (ix, iy, bitmap, bm_nx, bm_ny);
			}
		}
		else {
			for (iy=py1; iy<=py0; iy++) {
				ix = px0 + lrint (((float) (iy - py0)) / slope);
				segy_paint (ix, iy, bitmap, bm_nx, bm_ny);
			}
		}
	}
}

void segy_shade_bmap (struct GMT_CTRL *GMT, double x0, float data0, float data1, double y0, double y1, int negative, unsigned char *bitmap, int bm_nx, int bm_ny) /* apply current samples with all options to bitmap */
{
	int px0, px00, py0, py1, ixx, ix, iy;
	double xp0, xp00, xp1, yp0, yp1, interp, slope;

	if ((data0 * data1) < 0.0) {
		/* points to plot are on different sides of zero - interpolate to find out where zero is */
		interp = y0 + (double)data0 * ((y0 - y1) / (double)(data1 - data0));
		if (((data0 < 0.0) && negative) || ((data0 > 0.0) && !negative)) {
			/* plot from top to zero */
			y1 = interp;
			data1 = 0.0;
		}
		else {
			y0 = interp;
			data0 = 0.0;
		}
	}


	GMT_geo_to_xy (GMT, x0+(double)data0, y0, &xp0, &yp0); /* returns 2 ends of line segment in plot coords */
	GMT_geo_to_xy (GMT, x0+(double)data1, y1, &xp1, &yp1);
	GMT_geo_to_xy (GMT, x0, y0, &xp00, &yp0); /* to get position of zero */

	slope = (yp1 - yp0) / (xp1 - xp0);

	px0  = lrint (0.49 + xp0  * PSL_DOTS_PER_INCH);
	px00 = lrint (0.49 + xp00 * PSL_DOTS_PER_INCH);
	py0  = lrint (0.49 + yp0  * PSL_DOTS_PER_INCH);
	py1  = lrint (0.49 + yp1  * PSL_DOTS_PER_INCH);


	/*  can rasterize simply by looping over values of y */
	if (py0 < py1) {
		for (iy = py0; iy <= py1; iy++) {
			ixx = px0 + lrint (((float) (iy - py0)) / slope);
			if (ixx < px00) {
				for (ix = ixx; ix <= px00; ix++) segy_paint (ix, iy, bitmap, bm_nx, bm_ny);
			}
			else {
				for (ix = px00; ix <= ixx; ix++) segy_paint (ix, iy, bitmap, bm_nx, bm_ny);
			}
		}
	}
	else {
		for (iy = py1; iy <= py0; iy++) {
			ixx = px0 + lrint (((float) (iy - py0)) / slope);
			if (ixx < px00) {
				for (ix = ixx; ix <= px00; ix++) segy_paint (ix, iy, bitmap, bm_nx, bm_ny);
			}
			else {
				for (ix = px00; ix<= ixx; ix++) segy_paint (ix, iy, bitmap, bm_nx, bm_ny);
			}
		}
	}
}

void segy_plot_trace (struct GMT_CTRL *GMT, float *data, double dy, double x0, int n_samp, int do_fill, int negative, int plot_wig, float toffset, unsigned char *bitmap, int bm_nx, int bm_ny)
	/* shell function to loop over all samples in the current trace, determine plot options
	 * and call the appropriate bitmap routine */
{
	int iy, paint_wiggle;
	double y0 = 0.0, y1;

	for (iy = 1; iy < n_samp; iy++) {	/* loop over samples on trace - refer to pairs iy-1, iy */
		y1 = dy * (float) iy + toffset;
		if (plot_wig) segy_wig_bmap (GMT, x0, data[iy-1],data[iy], y0, y1, bitmap, bm_nx, bm_ny); /* plotting wiggle */
		if (do_fill) {	/* plotting VA -- check data points first */
			paint_wiggle = ((!negative && ((data[iy-1] >= 0.0) || (data[iy] >= 0.0))) || (negative && ((data[iy-1] <= 0.0) || (data[iy] <= 0.0))));
			if (paint_wiggle) segy_shade_bmap (GMT, x0, data[iy-1], data[iy], y0, y1, negative, bitmap, bm_nx, bm_ny);
		}
		y0=y1;
	}
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_pssegy_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_pssegy (void *V_API, int mode, void *args)
{
	bool plot_it = false;
	unsigned int i, nm, ix, iy;
	uint32_t n_samp = 0, n_tracelist = 0;
	int error = 0, check, bm_nx, bm_ny;

	float scale = 1.0f, toffset = 0.0f, *data = NULL;
	double xlen, ylen, xpix, ypix, x0, test, *tracelist = NULL, trans[3] = {-1.0, -1.0, -1.0};

	unsigned char *bitmap = NULL;

	char reelhead[3200];
	SEGYHEAD *header = NULL;
	SEGYREEL binhead;

	FILE *fpi = NULL, *fpt = NULL;

	struct PSSEGY_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;		/* General GMT interal parameters */
	struct GMT_OPTION *options = NULL;
	struct PSL_CTRL *PSL = NULL;				/* General PSL interal parameters */
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_NOT_A_SESSION);
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_pssegy_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_pssegy_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments; return if errors are encountered */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, GMT_PROG_OPTIONS, options)) Return (API->error);
	Ctrl = New_pssegy_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_pssegy_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the pssegy main code ----------------------------*/

	if (Ctrl->In.active) {
		GMT_Report (API, GMT_MSG_VERBOSE, "Will read segy file %s\n", Ctrl->In.file);
		if ((fpi = fopen (Ctrl->In.file, "rb")) == NULL) {
			GMT_Report (API, GMT_MSG_NORMAL, "Cannot find segy file %s\n", Ctrl->In.file);
			Return (EXIT_FAILURE);
		}
	}
	else {
		GMT_Report (API, GMT_MSG_VERBOSE, "Will read segy file from standard input\n");
		if (fpi == NULL) fpi = stdin;
	}

	if ((fpt = fopen (Ctrl->T.file, "r")) == NULL) {
		GMT_Report (API, GMT_MSG_NORMAL, "Cannot find trace list file %s\n", Ctrl->T.file);
		Return (EXIT_FAILURE);
	}

	if (!GMT_IS_LINEAR (GMT)) GMT_Report (API, GMT_MSG_VERBOSE, "Warning: you asked for a non-rectangular projection. \n It will probably still work, but be prepared for problems\n");
	if (Ctrl->Q.value[Y_ID]) GMT_Report (API, GMT_MSG_VERBOSE, "Overriding sample interval dy = %f\n", Ctrl->Q.value[Y_ID]);


	if (Ctrl->T.active) { /* must read in file of desired trace locations */
		tracelist = GMT_memory (GMT, NULL, GMT_CHUNK, double);
		n_tracelist = GMT_CHUNK;
		ix = 0;
		while ((fscanf (fpt, "%lf", &test)) != EOF) {
			tracelist[ix] = test;
			ix++;
			if (ix == n_tracelist) {	/* need more memory in array */
				n_tracelist += GMT_CHUNK;
				tracelist = GMT_memory (GMT, tracelist, n_tracelist, double);
			}
		}
		n_tracelist = (int)ix;
		GMT_Report (API, GMT_MSG_VERBOSE, "read in %d trace locations\n", n_tracelist);
	}

	/* set up map projection and PS plotting */
	if (GMT_map_setup (GMT, GMT->common.R.wesn)) Return (GMT_RUNTIME_ERROR);
	PSL = GMT_plotinit (GMT, options);
	GMT_plane_perspective (GMT, GMT->current.proj.z_project.view_plane, GMT->current.proj.z_level);
	GMT_plotcanvas (GMT);	/* Fill canvas if requested */

	/* define area for plotting and size of array for bitmap */
	xlen = GMT->current.proj.rect[XHI] - GMT->current.proj.rect[XLO];
	xpix = xlen * PSL_DOTS_PER_INCH;	/* pixels in x direction */
	/* xpix /= 8.0;
	bm_nx = 1 +(int) xpix;*/
	bm_nx = lrint (ceil (xpix / 8.0)); /* store 8 pixels per byte in x direction but must have
		whole number of bytes per scan */
	ylen = GMT->current.proj.rect[YHI] - GMT->current.proj.rect[YLO];
	ypix = ylen * PSL_DOTS_PER_INCH;	/* pixels in y direction */
	bm_ny = lrint (ypix);
	nm = bm_nx * bm_ny;

	/* read in reel headers from segy file */
	if ((check = get_segy_reelhd (fpi, reelhead)) != true) Return (GMT_RUNTIME_ERROR);
	if ((check = get_segy_binhd (fpi, &binhead)) != true) Return (GMT_RUNTIME_ERROR);

	if (Ctrl->A.active) {
		/* this is a little-endian system, and we need to byte-swap ints in the header - we only
		   use a few of these*/
		GMT_Report (API, GMT_MSG_VERBOSE, "Swapping bytes for ints in the headers\n");
		binhead.num_traces = bswap16 (binhead.num_traces);
		binhead.nsamp = bswap16 (binhead.nsamp);
		binhead.dsfc = bswap16 (binhead.dsfc);
		binhead.sr = bswap16 (binhead.sr);
	}

/* set parameters from the reel headers */
	if (!Ctrl->M.value) Ctrl->M.value = binhead.num_traces;

	GMT_Report (API, GMT_MSG_VERBOSE, "Number of traces in header is %d\n", Ctrl->M.value);

	if (!Ctrl->L.value) {/* number of samples not overridden*/
		Ctrl->L.value = binhead.nsamp;
		GMT_Report (API, GMT_MSG_VERBOSE, "Number of samples per trace is %d\n", Ctrl->L.value);
	}
	else if ((Ctrl->L.value != binhead.nsamp) && (binhead.nsamp))
		GMT_Report (API, GMT_MSG_VERBOSE, "Warning nsampr input %d, nsampr in header %d\n", Ctrl->L.value,  binhead.nsamp);

	if (!Ctrl->L.value) { /* no number of samples still - a problem! */
		GMT_Report (API, GMT_MSG_NORMAL, "Error, number of samples per trace unknown\n");
		Return (EXIT_FAILURE);
	}

	GMT_Report (API, GMT_MSG_VERBOSE, "Number of samples for reel is %d\n", Ctrl->L.value);

	if (binhead.dsfc != 5) GMT_Report (API, GMT_MSG_VERBOSE, "Warning: data not in IEEE format\n");

	if (!Ctrl->Q.value[Y_ID]) {
		Ctrl->Q.value[Y_ID] = (double) binhead.sr; /* sample interval of data (microseconds) */
		Ctrl->Q.value[Y_ID] /= 1000000.0;
		GMT_Report (API, GMT_MSG_VERBOSE, "Sample interval is %f s\n", Ctrl->Q.value[Y_ID]);
	}
	else if ((Ctrl->Q.value[Y_ID] != binhead.sr) && (binhead.sr)) /* value in header overridden by input */
		GMT_Report (API, GMT_MSG_VERBOSE, "Warning dy input %f, dy in header %f\n", Ctrl->Q.value[Y_ID], (float)binhead.sr);

	if (!Ctrl->Q.value[Y_ID]) { /* still no sample interval at this point is a problem! */
		GMT_Report (API, GMT_MSG_NORMAL, "Error, no sample interval in reel header\n");
		Return (EXIT_FAILURE);
	}


	bitmap = GMT_memory (GMT, NULL, nm, unsigned char);

	ix=0;
	while ((ix < Ctrl->M.value) && (header = get_segy_header (fpi))) {
		/* read traces one by one */
		if (Ctrl->S.mode == PLOT_OFFSET) {
			/* plot traces by offset, cdp, or input order */
			int32_t tmp = header->sourceToRecDist;
			if (Ctrl->A.active) {
				uint32_t *p = (uint32_t *)&tmp;
				*p = bswap32 (*p);
			}
			x0 = (double) tmp;
		}
		else if (Ctrl->S.mode == PLOT_CDP) {
			int32_t tmp = header->cdpEns;
			if (Ctrl->A.active) {
				uint32_t *p = (uint32_t *)&tmp;
				*p = bswap32 (*p);
			}
			x0 = (double) tmp;
		}
		else if (Ctrl->S.value) {
			/* get value starting at Ctrl->S.value of header into a double */
			uint32_t tmp;
			memcpy (&tmp, &header[Ctrl->S.value], sizeof (uint32_t));
			x0 = (Ctrl->A.active) ? (double) bswap32 (tmp) : (double) tmp;
		}
		else
			x0 = (1.0 + (double) ix);

		x0 *= Ctrl->Q.value[X_ID];

		if (Ctrl->A.active) {
			/* need to permanently byte-swap some things in the trace header
				 do this after getting the location of where traces are plotted
				 in case the general Ctrl->S.value case overlaps a defined header
				 in a strange way */
			uint32_t *p = (uint32_t *)&header->sourceToRecDist;
			*p = bswap32 (*p);
			header->sampleLength = bswap16 (header->sampleLength);
			header->num_samps = bswap32 (header->num_samps);
		}

		/* now check that on list to plot if list exists */
		if (n_tracelist) {
			plot_it = false;
			for (i = 0; i< n_tracelist; i++) {
				if (fabs (x0 - tracelist[i]) <= Ctrl->E.value) plot_it = true;
			}
		}

		if (Ctrl->Q.value[U_ID]) {
			toffset = (float) - (fabs ((double)(header->sourceToRecDist)) / Ctrl->Q.value[U_ID]);
			GMT_Report (API, GMT_MSG_VERBOSE, "pssegy: time shifted by %f\n", toffset);
		}

		data = get_segy_data (fpi, header); /* read a trace */
		/* get number of samples in _this_ trace (e.g. OMEGA has strange ideas about SEGY standard)
		or set to number in reel header */
		if (!(n_samp = samp_rd (header))) n_samp = Ctrl->L.value;

		if (Ctrl->A.active) {
			/* need to swap the order of the bytes in the data even though assuming IEEE format */
			uint32_t tmp;
			for (iy = 0; iy < n_samp; ++iy) {
				memcpy (&tmp, &data[iy], sizeof(uint32_t));
				tmp = bswap32 (tmp);
				memcpy (&data[iy], &tmp, sizeof(uint32_t));
			}
		}

		if (Ctrl->N.active || Ctrl->Z.active) {
			scale = segy_rms (data, n_samp);
			GMT_Report (API, GMT_MSG_VERBOSE, "pssegy: \t\t rms value is %f\n",scale);
		}
		for (iy = 0; iy < n_samp; iy++) { /* scale bias and clip each sample in the trace */
			if (Ctrl->N.active) data[iy] /= scale;
			data[iy] += (float)Ctrl->Q.value[B_ID];
			if (Ctrl->C.active && (fabs(data[iy]) > Ctrl->C.value)) data[iy] = (float)(Ctrl->C.value*data[iy] / fabs (data[iy])); /* apply bias and then clip */
			data[iy] *= (float)Ctrl->D.value;
		}

		if ((!Ctrl->Z.active || scale) && (plot_it || !n_tracelist)) {
			GMT_Report (API, GMT_MSG_VERBOSE, "pssegy: trace %d plotting at %f \n", ix+1, x0);
			segy_plot_trace (GMT, data, Ctrl->Q.value[Y_ID], x0, (int)n_samp, (int)Ctrl->F.active, (int)Ctrl->I.active, (int)Ctrl->W.active, toffset, bitmap, bm_nx, bm_ny);
		}
		free (data);
		free (header);
		ix++;
	}

	GMT_map_clip_on (GMT, GMT->session.no_rgb, 3); /* set a clip at the map boundary since the image space overlaps a little */
	PSL_plotbitimage (PSL, 0.0, 0.0, xlen, ylen, 1, bitmap, 8*bm_nx, bm_ny, trans, Ctrl->F.rgb);
	/* have to multiply by 8 since pslib version of ps_imagemask is based on a _pixel_ count, whereas pssegy uses _byte_ count internally */
	GMT_map_clip_off (GMT);

	if (fpi != stdin) fclose (fpi);

	GMT_plane_perspective (GMT, -1, 0.0);
	GMT_plotend (GMT);

	GMT_free (GMT, bitmap);
	GMT_free (GMT, tracelist);

	Return (EXIT_SUCCESS);
}
