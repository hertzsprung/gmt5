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
 * Brief synopsis: grd2rgb reads either (1) an 8, 24, or 32 bit Sun rasterfile and writes out the
 * red, green, and blue components in three separate grid files, or (2) a z grd
 * file and a cpt file and compute r, g, b and write these out instead.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 */

#include "pslib.h"
#include "gmt.h"

struct GRD2RGB_CTRL {
	struct In {
		bool active;
		char *file;
	} In;
	struct C {	/* -C<cptfile> */
		bool active;
		char *file;
	} C;
	struct G {	/* -G<nametemplate> */
		bool active;
		char *name;
	} G;
	struct I {	/* -Idx[/dy] */
		bool active;
		double inc[2];
	} I;
	struct L {	/* -L<layer> */
		bool active;
		char layer;
	} L;
	struct W {	/* -W<width/height>[/<n_bytes>] */
		bool active;
		unsigned int nx, ny;	/* Dimension of image */
		unsigned int size;	/* Number of bytes per pixels */
	} W;
};

void *New_grd2rgb_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRD2RGB_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct GRD2RGB_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
	
	C->G.name = strdup ("grd2rgb_%c.nc");
	C->W.size = 3;	/* 3 bytes per pixel */
		
	return (C);
}

void Free_grd2rgb_Ctrl (struct GMT_CTRL *GMT, struct GRD2RGB_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->In.file) free (C->In.file);	
	if (C->C.file) free (C->C.file);	
	if (C->G.name) free (C->G.name);	
	GMT_free (GMT, C);	
}

int loadraw (struct GMT_CTRL *GMT, char *file, struct imageinfo *header, int byte_per_pixel, int nx, int ny, unsigned char **P) {
	/* loadraw reads a raw binary grb or rgba rasterfile of depth 24, or 32 into memory */

	unsigned int k;
	uint64_t j, i;
	size_t nm;
	unsigned char *buffer = NULL;

	FILE *fp = NULL;

	if ((fp = GMT_fopen (GMT, file, "rb")) == NULL) {
		GMT_report (GMT, GMT_MSG_FATAL, "Cannot open rasterfile %s!\n", file);
		return (EXIT_FAILURE);
	}

	/* Lets pretend that the raw file is a sunraster file. This way the grd2rgb code
	   can be used with very little changes */
	header->depth = 24;
	header->width = nx;
	header->height = ny;
	nm = (size_t)nx * (size_t)ny * (size_t)byte_per_pixel;
	header->length = nm;

	buffer = GMT_memory (GMT, NULL, nm, unsigned char);
	if (GMT_fread (buffer, 1U, nm, fp) != nm) {
		if (byte_per_pixel == 3)
			GMT_report (GMT, GMT_MSG_FATAL, "Trouble reading raw 24-bit rasterfile!\n");
		if (byte_per_pixel == 4)
			GMT_report (GMT, GMT_MSG_FATAL, "Trouble reading raw 32-bit rasterfile!\n");
		return (EXIT_FAILURE);
	}

	if (byte_per_pixel == 4) {		/* RGBA */
		for (i = 3, j = 4; j < nm; i += 3, j += 4) {
			for (k = 0; k < 3; k++) buffer[i+k] = buffer[j+k];
		}
	}

	GMT_fclose (GMT, fp);
	*P = buffer;
	return (0);
}

int guess_width (struct GMT_CTRL *GMT, char *file, unsigned int byte_per_pixel, unsigned int *raw_nx, unsigned int *raw_ny) {
	int inc, even;
	unsigned int k = 0, j, i, l, n_pix;
	unsigned char *buffer = NULL;
	size_t img_size;
	float *work = NULL, *datac = NULL, *img_pow = NULL, pow_max = -FLT_MAX, pm;
	int rgb[3];
	FILE *fp = NULL;

	if ((fp = GMT_fopen (GMT, file, "rb")) == NULL) {
		GMT_report (GMT, GMT_MSG_FATAL, "Cannot open rasterfile %s!\n", file);
		return (EXIT_FAILURE);
	}

	fseek (fp, (off_t)0, SEEK_END);
	img_size = ftell (fp);
	fseek (fp, 0, SEEK_SET);

	n_pix = (unsigned int) (img_size / byte_per_pixel);

	buffer  = GMT_memory (GMT, NULL, img_size, unsigned char);
	datac   = GMT_memory (GMT, NULL, 2*n_pix, float);
	work    = GMT_memory (GMT, NULL, 2*n_pix, float);
	img_pow = GMT_memory (GMT, NULL, n_pix/2, float);
	GMT_memset (work, 2*n_pix, float);

	if (GMT_fread (buffer, 1U, img_size, fp) != img_size) {
		if (byte_per_pixel == 3)
			GMT_report (GMT, GMT_MSG_FATAL, "Trouble_ reading raw 24-bit rasterfile!\n");
		if (byte_per_pixel == 4)
			GMT_report (GMT, GMT_MSG_FATAL, "Trouble_ reading raw 32-bit rasterfile!\n");
		return (EXIT_FAILURE);
	}
	GMT_fclose (GMT, fp);

	inc = (byte_per_pixel == 3) ? 3: 4;
	for (j = 0; j < img_size; j += inc) {
		for (i = 0; i < 3; i++) rgb[i] = buffer[j+i];
		/* Convert rgb to gray using the GMT_YIQ transformation */
		datac[k] = (float) GMT_YIQ (rgb);
		k += 2;
	}

	GMT_fft_1d (GMT, datac, n_pix, GMT_FFT_FWD, GMT_FFT_COMPLEX);

	/* Now compute the image's power spectrum */
	for (k = 0, j = 0; k < n_pix; k += 2, j++) {
		img_pow[j] = (datac[k]*datac[k] + datac[k+1]*datac[k+1]) / n_pix; /* I*I-conj = power */
	}

	/* I'll assume that searching on one fifth of the spectrum is enough to find the
	   line frequency. */
	for (k = 5; k < n_pix/10; k++) {
		if (img_pow[k] > pow_max) {
			pow_max = img_pow[k];	 j = k+1;
		}
	}

	/* That's the way it should be but, I don't know why, the result is transposed. Instead
	   of the number of lines I get number of columns. This is very weird and smells BUG */
	/* *raw_nx = j;		*raw_ny = lrint((float)n_pix / raw_nx);*/

	/* So be it */
	*raw_ny = j;		*raw_nx = lrint((float)n_pix / (*raw_ny));

	if ((*raw_nx) * (*raw_ny) != n_pix) {
		/* Let's make another attempt to find the right nx * ny combination. The idea is that we
	   	failed by a little, so we'll look arround the approximate solution by adding 1 to nx and
	   	subtracting 1 to ny. Then we revert (subtract 1 to nx and add 1 to ny). Next apply the
	   	same test with an offset of 2, and so on until the offset is 10. */
		GMT_report (GMT, GMT_MSG_NORMAL, "Warning: first test based on FFT failed to guess image dimensions.\n\tI'll do now a second try\t");
		k = 1;		pm = 1;		l = 1;
		while (k < 41) {
			i = *raw_ny + lrint (copysign((double)l, (double)pm));
			pm *= -1.;
			j = (*raw_nx) + lrint (copysign((double)l, (double)pm));
			if (i*j == n_pix) {	/* Got a good candidate */
				*raw_ny = i;	*raw_nx = j;
				GMT_report (GMT, GMT_MSG_NORMAL, "... SUCCESS (W = %d, H = %d)\n", *raw_nx, *raw_ny);
				break;
			}
			even = (k%2 == 0) ? 1 : 0;
			if (even) l++;
			k++;
		}
	}
	else
		GMT_report (GMT, GMT_MSG_NORMAL, "File %s has %d Lines and %d Cols\n", file, *raw_ny, *raw_nx);

	/* If both attempts failed */
	if ((*raw_nx) * (*raw_ny) != n_pix) {
		GMT_report (GMT, GMT_MSG_FATAL, "FAILURE while guessing image dimensions (W = %d, H = %d)\n", *raw_nx, *raw_ny);
		return (EXIT_FAILURE);
	}

	GMT_free (GMT, buffer);
	GMT_free (GMT, datac);
	GMT_free (GMT, work);
	GMT_free (GMT, img_pow);
	
	return (0);
}

int GMT_grd2rgb_usage (struct GMTAPI_CTRL *C, int level) {
	struct GMT_CTRL *GMT = C->GMT;

	GMT_message (GMT, "grd2rgb %s [API] - Write r/g/b grid files from a grid file, a raw RGB file, or SUN rasterfile\n\n", GMT_VERSION);
	GMT_message (GMT, "usage: grd2rgb <infile> [-C<cpt>] [-G<template>] [%s]\n", GMT_Id_OPT);
	GMT_message (GMT, "\t[-L<layer>] [%s] [%s] [-W<width>/<height>[/<n_bytes>]]\n\t[%s]\n\n", GMT_Rgeo_OPT, GMT_V_OPT, GMT_r_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<infile> can be one of three different intput files:\n");
	GMT_message (GMT, "\t  (1) An 8, 24, or 32-bit Sun rasterfile.  Use -I, -R, and -F to change the\n");
	GMT_message (GMT, "\t      the default values of dx = dy = 1 and region 1/ncols/1/nrows.\n");
	GMT_message (GMT, "\t  (2) A regular z grid file.  Use -C to provide a cpt file with which\n");
	GMT_message (GMT, "\t      to convert z to r/g/b triplets. -R, -I, and -F are ignored.\n");
	GMT_message (GMT, "\t  (3) A RGB or RGBA raw rasterfile. Since raw rasterfiles have no header, you have to\n");
	GMT_message (GMT, "\t      give the image dimensions via -W.\n");
	GMT_message (GMT, "\t      However, you may take the chance of letting the program try to\n");
	GMT_message (GMT, "\t      guess the image dimensions (slow, via FFT spectrum).\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t-C Color palette file to convert z to rgb.  If given, we assume a z grid file is provided,\n");
	GMT_message (GMT, "\t   else we will try to read a Sun rasterfile.\n");
	GMT_message (GMT, "\t-G Give outputfile name template for the three red, green, blue grid files.\n");
	GMT_message (GMT, "\t   The template MUST contain the format code %%c which will be replaced with r, g, and b\n");
	GMT_message (GMT, "\t   [Default is grd2rgb_%%c.nc].\n");
	GMT_message (GMT, "\t-I Specify grid size(s).  Append m (or c) to <dx> and/or <dy> for minutes (or seconds).\n");
	GMT_message (GMT, "\t-L Only output the given layer (r, g, or b) [Default output all three].\n");
	GMT_explain_options (GMT, "RV");
	GMT_message (GMT, "\t-W Set the size of the raw raster file. By default an RGB file (which has 3 bytes/pixel)\n");
	GMT_message (GMT, "\t   is assumed. For RGBA files use n_bytes = 4.\n");
	GMT_message (GMT, "\t   Use -W for guessing the image size of a RGB raw file, and -W=/=/4.\n");
	GMT_message (GMT, "\t   if the raw image is of the RGBA type. Notice that this might be a\n");
	GMT_message (GMT, "\t   bit slow because the guessing algorithm makes uses of FFTs.\n");
	GMT_explain_options (GMT, "F");
	
	return (EXIT_FAILURE);
}

int GMT_grd2rgb_parse (struct GMTAPI_CTRL *C, struct GRD2RGB_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to grdcut and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_files = 0, pos, entry;
	bool guess = false;
	char ptr[GMT_BUFSIZ];
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;
	
	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {
			case '<':	/* Input file (only one is accepted) */
				Ctrl->In.active = true;
				if (n_files++ == 0) Ctrl->In.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'C':	/* Use cpt table */
				Ctrl->C.file = strdup (opt->arg);
				Ctrl->C.active = true;
				break;
			case 'G':	/* Output file template */
				Ctrl->G.name = strdup (opt->arg);
				break;
			case 'I':	/* Get grid spacings */
				if (GMT_getinc (GMT, opt->arg, Ctrl->I.inc)) {
					GMT_inc_syntax (GMT, 'I', 1);
					n_errors++;
				}
				Ctrl->I.active = true;
				break;
			case 'L':	/* Select one layer */
				Ctrl->L.layer = opt->arg[0];
				Ctrl->L.active = true;
				break;
			case 'W':	/* Give raw size */
				Ctrl->W.active = true;
				guess = true;
				entry = pos = 0;
				while ((GMT_strtok (opt->arg, "/", &pos, ptr))) {
					if (ptr[0] != '=') {
						switch (entry) {
							case 0:
								Ctrl->W.nx = atoi (ptr);
								guess = false; break;
							case 1:
								Ctrl->W.ny = atoi (ptr); break;
							case 2:
								Ctrl->W.size = atoi (ptr); break;
						}
					}
					entry++;
				}
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	/* Check that the options selected are mutually consistent */

	GMT_check_lattice (GMT, Ctrl->I.inc, &GMT->common.r.active, &Ctrl->I.active);

	if (!Ctrl->C.active) {
		n_errors += GMT_check_condition (GMT, !Ctrl->In.file, "Syntax error: Must specify input raster file\n");
		n_errors += GMT_check_condition (GMT, Ctrl->I.active && (Ctrl->I.inc[GMT_X] == 0.0 || Ctrl->I.inc[GMT_Y] == 0.0), "Syntax error: increments must be positive\n");
		n_errors += GMT_check_condition (GMT, Ctrl->W.size != 3 && Ctrl->W.size != 4, "Syntax error: byte_per_pixel must be either 3 or 4\n");
		if (guess) guess_width (GMT, Ctrl->In.file, Ctrl->W.size, &Ctrl->W.nx, &Ctrl->W.ny);

		n_errors += GMT_check_condition (GMT, Ctrl->W.active && Ctrl->W.nx <= 0, "Syntax error: Witdth of raw raster file must be a positive integer. Not %d\n", Ctrl->W.nx);
		n_errors += GMT_check_condition (GMT, Ctrl->W.active && Ctrl->W.ny <= 0, "Syntax error: Height of raw raster file must be a positive integer. Not %d\n", Ctrl->W.ny);
	}
	else {
		n_errors += GMT_check_condition (GMT, !Ctrl->In.file, "Syntax error: Must specify input z grid file\n");
		n_errors += GMT_check_condition (GMT, !Ctrl->C.file, "Syntax error: Must specify cpt file\n");
	}
	n_errors += GMT_check_condition (GMT, !strstr (Ctrl->G.name, "%c"), "Syntax error: output template must contain %%c\n");
	n_errors += GMT_check_condition (GMT, Ctrl->I.active && !strchr ("rgb", Ctrl->L.layer), "Syntax error: -L layer must be one of r, g, or b\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grd2rgb_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grd2rgb (struct GMTAPI_CTRL *API, int mode, void *args)
{
	unsigned int channel, row, col;
	int error = 0;
	
	uint64_t ij, k, k3;
	
	char rgb[3] = {'r', 'g', 'b'}, *comp[3] = {"red", "green", "blue"};
	char buffer[GMT_BUFSIZ], *grdfile = NULL;
	
	unsigned char *picture = NULL;
	
	double f_rgb[4];
	
	struct imageinfo header;
	struct GMT_GRID *Grid = NULL, *Out = NULL;
	struct GMT_PALETTE *P = NULL;
	struct GRD2RGB_CTRL *Ctrl = NULL;
	struct PSL_CTRL *PSL = NULL;	/* General PSL interal parameters */
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */
	
	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grd2rgb_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grd2rgb_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_module (API, "GMT_grd2rgb", &GMT_cpy);	/* Save current state */
	if (GMT_Parse_Common (API, "-VRr", GMT_OPT("F"), options)) Return (API->error);
	Ctrl = New_grd2rgb_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grd2rgb_parse (API, Ctrl, options))) Return (error);
	PSL = GMT->PSL;	/* This module also needs PSL */

	/*---------------------------- This is the gmt2kml main code ----------------------------*/

	/* No command line files or std** to add via GMT_Init_IO */
	
	if (Ctrl->C.active) {	/* Apply CPT to get three r,g,b channel files */
		bool new_grid = false;
		/* Since these GMT grids COULD be passed in via memory locations, they COULD have pads so we must use general IJ access */
		if ((P = GMT_Read_Data (API, GMT_IS_CPT, GMT_IS_FILE, GMT_IS_POINT, GMT_READ_NORMAL, NULL, Ctrl->C.file, NULL)) == NULL) {
			Return (API->error);
		}
		
		for (channel = 0; channel < 3; channel++) {	/* Do the r, g, and b channels */
			if (Ctrl->L.active && Ctrl->L.layer != rgb[channel]) continue;	/* Only do one of the layers */
			GMT_report (GMT, GMT_MSG_NORMAL, "Processing the %s components\n", comp[channel]);
			if ((Grid = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->In.file, NULL)) == NULL) {
				Return (API->error);
			}
			GMT_grd_init (GMT, Grid->header, options, false);

			new_grid = GMT_set_outgrid (GMT, Grid, &Out);	/* true if input is a read-only array; else Out == Grid */
				
			sprintf (buffer, Ctrl->G.name, rgb[channel]);
			grdfile = strdup (buffer);
			sprintf (Out->header->remark, "Grid of %s components in the 0-255 range", comp[channel]);
			GMT_grd_loop (GMT, Grid, row, col, ij) {
				(void)GMT_get_rgb_from_z (GMT, P, Grid->data[ij], f_rgb);
				Out->data[ij] = (float)GMT_s255 (f_rgb[channel]);
			}
			if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, grdfile, Out) != GMT_OK) {
				Return (API->error);
			}
			if (GMT_Destroy_Data (API, GMT_ALLOCATED, &Grid) != GMT_OK) {
				Return (API->error);
			}
			if (new_grid && GMT_Destroy_Data (API, GMT_ALLOCATED, &Out) != GMT_OK) {
				Return (API->error);
			}
			free (grdfile);
		}
		if (GMT_Destroy_Data (API, GMT_ALLOCATED, &P) != GMT_OK) {
			Return (API->error);
		}
	}
	else {
		if (GMT_access (GMT, Ctrl->In.file, R_OK)) {
			GMT_report (GMT, GMT_MSG_FATAL, "Cannot find/open/read file %s\n", Ctrl->In.file);
			Return (EXIT_FAILURE);
		}

		if (!Ctrl->W.active) {
			if (PSL_loadimage (PSL, Ctrl->In.file, &header, &picture)) {
				GMT_report (GMT, GMT_MSG_FATAL, "Trouble loading/converting Sun rasterfile!\n");
				Return (EXIT_FAILURE);
			}
		}
		else {
			if (loadraw (GMT, Ctrl->In.file, &header, Ctrl->W.size, Ctrl->W.nx, Ctrl->W.ny, &picture)) {
				GMT_report (GMT, GMT_MSG_FATAL, "Trouble loading/converting RGB image!\n");
				Return (EXIT_FAILURE);
			}
		}

		if (header.depth < 8) {
			GMT_report (GMT, GMT_MSG_FATAL, "Sun rasterfile must be at least 8 bits deep\n");
			Return (EXIT_FAILURE);
		}

		if ((Grid = GMT_Create_Data (API, GMT_IS_GRID, NULL)) == NULL) Return (API->error);
		GMT_grd_init (GMT, Grid->header, options, false);
		Grid->header->registration = (int)GMT->common.r.active;
		if (!Ctrl->I.active) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Assign default dx = dy = 1\n");
			Ctrl->I.inc[GMT_X] = Ctrl->I.inc[GMT_Y] = 1.0;	Ctrl->I.active = true;
		}
		if (!GMT->common.R.active) {	/* R not given, provide default */
			GMT->common.R.wesn[XLO] = GMT->common.R.wesn[YLO] = 0.0;
			GMT->common.R.wesn[XHI] = header.width  - 1 + Grid->header->registration;
			GMT->common.R.wesn[YHI] = header.height - 1 + Grid->header->registration;
			GMT->common.R.active = true;
			GMT_report (GMT, GMT_MSG_NORMAL, "Assign default -R0/%g/0/%g\n", GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI]);
		}

		Grid->header->nx = GMT_get_n (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], Ctrl->I.inc[GMT_X], Grid->header->registration);
		Grid->header->ny = GMT_get_n (GMT, GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI], Ctrl->I.inc[GMT_Y], Grid->header->registration);
		if (Ctrl->W.active && !Ctrl->I.active) {		/* This isn't correct because it doesn't deal with -r */
			Grid->header->nx = Ctrl->W.nx;
			Grid->header->ny = Ctrl->W.ny;
			Ctrl->I.inc[GMT_X] = GMT_get_inc (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], Grid->header->nx, Grid->header->registration);
			Ctrl->I.inc[GMT_Y] = GMT_get_inc (GMT, GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI], Grid->header->ny, Grid->header->registration);
		}
		if (header.width != (int)Grid->header->nx) {
			GMT_report (GMT, GMT_MSG_FATAL, "Sun rasterfile width and -R -I do not match (%d versus %d)  Need -r?\n", header.width, Grid->header->nx);
			Return (EXIT_FAILURE);
		}
		if (header.height != (int)Grid->header->ny) {
			GMT_report (GMT, GMT_MSG_FATAL, "Sun rasterfile height and -R -I do not match (%d versus %d)  Need -r?\n", header.height, Grid->header->ny);
			Return (EXIT_FAILURE);
		}
		/* Completely determine the header for the new grid; croak if there are issues.  No memory is allocated here. */
		GMT_err_fail (GMT, GMT_init_newgrid (GMT, Grid, GMT->common.R.wesn, Ctrl->I.inc, GMT->common.r.active), "stdout");
		
		GMT_report (GMT, GMT_MSG_NORMAL, "nx = %d  ny = %d\n", Grid->header->nx, Grid->header->ny);

		Grid->data = GMT_memory (GMT, NULL, Grid->header->size, float);

		/* Note: While the picture array has no pads, we must assume the output grid may have one */
		
		for (channel = 0; channel < 3; channel++) {	/* Do the r, g, and b channels */
			if (Ctrl->L.active && Ctrl->L.layer != rgb[channel]) continue;
			GMT_report (GMT, GMT_MSG_NORMAL, "Processing the %s components\n", comp[channel]);
			sprintf (buffer, Ctrl->G.name, rgb[channel]);
			grdfile = strdup (buffer);
			sprintf (Grid->header->remark, "Grid of %s components in the 0-255 range", comp[channel]);
			k3 = channel;
			k = 0;
			GMT_grd_loop (GMT, Grid, row, col, ij) {
				if (header.depth == 8)	/* Gray ramp */
					Grid->data[ij] = (float)picture[k++];
				else {				/* 24-bit image */
					Grid->data[ij] = (float)picture[k3];
					k3 += 3;
				}
			}
			if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, grdfile, Grid) != GMT_OK) {
				Return (API->error);
			}
			free (grdfile);
		}
		if (Ctrl->W.active)
			GMT_free (GMT, picture);
		else
			PSL_free (picture);
		if (GMT_Destroy_Data (API, GMT_ALLOCATED, &Grid) != GMT_OK) {
			Return (API->error);
		}
	}

	Return (GMT_OK);
}
