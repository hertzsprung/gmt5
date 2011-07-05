/*--------------------------------------------------------------------
 *	$Id: testapi_func.c,v 1.5 2011-07-05 19:58:01 guru Exp $
 *
 *	Copyright (c) 1991-2011 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 or any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 * Brief synopsis: testapi allows us to test the API i/o functions.
 *
 */

#include "gmt.h"

/* Control structure for testapi */

struct TESTAPI_CTRL {
	struct T {	/* -T sets data type */
		GMT_LONG active;
		GMT_LONG mode;
	} T;
	struct I {	/* -I sets input method */
		GMT_LONG active;
		GMT_LONG mode;
	} I;
	struct W {	/* -W sets output method */
		GMT_LONG active;
		GMT_LONG mode;
	} W;
};

void *New_testapi_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct TESTAPI_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct TESTAPI_CTRL);

	/* Initialize values whose defaults are not 0/FALSE/NULL */

	return ((void *)C);
}

void Free_testapi_Ctrl (struct GMT_CTRL *GMT, struct TESTAPI_CTRL *C) {	/* Deallocate control structure */
	GMT_free (GMT, C);
}

GMT_LONG GMT_testapi_usage (struct GMTAPI_CTRL *C, GMT_LONG level) {
	struct GMT_CTRL *GMT = C->GMT;

	GMT_message (GMT, "testapi %s [API] - test API i/o methods for any data type\n\n", GMT_VERSION);
	GMT_message (GMT, "usage: testapi -If|s|d|c|r -Td|t|g|c|i|v|m -Wf|s|d [%s]\n", GMT_V_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t-I Specify input resource.  Choose among:\n");
	GMT_message (GMT, "\t   f : File\n");
	GMT_message (GMT, "\t   s : Stream\n");
	GMT_message (GMT, "\t   d : File descriptor\n");
	GMT_message (GMT, "\t   c : Memory Copy\n");
	GMT_message (GMT, "\t   r : Memory Reference\n");
	GMT_message (GMT, "\t-T Specify data type.  Choose among:\n");
	GMT_message (GMT, "\t   d : Dataset\n");
	GMT_message (GMT, "\t   t : Textset\n");
	GMT_message (GMT, "\t   g : Grid\n");
	GMT_message (GMT, "\t   C : CPT\n");
	GMT_message (GMT, "\t   i : Image\n");
	GMT_message (GMT, "\t   v : Vector\n");
	GMT_message (GMT, "\t   m : Matrix\n");
	GMT_message (GMT, "\t-W Specify write destination.  Choose among:\n");
	GMT_message (GMT, "\t   f : File\n");
	GMT_message (GMT, "\t   s : Stream\n");
	GMT_message (GMT, "\t   d : File descriptor\n");
	GMT_message (GMT, "\t   c : Memory Copy\n");
	GMT_message (GMT, "\t   r : Memory Reference\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "V.");
	
	return (EXIT_FAILURE);
}

GMT_LONG GMT_testapi_parse (struct GMTAPI_CTRL *C, struct TESTAPI_CTRL *Ctrl, struct GMT_OPTION *options) {

	/* This parses the options provided to grdcut and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	GMT_LONG n_errors = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {
			/* Processes program-specific parameters */

			case 'I':	/* Input */
				Ctrl->I.active = TRUE;
				switch (opt->arg[0]) {
					case 'f': Ctrl->I.mode = GMT_IS_FILE; break;
					case 's': Ctrl->I.mode = GMT_IS_STREAM; break;
					case 'd': Ctrl->I.mode = GMT_IS_FDESC; break;
					case 'c': Ctrl->I.mode = GMT_IS_COPY; break;
					case 'r': Ctrl->I.mode = GMT_IS_REF; break;
				}
				break;
			case 'T':	/* Type */
				Ctrl->T.active = TRUE;
				switch (opt->arg[0]) {
					case 'd': Ctrl->T.mode = GMT_IS_DATASET; break;
					case 't': Ctrl->T.mode = GMT_IS_TEXTSET; break;
					case 'g': Ctrl->T.mode = GMT_IS_GRID; break;
					case 'c': Ctrl->T.mode = GMT_IS_CPT; break;
					case 'i': Ctrl->T.mode = GMT_IS_IMAGE; break;
					case 'v': Ctrl->T.mode = GMT_IS_VECTOR; break;
					case 'm': Ctrl->T.mode = GMT_IS_MATRIX; break;
				}
				break;

			case 'W':	/* Output */
				Ctrl->W.active = TRUE;
				switch (opt->arg[0]) {
					case 'f': Ctrl->W.mode = GMT_IS_FILE; break;
					case 's': Ctrl->W.mode = GMT_IS_STREAM; break;
					case 'd': Ctrl->W.mode = GMT_IS_FDESC; break;
					case 'c': Ctrl->W.mode = GMT_IS_COPY; break;
					case 'r': Ctrl->W.mode = GMT_IS_REF; break;
				}
				break;
			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define Return(code) {Free_testapi_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); return (code);}

GMT_LONG GMT_testapi (struct GMTAPI_CTRL *API, struct GMT_OPTION *options)
{
	GMT_LONG error = 0, in_ID, out_ID;
	GMT_LONG geometry[7] = {GMT_IS_POINT, GMT_IS_TEXT, GMT_IS_SURFACE, GMT_IS_TEXT, GMT_IS_SURFACE, GMT_IS_POINT, GMT_IS_SURFACE};
	
	char *ikind[7] = {"DATASET", "TEXTSET", "GRID", "CPT", "IMAGE", "VECTOR", "MATRIX"};
	char *ivia[6] = {"FILE", "STREAM", "FDESC", "COPY", "REF", "READONLY"};
	char *ifile[7] = {"dtesti.txt", "ttesti.txt", "gtesti.nc", "ctesti.cpt", "itesti.jpg", "vtesti.bin", "mtesti.bin"};
	char *ofile[7] = {"dtesto.txt", "ttesto.txt", "gtesto.nc", "ctesto.cpt", "itesto.jpg", "vtesto.bin", "mtesto.bin"};
	char string[GMTAPI_STRLEN], *input = NULL, *output = NULL;

	FILE *fp = NULL;
	int *fdp = NULL, fd = 0;
	
	struct TESTAPI_CTRL *Ctrl = NULL;
	void *In = NULL, *Out = NULL, *Intmp = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));

	if (!options || options->option == GMTAPI_OPT_USAGE) return (GMT_testapi_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) return (GMT_testapi_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_module (API, "GMT_testapi", &GMT_cpy);	/* Save current state */
	if ((error = GMT_Parse_Common (API, "-V", ">", options))) Return (error);
	Ctrl = (struct TESTAPI_CTRL *)New_testapi_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_testapi_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the testapi main code ----------------------------*/

	/* Get input and register it */
	
	GMT_report (GMT, GMT_MSG_NORMAL, "Read %s %s via %s and write to %s via %s\n", ikind[Ctrl->T.mode], ifile[Ctrl->T.mode], ivia[Ctrl->I.mode], ofile[Ctrl->T.mode], ivia[Ctrl->W.mode]);
	
	if ((error = GMT_Init_IO (API, Ctrl->T.mode, geometry[Ctrl->T.mode], GMT_IN, GMT_REG_FILES_IF_NONE, options))) Return (error);	/* Registers default input destination, unless already set */
	if ((error = GMT_Begin_IO (API, Ctrl->T.mode, GMT_IN, GMT_BY_SET))) Return (error);	/* Enables data input and sets access mode */

	switch (Ctrl->I.mode) {
		case GMT_IS_FILE:	/* Pass filename */
			error += GMT_Register_IO (API, Ctrl->T.mode, Ctrl->I.mode, geometry[Ctrl->T.mode], GMT_IN, (void **)&(ifile[Ctrl->T.mode]), NULL, NULL, &in_ID);
			break;
		case GMT_IS_STREAM:
			switch (Ctrl->T.mode) {	/* Can only do d, t, c */
				case GMT_IS_DATASET: case GMT_IS_TEXTSET: case GMT_IS_CPT:
					fp = GMT_fopen (GMT, ifile[Ctrl->T.mode], "r");
					error += GMT_Register_IO (API, Ctrl->T.mode, Ctrl->I.mode, geometry[Ctrl->T.mode], GMT_IN, (void **)&fp, NULL, NULL, &in_ID);
					break;
				default:
					GMT_report (GMT, GMT_MSG_FATAL, "GMT_IS_STREAM only allows d, t, c!\n");
					Return (GMT_WRONG_KIND);
					break;
			}
			break;
		case GMT_IS_FDESC:
			switch (Ctrl->T.mode) {	/* Can only do d, t, c */
				case GMT_IS_DATASET: case GMT_IS_TEXTSET: case GMT_IS_CPT:
					fd = open (ifile[Ctrl->T.mode], O_RDONLY);
					fdp = &fd;
					error += GMT_Register_IO (API, Ctrl->T.mode, Ctrl->I.mode, geometry[Ctrl->T.mode], GMT_IN, (void **)&fdp, NULL, NULL, &in_ID);
					break;
				default:
					GMT_report (GMT, GMT_MSG_FATAL, "GMT_IS_FDESC only allows d, t, c!\n");
					Return (GMT_WRONG_KIND);
					break;
			}
			break;
		case GMT_IS_COPY: case GMT_IS_REF:
			error += GMT_Get_Data (API, Ctrl->T.mode, GMT_IS_FILE, geometry[Ctrl->T.mode], NULL, 0, (void **)&(ifile[Ctrl->T.mode]), &Intmp);
			error += GMT_Register_IO (API, Ctrl->T.mode, Ctrl->I.mode, geometry[Ctrl->T.mode], GMT_IN, &Intmp, NULL, Intmp, &in_ID);
			break;
		default:
			GMT_report (GMT, GMT_MSG_FATAL, "Bad Input mode\n");
			Return (GMT_WRONG_KIND);
			break;
	}
	
	if (error) Return (GMT_RUNTIME_ERROR);
	
	/* Now get the data from the source */
	
	GMT_Encode_ID (API, string, in_ID);	/* Make filename with embedded object ID */
	input = strdup (string);
	if (Ctrl->T.mode == GMT_IS_IMAGE) GMT_set_pad (GMT, 0);	/* Temporary turn off padding (and thus BC setting) since we will use image exactly as is */
	error = GMT_Get_Data (API, Ctrl->T.mode, Ctrl->I.mode, geometry[Ctrl->T.mode], NULL, 0, (void **)&input, &In);
	if (Ctrl->T.mode == GMT_IS_IMAGE) GMT_set_pad (GMT, 2);	/* Reset to GMT default */
	if ((error = GMT_End_IO (API, GMT_IN, 0))) Return (error);	/* Disables further data input */
	
	if (Ctrl->T.mode == GMT_IS_IMAGE) {	/* Since writing is not supported we just make a plot via GMT_psimage */
		char buffer[GMT_BUFSIZ];
		error += GMT_Register_IO (API, Ctrl->T.mode, GMT_IS_REF, geometry[Ctrl->T.mode], GMT_IN, &In, NULL, In, &in_ID);
		GMT_Encode_ID (API, string, in_ID);	/* Make filename with embedded object ID */
		sprintf (buffer, "%s -W6i -P -F0.25p --PS_MEDIA=letter", string);
		error += GMT_psimage_cmd (API, 0, (void *)buffer);	/* Plot the image */
		GMT_Destroy_Data (API, GMT_CLOBBER, (void **)&Intmp);
		GMT_report (GMT, GMT_MSG_NORMAL, "Done!\n");
		Return (GMT_OK);
	}
	
	/* Get output and register it */
	
	switch (Ctrl->W.mode) {
		case GMT_IS_FILE:	/* Pass filename */
			error = GMT_Register_IO (API, Ctrl->T.mode, Ctrl->W.mode, geometry[Ctrl->T.mode], GMT_OUT, (void **)&(ofile[Ctrl->T.mode]), NULL, NULL, &out_ID);
			break;
		case GMT_IS_STREAM:
			switch (Ctrl->T.mode) {	/* Can only do d, t, c */
				case GMT_IS_DATASET: case GMT_IS_TEXTSET: case GMT_IS_CPT:
					fp = GMT_fopen (GMT, ofile[Ctrl->T.mode], "w");
					error = GMT_Register_IO (API, Ctrl->T.mode, Ctrl->W.mode, geometry[Ctrl->T.mode], GMT_OUT, (void **)&fp, NULL, NULL, &out_ID);
					break;
				default:
					GMT_report (GMT, GMT_MSG_FATAL, "GMT_IS_STREAM only allows d, t, c!\n");
					Return (GMT_WRONG_KIND);
					break;
			}
			break;
		case GMT_IS_FDESC:
			switch (Ctrl->T.mode) {	/* Can only do d, t, c */
				case GMT_IS_DATASET: case GMT_IS_TEXTSET: case GMT_IS_CPT:
					fd = open (ofile[Ctrl->T.mode], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
					fdp = &fd;
					error = GMT_Register_IO (API, Ctrl->T.mode, Ctrl->W.mode, geometry[Ctrl->T.mode], GMT_OUT, (void **)&fdp, NULL, NULL, &out_ID);
					break;
				default:
					GMT_report (GMT, GMT_MSG_FATAL, "GMT_IS_FDESC only allows d, t, c!\n");
					Return (GMT_WRONG_KIND);
					break;
			}
			break;
		case GMT_IS_COPY: case GMT_IS_REF:
			error = GMT_Register_IO (API, Ctrl->T.mode, Ctrl->W.mode, geometry[Ctrl->T.mode], GMT_OUT, &Out, NULL, Out, &out_ID);
			break;
		default:
			GMT_report (GMT, GMT_MSG_FATAL, "Bad Input mode\n");
			Return (GMT_WRONG_KIND);
			break;
	}
	if (error) Return (GMT_RUNTIME_ERROR);
			
	/* Now put the data to the destination */
	
	GMT_Encode_ID (API, string, out_ID);	/* Make filename with embedded object ID */
	output = strdup (string);

	if ((error = GMT_Init_IO (API, Ctrl->T.mode, geometry[Ctrl->T.mode], GMT_OUT, GMT_REG_FILES_IF_NONE, options))) Return (error);	/* Registers default output destination, unless already set */
	if ((error = GMT_Begin_IO (API, Ctrl->T.mode, GMT_OUT, GMT_BY_SET))) Return (error);	/* Enables data output and sets access mode */
	error = GMT_Put_Data (API, Ctrl->T.mode, Ctrl->W.mode, geometry[Ctrl->T.mode], NULL, 0, (void **)&output, In);
	
	if (Ctrl->W.mode == GMT_IS_COPY || Ctrl->W.mode == GMT_IS_REF) {	/* Must write out what is in memory to the file */
		error = GMT_Put_Data (API, Ctrl->T.mode, GMT_IS_FILE, geometry[Ctrl->T.mode], NULL, 0, (void **)&ofile[Ctrl->T.mode], Out);
	}
	if ((error = GMT_End_IO (API, GMT_OUT, 0))) Return (error);	/* Disables further data output */
		
	free ((void *)input);	free ((void *)output);
	
	GMT_Destroy_Data (API, GMT_CLOBBER, (void **)&Intmp);
	if (!(Ctrl->I.mode == GMT_IS_REF && Ctrl->W.mode == GMT_IS_REF)) GMT_Destroy_Data (API, GMT_CLOBBER, (void **)&Out);
	GMT_report (GMT, GMT_MSG_NORMAL, "Done!\n");
	Return (GMT_OK);
}
