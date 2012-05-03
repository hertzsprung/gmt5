/* $Id$
 *
 * Copyright (c) 1991-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
 * See LICENSE.TXT file for copying and redistribution conditions.
 *
 * img2grd.c
 *
 * img2grd reads an "img" format file (used by Sandwell and Smith
 * in their estimates of gravity and depth from satellite altimetry),
 * extracts a Region [with optional N by N averaging], and writes out
 * the data [or Track control] as a pixel-registered GMT "grd" file, 
 * preserving the Mercator projection (gmtdefaults PROJ_ELLIPSOID = Sphere) 
 * inherent in the img file.
 *
 * img file is read from dir GMT_IMGDIR if this is defined as an
 * environment variable; else img file name is opened directly.
 *
 * The coordinates in the img file are determined by the latitude and
 * longitude span of the img file and the width of an img pixel in
 * minutes of longitude.  These may be changed from their default values
 * using the lower-case [-W<maxlon>] [-D<minlat>/<maxlat>] [-I<minutes>]
 * but must be set to match the coverage of the input img file.
 *
 * The user-supplied -R<w>/<e>/<s>/<n> will be adjusted by rounding
 * outward so that the actual range spanned by the file falls exactly on
 * the edges of the input pixels.  If the user uses the option to 
 * average the input pixels into N by N square block averages, then the
 * -R will be adjusted so that the range spans the block averages.
 *
 * The coordinates in the output file are in spherical mercator projected
 * units ranging from 0,0 in the lower left corner of the output to
 * xmax, ymax in the upper right corner, where xmax,ymax are the width
 * and height of the (spherical) Mercator map with -Rww/ee/ss/nn and -Jm1.
 * Here ww/ee/ss/nn are the outward-rounded range values.
 *
 * Further details on the coordinate system can be obtained from the
 * comments in gmt_imgsubs.c and gmt_imgsubs.h
 *
 * This is a complete rebuild (for v3.1) of the old program by this name.
 * New functionality added here is the averaging option [-N] and the
 * options to define -I, -W, -y, which permit handling very early versions
 * of these files and anticipate higher-resolutions to come down the road.
 * Also added is the option to look for the img file in GMT_IMGDIR
 *
 * Author:	Walter H F Smith
 * Date:	8 October, 1998
 * 
 **WHFS 27 Nov 1998 fixed bug so that -T0 gives correct results
 **  PW 18 Oct 1999 Use WORDS_BIGENDIAN macro set by configure.
 *   PW 12 Apr 2006 Replaced -x -y with -W -D
 *   PW 28 Nov 2006 Added -C for setting origin to main img origin (0,0)
 *
 */

#include "gmt_imgsubs.h"
#include "gmt_imgsrc.h"
#include "common_byteswap.h"

struct IMG2GRD_CTRL {
	struct In {	/* Input file name */
		BOOLEAN active;
		char *file;	/* Input file name */
	} In;
	struct C {	/* -C */
		BOOLEAN active;
	} C;
	struct D {	/* -D[<minlat>/<maxlat>] */
		BOOLEAN active;
		double min, max;
	} D;
	struct E {	/* -E */
		BOOLEAN active;
	} E;
	struct G {	/* -G<output_grdfile> */
		BOOLEAN active;
		char *file;
	} G;
	struct I {	/* -I<minutes> */
		BOOLEAN active;
		double value;
	} I;
	struct M {	/* -M */
		BOOLEAN active;
	} M;
	struct N {	/* -N<ave> */
		BOOLEAN active;
		int value;
	} N;
	struct S {	/* -S<scale> */
		BOOLEAN active;
		GMT_LONG mode;
		double value;
	} S;
	struct T {	/* -T<type> */
		BOOLEAN active;
		int value;
	} T;
	struct W {	/* -W<maxlon> */
		BOOLEAN active;
		double value;
	} W;
};

void *New_img2grd_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct IMG2GRD_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct IMG2GRD_CTRL);
	
	/* Initialize values whose defaults are not 0/FALSE/NULL */
	C->D.min = GMT_IMG_MINLAT;
	C->D.max = GMT_IMG_MAXLAT;
	C->N.value = 1;		/* No averaging */
	C->T.value = 1;		/* Default img type */
	C->I.value = GMT_IMG_MPIXEL;
	
	return (C);
}

void Free_img2grd_Ctrl (struct GMT_CTRL *GMT, struct IMG2GRD_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->In.file) free (C->In.file);	
	if (C->G.file) free (C->G.file);	
	GMT_free (GMT, C);	
}

GMT_LONG GMT_img2grd_usage (struct GMTAPI_CTRL *C, GMT_LONG level) {
	struct GMT_CTRL *GMT = C->GMT;

	GMT_message (GMT, "img2grd %s [API] - Extract a subset from an img file in Mercator or Geographic format\n\n", GMT_VERSION);
	GMT_message (GMT, "usage: img2grd <world_image_filename> %s -G<outgrid> -T<type> [-C]\n", GMT_Rgeo_OPT);
	GMT_message (GMT, "\t[-D[<minlat>/<maxlat>]] [-E] [-I<min>] [-M] [-N<navg>] [-S[<scale>]] [-V] [-W<maxlon>] [%s]\n\n", GMT_n_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);
	
	GMT_message (GMT, "\t<world_image_filename> gives name of img file.\n");
	GMT_message (GMT, "\t-G Set filename for the output grid file.\n");
	GMT_message (GMT, "\t-R Specify the region in decimal degrees or degrees:minutes.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t-C Refer Mercator coordinates to img source origin and requires -M\n");
	GMT_message (GMT, "\t   [Default sets lower left to 0,0].\n");
	GMT_message (GMT, "\t-D Set input img file bottom and top latitudes [%.3f/%.3f].\n", GMT_IMG_MINLAT, GMT_IMG_MAXLAT);
	GMT_message (GMT, "\t   If no latitudes are given it is taken to mean %.3f/%.3f.\n", GMT_IMG_MINLAT_80, GMT_IMG_MAXLAT_80);
	GMT_message (GMT, "\t   Without -D we automatically determine the extent from the file size.\n");
	GMT_message (GMT, "\t-E Resample geographic grid to the specified -R.  Cannot be used with -M .\n");
	GMT_message (GMT, "\t   (Default gives the exact -R of the Mercator grid).\n");
	GMT_message (GMT, "\t-I Set input img pixels to be <min> minutes of longitude wide [2.0].\n");
	GMT_message (GMT, "\t   Without -I we automatically determine the pixel size from the file size.\n");
	GMT_message (GMT, "\t-M Write a Mercator grid [Default writes a geographic grid].\n");
	GMT_message (GMT, "\t-N Output averages of input in navg by navg squares [no averaging].\n");
	GMT_message (GMT, "\t-S Multiply img integer values by <scale> before output [1].\n");
	GMT_message (GMT, "\t   To set scale based on information encoded in filename, just give -S.\n");
	GMT_message (GMT, "\t-T Select the img type format:\n");
	GMT_message (GMT, "\t   -T0 for obsolete img files w/ no constraint code, gets data.\n");
	GMT_message (GMT, "\t   -T1 for new img file w/ constraints coded, gets data at all points [Default].\n");
	GMT_message (GMT, "\t   -T2 for new img file w/ constraints coded, gets data only at constrained points, NaN elsewhere.\n");
	GMT_message (GMT, "\t   -T3 for new img file w/ constraints coded, gets 1 at constraints, 0 elsewhere.\n");
	GMT_explain_options (GMT, "V");
	GMT_message (GMT, "\t-W Input img file runs from 0 to <maxlon> longitude [360.0].\n");
	GMT_explain_options (GMT, "n.");
	
	return (EXIT_FAILURE);
}

GMT_LONG GMT_img2grd_parse (struct GMTAPI_CTRL *C, struct IMG2GRD_CTRL *Ctrl, struct GMT_OPTION *options) {

	/* This parses the options provided to grdcut and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	GMT_LONG n_errors = 0, n_files = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {
			/* Common parameters */

			case '<':	/* Input files */
				if (n_files == 0) Ctrl->In.file = strdup (opt->arg);
				n_files++;
				break;

			/* Processes program-specific parameters */

			case 'C':
				Ctrl->C.active = TRUE;
				break;
			case 'D':
				Ctrl->D.active = TRUE;
				if (opt->arg[0] && (sscanf (opt->arg, "%lf/%lf", &Ctrl->D.min, &Ctrl->D.max)) != 2) {
					n_errors++;
					GMT_report (GMT, GMT_MSG_FATAL, "Syntax error -D: Failed to decode <minlat>/<maxlat>.\n");
				}
				else {
					Ctrl->D.min = GMT_IMG_MINLAT_80;
					Ctrl->D.max = GMT_IMG_MAXLAT_80;
				}
				break;
			case 'E':
				Ctrl->E.active = TRUE;
				break;
			case 'G':
				Ctrl->G.active = TRUE;
				Ctrl->G.file = strdup (opt->arg);
				break;
#ifdef GMT_COMPAT
			case 'm':
				GMT_report (GMT, GMT_MSG_COMPAT, "Warning: -m<inc> is deprecated; use -I<inc> instead.\n");
#endif
			case 'I':
				Ctrl->I.active = TRUE;
				if ((sscanf (opt->arg, "%lf", &Ctrl->I.value)) != 1) {
					n_errors++;
					GMT_report (GMT, GMT_MSG_FATAL, "Syntax error: -I requires a positive value.\n");
				}
				break;
			case 'M':
				Ctrl->M.active = TRUE;
				break;
			case 'N':
				Ctrl->N.active = TRUE;
				if ((sscanf (opt->arg, "%d", &Ctrl->N.value)) != 1 || Ctrl->N.value < 1) {
					n_errors++;
					GMT_report (GMT, GMT_MSG_FATAL, "Syntax error: -N requires an integer > 1.\n");
				}
				break;
			case 'S':
				Ctrl->S.active = TRUE;
				if (sscanf (opt->arg, "%lf", &Ctrl->S.value) != 1) Ctrl->S.mode = 1;
				break;				
			case 'T':
				Ctrl->T.active = TRUE;
				if ((sscanf (opt->arg, "%d", &Ctrl->T.value)) != 1) {
					n_errors++;
					GMT_report (GMT, GMT_MSG_FATAL, "Syntax error: -T requires an output type 0-3.\n");
				}
				break;
			case 'W':
				Ctrl->W.active = TRUE;
				if ((sscanf (opt->arg, "%lf", &Ctrl->W.value)) != 1) {
					n_errors++;
					GMT_report (GMT, GMT_MSG_FATAL, "Syntax error: -W requires a longitude >= 360.0.\n");
				}
				break;
			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, Ctrl->In.file == NULL, "Syntax error: Must specify input imgfile name.\n");
	n_errors += GMT_check_condition (GMT, n_files > 1, "Syntax error: More than one world image file name given.\n");
	n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error: Must specify -R option.\n");
	n_errors += GMT_check_condition (GMT, GMT->common.R.active && (GMT->common.R.wesn[XLO] >= GMT->common.R.wesn[XHI] || GMT->common.R.wesn[YLO] >= GMT->common.R.wesn[YHI]), "Syntax error:Must specify -R with west < east and south < north.\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->G.active || Ctrl->G.file == NULL, "Syntax error: Must specify output grid file name with -G.\n");
	n_errors += GMT_check_condition (GMT, Ctrl->D.active && (Ctrl->D.min <= -90 || Ctrl->D.max >= 90.0 || Ctrl->D.max <= Ctrl->D.min), "Syntax error: Min/max latitudes are invalid.\n");
	n_errors += GMT_check_condition (GMT, Ctrl->T.value < 0 || Ctrl->T.value > 3, "Syntax error: Must specify output type in the range 0-3 with -T.\n");
	n_errors += GMT_check_condition (GMT, Ctrl->W.active && Ctrl->W.value < 360.0, "Syntax error: Requires a maximum longitude >= 360.0 with -W.\n");
	n_errors += GMT_check_condition (GMT, Ctrl->I.active && Ctrl->I.value <= 0.0, "Syntax error: Requires a positive value with -I.\n");
	n_errors += GMT_check_condition (GMT, Ctrl->C.active && !Ctrl->M.active, "Syntax error: -C requires -M.\n");
	n_errors += GMT_check_condition (GMT, Ctrl->E.active && Ctrl->M.active, "Syntax error: -E cannot be used with -M.\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_img2grd_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

GMT_LONG GMT_img2grd (struct GMTAPI_CTRL *API, GMT_LONG mode, void *args)
{
	BOOLEAN error = FALSE;
	COUNTER_MEDIUM navgsq, navg;	/* navg by navg pixels are averaged if navg > 1; else if navg == 1 do nothing */
	COUNTER_MEDIUM iout, jout, iinstart, iinstop, jinstart, jinstop, k, kk, ion, jin, jj, iin, ii, kstart, *ix = NULL;
	GMT_LONG in_ID, out_ID = GMTAPI_NOTSET;

	COUNTER_LARGE ij;
	int16_t tempint;
	
	size_t n_expected;	/* Expected items per row */

	double west, east, south, north, wesn[4], toplat, botlat, dx;
	double south2, north2, rnavgsq, csum, dsum, left, bottom, inc[2];

	int16_t *row = NULL;
	uint16_t *u2;

	char infile[GMT_BUFSIZ], cmd[GMT_BUFSIZ], s_in_ID[GMTAPI_STRLEN], s_out_ID[GMT_TEXT_LEN256];
	char z_units[GRD_UNIT_LEN80];

	FILE *fp = NULL;

	struct GMT_IMG_COORD imgcoord;
	struct GMT_IMG_RANGE imgrange = { GMT_IMG_MAXLON, GMT_IMG_MINLAT, GMT_IMG_MAXLAT, GMT_IMG_MPIXEL };
	struct GMT_GRID *Merc = NULL;
	struct IMG2GRD_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL)
		return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);
	if (API->error)
		return (API->error); /* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE)
		bailout (GMT_img2grd_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS)
		bailout (GMT_img2grd_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_module (API, "GMT_img2grd", &GMT_cpy);	/* Save current state */
	if (GMT_Parse_Common (API, "-VRf", GMT_OPT("m"), options))
		Return (API->error);
	Ctrl = New_img2grd_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_img2grd_parse (API, Ctrl, options)))
		Return (error);

	/*---------------------------- This is the img2grd main code ----------------------------*/

	/* Set up default settings if not specified */

	if (Ctrl->W.active) imgrange.maxlon = Ctrl->W.value;
	if (Ctrl->I.active) imgrange.mpixel = Ctrl->I.value;
	if (Ctrl->D.active) {
		imgrange.minlat = Ctrl->D.min;
		imgrange.maxlat = Ctrl->D.max;
	}

	if (!GMT_getdatapath (GMT, Ctrl->In.file, infile)) {
		GMT_report (GMT, GMT_MSG_FATAL, "img file %s not found\n", Ctrl->In.file);
		Return (GMT_GRDIO_FILE_NOT_FOUND);
	}

	if (! (Ctrl->I.active || Ctrl->D.active)) {
		GMT_LONG min;
		double lat = 0.0;
		struct GMT_STAT buf;
		if (GMT_STAT (infile, &buf)) return (GMT_GRDIO_STAT_FAILED);	/* Inquiry about file failed somehow */

		switch (buf.st_size) {	/* Known sizes are 1 or 2 min at lat_max = ~72 or 80 */
			case GMT_IMG_NLON_1M*GMT_IMG_NLAT_1M_80*GMT_IMG_ITEMSIZE:
				if (lat == 0.0) lat = GMT_IMG_MAXLAT_80;
			case GMT_IMG_NLON_1M*GMT_IMG_NLAT_1M_72*GMT_IMG_ITEMSIZE:
				if (lat == 0.0) lat = GMT_IMG_MAXLAT_72;
				min = 1;
				break;
			case GMT_IMG_NLON_2M*GMT_IMG_NLAT_2M_80*GMT_IMG_ITEMSIZE:
				if (lat == 0.0) lat = GMT_IMG_MAXLAT_80;
			case GMT_IMG_NLON_2M*GMT_IMG_NLAT_2M_72*GMT_IMG_ITEMSIZE:
				if (lat == 0.0) lat = GMT_IMG_MAXLAT_72;
				min = 2;
				break;
			default:
				if (lat == 0.0) return (GMT_GRDIO_BAD_IMG_LAT);
				min = (buf.st_size > GMT_IMG_NLON_2M*GMT_IMG_NLAT_2M_80*GMT_IMG_ITEMSIZE) ? 1 : 2;
				if (!Ctrl->I.active) GMT_report (GMT, GMT_MSG_FATAL, "img file %s has unusual size - grid increment defaults to %ld min\n", infile, min);
				break;
		}
		if (!Ctrl->D.active) {imgrange.minlat = -lat;	imgrange.maxlat = +lat;}
		if (!Ctrl->I.active) imgrange.mpixel = Ctrl->I.value = (double) min;
		GMT_report (GMT, GMT_MSG_VERBOSE, "img file %s determined to have an increment of %ld min and a latitude bound at +/- %g\n", infile, min, lat);
	}
	
	strcpy (z_units, "meters, mGal, Eotvos, micro-radians or Myr, depending on img file and -S.");
	if (Ctrl->S.mode) {	/* Guess the scaling */
		if (strstr (infile, "age")) {
			Ctrl->S.value = 0.01;
			strcpy (z_units, "Myr");
			GMT_report (GMT, GMT_MSG_NORMAL, "img file %s determined to be crustal ages with scale 0.01.\n", infile);
		}
		if (strstr (infile, "topo")) {
			Ctrl->S.value = 1.0;
			strcpy (z_units, "meter");
			GMT_report (GMT, GMT_MSG_NORMAL, "img file %s determined to be bathymetry with scale 1.\n", infile);
		}
		else if (strstr (infile, "grav")) {
			Ctrl->S.value = 0.1;
			strcpy (z_units, "mGal");
			GMT_report (GMT, GMT_MSG_NORMAL, "img file %s determined to be free-air anomalies with scale 0.1.\n", infile);
		}
		else if (strstr (infile, "curv") || strstr (infile, "vgg")) {
			Ctrl->S.value = (Ctrl->I.value == 2.0) ? 0.05 : 0.02;
			strcpy (z_units, "Eotvos");
			GMT_report (GMT, GMT_MSG_NORMAL, "img file %s determined to be VGG anomalies with scale %g.\n", infile, Ctrl->S.value);
		}
		else {
			GMT_report (GMT, GMT_MSG_NORMAL, "Unable to determine data type for img file %s; scale not used.\n", infile);
			Ctrl->S.value = 1.0;
		}
	}
	if (Ctrl->T.value == 3) strcpy (z_units, "T/F, one or more constraints fell in this pixel.");
	
	if (GMT_img_setup_coord (GMT, &imgrange, &imgcoord) ) {
		GMT_report (GMT, GMT_MSG_FATAL, "Syntax error: Error in img coordinate specification [-I -W or -D].\n");
		Return (GMT_RUNTIME_ERROR);
	}
	else if (Ctrl->N.value && (imgcoord.nx360%Ctrl->N.value != 0 || imgcoord.nyrow%Ctrl->N.value != 0) ) {
		GMT_report (GMT, GMT_MSG_FATAL, "Syntax error: Bad choice of navg in -N.  Must divide %ld and %ld\n", imgcoord.nx360, imgcoord.nyrow);
		Return (GMT_RUNTIME_ERROR);
	}

	if ((fp = fopen (infile, "rb")) == NULL) {
		GMT_report (GMT, GMT_MSG_FATAL, "Syntax error: Cannot open %s for binary read.\n", infile);
		Return (GMT_RUNTIME_ERROR);
	}
	
	if ((Merc = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);
	GMT_grd_init (GMT, Merc->header, options, FALSE);

	GMT->current.io.col_type[GMT_IN][GMT_X] = GMT_IS_LON;	GMT->current.io.col_type[GMT_IN][GMT_Y] = GMT_IS_LAT;
	if (Ctrl->M.active) {
		GMT->current.io.col_type[GMT_OUT][GMT_X] = GMT->current.io.col_type[GMT_OUT][GMT_Y] = GMT_IS_FLOAT;		/* Since output is no longer lon/lat */
	}
	else {
		GMT->current.io.col_type[GMT_OUT][GMT_X] = GMT_IS_LON; GMT->current.io.col_type[GMT_OUT][GMT_Y] = GMT_IS_LAT;	/* Output lon/lat */
	}
	
	/* Keep original -R for later */
	
	west  = wesn[XLO] = GMT->common.R.wesn[XLO];
	east  = wesn[XHI] = GMT->common.R.wesn[XHI];
	south = wesn[YLO] = GMT->common.R.wesn[YLO];
	north = wesn[YHI] = GMT->common.R.wesn[YHI];
	
	navg = Ctrl->N.value;
	
	/* Expected edges of input image based on coordinate initialization (might not exactly match user spec) */
	
	toplat = GMT_img_ypix_to_lat (0.0, &imgcoord);
	botlat = GMT_img_ypix_to_lat ((double)imgcoord.nyrow, &imgcoord);
	dx = 1.0 / ((double)imgcoord.nx360 / 360.0);
	inc[GMT_X] = inc[GMT_Y] = dx * navg;
	
	GMT_report (GMT, GMT_MSG_NORMAL, "Expects %s to be %ld by %ld pixels spanning 0/%5.1f/%.8g/%.8g.\n", infile, imgcoord.nxcol, imgcoord.nyrow, dx*imgcoord.nxcol, botlat, toplat);

	if (toplat < Merc->header->wesn[YHI]) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Warning: Your top latitude (%.12g) lies outside top latitude of input (%.12g) - now truncated.\n", Merc->header->wesn[YHI], toplat);
		wesn[YHI] = toplat - GMT_CONV_LIMIT;	/* To ensure proper round-off in calculating ny */
	}
	if (botlat > Merc->header->wesn[YLO]) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Warning: Your bottom latitude (%.12g) lies outside bottom latitude of input (%.12g) - now truncated.\n", Merc->header->wesn[YLO], botlat);
		wesn[YLO] = botlat + GMT_CONV_LIMIT;	/* To ensure proper round-off in calculating ny */
	}
	
	/* Re-adjust user's -R so that it falls on pixel coordinate boundaries */
	
	jinstart = navg * (int)floor (GMT_img_lat_to_ypix (wesn[YHI], &imgcoord) / navg);
	jinstop  = navg * (int)ceil  (GMT_img_lat_to_ypix (wesn[YLO], &imgcoord) / navg);
	/* jinstart <= jinputrow < jinstop  */
	Merc->header->ny = (int)((jinstop - jinstart) / navg);
	north2 = wesn[YHI] = GMT_img_ypix_to_lat ((double)jinstart, &imgcoord);
	south2 = wesn[YLO] = GMT_img_ypix_to_lat ((double)jinstop,  &imgcoord);

	iinstart = navg * lrint (floor (wesn[XLO]/inc[GMT_X]));
	iinstop  = navg * lrint (ceil  (wesn[XHI]/inc[GMT_X]));
	/* iinstart <= ipixelcol < iinstop, but modulo all with imgcoord.nx360  */
	/* Reset left and right edges of user area */
	wesn[XLO] = iinstart * dx;
	wesn[XHI] = iinstop  * dx;
	Merc->header->nx = (int)((iinstop - iinstart) / navg);

	GMT_report (GMT, GMT_MSG_NORMAL, "To fit [averaged] input, your %s is adjusted to -R%.12g/%.12g/%.12g/%.12g.\n", Ctrl->In.file, wesn[XLO], wesn[XHI], wesn[YLO], wesn[YHI]);
	GMT_report (GMT, GMT_MSG_NORMAL, "The output grid size will be %d by %d pixels.\n", Merc->header->nx, Merc->header->ny);

	/* Set iinstart so that it is non-negative, for use to index pixels.  */
	while (iinstart < 0) iinstart += imgcoord.nx360;
	
	/* Set navgsq, rnavgsq, for the averaging */
	navgsq = navg * navg;
	rnavgsq = 1.0 / navgsq;

	/* Set up header with Mercatorized dimensions assuming -Jm1  */
	if (Ctrl->C.active) {
		GMT_LONG equator;
		equator = lrint (GMT_img_lat_to_ypix (0.0, &imgcoord));
		wesn[XLO] = iinstart * inc[GMT_X];
		wesn[XHI] = wesn[XLO] + Merc->header->nx * inc[GMT_X];
		wesn[YHI] = (imgcoord.nyrow - jinstart - equator) * inc[GMT_Y];
		wesn[YLO] = wesn[YHI] - Merc->header->ny * inc[GMT_Y];
		left = bottom = 0.0;
		if (wesn[XHI] > 360.0) {
			wesn[XHI] -= 360.0;
			wesn[XLO] -= 360.0;
		}
	}
	else {
		wesn[XLO] = 0.0;
		wesn[XHI] = Merc->header->nx * inc[GMT_X];
		wesn[YLO] = 0.0;
		wesn[YHI] = Merc->header->ny * inc[GMT_Y];
		left = wesn[XLO];
		bottom = wesn[YLO];
	}
	if (Ctrl->M.active) {
		sprintf (Merc->header->x_units, "Spherical Mercator projected Longitude, -Jm1, length from %.12g", left);
		sprintf (Merc->header->y_units, "Spherical Mercator projected Latitude, -Jm1, length from %.12g", bottom);
		sprintf (Merc->header->remark, "Spherical Mercator Projected with -Jm1 -R%.12g/%.12g/%.12g/%.12g", wesn[XLO], wesn[XHI], south2, north2);
	}
	else {
		sprintf (Merc->header->x_units, "longitude [degrees_east]");
		sprintf (Merc->header->y_units, "latitude [degrees_north]");
	}
	strcpy (Merc->header->z_units, z_units);
	strcpy (Merc->header->title, "Data from Altimetry");
	Merc->header->z_min = DBL_MAX;	Merc->header->z_max = -DBL_MAX;

	/* Now malloc some space for float grd array, integer pixel index, and int16_t data buffer.  */

	GMT_err_fail (GMT, GMT_init_newgrid (GMT, Merc, wesn, inc, TRUE), Ctrl->G.file);
	Merc->data = GMT_memory (GMT, NULL, Merc->header->size, float);
	row = GMT_memory (GMT, NULL, navg * imgcoord.nxcol, int16_t);
	ix = GMT_memory (GMT, NULL, navgsq * Merc->header->nx, COUNTER_MEDIUM);

	/* Load ix with the index to the correct column, for each output desired.  This helps for Greenwich, 
	   also faster averaging of the file, etc.  Note for averaging each n by n block is looped in turn. */
	
	if (navg > 1) {
		for (iout = k = 0; iout < Merc->header->nx; iout++) {
			ion = iout * navg;
			for (jin = 0; jin < navg; jin++) {
				jj = jin * imgcoord.nxcol;
				for (iin = 0; iin < navg; iin++) {
					ii = (iin + iinstart + ion) % imgcoord.nx360;
					ix[k] = ii + jj;
					k++;
				}
			}
		}
	}
	else {
		for (iout = 0; iout < Merc->header->nx; iout++) ix[iout] = (iout + iinstart) % imgcoord.nx360;
	}


	/* Now before beginning data loop, fseek if needed.  */
	if (jinstart > 0 && jinstart < imgcoord.nyrow) fseek (fp, (off_t)(2 * imgcoord.nxcol * jinstart), SEEK_SET);
	
	/* Now loop over output points, reading and handling data as needed */

	n_expected = navg * imgcoord.nxcol;
	for (jout = 0; jout < Merc->header->ny; jout++) {
		jin = jinstart + navg * jout;
		ij = GMT_IJP (Merc->header, jout, 0);	/* Left-most index of this row */
		if (jin < 0 || jin >= imgcoord.nyrow) {	/* Outside latitude range; set row to NaNs */
			for (iout = 0; iout < Merc->header->nx; iout++, ij++) Merc->data[ij] = GMT->session.f_NaN;
			continue;
		}
		if ((fread (row, sizeof (int16_t), n_expected, fp) ) != n_expected) {
			GMT_report (GMT, GMT_MSG_FATAL, "Error: Read failure at jin = %ld.\n", jin);
			exit (EXIT_FAILURE);
		}

#ifndef WORDS_BIGENDIAN
		u2 = (uint16_t *)row;
		for (iout = 0; iout < navg * imgcoord.nxcol; iout++)
			u2[iout] = bswap16 (u2[iout]);
#endif

		for (iout = 0, kstart = 0; iout < Merc->header->nx; iout++, ij++, kstart += navgsq) {
			if (navg) {
				csum = dsum = 0.0;
				for (k = 0, kk = kstart; k < navgsq; k++, kk++) {
					tempint = row[ix[kk]];
					if (Ctrl->T.value && abs (tempint) % 2 != 0) {
						csum += 1.0;
						tempint--;
					}
					dsum += (double) tempint;
				}
				csum *= rnavgsq;
				dsum *= rnavgsq;
			}
			else {
				tempint = row[ix[iout]];
				if (Ctrl->T.value && abs (tempint) %2 != 0) {
					csum = 1.0;
					tempint--;
				}
				else
					csum = 0.0;
				dsum = (double) tempint;
			}
			
			if (Ctrl->S.active) dsum *= Ctrl->S.value;
			
			switch (Ctrl->T.value) {
				case 0:
				case 1:
					Merc->data[ij] = (float) dsum;
					break;
				case 2:
					Merc->data[ij] = (float)((csum >= 0.5) ? dsum : GMT->session.f_NaN);
					break;
				case 3:
					Merc->data[ij] = (float)csum;
					break;
			}
			

			if (Ctrl->T.value != 2 || csum >= 0.5) {
				if (Merc->header->z_min > Merc->data[ij]) Merc->header->z_min = Merc->data[ij];
				if (Merc->header->z_max < Merc->data[ij]) Merc->header->z_max = Merc->data[ij];
			}
		}
	}
	fclose (fp);

	GMT_free (GMT, ix);
	GMT_free (GMT, row);

	/* We now have the Mercator grid in Grid. */
	
	GMT_report (GMT, GMT_MSG_NORMAL, "Created %d by %d Mercatorized grid file.  Min, Max values are %.8g  %.8g\n", Merc->header->nx, Merc->header->ny, Merc->header->z_min, Merc->header->z_max);
	if (Ctrl->M.active) {	/* Write out the Mercator grid and return, no projection needed */
		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, NULL, GMT_GRID_ALL, Ctrl->G.file, Merc) != GMT_OK) {
			Return (API->error);
		}
		Return (GMT_OK);
	}

	/* Here we need to reproject the data, and Merc becomes a temporary grid */

	GMT_report (GMT, GMT_MSG_NORMAL, "Undo the implicit spherical Mercator -Jm1i projection.\n");
	/* Preparing source and destination for GMT_grdproject */
	/* a. Register the Mercator grid to be the source read by GMT_grdproject by passing a pointer */
	if ((in_ID = GMT_Register_IO (API, GMT_IS_GRID, GMT_IS_REF, GMT_IS_SURFACE, GMT_IN, Merc, NULL)) == GMTAPI_NOTSET) {
		Return (API->error);
	}
	if (GMT_Encode_ID (API, s_in_ID, in_ID) != GMT_OK) {
		Return (API->error);	/* Make filename with embedded object ID */
	}
	/* b. If -E: Register a grid struct Geo to be the destination allocated and written to by GMT_grdproject, else write to -G<file> */
	if (Ctrl->E.active) {	/* Since we will resample again, register a memory location for the result */
		if ((out_ID = GMT_Register_IO (API, GMT_IS_GRID, GMT_IS_REF, GMT_IS_SURFACE, GMT_OUT, NULL, NULL)) == GMTAPI_NOTSET) {
			Return (API->error);
		}
		if (GMT_Encode_ID (API, s_out_ID, out_ID) != GMT_OK) {
			Return (API->error);	/* Make filename with embedded object ID */
		}
	}
	else	/* The output here is the final result */
		strcpy (s_out_ID, Ctrl->G.file);
	sprintf (cmd, "-R%g/%g/%g/%g -Jm1 -I %s -G%s --PROJ_ELLIPSOID=Sphere --PROJ_LENGTH_UNIT=inch", west, east, south2, north2, s_in_ID, s_out_ID);
	if (GMT_grdproject (API, 0, cmd)!= GMT_OK) {	/* Inverse project the grid or fail */
		Return (API->error);
	}
	if (GMT_Destroy_Data (API, GMT_CLOBBER, &Merc) != GMT_OK) {
		Return (API->error);	/* Clobber since we know we allocated this grid */
	}
	if (Ctrl->E.active) {	/* Resample again using the given -R and the dx/dy in even minutes */
		/* Preparing source and destination for GMT_grdsample */
		/* a. Register the Geographic grid returned by GMT_grdproject to be the source read by GMT_grdsample by passing a pointer */
		struct GMT_GRID *Geo = NULL;
		if ((Geo = GMT_Retrieve_Data (API, out_ID)) == NULL) {
			Return (API->error);
		}
		strcpy (Geo->header->title, "Data from Altimetry");
		strcpy (Geo->header->z_units, z_units);
		sprintf (Geo->header->x_units, "longitude [degrees_east]");
		sprintf (Geo->header->y_units, "latitude [degrees_north]");
		if ((in_ID = GMT_Register_IO (API, GMT_IS_GRID, GMT_IS_REF, GMT_IS_SURFACE, GMT_IN, Geo, NULL)) == GMTAPI_NOTSET) {
			Return (API->error);
		}
		if (GMT_Encode_ID (API, s_in_ID, in_ID) != GMT_OK) {	/* Make filename with embedded object ID */
			Return (API->error);
		}
		sprintf (cmd, "-R%g/%g/%g/%g -I%gm %s -G%s -fg", west, east, south, north, Ctrl->I.value, s_in_ID, Ctrl->G.file);
		if (GMT_grdsample (API, 0, cmd) != GMT_OK) {	/* Resample the grid or fail */
			Return (API->error);
		}
		if (GMT_Destroy_Data (API, GMT_CLOBBER, &Geo) != GMT_OK) {	/* Clobber since we know we allocated this grid */
			Return (API->error);
		}
	}

	Return (GMT_OK);
}
