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
 * Brief synopsis: kml2gmt is a reformatter that takes KML files and extracts GMT tables;
 * it is the opposite of kml2gmt.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */
 
#define THIS_MODULE k_mod_kml2gmt /* I am kml2gmt */

#include "gmt.h"

#define POINT			0
#define LINE			1
#define POLYGON			2

struct KML2GMT_CTRL {
	struct In {	/* in file */
		bool active;
		char *file;
	} In;
	struct Z {	/* -Z */
		bool active;
	} Z;
};
	
void *New_kml2gmt_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct KML2GMT_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct KML2GMT_CTRL);

	/* Initialize values whose defaults are not 0/false/NULL */

	return (C);
}

void Free_kml2gmt_Ctrl (struct GMT_CTRL *GMT, struct KML2GMT_CTRL *C) {	/* Deallocate control structure */
	if (C->In.file) free (C->In.file);
	GMT_free (GMT, C);
}

int GMT_kml2gmt_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: kml2gmt [<kmlfiles>] [-V] [-Z] [%s] [%s] > GMTdata.txt\n", GMT_bo_OPT, GMT_colon_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\tinfile is the Google Earth KML file.\n");
	GMT_message (GMT, "\t  If no file(s) is given, standard input is read.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t<kmlfiles> is one or more KML files from Google Earth or similar.\n");
	GMT_message (GMT, "\t   If no files are given, standard input is read.\n");
	GMT_message (GMT, "\t-Z Output the z-column from the KML file [Only lon,lat is output].\n");
	GMT_explain_options (GMT, "VD0:.");

	return (EXIT_FAILURE);
}

int GMT_kml2gmt_parse (struct GMTAPI_CTRL *C, struct KML2GMT_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to kml2gmt and sets parameters in Ctrl.
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
				Ctrl->In.active = true;
				if (n_files++ == 0) Ctrl->In.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'Z':
 				Ctrl->Z.active = true;
				break;
			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	if (GMT->common.b.active[GMT_IN] && GMT->common.b.ncol[GMT_IN] == 0) GMT->common.b.ncol[GMT_IN] = 2;
	n_errors += GMT_check_condition (GMT, n_files > 1, "Syntax error: Only one file can be processed at the time\n");
	n_errors += GMT_check_condition (GMT, Ctrl->In.active && access (Ctrl->In.file, R_OK), "Syntax error: Cannot read file %s\n", Ctrl->In.file);

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_kml2gmt_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_kml2gmt (void *V_API, int mode, void *args)
{
	unsigned int i, start, fmode = POINT;
	int error = 0;
	size_t length;
	bool scan = true, first = true;
	
	char line[GMT_BUFSIZ], buffer[GMT_BUFSIZ], header[GMT_BUFSIZ], name[GMT_BUFSIZ], description[GMT_BUFSIZ];

	double out[3];
	
	FILE *fp = NULL;

	struct KML2GMT_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;		/* General GMT interal parameters */
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_kml2gmt_usage (API, GMTAPI_USAGE));/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_kml2gmt_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-Vb:", "" GMT_OPT("HMm"), options)) Return (API->error);
	Ctrl = New_kml2gmt_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_kml2gmt_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the kml2gmt main code ----------------------------*/

	GMT_report (GMT, GMT_MSG_VERBOSE, "Processing input KML data\n");
	GMT->current.io.col_type[GMT_IN][GMT_X] = GMT->current.io.col_type[GMT_OUT][GMT_X] = GMT_IS_LON;
	GMT->current.io.col_type[GMT_IN][GMT_Y] = GMT->current.io.col_type[GMT_OUT][GMT_Y] = GMT_IS_LAT;
	GMT_set_segmentheader (GMT, GMT_OUT, true);	/* Turn on segment headers on output */
	GMT_memset (header, GMT_BUFSIZ, char);
	GMT_memset (name, GMT_BUFSIZ, char);
	GMT_memset (description, GMT_BUFSIZ, char);
	
	if ((error = GMT_set_cols (GMT, GMT_OUT, 2 + Ctrl->Z.active)) != GMT_OK) {
		Return (error);
	}
	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_OUT, GMT_REG_DEFAULT, 0, options) != GMT_OK) {	/* Registers default output destination, unless already set */
		Return (API->error);
	}
	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_OUT) != GMT_OK) {	/* Enables data output and sets access mode */
		Return (API->error);
	}

	if (Ctrl->In.active) {
		if ((fp = fopen (Ctrl->In.file, "r")) == NULL) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Cannot open file %s\n", Ctrl->In.file);
			Return (EXIT_FAILURE);
		}
		GMT_report (GMT, GMT_MSG_VERBOSE, "Processing %s\n", Ctrl->In.file);
		sprintf (buffer, "# kml2gmt: KML read from %s\n", Ctrl->In.file);
	}
	else {     /* Just read standard input */
		fp = stdin;
		GMT_report (GMT, GMT_MSG_VERBOSE, "Reading from standard input\n");
		sprintf (buffer, "# kml2gmt: KML read from standard input\n");
	}
	/* Now we are ready to take on some input values */

	strcpy (GMT->current.setting.format_float_out, "%.12g");	/* Get enough decimals */
	
	GMT_Put_Record (API, GMT_WRITE_TABLE_HEADER, buffer);	/* Write this to output */

	while (fgets (line, GMT_BUFSIZ, fp)) {
		if (strstr (line, "<Placemark")) scan = true;
		if (strstr (line, "</Placemark")) scan = false;
		if (!scan) continue;
		if (strstr (line, "<Point")) fmode = POINT;
		if (strstr (line, "<LineString")) fmode = LINE;
		if (strstr (line, "<Polygon")) fmode = POLYGON;
		length = strlen (line);
		if (strstr (line, "<name>")) {
			for (i = 0; i < length && line[i] != '>'; i++);	/* Find end of <name> */
			start = i + 1;
			for (i = start; i < length && line[i] != '<'; i++);	/* Find start of </name> */
			line[i] = '\0';
			strncpy (name, &line[start], GMT_BUFSIZ);
			GMT_chop (name);
			if (first) {
				sprintf (buffer, "# %s\n", &line[start]);
				GMT_Put_Record (API, GMT_WRITE_TABLE_HEADER, buffer);	/* Write this to output */
			}
			first = false;
		}
		if (strstr (line, "<description>")) {
			for (i = 0; i < length && line[i] != '>'; i++);	/* Find end of <description> */
			start = i + 1;
			for (i = start; i < length && line[i] != '<'; i++);	/* Find start of </description> */
			line[i] = '\0';
			strncpy (description, &line[start], GMT_BUFSIZ);
			GMT_chop (description);
			if (first) {
				sprintf (buffer, "# %s\n", &line[start]);
				GMT_Put_Record (API, GMT_WRITE_TABLE_HEADER, buffer);	/* Write this to output */
			}
			first = false;
		}
		if (name[0] || description[0]) {
			GMT->current.io.segment_header[0] = 0;
			if (name[0]) { strcat (GMT->current.io.segment_header, "-L\""); strcat (GMT->current.io.segment_header, name); strcat (GMT->current.io.segment_header, "\""); }
			if (name[0] && description[0]) strcat (GMT->current.io.segment_header, " ");
			if (description[0]) { strcat (GMT->current.io.segment_header, "-D\""); strcat (GMT->current.io.segment_header, description); strcat (GMT->current.io.segment_header, "\""); }
		}
		
		if (!strstr (line, "<coordinates>")) continue;
		/* We get here when the line says coordinates */
		if (fmode == POINT) {	/* Process the single point */
			for (i = 0; i < length && line[i] != '>'; i++);		/* Find end of <coordinates> */
			sscanf (&line[i+1], "%lg,%lg,%lg", &out[GMT_X], &out[GMT_Y], &out[GMT_Z]);
			if (!GMT->current.io.segment_header[0]) sprintf (GMT->current.io.segment_header, "Next Point\n");
			GMT_Put_Record (API, GMT_WRITE_SEGMENT_HEADER, NULL);	/* Write segment header */
			GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);	/* Write this to output */
		}
		else {
			if (!GMT->current.io.segment_header[0]) sprintf (GMT->current.io.segment_header, "Next feature\n");
			GMT_Put_Record (API, GMT_WRITE_SEGMENT_HEADER, NULL);	/* Write segment header */
			
			name[0] = description[0] = 0;
			while (fscanf (fp, "%lg,%lg,%lg", &out[GMT_X], &out[GMT_Y], &out[GMT_Z])) {
				GMT_Put_Record (API, GMT_WRITE_DOUBLE, out);	/* Write this to output */
			}
		}
	}
	if (fp != stdin) fclose (fp);
	if (GMT_End_IO (API, GMT_OUT, 0) != GMT_OK) {	/* Disables further data output */
		Return (API->error);
	}
	
	Return (GMT_OK);
}
