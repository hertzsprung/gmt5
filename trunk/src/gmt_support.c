/*--------------------------------------------------------------------
 *	$Id: gmt_support.c,v 1.318 2007-09-29 23:44:14 remko Exp $
 *
 *	Copyright (c) 1991-2007 by P. Wessel and W. H. F. Smith
 *	See COPYING file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 *
 *			G M T _ S U P P O R T . C
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * GMT_support.c contains code used by most GMT programs
 *
 * Author:	Paul Wessel
 * Date:	13-JUL-2000
 * Version:	4
 *
 * Modules in this file:
 *
 *	GMT_akima		Akima's 1-D spline
 *	GMT_boundcond_init	Initialize struct GMT_EDGEINFO to unset flags
 *	GMT_boundcond_parse	Set struct GMT_EDGEINFO to user's requests
 *	GMT_boundcond_param_prep	Set struct GMT_EDGEINFO to what is doable
 *	GMT_boundcond_set	Set two rows of padding according to bound cond
 *	GMT_check_rgb		Check rgb for valid range
 *	GMT_comp_double_asc	Used when sorting doubles into ascending order [checks for NaN]
 *	GMT_comp_float_asc	Used when sorting floats into ascending order [checks for NaN]
 *	GMT_comp_int_asc	Used when sorting ints into ascending order
 *	GMT_contours		Subroutine for contouring
 *	GMT_cspline		Natural cubic 1-D spline solver
 *	GMT_csplint		Natural cubic 1-D spline evaluator
 *	GMT_delaunay		Performs a Delaunay triangulation
 *	GMT_epsinfo		Fill out info need for PostScript header
 *	GMT_get_annot_label	Construct degree/minute label
 *	GMT_get_annot_offset	Return offset in inches for text annotation
 *	GMT_get_index		Return color table entry for given z
 *	GMT_get_format		Find # of decimals and create format string
 *	GMT_get_rgb_from_z		Return rgb for given z
 *	GMT_get_plot_array	Allocate memory for plotting arrays
 *	GMT_getfill		Decipher and check fill argument
 *	GMT_getinc		Decipher and check increment argument
 *	GMT_getpen		Decipher and check pen argument
 *	GMT_getrgb		Decipher and check color argument
 *	GMT_init_fill		Initialize fill attributes
 *	GMT_init_pen		Initialize pen attributes
 *	GMT_hsv_to_rgb		Convert HSV to RGB
 *	GMT_illuminate		Add illumination effects to rgb
 *	GMT_intpol		1-D interpolation
 *	GMT_memory		Memory allocation/reallocation
 *	GMT_free		Memory deallocation
 *	GMT_non_zero_winding	Finds if a point is inside/outside a polygon
 *	GMT_read_cpt		Read color palette file
 *	GMT_rgb_to_hsv		Convert RGB to HSV
 *	GMT_smooth_contour	Use Akima's spline to smooth contour
 *	GMT_start_trace		Subfunction used by GMT_trace_contour
 *	GMT_trace_contour	Function that trace the contours in GMT_contours
 *	GMT_polar_adjust	Adjust label justification for polar projection
 *	GMT_fourt		Fourier transform routine
 */

#define GMT_WITH_NO_PS
#include "gmt.h"

#define I_255	(1.0 / 255.0)
#define DEG_TO_METER (6371008.7714 * D2R)
#define DEG_TO_KM (6371.0087714 * D2R)
#define KM_TO_DEG (1.0 / DEG_TO_KM)
#define GMT_INSIDE_POLYGON	2
#define GMT_OUTSIDE_POLYGON	0
#define GMT_ONSIDE_POLYGON	1

int GMT_color_rgb[GMT_N_COLOR_NAMES][3] = {	/* r/g/b of X11 colors */
#include "gmt_color_rgb.h"
};
struct GMT_PEN_NAME {	/* Names of pens and their thicknesses */
	char name[16];
	double width;
};

struct GMT_PEN_NAME GMT_penname[GMT_N_PEN_NAMES] = {		/* Names and widths of pens */
#include "gmt_pennames.h"
};


int GMT_polar_adjust(int side, double angle, double x, double y);
int GMT_start_trace(float first, float second, int *edge, int edge_word, int edge_bit, unsigned int *bit);
int GMT_trace_contour(float *grd, struct GRD_HEADER *header, double x0, double y0, int *edge, double **x_array, double **y_array, int i, int j, int kk, int offset, int *i_off, int *j_off, int *k_off, int *p, unsigned int *bit, int *nan_flag);
int GMT_smooth_contour(double **x_in, double **y_in, int n, int sfactor, int stype);
int GMT_splice_contour(double **x, double **y, int n, double **x2, double **y2, int n2);
void GMT_orient_contour (float *grd, struct GRD_HEADER *header, double *x, double *y, int n, int orient);
void GMT_setcontjump (float *z, int nz);
void GMT_rgb_to_hsv(int rgb[], double *h, double *s, double *v);
void GMT_hsv_to_rgb(int rgb[], double h, double s, double v);
void GMT_rgb_to_cmyk (int rgb[], double cmyk[]);
void GMT_cmyk_to_rgb (int rgb[], double cmyk[]);
#if 0
void GMT_get_bcr_cardinals (double x, double y, struct GMT_BCR *bcr);
void GMT_get_bcr_ij (struct GRD_HEADER *grd, double xx, double yy, int *ii, int *jj, struct GMT_EDGEINFO *edgeinfo, struct GMT_BCR *bcr);
void GMT_get_bcr_xy(struct GRD_HEADER *grd, double xx, double yy, double *x, double *y, struct GMT_BCR *bcr);
void GMT_get_bcr_nodal_values(float *z, int ii, int jj, struct GMT_BCR *bcr);
#endif
int GMT_check_hsv (double h, double s, double v);
int GMT_check_cmyk (double cmyk[]);
int GMT_char_count (char *txt, char c);
int GMT_colorname2index (char *name);
int GMT_name2pen (char *name);
int GMT_gettexture (char *line, int unit, double scale, struct GMT_PEN *P);
int GMT_getpenwidth (char *line, int *pen_unit, double *pen_scale, struct GMT_PEN *P);
int GMT_penunit (char c, double *pen_scale);
void GMT_old2newpen (char *line);
BOOLEAN GMT_is_texture (char *word);
BOOLEAN GMT_is_penwidth (char *word);
BOOLEAN GMT_is_color (char *word, int max_slashes);
BOOLEAN GMT_is_pattern (char *word);
int GMT_ysort (const void *p1, const void *p2);
void GMT_x_alloc (struct GMT_XOVER *X, int nx_alloc);
int sort_label_struct (const void *p_1, const void *p_2);
struct GMT_LABEL * GMT_contlabel_new (void);
void GMT_place_label (struct GMT_LABEL *L, char *txt, struct GMT_CONTOUR *G, BOOLEAN use_unit);
void GMT_contlabel_fixpath (double **xin, double **yin, double d[], int *n, struct GMT_CONTOUR *G);
void GMT_contlabel_addpath (double x[], double y[], int n, double zval, char *label, BOOLEAN annot, struct GMT_CONTOUR *G);
void GMT_hold_contour_sub (double **xxx, double **yyy, int nn, double zval, char *label, char ctype, double cangle, int closed, struct GMT_CONTOUR *G);
void GMT_get_radii_of_curvature (double x[], double y[], int n, double r[]);
int GMT_label_is_OK (struct GMT_LABEL *L, char *this_label, char *label, double this_dist, double this_value_dist, int xl, int fj, struct GMT_CONTOUR *G);
int GMT_contlabel_specs_old (char *txt, struct GMT_CONTOUR *G);
int GMT_init_custom_symbol (char *name, struct GMT_CUSTOM_SYMBOL **S);
int GMT_get_label_parameters(int side, double line_angle, int type, double *text_angle, int *justify);
int GMT_inonout_sphpol_count (double plon, double plat, const struct GMT_LINE_SEGMENT *P, int count[]);
#if 0
void GMT_near_zero_roundoff_fixer_upper (double *ww, int axis);
#endif
int GMT_gethsv (char *line, double hsv[]);
void GMT_cmyk_to_hsv (double hsv[], double cmyk[]);
void get_rgb_lookup (int index, double value, int *rgb);

double *GMT_x2sys_Y;

const char * GMT_strerror (int err)
{
/* Returns the error string for a given error code "err"
   Passes "err" on to nc_strerror if the error code is not one we defined */
	switch (err) {
		case GMT_GRDIO_FILE_NOT_FOUND:
			return "Could not find file";
		case GMT_GRDIO_OPEN_FAILED:
			return "Could not open file";
		case GMT_GRDIO_CREATE_FAILED:
			return "Could not create file";
		case GMT_GRDIO_STAT_FAILED:
			return "Could not stat file";
		case GMT_GRDIO_READ_FAILED:
			return "Could not read from file";
		case GMT_GRDIO_WRITE_FAILED:
			return "Could not write to file";
		case GMT_GRDIO_SEEK_FAILED:
			return "Failed to fseek on file";
		case GMT_GRDIO_NOT_RAS:
			return "Not a Sun raster file";
		case GMT_GRDIO_NOT_8BIT_RAS:
			return "Not a standard 8-bit Sun raster file";
		case GMT_GRDIO_PIPE_CODECHECK:
			return "Cannot guess grid format type if grid is passed via pipe";
		case GMT_GRDIO_UNKNOWN_FORMAT:
			return "Not a supported grid format";
		case GMT_GRDIO_UNKNOWN_ID:
			return "Unknown grid format id number";
		case GMT_GRDIO_UNKNOWN_TYPE:
			return "Unknown grid data type";
		case GMT_GRDIO_NOT_SURFER:
			return "Not a valid Surfer 6|7 grid file";
		case GMT_GRDIO_SURF7_UNSUPPORTED:
			return "This Surfer 7 format (full extent or with break lines) is not supported";
		case GMT_GRDIO_BAD_IMG_LAT:
			return "Must specify max latitude for img file";
		case GMT_GRDIO_DOMAIN_VIOLATION:
			return "Tried to read beyond grid domain";
		case GMT_GRDIO_NO_2DVAR:
			return "No 2-D variable in file";
		case GMT_GRDIO_NO_VAR:
			return "Named variable does not exist in file";
		case GMT_GRDIO_BAD_DIM:
			return "Named variable is not 2-, 3-, 4- or 5-D";
		case GMT_GRDIO_NC_NO_PIPE:
			return "NetCDF-based I/O does not support piping";
		case GMT_GRDIO_GRD98_XINC:
			return "GRD98 format requires n = 1/x_inc to be an integer";
		case GMT_GRDIO_GRD98_BADMAGIC:
			return "GRD98 grid file has wrong magic number";
		case GMT_GRDIO_GRD98_BADLENGTH:
			return "GRD98 grid file has wrong length";
		case GMT_GRDIO_GRD98_COMPLEX:
			return "GRD98 grid file cannot hold complex data";
		case GMT_GRDIO_RI_OLDBAD:
			return "Use grdedit -A on your grid file to make region and increments compatible";
		case GMT_GRDIO_RI_NEWBAD:
			return "Please select compatible -R and -I values";
		case GMT_GRDIO_BAD_XINC:
			return "Grid x increment <= 0.0";
		case GMT_GRDIO_BAD_XRANGE:
			return "Subset x range <= 0.0";
		case GMT_GRDIO_BAD_YINC:
			return "Grid y increment <= 0.0";
		case GMT_GRDIO_BAD_YRANGE:
			return "Subset y range <= 0.0";
		case GMT_GRDIO_RI_NOREPEAT:
			return "Pixel format grids do not have repeating rows or columns";
		case GMT_IO_BAD_PLOT_DEGREE_FORMAT:
			return "Unacceptable PLOT_DEGREE_FORMAT template. A not allowed";
		case GMT_CHEBYSHEV_NEG_ORDER:
			return "GMT_chebyshev given negative degree";
		case GMT_CHEBYSHEV_BAD_DOMAIN:
			return "GMT_chebyshev given |x| > 1";
		case GMT_MAP_EXCEEDS_360:
			return "Map region exceeds 360 degrees";
		case GMT_MAP_BAD_ELEVATION_MIN:
			return "\"South\" (min elevation) is outside 0-90 degree range";
		case GMT_MAP_BAD_ELEVATION_MAX:
			return "\"North\" (max elevation) is outside 0-90 degree range";
		case GMT_MAP_BAD_LAT_MIN:
			return "South is outside -90 to +90 degree range";
		case GMT_MAP_BAD_LAT_MAX:
			return "North is outside -90 to +90 degree range";
		case GMT_MAP_NO_REGION:
			return "No map region selected";
		case GMT_MAP_NO_PROJECTION:
			return "No projection selected";
		case GMT_MAP_BAD_DIST_FLAG:
			return "Wrong flag passed to GMT_distances";
		case GMT_MAP_BAD_MEASURE_UNIT:
			return "Bad meusurement unit.  Choose among cimp";
		default:	/* default passes through to NC error */
			return nc_strerror(err);
	}
}

int GMT_err_pass (int err, char *file)
{
	if (err == GMT_NOERROR) return (err);
	/* When error code is non-zero: print error message and pass error code on */
	if (file && file[0])
		fprintf (stderr, "%s: %s [%s]\n", GMT_program, GMT_strerror(err), file);
	else
		fprintf (stderr, "%s: %s\n", GMT_program, GMT_strerror(err));
	return (err);
}

void GMT_err_fail (int err, char *file)
{
	if (err == GMT_NOERROR) return;
	/* When error code is non-zero: print error message and exit */
	if (file && file[0])
		fprintf (stderr, "%s: %s [%s]\n", GMT_program, GMT_strerror(err), file);
	else
		fprintf (stderr, "%s: %s\n", GMT_program, GMT_strerror(err));
	exit (EXIT_FAILURE);
}

int GMT_parse_multisegment_header (char *header, BOOLEAN use_cpt, BOOLEAN *use_fill, struct GMT_FILL *fill, struct GMT_FILL *def_fill,  BOOLEAN *use_pen, struct GMT_PEN *pen, struct GMT_PEN *def_pen, int def_outline)
{
	/* Scan header for occurences of -W, -G, -Z as they affect pens and fills.
	 * The possibilities are:
	 * Fill:  -G<fill>	Use the new fill and turn filling on
	 *	 -G-		Turn filling OFF
	 *	 -G+		Revert to default fill [none if not set on command line]
	 * Pens: -W<pen>	Use the new pen and turn outline on
	 *	 -W-		Turn outline OFF
	 *	 -W+		Revert to default pen [none if not set on command line]
	 * z:	-Z<zval>	Obtain fill via cpt lookup using this z value
	 *	-ZNaN		Get the NaN color from the cpt file
	 *
	 * header is the text string to process
	 * use_fill is set to TRUE, FALSE or left alone if no change
	 * fill is the fill structure to use after this function returns
	 * def_fill holds the default fill (if any) to use if -G+ is found
	 * use_pen is set to TRUE, FALSE or left alone if no change
	 * pen is the pen structure to use after this function returns
	 * def_pen holds the default pen (if any) to use if -W+ is found
	 * def_outline holds the default outline setting (TRUE/FALSE)
	 *
	 * Function returns TRUE if the pen parameters have changed.
	 */

	char *p, line[BUFSIZ];
	int i, processed = 0, change = 0;
	double z;
	struct GMT_FILL test_fill;
	struct GMT_PEN test_pen;

	if ((p = strstr (header, " -G")) || (p = strstr (header, "\t-G"))) {	/* Found a potential -G option */
		strcpy (line, &p[3]);
		for (i = 0; line[i]; i++) if (line[i] == ' ' || line[i] == '\t') line[i] = '\0';
		if (line[0] == '-') {	/* turn fill OFF */
			fill->rgb[0] = fill->rgb[1] = fill->rgb[2] = -1;
			*use_fill = FALSE;
			fill->use_pattern = FALSE;
			processed++;	/* Processed one option */
		}
		else if (line[0] == '+') {	/* Revert to default fill */
			memcpy ((void *)fill, (void *)def_fill, sizeof (struct GMT_FILL));
			*use_fill = (def_fill->use_pattern || def_fill->rgb[0] != -1);
			if (*use_fill) change = 1;
			processed++;	/* Processed one option */
		}
		else if (!GMT_getfill (line, &test_fill)) {	/* Successfully processed a -G<fill> option */
			memcpy ((void *)fill, (void *)&test_fill, sizeof (struct GMT_FILL));
			*use_fill = TRUE;
			change = 1;
			processed++;	/* Processed one option */
		}
		/* Failure is OK since -Gjunk may appear in text strings - we then do nothing (hence no else clause) */
	}
	if (use_cpt && ((p = strstr (header, " -Z")) || (p = strstr (header, "\t-Z")))) {	/* Set symbol r/g/b via cpt-lookup */
		if(!strncmp (&p[3], "NaN", 3))	{	/* Got -ZNaN */
			GMT_get_fill_from_z (GMT_d_NaN, fill);
			*use_fill = TRUE;
			change |= 2;
			processed++;	/* Processed one option */
		}
		else if (sscanf (&p[3], "%lg", &z) == 1) {
			GMT_get_fill_from_z (z, fill);
			*use_fill = TRUE;
			change |= 2;
			processed++;	/* Processed one option */
		}
		/* Failure is OK since -Zjunk may appear in text strings - we then do nothing (hence no else clause) */
	}

	if (processed == 2) fprintf (stderr, "%s: Warning: multisegment header has both -G and -Z options\n", GMT_program);	/* Giving both -G and -Z is a problem */

	if ((p = strstr (header, " -W")) || (p = strstr (header, "\t-W"))) {	/* Found a potential -W option */
		strcpy (line, &p[3]);
		for (i = 0; line[i]; i++) if (line[i] == ' ' || line[i] == '\t') line[i] = '\0';
		if (line[0] == '-') {	/* turn outline OFF */
			*use_pen = FALSE;
		}
		else if (line[0] == '+') {	/* Revert to default pen/outline */
			*use_pen = def_outline;
			memcpy ((void *)pen, (void *)def_pen, sizeof (struct GMT_PEN));
			if (def_outline) change |= 4;
		}
		else if (!GMT_getpen (line, &test_pen)) {	/* Successfully processed a -W<pen> option */
			memcpy ((void *)pen, (void *)&test_pen, sizeof (struct GMT_PEN));
			*use_pen = TRUE;
			change |= 4;
		}
		/* Failure is OK since -W may appear in text strings (hence no else clause) */
	}
	return (change);
}

int GMT_check_rgb (int rgb[])
{
	return (( (rgb[0] < 0 || rgb[0] > 255) || (rgb[1] < 0 || rgb[1] > 255) || (rgb[2] < 0 || rgb[2] > 255) ));
}

int GMT_check_hsv (double h, double s, double v)
{
	return (( (h < 0.0 || h > 360.0) || (s < 0.0 || s > 1.0) || (h < 0.0 || v > 1.0) ));
}

int GMT_check_cmyk (double cmyk[])
{
	int i;
	for (i = 0; i < 4; i++) if (cmyk[i] < 0.0 || cmyk[i] > 100.0) return (TRUE);
	return (FALSE);
}

void GMT_init_fill (struct GMT_FILL *fill, int r, int g, int b)
{	/* Initialize FILL structure */
	int i;

	fill->use_pattern = fill->inverse = FALSE;
	fill->pattern[0] = 0;
	fill->pattern_no = 0;
	fill->dpi = 0;
	for (i = 0; i < 3; i++) fill->f_rgb[i] = 0;
	for (i = 0; i < 3; i++) fill->b_rgb[i] = 255;
	fill->rgb[0] = r; fill->rgb[1] = g; fill->rgb[2] = b;
}

int GMT_getfill (char *line, struct GMT_FILL *fill)
{
	int n, end, error = 0;
	int pos, i, fb_rgb[3];
	char f, word[GMT_LONG_TEXT];

	/* Syntax:   -G<gray>, -G<rgb>, -G<cmyk>, -G<hsv> or -Gp|P<dpi>/<image>[:F<rgb>B<rgb>]   */
	/* Note, <rgb> can be r/g/b, gray, or - for masks */

	GMT_init_fill (fill, -1, -1, -1);	/* Initialize fill structure */
	GMT_chop (line);	/* Remove trailing CR, LF and properly NULL-terminate the string */

	if ((line[0] == 'p' || line[0] == 'P') && isdigit((int)line[1])) {	/* Image specified */
		n = sscanf (&line[1], "%d/%s", &fill->dpi, fill->pattern);
		if (n != 2) error = 1;
		for (i = 0, pos = -1; fill->pattern[i] && pos == -1; i++) if (fill->pattern[i] == ':') pos = i;
		if (pos > -1) fill->pattern[pos] = '\0';
		fill->pattern_no = atoi (fill->pattern);
		if (fill->pattern_no == 0) fill->pattern_no = -1;
		fill->inverse = !(line[0] == 'P');
		fill->use_pattern = TRUE;

		/* See if fore- and background colors are given */

		for (i = 0, pos = -1; line[i] && pos == -1; i++) if (line[i] == ':') pos = i;
		pos++;

		if (pos > 0 && line[pos]) {	/* Gave colors */
			while (line[pos]) {
				f = line[pos++];
				if (line[pos] == '-')	/* Signal for transpacency masking */
					fb_rgb[0] = fb_rgb[1] = fb_rgb[2] = -1;
				else {
					end = pos;
					while (line[end] && !(line[end] == 'F' || line[end] == 'B')) end++;
					strncpy (word, &line[pos], end - pos);
					word[end - pos] = '\0';
					if (GMT_getrgb (word, fb_rgb)) {
						fprintf (stderr, "%s: Colorizing value %s not recognized!\n", GMT_program, word);
						GMT_exit (EXIT_FAILURE);
					}
				}
				if (f == 'f' || f == 'F')
					memcpy ((void *)fill->f_rgb, (void *)fb_rgb, (size_t)(3 * sizeof (int)));
				else if (f == 'b' || f == 'B')
					memcpy ((void *)fill->b_rgb, (void *)fb_rgb, (size_t)(3 * sizeof (int)));
				else {
					fprintf (stderr, "%s: Colorizing argument %c not recognized!\n", GMT_program, f);
					GMT_exit (EXIT_FAILURE);
				}
				while (line[pos] && !(line[pos] == 'F' || line[pos] == 'B')) pos++;
			}
		}
	}
	else {	/* Plain color or shade */
		error = GMT_getrgb (line, fill->rgb);
		fill->use_pattern = FALSE;
	}
	return (error);
}

int GMT_char_count (char *txt, char c)
{
	int i = 0, n = 0;
	while (txt[i]) if (txt[i++] == c) n++;
	return (n);
}

int GMT_getrgb (char *line, int rgb[])
{
	int n, i, count, hyp;

	if (!line[0]) return (FALSE);	/* Nothing to do - accept default action */

	count = GMT_char_count (line, '/');
	hyp   = GMT_char_count (line, '-');

	if (count == 3) {	/* c/m/y/k */
		double cmyk[4];
		n = sscanf (line, "%lf/%lf/%lf/%lf", &cmyk[0], &cmyk[1], &cmyk[2], &cmyk[3]);
		if (n != 4 || GMT_check_cmyk (cmyk)) return (TRUE);
		GMT_cmyk_to_rgb (rgb, cmyk);
		return (FALSE);
	}

	if (count == 2) {	/* r/g/b */
		if (gmtdefs.color_model & GMT_READ_RGB) {	/* r/g/b */
			n = sscanf (line, "%d/%d/%d", &rgb[0], &rgb[1], &rgb[2]);
			if (n != 3 || GMT_check_rgb (rgb)) return (TRUE);
		}
		else {					/* h/s/v */
			double h, s, v;
			n = sscanf (line, "%lf/%lf/%lf", &h, &s, &v);
			if (n != 3 || GMT_check_hsv (h, s, v)) return (TRUE);
			GMT_hsv_to_rgb (rgb, h, s, v);
		}
		return (FALSE);
	}

	if (hyp == 2) {	/* h-s-v */
		double h, s, v;
		n = sscanf (line, "%lf-%lf-%lf", &h, &s, &v);
		if (n != 3 || GMT_check_hsv (h, s, v)) return (TRUE);
		GMT_hsv_to_rgb (rgb, h, s, v);
		return (FALSE);
	}

	if (count == 0) {				/* gray or colorname */
		if (isdigit((int)line[0])) {
			n = sscanf (line, "%d", &rgb[0]);
			rgb[1] = rgb[2] = rgb[0];
			if (n != 1 || GMT_check_rgb (rgb)) return (TRUE);
		}
		else {
			if ((n = GMT_colorname2index (line)) < 0) {
				fprintf (stderr, "%s: Colorname %s not recognized!\n", GMT_program, line);
				GMT_exit (EXIT_FAILURE);
			}
			for (i = 0; i < 3; i++) rgb[i] = GMT_color_rgb[n][i];
		}
		return (FALSE);
	}

	/* Get here if there is a problem */

	return (TRUE);
}

int GMT_gethsv (char *line, double hsv[])
{
	int n, i, count, hyp, rgb[3];

	if (!line[0]) return (FALSE);	/* Nothing to do - accept default action */

	count = GMT_char_count (line, '/');
	hyp   = GMT_char_count (line, '-');

	if (count == 3) {	/* c/m/y/k */
		double cmyk[4];
		n = sscanf (line, "%lf/%lf/%lf/%lf", &cmyk[0], &cmyk[1], &cmyk[2], &cmyk[3]);
		if (n != 4 || GMT_check_cmyk (cmyk)) return (TRUE);
		GMT_cmyk_to_hsv (hsv, cmyk);
		return (FALSE);
	}

	if (count == 2) {	/* r/g/b */
		if (gmtdefs.color_model & GMT_READ_RGB) {	/* r/g/b */
			n = sscanf (line, "%d/%d/%d", &rgb[0], &rgb[1], &rgb[2]);
			if (n != 3 || GMT_check_rgb (rgb)) return (TRUE);
			GMT_rgb_to_hsv (rgb, &hsv[0], &hsv[1], &hsv[2]);
		}
		else {					/* h/s/v */
			n = sscanf (line, "%lf/%lf/%lf", &hsv[0], &hsv[1], &hsv[2]);
			if (n != 3 || GMT_check_hsv (hsv[0], hsv[1], hsv[2])) return (TRUE);
		}
		return (FALSE);
	}

	if (hyp == 2) {	/* h-s-v */
		n = sscanf (line, "%lf-%lf-%lf", &hsv[0], &hsv[1], &hsv[2]);
		if (n != 3 || GMT_check_hsv (hsv[0], hsv[1], hsv[2])) return (TRUE);
		return (FALSE);
	}

	if (count == 0) {				/* gray or colorname */
		if (isdigit((int)line[0])) {
			n = sscanf (line, "%d", &rgb[0]);
			rgb[1] = rgb[2] = rgb[0];
			if (n != 1 || GMT_check_rgb (rgb)) return (TRUE);
			GMT_rgb_to_hsv (rgb, &hsv[0], &hsv[1], &hsv[2]);
		}
		else {
			if ((n = GMT_colorname2index (line)) < 0) {
				fprintf (stderr, "%s: Colorname %s not recognized!\n", GMT_program, line);
				GMT_exit (EXIT_FAILURE);
			}
			for (i = 0; i < 3; i++) rgb[i] = GMT_color_rgb[n][i];
			GMT_rgb_to_hsv (rgb, &hsv[0], &hsv[1], &hsv[2]);
		}
		return (FALSE);
	}

	/* Get here if there is a problem */

	return (TRUE);
}

void GMT_enforce_rgb_triplets (char *text, int size)
{
	/* Purpose is to replace things like @;lightgreen; with @r/g/b; which ps_text understands */

	int i, j, k = 0, n, last = 0, n_slash, rgb[3];
	char buffer[BUFSIZ], color[16], *p;

	if (!strchr (text, '@')) return;	/* Nothing to do since no espace sequence in string */

	while ((p = strstr (text, "@;"))) {	/* Found a @; sequence */
		i = (int)(p - text) + 2;	/* Position of first character after @; */
		for (j = last; j < i; j++, k++) buffer[k] = text[j];	/* Copy everything from last stop up to the color specification */
		text[i-1] = 'X';	/* Wipe the ; so that @; wont be found a 2nd time */
		if (text[i] != ';') {	/* Color info now follows */
			n = i;
			n_slash = 0;
			while (text[n] && text[n] != ';') {	/* Find end of the color info and also count slashes */
				if (text[n] == '/') n_slash++;
				n++;
			}
			if (n_slash != 2) {	/* r/g/b not given, must replace whatever it was with a r/g/b triplet */
				text[n] = '\0';	/* Temporarily terminate strong so getrgb can work */
				GMT_getrgb (&text[i], rgb);
				text[n] = ';';	/* Undo damage */
				sprintf (color, "%d/%d/%d", rgb[0], rgb[1], rgb[2]);	/* Format triplet */
				for (j = 0; color[j]; j++, k++) buffer[k] = color[j];	/* Copy over triplet and update buffer pointer k */
			}
			else	/* Already in r/g/b format, just copy */
				for (j = i; j < n; j++, k++) buffer[k] = text[j];
			i = n;	/* Position of terminating ; */
		}
		buffer[k++] = ';';	/* Finish the specification */
		last = i + 1;	/* Start of next part to copy */
	}
	i = last;	/* Finish copying everything left in the text string */
	while (text[i]) buffer[k++] = text[i++];
	buffer[k++] = '\0';	/* Properly terminate buffer */

	if (k > size) fprintf (stderr, "GMT_enforce_rgb_triplets: Replacement string too long - truncated\n");
	strncpy (text, buffer, k);	/* Copy back the revised string */
}

int GMT_colorname2index (char *name)
{
	/* Return index into structure with colornames and r/g/b */

	int k;
	char Lname[GMT_TEXT_LEN];

	strcpy (Lname, name);
	GMT_str_tolower (Lname);
	k = GMT_hash_lookup (Lname, GMT_rgb_hashnode, GMT_N_COLOR_NAMES, GMT_N_COLOR_NAMES);

	return (k);
}

int GMT_name2pen (char *name)
{
	/* Return index into structure with pennames and width */

	int i, k;
	char Lname[GMT_TEXT_LEN];

	strcpy (Lname, name);
	GMT_str_tolower (Lname);
	for (i = 0, k = -1; k < 0 && i < GMT_N_PEN_NAMES; i++) if (!strcmp (Lname, GMT_penname[i].name)) k = i;

	return (k);
}

void GMT_init_pen (struct GMT_PEN *pen, double width)
{
	/* Sets default black solid pen of given width in points */

	pen->width = width;
	pen->rgb[0] = gmtdefs.basemap_frame_rgb[0];
	pen->rgb[1] = gmtdefs.basemap_frame_rgb[1];
	pen->rgb[2] = gmtdefs.basemap_frame_rgb[2];
	pen->texture[0] = 0;
	pen->offset = 0.0;
}

void GMT_old2newpen (char *line)
{
	int i, j, n_slash, t_pos, s_pos, texture_unit = 0;
	BOOLEAN got_pen = FALSE;
	double texture_scale = 1.0, width;
	char pstring[GMT_LONG_TEXT], pcolor[GMT_LONG_TEXT], ptexture[GMT_LONG_TEXT], buffer[BUFSIZ], saved[BUFSIZ], tmp[2], set_points = 0;

	/* Old Syntax:	[<width][/<color>][t<texture>][p]	p can be anywhere but oughto go just after width */

	/* We will translate the old v3 pen format into the ew format:
	 *
	 *	[<width>[<punit>][,<color>[,<texture><[<tunit>]]]
	 *
	 * We do this by separating out the three strings pstring, pcolor, and ptexture
	 */

	strcpy (saved, line);	/* Save original string */
	s_pos = t_pos = -1;
	i = 0;
	tmp[1] = '\0';
	memset ((void *)pstring,  0, (size_t)(GMT_LONG_TEXT*sizeof(char)));
	memset ((void *)pcolor,   0, (size_t)(GMT_LONG_TEXT*sizeof(char)));
	memset ((void *)ptexture, 0, (size_t)(GMT_LONG_TEXT*sizeof(char)));

	while (line[i] && (line[i] == '.' || isdigit ((int)line[i]))) i++;	/* scanning across valid characters for a pen width */

	if (i) {	/* Case 1: i > 0 which means a numerical width was specified */
		if (strchr ("cimp", line[i])) i++;	/* Got a trailing c|i|m|p for pen width unit */
		strncpy (pstring, line, i);
		got_pen = TRUE;
		j = i;
	}
	else {	/* Here, i == 0 and line[0] == '/' or 't' or first letter of a pen name (f|t) [faint|thin|thick|fat] */
		if (line[0] == '/') {	/* No pen given, / is just the start of /<color> */
			pstring[0] = 0;
			s_pos = j = 0;
		}
		else if (line[0] == 't' && (line[1] == 'a' || line[1] == 'o' || isdigit((int)line[1]))) {	/* This is t<texture> so no pen given */
			pstring[0] = 0;
			t_pos = j = 0;
		}
		else {	/* Must be pen name.  Determine end of it; tricky because of the t<texture> format */
			for (j = i + 1, n_slash = 0; line[j] && n_slash == 0; j++) if (line[j] == '/') n_slash = j;
			if (n_slash) {	/* Reached /<color> which means we can find the end of the pen width string */
				s_pos = n_slash;
				strncpy (pstring, line, s_pos);
			}
			else {	/* Either line ends in a t<texture> or nothing. j is now strlen (line) */
				j--;	/* Position of last character in line */
				if (strchr ("cimp", line[j])) {	/* Got a trailing c|i|m|p for unit appended to texture */
					texture_unit = GMT_penunit (line[j], &texture_scale);
					set_points = line[j];
					j--;
				}
				if (line[j-1] == 't' && (line[j] == 'a' || line[j] == 'o')) {	/* Trailing ta|to[p] */
					t_pos = j - 1;
					strncpy (pstring, line, t_pos);
				}
				else if (strchr (line, ':')) {	/* Trailing t<pattern>:<offset>[p], rewind to 't' */
					while (line[j] && line[j] != 't') j--;
					t_pos = j;
					strncpy (pstring, line, t_pos);
				}
				else	/* Nothing; all we were given was a pen name */
					strcpy (pstring, line);
			}
		}
	}

	i = j;	/* First character position AFTER the pen width (if any) */

	/* Then look for slash which indicate start of color information */

	if (s_pos == -1) { /* We have not yet searched for start of /<color>, if present */
		for (j = i, n_slash = 0; line[j]; j++) if (line[j] == '/') {
			n_slash++;
			if (s_pos < 0) s_pos = j;	/* First slash position (but keep counting the slashes) */
		}
	}

	/* Finally see if a texture is given */

	if (t_pos == -1) { /* Not yet found start of t<texture>, if present */
		for (j = i; line[j] && t_pos == -1; j++) if (line[j] == 't') t_pos = j;
	}

	if (t_pos >= 0) {	/* Texture was specified */
		t_pos++;	/* Step over the leading 't' */
		if (s_pos > t_pos)	/* User specified color AFTER texture */
			strncpy (ptexture, &line[t_pos], s_pos - t_pos);
		else
			strcpy (ptexture, &line[t_pos]);
		if (strchr ("cimp", ptexture[strlen(ptexture)-1])) {	/* c|i|m|p given after texture */
			set_points = ptexture[strlen(ptexture)-1];
			texture_unit = GMT_penunit (set_points, &texture_scale);
		}
	}
	else
		ptexture[0] = '\0';

	if (s_pos >= 0) {	/* Got color of pen */
		s_pos++;	/* Step over the leading '/' */
		if (t_pos >= 0 && t_pos > s_pos)	/* color ends with a texture specification at t_pos */
			strncpy (pcolor, &line[s_pos], t_pos - s_pos - 1);
		else		/* Nothing follows the color */
			strcpy (pcolor, &line[s_pos]);
	}

	if (got_pen && set_points) {	/* The pattern string said values are in given units; use for width if not set */
		if (!pstring[0]) {	/* No width given, set default width and indicate the texture unit as width unit */
			width = GMT_PENWIDTH / (GMT_u2u[texture_unit][GMT_PT] * texture_scale);
			sprintf (pstring, "%g%c", width, set_points);
		}
		else if (!strchr ("cimp", pstring[strlen(pstring)-1])) {	/* No c|i|m|p given after pen width */
			tmp[0] = set_points;
			strcat (pstring, tmp);	/* Append unit used for texture */
		}
	}

	/* Last-minute sanity check for "quick-n-dirty" usage */

	if (GMT_is_penwidth (saved)) {		/* Stand-alone pen width only */
		strcpy (pstring, saved);
		pcolor[0] = ptexture[0] = '\0';
	}
	else if (GMT_is_color (saved,2)) {	/* Stand-alone pen color only.  Only 0-2 slashes allowed in GMT 3.x */
		strcpy (pcolor, saved);
		pstring[0] = ptexture[0] = '\0';
	}
	else if (GMT_is_texture (saved)) {	/* Stand-alone pen texture only */
		strcpy (ptexture, saved);
		pstring[0] = pcolor[0] = '\0';
	}

	/* Build newstyle pen specification string */

	sprintf (buffer, "%s,", pstring);
	strcat (buffer, pcolor);
	strcat (buffer, ",");
	strcat (buffer, ptexture);
	for (i = strlen(buffer)-1; buffer[i] && buffer[i] == ','; i--);	/* Get rid of trailing commas, if any */
	buffer[++i] = '\0';
	if (gmtdefs.verbose == 2) fprintf (stderr, "%s: Old-style pen %s translated to %s\n", GMT_program, saved, buffer);
	strcpy (line, buffer);
}

int GMT_getpenwidth (char *line, int *pen_unit, double *pen_scale, struct GMT_PEN *P) {
	int n;

	/* SYNTAX for pen width:  <floatingpoint>[p] or <name> [fat, thin, etc] */

	if (!line[0]) {	/* Nothing given, set default pen width and units/scale */
		P->width = GMT_PENWIDTH;
		*pen_unit = GMT_INCH;
		*pen_scale = 1.0 / gmtdefs.dpi;
		return (GMT_NOERROR);
	}

	if ((line[0] == '.' && (line[1] >= '0' && line[1] <= '9')) || (line[0] >= '0' && line[0] <= '9')) { /* <floatingpoint>[<unit>] */
		/* Check for pen thickness unit at end */
		n = strlen (line) - 1;	/* Position of last character in string */
		*pen_unit = GMT_penunit (line[n], pen_scale);
		P->width = atof (line) * GMT_u2u[*pen_unit][GMT_PT] * (*pen_scale);
	}
	else {	/* Pen name was given - these refer to fixed widths in points */
		if ((n = GMT_name2pen (line)) < 0) {
			fprintf (stderr, "%s: Pen name %s not recognized!\n", GMT_program, line);
			GMT_exit (EXIT_FAILURE);
		}
		P->width = GMT_penname[n].width;
		*pen_unit = GMT_PT;
		*pen_scale = 1.0;	/* Default scale */
	}
	return (GMT_NOERROR);
}

int GMT_penunit (char c, double *pen_scale)
{
	int unit;
	*pen_scale = 1.0;
	if (c == 'p')
		unit = GMT_PT;
	else if (c == 'i')
		unit = GMT_INCH;
	else if (c == 'c')
		unit = GMT_CM;
	else if (c == 'm')
		unit = GMT_M;
	else {	/* For pens, the default unit is dpi; must apply scaling to get inch first */
		unit = GMT_INCH;
		(*pen_scale) = 1.0 / gmtdefs.dpi;
	}
	return (unit);
}

int GMT_getpen (char *buffer, struct GMT_PEN *P)
{
	int i, n, pen_unit = GMT_PT;
	double pen_scale = 1.0;
	char pen[GMT_LONG_TEXT], color[GMT_LONG_TEXT], texture[GMT_LONG_TEXT], line[BUFSIZ];

	strcpy (line, buffer);	/* Work on a copy of the arguments */
	GMT_chop (line);	/* Remove trailing CR, LF and propoerly NULL-terminate the string */
	if (!strchr (line, ',')) {	/* Most likely old-style pen specification.  Translate */
		GMT_old2newpen (line);
	}

	/* Processes new pen specifications now given as [pen[<punit>][,color[,texture[<tunit>]]] */

	memset ((void *)pen, 0, (size_t)(GMT_LONG_TEXT*sizeof(char)));
	memset ((void *)color, 0, (size_t)(GMT_LONG_TEXT*sizeof(char)));
	memset ((void *)texture, 0, (size_t)(GMT_LONG_TEXT*sizeof(char)));
	for (i = 0; line[i]; i++) if (line[i] == ',') line[i] = ' ';	/* Replace , with space */
	n = sscanf (line, "%s %s %s", pen, color, texture);
	for (i = 0; line[i]; i++) if (line[i] == ' ') line[i] = ',';	/* Replace space with , */
	if (n == 2) {	/* Could be pen,color  pen,[,]texture, or [,]color,texture */
		if (line[0] == ',' || (GMT_is_color(pen,3) && GMT_is_texture(color))) {	/* Got [,]color,texture which got stored in pen, color */
			strcpy (texture, color);
			strcpy (color, pen);
			pen[0] = '\0';
		}
		else if ((GMT_is_penwidth(pen) && GMT_is_texture(color))) {	/* Got pen,texture which got stored in pen, color */
			strcpy (texture, color);
			color[0] = '\0';
		}
		else if (strstr(line, ",,") || GMT_is_texture(color)) {	/* Got pen[,],texture so texture got stored in color */
			strcpy (texture, color);
			color[0] = '\0';
		}
		/* unstated else branch means we got pen,color which are stored correctly */
	}
	else if (n == 1) {	/* Could be pen  [,]color or [,,]texture */
		if (strstr (line, ",,") || GMT_is_texture (line)) {	/* Got [,,]texture so texture got stored in pen */
			strcpy (texture, pen);
			pen[0] = color[0] = '\0';
		}
		else if (line[0] == ',' || GMT_is_color(line,3)) {	/* Got [,]color so color got stored in pen */
			strcpy (color, pen);
			pen[0] = '\0';
		}
		/* unstated else branch means we got pen which is stored correctly */
	}
	/* unstated else branch means we got all 3: pen,color,texture */

	GMT_init_pen (P, GMT_PENWIDTH);	/* Default pen */

	GMT_getpenwidth (pen, &pen_unit, &pen_scale, P);	/* Assign pen width if given */
	GMT_getrgb (color, P->rgb);				/* Assign color if given */
	GMT_gettexture (texture, pen_unit, pen_scale, P);	/* Get texture, if given */

	return (P->width < 0.0 || GMT_check_rgb (P->rgb));
}

BOOLEAN GMT_is_penwidth (char *word)
{
	int n;

	/* Returns TRUE if we are sure the word is a penwidth string - else FALSE.
	 * width syntax is <penname> or <floatingpoint>[<unit>] */

	n = strlen (word);
	if (n == 0) return (FALSE);

	n--;
	if (strchr ("cimp", word[n])) n--;	/* Reduce length by 1; the unit character */
	if (n < 0) return (FALSE);		/* word only contained a unit character? */
	if (GMT_name2pen (word) >= 0) return (TRUE);	/* Valid pen name */
	while (n >= 0 && (word[n] == '.' || isdigit((int)word[n]))) n--;	/* Wind down as long as we find . or integers */
	return (n == -1);	/* TRUE if we only found ploating point FALSE otherwise */
}

BOOLEAN GMT_is_texture (char *word)
{
	int n;

	/* Returns TRUE if we are sure the word is a texture string - else FALSE.
	 * texture syntax is a|o|<pattern>:<phase>|<string made up of -|. only>[<unit>] */

	n = strlen (word);
	if (n == 0) return (FALSE);

	n--;
	if (strchr ("cimp", word[n])) n--;	/* Reduce length by 1; the unit character */
	if (n < 0) return (FALSE);		/* word only contained a unit character? */
	if (n == 0) {
		if (word[0] == '-' || word[0] == 'a' || word[0] == '.' || word[0] == 'o') return (TRUE);
		return (FALSE);	/* No other 1-char texture patterns possible */
	}
	if (strchr(word,'t')) return (FALSE);	/* Got a t somewhere */
	if (strchr(word,':')) return (TRUE);	/* Got <pattern>:<phase> */
	while (n >= 0 && (word[n] == '-' || word[n] == '.')) n--;	/* Wind down as long as we find - or . */
	return (n == -1);	/* TRUE if we only found -/., FALSE otherwise */
}

BOOLEAN GMT_is_color (char *word, int max_slashes)
{
	int i, k, n, n_hyphen = 0;

	/* Returns TRUE if we are sure the word is a color string - else FALSE.
	 * color syntax is <gray>|<r/g/b>|<h-s-v>/<c/m/y/k>/<colorname>.
	 * NOTE: we are not checking if the values are kosher; just the pattern  */

	n = strlen (word);
	if (n == 0) return (FALSE);

	if (GMT_colorname2index (word) >= 0) return (TRUE);	/* Valid color name */
	if (strchr(word,'t')) return (FALSE);		/* Got a t somewhere */
	if (strchr(word,':')) return (FALSE);		/* Got a : somewhere */
	if (strchr(word,'c')) return (FALSE);		/* Got a c somewhere */
	if (strchr(word,'i')) return (FALSE);		/* Got a i somewhere */
	if (strchr(word,'m')) return (FALSE);		/* Got a m somewhere */
	if (strchr(word,'p')) return (FALSE);		/* Got a p somewhere */
	for (i = k = 0; word[i]; i++) if (word[i] == '/') k++;
	if (k == 1 || k > max_slashes) return (FALSE);	/* No color spec takes only 1 slash */
	if ((k == 2 || k == 3) && k <= max_slashes) return (TRUE);		/* Only color (r/g/b [and c/m/y/k if max_slashes = 3]) may have slashes */
	n--;
	while (n >= 0 && (word[n] == '-' || word[n] == '.' || isdigit ((int)word[n]))) {
		if (word[n] == '-') n_hyphen++;
		n--;	/* Wind down as long as we find -,., or digits */
	}
	return (n == -1 && n_hyphen == 2);	/* TRUE if we only found h-s-v and FALSE otherwise */
}

BOOLEAN GMT_is_pattern (char *word) {
	/* Returns TRUE if the word is a pattern specification P|p<dpi>/<pattern>[:B<color>[F<color>]] */

	if (strchr (word, ':')) return (TRUE);			/* Only patterns may have a colon */
	if (!(word[0] == 'P' || word[0] == 'p')) return FALSE;	/* Patterns must start with P or p */
	if (!strchr (word, '/')) return (FALSE);		/* Patterns separate dpi and pattern with a slash */
	/* Here we know we start with P|p and there is a slash - this can only be a pattern specification */
	return (TRUE);
}

int GMT_gettexture (char *line, int unit, double scale, struct GMT_PEN *P) {
	int i, n, pos;
	double width, pen_scale;
	char tmp[GMT_LONG_TEXT], string[BUFSIZ], ptr[BUFSIZ];

	if (!line[0]) return (GMT_NOERROR);	/* Nothing to do */
	pen_scale = scale;
	n = strlen (line) - 1;
	if (strchr ("cimp", line[n])) {	/* Separate unit given to texture string */
		unit = GMT_penunit (line[n], &pen_scale);
	}

	width = (P->width < GMT_SMALL) ? GMT_PENWIDTH : P->width;
	if (line[0] == 'o') {	/* Default Dotted */
		sprintf (P->texture, "%g %g", width, 4.0 * width);
		P->offset = 0.0;
	}
	else if (line[0] == 'a') {	/* Default Dashed */
		sprintf (P->texture, "%g %g", 8.0 * width, 4.0 * width);
		P->offset = 4.0 * width;
	}
	else if (isdigit ((int)line[0])) {	/* Specified numeric pattern will start with an integer*/
		int c_pos;

		for (i = 1, c_pos = 0; line[i] && c_pos == 0; i++) if (line[i] == ':') c_pos = i;
		if (c_pos == 0) {
			fprintf (stderr, "%s: Warning: Pen texture %s do not follow format <pattern>:<phase>. <phase> set to 0\n", GMT_program, line);
			P->offset = 0.0;
		}
		else {
			line[c_pos] = ' ';
			sscanf (line, "%s %lf", P->texture, &P->offset);
			line[c_pos] = ':';
		}
		for (i = 0; P->texture[i]; i++) if (P->texture[i] == '_') P->texture[i] = ' ';

		/* Must convert given units to points */

		memset ((void *)string, 0, (size_t)BUFSIZ);
		pos = 0;
		while ((GMT_strtok (P->texture, " ", &pos, ptr))) {
			sprintf (tmp, "%g ", (atof (ptr) * GMT_u2u[unit][GMT_PT] * scale));
			strcat (string, tmp);
		}
		string[strlen (string) - 1] = 0;
		if (strlen (string) >= GMT_PEN_LEN) {
			fprintf (stderr, "%s: GMT Error: Pen attributes too long!\n", GMT_program);
			GMT_exit (EXIT_FAILURE);
		}
		strcpy (P->texture, string);
		P->offset *= GMT_u2u[unit][GMT_PT] * scale;
	}
	else  {	/* New way of building it up with - and . */
		P->texture[0] = '\0';
		P->offset = 0.0;
		for (i = 0; line[i]; i++) {
			if (line[i] == '-') { /* Dash */
				sprintf (tmp, "%g %g ", 8.0 * width, 4.0 * width);
				strcat (P->texture, tmp);
			}
			else if (line[i] == '.') { /* Dot */
				sprintf (tmp, "%g %g ", width, 4.0 * width);
				strcat (P->texture, tmp);
			}
		}
		P->texture[strlen(P->texture)-1] = '\0';	/* Chop off trailing space */
	}
	return (GMT_NOERROR);
}

#define GMT_INC_IS_M		1
#define GMT_INC_IS_KM		2
#define GMT_INC_IS_MILES	4
#define GMT_INC_IS_NMILES	8
#define GMT_INC_IS_NNODES	16
#define GMT_INC_IS_EXACT	32
#define GMT_INC_UNITS		15

int GMT_getinc (char *line, double *dx, double *dy)
{	/* Special case of getincn use where n is two. */

	int n;
	double inc[2];

	/* Syntax: -I<xinc>[m|c|e|i|k|n|+|=][/<yinc>][m|c|e|i|k|n|+|=]
	 * Units: m = minutes
	 *	  c = seconds
	 *	  e = meter [Convert to degrees]
	 *	  i = miles [Convert to degrees]
	 *	  k = km [Convert to degrees]
	 *	  n = nautical miles [Convert to degrees]
	 * Flags: = = Adjust -R to fit exact -I [Default modifies -I to fit -R]
	 *	  + = incs are actually nx/ny - convert to get xinc/yinc
	 */

	n = GMT_getincn (line, inc, 2);
	*dx = inc[0] ; *dy = inc[1];
	if (n == 1) {	/* Must copy y info from x */
		*dy = *dx;
		GMT_inc_code[1] = GMT_inc_code[0];	/* Use exact inc codes for both x and y */
	}

	if (GMT_inc_code[0] & GMT_INC_IS_NNODES && GMT_inc_code[0] & GMT_INC_UNITS) {
		fprintf (stderr, "%s: ERROR: number of x nodes cannot have units\n", GMT_program);
		GMT_exit (EXIT_FAILURE);
	}
	if (GMT_inc_code[1] & GMT_INC_IS_NNODES && GMT_inc_code[1] & GMT_INC_UNITS) {
		fprintf (stderr, "%s: ERROR: number of y nodes cannot have units\n", GMT_program);
		GMT_exit (EXIT_FAILURE);
	}
	return (GMT_NOERROR);
}

int GMT_getincn (char *line, double inc[], int n)
{
	int last, i, pos;
	char p[BUFSIZ];
	double scale = 1.0;

	/* Dechipers dx/dy/dz/dw/du/dv/... increment strings with n items */

	memset ((void *)inc, 0, (size_t)(n * sizeof (double)));

	i = pos = GMT_inc_code[0] = GMT_inc_code[1] = 0;

	while (i < n && (GMT_strtok (line, "/", &pos, p))) {
		last = strlen (p) - 1;
		if (p[last] == '=') {	/* Let -I override -R */
			p[last] = 0;
			if (i < 2) GMT_inc_code[i] |= GMT_INC_IS_EXACT;
			last--;
		}
		else if (p[last] == '+' || p[last] == '!') {	/* Number of nodes given, determine inc from domain (! added since documentation mentioned this once... */
			p[last] = 0;
			if (i < 2) GMT_inc_code[i] |= GMT_INC_IS_NNODES;
			last--;
		}
		switch (p[last]) {
			case 'm':
			case 'M':	/* Gave arc minutes */
				p[last] = 0;
				scale = GMT_MIN2DEG;
				break;
			case 'c':
			case 'C':	/* Gave arc seconds */
				p[last] = 0;
				scale = GMT_SEC2DEG;
				break;
			case 'e':
			case 'E':	/* Gave meters along mid latitude */
				p[last] = 0;
				if (i < 2) GMT_inc_code[i] |= GMT_INC_IS_M;
				break;
			case 'K':	/* Gave km along mid latitude */
			case 'k':
				p[last] = 0;
				if (i < 2) GMT_inc_code[i] |= GMT_INC_IS_KM;
				break;
			case 'I':	/* Gave miles along mid latitude */
			case 'i':
				p[last] = 0;
				if (i < 2) GMT_inc_code[i] |= GMT_INC_IS_MILES;
				break;
			case 'N':	/* Gave nautical miles along mid latitude */
			case 'n':
				p[last] = 0;
				if (i < 2) GMT_inc_code[i] |= GMT_INC_IS_NMILES;
				break;
			default:	/* No special flags or units */
				scale = 1.0;
				break;
		}
		if ( (sscanf(p, "%lf", &inc[i])) != 1) {
			fprintf (stderr, "%s: ERROR: Unable to decode %s as a floating point number\n", GMT_program, p);
			GMT_exit (EXIT_FAILURE);
		}
		inc[i] *= scale;
		i++;	/* Goto next increment */
	}

	return (i);	/* Returns the number of increments found */
}

double GMT_getradius (char *line)
{
	int last, save = 0;
	double radius, scale = 1.0;

	/* Dechipers a single radius argument */

	last = strlen (line) - 1;
	switch (line[last]) {
		case 'm':
		case 'M':	/* Gave arc minutes */
			save = line[last];
			line[last] = 0;
			scale = GMT_MIN2DEG;
			break;
		case 'c':
		case 'C':	/* Gave arc seconds */
			save = line[last];
			line[last] = 0;
			scale = GMT_SEC2DEG;
			break;
		default:	/* No special flags or units */
			scale = 1.0;
			break;
	}
	if ( (sscanf(line, "%lf", &radius)) != 1) {
		fprintf (stderr, "%s: ERROR: Unable to decode %s as a floating point number\n", GMT_program, line);
		radius = GMT_d_NaN;
	}
	if (save) line[last] = save;

	return (radius * scale);
}

void GMT_RI_prepare (struct GRD_HEADER *h)
{
	int one_or_zero;
	double s = 1.0, f, m_pr_degree;

	/* May have to adjust -R -I depending on how GMT_inc_code was set */

	one_or_zero = !h->node_offset;
	m_pr_degree = project_info.M_PR_DEG;
	h->xy_off = 0.5 * h->node_offset;	/* Use to calculate mean location of block */

	/* XINC AND XMIN/XMAX CHECK FIRST */

	if (GMT_inc_code[0] == 0) {	/* Standard -R -I given, just set nx */
		h->nx = GMT_get_n (h->x_min, h->x_max, h->x_inc, h->node_offset);
	}
	else if (GMT_inc_code[0] & GMT_INC_IS_NNODES) {	/* Got nx */
		h->nx = irint (h->x_inc);
		h->x_inc = GMT_get_inc (h->x_min, h->x_max, h->nx, h->node_offset);
		if (gmtdefs.verbose) fprintf (stderr, "%s: Given nx implies x_inc = %g\n", GMT_program, h->x_inc);
	}
	else {	/* Got funny units */
		switch (GMT_inc_code[0] & GMT_INC_UNITS) {
			case GMT_INC_IS_M:	/* Meter */
				s = 1.0;
				break;
			case GMT_INC_IS_KM:	/* km */
				s = 1000.0;
				break;
			case GMT_INC_IS_MILES:	/* miles */
				s = 1609.433;
				break;
			case GMT_INC_IS_NMILES:	/* nmiles */
				s = 1852.0;
				break;
		}
		if (GMT_inc_code[0] & GMT_INC_IS_EXACT && !(GMT_inc_code[0] & GMT_INC_UNITS)) {
			f = m_pr_degree = 1.0;
		}
		else
			f = cosd (0.5 * (h->y_max + h->y_min));	/* Latitude scaling of E-W distances */

		h->x_inc = h->x_inc * s / (m_pr_degree * f);
		if (gmtdefs.verbose) fprintf (stderr, "%s: Distance to degree conversion implies x_inc = %g\n", GMT_program, h->x_inc);
		h->nx = GMT_get_n (h->x_min, h->x_max, h->x_inc, h->node_offset);
	}
	if (GMT_inc_code[0] & GMT_INC_IS_EXACT) {	/* Want to keep dx exactly as given; adjust x_max accordingly */
		s = (h->x_max - h->x_min) - h->x_inc * (h->nx - one_or_zero);
		if (fabs (s) > 0.0) {
			h->x_max -= s;
			if (gmtdefs.verbose) fprintf (stderr, "%s: x_max adjusted to %g\n", GMT_program, h->x_max);
		}
	}
	else if (!(GMT_inc_code[0] & GMT_INC_IS_NNODES)) {	/* Adjust x_inc to exactly fit west/east */
		s = h->x_max - h->x_min;
		h->nx = irint (s / h->x_inc);
		f = s / h->nx;
		h->nx += one_or_zero;
		if (fabs (f - h->x_inc) > 0.0) {
			h->x_inc = f;
			if (gmtdefs.verbose) fprintf (stderr, "%s: Given domain implies x_inc = %g\n", GMT_program, h->x_inc);
		}
	}

	/* YINC AND YMIN/YMAX CHECK SECOND */
	s = 1.0;	/* s was used above with a different purpose */

	if (GMT_inc_code[1] == 0) {	/* Standard -R -I given, just set ny */
		h->ny = GMT_get_n (h->y_min, h->y_max, h->y_inc, h->node_offset);
	}
	else if (GMT_inc_code[1] & GMT_INC_IS_NNODES) {	/* Got ny */
		h->ny = irint (h->y_inc);
		h->y_inc = GMT_get_inc (h->y_min, h->y_max, h->ny, h->node_offset);
		if (gmtdefs.verbose) fprintf (stderr, "%s: Given ny implies y_inc = %g\n", GMT_program, h->y_inc);
		return;
	}
	else {	/* Got funny units */
		switch (GMT_inc_code[1] & GMT_INC_UNITS) {
			case GMT_INC_IS_M:	/* Meter */
				s = 1.0;
				break;
			case GMT_INC_IS_KM:	/* km */
				s = 1000.0;
				break;
			case GMT_INC_IS_MILES:	/* miles */
				s = 1609.433;
				break;
			case GMT_INC_IS_NMILES:	/* nmiles */
				s = 1852.0;
				break;
		}
		if (GMT_inc_code[1] & GMT_INC_IS_EXACT && !(GMT_inc_code[1] & GMT_INC_UNITS))
			m_pr_degree = 1.0;

		else	/* m_pr_degree might have been reset to 1 in the XINC ... case */
			m_pr_degree = project_info.M_PR_DEG;

		h->y_inc = (h->y_inc == 0.0) ? h->x_inc : h->y_inc * s / m_pr_degree;
		if (gmtdefs.verbose) fprintf (stderr, "%s: Distance to degree conversion implies y_inc = %g\n", GMT_program, h->y_inc);
		h->ny = GMT_get_n (h->y_min, h->y_max, h->y_inc, h->node_offset);
	}

	if (GMT_inc_code[1] & GMT_INC_IS_EXACT) {	/* Want to keep dy exactly as given; adjust y_max accordingly */
		s = (h->y_max - h->y_min) - h->y_inc * (h->ny - one_or_zero);
		if (fabs (s) > 0.0) {
			h->y_max -= s;
			if (gmtdefs.verbose) fprintf (stderr, "%s: y_max adjusted to %g\n", GMT_program, h->y_max);
		}
	}
	else if (!(GMT_inc_code[1] & GMT_INC_IS_NNODES)) {	/* Adjust y_inc to exactly fit south/north */
		s = h->y_max - h->y_min;
		h->ny = irint (s / h->y_inc);
		f = s / h->ny;
		h->ny += one_or_zero;
		if (fabs (f - h->y_inc) > 0.0) {
			h->y_inc = f;
			if (gmtdefs.verbose) fprintf (stderr, "%s: Given domain implies y_inc = %g\n", GMT_program, h->y_inc);
		}
	}
}

int GMT_set_cpt_path (char *CPT_file, char *table)
{
	char stem[GMT_TEXT_LEN];
	char *l, *ok;

	/* Try table[.cpt] */
	strcpy (stem, table);
	if ((l = strstr (stem, ".cpt"))) *l = 0;
	ok = GMT_getsharepath ("cpt", stem, ".cpt", CPT_file);

	/* Alternatively, look for GMT_table[.cpt] */
	if (!ok) {
		sprintf (stem, "GMT_%s", table);
		if ((l = strstr (stem, ".cpt"))) *l = 0;
		ok = GMT_getsharepath ("cpt", stem, ".cpt", CPT_file);
	}

	/* Have we found something? */
	if (!ok)
		fprintf (stderr, "%s: ERROR: Cannot find colortable %s\n", GMT_program, table);
	else
		if (gmtdefs.verbose) fprintf (stderr, "%s: Reading colortable %s\n", GMT_program, CPT_file);
	return (!ok);
}

int GMT_read_cpt (char *cpt_file)
{
	/* Opens and reads a color palette file in RGB, HSV, or CMYK of arbitrary length */

	int n = 0, i, nread, annot, n_alloc = GMT_SMALL_CHUNK, color_model, id, n_cat_records = 0;
	long k;
	double dz;
	BOOLEAN gap, error = FALSE;
	char T0[GMT_TEXT_LEN], T1[GMT_TEXT_LEN], T2[GMT_TEXT_LEN], T3[GMT_TEXT_LEN], T4[GMT_TEXT_LEN];
	char T5[GMT_TEXT_LEN], T6[GMT_TEXT_LEN], T7[GMT_TEXT_LEN], T8[GMT_TEXT_LEN], T9[GMT_TEXT_LEN];
	char line[BUFSIZ], option[GMT_TEXT_LEN], c;
	FILE *fp = NULL;

	if (!cpt_file)
		fp = GMT_stdin;
	else if ((fp = fopen (cpt_file, "r")) == NULL) {
		fprintf (stderr, "%s: GMT Fatal Error: Cannot open color palette table %s\n", GMT_program, cpt_file);
		GMT_exit (EXIT_FAILURE);
	}

	GMT_lut = (struct GMT_LUT *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (struct GMT_LUT), "GMT_read_cpt");

	GMT_b_and_w = GMT_gray = TRUE;
	GMT_continuous = GMT_cpt_pattern = GMT_categorical = FALSE;
	color_model = gmtdefs.color_model;		/* Save the original setting since it may be modified by settings in the CPT file */

	while (!error && fgets (line, BUFSIZ, fp)) {

		if (strstr (line, "COLOR_MODEL")) {	/* cpt file overrides default color model */
			if (strstr (line, "+RGB") || strstr (line, "+rgb"))
				gmtdefs.color_model = GMT_RGB;
			else if (strstr (line, "RGB") || strstr (line, "rgb"))
				gmtdefs.color_model = GMT_READ_RGB;
			else if (strstr (line, "+HSV") || strstr (line, "+hsv"))
				gmtdefs.color_model = GMT_HSV;
			else if (strstr (line, "HSV") || strstr (line, "hsv"))
				gmtdefs.color_model = GMT_READ_HSV;
			else if (strstr (line, "+CMYK") || strstr (line, "+cmyk"))
				gmtdefs.color_model = GMT_CMYK;
			else if (strstr (line, "CMYK") || strstr (line, "cmyk"))
				gmtdefs.color_model = GMT_READ_CMYK;
			else {
				fprintf (stderr, "%s: GMT Fatal Error: unrecognized COLOR_MODEL in color palette table %s\n", GMT_program, cpt_file);
				GMT_exit (EXIT_FAILURE);
			}
		}

		c = line[0];
		if (c == '#' || c == '\n') continue;	/* Comment or blank */

		T1[0] = T2[0] = T3[0] = T4[0] = T5[0] = T6[0] = T7[0] = T8[0] = T9[0] = 0;
		switch (c) {
			case 'B':
				id = 0;
				break;
			case 'F':
				id = 1;
				break;
			case 'N':
				id = 2;
				break;
			default:
				id = 3;
				break;
		}

		if (id < 3) {	/* Foreground, background, or nan color */
			if (GMT_cpt_flags & 4) continue; /* Suppress parsing B, F, N lines when bit 2 of GMT_cpt_flags is set */
			if ((nread = sscanf (&line[2], "%s %s %s %s", T1, T2, T3, T4)) < 1) error = TRUE;
			if (T1[0] == 'p' || T1[0] == 'P') {	/* Gave a pattern */
				GMT_bfn[id].fill = (struct GMT_FILL *) GMT_memory (VNULL, 1, sizeof (struct GMT_FILL), GMT_program);
				if (GMT_getfill (T1, GMT_bfn[id].fill)) {
					fprintf (stderr, "%s: GMT Fatal Error: CPT Pattern fill (%s) not understood!\n", GMT_program, T1);
					GMT_exit (EXIT_FAILURE);
				}
				GMT_cpt_pattern = TRUE;
			}
			else {	/* Shades, RGB, HSV, or CMYK */
				if (T1[0] == '-')	/* Skip this slice */
					GMT_bfn[id].skip = TRUE;
				else if (nread == 1) {	/* Gray shade */
					sprintf (option, "%s", T1);
					if (GMT_getrgb (option, GMT_bfn[id].rgb)) error++;
				}
				else if (gmtdefs.color_model & GMT_READ_CMYK) {
					sprintf (option, "%s/%s/%s/%s", T1, T2, T3, T4);
					if (GMT_getrgb (option, GMT_bfn[id].rgb)) error++;
				}
				else {
					sprintf (option, "%s/%s/%s", T1, T2, T3);
					if (GMT_getrgb (option, GMT_bfn[id].rgb)) error++;
				}
			}
			continue;
		}


		/* Here we have regular z-slices.  Allowable formats are
		 *
		 * kei <fill> <label>	for categorical data
		 * z0 - z1 - [LUB] ;<label>
		 * z0 pattern z1 - [LUB] ;<label>
		 * z0 r0 z1 r1 [LUB] ;<label>
		 * z0 r0 g0 b0 z1 r1 g1 b1 [LUB] ;<label>
		 * z0 h0 s0 v0 z1 h1 s1 v1 [LUB] ;<label>
		 * z0 c0 m0 y0 k0 z1 c1 m1 y1 k1 [LUB] ;<label>
		 *
		 * z can be in any format (float, dd:mm:ss, dateTclock)
		 */

		/* First determine if a label is given */

		if ((k = (long)strchr (line, ';'))) {	/* OK, find the label and chop it off */
			k -= (long)line;	/* Position of the column */
			GMT_lut[n].label = (char *)GMT_memory (VNULL, strlen (line) - k, sizeof (char), GMT_program);
			strcpy (GMT_lut[n].label, &line[k+1]);
			GMT_lut[n].label[strlen(line)-k-2] = '\0';	/* Strip off trailing return */
			line[k] = '\0';	/* Chop label off from line */
		}

		/* Determine if psscale need to label these steps by examining for the optional L|U|B character at the end */

		c = line[strlen(line)-2];
		if (c == 'L')
			GMT_lut[n].annot = 1;
		else if (c == 'U')
			GMT_lut[n].annot = 2;
		else if (c == 'B')
			GMT_lut[n].annot = 3;
		if (GMT_lut[n].annot) line[strlen(line)-2] = '\0';	/* Chop off this information so it does not affect our column count below */

		nread = sscanf (line, "%s %s %s %s %s %s %s %s %s %s", T0, T1, T2, T3, T4, T5, T6, T7, T8, T9);	/* Hope to read 4, 8, or 10 fields */

		if (nread <= 0) continue;								/* Probably a line with spaces - skip */
		if (gmtdefs.color_model & GMT_READ_CMYK && nread != 10) error = TRUE;			/* CMYK should results in 10 fields */
		if (!(gmtdefs.color_model & GMT_READ_CMYK) && !(nread == 4 || nread == 8)) error = TRUE;	/* HSV or RGB should result in 8 fields, gray, patterns, or skips in 4 */
		GMT_scanf_arg (T0, GMT_IS_UNKNOWN, &GMT_lut[n].z_low);
		if (nread == 3 && GMT_IS_ZERO (GMT_lut[n].z_low - irint (GMT_lut[n].z_low))) error = FALSE;	/* Categorical CPT record with integer key */
		GMT_lut[n].skip = FALSE;
		if (T1[0] == '-') {				/* Skip this slice */
			if (nread != 4) {
				fprintf (stderr, "%s: GMT Fatal Error: z-slice to skip not in [z0 - z1 -] format!\n", GMT_program);
				GMT_exit (EXIT_FAILURE);
			}
			GMT_scanf_arg (T2, GMT_IS_UNKNOWN, &GMT_lut[n].z_high);
			GMT_lut[n].skip = TRUE;		/* Don't paint this slice if possible*/
			for (i = 0; i < 3; i++) GMT_lut[n].rgb_low[i] = GMT_lut[n].rgb_high[i] = gmtdefs.page_rgb[i];	/* If you must, use page color */
		}
		else if (nread != 3 && GMT_is_pattern (T1)) {	/* Gave pattern fill */
			GMT_lut[n].fill = (struct GMT_FILL *) GMT_memory (VNULL, 1, sizeof (struct GMT_FILL), GMT_program);
			if (GMT_getfill (T1, GMT_lut[n].fill)) {
				fprintf (stderr, "%s: GMT Fatal Error: CPT Pattern fill (%s) not understood!\n", GMT_program, T1);
				GMT_exit (EXIT_FAILURE);
			}
			else if (nread != 4) {
				fprintf (stderr, "%s: GMT Fatal Error: z-slice with pattern fill not in [z0 pattern z1 -] format!\n", GMT_program);
				GMT_exit (EXIT_FAILURE);
			}
			GMT_scanf_arg (T2, GMT_IS_UNKNOWN, &GMT_lut[n].z_high);
			GMT_cpt_pattern = TRUE;
		}
		else {							/* Shades, RGB, HSV, or CMYK */
			if (nread == 3) {	/* Categorical cpt records with key color label */
				GMT_lut[n].label = (char *)GMT_memory (VNULL, strlen (T2) + 1, sizeof (char), GMT_program);
				strcpy (GMT_lut[n].label, T2);
				GMT_lut[n].z_high = GMT_lut[n].z_low;
				if (GMT_is_pattern (T1)) {	/* Gave pattern fill */
					GMT_lut[n].fill = (struct GMT_FILL *) GMT_memory (VNULL, 1, sizeof (struct GMT_FILL), GMT_program);
					if (GMT_getfill (T1, GMT_lut[n].fill)) {
						fprintf (stderr, "%s: GMT Fatal Error: CPT Pattern fill (%s) not understood!\n", GMT_program, T1);
						GMT_exit (EXIT_FAILURE);
					}
				}
				else {
					if (GMT_getrgb (T1, GMT_lut[n].rgb_low)) error++;
					if (GMT_getrgb (T1, GMT_lut[n].rgb_high)) error++;
				}
				n_cat_records++;
				GMT_categorical = TRUE;
			}
			else if (nread == 4) {	/* gray shades */
				GMT_scanf_arg (T2, GMT_IS_UNKNOWN, &GMT_lut[n].z_high);
				if (GMT_getrgb (T1, GMT_lut[n].rgb_low)) error++;
				if (GMT_getrgb (T3, GMT_lut[n].rgb_high)) error++;
			}
			else if (gmtdefs.color_model & GMT_READ_CMYK) {
				GMT_scanf_arg (T5, GMT_IS_UNKNOWN, &GMT_lut[n].z_high);
				sprintf (option, "%s/%s/%s/%s", T1, T2, T3, T4);
				if (GMT_getrgb (option, GMT_lut[n].rgb_low)) error++;
				sprintf (option, "%s/%s/%s/%s", T6, T7, T8, T9);
				if (GMT_getrgb (option, GMT_lut[n].rgb_high)) error++;
			}
			else if (gmtdefs.color_model & GMT_READ_HSV) {
				GMT_scanf_arg (T4, GMT_IS_UNKNOWN, &GMT_lut[n].z_high);
				sprintf (option, "%s/%s/%s", T1, T2, T3);
				if (GMT_gethsv (option, GMT_lut[n].hsv_low)) error++;
				sprintf (option, "%s/%s/%s", T5, T6, T7);
				if (GMT_gethsv (option, GMT_lut[n].hsv_high)) error++;
				GMT_hsv_to_rgb (GMT_lut[n].rgb_low,  GMT_lut[n].hsv_low[0],  GMT_lut[n].hsv_low[1],  GMT_lut[n].hsv_low[2]);
				GMT_hsv_to_rgb (GMT_lut[n].rgb_high, GMT_lut[n].hsv_high[0], GMT_lut[n].hsv_high[1], GMT_lut[n].hsv_high[2]);
			}
			else {			/* RGB */
				GMT_scanf_arg (T4, GMT_IS_UNKNOWN, &GMT_lut[n].z_high);
				sprintf (option, "%s/%s/%s", T1, T2, T3);
				if (GMT_getrgb (option, GMT_lut[n].rgb_low)) error++;
				sprintf (option, "%s/%s/%s", T5, T6, T7);
				if (GMT_getrgb (option, GMT_lut[n].rgb_high)) error++;
			}

			if (!GMT_categorical) {
				dz = GMT_lut[n].z_high - GMT_lut[n].z_low;
				if (dz == 0.0) {
					fprintf (stderr, "%s: GMT Fatal Error: Z-slice with dz = 0\n", GMT_program);
					GMT_exit (EXIT_FAILURE);
				}
				GMT_lut[n].i_dz = 1.0 / dz;
			}
			if (!GMT_is_gray (GMT_lut[n].rgb_low[0],  GMT_lut[n].rgb_low[1],  GMT_lut[n].rgb_low[2]))  GMT_gray = FALSE;
			if (!GMT_is_gray (GMT_lut[n].rgb_high[0], GMT_lut[n].rgb_high[1], GMT_lut[n].rgb_high[2])) GMT_gray = FALSE;
			if (GMT_gray && !GMT_is_bw(GMT_lut[n].rgb_low[0]))  GMT_b_and_w = FALSE;
			if (GMT_gray && !GMT_is_bw(GMT_lut[n].rgb_high[0])) GMT_b_and_w = FALSE;
			for (i = 0; !GMT_continuous && i < 3; i++) if (GMT_lut[n].rgb_low[i] != GMT_lut[n].rgb_high[i]) GMT_continuous = TRUE;
			for (i = 0; i < 3; i++) GMT_lut[n].rgb_diff[i] = GMT_lut[n].rgb_high[i] - GMT_lut[n].rgb_low[i];	/* Used in GMT_get_rgb_from_z */
			if (! (gmtdefs.color_model & GMT_READ_HSV)) {
				GMT_rgb_to_hsv (GMT_lut[n].rgb_low, &GMT_lut[n].hsv_low[0], &GMT_lut[n].hsv_low[1], &GMT_lut[n].hsv_low[2]);
				GMT_rgb_to_hsv (GMT_lut[n].rgb_high, &GMT_lut[n].hsv_high[0], &GMT_lut[n].hsv_high[1], &GMT_lut[n].hsv_high[2]);
			}
			for (i = 0; i < 3; i++) GMT_lut[n].hsv_diff[i] = GMT_lut[n].hsv_high[i] - GMT_lut[n].hsv_low[i];	/* Used in GMT_get_rgb_from_z */
		}

		n++;
		if (n == n_alloc) {
			i = n_alloc;
			n_alloc += GMT_SMALL_CHUNK;
			GMT_lut = (struct GMT_LUT *) GMT_memory ((void *)GMT_lut, (size_t)n_alloc, sizeof (struct GMT_LUT), "GMT_read_cpt");
			memset ((void *)&GMT_lut[i], 0, (size_t)(GMT_SMALL_CHUNK * sizeof (struct GMT_LUT)));	/* Initialize new structs to zero */
		}
	}

	if (fp != GMT_stdin) fclose (fp);

	if (GMT_categorical && n_cat_records != n) {
		fprintf (stderr, "%s: GMT Fatal Error: Error when decoding %s as categorical cpt file - aborts!\n", GMT_program, cpt_file);
		GMT_exit (EXIT_FAILURE);
	}

	if (error) {
		fprintf (stderr, "%s: GMT Fatal Error: Error when decoding %s - aborts!\n", GMT_program, cpt_file);
		GMT_exit (EXIT_FAILURE);
	}
	if (n == 0) {
		fprintf (stderr, "%s: GMT Fatal Error: CPT file %s has no z-slices!\n", GMT_program, cpt_file);
		GMT_exit (EXIT_FAILURE);
	}

	GMT_lut = (struct GMT_LUT *) GMT_memory ((void *)GMT_lut, (size_t)n, sizeof (struct GMT_LUT), "GMT_read_cpt");
	GMT_n_colors = n;

	if (GMT_categorical) {	/* Set up fake ranges so CPT is continuous */
		for (i = 0; i < GMT_n_colors; i++) {
			if (i == (GMT_n_colors-1)) {
				GMT_lut[i].z_high += 1.0;	/* Upper limit is one up */
			}
			else {
				GMT_lut[i].z_high = GMT_lut[i+1].z_low;
			}
			dz = GMT_lut[i].z_high - GMT_lut[i].z_low;
			if (dz == 0.0) {
				fprintf (stderr, "%s: GMT Fatal Error: Z-slice with dz = 0\n", GMT_program);
				GMT_exit (EXIT_FAILURE);
			}
			GMT_lut[i].i_dz = 1.0 / dz;
		}
	}

	for (i = annot = 0, gap = FALSE; i < GMT_n_colors - 1; i++) {
		if (GMT_lut[i].z_high != GMT_lut[i+1].z_low) gap = TRUE;
		annot += GMT_lut[i].annot;
	}
	annot += GMT_lut[i].annot;
	if (gap) {
		fprintf (stderr, "%s: GMT Fatal Error: Color palette table %s has gaps - aborts!\n", GMT_program, cpt_file);
		GMT_exit (EXIT_FAILURE);
	}
	if (!annot) {	/* Must set default annotation flags */
		for (i = 0; i < GMT_n_colors; i++) GMT_lut[i].annot = 1;
		GMT_lut[i-1].annot = 3;
	}
	for (id = 0; id < 3; id++) {
		if (!GMT_is_gray (GMT_bfn[id].rgb[0], GMT_bfn[id].rgb[1],  GMT_bfn[id].rgb[2]))  GMT_gray = FALSE;
		if (GMT_gray && !GMT_is_bw(GMT_bfn[id].rgb[0]))  GMT_b_and_w = FALSE;
		GMT_rgb_to_hsv (GMT_bfn[id].rgb, &GMT_bfn[id].hsv[0], &GMT_bfn[id].hsv[1], &GMT_bfn[id].hsv[2]);
	}
	if (!GMT_gray) GMT_b_and_w = FALSE;
	if (!(gmtdefs.color_model & (GMT_USE_RGB | GMT_USE_HSV | GMT_USE_CMYK))) gmtdefs.color_model = color_model;	/* Reset to what it was before */

	return (GMT_NOERROR);
}

void GMT_sample_cpt (double z[], int nz, BOOLEAN continuous, BOOLEAN reverse, int log_mode)
{
	/* Resamples the current cpt table based on new z-array.
	 * Old cpt is normalized to 0-1 range and scaled to fit new z range.
	 * New cpt may be continuous and/or reversed.
	 * We write the new cpt table to stdout. */

	int i, j, k, upper, lower, rgb_low[3], rgb_high[3], rgb_fore[3], rgb_back[3];
	BOOLEAN even = FALSE;	/* TRUE when nz is passed as negative */
	double *x, *z_out, a, b, f, x_inc, cmyk_low[4], cmyk_high[4];
	double hsv_low[3], hsv_high[3], hsv_fore[3], hsv_back[3];

	char format[BUFSIZ], code[3] = {'B', 'F', 'N'};
	struct GMT_LUT *lut;

	if (!GMT_continuous && continuous) fprintf (stderr, "%s: Warning: Making a continous cpt from a discrete cpt may give unexpected results!\n", GMT_program);

	if (nz < 0) {	/* Called from grd2cpt which want equal area colors */
		nz = -nz;
		even = TRUE;
	}

	lut = (struct GMT_LUT *) GMT_memory (VNULL, (size_t)GMT_n_colors, sizeof (struct GMT_LUT), GMT_program);

	/* First normalize old cpt file so z-range is 0-1 */

	b = 1.0 / (GMT_lut[GMT_n_colors-1].z_high - GMT_lut[0].z_low);
	a = -GMT_lut[0].z_low * b;

	for (i = 0; i < GMT_n_colors; i++) {	/* Copy/normalize cpt file and reverse if needed */
		lut[i].z_low = a + b * GMT_lut[i].z_low;
		lut[i].z_high = a + b * GMT_lut[i].z_high;
		if (reverse) {
			j = GMT_n_colors - i - 1;
			memcpy ((void *)lut[i].rgb_high, (void *)GMT_lut[j].rgb_low,  (size_t)(3 * sizeof (int)));
			memcpy ((void *)lut[i].rgb_low,  (void *)GMT_lut[j].rgb_high, (size_t)(3 * sizeof (int)));
			memcpy ((void *)lut[i].hsv_high, (void *)GMT_lut[j].hsv_low,  (size_t)(3 * sizeof (double)));
			memcpy ((void *)lut[i].hsv_low,  (void *)GMT_lut[j].hsv_high, (size_t)(3 * sizeof (double)));
		}
		else {
			j = i;
			memcpy ((void *)lut[i].rgb_high, (void *)GMT_lut[j].rgb_high, (size_t)(3 * sizeof (int)));
			memcpy ((void *)lut[i].rgb_low,  (void *)GMT_lut[j].rgb_low,  (size_t)(3 * sizeof (int)));
			memcpy ((void *)lut[i].hsv_high, (void *)GMT_lut[j].hsv_high, (size_t)(3 * sizeof (double)));
			memcpy ((void *)lut[i].hsv_low,  (void *)GMT_lut[j].hsv_low,  (size_t)(3 * sizeof (double)));
		}
	}
	lut[0].z_low = 0.0;			/* Prevent roundoff errors */
	lut[GMT_n_colors-1].z_high = 1.0;

	/* Then set up normalized output locations x */

	x = (double *) GMT_memory (VNULL, (size_t)nz, sizeof(double), GMT_program);
	if (log_mode) {	/* Our z values are actually log10(z), need array with z for output */
		z_out = (double *) GMT_memory (VNULL, (size_t)nz, sizeof(double), GMT_program);
		for (i = 0; i < nz; i++) z_out[i] = pow (10.0, z[i]);
	}
	else
		z_out = z;	/* Just point to the incoming z values */

	if (nz < 2)	/* Want a single color point, assume range 0-1 */
		nz = 2;
	else if (even) {
		x_inc = 1.0 / (nz - 1);
		for (i = 0; i < nz; i++) x[i] = i * x_inc;	/* Normalized z values 0-1 */
	}
	else {	/* As with LUT, translate users z-range to 0-1 range */
		b = 1.0 / (z[nz-1] - z[0]);
		a = -z[0] * b;
		for (i = 0; i < nz; i++) x[i] = a + b * z[i];	/* Normalized z values 0-1 */
	}
	x[0] = 0.0;	/* Prevent bad roundoff */
	x[nz-1] = 1.0;

	/* Start writing cpt file info to stdout */

	if (gmtdefs.color_model & GMT_USE_HSV)
		fprintf (GMT_stdout, "#COLOR_MODEL = +HSV\n");
	else if (gmtdefs.color_model & GMT_READ_HSV)
		fprintf (GMT_stdout, "#COLOR_MODEL = HSV\n");
	else if (gmtdefs.color_model & GMT_USE_CMYK)
		fprintf (GMT_stdout, "#COLOR_MODEL = +CMYK\n");
	else if (gmtdefs.color_model & GMT_READ_CMYK)
		fprintf (GMT_stdout, "#COLOR_MODEL = CMYK\n");
	else if (gmtdefs.color_model == GMT_USE_RGB)
		fprintf (GMT_stdout, "#COLOR_MODEL = +RGB\n");
	else
		fprintf (GMT_stdout, "#COLOR_MODEL = RGB\n");

	fprintf (GMT_stdout, "#\n");

	if (gmtdefs.color_model & GMT_READ_HSV) {
		sprintf(format, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format);
	}
	else if (gmtdefs.color_model & GMT_READ_CMYK) {
		sprintf(format, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format);
	}
	else {
		sprintf(format, "%s\t%%d\t%%d\t%%d\t%s\t%%d\t%%d\t%%d\n", gmtdefs.d_format, gmtdefs.d_format);
	}

	/* Determine color at lower and upper limit of each interval */

	for (i = 0; i < nz-1; i++) {

		lower = i;
		upper = i + 1;

		if (continuous) { /* Interpolate color at lower and upper value */

			for (j = 0; j < GMT_n_colors && x[lower] >= lut[j].z_high; j++);
			if (j == GMT_n_colors) j--;

			f = 1.0 / (lut[j].z_high - lut[j].z_low);

			if (gmtdefs.color_model & GMT_READ_HSV) {	/* Interpolation in HSV space */
				for (k = 0; k < 3; k++) hsv_low[k] = lut[j].hsv_low[k] + (lut[j].hsv_high[k] - lut[j].hsv_low[k]) * f * (x[lower] - lut[j].z_low);
			}
			else {	/* Interpolation in RGB space */
				for (k = 0; k < 3; k++) rgb_low[k] = lut[j].rgb_low[k] + irint ((lut[j].rgb_high[k] - lut[j].rgb_low[k]) * f * (x[lower] - lut[j].z_low));
			}

			while (j < GMT_n_colors && x[upper] > lut[j].z_high) j++;

			f = 1.0 / (lut[j].z_high - lut[j].z_low);


			if (gmtdefs.color_model & GMT_READ_HSV) {	/* Interpolation in HSV space */
				for (k = 0; k < 3; k++) hsv_high[k] = lut[j].hsv_low[k] + (lut[j].hsv_high[k] - lut[j].hsv_low[k]) * f * (x[upper] - lut[j].z_low);
			}
			else {	/* Interpolation in RGB space */
				for (k = 0; k < 3; k++) rgb_high[k] = lut[j].rgb_low[k] + irint ((lut[j].rgb_high[k] - lut[j].rgb_low[k]) * f * (x[upper] - lut[j].z_low));
			}
		}
		else {	 /* Interpolate central value and assign color to both lower and upper limit */

			a = (x[lower] + x[upper]) / 2;
			for (j = 0; j < GMT_n_colors && a >= lut[j].z_high; j++);
			if (j == GMT_n_colors) j--;

			f = 1.0 / (lut[j].z_high - lut[j].z_low);

			if (gmtdefs.color_model & GMT_READ_HSV) {	/* Interpolation in HSV space */
				for (k = 0; k < 3; k++) hsv_low[k] = hsv_high[k] = lut[j].hsv_low[k] + (lut[j].hsv_high[k] - lut[j].hsv_low[k]) * f * (a - lut[j].z_low);
			}
			else {	/* Interpolation in RGB space */
				for (k = 0; k < 3; k++) rgb_low[k] = rgb_high[k] = lut[j].rgb_low[k] + irint ((lut[j].rgb_high[k] - lut[j].rgb_low[k]) * f * (a - lut[j].z_low));
			}
		}

		if (lower == 0) {
			memcpy ((void *)rgb_back, (void *)rgb_low, (size_t)(3 * sizeof (int)));
			memcpy ((void *)hsv_back, (void *)hsv_low, (size_t)(3 * sizeof (double)));
		}
		if (upper == (nz-1)) {
			memcpy ((void *)rgb_fore, (void *)rgb_high, (size_t)(3 * sizeof (int)));
			memcpy ((void *)hsv_fore, (void *)hsv_high, (size_t)(3 * sizeof (double)));
		}

		if (gmtdefs.color_model & GMT_READ_HSV) {
			fprintf (GMT_stdout, format, z_out[lower], hsv_low[0], hsv_low[1], hsv_low[2], z_out[upper], hsv_high[0], hsv_high[1], hsv_high[2]);
		}
		else if (gmtdefs.color_model & GMT_READ_CMYK) {
			GMT_rgb_to_cmyk (rgb_low, cmyk_low);
			GMT_rgb_to_cmyk (rgb_high, cmyk_high);
			fprintf (GMT_stdout, format, z_out[lower], cmyk_low[0], cmyk_low[1], cmyk_low[2], cmyk_low[3],
				z_out[upper], cmyk_high[0], cmyk_high[1], cmyk_high[2], cmyk_high[3]);
		}
		else
			fprintf (GMT_stdout, format, z_out[lower], rgb_low[0], rgb_low[1], rgb_low[2], z_out[upper], rgb_high[0], rgb_high[1], rgb_high[2]);
	}

	GMT_free ((void *)x);
	GMT_free ((void *)lut);
	if (log_mode) GMT_free ((void *)z_out);

	/* Background, foreground, and nan colors */

	if (GMT_cpt_flags & 1) return;	/* Do not want to write BFN to the cpt file */

	if (GMT_cpt_flags & 2) {	/* Use low and high colors as back and foreground */
		memcpy ((void *)GMT_bfn[GMT_BGD].rgb, (void *)rgb_back, (size_t)(3 * sizeof (int)));
		memcpy ((void *)GMT_bfn[GMT_FGD].rgb, (void *)rgb_fore, (size_t)(3 * sizeof (int)));
		memcpy ((void *)GMT_bfn[GMT_BGD].hsv, (void *)hsv_back, (size_t)(3 * sizeof (double)));
		memcpy ((void *)GMT_bfn[GMT_FGD].hsv, (void *)hsv_fore, (size_t)(3 * sizeof (double)));
	}
	else if (reverse) {	/* Flip foreground and background colors */
		memcpy ((void *)rgb_low, (void *)GMT_bfn[GMT_BGD].rgb, (size_t)(3 * sizeof (int)));
		memcpy ((void *)GMT_bfn[GMT_BGD].rgb, (void *)GMT_bfn[GMT_FGD].rgb, (size_t)(3 * sizeof (int)));
		memcpy ((void *)GMT_bfn[GMT_FGD].rgb, (void *)rgb_low, (size_t)(3 * sizeof (int)));
		memcpy ((void *)hsv_low, (void *)GMT_bfn[GMT_BGD].hsv, (size_t)(3 * sizeof (double)));
		memcpy ((void *)GMT_bfn[GMT_BGD].hsv, (void *)GMT_bfn[GMT_FGD].hsv, (size_t)(3 * sizeof (double)));
		memcpy ((void *)GMT_bfn[GMT_FGD].hsv, (void *)hsv_low, (size_t)(3 * sizeof (double)));
	}

	if (gmtdefs.color_model & GMT_READ_HSV) {
		sprintf(format, "%%c\t%s\t%s\t%s\n", gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format);
		for (k = 0; k < 3; k++) {
			if (GMT_bfn[k].skip)
				fprintf (GMT_stdout, "%c -\n", code[k]);
			else {
				fprintf (GMT_stdout, format, code[k], GMT_bfn[k].hsv[0], GMT_bfn[k].hsv[1], GMT_bfn[k].hsv[2]);
			}
		}
	}
	else if (gmtdefs.color_model & GMT_USE_CMYK) {
		sprintf(format, "%%c\t%s\t%s\t%s\t%s\n", gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format);
		for (k = 0; k < 3; k++) {
			if (GMT_bfn[k].skip)
				fprintf (GMT_stdout, "%c -\n", code[k]);
			else {
				GMT_rgb_to_cmyk (GMT_bfn[k].rgb, cmyk_low);
				fprintf (GMT_stdout, format, code[k], cmyk_low[0], cmyk_low[1], cmyk_low[2], cmyk_low[3]);
			}
		}
	}
	else {
		for (k = 0; k < 3; k++) {
			if (GMT_bfn[k].skip)
				fprintf (GMT_stdout, "%c -\n", code[k]);
			else
				fprintf (GMT_stdout, "%c\t%d\t%d\t%d\n", code[k], GMT_bfn[k].rgb[0], GMT_bfn[k].rgb[1], GMT_bfn[k].rgb[2]);
		}
	}
}

int GMT_get_index (double value)
{
	int index;
	int lo, hi, mid;

	if (GMT_is_dnan (value)) return (-1);				/* Set to NaN color */
	if (value > GMT_lut[GMT_n_colors-1].z_high) return (-2);	/* Set to foreground color */
	if (value < GMT_lut[0].z_low) return (-3);			/* Set to background color */

	/* Must search for correct index */

	/* Speedup by Mika Heiskanen. This works if the colortable
	 * has been is sorted into increasing order. Careful when
	 * modifying the tests in the loop, the test and the mid+1
	 * parts are especially designed to make sure the loop
	 * converges to a single index.
	 */

        lo = 0;
        hi = GMT_n_colors - 1;
        while (lo != hi)
	{
		mid = (lo + hi) / 2;
		if (value >= GMT_lut[mid].z_high)
			lo = mid + 1;
		else
			hi = mid;
	}
        index = lo;
        if (value >= GMT_lut[index].z_low && value < GMT_lut[index].z_high) return (index);

        /* Slow search in case the table was not sorted
         * No idea whether it is possible, but it most certainly
         * does not hurt to have the code here as a backup.
         */

	index = 0;
	while (index < GMT_n_colors && ! (value >= GMT_lut[index].z_low && value < GMT_lut[index].z_high) ) index++;
	if (index == GMT_n_colors) index--;	/* Because we use <= for last range */
	return (index);
}

int GMT_get_rgb_from_z (double value, int *rgb)
{
	int index;
	index = GMT_get_index (value);
	get_rgb_lookup (index, value, rgb);
	return (index);
}

void get_rgb_lookup (int index, double value, int *rgb)
{
	int i;
	double rel;

	if (index == -1) {	/* Nan */
		memcpy ((void *)rgb, (void *)GMT_bfn[GMT_NAN].rgb, 3 * sizeof (int));
		GMT_cpt_skip = GMT_bfn[GMT_NAN].skip;
	}
	else if (index == -2) {	/* Foreground */
		memcpy ((void *)rgb, (void *)GMT_bfn[GMT_FGD].rgb, 3 * sizeof (int));
		GMT_cpt_skip = GMT_bfn[GMT_FGD].skip;
	}
	else if (index == -3) {	/* Background */
		memcpy ((void *)rgb, (void *)GMT_bfn[GMT_BGD].rgb, 3 * sizeof (int));
		GMT_cpt_skip = GMT_bfn[GMT_BGD].skip;
	}
	else if (GMT_lut[index].skip) {		/* Set to page color for now */
		memcpy ((void *)rgb, (void *)gmtdefs.page_rgb, 3 * sizeof (int));
		GMT_cpt_skip = TRUE;
	}
	else {	/* Do linear interpolation between low and high colors */
		rel = (value - GMT_lut[index].z_low) * GMT_lut[index].i_dz;
		if (gmtdefs.color_model & GMT_USE_HSV) {	/* Interpolation in HSV space */
			double hsv[3];
			for (i = 0; i < 3; i++) hsv[i] = GMT_lut[index].hsv_low[i] + rel * GMT_lut[index].hsv_diff[i];
			GMT_hsv_to_rgb (rgb, hsv[0], hsv[1], hsv[2]);
		}
		else {	/* Interpolation in RGB space */
			for (i = 0; i < 3; i++) rgb[i] = GMT_lut[index].rgb_low[i] + irint (rel * GMT_lut[index].rgb_diff[i]);
		}
		GMT_cpt_skip = FALSE;
	}
}

int GMT_get_fill_from_z (double value, struct GMT_FILL *fill)
{
	int index;
	struct GMT_FILL *f;

	index = GMT_get_index (value);

	/* Check if pattern */

	if (index >= 0 && (f = GMT_lut[index].fill))
		memcpy ((void *)fill, (void *)f, sizeof (struct GMT_FILL));
	else if (index < 0 && (f = GMT_bfn[index+3].fill))
		memcpy ((void *)fill, (void *)f, sizeof (struct GMT_FILL));
	else {
		get_rgb_lookup (index, value, fill->rgb);
		fill->use_pattern = FALSE;
	}
	return (index);
}

void GMT_rgb_to_hsv (int rgb[], double *h, double *s, double *v)
{
	double xr, xg, xb, r_dist, g_dist, b_dist, max_v, min_v, diff, idiff;

	xr = rgb[0] * I_255;
	xg = rgb[1] * I_255;
	xb = rgb[2] * I_255;
	max_v = MAX (MAX (xr, xg), xb);
	min_v = MIN (MIN (xr, xg), xb);
	diff = max_v - min_v;
	*h = 0.0;
	*s = (max_v == 0.0) ? 0.0 : diff / max_v;
	*v = max_v;
	if ((*s) == 0.0) return;	/* Hue is undefined */
	idiff = 1.0 / diff;
	r_dist = (max_v - xr) * idiff;
	g_dist = (max_v - xg) * idiff;
	b_dist = (max_v - xb) * idiff;
	if (xr == max_v)
		*h = b_dist - g_dist;
	else if (xg == max_v)
		*h = 2.0 + r_dist - b_dist;
	else
		*h = 4.0 + g_dist - r_dist;
	(*h) *= 60.0;
	if ((*h) < 0.0) (*h) += 360.0;
}

void GMT_hsv_to_rgb (int rgb[], double h, double s, double v)
{
	int i;
	double f, p, q, t, rr, gg, bb;

	if (s == 0.0)
		rgb[0] = rgb[1] = rgb[2] = (int) floor (255.999 * v);
	else {
		while (h >= 360.0) h -= 360.0;
		while (h < 0.0) h += 360.0;
		h /= 60.0;
		i = (int)h;
		f = h - i;
		p = v * (1.0 - s);
		q = v * (1.0 - (s * f));
		t = v * (1.0 - (s * (1.0 - f)));
		switch (i) {
			case 0:
				rr = v;	gg = t;	bb = p;
				break;
			case 1:
				rr = q;	gg = v;	bb = p;
				break;
			case 2:
				rr = p;	gg = v;	bb = t;
				break;
			case 3:
				rr = p;	gg = q;	bb = v;
				break;
			case 4:
				rr = t;	gg = p;	bb = v;
				break;
			default:
				rr = v;	gg = p;	bb = q;
				break;
		}

		rgb[0] = (rr < 0.0) ? 0 : (int) floor (rr * 255.999);
		rgb[1] = (gg < 0.0) ? 0 : (int) floor (gg * 255.999);
		rgb[2] = (bb < 0.0) ? 0 : (int) floor (bb * 255.999);
	}
}

void GMT_rgb_to_cmyk (int rgb[], double cmyk[])
{
	/* Plain conversion; with default undercolor removal or blackgeneration */

	int i;

	/* RGB is in integer 0-255, CMYK will be in float 0-100 range */

	for (i = 0; i < 3; i++) cmyk[i] = 100.0 - (rgb[i] / 2.55);
	cmyk[3] = MIN (cmyk[0], MIN (cmyk[1], cmyk[2]));	/* Default Black generation */
	if (cmyk[3] < GMT_CONV_LIMIT) cmyk[3] = 0.0;
	/* To implement device-specific blackgeneration, supply lookup table K = BG[cmyk[3]] */

	for (i = 0; i < 3; i++) {
		cmyk[i] -= cmyk[3];		/* Default undercolor removal */
		if (cmyk[i] < GMT_CONV_LIMIT) cmyk[i] = 0.0;
	}

	/* To implement device-specific undercolor removal, supply lookup table u = UR[cmyk[3]] */
}

void GMT_cmyk_to_rgb (int rgb[], double cmyk[])
{
	/* Plain conversion; no undercolor removal or blackgeneration */

	int i;

	/* CMYK is in 0-100, RGB will be in 0-255 range */

	for (i = 0; i < 3; i++) rgb[i] = (int) floor ((100.0 - cmyk[i] - cmyk[3]) * 2.55999);
}

void GMT_cmyk_to_hsv (double hsv[], double cmyk[])
{
	/* Plain conversion; no undercolor removal or blackgeneration */

	int i, rgb[3];

	/* CMYK is in 0-100, RGB will be in 0-255 range */

	for (i = 0; i < 3; i++) rgb[i] = (int) floor ((100.0 - cmyk[i] - cmyk[3]) * 2.55999);
	GMT_rgb_to_hsv (rgb, &hsv[0], &hsv[1], &hsv[2]);
}

void GMT_illuminate (double intensity, int rgb[])
{
	double h, s, v, di;

	if (GMT_is_dnan (intensity)) return;
	if (intensity == 0.0) return;
	if (fabs (intensity) > 1.0) intensity = copysign (1.0, intensity);

	GMT_rgb_to_hsv (rgb, &h, &s, &v);
	if (intensity > 0.0) {	/* Lighten the color */
		di = 1.0 - intensity;
		if (s != 0.0) s = di * s + intensity * gmtdefs.hsv_max_saturation;
		v = di * v + intensity * gmtdefs.hsv_max_value;
	}
	else {			/* Darken the color */
		di = 1.0 + intensity;
		if (s != 0.0) s = di * s - intensity * gmtdefs.hsv_min_saturation;
		v = di * v - intensity * gmtdefs.hsv_min_value;
	}
	if (v < 0.0) v = 0.0;
	if (s < 0.0) s = 0.0;
	if (v > 1.0) v = 1.0;
	if (s > 1.0) s = 1.0;
	GMT_hsv_to_rgb (rgb, h, s, v);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * GMT_akima computes the coefficients for a quasi-cubic hermite spline.
 * Same algorithm as in the IMSL library.
 * Programmer:	Paul Wessel
 * Date:	16-JAN-1987
 * Ver:		v.1-pc
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */

int GMT_akima (double *x, double *y, int nx, double *c)
{
	int i, no;
	double t1, t2, b, rm1, rm2, rm3, rm4;

	/* Assumes that n >= 4 and x is monotonically increasing */

	rm3 = (y[1] - y[0])/(x[1] - x[0]);
	t1 = rm3 - (y[1] - y[2])/(x[1] - x[2]);
	rm2 = rm3 + t1;
	rm1 = rm2 + t1;

	/* get slopes */

	no = nx - 2;
	for (i = 0; i < nx; i++) {
		if (i >= no)
			rm4 = rm3 - rm2 + rm3;
		else
			rm4 = (y[i+2] - y[i+1])/(x[i+2] - x[i+1]);
		t1 = fabs(rm4 - rm3);
		t2 = fabs(rm2 - rm1);
		b = t1 + t2;
		c[3*i] = (b != 0.0) ? (t1*rm2 + t2*rm3) / b : 0.5*(rm2 + rm3);
		rm1 = rm2;
		rm2 = rm3;
		rm3 = rm4;
	}
	no = nx - 1;

	/* compute the coefficients for the nx-1 intervals */

	for (i = 0; i < no; i++) {
		t1 = 1.0 / (x[i+1] - x[i]);
		t2 = (y[i+1] - y[i])*t1;
		b = (c[3*i] + c[3*i+3] - t2 - t2)*t1;
		c[3*i+2] = b*t1;
		c[3*i+1] = -b + (t2 - c[3*i])*t1;
	}
	return (GMT_NOERROR);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * GMT_cspline computes the coefficients for a natural cubic spline.
 * To evaluate, call GMT_csplint
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */

int GMT_cspline (double *x, double *y, int n, double *c)
{
	int i, k;
	double ip, s, dx1, i_dx2, *u;

	/* Assumes that n >= 4 and x is monotonically increasing */

	u = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "GMT_cspline");
	for (i = 1; i < n-1; i++) {
		i_dx2 = 1.0 / (x[i+1] - x[i-1]);
		dx1 = x[i] - x[i-1];
		s = dx1 * i_dx2;
		ip = 1.0 / (s * c[i-1] + 2.0);
		c[i] = (s - 1.0) * ip;
		u[i] = (y[i+1] - y[i]) / (x[i+1] - x[i]) - (y[i] - y[i-1]) / dx1;
		u[i] = (6.0 * u[i] * i_dx2 - s * u[i-1]) * ip;
	}
	for (k = n-2; k >= 0; k--) c[k] = c[k] * c[k+1] + u[k];
	GMT_free ((void *)u);

	return (GMT_NOERROR);
}

double GMT_csplint (double *x, double *y, double *c, double xp, int klo)
{
	int khi;
	double h, ih, b, a, yp;

	khi = klo + 1;
	h = x[khi] - x[klo];
	ih = 1.0 / h;
	a = (x[khi] - xp) * ih;
	b = (xp - x[klo]) * ih;
	yp = a * y[klo] + b * y[khi] + ((a*a*a - a) * c[klo] + (b*b*b - b) * c[khi]) * (h*h) / 6.0;

	return (yp);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * GMT_intpol will interpolate from the dataset <x,y> onto a new set <u,v>
 * where <x,y> and <u> is supplied by the user. <v> is returned. The
 * parameter mode governs what interpolation scheme that will be used.
 * If u[i] is outside the range of x, then v[i] will contain NaN.
 *
 * input:  x = x-values of input data
 *	   y = y-values "    "     "
 *	   n = number of data pairs
 *	   m = number of desired interpolated values
 *	   u = x-values of these points
 *	mode = type of interpolation
 *	  mode = 0 : Linear interpolation
 *	  mode = 1 : Quasi-cubic hermite spline (GMT_akima)
 *	  mode = 2 : Natural cubic spline (cubspl)
 *        mode = 3 : No interpolation (closest point)
 * output: v = y-values at interpolated points
 * PS. v must have space allocated before calling GMT_intpol
 *
 * Programmer:	Paul Wessel
 * Date:	16-JAN-1987
 * Ver:		v.2
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 */

int GMT_intpol (double *x, double *y, int n, int m, double *u, double *v, int mode)
{
	int i, j, err_flag = 0;
	BOOLEAN down = FALSE, check = TRUE;
	double dx, x_min, x_max, *c = VNULL, GMT_csplint (double *x, double *y, double *c, double xp, int klo);

	if (mode < 0) {	/* No need to check for sanity */
		check = FALSE;
		mode = -mode;
	}

	if (mode > 3) mode = 0;
	if (mode != 3 && n < 4) mode = 0;
	if (n < 2) {
		fprintf (stderr, "%s: GMT Fatal Error: need at least 2 x-values\n", GMT_program);
		return (EXIT_FAILURE);
	}

	if (check) {
		/* Check to see if x-values are monotonically increasing/decreasing */

		dx = x[1] - x[0];
		if (dx > 0.0) {
			for (i = 2; i < n && err_flag == 0; i++) {
				if ((x[i] - x[i-1]) <= 0.0) err_flag = i;
			}
		}
		else {
			down = TRUE;
			for (i = 2; i < n && err_flag == 0; i++) {
				if ((x[i] - x[i-1]) >= 0.0) err_flag = i;
			}
		}

		if (err_flag) {
			fprintf (stderr, "%s: GMT Fatal Error: x-values are not monotonically increasing/decreasing!\n", GMT_program);
			return (err_flag);
		}

		if (down) {	/* Must flip directions temporarily */
			for (i = 0; i < n; i++) x[i] = -x[i];
			for (i = 0; i < m; i++) u[i] = -u[i];
		}
	}

	/* Set minimum and maximum */

	if (mode == 3) {
		x_min = (3*x[0] - x[1]) / 2;
		x_max = (3*x[n-1] - x[n-2]) / 2;
	}
	else {
		x_min = x[0]; x_max = x[n-1];
	}

	/* Allocate memory for spline factors */

	if (mode == 1) {	/* Akima's spline */
		c = (double *) GMT_memory (VNULL, (size_t)(3*n), sizeof(double), "GMT_intpol");
		err_flag = GMT_akima (x, y, n, c);
	}
	else if (mode == 2) {	/* Natural cubic spline */
		c = (double *) GMT_memory (VNULL, (size_t)(3*n), sizeof(double), "GMT_intpol");
		err_flag = GMT_cspline (x, y, n, c);
	}
	if (err_flag != 0) {
		GMT_free ((void *)c);
		return (err_flag);
	}

	/* Compute the interpolated values from the coefficients */

	j = 0;
	for (i = 0; i < m; i++) {
		if (u[i] < x_min || u[i] > x_max) {	/* Desired point outside data range */
			v[i] = GMT_d_NaN;
			continue;
		}
		while (x[j] > u[i] && j > 0) j--;	/* In case u is not sorted */
		while (j < n && x[j] <= u[i]) j++;
		if (j == n) j--;
		if (j > 0) j--;

		switch (mode) {
			case 0:
				dx = u[i] - x[j];
				v[i] = (y[j+1]-y[j])*dx/(x[j+1]-x[j]) + y[j];
				break;
			case 1:
				dx = u[i] - x[j];
				v[i] = ((c[3*j+2]*dx + c[3*j+1])*dx + c[3*j])*dx + y[j];
				break;
			case 2:
				v[i] = GMT_csplint (x, y, c, u[i], j);
				break;
			case 3:
				v[i] = (u[i] - x[j] < x[j+1] - u[i]) ? y[j] : y[j+1];
				break;
		}
	}
	if (c) GMT_free ((void *)c);

	if (down) {	/* Must reverse directions */
		for (i = 0; i < n; i++) x[i] = -x[i];
		for (i = 0; i < m; i++) u[i] = -u[i];
	}

	return (GMT_NOERROR);
}

void *GMT_memory (void *prev_addr, size_t nelem, size_t size, char *progname)
{
	void *tmp;
	static char *m_unit[4] = {"bytes", "kb", "Mb", "Gb"};
	double mem;
	int k;

	if (nelem == 0) return(VNULL); /* Take care of n = 0 */

	if (prev_addr) {
		if ((tmp = realloc ((void *) prev_addr, (nelem * size))) == VNULL) {
			mem = (double)(nelem * size);
			k = 0;
			while (mem >= 1024.0 && k < 3) mem /= 1024.0, k++;
			fprintf (stderr, "GMT Fatal Error: %s could not reallocate memory [%.2f %s, n_items = %d]\n", progname, mem, m_unit[k], (int)nelem);
			exit (EXIT_FAILURE);
		}
	}
	else {
		if ((tmp = calloc ((size_t) nelem, (unsigned) size)) == VNULL) {
			mem = (double)(nelem * size);
			k = 0;
			while (mem >= 1024.0 && k < 3) mem /= 1024.0, k++;
			fprintf (stderr, "GMT Fatal Error: %s could not allocate memory [%.2f %s, n_items = %d]\n", progname, mem, m_unit[k], (int)nelem);
			exit (EXIT_FAILURE);
		}
	}
	return (tmp);
}

void GMT_free (void *addr)
{

	free (addr);
}

void GMT_contlabel_init (struct GMT_CONTOUR *G, int mode)
{	/* Assign default values to structure */
	memset ((void *)G, 0, sizeof (struct GMT_CONTOUR));	/* Sets all to 0 */
	if (mode == 1) {
		G->line_type = 1;
		strcpy (G->line_name, "Contour");
		}
	else {
		G->line_type = 0;
		strcpy (G->line_name, "Line");
	}
	G->transparent = TRUE;
	G->spacing = TRUE;
	G->half_width = 5;
	G->label_font_size = 9.0;
	G->label_dist_spacing = 4.0;	/* Inches */
	G->label_dist_frac = 0.25;	/* Fraction of above head start for closed labels */
	G->box = 2;			/* Rect box shape is Default */
	if (gmtdefs.measure_unit == GMT_CM) G->label_dist_spacing = 10.0 / 2.54;
	G->clearance[0] = G->clearance[1] = 15.0;	/* 15 % */
	G->clearance_flag = 1;	/* Means we gave percentages of label font size */
	G->just = 6;	/* CM */
	G->label_font = gmtdefs.annot_font[0];	/* ANNOT_FONT_PRIMARY */
	G->dist_unit = gmtdefs.measure_unit;
	GMT_init_pen (&G->pen, GMT_PENWIDTH);
	GMT_init_pen (&G->line_pen, GMT_PENWIDTH);
	memcpy ((void *)G->rgb, (void *)gmtdefs.page_rgb, (size_t)(3 * sizeof (int)));			/* Default box color is page color [nominally white] */
	memcpy ((void *)G->font_rgb, (void *)gmtdefs.background_rgb, (size_t)(3 * sizeof (int)));	/* Default label font color is page background [nominally black] */
}

int GMT_contlabel_specs (char *txt, struct GMT_CONTOUR *G)
{
	int k, bad = 0, pos = 0;
	BOOLEAN g_set = FALSE;
	char txt_cpy[BUFSIZ], p[BUFSIZ], txt_a[GMT_LONG_TEXT], txt_b[GMT_LONG_TEXT], c;

	/* Decode [+a<angle>|n|p[u|d]][+c<dx>[/<dy>]][+f<font>][+g<fill>][+j<just>][+k<fontcolor>][+l<label>][+n|N<dx>[/<dy>]][+o][+v][+r<min_rc>][+s<size>][+p[<pen>]][+u<unit>][+w<width>][+=<prefix>] strings */

	for (k = 0; txt[k] && txt[k] != '+'; k++);	/* Look for +<options> strings */
	if (!txt[k]) {	/* Does not contain new-style settings, look for old-style (v3.4) syntax */
		if (strchr (txt, 'a') || strchr (txt, 'f') || strchr (txt, 'o') || strchr (txt, 't') || strchr (txt, '/'))
			/* Decode <val>a<angle>f<font>o/r/g/b strings */
			return (GMT_contlabel_specs_old (txt, G));	/* Old-style info strings */
		else
			return (0);	/* Nothing to do */
	}

	/* Decode new-style +separated substrings */

	G->nudge_flag = 0;
	strcpy (txt_cpy, &txt[k+1]);
	while ((GMT_strtok (txt_cpy, "+", &pos, p))) {
		switch (p[0]) {
			case 'a':	/* Angle specification */
				if (p[1] == 'p' || p[1] == 'P')	{	/* Line-parallel label */
					G->angle_type = G->hill_label = 0;
					if (p[2] == 'u' || p[2] == 'U')		/* Line-parallel label readable when looking up hill */
						G->hill_label = +1;
					else if (p[2] == 'd' || p[2] == 'D')	/* Line-parallel label readable when looking down hill */
						G->hill_label = -1;
				}
				else if (p[1] == 'n' || p[1] == 'N')	/* Line-normal label */
					G->angle_type = 1;
				else {					/* Label at a fixed angle */
					G->label_angle = atof (&p[1]);
					G->angle_type = 2;
					GMT_lon_range_adjust (2, &G->label_angle);	/* Now -180/+180 */
					while (fabs (G->label_angle) > 90.0) G->label_angle -= copysign (180.0, G->label_angle);
				}
				break;

			case 'c':	/* Clearance specification */
				k = sscanf (&p[1], "%[^/]/%s", txt_a, txt_b);
				G->clearance[GMT_X] = GMT_convert_units (txt_a, GMT_INCH);
				G->clearance[GMT_Y] = (k == 2 ) ? GMT_convert_units (txt_b, GMT_INCH) : G->clearance[GMT_X];
				G->clearance_flag = ((strchr (txt_a, '%')) ? 1 : 0);
				if (k == 0) bad++;
				break;

			case 'd':	/* Debug option - draw helper points or lines */
				G->debug = TRUE;
				break;


			case 'f':	/* Font specification */
				if (p[1] >= '0' && p[1] <= '9')
					k = atoi (&p[1]);
				else
					k = GMT_font_lookup (&p[1], GMT_font, GMT_N_FONTS);
				if (k < 0 || k >= GMT_N_FONTS)
					bad++;
				else
					G->label_font = k;
				break;

			case 'g':	/* Box Fill specification */
				if (GMT_getrgb (&p[1], G->rgb)) bad++;
				G->transparent = FALSE;
				g_set = TRUE;
				break;

			case 'j':	/* Justification specification */
				txt_a[0] = p[1];	txt_a[1] = p[2];	txt_a[2] = '\0';
				G->just = GMT_just_decode (txt_a, 2, 4);
				break;

			case 'k':	/* Font color specification */
				if (GMT_getrgb (&p[1], G->font_rgb)) bad++;
				G->got_font_rgb = TRUE;
				break;

			case 'l':	/* Exact Label specification */
				strcpy (G->label, &p[1]);
				G->label_type = 1;
				break;

			case 'L':	/* Label code specification */
				switch (p[1]) {
					case 'h':	/* Take the first string in multisegment headers */
						G->label_type = 2;
						break;
					case 'd':	/* Use the current plot distance in chosen units */
						G->label_type = 3;
						G->dist_unit = GMT_unit_lookup (p[2]);
						break;
					case 'D':	/* Use current map distance in chosen units */
						G->label_type = 4;
						if (p[2] && strchr ("dekmn", (int)p[2])) {	/* Found a valid unit */
							c = p[2];
							bad += GMT_get_dist_scale (c, &G->L_d_scale, &G->L_proj_type, &G->L_dist_func);
						}
						else
							c = 0;	/* Meaning "not set" */
						bad += GMT_get_dist_scale (c, &G->L_d_scale, &G->L_proj_type, &G->L_dist_func);
						G->dist_unit = (int)c;
						break;
					case 'f':	/* Take the 3rd column in fixed contour location file */
						G->label_type = 5;
						break;
					case 'x':	/* Take the first string in multisegment headers in the crossing file */
						G->label_type = 6;
						break;
					case 'n':	/* Use the current multisegment number */
						G->label_type = 7;
						break;
					case 'N':	/* Use <current file number>/<multisegment number> */
						G->label_type = 8;
						break;
					default:	/* Probably meant lower case l */
						strcpy (G->label, &p[1]);
						G->label_type = 1;
						break;
				}
				break;

			case 'n':	/* Nudge specification; dx/dy are increments along local line axes */
				G->nudge_flag = 1;
			case 'N':	/* Nudge specification; dx/dy are increments along plot axes */
				G->nudge_flag++;
				k = sscanf (&p[1], "%[^/]/%s", txt_a, txt_b);
				G->nudge[GMT_X] = GMT_convert_units (txt_a, GMT_INCH);
				G->nudge[GMT_Y] = (k == 2 ) ? GMT_convert_units (txt_b, GMT_INCH) : G->nudge[GMT_X];
				if (k == 0) bad++;
				break;
			case 'o':	/* Use rounded rectangle textbox shape */
				G->box = 4 + (G->box & 1);
				break;

			case 'p':	/* Draw text box outline [with optional textbox pen specification] */
				if (GMT_getpen (&p[1], &G->pen)) bad++;
				G->box |= 1;
				break;

			case 'r':	/* Minimum radius of curvature specification */
				G->min_radius = GMT_convert_units (&p[1], GMT_INCH);
				break;

			case 's':	/* Font size specification */
				G->label_font_size = atof (&p[1]);
				if (G->label_font_size <= 0.0) bad++;
				break;

			case 'u':	/* Label Unit specification */
				if (p[1]) strcpy (G->unit, &p[1]);
				break;

			case 'v':	/* Curved text [Default is straight] */
				G->curved_text = TRUE;
				break;

			case 'w':	/* Angle filter width [Default is 10 points] */
				G->half_width = atoi (&p[1]) / 2;
				break;

			case '=':	/* Label Prefix specification */
				if (p[1]) strcpy (G->prefix, &p[1]);
				break;

			default:
				bad++;
				break;
		}
	}

	return (bad);
}

int GMT_contlabel_specs_old (char *txt, struct GMT_CONTOUR *G)
{	/* For backwards compatibility with 3.4.x */
	int j, bad;

	G->transparent = FALSE;
	for (j = 0, bad = 0; txt[j] && txt[j] != 'f'; j++);
	if (txt[j])	{ /* Found font size option */
		G->label_font_size = atof (&txt[j+1]);
		if (G->label_font_size <= 0.0) bad++;
	}

	for (j = 0; txt[j] && txt[j] != 'a'; j++);
	if (txt[j])	{ /* Found fixed angle option */
		G->label_angle = atof (&txt[j+1]);
		G->angle_type = 2;
		if (G->label_angle <= -90.0 || G->label_angle > 180.0) bad++;
	}

	for (j = 0; txt[j] && txt[j] != '/'; j++);
	if (txt[j] && GMT_getrgb (&txt[j+1], G->rgb)) bad++;
	if (strchr (txt, 't')) G->transparent = TRUE;	/* transparent box must be rectangular */

	return (bad);
}

int GMT_contlabel_info (char flag, char *txt, struct GMT_CONTOUR *L)
{
	/* Interpret the contour-label information string and set structure items */
	int k, j = 0, colon = 0, error = 0;
	char txt_a[GMT_LONG_TEXT], c;

	L->spacing = FALSE;	/* Turn off the default since we gave an option */
	strcpy (L->option, &txt[1]);	 /* May need to process L->option later after -R,-J have been set */
	for (colon = 0; txt[colon] && txt[colon] != ':'; colon++);	/* Search for :<isolate_radius>[unit] setting */
	if (txt[colon] == ':') {	/* Want to isolate labels by given radius */
		txt[colon] = '\0';	/* Temporarily chop off the :<radius> part */
		L->isolate = TRUE;
		L->label_isolation = GMT_convert_units (&txt[colon+1], GMT_INCH);
	}

	L->flag = flag;
	switch (txt[0]) {
		case 'L':	/* Quick straight lines for intersections */
			L->do_interpolate = TRUE;
		case 'l':
			L->crossing = GMT_CONTOUR_XLINE;
			break;
		case 'N':	/* Specify number of labels per segment */
			L->number_placement = 1;	/* Distribution of labels */
			if (txt[1] == '-') L->number_placement = -1, j = 1;	/* Left label if n = 1 */
			if (txt[1] == '+') L->number_placement = +1, j = 1;	/* Right label if n = 1 */
		case 'n':	/* Specify number of labels per segment */
			L->number = TRUE;
			k = sscanf (&txt[1+j], "%d/%s", &L->n_cont, txt_a);
			if (k == 2) L->min_dist = GMT_convert_units (txt_a, GMT_INCH);
			if (L->n_cont == 0) {
				fprintf (stderr, "%s: GMT SYNTAX ERROR -%c.  Number of labels must exceed zero\n", GMT_program, L->flag);
				error++;
			}
			if (L->min_dist < 0.0) {
				fprintf (stderr, "%s: GMT SYNTAX ERROR -%c.  Minimum label separation cannot be negative\n", GMT_program, L->flag);
				error++;
			}
			break;
		case 'f':	/* fixed points file */
			L->fixed = TRUE;
			k = sscanf (&txt[1], "%[^/]/%lf", L->file, &L->slop);
			if (k == 1) L->slop = GMT_CONV_LIMIT;
			break;
		case 'X':	/* Crossing complicated curve */
			L->do_interpolate = TRUE;
		case 'x':	/* Crossing line */
			L->crossing = GMT_CONTOUR_XCURVE;
			strcpy (L->file, &txt[1]);
			break;
		case 'D':	/* Specify distances in geographic units (km, degrees, etc) */
			L->dist_kind = 1;
		case 'd':	/* Specify distances in plot units [cimp] */
			L->spacing = TRUE;
			k = sscanf (&txt[j], "%[^/]/%lf", txt_a, &L->label_dist_frac);
			if (k == 1) L->label_dist_frac = 0.25;
			if (L->dist_kind == 1) {	/* Distance units other than xy specified */
				k = strlen (txt_a) - 1;
				c = (isdigit ((int)txt_a[k]) || txt_a[k] == '.') ? 0 : txt_a[k];
				L->label_dist_spacing = atof (&txt_a[1]);
				error += GMT_get_dist_scale (c, &L->d_scale, &L->proj_type, &L->dist_func);
			}
			else
				L->label_dist_spacing = GMT_convert_units (&txt_a[1], GMT_INCH);
			if (L->label_dist_spacing <= 0.0) {
				fprintf (stderr, "%s: GMT SYNTAX ERROR -%c.  Spacing between labels must exceed 0.0\n", GMT_program, L->flag);
				error++;
			}
			break;
		default:	/* For the old 3.4-style -G<gap>[/<width>] option format */
			L->spacing = TRUE;
			k = sscanf (&txt[j], "%[^/]/%d", txt_a, &L->half_width);
			if (k == 0) {
				fprintf (stderr, "%s: GMT SYNTAX ERROR -%c[d]: Give label spacing\n", GMT_program, L->flag);
				error++;
			}
			L->label_dist_spacing = GMT_convert_units (txt_a, GMT_INCH);
			if (k == 2) L->half_width /= 2;
			if (L->label_dist_spacing <= 0.0) {
				fprintf (stderr, "%s: GMT SYNTAX ERROR -%c.  Spacing between labels must exceed 0.0\n", GMT_program, L->flag);
				error++;
			}
			if (L->half_width < 0) {
				fprintf (stderr, "%s: GMT SYNTAX ERROR -%c.  Label smoothing width must >= 0 points\n", GMT_program, L->flag);
				error++;
			}
			break;
	}
	if (L->isolate) txt[colon] = ':';	/* Replace the : from earlier */

	return (error);
}

int GMT_contlabel_prep (struct GMT_CONTOUR *G, double xyz[2][3])
{
	/* G is pointer to the LABELED CONTOUR structure
	 * xyz, if not NULL, have the (x,y,z) min and max values for a grid
	 */

	/* Prepares contour labeling machinery as needed */

	int i, k, n, error = 0, pos;
	size_t n_alloc = GMT_SMALL_CHUNK;
	double x, y, step;
	char buffer[BUFSIZ], p[BUFSIZ], txt_a[GMT_LONG_TEXT], txt_b[GMT_LONG_TEXT], txt_c[GMT_LONG_TEXT], txt_d[GMT_LONG_TEXT];

	/* Maximum step size (in degrees) used for interpolation of line segments along great circles (if requested) */
	step = gmtdefs.line_step / project_info.x_scale / project_info.M_PR_DEG;

	if (G->clearance_flag) {	/* Gave a percentage of fontsize as clearance */
		G->clearance[0] = 0.01 * G->clearance[0] * G->label_font_size / 72.0;
		G->clearance[1] = 0.01 * G->clearance[1] * G->label_font_size / 72.0;
	}
	if (G->label_type == 5 && !G->fixed) {	/* Requires fixed file */
		error++;
		fprintf (stderr, "%s: GMT SYNTAX ERROR -%c:  Labeling option +Lf requires the fixed label location setting\n", GMT_program, G->flag);
	}
	if (G->label_type == 6 && G->crossing != GMT_CONTOUR_XCURVE) {	/* Requires cross file */
		error++;
		fprintf (stderr, "%s: GMT SYNTAX ERROR -%c:  Labeling option +Lx requires the crossing lines setting\n", GMT_program, G->flag);
	}
	if (G->spacing && G->dist_kind == 1 && G->label_type == 4 && G->dist_unit == 0) {	/* Did not specify unit - use same as in -G */
		G->L_d_scale = G->d_scale;
		G->L_proj_type = G->proj_type;
		G->L_dist_func = G->dist_func;
	}
	if ((G->dist_kind == 1 || G->label_type == 4) && !GMT_IS_MAPPING) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -%c:  Map distance options requires a map projection.\n", GMT_program, G->flag);
		error++;
	}
	if (G->angle_type == 0)
		G->no_gap = (G->just < 5 || G->just > 7);	/* Don't clip contour if label is not in the way */
	else if (G->angle_type == 1)
		G->no_gap = ((G->just + 2)%4);	/* Don't clip contour if label is not in the way */

	if (G->crossing == GMT_CONTOUR_XLINE) {
		strcpy (buffer, G->option);
		G->xp = (struct GMT_TABLE *) GMT_memory (VNULL, 1, sizeof (struct GMT_TABLE), GMT_program);
		G->xp->segment = (struct GMT_LINE_SEGMENT **) GMT_memory (VNULL, n_alloc, sizeof (struct GMT_LINE_SEGMENT *), GMT_program);
		pos = 0;
		while ((GMT_strtok (buffer, ",", &pos, p))) {
			G->xp->segment[G->xp->n_segments] = (struct GMT_LINE_SEGMENT *) GMT_memory (VNULL, 1, sizeof (struct GMT_LINE_SEGMENT), GMT_program);
			GMT_alloc_segment (G->xp->segment[G->xp->n_segments], 2, 2, TRUE);
			G->xp->segment[G->xp->n_segments]->n_rows = 2;
			n = sscanf (p, "%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d);
			if (n == 4) {	/* Easy, got lon0/lat0/lon1/lat1 */
				error += GMT_verify_expectations (GMT_io.in_col_type[GMT_X], GMT_scanf_arg (txt_a, GMT_io.in_col_type[GMT_X], &G->xp->segment[G->xp->n_segments]->coord[GMT_X][0]), txt_a);
				error += GMT_verify_expectations (GMT_io.in_col_type[GMT_Y], GMT_scanf_arg (txt_b, GMT_io.in_col_type[GMT_Y], &G->xp->segment[G->xp->n_segments]->coord[GMT_Y][0]), txt_b);
				error += GMT_verify_expectations (GMT_io.in_col_type[GMT_X], GMT_scanf_arg (txt_c, GMT_io.in_col_type[GMT_X], &G->xp->segment[G->xp->n_segments]->coord[GMT_X][1]), txt_c);
				error += GMT_verify_expectations (GMT_io.in_col_type[GMT_Y], GMT_scanf_arg (txt_d, GMT_io.in_col_type[GMT_Y], &G->xp->segment[G->xp->n_segments]->coord[GMT_Y][1]), txt_d);
			}
			else if (n == 2) { /* Easy, got <code>/<code> */
				error += GMT_code_to_lonlat (txt_a, &G->xp->segment[G->xp->n_segments]->coord[GMT_X][0], &G->xp->segment[G->xp->n_segments]->coord[GMT_Y][0]);
				error += GMT_code_to_lonlat (txt_b, &G->xp->segment[G->xp->n_segments]->coord[GMT_X][1], &G->xp->segment[G->xp->n_segments]->coord[GMT_Y][1]);
			}
			else if (n == 3) {	/* More complicated: <code>/<lon>/<lat> or <lon>/<lat>/<code> */
				if (GMT_code_to_lonlat (txt_a, &G->xp->segment[G->xp->n_segments]->coord[GMT_X][0], &G->xp->segment[G->xp->n_segments]->coord[GMT_Y][0])) {	/* Failed, so try the other way */
					error += GMT_verify_expectations (GMT_io.in_col_type[GMT_X], GMT_scanf_arg (txt_a, GMT_io.in_col_type[GMT_X], &G->xp->segment[G->xp->n_segments]->coord[GMT_X][0]), txt_a);
					error += GMT_verify_expectations (GMT_io.in_col_type[GMT_Y], GMT_scanf_arg (txt_b, GMT_io.in_col_type[GMT_Y], &G->xp->segment[G->xp->n_segments]->coord[GMT_Y][0]), txt_b);
					error += GMT_code_to_lonlat (txt_c, &G->xp->segment[G->xp->n_segments]->coord[GMT_X][1], &G->xp->segment[G->xp->n_segments]->coord[GMT_Y][1]);
				}
				else {	/* Worked, pick up second point */
					error += GMT_verify_expectations (GMT_io.in_col_type[GMT_X], GMT_scanf_arg (txt_b, GMT_io.in_col_type[GMT_X], &G->xp->segment[G->xp->n_segments]->coord[GMT_X][1]), txt_b);
					error += GMT_verify_expectations (GMT_io.in_col_type[GMT_Y], GMT_scanf_arg (txt_c, GMT_io.in_col_type[GMT_Y], &G->xp->segment[G->xp->n_segments]->coord[GMT_Y][1]), txt_c);
				}
			}
			for (i = 0; i < 2; i++) {	/* Reset any zmin/max settings if used and applicable */
				if (G->xp->segment[G->xp->n_segments]->coord[GMT_X][i] == DBL_MAX) {	/* Meant zmax location */
					if (xyz) {
						G->xp->segment[G->xp->n_segments]->coord[GMT_X][i] = xyz[1][0];
						G->xp->segment[G->xp->n_segments]->coord[GMT_Y][i] = xyz[1][1];
					}
					else {
						error++;
						fprintf (stderr, "%s: GMT SYNTAX ERROR -%c:  z+ option not applicable here\n", GMT_program, G->flag);
					}
				}
				else if (G->xp->segment[G->xp->n_segments]->coord[GMT_X][i] == -DBL_MAX) {	/* Meant zmin location */
					if (xyz) {
						G->xp->segment[G->xp->n_segments]->coord[GMT_X][i] = xyz[0][0];
						G->xp->segment[G->xp->n_segments]->coord[GMT_Y][i] = xyz[0][1];
					}
					else {
						error++;
						fprintf (stderr, "%s: GMT SYNTAX ERROR -%c:  z- option not applicable here\n", GMT_program, G->flag);
					}
				}
			}
			if (G->do_interpolate) G->xp->segment[G->xp->n_segments]->n_rows = GMT_fix_up_path (&G->xp->segment[G->xp->n_segments]->coord[GMT_X], &G->xp->segment[G->xp->n_segments]->coord[GMT_Y], G->xp->segment[G->xp->n_segments]->n_rows, step, 0);
			for (i = 0; i < G->xp->segment[G->xp->n_segments]->n_rows; i++) {	/* Project */
				GMT_geo_to_xy (G->xp->segment[G->xp->n_segments]->coord[GMT_X][i], G->xp->segment[G->xp->n_segments]->coord[GMT_Y][i], &x, &y);
				G->xp->segment[G->xp->n_segments]->coord[GMT_X][i] = x;
				G->xp->segment[G->xp->n_segments]->coord[GMT_Y][i] = y;
			}
			G->xp->n_segments++;
			if (G->xp->n_segments == (int)n_alloc) {
				n_alloc += GMT_SMALL_CHUNK;
				G->xp->segment = (struct GMT_LINE_SEGMENT **) GMT_memory ((void *)G->xp->segment, n_alloc, sizeof (struct GMT_LINE_SEGMENT *), GMT_program);
			}
		}
		if (G->xp->n_segments < (int)n_alloc) {
			n_alloc += GMT_SMALL_CHUNK;
			G->xp->segment = (struct GMT_LINE_SEGMENT **) GMT_memory ((void *)G->xp->segment, (size_t)G->xp->n_segments, sizeof (struct GMT_LINE_SEGMENT *), GMT_program);
		}
	}
	else if (G->crossing == GMT_CONTOUR_XCURVE) {
		GMT_import_table ((void *)G->option, GMT_IS_FILE, &G->xp, 0.0, FALSE, FALSE, FALSE);
		for (k = 0; k < G->xp->n_segments; k++) {
			for (i = 0; i < G->xp->segment[k]->n_rows; i++) {	/* Project */
				GMT_geo_to_xy (G->xp->segment[k]->coord[GMT_X][i], G->xp->segment[k]->coord[GMT_Y][i], &x, &y);
				G->xp->segment[k]->coord[GMT_X][i] = x;
				G->xp->segment[k]->coord[GMT_Y][i] = y;
			}
		}
	}
	else if (G->fixed) {
		FILE *fp;
		int n_col, len;
		BOOLEAN bad_record = FALSE;
		double xy[2];

		if ((fp = GMT_fopen (G->file, "r")) == NULL) {
			fprintf (stderr, "%s: GMT SYNTAX ERROR -%c:  Could not open file %s\n", GMT_program, G->flag, G->file);
			error++;
		}
		n_col = (G->label_type == 5) ? 3 : 2;
		G->f_xy[0] = (double *) GMT_memory ((void *)VNULL, n_alloc, sizeof (double), GMT_program);
		G->f_xy[1] = (double *) GMT_memory ((void *)VNULL, n_alloc, sizeof (double), GMT_program);
		if (n_col == 3) G->f_label = (char **) GMT_memory ((void *)VNULL, n_alloc, sizeof (char *), GMT_program);
		G->f_n = 0;
		while (GMT_fgets (buffer, BUFSIZ, fp)) {
			if (buffer[0] == '#' || buffer[0] == '>' || buffer[0] == '\n') continue;
			len = strlen (buffer);
			for (i = len - 1; i >= 0 && strchr (" \t,\r\n", (int)buffer[i]); i--);
			buffer[++i] = '\n';	buffer[++i] = '\0';	/* Now have clean C string with \n\0 at end */
			sscanf (buffer, "%s %s %[^\n]", txt_a, txt_b, txt_c);	/* Get first 2-3 fields */
			if (GMT_scanf (txt_a, GMT_io.in_col_type[0], &xy[0]) == GMT_IS_NAN) bad_record = TRUE;	/* Got NaN or it failed to decode */
			if (GMT_scanf (txt_b, GMT_io.in_col_type[1], &xy[1]) == GMT_IS_NAN) bad_record = TRUE;	/* Got NaN or it failed to decode */
			if (bad_record) {
				GMT_io.n_bad_records++;
				if (GMT_io.give_report && (GMT_io.n_bad_records == 1)) {	/* Report 1st occurrence */
					fprintf (stderr, "%s: Encountered first invalid record near/at line # %d\n", GMT_program, GMT_io.rec_no);
					fprintf (stderr, "%s: Likely causes:\n", GMT_program);
					fprintf (stderr, "%s: (1) Invalid x and/or y values, i.e. NaNs or garbage in text strings.\n", GMT_program);
					fprintf (stderr, "%s: (2) Incorrect data type assumed if -J, -f are not set or set incorrectly.\n", GMT_program);
					fprintf (stderr, "%s: (3) The -: switch is implied but not set.\n", GMT_program);
					fprintf (stderr, "%s: (4) Input file in multiple segment format but the -M switch is not set.\n", GMT_program);
				}
				continue;
			}
			/* Got here if data are OK */

			if (gmtdefs.xy_toggle[0]) d_swap (xy[0], xy[1]);				/* Got lat/lon instead of lon/lat */
			GMT_map_outside (xy[0], xy[1]);
			if ( abs (GMT_x_status_new) > 1 || abs (GMT_y_status_new) > 1) continue;	/* Outside map region */

			GMT_geo_to_xy (xy[0], xy[1], &G->f_xy[0][G->f_n], &G->f_xy[1][G->f_n]);		/* Project -> xy inches */
			if (n_col == 3) {	/* The label part if asked for */
				G->f_label[G->f_n] = (char *) GMT_memory ((void *)VNULL, (size_t)(strlen(txt_c)+1), sizeof (char), GMT_program);
				strcpy (G->f_label[G->f_n], txt_c);
			}
			G->f_n++;
			if (G->f_n == (int)n_alloc) {
				n_alloc += GMT_SMALL_CHUNK;
				G->f_xy[0] = (double *) GMT_memory ((void *)G->f_xy[0], n_alloc, sizeof (double), GMT_program);
				G->f_xy[1] = (double *) GMT_memory ((void *)G->f_xy[1], n_alloc, sizeof (double), GMT_program);
				if (n_col == 3) G->f_label = (char **) GMT_memory ((void *)G->f_label, n_alloc, sizeof (char *), GMT_program);
			}
		}
		GMT_fclose (fp);
	}
	if (error) fprintf (stderr, "%s: GMT SYNTAX ERROR -%c:  Valid codes are [lcr][bmt] and z[+-]\n", GMT_program, G->flag);

	return (error);
}

void GMT_contlabel_angle (double x[], double y[], int start, int stop, double cangle, int n, struct GMT_LABEL *L, struct GMT_CONTOUR *G)
{
	int j, this_angle_type;
	double sum_x2 = 0.0, sum_xy = 0.0, sum_y2 = 0.0, dx, dy;

	if (start == stop) {	/* Can happen if we want no smoothing but landed exactly on a knot point */
		if (start > 0)
			start--;
		else if (stop < (n-1))
			stop++;
	}
	for (j = start - G->half_width; j <= stop + G->half_width; j++) {	/* L2 fit for slope over this range of points */
		if (j < 0 || j >= n) continue;
		dx = x[j] - L->x;
		dy = y[j] - L->y;
		sum_x2 += dx * dx;
		sum_y2 += dy * dy;
		sum_xy += dx * dy;
	}
	if (sum_y2 < GMT_CONV_LIMIT)	/* Line is horizontal */
		L->line_angle = 0.0;
	else if (sum_x2 < GMT_CONV_LIMIT)	/* Line is vertical */
		L->line_angle = 90.0;
	else
		L->line_angle = (GMT_IS_ZERO (sum_xy)) ? 90.0 : d_atan2 (sum_xy, sum_x2) * R2D;
	this_angle_type = G->angle_type;
	if (this_angle_type == 2) {	/* Just return the fixed angle given (unless NaN) */
		if (GMT_is_dnan (cangle)) /* Cannot use this angle - default to along-line angle */
			this_angle_type = 0;
		else
			L->angle = cangle;
	}
	if (this_angle_type != 2) {	/* Must base label angle on the contour angle */
		L->angle = L->line_angle + this_angle_type * 90.0;	/* May add 90 to get normal */
		if (L->angle < 0.0) L->angle += 360.0;
		if (L->angle > 90.0 && L->angle < 270) L->angle -= 180.0;
	}
}

struct GMT_LABEL * GMT_contlabel_new (void)
{
	/* Allocate space for one label structure using prev pointer (unless NULL).
	 * np is the number of points for x/y point */

	struct GMT_LABEL *L;
	L = (struct GMT_LABEL *) GMT_memory (VNULL, (size_t)1, sizeof (struct GMT_LABEL), GMT_program);
	return (L);
}

void GMT_contlabel_fixpath (double **xin, double **yin, double d[], int *n, struct GMT_CONTOUR *G)
{	/* Sorts labels based on distance and inserts the label (x,y) point into the x,y path */
	int i, j, k, np;
	double *xp, *yp, *x, *y;

	if (G->n_label == 0) return;	/* No labels, no need to insert points */

	/* Sort lables based on distance along contour if more than 1 */
	if (G->n_label > 1) qsort((void *)G->L, (size_t)G->n_label, sizeof (struct GMT_LABEL *), sort_label_struct);

	np = *n + G->n_label;	/* Lenght of extended path that includes inserted label coordinates */
	xp = (double *) GMT_memory (VNULL, (size_t)np, sizeof (double), GMT_program);
	yp = (double *) GMT_memory (VNULL, (size_t)np, sizeof (double), GMT_program);
	x = *xin;	y = *yin;	/* Input coordinate arrays */

	/* Make sure the xp, yp array contains the exact points at which labels should be placed */

	for (k = 0, i = j = 0; i < *n && j < np && k < G->n_label; k++) {
		while (i < *n && d[i] < G->L[k]->dist) {	/* Not at the label point yet - just copy points */
			xp[j] = x[i];
			yp[j] = y[i];
			j++;
			i++;
		}
		/* Add the label point */
		G->L[k]->node = j;		/* Update node since we have been adding new points */
		xp[j] = G->L[k]->x;
		yp[j] = G->L[k]->y;
		j++;
	}
	while (i < *n) {	/* Append the rest of the path */
		xp[j] = x[i];
		yp[j] = y[i];
		j++;
		i++;
	}

	GMT_free ((void *)x);	/* Get rid of old path... */
	GMT_free ((void *)y);

	*xin = xp;		/* .. and pass out new path */
	*yin = yp;

	*n = np;		/* and the new length */
}

void GMT_contlabel_addpath (double x[], double y[], int n, double zval, char *label, BOOLEAN annot, struct GMT_CONTOUR *G)
{
	int i;
	double s = 0.0, c = 1.0, sign = 1.0;
	struct GMT_CONTOUR_LINE *C;
	/* Adds this segment to the list of contour lines */

	if (G->n_alloc == 0 || G->n_segments == (int)G->n_alloc) {
		G->n_alloc += GMT_SMALL_CHUNK;
		G->segment = (struct GMT_CONTOUR_LINE **) GMT_memory ((void *)G->segment, G->n_alloc, sizeof (struct GMT_CONTOUR_LINE *), GMT_program);
	}
	G->segment[G->n_segments] = (struct GMT_CONTOUR_LINE *) GMT_memory (VNULL, 1, sizeof (struct GMT_CONTOUR_LINE), GMT_program);
	C = G->segment[G->n_segments];	/* Pointer to current segment */
	C->n = n;
	C->x = (double *) GMT_memory (VNULL, (size_t)C->n, sizeof (double), GMT_program);
	C->y = (double *) GMT_memory (VNULL, (size_t)C->n, sizeof (double), GMT_program);
	memcpy ((void *)C->x, (void *)x, (size_t)(C->n * sizeof (double)));
	memcpy ((void *)C->y, (void *)y, (size_t)(C->n * sizeof (double)));
	memcpy ((void *)&C->pen, (void *)&G->line_pen, sizeof (struct GMT_PEN));
	memcpy ((void *)&C->font_rgb, (void *)&G->font_rgb, (size_t)(3*sizeof (int)));
	C->name = (char *) GMT_memory (VNULL, (size_t)(strlen (label)+1), sizeof (char), GMT_program);
	strcpy (C->name, label);
	C->annot = annot;
	C->z = zval;
	if (G->n_label) {	/* There are labels */
		C->n_labels = G->n_label;
		C->L = (struct GMT_LABEL *) GMT_memory (VNULL, (size_t)C->n_labels, sizeof (struct GMT_LABEL), GMT_program);
		for (i = 0; i < C->n_labels; i++) {
			C->L[i].x = G->L[i]->x;
			C->L[i].y = G->L[i]->y;
			C->L[i].line_angle = G->L[i]->line_angle;
			if (G->nudge_flag) {	/* Must adjust point a bit */
				if (G->nudge_flag == 2) sincosd (C->L[i].line_angle, &s, &c);
				/* If N+1 or N-1 is used we want positive x nudge to extend away from end point */
				sign = (G->number_placement) ? (double)C->L[i].end : 1.0;
				C->L[i].x += sign * (G->nudge[GMT_X] * c - G->nudge[GMT_Y] * s);
				C->L[i].y += sign * (G->nudge[GMT_X] * s + G->nudge[GMT_Y] * c);
			}
			C->L[i].angle = G->L[i]->angle;
			C->L[i].dist = G->L[i]->dist;
			C->L[i].node = G->L[i]->node;
			C->L[i].label = (char *) GMT_memory (VNULL, (size_t)(strlen (G->L[i]->label)+1), sizeof (char), GMT_program);
			strcpy (C->L[i].label, G->L[i]->label);
		}
	}
	G->n_segments++;
}

int sort_label_struct (const void *p_1, const void *p_2)
{
	struct GMT_LABEL **point_1, **point_2;

	point_1 = (struct GMT_LABEL **)p_1;
	point_2 = (struct GMT_LABEL **)p_2;
	if ((*point_1)->dist < (*point_2)->dist) return -1;
	if ((*point_1)->dist > (*point_2)->dist) return +1;
	return 0;
}

int GMT_code_to_lonlat (char *code, double *lon, double *lat)
{
	int i, n, error = 0;
	BOOLEAN z_OK = FALSE;

	n = strlen (code);
	if (n != 2) return (1);

	for (i = 0; i < 2; i++) {
		switch (code[i]) {
			case 'l':
			case 'L':	/* Left */
				*lon = project_info.w;
				break;
			case 'c':
			case 'C':	/* center */
				*lon = 0.5 * (project_info.w + project_info.e);
				break;
			case 'r':
			case 'R':	/* right */
				*lon = project_info.e;
				break;
			case 'b':
			case 'B':	/* bottom */
				*lat = project_info.s;
				break;
			case 'm':
			case 'M':	/* center */
				*lat = 0.5 * (project_info.s + project_info.n);
				break;
			case 't':
			case 'T':	/* top */
				*lat = project_info.n;
				break;
			case 'z':
			case 'Z':	/* z-value */
				z_OK = TRUE;
				break;
			case '+':	/* zmax-location */
				if (z_OK)
					*lon = *lat = DBL_MAX;
				else
					error++;
				break;
			case '-':	/* zmin-location */
				if (z_OK)
					*lon = *lat = -DBL_MAX;
				else
					error++;
				break;
			default:
				error++;
				break;
		}
	}
	return (error);
}

void GMT_get_radii_of_curvature (double x[], double y[], int n, double r[])
{
	/* Calculates radius of curvature along the spatial curve x(t), y(t) */

	int i, im, ip;
	double a, b, c, d, e, f, x0, y0, denom;

	for (im = 0, i = 1, ip = 2; ip < n; i++, im++, ip++) {
		a = (x[im] - x[i]);	b = (y[im] - y[i]);	e = 0.5 * (x[im] * x[im] + y[im] * y[im] - x[i] * x[i] - y[i] * y[i]);
		c = (x[i] - x[ip]);	d = (y[i] - y[ip]);	f = 0.5 * (x[i] * x[i] + y[i] * y[i] - x[ip] * x[ip] - y[ip] * y[ip]);
		denom = b * c - a * d;;
		if (denom == 0.0)
			r[i] = DBL_MAX;
		else {
			x0 = (-d * e + b * f) / denom;
			y0 = (c * e - a * f) / denom;
			r[i] = hypot (x[i] - x0, y[i] - y0);
		}
	}
	r[0] = r[n-1] = DBL_MAX;	/* Boundary conditions has zero curvature at end points so r = inf */
}

#define CONTOUR_IJ(i,j) ((i) + (j) * nx)

int GMT_contours (float *grd, struct GRD_HEADER *header, int smooth_factor, int int_scheme, int orient, int *side, int *edge, int first, double **x_array, double **y_array)
{
	/* The routine finds the zero-contour in the grd dataset.  it assumes that
	 * no node has a value exactly == 0.0.  If more than max points are found
	 * GMT_trace_contour will try to allocate more memory in blocks of GMT_CHUNK points.
	 * orient arranges the contour so that the values to the left of the contour is higher (orient = 1)
	 * or lower (orient = -1) than the contour value.
	 */

	static int i0, j0;
	int i, j, ij, n = 0, n2, n_edges, edge_word, edge_bit, nx, ny, nans = 0;
	BOOLEAN go_on = TRUE;
	float z[2];
	double x0, y0, r, west, east, south, north, dx, dy, xinc2, yinc2, *x, *y, *x2, *y2;
	int p[5], i_off[5], j_off[5], k_off[5], offset;
	unsigned int bit[32];

	nx = header->nx;	ny = header->ny;
	west = header->x_min;	east = header->x_max;
	south = header->y_min;	north = header->y_max;
	dx = header->x_inc;	dy = header->y_inc;
	xinc2 = (header->node_offset) ? 0.5 * dx : 0.0;
	yinc2 = (header->node_offset) ? 0.5 * dy : 0.0;
	x = *x_array;	y = *y_array;

	n_edges = ny * (int) ceil (nx / 16.0);
	offset = n_edges / 2;

	/* Reset edge-flags to zero, if necessary */
	if (first) {	/* Set i0,j0 for southern boundary */
		memset ((void *)edge, 0, (size_t)(n_edges * sizeof (int)));
		i0 = 0;
		j0 = ny - 1;
	}
	p[0] = p[4] = 0;	p[1] = 1;	p[2] = 1 - nx;	p[3] = -nx;
	i_off[0] = i_off[2] = i_off[3] = i_off[4] = 0;	i_off[1] =  1;
	j_off[0] = j_off[1] = j_off[3] = j_off[4] = 0;	j_off[2] = -1;
	k_off[0] = k_off[2] = k_off[4] = 0;	k_off[1] = k_off[3] = 1;
	for (i = 1, bit[0] = 1; i < 32; i++) bit[i] = bit[i-1] << 1;

	switch (*side) {
		case 0:	/* Southern boundary */
			y0 = GMT_j_to_y (header->ny-1, header->y_min, header->y_max, header->y_inc, header->xy_off, header->ny);
			for (i = i0, j = j0, ij = (ny - 1) * nx + i0; go_on && i < nx-1; i++, ij++) {
				edge_word = ij / 32;
				edge_bit = ij % 32;
				z[0] = grd[ij+1];
				z[1] = grd[ij];
				if (GMT_z_periodic) GMT_setcontjump (z, 2);
				if (GMT_start_trace (z[0], z[1], edge, edge_word, edge_bit, bit)) { /* Start tracing contour */
					r = z[0] - z[1];
					x0 = west + (i - z[1]/r)*dx + xinc2;
					edge[edge_word] |= bit[edge_bit];
					n = GMT_trace_contour (grd, header, x0, y0, edge, &x, &y, i, j, 0, offset, i_off, j_off, k_off, p, bit, &nans);
					if (orient) GMT_orient_contour (grd, header, x, y, n, orient);
					n = GMT_smooth_contour (&x, &y, n, smooth_factor, int_scheme);
					go_on = FALSE;
					i0 = i + 1;	j0 = j;
				}
			}
			if (n == 0) {
				i0 = nx - 2;
				j0 = ny - 1;
				(*side)++;
			}
			break;

		case 1:		/* Eastern boundary */

			x0 = GMT_i_to_x (header->nx-1, header->x_min, header->x_max, header->x_inc, header->xy_off, header->nx);
			for (j = j0, ij = nx * (j0 + 1) - 1, i = i0; go_on && j > 0; j--, ij -= nx) {
				edge_word = ij / 32 + offset;
				edge_bit = ij % 32;
				z[0] = grd[ij-nx];
				z[1] = grd[ij];
				if (GMT_z_periodic) GMT_setcontjump (z, 2);
				if (GMT_start_trace (z[0], z[1], edge, edge_word, edge_bit, bit)) { /* Start tracing contour */
					r = z[1] - z[0];
					y0 = north - (j - z[1]/r) * dy - yinc2;
					edge[edge_word] |= bit[edge_bit];
					n = GMT_trace_contour (grd, header, x0, y0, edge, &x, &y, i, j, 1, offset, i_off, j_off, k_off, p, bit, &nans);
					if (orient) GMT_orient_contour (grd, header, x, y, n, orient);
					n = GMT_smooth_contour (&x, &y, n, smooth_factor, int_scheme);
					go_on = FALSE;
					i0 = i;	j0 = j - 1;
				}
			}
			if (n == 0) {
				i0 = nx - 2;
				j0 = 1;
				(*side)++;
			}
			break;

		case 2:		/* Northern boundary */

			y0 = GMT_j_to_y (0, header->y_min, header->y_max, header->y_inc, header->xy_off, header->ny);
			for (i = i0, j = j0, ij = i0; go_on && i >= 0; i--, ij--) {
				edge_word = ij / 32;
				edge_bit = ij % 32;
				z[0] = grd[ij];
				z[1] = grd[ij+1];
				if (GMT_z_periodic) GMT_setcontjump (z, 2);
				if (GMT_start_trace (z[0], z[1], edge, edge_word, edge_bit, bit)) { /* Start tracing contour */
					r = z[1] - z[0];
					x0 = west + (i - z[0]/r)*dx + xinc2;
					edge[edge_word] |= bit[edge_bit];
					n = GMT_trace_contour (grd, header, x0, y0, edge, &x, &y, i, j, 2, offset, i_off, j_off, k_off, p, bit, &nans);
					if (orient) GMT_orient_contour (grd, header, x, y, n, orient);
					n = GMT_smooth_contour (&x, &y, n, smooth_factor, int_scheme);
					go_on = FALSE;
					i0 = i - 1;	j0 = j;
				}
			}
			if (n == 0) {
				i0 = 0;
				j0 = 1;
				(*side)++;
			}
			break;

		case 3:		/* Western boundary */

			x0 = GMT_i_to_x (0, header->x_min, header->x_max, header->x_inc, header->xy_off, header->nx);
			for (j = j0, ij = j * nx, i = i0; go_on && j < ny; j++, ij += nx) {
				edge_word = ij / 32 + offset;
				edge_bit = ij % 32;
				z[0] = grd[ij];
				z[1] = grd[ij-nx];
				if (GMT_z_periodic) GMT_setcontjump (z, 2);
				if (GMT_start_trace (z[0], z[1], edge, edge_word, edge_bit, bit)) { /* Start tracing contour */
					r = z[0] - z[1];
					y0 = north - (j - z[0] / r) * dy - yinc2;
					edge[edge_word] |= bit[edge_bit];
					n = GMT_trace_contour (grd, header, x0, y0, edge, &x, &y, i, j, 3, offset, i_off, j_off, k_off, p, bit, &nans);
					if (orient) GMT_orient_contour (grd, header, x, y, n, orient);
					n = GMT_smooth_contour (&x, &y, n, smooth_factor, int_scheme);
					go_on = FALSE;
					i0 = i;	j0 = j + 1;
				}
			}
			if (n == 0) {
				i0 = 1;
				j0 = 1;
				(*side)++;
			}
			break;

		case 4:	/* Then loop over interior boxes (vertical edges) */

			for (j = j0; go_on && j < ny; j++) {
				ij = i0 + j * nx;
				for (i = i0; go_on && i < nx-1; i++, ij++) {
					edge_word = ij / 32 + offset;
					z[0] = grd[ij];
					z[1] = grd[ij-nx];
					edge_bit = ij % 32;
					if (GMT_z_periodic) GMT_setcontjump (z, 2);
					if (GMT_start_trace (z[0], z[1], edge, edge_word, edge_bit, bit)) { /* Start tracing contour */
						r = z[0] - z[1];
						x0 = west + i*dx + xinc2;
						y0 = north - (j - z[0] / r) * dy - yinc2;
						/* edge[edge_word] |= bit[edge_bit]; */
						nans = 0;
						n = GMT_trace_contour (grd, header, x0, y0, edge, &x, &y, i, j, 3, offset, i_off, j_off, k_off, p, bit, &nans);
						if (nans) {	/* Must trace in other direction, then splice */
							n2 = GMT_trace_contour (grd, header, x0, y0, edge, &x2, &y2, i-1, j, 1, offset, i_off, j_off, k_off, p, bit, &nans);
							n = GMT_splice_contour (&x, &y, n, &x2, &y2, n2);
							GMT_free ((void *)x2);
							GMT_free ((void *)y2);
						}
						if (orient) GMT_orient_contour (grd, header, x, y, n, orient);
						n = GMT_smooth_contour (&x, &y, n, smooth_factor, int_scheme);
						i0 = i + 1;
						go_on = FALSE;
						j0 = j;
					}
				}
				if (go_on) i0 = 1;
			}
			if (n == 0) {
				i0 = 0;
				j0 = 1;
				(*side)++;
			}
			break;

		case 5:	/* Then loop over interior boxes (horizontal edges) */

			for (j = j0; go_on && j < ny; j++) {
				ij = i0 + j * nx;
				for (i = i0; go_on && i < nx-1; i++, ij++) {
					edge_word = ij / 32 + offset;
					z[0] = grd[ij];
					z[1] = grd[ij+1];
					edge_bit = ij % 32;
					if (GMT_z_periodic) GMT_setcontjump (z, 2);
					if (GMT_start_trace (z[0], z[1], edge, edge_word, edge_bit, bit)) { /* Start tracing contour */
						r = z[1] - z[0];
						x0 = west + (i - z[0]/r)*dx + xinc2;
						y0 = north - j * dy - yinc2;
						edge[edge_word] |= bit[edge_bit];
						nans = 0;
						n = GMT_trace_contour (grd, header, x0, y0, edge, &x, &y, i, j, 2, offset, i_off, j_off, k_off, p, bit, &nans);
						if (nans) {	/* Must trace in other direction, then splice */
							n2 = GMT_trace_contour (grd, header, x0, y0, edge, &x2, &y2, i-1, j, 0, offset, i_off, j_off, k_off, p, bit, &nans);
							n = GMT_splice_contour (&x, &y, n, &x2, &y2, n2);
							GMT_free ((void *)x2);
							GMT_free ((void *)y2);
						}
						if (orient) GMT_orient_contour (grd, header, x, y, n, orient);
						n = GMT_smooth_contour (&x, &y, n, smooth_factor, int_scheme);
						i0 = i + 1;
						go_on = FALSE;
						j0 = j;
					}
				}
				if (go_on) i0 = 1;
			}
			if (n == 0) (*side)++;
			break;

		default:
			break;
	}
	*x_array = x;	*y_array = y;
	return (n);
}

int GMT_start_trace (float first, float second, int *edge, int edge_word, int edge_bit, unsigned int *bit)
{
	/* First make sure we haven't been here before */

	if ( (edge[edge_word] & bit[edge_bit]) ) return (FALSE);

	/* Then make sure both points are real numbers */

	if (GMT_is_fnan (first) ) return (FALSE);
	if (GMT_is_fnan (second) ) return (FALSE);

	/* Finally return TRUE if values of opposite sign */

	return ( first * second < 0.0);
}

int GMT_splice_contour (double **x, double **y, int n, double **x2, double **y2, int n2)
{	/* Basically does a "tail -r" on the array x,y and append arrays x2/y2 */

	int i, j, m;
	double *xtmp, *ytmp, *xa, *ya, *xb, *yb;

	xa = *x;	ya = *y;
	xb = *x2;	yb = *y2;

	m = n + n2 - 1;	/* Total length since one point is shared */

	/* First copy and reverse first contour piece */

	xtmp = (double *) GMT_memory (VNULL, (size_t)n , sizeof (double), "GMT_splice_contour");
	ytmp = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "GMT_splice_contour");

	memcpy ((void *)xtmp, (void *)xa, (size_t)(n * sizeof (double)));
	memcpy ((void *)ytmp, (void *)ya, (size_t)(n * sizeof (double)));

	xa = (double *) GMT_memory ((void *)xa, (size_t)m, sizeof (double), "GMT_splice_contour");
	ya = (double *) GMT_memory ((void *)ya, (size_t)m, sizeof (double), "GMT_splice_contour");

	for (i = 0; i < n; i++) xa[i] = xtmp[n-i-1];
	for (i = 0; i < n; i++) ya[i] = ytmp[n-i-1];

	/* Then append second piece */

	for (j = 1, i = n; j < n2; j++, i++) xa[i] = xb[j];
	for (j = 1, i = n; j < n2; j++, i++) ya[i] = yb[j];

	GMT_free ((void *)xtmp);
	GMT_free ((void *)ytmp);

	*x = xa;
	*y = ya;

	return (m);
}

void GMT_orient_contour (float *grd, struct GRD_HEADER *h, double *x, double *y, int n, int orient)
{
	/* Determine handedness of the contour and if opposite of orient reverse the contour */
	BOOLEAN reverse;
	int i, j, k, k2, side[2], z_dir;
	double fx[2], fy[2], dx, dy;

	if (n < 2) return;	/* Cannot work on a single point */

	for (k = 0; k < 2; k++) {	/* Calculate fractional node numbers from left/top */
		fx[k] = (x[k] - h->x_min) / h->x_inc - h->xy_off;
		fy[k] = (h->y_max - y[k]) / h->y_inc - h->xy_off;
	}

	/* Get(i,j) of the lower left node in the rectangle containing this contour segment.
	   We use the average x and y coordinate for this to avoid any round-off involved in
	   working on a single coordinate. The average coordinate should always be inside the
	   rectangle and hence the floor/ceil operators will yield the LL node. */

	i = (int) floor (0.5 * (fx[0] + fx[1]));
	j = (int) ceil  (0.5 * (fy[0] + fy[1]));

	z_dir = (grd[j*h->nx+i] > 0.0) ? +1 : -1;	/* +1 if lower-left node is higher than contour value, else -1 */

	for (k = 0; k < 2; k++) {	/* Determine which edge the contour points lie on (0-3) */
		/* We KNOW that for each k, either x[k] or y[k] lies EXACTLY on a gridline.  This is used
		 * to deal with the inevitable round-off that places points slightly off the gridline.  We
		 * pick the coordinate closest to the gridline as the one that should be exactly on the gridline */

		k2 = 1 - k;	/* The other point */
		dx = fmod (fx[k], 1.0);
		if (dx > 0.5) dx = 1.0 - dx;	/* Fraction to closest vertical gridline */
		dy = fmod (fy[k], 1.0);
		if (dy > 0.5) dy = 1.0 - dy;	/* Fraction to closest horizontal gridline */
		if (dx < dy)		/* Point is on a vertical grid line (left [3] or right [1]) */
			side[k] = (fx[k] < fx[k2]) ? 3 : 1;	/* Simply check order of fx to determine which it is */
		else						/* Point must be on horizontal grid line (top [2] or bottom [0]) */
			side[k] = (fy[k] > fy[k2]) ? 0 : 2;	/* Same for fy */
	}

	switch (side[0]) {	/* Entry side */
		case 0:	/* Bottom: Regardless of exit the LL node is to the left of line vector */
			reverse = (z_dir == orient);
			break;
		case 1:	/* Right: Must check exit */
			switch (side[1]) {
				case  0:	/* Bottom: LL Node is the the right of vector */
					reverse = (z_dir != orient);
					break;
				default:	/* left and top it is to the left */
					reverse = (z_dir == orient);
					break;
			}
			break;
		case 2:	/* Top: Must check exit */
			switch (side[1]) {
				case 3:		/* Left: LL node is to the left of vector */
					reverse = (z_dir == orient);
					break;
				default:	/* Bottom and right: LL node is to the right of vector */
					reverse = (z_dir != orient);
					break;
			}
			break;
		default:/* Left: LL node is always to the right of line vector */
			reverse = (z_dir != orient);
			break;
	}

	if (reverse) {	/* Must reverse order of contour */
		for (i = 0, j = n-1; i < n/2; i++, j--) {
			d_swap (x[i], x[j]);
			d_swap (y[i], y[j]);
		}
	}
}

int GMT_trace_contour (float *grd, struct GRD_HEADER *header, double x0, double y0, int *edge, double **x_array, double **y_array, int i, int j, int kk, int offset, int *i_off, int *j_off, int *k_off, int *p, unsigned int *bit, int *nan_flag)
{
	int side = 0, n = 1, k, k0, ij, ij0, n_cuts, k_index[2], kk_opposite, more;
	int edge_word, edge_bit, m, n_nan, nx, ny, n_alloc, ij_in;
	float z[5];
	double xk[4], yk[4], r, dr[2];
	double west, north, dx, dy, xinc2, yinc2, *xx, *yy;

	west = header->x_min;	north = header->y_max;
	dx = header->x_inc;	dy = header->y_inc;
	nx = header->nx;	ny = header->ny;
	xinc2 = (header->node_offset) ? 0.5 * dx : 0.0;
	yinc2 = (header->node_offset) ? 0.5 * dy : 0.0;

	n_alloc = GMT_CHUNK;
	m = n_alloc - 2;

	xx = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), "GMT_trace_contour");
	yy = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), "GMT_trace_contour");

	xx[0] = x0;	yy[0] = y0;
	ij_in = j * nx + i - 1;

	more = TRUE;
	do {
		ij = i + j * nx;
		x0 = west + i * dx + xinc2;
		y0 = north - j * dy - yinc2;
		n_cuts = 0;
		k0 = kk;
		for (k = 0; k < 5; k++) z[k] = grd[ij+p[k]];	/* Copy the 4 corners */
		if (GMT_z_periodic) GMT_setcontjump (z, 5);

		for (k = n_nan = 0; k < 4; k++) {	/* Loop over box sides */

			/* Skip where we already have a cut (k == k0) */

			if (k == k0) continue;

			/* Skip if NAN encountered */

			if (GMT_is_fnan (z[k+1]) || GMT_is_fnan (z[k])) {
				n_nan++;
				continue;
			}

			/* Skip edge already has been used */

			ij0 = CONTOUR_IJ (i + i_off[k], j + j_off[k]);
			edge_word = ij0 / 32 + k_off[k] * offset;
			edge_bit = ij0 % 32;
			if (edge[edge_word] & bit[edge_bit]) continue;

			/* Skip if no zero-crossing on this edge */

			if (z[k+1] * z[k] > 0.0) continue;

			/* Here we have a crossing */

			r = z[k+1] - z[k];

			if (k%2) {	/* Cutting a S-N line */
				if (k == 1) {
					xk[1] = x0 + dx;
					yk[1] = y0 - z[k]*dy/r;
				}
				else {
					xk[3] = x0;
					yk[3] = y0 + (1.0 + z[k]/r)*dy;
				}
			}
			else {	/* Cutting a E-W line */
				if (k == 0) {
					xk[0] = x0 - z[k]*dx/r;
					yk[0] = y0;
				}
				else {
					xk[2] = x0 + (1.0 + z[k]/r)*dx;
					yk[2] = y0 + dy;
				}
			}
			kk = k;
			n_cuts++;
		}

		if (n > m) {	/* Must try to allocate more memory */
			n_alloc += GMT_CHUNK;
			m += GMT_CHUNK;
			xx = (double *) GMT_memory ((void *)xx, (size_t)n_alloc, sizeof (double), "GMT_trace_contour");
			yy = (double *) GMT_memory ((void *)yy, (size_t)n_alloc, sizeof (double), "GMT_trace_contour");
		}
		if (n_cuts == 0) {	/* Close interior contour and return */
		/*	if (n_nan == 0 && (n > 0 && fabs (xx[n-1] - xx[0]) <= dx && fabs (yy[n-1] - yy[0]) <= dy)) { */
			if (ij == ij_in) {	/* Close interior contour */
				xx[n] = xx[0];
				yy[n] = yy[0];
				n++;
			}
			more = FALSE;
			*nan_flag = n_nan;
		}
		else if (n_cuts == 1) {	/* Draw a line to this point and keep tracing */
			xx[n] = xk[kk];
			yy[n] = yk[kk];
			n++;
		}
		else {	/* Saddle point, we decide to connect to the point nearest previous point */
			kk_opposite = (k0 + 2) % 4;	/* But it cannot be on opposite side */
			for (k = side = 0; k < 4; k++) {
				if (k == k0 || k == kk_opposite) continue;
				dr[side] = (xx[n-1]-xk[k])*(xx[n-1]-xk[k]) + (yy[n-1]-yk[k])*(yy[n-1]-yk[k]);
				k_index[side++] = k;
			}
			kk = (dr[0] < dr[1]) ? k_index[0] : k_index[1];
			xx[n] = xk[kk];
			yy[n] = yk[kk];
			n++;
		}
		if (more) {	/* Mark this edge as used */
			ij0 = CONTOUR_IJ (i + i_off[kk], j + j_off[kk]);
			edge_word = ij0 / 32 + k_off[kk] * offset;
			edge_bit = ij0 % 32;
			edge[edge_word] |= bit[edge_bit];
		}

		if ((kk == 0 && j == ny - 1) || (kk == 1 && i == nx - 2) ||
			(kk == 2 && j == 1) || (kk == 3 && i == 0))	/* Going out of grid */
			more = FALSE;

		/* Get next box (i,j,kk) */

		i -= (kk-2)%2;
		j -= (kk-1)%2;
		kk = (kk+2)%4;

	} while (more);

	xx = (double *) GMT_memory ((void *)xx, (size_t)n, sizeof (double), "GMT_trace_contour");
	yy = (double *) GMT_memory ((void *)yy, (size_t)n, sizeof (double), "GMT_trace_contour");

	*x_array = xx;	*y_array = yy;
	return (n);
}

int GMT_smooth_contour (double **x_in, double **y_in, int n, int sfactor, int stype)
{
	/* Input (x,y) points */
	/* Number of input points */
	/* n_out = sfactor * n -1 */
        /* Interpolation scheme used (0 = linear, 1 = Akima, 2 = Cubic spline, 3 = None */
	int i, j, k, n_out;
	double ds, t_next, *x, *y;
	double *t_in, *t_out, *x_tmp, *y_tmp, x0, x1, y0, y1;
	char *flag;

	if (sfactor == 0 || n < 4) return (n);	/* Need at least 4 points to call Akima */

	x = *x_in;	y = *y_in;

	n_out = sfactor * n - 1;	/* Number of new points */

	t_in = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "GMT_smooth_contour");
	t_out = (double *) GMT_memory (VNULL, (size_t)(n_out + n), sizeof (double), "GMT_smooth_contour");
	x_tmp = (double *) GMT_memory (VNULL, (size_t)(n_out + n), sizeof (double), "GMT_smooth_contour");
	y_tmp = (double *) GMT_memory (VNULL, (size_t)(n_out + n), sizeof (double), "GMT_smooth_contour");
	flag = (char *)GMT_memory (VNULL, (size_t)(n_out + n), sizeof (char), "GMT_smooth_contour");

	/* Create dummy distance values for input points, and throw out duplicate points while at it */

	t_in[0] = 0.0;
	for (i = j = 1; i < n; i++)	{
		ds = hypot ((x[i]-x[i-1]), (y[i]-y[i-1]));
		if (ds > 0.0) {
			t_in[j] = t_in[j-1] + ds;
			x[j] = x[i];
			y[j] = y[i];
			j++;
		}
	}

	n = j;	/* May have lost some duplicates */
	if (sfactor == 0 || n < 4) return (n);	/* Need at least 4 points to call Akima */

	/* Create equidistance output points */

	ds = t_in[n-1] / (n_out-1);
	t_next = ds;
	t_out[0] = 0.0;
	flag[0] = TRUE;
	for (i = j = 1; i < n_out; i++) {
		if (j < n && t_in[j] < t_next) {
			t_out[i] = t_in[j];
			flag[i] = TRUE;
			j++;
			n_out++;
		}
		else {
			t_out[i] = t_next;
			t_next += ds;
		}
	}
	t_out[n_out-1] = t_in[n-1];
	if (t_out[n_out-1] == t_out[n_out-2]) n_out--;
	flag[n_out-1] = TRUE;

	GMT_intpol (t_in, x, n, n_out, t_out, x_tmp, stype);
	GMT_intpol (t_in, y, n, n_out, t_out, y_tmp, stype);

	/* Make sure interpolated function is bounded on each segment interval */

	i = 0;
	while (i < (n_out - 1)) {
		j = i + 1;
		while (j < n_out && !flag[j]) j++;
		x0 = x_tmp[i];	x1 = x_tmp[j];
		if (x0 > x1) d_swap (x0, x1);
		y0 = y_tmp[i];	y1 = y_tmp[j];
		if (y0 > y1) d_swap (y0, y1);
		for (k = i + 1; k < j; k++) {
			if (x_tmp[k] < x0)
				x_tmp[k] = x0 + 1.0e-10;
			else if (x_tmp[k] > x1)
				x_tmp[k] = x1 - 1.0e-10;
			if (y_tmp[k] < y0)
				y_tmp[k] = y0 + 1.0e-10;
			else if (y_tmp[k] > y1)
				y_tmp[k] = y1 - 1.0e-10;
		}
		i = j;
	}

	GMT_free ((void *)x);
	GMT_free ((void *)y);

	*x_in = x_tmp;
	*y_in = y_tmp;

	GMT_free ((void *) t_in);
	GMT_free ((void *) t_out);
	GMT_free ((void *) flag);

	return (n_out);
}

void GMT_dump_contour (double *xx, double *yy, int nn, double cval, int id, BOOLEAN interior, char *file)
{
	int i;
	static int int_cont_count = 0, ext_cont_count = 0;
	char fname[BUFSIZ], format[GMT_TEXT_LEN], suffix[4];
	double out[3];
	FILE *fp;

	if (nn < 2) return;

	out[2] = cval;
	(GMT_io.binary[1]) ? strcpy (suffix, "b") : strcpy (suffix, "xyz");
	sprintf (format, "%s\t%s\t%s\n", gmtdefs.d_format, gmtdefs.d_format, gmtdefs.d_format);
	if (!GMT_io.binary[1] && GMT_io.multi_segments[GMT_OUT]) {
		if (GMT_io.multi_segments[GMT_OUT] == 2) {	/* Must create file the first time around */
			fp = GMT_fopen (file, "w");
			GMT_io.multi_segments[GMT_OUT] = TRUE;
		}
		else	/* Later we append to it */
			fp = GMT_fopen (file, "a+");
		sprintf (GMT_io.segment_header, "%c %g contour -Z%g\n", GMT_io.EOF_flag[GMT_OUT], cval, cval);
		GMT_write_segmentheader (fp, 3);
	}
	else {
		if (interior) {
			if (file[0] == '-')	/* Running numbers only */
				sprintf (fname, "C%d_i.%s", int_cont_count++, suffix);
			else
				sprintf (fname, "%s_%g_%d_i.%s", file, cval, id, suffix);
		}
		else {
			if (file[0] == '-')	/* Running numbers only */
				sprintf (fname, "C%d_e.%s", ext_cont_count++, suffix);
			else
				sprintf (fname, "%s_%g_%d.%s", file, cval, id, suffix);
		}
		fp = GMT_fopen (fname, GMT_io.w_mode);
	}
	for (i = 0; i < nn; i++) {
		out[0] = xx[i];	out[1] = yy[i];
		GMT_output (fp, 3, out);
	}
	GMT_fclose (fp);
}

void GMT_hold_contour (double **xxx, double **yyy, int nn, double zval, char *label, char ctype, double cangle, int closed, struct GMT_CONTOUR *G)
{	/* The xx, yy are expected to be projected x/y inches.
	 * This function just makes sure that the xxx/yyy are continuous and do not have map jumps.
	 * If there are jumps we find them and call the main GMT_hold_contour_sub for each segment
	 */

	int seg, first, n, *split;
	double *xs, *ys, *xin, *yin;

	if ((split = GMT_split_line (xxx, yyy, &nn, G->line_type)) == NULL) {	/* Just one long line */
		GMT_hold_contour_sub (xxx, yyy, nn, zval, label, ctype, cangle, closed, G);
		return;
	}

	/* Here we had jumps and need to call the _sub function once for each segment */

	xin = *xxx;
	yin = *yyy;
	for (seg = 0, first = 0; seg <= split[0]; seg++) {	/* Number of segments are given by split[0] + 1 */
		n = split[seg+1] - first;
		xs = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), GMT_program);
		ys = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), GMT_program);
		memcpy ((void *)xs, (void *)&xin[first], (size_t)(n * sizeof (double)));
		memcpy ((void *)ys, (void *)&yin[first], (size_t)(n * sizeof (double)));
		GMT_hold_contour_sub (&xs, &ys, n, zval, label, ctype, cangle, closed, G);
		GMT_free ((void *)xs);
		GMT_free ((void *)ys);
		first = n;	/* First point in next segment */
	}
	GMT_free ((void *)split);
}

void GMT_hold_contour_sub (double **xxx, double **yyy, int nn, double zval, char *label, char ctype, double cangle, int closed, struct GMT_CONTOUR *G)
{	/* The xx, yy are expected to be projected x/y inches */
	int i, j, start = 0;
	size_t n_alloc = GMT_SMALL_CHUNK;
	double dx, dy, width, f, this_dist, step, stept, *track_dist, *map_dist, *value_dist, *radii;
	double this_value_dist, lon[2], lat[2], *xx, *yy;
	struct GMT_LABEL *new_label;
	char this_label[BUFSIZ];

	if (nn < 2) return;

	xx = *xxx;	yy = *yyy;
	G->n_label = 0;

	/* OK, line is long enough to be added to array of lines */

	if (ctype == 'A' || ctype == 'a') {	/* Annotated contours, must find label placement */

		/* Calculate distance along contour and store in track_dist array */

		if (G->dist_kind == 1) GMT_xy_to_geo (&lon[1], &lat[1], xx[0], yy[0]);
		map_dist = (double *) GMT_memory (VNULL, (size_t)nn, sizeof (double), GMT_program);	/* Distances on map in inches */
		track_dist = (double *) GMT_memory (VNULL, (size_t)nn, sizeof (double), GMT_program);	/* May be km ,degrees or whatever */
		value_dist = (double *) GMT_memory (VNULL, (size_t)nn, sizeof (double), GMT_program);	/* May be km ,degrees or whatever */
		radii = (double *) GMT_memory (VNULL, (size_t)nn, sizeof (double), GMT_program);	/* Radius of curvature, in inches */

		/* We will calculate the radii of curvature at all points.  By default we dont care and
		 * will place labels at whatever distance we end up with.  However, if the user has asked
		 * for a minimum limit on the radius of curvature [Default 0] we do not want to place labels
		 * at those sections where the curvature is large.  Since labels are placed according to
		 * distance along track, the way we deal with this is to set distance increments to zero
		 * where curvature is large:  that way, there is no increase in distance over those patches
		 * and the machinery for determining when we exceed the next label distance will not kick
		 * in until after curvature drops and increments are again nonzero.  This procedure only
		 * applyes to the algorithms based on distance along track.
		 */

		GMT_get_radii_of_curvature (xx, yy, nn, radii);

		map_dist[0] = track_dist[0] = value_dist[0] = 0.0;	/* Unnecessary, just so we understand the logic */
		for (i = 1; i < nn; i++) {
			/* Distance from xy */
			dx = xx[i] - xx[i-1];
			if (GMT_IS_MAPPING && GMT_world_map && fabs (dx) > (width = GMT_half_map_width (yy[i-1]))) {
				width *= 2.0;
				dx = copysign (width - fabs (dx), -dx);
				if (xx[i] < width)
					xx[i-1] -= width;
				else
					xx[i-1] += width;
			}
			dy = yy[i] - yy[i-1];
			step = stept = hypot (dx, dy);
			map_dist[i] = map_dist[i-1] + step;
			if (G->dist_kind == 1 || G->label_type == 4) {
				lon[0] = lon[1];	lat[0] = lat[1];
				GMT_xy_to_geo (&lon[1], &lat[1], xx[i], yy[i]);
				if (G->dist_kind == 1) step = G->d_scale * (G->dist_func) (lon[0], lat[0], lon[1], lat[1]);
				if (G->label_type == 4) stept = G->L_d_scale * (G->L_dist_func) (lon[0], lat[0], lon[1], lat[1]);
			}
			if (radii[i] < G->min_radius) step = stept = 0.0;	/* If curvature is too great we simply don't add up distances */
			track_dist[i] = track_dist[i-1] + step;
			value_dist[i] = value_dist[i-1] + stept;
		}

		/* G->L array is only used so we can later sort labels based on distance along track.  Once
		 * GMT_contlabel_draw has been called we will free up the memory as the labels are kept in
		 * the linked list starting at G->anchor. */

		G->L = (struct GMT_LABEL **) GMT_memory (VNULL, n_alloc, sizeof (struct GMT_LABEL *), GMT_program);

		if (G->spacing) {	/* Place labels based on distance along contours */
			double last_label_dist, dist_offset, dist;

			dist_offset = (closed && G->dist_kind == 0) ? (1.0 - G->label_dist_frac) * G->label_dist_spacing : 0.0;	/* Label closed contours longer than frac of dist_spacing */
			last_label_dist = 0.0;
			i = 1;
			while (i < nn) {

				dist = track_dist[i] + dist_offset - last_label_dist;
				if (dist > G->label_dist_spacing) {	/* Time for label */
					new_label = GMT_contlabel_new ();
					f = (dist - G->label_dist_spacing) / (track_dist[i] - track_dist[i-1]);
					if (f < 0.5) {
						new_label->x = xx[i] - f * (xx[i] - xx[i-1]);
						new_label->y = yy[i] - f * (yy[i] - yy[i-1]);
						new_label->dist = map_dist[i] - f * (map_dist[i] - map_dist[i-1]);
						this_value_dist = value_dist[i] - f * (value_dist[i] - value_dist[i-1]);
					}
					else {
						f = 1.0 - f;
						new_label->x = xx[i-1] + f * (xx[i] - xx[i-1]);
						new_label->y = yy[i-1] + f * (yy[i] - yy[i-1]);
						new_label->dist = map_dist[i-1] + f * (map_dist[i] - map_dist[i-1]);
						this_value_dist = value_dist[i-1] + f * (value_dist[i] - value_dist[i-1]);
					}
					this_dist = G->label_dist_spacing - dist_offset + last_label_dist;
					if (GMT_label_is_OK (new_label, this_label, label, this_dist, this_value_dist, -1, -1, G)) {
						GMT_place_label (new_label, this_label, G, !(G->label_type == 0 || G->label_type == 3));
						new_label->node = i - 1;
						GMT_contlabel_angle (xx, yy, i - 1, i, cangle, nn, new_label, G);
						G->L[G->n_label++] = new_label;
						if (G->n_label == (int)n_alloc) {
							n_alloc += GMT_SMALL_CHUNK;
							G->L = (struct GMT_LABEL **) GMT_memory ((void *)G->L, n_alloc, sizeof (struct GMT_LABEL *), GMT_program);
						}
					}
					dist_offset = 0.0;
					last_label_dist = this_dist;
				}
				else	/* Go to next point in line */
					i++;
			}
			if (G->n_label == 0 && gmtdefs.verbose) fprintf (stderr, "%s: Warning: Your -Gd|D option produced no contour labels for z = %g\n", GMT_program, zval);

		}
		if (G->number) {	/* Place prescribed number of labels evenly along contours */
			int nc, e_val = 0;
			double dist, last_dist;

			last_dist = (G->n_cont > 1) ? -map_dist[nn-1] / (G->n_cont - 1) : -0.5 * map_dist[nn-1];
			nc = (map_dist[nn-1] > G->min_dist) ? G->n_cont : 0;
			for (i = j = 0; i < nc; i++) {
				new_label = GMT_contlabel_new ();
				if (G->number_placement && !closed) {
					e_val = G->number_placement;
					if (G->number_placement == -1 && G->n_cont == 1) {	/* Label justified with start of segment */
						f = d_atan2 (xx[0] - xx[1], yy[0] - yy[1]) * R2D + 180.0;	/* 0-360 */
						G->end_just[0] = (f >= 90.0 && f <= 270) ? 7 : 5;
					}
					else if (G->number_placement == +1 && G->n_cont == 1) {	/* Label justified with end of segment */
						f = d_atan2 (xx[nn-1] - xx[nn-2], yy[nn-1] - yy[nn-2]) * R2D + 180.0;	/* 0-360 */
						G->end_just[1] = (f >= 90.0 && f <= 270) ? 7 : 5;
					}
					else if (G->number_placement && G->n_cont > 1)	/* One of the end labels */
						e_val = (i == 0) ? -1 : +1;
					dist = (G->n_cont > 1) ? i * track_dist[nn-1] / (G->n_cont - 1) : 0.5 * (G->number_placement + 1.0) * track_dist[nn-1];
					this_value_dist = (G->n_cont > 1) ? i * value_dist[nn-1] / (G->n_cont - 1) : 0.5 * (G->number_placement + 1.0) * value_dist[nn-1];
				}
				else {
					dist = (i + 1 - 0.5 * closed) * track_dist[nn-1] / (G->n_cont + 1 - closed);
					this_value_dist = (i + 1 - 0.5 * closed) * value_dist[nn-1] / (G->n_cont + 1 - closed);
				}
				while (j < nn && track_dist[j] < dist) j++;
				if (j == nn) j--;	/* Safety precaution */
				f = ((j == 0) ? 1.0 : (dist - track_dist[j-1]) / (track_dist[j] - track_dist[j-1]));
				if (f < 0.5) {	/* Pick the smallest fraction to minimize Taylor shortcomings */
					new_label->x = xx[j-1] + f * (xx[j] - xx[j-1]);
					new_label->y = yy[j-1] + f * (yy[j] - yy[j-1]);
					new_label->dist = map_dist[j-1] + f * (map_dist[j] - map_dist[j-1]);
				}
				else {
					f = 1.0 - f;
					new_label->x = (j == 0) ? xx[0] : xx[j] - f * (xx[j] - xx[j-1]);
					new_label->y = (j == 0) ? yy[0] : yy[j] - f * (yy[j] - yy[j-1]);
					new_label->dist = (j == 0) ? 0.0 : map_dist[j] - f * (map_dist[j] - map_dist[j-1]);
				}
				if ((new_label->dist - last_dist) >= G->min_dist) {	/* OK to accept this label */
					this_dist = dist;
					if (GMT_label_is_OK (new_label, this_label, label, this_dist, this_value_dist, -1, -1, G)) {
						GMT_place_label (new_label, this_label, G, !(G->label_type == 0));
						new_label->node = (j == 0) ? 0 : j - 1;
						GMT_contlabel_angle (xx, yy, new_label->node, j, cangle, nn, new_label, G);
						if (G->number_placement) new_label->end = e_val;
						G->L[G->n_label++] = new_label;
						if (G->n_label == (int)n_alloc) {
							n_alloc += GMT_SMALL_CHUNK;
							G->L = (struct GMT_LABEL **) GMT_memory ((void *)G->L, n_alloc, sizeof (struct GMT_LABEL *), GMT_program);
						}
					}
					last_dist = new_label->dist;
				}
				else	/* All in vain... */
					GMT_free ((void *)new_label);
			}
			if (G->n_label == 0 && gmtdefs.verbose) fprintf (stderr, "%s: Warning: Your -Gn|N option produced no contour labels for z = %g\n", GMT_program, zval);
		}
		if (G->crossing) {	/* Determine label positions based on crossing lines */
			int left, right, line_no;
			GMT_init_track (yy, nn, &(G->ylist));
			for (line_no = 0; line_no < G->xp->n_segments; line_no++) {	/* For each of the crossing lines */
				GMT_init_track (G->xp->segment[line_no]->coord[GMT_Y], G->xp->segment[line_no]->n_rows, &(G->ylist_XP));
				G->nx = GMT_crossover (G->xp->segment[line_no]->coord[GMT_X], G->xp->segment[line_no]->coord[GMT_Y], NULL, G->ylist_XP, G->xp->segment[line_no]->n_rows, xx, yy, NULL, G->ylist, nn, FALSE, &G->XC);
				GMT_free ((void *)G->ylist_XP);
				if (G->nx == 0) continue;

				/* OK, we found intersections for labels */

				for (i = 0; i < G->nx; i++) {
					left  = (int) floor (G->XC.xnode[1][i]);
					right = (int) ceil  (G->XC.xnode[1][i]);
					new_label = GMT_contlabel_new ();
					new_label->x = G->XC.x[i];
					new_label->y = G->XC.y[i];
					new_label->node = left;
					new_label->dist = track_dist[left];
					f = G->XC.xnode[1][i] - left;
					if (f < 0.5) {
						this_dist = track_dist[left] + f * (track_dist[right] - track_dist[left]);
						new_label->dist = map_dist[left] + f * (map_dist[right] - map_dist[left]);
						this_value_dist = value_dist[left] + f * (value_dist[right] - value_dist[left]);
					}
					else {
						f = 1.0 - f;
						this_dist = track_dist[right] - f * (track_dist[right] - track_dist[left]);
						new_label->dist = map_dist[right] - f * (map_dist[right] - map_dist[left]);
						this_value_dist = value_dist[right] - f * (value_dist[right] - value_dist[left]);
					}
					if (GMT_label_is_OK (new_label, this_label, label, this_dist, this_value_dist, line_no, -1, G)) {
						GMT_place_label (new_label, this_label, G, !(G->label_type == 0));
						GMT_contlabel_angle (xx, yy, left, right, cangle, nn, new_label, G);
						G->L[G->n_label++] = new_label;
						if (G->n_label == (int)n_alloc) {
							n_alloc += GMT_SMALL_CHUNK;
							G->L = (struct GMT_LABEL **) GMT_memory ((void *)G->L, n_alloc, sizeof (struct GMT_LABEL *), GMT_program);
						}
					}
				}
				GMT_x_free (&G->XC);
			}
			GMT_free ((void *)G->ylist);
			if (G->n_label == 0 && gmtdefs.verbose) fprintf (stderr, "%s: Warning: Your -Gx|X|l|L option produced no contour labels for z = %g\n", GMT_program, zval);
		}
		if (G->fixed) {	/* Prescribed point locations for labels that match points in input records */
			double dist, min_dist;
			for (j = 0; j < G->f_n; j++) {	/* Loop over fixed point list */
				min_dist = DBL_MAX;
				for (i = 0; i < nn; i++) {	/* Loop over input line/contour */
					if ((dist = hypot (xx[i] - G->f_xy[0][j], yy[i] - G->f_xy[1][j])) < min_dist) {	/* Better fit */
						min_dist = dist;
						start = i;
					}
				}
				if (min_dist < G->slop) {	/* Closest point within tolerance */
					new_label = GMT_contlabel_new ();
					new_label->x = xx[start];
					new_label->y = yy[start];
					new_label->node = start;
					new_label->dist = track_dist[start];
					this_dist = track_dist[start];
					new_label->dist = map_dist[start];
					this_value_dist = value_dist[start];
					if (GMT_label_is_OK (new_label, this_label, label, this_dist, this_value_dist, -1, j, G)) {
						GMT_place_label (new_label, this_label, G, !(G->label_type == 0));
						GMT_contlabel_angle (xx, yy, start, start, cangle, nn, new_label, G);
						G->L[G->n_label++] = new_label;
						if (G->n_label == (int)n_alloc) {
							n_alloc += GMT_SMALL_CHUNK;
							G->L = (struct GMT_LABEL **) GMT_memory ((void *)G->L, n_alloc, sizeof (struct GMT_LABEL *), GMT_program);
						}
					}
				}
			}

			if (G->n_label == 0 && gmtdefs.verbose) fprintf (stderr, "%s: Warning: Your -Gf option produced no contour labels for z = %g\n", GMT_program, zval);
		}
		GMT_contlabel_fixpath (&xx, &yy, map_dist, &nn, G);	/* Inserts the label x,y into path */
		GMT_contlabel_addpath (xx, yy, nn, zval, label, TRUE, G);		/* Appends this path and the labels to list */

		GMT_free ((void *)track_dist);
		GMT_free ((void *)map_dist);
		GMT_free ((void *)value_dist);
		GMT_free ((void *)G->L);
	}
	else {   /* just one line, no holes for labels */
		GMT_contlabel_addpath (xx, yy, nn, zval, label, FALSE, G);		/* Appends this path to list */
	}
	*xxx = xx;
	*yyy = yy;
}

void GMT_place_label (struct GMT_LABEL *L, char *txt, struct GMT_CONTOUR *G, BOOLEAN use_unit)
{	/* Allocates needed space and copies in the label */
	int n, m = 0;
	if (use_unit && G->unit && G->unit[0]) m = strlen (G->unit) + (G->unit[0] != '-');	/* Must allow extra space for a unit string */
	n = strlen (txt) + 1 + m;
	if (G->prefix && G->prefix[0]) {	/* Must prepend the prefix string */
		n += strlen (G->prefix) + 1;
		L->label = (char *) GMT_memory (VNULL, (size_t)n, sizeof (char), "gmt");
		if (G->prefix[0] == '-')	/* No space between annotation and prefix */
			sprintf (L->label, "%s%s", &G->prefix[1], txt);
		else
			sprintf (L->label, "%s %s", G->prefix, txt);
	}
	else {
		L->label = (char *) GMT_memory (VNULL, (size_t)n, sizeof (char), "gmt");
		strcpy (L->label, txt);
	}
	if (use_unit && G->unit && G->unit[0]) {	/* Append a unit string */
		if (G->unit[0] == '-')	/* No space between annotation and prefix */
			strcat (L->label, &G->unit[1]);
		else {
			strcat (L->label, " ");
			strcat (L->label, G->unit);
		}
	}
}

int GMT_label_is_OK (struct GMT_LABEL *L, char *this_label, char *label, double this_dist, double this_value_dist, int xl, int fj, struct GMT_CONTOUR *G)
{
	int label_OK = TRUE, i, k;
	char format[GMT_LONG_TEXT];
	struct GMT_CONTOUR_LINE *C = VNULL;

	if (G->isolate) {	/* Must determine if the proposed label is withing radius distance of any other label already accepted */
		for (i = 0; i < G->n_segments; i++) {	/* Previously processed labels */
			C = G->segment[i];	/* Pointer to current segment */
			for (k = 0; k < C->n_labels; k++) if (hypot (L->x - C->L[k].x, L->y - C->L[k].y) < G->label_isolation) return (FALSE);
		}
		/* Also check labels for current segment */
		for (k = 0; k < G->n_label; k++) if (hypot (L->x - G->L[k]->x, L->y - G->L[k]->y) < G->label_isolation) return (FALSE);
	}

	switch (G->label_type) {
		case 0:
			if (label && label[0])
				strcpy (this_label, label);
			else
				label_OK = FALSE;
			break;

		case 1:
		case 2:
			if (G->label && G->label[0])
				strcpy (this_label, G->label);
			else
				label_OK = FALSE;
			break;

		case 3:
			if (G->spacing) {	/* Distances are even so use special contour format */
				GMT_get_format (this_dist * GMT_u2u[GMT_INCH][G->dist_unit], G->unit, CNULL, format);
				sprintf (this_label, format, this_dist * GMT_u2u[GMT_INCH][G->dist_unit]);
			}
			else {
				sprintf (this_label, gmtdefs.d_format, this_dist * GMT_u2u[GMT_INCH][G->dist_unit]);
			}
			break;

		case 4:
			sprintf (this_label, gmtdefs.d_format, this_value_dist);
			break;

		case 5:
			if (G->f_label[fj] && G->f_label[fj][0])
				strcpy (this_label, G->f_label[fj]);
			else
				label_OK = FALSE;
			break;

		case 6:
			if (G->xp->segment[xl]->label && G->xp->segment[xl]->label[0])
				strcpy (this_label, G->xp->segment[xl]->label);
			else
				label_OK = FALSE;
			break;

		case 7:
			sprintf (this_label, "%d", (GMT_io.status & GMT_IO_SEGMENT_HEADER) ? GMT_io.seg_no - 1 : GMT_io.seg_no);
			break;

		case 8:
			sprintf (this_label, "%d/%d", GMT_io.file_no, (GMT_io.status & GMT_IO_SEGMENT_HEADER) ? GMT_io.seg_no - 1 : GMT_io.seg_no);
			break;

		default:	/* Should not happen... */
			fprintf (stderr, "%s: ERROR in GMT_label_is_OK. Notify gmt-team@hawaii.edu\n", GMT_program);
			GMT_exit (EXIT_FAILURE);
			break;
	}

	return (label_OK);
}

void GMT_get_plot_array (void) {      /* Allocate more space for plot arrays */

	GMT_n_alloc += GMT_CHUNK;
	GMT_x_plot = (double *) GMT_memory ((void *)GMT_x_plot, (size_t)GMT_n_alloc, sizeof (double), "gmt");
	GMT_y_plot = (double *) GMT_memory ((void *)GMT_y_plot, (size_t)GMT_n_alloc, sizeof (double), "gmt");
	GMT_pen = (int *) GMT_memory ((void *)GMT_pen, (size_t)GMT_n_alloc, sizeof (int), "gmt");
}

int GMT_comp_double_asc (const void *p_1, const void *p_2)
{
	/* Returns -1 if point_1 is < that point_2,
	   +1 if point_2 > point_1, and 0 if they are equal
	*/
	int bad_1, bad_2;
	double *point_1, *point_2;

	point_1 = (double *)p_1;
	point_2 = (double *)p_2;
	bad_1 = GMT_is_dnan ((*point_1));
	bad_2 = GMT_is_dnan ((*point_2));

	if (bad_1 && bad_2) return (0);
	if (bad_1) return (1);
	if (bad_2) return (-1);

	if ( (*point_1) < (*point_2) )
		return (-1);
	else if ( (*point_1) > (*point_2) )
		return (1);
	else
		return (0);
}

int GMT_comp_float_asc (const void *p_1, const void *p_2)
{
	/* Returns -1 if point_1 is < that point_2,
	   +1 if point_2 > point_1, and 0 if they are equal
	*/
	int bad_1, bad_2;
	float	*point_1, *point_2;

	point_1 = (float *)p_1;
	point_2 = (float *)p_2;
	bad_1 = GMT_is_fnan ((*point_1));
	bad_2 = GMT_is_fnan ((*point_2));

	if (bad_1 && bad_2) return (0);
	if (bad_1) return (1);
	if (bad_2) return (-1);

	if ( (*point_1) < (*point_2) )
		return (-1);
	else if ( (*point_1) > (*point_2) )
		return (1);
	else
		return (0);
}

int GMT_comp_int_asc (const void *p_1, const void *p_2)
{
	/* Returns -1 if point_1 is < that point_2,
	   +1 if point_2 > point_1, and 0 if they are equal
	*/
	int *point_1, *point_2;

	point_1 = (int *)p_1;
	point_2 = (int *)p_2;
	if ( (*point_1) < (*point_2) )
		return (-1);
	else if ( (*point_1) > (*point_2) )
		return (1);
	else
		return (0);
}

int GMT_get_format (double interval, char *unit, char *prefix, char *format)
{
	int i, j, ndec = 0;
	char text[BUFSIZ];

	if (strchr (gmtdefs.d_format, 'g')) {	/* General format requested */

		/* Find number of decimals needed in the format statement */

		sprintf (text, "%.12g", interval);
		for (i = 0; text[i] && text[i] != '.'; i++);
		if (text[i]) {	/* Found a decimal point */
			for (j = i + 1; text[j] && text[j] != 'e'; j++);
			ndec = j - i - 1;
			if (text[j] == 'e') {	/* Exponential notation, modify ndec */
				ndec -= atoi (&text[++j]);
				if (ndec < 0) ndec = 0;	/* since a positive exponent could give -ve answer */
			}
		}
	}

	if (unit && unit[0]) {	/* Must append the unit string */
		if (!strchr (unit, '%'))	/* No percent signs */
			strncpy (text, unit, 80);
		else {
			for (i = j = 0; i < (int)strlen (unit); i++) {
				text[j++] = unit[i];
				if (unit[i] == '%') text[j++] = unit[i];
			}
			text[j] = 0;
		}
		if (text[0] == '-') {	/* No space between annotation and unit */
			if (ndec > 0)
				sprintf (format, "%%.%df%s", ndec, &text[1]);
			else
				sprintf (format, "%s%s", gmtdefs.d_format, &text[1]);
		}
		else {			/* 1 space between annotation and unit */
			if (ndec > 0)
				sprintf (format, "%%.%df %s", ndec, text);
			else
				sprintf (format, "%s %s", gmtdefs.d_format, text);
		}
		if (ndec == 0) ndec = 1;	/* To avoid resetting format later */
	}
	else if (ndec > 0)
		sprintf (format, "%%.%df", ndec);
	else {	/* Pull ndec from given format if .<precision> is given */
		for (i = 0, j = -1; j == -1 && gmtdefs.d_format[i]; i++) if (gmtdefs.d_format[i] == '.') j = i;
		if (j > -1) ndec = atoi (&gmtdefs.d_format[j+1]);
		strcpy (format, gmtdefs.d_format);
	}
	if (prefix && prefix[0]) {	/* Must prepend the prefix string */
		if (prefix[0] == '-')	/* No space between annotation and unit */
			sprintf (text, "%s%s", &prefix[1], format);
		else
			sprintf (text, "%s %s", prefix, format);
		strcpy (format, text);
	}
	return (ndec);
}

int	GMT_non_zero_winding (double xp, double yp, double *x, double *y, int n_path)
{
	/* Routine returns (2) if (xp,yp) is inside the
	   polygon x[n_path], y[n_path], (0) if outside,
	   and (1) if exactly on the path edge.
	   Uses non-zero winding rule in Adobe PostScript
	   Language reference manual, section 4.6 on Painting.
	   Path should have been closed first, so that
	   x[n_path-1] = x[0], and y[n_path-1] = y[0].

	   This is version 2, trying to kill a bug
	   in above routine:  If point is on X edge,
	   fails to discover that it is on edge.

	   We are imagining a ray extending "up" from the
	   point (xp,yp); the ray has equation x = xp, y >= yp.
	   Starting with crossing_count = 0, we add 1 each time
	   the path crosses the ray in the +x direction, and
	   subtract 1 each time the ray crosses in the -x direction.
	   After traversing the entire polygon, if (crossing_count)
	   then point is inside.  We also watch for edge conditions.

	   If two or more points on the path have x[i] == xp, then
	   we have an ambiguous case, and we have to find the points
	   in the path before and after this section, and check them.
	   */

	int	i, j, k, jend, crossing_count, above;
	double	y_sect;

	if (n_path < 2) return (GMT_OUTSIDE_POLYGON);	/* Cannot be inside a null set or a point so default to outside */

	if (!(x[n_path-1] == x[0] && y[n_path-1] == y[0])) {
		fprintf (stderr, "%s: GMT_non_zero_winding given non-closed polygon\n", GMT_program);
		GMT_exit (EXIT_FAILURE);
	}

	above = FALSE;
	crossing_count = 0;

	/* First make sure first point in path is not a special case:  */
	j = jend = n_path - 1;
	if (x[j] == xp) {
		/* Trouble already.  We might get lucky:  */
		if (y[j] == yp) return (GMT_ONSIDE_POLYGON);

		/* Go backward down the polygon until x[i] != xp:  */
		if (y[j] > yp) above = TRUE;
		i = j - 1;
		while (x[i] == xp && i > 0) {
			if (y[i] == yp) return (GMT_ONSIDE_POLYGON);
			if (!(above) && y[i] > yp) above = TRUE;
			i--;
		}

		/* Now if i == 0 polygon is degenerate line x=xp;
		   since we know xp,yp is inside bounding box,
		   it must be on edge:  */
		if (i == 0) return (GMT_ONSIDE_POLYGON);

		/* Now we want to mark this as the end, for later:  */
		jend = i;

		/* Now if (j-i)>1 there are some segments the point could be exactly on:  */
		for (k = i+1; k < j; k++) {
			if ( (y[k] <= yp && y[k+1] >= yp) || (y[k] >= yp && y[k+1] <= yp) ) return (GMT_ONSIDE_POLYGON);
		}


		/* Now we have arrived where i is > 0 and < n_path-1, and x[i] != xp.
			We have been using j = n_path-1.  Now we need to move j forward
			from the origin:  */
		j = 1;
		while (x[j] == xp) {
			if (y[j] == yp) return (GMT_ONSIDE_POLYGON);
			if (!(above) && y[j] > yp) above = TRUE;
			j++;
		}

		/* Now at the worst, j == jstop, and we have a polygon with only 1 vertex
			not at x = xp.  But now it doesn't matter, that would end us at
			the main while below.  Again, if j>=2 there are some segments to check:  */
		for (k = 0; k < j-1; k++) {
			if ( (y[k] <= yp && y[k+1] >= yp) || (y[k] >= yp && y[k+1] <= yp) ) return (GMT_ONSIDE_POLYGON);
		}


		/* Finally, we have found an i and j with points != xp.  If (above) we may have crossed the ray:  */
		if (above && x[i] < xp && x[j] > xp)
			crossing_count++;
		else if (above && x[i] > xp && x[j] < xp)
			crossing_count--;

		/* End nightmare scenario for x[0] == xp.  */
	}

	else {
		/* Get here when x[0] != xp:  */
		i = 0;
		j = 1;
		while (x[j] == xp && j < jend) {
			if (y[j] == yp) return (GMT_ONSIDE_POLYGON);
			if (!(above) && y[j] > yp) above = TRUE;
			j++;
		}
		/* Again, if j==jend, (i.e., 0) then we have a polygon with only 1 vertex
			not on xp and we will branch out below.  */

		/* if ((j-i)>2) the point could be on intermediate segments:  */
		for (k = i+1; k < j-1; k++) {
			if ( (y[k] <= yp && y[k+1] >= yp) || (y[k] >= yp && y[k+1] <= yp) ) return (GMT_ONSIDE_POLYGON);
		}

		/* Now we have x[i] != xp and x[j] != xp.
			If (above) and x[i] and x[j] on opposite sides, we are certain to have crossed the ray.
			If not (above) and (j-i)>1, then we have not crossed it.
			If not (above) and j-i == 1, then we have to check the intersection point.  */

		if (x[i] < xp && x[j] > xp) {
			if (above)
				crossing_count++;
			else if ( (j-i) == 1) {
				y_sect = y[i] + (y[j] - y[i]) * ( (xp - x[i]) / (x[j] - x[i]) );
				if (y_sect == yp) return (GMT_ONSIDE_POLYGON);
				if (y_sect > yp) crossing_count++;
			}
		}
		if (x[i] > xp && x[j] < xp) {
			if (above)
				crossing_count--;
			else if ( (j-i) == 1) {
				y_sect = y[i] + (y[j] - y[i]) * ( (xp - x[i]) / (x[j] - x[i]) );
				if (y_sect == yp) return (GMT_ONSIDE_POLYGON);
				if (y_sect > yp) crossing_count--;
			}
		}

		/* End easier case for x[0] != xp  */
	}

	/* Now MAIN WHILE LOOP begins:
		Set i = j, and search for a new j, and do as before.  */

	i = j;
	while (i < jend) {
		above = FALSE;
		j = i+1;
		while (x[j] == xp) {
			if (y[j] == yp) return (GMT_ONSIDE_POLYGON);
			if (!(above) && y[j] > yp) above = TRUE;
			j++;
		}
		/* if ((j-i)>2) the point could be on intermediate segments:  */
		for (k = i+1; k < j-1; k++) {
			if ( (y[k] <= yp && y[k+1] >= yp) || (y[k] >= yp && y[k+1] <= yp) ) return (GMT_ONSIDE_POLYGON);
		}

		/* Now we have x[i] != xp and x[j] != xp.
			If (above) and x[i] and x[j] on opposite sides, we are certain to have crossed the ray.
			If not (above) and (j-i)>1, then we have not crossed it.
			If not (above) and j-i == 1, then we have to check the intersection point.  */

		if (x[i] < xp && x[j] > xp) {
			if (above)
				crossing_count++;
			else if ( (j-i) == 1) {
				y_sect = y[i] + (y[j] - y[i]) * ( (xp - x[i]) / (x[j] - x[i]) );
				if (y_sect == yp) return (GMT_ONSIDE_POLYGON);
				if (y_sect > yp) crossing_count++;
			}
		}
		if (x[i] > xp && x[j] < xp) {
			if (above)
				crossing_count--;
			else if ( (j-i) == 1) {
				y_sect = y[i] + (y[j] - y[i]) * ( (xp - x[i]) / (x[j] - x[i]) );
				if (y_sect == yp) return (GMT_ONSIDE_POLYGON);
				if (y_sect > yp) crossing_count--;
			}
		}

		/* That's it for this piece.  Advance i:  */

		i = j;
	}

	/* End of MAIN WHILE.  Get here when we have gone all around without landing on edge.  */

	if (crossing_count)
		return (GMT_INSIDE_POLYGON);
	else
		return (GMT_OUTSIDE_POLYGON);
}

int GMT_inonout_sphpol (double plon, double plat, const struct GMT_LINE_SEGMENT *P)
/* This function is used to see if some point P is located inside, outside, or on the boundary of the
 * spherical polygon S read by GMT_import_table.
 * Returns the following values:
 *	0:	P is outside of S
 *	1:	P is inside of S
 *	2:	P is on boundary of S
 */
{
	/* Algorithm:
	 * Case 1: The polygon S contains a geographical pole
	 *	   a) if P is beyond the far latitude then P is outside
	 *	   b) Draw meridian through P and count intersections:
	 *		odd: P is outside; even: P is inside
	 * Case 2: S does not contain a pole
	 *	   a) If P is outside range of latitudes then P is outside
	 *	   c) Draw meridian through P and count intersections:
	 *		odd: P is inside; even: P is outside
	 * In all cases, we check if P is on the outline of S
	 */

	int count[2];

	if (P->pole) {	/* Case 1 of an enclosed polar cap */
		if (P->pole == +1) {	/* N polar cap */
			if (plat < P->min[GMT_Y]) return (GMT_OUTSIDE_POLYGON);	/* South of a N polar cap */
			if (plat > P->max[GMT_Y]) return (GMT_INSIDE_POLYGON);	/* Clearly inside of a N polar cap */
		}
		if (P->pole == -1) {	/* S polar cap */
			if (plat > P->max[GMT_Y]) return (GMT_OUTSIDE_POLYGON);	/* North of a S polar cap */
			if (plat < P->min[GMT_Y]) return (GMT_INSIDE_POLYGON);	/* North of a S polar cap */
		}

		/* Tally up number of intersections between polygon and meridian through P */

		if (GMT_inonout_sphpol_count (plon, plat, P, count)) return (GMT_ONSIDE_POLYGON);	/* Found P is on S */

		if (P->pole == +1 && count[0] % 2 == 0) return (GMT_INSIDE_POLYGON);
		if (P->pole == -1 && count[1] % 2 == 0) return (GMT_INSIDE_POLYGON);

		return (GMT_OUTSIDE_POLYGON);
	}

	/* Here is Case 2.  First check latitude range */

	if (plat < P->min[GMT_Y] || plat > P->max[GMT_Y]) return (GMT_OUTSIDE_POLYGON);

	/* Longitudes are tricker and are tested with the tallying of intersections */

	if (GMT_inonout_sphpol_count (plon, plat, P, count)) return (GMT_ONSIDE_POLYGON);	/* Found P is on S */

	if (count[0] % 2) return (GMT_INSIDE_POLYGON);

	return (GMT_OUTSIDE_POLYGON);	/* Nothing triggered the tests; we are outside */
}

int GMT_inonout_sphpol_count (double plon, double plat, const struct GMT_LINE_SEGMENT *P, int count[])
{	/* Case of a polar cap */
	int i, in, cut, n_node_hit = 0;
	double W, E, S, N, lon, lon1, lon2, dlon, x_lat, this_lon, last_lon = 0.0;

	/* Draw meridian through P and count all the crossings with S */

	for (i = count[0] = count[1] = 0; i < P->n_rows - 1; i++) {	/* -1, since we now last point repeats the first */
		in = i + 1;		/* Next point index */
		lon1 = P->coord[GMT_X][i];	/* Copy the two longitudes since we need to mess with them */
		lon2 = P->coord[GMT_X][in];
		dlon = lon2 - lon1;
		if (dlon > 180.0)		/* Jumped across Greenwhich going westward */
			lon2 -= 360.0;
		else if (dlon < -180.0)		/* Jumped across Greenwhich going eastward */
			lon1 -= 360.0;
		if (lon1 <= lon2) {	/* Segment goes W to E (or N-S) */
			W = lon1;
			E = lon2;
		}
		else {			/* Segment goes E to W */
			W = lon2;
			E = lon1;
		}
		lon = plon;	/* Local copy of plon, below adjusted given the segment lon range */
		while (lon > W) lon -= 360.0;	/* Make sure we rewind way west for starters */
		while (lon < W) lon += 360.0;	/* Then make sure we wind to inside the lon range or way east */
		if (lon > E) continue;	/* Not crossing this segment */
		if (dlon == 0.0) {	/* Special case of N-S segment: does P lie on it? */
			if (P->coord[GMT_Y][in] < P->coord[GMT_Y][i]) {	/* Get N and S limits for segment */
				S = P->coord[GMT_Y][in];
				N = P->coord[GMT_Y][i];
			}
			else {
				N = P->coord[GMT_Y][in];
				S = P->coord[GMT_Y][i];
			}
			if (plat < S || plat > N) continue;	/* P is not on this segment */
			return (1);	/* P is on segment boundary; we are done*/
		}
		/* Calculate latitude at intersection */
		x_lat = P->coord[GMT_Y][i] + ((P->coord[GMT_Y][in] - P->coord[GMT_Y][i]) / (lon2 - lon1)) * (lon - lon1);
		if (GMT_IS_ZERO (x_lat - plat)) return (1);	/* P is on S boundary */

		if (lon == lon1 || lon == lon2) {	/* Special checking for cutting through a node */
			double dlon_a, dlon_b;
			n_node_hit++;
			if (n_node_hit == 2) {	/* Now processed two neighboring segments whose connecting node has lon = our lon */
				this_lon = (lon == lon1) ? lon2 : lon1;
				dlon_a = last_lon - lon;
				if (fabs (dlon_a) > 180.0) dlon_a = copysign ((360.0 - fabs (dlon_a)), -dlon_a);
				dlon_b = this_lon - lon;
				if (fabs (dlon_b) > 180.0) dlon_b = copysign ((360.0 - fabs (dlon_b)), -dlon_b);
				if ((dlon_a * dlon_b) < 0.0) {
					cut = (x_lat > plat) ? 0 : 1;	/* Cut is north (0) or south (1) of P */
					count[cut]++;
				}
				n_node_hit = 0;
			}
			else {	/* Keep track of the longitude of the point away from the node */
				last_lon = (lon == lon1) ? lon2 : lon1;
			}
		}
		else {
			cut = (x_lat > plat) ? 0 : 1;	/* Cut is north (0) or south (1) of P */
			count[cut]++;
		}
	}
	if (n_node_hit == 1) {	/* Fist and last segment connects through a node that equals our lon */
		fprintf (stderr, "%s: GMT_inonout_sphpol ends with n_node_hit == 1 which should not happen?\n", GMT_program);
	}

	return (0);	/* This means no special cases were detected that warranted an immediate return */
}

/* GMT can either compile with its standard Delaunay triangulation routine
 * based on the work by Dave Watson, OR you may link with the triangle.o
 * module from Jonathan Shewchuk, Berkeley U.  By default, the former is
 * chosen unless the compiler directive -DTRIANGLE_D is passed.  The latter
 * is much faster and will hopefully become the standard once we sort out
 * copyright issues etc.
 */

#ifdef TRIANGLE_D

/*
 * New GMT_delaunay interface routine that calls the triangulate function
 * developed by Jonathan Richard Shewchuk, Berkeley University.
 * Suggested by alert GMT user Alain Coat.  You need to get triangle.c and
 * triangle.h from www.cs.cmu.edu/
 */

#define REAL double

#include "triangle.h"

int GMT_delaunay (double *x_in, double *y_in, int n, int **link)
{
	/* GMT interface to the triangle package; see above for references.
	 * All that is done is reformatting of parameters and calling the
	 * main triangulate routine.  Thanx to Alain Coat for the tip.
	 */

	int i, j;
	struct triangulateio In, Out, vorOut;

	/* Set everything to 0 and NULL */

	memset ((void *)&In,	 0, sizeof (struct triangulateio));
	memset ((void *)&Out,	 0, sizeof (struct triangulateio));
	memset ((void *)&vorOut, 0, sizeof (struct triangulateio));

	/* Allocate memory for input points */

	In.numberofpoints = n;
	In.pointlist = (double *) GMT_memory ((void *)NULL, (size_t)(2 * n), sizeof (double), "GMT_delaunay");

	/* Copy x,y points to In structure array */

	for (i = j = 0; i < n; i++) {
		In.pointlist[j++] = x_in[i];
		In.pointlist[j++] = y_in[i];
	}

	/* Call Jonathan Shewchuk's triangulate algorithm.  This is 64-bit safe since
	 * all the structures use 4-byte ints (longs are used internally). */

	triangulate ("zIQB", &In, &Out, &vorOut);

	*link = Out.trianglelist;	/* List of node numbers to return via link */

	if (Out.pointlist) GMT_free ((void *)Out.pointlist);

	return (Out.numberoftriangles);
}

#else

/*
 * GMT_delaunay performs a Delaunay triangulation on the input data
 * and returns a list of indices of the points for each triangle
 * found.  Algorithm translated from
 * Watson, D. F., ACORD: Automatic contouring of raw data,
 *   Computers & Geosciences, 8, 97-101, 1982.
 */

int GMT_delaunay (double *x_in, double *y_in, int n, int **link)
	/* Input point x coordinates */
	/* Input point y coordinates */
	/* Number of input points */
	/* pointer to List of point ids per triangle.  Vertices for triangle no i
	   is in link[i*3], link[i*3+1], link[i*3+2] */
{
	int ix[3], iy[3];
	int i, j, ij, nuc, jt, km, id, isp, l1, l2, k, k1, jz, i2, kmt, kt, done, size;
	int *index, *istack, *x_tmp, *y_tmp;
	double det[2][3], *x_circum, *y_circum, *r2_circum, *x, *y;
	double xmin, xmax, ymin, ymax, datax, dx, dy, dsq, dd;

	size = 10 * n + 1;
	n += 3;

	index = (int *) GMT_memory (VNULL, (size_t)(3 * size), sizeof (int), "GMT_delaunay");
	istack = (int *) GMT_memory (VNULL, (size_t)size, sizeof (int), "GMT_delaunay");
	x_tmp = (int *) GMT_memory (VNULL, (size_t)size, sizeof (int), "GMT_delaunay");
	y_tmp = (int *) GMT_memory (VNULL, (size_t)size, sizeof (int), "GMT_delaunay");
	x_circum = (double *) GMT_memory (VNULL, (size_t)size, sizeof (double), "GMT_delaunay");
	y_circum = (double *) GMT_memory (VNULL, (size_t)size, sizeof (double), "GMT_delaunay");
	r2_circum = (double *) GMT_memory (VNULL, (size_t)size, sizeof (double), "GMT_delaunay");
	x = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "GMT_delaunay");
	y = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "GMT_delaunay");

	x[0] = x[1] = -1.0;	x[2] = 5.0;
	y[0] = y[2] = -1.0;	y[1] = 5.0;
	x_circum[0] = y_circum[0] = 2.0;	r2_circum[0] = 18.0;

	ix[0] = ix[1] = 0;	ix[2] = 1;
	iy[0] = 1;	iy[1] = iy[2] = 2;

	for (i = 0; i < 3; i++) index[i] = i;
	for (i = 0; i < size; i++) istack[i] = i;

	xmin = ymin = 1.0e100;	xmax = ymax = -1.0e100;

	for (i = 3, j = 0; i < n; i++, j++) {	/* Copy data and get extremas */
		x[i] = x_in[j];
		y[i] = y_in[j];
		if (x[i] > xmax) xmax = x[i];
		if (x[i] < xmin) xmin = x[i];
		if (y[i] > ymax) ymax = y[i];
		if (y[i] < ymin) ymin = y[i];
	}

	/* Normalize data */

	datax = 1.0 / MAX (xmax - xmin, ymax - ymin);

	for (i = 3; i < n; i++) {
		x[i] = (x[i] - xmin) * datax;
		y[i] = (y[i] - ymin) * datax;
	}

	isp = id = 1;
	for (nuc = 3; nuc < n; nuc++) {

		km = 0;

		for (jt = 0; jt < isp; jt++) {	/* loop through established 3-tuples */

			ij = 3 * jt;

			/* Test if new data is within the jt circumcircle */

			dx = x[nuc] - x_circum[jt];
			if ((dsq = r2_circum[jt] - dx * dx) < 0.0) continue;
			dy = y[nuc] - y_circum[jt];
			if ((dsq -= dy * dy) < 0.0) continue;

			/* Delete this 3-tuple but save its edges */

			id--;
			istack[id] = jt;

			/* Add edges to x/y_tmp but delete if already listed */

			for (i = 0; i < 3; i++) {
				l1 = ix[i];
				l2 = iy[i];
				if (km > 0) {
					kmt = km;
					for (j = 0, done = FALSE; !done && j < kmt; j++) {
						if (index[ij+l1] != x_tmp[j]) continue;
						if (index[ij+l2] != y_tmp[j]) continue;
						km--;
						if (j >= km) {
							done = TRUE;
							continue;
						}
						for (k = j; k < km; k++) {
							k1 = k + 1;
							x_tmp[k] = x_tmp[k1];
							y_tmp[k] = y_tmp[k1];
							done = TRUE;
						}
					}
				}
				else
					done = FALSE;
				if (!done) {
					x_tmp[km] = index[ij+l1];
					y_tmp[km] = index[ij+l2];
					km++;
				}
			}
		}

		/* Form new 3-tuples */

		for (i = 0; i < km; i++) {
			kt = istack[id];
			ij = 3 * kt;
			id++;

			/* Calculate the circumcircle center and radius
			 * squared of points ktetr[i,*] and place in tetr[kt,*] */

			for (jz = 0; jz < 2; jz++) {
				i2 = (jz == 0) ? x_tmp[i] : y_tmp[i];
				det[jz][0] = x[i2] - x[nuc];
				det[jz][1] = y[i2] - y[nuc];
				det[jz][2] = 0.5 * (det[jz][0] * (x[i2] + x[nuc]) + det[jz][1] * (y[i2] + y[nuc]));
			}
			dd = 1.0 / (det[0][0] * det[1][1] - det[0][1] * det[1][0]);
			x_circum[kt] = (det[0][2] * det[1][1] - det[1][2] * det[0][1]) * dd;
			y_circum[kt] = (det[0][0] * det[1][2] - det[1][0] * det[0][2]) * dd;
			dx = x[nuc] - x_circum[kt];
			dy = y[nuc] - y_circum[kt];
			r2_circum[kt] = dx * dx + dy * dy;
			index[ij++] = x_tmp[i];
			index[ij++] = y_tmp[i];
			index[ij] = nuc;
		}
		isp += 2;
	}

	for (jt = i = 0; jt < isp; jt++) {
		ij = 3 * jt;
		if (index[ij] < 3 || r2_circum[jt] > 1.0) continue;
		index[i++] = index[ij++] - 3;
		index[i++] = index[ij++] - 3;
		index[i++] = index[ij++] - 3;
	}

	index = (int *) GMT_memory ((void *)index, (size_t)i, sizeof (int), "GMT_delaunay");
	*link = index;

	GMT_free ((void *)istack);
	GMT_free ((void *)x_tmp);
	GMT_free ((void *)y_tmp);
	GMT_free ((void *)x_circum);
	GMT_free ((void *)y_circum);
	GMT_free ((void *)r2_circum);
	GMT_free ((void *)x);
	GMT_free ((void *)y);

	return (i/3);
}

#endif

/*
 * This section holds functions used for setting boundary  conditions in
 * processing grd file data.
 *
 * This is a new feature of GMT v3.1.   My first draft of this (April 1998)
 * set only one row of padding.  Additional thought showed that the bilinear
 * invocation of bcr routines would not work properly on periodic end conditions
 * in this case.  So second draft (May 5-6, 1998) will set two rows of padding,
 * and I will also have to edit bcr so that it works for this case.  The GMT
 * bcr routines are currently used in grdsample, grdtrack, and grdview.
 *
 * I anticipate that later (GMT v4 ?) this code could (?) be modified to also
 * handle the boundary conditions needed by surface.
 *
 * The default boundary condition is derived from application of Green's
 * theorem to the conditions for minimizing curvature:
 * laplacian (f) = 0 on edges, and d[laplacian(f)]/dn = 0 on edges, where
 * n is normal to an edge.  We also set d2f/dxdy = 0 at corners.
 *
 * The new routines here allow the user to choose other specifications:
 *
 * Either
 *	one or both of
 *	data are periodic in (x_max - x_min)
 *	data are periodic in (y_max - y_min)
 *
 * Or
 *	data are a geographical grid.
 *
 * Periodicities assume that the min,max are compatible with the inc;
 * that is, (x_max - x_min)modulo(x_inc) ~= 0.0 within precision tolerances,
 * and similarly for y.  It is assumed that this is OK and that gmt_grd_RI_verify
 * was called during read grd and found to be OK.
 *
 * In the geographical case, if x_max - x_min < 360 we will use the default
 * boundary conditions, but if x_max - x_min >= 360 the 360 periodicity in x
 * will be used to set the x boundaries, and so we require 360modulo(x_inc)
 * == 0.0 within precision tolerance.  If y_max != 90 the north edge will
 * default, and similarly for y_min != -90.  If a y edge is at a pole and
 * x_max - x_min >= 360 then the geographical y uses a 180 degree phase
 * shift in the values, so we require 180modulo(x_inc) == 0.
 * Note that a grid-registered file will require that the entire row of
 * values representing a pole must all be equal, else d/dx != 0 which
 * is wrong.  So compatibility error-checking is built in.
 *
 * Note that a periodicity or polar boundary eliminates the need for
 * d2/dxdy = 0 at a corner.  There are no "corners" in those cases.
 *
 * Author:	W H F Smith
 * Date:	17 April 1998
 * Revised:	5  May 1998
 *
 */

void GMT_boundcond_init (struct GMT_EDGEINFO *edgeinfo)
{
	edgeinfo->nxp = 0;
	edgeinfo->nyp = 0;
	edgeinfo->gn = FALSE;
	edgeinfo->gs = FALSE;
	return;
}


int GMT_boundcond_parse (struct GMT_EDGEINFO *edgeinfo, char *edgestring)
{
	/* Parse string beginning at argv[i][2] and load user's
		requests in edgeinfo->  Return success or failure.
		Requires that edgeinfo previously initialized to
		zero/FALSE stuff.  Expects g or (x and or y) is
		all that is in string.  */

	int	i, ier;

	i = 0;
	ier = FALSE;
	while (!ier && edgestring[i]) {
		switch (edgestring[i]) {
			case 'g':
			case 'G':
				edgeinfo->gn = TRUE;
				edgeinfo->gs = TRUE;
				break;
			case 'x':
			case 'X':
				edgeinfo->nxp = -1;
				break;
			case 'y':
			case 'Y':
				edgeinfo->nyp = -1;
				break;
			default:
				ier = TRUE;
				break;

		}
		i++;
	}

	if (ier) return (-1);

	if (edgeinfo->gn && (edgeinfo->nxp == -1 || edgeinfo->nxp == -1) ) {
		(void) fprintf (stderr, "WARNING:  GMT boundary condition g overrides x or y\n");
	}

	return (GMT_NOERROR);
}

int GMT_boundcond_param_prep (struct GRD_HEADER *h, struct GMT_EDGEINFO *edgeinfo)
{
	/* Called when edgeinfo holds user's choices.  Sets edgeinfo according to choices and h.  */

	double	xtest;

	if (edgeinfo->gn || GMT_grd_is_global(h)) {
		/* User has requested geographical conditions.  */
		if ( (h->x_max - h->x_min) < (360.0 - GMT_SMALL * h->x_inc) ) {
			fprintf (stderr, "GMT Warning:  x range too small; g boundary condition ignored.\n");
			edgeinfo->nxp = edgeinfo->nyp = 0;
			edgeinfo->gn  = edgeinfo->gs = FALSE;
			return (GMT_NOERROR);
		}
		xtest = fmod (180.0, h->x_inc) / h->x_inc;
		/* xtest should be within GMT_SMALL of zero or of one.  */
		if ( xtest > GMT_SMALL && xtest < (1.0 - GMT_SMALL) ) {
			/* Error.  We need it to divide into 180 so we can phase-shift at poles.  */
			fprintf (stderr, "GMT Warning:  x_inc does not divide 180; g boundary condition ignored.\n");
			edgeinfo->nxp = edgeinfo->nyp = 0;
			edgeinfo->gn  = edgeinfo->gs = FALSE;
			return (GMT_NOERROR);
		}
		edgeinfo->nxp = irint(360.0/h->x_inc);
		edgeinfo->nyp = 0;
		edgeinfo->gn = ( (fabs(h->y_max - 90.0) ) < (GMT_SMALL * h->y_inc) );
		edgeinfo->gs = ( (fabs(h->y_min + 90.0) ) < (GMT_SMALL * h->y_inc) );
	}
	else {
		if (edgeinfo->nxp != 0) edgeinfo->nxp = (h->node_offset) ? h->nx : h->nx - 1;
		if (edgeinfo->nyp != 0) edgeinfo->nyp = (h->node_offset) ? h->ny : h->ny - 1;
	}
	if (gmtdefs.verbose) fprintf (stderr, "GMT_boundcond_param_prep determined edgeinfo: gn = %i, gs = %i, nxp = %i, nyp = %i\n", edgeinfo->gn, edgeinfo->gs, edgeinfo->nxp, edgeinfo->nyp);
	return (GMT_NOERROR);
}


int GMT_boundcond_set (struct GRD_HEADER *h, struct GMT_EDGEINFO *edgeinfo, int *pad, float *a)
{
	/* Set two rows of padding (pad[] can be larger) around data according
		to desired boundary condition info in edgeinfo.
		Returns -1 on problem, 0 on success.
		If either x or y is periodic, the padding is entirely set.
		However, if neither is true (this rules out geographical also)
		then all but three corner-most points in each corner are set.

		As written, not ready to use with "surface" for GMT v4, because
		assumes left/right is +/- 1 and down/up is +/- mx.  In "surface"
		the amount to move depends on the current mesh size, a parameter
		not used here.

		This is the revised, two-rows version (WHFS 6 May 1998).
	*/

	int	bok;	/* Counter used to test that things are OK  */
	int	mx;	/* Width of padded array; width as malloc'ed  */
	int	mxnyp;	/* distance to periodic constraint in j direction  */
	int	i, jmx;	/* Current i, j * mx  */
	int	nxp2;	/* 1/2 the xg period (180 degrees) in cells  */
	int	i180;	/* index to 180 degree phase shift  */
	int	iw, iwo1, iwo2, iwi1, ie, ieo1, ieo2, iei1;  /* see below  */
	int	jn, jno1, jno2, jni1, js, jso1, jso2, jsi1;  /* see below  */
	int	jno1k, jno2k, jso1k, jso2k, iwo1k, iwo2k, ieo1k, ieo2k;
	int	j1p, j2p;	/* j_o1 and j_o2 pole constraint rows  */




	/* Check pad  */
	bok = 0;
	for (i = 0; i < 4; i++) {
		if (pad[i] < 2) bok++;
	}
	if (bok > 0) {
		fprintf (stderr, "GMT BUG:  bad pad for GMT_boundcond_set.\n");
		return (-1);
	}

	/* Check minimum size:  */
	if (h->nx < 2 || h->ny < 2) {
		fprintf (stderr, "GMT ERROR:  GMT_boundcond_set requires nx,ny at least 2.\n");
		return (-1);
	}

	/* Initialize stuff:  */

	mx = h->nx + pad[0] + pad[1];
	nxp2 = edgeinfo->nxp / 2;	/* Used for 180 phase shift at poles  */

	iw = pad[0];		/* i for west-most data column */
	iwo1 = iw - 1;		/* 1st column outside west  */
	iwo2 = iwo1 - 1;	/* 2nd column outside west  */
	iwi1 = iw + 1;		/* 1st column  inside west  */

	ie = pad[0] + h->nx - 1;	/* i for east-most data column */
	ieo1 = ie + 1;		/* 1st column outside east  */
	ieo2 = ieo1 + 1;	/* 2nd column outside east  */
	iei1 = ie - 1;		/* 1st column  inside east  */

	jn = mx * pad[3];	/* j*mx for north-most data row  */
	jno1 = jn - mx;		/* 1st row outside north  */
	jno2 = jno1 - mx;	/* 2nd row outside north  */
	jni1 = jn + mx;		/* 1st row  inside north  */

	js = mx * (pad[3] + h->ny - 1);	/* j*mx for south-most data row  */
	jso1 = js + mx;		/* 1st row outside south  */
	jso2 = jso1 + mx;	/* 2nd row outside south  */
	jsi1 = js - mx;		/* 1st row  inside south  */

	mxnyp = mx * edgeinfo->nyp;

	jno1k = jno1 + mxnyp;	/* data rows periodic to boundary rows  */
	jno2k = jno2 + mxnyp;
	jso1k = jso1 - mxnyp;
	jso2k = jso2 - mxnyp;

	iwo1k = iwo1 + edgeinfo->nxp;	/* data cols periodic to bndry cols  */
	iwo2k = iwo2 + edgeinfo->nxp;
	ieo1k = ieo1 - edgeinfo->nxp;
	ieo2k = ieo2 - edgeinfo->nxp;

	/* Check poles for grid case.  It would be nice to have done this
		in GMT_boundcond_param_prep() but at that point the data
		array isn't passed into that routine, and may not have been
		read yet.  Also, as coded here, this bombs with error if
		the pole data is wrong.  But there could be an option to
		to change the condition to Natural in that case, with warning.  */

	if (h->node_offset == 0) {
		if (edgeinfo->gn) {
			bok = 0;
			if (GMT_is_fnan (a[jn + iw])) {
				for (i = iw+1; i <= ie; i++) {
					if (!GMT_is_fnan (a[jn + i])) bok++;
				}
			}
			else {
				for (i = iw+1; i <= ie; i++) {
					if (a[jn + i] != a[jn + iw]) bok++;
				}
			}
			if (bok > 0) fprintf (stderr, "GMT Warning: Inconsistent grid values at North pole.\n");
		}

		if (edgeinfo->gs) {
			bok = 0;
			if (GMT_is_fnan (a[js + iw])) {
				for (i = iw+1; i <= ie; i++) {
					if (!GMT_is_fnan (a[js + i])) bok++;
				}
			}
			else {
				for (i = iw+1; i <= ie; i++) {
					if (a[js + i] != a[js + iw]) bok++;
				}
			}
			if (bok > 0) fprintf (stderr, "GMT Warning: Inconsistent grid values at South pole.\n");
		}
	}

	/* Start with the case that x is not periodic, because in that
		case we also know that y cannot be polar.  */

	if (edgeinfo->nxp <= 0) {

		/* x is not periodic  */

		if (edgeinfo->nyp > 0) {

			/* y is periodic  */

			for (i = iw; i <= ie; i++) {
				a[jno1 + i] = a[jno1k + i];
				a[jno2 + i] = a[jno2k + i];
				a[jso1 + i] = a[jso1k + i];
				a[jso2 + i] = a[jso2k + i];
			}

			/* periodic Y rows copied.  Now do X naturals.
				This is easy since y's are done; no corner problems.
				Begin with Laplacian = 0, and include 1st outside rows
				in loop, since y's already loaded to 2nd outside.  */

			for (jmx = jno1; jmx <= jso1; jmx += mx) {
				a[jmx + iwo1] = (float)(4.0 * a[jmx + iw])
					- (a[jmx + iw + mx] + a[jmx + iw - mx] + a[jmx + iwi1]);
				a[jmx + ieo1] = (float)(4.0 * a[jmx + ie])
					- (a[jmx + ie + mx] + a[jmx + ie - mx] + a[jmx + iei1]);
			}

			/* Copy that result to 2nd outside row using periodicity.  */
			a[jno2 + iwo1] = a[jno2k + iwo1];
			a[jso2 + iwo1] = a[jso2k + iwo1];
			a[jno2 + ieo1] = a[jno2k + ieo1];
			a[jso2 + ieo1] = a[jso2k + ieo1];

			/* Now set d[laplacian]/dx = 0 on 2nd outside column.  Include
				1st outside rows in loop.  */
			for (jmx = jno1; jmx <= jso1; jmx += mx) {
				a[jmx + iwo2] = (a[jmx + iw - mx] + a[jmx + iw + mx] + a[jmx + iwi1])
					- (a[jmx + iwo1 - mx] + a[jmx + iwo1 + mx])
					+ (float)(5.0 * (a[jmx + iwo1] - a[jmx + iw]));

				a[jmx + ieo2] = (a[jmx + ie - mx] + a[jmx + ie + mx] + a[jmx + iei1])
					- (a[jmx + ieo1 - mx] + a[jmx + ieo1 + mx])
					+ (float)(5.0 * (a[jmx + ieo1] - a[jmx + ie]));
			}

			/* Now copy that result also, for complete periodicity's sake  */
			a[jno2 + iwo2] = a[jno2k + iwo2];
			a[jso2 + iwo2] = a[jso2k + iwo2];
			a[jno2 + ieo2] = a[jno2k + ieo2];
			a[jso2 + ieo2] = a[jso2k + ieo2];

			/* DONE with X not periodic, Y periodic case.  Fully loaded.  */

			return (GMT_NOERROR);
		}
		else {
			/* Here begins the X not periodic, Y not periodic case  */

			/* First, set corner points.  Need not merely Laplacian(f) = 0
				but explicitly that d2f/dx2 = 0 and d2f/dy2 = 0.
				Also set d2f/dxdy = 0.  Then can set remaining points.  */

	/* d2/dx2 */	a[jn + iwo1]   = (float)(2.0 * a[jn + iw] - a[jn + iwi1]);
	/* d2/dy2 */	a[jno1 + iw]   = (float)(2.0 * a[jn + iw] - a[jni1 + iw]);
	/* d2/dxdy */	a[jno1 + iwo1] = -(a[jno1 + iwi1] - a[jni1 + iwi1]
						+ a[jni1 + iwo1]);


	/* d2/dx2 */	a[jn + ieo1]   = (float)(2.0 * a[jn + ie] - a[jn + iei1]);
	/* d2/dy2 */	a[jno1 + ie]   = (float)(2.0 * a[jn + ie] - a[jni1 + ie]);
	/* d2/dxdy */	a[jno1 + ieo1] = -(a[jno1 + iei1] - a[jni1 + iei1]
						+ a[jni1 + ieo1]);

	/* d2/dx2 */	a[js + iwo1]   = (float)(2.0 * a[js + iw] - a[js + iwi1]);
	/* d2/dy2 */	a[jso1 + iw]   = (float)(2.0 * a[js + iw] - a[jsi1 + iw]);
	/* d2/dxdy */	a[jso1 + iwo1] = -(a[jso1 + iwi1] - a[jsi1 + iwi1]
						+ a[jsi1 + iwo1]);

	/* d2/dx2 */	a[js + ieo1]   = (float)(2.0 * a[js + ie] - a[js + iei1]);
	/* d2/dy2 */	a[jso1 + ie]   = (float)(2.0 * a[js + ie] - a[jsi1 + ie]);
	/* d2/dxdy */	a[jso1 + ieo1] = -(a[jso1 + iei1] - a[jsi1 + iei1]
						+ a[jsi1 + ieo1]);

			/* Now set Laplacian = 0 on interior edge points,
				skipping corners:  */
			for (i = iwi1; i <= iei1; i++) {
				a[jno1 + i] = (float)(4.0 * a[jn + i])
					- (a[jn + i - 1] + a[jn + i + 1]
						+ a[jni1 + i]);

				a[jso1 + i] = (float)(4.0 * a[js + i])
					- (a[js + i - 1] + a[js + i + 1]
						+ a[jsi1 + i]);
			}
			for (jmx = jni1; jmx <= jsi1; jmx += mx) {
				a[iwo1 + jmx] = (float)(4.0 * a[iw + jmx])
					- (a[iw + jmx + mx] + a[iw + jmx - mx]
						+ a[iwi1 + jmx]);
				a[ieo1 + jmx] = (float)(4.0 * a[ie + jmx])
					- (a[ie + jmx + mx] + a[ie + jmx - mx]
						+ a[iei1 + jmx]);
			}

			/* Now set d[Laplacian]/dn = 0 on all edge pts, including
				corners, since the points needed in this are now set.  */
			for (i = iw; i <= ie; i++) {
				a[jno2 + i] = a[jni1 + i]
					+ (float)(5.0 * (a[jno1 + i] - a[jn + i]))
					+ (a[jn + i - 1] - a[jno1 + i - 1])
					+ (a[jn + i + 1] - a[jno1 + i + 1]);
				a[jso2 + i] = a[jsi1 + i]
					+ (float)(5.0 * (a[jso1 + i] - a[js + i]))
					+ (a[js + i - 1] - a[jso1 + i - 1])
					+ (a[js + i + 1] - a[jso1 + i + 1]);
			}
			for (jmx = jn; jmx <= js; jmx += mx) {
				a[iwo2 + jmx] = a[iwi1 + jmx]
					+ (float)(5.0 * (a[iwo1 + jmx] - a[iw + jmx]))
					+ (a[iw + jmx - mx] - a[iwo1 + jmx - mx])
					+ (a[iw + jmx + mx] - a[iwo1 + jmx + mx]);
				a[ieo2 + jmx] = a[iei1 + jmx]
					+ (float)(5.0 * (a[ieo1 + jmx] - a[ie + jmx]))
					+ (a[ie + jmx - mx] - a[ieo1 + jmx - mx])
					+ (a[ie + jmx + mx] - a[ieo1 + jmx + mx]);
			}
			/* DONE with X not periodic, Y not periodic case.
				Loaded all but three cornermost points at each corner.  */

			return (GMT_NOERROR);
		}
		/* DONE with all X not periodic cases  */
	}
	else {
		/* X is periodic.  Load x cols first, then do Y cases.  */

		for (jmx = jn; jmx <= js; jmx += mx) {
			a[iwo1 + jmx] = a[iwo1k + jmx];
			a[iwo2 + jmx] = a[iwo2k + jmx];
			a[ieo1 + jmx] = a[ieo1k + jmx];
			a[ieo2 + jmx] = a[ieo2k + jmx];
		}

		if (edgeinfo->nyp > 0) {
			/* Y is periodic.  copy all, including boundary cols:  */
			for (i = iwo2; i <= ieo2; i++) {
				a[jno1 + i] = a[jno1k + i];
				a[jno2 + i] = a[jno2k + i];
				a[jso1 + i] = a[jso1k + i];
				a[jso2 + i] = a[jso2k + i];
			}
			/* DONE with X and Y both periodic.  Fully loaded.  */

			return (GMT_NOERROR);
		}

		/* Do north (top) boundary:  */

		if (edgeinfo->gn) {
			/* Y is at north pole.  Phase-shift all, incl. bndry cols. */
			if (h->node_offset) {
				j1p = jn;	/* constraint for jno1  */
				j2p = jni1;	/* constraint for jno2  */
			}
			else {
				j1p = jni1;	/* constraint for jno1  */
				j2p = jni1 + mx;	/* constraint for jno2  */
			}
			for (i = iwo2; i <= ieo2; i++) {
				i180 = pad[0] + ((i + nxp2)%edgeinfo->nxp);
				a[jno1 + i] = a[j1p + i180];
				a[jno2 + i] = a[j2p + i180];
			}
		}
		else {
			/* Y needs natural conditions.  x bndry cols periodic.
				First do Laplacian.  Start/end loop 1 col outside,
				then use periodicity to set 2nd col outside.  */

			for (i = iwo1; i <= ieo1; i++) {
				a[jno1 + i] = (float)(4.0 * a[jn + i])
					- (a[jn + i - 1] + a[jn + i + 1] + a[jni1 + i]);
			}
			a[jno1 + iwo2] = a[jno1 + iwo2 + edgeinfo->nxp];
			a[jno1 + ieo2] = a[jno1 + ieo2 - edgeinfo->nxp];


			/* Now set d[Laplacian]/dn = 0, start/end loop 1 col out,
				use periodicity to set 2nd out col after loop.  */

			for (i = iwo1; i <= ieo1; i++) {
				a[jno2 + i] = a[jni1 + i]
					+ (float)(5.0 * (a[jno1 + i] - a[jn + i]))
					+ (a[jn + i - 1] - a[jno1 + i - 1])
					+ (a[jn + i + 1] - a[jno1 + i + 1]);
			}
			a[jno2 + iwo2] = a[jno2 + iwo2 + edgeinfo->nxp];
			a[jno2 + ieo2] = a[jno2 + ieo2 - edgeinfo->nxp];

			/* End of X is periodic, north (top) is Natural.  */

		}

		/* Done with north (top) BC in X is periodic case.  Do south (bottom)  */

		if (edgeinfo->gs) {
			/* Y is at south pole.  Phase-shift all, incl. bndry cols. */
			if (h->node_offset) {
				j1p = js;	/* constraint for jso1  */
				j2p = jsi1;	/* constraint for jso2  */
			}
			else {
				j1p = jsi1;	/* constraint for jso1  */
				j2p = jsi1 - mx;	/* constraint for jso2  */
			}
			for (i = iwo2; i <= ieo2; i++) {
				i180 = pad[0] + ((i + nxp2)%edgeinfo->nxp);
				a[jso1 + i] = a[j1p + i180];
				a[jso2 + i] = a[j2p + i180];
			}
		}
		else {
			/* Y needs natural conditions.  x bndry cols periodic.
				First do Laplacian.  Start/end loop 1 col outside,
				then use periodicity to set 2nd col outside.  */

			for (i = iwo1; i <= ieo1; i++) {
				a[jso1 + i] = (float)(4.0 * a[js + i])
					- (a[js + i - 1] + a[js + i + 1] + a[jsi1 + i]);
			}
			a[jso1 + iwo2] = a[jso1 + iwo2 + edgeinfo->nxp];
			a[jso1 + ieo2] = a[jso1 + ieo2 - edgeinfo->nxp];


			/* Now set d[Laplacian]/dn = 0, start/end loop 1 col out,
				use periodicity to set 2nd out col after loop.  */

			for (i = iwo1; i <= ieo1; i++) {
				a[jso2 + i] = a[jsi1 + i]
					+ (float)(5.0 * (a[jso1 + i] - a[js + i]))
					+ (a[js + i - 1] - a[jso1 + i - 1])
					+ (a[js + i + 1] - a[jso1 + i + 1]);
			}
			a[jso2 + iwo2] = a[jso2 + iwo2 + edgeinfo->nxp];
			a[jso2 + ieo2] = a[jso2 + ieo2 - edgeinfo->nxp];

			/* End of X is periodic, south (bottom) is Natural.  */

		}

		/* Done with X is periodic cases.  */

		return (GMT_NOERROR);
	}
}

BOOLEAN GMT_y_out_of_bounds (int *j, struct GRD_HEADER *h, struct GMT_EDGEINFO *edgeinfo, BOOLEAN *wrap_180) {
	/* Adjusts the j (y-index) value if we are dealing with some sort of periodic boundary
	* condition.  If a north or south pole condition we must "go over the pole" and access
	* the longitude 180 degrees away - this is achieved by passing the wrap_180 flag; the
	* shifting of longitude is then deferred to GMT_x_out_of_bounds.
	* If no periodicities are present then nothing happens here.  If we end up being
	* out of bounds we return TRUE (and main program can take action like continue);
	* otherwise we return FALSE.
	*/

	if ((*j) < 0) {	/* Depending on BC's we wrap around or we are above the top of the domain */
		if (edgeinfo->gn) {	/* N Polar condition - adjust j and set wrap flag */
			(*j) = abs (*j) - h->node_offset;
			(*wrap_180) = TRUE;	/* Go "over the pole" */
		}
		else if (edgeinfo->nyp) {	/* Periodic in y */
			(*j) += edgeinfo->nyp;
			(*wrap_180) = FALSE;
		}
		else
			return (TRUE);	/* We are outside the range */
	}
	else if ((*j) >= h->ny) {	/* Depending on BC's we wrap around or we are below the bottom of the domain */
		if (edgeinfo->gs) {	/* S Polar condition - adjust j and set wrap flag */
			(*j) += h->node_offset - 2;
			(*wrap_180) = TRUE;	/* Go "over the pole" */
		}
		else if (edgeinfo->nyp) {	/* Periodic in y */
			(*j) -= edgeinfo->nyp;
			(*wrap_180) = FALSE;
		}
		else
			return (TRUE);	/* We are outside the range */
	}
	else
		(*wrap_180) = FALSE;

	return (FALSE);	/* OK, we are inside grid now for sure */
}

BOOLEAN GMT_x_out_of_bounds (int *i, struct GRD_HEADER *h, struct GMT_EDGEINFO *edgeinfo, BOOLEAN wrap_180) {
	/* Adjusts the i (x-index) value if we are dealing with some sort of periodic boundary
	* condition.  If a north or south pole condition we must "go over the pole" and access
	* the longitude 180 degrees away - this is achieved by examining the wrap_180 flag and take action.
	* If no periodicities are present and we end up being out of bounds we return TRUE (and
	* main program can take action like continue); otherwise we return FALSE.
	*/

	/* Depending on BC's we wrap around or leave as is. */

	if ((*i) < 0) {	/* Potentially outside to the left of the domain */
		if (edgeinfo->nxp)	/* Periodic in x - always inside grid */
			(*i) += edgeinfo->nxp;
		else	/* Sorry, you're outside */
			return (TRUE);
	}
	else if ((*i) >= h->nx) {	/* Potentially outside to the right of the domain */
		if (edgeinfo->nxp)	/* Periodic in x -always inside grid */
			(*i) -= edgeinfo->nxp;
		else	/* Sorry, you're outside */
			return (TRUE);
	}

	if (wrap_180) (*i) = ((*i) + (edgeinfo->nxp / 2)) % edgeinfo->nxp;	/* Must move 180 degrees */

	return (FALSE);	/* OK, we are inside grid now for sure */
}

void GMT_setcontjump (float *z, int nz)
{
/* This routine will check if there is a 360 jump problem
 * among these coordinates and adjust them accordingly so
 * that subsequent testing can determine if a zero contour
 * goes through these edges.  E.g., values like 359, 1
 * should become -1, 1 after this function */

	int i;
	BOOLEAN jump = FALSE;
	double dz;

	for (i = 1; !jump && i < nz; i++) {
		dz = z[i] - z[0];
		if (fabs (dz) > 180.0) jump = TRUE;
	}

	if (!jump) return;

	z[0] = (float)fmod (z[0], 360.0);
	if (z[0] > 180.0) z[0] -= 360.0;
	for (i = 1; i < nz; i++) {
		if (z[i] > 180.0) z[i] -= 360.0;
		dz = z[i] - z[0];
		if (fabs (dz) > 180.0) z[i] -= (float)copysign (360.0, dz);
		z[i] = (float)fmod (z[i], 360.0);
	}
}

void GMT_set_xy_domain (double wesn_extended[], struct GRD_HEADER *h)
{
	double off;
	/* Sets the domain boundaries to be used to determine if x,y coordinates
	 * are outside the domain of a grid.  If gridline-registered then the
	 * domain is extended by 0.5 the grid interval.  Note that points with
	 * x == x_max and y = y_max are considered inside.
	 */

	off = 0.5 - h->xy_off;
	if (GMT_io.in_col_type[0] == GMT_IS_LON && GMT_360_RANGE (h->x_max, h->x_min)) {	/* Global longitude range */
		wesn_extended[0] = h->x_min;	wesn_extended[1] = h->x_max;
	}
	else {
		wesn_extended[0] = h->x_min - off * h->x_inc;	wesn_extended[1] = h->x_max + off * h->x_inc;
	}
	/* Latitudes can be extended provided we are not at the poles */
	wesn_extended[2] = h->y_min - off * h->y_inc;
	wesn_extended[3] = h->y_max + off * h->y_inc;
	if (GMT_io.in_col_type[1] == GMT_IS_LAT) {
		if (wesn_extended[2] < -90.0) wesn_extended[2] = -90.0;
		if (wesn_extended[3] > +90.0) wesn_extended[3] = +90.0;
	}
}

BOOLEAN GMT_x_is_outside (double *x, double left, double right)
{
	/* Determines if this x is inside the effective x-domain.  This is normally
	 * west to east, but when gridding is concerned it can be extended by +-0.5 * dx
	 * for gridline-registered grids.  Also, if x is longitude we must check for
	 * wrap-arounds by 360 degrees, and x may be modified accordingly.
	 */

	if (GMT_io.in_col_type[0] == GMT_IS_LON) {	/* Periodic longitude test */
		while ((*x) > left) (*x) -= 360.0;	/* Make sure we start west or west */
		while ((*x) < left) (*x) += 360.0;	/* See if we are outside east */
		return (((*x) > right) ? TRUE : FALSE);
	}
	else	/* Cartesian test */
		return (((*x) < left || (*x) > right) ? TRUE : FALSE);
}

int GMT_getscale (char *text, struct GMT_MAP_SCALE *ms)
{
	/* Pass text as &argv[i][2] */

	int j = 0, i, ns, n_slash, error = 0, colon, plus, k;
	char txt_a[GMT_LONG_TEXT], txt_b[GMT_LONG_TEXT], txt_sx[GMT_LONG_TEXT], txt_sy[GMT_LONG_TEXT], txt_pf[2][GMT_LONG_TEXT];
	BOOLEAN ok;

	ms->fancy = ms->gave_xy = FALSE;
	ms->measure = ms->label[0] = '\0';
	ms->length = 0.0;
	ms->justify = 't';
	ms->boxdraw = ms->boxfill = FALSE;
	memcpy ((void *)ms->fill.rgb, (void *)GMT_no_rgb, 3 * sizeof (int));

	/* First deal with possible prefixes f and x (i.e., f, x, xf, fx) */
	if (text[j] == 'f') ms->fancy = TRUE, j++;
	if (text[j] == 'x') ms->gave_xy = TRUE, j++;
	if (text[j] == 'f') ms->fancy = TRUE, j++;	/* in case we got xf instead of fx */

	/* Determine if we have the optional longitude component specified */

	for (n_slash = 0, i = j; text[i] && text[i] != '+'; i++) if (text[i] == '/') n_slash++;

	/* Determine if we have the optional label/justify component specified.  We search from
	 * the end of the text until we reach the last slash, and update the colon position when
	 * we find a colon.  When the loop exits the colon variable should point to the start of
	 * the label string */

	for (colon = -1, i = strlen (text) - 1, ok = TRUE; i && text[i] && ok; i--) {
		if (text[i] == ':') colon = i+1;
		if (text[i] == '/') ok = FALSE;	/* Do not search further than to the last slash */
	}
	for (plus = -1, i = j; text[i] && plus < 0; i++) if (text[i] == '+') plus = i+1;

	if (n_slash == 4) {		/* -L[f][x]<x0>/<y0>/<lon>/<lat>/<length>[m|n|k][:label:<just>][+p<pen>]+[f<fill>] */
		k = sscanf (&text[j], "%[^/]/%[^/]/%[^/]/%[^/]/%lf", txt_a, txt_b, txt_sx, txt_sy, &ms->length);
	}
	else if (n_slash == 3) {	/* -L[f][x]<x0>/<y0>/<lat>/<length>[m|n|k][:label:<just>][+p<pen>]+[f<fill>] */
		k = sscanf (&text[j], "%[^/]/%[^/]/%[^/]/%lf", txt_a, txt_b, txt_sy, &ms->length);
	}
	else {	/* Wrong number of slashes */
		fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  Correct syntax\n", GMT_program);
		fprintf (stderr, "\t-L[f][x]<x0>/<y0>/[<lon>/]<lat>/<length>[m|n|k], append m, n, or k for miles, nautical miles, or km [Default]\n");
		k = 0;
		error++;
	}
	if (colon > 0) {	/* Get label and justification which are in :label:<just> format */
		sscanf (&text[colon], "%[^:]:%c", ms->label, &ms->justify);
		ms->measure = text[colon-2];
	}
	if (plus > 0) {	/* Get pen/fill for bacground rectangle */
		ns = sscanf (&text[plus], "%[^+]+%s", txt_pf[0], txt_pf[1]);
		if (colon <= 0) ms->measure = text[plus-2];
		for (i = 0; i < ns; i++) {
			if (txt_pf[i][0] == 'p') {	/* Pen specification */
				error += GMT_getpen (&txt_pf[i][1], &ms->pen);
				ms->boxdraw = TRUE;
			}
			else if (txt_pf[i][0] == 'f') {	/* Fill specification */
				error += GMT_getfill (&txt_pf[i][1], &ms->fill);
				ms->boxfill = TRUE;
			}
		}
	}
	if (!(colon > 0 || plus > 0)) ms->measure = text[strlen(text)-1];
	if (ms->gave_xy) {	/* Convert user's x/y to inches */
		ms->x0 = GMT_convert_units (txt_a, GMT_INCH);
		ms->y0 = GMT_convert_units (txt_b, GMT_INCH);
	}
	else {	/* Read geographical coordinates */
		error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &ms->x0), txt_a);
		error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &ms->y0), txt_b);
		if (fabs (ms->y0) > 90.0) {
			fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  Position latitude is out of range\n", GMT_program);
			error++;
		}
		if (fabs (ms->x0) > 360.0) {
			fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  Position longitude is out of range\n", GMT_program);
			error++;
		}
	}
	error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_sy, GMT_IS_LAT, &ms->scale_lat), txt_sy);
	if (k == 5)	/* Must also decode longitude of scale */
		error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_sx, GMT_IS_LON, &ms->scale_lon), txt_sx);
	else		/* Default to central meridian */
		ms->scale_lon = project_info.central_meridian;
	if (fabs (ms->scale_lat) > 90.0) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  Scale latitude is out of range\n", GMT_program);
		error++;
	}
	if (fabs (ms->scale_lon) > 360.0) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  Scale longitude is out of range\n", GMT_program);
		error++;
	}
	if (k < 4 || k > 5) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  Correct syntax\n", GMT_program);
		fprintf (stderr, "\t-L[f][x]<x0>/<y0>/[<lon>/]<lat>/<length>[m|n|k], append m, n, or k for miles, nautical miles, or km [Default]\n");
		error++;
	}
	if (ms->length <= 0.0) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  Length must be positive\n", GMT_program);
		error++;
	}
	if (fabs (ms->scale_lat) > 90.0) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  Defining latitude is out of range\n", GMT_program);
		error++;
	}
	if (isalpha ((int)(ms->measure)) && ! ((ms->measure) == 'm' || (ms->measure) == 'n' || (ms->measure) == 'k')) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  Valid units are m, n, or k\n", GMT_program);
		error++;
	}

	ms->plot = TRUE;
	return (error);
}

int GMT_getrose (char *text, struct GMT_MAP_ROSE *ms)
{
	/* Pass text as &argv[i][2] */

	int j = 0, i,error = 0, colon, plus, slash, k, pos, order[4] = {3,1,0,2};
	char txt_a[GMT_LONG_TEXT], txt_b[GMT_LONG_TEXT], txt_c[GMT_LONG_TEXT], txt_d[GMT_LONG_TEXT], tmpstring[GMT_LONG_TEXT], p[GMT_LONG_TEXT];

	/* SYNTAX is -T[f|m][x]<lon0>/<lat0>/<size>[/<info>][:label:][+<aint>/<fint>/<gint>[/<aint>/<fint>/<gint>]], where <info> is
	 * 1)  -Tf: <info> is <kind> = 1,2,3 which is the level of directions [1].
	 * 2)  -Tm: <info> is <dec>/<dlabel>, where <Dec> is magnetic declination and dlabel its label [no mag].
	 * If -Tm, optionally set annotation interval with +
	 */

	ms->fancy = ms->gave_xy = FALSE;
	ms->size = 0.0;
	ms->a_int[0] = 10.0;	ms->f_int[0] = 5.0;	ms->g_int[0] = 1.0;
	ms->a_int[1] = 30.0;	ms->f_int[1] = 5.0;	ms->g_int[1] = 1.0;
	strcpy (ms->label[0], "S");
	strcpy (ms->label[1], "E");
	strcpy (ms->label[2], "N");
	strcpy (ms->label[3], "W");

	/* First deal with possible prefixes f and x (i.e., f|m, x, xf|m, f|mx) */
	if (text[j] == 'f') ms->fancy = TRUE, j++;
	if (text[j] == 'm') ms->fancy = 2, j++;
	if (text[j] == 'x') ms->gave_xy = TRUE, j++;
	if (text[j] == 'f') ms->fancy = TRUE, j++;	/* in case we got xf instead of fx */
	if (text[j] == 'm') ms->fancy = 2, j++;		/* in case we got xm instead of mx */

	/* Determine if we have the optional label components specified */

	for (i = j, slash = 0; text[i] && slash < 2; i++) if (text[i] == '/') slash++;	/* Move i until the 2nd slash is reached */
	for (k = strlen(text) - 1, colon = 0; text[k] && k > i && colon < 2; k--) if (text[k] == ':') colon++;	/* Move k to starting colon of :label: */
	if (colon == 2 && k > i) {
		colon = k + 2;	/* Beginning of label */
	}
	else
		colon = 0;	/* No labels given */

	for (plus = -1, i = slash; text[i] && plus < 0; i++) if (text[i] == '+') plus = i+1;	/* Find location of + */
	if (plus > 0) {		/* Get annotation interval(s) */
		k = sscanf (&text[plus], "%lf/%lf/%lf/%lf/%lf/%lf", &ms->a_int[1], &ms->f_int[1], &ms->g_int[1], &ms->a_int[0], &ms->f_int[0], &ms->g_int[0]);
		if (k < 1) {
			fprintf (stderr, "%s: GMT SYNTAX ERROR -T option:  Give annotation interval(s)\n", GMT_program);
			error++;
		}
		if (k == 3) ms->a_int[0] = ms->a_int[1], ms->f_int[0] = ms->f_int[1], ms->g_int[0] = ms->g_int[1];
		text[plus-1] = '\0';	/* Break string so sscanf wont get confused later */
	}
	if (colon > 0) {	/* Get labels in string :w,e,s,n: */
		for (k = colon; text[k] && text[k] != ':'; k++);	/* Look for terminating colon */
		if (text[k] != ':') { /* Ran out, missing terminating colon */
			fprintf (stderr, "%s: GMT SYNTAX ERROR -T option: Labels must be given in format :w,e,s,n:\n", GMT_program);
			error++;
			return (error);
		}
		strncpy (tmpstring, &text[colon], k-colon);
		tmpstring[k-colon] = '\0';
		k = pos = 0;
		while (k < 4 && (GMT_strtok (tmpstring, ",", &pos, p))) {	/* Get the four labels */
			if (strcmp (p, "-")) strcpy (ms->label[order[k]], p);
			k++;
		}
		if (k == 0) {	/* No labels wanted */
			ms->label[0][0] = ms->label[1][0] = ms->label[2][0] = ms->label[3][0] = '\0';
		}
		else if (k != 4) {	/* Ran out of labels */
			fprintf (stderr, "%s: GMT SYNTAX ERROR -T option: Labels must be given in format :w,e,s,n:\n", GMT_program);
			error++;
		}
		text[colon-1] = '\0';	/* Break string so sscanf wont get confused later */
	}

	/* -L[f][x]<x0>/<y0>/<size>[/<kind>][:label:] OR -L[m][x]<x0>/<y0>/<size>[/<dec>/<declabel>][:label:][+gint[/mint]] */
	if (ms->fancy == 2) {	/* Magnetic rose */
		k = sscanf (&text[j], "%[^/]/%[^/]/%[^/]/%[^/]/%[^/]", txt_a, txt_b, txt_c, txt_d, ms->dlabel);
		if (! (k == 3 || k == 5)) {	/* Wrong number of parameters */
			fprintf (stderr, "%s: GMT SYNTAX ERROR -T option:  Correct syntax\n", GMT_program);
			fprintf (stderr, "\t-T[f|m][x]<x0>/<y0>/<size>[/<info>][:wesnlabels:][+<gint>[/<mint>]]\n");
			error++;
		}
		if (k == 3) {	/* No magnetic north directions */
			ms->kind = 1;
			ms->declination = 0.0;
			ms->dlabel[0] = '\0';
		}
		else {
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_d, GMT_IS_LON, &ms->declination), txt_d);
			ms->kind = 2;
		}
	}
	else {
		k = sscanf (&text[j], "%[^/]/%[^/]/%[^/]/%d", txt_a, txt_b, txt_c, &ms->kind);
		if (k == 3) ms->kind = 1;
		if (k < 3 || k > 4) {	/* Wrong number of parameters */
			fprintf (stderr, "%s: GMT SYNTAX ERROR -T option:  Correct syntax\n", GMT_program);
			fprintf (stderr, "\t-T[f|m][x]<x0>/<y0>/<size>[/<info>][:wesnlabels:][+<gint>[/<mint>]]\n");
			error++;
		}
	}
	if (colon > 0) text[colon-1] = ':';	/* Put it back */
	if (plus > 0) text[plus-1] = '+';	/* Put it back */
	if (ms->gave_xy) {	/* Convert user's x/y to inches */
		ms->x0 = GMT_convert_units (txt_a, GMT_INCH);
		ms->y0 = GMT_convert_units (txt_b, GMT_INCH);
	}
	else {	/* Read geographical coordinates */
		error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &ms->x0), txt_a);
		error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &ms->y0), txt_b);
		if (fabs (ms->y0) > 90.0) {
			fprintf (stderr, "%s: GMT SYNTAX ERROR -T option:  Position latitude is out of range\n", GMT_program);
			error++;
		}
		if (fabs (ms->x0) > 360.0) {
			fprintf (stderr, "%s: GMT SYNTAX ERROR -T option:  Position longitude is out of range\n", GMT_program);
			error++;
		}
	}
	ms->size = GMT_convert_units (txt_c, GMT_INCH);
	if (ms->size <= 0.0) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -T option:  Size must be positive\n", GMT_program);
		error++;
	}
	if (ms->kind < 1 || ms->kind > 3) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR -L option:  <kind> must be 1, 2, or 3\n", GMT_program);
		error++;
	}

	ms->plot = TRUE;
	return (error);
}

int GMT_minmaxinc_verify (double min, double max, double inc, double slop)
{
	double checkval, range;

	/* Check for how compatible inc is with the range max - min.
	   We will tolerate a fractional sloppiness <= slop.  The
	   return values are:
	   0 : Everything is ok
	   1 : range is not a whole multiple of inc (within assigned slop)
	   2 : the range (max - min) is < 0
	   3 : inc is <= 0
	*/

	if (inc <= 0.0) return (3);

	if ((range = (max - min)) < 0.0) return (2);

	checkval = (fmod (max - min, inc)) / inc;
	if (checkval > slop && checkval < (1.0 - slop)) return 1;
	return 0;
}

void GMT_str_tolower (char *value)
{
	/* Convert entire string to lower case */
	int i, c;
	for (i = 0; value[i]; i++) {
		c = (int)value[i];
		value[i] = (char) tolower (c);
	}
}

void GMT_str_toupper (char *value)
{
	/* Convert entire string to upper case */
	int i, c;
	for (i = 0; value[i]; i++) {
		c = (int)value[i];
		value[i] = (char) toupper (c);
	}
}

void GMT_chop (char *string)
{
	/* Chops off any CR or LF at end of string and ensures it is null-terminated */
	int i, n;
	if (!string) return;	/* NULL pointer */
	if ((n = strlen (string)) == 0) return;	/* Empty string */
	for (i = n - 1; i >= 0 && (string[i] == '\n' || string[i] == '\r'); i--);
	i++;
	if (i >= 0) string[i] = '\0';	/* Terminate string */
}

int GMT_strtok (const char *string, const char *sep, int *pos, char *token)
{
	/* Reentrant replacement for strtok that uses no static variables.
	 * Breaks string into tokens separated by one of more separator
	 * characters (in sep).  Set *pos to 0 before first call.  Unlike
	 * strtok, always pass the original string as first argument.
	 * Returns 1 if it finds a token and 0 if no more tokens left.
	 * pos is updated and token is returned.  char *token must point
	 * to memory of length >= strlen (string).
	 */

	int i, j, string_len;

	string_len = strlen (string);

	/* Wind up *pos to first non-separating character: */
	while (string[*pos] && strchr (sep, (int)string[*pos])) (*pos)++;

	token[0] = '\0';	/* Initialize token to NULL in case we are at end */

	if (*pos >= string_len || string_len == 0) return 0;	/* Got NULL string or no more string left to search */

	/* Search for next non-separating character */
	for (i = *pos; string[i] && !strchr (sep, (int)string[i]); i++);

	/* Copy token */
	j = i - *pos;
	strncpy (token, &string[*pos], j);
	token[j] = 0;

	/* Wind up *pos to next non-separating character */
	while (string[i] && strchr (sep, (int)string[i])) i++;
	*pos = i;

	return 1;
}

double GMT_get_map_interval (int axis, int item) {

	if (item < GMT_ANNOT_UPPER || item > GMT_GRID_LOWER) {
		fprintf (stderr, "GMT ERROR in GMT_get_map_interval (wrong item %d)\n", item);
		GMT_exit (EXIT_FAILURE);
	}

	switch (frame_info.axis[axis].item[item].unit) {
		case 'm':	/* arc Minutes */
			return (frame_info.axis[axis].item[item].interval * GMT_MIN2DEG);
			break;
		case 'c':	/* arc Seconds */
			return (frame_info.axis[axis].item[item].interval * GMT_SEC2DEG);
			break;
		default:
			return (frame_info.axis[axis].item[item].interval);
			break;
	}
}

int GMT_just_decode (char *key, int i, int j)
{
	int k;
	/* i and j holds the default values - give -1 if they must be overridden */

	/* Converts justification info like LL (lower left) to justification indices */

	for (k = 0; k < (int)strlen (key); k++) {
		switch (key[k]) {
			case 'b':	/* Bottom baseline */
			case 'B':
				j = 0;
				break;
			case 'm':	/* Middle baseline */
			case 'M':
				j = 4;
				break;
			case 't':	/* Top baseline */
			case 'T':
				j = 8;
				break;
			case 'l':	/* Left Justified */
			case 'L':
				i = 1;
				break;
			case 'c':	/* Center Justified */
			case 'C':
				i = 2;
				break;
			case 'r':	/* Right Justified */
			case 'R':
				i = 3;
				break;
			default:
				return (-99);
		}
	}

	if (i < 0) {
		fprintf (stderr, "%s: Horizontal text justification not set, defaults to L(eft)\n", GMT_program);
		i = 1;
	}
	if (j < 0) {
		fprintf (stderr, "%s: Vertical text justification not set, defaults to B(ottom)\n", GMT_program);
		j = 1;
	}

	return (j + i);
}

void GMT_smart_justify (int just, double angle, double dx, double dy, double *x_shift, double *y_shift)
{
	double s, c, xx, yy;
	sincosd (angle, &s, &c);
	xx = (2 - (just%4)) * dx;	/* Smart shift in x */
	yy = (1 - (just/4)) * dy;	/* Smart shift in x */
	*x_shift += c * xx - s * yy;	/* Must account for angle of label */
	*y_shift += s * xx + c * yy;
}

int GMT_verify_expectations (int wanted, int got, char *item)
{	/* Compare what we wanted with what we got and see if it is OK */
	int error = 0;

	if (wanted == GMT_IS_UNKNOWN) {	/* No expectations set */
		switch (got) {
			case GMT_IS_ABSTIME:	/* Found a T in the string - ABSTIME ? */
				fprintf (stderr, "%s: GMT ERROR: %s appears to be an Absolute Time String: ", GMT_program, item);
				if (GMT_IS_MAPPING)
					fprintf (stderr, "This is not allowed for a map projection\n");
				else
					fprintf (stderr, "You must specify time data type with option -f.\n");
				error++;
				break;

			case GMT_IS_GEO:	/* Found a : in the string - GEO ? */
				fprintf (stderr, "%s: GMT Warning:  %s appears to be a Geographical Location String: ", GMT_program, item);
				if (project_info.projection == GMT_LINEAR)
					fprintf (stderr, "You should append d to the -Jx or -JX projection for geographical data.\n");
				else
					fprintf (stderr, "You should specify geographical data type with option -f.\n");
				fprintf (stderr, "%s will proceed assuming geographical input data.\n", GMT_program);
				break;

			case GMT_IS_LON:	/* Found a : in the string and then W or E - LON ? */
				fprintf (stderr, "%s: GMT Warning:  %s appears to be a Geographical Longitude String: ", GMT_program, item);
				if (project_info.projection == GMT_LINEAR)
					fprintf (stderr, "You should append d to the -Jx or -JX projection for geographical data.\n");
				else
					fprintf (stderr, "You should specify geographical data type with option -f.\n");
				fprintf (stderr, "%s will proceed assuming geographical input data.\n", GMT_program);
				break;

			case GMT_IS_LAT:	/* Found a : in the string and then S or N - LAT ? */
				fprintf (stderr, "%s: GMT Warning:  %s appears to be a Geographical Latitude String: ", GMT_program, item);
				if (project_info.projection == GMT_LINEAR)
					fprintf (stderr, "You should append d to the -Jx or -JX projection for geographical data.\n");
				else
					fprintf (stderr, "You should specify geographical data type with option -f.\n");
				fprintf (stderr, "%s will proceed assuming geographical input data.\n", GMT_program);
				break;

			case GMT_IS_FLOAT:
				break;
			default:
				break;
		}
	}
	else {
		switch (got) {
			case GMT_IS_NAN:
				fprintf (stderr, "%s: GMT ERROR:  Could not decode %s, return NaN.\n", GMT_program, item);
				error++;
				break;

			case GMT_IS_LAT:
				if (wanted == GMT_IS_LON) {
					fprintf (stderr, "%s: GMT ERROR:  Expected longitude, but %s is a latitude!\n", GMT_program, item);
					error++;
				}
				break;

			case GMT_IS_LON:
				if (wanted == GMT_IS_LAT) {
					fprintf (stderr, "%s: GMT ERROR:  Expected latitude, but %s is a longitude!\n", GMT_program, item);
					error++;
				}
				break;
			default:
				break;
		}
	}
	return (error);
}

void GMT_list_custom_symbols (void)
{
	/* Opens up GMT_custom_symbols.lis and dislays the list of custom symbols */

	FILE *fp;
	char list[GMT_LONG_TEXT], buffer[GMT_LONG_TEXT];

	/* Open the list in $GMT_SHAREDIR */

	GMT_getsharepath ("conf", "gmt_custom_symbols", ".conf", list);
	if ((fp = fopen (list, "r")) == NULL) {
		fprintf (stderr, "%s: ERROR: Cannot open file %s\n", GMT_program, list);
		return;
	}

	fprintf (stderr, "\t   Available custom symbols (See Appendix N):\n");
	fprintf (stderr, "\t   ---------------------------------------------------------\n");
	while (fgets (buffer, BUFSIZ, fp)) if (!(buffer[0] == '#' || buffer[0] == 0)) fprintf (stderr, "\t   %s", buffer);
	fclose (fp);
	fprintf (stderr, "\t   ---------------------------------------------------------\n");
}

/* Functions dealing with distance between points */

double GMT_dist_to_point (double lon, double lat, struct GMT_TABLE *T, int *id)
{
	int i, j;
	double d, d_min;

	d_min = DBL_MAX;
	for (i = 0; i < T->n_segments; i++) {
		for (j = 0; j < T->segment[i]->n_rows; j++) {
			d = (*GMT_distance_func) (lon, lat, T->segment[i]->coord[GMT_X][j], T->segment[i]->coord[GMT_Y][j]);
			if (d < d_min) {	/* Update the shortest distance and the point responsible */
				d_min = d;
				id[0] = i;
				id[1] = j;
			}
		}
	}
	return (d_min);
}

int GMT_near_a_point_spherical (double x, double y, struct GMT_TABLE *T, double dist)
{
	int i, j;
	BOOLEAN inside = FALSE, each_point_has_distance;
	double d;

	each_point_has_distance = (dist <= 0.0 && T->segment[0]->n_columns > 2);
	for (i = 0; !inside && i < T->n_segments; i++) {
		for (j = 0; !inside && j < T->segment[i]->n_rows; j++) {
			d = (*GMT_distance_func) (x, y, T->segment[i]->coord[GMT_X][j], T->segment[i]->coord[GMT_Y][j]);
			if (each_point_has_distance) dist = T->segment[i]->coord[GMT_Z][j];
			inside = (d <= dist);
		}
	}
	return (inside);
}

int GMT_near_a_point_cartesian (double x, double y, struct GMT_TABLE *T, double dist)
{
	int i, j;
	BOOLEAN inside = FALSE, each_point_has_distance;
	double d, x0, y0, xn, d0, dn;

	each_point_has_distance = (dist <= 0.0 && T->segment[0]->n_columns > 2);

	/* Assumes the points have been sorted so xp[0] is xmin and xp[n-1] is xmax] !!! */

	/* See if we are safely outside the range */
	x0 = T->segment[0]->coord[GMT_X][0];
	d0 = (each_point_has_distance) ? T->segment[0]->coord[GMT_Z][0] : dist;
	xn = T->segment[T->n_segments-1]->coord[GMT_X][T->segment[T->n_segments-1]->n_rows-1];
	dn = (each_point_has_distance) ? T->segment[T->n_segments-1]->coord[GMT_Z][T->segment[T->n_segments-1]->n_rows-1] : dist;
	if ((x < (x0 - d0)) || (x > (xn) + dn)) return (FALSE);

	/* No, must search the points */

	for (i = 0; !inside && i < T->n_segments; i++) {
		for (j = 0; !inside && j < T->segment[i]->n_rows; j++) {
			x0 = T->segment[i]->coord[GMT_X][j];
			d0 = (each_point_has_distance) ? T->segment[i]->coord[GMT_Z][j] : dist;
			if (fabs (x - x0) <= d0) {	/* Simple x-range test first */
				y0 = T->segment[i]->coord[GMT_Y][j];
				if (fabs (y - y0) <= d0) {	/* Simple y-range test next */
					/* Here we must compute distance */
					d = (*GMT_distance_func) (x, y, x0, y0);
					inside = (d <= d0);
				}
			}
		}
	}
	return (inside);
}

double GMT_cartesian_dist (double x0, double y0, double x1, double y1)
{
	/* Calculates the good-old straight line distance in users units */

	return (hypot ( (x1 - x0), (y1 - y0)));
}

double GMT_flatearth_dist_meter (double x0, double y0, double x1, double y1)
{
	/* Calculates the approximate flat earth distance in km.
	   If difference in longitudes exceeds 180 we pick the other
	   offset (360 - offset)
	 */
	double dlon;

	dlon = x1 - x0;
	if (fabs (dlon) > 180.0) dlon = copysign ((360.0 - fabs (dlon)), dlon);

	return (hypot ( dlon * cosd (0.5 * (y1 + y0)), (y1 - y0)) * DEG_TO_METER);
}

double GMT_flatearth_dist_km (double x0, double y0, double x1, double y1)
{
	/* Calculates the approximate flat earth distance in km.
	   If difference in longitudes exceeds 180 we pick the other
	   offset (360 - offset)
	 */

	return (0.001 * GMT_flatearth_dist_meter (x0, y0, x1, y1));
}

double GMT_great_circle_dist_km (double x0, double y0, double x1, double y1)
{
	/* Calculates the grdat circle distance in km */

	return (GMT_great_circle_dist (x0, y0, x1, y1) * DEG_TO_KM);
}

double GMT_great_circle_dist_meter (double x0, double y0, double x1, double y1)
{
	/* Calculates the grdat circle distance in meter */

	return (GMT_great_circle_dist (x0, y0, x1, y1) * DEG_TO_METER);
}

/* Functions involving distance from arbitrary points to a line */

int GMT_near_a_line_cartesian (double lon, double lat, struct GMT_TABLE *T, BOOLEAN return_mindist, double *dist_min, double *x_near, double *y_near)
{
	int seg, j0, j1;
	double edge, dx, dy, xc, yc, s, s_inv, d, dist_AB, fraction;
	BOOLEAN perpendicular_only = FALSE, interior;
	/* return_mindist is 1, 2, or 3.  All return the minimum distance. 2 also returns the coordinate of nearest point,
	   3 instead returns segment number and point number (fractional) of that point.  If 10 is added we exclude point
	   that are withing distance of the linesegment endpoints but project onto the extension of the line.  */

	if (return_mindist >= 10) {	/* Exclude circular region surrounding line endpoints */
		perpendicular_only = TRUE;
		return_mindist -= 10;
	}
	if (return_mindist) *dist_min = DBL_MAX;
	for (seg = 0; seg < T->n_segments; seg++) {	/* Loop over each line segment */

		if (T->segment[seg]->n_rows <= 0) continue;	/* empty; skip */

		if (return_mindist) T->segment[seg]->dist = 0.0;	/* Explicitly set dist to zero so the shortest distance can be found */

		/* Find nearest point on this line */

		for (j0 = 0; j0 < T->segment[seg]->n_rows; j0++) {	/* loop over nodes on current line */
			d = (*GMT_distance_func) (lon, lat, T->segment[seg]->coord[GMT_X][j0], T->segment[seg]->coord[GMT_Y][j0]);	/* Distance between our point and j'th node on seg'th line */
			if (return_mindist && d < (*dist_min)) {	/* Update min distance */
				*dist_min = d;
				if (return_mindist == 2) *x_near = T->segment[seg]->coord[GMT_X][j0], *y_near = T->segment[seg]->coord[GMT_Y][j0];	/* Also update (x,y) of nearest point on the line */
				if (return_mindist == 3) *x_near = (double)seg, *y_near = (double)j0;		/* Instead update (seg, pt) of nearest point on the line */
			}
			interior = (j0 > 0 && j0 < (T->segment[seg]->n_rows - 1));	/* Only FALSE if we are processing one of the end points */
			if (d <= T->segment[seg]->dist && (interior || !perpendicular_only)) return (TRUE);		/* Node inside the critical distance; we are done */
		}

		if (T->segment[seg]->n_rows < 2) continue;	/* 1-point "line" is a point; skip segment check */

		/* If we get here we must check for intermediate points along the straight lines between segment nodes.
		 * However, since we know all nodes are outside the circle, we first check if the pair of nodes making
		 * up the next line segment are outside of the circumscribing square before we need to solve for the
		 * intersection between the line segment and the normal from our point. */

		for (j0 = 0, j1 = 1; j1 < T->segment[seg]->n_rows; j0++, j1++) {	/* loop over straight segments on current line */
			if (!return_mindist) {
				edge = lon - T->segment[seg]->dist;
				if (T->segment[seg]->coord[GMT_X][j0] < edge && T->segment[seg]->coord[GMT_X][j1] < edge) continue;	/* Left of square */
				edge = lon + T->segment[seg]->dist;
				if (T->segment[seg]->coord[GMT_X][j0] > edge && T->segment[seg]->coord[GMT_X][j1] > edge) continue;	/* Right of square */
				edge = lat - T->segment[seg]->dist;
				if (T->segment[seg]->coord[GMT_Y][j0] < edge && T->segment[seg]->coord[GMT_Y][j1] < edge) continue;	/* Below square */
				edge = lat + T->segment[seg]->dist;
				if (T->segment[seg]->coord[GMT_Y][j0] > edge && T->segment[seg]->coord[GMT_Y][j1] > edge) continue;	/* Above square */
			}

			/* Here there is potential for the line segment crossing inside the circle */

			dx = T->segment[seg]->coord[GMT_X][j1] - T->segment[seg]->coord[GMT_X][j0];
			dy = T->segment[seg]->coord[GMT_Y][j1] - T->segment[seg]->coord[GMT_Y][j0];
			if (dx == 0.0) {		/* Line segment is vertical, our normal is thus horizontal */
				if (dy == 0.0) continue;	/* Dummy segment with no length */
				xc = T->segment[seg]->coord[GMT_X][j0];
				yc = lat;
				if (T->segment[seg]->coord[GMT_Y][j0] < yc && T->segment[seg]->coord[GMT_Y][j1] < yc ) continue;	/* Cross point is on extension */
				if (T->segment[seg]->coord[GMT_Y][j0] > yc && T->segment[seg]->coord[GMT_Y][j1] > yc ) continue;	/* Cross point is on extension */
			}
			else {	/* Line segment is not vertical */
				if (dy == 0.0) {	/* Line segment is horizontal, our normal is thus vertical */
					xc = lon;
					yc = T->segment[seg]->coord[GMT_Y][j0];
				}
				else {	/* General case of oblique line */
					s = dy / dx;
					s_inv = -1.0 / s;
					xc = (lat - T->segment[seg]->coord[GMT_Y][j0] + s * T->segment[seg]->coord[GMT_X][j0] - s_inv * lon ) / (s - s_inv);
					yc = T->segment[seg]->coord[GMT_Y][j0] + s * (xc - T->segment[seg]->coord[GMT_X][j0]);

				}
				/* To be inside, (xc, yc) must (1) be on the line segment and not its extension and (2) be within dist of our point */

				if (T->segment[seg]->coord[GMT_X][j0] < xc && T->segment[seg]->coord[GMT_X][j1] < xc ) continue;	/* Cross point is on extension */
				if (T->segment[seg]->coord[GMT_X][j0] > xc && T->segment[seg]->coord[GMT_X][j1] > xc ) continue;	/* Cross point is on extension */
			}

			/* OK, here we must check how close the crossing point is */

			d = (*GMT_distance_func) (lon, lat, xc, yc);			/* Distance between our point and intersection */
			if (return_mindist && d < (*dist_min)) {			/* Update min distance */
				*dist_min = d;
				if (return_mindist == 2) *x_near = xc, *y_near = yc;	/* Also update nearest point on the line */
				if (return_mindist == 3) {	/* Instead update (seg, pt) of nearest point on the line */
					*x_near = (double)seg;
					dist_AB = (*GMT_distance_func) (T->segment[seg]->coord[GMT_X][j0], T->segment[seg]->coord[GMT_Y][j0], T->segment[seg]->coord[GMT_X][j1], T->segment[seg]->coord[GMT_Y][j1]);
					fraction = (dist_AB > 0.0) ? (*GMT_distance_func) (T->segment[seg]->coord[GMT_X][j0], T->segment[seg]->coord[GMT_Y][j0], xc, yc) / dist_AB : 0.0;
					*y_near = (double)j0 + fraction;
				}
			}
			if (d <= T->segment[seg]->dist) return (TRUE);		/* Node inside the critical distance; we are done */
		}
	}
	return (FALSE);	/* All tests failed, we are not close to the line(s) */
}

int GMT_near_a_line_spherical (double lon, double lat, struct GMT_TABLE *T, BOOLEAN return_mindist, double *dist_min, double *x_near, double *y_near)
{
	int seg, row, j0;
	double d, A[3], B[3], C[3], X[3], plon, plat, xlon, xlat, cx_dist, cos_dist, dist_AB, fraction;
	BOOLEAN perpendicular_only = FALSE, interior;

	/* return_mindist is 1, 2, or 3.  All return the minimum distance. 2 also returns the coordinate of nearest point,
	   3 instead returns segment number and point number (fractional) of that point.  If 10 is added we exclude point
	   that are withing distance of the linesegment endpoints but project onto the extension of the line.  */

	if (return_mindist >= 10) {	/* Exclude circular region surrounding line endpoints */
		perpendicular_only = TRUE;
		return_mindist -= 10;
	}
	plon = lon;	plat = lat;
	GMT_geo_to_cart (&plat, &plon, C, TRUE);	/* Our point to test is now C */
	if (return_mindist) *dist_min = DBL_MAX;

	for (seg = 0; seg < T->n_segments; seg++) {	/* Loop over each line segment */

		if (T->segment[seg]->n_rows <= 0) continue;	/* Empty ; skip */

		/* Find nearest point on this line */

		if (return_mindist) T->segment[seg]->dist = 0.0;	/* Explicitly set dist to zero so the shortest distance can be found */

		for (row = 0; row < T->segment[seg]->n_rows; row++) {	/* loop over nodes on current line */
			d = (*GMT_distance_func) (lon, lat, T->segment[seg]->coord[GMT_X][row], T->segment[seg]->coord[GMT_Y][row]);	/* Distance between our point and row'th node on seg'th line */
			if (return_mindist && d < (*dist_min)) {		/* Update minimum distance */
				*dist_min = d;
				if (return_mindist == 2) *x_near = T->segment[seg]->coord[GMT_X][row], *y_near = T->segment[seg]->coord[GMT_Y][row];	/* Also update (x,y) of nearest point on the line */
				if (return_mindist == 3) *x_near = (double)seg, *y_near = (double)row;	/* Also update (seg, pt) of nearest point on the line */
			}
			interior = (row > 0 && row < (T->segment[seg]->n_rows - 1));	/* Only FALSE if we are processing one of the end points */
			if (d <= T->segment[seg]->dist && (interior || !perpendicular_only)) return (TRUE);			/* Node inside the critical distance; we are done */
		}

		if (T->segment[seg]->n_rows < 2) continue;	/* 1-point "line" is a point; skip segment check */

		/* If we get here we must check for intermediate points along the great circle lines between segment nodes.*/

		cos_dist = (return_mindist) ? 2.0 : cosd (T->segment[seg]->dist * KM_TO_DEG);		/* Cosine of the great circle distance we are checking for. 2 ensures failure to be closer */
		plon = T->segment[seg]->coord[GMT_X][0];	plat = T->segment[seg]->coord[GMT_Y][0];
		GMT_geo_to_cart (&plat, &plon, B, TRUE);		/* 3-D vector of end of last segment */

		for (row = 1; row < T->segment[seg]->n_rows; row++) {				/* loop over great circle segments on current line */
			memcpy ((void *)A, (void *)B, (size_t)(3 * sizeof (double)));	/* End of last segment is start of new segment */
			plon = T->segment[seg]->coord[GMT_X][row];	plat = T->segment[seg]->coord[GMT_Y][row];
			GMT_geo_to_cart (&plat, &plon, B, TRUE);	/* 3-D vector of end of this segment */
			if (GMT_great_circle_intersection (A, B, C, X, &cx_dist)) continue;	/* X not between A and B */
			if (return_mindist) {		/* Get lon, lat of X, calculate distance, and update min_dist if needed */
				GMT_cart_to_geo (&xlat, &xlon, X, TRUE);
				d = (*GMT_distance_func) (xlon, xlat, lon, lat);	/* Distance between our point and row'th node on seg'th line */
				if (d < (*dist_min)) {
					*dist_min = d;				/* Update minimum distance */
					if (return_mindist == 2) *x_near = xlon, *y_near = xlat;	/* Also update (x,y) of nearest point on the line */
					if (return_mindist == 3) {	/* Also update (seg, pt) of nearest point on the line */
						*x_near = (double)seg;
						j0 = row - 1;
						dist_AB = (*GMT_distance_func) (T->segment[seg]->coord[GMT_X][j0], T->segment[seg]->coord[GMT_Y][j0], T->segment[seg]->coord[GMT_X][row], T->segment[seg]->coord[GMT_Y][row]);
						fraction = (dist_AB > 0.0) ? (*GMT_distance_func) (T->segment[seg]->coord[GMT_X][j0], T->segment[seg]->coord[GMT_Y][j0], xlon, xlat) / dist_AB : 0.0;
						*y_near = (double)j0 + fraction;
					}
				}
			}
			if (cx_dist >= cos_dist) return (TRUE);	/* X is on the A-B extension AND within specified distance */
		}
	}
	return (FALSE);	/* All tests failed, we are not close to the line(s) */
}

void GMT_rotate2D (double x[], double y[], int n, double x0, double y0, double angle, double xp[], double yp[])
{	/* Cartesian rotation of x,y in the plane by angle followed by translation by (x0, y0) */
	int i;
	double s, c;

	sincosd (angle, &s, &c);
	for (i = 0; i < n; i++) {	/* Coordinate transformation: Rotate and add new (x0, y0) offset */
		xp[i] = x0 + x[i] * c - y[i] * s;
		yp[i] = y0 + x[i] * s + y[i] * c;
	}
}

int GMT_get_arc (double x0, double y0, double r, double dir1, double dir2, double **x, double **y)
{
	/* Create an array with a circular arc. r in inches, angles in degrees */

	int i, n;
	double da, s, c, *xx, *yy;

	n = irint (D2R * fabs (dir2 - dir1) * r / gmtdefs.line_step);
	xx = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), GMT_program);
	yy = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), GMT_program);
	da = (dir2 - dir1) / (n - 1);
	for (i = 0; i < n; i++) {
		sincosd (dir1 + i * da, &s, &c);
		xx[i] = x0 + r * c;
		yy[i] = y0 + r * s;
	}
	*x = xx;
	*y = yy;

	return (n);
}

/* Here lies GMT Crossover core functions that previously was in X2SYS only */

int GMT_init_track (double y[], int n, struct GMT_XSEGMENT **S)
{
	/* GMT_init_track accepts the y components of an x-y track of length n and returns an array of
	 * line segments that have been sorted on the minimum y-coordinate
	 */

	int a, b;
	size_t nl = n - 1;
	struct GMT_XSEGMENT *L;
	int GMT_ysort (const void *p1, const void *p2);

	if (nl <= 0) {
		fprintf (stderr, "GMT: ERROR in GMT_init_track; nl = %d\n", (int)nl);
		GMT_exit (EXIT_FAILURE);
	}

	L = (struct GMT_XSEGMENT *) GMT_memory (VNULL, nl, sizeof (struct GMT_XSEGMENT), "GMT_init_track");

	for (a = 0, b = 1; b < n; a++, b++) {
		if (y[b] < y[a]) {
			L[a].start = b;
			L[a].stop = a;
		}
		else {
			L[a].start = a;
			L[a].stop = b;
		}
	}

	/* Sort on minimum y-coordinate, if tie then on 2nd coordinate */

	GMT_x2sys_Y = y;	/* Sort routine needs this pointer */

	qsort ((void *)L, nl, sizeof (struct GMT_XSEGMENT), GMT_ysort);

	GMT_x2sys_Y = (double *)NULL;

	*S = L;

	return (GMT_NOERROR);
}

int GMT_ysort (const void *p1, const void *p2)
{
	/* The double pointer GMT_x2sys_Y must be set to point to the relevant y-array
	 * before this call!!! */

	struct GMT_XSEGMENT *a, *b;

	a = (struct GMT_XSEGMENT *)p1;
	b = (struct GMT_XSEGMENT *)p2;

	if (GMT_x2sys_Y[a->start] < GMT_x2sys_Y[b->start]) return -1;
	if (GMT_x2sys_Y[a->start] > GMT_x2sys_Y[b->start]) return  1;

	/* Here they have the same low y-value, now sort on other y value */

	if (GMT_x2sys_Y[a->stop] < GMT_x2sys_Y[b->stop]) return -1;
	if (GMT_x2sys_Y[a->stop] > GMT_x2sys_Y[b->stop]) return  1;

	/* Identical */

	return (0);
}

int GMT_crossover (double xa[], double ya[], int *sa0, struct GMT_XSEGMENT A[], int na, double xb[], double yb[], int *sb0, struct GMT_XSEGMENT B[], int nb, BOOLEAN internal, struct GMT_XOVER *X)
{
	int this_a, this_b, n_seg_a;
	int n_seg_b, nx, xa_start = 0, xa_stop = 0, xb_start = 0, xb_stop = 0, ta_start = 0, ta_stop = 0, tb_start, tb_stop;
	int *sa, *sb, nx_alloc;
	BOOLEAN new_a, new_b, new_a_time = FALSE, xa_OK = FALSE, xb_OK = FALSE;
	double del_xa, del_xb, del_ya, del_yb, i_del_xa, i_del_xb, i_del_ya, i_del_yb, slp_a, slp_b, xc, yc, tx_a, tx_b;

	if (na < 2 || nb < 2) return (0);	/* Need at least 2 points to make a segment */

	this_a = this_b = nx = 0;
	new_a = new_b = TRUE;
	nx_alloc = GMT_SMALL_CHUNK;

	n_seg_a = na - 1;
	n_seg_b = nb - 1;

	/* Assign pointers to segment info given, or initialize zero arrays if not given */
	sa = (sa0) ? sa0 : (int *) GMT_memory (VNULL, (size_t)na, sizeof (int), "GMT_crossover");
	sb = (sb0) ? sb0 : (int *) GMT_memory (VNULL, (size_t)nb, sizeof (int), "GMT_crossover");

	GMT_x_alloc (X, -nx_alloc);

	while (this_a < n_seg_a && yb[B[this_b].start] > ya[A[this_a].stop]) this_a++;	/* Go to first possible A segment */

	while (this_a < n_seg_a) {

		/* First check for internal neighboring segments which cannot cross */

		if (internal && (this_a == this_b || (A[this_a].stop == B[this_b].start || A[this_a].start == B[this_b].stop) || (A[this_a].start == B[this_b].start || A[this_a].stop == B[this_b].stop))) {	/* Neighboring segments cannot cross */
			this_b++;
			new_b = TRUE;
		}
		else if (yb[B[this_b].start] > ya[A[this_a].stop]) {	/* Reset this_b and go to next A */
			this_b = n_seg_b;
		}
		else if (yb[B[this_b].stop] < ya[A[this_a].start]) {	/* Must advance B in y-direction */
			this_b++;
			new_b = TRUE;
		}
		else if (sb[B[this_b].stop] != sb[B[this_b].start]) {	/* Must advance B in y-direction since this segment crosses multiseg boundary*/
			this_b++;
			new_b = TRUE;
		}
		else {	/* Current A and B segments overlap in y-range */

			if (new_a) {	/* Must sort this A new segment in x */
				if (xa[A[this_a].stop] < xa[A[this_a].start]) {
					xa_start = A[this_a].stop;
					xa_stop  = A[this_a].start;
				}
				else {
					xa_start = A[this_a].start;
					xa_stop  = A[this_a].stop;
				}
				new_a = FALSE;
				new_a_time = TRUE;
				xa_OK = (sa[xa_start] == sa[xa_stop]);	/* FALSE if we cross between multiple segments */
			}

			if (new_b) {	/* Must sort this new B segment in x */
				if (xb[B[this_b].stop] < xb[B[this_b].start]) {
					xb_start = B[this_b].stop;
					xb_stop  = B[this_b].start;
				}
				else {
					xb_start = B[this_b].start;
					xb_stop  = B[this_b].stop;
				}
				new_b = FALSE;
				xb_OK = (sb[xb_start] == sb[xb_stop]);	/* FALSE if we cross between multiple segments */
			}

			/* OK, first check for any overlap in x range */

			if (xa_OK && xb_OK && !((xa[xa_stop] < xb[xb_start]) || (xa[xa_start] > xb[xb_stop]))) {

				/* We have segment overlap in x.  Now check if the segments cross  */

				del_xa = xa[xa_stop] - xa[xa_start];
				del_xb = xb[xb_stop] - xb[xb_start];
				del_ya = ya[xa_stop] - ya[xa_start];
				del_yb = yb[xb_stop] - yb[xb_start];

				if (del_xa == 0.0) {	/* Vertical A segment: Special case */
					if (del_xb == 0.0) {	/* Two vertical segments with some overlap */
						double y4[4];
						/* Assign as crossover the middle of the overlapping segments */
						X->x[nx] = xa[xa_start];
						y4[0] = ya[xa_start];	y4[1] = ya[xa_stop];	y4[2] = yb[xb_start];	y4[3] = yb[xb_stop];
						qsort ((void *)y4, (size_t)4, sizeof (double), GMT_comp_double_asc);
						if (y4[1] != y4[2]) {
							X->y[nx] = 0.5 * (y4[1] + y4[2]);
							X->xnode[0][nx] = 0.5 * (xa_start + xa_stop);
							X->xnode[1][nx] = 0.5 * (xb_start + xb_stop);
							nx++;
						}
					}
					else {
						i_del_xb = 1.0 / del_xb;
						yc = yb[xb_start] + (xa[xa_start] - xb[xb_start]) * del_yb * i_del_xb;
						if (!(yc < ya[A[this_a].start] || yc > ya[A[this_a].stop])) {	/* Did cross within the segment extents */
							/* Only accept xover if occurring before segment end (in time) */

							if (xb_start < xb_stop) {
								tb_start = xb_start;	/* B Node first in time */
								tb_stop = xb_stop;	/* B Node last in time */
							}
							else {
								tb_start = xb_stop;	/* B Node first in time */
								tb_stop = xb_start;	/* B Node last in time */
							}
							if (new_a_time) {
								if (xa_start < xa_stop) {
									ta_start = xa_start;	/* A Node first in time */
									ta_stop = xa_stop;	/* A Node last in time */
								}
								else {
									ta_start = xa_stop;	/* A Node first in time */
									ta_stop = xa_start;	/* A Node last in time */
								}
								new_a_time = FALSE;
							}

							tx_a = ta_start + fabs ((yc - ya[ta_start]) / del_ya);
							tx_b = tb_start + fabs (xa[xa_start] - xb[tb_start]) * i_del_xb;
							if (tx_a < ta_stop && tx_b < tb_stop) {
								X->x[nx] = xa[xa_start];
								X->y[nx] = yc;
								X->xnode[0][nx] = tx_a;
								X->xnode[1][nx] = tx_b;
								nx++;
							}
						}
					}
				}
				else if (del_xb == 0.0) {	/* Vertical B segment: Special case */

					i_del_xa = 1.0 / del_xa;
					yc = ya[xa_start] + (xb[xb_start] - xa[xa_start]) * del_ya * i_del_xa;
					if (!(yc < yb[B[this_b].start] || yc > yb[B[this_b].stop])) {	/* Did cross within the segment extents */
						/* Only accept xover if occurring before segment end (in time) */

						if (xb_start < xb_stop) {
							tb_start = xb_start;	/* B Node first in time */
							tb_stop = xb_stop;	/* B Node last in time */
						}
						else {
							tb_start = xb_stop;	/* B Node first in time */
							tb_stop = xb_start;	/* B Node last in time */
						}
						if (new_a_time) {
							if (xa_start < xa_stop) {
								ta_start = xa_start;	/* A Node first in time */
								ta_stop = xa_stop;	/* A Node last in time */
							}
							else {
								ta_start = xa_stop;	/* A Node first in time */
								ta_stop = xa_start;	/* A Node last in time */
							}
							new_a_time = FALSE;
						}

						tx_a = ta_start + fabs (xb[xb_start] - xa[ta_start]) * i_del_xa;
						tx_b = tb_start + fabs ((yc - yb[tb_start]) / del_yb);
						if (tx_a < ta_stop && tx_b < tb_stop) {
							X->x[nx] = xb[xb_start];
							X->y[nx] = yc;
							X->xnode[0][nx] = tx_a;
							X->xnode[1][nx] = tx_b;
							nx++;
						}
					}
				}
				else if (del_ya == 0.0) {	/* Horizontal A segment: Special case */

					if (del_yb == 0.0) {	/* Two horizontal segments with some overlap */
						double x4[4];
						/* Assign as crossover the middle of the overlapping segments */
						X->y[nx] = ya[xa_start];
						x4[0] = xa[xa_start];	x4[1] = xa[xa_stop];	x4[2] = xb[xb_start];	x4[3] = xb[xb_stop];
						qsort ((void *)x4, (size_t)4, sizeof (double), GMT_comp_double_asc);
						if (x4[1] != x4[2]) {
							X->x[nx] = 0.5 * (x4[1] + x4[2]);
							X->xnode[0][nx] = 0.5 * (xa_start + xa_stop);
							X->xnode[1][nx] = 0.5 * (xb_start + xb_stop);
							nx++;
						}
					}
					else {
						i_del_yb = 1.0 / del_yb;
						xc = xb[xb_start] + (ya[xa_start] - yb[xb_start]) * del_xb * i_del_yb;
						if (!(xc < xa[xa_start] || xc > xa[xa_stop])) {	/* Did cross within the segment extents */

							/* Only accept xover if occurring before segment end (in time) */

							if (xb_start < xb_stop) {
								tb_start = xb_start;	/* B Node first in time */
								tb_stop = xb_stop;	/* B Node last in time */
							}
							else {
								tb_start = xb_stop;	/* B Node first in time */
								tb_stop = xb_start;	/* B Node last in time */
							}
							if (new_a_time) {
								if (xa_start < xa_stop) {
									ta_start = xa_start;	/* A Node first in time */
									ta_stop = xa_stop;	/* A Node last in time */
								}
								else {
									ta_start = xa_stop;	/* A Node first in time */
									ta_stop = xa_start;	/* A Node last in time */
								}
								new_a_time = FALSE;
							}

							tx_a = ta_start + fabs (xc - xa[ta_start]) / del_xa;
							tx_b = tb_start + fabs ((ya[xa_start] - yb[tb_start]) * i_del_yb);
							if (tx_a < ta_stop && tx_b < tb_stop) {
								X->y[nx] = ya[xa_start];
								X->x[nx] = xc;
								X->xnode[0][nx] = tx_a;
								X->xnode[1][nx] = tx_b;
								nx++;
							}
						}
					}
				}
				else if (del_yb == 0.0) {	/* Horizontal B segment: Special case */

					i_del_ya = 1.0 / del_ya;
					xc = xa[xa_start] + (yb[xb_start] - ya[xa_start]) * del_xa * i_del_ya;
					if (!(xc < xb[xb_start] || xc > xb[xb_stop])) {	/* Did cross within the segment extents */

						/* Only accept xover if occurring before segment end (in time) */

						if (xb_start < xb_stop) {
							tb_start = xb_start;	/* B Node first in time */
							tb_stop = xb_stop;	/* B Node last in time */
						}
						else {
							tb_start = xb_stop;	/* B Node first in time */
							tb_stop = xb_start;	/* B Node last in time */
						}
						if (new_a_time) {
							if (xa_start < xa_stop) {
								ta_start = xa_start;	/* A Node first in time */
								ta_stop = xa_stop;	/* A Node last in time */
							}
							else {
								ta_start = xa_stop;	/* A Node first in time */
								ta_stop = xa_start;	/* A Node last in time */
							}
							new_a_time = FALSE;
						}

						tx_a = ta_start + fabs ((yb[xb_start] - ya[ta_start]) * i_del_ya);
						tx_b = tb_start + fabs (xc - xb[tb_start]) / del_xb;
						if (tx_a < ta_stop && tx_b < tb_stop) {
							X->y[nx] = yb[xb_start];
							X->x[nx] = xc;
							X->xnode[0][nx] = tx_a;
							X->xnode[1][nx] = tx_b;
							nx++;
						}
					}
				}
				else {	/* General case */

					i_del_xa = 1.0 / del_xa;
					i_del_xb = 1.0 / del_xb;
					slp_a = del_ya * i_del_xa;
					slp_b = del_yb * i_del_xb;
					if (slp_a == slp_b) {	/* Segments are parallel */
						double x4[4];
						/* Assign as possible crossover the middle of the overlapping segments */
						x4[0] = xa[xa_start];	x4[1] = xa[xa_stop];	x4[2] = xb[xb_start];	x4[3] = xb[xb_stop];
						qsort ((void *)x4, (size_t)4, sizeof (double), GMT_comp_double_asc);
						if (x4[1] != x4[2]) {
							xc = 0.5 * (x4[1] + x4[2]);
							yc = slp_a * (xc - xa[xa_start]) + ya[xa_start];
							if ((slp_b * (xc - xb[xb_start]) + yb[xb_start]) == yc) {
								X->y[nx] = yc;
								X->x[nx] = xc;
								X->xnode[0][nx] = 0.5 * (xa_start + xa_stop);
								X->xnode[1][nx] = 0.5 * (xb_start + xb_stop);
								nx++;
							}
						}
					}
					else {	/* Segments are not parallel */
						xc = (yb[xb_start] - ya[xa_start] + slp_a * xa[xa_start] - slp_b * xb[xb_start]) / (slp_a - slp_b);
						if (!(xc < xa[xa_start] || xc > xa[xa_stop] || xc < xb[xb_start] || xc > xb[xb_stop])) {	/* Did cross within the segment extents */

							/* Only accept xover if occurring before segment end (in time) */

							if (xb_start < xb_stop) {
								tb_start = xb_start;	/* B Node first in time */
								tb_stop = xb_stop;	/* B Node last in time */
							}
							else {
								tb_start = xb_stop;	/* B Node first in time */
								tb_stop = xb_start;	/* B Node last in time */
							}
							if (new_a_time) {
								if (xa_start < xa_stop) {
									ta_start = xa_start;	/* A Node first in time */
									ta_stop = xa_stop;	/* A Node last in time */
								}
								else {
									ta_start = xa_stop;	/* A Node first in time */
									ta_stop = xa_start;	/* A Node last in time */
								}
								new_a_time = FALSE;
							}

							tx_a = ta_start + fabs (xc - xa[ta_start]) * i_del_xa;
							tx_b = tb_start + fabs (xc - xb[tb_start]) * i_del_xb;
							if (tx_a < ta_stop && tx_b < tb_stop) {
								X->x[nx] = xc;
								X->y[nx] = ya[xa_start] + (xc - xa[xa_start]) * slp_a;
								X->xnode[0][nx] = tx_a;
								X->xnode[1][nx] = tx_b;
								nx++;
							}
						}
					}
				}

				if (nx == nx_alloc) {
					nx_alloc += GMT_SMALL_CHUNK;
					GMT_x_alloc (X, nx_alloc);
				}
			} /* End x-overlap */

			this_b++;
			new_b = TRUE;

		} /* End y-overlap */

		if (this_b == n_seg_b) {
			this_a++;
			this_b = (internal) ? this_a : 0;
			new_a = new_b = TRUE;
		}

	} /* End while loop */

	if (!sa0) GMT_free ((void *)sa);
	if (!sb0) GMT_free ((void *)sb);

	return (nx);
}

void GMT_x_alloc (struct GMT_XOVER *X, int nx_alloc) {
	if (nx_alloc < 0) {	/* Initial allocation */
		nx_alloc = -nx_alloc;
		X->x = (double *) GMT_memory (VNULL, (size_t)nx_alloc, sizeof (double), "GMT_x_alloc");
		X->y = (double *) GMT_memory (VNULL, (size_t)nx_alloc, sizeof (double), "GMT_x_alloc");
		X->xnode[0] = (double *) GMT_memory (VNULL, (size_t)nx_alloc, sizeof (double), "GMT_x_alloc");
		X->xnode[1] = (double *) GMT_memory (VNULL, (size_t)nx_alloc, sizeof (double), "GMT_x_alloc");
	}
	else {	/* Increment */
		X->x = (double *) GMT_memory ((void *)X->x, (size_t)nx_alloc, sizeof (double), "GMT_x_alloc");
		X->y = (double *) GMT_memory ((void *)X->y, (size_t)nx_alloc, sizeof (double), "GMT_x_alloc");
		X->xnode[0] = (double *) GMT_memory ((void *)X->xnode[0], (size_t)nx_alloc, sizeof (double), "GMT_x_alloc");
		X->xnode[1] = (double *) GMT_memory ((void *)X->xnode[1], (size_t)nx_alloc, sizeof (double), "GMT_x_alloc");
	}
}

void GMT_x_free (struct GMT_XOVER *X) {
	GMT_free ((void *)X->x);
	GMT_free ((void *)X->y);
	GMT_free ((void *)X->xnode[0]);
	GMT_free ((void *)X->xnode[1]);
}

int GMT_get_dist_scale (char c, double *d_scale, int *proj_type, PFD *distance_func)
{
	int error = 0;

	*distance_func = (GMT_IS_SPHERICAL) ? GMT_great_circle_dist : GMT_geodesic_dist_meter;
	switch (c) {
		case '\0':	/* Spherical m along great circle */
		case 'e':
			*distance_func = GMT_great_circle_dist;
			*d_scale = project_info.M_PR_DEG;
			break;
		case 'E':	/* m along geodesic */
			*distance_func = (GMT_IS_SPHERICAL) ? GMT_great_circle_dist : GMT_geodesic_dist_meter;
			*d_scale = (GMT_IS_SPHERICAL) ? project_info.M_PR_DEG : 1.0;
			break;
		case 'k':	/* km along great circle */
			*distance_func = GMT_great_circle_dist;
			*d_scale = project_info.KM_PR_DEG;
			break;
		case 'K':	/* km along geodesic */
			*distance_func = (GMT_IS_SPHERICAL) ? GMT_great_circle_dist : GMT_geodesic_dist_meter;
			*d_scale = (GMT_IS_SPHERICAL) ? project_info.KM_PR_DEG : 0.001;
			break;
		case 'm':	/* Miles along great circle */
			*distance_func = GMT_great_circle_dist;
			*d_scale = project_info.M_PR_DEG / 1609.334;
			break;
		case 'M':	/* Miles along geodesic */
			*distance_func = (GMT_IS_SPHERICAL) ? GMT_great_circle_dist : GMT_geodesic_dist_meter;
			*d_scale = ((GMT_IS_SPHERICAL) ? project_info.M_PR_DEG : 1.0) / 1609.334;
			break;
		case 'n':	/* Nautical miles along great circle */
			*distance_func = GMT_great_circle_dist;
			*d_scale = project_info.M_PR_DEG / 1852.0;
			break;
		case 'N':	/* Nautical miles along geodesic */
			*distance_func = (GMT_IS_SPHERICAL) ? GMT_great_circle_dist : GMT_geodesic_dist_meter;
			*d_scale = ((GMT_IS_SPHERICAL) ? project_info.M_PR_DEG : 1.0) / 1852.0;
			break;
		case 'C':	/* Cartesian distances in projected units */
			*d_scale = 1.0;
			*proj_type = 2;
			break;
		case 'c':	/* Cartesian distances in user units */
			*d_scale = 1.0;
			*proj_type = 1;
			break;
		case 'd':	/* Degrees along great circle */
			*distance_func = GMT_great_circle_dist;
			*d_scale = 1.0;
			break;
		case 'D':	/* Degrees along geodesic */
			*d_scale = 1.0;
			*distance_func = (GMT_IS_SPHERICAL) ? GMT_great_circle_dist : GMT_geodesic_dist_degree;
			break;
		default:
			fprintf (stderr, "%s: GMT SYNTAX ERROR -G.  Units must be one of k|m|n|c|C|d\n", GMT_program);
			error++;
			break;
	}
	return (error);
}

int GMT_linear_array (double min, double max, double delta, double phase, double **array)
{
	double first, small, *val;
	int i, n;

	if (delta <= 0.0) return (0);
	small = GMT_SMALL * delta;
	first = floor ((min - delta - phase) / delta) * delta + phase;
	while ((min - first) > small) first += delta;
	if (first > max) return (0);

	n = irint ((max - first) / delta) + 1;
	val = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "GMT_linear_array");
	for (i = 0; i < n; i++) {
		val[i] = first + i * delta;
		if (fabs(val[i] - phase) < small) val[i] = phase;	/* Kill small numbers when phase==0 */
	}
	while (n && (val[n-1] - small) > max) n--;	/* In case of over-run */

	*array = val;

	return (n);
}

int GMT_log_array (double min, double max, double delta, double **array)
{
	int i, n, nticks, test, n_alloc = GMT_SMALL_CHUNK, start_log;
	double *val, log10_min, log10_max, tvals[9];

	/* Because min and max may be tiny values (e.g., 10^-20) we must do all calculations on the log10 (value) */

	if (delta <= 0.0) return (0);
	val = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), "GMT_log_array");

	test = irint (fabs (delta)) - 1;
	if (test == 1) {
		tvals[0] = 0.0;	/* = log10 (1.0) */
		tvals[1] = log10 (2.0);
		tvals[2] = log10 (5.0);
		nticks = 3;
	}
	else if (test == 2) {
		nticks = 9;
		for (i = 0; i < nticks; i++) tvals[i] = log10 ((double)(i + 1));
	}
	else {
		tvals[0] = 0.0;
		nticks = 1;
		test = 0;
	}

	log10_min = d_log10 (min);
	log10_max = d_log10 (max);
	start_log = irint (floor (log10_min));
	val[0] = (double)start_log;
	i = 1;	/* Because val[0] is initially set to be a power or ten (i = 0), so next should be 1 */
	while ((log10_min - val[0]) > GMT_SMALL) {
		if (i < nticks)
			val[0] = start_log + tvals[i];
		else {
			val[0] = ++start_log;
			i = 0;
		}
		i++;
	}
	i--;

	n = 0;
	while ((log10_max - val[n]) > GMT_CONV_LIMIT) {
		i++;
		n++;
		if (n == n_alloc) {
			n_alloc += GMT_SMALL_CHUNK;
			val = (double *) GMT_memory ((void *)val, (size_t)n_alloc, sizeof (double), "GMT_log_array");
		}

		if (i < nticks)
			val[n] = start_log + tvals[i];
		else {
			val[n] = ++start_log;
			i = 0;
		}
	}
	while (n && val[n] > log10_max) n--;	/* In case of over-run */
	n++;

	for (i = 0; i < n; i++) val[i] = pow (10.0, val[i]);	/* Convert from log10 to values */

	val = (double *) GMT_memory ((void *)val, (size_t)n, sizeof (double), "GMT_log_array");

	*array = val;

	return (n);
}

int GMT_pow_array (double min, double max, double delta, int x_or_y, double **array)
{
	int annottype, n, n_alloc = GMT_SMALL_CHUNK;
	double *val, tval, v0, v1, small, start_val, end_val;
	PFI fwd, inv;

	if (delta <= 0.0) return (0);
	val = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), "GMT_pow_array");

	annottype = (frame_info.axis[x_or_y].type == 2) ? 2 : 0;
	if (x_or_y == 0) { /* x-axis */
		fwd = GMT_x_forward;
		inv = GMT_x_inverse;
	}
	else {	/* y-axis */
		fwd = GMT_y_forward;
		inv = GMT_y_inverse;
	}

	small = GMT_SMALL * delta;
	if (annottype == 2) {
		(*fwd) (min, &v0);
		(*fwd) (max, &v1);

		tval = (delta <= 0.0) ? 0.0 : floor (v0 / delta) * delta;
		if (fabs (tval - v0) > small) tval += delta;
		start_val = tval;
		tval = (delta <= 0.0) ? 0.0 : ceil (v1 / delta) * delta;
		if (fabs (tval - v1) > small) tval -= delta;
		end_val = tval;
	}
	else {
		tval = (delta <= 0.0) ? 0.0 : floor (min / delta) * delta;
		if (fabs (tval - min) > small) tval += delta;
		start_val = tval;
		tval = (delta <= 0.0) ? 0.0 : ceil (max / delta) * delta;
		if (fabs (tval - max) > small) tval -= delta;
		end_val = tval;
	}

	tval = start_val;
	n = 0;
	while (tval <= end_val) {
		if (annottype == 2) {
			(*inv) (&val[n], tval);
		}
		else {
			val[n] = tval;
		}
		tval += delta;
		n++;
		if (n == n_alloc) {
			n_alloc += GMT_SMALL_CHUNK;
			val = (double *) GMT_memory ((void *)val, (size_t)n_alloc, sizeof (double), "GMT_pow_array");
		}
	}
	if (annottype == 2) {
		(*inv) (&tval, max);
		while (n && val[n-1] > tval) n--;	/* In case of over-run */
	}
	else {
		while (n && val[n-1] > end_val) n--;	/* In case of over-run */
	}

	val = (double *) GMT_memory ((void *)val, (size_t)n, sizeof (double), "GMT_log_array");

	*array = val;

	return (n);
}

int GMT_time_array (double min, double max, struct GMT_PLOT_AXIS_ITEM *T, double **array)
{	/* When interval is TRUE we must return interval start/stop even if outside min/max range */
	struct GMT_MOMENT_INTERVAL I;
	double *val;
	int n_alloc = GMT_SMALL_CHUNK, n = 0;
	BOOLEAN interval;

	if (T->interval <= 0.0) return (0);
	val = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), "GMT_time_array");
	I.unit = T->unit;
	I.step = (int)T->interval;
	interval = (T->id == 2 || T->id == 3);	/* Only for I/i axis items */
	GMT_moment_interval (&I, min, TRUE);	/* First time we pass TRUE for initialization */
	while (I.dt[0] <= max) {		/* As long as we are not gone way past the end time */
		if (I.dt[0] >= min || interval) val[n++] = I.dt[0];		/* Was inside region */
		GMT_moment_interval (&I, 0.0, FALSE);			/* Advance to next interval */
		if (n == n_alloc) {					/* Allocate more space */
			n_alloc += GMT_SMALL_CHUNK;
			val = (double *) GMT_memory ((void *)val, (size_t)n_alloc, sizeof (double), "GMT_time_array");
		}
	}
	if (interval) val[n++] = I.dt[0];	/* Must get end of interval too */
	val = (double *) GMT_memory ((void *)val, (size_t)n, sizeof (double), "GMT_time_array");

	*array = val;

	return (n);
}

int GMT_coordinate_array (double min, double max, struct GMT_PLOT_AXIS_ITEM *T, double **array)
{
	int n;
	switch (project_info.xyz_projection[T->parent]) {
		case GMT_LINEAR:
			n = GMT_linear_array (min, max, GMT_get_map_interval (T->parent, T->id), frame_info.axis[T->parent].phase, array);
			break;
		case GMT_LOG10:
			n = GMT_log_array (min, max, GMT_get_map_interval (T->parent, T->id), array);
			break;
		case GMT_POW:
			n = GMT_pow_array (min, max, GMT_get_map_interval (T->parent, T->id), T->parent, array);
			break;
		case GMT_TIME:
			n = GMT_time_array (min, max, T, array);
			break;
		default:
			fprintf (stderr, "GMT ERROR: Invalid projection type (%d) passed to GMT_coordinate_array!\n", project_info.xyz_projection[T->parent]);
			GMT_exit (EXIT_FAILURE);
			break;
	}
	return (n);
}

void GMT_get_primary_annot (struct GMT_PLOT_AXIS *A, int *primary, int *secondary)
{	/* Return the primary and secondary annotation item numbers [== 1 if there are no unit set ]*/

	int i, no[2] = {GMT_ANNOT_UPPER, GMT_ANNOT_LOWER};
	double val[2], s;

	for (i = 0; i < 2; i++) {
		switch (A->item[no[i]].unit) {
			case 'Y':
			case 'y':
				s = GMT_DAY2SEC_F * 365.25;
				break;
			case 'O':
			case 'o':
				s = GMT_DAY2SEC_F * 30.5;
				break;
			case 'U':
			case 'u':
				s = GMT_DAY2SEC_F * 7.0;
				break;
			case 'K':
			case 'k':
			case 'D':
			case 'd':
				s = GMT_DAY2SEC_F;
				break;
			case 'H':
			case 'h':
				s = GMT_HR2SEC_F;
				break;
			case 'M':
			case 'm':
				s = GMT_MIN2SEC_F;
				break;
			case 'C':
			case 'c':
				s = 1.0;
				break;
			default:
				/* No unit specified - probably not a time axis */
				s = 1.0;
				break;
		}
		val[i] = A->item[no[i]].interval * s;
	}
	if (val[0] > val[1]) {
		*primary = GMT_ANNOT_UPPER;
		*secondary = GMT_ANNOT_LOWER;
	}
	else {
		*primary = GMT_ANNOT_LOWER;
		*secondary = GMT_ANNOT_UPPER;
	}
}

BOOLEAN GMT_skip_second_annot (int item, double x, double x2[], int n, int primary, int secondary)
{
	int i;
	double small;
	BOOLEAN found;

	if (primary == secondary) return (FALSE);	/* Not set, no need to skip */
	if (secondary != item) return (FALSE);		/* Not working on secondary annotation */
	if (!x2) return (FALSE);			/* None given */

	small = (x2[1] - x2[0]) * GMT_CONV_LIMIT;
	for (i = 0, found = FALSE; !found && i < n; i++) found = (fabs (x2[i] - x) < small);
	return (found);
}

int GMT_annot_pos (double min, double max, struct GMT_PLOT_AXIS_ITEM *T, double coord[], double *pos)
{
	/* Calculates the location of the next annotation in user units.  This is
	 * trivial for tick annotations but can be tricky for interval annotations
	 * since the annotation location is not necessarily centered on the interval.
	 * For instance, if our interval is 3 months we do not want "January" centered
	 * on that quarter.  If the position is outside our range we return TRUE
	 */
	double range, start, stop;

	if (GMT_interval_axis_item(T->id)) {
		if (GMT_uneven_interval (T->unit) || T->interval != 1.0) {	/* Must find next month to get month centered correctly */
			struct GMT_MOMENT_INTERVAL Inext;
			Inext.unit = T->unit;		/* Initialize MOMENT_INTERVAL structure members */
			Inext.step = 1;
			GMT_moment_interval (&Inext, coord[0], TRUE);	/* Get this one interval only */
			range = 0.5 * (Inext.dt[1] - Inext.dt[0]);	/* Half width of interval in internal representation */
			start = MAX (min, Inext.dt[0]);			/* Start of interval, but not less that start of axis */
			stop  = MIN (max, Inext.dt[1]);			/* Stop of interval,  but not beyond end of axis */
		}
		else {
			range = 0.5 * (coord[1] - coord[0]);	/* Half width of interval in internal representation */
			start = MAX (min, coord[0]);			/* Start of interval, but not less that start of axis */
			stop  = MIN (max, coord[1]);			/* Stop of interval,  but not beyond end of axis */
		}
		if ((stop - start) < (gmtdefs.time_interval_fraction * range)) return (TRUE);		/* Sorry, fraction not large enough to annotate */
		*pos = 0.5 * (start + stop);				/* Set half-way point */
		if (((*pos) - GMT_CONV_LIMIT) < min || ((*pos) + GMT_CONV_LIMIT) > max) return (TRUE);	/* Outside axis range */
	}
	else if (coord[0] < (min - GMT_CONV_LIMIT) || coord[0] > (max + GMT_CONV_LIMIT))		/* Outside axis range */
		return (TRUE);
	else
		*pos = coord[0];

	return (FALSE);
}

int GMT_get_coordinate_label (char *string, struct GMT_PLOT_CALCLOCK *P, char *format, struct GMT_PLOT_AXIS_ITEM *T, double coord)
{
	/* Returns the formatted annotation string for the non-geographic axes */

	switch (frame_info.axis[T->parent].type) {
		case GMT_LINEAR:
#if 0
			GMT_near_zero_roundoff_fixer_upper (&coord, T->parent);	/* Try to adjust those ~0 "gcc -O" values to exact 0 */
#endif
			sprintf (string, format, coord);
			break;
		case GMT_LOG10:
			sprintf (string, "%d", irint (d_log10 (coord)));
			break;
		case GMT_POW:
			if (project_info.xyz_projection[T->parent] == GMT_POW)
				sprintf (string, format, coord);
			else
				sprintf (string, "10@+%d@+", irint (d_log10 (coord)));
			break;
		case GMT_TIME:
			GMT_get_time_label (string, P, T, coord);
			break;
		default:
			fprintf (stderr, "%s: GMT ERROR: Wrong type (%d) passed to GMT_get_coordinate_label!\n", GMT_program, frame_info.axis[T->parent].type);
			GMT_exit (EXIT_FAILURE);
			break;
	}
	return (GMT_NOERROR);
}

#if 0
void GMT_near_zero_roundoff_fixer_upper (double *ww, int axis)
{	/* Try to adjust those pesky ~0 "gcc -O" values to exact 0 */
	double almost_zero_proj, exact_zero_proj;

	if (strcmp (gmtdefs.d_format, "%g") && strcmp (gmtdefs.d_format, "%lg")) return;	/* Only try to fix it if format is %lg or %g */

	switch (axis) {
		case 0:	/* X-axis */
			GMT_x_to_xx (*ww, &almost_zero_proj);
			GMT_x_to_xx (0.0, &exact_zero_proj);
			break;
		case 1:	/* Y-axis */
			GMT_y_to_yy (*ww, &almost_zero_proj);
			GMT_y_to_yy (0.0, &exact_zero_proj);
			break;
		case 2:	/* Z-axis */
			GMT_z_to_zz (*ww, &almost_zero_proj);
			GMT_z_to_zz (0.0, &exact_zero_proj);
			break;
	}
	if (GMT_IS_ZERO (*ww) && GMT_IS_ZERO (almost_zero_proj - exact_zero_proj)) *ww = 0.0;
}
#endif

double GMT_set_label_offsets (int axis, double val0, double val1, struct GMT_PLOT_AXIS *A, int below, double annot_off[], double *label_off, int *annot_justify, int *label_justify, char *format)
{
	/* Determines what the offsets will be for annotations and labels */

	int ndec;
	BOOLEAN as_is, flip, both;
	double v0, v1, tmp_offset, off, angle, sign, len;
	char text_l[GMT_LONG_TEXT], text_u[GMT_LONG_TEXT];
	struct GMT_PLOT_AXIS_ITEM *T;	/* Pointer to the current axis item */

	both = GMT_upper_and_lower_items(axis);							/* Two levels of annotations? */
	sign = ((below && axis == 0) || (!below && axis == 1)) ? -1.0 : 1.0;			/* since annotations go either below or above */
	len = (gmtdefs.tick_length > 0.0) ? gmtdefs.tick_length : 0.0;
	if (axis == 0) {
		if (A->type != GMT_TIME) GMT_get_format (GMT_get_map_interval (axis, GMT_ANNOT_UPPER), A->unit, A->prefix, format);	/* Set the annotation format template */
		annot_off[0] = GMT_get_annot_offset (&flip, 0);										/* Set upper annotation offset and flip depending on annot_offset */
		annot_off[1] = annot_off[0] + (gmtdefs.annot_font_size[0] * GMT_u2u[GMT_PT][GMT_INCH]) + 0.5 * fabs (gmtdefs.annot_offset[0]);	/* Lower annotation offset */
		if (both)	/* Must move label farther from axis given both annotation levels */
			*label_off = sign * (((flip) ? len : fabs (annot_off[1]) + (gmtdefs.annot_font_size[1] * GMT_u2u[GMT_PT][GMT_INCH]) * GMT_font[gmtdefs.annot_font[1]].height) + 1.5 * fabs (gmtdefs.annot_offset[0]));
		else		/* Just one level of annotation to clear */
			*label_off = sign * (((flip) ? len : fabs (annot_off[0]) + (gmtdefs.annot_font_size[0] * GMT_u2u[GMT_PT][GMT_INCH]) * GMT_font[gmtdefs.annot_font[0]].height) + 1.5 * fabs (gmtdefs.annot_offset[0]));
		annot_off[0] *= sign;		/* Change sign according to which axis we are doing */
		annot_off[1] *= sign;
		annot_justify[0] = annot_justify[1] = *label_justify = (below) ? 10 : 2;				/* Justification of annotation and label strings */
		if (flip) annot_justify[0] = GMT_flip_justify (annot_justify[0]);					/* flip is TRUE so flip the justification */
		angle = 0.0;
	}
	else {
		ndec = GMT_get_format (GMT_get_map_interval (axis, GMT_ANNOT_UPPER), A->unit, A->prefix, format);
		as_is = (ndec == 0 && !strchr (format, 'g'));	/* Use the d_format as is */

		switch (project_info.xyz_projection[axis]) {
			case GMT_POW:
				if (as_is) {
					sprintf (text_l, format, val0);
					sprintf (text_u, format, val1);
				}
				else {
					sprintf (text_l, "%d", (int)floor (val0));
					sprintf (text_u, "%d", (int)ceil (val1));
				}
				break;
			case GMT_LOG10:
				v0 = d_log10 (val0);
				v1 = d_log10 (val1);
				if (A->type == 2) {	/* 10 ^ pow annotations */
					sprintf (text_l, "10%d", (int)floor (v0));
					sprintf (text_u, "10%d", (int)ceil (v1));
				}
				else {
					if (as_is) {
						sprintf (text_l, format, val0);
						sprintf (text_u, format, val1);
					}
					else if (A->type == 1) {
						sprintf (text_l, "%d", (int)floor (v0));
						sprintf (text_u, "%d", (int)ceil (v1));
					}
					else {
						sprintf (text_l, format, val0);
						sprintf (text_u, format, val1);
					}
				}
				break;
			case GMT_LINEAR:
				if (as_is) {
					sprintf (text_l, format, val0);
					sprintf (text_u, format, val1);
				}
				else {
					sprintf (text_l, "%d", (int)floor (val0));
					sprintf (text_u, "%d", (int)ceil (val1));
				}
				break;
			case GMT_TIME:
				T = (A->item[GMT_ANNOT_UPPER].active) ? &A->item[GMT_ANNOT_UPPER] : &A->item[GMT_INTV_UPPER];
				GMT_get_coordinate_label (text_l, &GMT_plot_calclock, format, T, val0);		/* Get time annotation string */
				GMT_get_coordinate_label (text_u, &GMT_plot_calclock, format, T, val1);		/* Get time annotation string */
				break;
		}

		/* Find offset based on no of digits before and after a period, if any */

		off = ((MAX ((int)strlen (text_l), (int)strlen (text_u)) + ndec) * GMT_DEC_SIZE + ((ndec > 0) ? GMT_PER_SIZE : 0.0))
			* gmtdefs.annot_font_size[0] * GMT_u2u[GMT_PT][GMT_INCH];

		tmp_offset = GMT_get_annot_offset (&flip, 0);
		if (A->unit && A->unit[0] && gmtdefs.y_axis_type == 0) {	/* Accommodate extra width of annotation */
			int i, u_len, n_comp, len;
			i = u_len = n_comp = 0;
			len = strlen (A->unit);
			if (A->unit[0] == '-') i++;	/* Leading - to mean no-space */
			while (i < len) {
				if (A->unit[i] == '@' &&  A->unit[i+1]) {	/* escape sequences */
					i++;
					switch (A->unit[i]) {
						case '@':	/* Print the @ sign */
							u_len++;
							break;
						case '~':	/* Toggle symbol */
						case '+':	/* Toggle superscript */
						case '-':	/* Toggle subscript */
						case '#':	/* Toggle small caps */
							break;
						case '%':	/* Set font */
							i++;
							while (A->unit[i] && A->unit[i] != '%') i++;	/* Skip font number and trailing % */
						case '!':	/* Composite character */
							n_comp++;
							break;
						default:
							break;
					}
				}
				else if (A->unit[i] == '\\' && (len - i) > 3 && isdigit (A->unit[i+1]) && isdigit (A->unit[i+2]) && isdigit (A->unit[i+3])) {	/* Octal code */
					i += 3;
					u_len++;
				}
				else if (A->unit[i] == '\\') {	/* Escaped character */
					i++;
					u_len++;
				}
				else	/* Regular char */
					u_len++;
				i++;
			}
			off += (u_len - n_comp) * 0.49 * gmtdefs.annot_font_size[0] * GMT_u2u[GMT_PT][GMT_INCH];
		}
		*label_justify = (below) ? 2 : 10;
		if (gmtdefs.y_axis_type == 0) {	/* Horizontal annotations */
			annot_justify[0] = 7;
			annot_off[0] = sign * tmp_offset;
			if (A->item[GMT_ANNOT_LOWER].active)
				annot_off[1] = sign * (((flip) ? len : fabs (tmp_offset)) + 1.5 * fabs (gmtdefs.annot_offset[0]));
			else
				annot_off[1] = sign * (((flip) ? len : fabs (tmp_offset + off)) + 1.5 * fabs (gmtdefs.annot_offset[0]));
			if ((below + flip) != 1) annot_off[0] -= off;
			angle = -90.0;
		}
		else {
			annot_off[0] = sign * tmp_offset;
			annot_off[1] = sign * (((flip) ? len : fabs (tmp_offset) + (gmtdefs.annot_font_size[0] * GMT_u2u[GMT_PT][GMT_INCH]) * GMT_font[gmtdefs.annot_font[0]].height) + 1.5 * fabs (gmtdefs.annot_offset[0]));
			annot_justify[0] = (below) ? 2 : 10;
			angle = 0.0;
			if (flip) annot_justify[0] = GMT_flip_justify (annot_justify[0]);
		}
		if (both)	/* Must move label farther from axis given both annotation levels */
			*label_off = sign * (((flip) ? len : fabs (annot_off[1]) + (gmtdefs.annot_font_size[1] * GMT_u2u[GMT_PT][GMT_INCH]) * GMT_font[gmtdefs.annot_font[1]].height) + 1.5 * fabs (gmtdefs.annot_offset[0]));
		else		/* Just one level of annotation to clear */
			*label_off = sign * (((flip) ? len : fabs (annot_off[0]) + (gmtdefs.annot_font_size[0] * GMT_u2u[GMT_PT][GMT_INCH]) * GMT_font[gmtdefs.annot_font[0]].height) + 1.5 * fabs (gmtdefs.annot_offset[0]));
		if (A->item[GMT_ANNOT_LOWER].active && gmtdefs.y_axis_type == 0) *label_off += off;
		annot_justify[1] = (below) ? 2 : 10;
		if (A->item[GMT_ANNOT_LOWER].active) annot_justify[1] = annot_justify[0];
	}
	return (angle);
}

BOOLEAN GMT_is_fancy_boundary (void)
{
	switch (project_info.projection) {
		case GMT_LINEAR:
			return (GMT_IS_MAPPING);
			break;
		case GMT_MERCATOR:
		case GMT_CYL_EQ:
		case GMT_CYL_EQDIST:
		case GMT_MILLER:
			return (TRUE);
			break;
		case GMT_ALBERS:
		case GMT_ECONIC:
		case GMT_LAMBERT:
			return (project_info.region);
			break;
		case GMT_STEREO:
		case GMT_ORTHO:
		case GMT_GENPER:
		case GMT_LAMB_AZ_EQ:
		case GMT_AZ_EQDIST:
		case GMT_GNOMONIC:
		case GMT_VANGRINTEN:
			return (project_info.polar);
			break;
		case GMT_POLAR:
		case GMT_OBLIQUE_MERC:
		case GMT_HAMMER:
		case GMT_MOLLWEIDE:
		case GMT_SINUSOIDAL:
		case GMT_TM:
		case GMT_UTM:
		case GMT_CASSINI:
		case GMT_WINKEL:
		case GMT_ECKERT4:
		case GMT_ECKERT6:
		case GMT_ROBINSON:
			return (FALSE);
			break;
		default:
			fprintf (stderr, "%s: Error in GMT_is_fancy_boundary - notify developers\n", GMT_program);
			return (FALSE);
	}
}

int GMT_prepare_label (double angle, int side, double x, double y, int type, double *line_angle, double *text_angle, int *justify)
{
	BOOLEAN set_angle;

	if (!project_info.edge[side]) return -1;		/* Side doesn't exist */
	if (frame_info.side[side] < 2) return -1;	/* Don't want labels here */

	if (frame_info.check_side == TRUE) {
		if (type == 0 && side%2) return -1;
		if (type == 1 && !(side%2)) return -1;
	}

	if (gmtdefs.oblique_annotation & 16 && !(side%2)) angle = -90.0;	/* GMT_get_label_parameters will make this 0 */

	if (angle < 0.0) angle += 360.0;

	set_angle = ((project_info.region && !(GMT_IS_AZIMUTHAL || GMT_IS_CONICAL)) || !project_info.region);
	if (set_angle) {
		if (side == 0 && angle < 180.0) angle -= 180.0;
		if (side == 1 && (angle > 90.0 && angle < 270.0)) angle -= 180.0;
		if (side == 2 && angle > 180.0) angle -= 180.0;
		if (side == 3 && (angle < 90.0 || angle > 270.0)) angle -= 180.0;
	}

	if (!GMT_get_label_parameters (side, angle, type, text_angle, justify)) return -1;
	*line_angle = angle;
	if (gmtdefs.oblique_annotation & 16) *line_angle = (side - 1) * 90.0;

	if (!set_angle) *justify = GMT_polar_adjust (side, angle, x, y);

	return 0;
}

void GMT_get_annot_label (double val, char *label, int do_minutes, int do_seconds, int lonlat, BOOLEAN worldmap)
/* val:		Degree value of annotation */
/* label:	String to hold the final annotation */
/* do_minutes:	TRUE if degree and minutes are desired, FALSE for just integer degrees */
/* do_seconds:	TRUE if degree, minutes, and seconds are desired */
/* lonlat:	0 = longitudes, 1 = latitudes, 2 non-geographical data passed */
/* worldmap:	T/F, whatever GMT_world_map is */
{
	int fmt, sign, d, m, s, m_sec, level, type;
	BOOLEAN zero_fix = FALSE;
	char letter = 0, format[GMT_TEXT_LEN];

	if (lonlat == 0) {	/* Fix longitudes range first */
		GMT_lon_range_adjust (GMT_plot_calclock.geo.range, &val);
	}

	if (lonlat < 2) {	/* i.e., for geographical data */
		if (GMT_IS_ZERO (val - 360.0) && !worldmap) val = 0.0;
		if (GMT_IS_ZERO (val - 360.0) && worldmap && project_info.projection == GMT_OBLIQUE_MERC) val = 0.0;
	}

	fmt = gmtdefs.degree_format % 100;	/* take out the optional 100 or 1000 */
	if (GMT_plot_calclock.geo.wesn) {
		if (lonlat == 0) {
			switch (GMT_plot_calclock.geo.range) {
				case 0:
					letter = (GMT_IS_ZERO (val)) ? 0 : 'E';
					break;
				case 1:
					letter = (GMT_IS_ZERO (val)) ? 0 : 'W';
					break;
				default:
					letter = (GMT_IS_ZERO (val) || GMT_IS_ZERO (val - 180.0) || GMT_IS_ZERO (val + 180.0)) ? 0 : ((val < 0.0) ? 'W' : 'E');
					break;
			}
		}
		else
			letter = (GMT_IS_ZERO (val)) ? 0 : ((val < 0.0) ? 'S' : 'N');
		val = fabs (val);
	}
	else
		letter = 0;
	if (GMT_plot_calclock.geo.no_sign) val = fabs (val);
	sign = (val < 0.0) ? -1 : 1;

	level = do_minutes + do_seconds;		/* 0, 1, or 2 */
	type = (GMT_plot_calclock.geo.n_sec_decimals > 0) ? 1 : 0;

	if (fmt == -1 && lonlat) {	/* the r in r-theta */
		sprintf (format, "%s", gmtdefs.d_format);
		sprintf (label, format, val);
	}
	else if (GMT_plot_calclock.geo.decimal)
		sprintf (label, GMT_plot_calclock.geo.x_format, val, letter);
	else {
		(void) GMT_geo_to_dms (val, do_seconds, GMT_plot_calclock.geo.f_sec_to_int, &d, &m, &s, &m_sec);	/* Break up into d, m, s, and remainder */
		if (d == 0 && sign == -1) {	/* Must write out -0 degrees, do so by writing -1 and change 1 to 0 */
			d = -1;
			zero_fix = TRUE;
		}
		switch (2*level+type) {
			case 0:
				sprintf (label, GMT_plot_format[level][type], d, letter);
				break;
			case 1:
				sprintf (label, GMT_plot_format[level][type], d, m_sec, letter);
				break;
			case 2:
				sprintf (label, GMT_plot_format[level][type], d, m, letter);
				break;
			case 3:
				sprintf (label, GMT_plot_format[level][type], d, m, m_sec, letter);
				break;
			case 4:
				sprintf (label, GMT_plot_format[level][type], d, m, s, letter);
				break;
			case 5:
				sprintf (label, GMT_plot_format[level][type], d, m, s, m_sec, letter);
				break;
		}
		if (zero_fix) label[1] = '0';	/* Undo the fix above */
	}

	return;
}

void GMT_label_trim (char *label, int stage)
{
	int i;
	if (stage) {	/* Must remove leading stuff for 2ndary annotations */
		for (i = 0; stage && label[i]; i++) if (!isdigit((int)label[i])) stage--;
		while (label[i]) label[stage++] = label[i++];	/* Chop of beginning */
		label[stage] = '\0';
		i = strlen (label) - 1;
		if (strchr ("WESN", label[i])) label[i] = '\0';
	}
}

int GMT_polar_adjust (int side, double angle, double x, double y)
{
	int justify, left, right, top, bottom, low;
	double x0, y0;

	/* GMT_geo_to_xy (project_info.central_meridian, project_info.pole, &x0, &y0); */

	x0 = project_info.c_x0;
	y0 = project_info.c_y0;
	if (project_info.north_pole) {
		low = 0;
		left = 7;
		right = 5;
	}
	else {
		low = 2;
		left = 5;
		right = 7;
	}
	if ((y - y0 - GMT_SMALL) > 0.0) { /* i.e., y > y0 */
		top = 2;
		bottom = 10;
	}
	else {
		top = 10;
		bottom = 2;
	}
	if (project_info.projection == GMT_POLAR && project_info.got_azimuths) i_swap (left, right);	/* Because with azimuths we get confused... */
	if (project_info.projection == GMT_POLAR && project_info.got_elevations) {
		i_swap (top, bottom);	/* Because with elevations we get confused... */
		i_swap (left, right);
		low = 2 - low;
	}
	if (side%2) {	/* W and E border */
		if ((y - y0 + GMT_SMALL) > 0.0)
			justify = (side == 1) ? left : right;
		else
			justify = (side == 1) ? right : left;
	}
	else {
		if (frame_info.horizontal) {
			if (side == low)
				justify = (GMT_IS_ZERO (angle - 180.0)) ? bottom : top;
			else
				justify = (GMT_IS_ZERO (angle)) ? top : bottom;
			if (project_info.got_elevations && (GMT_IS_ZERO (angle - 180.0) || GMT_IS_ZERO (angle))) justify = (justify + 8) % 16;
		}
		else {
			if (x >= x0)
				justify = (side == 2) ? left : right;
			else
				justify = (side == 2) ? right : left;
		}
	}
	return (justify);
}

double GMT_get_angle (double lon1, double lat1, double lon2, double lat2)
{
	double x1, y1, x2, y2, dx, dy, angle, direction;

	GMT_geo_to_xy (lon1, lat1, &x1, &y1);
	GMT_geo_to_xy (lon2, lat2, &x2, &y2);
	dx = x2 - x1;
	dy = y2 - y1;
	if (GMT_IS_ZERO (dy) && GMT_IS_ZERO (dx)) {	/* Special case that only(?) occurs at N or S pole or r=0 for GMT_POLAR */
		if (fabs (fmod (lon1 - project_info.w + 360.0, 360.0)) > fabs (fmod (lon1 - project_info.e + 360.0, 360.0))) {	/* East */
			GMT_geo_to_xy (project_info.e, project_info.s, &x1, &y1);
			GMT_geo_to_xy (project_info.e, project_info.n, &x2, &y2);
			GMT_corner = 1;
		}
		else {
			GMT_geo_to_xy (project_info.w, project_info.s, &x1, &y1);
			GMT_geo_to_xy (project_info.w, project_info.n, &x2, &y2);
			GMT_corner = 3;
		}
		angle = d_atan2 (y2-y1, x2-x1) * R2D - 90.0;
		if (project_info.got_azimuths) angle += 180.0;
	}
	else
		angle = d_atan2 (dy, dx) * R2D;

	if (abs (GMT_x_status_old) == 2 && abs (GMT_y_status_old) == 2)	/* Last point outside */
		direction = angle + 180.0;
	else if (GMT_x_status_old == 0 && GMT_y_status_old == 0)		/* Last point inside */
		direction = angle;
	else {
		if (abs (GMT_x_status_new) == 2 && abs (GMT_y_status_new) == 2)	/* This point outside */
			direction = angle;
		else if (GMT_x_status_new == 0 && GMT_y_status_new == 0)		/* This point inside */
			direction = angle + 180.0;
		else {	/* Special case of corners and sides only */
			if (GMT_x_status_old == GMT_x_status_new)
				direction = (GMT_y_status_old == 0) ? angle : angle + 180.0;
			else if (GMT_y_status_old == GMT_y_status_new)
				direction = (GMT_x_status_old == 0) ? angle : angle + 180.0;
			else
				direction = angle;

		}
	}

	if (direction < 0.0) direction += 360.0;
	if (direction >= 360.0) direction -= 360.0;
	return (direction);
}

int GMT_get_label_parameters (int side, double line_angle, int type, double *text_angle, int *justify)
{
	int ok;

	*text_angle = line_angle;
#ifdef OPT_WORKS_BADLY
	if (*text_angle < -90.0) *text_angle += 360.0;
	if (frame_info.horizontal && !(side%2)) *text_angle += 90.0;
	if (*text_angle > 270.0 ) *text_angle -= 360.0;
	else if (*text_angle > 90.0) *text_angle -= 180.0;
#else
	if ( (*text_angle + 90.0) < GMT_CONV_LIMIT) *text_angle += 360.0;
	if (frame_info.horizontal && !(side%2)) *text_angle += 90.0;
	if ( (*text_angle - 270.0) > GMT_CONV_LIMIT ) *text_angle -= 360.0;
	else if ( (*text_angle - 90.0) > GMT_CONV_LIMIT) *text_angle -= 180.0;
#endif

	if (type == 0 && gmtdefs.oblique_annotation & 2) *text_angle = 0.0;	/* Force horizontal lon annotation */
	if (type == 1 && gmtdefs.oblique_annotation & 4) *text_angle = 0.0;	/* Force horizontal lat annotation */

	switch (side) {
		case 0:		/* S */
			if (frame_info.horizontal)
				*justify = (project_info.got_elevations) ? 2 : 10;
			else
				*justify = ((*text_angle) < 0.0) ? 5 : 7;
			break;
		case 1:		/* E */
			if (type == 1 && gmtdefs.oblique_annotation & 32) {
				*text_angle = 90.0;	/* Force parallel lat annotation */
				*justify = 10;
			}
			else
				*justify = 5;
			break;
		case 2:		/* N */
			if (frame_info.horizontal)
				*justify = (project_info.got_elevations) ? 10 : 2;
			else
				*justify = ((*text_angle) < 0.0) ? 7 : 5;
			break;
		default:	/* W */
			if (type == 1 && gmtdefs.oblique_annotation & 32) {
				*text_angle = 90.0;	/* Force parallel lat annotation */
				*justify = 2;
			}
			else
				*justify = 7;
			break;
	}

	if (frame_info.horizontal) return (TRUE);

	switch (side) {
		case 0:		/* S */
		case 2:		/* N */
			ok = (fabs ((*text_angle)) >= gmtdefs.annot_min_angle);
			break;
		default:	/* E or W */
			ok = (fabs ((*text_angle)) <= (90.0 - gmtdefs.annot_min_angle));
			break;
	}
	return (ok);
}

char *GMT_convertpen (struct GMT_PEN *pen, int *width, int *offset, int rgb[])
{
	/* GMT_convertpen converts from internal points to current dpi unit.
	 * It allocates space and returns a pointer to the texture, if not null */

	char tmp[GMT_TEXT_LEN], buffer[BUFSIZ], ptr[BUFSIZ], *texture = CNULL;
	double pt_to_dpi;
	int n, pos;

	pt_to_dpi = GMT_u2u[GMT_PT][GMT_INCH] * gmtdefs.dpi;

	*width = irint (pen->width * pt_to_dpi);

	if (pen->texture[0]) {
		texture = (char *) GMT_memory (VNULL, BUFSIZ, sizeof (char), "GMT_convertpen");
		strcpy (buffer, pen->texture);
		pos = 0;
		while ((GMT_strtok (buffer, " ", &pos, ptr))) {
			sprintf (tmp, "%d ", irint (atof (ptr) * pt_to_dpi));
			strcat (texture, tmp);
		}
		n = strlen (texture);
		texture[n-1] = 0;
		texture = (char *) GMT_memory ((void *)texture, n, sizeof (char), "GMT_convertpen");
		*offset = irint (pen->offset * pt_to_dpi);
	}
	else
		*offset = 0;

	memcpy ((void *)rgb, (void *)pen->rgb, (size_t)(3 * sizeof (int)));
	return (texture);
}

int GMT_grid_clip_path (struct GRD_HEADER *h, double **x, double **y, BOOLEAN *donut)
{
	/* This function returns a clip path corresponding to the
	 * extent of the grid.
	 */

	int np, i, j;
	double *work_x, *work_y;

	*donut = FALSE;

	if (GMT_IS_RECT_GRATICULE) {	/* Where wesn are straight hor/ver lines */
		np = 4;
		work_x = (double *)GMT_memory (VNULL, (size_t)np, sizeof (double), "GMT_map_clip_path");
		work_y = (double *)GMT_memory (VNULL, (size_t)np, sizeof (double), "GMT_map_clip_path");
		GMT_geo_to_xy (h->x_min, h->y_min, &work_x[0], &work_y[0]);
		GMT_geo_to_xy (h->x_max, h->y_max, &work_x[2], &work_y[2]);
		if (work_x[0] < project_info.xmin) work_x[0] = project_info.xmin;
		if (work_x[2] > project_info.xmax) work_x[2] = project_info.xmax;
		if (work_y[0] < project_info.ymin) work_y[0] = project_info.ymin;
		if (work_y[2] > project_info.ymax) work_y[2] = project_info.ymax;
		work_x[3] = work_x[0];	work_x[1] = work_x[2];
		work_y[1] = work_y[0];	work_y[3] = work_y[2];

	}
	else {	/* WESN are complex curved lines */

		np = 2 * (h->nx + h->ny - 2);
		work_x = (double *)GMT_memory (VNULL, (size_t)np, sizeof (double), "GMT_map_clip_path");
		work_y = (double *)GMT_memory (VNULL, (size_t)np, sizeof (double), "GMT_map_clip_path");
		for (i = j = 0; i < h->nx-1; i++, j++)	/* South */
			GMT_geo_to_xy (h->x_min + i * h->x_inc, h->y_min, &work_x[j], &work_y[j]);
		for (i = 0; i < h->ny-1; j++, i++)	/* East */
			GMT_geo_to_xy (h->x_max, h->y_min + i * h->y_inc, &work_x[j], &work_y[j]);
		for (i = 0; i < h->nx-1; i++, j++)	/* North */
			GMT_geo_to_xy (h->x_max - i * h->x_inc, h->y_max, &work_x[j], &work_y[j]);
		for (i = 0; i < h->ny-1; j++, i++)	/* West */
			GMT_geo_to_xy (h->x_min, h->y_max - i * h->y_inc, &work_x[j], &work_y[j]);
	}

	if (!(*donut)) np = GMT_compact_line (work_x, work_y, np, FALSE, (int *)0);
	if (project_info.three_D) GMT_2D_to_3D (work_x, work_y, project_info.z_level, np);

	*x = work_x;
	*y = work_y;

	return (np);
}

double GMT_get_annot_offset (BOOLEAN *flip, int level)
{
	/* Return offset in inches for text annotation.  If annotation
	 * is to be placed 'inside' the map, set flip to TRUE */

	double a;

	a = gmtdefs.annot_offset[level];
	if (a >= 0.0) {	/* Outside annotation */
		if (gmtdefs.tick_length > 0.0) a += gmtdefs.tick_length;
		*flip = FALSE;
	}
	else {		/* Inside annotation */
		if (gmtdefs.tick_length < 0.0) a += gmtdefs.tick_length;
		*flip = TRUE;
	}

	return (a);
}

int GMT_flip_justify (int justify)
{
	/* Return the opposite justification */

	int j;

	switch (justify) {
		case 2:
			j = 10;
			break;
		case 5:
			j = 7;
			break;
		case 7:
			j = 5;
			break;
		case 10:
			j = 2;
			break;
		default:
			j = justify;
			fprintf (stderr, "%s: GMT_flip_justify called with incorrect argument (%d)\n", GMT_program, j);
			break;
	}

	return (j);
}

struct GMT_CUSTOM_SYMBOL * GMT_get_custom_symbol (char *name) {
	int i, found = -1;

	/* First see if we already have loaded this symbol */

	for (i = 0; found == -1 && i < GMT_n_custom_symbols; i++) if (!strcmp (name, GMT_custom_symbol[i]->name)) found = i;

	if (found >= 0) return (GMT_custom_symbol[found]);	/* Return a previously loaded symbol */

	/* Must load new symbol */

	GMT_custom_symbol = (struct GMT_CUSTOM_SYMBOL **) GMT_memory ((void *)GMT_custom_symbol, (size_t)(GMT_n_custom_symbols+1), sizeof (struct GMT_CUSTOM_SYMBOL *), GMT_program);
	GMT_init_custom_symbol (name, &(GMT_custom_symbol[GMT_n_custom_symbols]));

	return (GMT_custom_symbol[GMT_n_custom_symbols++]);
}

int GMT_init_custom_symbol (char *name, struct GMT_CUSTOM_SYMBOL **S) {
	int nc, last, error = 0;
	BOOLEAN do_fill, do_pen, first = TRUE;
	char file[BUFSIZ], buffer[BUFSIZ], col[8][GMT_TEXT_LEN];
	char *fill_p = VNULL, *pen_p = VNULL;
	FILE *fp;
	struct GMT_CUSTOM_SYMBOL *head;
	struct GMT_CUSTOM_SYMBOL_ITEM *s = NULL, *previous = NULL;

	GMT_getsharepath ("custom", name, ".def", file);
	if ((fp = fopen (file, "r")) == NULL) {
		fprintf (stderr, "GMT ERROR: %s : Could not find custom symbol %s\n", GMT_program, name);
		GMT_exit (EXIT_FAILURE);
	}

	head = (struct GMT_CUSTOM_SYMBOL *) GMT_memory (VNULL, (size_t)1, sizeof (struct GMT_CUSTOM_SYMBOL), GMT_program);
	strcpy (head->name, name);
	while (fgets (buffer, BUFSIZ, fp)) {
		if (buffer[0] == '#' || buffer[0] == '\n') continue;

		nc = sscanf (buffer, "%s %s %s %s %s %s %s", col[0], col[1], col[2], col[3], col[4], col[5], col[6]);

		s = (struct GMT_CUSTOM_SYMBOL_ITEM *) GMT_memory (VNULL, (size_t)1, sizeof (struct GMT_CUSTOM_SYMBOL_ITEM), GMT_program);
		if (first) head->first = s;
		first = FALSE;

		s->x = atof (col[0]);
		s->y = atof (col[1]);

		do_fill = do_pen = FALSE;

		last = nc - 1;
		if (col[last][0] == '-' && col[last][1] == 'G') fill_p = &col[last][2], do_fill = TRUE, last--;
		if (col[last][0] == '-' && col[last][1] == 'W') pen_p = &col[last][2], do_pen = TRUE, last--;
		if (col[last][0] == '-' && col[last][1] == 'G') fill_p = &col[last][2], do_fill = TRUE, last--;	/* Check again for -G since perhaps -G -W was given */
		if (last < 2) error++;

		switch (col[last][0]) {

			/* M, D, and A allows for arbitrary polygons to be designed - these may be painted or filled with pattern */

			case 'M':		/* Set new anchor point */
				if (last != 2) error++;
				s->action = GMT_ACTION_MOVE;
				break;

			case 'D':		/* Draw to next point */
				if (last != 2) error++;
				s->action = GMT_ACTION_DRAW;
				break;

			case 'A':		/* Draw arc of a circle */
				if (last != 5) error++;
				s->p[0] = atof (col[2]);
				s->p[1] = atof (col[3]);
				s->p[2] = atof (col[4]);
				s->action = GMT_ACTION_ARC;
				break;

			/* These are standard psxy-type symbols.  They can only be painted, not used with pattern fill.  Exception is circle which can take pattern */

			case 'C':		/* Draw complete circle (backwards compatible) */
			case 'c':		/* Draw complete circle */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_CIRCLE;
				break;

			case 'a':		/* Draw star symbol */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_STAR;
				break;

			case 'd':		/* Draw diamond symbol */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_DIAMOND;
				break;

			case 'h':		/* Draw hexagon symbol */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_HEXAGON;
				break;

			case 'i':		/* Draw inverted triangle symbol */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_ITRIANGLE;
				break;

			case 'l':		/* Draw letter/text symbol */
				if (last != 4) error++;
				s->p[0] = atof (col[2]);
				s->string = (char *)GMT_memory (VNULL, (size_t)(strlen (col[3]) + 1), sizeof (char), GMT_program);
				strcpy (s->string, col[3]);
				s->action = GMT_ACTION_TEXT;
				break;

			case 'n':		/* Draw pentagon symbol */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_PENTAGON;
				break;

			case 'g':		/* Draw octagon symbol */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_OCTAGON;
				break;

			case 's':		/* Draw square symbol */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_SQUARE;
				break;

			case 't':		/* Draw triangle symbol */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_TRIANGLE;
				break;

			case 'x':		/* Draw cross symbol */
				if (last != 3) error++;
				s->p[0] = atof (col[2]);
				s->action = GMT_ACTION_CROSS;
				break;

			case 'r':		/* Draw rect symbol */
				if (last != 4) error++;
				s->p[0] = atof (col[2]);
				s->p[1] = atof (col[3]);
				s->action = GMT_ACTION_RECT;
				break;

			case 'w':		/* Draw wedge (pie) symbol */
				if (last != 5) error++;
				s->p[0] = atof (col[2]);
				s->p[1] = atof (col[3]);	/* Leave angles in degrees */
				s->p[2] = atof (col[4]);
				s->action = GMT_ACTION_PIE;
				break;

			case 'e':		/* Draw ellipse symbol */
				if (last != 5) error++;
				s->p[0] = atof (col[2]);	/* Leave direction in degrees */
				s->p[1] = atof (col[3]);
				s->p[2] = atof (col[4]);
				s->action = GMT_ACTION_ELLIPSE;
				break;

			default:
				error++;
				break;
		}

		if (error) {
			fprintf (stderr, "GMT ERROR: %s : Error in parsing symbol commands in file %s\n", GMT_program, file);
			fprintf (stderr, "GMT ERROR: %s : Offending line: %s\n", GMT_program, buffer);
			GMT_exit (EXIT_FAILURE);
		}

		if (do_fill) {
			s->fill = (struct GMT_FILL *) GMT_memory (VNULL, (size_t)1, sizeof (struct GMT_FILL), GMT_program);
			if (fill_p[0] == '-')	/* Do not want to fill this polygon */
				s->fill->rgb[0] = -1;
			else if (GMT_getfill (fill_p, s->fill)) {
				GMT_fill_syntax ('G', " ");
				GMT_exit (EXIT_FAILURE);
			}
		}
		else
			s->fill = NULL;
		if (do_pen) {
			s->pen = (struct GMT_PEN *) GMT_memory (VNULL, (size_t)1, sizeof (struct GMT_PEN), GMT_program);
			if (pen_p[0] == '-')	/* Do not want to draw outline */
				s->pen->rgb[0] = -1;
			else if (GMT_getpen (pen_p, s->pen)) {
				GMT_pen_syntax ('W', " ");
				GMT_exit (EXIT_FAILURE);
			}
		}
		else
			s->pen = NULL;

		if (previous) previous->next = s;
		previous = s;
	}
	fclose (fp);
	*S = head;
	return (GMT_NOERROR);
}

void GMT_NaN_pen_up (double x[], double y[], int pen[], int n)
{
	/* Ensure that if there are NaNs we set pen = 3 */

	int i, n1;

	for (i = 0, n1 = n - 1; i < n; i++) {
		if (GMT_is_dnan (x[i]) || GMT_is_dnan (y[i])) {
			pen[i] = 3;
			if (i < n1) pen[i+1] = 3;	/* Since the next point must become the new anchor */
		}
	}
}

BOOLEAN GMT_polygon_is_open (double x[], double y[], int n)
{	/* Returns TRUE if the first and last point is not identical */

	return (!(x[0] == x[n-1] && y[0] == y[n-1]));
}

/*--------------------------------------------------------------------
 *	Translation of old fourt.f FORTRAN code to C using the automatic
 *	translator f2c written by S.I. Feldman, David M. Gay, Mark W. Maimone,
 *	and N.L. Schryer.  Translated version provided by Andrew MacRae
 *	at the University of Calgary.  I've cleaned up the resulting code
 *	since much of the f2c.h include stuff was unnecessary for this
 *	function.
 *
 *	P. Wessel, last century sometime
 *--------------------------------------------------------------------*/

/* C-callable wrapper for fourt_ */

void GMT_fourt (float *data, int *nn, int ndim, int ksign, int iform, float *work)
	/* Data array */
	/* Dimension array */
	/* Number of dimensions */
	/* Forward(-1) or Inverse(+1) */
	/* Real(0) or complex(1) data */
	/* Work array */
{
	int fourt_ (float *data, int *nn, int *ndim, int *ksign, int *iform, float *work);
	(void) fourt_ (data, nn, &ndim, &ksign, &iform, work);
}

int fourt_ (float *data, int *nn, int *ndim, int *ksign, int *iform, float *work)
{

    /* System generated locals */
    int i__1, i__2, i__3, i__4, i__5, i__6, i__7, i__8, i__9, i__10, i__11, i__12;

    /* Builtin functions */

    double cos(double), sin(double);

    /* Local variables */

    static int j1rg2, idiv, irem, ipar, kmin, imin, jmin, lmax, mmax, imax, jmax;
    static int ntwo, j1cnj, np1hf, np2hf, j1min, i1max, i1rng, j1rng, j2min, j3max;
    static int j1max, j2max, i2max, non2t, j2stp, i, j, k, l, m, n, icase, ifact[32];
    static int nhalf, krang, kconj, kdif, idim, ntot, kstep, k2, k3, k4, nprev, iquot;
    static int i2, i1, i3, j3, k1, j2, j1, if_, np0, np1, np2, ifp1, ifp2, non2;

    static float theta, oldsi, tempi, oldsr, sinth, difi, difr, sumi, sumr, tempr, twopi;
    static float wstpi, wstpr, twowr, wi, wr, u1i, w2i, w3i, u2i, u3i, u4i, t2i, u1r;
    static float u2r, u3r, w2r, w3r, u4r, t2r, t3r, t3i, t4r, t4i;
    static double wrd, wid;

/*---------------------------------------------------------------------------
       ARGUMENTS :
		DATA - COMPLEX ARRAY, LENGTH NN
		NN - ARRAY OF NUMBER OF POINTS IN EACH DIMENSION
		NDIM - NUMBER OF DIMENSIONS (FOR OUR PURPOSES, NDIM=1)
		KSIGN - +1 FOR INVERSE TRANSFORM (FREQ TO GMT_TIME DOMAIN)
			-1 FOR FORWARD TRANSFORM (GMT_TIME TO FREQ DOMAIN)
		IFORM - 0 REAL DATA
			+1 COMPLEX DATA
		WORK - 0 IF ALL DIMENSIONS ARE RADIX 2
		COMPLEX ARRAY, LARGE AS LARGEST NON-RADIX 2 DIMENSI0N

	PROGRAM BY NORMAN BRENNER FROM THE BASIC PROGRAM BY CHARLES
	RADER.  RALPH ALTER SUGGESTED THE IDEA FOR THE DIGIT REVERSAL.
	MIT LINCOLN LABORATORY, AUGUST 1967.

---------------------------------------------------------------------------*/

   /* Parameter adjustments */
    --work;
    --nn;
    --data;

    /* Function Body */
    wr = wi = wstpr = wstpi = (float)0.0;
    twopi = (float)6.283185307;
    if (*ndim - 1 >= 0) {
	goto L1;
    } else {
	goto L920;
    }
L1:
    ntot = 2;
    i__1 = *ndim;
    for (idim = 1; idim <= i__1; ++idim) {
	if (nn[idim] <= 0) {
	    goto L920;
	} else {
	    goto L2;
	}
L2:
	ntot *= nn[idim];
    }
    np1 = 2;
    i__1 = *ndim;
    for (idim = 1; idim <= i__1; ++idim) {
	n = nn[idim];
	np2 = np1 * n;
	if ((i__2 = n - 1) < 0) {
	    goto L920;
	} else if (i__2 == 0) {
	    goto L900;
	} else {
	    goto L5;
	}
L5:
	m = n;
	ntwo = np1;
	if_ = 1;
	idiv = 2;
L10:
	iquot = m / idiv;
	irem = m - idiv * iquot;
	if (iquot - idiv >= 0) {
	    goto L11;
	} else {
	    goto L50;
	}
L11:
	if (irem != 0) {
	    goto L20;
	} else {
	    goto L12;
	}
L12:
	ntwo += ntwo;
	m = iquot;
	goto L10;
L20:
	idiv = 3;
L30:
	iquot = m / idiv;
	irem = m - idiv * iquot;
	if (iquot - idiv >= 0) {
	    goto L31;
	} else {
	    goto L60;
	}
L31:
	if (irem != 0) {
	    goto L40;
	} else {
	    goto L32;
	}
L32:
	ifact[if_ - 1] = idiv;
	++if_;
	m = iquot;
	goto L30;
L40:
	idiv += 2;
	goto L30;
L50:
	if (irem != 0) {
	    goto L60;
	} else {
	    goto L51;
	}
L51:
	ntwo += ntwo;
	goto L70;
L60:
	ifact[if_ - 1] = m;
L70:
	non2 = np1 * (np2 / ntwo);
	icase = 1;
	if (idim - 4 >= 0) {
	    goto L90;
	} else {
	    goto L71;
	}
L71:
	if (*iform <= 0) {
	    goto L72;
	} else {
	    goto L90;
	}
L72:
	icase = 2;
	if (idim - 1 <= 0) {
	    goto L73;
	} else {
	    goto L90;
	}
L73:
	icase = 3;
	if (ntwo - np1 <= 0) {
	    goto L90;
	} else {
	    goto L74;
	}
L74:
	icase = 4;
	ntwo /= 2;
	n /= 2;
	np2 /= 2;
	ntot /= 2;
	i = 3;
	i__2 = ntot;
	for (j = 2; j <= i__2; ++j) {
	    data[j] = data[i];
	    i += 2;
	}
L90:
	i1rng = np1;
	if (icase - 2 != 0) {
	    goto L100;
	} else {
	    goto L95;
	}
L95:
	i1rng = np0 * (nprev / 2 + 1);
L100:
	if (ntwo - np1 <= 0) {
	    goto L600;
	} else {
	    goto L110;
	}
L110:
	np2hf = np2 / 2;
	j = 1;
	i__2 = np2;
	i__3 = non2;
	for (i2 = 1; i__3 < 0 ? i2 >= i__2 : i2 <= i__2; i2 += i__3) {
	    if (j - i2 >= 0) {
		goto L130;
	    } else {
		goto L120;
	    }
L120:
	    i1max = i2 + non2 - 2;
	    i__4 = i1max;
	    for (i1 = i2; i1 <= i__4; i1 += 2) {
		i__5 = ntot;
		i__6 = np2;
		for (i3 = i1; i__6 < 0 ? i3 >= i__5 : i3 <= i__5; i3 += i__6) {
		    j3 = j + i3 - i2;
		    tempr = data[i3];
		    tempi = data[i3 + 1];
		    data[i3] = data[j3];
		    data[i3 + 1] = data[j3 + 1];
		    data[j3] = tempr;
		    data[j3 + 1] = tempi;
		}
	    }
L130:
	    m = np2hf;
L140:
	    if (j - m <= 0) {
		goto L150;
	    } else {
		goto L145;
	    }
L145:
	    j -= m;
	    m /= 2;
	    if (m - non2 >= 0) {
		goto L140;
	    } else {
		goto L150;
	    }
L150:
	    j += m;
	}
	non2t = non2 + non2;
	ipar = ntwo / np1;
L310:
	if ((i__3 = ipar - 2) < 0) {
	    goto L350;
	} else if (i__3 == 0) {
	    goto L330;
	} else {
	    goto L320;
	}
L320:
	ipar /= 4;
	goto L310;
L330:
	i__3 = i1rng;
	for (i1 = 1; i1 <= i__3; i1 += 2) {
	    i__2 = non2;
	    i__6 = np1;
	    for (j3 = i1; i__6 < 0 ? j3 >= i__2 : j3 <= i__2; j3 +=  i__6) {
		i__5 = ntot;
		i__4 = non2t;
		for (k1 = j3; i__4 < 0 ? k1 >= i__5 : k1 <= i__5; k1 += i__4) {
		    k2 = k1 + non2;
		    tempr = data[k2];
		    tempi = data[k2 + 1];
		    data[k2] = data[k1] - tempr;
		    data[k2 + 1] = data[k1 + 1] - tempi;
		    data[k1] += tempr;
		    data[k1 + 1] += tempi;
		}
	    }
	}
L350:
	mmax = non2;
L360:
	if (mmax - np2hf >= 0) {
	    goto L600;
	} else {
	    goto L370;
	}
L370:
/* Computing MAX */
	i__4 = non2t, i__5 = mmax / 2;
	lmax = MAX(i__4,i__5);
	if (mmax - non2 <= 0) {
	    goto L405;
	} else {
	    goto L380;
	}
L380:
	theta = -twopi * (float) non2 / (float) (mmax <<  2);
	if (*ksign >= 0) {
	    goto L390;
	} else {
	    goto L400;
	}
L390:
	theta = -theta;
L400:
	sincos ((double)theta, &wid, &wrd);
	wr = (float)wrd;
	wi = (float)wid;
	wstpr = (float)-2.0 * wi * wi;
	wstpi = (float)2.0 * wr * wi;
L405:
	i__4 = lmax;
	i__5 = non2t;
	for (l = non2; i__5 < 0 ? l >= i__4 : l <= i__4; l += i__5) {
	    m = l;
	    if (mmax - non2 <= 0) {
		goto L420;
	    } else {
		goto L410;
	    }
L410:
	    w2r = wr * wr - wi * wi;
	    w2i = (float)(wr * 2.0 * wi);
	    w3r = w2r * wr - w2i * wi;
	    w3i = w2r * wi + w2i * wr;
L420:
	    i__6 = i1rng;
	    for (i1 = 1; i1 <= i__6; i1 += 2) {
		i__2 = non2;
		i__3 = np1;
		for (j3 = i1; i__3 < 0 ? j3 >= i__2 : j3 <= i__2; j3  += i__3) {
		    kmin = j3 + ipar * m;
		    if (mmax - non2 <= 0) {
			goto L430;
		    } else {
			goto L440;
		    }
L430:
		    kmin = j3;
L440:
		    kdif = ipar * mmax;
L450:
		    kstep = kdif << 2;
		    i__7 = ntot;
		    i__8 = kstep;
		    for (k1 = kmin; i__8 < 0 ? k1 >= i__7 : k1 <=  i__7; k1 += i__8) {
			k2 = k1 + kdif;
			k3 = k2 + kdif;
			k4 = k3 + kdif;
			if (mmax - non2 <= 0) {
			    goto L460;
			} else {
			    goto L480;
			}
L460:
			u1r = data[k1] + data[k2];
			u1i = data[k1 + 1] + data[k2 + 1];
			u2r = data[k3] + data[k4];
			u2i = data[k3 + 1] + data[k4 + 1];
			u3r = data[k1] - data[k2];
			u3i = data[k1 + 1] - data[k2 + 1];
			if (*ksign >= 0) {
			    goto L475;
			} else {
			    goto L470;
			}
L470:
			u4r = data[k3 + 1] - data[k4 + 1];
			u4i = data[k4] - data[k3];
			goto L510;
L475:
			u4r = data[k4 + 1] - data[k3 + 1];
			u4i = data[k3] - data[k4];
			goto L510;
L480:
			t2r = w2r * data[k2] - w2i * data[k2 + 1];
			t2i = w2r * data[k2 + 1] + w2i * data[k2];
			t3r = wr * data[k3] - wi * data[k3 + 1];
			t3i = wr * data[k3 + 1] + wi * data[k3];
			t4r = w3r * data[k4] - w3i * data[k4 + 1];
			t4i = w3r * data[k4 + 1] + w3i * data[k4];
			u1r = data[k1] + t2r;
			u1i = data[k1 + 1] + t2i;
			u2r = t3r + t4r;
			u2i = t3i + t4i;
			u3r = data[k1] - t2r;
			u3i = data[k1 + 1] - t2i;
			if (*ksign >= 0) {
			    goto L500;
			} else {
			    goto L490;
			}
L490:
			u4r = t3i - t4i;
			u4i = t4r - t3r;
			goto L510;
L500:
			u4r = t4i - t3i;
			u4i = t3r - t4r;
L510:
			data[k1] = u1r + u2r;
			data[k1 + 1] = u1i + u2i;
			data[k2] = u3r + u4r;
			data[k2 + 1] = u3i + u4i;
			data[k3] = u1r - u2r;
			data[k3 + 1] = u1i - u2i;
			data[k4] = u3r - u4r;
			data[k4 + 1] = u3i - u4i;
		    }
		    kmin = ((kmin - j3) << 2) + j3;
		    kdif = kstep;
		    if (kdif - np2 >= 0) {
			goto L530;
		    } else {
			goto L450;
		    }
L530:
		    ;
		}
	    }
	    m = mmax - m;
	    if (*ksign >= 0) {
		goto L550;
	    } else {
		goto L540;
	    }
L540:
	    tempr = wr;
	    wr = -wi;
	    wi = -tempr;
	    goto L560;
L550:
	    tempr = wr;
	    wr = wi;
	    wi = tempr;
L560:
	    if (m - lmax <= 0) {
		goto L565;
	    } else {
		goto L410;
	    }
L565:
	    tempr = wr;
	    wr = wr * wstpr - wi * wstpi + wr;
	    wi = wi * wstpr + tempr * wstpi + wi;
	}
	ipar = 3 - ipar;
	mmax += mmax;
	goto L360;
L600:
	if (ntwo - np2 >= 0) {
	    goto L700;
	} else {
	    goto L605;
	}
L605:
	ifp1 = non2;
	if_ = 1;
	np1hf = np1 / 2;
L610:
	ifp2 = ifp1 / ifact[if_ - 1];
	j1rng = np2;
	if (icase - 3 != 0) {
	    goto L612;
	} else {
	    goto L611;
	}
L611:
	j1rng = (np2 + ifp1) / 2;
	j2stp = np2 / ifact[if_ - 1];
	j1rg2 = (j2stp + ifp2) / 2;
L612:
	j2min = ifp2 + 1;
	if (ifp1 - np2 >= 0) {
	    goto L640;
	} else {
	    goto L615;
	}
L615:
	i__5 = ifp1;
	i__4 = ifp2;
	for (j2 = j2min; i__4 < 0 ? j2 >= i__5 : j2 <= i__5; j2 +=  i__4) {
	    theta = -twopi * (float) (j2 - 1) / (float)  np2;
	    if (*ksign >= 0) {
		goto L620;
	    } else {
		goto L625;
	    }
L620:
	    theta = -theta;
L625:
	    sinth = (float)sin((double)(0.5 * theta));
	    wstpr = sinth * (float)(-2. * sinth);
	    wstpi = (float)sin((double)theta);
	    wr = wstpr + (float)1.0;
	    wi = wstpi;
	    j1min = j2 + ifp1;
	    i__3 = j1rng;
	    i__2 = ifp1;
	    for (j1 = j1min; i__2 < 0 ? j1 >= i__3 : j1 <= i__3; j1 += i__2) {

		i1max = j1 + i1rng - 2;
		i__6 = i1max;
		for (i1 = j1; i1 <= i__6; i1 += 2) {
		    i__8 = ntot;
		    i__7 = np2;
		    for (i3 = i1; i__7 < 0 ? i3 >= i__8 : i3 <= i__8; i3 += i__7) {
			j3max = i3 + ifp2 - np1;
			i__9 = j3max;
			i__10 = np1;
			for (j3 = i3; i__10 < 0 ? j3 >= i__9 : j3 <= i__9; j3 += i__10) {
			    tempr = data[j3];
			    data[j3] = data[j3] * wr - data[j3 + 1] *  wi;
			    data[j3 + 1] = tempr * wi + data[j3 + 1]  * wr;
			}
		    }
		}
		tempr = wr;
		wr = wr * wstpr - wi * wstpi + wr;
		wi = tempr * wstpi + wi * wstpr + wi;
	    }
	}
L640:
	theta = -twopi / (float) ifact[if_ - 1];
	if (*ksign >= 0) {
	    goto L645;
	} else {
	    goto L650;
	}
L645:
	theta = -theta;
L650:
	sinth = (float)sin((double)(0.5 * theta));
	wstpr = sinth * (float)(-2. * sinth);
	wstpi = (float)sin((double)theta);
	kstep = (n << 1) / ifact[if_ - 1];
	krang = kstep * (ifact[if_ - 1] / 2) + 1;
	i__2 = i1rng;
	for (i1 = 1; i1 <= i__2; i1 += 2) {
	    i__3 = ntot;
	    i__4 = np2;
	    for (i3 = i1; i__4 < 0 ? i3 >= i__3 : i3 <= i__3; i3 += i__4) {
		i__5 = krang;
		i__10 = kstep;
		for (kmin = 1; i__10 < 0 ? kmin >= i__5 : kmin <= i__5; kmin += i__10) {
		    j1max = i3 + j1rng - ifp1;
		    i__9 = j1max;
		    i__7 = ifp1;
		    for (j1 = i3; i__7 < 0 ? j1 >= i__9 : j1 <= i__9; j1 += i__7) {
			j3max = j1 + ifp2 - np1;
			i__8 = j3max;
			i__6 = np1;
			for (j3 = j1; i__6 < 0 ? j3 >= i__8 : j3 <= i__8; j3 += i__6) {
			    j2max = j3 + ifp1 - ifp2;
			    k = kmin + (j3 - j1 + (j1 - i3) / ifact[if_ - 1]) / np1hf;
			    if (kmin - 1 <= 0) {
				goto L655;
			    } else {
				goto L665;
			    }
L655:
			    sumr = (float)0.0;
			    sumi = (float)0.0;
			    i__11 = j2max;
			    i__12 = ifp2;
			    for (j2 = j3; i__12 < 0 ? j2 >= i__11 : j2 <= i__11; j2 += i__12) {
				sumr += data[j2];
				sumi += data[j2 + 1];
			    }
			    work[k] = sumr;
			    work[k + 1] = sumi;
			    goto L680;
L665:
			    kconj = k + ((n - kmin + 1) << 1);
			    j2 = j2max;
			    sumr = data[j2];
			    sumi = data[j2 + 1];
			    oldsr = (float)0.0;
			    oldsi = (float)0.0;
			    j2 -= ifp2;
L670:
			    tempr = sumr;
			    tempi = sumi;
			    sumr = twowr * sumr - oldsr + data[j2];
			    sumi = twowr * sumi - oldsi + data[j2 + 1];
			    oldsr = tempr;
			    oldsi = tempi;
			    j2 -= ifp2;
			    if (j2 - j3 <= 0) {
				goto L675;
			    } else {
				goto L670;
			    }
L675:
			    tempr = wr * sumr - oldsr + data[j2];
			    tempi = wi * sumi;
			    work[k] = tempr - tempi;
			    work[kconj] = tempr + tempi;
			    tempr = wr * sumi - oldsi + data[j2 + 1];
			    tempi = wi * sumr;
			    work[k + 1] = tempr + tempi;
			    work[kconj + 1] = tempr - tempi;
L680:
			    ;
			}
		    }
		    if (kmin - 1 <= 0) {
			goto L685;
		    } else {
			goto L686;
		    }
L685:
		    wr = wstpr + (float)1.0;
		    wi = wstpi;
		    goto L690;
L686:
		    tempr = wr;
		    wr = wr * wstpr - wi * wstpi + wr;
		    wi = tempr * wstpi + wi * wstpr + wi;
L690:
		    twowr = wr + wr;
		}
		if (icase - 3 != 0) {
		    goto L692;
		} else {
		    goto L691;
		}
L691:
		if (ifp1 - np2 >= 0) {
		    goto L692;
		} else {
		    goto L695;
		}
L692:
		k = 1;
		i2max = i3 + np2 - np1;
		i__10 = i2max;
		i__5 = np1;
		for (i2 = i3; i__5 < 0 ? i2 >= i__10 : i2 <= i__10; i2 += i__5) {
		    data[i2] = work[k];
		    data[i2 + 1] = work[k + 1];
		    k += 2;
		}
		goto L698;
L695:
		j3max = i3 + ifp2 - np1;
		i__5 = j3max;
		i__10 = np1;
		for (j3 = i3; i__10 < 0 ? j3 >= i__5 : j3 <= i__5; j3 += i__10) {
		    j2max = j3 + np2 - j2stp;
		    i__6 = j2max;
		    i__8 = j2stp;
		    for (j2 = j3; i__8 < 0 ? j2 >= i__6 : j2 <= i__6; j2 += i__8) {
			j1max = j2 + j1rg2 - ifp2;
			j1cnj = j3 + j2max + j2stp - j2;
			i__7 = j1max;
			i__9 = ifp2;
			for (j1 = j2; i__9 < 0 ? j1 >= i__7 : j1 <= i__7; j1 += i__9) {
			    k = j1 + 1 - i3;
			    data[j1] = work[k];
			    data[j1 + 1] = work[k + 1];
			    if (j1 - j2 <= 0) {
				goto L697;
			    } else {
				goto L696;
			    }
L696:
			    data[j1cnj] = work[k];
			    data[j1cnj + 1] = -work[k + 1];
L697:
			    j1cnj -= ifp2;
			}
		    }
		}
L698:
		;
	    }
	}
	++if_;
	ifp1 = ifp2;
	if (ifp1 - np1 <= 0) {
	    goto L700;
	} else {
	    goto L610;
	}
L700:
	switch ((int)icase) {
	    case 1:  goto L900;
	    case 2:  goto L800;
	    case 3:  goto L900;
	    case 4:  goto L701;
	}
L701:
	nhalf = n;
	n += n;
	theta = -twopi / (float) n;
	if (*ksign >= 0) {
	    goto L702;
	} else {
	    goto L703;
	}
L702:
	theta = -theta;
L703:
	sinth = (float)sin((double)(0.5 * theta));
	wstpr = sinth * (float)(-2. * sinth);
	wstpi = (float)sin((double)theta);
	wr = wstpr + (float)1.0;
	wi = wstpi;
	imin = 3;
	jmin = (nhalf << 1) - 1;
	goto L725;
L710:
	j = jmin;
	i__4 = ntot;
	i__3 = np2;
	for (i = imin; i__3 < 0 ? i >= i__4 : i <= i__4; i += i__3) {
	    sumr = (float)0.5 * (data[i] + data[j]);
	    sumi = (float)0.5 * (data[i + 1] + data[j + 1]);
	    difr = (float)0.5 * (data[i] - data[j]);
	    difi = (float)0.5 * (data[i + 1] - data[j + 1]);
	    tempr = wr * sumi + wi * difr;
	    tempi = wi * sumi - wr * difr;
	    data[i] = sumr + tempr;
	    data[i + 1] = difi + tempi;
	    data[j] = sumr - tempr;
	    data[j + 1] = -difi + tempi;
	    j += np2;
	}
	imin += 2;
	jmin += -2;
	tempr = wr;
	wr = wr * wstpr - wi * wstpi + wr;
	wi = tempr * wstpi + wi * wstpr + wi;
L725:
	if ((i__3 = imin - jmin) < 0) {
	    goto L710;
	} else if (i__3 == 0) {
	    goto L730;
	} else {
	    goto L740;
	}
L730:
	if (*ksign >= 0) {
	    goto L740;
	} else {
	    goto L731;
	}
L731:
	i__3 = ntot;
	i__4 = np2;
	for (i = imin; i__4 < 0 ? i >= i__3 : i <= i__3; i += i__4) {
	    data[i + 1] = -data[i + 1];
	}
L740:
	np2 += np2;
	ntot += ntot;
	j = ntot + 1;
	imax = ntot / 2 + 1;
L745:
	imin = imax - (nhalf << 1);
	i = imin;
	goto L755;
L750:
	data[j] = data[i];
	data[j + 1] = -data[i + 1];
L755:
	i += 2;
	j += -2;
	if (i - imax >= 0) {
	    goto L760;
	} else {
	    goto L750;
	}
L760:
	data[j] = data[imin] - data[imin + 1];
	data[j + 1] = (float)0.0;
	if (i - j >= 0) {
	    goto L780;
	} else {
	    goto L770;
	}
L765:
	data[j] = data[i];
	data[j + 1] = data[i + 1];
L770:
	i += -2;
	j += -2;
	if (i - imin <= 0) {
	    goto L775;
	} else {
	    goto L765;
	}
L775:
	data[j] = data[imin] + data[imin + 1];
	data[j + 1] = (float)0.0;
	imax = imin;
	goto L745;
L780:
	data[1] += data[2];
	data[2] = (float)0.0;
	goto L900;
L800:
	if (i1rng - np1 >= 0) {
	    goto L900;
	} else {
	    goto L805;
	}
L805:
	i__4 = ntot;
	i__3 = np2;
	for (i3 = 1; i__3 < 0 ? i3 >= i__4 : i3 <= i__4; i3 += i__3) {
	    i2max = i3 + np2 - np1;
	    i__2 = i2max;
	    i__9 = np1;
	    for (i2 = i3; i__9 < 0 ? i2 >= i__2 : i2 <= i__2; i2 += i__9) {
		imin = i2 + i1rng;
		imax = i2 + np1 - 2;
		jmax = (i3 << 1) + np1 - imin;
		if (i2 - i3 <= 0) {
		    goto L820;
		} else {
		    goto L810;
		}
L810:
		jmax += np2;
L820:
		if (idim - 2 <= 0) {
		    goto L850;
		} else {
		    goto L830;
		}
L830:
		j = jmax + np0;
		i__7 = imax;
		for (i = imin; i <= i__7; i += 2) {
		    data[i] = data[j];
		    data[i + 1] = -data[j + 1];
		    j += -2;
		}
L850:
		j = jmax;
		i__7 = imax;
		i__8 = np0;
		for (i = imin; i__8 < 0 ? i >= i__7 : i <= i__7; i += i__8) {
		    data[i] = data[j];
		    data[i + 1] = -data[j + 1];
		    j -= np0;
		}
	    }
	}
L900:
	np0 = np1;
	np1 = np2;
	nprev = n;
    }
L920:
    return 0;
} /* fourt_ */
