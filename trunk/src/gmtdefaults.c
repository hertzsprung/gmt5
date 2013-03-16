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
 * Brief synopsis: gmtdefaults will list the user's GMT default settings
 * or (by using the -D option), get the site GMT default settings.
 *
 */
 
#define GMT_WITH_NO_PS
#define THIS_MODULE k_mod_gmtdefaults /* I am gmtdefaults */

#include "gmt_dev.h"

#define GMT_PROG_OPTIONS "-V"

/* Control structure for gmtdefaults */

struct GMTDEFAULTS_CTRL {
	struct D {	/* -D[s|u] */
		bool active;
		char mode;
	} D;
};

void *New_gmtdefaults_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GMTDEFAULTS_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct GMTDEFAULTS_CTRL);
	return (C);
}

void Free_gmtdefaults_Ctrl (struct GMT_CTRL *GMT, struct GMTDEFAULTS_CTRL *C) {	/* Deallocate control structure */
	GMT_free (GMT, C);	
}

int GMT_gmtdefaults_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: gmtdefaults [-D[s|u]]\n\n");
	
	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);
	
	GMT_message (GMT, "\t-D Print the default settings for the GMT system.\n");
	GMT_message (GMT, "\t   Append s to see the SI version of defaults.\n");
	GMT_message (GMT, "\t   Append u to see the US version of defaults.\n");
	
	return (EXIT_FAILURE);
}

int GMT_gmtdefaults_parse (struct GMTAPI_CTRL *C, struct GMTDEFAULTS_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to gmtdefaults and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_files = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Count input files */
				n_files++;
				break;

			/* Processes program-specific parameters */

			case 'D':	/* Get GMT system-wide defaults settings */
				Ctrl->D.active = true;
				Ctrl->D.mode = opt->arg[0];
				break;
#ifdef GMT_COMPAT
			case 'L':	/* List the user's current GMT defaults settings */
				GMT_report (GMT, GMT_MSG_COMPAT, "Warning: Option -L is deprecated; it is the default behavior.\n");
				break;
#endif

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, n_files, "Syntax error: No input files are expected\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

/* Must free allocated memory before returning */
#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_gmtdefaults_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_gmtdefaults (void *V_API, int mode, void *args)
{
	int error;
	
	char path[GMT_TEXT_LEN256];
	
	struct GMTDEFAULTS_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (options) {
		if (options->option == GMTAPI_OPT_USAGE) bailout (GMT_gmtdefaults_usage (API, GMTAPI_USAGE));		/* Return the usage message */
		if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_gmtdefaults_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */
	}

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, GMT_PROG_OPTIONS, options)) Return (API->error);
	Ctrl = New_gmtdefaults_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_gmtdefaults_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the gmtdefaults main code ----------------------------*/

	if (Ctrl->D.active) {
		GMT_getsharepath (GMT, "conf", "gmt", (Ctrl->D.mode == 's') ? "_SI" : (Ctrl->D.mode == 'u') ? "_US" : ".conf", path);
		GMT_loaddefaults (GMT, path);
	}

	GMT_putdefaults (GMT, "-");

	Return (GMT_OK);
}
