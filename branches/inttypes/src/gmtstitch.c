/*
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
 * API functions to support the gmtstitch application.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 *
 * Brief synopsis: gmtstitch will combine pieces of coastlines or similar segments
 * into acontinuous line, polygon, or group of lines/polygons so that the jump
 * between segment endpoints exceeds a specified threshold.
 */

#include "gmt.h"

/* Control structure for gmtstitch */

struct GMTSTITCH_CTRL {
	struct Out {	/* -> */
		BOOLEAN active;
		char *file;
	} Out;
	struct C {	/* -C[<file>] */
		BOOLEAN active;
		char *file;
	} C;
	struct D {	/* -D[<file>] */
		BOOLEAN active;
		char *format;
	} D;
	struct L {	/* -L[<file>] */
		BOOLEAN active;
		char *file;
	} L;
	struct Q {	/* -Q[<file>] */
		BOOLEAN active;
		char *file;
	} Q;
	struct T {	/* -T<cutoff[unit][/<nn_dist]> */
		BOOLEAN active[2];
		GMT_LONG mode;
		double dist[2];
		char unit;
	} T;
};

#define SEG_I	0
#define SEG_J	1
#define END_A	0
#define END_B	1

#define CLOSED	0
#define OPEN	1

struct BUDDY {
	COUNTER_LARGE id;
	COUNTER_LARGE orig_id;
	GMT_LONG end_order;
	double dist, next_dist;
};

struct LINK {
	COUNTER_LARGE id;
	COUNTER_LARGE orig_id;
	COUNTER_LARGE pos;
	COUNTER_LARGE n;
	COUNTER_MEDIUM group;
	BOOLEAN used;
	double x_end[2];
	double y_end[2];
	struct BUDDY buddy[2];
};

static void *New_gmtstitch_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GMTSTITCH_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct GMTSTITCH_CTRL);

	return (C);
}

static void Free_gmtstitch_Ctrl (struct GMT_CTRL *GMT, struct GMTSTITCH_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->Out.file) free (C->Out.file);
	if (C->C.file) free (C->C.file);
	if (C->D.format) free (C->D.format);
	if (C->L.file) free (C->L.file);
	if (C->Q.file) free (C->Q.file);
	GMT_free (GMT, C);
}

static GMT_LONG GMT_gmtstitch_usage (struct GMTAPI_CTRL *C, GMT_LONG level)
{
	struct GMT_CTRL *GMT = C->GMT;

	GMT_message (GMT, "gmtstitch %s [API] - Join individual lines whose end points match within tolerance\n\n", GMT_VERSION);
	GMT_message (GMT, "usage: gmtstitch [<table>] [-C<closedfile>] [-D[<template>]] [-L[<linkfile>]] [-Q<list>]\n");
	GMT_message (GMT, "\t-T%s[/<nn_dist>] [%s] [%s]\n\t[%s] [%s] [%s]\n\t[%s] [%s] [%s]\n\n",
		GMT_DIST_OPT, GMT_V_OPT, GMT_b_OPT, GMT_f_OPT, GMT_g_OPT, GMT_h_OPT, GMT_i_OPT, GMT_i_OPT, GMT_colon_OPT);

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_explain_options (GMT, "<");
	GMT_message (GMT, "\t-C Write already-closed polygons to a separate <closedfile> [gmtstitch_closed.txt]\n");
	GMT_message (GMT, "\t   than all other segments [All segments are written to one file; see -D].\n");
	GMT_message (GMT, "\t-D Write individual segments to separate files [Default writes one multisegment file to stdout].\n");
	GMT_message (GMT, "\t   Append file name template which MUST contain a C-format specifier for an integer (e.g., %%d).\n");
	GMT_message (GMT, "\t   If the format also includes a %%c string BEFORE the %%d part we replace it with C(losed) or O(pen)\n");
	GMT_message (GMT, "\t   [Default uses gmtstitch_segment_%%d.txt].\n");
	GMT_message (GMT, "\t-L Write link information (seg id, begin/end nearest seg id, end, and distance) to file [gmtstitch_link.txt].\n");
	GMT_message (GMT, "\t   Link output excludes duplicates and segments already forming a closed polygon.\n");
	GMT_explain_options (GMT, "V");
	GMT_dist_syntax (GMT, 'T', "Set cutoff distance to determine if a segment is closed.");
	GMT_message (GMT, "\t   If two lines has endpoints closer than this cutoff they will be joined.\n");
	GMT_message (GMT, "\t   Optionally, append <nn_dist> which adds the requirement that the second closest\n");
	GMT_message (GMT, "\t   match must exceed <nn_dist> (must be in the same units as <cutoff>).\n");
	GMT_message (GMT, "\t-Q Used with -D to write names of files to a list.  Optionally give list name [gmtstitch_list.txt].\n");
	GMT_message (GMT, "\t   Embed %%c in the list name to write two separate lists: one for C(losed) and one for O(pen).\n");
	GMT_explain_options (GMT, "C2D0fghio:.");

	return (EXIT_FAILURE);
}

static GMT_LONG GMT_gmtstitch_parse (struct GMTAPI_CTRL *C, struct GMTSTITCH_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to gmtstitch and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	COUNTER_MEDIUM n_errors = 0, n = 0, n_files = 0;
	char A[GMT_TEXT_LEN64], B[GMT_TEXT_LEN64];
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {
		switch (opt->option) {

			case '<':	/* Skip input files */
				break;
			case '>':	/* Got named output file */
				if (n_files++ == 0) Ctrl->Out.file = strdup (opt->arg);
				break;

			/* Processes program-specific parameters */

			case 'C':	/* Separate closed from open segments  */
				Ctrl->C.active = TRUE;
				if (opt->arg[0]) Ctrl->C.file = strdup (opt->arg);
				break;
			case 'D':	/* Write each segment to a separate output file */
				Ctrl->D.active = TRUE;
				if (opt->arg[0]) Ctrl->D.format = strdup (opt->arg);
				break;
			case 'L':	/* Write link information to file */
				Ctrl->L.active = TRUE;
				if (opt->arg[0]) Ctrl->L.file = strdup (opt->arg);
				break;
			case 'Q':	/* Write names of individual files to list(s) */
				Ctrl->Q.active = TRUE;
				if (opt->arg[0]) Ctrl->Q.file = strdup (opt->arg);
				break;
			case 'T':	/* Set threshold distance */
				Ctrl->T.active[0] = TRUE;
				n = sscanf (opt->arg, "%[^/]/%s", A, B);
				Ctrl->T.mode = GMT_get_distance (GMT, A, &(Ctrl->T.dist[0]), &(Ctrl->T.unit));
				if (n == 2) {
					Ctrl->T.dist[1] = atof (B);
					Ctrl->T.active[1] = TRUE;
				}
				break;
			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += GMT_check_condition (GMT, Ctrl->T.mode == -1, "Syntax error -T: Unrecognized unit\n");
	n_errors += GMT_check_condition (GMT, Ctrl->T.mode == -2, "Syntax error -T: Unable to decode distance\n");
	n_errors += GMT_check_condition (GMT, Ctrl->T.mode == -3, "Syntax error -T: Distance is negative\n");
	if (GMT->common.b.active[GMT_IN] && GMT->common.b.ncol[GMT_IN] == 0) GMT->common.b.ncol[GMT_IN] = 2;
	n_errors += GMT_check_condition (GMT, GMT->common.b.active[GMT_IN] && GMT->common.b.ncol[GMT_IN] < 2, "Syntax error: Binary input data (-bi) must have at least 2 columns\n");
	n_errors += GMT_check_condition (GMT, Ctrl->C.active && Ctrl->D.active, "Syntax error: Option -C cannot be used with -D!\n");
	n_errors += GMT_check_condition (GMT, Ctrl->C.active && Ctrl->D.active, "Syntax error: Option -C cannot be used with -D!\n");
	n_errors += GMT_check_condition (GMT, n_files > 1, "Syntax error: Only one output destination can be specified\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

static GMT_LONG connect (struct LINK *S, GMT_LONG id, GMT_LONG order, double cutoff, GMT_LONG nn_check, double nn_dist)
{	/* Checks if OK to connect this segment to its nearest neighbor and returns TRUE if OK */

	if (S[S[id].buddy[order].id].used) return (FALSE);		/* Segment has been used already */
	if (S[id].buddy[order].dist > cutoff) return (FALSE);		/* Exceeds minimum gap */
	if (!nn_check) return (TRUE);					/* Passed all requirements */
	if (S[id].buddy[order].next_dist > nn_dist) return (TRUE);	/* Next neighboor is far enough away */
	return (FALSE);							/* Failed all tests */
}

static COUNTER_LARGE Copy_This_Segment (struct GMT_LINE_SEGMENT *in, struct GMT_LINE_SEGMENT *out, COUNTER_LARGE out_start, COUNTER_LARGE in_start, COUNTER_LARGE in_end)
{
	COUNTER_LARGE row_in, row_out;
	COUNTER_MEDIUM col;
	GMT_LONG inc;
	BOOLEAN done = FALSE;

	/* We will copy the records from the out segment from rows in_start up to and including in_end.
	 * If in_start > in_end then we will end up reversing the order of the records.
	 * The records are copied to the out segment starting at output record out_start (usually 0).
	 * We return the next row number for output.
	 */
	inc = (in_start < in_end) ? +1 : -1;
	for (row_in = in_start, row_out = out_start; !done; row_in += inc, row_out++) {	/* Either loop from 1st to last or the other way */
		for (col = 0; col < in->n_columns; col++) out->coord[col][row_out] = in->coord[col][row_in];
		done = (row_in == in_end);
	}
	return (row_out);	/* The next output record number */
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_gmtstitch_Ctrl (GMT, Ctrl); GMT_free (GMT, segment); GMT_end_module (GMT, GMT_cpy); bailout (code);}

GMT_LONG GMT_gmtstitch (struct GMTAPI_CTRL *API, GMT_LONG mode, void *args)
{
	GMT_LONG nearest_end[2][2], ii, end, end_order, error = 0, d_mode = 0;
	GMT_LONG io_mode = GMT_WRITE_SET, q_mode = 0;
	
	BOOLEAN save_type = FALSE, first, wrap_up = FALSE, done, *skip = NULL;
	
	COUNTER_MEDIUM j, tbl, n_columns, n_qfiles = 0, G;
	
	COUNTER_LARGE dim_tscr[4] = {1, 1, 0, 0};
	
	size_t n_id_alloc = GMT_CHUNK, n_seg_alloc[2] = {0, 0}, n_alloc_pts;
	
	COUNTER_LARGE n, k, n_rows, seg, np, ns, n_open, out_seg, out_p, id, id2, start_id, iseg, jseg, n_steps, n_seg_length;
	COUNTER_LARGE n_islands = 0, n_trouble = 0, n_closed = 0, chain = 0, match = 0, L;

	double dd[2][2], p_dummy_x, p_dummy_y, p_last_x, p_last_y, p_first_x, p_first_y, distance;
	double closed_dist = 0.0;

	char buffer[GMT_BUFSIZ], *BE = "BE", *ofile = NULL;

	struct LINK *segment = NULL;
	struct GMT_DATASET *D[2] = {NULL, NULL}, *C = NULL;
	struct GMT_TEXTSET *Q = NULL;
	struct GMT_LINE_SEGMENT **T[2] = {NULL, NULL};
	struct GMT_TEXT_SEGMENT *QT[2] = {NULL, NULL};
	struct GMTSTITCH_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_gmtstitch_usage (API, GMTAPI_USAGE));/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_gmtstitch_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_module (API, "GMT_gmtstitch", &GMT_cpy);	/* Save current state */
	if (GMT_Parse_Common (API, "-Vbf:", "ghios>" GMT_OPT("HMm"), options)) Return (API->error);
	Ctrl = New_gmtstitch_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_gmtstitch_parse (API, Ctrl, options))) Return (error);

	/*---------------------------- This is the gmtstitch main code ----------------------------*/

	/* Now we are ready to take on some input values */

	if (Ctrl->D.active) {	/* We want to output to go to individual files for each segment */
		io_mode = GMT_WRITE_SEGMENTS;
		if (!Ctrl->D.format) Ctrl->D.format = strdup ("gmtstitch_segment_%ld.txt");
		if (strstr (Ctrl->D.format, "%c")) save_type = TRUE;
		if (Ctrl->Q.active) {	/* We also want to build list(s) those files */
			if (!Ctrl->Q.file) Ctrl->Q.file = strdup ("gmtstitch_list.txt");
			dim_tscr[0] = n_qfiles = (strstr (Ctrl->Q.file, "%c")) ? 2 : 1;	/* Build one or two tables (closed and open) */
			if ((Q = GMT_Create_Data (GMT->parent, GMT_IS_TEXTSET, dim_tscr)) == NULL) {
				GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a text set for segment lists\n");
				return (GMT->parent->error);
			}
			if (dim_tscr[0] == 2) {	/* We want to build two lists (closed and open) */
				q_mode = GMT_WRITE_TABLES;
				dim_tscr[0] = 1;	/* Reset */
				sprintf (buffer, Ctrl->Q.file, 'C');	Q->table[CLOSED]->file[GMT_OUT] = strdup (buffer);
				sprintf (buffer, Ctrl->Q.file, 'O');	Q->table[OPEN]->file[GMT_OUT] = strdup (buffer);
				QT[CLOSED] = Q->table[CLOSED]->segment[0];		QT[OPEN] = Q->table[OPEN]->segment[0];
				QT[OPEN]->n_alloc = QT[CLOSED]->n_alloc = GMT_CHUNK;
				QT[OPEN]->record = GMT_memory (GMT, NULL, QT[OPEN]->n_alloc, char *);
				QT[CLOSED]->record = GMT_memory (GMT, NULL, QT[CLOSED]->n_alloc, char *);
			}
			else {	/* A single list will do */
				q_mode = GMT_WRITE_SET;
				Q->table[0]->file[GMT_OUT] = strdup (Ctrl->Q.file);
				QT[OPEN] = QT[CLOSED] = Q->table[0]->segment[0];	/* Same table */
				QT[OPEN]->n_alloc = GMT_CHUNK;
				QT[OPEN]->record = GMT_memory (GMT, NULL, QT[OPEN]->n_alloc, char *);
			}
		}
	}

	GMT_init_distaz (GMT, Ctrl->T.unit, Ctrl->T.mode, GMT_MAP_DIST);

	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_IN, GMT_REG_DEFAULT, 0, options) != GMT_OK) {	/* Establishes data input */
		Return (API->error);
	}
	if ((D[GMT_IN] = GMT_Read_Data (API, GMT_IS_DATASET, GMT_IS_FILE, 0, GMT_READ_NORMAL, NULL, NULL, NULL)) == NULL) {
		Return (API->error);
	}

	if (D[GMT_IN]->n_records == 0) {	/* Empty files, nothing to do */
		GMT_report (GMT, GMT_MSG_NORMAL, "No data records found.\n");
		Return (GMT_RUNTIME_ERROR);
	}

	segment = GMT_memory (GMT, NULL, n_id_alloc, struct LINK);
	id = ns = out_seg = 0;
	GMT_report (GMT, GMT_MSG_NORMAL, "Check for closed polygons\n");

	/* Closed polygons are already finished - just identify, write out, and move on */

	/* Allocated D[GMT_OUT] and possibly C, both with nrows = ncolumns so just segment structs are allocated */

	n_columns = dim_tscr[2] = D[GMT_IN]->n_columns;	/* Set the required columns for output */

	n_seg_alloc[0] = dim_tscr[1] = 0;	/* Allocate no segments for now - we will do this as needed */
	if ((D[GMT_OUT] = GMT_Create_Data (GMT->parent, GMT_IS_DATASET, dim_tscr)) == NULL) {
		GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a data set for output segments\n");
		return (GMT->parent->error);
	}
	n_seg_alloc[0] = D[GMT_IN]->n_segments;	/* Cannot end up with more segments than given on input  */
	T[OPEN] = GMT_memory (GMT, NULL, n_seg_alloc[0], struct GMT_LINE_SEGMENT *);
	n_seg_alloc[1] = 0;	/* Allocate no segments for now - we will do this as needed */

	if (Ctrl->C.active) {	/* Wish to return already-closed polygons via a separate file */
		if ((C = GMT_Create_Data (GMT->parent, GMT_IS_DATASET, dim_tscr)) == NULL) {
			GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a data set for closed segments\n");
			return (GMT->parent->error);
		}
		if (!Ctrl->C.file[0]) strcpy (Ctrl->C.file, "gmtstitch_closed.txt");
		n_seg_alloc[1] = n_seg_alloc[0];	/* Cannot end up with more closed segments than given on input  */
		T[CLOSED] = GMT_memory (GMT, NULL, n_seg_alloc[1], struct GMT_LINE_SEGMENT *);
	}
	else
		T[CLOSED] = T[OPEN];	/* Everything returned via same dataset */

	/* Start processing all the segments */

	ns = 0;		/* So the ID of the very first segment will be 0 */
	n_open = n_closed = 0;	/* Numbers of open segments and closed polygons found */
	closed_dist = (Ctrl->C.active) ? Ctrl->T.dist[0] : 0.0;

	for (tbl = 0; tbl < D[GMT_IN]->n_tables; tbl++) {	/* For each data table */
		for (seg = 0; seg < D[GMT_IN]->table[tbl]->n_segments; seg++) {	/* For each segment */
			np = D[GMT_IN]->table[tbl]->segment[seg]->n_rows;	/* Short-hand to avoid the full expression below */
			/* Get distance between first and last point in this segment */
			distance = GMT_distance (GMT, D[GMT_IN]->table[tbl]->segment[seg]->coord[GMT_X][0], D[GMT_IN]->table[tbl]->segment[seg]->coord[GMT_Y][0], D[GMT_IN]->table[tbl]->segment[seg]->coord[GMT_X][np-1], D[GMT_IN]->table[tbl]->segment[seg]->coord[GMT_Y][np-1]);
			if (np > 2 && distance <= closed_dist) {	/* Already closed, just write out and forget in the rest of the program */
				T[CLOSED][out_seg] = GMT_memory (GMT, NULL, 1, struct GMT_LINE_SEGMENT);	/* Allocate segment structure */
				if (Ctrl->D.active) {	/* Write closed polygons to individual files */
					(save_type) ? sprintf (buffer, Ctrl->D.format, 'C', out_seg) : sprintf (buffer, Ctrl->D.format, out_seg);
					T[CLOSED][out_seg]->file[GMT_OUT] = strdup (buffer);
					if (Ctrl->Q.active) {	/* Also maintain list of such files */
						QT[CLOSED]->record[QT[CLOSED]->n_rows++] = strdup (buffer);
						if (QT[CLOSED]->n_rows == QT[CLOSED]->n_alloc) QT[CLOSED]->record = GMT_memory (GMT, QT[CLOSED]->record, (QT[CLOSED]->n_alloc <<= 2), char *);
					}
				}
				/* Allocate space for this segment */
				n_rows = (Ctrl->C.active && distance > 0.0) ? np + 1 : np;
				GMT_alloc_segment (GMT, T[CLOSED][out_seg], n_rows, n_columns, TRUE);

				if (D[GMT_IN]->table[tbl]->segment[seg]->header) T[CLOSED][out_seg]->header = strdup (D[GMT_IN]->table[tbl]->segment[seg]->header);
				out_p = Copy_This_Segment (D[GMT_IN]->table[tbl]->segment[seg], T[CLOSED][out_seg], 0, 0, np-1);
				if (Ctrl->C.active && distance > 0.0) out_p = Copy_This_Segment (D[GMT_IN]->table[tbl]->segment[seg], T[CLOSED][out_seg], out_p, 0, 0);	/* Close polygon */
				n_islands++;
				out_seg++;	/* Number of closed segments placed in T[CLOSED] so far */
				n_closed++;
			}
			else if (Ctrl->C.active) {	/* Copy open segment to separate output dataset */
				/* Allocate space for this segment */
				T[OPEN][n_open] = GMT_memory (GMT, NULL, 1, struct GMT_LINE_SEGMENT);	/* Allocate segment structure */
				GMT_alloc_segment (GMT, T[OPEN][n_open], np, n_columns, TRUE);
				if (D[GMT_IN]->table[tbl]->segment[seg]->header) T[OPEN][n_open]->header = strdup (D[GMT_IN]->table[tbl]->segment[seg]->header);
				out_p = Copy_This_Segment (D[GMT_IN]->table[tbl]->segment[seg], T[OPEN][n_open], 0, 0, np-1);
				n_open++;	/* Number of open segments placed in T[OPEN] so far */
			}
			else { /* No -C: Here we have a segment that is not closed.  Store refs to D[GMT_IN]->table and copy end points; more work on linking takes place below */

				if (np == 1) GMT_report (GMT, GMT_MSG_NORMAL, "Segment %ld only consists of a single point.  Stitching may require additional stitching.\n", id);
				segment[id].id = id;
				segment[id].orig_id = ns;
				segment[id].group = tbl;
				segment[id].pos = seg;
				segment[id].n = np;
				segment[id].x_end[0] = D[GMT_IN]->table[tbl]->segment[seg]->coord[GMT_X][0];
				segment[id].y_end[0] = D[GMT_IN]->table[tbl]->segment[seg]->coord[GMT_Y][0];
				segment[id].x_end[1] = D[GMT_IN]->table[tbl]->segment[seg]->coord[GMT_X][np-1];
				segment[id].y_end[1] = D[GMT_IN]->table[tbl]->segment[seg]->coord[GMT_Y][np-1];
				segment[id].buddy[0].dist = segment[id].buddy[1].dist = segment[id].buddy[0].next_dist = segment[id].buddy[1].next_dist = DBL_MAX;
				id++;
				if (id == n_id_alloc) {
					size_t old_n_id_alloc = n_id_alloc;
					n_id_alloc <<= 1;
					segment = GMT_memory (GMT, segment, n_id_alloc, struct LINK);
					GMT_memset (&(segment[old_n_id_alloc]), n_id_alloc - old_n_id_alloc, struct LINK);	/* Set to NULL/0 */
				}
			}
			ns++;	/* Increment running segment ID */
		}
	}
	if (Ctrl->C.active) {
		C->table[0]->segment = GMT_memory (GMT, T[OPEN], n_open, struct GMT_LINE_SEGMENT *);
		C->n_segments = C->table[0]->n_segments = n_closed;
	}

	if (Ctrl->C.active) {	/* With -C we only separate closed from open and then we are done */
		GMT_report (GMT, GMT_MSG_NORMAL, "Separated %ld closed and %ld open segments\n", n_closed, n_open);
		wrap_up = 2;
	}
	else if (id == 0) {	/* All segments were already closed polygons */
		GMT_report (GMT, GMT_MSG_NORMAL, "All segments already form closed polygons - no new segment file created\n");
		wrap_up = 1;
	}
	if (n_open > 1 || n_closed > 1) GMT_set_segmentheader (GMT, GMT_OUT, TRUE);	/* Turn on segment headers on output */
	if (wrap_up) {	/* Write out results and return */
		if (wrap_up == 2) {	/* Write n_open segments to D[OUT] and n_closed to C */
			D[GMT_OUT]->table[0]->segment = GMT_memory (GMT, T[CLOSED], n_closed, struct GMT_LINE_SEGMENT *);
			D[GMT_OUT]->n_segments = D[GMT_OUT]->table[0]->n_segments = n_closed;
			if (GMT_Write_Data (API, GMT_IS_TEXTSET, GMT_IS_FILE, GMT_IS_POINT, GMT_WRITE_SET, NULL, Ctrl->C.file, C) != GMT_OK) {
				Return (API->error);
			}
			if (GMT_Write_Data (API, GMT_IS_TEXTSET, GMT_IS_FILE, GMT_IS_POINT, GMT_WRITE_SET, NULL, Ctrl->Out.file, D[GMT_OUT]) != GMT_OK) {
				Return (API->error);
			}
		}
		if (Ctrl->Q.active) {
			Q->table[CLOSED]->segment[0]->record = GMT_memory (GMT, QT[CLOSED]->record, QT[CLOSED]->n_rows, char *);
			if (n_qfiles == 2) Q->table[OPEN]->segment[0]->record = GMT_memory (GMT, QT[OPEN]->record, QT[OPEN]->n_rows, char *);
			if (GMT_Write_Data (API, GMT_IS_TEXTSET, GMT_IS_FILE, GMT_IS_POINT, q_mode, NULL, Ctrl->Q.file, Q) != GMT_OK) {
				Return (API->error);
			}
		}
		Return (GMT_OK);
	}

	/* Below here, Ctrl->C.active is FALSE since those cases have already been dealt with.  Also T[OPEN] = T[CLOSED] and we already have found out_seg segments. */

	/* Here we need to do the stitching work.  We already have n_closed polygons in D[GMT_OUT] at this point */

	ns = id;
	if (ns < n_id_alloc) segment = GMT_memory (GMT, segment, ns, struct LINK);
	skip = GMT_memory (GMT, NULL, ns, BOOLEAN);

	GMT_report (GMT, GMT_MSG_NORMAL, "Found %ld closed polygons\n", n_islands);

	/* The algorithm will be confused if there are identical duplicates of segments - thus we check */

	GMT_report (GMT, GMT_MSG_NORMAL, "Check for duplicate lines\n");
	for (iseg = 0; iseg < ns; iseg++) {
		if (skip[iseg]) continue;	/* Skip segment that has been determined to be a duplicate segment */
		for (jseg = iseg + 1; jseg < ns; jseg++) {
			if (skip[jseg]) continue;	/* Skip segment that has been determined to be a duplicate segment */
			if ((segment[iseg].x_end[0] == segment[jseg].x_end[0] && segment[iseg].y_end[0] == segment[jseg].y_end[0]) ||
			    (segment[iseg].x_end[0] == segment[jseg].x_end[1] && segment[iseg].y_end[0] == segment[jseg].y_end[1]) ||
			    (segment[iseg].x_end[1] == segment[jseg].x_end[0] && segment[iseg].y_end[1] == segment[jseg].y_end[0]) ||
			    (segment[iseg].x_end[1] == segment[jseg].x_end[1] && segment[iseg].y_end[1] == segment[jseg].y_end[1])) {
			    	if (segment[iseg].n == segment[jseg].n) {
					for (k = match = 0; k < segment[iseg].n && k == match; k++) {
						match += (D[GMT_IN]->table[segment[iseg].group]->segment[segment[iseg].pos]->coord[GMT_X][k] == D[GMT_IN]->table[segment[jseg].group]->segment[segment[jseg].pos]->coord[GMT_X][k] &&
						          D[GMT_IN]->table[segment[iseg].group]->segment[segment[iseg].pos]->coord[GMT_Y][k] == D[GMT_IN]->table[segment[jseg].group]->segment[segment[jseg].pos]->coord[GMT_Y][k]);
					}
					match = (match == segment[iseg].n) ? 1 : 0;
					if (match) {
						GMT_report (GMT, GMT_MSG_NORMAL, "Segments %ld and %ld are duplicates - Segment %ld will be ignored\n", iseg, jseg, jseg);
						skip[jseg] = TRUE;
					}
				}
			}
		}
	}

	/* Eliminate the duplicate segments from consideration */

	for (iseg = jseg = 0; iseg < ns; iseg++) {
		if (skip[iseg]) continue;
		if (iseg > jseg) segment[jseg] = segment[iseg];
		jseg++;
	}
	if (jseg < ns) GMT_report (GMT, GMT_MSG_NORMAL, "%ld duplicate segment removed\n", ns - jseg);
	ns = jseg;
	GMT_free (GMT, skip);

	GMT_report (GMT, GMT_MSG_NORMAL, "Calculate and rank end point separations [cutoff = %g nn_dist = %g]\n", Ctrl->T.dist[0], Ctrl->T.dist[1]);

	/* We determine the distance from each segments two endpoints to the two endpoints on every other
	 * segment; this is four distances per segment.  We then assign the nearest endpoint to each end
	 * of a segment to the buddy structure which keeps the id of the nearest segment so far.
	 */

	for (iseg = 0; iseg < ns; iseg++) {
		for (jseg = iseg; jseg < ns; jseg++) {
			/* nearest_end indicates which end is closest to this end */
			if (iseg == jseg) {	/* Store offset between the endpoints of a single segment (should be 0 if closed) */
				dd[SEG_I][END_A] = dd[SEG_J][END_B] = DBL_MAX;
				dd[SEG_I][END_B] = dd[SEG_J][END_A] = (segment[iseg].n == 1) ? DBL_MAX : GMT_distance (GMT, segment[iseg].x_end[END_A], segment[iseg].y_end[END_A], segment[iseg].x_end[END_B], segment[iseg].y_end[END_B]);
    				nearest_end[SEG_I][END_A] = nearest_end[SEG_J][END_A] = END_B;
    				nearest_end[SEG_J][END_B] = nearest_end[SEG_I][END_B] = END_A;
			}
			else {	/* Store the distances between the 4 possible end-to-end configurations */
				dd[SEG_I][END_A] = GMT_distance (GMT, segment[iseg].x_end[END_A], segment[iseg].y_end[END_A], segment[jseg].x_end[END_A], segment[jseg].y_end[END_A]);
				dd[SEG_I][END_B] = GMT_distance (GMT, segment[iseg].x_end[END_A], segment[iseg].y_end[END_A], segment[jseg].x_end[END_B], segment[jseg].y_end[END_B]);
				dd[SEG_J][END_A] = GMT_distance (GMT, segment[iseg].x_end[END_B], segment[iseg].y_end[END_B], segment[jseg].x_end[END_A], segment[jseg].y_end[END_A]);
				dd[SEG_J][END_B] = GMT_distance (GMT, segment[iseg].x_end[END_B], segment[iseg].y_end[END_B], segment[jseg].x_end[END_B], segment[jseg].y_end[END_B]);
    				for (end = 0; end < 2; end++) nearest_end[SEG_I][end] = (dd[end][END_A] < dd[end][END_B]) ? END_A : END_B;
    				for (end = 0; end < 2; end++) nearest_end[SEG_J][end] = (dd[END_A][end] < dd[END_B][end]) ? END_A : END_B;
    			}
    			/* Update list of closest matches for both ends */
    			for (ii = 0; ii < 2; ii++) {	/* For each end of the segment */
    				end = nearest_end[SEG_I][ii];	/* The end of segment jseg that was closest to segment iseg's end ii */
    				if (dd[ii][end] < segment[iseg].buddy[ii].dist) {	/* This distance is shorter than the previous shortest distance */
					segment[iseg].buddy[ii].next_dist = segment[iseg].buddy[ii].dist;	/* Previous closest distance */
					segment[iseg].buddy[ii].orig_id = segment[jseg].orig_id;
					segment[iseg].buddy[ii].id = jseg;
					segment[iseg].buddy[ii].dist = dd[ii][end];
					segment[iseg].buddy[ii].end_order = end;
    				}
    				end = nearest_end[SEG_J][ii];	/* The end of segment iseg that was closest to segment jseg's end ii */
    				if (dd[end][ii] < segment[jseg].buddy[ii].dist) {	/* This distance is shorter than the previous shortest distance */
 					segment[jseg].buddy[ii].next_dist = segment[jseg].buddy[ii].dist;	/* Previous closest distance */
					segment[jseg].buddy[ii].orig_id = segment[iseg].orig_id;
 					segment[jseg].buddy[ii].id = iseg;
					segment[jseg].buddy[ii].dist = dd[end][ii];
					segment[jseg].buddy[ii].end_order = end;
    				}
    			}
		}
	}
	if (Ctrl->L.active) {	/* Write out the link information */
		struct GMT_TEXTSET *LNK = NULL;
		char name[GMT_BUFSIZ], name0[GMT_BUFSIZ], name1[GMT_BUFSIZ], fmt[GMT_BUFSIZ], *pp = NULL, *s = GMT->current.setting.io_col_separator;
		if (!Ctrl->L.file) Ctrl->L.file = strdup ("gmtstitch_link.txt");	/* Use default output filename */
		dim_tscr[0] = 1;	dim_tscr[1] = 1;	dim_tscr[2] = ns;
		if ((LNK = GMT_Create_Data (GMT->parent, GMT_IS_TEXTSET, dim_tscr)) == NULL) {
			GMT_report (GMT, GMT_MSG_FATAL, "Unable to create a text set for link lists\n");
			return (GMT->parent->error);
		}
		sprintf (fmt, "%%s%s%%s%s%%c%s%s%s%s%s%%s%s%%c%s%s%s%%d", s, s, s, GMT->current.setting.format_float_out, s, GMT->current.setting.format_float_out, s, s, s, GMT->current.setting.format_float_out, s);

		GMT->current.io.io_header[GMT_OUT] = TRUE;	/* Turn on table headers on output */
		sprintf (buffer, "# segid%sbegin_id%sb_pt%sb_dist%sb_nndist%send_id%se_pt%se_dist%se_nndist", s, s, s, s, s, s, s, s);
		LNK->table[0]->n_headers = 1;
		LNK->table[0]->header = GMT_memory (GMT, NULL, 1, char *);
		LNK->table[0]->header[0] = strdup (buffer);
		for (iseg = 0; iseg < ns; iseg++) {
			G = segment[iseg].group;	L = segment[iseg].pos;
			if (D[GMT_IN]->table[G]->segment[L]->header && (pp = strstr (D[GMT_IN]->table[G]->segment[L]->header, "-L"))) {
				strcpy (name, &pp[2]);
				for (j = 0; name[j]; j++) if (name[j] == ' ') name[j] = '\0';		/* Just truncate after 1st word */
			} else sprintf (name, "%" PRIu64, segment[iseg].orig_id);
			G = segment[segment[iseg].buddy[0].id].group;	L = segment[segment[iseg].buddy[0].id].pos;
			if (D[GMT_IN]->table[G]->segment[L]->header && (pp = strstr (D[GMT_IN]->table[G]->segment[L]->header, "-L"))) {
				strcpy (name0, &pp[2]);
				for (j = 0; name0[j]; j++) if (name0[j] == ' ') name0[j] = '\0';	/* Just truncate after 1st word */
			} else sprintf (name0, "%" PRIu64, segment[iseg].buddy[0].orig_id);
			G = segment[segment[iseg].buddy[1].id].group;	L = segment[segment[iseg].buddy[1].id].pos;
			if (D[GMT_IN]->table[G]->segment[L]->header && (pp = strstr (D[GMT_IN]->table[G]->segment[L]->header, "-L"))) {
				strcpy (name1, &pp[2]);
				for (j = 0; name1[j]; j++) if (name1[j] == ' ') name1[j] = '\0';	/* Just truncate after 1st word */
			} else sprintf (name1, "%" PRIu64, segment[iseg].buddy[1].orig_id);
			sprintf (buffer, fmt, name, name0, BE[segment[iseg].buddy[0].end_order], segment[iseg].buddy[0].dist, segment[iseg].buddy[0].next_dist, name1, \
				BE[segment[iseg].buddy[1].end_order], segment[iseg].buddy[1].dist, segment[iseg].buddy[1].next_dist);
			LNK->table[0]->segment[0]->record[iseg] = strdup (buffer);
		}
		LNK->table[0]->n_records = LNK->table[0]->segment[0]->n_rows = ns;
		if (GMT_Write_Data (API, GMT_IS_TEXTSET, GMT_IS_FILE, GMT_IS_POINT, GMT_WRITE_SET, NULL, Ctrl->L.file, LNK) != GMT_OK) {
			Return (API->error);
		}
		if (GMT_Destroy_Data (API, GMT_ALLOCATED, &LNK) != GMT_OK) {
			Return (API->error);
		}
	}

	start_id = n_closed = 0;
	done = FALSE;
	p_dummy_x = p_dummy_y = DBL_MAX;

	GMT_report (GMT, GMT_MSG_NORMAL, "Assemble new segments\n");

	while (!done) {

		/* Find the 'beginning' of the chain that this segment belongs to by tracing the connections
		 * until we either reappear at the starting point (a closed loop) or we reach an end (i.e.,
		 * the nearest next endpoint is beyond the separation threshold. */

		done = FALSE;
		id = start_id;
		end_order = n_steps = n_alloc_pts = 0;
#ifdef DEBUG2
		GMT_report (GMT, GMT_MSG_NORMAL, "%ld\n", segment[id].orig_id);
#endif
		while (!done && connect (segment, id, end_order, Ctrl->T.dist[0], Ctrl->T.active[1], Ctrl->T.dist[1])) {
			id2 = segment[id].buddy[end_order].id;
#ifdef DEBUG2
			GMT_report (GMT, GMT_MSG_NORMAL, "%ld\n", segment[id2].orig_id);
#endif
			if (id2 == start_id)	/* Closed polygon, start here */
				done = TRUE;
			if (id2 == id || n_steps > ns) {	/* Not good... */
				done = TRUE;
				n_trouble++;
			}
			else {	/* Trace the connection to the next segment */
				end_order = !segment[id].buddy[end_order].end_order;
				id = id2;
				n_alloc_pts += segment[id].n;		/* Update length of combined polygon so far */
			}
			n_steps++;
		}
		n_alloc_pts = MAX (GMT_CHUNK, n_alloc_pts);	/* Allocate enough space to get us started */

		/* This id should be the beginning of a segment.  Now trace forward and dump out the chain */

		T[CLOSED][out_seg] = GMT_memory (GMT, NULL, 1, struct GMT_LINE_SEGMENT);	/* Get a new segment structure */
		GMT_alloc_segment (GMT, T[OPEN][out_seg], n_alloc_pts, n_columns, TRUE);

		sprintf (buffer, "Possibly a composite segment; see comments for individual segment headers");
		T[OPEN][out_seg]->header = strdup (buffer);
		/* First dump start segment */

		start_id = id;

		GMT_memset (GMT->current.io.segment_header, GMT_BUFSIZ, char);
		if (Ctrl->D.active) {	/* Prepare and set segment output file name */
			d_mode = OPEN;
			(save_type) ? sprintf (buffer, Ctrl->D.format, 'O', out_seg) : sprintf (buffer, Ctrl->D.format, out_seg);
			T[OPEN][out_seg]->file[GMT_OUT] = strdup (buffer);
		}

		p_first_x = p_last_x = p_dummy_x;
		p_first_y = p_last_y = p_dummy_y;
		k = out_p = n_seg_length = 0;
		done = FALSE;
		first = TRUE;
		do {
			G = segment[id].group;
			L = segment[id].pos;
			np = segment[id].n;
			if (end_order == 0) {	/* Already in the right order */
				if (D[GMT_IN]->table[G]->segment[L]->coord[GMT_X][0] == p_last_x && D[GMT_IN]->table[G]->segment[L]->coord[GMT_Y][0] == p_last_y) {	/* Skip duplicate anchor point */
					j = 1;
					n = np - 1;
				}
				else {	/* We need all the points */
					j = 0;
					n = np;
				}
				n_seg_length += n;
				GMT_report (GMT, GMT_MSG_DEBUG, "Forward Segment no %ld-%ld ", G, L);
				if (n_seg_length > n_alloc_pts) GMT_alloc_segment (GMT, T[OPEN][out_seg], n_alloc_pts << 1, n_columns, FALSE);
				out_p = Copy_This_Segment (D[GMT_IN]->table[G]->segment[L], T[OPEN][out_seg], out_p, j, np-1);
				p_last_x = D[GMT_IN]->table[G]->segment[L]->coord[GMT_X][np-1];
				p_last_y = D[GMT_IN]->table[G]->segment[L]->coord[GMT_Y][np-1];
				if (first) p_first_x = D[GMT_IN]->table[G]->segment[L]->coord[GMT_X][0], p_first_y = D[GMT_IN]->table[G]->segment[L]->coord[GMT_Y][0];
			}
			else {	/* Must reverse the segment's order of points */
				if (D[GMT_IN]->table[G]->segment[L]->coord[GMT_X][np-1] == p_last_x && D[GMT_IN]->table[G]->segment[L]->coord[GMT_Y][np-1] == p_last_y) {	/* Skip duplicate anchor point */
					j = 1;
					n = np - 1;
				}
				else {	/* We need all the points */
					j = 0;
					n = np;
				}
				n_seg_length += n;
				GMT_report (GMT, GMT_MSG_DEBUG, "Reverse Segment %ld-%ld ", G, L);
				if (n_seg_length > n_alloc_pts) GMT_alloc_segment (GMT, T[OPEN][out_seg], n_alloc_pts << 1, n_columns, FALSE);
				out_p = Copy_This_Segment (D[GMT_IN]->table[G]->segment[L], T[OPEN][out_seg], out_p, np-1-j, 0);
				p_last_x = D[GMT_IN]->table[G]->segment[L]->coord[GMT_X][0];
				p_last_y = D[GMT_IN]->table[G]->segment[L]->coord[GMT_Y][0];
				if (first) p_first_x = D[GMT_IN]->table[G]->segment[L]->coord[GMT_X][np-1], p_first_y = D[GMT_IN]->table[G]->segment[L]->coord[GMT_Y][np-1];
			}
			first = FALSE;
			end_order = !end_order;
			segment[id].used = TRUE;
			if (segment[id].buddy[end_order].dist <= Ctrl->T.dist[0] && !segment[segment[id].buddy[end_order].id].used) {
				/* Not done, trace into the next connecting segment */
				id2 = segment[id].buddy[end_order].id;
				end_order = segment[id].buddy[end_order].end_order;
				done = (id2 == start_id || id2 == id);
				id = id2;
			}
			else	/* End of the D[GMT_IN]->table for this segment */
				done = TRUE;
			k++;
		} while (!done);
		GMT_report (GMT, GMT_MSG_DEBUG, "\n");
		GMT_report (GMT, GMT_MSG_NORMAL, "Segment %ld made from %ld pieces\n", out_seg, k);
		if (n_seg_length < n_alloc_pts) GMT_alloc_segment (GMT, T[OPEN][out_seg], n_seg_length, n_columns, FALSE);

		if (p_first_x == p_last_x && p_first_y == p_last_y) {
			if (Ctrl->D.active && save_type) {	/* Ended up closed, rename with the C type */
				sprintf (buffer, Ctrl->D.format, 'C', out_seg);
				free (T[OPEN][out_seg]->file[GMT_OUT]);
				T[OPEN][out_seg]->file[GMT_OUT] = strdup (buffer);
				d_mode = CLOSED;	/* Mode is used with -Q only */
			}
			n_closed++;
		}
		if (Ctrl->Q.active) {
			QT[d_mode]->record[QT[d_mode]->n_rows++] = strdup (buffer);
			if (QT[d_mode]->n_rows == QT[d_mode]->n_alloc) QT[d_mode]->record = GMT_memory (GMT, QT[d_mode]->record, (QT[d_mode]->n_alloc <<= 2), char *);
		}

		chain++;
		out_seg++;

		/* Wind to the next unused segments to start the connection search again */
		start_id = 0;
		while (start_id < ns && segment[start_id].used) start_id++;
		done = (start_id == ns);	/* No more unused segments */
	}

	if (out_seg > 1) GMT_set_segmentheader (GMT, GMT_OUT, TRUE);	/* Turn on segment headers on output */
	if (Ctrl->Q.active) {	/* Write out the list(s) with individual file names */
		Q->table[CLOSED]->segment[0]->record = GMT_memory (GMT, QT[CLOSED]->record, QT[CLOSED]->n_rows, char *);
		if (n_qfiles == 2) Q->table[OPEN]->segment[0]->record = GMT_memory (GMT, QT[OPEN]->record, QT[OPEN]->n_rows, char *);
		if (GMT_Write_Data (API, GMT_IS_TEXTSET, GMT_IS_FILE, GMT_IS_POINT, q_mode, NULL, Ctrl->Q.file, Q) != GMT_OK) {
			Return (API->error);
		}
	}

	/* Write out the new multisegment file with polygons and segments */

	D[GMT_OUT]->table[0]->segment = GMT_memory (GMT, T[OPEN], out_seg, struct GMT_LINE_SEGMENT *);
	D[GMT_OUT]->n_segments = D[GMT_OUT]->table[0]->n_segments = out_seg;
	ofile = (Ctrl->D.active) ? Ctrl->D.format : Ctrl->Out.file;
	if (GMT_Write_Data (API, GMT_IS_DATASET, GMT_IS_FILE, GMT_IS_POINT, io_mode, NULL, ofile, D[GMT_OUT]) != GMT_OK) {
		Return (API->error);
	}

	GMT_report (GMT, GMT_MSG_NORMAL, "Segments in: %ld Segments out: %ld\n", ns + n_islands, chain + n_islands);
	if (n_trouble) GMT_report (GMT, GMT_MSG_NORMAL, "%ld trouble spots\n", n_trouble);
	if (n_closed) GMT_report (GMT, GMT_MSG_NORMAL, "%ld new closed segments\n", n_closed);
	if (n_islands) GMT_report (GMT, GMT_MSG_NORMAL, "%ld were already closed\n", n_islands);

	Return (GMT_OK);
}
