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
 * API functions to support the gmtconvert application.
 *
 * Author:	Walter H.F. Smith
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 * Brief synopsis: Read a grid file and sets all values < the user-supplied
 * lower limit to the value <below>, and all values > the user-supplied
 * upper limit to the value <above>.  above/below can be any number,
 * including NaN.
 */

#define THIS_MODULE k_mod_grdclip /* I am grdclip */

#include "gmt_dev.h"

enum Grdclip_cases {
	GRDCLIP_BELOW	= 1,
	GRDCLIP_BETWEEN	= 2,
	GRDCLIP_ABOVE	= 4
};

/* Control structure for grdclip */

struct GRDCLIP_RECLASSIFY {
	float low, high, between;
	uint64_t n_between;
};

struct GRDCLIP_CTRL {
	struct In {
		bool active;
		char *file;
	} In;
	struct G {	/* -G<output_grdfile> */
		bool active;
		char *file;
	} G;
	struct S {	/* -Sa<high/above>, -Sb<low/below> or -Si<low/high/between> */
		bool active;
		unsigned int mode;
		unsigned int n_class;
		float high, above;
		float low, below;
		struct GRDCLIP_RECLASSIFY *class;
	} S;
};

void *New_grdclip_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDCLIP_CTRL *C;
	
	C = GMT_memory (GMT, NULL, 1, struct GRDCLIP_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */
			
	return (C);
}

void Free_grdclip_Ctrl (struct GMT_CTRL *GMT, struct GRDCLIP_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->In.file) free (C->In.file);	
	if (C->G.file) free (C->G.file);	
	if (C->S.class) free (C->S.class);	
	GMT_free (GMT, C);	
}

int GMT_grdclip_usage (struct GMTAPI_CTRL *C, int level) {
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: grdclip <ingrid> -G<outgrid> [%s]\n", GMT_Rgeo_OPT);
	GMT_message (GMT, "\t[-Sa<high>/<above>] [-Sb<low>/<below>] [-Si<low>/<high>/<between>] [%s]\n", GMT_V_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\n\t<ingrid> is a single grid file.\n");
	GMT_message (GMT, "\t-G Set name of output grid.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "R");
	GMT_message (GMT, "\t-Sa will set all data > high to the <above> value.\n");
	GMT_message (GMT, "\t-Sb will set all data < low to the <below> value.\n");
	GMT_message (GMT, "\t-Si will set all data >= low and <= high to the <between> value.\n");
	GMT_message (GMT, "\t    <above>, <below> and <between> can be any number, including NaN.\n");
	GMT_message (GMT, "\t    Choose at least one -S option; the -Si may be repeated.\n");
	GMT_explain_options (GMT, "V.");
	
	return (EXIT_FAILURE);
}

int compare_classes (const void *point_1v, const void *point_2v)
{
	/*  Needed to sort classes on low value. */
	const struct GRDCLIP_RECLASSIFY *point_1 = point_1v, *point_2 = point_2v;
	
	if (point_1->low < point_2->low) return (-1);
	if (point_1->low > point_2->low) return (+1);
	return (0);
}

int GMT_grdclip_parse (struct GMTAPI_CTRL *C, struct GRDCLIP_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to grdcut and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_files = 0, n_class = 0;
	int n;
	size_t n_alloc = GMT_TINY_CHUNK;
	char txt[GMT_TEXT_LEN64];
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {

			case '<':	/* Input file (only one is accepted) */
				Ctrl->In.active = true;
				if (n_files++ == 0) Ctrl->In.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'G':	/* Output filename */
				Ctrl->G.active = true;
				Ctrl->G.file = strdup (&opt->arg[0]);
				break;
			case 'S':	/* Set limits */
				Ctrl->S.active = true;
				switch (opt->arg[0]) {
				case 'a':
					Ctrl->S.mode |= GRDCLIP_ABOVE;
					n = sscanf (&opt->arg[1], "%f/%s", &Ctrl->S.high, txt);
					if (n != 2) {
						GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -Sa option: Expected -Sa<high>/<above>, <above> may be set to NaN\n");
						n_errors++;
					}
					else 
						Ctrl->S.above = (txt[0] == 'N' || txt[0] == 'n') ? GMT->session.f_NaN : (float)atof (txt);
					break;
				case 'b':
					Ctrl->S.mode |= GRDCLIP_BELOW;
					n = sscanf (&opt->arg[1], "%f/%s", &Ctrl->S.low, txt);
					if (n != 2) {
						GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -Sb option: Expected -Sb<low>/<below>, <below> may be set to NaN\n");
						n_errors++;
					}
					else
						Ctrl->S.below = (txt[0] == 'N' || txt[0] == 'n') ? GMT->session.f_NaN : (float)atof (txt);
					break;
				case 'i':
					Ctrl->S.mode |= GRDCLIP_BETWEEN;
					if (n_class == Ctrl->S.n_class) {	/* Need more memory */
						n_alloc <<= 2;
						Ctrl->S.class = GMT_memory (GMT, Ctrl->S.class, n_alloc, struct GRDCLIP_RECLASSIFY);
					}
					n = sscanf (&opt->arg[1], "%f/%f/%s", &Ctrl->S.class[n_class].low, &Ctrl->S.class[n_class].high, txt);
					if (n != 3) {
						GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -Si option: Expected -Si<low>/<high>/<between>, <between> may be set to NaN\n");
						n_errors++;
					}
					else
						Ctrl->S.class[n_class].between = (txt[0] == 'N' || txt[0] == 'n') ? GMT->session.f_NaN : (float)atof (txt);
					if (Ctrl->S.class[n_class].low > Ctrl->S.class[n_class].high) {
						GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -Si option: <low> cannot exceed <high>!\n");
						n_errors++;
					}
					n_class++;
					break;
				default:
					GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -S option: Expected -Sa<high>/<above>, -Sb<low>/<below> or -Si<low>/<high>/<between>\n");
					n_errors++;
				}
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}
	if (Ctrl->S.mode & GRDCLIP_BETWEEN) {	/* Reallocate, sort and check that no classes overlap */
		unsigned int k;
		Ctrl->S.class = GMT_memory (GMT, Ctrl->S.class, n_class, struct GRDCLIP_RECLASSIFY);
		Ctrl->S.n_class = n_class;
		qsort (Ctrl->S.class, Ctrl->S.n_class, sizeof (struct GRDCLIP_RECLASSIFY), compare_classes);
		for (k = 1; k < Ctrl->S.n_class; k++) {
			if (Ctrl->S.class[k].low < Ctrl->S.class[k-1].high) {
				GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -S option: Reclassification case %d overlaps with case %d\n", k, k-1);
				n_errors++;
			}
		}
		if (Ctrl->S.mode & GRDCLIP_ABOVE && Ctrl->S.high < Ctrl->S.class[Ctrl->S.n_class-1].high) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -S option: Your highest reclassification case overlaps with your -Sa selection\n");
			n_errors++;
		}
		if (Ctrl->S.mode & GRDCLIP_BELOW && Ctrl->S.low > Ctrl->S.class[0].low) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -S option: Your lowest reclassification case overlaps with your -Sb selection\n");
			n_errors++;
		}
	}
	if ((Ctrl->S.mode & GRDCLIP_ABOVE) && (Ctrl->S.mode & GRDCLIP_BELOW) && (Ctrl->S.high < Ctrl->S.low)) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Syntax error -S option: Your -Sa selection overlaps with your -Sb selection\n");
		n_errors++;
	}
	
	n_errors += GMT_check_condition (GMT, n_files != 1, "Syntax error: Must specify a single grid file\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->G.file, "Syntax error -G option: Must specify output file\n");
	n_errors += GMT_check_condition (GMT, !Ctrl->S.mode, "Syntax error -S option: Must specify at least one of -Sa, -Sb, -Si\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_grdclip_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grdclip (void *V_API, int mode, void *args) {
	unsigned int row, col, k;
	int error = 0;
	bool new_grid, go = false;
	
	uint64_t ij, n_above = 0, n_below = 0;
	
	double wesn[4];
	
	struct GRDCLIP_CTRL *Ctrl = NULL;
	struct GMT_GRID *G = NULL, *Out = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_grdclip_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_grdclip_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-VR", "", options)) Return (API->error);
	Ctrl = New_grdclip_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_grdclip_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the grdclip main code ----------------------------*/

	GMT_report (GMT, GMT_MSG_VERBOSE, "Processing input grid\n");
	GMT_memcpy (wesn, GMT->common.R.wesn, 4, double);	/* Current -R setting, if any */
	
	if ((G = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER_ONLY, NULL, Ctrl->In.file, NULL)) == NULL) {
		Return (API->error);
	}
	if (GMT_is_subset (GMT, G->header, wesn)) GMT_err_fail (GMT, GMT_adjust_loose_wesn (GMT, wesn, G->header), "");	/* Subset requested; make sure wesn matches header spacing */
	if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA_ONLY, wesn, Ctrl->In.file, G) == NULL) {
		Return (API->error);	/* Get subset */
	}

	new_grid = GMT_set_outgrid (GMT, G, &Out);	/* true if input is a read-only array */

	GMT_grd_loop (GMT, G, row, col, ij) {
		/* Checking if extremes are exceeded (need not check NaN) */
		if (Ctrl->S.mode & GRDCLIP_ABOVE && G->data[ij] > Ctrl->S.high) {
			Out->data[ij] = Ctrl->S.above;
			n_above++;
		}
		else if (Ctrl->S.mode & GRDCLIP_BELOW && G->data[ij] < Ctrl->S.low) {
			Out->data[ij] = Ctrl->S.below;
			n_below++;
		}
		else if (Ctrl->S.mode & GRDCLIP_BETWEEN) {	/* Reclassifications */
			for (k = 0, go = true; go && k < Ctrl->S.n_class; k++) {
				if ((G->data[ij] >= Ctrl->S.class[k].low && G->data[ij] <= Ctrl->S.class[k].high)) {
					Out->data[ij] = Ctrl->S.class[k].between;
					Ctrl->S.class[k].n_between++;
					go = false;
				}
			}
		}
		else if (new_grid)
			Out->data[ij] = G->data[ij];
	}	

	if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, Ctrl->G.file, Out) != GMT_OK) {
		Return (API->error);
	}

	if (GMT_is_verbose (GMT, GMT_MSG_VERBOSE)) {
		char format[GMT_BUFSIZ], buffer[GMT_BUFSIZ];
		strcpy (format, "%" PRIu64 " values ");
		if (Ctrl->S.mode & GRDCLIP_BELOW) {
			sprintf (buffer, "< %s set to %s\n", GMT->current.setting.format_float_out, GMT->current.setting.format_float_out);
			strcat (format, buffer);
			GMT_report (GMT, GMT_MSG_VERBOSE, format, n_below, (double)Ctrl->S.low, (double)Ctrl->S.below);
		}
		if (Ctrl->S.mode & GRDCLIP_BETWEEN) {
			strcpy (format, "%" PRIu64 " values ");
			sprintf (buffer, "between %s and %s set to %s\n", GMT->current.setting.format_float_out, 
				GMT->current.setting.format_float_out, GMT->current.setting.format_float_out);
			strcat (format, buffer);
			for (k = 0; k < Ctrl->S.n_class; k++) {
				GMT_report (GMT, GMT_MSG_VERBOSE, format, Ctrl->S.class[k].n_between, (double)Ctrl->S.class[k].low, (double)Ctrl->S.class[k].high, (double)Ctrl->S.class[k].between);
			}
		}
		if (Ctrl->S.mode & GRDCLIP_ABOVE) {
			strcpy (format, "%" PRIu64 " values ");
			sprintf (buffer, "> %s set to %s\n", GMT->current.setting.format_float_out, GMT->current.setting.format_float_out);
			GMT_report (GMT, GMT_MSG_VERBOSE, format, n_above, (double)Ctrl->S.high, (double)Ctrl->S.above);
		}
	}

	GMT_report (GMT, GMT_MSG_VERBOSE, "Done!\n");
	Return (GMT_OK);
}
