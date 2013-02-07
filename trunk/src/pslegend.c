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
 * Brief synopsis: pslegend will make map legends from input that specifies what will go
 * into the legend, such as headers, symbols with explanatory text,
 * paragraph text, and empty space and horizontal/vertical lines.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5 API
 */

#define THIS_MODULE k_mod_pslegend /* I am pslegend */

#include "pslib.h"
#include "gmt.h"

#define FRAME_CLEARANCE	4.0	/* In points */
#define FRAME_GAP	2.0	/* In points */
#define FRAME_RADIUS	6.0	/* In points */

struct PSLEGEND_CTRL {
	struct C {	/* -C<dx>/<dy> */
		bool active;
		double dx, dy;
	} C;
	struct D {	/* -D[x]<x0>/<y0>/w/h/just[/xoff/yoff] */
		bool active;
		bool cartesian;
		double lon, lat, width, height, dx, dy;
		char justify[3];
	} D;
	struct F {	/* -F+r[<radius>][+p<pen>][+i[<off>/][<pen>]][+s[<dx>/<dy>/][<fill>]] */
		bool active;
		unsigned int mode;		/* 0 = rectangular, 1 = rounded, 2 = secondary frame, 4 = shade */
		double radius;			/* Radius for rounded corner */
		double dx, dy;			/* Offset for background shaded rectangle (+s) */
		double gap;			/* Space bewteen main and secondary frame */
		struct GMT_PEN pen1, pen2;	/* Pen for main and secondary frame outline */
		struct GMT_FILL fill;		/* Background shade */
	} F;
	struct G {	/* -G<fill> */
		bool active;
		struct GMT_FILL fill;
	} G;
	struct L {	/* -L<spacing> */
		bool active;
		double spacing;
	} L;
};

void *New_pslegend_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct PSLEGEND_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct PSLEGEND_CTRL);

	/* Initialize values whose defaults are not 0/false/NULL */

	C->C.dx = C->C.dy = GMT->session.u2u[GMT_PT][GMT_INCH] * FRAME_CLEARANCE;	/* 4 pt */
	C->D.width = C->D.height = 1.0;
	C->F.radius = GMT->session.u2u[GMT_PT][GMT_INCH] * FRAME_RADIUS;		/* 6 pt */
	GMT_init_fill (GMT, &C->G.fill, -1.0, -1.0, -1.0);		/* Default is no fill */
	C->F.pen1 = GMT->current.setting.map_frame_pen;
	C->F.pen2 = GMT->current.setting.map_default_pen;
	C->F.gap = GMT->session.u2u[GMT_PT][GMT_INCH] * FRAME_GAP;	/* Default is 2p */
	C->F.dx = C->C.dx;	C->F.dy = -C->C.dy;			/* Default is (4p, -4p) */
	GMT_init_fill (GMT, &C->F.fill, 0.5, 0.5, 0.5);			/* Default is gray shade if used */
	C->L.spacing = 1.1;
	return (C);
}

void Free_pslegend_Ctrl (struct GMT_CTRL *GMT, struct PSLEGEND_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	GMT_free (GMT, C);
}

int GMT_pslegend_usage (struct GMTAPI_CTRL *C, int level)
{
	struct GMT_CTRL *GMT = C->GMT;

	/* This displays the pslegend synopsis and optionally full usage information */

	gmt_module_show_name_and_purpose (THIS_MODULE);
	GMT_message (GMT, "usage: pslegend [<infofile>] -D[x]<x0>/<y0>/<w>[/<h>]/<just>[/<dx>/<dy>] [%s]\n", GMT_B_OPT);
	GMT_message (GMT, "\t[-C<dx>/<dy>] [-F[+i[[<gap>/]<pen>]][+p<pen>][+r[<radius>]][+s[<dx>/<dy>/][<fill>]]\n");
	GMT_message (GMT, "\t[-G<fill>] [%s] [-K] [-L<spacing>] [-O] [-P] [%s]\n", GMT_J_OPT, GMT_Rgeo_OPT);
	GMT_message (GMT, "\t[%s] [%s] [%s] [%s]\n\t[%s] [%s]\n\t[%s]\n\n", GMT_U_OPT, GMT_V_OPT, GMT_X_OPT, GMT_Y_OPT, GMT_c_OPT, GMT_p_OPT, GMT_t_OPT);
	GMT_message (GMT, "\tReads legend layout information from <infofile> [or stdin].\n");
	GMT_message (GMT, "\t(See manual page for more information and <infofile> format).\n");

	if (level == GMTAPI_SYNOPSIS) return (EXIT_FAILURE);

	GMT_message (GMT, "\t-D Set position and size of legend box.  Prepend x if coordinates are projected;\n");
	GMT_message (GMT, "\t   if so the -R -J options only required if -O is not given.  Append the justification\n");
	GMT_message (GMT, "\t   of the whole legend box using pstext justification codes.  Optionally, append offsets\n");
	GMT_message (GMT, "\t   to shift the box from the selected point in the direction implied by <just>.\n");
	GMT_message (GMT, "\t   If legend box height <h> is 0 or not specified then we estimate it from <infofile>.\n");
	GMT_message (GMT, "\n\tOPTIONS:\n");
	GMT_message (GMT, "\t<infofile> is one or more ASCII information files with legend commands.\n");
	GMT_message (GMT, "\t   If no files are given, standard input is read.\n");
	GMT_explain_options (GMT, "b");
	GMT_message (GMT, "\t-C Set the clearance between legend frame and internal items [%gp/%gp].\n", FRAME_CLEARANCE, FRAME_CLEARANCE);
	GMT_message (GMT, "\t-F Draw rectangular border around the legend (using MAP_FRAME_PEN) [Default is no border].\n");
	GMT_message (GMT, "\t   Append +i[[<gap>/]<pen>] to add a secondary inner frame boundary [Default gap is %gp].\n", FRAME_GAP);
	GMT_message (GMT, "\t   Append +p<pen> to change the border pen [%s].\n", GMT_putpen (GMT, GMT->current.setting.map_frame_pen));
	GMT_message (GMT, "\t   Append +r[<radius>] to plot rounded rectangles instead [Default radius is %gp].\n", FRAME_RADIUS);
	GMT_message (GMT, "\t   Append +s[<dx>/<dy>/]<fill> to plot a shadow behind the legend box [Default offset is %gp/%g].\n", FRAME_CLEARANCE, -FRAME_CLEARANCE);
	GMT_fill_syntax (GMT, 'G', "Set the fill for the legend box [Default is no fill].");
	GMT_explain_options (GMT, "jK");
	GMT_message (GMT, "\t-L Set the linespacing factor in units of the current annotation font size [1.1].\n");
	GMT_explain_options (GMT, "OPR");
	GMT_explain_options (GMT, "UVXpt.");

	return (EXIT_FAILURE);
}

int GMT_pslegend_parse (struct GMTAPI_CTRL *C, struct PSLEGEND_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to pslegend and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int k, n_errors = 0, pos;
	unsigned int n;
	char txt_a[GMT_TEXT_LEN256], txt_b[GMT_TEXT_LEN256], txt_c[GMT_TEXT_LEN256], txt_d[GMT_TEXT_LEN256], txt_e[GMT_TEXT_LEN256], txt_f[GMT_TEXT_LEN256], p[GMT_BUFSIZ];
	struct GMT_OPTION *opt = NULL;
	struct GMT_CTRL *GMT = C->GMT;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {

			case '<':	/* Input files */
				break;

			/* Processes program-specific parameters */

			case 'C':	/* Sets the clearance between frame and internal items */
				Ctrl->C.active = true;
				sscanf (opt->arg, "%[^/]/%s", txt_a, txt_b);
				Ctrl->C.dx = GMT_to_inch (GMT, txt_a);
				Ctrl->C.dy = GMT_to_inch (GMT, txt_b);
				break;
			case 'D':	/* Sets position and size of legend */
				Ctrl->D.active = true;
				if (opt->arg[0] == 'x') {	/* Gave location directly in projected units (inches, cm, etc) */
					Ctrl->D.cartesian = true;
					k = 1;
				}
				else				/* Gave lon, lat */
					k = 0;
				n = sscanf (&opt->arg[k], "%[^/]/%[^/]/%[^/]/%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d, Ctrl->D.justify, txt_e, txt_f);
				n_errors += GMT_check_condition (GMT, n < 4 || n > 7, "Error: Syntax is -D[x]<xpos>/<ypos>/<width>[/<height>]/<justify>[<dx>/<dy>]\n");
				if (opt->arg[0] == 'x') {
					Ctrl->D.lon = GMT_to_inch (GMT, txt_a);
					Ctrl->D.lat = GMT_to_inch (GMT, txt_b);
				}
				else {	/* Given in user units, likely degrees */
					n_errors += GMT_verify_expectations (GMT, GMT->current.io.col_type[GMT_IN][GMT_X], GMT_scanf (GMT, txt_a, GMT->current.io.col_type[GMT_IN][GMT_X], &Ctrl->D.lon), txt_a);
					n_errors += GMT_verify_expectations (GMT, GMT->current.io.col_type[GMT_IN][GMT_Y], GMT_scanf (GMT, txt_b, GMT->current.io.col_type[GMT_IN][GMT_Y], &Ctrl->D.lat), txt_b);
				}
				Ctrl->D.width   = GMT_to_inch (GMT, txt_c);
				if (n == 4 || n == 6) {	/* Did not give height, so shuffle the following 3 items */
					Ctrl->D.height  = 0.0;
					strncpy (txt_f, txt_e, GMT_TEXT_LEN256);
					strncpy (txt_e, Ctrl->D.justify, GMT_TEXT_LEN256);
					strncpy (Ctrl->D.justify, txt_d, 3U);
				}
				else
					Ctrl->D.height  = GMT_to_inch (GMT, txt_d);
				if (n > 5) {	/* Got the optional offsets */
					Ctrl->D.dx = GMT_to_inch (GMT, txt_e);
					Ctrl->D.dy = GMT_to_inch (GMT, txt_f);
				}
				break;
			case 'F':
				Ctrl->F.active = true;
				pos = 0;
				while (GMT_getmodopt (GMT, opt->arg, "iprs", &pos, p)) {	/* Looking for +i, +p, +r, +s */
					switch (p[0]) {
						case 'i':	/* Secondary pen info */
							Ctrl->F.mode |= 1;
							if (p[1]) {	/* Gave 1-2 attributes */
								n = sscanf (&p[1], "%[^/]/%s", txt_a, txt_b);
								if (n == 2) {	/* Got both gap and pen */
									Ctrl->F.gap = GMT_to_inch (GMT, txt_a);
									if (GMT_getpen (GMT, txt_b, &Ctrl->F.pen2)) n_errors++;
								}
								else	/* Only got pen; use dfault gap */
									if (GMT_getpen (GMT, txt_a, &Ctrl->F.pen2)) n_errors++;
							}
							break;
						case 'p':	/* Change primary pen info */
							if (!p[1] || GMT_getpen (GMT, &p[1], &Ctrl->F.pen1)) n_errors++;
							break;
						case 'r':	/* corner radius */
							if (p[1]) Ctrl->F.radius = GMT_to_inch (GMT, &p[1]);
							Ctrl->F.mode |= 2;
							break;
						case 's':	/* Get shade settings */
							if (p[1]) {
								n = sscanf (&p[1], "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
								if (n == 3) {
									Ctrl->F.dx = GMT_to_inch (GMT, txt_a);
									Ctrl->F.dy = GMT_to_inch (GMT, txt_b);
									if (GMT_getfill (GMT, txt_c, &Ctrl->F.fill)) n_errors++;
								}
								else if (n == 1) {
									if (GMT_getfill (GMT, txt_a, &Ctrl->F.fill)) n_errors++;
								}
								else n_errors++;
							}
							Ctrl->F.mode |= 4;
							break;
						default:
							n_errors++;
							break;
					}
				}
				break;
			case 'G':	/* Inside legend box fill */
				Ctrl->G.active = true;
				if (GMT_getfill (GMT, opt->arg, &Ctrl->G.fill)) {	/* We check syntax here */
					GMT_fill_syntax (GMT, 'G', " ");
					n_errors++;
				}
				break;
			case 'L':			/* Sets linespacing in units of fontsize [1.1] */
				Ctrl->L.active = true;
				Ctrl->L.spacing = atof (opt->arg);
				break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	/* Check that the options selected are mutually consistent */

	n_errors += GMT_check_condition (GMT, Ctrl->C.dx < 0.0 || Ctrl->C.dy < 0.0, "Syntax error -C option: clearances cannot be negative!\n");
	n_errors += GMT_check_condition (GMT, Ctrl->D.width < 0.0 || Ctrl->D.height < 0.0, "Syntax error -D option: legend box sizes cannot be negative!\n");
	if (!Ctrl->D.cartesian || !GMT->common.O.active) {	/* Overlays with -Dx does not need -R -J; other cases do */
		n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error: Must specify -R option\n");
		n_errors += GMT_check_condition (GMT, !GMT->common.J.active, "Syntax error: Must specify a map projection with the -J option\n");
	}

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_pslegend_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

#ifdef DEBUG
/* Used to draw the current y-line for debug purposes only.  To use you would also
 * have to set the variable guide to 1 below. */
void drawbase (struct GMT_CTRL *C, struct PSL_CTRL *P, double x0, double x1, double y0)
{
	struct GMT_PEN faint_pen;
	GMT_init_pen (C, &faint_pen, 0.0);
	GMT_setpen (C, &faint_pen);
	PSL_plotsegment (P, x0, y0, x1, y0);
}
#endif

/* Define the fraction of the height of the font to the font size */
#define FONT_HEIGHT_PRIMARY (GMT->session.font[GMT->current.setting.font_annot[0].id].height)
#define FONT_HEIGHT(font_id) (GMT->session.font[font_id].height)
#define FONT_HEIGHT_LABEL (GMT->session.font[GMT->current.setting.font_label.id].height)

#define SYM 0
#define TXT 1
#define PAR 2
#define N_CMD 3

int GMT_pslegend (void *V_API, int mode, void *args)
{	/* High-level function that implements the pslegend task */
	unsigned int tbl;
	int i, k, n = 0, justify = 0, n_columns = 1, error = 0, column_number = 0, id, n_scan;
	int status = 0, object_ID;
	bool flush_paragraph = false, draw_vertical_line = false, gave_label, gave_mapscale_options, did_old = false;
	uint64_t seg, row, dim[4] = {1, 1, 0, 2};
	size_t n_char = 0;
	 
	char txt_a[GMT_TEXT_LEN256], txt_b[GMT_TEXT_LEN256], txt_c[GMT_TEXT_LEN256], txt_d[GMT_TEXT_LEN256], txt_e[GMT_TEXT_LEN256];
	char txt_f[GMT_TEXT_LEN256], key[GMT_TEXT_LEN256], sub[GMT_TEXT_LEN256], tmp[GMT_TEXT_LEN256], just;
	char symbol[GMT_TEXT_LEN256], text[GMT_BUFSIZ], image[GMT_BUFSIZ], xx[GMT_TEXT_LEN256], yy[GMT_TEXT_LEN256];
	char size[GMT_TEXT_LEN256], angle[GMT_TEXT_LEN256], mapscale[GMT_TEXT_LEN256], font[GMT_TEXT_LEN256], lspace[GMT_TEXT_LEN256];
	char tw[GMT_TEXT_LEN256], jj[GMT_TEXT_LEN256], sarg[GMT_TEXT_LEN256], txtcolor[GMT_TEXT_LEN256] = {""}, buffer[GMT_BUFSIZ];
	char bar_cpt[GMT_TEXT_LEN256], bar_gap[GMT_TEXT_LEN256], bar_height[GMT_TEXT_LEN256], bar_opts[GMT_BUFSIZ], *opt = NULL;
	char *line = NULL, string[GMTAPI_STRLEN];
#ifdef GMT_COMPAT
	char save_EOF;
#endif
#ifdef DEBUG
	int guide = 0;
#endif

	unsigned char *dummy = NULL;

	double x_orig, y_orig, x_off, x, y, x0, y0, L, off_ss, off_tt, V = 0.0, sdim[3] = {0.0, 0.0, 0.0};
	double half_line_spacing, quarter_line_spacing, one_line_spacing, y_start = 0.0, d_off, height;

	struct imageinfo header;
	struct PSLEGEND_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct PSL_CTRL *PSL = NULL;		/* General PSL interal parameters */
	struct GMT_OPTION *r_ptr = NULL, *j_ptr = NULL;
	struct GMT_FONT ifont;
	struct GMT_FILL current_fill;
	struct GMT_PEN current_pen;
	struct GMT_TEXTSET *In = NULL, *D[N_CMD] = {NULL, NULL, NULL};
	struct GMT_DATASET *Front = NULL;
	struct GMT_TEXT_SEGMENT *S[N_CMD] = {NULL, NULL, NULL};
	struct GMT_LINE_SEGMENT *F = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_Report_Error (API, GMT_NOT_A_SESSION));
	options = GMT_Prep_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMTAPI_OPT_USAGE) bailout (GMT_pslegend_usage (API, GMTAPI_USAGE));	/* Return the usage message */
	if (options->option == GMTAPI_OPT_SYNOPSIS) bailout (GMT_pslegend_usage (API, GMTAPI_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments; return if errors are encountered */

	GMT = GMT_begin_gmt_module (API, THIS_MODULE, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, "-VJR", "BKOPUXYcpt>", options)) Return (API->error);
	Ctrl = New_pslegend_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_pslegend_parse (API, Ctrl, options))) Return (error);
	PSL = GMT->PSL;		/* This module also needs PSL */

	/*---------------------------- This is the pslegend main code ----------------------------*/

#ifdef GMT_COMPAT
	/* Since pslegend v4 used '>' to indicate a paragraph record we avoid confusion with multiple segmentheaders by *
	 * temporarily setting # as segment header flag since all headers are skipped anyway */
	save_EOF = GMT->current.setting.io_seg_marker[GMT_IN];
	GMT->current.setting.io_seg_marker[GMT_IN] = '#';
#endif

	if (GMT_Init_IO (API, GMT_IS_TEXTSET, GMT_IS_TEXT, GMT_IN, GMT_REG_DEFAULT, 0, options) != GMT_OK) {	/* Register data input */
		Return (API->error);
	}
	if ((In = GMT_Read_Data (API, GMT_IS_TEXTSET, GMT_IS_FILE, GMT_IS_ANY, GMT_READ_NORMAL, NULL, NULL, NULL)) == NULL) {
		Return (API->error);
	}

	/* First attempt to compute the legend height */

	one_line_spacing = Ctrl->L.spacing * GMT->current.setting.font_annot[0].size / PSL_POINTS_PER_INCH;
	half_line_spacing    = 0.5  * one_line_spacing;
	quarter_line_spacing = 0.25 * one_line_spacing;

	height = 2.0 * Ctrl->C.dy;
	for (tbl = 0; tbl < In->n_tables; tbl++) {	/* We only expect one table but who knows what the user does */
		for (seg = 0; seg < In->table[tbl]->n_segments; seg++) {	/* We only expect one segment in each table but again... */
			for (row = 0; row < In->table[tbl]->segment[seg]->n_rows; row++) {	/* Finally processing the rows */
				line = In->table[tbl]->segment[seg]->record[row];
				if (line[0] == '#') continue;	/* Skip all headers */

				/* Data record to process */

				if (line[0] != 'T' && flush_paragraph) {	/* Flush contents of pending paragraph [Call GMT_pstext] */
					flush_paragraph = false;
					column_number = 0;
				}

				switch (line[0]) {
					case 'B':	/* Color scale Bar [Use GMT_psscale] */
						sscanf (&line[2], "%*s %*s %s", bar_height);
						height += GMT_to_inch (GMT, bar_height) + GMT->current.setting.map_tick_length[0] + GMT->current.setting.map_annot_offset[0] + FONT_HEIGHT_PRIMARY * GMT->current.setting.font_annot[0].size / PSL_POINTS_PER_INCH;
						column_number = 0;
						break;

					case 'C':	/* Color change */
						break;

					case 'D':	/* Delimiter record */
						height += half_line_spacing;
						column_number = 0;
						break;

					case 'G':	/* Gap record */
						sscanf (&line[2], "%s", txt_a);
						height += (txt_a[strlen(txt_a)-1] == 'l') ? atoi (txt_a) * one_line_spacing : GMT_to_inch (GMT, txt_a);
						column_number = 0;
						break;

					case 'H':	/* Header record */
						sscanf (&line[2], "%s %s %[^\n]", size, font, text);
						if (size[0] == '-') size[0] = 0;
						if (font[0] == '-') font[0] = 0;
						sprintf (tmp, "%s,%s,%s", size, font, txtcolor);		/* Put size, font and color together for parsing by GMT_getfont */
						ifont = GMT->current.setting.font_title;	/* Set default font */
						GMT_getfont (GMT, tmp, &ifont);
						height += Ctrl->L.spacing * ifont.size / PSL_POINTS_PER_INCH;
						column_number = 0;
						break;

					case 'I':	/* Image record [use GMT_psimage] */
						sscanf (&line[2], "%s %s %s", image, size, key);
						PSL_loadimage (PSL, image, &header, &dummy);
						height += GMT_to_inch (GMT, size) * (double)header.height / (double)header.width;
						PSL_free (dummy);
						column_number = 0;
						break;

					case 'L':	/* Label record */
						sscanf (&line[2], "%s %s %s %[^\n]", size, font, key, text);
						if (size[0] == '-') size[0] = 0;
						if (font[0] == '-') font[0] = 0;
						sprintf (tmp, "%s,%s,%s", size, font, txtcolor);		/* Put size, font and color together for parsing by GMT_getfont */
						ifont = GMT->current.setting.font_label;	/* Set default font */
						GMT_getfont (GMT, tmp, &ifont);
						if (column_number%n_columns == 0) height += Ctrl->L.spacing * ifont.size / PSL_POINTS_PER_INCH;
						column_number++;
						break;

					case 'M':	/* Map scale record M lon0|- lat0 length[n|m|k][+opts] f|p  [-R -J] */
						n_scan = sscanf (&line[2], "%s %s %s %s %s %s", txt_a, txt_b, txt_c, txt_d, txt_e, txt_f);
						k = (txt_d[0] != 'f') ? 1 : 0;	/* Determines if we start -L with f or not */
						for (i = 0, gave_mapscale_options = false; txt_c[i] && !gave_mapscale_options; i++) if (txt_c[i] == '+') gave_mapscale_options = true;
						/* Default assumes label is added on top */
						just = 't';
						gave_label = true;
						d_off = FONT_HEIGHT_LABEL * GMT->current.setting.font_label.size / PSL_POINTS_PER_INCH + fabs(GMT->current.setting.map_label_offset);

						if ((opt = strchr (txt_c, '+'))) {	/* Specified alternate label (could be upper case, hence 0.85) and justification */
							char txt_cpy[GMT_BUFSIZ], p[GMT_TEXT_LEN256];
							unsigned int pos = 0;
							strncpy (txt_cpy, opt, GMT_BUFSIZ);
							while ((GMT_strtok (txt_cpy, "+", &pos, p))) {
								switch (p[0]) {
									case 'u':	/* Label put behind annotation */
										gave_label = false;
										break;
									case 'j':	/* Justification */
										just = p[1];
										break;
									default:	/* Just ignore */
										break;
								}
							}
						}
						if (gave_label && (just == 't' || just == 'b')) height += d_off;
						height += GMT->current.setting.map_scale_height + FONT_HEIGHT_PRIMARY * GMT->current.setting.font_annot[0].size / PSL_POINTS_PER_INCH + GMT->current.setting.map_annot_offset[0];
						column_number = 0;
						break;

					case 'N':	/* n_columns record */
						sscanf (&line[2], "%s", txt_a);
						n_columns = atoi (txt_a);
						column_number = 0;
						break;

#ifdef GMT_COMPAT
					case '>':	/* Paragraph text header */
						GMT_report (GMT, GMT_MSG_COMPAT, "Warning: paragraph text header flag > is deprecated; use P instead\n");
#endif
					case 'P':	/* Paragraph text header */
						flush_paragraph = true;
						column_number = 0;
						break;

					case 'S':	/* Symbol record */
						if (column_number%n_columns == 0) height += one_line_spacing;
						column_number++;
						break;

					case 'T':	/* paragraph text record */
						n_char += strlen (line) - 2;
						flush_paragraph = true;
						column_number = 0;
						break;

					case 'V':	/* Vertical line from here to next V */
						column_number = 0;
						break;

					default:
						GMT_report (GMT, GMT_MSG_NORMAL, "Error: Unrecognized record (%s)\n", line);
						Return (GMT_RUNTIME_ERROR);
					break;
				}
			}
		}
	}

	if (n_char) {	/* Typesetting paragraphs, make a guesstimate of number of typeset lines */
		int n_lines;
		double average_char_width = 0.44;	/* There is no such thing but this is just a 1st order estimate */
		double x_lines;
		/* Guess: Given legend width and approximate char width, do the simple expression */
		x_lines = n_char * (average_char_width * GMT->current.setting.font_annot[0].size / PSL_POINTS_PER_INCH) / ((Ctrl->D.width - 2 * Ctrl->C.dx));
		n_lines = lrint (ceil (x_lines));
		height += n_lines * Ctrl->L.spacing * GMT->current.setting.font_annot[0].size / PSL_POINTS_PER_INCH;
		GMT_report (GMT, GMT_MSG_DEBUG, "Estimating %d lines of typeset paragraph text [%.1f].\n", n_lines, x_lines);
	}

	if (Ctrl->D.height == 0.0) {	/* Use the computed height */
		Ctrl->D.height = height;
		GMT_report (GMT, GMT_MSG_VERBOSE, "No legend height given, use an estimated height of %g inches.\n", height);
	}
	else
		GMT_report (GMT, GMT_MSG_VERBOSE, "Legend height given as %g inches; estimated height is %g inches.\n", Ctrl->D.height, height);
	
	if (!(GMT->common.R.active && GMT->common.J.active)) {	/* When no projection specified (i.e, -Dx is used), use fake linear projection -Jx1i */
		double wesn[4];
		GMT_memset (wesn, 4, double);
		GMT->common.R.active = true;
		GMT->common.J.active = false;
		GMT_parse_common_options (GMT, "J", 'J', "x1i");
		wesn[XHI] = Ctrl->D.width;	wesn[YHI] = Ctrl->D.height;
		GMT_err_fail (GMT, GMT_map_setup (GMT, wesn), "");
	}
	else {
		r_ptr = GMT_Find_Option (API, 'R', options);
		j_ptr = GMT_Find_Option (API, 'J', options);
		if (GMT_err_pass (GMT, GMT_map_setup (GMT, GMT->common.R.wesn), "")) Return (GMT_RUNTIME_ERROR);
	}
	GMT_plotinit (GMT, options);
	GMT_plane_perspective (GMT, GMT->current.proj.z_project.view_plane, GMT->current.proj.z_level);

	/* Must reset any -X -Y to 0 so they are not used further in the GMT_modules we call below */
	GMT_memset (GMT->current.setting.map_origin, 2, double);

	justify = GMT_just_decode (GMT, Ctrl->D.justify, 12);

	if (!Ctrl->D.cartesian) {	/* Convert to map coordinates first */
		GMT_geo_to_xy (GMT, Ctrl->D.lon, Ctrl->D.lat, &x, &y);
		Ctrl->D.lon = x;	Ctrl->D.lat = y;
	}

	/* Allow for justification so that D.lon/D.lat is the plot location of the lower left corner of box */

	Ctrl->D.lon -= 0.5 * ((justify-1)%4) * Ctrl->D.width;
	Ctrl->D.lat -= 0.5 * (justify/4) * Ctrl->D.height;

	/* Also deal with any justified offsets if given */
	
	Ctrl->D.lon -= ((justify%4)-2) * Ctrl->D.dx;
	Ctrl->D.lat -= ((justify/4)-1) * Ctrl->D.dy;
	
	/* Set new origin */
	
	PSL_setorigin (PSL, Ctrl->D.lon, Ctrl->D.lat, 0.0, PSL_FWD);
	x_orig = Ctrl->D.lon;	y_orig = Ctrl->D.lat;
	Ctrl->D.lon = Ctrl->D.lat = 0.0;	/* For now */
	
	/* First draw legend frame box. */

	current_pen = GMT->current.setting.map_default_pen;
	current_fill = Ctrl->G.fill;

	if (Ctrl->F.active) {	/* First draw legend frame box */
		sdim[0] = Ctrl->D.width;
		sdim[1] = Ctrl->D.height;
		sdim[2] = Ctrl->F.radius;
		if (Ctrl->F.mode & 4) {	/* Draw offset background shade */
			GMT_setfill (GMT, &Ctrl->F.fill, false);
			PSL_plotsymbol (PSL, Ctrl->D.lon + 0.5 * Ctrl->D.width + Ctrl->F.dx, Ctrl->D.lat + 0.5 * Ctrl->D.height + Ctrl->F.dy, sdim, (Ctrl->F.mode & 2) ? PSL_RNDRECT : PSL_RECT);
		}
		GMT_setpen (GMT, &Ctrl->F.pen1);	/* Always draw frame outline, with or without fill */
		GMT_setfill (GMT, (Ctrl->G.active) ? &current_fill : NULL, true);
		PSL_plotsymbol (PSL, Ctrl->D.lon + 0.5 * Ctrl->D.width, Ctrl->D.lat + 0.5 * Ctrl->D.height, sdim, (Ctrl->F.mode & 2) ? PSL_RNDRECT : PSL_RECT);
		if (Ctrl->F.mode & 1) {	/* Also draw secondary frame on the inside */
			sdim[0] = Ctrl->D.width - 2.0 * Ctrl->F.gap;
			sdim[1] = Ctrl->D.height- 2.0 * Ctrl->F.gap;
			GMT_setpen (GMT, &Ctrl->F.pen2);
			GMT_setfill (GMT, NULL, true);	/* No fill for inner frame */
			PSL_plotsymbol (PSL, Ctrl->D.lon + 0.5 * Ctrl->D.width, Ctrl->D.lat + 0.5 * Ctrl->D.height, sdim, (Ctrl->F.mode & 2) ? PSL_RNDRECT : PSL_RECT);
		}
	}

	/* We use a standard x/y inch coordinate system here, unlike old pslegend. */

	x0 = Ctrl->D.lon + Ctrl->C.dx;			/* Left justification edge of items inside legend box */
	y0 = Ctrl->D.lat + Ctrl->D.height - Ctrl->C.dy;	/* Top justification edge of items inside legend box  */
	column_number = 0;
	txtcolor[0] = 0;

	dim[2] = 2;	/* We will a 2-row data set for fronts */
	if ((Front = GMT_Create_Data (API, GMT_IS_DATASET, dim)) == NULL) {
		GMT_report (GMT, GMT_MSG_NORMAL, "Unable to create a Front text set for pslegend\n");
		return (API->error);
	}
	dim[2] = GMT_SMALL_CHUNK;	/* We will allocate 3 more textsets; one for text, paragraph text, symbols */
	for (id = 0; id < N_CMD; id++) {
		if ((D[id] = GMT_Create_Data (API, GMT_IS_TEXTSET, dim)) == NULL) {
			GMT_report (GMT, GMT_MSG_NORMAL, "Unable to create a text set for pslegend\n");
			Return (API->error);
		}
	}

#ifdef DEBUG
	if (guide) drawbase (GMT, PSL, Ctrl->D.lon, Ctrl->D.lon + Ctrl->D.width, y0);
#endif

	for (tbl = 0; tbl < In->n_tables; tbl++) {	/* We only expect one table but who knows what the user does */
		for (seg = 0; seg < In->table[tbl]->n_segments; seg++) {	/* We only expect one segment in each table but again... */
			for (row = 0; row < In->table[tbl]->segment[seg]->n_rows; row++) {	/* Finally processing the rows */
				line = In->table[tbl]->segment[seg]->record[row];
				if (line[0] == '#') continue;	/* Skip all headers */

				/* Data record to process */

				if (line[0] != 'T' && flush_paragraph) {	/* Flush contents of pending paragraph [Call GMT_pstext] */
					flush_paragraph = false;
					column_number = 0;
				}

				switch (line[0]) {
					case 'B':	/* Color scale Bar [Use GMT_psscale] */
						bar_opts[0] = '\0';
						sscanf (&line[2], "%s %s %s %[^\n]", bar_cpt, bar_gap, bar_height, bar_opts);
						x_off = GMT_to_inch (GMT, bar_gap);
						sprintf (buffer, "-C%s -O -K -D%gi/%gi/%gi/%sh %s", bar_cpt, Ctrl->D.lon + 0.5 * Ctrl->D.width, y0, Ctrl->D.width - 2 * x_off, bar_height, bar_opts);
						status = GMT_psscale (API, 0, buffer);	/* Plot the colorbar */
						if (status) {
							GMT_report (GMT, GMT_MSG_NORMAL, "GMT_psscale returned error %d.\n", status);
							Return (EXIT_FAILURE);
						}
						y0 -= GMT_to_inch (GMT, bar_height) + GMT->current.setting.map_tick_length[0] + GMT->current.setting.map_annot_offset[0] + FONT_HEIGHT_PRIMARY * GMT->current.setting.font_annot[0].size / PSL_POINTS_PER_INCH;
						column_number = 0;
						API->io_enabled[GMT_IN] = true;	/* UNDOING SETTING BY psscale */
						break;

					case 'C':	/* Color change */
						sscanf (&line[2], "%[^\n]", txtcolor);
						break;

					case 'D':	/* Delimiter record */
						sscanf (&line[2], "%s %s", txt_a, txt_b);
						L = GMT_to_inch (GMT, txt_a);
						if (txt_b[0] && GMT_getpen (GMT, txt_b, &current_pen)) GMT_pen_syntax (GMT, 'W', " ");
						GMT_setpen (GMT, &current_pen);
						y0 -= quarter_line_spacing;
						PSL_plotsegment (PSL, Ctrl->D.lon + L, y0, Ctrl->D.lon + Ctrl->D.width - L, y0);
						y0 -= quarter_line_spacing;
						column_number = 0;
#ifdef DEBUG
						if (guide) drawbase (GMT, PSL, Ctrl->D.lon, Ctrl->D.lon + Ctrl->D.width, y0);
#endif
						break;

					case 'G':	/* Gap record */
						sscanf (&line[2], "%s", txt_a);
						y0 -= (txt_a[strlen(txt_a)-1] == 'l') ? atoi (txt_a) * one_line_spacing : GMT_to_inch (GMT, txt_a);
						column_number = 0;
#ifdef DEBUG
						if (guide) drawbase (GMT, PSL, Ctrl->D.lon, Ctrl->D.lon + Ctrl->D.width, y0);
#endif
						break;

					case 'H':	/* Header record */
						sscanf (&line[2], "%s %s %[^\n]", size, font, text);
						if (size[0] == '-') size[0] = 0;
						if (font[0] == '-') font[0] = 0;
						sprintf (tmp, "%s,%s,%s", size, font, txtcolor);		/* Put size, font and color together for parsing by GMT_getfont */
						ifont = GMT->current.setting.font_title;	/* Set default font */
						GMT_getfont (GMT, tmp, &ifont);
						d_off = 0.5 * (Ctrl->L.spacing - FONT_HEIGHT (ifont.id)) * ifont.size / PSL_POINTS_PER_INCH;	/* To center the text */
						y0 -= Ctrl->L.spacing * ifont.size / PSL_POINTS_PER_INCH;
						sprintf (buffer, "%g %g %s BC %s", Ctrl->D.lon + 0.5 * Ctrl->D.width, y0 + d_off, GMT_putfont (GMT, ifont), text);
						S[TXT] = D[TXT]->table[0]->segment[0];	/* Since there will only be one table with one segment for each set, except for fronts */
						S[TXT]->record[S[TXT]->n_rows++] = strdup (buffer);
						if (S[TXT]->n_rows == S[TXT]->n_alloc) S[TXT]->record = GMT_memory (GMT, S[TXT]->record, S[TXT]->n_alloc += GMT_SMALL_CHUNK, char *);
						column_number = 0;
#ifdef DEBUG
						if (guide) drawbase (GMT, PSL, Ctrl->D.lon, Ctrl->D.lon + Ctrl->D.width, y0);
#endif
						break;

					case 'I':	/* Image record [use GMT_psimage] */
						sscanf (&line[2], "%s %s %s", image, size, key);
						PSL_loadimage (PSL, image, &header, &dummy);
						justify = GMT_just_decode (GMT, key, 12);
						x_off = Ctrl->D.lon;
						x_off += (justify%4 == 1) ? Ctrl->C.dx : ((justify%4 == 3) ? Ctrl->D.width - Ctrl->C.dx : 0.5 * Ctrl->D.width);
						sprintf (buffer, "-O -K %s -W%s -C%gi/%gi/%s", image, size, x_off, y0, key);
						status = GMT_psimage (API, 0, buffer);	/* Plot the image */
						if (status) {
							GMT_report (GMT, GMT_MSG_NORMAL, "GMT_psimage returned error %d.\n", status);
							Return (EXIT_FAILURE);
						}
						y0 -= GMT_to_inch (GMT, size) * (double)header.height / (double)header.width;
						column_number = 0;
						break;

					case 'L':	/* Label record */
						sscanf (&line[2], "%s %s %s %[^\n]", size, font, key, text);
						if (size[0] == '-') size[0] = 0;
						if (font[0] == '-') font[0] = 0;
						sprintf (tmp, "%s,%s,%s", size, font, txtcolor);		/* Put size, font and color together for parsing by GMT_getfont */
						ifont = GMT->current.setting.font_label;	/* Set default font */
						GMT_getfont (GMT, tmp, &ifont);
						d_off = 0.5 * (Ctrl->L.spacing - FONT_HEIGHT (ifont.id)) * ifont.size / PSL_POINTS_PER_INCH;	/* To center the text */
						if (column_number%n_columns == 0) y0 -= Ctrl->L.spacing * ifont.size / PSL_POINTS_PER_INCH;
						justify = GMT_just_decode (GMT, key, 0);
						x_off = Ctrl->D.lon + (Ctrl->D.width / n_columns) * (column_number%n_columns);
						x_off += (justify%4 == 1) ? Ctrl->C.dx : ((justify%4 == 3) ? Ctrl->D.width / n_columns - Ctrl->C.dx : 0.5 * Ctrl->D.width / n_columns);
						sprintf (buffer, "%g %g %s B%s %s", x_off, y0 + d_off, GMT_putfont (GMT, ifont), key, text);
						S[TXT] = D[TXT]->table[0]->segment[0];	/* Since there will only be one table with one segment for each set, except for fronts */
						S[TXT]->record[S[TXT]->n_rows++] = strdup (buffer);
						if (S[TXT]->n_rows == S[TXT]->n_alloc) S[TXT]->record = GMT_memory (GMT, S[TXT]->record, S[TXT]->n_alloc += GMT_SMALL_CHUNK, char *);
						column_number++;
#ifdef DEBUG
						if (guide) drawbase (GMT, PSL, Ctrl->D.lon, Ctrl->D.lon + Ctrl->D.width, y0);
#endif
						break;

					case 'M':	/* Map scale record M lon0|- lat0 length[n|m|k][+opts] f|p  [-R -J] */
						n_scan = sscanf (&line[2], "%s %s %s %s %s %s", txt_a, txt_b, txt_c, txt_d, txt_e, txt_f);
						k = (txt_d[0] != 'f') ? 1 : 0;	/* Determines if we start -L with f or not */
						for (i = 0, gave_mapscale_options = false; txt_c[i] && !gave_mapscale_options; i++) if (txt_c[i] == '+') gave_mapscale_options = true;
						/* Default assumes label is added on top */
						just = 't';
						gave_label = true;
						d_off = FONT_HEIGHT_LABEL * GMT->current.setting.font_label.size / PSL_POINTS_PER_INCH + fabs(GMT->current.setting.map_label_offset);

						if ((opt = strchr (txt_c, '+'))) {	/* Specified alternate label (could be upper case, hence 0.85) and justification */
							char txt_cpy[GMT_BUFSIZ], p[GMT_TEXT_LEN256];
							unsigned int pos = 0;
							strncpy (txt_cpy, opt, GMT_BUFSIZ);
							while ((GMT_strtok (txt_cpy, "+", &pos, p))) {
								switch (p[0]) {
									case 'u':	/* Label put behind annotation */
										gave_label = false;
										break;
									case 'j':	/* Justification */
										just = p[1];
										break;
									default:	/* Just ignore */
										break;
								}
							}
						}
						if (gave_label && just == 't') y0 -= d_off;
						if (!strcmp (txt_a, "-"))	/* No longitude needed */
							sprintf (mapscale, "fx%gi/%gi/%s/%s", Ctrl->D.lon + 0.5 * Ctrl->D.width, y0, txt_b, txt_c);
						else				/* Gave both lon and lat for scale */
							sprintf (mapscale, "fx%gi/%gi/%s/%s/%s", Ctrl->D.lon + 0.5 * Ctrl->D.width, y0, txt_a, txt_b, txt_c);
						if (n_scan == 6)	/* Gave specific -R -J on M line */
							sprintf (buffer, "%s %s -O -K -L%s", txt_e, txt_f, &mapscale[k]);
						else {	/* Use -R -J supplied to pslegend */
							if (!r_ptr || !j_ptr) {
								GMT_report (GMT, GMT_MSG_NORMAL, "Error: The M record must have map -R -J if -Dx and no -R -J is used\n");
								Return (GMT_RUNTIME_ERROR);
							}
							sprintf (buffer, "-R%s -J%s -O -K -L%s", r_ptr->arg, j_ptr->arg, &mapscale[k]);
						}
						status = GMT_psbasemap (API, 0, buffer);	/* Plot the scale */
						if (status) {
							GMT_report (GMT, GMT_MSG_NORMAL, "GMT_psbasemap returned error %d.\n", status);
							Return (EXIT_FAILURE);
						}
						if (gave_label && just == 'b') y0 -= d_off;
						y0 -= GMT->current.setting.map_scale_height + FONT_HEIGHT_PRIMARY * GMT->current.setting.font_annot[0].size / PSL_POINTS_PER_INCH + GMT->current.setting.map_annot_offset[0];
						column_number = 0;
#ifdef DEBUG
						if (guide) drawbase (GMT, PSL, Ctrl->D.lon, Ctrl->D.lon + Ctrl->D.width, y0);
#endif
						break;

					case 'N':	/* n_columns record */
						sscanf (&line[2], "%s", txt_a);
						n_columns = atoi (txt_a);
						column_number = 0;
						break;

#ifdef GMT_COMPAT
					case '>':	/* Paragraph text header */
						GMT_report (GMT, GMT_MSG_COMPAT, "Warning: paragraph text header flag > is deprecated; use P instead\n");
						n = sscanf (&line[1], "%s %s %s %s %s %s %s %s %s", xx, yy, size, angle, font, key, lspace, tw, jj);
						if (n < 0) n = 0;	/* Since -1 is returned if no arguments */
						if (!(n == 0 || n == 9)) {
							GMT_report (GMT, GMT_MSG_NORMAL, "Error: The > record must have 0 or 9 arguments (only %d found)\n", n);
							Return (GMT_RUNTIME_ERROR);
						}
						if (n == 0 || size[0] == '-') sprintf (size, "%g", GMT->current.setting.font_annot[0].size);
						if (n == 0 || font[0] == '-') sprintf (font, "%d", GMT->current.setting.font_annot[0].id);
						sprintf (tmp, "%s,%s,", size, font);
						did_old = true;
#endif
					case 'P':	/* Paragraph text header */
						if (!did_old) {
							n = sscanf (&line[1], "%s %s %s %s %s %s %s %s", xx, yy, tmp, angle, key, lspace, tw, jj);
							if (n < 0) n = 0;	/* Since -1 is returned if no arguments */
							if (!(n == 0 || n == 8)) {
								GMT_report (GMT, GMT_MSG_NORMAL, "Error: The P record must have 0 or 9 arguments (only %d found)\n", n);
								Return (GMT_RUNTIME_ERROR);
							}
						}
						did_old = false;
#ifdef DEBUG
						if (guide) drawbase (GMT, PSL, Ctrl->D.lon, Ctrl->D.lon + Ctrl->D.width, y0);
#endif
						if (n == 0 || xx[0] == '-') sprintf (xx, "%g", x0);
						if (n == 0 || yy[0] == '-') sprintf (yy, "%g", y0);
						if (n == 0 || tmp[0] == '-') sprintf (tmp, "%g,%d,%s", GMT->current.setting.font_annot[0].size, GMT->current.setting.font_annot[0].id, txtcolor);
						if (n == 0 || angle[0] == '-') sprintf (angle, "0");
						if (n == 0 || key[0] == '-') sprintf (key, "TL");
						if (n == 0 || lspace[0] == '-') sprintf (lspace, "%gi", one_line_spacing);
						if (n == 0 || tw[0] == '-') sprintf (tw, "%gi", Ctrl->D.width - 2.0 * Ctrl->C.dx);
						if (n == 0 || jj[0] == '-') sprintf (jj, "j");
						sprintf (buffer, "> %s %s %s %s %s %s %s %s", xx, yy, tmp, angle, key, lspace, tw, jj);
						S[PAR] = D[PAR]->table[0]->segment[0];	/* Since there will only be one table with one segment for each set, except for fronts */
						S[PAR]->record[S[PAR]->n_rows++] = strdup (buffer);
						if (S[PAR]->n_rows == S[PAR]->n_alloc) S[PAR]->record = GMT_memory (GMT, S[PAR]->record, S[PAR]->n_alloc += GMT_SMALL_CHUNK, char *);
						flush_paragraph = true;
						column_number = 0;
						break;

					case 'S':	/* Symbol record */
						n_scan = sscanf (&line[2], "%s %s %s %s %s %s %[^\n]", txt_a, symbol, size, txt_c, txt_d, txt_b, text);
						off_ss = GMT_to_inch (GMT, txt_a);
						off_tt = GMT_to_inch (GMT, txt_b);
						d_off = 0.5 * (Ctrl->L.spacing - FONT_HEIGHT_PRIMARY) * GMT->current.setting.font_annot[0].size / PSL_POINTS_PER_INCH;	/* To center the text */
						if (column_number%n_columns == 0) y0 -= one_line_spacing;
						y0 += half_line_spacing;	/* Move to center of box */
						x_off = x0 + (Ctrl->D.width / n_columns) * (column_number%n_columns);
						S[SYM] = D[SYM]->table[0]->segment[0];	/* Since there will only be one table with one segment for each set, except for fronts */
						if (symbol[0] == 'f') {	/* Front is different, must plot as a line segment */
							i = 0;
							while (size[i] != '/' && size[i]) i++;
							if (size[i] != '/') {
								GMT_report (GMT, GMT_MSG_NORMAL, "Error: -Sf option must have a tick length\n");
								Return (EXIT_FAILURE);
							}
							size[i] = '\0';	/* Temporarily truncate */
							x = 0.5 * GMT_to_inch (GMT, size);
							size[i] = '/';	/* Undo truncation */
							i++;
							F = Front->table[0]->segment[0];	/* Since we only will have one segment */
							F->coord[GMT_X][0] = x_off + off_ss-x;	F->coord[GMT_Y][0] = y0;
							F->coord[GMT_X][1] = x_off + off_ss+x;	F->coord[GMT_Y][1] = y0;
							Front->n_records = F->n_rows = 2;
							if ((object_ID = GMT_Register_IO (API, GMT_IS_DATASET, GMT_IS_REF, GMT_IS_LINE, GMT_IN, NULL, Front)) == GMTAPI_NOTSET) {
								Return (API->error);
							}
							if (GMT_Encode_ID (API, string, object_ID) != GMT_OK) {	/* Make filename with embedded object ID */
								Return (API->error);
							}
							sprintf (buffer, "-R0/%g/0/%g -Jx1i -O -K -N -S%s%s %s", GMT->current.proj.rect[XHI], GMT->current.proj.rect[YHI], symbol, &size[i], string);
							if (txt_c[0] != '-') {strcat (buffer, " -G"); strcat (buffer, txt_c);}
							if (txt_d[0] != '-') {strcat (buffer, " -W"); strcat (buffer, txt_d);}
							status = GMT_psxy (API, 0, buffer);	/* Plot the front */
							if (status) {
								GMT_report (GMT, GMT_MSG_NORMAL, "GMT_psxy returned error %d.\n", status);
								Return (EXIT_FAILURE);
							}
							API->io_enabled[GMT_IN] = true;	/* UNDOING SETTING BY psxy */
						}
						else {	/* Regular symbols */
							if (symbol[0] == 'k')
								sprintf (sub, "%s/%s", symbol, size);
							else
								sprintf (sub, "%s%s", symbol, size);
							if (symbol[0] == 'E' || symbol[0] == 'e') {	/* Ellipse needs more arguments we use minor = 0.65*major, az = 0 */
								x = GMT_to_inch (GMT, size);
								sprintf (sarg, "%g %g 0 %gi %gi", x_off + off_ss, y0, x, 0.65 * x);
							}
							else if (symbol[0] == 'V' || symbol[0] == 'v') {	/* Vector needs a prepended length/ string */
								i = 0;
								while (size[i] != '/' && size[i]) i++;
								if (size[i] != '/') {	/* The necessary arguments not supplied! */
									sprintf (sub, "v0.15i+jc+e");	/* Somewhat arbitrary default */
									Return (GMT_RUNTIME_ERROR);
								}
								else {
									size[i++] = '\0';	/* So GMT_to_inch won't complain */
									sprintf (sub, "%s%s+jc+e", symbol, &size[i]);
								}
								if (txt_c[0] == '-') strcat (sub, "+g-");
								else { strcat (sub, "+g"); strcat (sub, txt_c);}
								if (txt_d[0] == '-') strcat (sub, "+p-");
								else { strcat (sub, "+p"); strcat (sub, txt_d);}
								x = GMT_to_inch (GMT, size);
								sprintf (sarg, "%g %g 0 %gi", x_off + off_ss, y0, x);
							}
							else if (symbol[0] == 'r') {	/* Rectangle also need more args, we use h = 0.65*w */
								x = GMT_to_inch (GMT, size);
								sprintf (sarg, "%g %g %gi %gi", x_off + off_ss, y0, x, 0.65*x);
							}
							else if (symbol[0] == 'w') {	/* Wedge also need more args; we set fixed az1,2 as -30 30 */
								x = GMT_to_inch (GMT, size);
								sprintf (sarg, "%g %g -30 30", x_off + off_ss -0.25*x, y0);
							}
							else
								sprintf (sarg, "%g %g", x_off + off_ss, y0);
							/* Place pen and fill colors in segment header */
							sprintf (buffer, ">");
							strcat (buffer, " -G"); strcat (buffer, txt_c);
							strcat (buffer, " -W"); strcat (buffer, txt_d);
							S[SYM]->record[S[SYM]->n_rows++] = strdup (buffer);
							if (S[SYM]->n_rows == S[SYM]->n_alloc) S[SYM]->record = GMT_memory (GMT, S[SYM]->record, S[SYM]->n_alloc += GMT_SMALL_CHUNK, char *);
							sprintf (buffer, "%s %s", sarg, sub);
							S[SYM]->record[S[SYM]->n_rows++] = strdup (buffer);
							if (S[SYM]->n_rows == S[SYM]->n_alloc) S[SYM]->record = GMT_memory (GMT, S[SYM]->record, S[SYM]->n_alloc += GMT_SMALL_CHUNK, char *);
						}
						/* Finally, print text; skip when empty */
						y0 -= half_line_spacing;	/* Go back to bottom of box */
						if (n_scan == 7) {	/* Place symbol text */
							S[TXT] = D[TXT]->table[0]->segment[0];	/* Since there will only be one table with one segment for each set, except for fronts */
							sprintf (buffer, "%g %g %g,%d,%s BL %s", x_off + off_tt, y0 + d_off, GMT->current.setting.font_annot[0].size, GMT->current.setting.font_annot[0].id, txtcolor, text);
							S[TXT]->record[S[TXT]->n_rows++] = strdup (buffer);
							if (S[TXT]->n_rows == S[TXT]->n_alloc) S[TXT]->record = GMT_memory (GMT, S[TXT]->record, S[TXT]->n_alloc += GMT_SMALL_CHUNK, char *);
						}
						column_number++;
#ifdef DEBUG
						if (guide) drawbase (GMT, PSL, Ctrl->D.lon, Ctrl->D.lon + Ctrl->D.width, y0);
#endif
						break;

					case 'T':	/* paragraph text record */
						/* If no previous > record, then use defaults */
						S[PAR] = D[PAR]->table[0]->segment[0];	/* Since there will only be one table with one segment for each set, except for fronts */
						if (!flush_paragraph) {
							d_off = 0.5 * (Ctrl->L.spacing - FONT_HEIGHT_PRIMARY) * GMT->current.setting.font_annot[0].size / PSL_POINTS_PER_INCH;
							sprintf (buffer, "> %g %g %g,%d,%s 0 TL %gi %gi j", x0, y0 - d_off, GMT->current.setting.font_annot[0].size, GMT->current.setting.font_annot[0].id, txtcolor, one_line_spacing, Ctrl->D.width - 2.0 * Ctrl->C.dx);
							S[PAR]->record[S[PAR]->n_rows++] = strdup (buffer);
							if (S[PAR]->n_rows == S[PAR]->n_alloc) S[PAR]->record = GMT_memory (GMT, S[PAR]->record, S[PAR]->n_alloc += GMT_SMALL_CHUNK, char *);
						}
						sscanf (&line[2], "%[^\n]", text);
						S[PAR]->record[S[PAR]->n_rows++] = strdup (text);
						if (S[PAR]->n_rows == S[PAR]->n_alloc) S[PAR]->record = GMT_memory (GMT, S[PAR]->record, S[PAR]->n_alloc += GMT_SMALL_CHUNK, char *);
						flush_paragraph = true;
						column_number = 0;
						break;

					case 'V':	/* Vertical line from here to next V */
						if (draw_vertical_line) {	/* Second time, now draw line */
							sscanf (&line[2], "%s %s", txt_a, txt_b);
							V = GMT_to_inch (GMT, txt_a);
							if (txt_b[0] && GMT_getpen (GMT, txt_b, &current_pen)) {
								GMT_pen_syntax (GMT, 'V', " ");
								Return (GMT_RUNTIME_ERROR);
							}
							GMT_setpen (GMT, &current_pen);
							for (i = 1; i < n_columns; i++) {
								x_off = Ctrl->D.lon + i * Ctrl->D.width / n_columns;
								PSL_plotsegment (PSL, x_off, y_start-V+quarter_line_spacing, x_off, y0+V-quarter_line_spacing);
							}
							draw_vertical_line = false;
						}
						else {
							draw_vertical_line = true;
							y_start = y0;
						}
						column_number = 0;
#ifdef DEBUG
						if (guide) drawbase (GMT, PSL, Ctrl->D.lon, Ctrl->D.lon + Ctrl->D.width, y0);
#endif
						break;

					default:
						GMT_report (GMT, GMT_MSG_NORMAL, "Error: Unrecognized record (%s)\n", line);
						Return (GMT_RUNTIME_ERROR);
					break;
				}
			}
		}
	}

	if (GMT_Destroy_Data (API, GMT_ALLOCATED, &In) != GMT_OK) {
		Return (API->error);
	}

#ifdef GMT_COMPAT
	/* Reset the flag */
	GMT->current.setting.io_seg_marker[GMT_IN] = save_EOF;
#endif

	Front->alloc_mode = GMT_ALLOCATED;	/* So we can free it */
	if (!F) GMT_free_dataset (GMT, &Front);	/* Was unused so free explicitly */
	else if (GMT_Destroy_Data (API, GMT_ALLOCATED, &Front) != GMT_OK) {
		Return (API->error);
	}
	
	/* Time to plot any symbols, text, and paragraphs we collected in the loop */

	if (S[SYM] && S[SYM]->n_rows) {
		/* Create option list, register D[SYM] as input source */
		if ((object_ID = GMT_Register_IO (API, GMT_IS_TEXTSET, GMT_IS_REF, GMT_IS_POINT, GMT_IN, NULL, D[SYM])) == GMTAPI_NOTSET) {
			Return (API->error);
		}
		if (GMT_Encode_ID (API, string, object_ID) != GMT_OK) {
			Return (API->error);	/* Make filename with embedded object ID */
		}
		sprintf (buffer, "-R0/%g/0/%g -Jx1i -O -K -N -S %s", GMT->current.proj.rect[XHI], GMT->current.proj.rect[YHI], string);
		if (GMT_psxy (API, 0, buffer) != GMT_OK) {
			Return (API->error);	/* Plot the symbols */
		}
	}
	if (S[TXT] && S[TXT]->n_rows) {
		/* Create option list, register D[TXT] as input source */
		if ((object_ID = GMT_Register_IO (API, GMT_IS_TEXTSET, GMT_IS_REF, GMT_IS_POINT, GMT_IN, NULL, D[TXT])) == GMTAPI_NOTSET) {
			Return (API->error);
		}
		if (GMT_Encode_ID (API, string, object_ID) != GMT_OK) {
			Return (API->error);	/* Make filename with embedded object ID */
		}
		sprintf (buffer, "-R0/%g/0/%g -Jx1i -O -K -N -F+f+j %s", GMT->current.proj.rect[XHI], GMT->current.proj.rect[YHI], string);
		if (GMT_pstext (API, 0, buffer) != GMT_OK) {
			Return (API->error);	/* Plot the symbols */
		}
	}
	if (S[PAR] && S[PAR]->n_rows) {
		/* Create option list, register D[PAR] as input source */
		if ((object_ID = GMT_Register_IO (API, GMT_IS_TEXTSET, GMT_IS_REF, GMT_IS_POINT, GMT_IN, NULL, D[PAR])) == GMTAPI_NOTSET) {
			Return (API->error);
		}
		if (GMT_Encode_ID (API, string, object_ID) != GMT_OK) {
			Return (API->error);	/* Make filename with embedded object ID */
		}
		sprintf (buffer, "-R0/%g/0/%g -Jx1i -O -K -N -M -F+f+a+j %s", GMT->current.proj.rect[XHI], GMT->current.proj.rect[YHI], string);
		if (GMT_pstext (API, 0, buffer) != GMT_OK) Return (API->error);	/* Plot the symbols */
	}

	for (id = 0; id < 3; id++) GMT_free_textset (GMT, &D[id]);	/* Free directly since never used in Get|Put statements */

	PSL_setorigin (PSL, -x_orig, -y_orig, 0.0, PSL_INV);	/* Reset */

	GMT_map_basemap (GMT);
	GMT_plotend (GMT);

	GMT_report (GMT, GMT_MSG_VERBOSE, "Done\n");

	Return (GMT_OK);
}
