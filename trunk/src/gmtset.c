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
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 * Brief synopsis: gmtset will override specified defaults parameters.
 *
 */
 
#define GMT_WITH_NO_PS
#define THIS_MODULE k_mod_gmtset /* I am gmtset */

#include "gmt_dev.h"

/* Control structure for gmtset */

struct GMTSET_CTRL {
	struct C {	/* -C */
		bool active;
	} C;
	struct D {	/* -D[s|u] */
		bool active;
		char mode;
	} D;
	struct G {	/* -Gfilename */
		bool active;
		char *file;
	} G;
};

void *New_gmtset_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GMTSET_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct GMTSET_CTRL);
	return (C);
}

void Free_gmtset_Ctrl (struct GMT_CTRL *GMT, struct GMTSET_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->G.file) free (C->G.file);
	GMT_free (GMT, C);	
}

int GMT_gmtset_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: gmtset [-C | -D[s|u] | -G<defaultsfile>] [-[" GMT_SHORTHAND_OPTIONS "]<value>] PARAMETER1 [=] value1 PARAMETER2 [=] value2 PARAMETER3 [=] value3 ...\n");
	GMT_message (GMT, "\n\tFor available PARAMETERS, see gmt.conf man page.\n");

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t-C Convert GMT4 .gmtdefaults4 to GMT5 gmt.conf file.\n");
	GMT_message (GMT, "\t   The original file is retained.\n");
	GMT_message (GMT, "\t-D Modify the default settings based on the GMT system defaults.\n");
	GMT_message (GMT, "\t   Append s to see the SI version of defaults.\n");
	GMT_message (GMT, "\t   Append u to see the US version of defaults.\n");
	GMT_message (GMT, "\t-G Set name of specific gmt.conf file to modify.\n");
	GMT_message (GMT, "\t   [Default looks for file in current directory.  If not found,\n");
	GMT_message (GMT, "\t   it looks in the home directory, if not found it uses GMT defaults.]\n");
	GMT_message (GMT, "\n\tThe modified defaults are written to the current directory as gmt.conf.\n");
	GMT_message (GMT, "\n\t-[" GMT_SHORTHAND_OPTIONS "]<value> (any of these options).\n");
	GMT_message (GMT, "\t   Set the expansion of any of these shorthand options.\n");
	
	return (EXIT_FAILURE);
}

int GMT_gmtset_parse (struct GMTAPI_CTRL *C, struct GMTSET_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to gmtset and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Input files: Here, arguments like ANNOT_OFFSET = -0.1i will show up as "files" or numbers */
			case '#':
				break;

			/* Processes program-specific parameters */

			case 'C':	/* Convert GMT4 .gmtdefaults4 to GMT5 gmt.conf */
				Ctrl->C.active = true;
				break;
			case 'D':	/* Get GMT system-wide defaults settings */
				Ctrl->D.active = true;
				Ctrl->D.mode = opt->arg[0];
				break;
			case 'G':	/* Optional defaults file on input and output */
				Ctrl->G.active = true;
				Ctrl->G.file = strdup (opt->arg);
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}
	
	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_gmtset_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_gmtset (void *V_API, int mode, void *args)
{
	int error = 0;

	char path[GMT_TEXT_LEN256];
	char* gmtconf_file;

	struct GMTSET_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (options) {
		if (options->option == GMTAPI_OPT_USAGE) bailout (GMT_gmtset_usage (API, GMTAPI_USAGE));		/* Return the usage message */
		if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_gmtset_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */
	}

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-V" GMT_SHORTHAND_OPTIONS, "", options)) Return (API->error);
	Ctrl = New_gmtset_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_gmtset_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the gmtset main code ----------------------------*/

	/* Read the supplied default file or the users defaults to override system settings */

	if (Ctrl->D.active) {
		switch (Ctrl->D.mode) {
			case 's':
				gmtconf_file = "gmt_SI.conf";
				break;
			case 'u':
				gmtconf_file = "gmt_US.conf";
				break;
			default:
				gmtconf_file = "gmt.conf";
				break;
		}

		if (! GMT_getsharepath (GMT, "conf", "", gmtconf_file, path))
			GMT_report (GMT, GMT_MSG_NORMAL, "Cannot find GMT configuration file: %s (%s)\n", gmtconf_file, path);
		GMT_getdefaults (GMT, path);
	}
	else if (Ctrl->C.active)
		GMT_getdefaults (GMT, ".gmtdefaults4");
	else if (Ctrl->G.active)
		GMT_getdefaults (GMT, Ctrl->G.file);

	GMT_setdefaults (GMT, options);		/* Process command line arguments */

	GMT_putdefaults (GMT, Ctrl->G.file);	/* Write out the revised settings */

	Return (GMT_OK);
}
