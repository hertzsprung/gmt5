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
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 * Brief synopsis: Read one or more data tables and can concatenated them
 * vertically [Default] or horizontally (pasting), select certain columns,
 * report only first and/or last record per segment, only print segment
 * headers, and only report segments that passes a segment header search.
 */

#define THIS_MODULE k_mod_gmtconvert /* I am gmtconvert */

#include "gmt_dev.h"

#define GMT_PROG_OPTIONS "-:>Vabfghios" GMT_OPT("HMm")

int gmt_get_ogr_id (struct GMT_OGR *G, char *name);
#ifdef GMT_COMPAT
int gmt_parse_o_option (struct GMT_CTRL *GMT, char *arg);
#endif

#define INV_ROWS	1
#define INV_SEGS	2
#define INV_TBLS	4

/* Control structure for gmtconvert */

struct GMTCONVERT_CTRL {
	struct Out {	/* -> */
		bool active;
		char *file;
	} Out;
	struct A {	/* -A */
		bool active;
	} A;
	struct D {	/* -D[<template>] */
		bool active;
		char *name;
	} D;
	struct E {	/* -E */
		bool active;
		int mode;	/* -3, -1, -1, 0, or increment stride */
	} E;
	struct L {	/* -L */
		bool active;
	} L;
	struct I {	/* -I[ast] */
		bool active;
		unsigned int mode;
	} I;
	struct N {	/* -N */
		bool active;
	} N;
	struct Q {	/* -Q<segno> */
		bool active;
		uint64_t seg;
	} Q;
	struct S {	/* -S[~]\"search string\" */
		bool active;
		bool inverse;
		bool regexp;
		bool caseless;
		char *pattern;
	} S;
	struct T {	/* -T */
		bool active;
	} T;
};

void *New_gmtconvert_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GMTCONVERT_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct GMTCONVERT_CTRL);

	/* Initialize values whose defaults are not 0/false/NULL */

	return (C);
}

void Free_gmtconvert_Ctrl (struct GMT_CTRL *GMT, struct GMTCONVERT_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->Out.file) free (C->Out.file);	
	if (C->D.name) free (C->D.name);	
	if (C->S.pattern) free (C->S.pattern);	
	GMT_free (GMT, C);	
}

int GMT_gmtconvert_usage (struct GMTAPI_CTRL *API, int level)
{
	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_Message (API, GMT_TIME_NONE, "usage: gmtconvert [<table>] [-A] [-D[<template>]] [-E[f|l|m<stride>]] [-I[tsr]] [-L] [-N] [-Q<seg>]\n");
	GMT_Message (API, GMT_TIME_NONE, "\t[-S[~]\"search string\"] [-T] [%s] [%s]\n\t[%s] [%s] [%s]\n", GMT_V_OPT, GMT_a_OPT, GMT_b_OPT, GMT_f_OPT, GMT_g_OPT);
	GMT_Message (API, GMT_TIME_NONE, "\t[%s] [%s] [%s]\n\t[%s] [%s]\n\n", GMT_h_OPT, GMT_i_OPT, GMT_o_OPT, GMT_s_OPT, GMT_colon_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_Message (API, GMT_TIME_NONE, "\n\tOPTIONS:\n");
	GMT_Option (API, "<");
	GMT_Message (API, GMT_TIME_NONE, "\t-A Paste files horizontally, not concatenate vertically [Default].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   All files must have the same number of segments and rows,\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   but they may differ in their number of columns.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-D Write individual segments to separate files [Default writes one\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   multisegment file to stdout].  Append file name template which MUST\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   contain a C-style format for a long integer (e.g., %%ld) that represents\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   a sequential segment number across all tables (if more than one table).\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   [Default uses gmtconvert_segment_%%ld.txt (or .bin for binary)].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   Alternatively, supply a template with two long formats and we will\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   replace them with the table number and table segment numbers.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-E Extract first and last point per segment only [Output all points].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   Append f for first only or l for last only.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   Append m<stride> to pass only 1 out of <stride> records.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-I Invert output order of (t)ables, (s)egments, or (r)ecords.  Append any combination of:\n");
	GMT_Message (API, GMT_TIME_NONE, "\t     t: reverse the order of input tables on output.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t     s: reverse the order of segments within each table on output.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t     r: reverse the order of records within each segment on output [Default].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-L Output only segment headers and skip all data records.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   Requires -m and ASCII input data [Output headers and data].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-N Skip records where all fields == NaN [Write all records].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-Q Only output segment number <seg> [All].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-S Only output segments whose headers contain the pattern \"string\".\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   Use -S~\"string\" to output segment that DO NOT contain this pattern.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   If your pattern begins with ~, escape it with \\~.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   To match OGR aspatial values, use name=value.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   To match against extended regular expressions use -S[~]/regexp/[i].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-T Prevent the writing of segment headers.\n");
	GMT_Option (API, "V,a,bi,bo,f,g,h,i,o,s,:,.");
	
	return (EXIT_FAILURE);
}

int GMT_gmtconvert_parse (struct GMT_CTRL *GMT, struct GMTCONVERT_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to gmtconvert and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	size_t arg_length;
	unsigned int n_errors = 0, k, n_files = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMTAPI_CTRL *API = GMT->parent;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Skip input files */
				break;
			case '>':	/* Got named output file */
				if (n_files++ == 0) Ctrl->Out.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'A':	/* pAste mode */
				Ctrl->A.active = true;
				break;
			case 'D':	/* Write each segment to a separate output file */
				Ctrl->D.active = true;
				if (*opt->arg) /* optarg is optional */
					Ctrl->D.name = strdup (opt->arg);
				break;
			case 'E':	/* Extract ends only */
				Ctrl->E.active = true;
				switch (opt->arg[0]) {
					case 'f':		/* Get first point only */
						Ctrl->E.mode = -1; break;
					case 'l':		/* Get last point only */
						Ctrl->E.mode = -2; break;
					case 'm':		/* Set modulo step */
						Ctrl->E.mode = atoi (&opt->arg[1]); break;
					default:		/* Get first and last point only */
						Ctrl->E.mode = -3; break;
				}
				break;
#ifdef GMT_COMPAT
			case 'F':	/* Now obsolete, using -o instead */
				GMT_Report (API, GMT_MSG_COMPAT, "Warning: Option -F is deprecated; use -o instead\n");
				gmt_parse_o_option (GMT, opt->arg);
				break;
#endif
			case 'I':	/* Invert order or tables, segments, rows as indicated */
				Ctrl->I.active = true;
				for (k = 0; opt->arg[k]; k++) {
					switch (opt->arg[k]) {
						case 'f': Ctrl->I.mode |= INV_TBLS; break;	/* Reverse table order */
						case 's': Ctrl->I.mode |= INV_SEGS; break;	/* Reverse segment order */
						case 'r': Ctrl->I.mode |= INV_ROWS; break;	/* Reverse record order */
						default:
							GMT_Report (API, GMT_MSG_NORMAL, "Error: The -I option does not recognize modifier %c\n", (int)opt->arg[k]);
							n_errors++;
							break;
					}
				}
				if (Ctrl->I.mode == 0) Ctrl->I.mode = INV_ROWS;	/* Default is -Ir */
				break;
			case 'L':	/* Only output segment headers */
				Ctrl->L.active = true;
				break;
			case 'N':	/* Skip all-NaN records */
				Ctrl->N.active = true;
				break;
			case 'Q':	/* Only report for specified segment number */
				Ctrl->Q.active = true;
				Ctrl->Q.seg = atol (opt->arg);
				break;
			case 'S':	/* Segment header pattern search */
				Ctrl->S.active = true;
				arg_length = strlen (opt->arg);
				k = (opt->arg[0] == '\\' && arg_length > 3 && opt->arg[1] == '~') ? 1 : 0;	/* Special escape if pattern starts with ~ */
				if (opt->arg[0] == '~') Ctrl->S.inverse = true;
				Ctrl->S.pattern = strdup (&opt->arg[k+Ctrl->S.inverse]);
				if (opt->arg[Ctrl->S.inverse] == '/' && opt->arg[arg_length-2]  == '/'  && opt->arg[arg_length-1]  == 'i' ) {
					Ctrl->S.regexp = true;
					Ctrl->S.caseless = true;
					opt->arg[arg_length-2] = '\0'; /* remove trailing '/i' from pattern string */
				}
				else if (opt->arg[Ctrl->S.inverse] == '/' && opt->arg[arg_length-1]  == '/' ) {
					Ctrl->S.regexp = true;
					opt->arg[arg_length-1] = '\0'; /* remove trailing '/' */
				}
				Ctrl->S.pattern = strdup (&opt->arg[k+Ctrl->S.inverse+Ctrl->S.regexp]); /* remove any '/', '\', and '~' from beginning of pattern */
				break;
			case 'T':	/* Do not write segment headers */
				Ctrl->T.active = true;
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}
	
	n_errors += GMT_check_condition (GMT, GMT->common.b.active[GMT_IN] && GMT->common.b.ncol[GMT_IN] == 0, "Syntax error: Must specify number of columns in binary input data (-bi)\n");
	n_errors += GMT_check_condition (GMT, GMT->common.b.active[GMT_IN] && (Ctrl->L.active || Ctrl->S.active), "Syntax error: -L or -S requires ASCII input data\n");
	n_errors += GMT_check_condition (GMT, Ctrl->D.active && Ctrl->D.name && !strstr (Ctrl->D.name, "%"), "Syntax error: -D Output template must contain %%d\n");
	n_errors += GMT_check_condition (GMT, Ctrl->Q.active && Ctrl->S.active, "Syntax error: Only one of -Q and -S can be used simultaneously\n");
	n_errors += GMT_check_condition (GMT, n_files > 1, "Syntax error: Only one output destination can be specified\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

/* Must free allocated memory before returning */
#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_gmtconvert_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_gmtconvert (void *V_API, int mode, void *args)
{
	bool match = false, ogr_match = false;
	int error = 0, ogr_item = 0;
	unsigned int out_col, col, n_cols_in, n_cols_out, tbl;
	unsigned int n_horizontal_tbls, n_vertical_tbls, tbl_ver, tbl_hor, use_tbl;
	uint64_t last_row, n_rows, row, seg, n_out_seg = 0, out_seg = 0;
	
	double *val = NULL;

	char *method[2] = {"concatenated", "pasted"}, *p = NULL;
	
	struct GMTCONVERT_CTRL *Ctrl = NULL;
	struct GMT_DATASET *D[2] = {NULL, NULL};	/* Pointer to GMT multisegment table(s) in and out */
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_NOT_A_SESSION);
	options = GMT_prep_module_options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_gmtconvert_usage (API, GMTAPI_USAGE));/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_gmtconvert_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, GMT_PROG_OPTIONS, options)) Return (API->error);
	Ctrl = New_gmtconvert_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_gmtconvert_parse (GMT, Ctrl, options))) Return (error);

	/*---------------------------- This is the gmtconvert main code ----------------------------*/

	GMT_Report (API, GMT_MSG_VERBOSE, "Processing input table data\n");

	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_NONE, GMT_IN, GMT_ADD_DEFAULT, 0, options) != GMT_OK) {
		Return (API->error);	/* Establishes data files or stdin */
	}
	
	/* Read in the input tables */
	
	if ((D[GMT_IN] = GMT_Read_Data (API, GMT_IS_DATASET, GMT_IS_FILE, 0, GMT_READ_NORMAL, NULL, NULL, NULL)) == NULL) {
		Return (API->error);
	}

	if (Ctrl->T.active) GMT_set_segmentheader (GMT, GMT_OUT, false);	/* Turn off segment headers on output */

	if (GMT->common.a.active && D[GMT_IN]->n_tables > 1) {
		GMT_Report (API, GMT_MSG_NORMAL, "The -a option requires a single table only.\n");
		Return (GMT_RUNTIME_ERROR);
	}
	if (GMT->common.a.active && D[GMT_IN]->table[0]->ogr) {
		GMT_Report (API, GMT_MSG_NORMAL, "The -a option requires a single table without OGR metadata.\n");
		Return (GMT_RUNTIME_ERROR);
	}
	
	/* Determine number of input and output columns for the selected options.
	 * For -A, require all tables to have the same number of segments and records. */

	for (tbl = n_cols_in = n_cols_out = 0; tbl < D[GMT_IN]->n_tables; tbl++) {
		if (Ctrl->A.active) {	/* All tables must be of the same vertical shape */
			if (tbl && D[GMT_IN]->table[tbl]->n_records  != D[GMT_IN]->table[tbl-1]->n_records)  error = true;
			if (tbl && D[GMT_IN]->table[tbl]->n_segments != D[GMT_IN]->table[tbl-1]->n_segments) error = true;
		}
		n_cols_in += D[GMT_IN]->table[tbl]->n_columns;				/* This is the case for -A */
		n_cols_out = MAX (n_cols_out, D[GMT_IN]->table[tbl]->n_columns);	/* The widest table encountered */
	}
	n_cols_out = (Ctrl->A.active) ? n_cols_in : n_cols_out;	/* Default or Reset since we did not have -A */
	
	if (error) {
		GMT_Report (API, GMT_MSG_NORMAL, "Parsing requires files with same number of records.\n");
		Return (GMT_RUNTIME_ERROR);
	}
	if (n_cols_out == 0) {
		GMT_Report (API, GMT_MSG_NORMAL, "Selection lead to no output columns.\n");
		Return (GMT_RUNTIME_ERROR);
		
	}
	if ((error = GMT_set_cols (GMT, GMT_OUT, n_cols_out)) != GMT_OK) {
		Return (error);
	}
	
	if (Ctrl->S.active && GMT->current.io.ogr == GMT_OGR_TRUE && (p = strchr (Ctrl->S.pattern, '=')) != NULL) {	/* Want to search for an aspatial value */
		*p = 0;	/* Skip the = sign */
		if ((ogr_item = gmt_get_ogr_id (GMT->current.io.OGR, Ctrl->S.pattern)) != GMTAPI_NOTSET) {
			ogr_match = true;
			p++;
			strcpy (Ctrl->S.pattern, p);	/* Move the value over to the start */
		}
	}
	
	/* We now know the exact number of segments and columns and an upper limit on total records.
	 * Allocate data set with a single table with those proportions. This copies headers as well */
	
	D[GMT_OUT] = GMT_alloc_dataset (GMT, D[GMT_IN], n_cols_out, 0, (Ctrl->A.active) ? GMT_ALLOC_HORIZONTAL : GMT_ALLOC_NORMAL);
	
	n_horizontal_tbls = (Ctrl->A.active) ? D[GMT_IN]->n_tables : 1;	/* Only with pasting do we go horizontally */
	n_vertical_tbls   = (Ctrl->A.active) ? 1 : D[GMT_IN]->n_tables;	/* Only for concatenation do we go vertically */
	val = GMT_memory (GMT, NULL, n_cols_in, double);
	
	for (tbl_ver = 0; tbl_ver < n_vertical_tbls; tbl_ver++) {	/* Number of tables to place vertically */
		for (seg = 0; seg < D[GMT_IN]->table[tbl_ver]->n_segments; seg++) {	/* For each segment in the tables */
			if (Ctrl->L.active) D[GMT_OUT]->table[tbl_ver]->segment[seg]->mode = GMT_WRITE_HEADER;	/* Only write segment header */
			if (Ctrl->S.active) {		/* See if the combined segment header has text matching our search string */
				if (match && GMT_polygon_is_hole (D[GMT_IN]->table[tbl_ver]->segment[seg])) match = true;	/* Extend a true match on a perimeter to the trailing holes */
				else if (ogr_match)	/* Compare to aspatial value */
					match = (D[GMT_IN]->table[tbl_ver]->segment[seg]->ogr && strstr (D[GMT_IN]->table[tbl_ver]->segment[seg]->ogr->value[ogr_item], Ctrl->S.pattern) != NULL);		/* true if we matched */
#if !defined(WIN32) || (defined(WIN32) && defined(HAVE_PCRE))
				else if (Ctrl->S.regexp)	/* Compare to ERE */
					match = (D[GMT_IN]->table[tbl_ver]->segment[seg]->header && gmt_regexp_match(GMT, D[GMT_IN]->table[tbl_ver]->segment[seg]->header, Ctrl->S.pattern, Ctrl->S.caseless));		/* true if we matched */
#endif
				else
					match = (D[GMT_IN]->table[tbl_ver]->segment[seg]->header && strstr (D[GMT_IN]->table[tbl_ver]->segment[seg]->header, Ctrl->S.pattern) != NULL);		/* true if we matched */
				if (Ctrl->S.inverse == match) D[GMT_OUT]->table[tbl_ver]->segment[seg]->mode = GMT_WRITE_SKIP;	/* Mark segment to be skipped */
			}
			if (Ctrl->Q.active && seg != Ctrl->Q.seg) D[GMT_OUT]->table[tbl_ver]->segment[seg]->mode = GMT_WRITE_SKIP;	/* Mark segment to be skipped */
			if (D[GMT_OUT]->table[tbl_ver]->segment[seg]->mode) continue;	/* No point copying values given segment content will be skipped */
			n_out_seg++;	/* Number of segments that passed the test */
			last_row = D[GMT_IN]->table[tbl_ver]->segment[seg]->n_rows - 1;
			for (row = n_rows = 0; row <= last_row; row++) {	/* Go down all the rows */
				if (!Ctrl->E.active) { /* Skip this section */ }
				else if (Ctrl->E.mode < 0) {	/* Only pass first or last or both of them, skipping all others */ 
					if (row > 0 && row < last_row) continue;		/* Always skip the middle of the segment */
					if (row == 0 && !(-Ctrl->E.mode & 1)) continue;		/* First record, but we are to skip it */
					if (row == last_row && !(-Ctrl->E.mode & 2)) continue;	/* Last record, but we are to skip it */
				}
				else {	/* Only pass modulo E.mode records */
					if (row % Ctrl->E.mode != 0) continue;
				}
				/* Pull out current virtual row (may consist of a single or many (-A) table rows) */
				for (tbl_hor = out_col = 0; tbl_hor < n_horizontal_tbls; tbl_hor++) {	/* Number of tables to place horizontally */
					use_tbl = (Ctrl->A.active) ? tbl_hor : tbl_ver;
					for (col = 0; col < D[GMT_IN]->table[use_tbl]->segment[seg]->n_columns; col++, out_col++) {	/* Now go across all columns in current table */
						val[out_col] = D[GMT_IN]->table[use_tbl]->segment[seg]->coord[col][row];
					}
				}
				for (col = 0; col < n_cols_out; col++) {	/* Now go across the single virtual row */
					if (col >= n_cols_in) continue;			/* This column is beyond end of this table */
					D[GMT_OUT]->table[tbl_ver]->segment[seg]->coord[col][n_rows] = val[col];
				}
				n_rows++;
			}
			D[GMT_OUT]->table[tbl_ver]->segment[seg]->id = out_seg++;
			D[GMT_OUT]->table[tbl_ver]->segment[seg]->n_rows = n_rows;	/* Possibly shorter than originally allocated if -E is used */
			D[GMT_OUT]->table[tbl_ver]->n_records += n_rows;
			D[GMT_OUT]->n_records = D[GMT_OUT]->table[tbl_ver]->n_records;
			if (D[GMT_IN]->table[tbl_ver]->segment[seg]->label) D[GMT_OUT]->table[tbl_ver]->segment[seg]->label = strdup (D[GMT_IN]->table[tbl_ver]->segment[seg]->label);
			if (D[GMT_IN]->table[tbl_ver]->segment[seg]->header) D[GMT_OUT]->table[tbl_ver]->segment[seg]->header = strdup (D[GMT_IN]->table[tbl_ver]->segment[seg]->header);
			if (D[GMT_IN]->table[tbl_ver]->segment[seg]->ogr) GMT_duplicate_ogr_seg (GMT, D[GMT_OUT]->table[tbl_ver]->segment[seg], D[GMT_IN]->table[tbl_ver]->segment[seg]);
		}
		D[GMT_OUT]->table[tbl_ver]->id = tbl_ver;
	}
	GMT_free (GMT, val);

	if (Ctrl->I.active) {	/* Must reverse the order of tables, segments and/or records */
		unsigned int tbl1, tbl2;
		uint64_t row1, row2, seg1, seg2;
		void *p = NULL;
		if (Ctrl->I.mode & INV_ROWS) {	/* Must actually swap rows */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reversing order of records within each segment.\n");
			for (tbl = 0; tbl < D[GMT_OUT]->n_tables; tbl++) {	/* Number of output tables */
				for (seg = 0; seg < D[GMT_OUT]->table[tbl]->n_segments; seg++) {	/* For each segment in the tables */
					for (row1 = 0, row2 = D[GMT_OUT]->table[tbl]->segment[seg]->n_rows - 1; row1 < D[GMT_OUT]->table[tbl]->segment[seg]->n_rows/2; row1++, row2--) {
						for (col = 0; col < D[GMT_OUT]->table[tbl]->segment[seg]->n_columns; col++)
							double_swap (D[GMT_OUT]->table[tbl]->segment[seg]->coord[col][row1], D[GMT_OUT]->table[tbl]->segment[seg]->coord[col][row2]);
					}
				}
			}
		}
		if (Ctrl->I.mode & INV_SEGS) {	/* Must reorder pointers to segments within each table */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reversing order of segments within each table.\n");
			for (tbl = 0; tbl < D[GMT_OUT]->n_tables; tbl++) {	/* Number of output tables */
				for (seg1 = 0, seg2 = D[GMT_OUT]->table[tbl]->n_segments-1; seg1 < D[GMT_OUT]->table[tbl]->n_segments/2; seg1++, seg2--) {	/* For each segment in the table */
					p = D[GMT_OUT]->table[tbl]->segment[seg1];
					D[GMT_OUT]->table[tbl]->segment[seg1] = D[GMT_OUT]->table[tbl]->segment[seg2];
					D[GMT_OUT]->table[tbl]->segment[seg2] = p;
				}
			}
		}
		if (Ctrl->I.mode & INV_TBLS) {	/* Must reorder pointers to tables within dataset  */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reversing order of tables within the data set.\n");
			for (tbl1 = 0, tbl2 = D[GMT_OUT]->n_tables-1; tbl1 < D[GMT_OUT]->n_tables/2; tbl1++, tbl2--) {	/* For each table */
				p = D[GMT_OUT]->table[tbl1];
				D[GMT_OUT]->table[tbl1] = D[GMT_OUT]->table[tbl2];
				D[GMT_OUT]->table[tbl2] = p;
			}
		}
	}

	/* Now ready for output */

	if (Ctrl->D.active) {
		/* TODO: move this block to GMT_gmtconvert_parse() and replace exit() with
		 * Return() */
		/* Must write individual segments to separate files so create the needed template */
		unsigned int n_formats = 0;
		if (!Ctrl->D.name) {
			Ctrl->D.name = GMT->common.b.active[GMT_OUT] ?
					strdup ("gmtconvert_segment_%d.bin") : strdup ("gmtconvert_segment_%d.txt");
			D[GMT_OUT]->io_mode = GMT_WRITE_SEGMENT;
		}
		else { /* Ctrl->D.name */
			/* need to check correct format */
			char *p = Ctrl->D.name;
			while ( (p = strchr (p, '%')) ) {
				/* found %, now check format */
				++p;	/* Skip the % sign */
				p += strspn (p, "0123456789"); /* span past digits */
				if ( strspn (p, "diu") == 0 ) {
					/* no valid conversion specifier */
					GMT_Report (API, GMT_MSG_NORMAL,
							"Syntax error: Use of unsupported conversion specifier at %" PRIuS " in format string '%s'.\n",
							p - Ctrl->D.name + 1, Ctrl->D.name);
					exit (EXIT_FAILURE);
				}
				++n_formats;
			}
			if ( n_formats == 0 || n_formats > 2 ) {
				/* Incorrect number of format specifiers */
					GMT_Report (API, GMT_MSG_NORMAL,
							"Syntax error: Incorrect number of format specifiers in format string '%s'.\n",
							Ctrl->D.name);
					exit (EXIT_FAILURE);
			}
			D[GMT_OUT]->io_mode = (n_formats == 2) ? GMT_WRITE_TABLE_SEGMENT: GMT_WRITE_SEGMENT;
			/* The io_mode tells the i/o function to split segments into files */
			if (Ctrl->Out.file)
				free (Ctrl->Out.file);
			Ctrl->Out.file = strdup (Ctrl->D.name);
		} /* Ctrl->D.name */
	}
	else {
		/* Just register output to stdout or given file via ->outfile */
		if (GMT->common.a.output)
			D[GMT_OUT]->io_mode = GMT_WRITE_OGR;
	}

	if (GMT_Write_Data (API, GMT_IS_DATASET, GMT_IS_FILE, D[GMT_IN]->geometry, D[GMT_OUT]->io_mode, NULL, Ctrl->Out.file, D[GMT_OUT]) != GMT_OK) {
		Return (API->error);
	}

	GMT_Report (API, GMT_MSG_VERBOSE, "%d tables %s, %ld records passed (input cols = %d; output cols = %d)\n", D[GMT_IN]->n_tables, method[Ctrl->A.active], D[GMT_OUT]->n_records, n_cols_in, n_cols_out);
	if (Ctrl->S.active) GMT_Report (API, GMT_MSG_VERBOSE, "Extracted %ld from a total of %ld segments\n", n_out_seg, D[GMT_OUT]->n_segments);

	Return (GMT_OK);
}
