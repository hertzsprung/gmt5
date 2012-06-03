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
 * Brief synopsis: psimage reads a 1, 8, 24, or 32 bit Sun rasterfile and plots it on the page
 * Other raster formats are supported if ImageMagick's convert is found in the
 * system path.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#define THIS_MODULE k_mod_psimage /* I am psimage */

#include "pslib.h"
#include "gmt.h"

struct PSIMAGE_CTRL {
	struct In {
		bool active;
		char *file;
	} In;
	struct C {	/* -C<xpos>/<ypos>[/<justify>] */
		bool active;
		double x, y;
		char justify[3];
	} C;
	struct E {	/* -E<dpi> */
		bool active;
		double dpi;
	} E;
	struct F {	/* -F<pen> */
		bool active;
		struct GMT_PEN pen;
	} F;
	struct G {	/* -G[f|b|t]<rgb> */
		bool active;
		unsigned int mode;	/* 0 for f|b, 1 for t */
		double f_rgb[4];
		double b_rgb[4];
		double t_rgb[4];
	} G;
	struct I {	/* -I */
		bool active;
	} I;
	struct M {	/* -M */
		bool active;
	} M;
	struct N {	/* -N<nx>/<ny> */
		bool active;
		unsigned int nx, ny;
	} N;
	struct W {	/* -W[-]<width>[/<height>] */
		bool active;
		bool interpolate;
		double width, height;
	} W;
};

void *New_psimage_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct PSIMAGE_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct PSIMAGE_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
	C->F.pen = GMT->current.setting.map_default_pen;
	strcpy (C->C.justify, "LB");
	C->G.f_rgb[0] = C->G.b_rgb[0] = C->G.t_rgb[0] = -2;
	C->N.nx = C->N.ny = 1;	
	return (C);
}

void Free_psimage_Ctrl (struct GMT_CTRL *GMT, struct PSIMAGE_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->In.file) free (C->In.file);
	GMT_free (GMT, C);
}

int GMT_psimage_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	/* This displays the psimage synopsis and optionally full usage information */

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: psimage <imagefile> [-E<dpi> or -W[-]<width>[/<height>]] [-C<xpos>/<ypos>[/<justify>]]\n");
	GMT_message (GMT, "\t[-F<pen>] [-G[b|f|t]<color>] [-I] [%s] [%s] [-K] [-M] [-N<nx>[/<ny>]] [-O] [-P]\n", GMT_J_OPT, GMT_Jz_OPT);
	GMT_message (GMT, "\t[%s] [%s]\n", GMT_Rgeoz_OPT, GMT_U_OPT);
	GMT_message (GMT, "\t[%s] [%s] [%s] [%s]\n\t[%s] [%s]\n\n", GMT_V_OPT, GMT_X_OPT, GMT_Y_OPT, GMT_c_OPT, GMT_p_OPT, GMT_t_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t<imagefile> is an EPS or image file.\n");
	GMT_message (GMT, "\t-E Set image dpi (dots per inch), OR\n");
	GMT_message (GMT, "\t-W Set the width (and height) of the image.  If <height> = 0\n");
	GMT_message (GMT, "\t   then the original aspect ratio is maintained.  If <width> < 0\n");
	GMT_message (GMT, "\t   then we use absolute value and interpolate image in PostScript.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t-C Set the lower left position on the map for raster image [0/0].\n");
	GMT_message (GMT, "\t   Optionally, append justification (see pstext for codes).\n");
	GMT_pen_syntax (GMT, 'F', "Draw a frame around the image with the given pen.");
	GMT_message (GMT, "\t-Gb and -Gf (1-bit images only) sets the background and foreground color,\n");
	GMT_message (GMT, "\t   respectively. Set <color> = - for transparency [Default is black and white].\n");
	GMT_message (GMT, "\t-Gt (not for 1-bit images) indicate which color to be made transparent\n");
	GMT_message (GMT, "\t   [Default no transparency].\n");
	GMT_message (GMT, "\t-I Invert 1-bit images (does not affect 8 or 24-bit images).\n");
	GMT_explain_options (GMT, "jZK");
	GMT_message (GMT, "\t-M Force color -> monochrome image using GMT_YIQ-transformation.\n");
	GMT_message (GMT, "\t-N Replicate image <nx> by <ny> times [Default is no replication].\n");
	GMT_explain_options (GMT, "OPRUVXcp");
	GMT_message (GMT, "\t   (Requires -R and -J for proper functioning).\n");
	GMT_explain_options (GMT, "t.");
	
	return (EXIT_FAILURE);
}

int GMT_psimage_parse (struct GMTAPI_CTRL *C, struct PSIMAGE_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to psimage and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_files = 0;
	int n;
	char txt_a[GMT_TEXT_LEN256], txt_b[GMT_TEXT_LEN256], letter;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {

			case '<':	/* Input files */
				if (n_files++ == 0) Ctrl->In.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'C':	/* Image placement */
				Ctrl->C.active = true;
				n = sscanf (opt->arg, "%[^/]/%[^/]/%2s", txt_a, txt_b, Ctrl->C.justify);
				n_errors += GMT_check_condition (GMT, n < 2 || n > 3, "Error: Syntax is -C<xpos>/<ypos>[/<justify>]\n");
				Ctrl->C.x = GMT_to_inch (GMT, txt_a);
				Ctrl->C.y = GMT_to_inch (GMT, txt_b);
				if (n == 2) strcpy (Ctrl->C.justify, "LB");	/* Default positioning */
				break;
			case 'E':	/* Specify image dpi */
				Ctrl->E.active = true;
				Ctrl->E.dpi = atof (opt->arg);
				break;
			case 'F':	/* Specify frame pen */
				Ctrl->F.active = true;
				if (GMT_getpen (GMT, opt->arg, &Ctrl->F.pen)) {
					GMT_pen_syntax (GMT, 'F', " ");
					n_errors++;
				}
				break;
			case 'G':	/* Background/foreground color for 1-bit images */
				Ctrl->G.active = true;
				letter = (GMT_colorname2index (GMT, opt->arg) >= 0) ? 'x' : opt->arg[0];	/* If we have -G<colorname>, the x is used to bypass the case F|f|B|b switching below */
				switch (letter) {
					case 'f':
						/* Set color for foreground pixels */
						if (opt->arg[1] == '-' && opt->arg[2] == '\0')
							Ctrl->G.f_rgb[0] = -1;
						else if (GMT_getrgb (GMT, &opt->arg[1], Ctrl->G.f_rgb)) {
							GMT_rgb_syntax (GMT, 'G', " ");
							n_errors++;
						}
						break;
					case 'b':
						/* Set color for background pixels */
						if (opt->arg[1] == '-' && opt->arg[2] == '\0')
							Ctrl->G.b_rgb[0] = -1;
						else if (GMT_getrgb (GMT, &opt->arg[1], Ctrl->G.b_rgb)) {
							GMT_rgb_syntax (GMT, 'G', " ");
							n_errors++;
						}
						break;
					case 't':
						/* Set transparent color */
						if (GMT_getrgb (GMT, &opt->arg[1], Ctrl->G.t_rgb)) {
							GMT_rgb_syntax (GMT, 'G', " ");
							n_errors++;
						}
						Ctrl->G.mode = 1;
						break;
					default:	/* Gave either -G<r/g/b>, -G-, or -G<colorname>; all treated as -Gf */
						if (opt->arg[0] == '-' && opt->arg[1] == '\0')
							Ctrl->G.f_rgb[0] = -1;
						else if (GMT_getrgb (GMT, opt->arg, Ctrl->G.f_rgb)) {
							GMT_rgb_syntax (GMT, 'G', " ");
							n_errors++;
						}
						break;
				}
				break;
			case 'I':	/* Invert 1-bit images */
				Ctrl->I.active = true;
				break;
			case 'M':	/* Monochrome image */
				Ctrl->M.active = true;
				break;
			case 'N':	/* Replicate image */
				Ctrl->N.active = true;
				n = sscanf (opt->arg, "%d/%d", &Ctrl->N.nx, &Ctrl->N.ny);
				if (n == 1) Ctrl->N.ny = Ctrl->N.nx;
				n_errors += GMT_check_condition (GMT, n < 1, "Syntax error -N option: Must values for replication\n");
				break;
			case 'W':	/* Image width */
				Ctrl->W.active = true;
				n = sscanf (opt->arg, "%[^/]/%s", txt_a, txt_b);
				Ctrl->W.width = GMT_to_inch (GMT, txt_a);
				if (n == 2) Ctrl->W.height = GMT_to_inch (GMT, txt_b);
				if (Ctrl->W.width < 0.0) {
					Ctrl->W.width = -Ctrl->W.width;
					Ctrl->W.interpolate = true;
				}
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	/* If not done previously, set foreground to black, background to white */

	if (Ctrl->G.f_rgb[0] == -2) { Ctrl->G.f_rgb[0] = Ctrl->G.f_rgb[1] = Ctrl->G.f_rgb[2] = 0.0; }
	if (Ctrl->G.b_rgb[0] == -2) { Ctrl->G.b_rgb[0] = Ctrl->G.b_rgb[1] = Ctrl->G.b_rgb[2] = 1.0; }

	/* Check that the options selected are mutually consistent */

	n_errors += GMT_check_condition (GMT, n_files != 1, "Syntax error: Must specify a single input raster or EPS file\n");
	n_errors += GMT_check_condition (GMT, Ctrl->W.width <= 0.0 && Ctrl->E.dpi <= 0.0, "Must specify image width (-W) or dpi (-E)\n");
	n_errors += GMT_check_condition (GMT, Ctrl->N.active && (Ctrl->N.nx < 1 || Ctrl->N.ny < 1), 
			"Syntax error -N option: Must specify positive values for replication\n");
	n_errors += GMT_check_condition (GMT, Ctrl->G.f_rgb[0] < 0 && Ctrl->G.b_rgb[0] < 0, 
			"Syntax error -G option: Only one of fore/back-ground can be transparent for 1-bit images\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

int file_is_known (struct GMT_CTRL *GMT, char *file)
{	/* Returns 1 if it is an EPS file, 2 if a Sun rasterfile; 0 for any other file.
       Returns -1 on read error */
	FILE *fp = NULL;
	unsigned char c[4], magic_ras[4] = {0x59, 0xa6, 0x6a, 0x95}, magic_ps[4] = {'%', '!', 'P', 'S'};
	int j;

	if (GMT_File_Is_Memory (file)) return (0);	/* Special passing of image */
	j = (int)strlen(file) - 1;
	while (j && file[j] && file[j] != '+') j--;	/* See if we have a band request */
	if (j && file[j+1] == 'b') file[j] = '\0';			/* Temporarily strip the band request string so that the opening test doesn't fail */

	if ((fp = GMT_fopen (GMT, file, "rb")) == NULL) {
		GMT_report (GMT, GMT_MSG_FATAL, "Cannot open file %s\n", file);
		return (-1);
	}
	if (GMT_fread (c, 1U, 4U, fp) != 4U) {
		GMT_report (GMT, GMT_MSG_FATAL, "Could not read 4 bytes from file %s\n", file);
		return (-1);
	}
	GMT_fclose (GMT, fp);
	if (j) file[j] = '+';			/* Reset the band request string */
	if (GMT_same_rgb (c, magic_ps)) return(1);
	if (GMT_same_rgb (c, magic_ras)) return(2);
	return (0);	/* Neither */
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_psimage_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_psimage (struct GMTAPI_CTRL *API, int mode, void *args)
{
	int i, j, n, justify, PS_interpolate = 1, PS_transparent = 1, known = 0;
	unsigned int row, col;
	bool error = false, free_GMT = false;

	double x, y, wesn[4];

	unsigned char *picture = NULL, *buffer = NULL;
	
	char *format[2] = {"EPS", "Sun raster"};

	struct imageinfo header;

	struct PSIMAGE_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;		/* General GMT interal parameters */
	struct GMT_OPTION *options = NULL;
	struct PSL_CTRL *PSL = NULL;		/* General PSL interal parameters */
#ifdef USE_GDAL
	struct GMT_IMAGE *I = NULL;		/* A GMT image datatype, if GDAL is used */
#endif

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_psimage_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_psimage_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments; return if errors are encountered */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-VJR", "KOPUXxYycpt>", options)) Return (API->error);
	Ctrl = New_psimage_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_psimage_parse (API, Ctrl, options))) Return (error);
	PSL = GMT->PSL;		/* This module also needs PSL */

	/*---------------------------- This is the psimage main code ----------------------------*/

	PS_interpolate = (Ctrl->W.interpolate) ? -1 : +1;
	GMT_memset (&header, 1, struct imageinfo);

	known = file_is_known (GMT, Ctrl->In.file);	/* Determine if this is an EPS file, Sun rasterfile, or other */
	if (known < 0) {
		GMT_report (GMT, GMT_MSG_FATAL, "Cannot find/open/read file %s\n", Ctrl->In.file);
		Return (EXIT_FAILURE);
	}
	
	if (known) {	/* Read an EPS or Sun raster file */
		if (PSL_loadimage (PSL, Ctrl->In.file, &header, &picture)) {
			GMT_report (GMT, GMT_MSG_FATAL, "Trouble loading %s file %s!\n", format[known-1], Ctrl->In.file);
			Return (EXIT_FAILURE);
		}
	}
#ifdef USE_GDAL
	else  {	/* Read a raster image */
		GMT_set_pad (GMT, 0);	/* Temporary turn off padding (and thus BC setting) since we will use image exactly as is */
		if ((I = GMT_Read_Data (API, GMT_IS_IMAGE, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->In.file, NULL)) == NULL) {
			Return (API->error);
		}
		GMT_set_pad (GMT, 2);	/* Reset to GMT default */

		if (I->ColorMap != NULL) {
			unsigned char *r_table = NULL, *g_table = NULL, *b_table = NULL;
			r_table = GMT_memory (GMT, NULL, 256, unsigned char);
			g_table = GMT_memory (GMT, NULL, 256, unsigned char);
			b_table = GMT_memory (GMT, NULL, 256, unsigned char);
			for (n = 0; n < 256; n++) {
				r_table[n] = I->ColorMap[n*4    ];	/* 4 because color table is RGBA */
				g_table[n] = I->ColorMap[n*4 + 1];
				b_table[n] = I->ColorMap[n*4 + 2];
			}
			I->data = GMT_memory (GMT, I->data, 3 * I->header->nm, unsigned char);	/* Expand to reuse */
			n = 3 * I->header->nm - 1;
			for (j = I->header->nm - 1; j >= 0; j--) {
				I->data[n--] = b_table[I->data[j]];
				I->data[n--] = g_table[I->data[j]];
				I->data[n--] = r_table[I->data[j]];	/* Now we can overwrite this value */
			}
			I->header->n_bands = 3;
			GMT_free (GMT, r_table);	GMT_free (GMT, g_table);	GMT_free (GMT, b_table);
		}

		picture = (unsigned char *)I->data;
		header.width = I->header->nx;
		header.height = I->header->ny;
		header.depth = (int)(I->header->n_bands * 8);
	}
#else
	else {	/* Without GDAL we can only read EPS and Sun raster */
		GMT_report (GMT, GMT_MSG_FATAL, "Unsupported file format for file %s!\n", Ctrl->In.file);
		Return (EXIT_FAILURE);
	}
#endif
	
	if (Ctrl->M.active && header.depth == 24) {	/* Downgrade to grayshade image */
		n = 3 * header.width * header.height;
		buffer = psl_gray_encode (PSL, &n, picture);
		header.depth = 8;
		if (known) PSL_free (picture); /* EPS or Sun raster file */
#ifdef USE_GDAL
		else {	/* Got it via GMT_Read_Data */
			if (GMT_Destroy_Data (API, GMT_ALLOCATED, &I) != GMT_OK) {
				Return (API->error);
			}
		}
#endif
		picture = buffer;
	}

	/* Add transparent color at beginning, if requested */
	if (Ctrl->G.t_rgb[0] < 0)
		PS_transparent = 1;
	else if (header.depth >= 8) {
		PS_transparent = -1;
		j = header.depth / 8;
		n = j * (header.width * header.height + 1);
		buffer = GMT_memory (GMT, NULL, n, unsigned char);
		for (i = 0; i < j; i++) buffer[i] = (unsigned char)Ctrl->G.t_rgb[i];
		GMT_memcpy (&(buffer[j]), picture, n, unsigned char);
#ifdef USE_GDAL
		if (GMT_Destroy_Data (API, GMT_ALLOCATED, &I) != GMT_OK) {	/* If I is NULL then nothing is done */
			Return (API->error);
		}
#else
		PSL_free (picture);
#endif
		picture = buffer;
		free_GMT = true;
	}
	else
		GMT_report (GMT, GMT_MSG_FATAL, "Can only do transparent color for 8- or 24-bit images. -Gt ignored\n");

	if (Ctrl->E.dpi > 0.0) Ctrl->W.width = (double) header.width / Ctrl->E.dpi;
	if (Ctrl->W.height == 0.0) Ctrl->W.height = header.height * Ctrl->W.width / header.width;
	justify = GMT_just_decode (GMT, Ctrl->C.justify, 12);
	Ctrl->C.x -= 0.5 * ((justify-1)%4) * Ctrl->W.width;
	Ctrl->C.y -= 0.5 * (justify/4) * Ctrl->W.height;

	/* The following is needed to have psimage work correctly in perspective */

	GMT_memset (wesn, 4, double);
	if (!(GMT->common.R.active && GMT->common.J.active)) {	/* When no projection specified, use fake linear projection */
		GMT->common.R.active = true;
		GMT->common.J.active = false;
		GMT_parse_common_options (GMT, "J", 'J', "X1i");
		wesn[XHI] = Ctrl->C.x + Ctrl->N.nx * Ctrl->W.width;	wesn[YHI] = Ctrl->C.y + Ctrl->N.ny * Ctrl->W.height;
		GMT_err_fail (GMT, GMT_map_setup (GMT, wesn), "");
		GMT_plotinit (GMT, options);
		GMT_plane_perspective (GMT, GMT->current.proj.z_project.view_plane, GMT->current.proj.z_level);
	}
	else {	/* First use current projection, project, then use fake projection */
		if (GMT_err_pass (GMT, GMT_map_setup (GMT, GMT->common.R.wesn), "")) Return (GMT_RUNTIME_ERROR);
		GMT_plotinit (GMT, options);
		GMT_plane_perspective (GMT, GMT->current.proj.z_project.view_plane, GMT->current.proj.z_level);
		GMT->common.J.active = false;
		GMT_parse_common_options (GMT, "J", 'J', "X1i");
		wesn[XHI] = Ctrl->C.x + Ctrl->N.nx * Ctrl->W.width;	wesn[YHI] = Ctrl->C.y + Ctrl->N.ny * Ctrl->W.height;
		GMT->common.R.active = GMT->common.J.active = true;
		GMT_err_fail (GMT, GMT_map_setup (GMT, wesn), "");
	}

	for (row = 0; row < Ctrl->N.ny; row++) {
		y = Ctrl->C.y + row * Ctrl->W.height;
		if (Ctrl->N.ny > 1) GMT_report (GMT, GMT_MSG_NORMAL, "Replicating image %d times for row %d\n", Ctrl->N.nx, row);
		for (col = 0; col < Ctrl->N.nx; col++) {
			x = Ctrl->C.x + col * Ctrl->W.width;
			if (header.depth == 0)
				PSL_plotepsimage (PSL, x, y, Ctrl->W.width, Ctrl->W.height, PSL_BL, picture, header.length, 
						header.width, header.height, header.xorigin, header.yorigin);
			else if (header.depth == 1) {
				/* Invert is opposite from what is expected. This is to match the behaviour of -Gp */
				if (Ctrl->I.active)
					PSL_plotbitimage (PSL, x, y, Ctrl->W.width, Ctrl->W.height, PSL_BL, picture, 
							header.width, header.height, Ctrl->G.f_rgb, Ctrl->G.b_rgb);
				else
					PSL_plotbitimage (PSL, x, y, Ctrl->W.width, Ctrl->W.height, PSL_BL, picture, 
							header.width, header.height, Ctrl->G.b_rgb, Ctrl->G.f_rgb);
			}
			else
				 PSL_plotcolorimage (PSL, x, y, Ctrl->W.width, Ctrl->W.height, PSL_BL, picture, 
						 PS_transparent * header.width, header.height, PS_interpolate * header.depth);
		}
	}

 	if (Ctrl->F.active) {	/* Draw frame */
 		GMT_setfill (GMT, NULL, true);
		GMT_setpen (GMT, &Ctrl->F.pen);
 		PSL_plotbox (PSL, Ctrl->C.x, Ctrl->C.y, Ctrl->C.x + Ctrl->N.nx * Ctrl->W.width, Ctrl->C.y + Ctrl->N.ny * Ctrl->W.height);
 	}

	GMT_plane_perspective (GMT, -1, 0.0);
	GMT_plotend (GMT);

#ifdef USE_GDAL
	if (GMT_Destroy_Data (API, GMT_ALLOCATED, &I) != GMT_OK) {
		Return (API->error);	/* If I is NULL then nothing is done */
	}
#endif
	if (free_GMT) {
		GMT_free (GMT, picture);
	}
	else if (known)
		PSL_free (picture);

	Return (GMT_OK);
}
