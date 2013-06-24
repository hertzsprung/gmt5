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
 * Launcher for any GMT5 module via the corresponding function.
 * Modules are loaded dynamically from the GMT core library, the
 * optional supplemental library, and any number of custom libraries
 * listed via GMT_CUSTOM_LIBS in gmt.conf.
 *
 * Version:	5
 * Created:	17-June-2013
 *
 */

#include "gmt_dev.h"
#define PROGRAM_NAME	"gmt"

/* Determine the system environmetal parameter that leads to shared libraries */
#if defined _WIN32
#define LIB_PATH "%%PATH%%"
#elif defined __APPLE__
#define LIB_PATH "DYLD_LIBRARY_PATH"
#else
#define LIB_PATH "LD_LIBRARY_PATH"
#endif

int main (int argc, char *argv[]) {
	int status = GMT_NOT_A_VALID_MODULE;	/* Default status code */
	unsigned int modulename_arg_n = 0;	/* Argument number in argv[] that contains module name */
	struct GMTAPI_CTRL *api_ctrl = NULL;	/* GMT API control structure */
	char gmt_module[GMT_TEXT_LEN16] = "gmt";
	char *progname = NULL;			/* Last component from the pathname */
	char *module = NULL;			/* Module name */

	/* Initialize new GMT session */
	if ((api_ctrl = GMT_Create_Session ("gmt", 2U, 0U, NULL)) == NULL)
		return EXIT_FAILURE;

	progname = strdup (GMT_basename (argv[0])); /* Last component from the pathname */
	/* Remove any filename extensions added for example
	 * by the MSYS shell when executing gmt via symlinks */
	GMT_chop_ext (progname);

	/* Test if argv[0] contains a module name: */
	module = progname;	/* Try this module name unless it equals PROGRAM_NAME in which case we just enter the test if argc > 1 */
	if ((!strcmp (module, PROGRAM_NAME) || (status = GMT_Call_Module (api_ctrl, module, GMT_MODULE_EXIST, NULL)) == GMT_NOT_A_VALID_MODULE) && argc > 1) {
		/* argv[0] does not contain a valid module name, and
		 * argv[1] either holds the name of the module or an option: */
		modulename_arg_n = 1;
		module = argv[1];	/* Try this module name */
		if ((status = GMT_Call_Module (api_ctrl, module, GMT_MODULE_EXIST, NULL) == GMT_NOT_A_VALID_MODULE)) {
			/* argv[1] does not contain a valid module name; try prepending gmt: */
			strncat (gmt_module, argv[1], GMT_TEXT_LEN16-4U);
			module = gmt_module;
			status = GMT_Call_Module (api_ctrl, module, GMT_MODULE_EXIST, NULL); /* either GMT_NOERROR or GMT_NOT_A_VALID_MODULE */
		}
	}
	
	if (status == GMT_NOT_A_VALID_MODULE) {
		/* neither argv[0] nor argv[1] contain a valid module name */

		int arg_n;
		for (arg_n = 1; arg_n < argc; ++arg_n) {
			/* Try all remaining arguments: */

			/* Print module list */
			if (!strcmp (argv[arg_n], "--help")) {
				GMT_Call_Module (api_ctrl, NULL, GMT_MODULE_PURPOSE, NULL);
				goto exit;
			}

			/* Print version and exit */
			if (!strcmp (argv[arg_n], "--version")) {
				fprintf (stdout, "%s\n", GMT_PACKAGE_VERSION_WITH_SVN_REVISION);
				goto exit;
			}

			/* Show share directory */
			if (!strcmp (argv[arg_n], "--show-sharedir")) {
				fprintf (stdout, "%s\n", api_ctrl->GMT->session.SHAREDIR);
				goto exit;
			}

			/* Show the directory that contains the 'gmt' executable */
			if (!strcmp (argv[arg_n], "--show-bindir")) {
				fprintf (stdout, "%s\n", api_ctrl->GMT->init.runtime_bindir);
				goto exit;
			}
		} /* for (arg_n = 1; arg_n < argc; ++arg_n) */

		/* If we get here, we were called without a recognized modulename or option
		 *
		 * gmt.c is itself not a module and hence can use fprintf (stderr, ...). Any API needing a
		 * gmt-like application will write one separately [see mex API] */
		fprintf (stderr, "\nGMT - The Generic Mapping Tools, Version %s\n", GMT_VERSION);
		fprintf (stderr, "Copyright 1991-%d Paul Wessel, Walter H. F. Smith, R. Scharroo, J. Luis, and F. Wobbe\n\n", GMT_VERSION_YEAR);

		fprintf (stderr, "This program comes with NO WARRANTY, to the extent permitted by law.\n");
		fprintf (stderr, "You may redistribute copies of this program under the terms of the\n");
		fprintf (stderr, "GNU Lesser General Public License.\n");
		fprintf (stderr, "For more information about these matters, see the file named LICENSE.TXT.\n\n");
		fprintf (stderr, "usage: %s [options] <module name> [module options]\n\n", PROGRAM_NAME);
		fprintf (stderr, "  --help               List and description of GMT modules\n");
		fprintf (stderr, "  --version            Print version and exit\n");
		fprintf (stderr, "  --show-sharedir      Show share directory and exit\n");
		fprintf (stderr, "  --show-bindir        Show directory of executables and exit\n\n");
		if (modulename_arg_n == 1) {
			fprintf (stderr, "ERROR: No module named %s was found,  This could mean:\n", module);
			fprintf (stderr, "  1. There actually is no such module; check your spelling.\n");
			if (strlen (GMT_SUPPL_LIB_NAME))
				fprintf (stderr, "  2. The module exists in the GMT supplemental library, but the library could not be found.\n");
			else
				fprintf (stderr, "  2. The module exists in the GMT supplemental library, but the library was not installed.\n");
			if (api_ctrl->n_shared_libs > 2)
				fprintf (stderr, "  3. The module exists in a GMT custom library, but the library could not be found.\n");
			else
				fprintf (stderr, "  3. The module exists in a GMT custom library, but none was specified via GMT_CUSTOM_LIBS.\n");
			fprintf (stderr, "Shared libraries must be in standard system paths or set via environmental parameter %s.\n\n", LIB_PATH);
		}
		status = EXIT_FAILURE;
		goto exit;
	} /* status == GMT_NOT_A_VALID_MODULE */

	/* Here we have found a recognized GMT module and the API has been initialized.
	 * Now run the specified GMT module: */
	
	status = GMT_Call_Module (api_ctrl, module, argc-1-modulename_arg_n, argv+1+modulename_arg_n);

exit:
	if (progname) free (progname);
	/* Destroy GMT session */
	if (GMT_Destroy_Session (api_ctrl))
		return EXIT_FAILURE;

	return status; /* Return the status from the module */
}
