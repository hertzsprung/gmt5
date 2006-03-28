/*--------------------------------------------------------------------
 *	$Id: gmt_init.c,v 1.222 2006-03-28 07:33:22 pwessel Exp $
 *
 *	Copyright (c) 1991-2006 by P. Wessel and W. H. F. Smith
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
 *--------------------------------------------------------------------
 *
 * gmt_init.c contains code which is used by all GMT programs
 *
 * Author:	Paul Wessel
 * Date:	15-FEB-2000
 * Version:	4
 *
 *
 * The PUBLIC functions are:
 *
 *	GMT_explain_option ()	:	Prints explanations for the common options
 *	GMT_get_common_args ()	:	Interprets -B -H -J -K -O -P -R -U -V -X -Y -: -c
 *	GMT_getdefaults ()	:	Initializes the GMT global parameters
 *	GMT_free_plot_array	:	Free plot memory
 *	GMT_key_lookup ()	:	Linear Key - id lookup function
 *	GMT_savedefaults ()	:	Writes the GMT global parameters to .gmtdefaults4
 *	GMT_hash_init ()	: 	Initializes a hash
 *	GMT_hash_lookup ()	:	Key - id lookup using hashing
 *	GMT_hash ()		:	Key - id lookup using hashing
 *	GMT_begin ()		:	Gets history and init parameters
 *	GMT_end ()		:	Cleans up and exits
 *	GMT_get_history ()	:	Read the .gmtcommands4 file
 *	GMT_putpen		:	Encode pen argument into textstring
 *	GMT_put_history ()	:	Writes updates to the .gmtcommands file
 *	GMT_map_getproject ()	:	Scans the -Jstring to set projection
 *
 * The INTERNAL functions are:
 *
 *	GMT_loaddefaults ()	:	Reads the GMT global parameters from .gmtdefaults4
 *	GMT_map_getframe ()	:	Scans the -Bstring to set tickinfo
 *	GMT_setparameter ()	:	Sets a default value given keyword,value-pair
 *	GMT_setshorthand ()	:	Reads and initializes the suffix shorthands
 *	GMT_get_ellipsoid()	:	Returns ellipsoid id based on name
 *	GMT_prepare_3D ()	:	Initialize 3-D parameters
 *	GMT_scanf_epoch ()	:	Get user time origin from user epoch string
 *	GMT_init_time_system_structure ()  Does what it says
 */

#define GMT_WITH_NO_PS
#include "gmt.h"
#include "gmt_globals.h"

/*--------------------------------------------------------------------*/
/* Load parameters from include files */
/*--------------------------------------------------------------------*/

#include "gmt_keycases.h"		/* Get all the default case values */

char *GMT_unique_option[N_UNIQUE] = {	/* The common GMT commandline options */
#include "gmt_unique.h"
};
 
char *GMT_keywords[N_KEYS] = {		/* Names of all parameters in .gmtdefaults4 */
#include "gmt_keywords.h"
};

#define GMT_N_MEDIA 29				/* Number of standard paper formats in the GMT_media_names.h include file */

char *GMT_media_name[GMT_N_MEDIA] = {		/* Names of all paper formats */
#include "gmt_media_name.h"
};

struct GMT_MEDIA {	/* Holds information about paper sizes in points */
	int width;		/* Width in points */
	int height;		/* Height in points */
};
struct GMT_MEDIA GMT_media[GMT_N_MEDIA] = {			/* Sizes in points of all paper formats */
#include "gmt_media_size.h"
};

/* For Custom paper types */

char **GMT_user_media_name = (char **)NULL;
struct GMT_MEDIA *GMT_user_media = (struct GMT_MEDIA *)NULL;
int GMT_n_user_media = 0;

#define USER_MEDIA_OFFSET 1000

char *GMT_color_name[GMT_N_COLOR_NAMES] = {	/* Names of X11 colors */
#include "gmt_colornames.h"
};

char *GMT_weekdays[7] = {	/* Days of the week in English */
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};

char *GMT_degree_choice[4] = {  /* Users choice for degree symbol */
	"none",
	"ring",
	"degree",
	"colon",
};

EXTERN_MSC void GMT_grdio_init (void);	/* Defined in gmt_customio.c and only used here */

int GMT_setparameter(char *keyword, char *value);
int GMT_load_user_media (void);
BOOLEAN true_false_or_error (char *value, int *answer);
void GMT_get_history(int argc, char **argv);
void GMT_prepare_3D(void);
void GMT_free_plot_array(void);
char *GMT_putpen (struct GMT_PEN *pen);
char *GMT_getdefpath (int get);
void GMT_get_time_language (char *name);
void GMT_init_time_system_structure ();
#ifndef OLDCAL
int GMT_scanf_epoch (char *s, int *day, double *t0);
#else
int GMT_scanf_epoch (char *s, double *t0);
#endif
void GMT_backwards_compatibility ();
void GMT_strip_colonitem (const char *in, const char *pattern, char *item, char *out);
void GMT_strip_wesnz (const char *in, int side[], BOOLEAN *draw_box, char *out);
void GMT_split_info (const char *in, char *info[]);
void GMT_decode_tinfo (char *in, struct PLOT_AXIS *A);
void GMT_set_titem (struct PLOT_AXIS *A, double val, double phase, char flag, char unit);
int GMT_map_getframe (char *in);
static void load_encoding (struct gmt_encoding *);
void GMT_verify_encodings ();
void GMT_prep_PS_bits ();
int GMT_key_lookup (char *name, char **list, int n);

/* Local variables to gmt_init.c */

struct GMT_HASH hashnode[HASH_SIZE];
BOOLEAN GMT_x_abs = FALSE, GMT_y_abs = FALSE;
BOOLEAN GMT_got_frame_rgb;
FILE *GMT_fp_history;	/* For .gmtcommands4 file */
struct GMT_BACKWARD {	/* Used to ensure backwards compatibility */
	BOOLEAN got_old_plot_format;		/* TRUE if DEGREE_FORMAT was decoded */
	BOOLEAN got_old_degree_symbol;		/* TRUE if DEGREE_FORMAT was decoded */
	BOOLEAN got_new_plot_format;		/* TRUE if PLOT_DEGREE_FORMAT was decoded */
	BOOLEAN got_new_degree_symbol;		/* TRUE if DEGREE_SYMBOL was decoded */
	BOOLEAN got_old_want_euro;		/* TRUE if WANT_EURO_FONTS was decoded */
	BOOLEAN got_new_char_encoding;		/* TRUE if CHAR_ENCODING was decoded */
} GMT_backward;

BOOLEAN GMT_force_resize = FALSE, GMT_annot_special = FALSE;
double save_annot_size[2], save_label_size, save_header_size;
double save_annot_offset[2], save_label_offset, save_header_offset, save_tick_length, save_frame_width;
BOOLEAN GMT_getuserpath (char *stem, char *path);
BOOLEAN GMT_primary;
char month_names[12][16], *months[12];

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void GMT_explain_option (char option)
{

	/* The function print to stderr a short explanation for the option indicated by
	 * the variable <option>.  Only the common parameter options are covered
	 */

	char *GMT_choice[2] = {"OFF", "ON"};

	switch (option) {

		case 'B':	/* Tickmark option */

			fprintf (stderr, "\t-B specifies Basemap frame info.  <tickinfo> is a textstring made up of one or\n");
			fprintf (stderr, "\t   more substrings of the form [t]<stride>[+-<phase>][<unit>], where the (optional) [t] is the\n");
			fprintf (stderr, "\t   axis item type, <stride> is the spacing between ticks or annotations, the (optional)\n");
			fprintf (stderr, "\t   <phase> specifies phase-shifted annoations by that amount, and the (optional)\n");
			fprintf (stderr, "\t   <unit> specifies the <stride> unit [Default is unit implied in -R]. There can be\n");
			fprintf (stderr, "\t   no spaces between the substrings - just append to make one very long string.\n");
			fprintf (stderr, "\t   -B[p] means (p)rimary annotations; use -Bs to specify (s)econdary annotations.\n");
			fprintf (stderr, "\t   Three axis item types exist:\n");
			fprintf (stderr, "\t     a: tick annotation stride.\n");
			fprintf (stderr, "\t     f: frame tick stride.\n");
			fprintf (stderr, "\t     g: grid line stride.\n");
			fprintf (stderr, "\t   The optional [<unit>] modifies the <stride> value accordingly.  For maps, you may use\n");
			fprintf (stderr, "\t     m: arc minutes [Default unit is degree].\n");
			fprintf (stderr, "\t     c: arc seconds.\n");
			fprintf (stderr, "\t   For time axes, several units are recognized:\n");
			fprintf (stderr, "\t     Y: year - plot using all 4 digits.\n");
			fprintf (stderr, "\t     y: year - plot only last 2 digits.\n");
			fprintf (stderr, "\t     O: month - format annotation according to PLOT_DATE_FORMAT.\n");
			fprintf (stderr, "\t     o: month - plot as 2-digit integer (1-12).\n");
			fprintf (stderr, "\t     U: ISO week - format annotation according to PLOT_DATE_FORMAT.\n");
			fprintf (stderr, "\t     u: ISO week - plot as 2-digit integer (1-53).\n");
			fprintf (stderr, "\t     r: Gregorian week - 7-day stride from chosen start of week (%s).\n", GMT_weekdays[gmtdefs.time_week_start]);
			fprintf (stderr, "\t     K: ISO weekday - format annotation according to PLOT_DATE_FORMAT.\n");
			fprintf (stderr, "\t     k: weekday - plot name of weekdays in selected language [%s].\n", gmtdefs.time_language);
			fprintf (stderr, "\t     D: day  - format annotation according to PLOT_DATE_FORMAT, which also determines whether\n");
			fprintf (stderr, "\t               we should plot day of month (1-31) or day of year (1-366).\n");
			fprintf (stderr, "\t     d: day - plot as 2- (day of month) or 3- (day of year) integer.\n");
			fprintf (stderr, "\t     R: Same as d but annotates from start of Gregorian week.\n");
			fprintf (stderr, "\t     H: hour - format annotation according to PLOT_CLOCK_FORMAT.\n");
			fprintf (stderr, "\t     h: hour - plot as 2-digit integer (0-23).\n");
			fprintf (stderr, "\t     M: minute - format annotation according to PLOT_CLOCK_FORMAT.\n");
			fprintf (stderr, "\t     m: minute - plot as 2-digit integer (0-59).\n");
			fprintf (stderr, "\t     C: second - format annotation according to PLOT_CLOCK_FORMAT.\n");
			fprintf (stderr, "\t     c: second - plot as 2-digit integer (0-59; 60-61 if leap seconds are enabled).\n");
			fprintf (stderr, "\t   Specify an axis label by surrounding it with colons (e.g., :\"my x label\":).\n");
			fprintf (stderr, "\t   To prepend a prefix to each annotation (e.g., $ 10, $ 20 ...) add a prefix that begins\n");
			fprintf (stderr, "\t     with the equal-sign (=); the rest is used as annotation prefix (e.g. :=\'$\':). If the prefix has\n");
			fprintf (stderr, "\t     a leading hyphen (-) there will be no space between prefix and annotation (e.g., :=-\'$\':).\n");
			fprintf (stderr, "\t   To append a unit to each annotation (e.g., 5 km, 10 km ...) add a label that begins\n");
			fprintf (stderr, "\t     with a comma; the rest is used as unit annotation (e.g. :\",km\":). If the unit has\n");
			fprintf (stderr, "\t     a leading hyphen (-) there will be no space between unit and annotation (e.g., :,-%%:).\n");
			fprintf (stderr, "\t   For separate x and y [and z if -Jz is used] tickinfo, separate the strings with slashes [/].\n");
			fprintf (stderr, "\t   Specify an plot title by adding a label whose first character is a period; the rest\n");
			fprintf (stderr, "\t     of the label is used as the title (e.g. :\".My Plot Title\":).\n");
			fprintf (stderr, "\t   Append any combination of W, E, S, N, Z to annotate those axes only [Default is WESNZ (all)].\n");
			fprintf (stderr, "\t     Use lower case w, e, s, n, z to draw & tick but not to annotate those axes.\n");
			fprintf (stderr, "\t     Z+ will also draw a 3-D box .\n");
			fprintf (stderr, "\t   Log10 axis: Append l to annotate log10 (x) or p for 10^(log10(x)) [Default annotates x].\n");
			fprintf (stderr, "\t   Power axis: append p to annotate x at equidistant pow increments [Default is nonlinear].\n");
			fprintf (stderr, "\t   See psbasemap man pages for more details and examples of all settings.\n");
			break;

		case 'b':	/* Condensed tickmark option */

			fprintf (stderr, "\t-B Boundary annotation, give -B[p|s]<xinfo>[/<yinfo>[/<zinfo>]][.:\"title\":][wesnzWESNZ+]\n");
			fprintf (stderr, "\t   <?info> is 1-3 substring(s) of form [<type>]<stride>[<unit>][l|p][:\"label\":][:,[-]\"unit\":]\n");
			fprintf (stderr, "\t   See psbasemap man pages for more details and examples of all settings.\n");
			break;

		case 'H':	/* Header */

			fprintf (stderr, "\t-H[i][n_rec] means input/output file has %d Header record(s) [%s]\n",
				gmtdefs.n_header_recs, GMT_choice[gmtdefs.io_header[GMT_IN]]);
			fprintf (stderr, "\t   Optionally, append i for input only and/or number of header records\n");
			break;

		case 'J':	/* Map projection option */

			fprintf (stderr, "\t-J Selects the map proJection system. (<width> is in %s)\n", GMT_unit_names[gmtdefs.measure_unit]);
			fprintf (stderr, "\t   Append h for map height, + for max map dimension, and - for min map dimension.\n");
			fprintf (stderr, "\t   Azimuthal projections set -Rg unless polar aspect or -R<...>r is given.\n\n");

			fprintf (stderr, "\t   -Ja<lon0>/<lat0>/<scale> OR -JA<lon0>/<lat0>/<width> (Lambert Azimuthal Equal Area)\n");
			fprintf (stderr, "\t     lon0/lat0 is the center or the projection.\n");
			fprintf (stderr, "\t     Scale is either <1:xxxx> or <radius>/<lat>, where <radius> distance\n");
			fprintf (stderr, "\t     is in %s to the oblique parallel <lat>.\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jb<lon0>/<lat0>/<lat1>/<lat2>/<scale> OR -JB<lon0>/<lat0>/<lat1>/<lat2>/<width> (Albers Equal-Area Conic)\n");
			fprintf (stderr, "\t     Give origin, 2 standard parallels, and true scale in %s/degree\n",
				GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jc<lon0>/<lat0><scale> OR -JC<lon0>/<lat0><width> (Cassini)\n");
			fprintf (stderr, "\t     Give central point and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jd<lon0>/<lat0>/<lat1>/<lat2>/<scale> OR -JD<lon0>/<lat0>/<lat1>/<lat2>/<width> (Equidistant Conic)\n");
			fprintf (stderr, "\t     Give origin, 2 standard parallels, and true scale in %s/degree\n",
				GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Je<lon0>/<lat0>/<scale> OR -JE<lon0>/<lat0>/<width> (Azimuthal Equidistant)\n");
			fprintf (stderr, "\t     lon0/lat0 is the center or the projection.\n");
			fprintf (stderr, "\t     Scale is either <1:xxxx> or <radius>/<lat>, where <radius> is distance\n");
			fprintf (stderr, "\t     in %s to the oblique parallel <lat>. \n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jf<lon0>/<lat0>/<horizon>/<scale> OR -JF<lon0>/<lat0>/<horizon>/<width> (Gnomonic)\n");
			fprintf (stderr, "\t     lon0/lat0 is the center or the projection.\n");
			fprintf (stderr, "\t     horizon is max distance from center of the projection (< 90).\n");
			fprintf (stderr, "\t     Scale is either <1:xxxx> or <radius>/<lat>, where <radius> is distance\n");
			fprintf (stderr, "\t     in %s to the oblique parallel <lat>. \n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jg<lon0>/<lat0>/<scale> OR -JG<lon0>/<lat0>/<width> (Orthographic)\n");
			fprintf (stderr, "\t     lon0/lat0 is the center or the projection.\n");
			fprintf (stderr, "\t     Scale is either <1:xxxx> or <radius>/<lat>, where <radius> is distance\n");
			fprintf (stderr, "\t     in %s to the oblique parallel <lat>. \n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jh<lon0>/<scale> OR -JH<lon0>/<width> (Hammer-Aitoff)\n");
			fprintf (stderr, "\t     Give central meridian and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Ji<lon0>/<scale> OR -JI<lon0>/<width> (Sinusoidal)\n");
			fprintf (stderr, "\t     Give central meridian and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jj<lon0>/<scale> OR -JJ<lon0>/<width> (Miller projection)\n");
			fprintf (stderr, "\t     Give central meridian and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jk[f|s]<lon0>/<scale> OR -JK[f|s]<lon0>/<width> (Eckert IV (f) or VI (s))\n");
			fprintf (stderr, "\t     Give central meridian and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jl<lon0>/<lat0>/<lat1>/<lat2>/<scale> OR -JL<lon0>/<lat0>/<lat1>/<lat2>/<width> (Lambert Conformal Conic)\n");
			fprintf (stderr, "\t     Give origin, 2 standard parallels,  and true scale in %s/degree\n",
				GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jm | -JM (Mercator).  Specify one of two definitions:\n");
			fprintf (stderr, "\t      -Jm<scale> OR -JM<width>\n");
			fprintf (stderr, "\t       Give true scale at Equator in %s/degree\n",
				GMT_unit_names[gmtdefs.measure_unit]);
			fprintf (stderr, "\t      -Jm<lon0>/<lat0>/<scale> OR -JM<lon0>/<lat0>/<width>\n");
			fprintf (stderr, "\t       Give true scale at parallel lat0 in %s/degree\n",
				GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jn<lon0>/<scale> OR -JN<lon0>/<width> (Robinson projection)\n");
			fprintf (stderr, "\t     Give central meridian and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jo | -JO (Oblique Mercator).  Specify one of three definitions:\n");
			fprintf (stderr, "\t      -Joa<orig_lon>/<orig_lat>/<azimuth>/<scale> OR -JOa<orig_lon>/<orig_lat>/<azimuth>/<width>\n");
			fprintf (stderr, "\t      		Give origin and azimuth of oblique equator\n");
			fprintf (stderr, "\t      -Job<orig_lon>/<orig_lat>/<b_lon>/<b_lat>/<scale> OR -JOb<orig_lon>/<orig_lat>/<b_lon>/<b_lat>/<width>\n");
			fprintf (stderr, "\t      		Give origin and second point on oblique equator\n");
			fprintf (stderr, "\t      -Joc<orig_lon>/<orig_lat>/<pole_lon>/<pole_lat>/<scale> OR -JOc<orig_lon>/<orig_lat>/<pole_lon>/<pole_lat>/<width>\n");
			fprintf (stderr, "\t      		Give origin and pole of projection\n");
			fprintf (stderr, "\t        Scale is true scale at oblique equator in %s/degree\n",
				GMT_unit_names[gmtdefs.measure_unit]);
			fprintf (stderr, "\t        Specify region in oblique degrees OR use -R<>r\n");

			fprintf (stderr, "\t   -Jq<lon0>/<scale> OR -JQ<lon0>/<width> (Equidistant Cylindrical)\n");
			fprintf (stderr, "\t     Give central meridian and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jr<lon0>/<scale> OR -JR<lon0>/<width> (Winkel Tripel)\n");
			fprintf (stderr, "\t     Give central meridian and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Js<lon0>/<lat0>/<scale> OR -JS<lon0>/<lat0>/<width> (Stereographic)\n");
			fprintf (stderr, "\t     lon0/lat0 is the center or the projection.\n");
			fprintf (stderr, "\t     Scale is either <1:xxxx> (true at pole) or <slat>/<1:xxxx> (true at <slat>)\n");
			fprintf (stderr, "\t     or <radius>/<lat> (distance in %s to the [oblique] parallel <lat>.\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jt | -JT (Transverse Mercator).  Specify one of two definitions:\n");
			fprintf (stderr, "\t      -Jt<lon0>/<scale> OR -JT<lon0>/<width>\n");
			fprintf (stderr, "\t         Give central meridian and scale as 1:xxxx or %s/degree\n",
				GMT_unit_names[gmtdefs.measure_unit]);
			fprintf (stderr, "\t      -Jt<lon0>/<lat0>/<scale> OR -JT<lon0>/<lat0>/<width>\n");
			fprintf (stderr, "\t         Give lon/lat of origin, and scale as 1:xxxx or %s/degree\n",
				GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Ju<zone>/<scale> OR -JU<zone>/<width> (UTM)\n");
			fprintf (stderr, "\t     Give zone (1-60, negative for S hemisphere) and scale as 1:xxxx or %s/degree\n",
				GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jv<lon0>/<scale> OR -JV<lon0>/<width> (van der Grinten)\n");
			fprintf (stderr, "\t     Give central meridian and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jw<lon0>/<scale> OR -JW<lon0>/<width> (Mollweide)\n");
			fprintf (stderr, "\t     Give central meridian and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);

			fprintf (stderr, "\t   -Jy<lon0>/<lats>/<scale> OR -JY<lon0>/<lats>/<width> (Cylindrical Equal-area)\n");
			fprintf (stderr, "\t     Give central meridian, standard parallel and scale as 1:xxxx or %s/degree\n", GMT_unit_names[gmtdefs.measure_unit]);
			fprintf (stderr, "\t     <slat> = 45 (Peters), 37.4 (Trystan Edwards), 30 (Behrmann), 0 (Lambert)\n");

			fprintf (stderr, "\t   -Jp[a]<scale>[/<base>][r|z] OR -JP[a]<width>[/<base>][r|z] (Polar (theta,radius))\n");
			fprintf (stderr, "\t     Linear scaling for polar coordinates.\n");
			fprintf (stderr, "\t     Optionally append 'a' to -Jp or -JP to use azimuths (CW from North) instead of directions (CCW from East) [default].\n");
			fprintf (stderr, "\t     Give scale in %s/units, and append theta value for angular offset (base) [0]\n", GMT_unit_names[gmtdefs.measure_unit]);
			fprintf (stderr, "\t     Append r to reverse radial direction (s/n must be in 0-90 range) or z to annotate depths rather than radius [Default]\n");

			fprintf (stderr, "\t   -Jx<x-scale>[/<y-scale>] OR -JX<width>[/<height]> (Linear, log, power scaling)\n");
			fprintf (stderr, "\t     Scale in %s/units (or 1:xxxx). Optionally, append to scale:\n",
				GMT_unit_names[gmtdefs.measure_unit]);
			fprintf (stderr, "\t       d         Geographic coordinate (in degrees)\n");
			fprintf (stderr, "\t       l         Log10 projection\n");
			fprintf (stderr, "\t       p<power>  x^power projection\n");
			fprintf (stderr, "\t       t         Calendar time projection using relative time coordinates\n");
			fprintf (stderr, "\t       T         Calendar time projection using absolute time coordinates\n");
			fprintf (stderr, "\t     Use / to specify separate x/y scaling (e.g., -Jx0.5/0.3.).  Not allowed with 1:xxxxx.\n");
			fprintf (stderr, "\t     If -JX is used then give axes lengths rather than scales.\n");
			break;

		case 'j':	/* Condensed version of J */

			fprintf (stderr, "\t-J Selects map proJection. (<scale> in %s/degree, <width> in %s)\n", GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
			fprintf (stderr, "\t   Append h for map height, + for max map dimension, and - for min map dimension.\n");
			fprintf (stderr, "\t   Azimuthal projections set -Rg unless polar aspect or -R<...>r is given.\n\n");

			fprintf (stderr, "\t   -Ja|A<lon0>/<lat0>/<scale (or radius/lat)|width> (Lambert Azimuthal Equal Area)\n");

			fprintf (stderr, "\t   -Jb|B<lon0>/<lat0>/<lat1>/<lat2>/<scale|width> (Albers Equal-Area Conic)\n");
			fprintf (stderr, "\t   -Jc|C<lon0>/<lat0><scale|width> (Cassini)\n");

			fprintf (stderr, "\t   -Jd|D<lon0>/<lat0>/<lat1>/<lat2>/<scale|width> (Equidistant Conic)\n");

			fprintf (stderr, "\t   -Je|E<lon0>/<lat0>/<scale (or radius/lat)|width>  (Azimuthal Equidistant)\n");

			fprintf (stderr, "\t   -Jf|F<lon0>/<lat0>/<horizon>/<scale (or radius/lat)|width>  (Gnomonic)\n");

			fprintf (stderr, "\t   -Jg|G<lon0>/<lat0>/<scale (or radius/lat)|width>  (Orthographic)\n");

			fprintf (stderr, "\t   -Jh|H<lon0>/<scale|width> (Hammer-Aitoff)\n");

			fprintf (stderr, "\t   -Ji|I<lon0>/<scale|width> (Sinusoidal)\n");

			fprintf (stderr, "\t   -Jj|J<lon0>/<scale|width> (Miller)\n");

			fprintf (stderr, "\t   -Jk|K[f|s]<lon0>/<scale/width> (Eckert IV (f) or VI (s))\n");


			fprintf (stderr, "\t   -Jl|L<lon0>/<lat0>/<lat1>/<lat2>/<scale|width> (Lambert Conformal Conic)\n");

			fprintf (stderr, "\t   -Jm|M (Mercator).  Specify one of two definitions:\n");
			fprintf (stderr, "\t      -Jm|M<scale|width>\n");
			fprintf (stderr, "\t      -Jm|M<lon0>/<lat0>/<scale|width>\n");

			fprintf (stderr, "\t   -Jn|N<lon0>/<scale|width> (Robinson projection)\n");

			fprintf (stderr, "\t   -Jo|O (Oblique Mercator).  Specify one of three definitions:\n");
			fprintf (stderr, "\t      -Jo|Oa<orig_lon>/<orig_lat>/<azimuth>/<scale|width>\n");
			fprintf (stderr, "\t      -Jo|Ob<orig_lon>/<orig_lat>/<b_lon>/<b_lat>/<scale|width>\n");
			fprintf (stderr, "\t      -Jo|Oc<orig_lon>/<orig_lat>/<pole_lon>/<pole_lat>/<scale|width>\n");

			fprintf (stderr, "\t   -Jq|Q<lon0>/<scale|width> (Equidistant Cylindrical)\n");

			fprintf (stderr, "\t   -Jr|R<lon0>/<scale|width> (Winkel Tripel)\n");

			fprintf (stderr, "\t   -Js|S<lon0>/<lat0>/[<slat>/]<scale (or radius/lat)|width> (Stereographic)\n");

			fprintf (stderr, "\t   -Jt|T (Transverse Mercator).  Specify one of two definitions:\n");
			fprintf (stderr, "\t      -Jt|T<lon0>/<scale|width>\n");
			fprintf (stderr, "\t      -Jt|T<lon0>/<lat0>/<scale|width>\n");

			fprintf (stderr, "\t   -Ju|U<zone>/<scale|width> (UTM)\n");

			fprintf (stderr, "\t   -Jv|V<lon0>/<scale/width> (van der Grinten)\n");

			fprintf (stderr, "\t   -Jw|W<lon0>/<scale|width> (Mollweide)\n");

			fprintf (stderr, "\t   -Jy|Y<lon0>/<lats>/<scale|width> (Cylindrical Equal-area)\n");

			fprintf (stderr, "\t   -Jp|P[a]<scale|width>[/<origin>][r|z] (Polar [azimuth] (theta,radius))\n");

			fprintf (stderr, "\t   -Jx|X<x-scale|width>[d|l|p<power>|t|T][/<y-scale|height>[d|l|p<power>|t|T]] (Linear, log, and power projections)\n");
			fprintf (stderr, "\t   (See psbasemap for more details on projection syntax)\n");
			break;

		case 'K':	/* Append-more-PostScript-later */

			fprintf (stderr, "\t-K means allow for more plot code to be appended later [%s].\n",
				GMT_choice[!gmtdefs.last_page]);
			break;

		case 'M':	/* Multisegment option */

			fprintf (stderr, "\t-M Input/output file(s) contain multiple segments separated by a record\n");
			fprintf (stderr, "\t   whose first character is <flag> [%c]\n", GMT_io.EOF_flag[GMT_IN]);
			fprintf (stderr, "\t   Use -Mi or -Mo to give different settings for input and output [Same]\n");
			break;

		case 'O':	/* Overlay plot */

			fprintf (stderr, "\t-O means Overlay plot mode [%s].\n",
				GMT_choice[gmtdefs.overlay]);
			break;

		case 'P':	/* Portrait or landscape */

			fprintf (stderr, "\t-P means Portrait page orientation [%s].\n",
				GMT_choice[(gmtdefs.page_orientation & 1)]);
			break;

		case 'R':	/* Region option */

			fprintf (stderr, "\t-R specifies the min/max coordinates of data region in user units.\n");
			fprintf (stderr, "\t   Use dd:mm[:ss] format for regions given in degrees and minutes [and seconds].\n");
			fprintf (stderr, "\t   Use [yyy[-mm[-dd]]]T[hh[:mm[:ss[.xxx]]]] format for time axes.\n");
			fprintf (stderr, "\t   Append r if -R specifies the longitudes/latitudes of the lower left\n");
			fprintf (stderr, "\t   and upper right corners of a rectangular area.\n");
			fprintf (stderr, "\t   -Rg -Rd are accepted shorthands for -R0/360/-90/90 -R-180/180/-90/90\n");
			break;

		case 'r':	/* Region option for 3-D */

			fprintf (stderr, "\t-R specifies the xyz min/max coordinates of the plot window in user units.\n");
			fprintf (stderr, "\t   Use dd:mm[:ss] format for regions given in degrees and minutes [and seconds].\n");
			fprintf (stderr, "\t   Append r if first 4 arguments to -R specify the longitudes/latitudes\n");
			fprintf (stderr, "\t   of the lower left and upper right corners of a rectangular area\n");
			break;

		case 'U':	/* Plot time mark and [optionally] command line */

			fprintf (stderr, "\t-U to plot Unix System Time stamp [and optionally appended text].\n");
			fprintf (stderr, "\t   You may also set the lower left corner position of stamp [%g/%g].\n",
				gmtdefs.unix_time_pos[0], gmtdefs.unix_time_pos[1]);
			fprintf (stderr, "	   Give -Uc to have the command line plotted [%s].\n",
				GMT_choice[gmtdefs.unix_time]);
			break;

		case 'V':	/* Verbose */

			fprintf (stderr, "\t-V Run in verbose mode [%s].\n", GMT_choice[gmtdefs.verbose]);
			break;

		case 'X':
		case 'Y':	/* Reset plot origin option */

			fprintf (stderr, "\t-X -Y to shift origin of plot to (<xshift>, <yshift>) [a%g,a%g].\n",
				gmtdefs.x_origin, gmtdefs.y_origin);
			fprintf (stderr, "\t   Prepend a for absolute [Default r is relative]\n");
			fprintf (stderr, "\t   (Note that for overlays (-O), the default is [r0,r0].)\n");
			fprintf (stderr, "\t   Give c to center plot on page in x and/or y.)\n");
			break;

		case 'Z':	/* Vertical scaling for 3-D plots */

			fprintf (stderr, "\t   -Jz for z component of 3-D projections.  Same syntax as -Jx.\n");
			break;

		case 'c':	/* Set number of plot copies option */

			fprintf (stderr, "\t-c specifies the number of copies [%d].\n", gmtdefs.n_copies);
			break;

		case 'i':	/* -b binary option with input only */

			fprintf (stderr, "\t-bi for binary input.  Append s for single precision [Default is double]\n");
			break;

		case 'n':	/* -bi addendum when input format is unknown */

			fprintf (stderr, "\t    Append <n> for the number of columns in binary file(s).\n");
			break;

		case 'o':	/* -b binary option with output only */

			fprintf (stderr, "\t-bo for binary output. Append s for single precision [Default is double]\n");
			break;

		case ':':	/* lon/lat or lat/lon */

			fprintf (stderr, "\t-: Expect lat/lon input/output rather than lon/lat [%s/%s].\n",
				GMT_choice[gmtdefs.xy_toggle[0]], GMT_choice[gmtdefs.xy_toggle[1]]);
			break;

		case 'f':	/* -f option to tell GMT which columns are time (and optionally geographical) */

			fprintf (stderr, "\t-f Special formatting of input/output columns (e.g., time or geographical)\n");
			fprintf (stderr, "\t   Specify i(nput) or o(utput) [Default is both input and output]\n");
			fprintf (stderr, "\t   Give one or more columns (or column ranges) separated by commas.\n");
			fprintf (stderr, "\t   Append T (Calendar format), t (time relative to TIME_EPOCH), f (plain floating point)\n");
			fprintf (stderr, "\t   x (longitude), y (latitude) to each col/range item.\n");
			fprintf (stderr, "\t   -f[i|o]g means -f[i|o]0x,1y (geographic coordinates).\n");
			break;

		case '.':	/* Trailer message */

			fprintf (stderr, "\t(See gmtdefaults man page for hidden GMT default parameters)\n");
			break;

		default:
			break;
	}
}

void GMT_label_syntax (int indent, int kind)
{
	/* Contour/line specifications in *contour and psxy[z]
	 * indent is the number of spaces to indent after the TAB.
	 * kind = 0 for *contour and 1 for psxy[z]
	 */

	int i;
	char pad[16];

	pad[0] = '\t';	for (i = 1; i <= indent; i++) pad[i] = ' ';	pad[i] = '\0';
	fprintf (stderr, "%s +a<angle> for annotations at a fixed angle, +an for line-normal, or +ap for line-parallel [Default]\n", pad);
	if (kind == 0) fprintf (stderr, "%s   For +ap, optionally append u for up-hill and d for down-hill cartographic annotations\n", pad);
	fprintf (stderr, "%s +c<dx>[/<dy>] to change the clearance between label and text box [15%%]\n", pad);
	fprintf (stderr, "%s +f followed by desired label font [Default is %d].\n", pad, gmtdefs.annot_font[0]);
	fprintf (stderr, "%s +g[<color>] for opaque text box [Default is transparent]; optionally give color [white]\n", pad);
	fprintf (stderr, "%s +j<just> to set label justification [Default is CM]\n", pad);
	fprintf (stderr, "%s +k<color> to change color of label text [Default is black]\n", pad);
	if (kind == 1) {
		fprintf (stderr, "%s +l<label> Use this fixed text as the label (quote text if containing spaces).\n", pad);
		fprintf (stderr, "%s +L<d|D|f|h|n|N|x>[<unit>] Sets the label according to the given flag:\n", pad);
		fprintf (stderr, "%s   d Cartesian plot distance; append desired unit c, i, m, or p.\n", pad);
		fprintf (stderr, "%s   D Map distance; append desired unit d, e, k, m, or n.\n", pad);
		fprintf (stderr, "%s   f Label is text after 2nd column the <ffile.d> fixed label location file.\n", pad);
		fprintf (stderr, "%s   h Use multisegment header labels (either in -Lstring or first word).\n", pad);
		fprintf (stderr, "%s   n Use the current multisegment number (starting at 0).\n", pad);
		fprintf (stderr, "%s   N Use current file number / multisegment number (starting at 0/0).\n", pad);
		fprintf (stderr, "%s   x Like h, but scan headers in the <xfile.d> crossing lines instead.\n", pad);
	}
	fprintf (stderr, "%s +o to use rounded rectangular text box [Default is rectangular]\n", pad);
	fprintf (stderr, "%s +p[<pen>] draw outline of textbox  [Default is no outline]; optionally give pen [Default is default pen]\n", pad);
	fprintf (stderr, "%s +r<min_rad> places no labels where radius of curvature < <min_rad> [Default is 0].\n", pad);
	fprintf (stderr, "%s +s followed by desired font size in points [Default is 9 point].\n", pad);
	fprintf (stderr, "%s +u<unit> to append unit to labels; Start with - for no space between annotation and unit.\n", pad);
	if (kind == 0) fprintf (stderr, "%s  If no unit appended, use z-unit from grdfile. [Default is no unit]\n", pad);
	fprintf (stderr, "%s +v for placing curved text along path [Default is straight]\n", pad);
	fprintf (stderr, "%s +w to set how many (x,y) points to use for angle calculation [Default is 10]\n", pad);
	fprintf (stderr, "%s +=<prefix> to give labels a prefix; Start with - for no space between annotation and prefix.\n", pad);
}

void GMT_cont_syntax (int indent, int kind)
{
	/* Contour/line label placement specifications in *contour and psxy[z]
	 * indent is the number of spaces to indent after the TAB.
	 * kind = 0 for *contour and 1 for psxy[z]
	 */
	int i;
	double gap;
	char pad[16];
	char *type[2] = {"contour", "quoted line"};

	gap = 4.0 * GMT_u2u[GMT_INCH][gmtdefs.measure_unit];

	pad[0] = '\t';	for (i = 1; i <= indent; i++) pad[i] = ' ';	pad[i] = '\0';
	fprintf (stderr, "%sd<dist>[c|i|m|p] or D<dist>[d|e|k|m|n].\n", pad);
	fprintf (stderr, "%s   d: Give distance between labels in specified unit [Default algorithm is d%g%c]\n", pad, gap, GMT_unit_names[gmtdefs.measure_unit][0]);
	fprintf (stderr, "%s   D: Specify distance between labels in m(e)ter [Default], (k)m, (m)ile, (n)autical mile, or (d)egree.\n", pad);
	fprintf (stderr, "%sf<ffile.d> reads the file <ffile.d> and places labels at those locations that match\n", pad);
	fprintf (stderr, "%s   individual points along the %ss\n", pad, type[kind]);
	fprintf (stderr, "%sl|L<line1>[,<line2>,...] Give start and stop coordinates for straight line segments.\n", pad);
	fprintf (stderr, "%s   Labels will be placed where these lines intersect %ss.  The format of each <line> is\n", pad, type[kind]);
	fprintf (stderr, "%s   <start>/<stop>, where <start> or <stop> = <lon/lat> or a 2-character XY key that uses the\n", pad);
	fprintf (stderr, "%s   \"pstext\"-style justification format to specify a point on the map as [LCR][BMT].\n", pad);
	if (kind == 0) fprintf (stderr, "%s   In addition, you can use Z-, Z+ to mean the global min, max locations in the grid.\n", pad);
	fprintf (stderr, "%s   L: Let point pairs define great circles [Default is a straight line].\n", pad);
	fprintf (stderr, "%sn|N<n_label> specifies the number of equidistant labels per %s.\n", pad, type[kind]);
	fprintf (stderr, "%s   N: Starts labeling exactly at the start of %s [Default centers the labels].\n", pad, type[kind]);
	fprintf (stderr, "%s   N-1 places one label at start, while N+1 places one label at the end of the %s.\n", pad, type[kind]);
	fprintf (stderr, "%s   Append /<min_dist> to enfore a minimum distance between succesive labels [0]\n", pad);
	fprintf (stderr, "%sx|X<xfile.d> reads the multi-segment file <xfile.d> and places labels at the intersections\n", pad);
	fprintf (stderr, "%s   between the %ss and the lines in <xfile.d>.  X: Resample the lines first.\n", pad, type[kind]);
}

void GMT_inc_syntax (char option, int error)
{
	if (error) fprintf (stderr, "%s: GMT SYNTAX ERROR -%c option.  Correct syntax:\n", GMT_program, option);
	fprintf (stderr, "\t-%c<xinc>[m|c|e|k|i|n|+][=][/<yinc>[m|c|e|k|i|n|+][=]]\n", option);
	fprintf (stderr, "\t  Give increment and append unit (m)inute, se(c)ond, m(e)ter, (k)ilometer, m(i)les, (n)autical miles.\n");
	fprintf (stderr, "\t  Append = to adjust the domain to fit the increment [Default adjusts increment to fit domain].\n");
	fprintf (stderr, "\t  Alternatively, specify number of nodes by appending +. Then, the increments are calculated\n");
	fprintf (stderr, "\t  from the given domain and grid-registration settings (see Appendix B for details).\n");
}

void GMT_fill_syntax (char option)
{
	fprintf (stderr, "%s: GMT SYNTAX ERROR -%c option.  Correct syntax:\n", GMT_program, option);
	fprintf (stderr, "\t-%cP|p<dpi>/<pattern>[:F<color>B<color>], dpi of pattern, pattern from 1-90 or a filename, optionally add fore/background colors (use - for transparency)\n", option);
	fprintf (stderr, "\t-%c<color>, <color> = <red>/<green>/<blue> or <gray>, all in the 0-255 range,\n", option);
	fprintf (stderr, "\t  <c>/<m>/<y>/<k> in 0-100%% range, <hue>-<sat>-<val> in 0-360, 0-1, 0-1 range,\n");
	fprintf (stderr, "\t  or black, white or [light|dark]{red, orange, yellow, green, cyan, blue, magenta, brown, gray}.\n");
}

void GMT_pen_syntax (char option)
{
	fprintf (stderr, "%s: GMT SYNTAX ERROR -%c option.  Correct syntax:\n", GMT_program, option);
	fprintf (stderr, "\t-%c[<width>[cipm]][,][<color>][,][<texture>[cipm]]\n", option);
	fprintf (stderr, "\t  Separate the <width>, <color>, and <texture> arguments with commas\n");
	fprintf (stderr, "\t  <width> >= 0.0, or a pen name: faint, default, or {thin, thick, fat}[er|est]\n");
	fprintf (stderr, "\t  <color> = <red>/<green>/<blue> or <gray>, all in the 0-255 range,\n");
	fprintf (stderr, "\t  <c>/<m>/<y>/<k> in 0-100%% range, <hue>-<sat>-<val> in 0-360, 0-1, 0-1 range,\n");
	fprintf (stderr, "\t  or black, white or [light|dark]{red, orange, yellow, green, cyan, blue, magenta, brown, gray}.\n");
	fprintf (stderr, "\t  If no unit is appended, then dots-per-inch is assumed [current dpi = %d].\n", gmtdefs.dpi);
	fprintf (stderr, "\t  <texture> = (1) pattern of dashes (-) and dots (.) which will be scaled by pen width.\n");
	fprintf (stderr, "\t              (2) a for d(a)shed or o for d(o)tted lines, caled by pen width.\n");
	fprintf (stderr, "\t              (3) <pattern>:<offset>; <pattern> holds lengths of lines and gaps separated\n");
	fprintf (stderr, "\t                  by underscores and <offset> is a phase offset.\n");
	fprintf (stderr, "\t  The default pen specification gives a solid black line [%gp]\n", GMT_PENWIDTH);
}

void GMT_rgb_syntax (char option)
{
	fprintf (stderr, "%s: GMT SYNTAX ERROR -%c option.  Correct syntax:\n", GMT_program, option);
	fprintf (stderr, "\t-%c<color>, <color> = <red>/<green>/<blue> or <gray>, all in the 0-255 range,\n", option);
	fprintf (stderr, "\t  <c>/<m>/<y>/<k> in 0-100%% range, or <hue>-<sat>-<val> in 0-360, 0-1, 0-1 range,\n");
	fprintf (stderr, "\t  or black, white or [light|dark]{red, orange, yellow, green, cyan, blue, magenta, brown, gray}.\n");
}

void GMT_syntax (char option)
{
	/* The function print to stderr the syntax for the option indicated by
	 * the variable <option>.  Only the common parameter options are covered
	 */

	fprintf (stderr, "%s: GMT SYNTAX ERROR -%c option.  Correct syntax:\n", GMT_program, option);

	switch (option) {

		case 'B':	/* Tickmark option */

			fprintf (stderr, "\t-B[p|s][a|f|g]<tick>[m][l|p][:\"label\":][:,\"unit\":][/.../...]:.\"Title\":[W|w|E|e|S|s|N|n][Z|z]\n");
			break;

		case 'H':	/* Header */

			fprintf (stderr, "\t-H[n-header-records]\n");
			break;

		case 'J':	/* Map projection option */

			switch (project_info.projection) {
				case LAMB_AZ_EQ:
					fprintf (stderr, "\t-Ja<lon0>/<lat0>/<scale> OR -JA<lon0>/<lat0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or <radius> (in %s)/<lat>, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case ALBERS:
					fprintf (stderr, "\t-Jb<lon0>/<lat0>/<lat1>/<lat2>/<scale> OR -JB<lon0>/<lat0>/<lat1>/<lat2>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case ECONIC:
					fprintf (stderr, "\t-Jd<lon0>/<lat0>/<lat1>/<lat2>/<scale> OR -JD<lon0>/<lat0>/<lat1>/<lat2>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case CASSINI:
					fprintf (stderr, "\t-Jc<lon0>/<lat0><scale> OR -JC<lon0>/<lat0><width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree ,or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case GNOMONIC:
					fprintf (stderr, "\t-Jf<lon0>/<lat0>/<horizon>/<scale> OR -JF<lon0>/<lat0>/<horizon>/<width>\n");
					fprintf (stderr, "\t   <horizon> is distance from center to perimeter (< 90)\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or <radius> (in %s)/<lat>, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case ORTHO:
					fprintf (stderr, "\t-Jg<lon0>/<lat0>/<scale> OR -JG<lon0>/<lat0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or <radius> (in %s)/<lat>, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case HAMMER:
					fprintf (stderr, "\t-Jh<lon0>/<scale> OR -JH<lon0>/<width\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case SINUSOIDAL:
					fprintf (stderr, "\t-Ji<lon0>/<scale> OR -JI<lon0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case LAMBERT:
					fprintf (stderr, "\t-Jl<lon0>/<lat0>/<lat1>/<lat2>/<scale> OR -JL<lon0>/<lat0>/<lat1>/<lat2>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case MERCATOR:
					fprintf (stderr, "\t-Jm<scale> OR -JM<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case ROBINSON:
					fprintf (stderr, "\t-Jn<lon0>/<scale> OR -JN<lon0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case OBLIQUE_MERC:
					fprintf (stderr, "\t-Joa<lon0>/<lat0>/<azimuth>/<scale> OR -JOa<lon0>/<lat0>/<azimuth>/<width>\n");
					fprintf (stderr, "\t-Job<lon0>/<lat0>/<b_lon>/<b_lat>/<scale> OR -JOb<lon0>/<lat0>/<b_lon>/<b_lat>/<width>\n");
					fprintf (stderr, "\t-Joc<lon0>/<lat0>/<lonp>/<latp>/<scale> OR -JOc<lon0>/<lat0>/<lonp>/<latp>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/oblique degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case WINKEL:
					fprintf (stderr, "\t-Jr<lon0>/<scale> OR -JR<lon0><width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case CYL_EQDIST:
					fprintf (stderr, "\t-Jq<lon0>/<scale> OR -JQ<lon0><width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case MILLER:
					fprintf (stderr, "\t-Jj<lon0>/<scale> OR -JJ<lon0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case STEREO:
					fprintf (stderr, "\t-Js<lon0>/<lat0>/<scale> OR -JS<lon0>/<lat0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx>, <lat>/<1:xxxx>, or <radius> (in %s)/<lat>, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case TM:
					fprintf (stderr, "\t-Jt<lon0>/<scale> OR -JT<lon0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case UTM:
					fprintf (stderr, "\t-Ju<zone>/<scale> OR -JU<zone>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case GRINTEN:
					fprintf (stderr, "\t-Jv<lon0>/<scale> OR -JV<lon0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case MOLLWEIDE:
					fprintf (stderr, "\t-Jw<lon0>/<scale> OR -JW<lon0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case ECKERT4:
					fprintf (stderr, "\t-Jkf<lon0>/<scale> OR -JKf<lon0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case ECKERT6:
					fprintf (stderr, "\t-Jk[s]<lon0>/<scale> OR -JK[s]<lon0>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case CYL_EQ:
					fprintf (stderr, "\t-Jy<lon0>/<lats>/<scale> OR -JY<lon0>/<lats>/<width>\n");
					fprintf (stderr, "\t  <scale is <1:xxxx> or %s/degree, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					break;
				case POLAR:
					fprintf (stderr, "\t-Jp[a]<scale>[/<origin>][r] OR -JP[a]<width>[/<origin>][r]\n");
					fprintf (stderr, "\t  <scale is %s/units, or use <width> in %s\n",
						GMT_unit_names[gmtdefs.measure_unit], GMT_unit_names[gmtdefs.measure_unit]);
					fprintf (stderr, "\t  Optionally, prepend a for azimuths, append theta as origin [0],\n");
					fprintf (stderr, "\t  or append r to reverse radial coordinates.\n");
				case LINEAR:
					fprintf (stderr, "\t-Jx<x-scale|width>[d|l|p<power>|t|T][/<y-scale|height>[d|l|p<power>|t|T]], scale in %s/units\n",
						GMT_unit_names[gmtdefs.measure_unit]);
					fprintf (stderr, "\t-Jz<z-scale>[l|p<power>], scale in %s/units\n",
						GMT_unit_names[gmtdefs.measure_unit]);
					fprintf (stderr, "\tUse / to specify separate x/y scaling (e.g., -Jx0.5/0.3.).  Not allowed with 1:xxxxx\n");
					fprintf (stderr, "\tUse -JX (and/or -JZ) to give axes lengths rather than scales\n");
					break;
				default:
					fprintf (stderr, "\tProjection not recognized!\n");
					break;
			}
			break;

		case 'R':	/* Region option */

			fprintf (stderr, "\t-R<xmin>/<xmax>/<ymin>/<ymax>[/<zmin>/<zmax>]\n");
			fprintf (stderr, "\tAppend r if giving lower left and upper right coordinates\n");
			break;

		case 'U':	/* Set time stamp option */

			fprintf (stderr, "\t-U[/<dx>/<dy>/][<string> | c], c will plot command line.\n");
			break;

		case ':':	/* lon/lat vs lat/lon i/o option  */

			fprintf (stderr, "\t-:[i|o], i for input, o for output [Default is both].\n");
			fprintf (stderr, "\t   Swap 1st and 2nd column on input and/or output.\n");
			break;

		case 'b':	/* Binary i/o option  */

			fprintf (stderr, "\t-b[i|o][s][<n>], i for input, o for output [Default is both].\n");
			fprintf (stderr, "\t   Use s for single precision [Default is double precision]\n");
			fprintf (stderr, "\t   and append the number of data columns (for input only).\n");
			break;

		case 'c':	/* Set number of plot copies option */

			fprintf (stderr, "\t-c<copies>, copies is number of copies\n");
			break;

		case 'f':	/* Column information option  */

			fprintf (stderr, "\t-f[i|o]<colinfo>, i for input, o for output [Default is both].\n");
			fprintf (stderr, "\t   <colinfo> is <colno|colrange>u, where column numbers start at 0\n");
			fprintf (stderr, "\t   a range is given as <first>-<last>, e.g., 2-5., u is type:\n");
			fprintf (stderr, "\t   t: relative time, T: absolute time, f: floating point,\n");
			fprintf (stderr, "\t   x: longitude, y: latitude, g: geographic coordinate.\n");
			break;

		default:
			break;
	}
}

void GMT_default_error (char option)
{
	fprintf (stderr, "%s: GMT SYNTAX ERROR:  Unrecognized option -%c\n", GMT_program, option);
}

int GMT_get_common_args (char *item, double *w, double *e, double *s, double *n)
{
	char text[BUFSIZ], string[BUFSIZ], txt_a[GMT_LONG_TEXT], txt_b[GMT_LONG_TEXT];

	/* GMT_get_common_args interprets the command line for the common, unique options
	 * -B, -H, -J, -K, -O, -P, -R, -U, -V, -X, -Y, -b, -c, -f, -:, -
	 */

	int i, j, icol, expect_to_read, got, nn = 0, n_slashes, error = 0, pos, j_type, col_type[2];
	BOOLEAN rect_box_given = FALSE;
	double *p[6];

	switch (item[1]) {
		case '\0':
			if (GMT_processed_option[0]) fprintf (stderr, "%s: Warning: Option - given more than once\n", GMT_program);
			GMT_processed_option[0] = TRUE;
			GMT_quick = TRUE;
			break;

		case 'B':
			switch (item[2]) {	/* Check for -B[p] and -Bs */
				case 's':
					if (GMT_processed_option[1] & 2) {
						fprintf (stderr, "%s: Error: Option -Bs given more than once\n", GMT_program);
						error++;
					}
					GMT_processed_option[1] |= 2;
					break;
				default:
					if (GMT_processed_option[1] & 1) {
						fprintf (stderr, "%s: Error: Option -B[p] given more than once\n", GMT_program);
						error++;
					}
					GMT_processed_option[1] |= 1;
					break;
			}
			error += (i = GMT_map_getframe (&item[2]));
			if (i) GMT_syntax ('B');
			break;

		case 'H':
			if (GMT_processed_option[2]) {
				fprintf (stderr, "%s: Error: Option -H given more than once\n", GMT_program);
				error++;
			}
			else {
				GMT_processed_option[2] = TRUE;
				j = 2;
				if (item[j] == 'i') j = 3;	/* -Hi[nrecs] given */
				if (item[j]) {
					i = atoi (&item[j]);
					if (i < 0) {
						GMT_syntax ('H');
						error++;
					}
					else
						gmtdefs.n_header_recs = i;
				}
				if (j == 2)	/* Both in and out may have header records */
					gmtdefs.io_header[GMT_IN] = gmtdefs.io_header[GMT_OUT] = (gmtdefs.n_header_recs > 0);
				else		/* Only input should have header records */
					gmtdefs.io_header[GMT_IN] = (gmtdefs.n_header_recs > 0);
			}
			break;
		case 'J':
			j_type = (item[2] == 'Z' || item[2] == 'z') ? 4 : 3;
			if (GMT_processed_option[j_type]) {
				fprintf (stderr, "%s: Error: Option -J given more than once\n", GMT_program);
				error++;
			}
			else {
				GMT_processed_option[j_type] = TRUE;
				error += (i = GMT_map_getproject (&item[2]));
				if (i) GMT_syntax ('J');
			}
			break;
		case 'K':
			if (GMT_processed_option[5]) fprintf (stderr, "%s: Warning: Option -K given more than once\n", GMT_program);
			GMT_processed_option[5] = TRUE;
			gmtdefs.last_page = FALSE;
			break;
		case 'O':
			if (GMT_processed_option[6]) fprintf (stderr, "%s: Warning: Option -O given more than once\n", GMT_program);
			GMT_processed_option[6] = TRUE;
			gmtdefs.overlay = TRUE;
			break;
		case 'P':
			if (GMT_processed_option[7]) fprintf (stderr, "%s: Warning: Option -P given more than once\n", GMT_program);
			GMT_processed_option[7] = TRUE;
			gmtdefs.page_orientation |= 1;	/* Bit arith because eurofont bit may be set */
			break;
		case 'R':
			if (GMT_processed_option[8]) {
				fprintf (stderr, "%s: Error: Option -R given more than once\n", GMT_program);
				error++;
				break;
			}
			GMT_processed_option[8] = TRUE;
			if (item[2] == 'g') {	/* Shorthand for -R0/360/-90/90 */
				*w = project_info.w = 0.0;	*e = project_info.e = 360.0;
				*s = project_info.s = -90.0;	*n = project_info.n = +90.0;
				GMT_io.in_col_type[0] = GMT_IS_LON;	GMT_io.in_col_type[1] = GMT_IS_LAT;
	 			project_info.region_supplied = TRUE;
				break;
			}
			if (item[2] == 'd') {	/* Shorthand for -R-180/+180/-90/90 */
				*w = project_info.w = -180.0;	*e = project_info.e = +180.0;
				*s = project_info.s = -90.0;	*n = project_info.n = +90.0;
				GMT_io.in_col_type[0] = GMT_IS_LON;	GMT_io.in_col_type[1] = GMT_IS_LAT;
	 			project_info.region_supplied = TRUE;
				break;
			}

			p[0] = w;	p[1] = e;	p[2] = s;	p[3] = n;
			p[4] = &project_info.z_bottom;	p[5] = &project_info.z_top;
			col_type[0] = col_type[1] = 0;
	 		project_info.region_supplied = TRUE;
	 		if (item[strlen(item)-1] == 'r') {
	 			rect_box_given = TRUE;
	 			project_info.region = FALSE;
				item[strlen(item)-1] = '\0';	/* Temporarily removing the trailing r so GMT_scanf will work */
	 		}
			i = pos = 0;
			strcpy (string, &item[2]);
			while ((GMT_strtok (string, "/", &pos, text))) {
				if (i > 5) {
					error++;
					GMT_syntax ('R');
					return (error);		/* Have to break out here to avoid segv on *p[6]  */
				}
				/* Figure out what column corresponds to a token to get in_col_type flag  */
				if (i > 3) {
					icol = 2;
				}
				else if (rect_box_given) {
					icol = i%2;
				}
				else {
					icol = i/2;
				}
				if (icol < 2 && gmtdefs.xy_toggle[0]) icol = 1 - icol;	/* col_types were swapped */
				/* If column is either RELTIME or ABSTIME, use ARGTIME */
				if (GMT_io.in_col_type[icol] == GMT_IS_UNKNOWN) {	/* No -J or -f set, proceed with caution */
					got = GMT_scanf_arg (text, GMT_io.in_col_type[icol], p[i]);
					error += GMT_verify_expectations (GMT_io.in_col_type[icol], got, text);
					if (got & GMT_IS_GEO) col_type[icol] = GMT_IS_GEO;		/* We will accept the implicit override only for geographical data */
					/* However, finding an abstime here will give error > 0 and an early exit below */
				}
				else {	/* Things are set, do or die */
					expect_to_read = (GMT_io.in_col_type[icol] & GMT_IS_RATIME) ? GMT_IS_ARGTIME : GMT_io.in_col_type[icol];
					error += GMT_verify_expectations (expect_to_read, GMT_scanf (text, expect_to_read, p[i]), text);
				}
				if (error) {
					GMT_syntax ('R');
					return (error);
				}

				i++;
			}
			if (col_type[0]) GMT_io.in_col_type[0] = col_type[0];	/* Set to what we found */
			if (col_type[1]) GMT_io.in_col_type[1] = col_type[1];	/* Set to what we found */
	 		if (rect_box_given) {
				d_swap (*p[2], *p[1]);	/* So w/e/s/n makes sense */
				item[strlen(item)] = 'r';	/* Put back the trailing r we temporarily removed */
			}
			if ((i < 4 || i > 6) || (GMT_check_region (*p[0], *p[1], *p[2], *p[3]) || (i == 6 && *p[4] >= *p[5]))) {
				error++;
				GMT_syntax ('R');
			}
			project_info.w = *p[0];	project_info.e = *p[1];	/* This will probably be reset by GMT_map_setup */
			project_info.s = *p[2];	project_info.n = *p[3];
			break;
		case 'U':
			if (GMT_processed_option[9]) {
				fprintf (stderr, "%s: Error: Option -U given more than once\n", GMT_program);
				error++;
				break;
			}
			GMT_processed_option[9] = TRUE;
			gmtdefs.unix_time = TRUE;
			for (i = j = n_slashes = 0; item[i]; i++) if (item[i] == '/') {
				n_slashes++;
				if (n_slashes < 4) j = i;
			}
			if (item[2] == '/' && n_slashes == 2) {	/* Gave -U/<dx>/<dy> */
				nn = sscanf (&item[3], "%[^/]/%s", txt_a, txt_b);
				gmtdefs.unix_time_pos[0] = GMT_convert_units (txt_a, GMT_INCH);
				gmtdefs.unix_time_pos[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			else if (item[2] == '/' && n_slashes > 2) {	/* Gave -U/<dx>/<dy>/<string> */
				nn = sscanf (&item[3], "%[^/]/%[^/]/%*s", txt_a, txt_b);
				gmtdefs.unix_time_pos[0] = GMT_convert_units (txt_a, GMT_INCH);
				gmtdefs.unix_time_pos[1] = GMT_convert_units (txt_b, GMT_INCH);
				strcpy (gmtdefs.unix_time_label, &item[j+1]);
			}
			else if (item[2] && item[2] != '/')	/* Gave -U<string> */
				strcpy (gmtdefs.unix_time_label, &item[2]);
			if ((item[2] == '/' && n_slashes == 1) || (item[2] == '/' && n_slashes >= 2 && nn != 2)) {
				error++;
				GMT_syntax ('U');
			}
			break;
		case 'V':
			if (GMT_processed_option[10]) fprintf (stderr, "%s: Warning: Option -V given more than once\n", GMT_program);
			GMT_processed_option[10] = TRUE;
			gmtdefs.verbose = (item[2] == 'l') ? 2 : TRUE;	/* -Vl is long verbose */
			gmtdefs.page_orientation |= 2;
			break;
		case 'X':
		case 'x':
			if (GMT_processed_option[11]) {
				fprintf (stderr, "%s: Error: Option -%c given more than once\n", GMT_program, item[1]);
				error++;
				break;
			}
			GMT_processed_option[11] = TRUE;
			i = 2;
			if (item[2] == 'r') i++;	/* Relative mode is default anyway */
			if (item[2] == 'a') i++, GMT_x_abs = TRUE;
			if (item[2] == 'c')
				project_info.x_off_supplied = 2;	/* Must center in map_setup */
			else {
				gmtdefs.x_origin = GMT_convert_units (&item[i], GMT_INCH);
				project_info.x_off_supplied = TRUE;
			}
			break;
		case 'Y':
		case 'y':
			if (GMT_processed_option[12]) {
				fprintf (stderr, "%s: Error: Option -%c given more than once\n", GMT_program, item[1]);
				error++;
				break;
			}
			GMT_processed_option[12] = TRUE;
			i = 2;
			if (item[2] == 'r') i++;	/* Relative mode is default anyway */
			if (item[2] == 'a') i++, GMT_y_abs = TRUE;
			if (item[2] == 'c')
				project_info.y_off_supplied = 2;	/* Must center in map_setup */
			else {
				gmtdefs.y_origin = GMT_convert_units (&item[i], GMT_INCH);
				project_info.y_off_supplied = TRUE;
			}
			break;
		case 'c':
			if (GMT_processed_option[13]) {
				fprintf (stderr, "%s: Error: Option -c given more than once\n", GMT_program);
				error++;
				break;
			}
			GMT_processed_option[13] = TRUE;
			i = atoi (&item[2]);
			if (i < 1) {
				error++;
				GMT_syntax ('c');
			}
			else
				gmtdefs.n_copies = i;
			break;
		case ':':	/* Toggle lon/lat - lat/lon */
			if (GMT_processed_option[14]) {
				fprintf (stderr, "%s: Error: Option -: given more than once\n", GMT_program);
				error++;
				break;
			}
			GMT_processed_option[14] = TRUE;
			switch (item[2]) {
				case 'i':	/* Toggle on input data only */
					gmtdefs.xy_toggle[0] = TRUE;
					i_swap (GMT_io.in_col_type[0], GMT_io.in_col_type[1]);
					break;
				case 'o':	/* Toggle on output data only */
					gmtdefs.xy_toggle[1] = TRUE;
					i_swap (GMT_io.out_col_type[0], GMT_io.out_col_type[1]);
					break;
				case '\0':	/* Toggle both input and output data */
					gmtdefs.xy_toggle[0] = gmtdefs.xy_toggle[1] = TRUE;
					i_swap (GMT_io.in_col_type[0], GMT_io.in_col_type[1]);
					i_swap (GMT_io.out_col_type[0], GMT_io.out_col_type[1]);
					break;
				default:
					GMT_syntax (':');
					error++;
					break;

			}
			break;
		case 'b':	/* Binary i/o */
			i = GMT_io_selection (&item[2]);
			if (i) GMT_syntax ('b');
			error += i;
			break;
		case 'f':	/* Column type specifications */
			if (GMT_processed_option[15] > 3) {
				fprintf (stderr, "%s: Error: Option -f given more than once\n", GMT_program);
				error++;
				break;
			}
			switch (item[2]) {
				case 'i':
					GMT_processed_option[15]++;
					break;
				case 'o':
					GMT_processed_option[15] += 2;
					break;
				default:
					GMT_processed_option[15] += 3;
					break;
			}
			i = GMT_decode_coltype (&item[2]);
			if (i) GMT_syntax ('f');
			error += i;
			break;
		default:	/* Should never get here, but... */
			error++;
			fprintf (stderr, "GMT: Warning: bad case in GMT_get_common_args\n");
			break;
	}

	return (error);
}

int GMT_loaddefaults (char *file)
{
	int error = 0;
	char line[BUFSIZ], keyword[GMT_LONG_TEXT], value[GMT_LONG_TEXT];
	FILE *fp = NULL;

	if ((fp = fopen (file, "r")) == NULL) return (-1);

	GMT_force_resize = FALSE;	/* "Listen" for +<size> for ANNOT_FONT */

	/* Set up hash table */

	GMT_hash_init (hashnode, GMT_keywords, HASH_SIZE, N_KEYS);

	while (fgets (line, BUFSIZ, fp)) {
		if (line[0] == '#') continue;	/* Skip comments */
		if (line[0] == '\n') continue;	/* Skip Blank lines */

		keyword[0] = value[0] = '\0';	/* Initialize */
		sscanf (line, "%s = %[^\n]", keyword, value);

		error += GMT_setparameter (keyword, value);
	}

	fclose (fp);
	GMT_backwards_compatibility ();
	GMT_prep_PS_bits ();
	if (!strstr (GMT_program, "gmtset")) GMT_verify_encodings ();

	if (error) fprintf (stderr, "GMT:  %d conversion errors in file %s!\n", error, file);

	return (0);
}

void GMT_prep_PS_bits ()
{
	/* Because some defaults may be set more than once - like overridden with --PAR
	 * we must be careful to set or unset the selections */
	 
	gmtdefs.page_orientation &= 1;	/* Unset all but bit 0 */
	if (gmtdefs.verbose) gmtdefs.page_orientation |= 2;
	if (gmtdefs.ps_heximage) gmtdefs.page_orientation |= 4;
	if (gmtdefs.ps_colormode) gmtdefs.page_orientation |= (gmtdefs.ps_colormode << 8);
	if (gmtdefs.ps_compress) gmtdefs.page_orientation |= (gmtdefs.ps_compress << 12);
	if (gmtdefs.ps_line_cap) gmtdefs.page_orientation |= (gmtdefs.ps_line_cap << 14);
	if (gmtdefs.ps_line_join) gmtdefs.page_orientation |= (gmtdefs.ps_line_join << 16);
	if (gmtdefs.ps_miter_limit) gmtdefs.page_orientation |= (gmtdefs.ps_miter_limit << 18);
	if (gmtdefs.ps_verbose) gmtdefs.page_orientation |= (1 << 30);
}

void GMT_setdefaults (int argc, char **argv)
{
	int j, k, p, error = 0;

	/* Set up hash table */

	GMT_hash_init (hashnode, GMT_keywords, HASH_SIZE, N_KEYS);

	GMT_got_frame_rgb = FALSE;	/* "Listen" for changes to basemap_frame RGB */

	j = 1;
	while (j < argc) {	/* j points to parameter, k to value */
		k = j + 1;
		if (strchr (argv[j], '=')) {	/* User forgot and gave parameter=value (1 word) */
			p = 0;
			while (argv[j][p] && argv[j][p] != '=') p++;
			if (argv[j][p] == '=') {
				argv[j][p] = '\0';
				error += GMT_setparameter (argv[j], &argv[j][p+1]);
				argv[j][p] = '=';
				k = j;
			}
			else {
				error++;
				break;
			}
		}	
		else if (!strcmp (argv[k], "=")) {	/* User forgot and gave parameter = value */
			k++;
			if (k >= argc) {
				error++;
				break;
			}
			error += GMT_setparameter (argv[j], argv[k]);
		}
		else {
			if (k >= argc) {	/* Ran out of arguments, error */
				error++;
				break;
			}
			error += GMT_setparameter (argv[j], argv[k]);
		}
		j = k + 1;	/* Goto next parameter */
	}

	GMT_backwards_compatibility ();
	GMT_prep_PS_bits ();

	if (GMT_got_frame_rgb) {	/* Must enforce change of frame, tick, and grid pen rgb */
		memcpy ((void *)gmtdefs.frame_pen.rgb, (void *)gmtdefs.basemap_frame_rgb, (size_t)(3 * sizeof (int)));
		memcpy ((void *)gmtdefs.tick_pen.rgb,  (void *)gmtdefs.basemap_frame_rgb, (size_t)(3 * sizeof (int)));
		memcpy ((void *)gmtdefs.grid_pen[0].rgb,   (void *)gmtdefs.basemap_frame_rgb, (size_t)(3 * sizeof (int)));
		memcpy ((void *)gmtdefs.grid_pen[1].rgb, (void *)gmtdefs.basemap_frame_rgb, (size_t)(3 * sizeof (int)));
	}

	if (error) fprintf (stderr, "%s:  %d conversion errors\n", GMT_program, error);
}

void GMT_backwards_compatibility () {
	/* Convert old GMT 3.4 DEGREE_FORMAT settings to the new PLOT_DEGREE_FORMAT string and DEGREE_SYMBOL setting */
	/* Also to automatic scaling of font sizes relative to ANNOT_FONT_SIZE if given with leading + */

	char string[GMT_LONG_TEXT];
	int k;

	if (GMT_backward.got_old_plot_format && GMT_backward.got_new_plot_format) {	/* Got both old and new */
		fprintf (stderr, "%s: WARNING: Both old-style DEGREE_FORMAT and PLOT_DEGREE_FORMAT present in .gmtdefaults\n", GMT_program);
		fprintf (stderr, "%s: WARNING: PLOT_DEGREE_FORMAT overrides old DEGREE_FORMAT\n", GMT_program);
	}
	else if (GMT_backward.got_old_plot_format && !GMT_backward.got_new_plot_format) {	/* Must decode old DEGREE_FORMAT */
		memset ((void *)string, 0, GMT_LONG_TEXT);
		k = gmtdefs.degree_format % 100;
		if (k == 0 || k == 4 || k == 6 || k == 8)	/* These were 0-360 values */
			strcpy (string, "+");
		else if (k >= 12 && k <= 17)			/* These were -360 to 0 numbers */
			strcpy (string, "-");
		/* else we do nothing and get -180/+180 */

		if ((k >= 4 && k <= 7) || k == 13 || k == 16)		/* Decimal degrees using D_FORMAT */
			strcat (string, "D");
		else if (( k >= 8 && k <= 11) || k == 14 || k == 17)	 /* Degrees and decimal minutes - pick 1 decimal */
			strcat (string, "ddd:mm.x");
		else
			strcat (string, "ddd:mm:ss");			/* Standard dd mm ss */
		if (k == 2 || k == 10)					/* Abs value */
			strcat (string, "A");
		else if (k == 3 || k == 6 || k == 7 || k == 11 || (k >= 15 && k <= 17))	/* Append WESN */
			strcat (string, "F");
		strcpy (gmtdefs.plot_degree_format, string);
		fprintf (stderr, "%s: WARNING: DEGREE_FORMAT decoded (%d) but is obsolete.  Please use PLOT_DEGREE_FORMAT (%s)\n", GMT_program, gmtdefs.degree_format, gmtdefs.plot_degree_format);
	}
	if (GMT_backward.got_old_degree_symbol && GMT_backward.got_new_degree_symbol) {	/* Got both old and new */
		fprintf (stderr, "%s: WARNING: Both old-style DEGREE_FORMAT and DEGREE_SYMBOL present in .gmtdefaults\n", GMT_program);
		fprintf (stderr, "%s: WARNING: DEGREE_SYMBOL overrides old DEGREE_FORMAT\n", GMT_program);
	}
	else if (GMT_backward.got_old_degree_symbol && !GMT_backward.got_new_degree_symbol) {	/* Must decode old DEGREE_FORMAT */
		fprintf (stderr, "%s: WARNING: DEGREE_FORMAT decoded but is obsolete.  Please use DEGREE_SYMBOL\n", GMT_program);
		if (gmtdefs.degree_format >= 1000)	/* No degree symbol */
			gmtdefs.degree_symbol = 3;
		else if (gmtdefs.degree_format >= 100)	/* Large degree symbol */
			gmtdefs.degree_symbol = 1;
	}
	if (GMT_backward.got_old_want_euro && GMT_backward.got_new_char_encoding) {	/* Got both old and new */
		fprintf (stderr, "%s: WARNING: Both old-style WANT_EURO_FONT and CHAR_ENCODING present in .gmtdefaults\n", GMT_program);
		fprintf (stderr, "%s: WARNING: CHAR_ENCODING overrides old WANT_EURO_FONT\n", GMT_program);
	}
	else if (GMT_backward.got_old_want_euro && GMT_backward.got_new_char_encoding)  {	/* Must decode old WANT_EURO_FONT */
		fprintf (stderr, "%s: WARNING: WANT_EURO_FONT decoded but is obsolete.  Please use CHAR_ENCODING\n", GMT_program);
		gmtdefs.encoding.name = strdup ("Standard+");
		load_encoding (&gmtdefs.encoding);
	}

	if (GMT_force_resize) {	/* Adjust fonts and offsets and ticklenghts relative to ANNOT_FONT_SIZE */
		gmtdefs.annot_font_size[1] = 16.0 * gmtdefs.annot_font_size[0] / 14.0;
		gmtdefs.label_font_size = 24.0 * gmtdefs.annot_font_size[0] / 14.0;
		gmtdefs.header_font_size = 36.0 * gmtdefs.annot_font_size[0] / 14.0;
		gmtdefs.annot_offset[0] = 0.075 * gmtdefs.annot_font_size[0] / 14.0;
		gmtdefs.tick_length = 0.075 * gmtdefs.annot_font_size[0] * copysign (1.0, gmtdefs.tick_length) / 14.0;
		gmtdefs.annot_offset[1] = 0.075 * gmtdefs.annot_font_size[1] / 14.0;
		gmtdefs.label_offset = 1.5 * fabs (gmtdefs.annot_offset[0]);
		gmtdefs.header_offset = 2.5 * fabs (gmtdefs.annot_offset[0]);
		gmtdefs.frame_width = 0.05 * gmtdefs.annot_font_size[0] / 14.0;
	}
}

int GMT_setparameter (char *keyword, char *value)
{
	int i, ival, case_val, rgb[3];
	BOOLEAN manual, eps, error = FALSE;
	char txt_a[GMT_LONG_TEXT], txt_b[GMT_LONG_TEXT], lower_value[BUFSIZ];
	double dval;

	if (!value) return (TRUE);		/* value argument missing */
	strncpy (lower_value, value, BUFSIZ);	/* Get a lower case version */
	GMT_str_tolower (lower_value);

	case_val = GMT_hash_lookup (keyword, hashnode, HASH_SIZE, HASH_SIZE);

	switch (case_val) {
		case GMTCASE_ANNOT_MIN_ANGLE:
		case GMTCASE_ANOT_MIN_ANGLE:
			dval = atof (value);
			if (dval < 0.0)
				error = TRUE;
			else
				gmtdefs.annot_min_angle = dval;
			break;
		case GMTCASE_ANNOT_MIN_SPACING:
		case GMTCASE_ANOT_MIN_SPACING:
			if (value[0] == '-')	/* Negative */
				error = TRUE;
			else
				gmtdefs.annot_min_spacing = GMT_convert_units (value, GMT_INCH);
			break;
		case GMTCASE_ANNOT_FONT_PRIMARY:
		case GMTCASE_ANOT_FONT_PRIMARY:
		case GMTCASE_ANNOT_FONT:
		case GMTCASE_ANOT_FONT:
			if (value[0] >= '0' && value[0] <= '9')
				ival = atoi (value);
			else
				ival = GMT_font_lookup (value, GMT_font, N_FONTS);
			if (ival < 0 || ival >= N_FONTS)
				error = TRUE;
			else
				gmtdefs.annot_font[0] = ival;
			break;
		case GMTCASE_ANNOT_FONT_SIZE_PRIMARY:
		case GMTCASE_ANOT_FONT_SIZE_PRIMARY:
		case GMTCASE_ANNOT_FONT_SIZE:
		case GMTCASE_ANOT_FONT_SIZE:
			if (value[0] == '+') GMT_force_resize = TRUE;	/* Turning on autoscaling of font sizes and ticklengths */
			if (value[0] != '+' && GMT_force_resize) GMT_annot_special = TRUE;	/* gmtset tries to turn off autoscaling - must report saved values but reset this one */
			dval = atof (value);
			if (dval > 0.0)
				gmtdefs.annot_font_size[0] = dval;
			else
				error = TRUE;
			break;
		case GMTCASE_ANNOT_OFFSET_PRIMARY:
		case GMTCASE_ANOT_OFFSET_PRIMARY:
		case GMTCASE_ANNOT_OFFSET:
		case GMTCASE_ANOT_OFFSET:
			save_annot_offset[0] = gmtdefs.annot_offset[0] = GMT_convert_units (value, GMT_INCH);
			break;
		case GMTCASE_BASEMAP_AXES:
			strcpy (gmtdefs.basemap_axes, value);
			for (i = 0; i < 4; i++) frame_info.side[i] = 0;	/* Otherwise we cannot unset default settings */
			for (i = 0; value[i]; i++) {
				switch (value[i]) {
					case 'W':	/* Upper case: Draw axis/ticks AND annotate */
						frame_info.side[3] = 2;
						break;
					case 'w':	/* Lower case: Draw axis/ticks only */
						frame_info.side[3] = 1;
						break;
					case 'E':
						frame_info.side[1] = 2;
						break;
					case 'e':
						frame_info.side[1] = 1;
						break;
					case 'S':
						frame_info.side[0] = 2;
						break;
					case 's':
						frame_info.side[0] = 1;
						break;
					case 'N':
						frame_info.side[2] = 2;
						break;
					case 'n':
						frame_info.side[2] = 1;
						break;
					case '-':	/* None */
						break;
					default:
						error = TRUE;
						break;
				}
			}
			break;
		case GMTCASE_BASEMAP_FRAME_RGB:
			i = (value[0] == '+') ? 1 : 0;	/* Plus means propagate color to frame, tick, and grid pens as well */
			error = GMT_getrgb (&value[i], rgb);
			if (GMT_check_rgb (rgb))
				error = TRUE;
			else {
				memcpy ((void *)gmtdefs.basemap_frame_rgb, (void *)rgb, (size_t)(3 * sizeof (int)));
				memcpy ((void *)gmtdefs.frame_pen.rgb, (void *)rgb, (size_t)(3 * sizeof (int)));
				if (i == 1) GMT_got_frame_rgb = TRUE;
			}
			break;
		case GMTCASE_BASEMAP_TYPE:
			if (!strcmp (lower_value, "plain"))
				gmtdefs.basemap_type = GMT_IS_PLAIN;
			else if (!strcmp (lower_value, "fancy"))
				gmtdefs.basemap_type = GMT_IS_FANCY;
			else if (!strcmp (lower_value, "fancy+"))
				gmtdefs.basemap_type = GMT_IS_ROUNDED;
			else
				error = TRUE;
			break;
		case GMTCASE_COLOR_BACKGROUND:
			if (value[0] == '-')
				gmtdefs.background_rgb[0] = gmtdefs.background_rgb[1] = gmtdefs.background_rgb[2] = -1;
			else {
				error = GMT_getrgb (value, rgb);
				if (GMT_check_rgb (rgb))
					error = TRUE;
				else
					memcpy ((void *)gmtdefs.background_rgb, (void *)rgb, (size_t)(3 * sizeof (int)));
			}
			break;
		case GMTCASE_COLOR_FOREGROUND:
			if (value[0] == '-')
				gmtdefs.foreground_rgb[0] = gmtdefs.foreground_rgb[1] = gmtdefs.foreground_rgb[2] = -1;
			else {
				error = GMT_getrgb (value, rgb);
				if (GMT_check_rgb (rgb))
					error = TRUE;
				else
					memcpy ((void *)gmtdefs.foreground_rgb, (void *)rgb, (size_t)(3 * sizeof (int)));
			}
			break;
		case GMTCASE_COLOR_NAN:
			if (value[0] == '-')
				gmtdefs.nan_rgb[0] = gmtdefs.nan_rgb[1] = gmtdefs.nan_rgb[2] = -1;
			else {
				error = GMT_getrgb (value, rgb);
				if (GMT_check_rgb (rgb))
					error = TRUE;
				else
					memcpy ((void *)gmtdefs.nan_rgb, (void *)rgb, (size_t)(3 * sizeof (int)));
			}
			break;
		case GMTCASE_COLOR_IMAGE:
			if (!strcmp (lower_value, "adobe"))
				gmtdefs.color_image = 0;
			else if (!strcmp (lower_value, "tiles"))
				gmtdefs.color_image = 1;
			else
				error = TRUE;
			break;
		case GMTCASE_COLOR_MODEL:
			if (!strcmp (lower_value, "+hsv"))
				gmtdefs.color_model = GMT_HSV;
			else if (!strcmp (lower_value, "hsv"))
				gmtdefs.color_model = GMT_READ_HSV;
			else if (!strcmp (lower_value, "+rgb"))
				gmtdefs.color_model = GMT_RGB;
			else if (!strcmp (lower_value, "rgb"))
				gmtdefs.color_model = GMT_READ_RGB;
			else if (!strcmp (lower_value, "+cmyk"))
				gmtdefs.color_model = GMT_CMYK;
			else if (!strcmp (lower_value, "cmyk"))
				gmtdefs.color_model = GMT_READ_CMYK;
			else
				error = TRUE;
			break;
		case GMTCASE_D_FORMAT:
			strcpy (gmtdefs.d_format, value);
			break;
		case GMTCASE_DEGREE_FORMAT:
			ival = atoi (value);
			if ((ival%100) >= 0 && (ival%100) <= 17)	/* Mod 100 since we may have added 100 to get big degree symbol or 1000 for no symbol */
				gmtdefs.degree_format = ival;
			else
				error = TRUE;
			GMT_backward.got_old_plot_format = TRUE;
			GMT_backward.got_old_degree_symbol = (ival >= 100);
			break;
		case GMTCASE_DOTS_PR_INCH:
			ival = atoi (value);
			if (ival > 0)
				gmtdefs.dpi = ival;
			else
				error = TRUE;
			break;
		case GMTCASE_ELLIPSOID:
			ival = GMT_get_ellipsoid (value);
			if (ival < 0)
				error = TRUE;
			else
				gmtdefs.ellipsoid = ival;
			break;
		case GMTCASE_FRAME_PEN:
			error = GMT_getpen (value, &gmtdefs.frame_pen);
			break;
		case GMTCASE_FRAME_WIDTH:
			dval = GMT_convert_units (value, GMT_INCH);
			if (dval > 0.0)
				save_frame_width = gmtdefs.frame_width = dval;
			else
				error = TRUE;
			break;
		case GMTCASE_GLOBAL_X_SCALE:
			dval = atof (value);
			if (dval > 0.0)
				gmtdefs.global_x_scale = dval;
			else
				error = TRUE;
			break;
		case GMTCASE_GLOBAL_Y_SCALE:
			dval = atof (value);
			if (dval > 0.0)
				gmtdefs.global_y_scale = dval;
			else
				error = TRUE;
			break;
		case GMTCASE_GRID_CROSS_SIZE_PRIMARY:
		case GMTCASE_GRID_CROSS_SIZE:
			dval = GMT_convert_units (value, GMT_INCH);
			if (dval >= 0.0)
				gmtdefs.grid_cross_size[0] = dval;
			else
				error = TRUE;
			break;
		case GMTCASE_GRID_CROSS_SIZE_SECONDARY:
			dval = GMT_convert_units (value, GMT_INCH);
			if (dval >= 0.0)
				gmtdefs.grid_cross_size[1] = dval;
			else
				error = TRUE;
			break;
		case GMTCASE_GRID_FORMAT:
			strcpy (gmtdefs.grid_format, value);
			break;
		case GMTCASE_GRID_PEN_PRIMARY:
		case GMTCASE_GRID_PEN:
			error = GMT_getpen (value, &gmtdefs.grid_pen[0]);
			break;
		case GMTCASE_GRID_PEN_SECONDARY:
			error = GMT_getpen (value, &gmtdefs.grid_pen[1]);
			break;
		case GMTCASE_GRIDFILE_SHORTHAND:
			error = true_false_or_error (lower_value, &gmtdefs.gridfile_shorthand);
			break;
		case GMTCASE_HEADER_FONT:
			if (value[0] >= '0' && value[0] <= '9')
				ival = atoi (value);
			else
				ival = GMT_font_lookup (value, GMT_font, N_FONTS);
			if (ival < 0 || ival >= N_FONTS)
				error = TRUE;
			else
				gmtdefs.header_font = ival;
			break;
		case GMTCASE_HEADER_FONT_SIZE:
			dval = atof (value);
			if (dval > 0.0)
				save_header_size = gmtdefs.header_font_size = dval;
			else
				error = TRUE;
			break;
		case GMTCASE_HSV_MIN_SATURATION:
			dval = atof (value);
			if (dval < 0.0 || dval > 1.0)
				error = TRUE;
			else
				gmtdefs.hsv_min_saturation = dval;
			break;
		case GMTCASE_HSV_MAX_SATURATION:
			dval = atof (value);
			if (dval < 0.0 || dval > 1.0)
				error = TRUE;
			else
				gmtdefs.hsv_max_saturation = dval;
			break;
		case GMTCASE_HSV_MIN_VALUE:
			dval = atof (value);
			if (dval < 0.0 || dval > 1.0)
				error = TRUE;
			else
				gmtdefs.hsv_min_value = dval;
			break;
		case GMTCASE_HSV_MAX_VALUE:
			dval = atof (value);
			if (dval < 0.0 || dval > 1.0)
				error = TRUE;
			else
				gmtdefs.hsv_max_value = dval;
			break;
		case GMTCASE_INTERPOLANT:
			if (!strcmp (lower_value, "linear"))
				gmtdefs.interpolant = 0;
			else if (!strcmp (lower_value, "akima"))
				gmtdefs.interpolant = 1;
			else if (!strcmp (lower_value, "cubic"))
				gmtdefs.interpolant = 2;
			else
				error = TRUE;
			break;
		case GMTCASE_IO_HEADER:
			error = true_false_or_error (lower_value, &gmtdefs.io_header[GMT_IN]);
			break;
		case GMTCASE_N_HEADER_RECS:
			ival = atoi (value);
			if (ival < 0)
				error = TRUE;
			else
				gmtdefs.n_header_recs = ival;
			break;
		case GMTCASE_LABEL_FONT:
			if (value[0] >= '0' && value[0] <= '9')
				ival = atoi (value);
			else
				ival = GMT_font_lookup (value, GMT_font, N_FONTS);
			if (ival < 0 || ival >= N_FONTS)
				error = TRUE;
			else
				gmtdefs.label_font = ival;
			break;
		case GMTCASE_LABEL_FONT_SIZE:
			dval = atof (value);
			if (dval > 0.0)
				save_label_size = gmtdefs.label_font_size = dval;
			else
				error = TRUE;
			break;
		case GMTCASE_LINE_STEP:
			if ((gmtdefs.line_step = GMT_convert_units (value, GMT_INCH)) <= 0.0) {
				gmtdefs.line_step = 0.01;
				fprintf (stderr, "%s: GMT WARNING: %s <= 0, reset to %g %s\n",
				GMT_program, keyword, gmtdefs.line_step, GMT_unit_names[GMT_INCH]);
			}
			break;
		case GMTCASE_MAP_SCALE_FACTOR:
			if (!strncmp (value, "def", 3)) /* Default scale for chosen projection */
				gmtdefs.map_scale_factor = -1.0;
			else {
				dval = atof (value);
				if (dval <= 0.0)
					error = TRUE;
				else
					gmtdefs.map_scale_factor = dval;
			}
			break;
		case GMTCASE_MAP_SCALE_HEIGHT:
			dval = GMT_convert_units (value, GMT_INCH);
			if (dval <= 0.0)
				error = TRUE;
			else
				gmtdefs.map_scale_height = dval;
			break;
		case GMTCASE_MEASURE_UNIT:
			if (!strcmp (lower_value, "cm"))
				gmtdefs.measure_unit = 0;
			else if (!strcmp (lower_value, "inch"))
				gmtdefs.measure_unit = 1;
			else if (!strcmp (lower_value, "m"))
				gmtdefs.measure_unit = 2;
			else if (!strcmp (lower_value, "point"))
				gmtdefs.measure_unit = 3;
			else
				error = TRUE;
			break;
		case GMTCASE_N_COPIES:
			ival = atoi (value);
			if (ival > 0)
				gmtdefs.n_copies = ival;
			else
				error = TRUE;
			break;
		case GMTCASE_OBLIQUE_ANNOTATION:
		case GMTCASE_OBLIQUE_ANOTATION:
			ival = atoi (value);
			if (ival >= 0 && ival < 64)
				gmtdefs.oblique_annotation = ival;
			else
				error = TRUE;
			break;
		case GMTCASE_PAGE_COLOR:
			error = GMT_getrgb (value, rgb);
			if (GMT_check_rgb (rgb))
				error = TRUE;
			else
				memcpy ((void *)gmtdefs.page_rgb, (void *)rgb, (size_t)(3 * sizeof (int)));
			break;
		case GMTCASE_PAGE_ORIENTATION:
			if (!strcmp (lower_value, "landscape"))
				gmtdefs.page_orientation = 0;
			else if (!strcmp (lower_value, "portrait"))
				gmtdefs.page_orientation = 1;
			else
				error = TRUE;
			break;
		case GMTCASE_PAPER_MEDIA:
			manual = eps = FALSE;
			ival = strlen (lower_value) - 1;
			if (lower_value[ival] == '-') {	/* Manual Feed selected */
				lower_value[ival] = '\0';
				manual = TRUE;
			}
			else if (lower_value[ival] == '+') {	/* EPS format selected */
				lower_value[ival] = '\0';
				eps = TRUE;
			}

			i = GMT_key_lookup (lower_value, GMT_media_name, GMT_N_MEDIA);
			if (i >= 0 && i < GMT_N_MEDIA) {	/* Use the specified standard format */
				gmtdefs.media = i;
				gmtdefs.paper_width[0] = GMT_media[i].width;
				gmtdefs.paper_width[1] = GMT_media[i].height;
			}
			else if (!strncmp (lower_value, "custom_", 7)) {	/* A custom paper size in W x H points */
				sscanf (&lower_value[7], "%dx%d", &gmtdefs.paper_width[0], &gmtdefs.paper_width[1]);
				if (gmtdefs.paper_width[0] <= 0) error++;
				if (gmtdefs.paper_width[1] <= 0) error++;
				gmtdefs.media = -USER_MEDIA_OFFSET;
			}
			else {	/* Not one of the standards, try the user-specified formats, if any */
				if ((GMT_n_user_media = GMT_load_user_media ())) {	/* Got some */
					i = GMT_key_lookup (lower_value, GMT_user_media_name, GMT_n_user_media);
					if (i < 0 || i >= GMT_n_user_media) {	/* Not found, give error */
						error = TRUE;
					}
					else {	/* User the user-specified format */
						gmtdefs.media = i + USER_MEDIA_OFFSET;
						gmtdefs.paper_width[0] = GMT_user_media[i].width;
						gmtdefs.paper_width[1] = GMT_user_media[i].height;
					}
				}
				else {	/* Not found, give error */
					error = TRUE;
				}
			}
			if (!error) {
				if (manual) gmtdefs.paper_width[0] = -gmtdefs.paper_width[0];
				if (eps) gmtdefs.paper_width[1] = -gmtdefs.paper_width[1];
			}
			break;
		case GMTCASE_POLAR_CAP:
			if (!strcmp (lower_value, "none")) {	/* Means reset to no cap -> lat = 90, dlon = 0 */
				gmtdefs.polar_cap[0] = 90.0;
				gmtdefs.polar_cap[1] = 0.0;
			}
			else {
				i = sscanf (lower_value, "%[^/]/%s", txt_a, txt_b);
				if (i != 2) error = TRUE;
				error = GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_a, GMT_IS_LAT, &gmtdefs.polar_cap[0]), txt_a);
				error += GMT_getinc (txt_b, &gmtdefs.polar_cap[1], &dval);
			}
			break;
		case GMTCASE_PS_COLOR:
			if (!strcmp (lower_value, "rgb"))
				gmtdefs.ps_colormode = 0;
			else if (!strcmp (lower_value, "cmyk"))
				gmtdefs.ps_colormode = 1;
			else if (!strcmp (lower_value, "hsv"))
				gmtdefs.ps_colormode = 2;
			else
				error = TRUE;
			break;
		case GMTCASE_PS_IMAGE_COMPRESS:
		case GMTCASE_PSIMAGE_COMPRESS:
			if (!strcmp (lower_value, "none"))
				gmtdefs.ps_compress = 0;
			else if (!strcmp (lower_value, "rle"))
				gmtdefs.ps_compress = 1;
			else if (!strcmp (lower_value, "lzw"))
				gmtdefs.ps_compress = 2;
			else
				error = TRUE;
			break;
		case GMTCASE_PS_IMAGE_FORMAT:
		case GMTCASE_PSIMAGE_FORMAT:
			if (!strcmp (lower_value, "ascii"))
				gmtdefs.ps_heximage = 1;
			else if (!strcmp (lower_value, "hex"))	/* Backwards compatible */
				gmtdefs.ps_heximage = 1;
			else if (!strcmp (lower_value, "bin"))
				gmtdefs.ps_heximage = 0;
			else
				error = TRUE;
			break;
		case GMTCASE_PS_LINE_CAP:
			if (!strcmp (lower_value, "butt"))
				gmtdefs.ps_line_cap = 0;
			else if (!strcmp (lower_value, "round"))
				gmtdefs.ps_line_cap = 1;
			else if (!strcmp (lower_value, "square"))
				gmtdefs.ps_line_cap = 2;
			else
				error = TRUE;
			break;
		case GMTCASE_PS_LINE_JOIN:
			if (!strcmp (lower_value, "miter"))
				gmtdefs.ps_line_join = 0;
			else if (!strcmp (lower_value, "round"))
				gmtdefs.ps_line_join = 1;
			else if (!strcmp (lower_value, "bevel"))
				gmtdefs.ps_line_join = 2;
			else
				error = TRUE;
			break;
		case GMTCASE_PS_MITER_LIMIT:
			ival = atoi (value);
			if (ival >= 0 && ival <= 180)
				gmtdefs.ps_miter_limit = ival;
			else
				error = TRUE;
			break;
		case GMTCASE_PS_VERBOSE:
			error = true_false_or_error (lower_value, &gmtdefs.ps_verbose);
			break;
		case GMTCASE_TICK_LENGTH:
			save_tick_length = gmtdefs.tick_length = GMT_convert_units (value, GMT_INCH);
			break;
		case GMTCASE_TICK_PEN:
			error = GMT_getpen (value, &gmtdefs.tick_pen);
			break;
		case GMTCASE_UNIX_TIME:
			error = true_false_or_error (lower_value, &gmtdefs.unix_time);
			break;
		case GMTCASE_UNIX_TIME_POS:
			sscanf (value, "%[^/]/%s", txt_a, txt_b);
			gmtdefs.unix_time_pos[0] = GMT_convert_units (txt_a, GMT_INCH);
			gmtdefs.unix_time_pos[1] = GMT_convert_units (txt_b, GMT_INCH);
			break;
		case GMTCASE_VECTOR_SHAPE:
			dval = atof (value);
			if (dval < -2.0 || dval > 2.0)
				error = TRUE;
			else
				gmtdefs.vector_shape = dval;
			break;
		case GMTCASE_VERBOSE:
			error = true_false_or_error (lower_value, &gmtdefs.verbose);
			break;
		case GMTCASE_WANT_EURO_FONT:
			error = true_false_or_error (lower_value, &gmtdefs.want_euro_font);
			break;
		case GMTCASE_X_AXIS_LENGTH:
			dval = GMT_convert_units (value, GMT_INCH);
			if (dval <= 0.0)
				error = TRUE;
			else
				gmtdefs.x_axis_length = dval;
			break;
		case GMTCASE_Y_AXIS_LENGTH:
			dval = GMT_convert_units (value, GMT_INCH);
			if (dval <= 0.0)
				error = TRUE;
			else
				gmtdefs.y_axis_length = dval;
			break;
		case GMTCASE_X_ORIGIN:
			gmtdefs.x_origin = GMT_convert_units (value, GMT_INCH);
			break;
		case GMTCASE_Y_ORIGIN:
			gmtdefs.y_origin = GMT_convert_units (value, GMT_INCH);
			break;
		case GMTCASE_XY_TOGGLE:
			if (!strcmp (lower_value, "true"))
				gmtdefs.xy_toggle[0] = gmtdefs.xy_toggle[1] = TRUE;
			else if (!strcmp (lower_value, "false"))
				gmtdefs.xy_toggle[0] = gmtdefs.xy_toggle[1] = FALSE;
			else if (!strcmp (lower_value, "in")) {
				gmtdefs.xy_toggle[0] = TRUE;
				gmtdefs.xy_toggle[1] = FALSE;
			}
			else if (!strcmp (lower_value, "out")) {
				gmtdefs.xy_toggle[0] = FALSE;
				gmtdefs.xy_toggle[1] = TRUE;
			}
			else
				error = TRUE;
			break;
		case GMTCASE_Y_AXIS_TYPE:
			if (!strcmp (lower_value, "ver_text"))
				gmtdefs.y_axis_type = 1;
			else if (!strcmp (lower_value, "hor_text"))
				gmtdefs.y_axis_type = 0;
			else
				error = TRUE;
			break;
		case GMTCASE_INPUT_CLOCK_FORMAT:
			strncpy (gmtdefs.input_clock_format, value, GMT_TEXT_LEN);
			break;
		case GMTCASE_INPUT_DATE_FORMAT:
			strncpy (gmtdefs.input_date_format, value, GMT_TEXT_LEN);
			break;
		case GMTCASE_OUTPUT_CLOCK_FORMAT:
			strncpy (gmtdefs.output_clock_format, value, GMT_TEXT_LEN);
			break;
		case GMTCASE_OUTPUT_DATE_FORMAT:
			strncpy (gmtdefs.output_date_format, value, GMT_TEXT_LEN);
			break;
		case GMTCASE_OUTPUT_DEGREE_FORMAT:
			strncpy (gmtdefs.output_degree_format, value, GMT_TEXT_LEN);
			break;
		case GMTCASE_PLOT_CLOCK_FORMAT:
			strncpy (gmtdefs.plot_clock_format, value, GMT_TEXT_LEN);
			break;
		case GMTCASE_PLOT_DATE_FORMAT:
			strncpy (gmtdefs.plot_date_format, value, GMT_TEXT_LEN);
			break;
		case GMTCASE_PLOT_DEGREE_FORMAT:
			strncpy (gmtdefs.plot_degree_format, value, GMT_TEXT_LEN);
			GMT_backward.got_new_plot_format = TRUE;
			break;
		case GMTCASE_TIME_FORMAT:
		case GMTCASE_TIME_FORMAT_PRIMARY:
			strncpy (gmtdefs.time_format[0], value, GMT_TEXT_LEN);
			break;
		case GMTCASE_TIME_FORMAT_SECONDARY:
			strncpy (gmtdefs.time_format[1], value, GMT_TEXT_LEN);
			break;
		case GMTCASE_TIME_IS_INTERVAL:
			if (value[0] == '+' || value[0] == '-') {	/* OK, gave +<n>u or -<n>u, check for unit */
				sscanf (&lower_value[1], "%d%c", &GMT_truncate_time.T.step, &GMT_truncate_time.T.unit);
				switch (GMT_truncate_time.T.unit) {
					case 'y':
					case 'o':
					case 'd':
					case 'h':
					case 'm':
					case 'c':
						GMT_truncate_time.direction = (lower_value[0] == '+') ? 0 : 1;
						break;
					default:
						error = TRUE;
						break;
				}
				if (GMT_truncate_time.T.step == 0) error = TRUE;
				gmtdefs.time_is_interval = TRUE;
			}
			else if (!strcmp (lower_value, "off"))
				gmtdefs.time_is_interval = FALSE;
			else
				error = TRUE;
			break;
		case GMTCASE_TIME_INTERVAL_FRACTION:
			gmtdefs.time_interval_fraction = atof (value);
			break;
		case GMTCASE_WANT_LEAP_SECONDS:
			error = true_false_or_error (lower_value, &gmtdefs.want_leap_seconds);
			break;
		case GMTCASE_TIME_EPOCH:
			strncpy (gmtdefs.time_epoch, value, GMT_TEXT_LEN);
			strncpy (GMT_time_system[GMT_N_SYSTEMS-1].epoch, value, GMT_TEXT_LEN);
#ifndef OLDCAL
			gmtdefs.time_system = GMT_N_SYSTEMS-1;
#endif
			break;
		case GMTCASE_TIME_UNIT:
			gmtdefs.time_unit = GMT_time_system[GMT_N_SYSTEMS-1].unit = value[0];
			break;
		case GMTCASE_TIME_SYSTEM:
			gmtdefs.time_system = GMT_get_time_system (lower_value);
			if (gmtdefs.time_system < 0 || gmtdefs.time_system >= GMT_N_SYSTEMS) {
				error = TRUE;
				gmtdefs.time_system = 0;
			}
			break;
		case GMTCASE_TIME_WEEK_START:
			gmtdefs.time_week_start = GMT_key_lookup (value, GMT_weekdays, 7);
			if (gmtdefs.time_week_start < 0 || gmtdefs.time_week_start >= 7) {
				error = TRUE;
				gmtdefs.time_week_start = 0;
			}
			break;
		case GMTCASE_TIME_LANGUAGE:
			strncpy (gmtdefs.time_language, value, GMT_TEXT_LEN);
			GMT_str_tolower (gmtdefs.time_language);
			break;
		case GMTCASE_CHAR_ENCODING:
			gmtdefs.encoding.name = strdup (value);
			load_encoding (&gmtdefs.encoding);
			break;
		case GMTCASE_Y2K_OFFSET_YEAR:
			if ((gmtdefs.Y2K_offset_year = atoi (value)) < 0) error = TRUE;
			break;
		case GMTCASE_FIELD_DELIMITER:
			if (value[0] == '\0' || !strcmp (lower_value, "tab"))	/* DEFAULT */
				strncpy (gmtdefs.field_delimiter, "\t", 8);
			else if (!strcmp (lower_value, "space"))
				strncpy (gmtdefs.field_delimiter, " ", 8);
			else if (!strcmp (lower_value, "comma"))
				strncpy (gmtdefs.field_delimiter, ",", 8);
			else if (!strcmp (lower_value, "none"))
				gmtdefs.field_delimiter[0] = 0;
			else
				strncpy (gmtdefs.field_delimiter, value, 8);
			gmtdefs.field_delimiter[7] = 0;	/* Just a precaution */
			break;
		case GMTCASE_DEGREE_SYMBOL:
			if (value[0] == '\0' || !strcmp (lower_value, "ring"))	/* DEFAULT */
				gmtdefs.degree_symbol = gmt_ring;
			else if (!strcmp (lower_value, "degree"))
				gmtdefs.degree_symbol = gmt_degree;
			else if (!strcmp (lower_value, "colon"))
				gmtdefs.degree_symbol = gmt_colon;
			else if (!strcmp (lower_value, "none"))
				gmtdefs.degree_symbol = gmt_none;
			else
				error = TRUE;
			GMT_backward.got_new_degree_symbol = TRUE;
			break;
		case GMTCASE_ANNOT_FONT_SECONDARY:
		case GMTCASE_ANOT_FONT_SECONDARY:
		case GMTCASE_ANNOT_FONT2:
		case GMTCASE_ANOT_FONT2:
			if (value[0] >= '0' && value[0] <= '9')
				ival = atoi (value);
			else
				ival = GMT_font_lookup (value, GMT_font, N_FONTS);
			if (ival < 0 || ival >= N_FONTS)
				error = TRUE;
			else
				gmtdefs.annot_font[1] = ival;
			break;
		case GMTCASE_ANNOT_FONT_SIZE_SECONDARY:
		case GMTCASE_ANOT_FONT_SIZE_SECONDARY:
		case GMTCASE_ANNOT_FONT2_SIZE:
		case GMTCASE_ANOT_FONT2_SIZE:
			dval = atof (value);
			if (dval > 0.0)
				save_annot_size[1] = gmtdefs.annot_font_size[1] = dval;
			else
				error = TRUE;
			break;

		case GMTCASE_ANNOT_OFFSET_SECONDARY:
		case GMTCASE_ANOT_OFFSET_SECONDARY:
		case GMTCASE_ANNOT_OFFSET2:
		case GMTCASE_ANOT_OFFSET2:
			save_annot_offset[1] = gmtdefs.annot_offset[1] = GMT_convert_units (value, GMT_INCH);
			break;
		case GMTCASE_LABEL_OFFSET:
			save_label_offset = gmtdefs.label_offset = GMT_convert_units (value, GMT_INCH);
			break;
		case GMTCASE_HEADER_OFFSET:
			save_header_offset = gmtdefs.header_offset = GMT_convert_units (value, GMT_INCH);
			break;
		default:
			error = TRUE;
			fprintf (stderr, "%s: GMT SYNTAX ERROR in GMT_setparameter:  Unrecognized keyword %s\n",
				GMT_program, keyword);
			break;
	}

	if (error && case_val < N_KEYS) fprintf (stderr, "%s: GMT SYNTAX ERROR:  %s given illegal value (%s)!\n", GMT_program, keyword, value);
	return (error);
}

BOOLEAN true_false_or_error (char *value, int *answer)
{
	/* Assigns 1 or 0 to answer if value is true or false and return FALSE.
	 * If not given true or false, return error TRUE */

	if (!strcmp (value, "true")) {	/* TRUE */
		*answer = 1;
		return (FALSE);
	}
	if (!strcmp (value, "false")) {	/* FALSE */
		*answer = 0;
		return (FALSE);
	}

	/* Got neither true or false.  Make no assignment and return TRUE for error */

	return (TRUE);
}

int GMT_savedefaults (char *file)
{
	FILE *fp;
	char u, abbrev[4] = {'c', 'i', 'm', 'p'}, pm[2] = {'+', '-'};
	double s;

	if (!file)
		fp = GMT_stdout;
	else if ((fp = fopen (file, "w")) == NULL) {
		fprintf (stderr, "GMT: Could not create file %s\n", file);
		return (-1);
	}

	u = abbrev[gmtdefs.measure_unit];
	s = GMT_u2u[GMT_INCH][gmtdefs.measure_unit];	/* Convert from internal inch to users unit */

	fprintf (fp, "#\n#	GMT-SYSTEM %s Defaults file\n#\n", GMT_VERSION);
	fprintf (fp, "#-------- Plot Media Parameters -------------\n");
	fprintf (fp, "PAGE_COLOR		= %d/%d/%d\n", gmtdefs.page_rgb[0], gmtdefs.page_rgb[1], gmtdefs.page_rgb[2]);
	(gmtdefs.page_orientation & 1) ? fprintf (fp, "PAGE_ORIENTATION	= portrait\n") : fprintf (fp, "PAGE_ORIENTATION	= landscape\n");
	if (gmtdefs.media == -USER_MEDIA_OFFSET)
		fprintf (fp, "PAPER_MEDIA		= Custom_%dx%d", abs(gmtdefs.paper_width[0]), abs(gmtdefs.paper_width[1]));
	else if (gmtdefs.media >= USER_MEDIA_OFFSET)
		fprintf (fp, "PAPER_MEDIA		= %s", GMT_user_media_name[gmtdefs.media-USER_MEDIA_OFFSET]);
	else
		fprintf (fp, "PAPER_MEDIA		= %s", GMT_media_name[gmtdefs.media]);
	if (gmtdefs.paper_width[0] < 0)
		fprintf (fp, "-\n");
	else if (gmtdefs.paper_width[1] < 0)
		fprintf (fp, "+\n");
	else
		fprintf (fp, "\n");
	fprintf (fp, "#-------- Basemap Annotation Parameters ------\n");
	fprintf (fp, "ANNOT_MIN_ANGLE		= %g\n", gmtdefs.annot_min_angle);
	fprintf (fp, "ANNOT_MIN_SPACING	= %g\n", gmtdefs.annot_min_spacing);
	fprintf (fp, "ANNOT_FONT_PRIMARY	= %s\n", GMT_font[gmtdefs.annot_font[0]].name);
	(GMT_force_resize && !GMT_annot_special) ? fprintf (fp, "ANNOT_FONT_SIZE_PRIMARY	= +%gp\n", gmtdefs.annot_font_size[0]) :  fprintf (fp, "ANNOT_FONT_SIZE		= %gp\n", gmtdefs.annot_font_size[0]);
	(GMT_force_resize) ? fprintf (fp, "ANNOT_OFFSET_PRIMARY	= %g%c\n", save_annot_offset[0] * s, u) : fprintf (fp, "ANNOT_OFFSET_PRIMARY	= %g%c\n", gmtdefs.annot_offset[0] * s, u);
	fprintf (fp, "ANNOT_FONT_SECONDARY	= %s\n", GMT_font[gmtdefs.annot_font[1]].name);
	(GMT_force_resize) ? fprintf (fp, "ANNOT_FONT_SIZE_SECONDARY	= %gp\n", save_annot_size[1]) : fprintf (fp, "ANNOT_FONT_SIZE_SECONDARY	= %gp\n", gmtdefs.annot_font_size[1]);
	(GMT_force_resize) ? fprintf (fp, "ANNOT_OFFSET_SECONDARY = %g%c\n", save_annot_offset[1] * s, u) : fprintf (fp, "ANNOT_OFFSET_SECONDARY	= %g%c\n", gmtdefs.annot_offset[1] * s, u);
	fprintf (fp, "DEGREE_SYMBOL		= %s\n", GMT_degree_choice[gmtdefs.degree_symbol - gmt_none]);
	fprintf (fp, "HEADER_FONT		= %s\n", GMT_font[gmtdefs.header_font].name);
	(GMT_force_resize) ? fprintf (fp, "HEADER_FONT_SIZE	= %gp\n", save_header_size) : fprintf (fp, "HEADER_FONT_SIZE	= %gp\n", gmtdefs.header_font_size);
	(GMT_force_resize) ? fprintf (fp, "HEADER_OFFSET			= %g%cp\n", save_header_offset * s, u) : fprintf (fp, "HEADER_OFFSET		= %g%c\n", gmtdefs.header_offset * s, u);
	fprintf (fp, "LABEL_FONT		= %s\n", GMT_font[gmtdefs.label_font].name);
	(GMT_force_resize) ? fprintf (fp, "LABEL_FONT_SIZE		= %gp\n", save_label_size) : fprintf (fp, "LABEL_FONT_SIZE		= %gp\n", gmtdefs.label_font_size);
	(GMT_force_resize) ? fprintf (fp, "LABEL_OFFSET			= %g%cp\n", save_label_offset * s, u) : fprintf (fp, "LABEL_OFFSET		= %g%c\n", gmtdefs.label_offset * s, u);
	fprintf (fp, "OBLIQUE_ANNOTATION	= %d\n", gmtdefs.oblique_annotation);
	fprintf (fp, "PLOT_CLOCK_FORMAT	= %s\n", gmtdefs.plot_clock_format);
	fprintf (fp, "PLOT_DATE_FORMAT	= %s\n", gmtdefs.plot_date_format);
	fprintf (fp, "PLOT_DEGREE_FORMAT	= %s\n", gmtdefs.plot_degree_format);
	(gmtdefs.y_axis_type == 1) ? fprintf (fp, "Y_AXIS_TYPE		= ver_text\n") : fprintf (fp, "Y_AXIS_TYPE		= hor_text\n");
	fprintf (fp, "#-------- Basemap Layout Parameters ---------\n");
	fprintf (fp, "BASEMAP_AXES		= %s\n", gmtdefs.basemap_axes);
	fprintf (fp, "BASEMAP_FRAME_RGB	= %d/%d/%d\n", gmtdefs.basemap_frame_rgb[0],
		gmtdefs.basemap_frame_rgb[1], gmtdefs.basemap_frame_rgb[2]);
	if (gmtdefs.basemap_type == GMT_IS_PLAIN)
		fprintf (fp, "BASEMAP_TYPE		= plain\n");
	else if (gmtdefs.basemap_type == GMT_IS_FANCY)
		fprintf (fp, "BASEMAP_TYPE		= fancy\n");
	else if (gmtdefs.basemap_type == GMT_IS_ROUNDED)
		fprintf (fp, "BASEMAP_TYPE		= fancy+\n");
	fprintf (fp, "FRAME_PEN		= %s\n", GMT_putpen (&gmtdefs.frame_pen));
	(GMT_force_resize) ? fprintf (fp, "FRAME_WIDTH		= %g%c\n", save_frame_width * s, u) :  fprintf (fp, "FRAME_WIDTH		= %g%c\n", gmtdefs.frame_width * s, u);
	fprintf (fp, "GRID_CROSS_SIZE_PRIMARY	= %g%c\n", gmtdefs.grid_cross_size[0] * s, u);
	fprintf (fp, "GRID_CROSS_SIZE_SECONDARY	= %g%c\n", gmtdefs.grid_cross_size[1] * s, u);
	fprintf (fp, "GRID_PEN_PRIMARY	= %s\n", GMT_putpen (&gmtdefs.grid_pen[0]));
	fprintf (fp, "GRID_PEN_SECONDARY	= %s\n", GMT_putpen (&gmtdefs.grid_pen[1]));
	fprintf (fp, "MAP_SCALE_HEIGHT	= %g%c\n", gmtdefs.map_scale_height * s, u);
	(GMT_force_resize) ? fprintf (fp, "TICK_LENGTH		= %g%c\n", save_tick_length * s, u) :  fprintf (fp, "TICK_LENGTH		= %g%c\n", gmtdefs.tick_length * s, u);
	if (fabs (gmtdefs.polar_cap[0] - 90.0) < GMT_CONV_LIMIT)
		fprintf (fp, "POLAR_CAP		= none\n");
	else
		fprintf (fp, "POLAR_CAP		= %g/%g\n", gmtdefs.polar_cap[0], gmtdefs.polar_cap[1]);
	fprintf (fp, "TICK_PEN		= %s\n", GMT_putpen (&gmtdefs.tick_pen));
	fprintf (fp, "X_AXIS_LENGTH		= %g%c\n", gmtdefs.x_axis_length * s, u);
	fprintf (fp, "Y_AXIS_LENGTH		= %g%c\n", gmtdefs.y_axis_length * s, u);
	fprintf (fp, "X_ORIGIN		= %g%c\n", gmtdefs.x_origin * s, u);
	fprintf (fp, "Y_ORIGIN		= %g%c\n", gmtdefs.y_origin * s, u);
	(gmtdefs.unix_time) ? fprintf (fp, "UNIX_TIME		= TRUE\n") : fprintf (fp, "UNIX_TIME		= FALSE\n");
	fprintf (fp, "UNIX_TIME_POS		= %g%c/%g%c\n", gmtdefs.unix_time_pos[0] * s, u, gmtdefs.unix_time_pos[1] * s, u);
	fprintf (fp, "#-------- Color System Parameters -----------\n");
	if (gmtdefs.background_rgb[0] == -1)
		fprintf (fp, "COLOR_BACKGROUND	= -\n");
	else
		fprintf (fp, "COLOR_BACKGROUND	= %d/%d/%d\n", gmtdefs.background_rgb[0], gmtdefs.background_rgb[1], gmtdefs.background_rgb[2]);
	if (gmtdefs.foreground_rgb[0] == -1)
		fprintf (fp, "COLOR_FOREGROUND	= -\n");
	else
		fprintf (fp, "COLOR_FOREGROUND	= %d/%d/%d\n", gmtdefs.foreground_rgb[0], gmtdefs.foreground_rgb[1], gmtdefs.foreground_rgb[2]);
	if (gmtdefs.nan_rgb[0] == -1)
		fprintf (fp, "COLOR_NAN		= -\n");
	else
		fprintf (fp, "COLOR_NAN		= %d/%d/%d\n", gmtdefs.nan_rgb[0], gmtdefs.nan_rgb[1], gmtdefs.nan_rgb[2]);
	fprintf (fp, "COLOR_IMAGE		= ");
	if (gmtdefs.color_image == 0)
		fprintf (fp, "adobe\n");
	else if (gmtdefs.color_image == 1)
		fprintf (fp, "tiles\n");
	if (gmtdefs.color_model & GMT_USE_HSV)
		fprintf (fp, "COLOR_MODEL		= +hsv\n");
	else if (gmtdefs.color_model & GMT_READ_HSV)
		fprintf (fp, "COLOR_MODEL		= hsv\n");
	else if (gmtdefs.color_model & GMT_USE_CMYK)
		fprintf (fp, "COLOR_MODEL		= +cmyk\n");
	else if (gmtdefs.color_model & GMT_READ_CMYK)
		fprintf (fp, "COLOR_MODEL		= cmyk\n");
	else if (gmtdefs.color_model & GMT_USE_RGB)
		fprintf (fp, "COLOR_MODEL		= +rgb\n");
	else
		fprintf (fp, "COLOR_MODEL		= rgb\n");
	fprintf (fp, "HSV_MIN_SATURATION	= %g\n", gmtdefs.hsv_min_saturation);
	fprintf (fp, "HSV_MAX_SATURATION	= %g\n", gmtdefs.hsv_max_saturation);
	fprintf (fp, "HSV_MIN_VALUE		= %g\n", gmtdefs.hsv_min_value);
	fprintf (fp, "HSV_MAX_VALUE		= %g\n", gmtdefs.hsv_max_value);
	fprintf (fp, "#-------- PostScript Parameters -------------\n");
	fprintf (fp, "CHAR_ENCODING		= %s\n", gmtdefs.encoding.name);
	fprintf (fp, "DOTS_PR_INCH		= %d\n", gmtdefs.dpi);
	fprintf (fp, "N_COPIES		= %d\n", gmtdefs.n_copies);
	if (gmtdefs.ps_colormode == 0)
		fprintf (fp, "PS_COLOR		= rgb\n");
	else if (gmtdefs.ps_colormode == 1)
		fprintf (fp, "PS_COLOR		= cmyk\n");
	else
		fprintf (fp, "PS_COLOR		= hsv\n");
	if (gmtdefs.ps_compress == 1)
		fprintf (fp, "PS_IMAGE_COMPRESS	= rle\n");
	else if (gmtdefs.ps_compress == 2)
		fprintf (fp, "PS_IMAGE_COMPRESS	= lzw\n");
	else
		fprintf (fp, "PS_IMAGE_COMPRESS	= none\n");
	(gmtdefs.ps_heximage) ? fprintf (fp, "PS_IMAGE_FORMAT		= ascii\n") : fprintf (fp, "PS_IMAGE_FORMAT		= bin\n");
	if (gmtdefs.ps_line_cap == 0)
		fprintf (fp, "PS_LINE_CAP		= butt\n");
	else if (gmtdefs.ps_line_cap == 1)
		fprintf (fp, "PS_LINE_CAP		= round\n");
	else
		fprintf (fp, "PS_LINE_CAP		= square\n");
	if (gmtdefs.ps_line_join == 0)
		fprintf (fp, "PS_LINE_JOIN		= miter\n");
	else if (gmtdefs.ps_line_join == 1)
		fprintf (fp, "PS_LINE_JOIN		= round\n");
	else
		fprintf (fp, "PS_LINE_JOIN		= bevel\n");
	fprintf (fp, "PS_MITER_LIMIT		= %d\n", gmtdefs.ps_miter_limit);
	(gmtdefs.ps_verbose) ? fprintf (fp, "PS_VERBOSE			= TRUE\n") : fprintf (fp, "PS_VERBOSE			= FALSE\n");
	fprintf (fp, "GLOBAL_X_SCALE		= %g\n", gmtdefs.global_x_scale);
	fprintf (fp, "GLOBAL_Y_SCALE		= %g\n", gmtdefs.global_y_scale);
	fprintf (fp, "#-------- I/O Format Parameters -------------\n");
	fprintf (fp, "D_FORMAT		= %s\n", gmtdefs.d_format);
	if (!strcmp (gmtdefs.field_delimiter, "\t"))
		fprintf (fp, "FIELD_DELIMITER		= tab\n");
	else if (!strcmp (gmtdefs.field_delimiter, " "))
		fprintf (fp, "FIELD_DELIMITER		= space\n");
	else if (!strcmp (gmtdefs.field_delimiter, ","))
		fprintf (fp, "FIELD_DELIMITER		= comma\n");
	else if (gmtdefs.field_delimiter[0] == 0)
		fprintf (fp, "FIELD_DELIMITER		= none\n");
	else
		fprintf (fp, "FIELD_DELIMITER		= %s\n", gmtdefs.field_delimiter);
	(gmtdefs.gridfile_shorthand) ? fprintf (fp, "GRIDFILE_SHORTHAND	= TRUE\n") : fprintf (fp, "GRIDFILE_SHORTHAND	= FALSE\n");
	fprintf (fp, "GRID_FORMAT       	= %s\n", gmtdefs.grid_format);
	fprintf (fp, "INPUT_CLOCK_FORMAT	= %s\n", gmtdefs.input_clock_format);
	fprintf (fp, "INPUT_DATE_FORMAT	= %s\n", gmtdefs.input_date_format);
	(gmtdefs.io_header[GMT_IN]) ? fprintf (fp, "IO_HEADER		= TRUE\n") : fprintf (fp, "IO_HEADER		= FALSE\n");
	fprintf (fp, "N_HEADER_RECS		= %d\n", gmtdefs.n_header_recs);
	fprintf (fp, "OUTPUT_CLOCK_FORMAT	= %s\n", gmtdefs.output_clock_format);
	fprintf (fp, "OUTPUT_DATE_FORMAT	= %s\n", gmtdefs.output_date_format);
	fprintf (fp, "OUTPUT_DEGREE_FORMAT	= %s\n", gmtdefs.output_degree_format);
	if (gmtdefs.xy_toggle[0] && gmtdefs.xy_toggle[1])
		fprintf (fp, "XY_TOGGLE	= TRUE\n");
	else if (!gmtdefs.xy_toggle[0] && !gmtdefs.xy_toggle[1])
		fprintf (fp, "XY_TOGGLE		= FALSE\n");
	else if (gmtdefs.xy_toggle[0] && !gmtdefs.xy_toggle[1])
		fprintf (fp, "XY_TOGGLE		= IN\n");
	else
		fprintf (fp, "XY_TOGGLE		= OUT\n");
	fprintf (fp, "#-------- Projection Parameters -------------\n");
	fprintf (fp, "ELLIPSOID		= %s\n", gmtdefs.ref_ellipsoid[gmtdefs.ellipsoid].name);
	if (gmtdefs.map_scale_factor == -1.0)
		fprintf (fp, "MAP_SCALE_FACTOR	= default\n");
	else
		fprintf (fp, "MAP_SCALE_FACTOR	= %g\n", gmtdefs.map_scale_factor);
	fprintf (fp, "MEASURE_UNIT		= %s\n", GMT_unit_names[gmtdefs.measure_unit]);
	fprintf (fp, "#-------- Calendar/Time Parameters ----------\n");
	fprintf (fp, "TIME_FORMAT_PRIMARY	= %s\n", gmtdefs.time_format[0]);
	fprintf (fp, "TIME_FORMAT_SECONDARY	= %s\n", gmtdefs.time_format[1]);
	fprintf (fp, "TIME_EPOCH		= %s\n", gmtdefs.time_epoch);
	if (gmtdefs.time_is_interval)
		fprintf (fp, "TIME_IS_INTERVAL	= %c%d%c\n", pm[GMT_truncate_time.direction], GMT_truncate_time.T.step, GMT_truncate_time.T.unit);
	else
		fprintf (fp, "TIME_IS_INTERVAL	= OFF\n");
	fprintf (fp, "TIME_INTERVAL_FRACTION	= %g\n", gmtdefs.time_interval_fraction);
	fprintf (fp, "TIME_LANGUAGE		= %s\n", gmtdefs.time_language);
	fprintf (fp, "TIME_SYSTEM		= %s\n", GMT_time_system[gmtdefs.time_system].name);
	fprintf (fp, "TIME_UNIT		= %c\n", gmtdefs.time_unit);
	fprintf (fp, "TIME_WEEK_START		= %s\n", GMT_weekdays[gmtdefs.time_week_start]);
	/*
	 * PW 4/2/03: LEAP_SECONDS is commented out for output until we actually want to implement this feature.  We still process on input to avoid error messages.
	(gmtdefs.want_leap_seconds) ? fprintf (fp, "WANT_LEAP_SECONDS	= TRUE\n") : fprintf (fp, "WANT_LEAP_SECONDS	= FALSE\n");
	 *
	 */
	fprintf (fp, "Y2K_OFFSET_YEAR		= %d\n", gmtdefs.Y2K_offset_year);
	fprintf (fp, "#-------- Miscellaneous Parameters ----------\n");
	fprintf (fp, "INTERPOLANT		= ");
	if (gmtdefs.interpolant == 0)
		fprintf (fp, "linear\n");
	else if (gmtdefs.interpolant == 1)
		fprintf (fp, "akima\n");
	else if (gmtdefs.interpolant == 2)
		fprintf (fp, "cubic\n");
	fprintf (fp, "LINE_STEP		= %g%c\n", gmtdefs.line_step * s, u);
	fprintf (fp, "VECTOR_SHAPE		= %g\n", gmtdefs.vector_shape);
	(gmtdefs.verbose) ? fprintf (fp, "VERBOSE			= TRUE\n") : fprintf (fp, "VERBOSE			= FALSE\n");

	if (fp != GMT_stdout) fclose (fp);

	return (0);
}

void GMT_getdefaults (char *this_file)	/* Read user's .gmtdefaults4 file and initialize parameters */
{
	int i;
	char file[BUFSIZ];

	/* Set up hash table for colornames */

	GMT_hash_init (GMT_rgb_hashnode, GMT_color_name, GMT_N_COLOR_NAMES, GMT_N_COLOR_NAMES);

	 /* Default is to draw AND annotate all sides */
	for (i = 0; i < 5; i++) frame_info.side[i] = 2;

	if (!this_file) {	/* Must figure out if there is a .gmtdefaults[4] file to use */

		if (! (GMT_getuserpath (".gmtdefaults4", file) || GMT_getuserpath (".gmtdefaults", file))) {
			/* No .gmtdefaults[4] files in sight; Must use GMT system defaults */
			char *path;
			path = GMT_getdefpath (0);
			strcpy (file, path);
			GMT_free ((void *)path);
		}
	}
	else
		strcpy (file, this_file);

	(void) GMT_loaddefaults (file);

}

BOOLEAN GMT_getuserpath (char *stem, char *path)
{
	/* Stem is the name of the file, e.g., .gmtdefaults4 or .gmtdefaults */
	/* path is the full path to the file in question */
	/* Returns TRUE if a workable path was found (sets path as well) */

	char *homedir;
        static char home [] = "HOME";

	/* First look in the current working directory */

	if (!access (stem, R_OK)) {	/* Yes, found it */
		strcpy (path, stem);
		return (TRUE);
	}

	/* Not found, see if there is a file in the user's home directory */

	if ((homedir = getenv (home)) == NULL) {	/* Oh well, probably a CGI script... */
		fprintf (stderr, "GMT Warning: Could not determine home directory!\n");
		return (FALSE);
	}

	/* See if the file is present in the home dir */

	sprintf (path, "%s%c%s", homedir, DIR_DELIM, stem);
	if (!access (path, R_OK)) return (TRUE);	/* Yes, use the file in HOME */

	return (FALSE);	/* No file found, give up */
}

char *GMT_getdefpath (int get)
{
	/* Return the full path to the chosen .gmtdefaults4 system file
	 * depending on the value of get:
	 * get = 0:	Use gmt.conf to set get to 1 or 2.
	 * get = 1:	Use the SI settings.
	 * get = 2:	Use the US settings. */

	int id;
	char line[BUFSIZ], *path, *suffix[2] = {"SI", "US"};
	FILE *fp;

	GMT_set_home ();

	if (get == 0) {	/* Must use GMT system defaults via gmt.conf */

		sprintf (line, "%s%cshare%cgmt.conf", GMTHOME, DIR_DELIM, DIR_DELIM);
		if ((fp = fopen (line, "r")) == NULL) {
			fprintf (stderr, "GMT Fatal Error: Cannot open/find GMT configuration file %s\n", line);
			exit (EXIT_FAILURE);
		}

		while (fgets (line, BUFSIZ, fp) && (line[0] == '#' || line[0] == '\n'));	/* Scan to first real line */
		fclose (fp);
		if (!strncmp (line, "US", 2))
			id = 2;
		else if (!strncmp (line, "SI", 2))
			id = 1;
		else {
			fprintf (stderr, "GMT Fatal Error: No SI/US keyword in GMT configuration file ($GMTHOME/share/gmt.conf)\n");
			exit (EXIT_FAILURE);
		}
	}
	else
		id = get;

	id--;	/* Get 0 or 1 */
	sprintf (line, "%s%cshare%c.gmtdefaults_%s", GMTHOME, DIR_DELIM, DIR_DELIM, suffix[id]);

	path = (char *) GMT_memory (VNULL, (size_t)(strlen (line) + 1), (size_t)1, GMT_program);

	strcpy (path, line);

	return (path);
}

double GMT_convert_units (char *from, int new_format)
{
	/* Converts the input value to new units indicated by way */

	int c = 0, len, old_format;
	BOOLEAN have_unit = FALSE;
	double value;

	if ((len = strlen(from))) {
		c = from[len-1];
		if ((have_unit = isalpha (c))) from[len-1] = '\0';	/* Temporarily remove unit */
	}

	old_format = GMT_unit_lookup (c);

	value = atof (from) * GMT_u2u[old_format][new_format];
	if (have_unit) from[len-1] = c;	/* Put back what we took out temporarily */

	return (value);

}

int GMT_unit_lookup (int c)
{
	int unit;

	switch (c) {
		case 'C':	/* Centimeters */
		case 'c':
			unit = GMT_CM;
			break;
		case 'I':	/* Inches */
		case 'i':
			unit = GMT_INCH;
			break;
		case 'M':	/* Meters */
		case 'm':
			unit = GMT_M;
			break;
		case 'P':	/* Points */
		case 'p':
			unit = GMT_PT;
			break;
		default:
			unit = gmtdefs.measure_unit;
			break;
	}

	return (unit);
}

int GMT_hash_lookup (char *key, struct GMT_HASH *hashnode, int n, int n_hash)
{
	int i;
	struct GMT_HASH *this;

	i = GMT_hash (key, n_hash);

	if (i >= n || i < 0 || !hashnode[i].next) return (-1);	/* Bad key */
	this = hashnode[i].next;
	while (this && strcmp (this->key, key)) this = this->next;

	return ((this) ? this->id : -1);
}

void GMT_hash_init (struct GMT_HASH *hashnode, char **keys, int n_hash, int n_keys)
{
	int i, entry;
	struct GMT_HASH *this;

	/* Set up hash table */

	for (i = 0; i < n_hash; i++) hashnode[i].next = (struct GMT_HASH *)0;
	for (i = 0; i < n_keys; i++) {
		entry = GMT_hash (keys[i], n_hash);
		this = &hashnode[entry];
		while (this->next) this = this->next;
		this->next = (struct GMT_HASH *)GMT_memory (VNULL, (size_t)1, sizeof (struct GMT_HASH), GMT_program);
		this->next->key = keys[i];
		this->next->id = i;
	}
}

int GMT_hash (char *v, int n_hash)
{
        int h;
        for (h = 0; *v != '\0'; v++) h = (64 * h + (*v)) % n_hash;
        return (h);
}

int GMT_get_ellipsoid (char *name)
{
	int i, n;

	for (i = 0; i < N_ELLIPSOIDS && strcmp (name, gmtdefs.ref_ellipsoid[i].name); i++);

	if (i == N_ELLIPSOIDS) {	/* Try to open as file first in (1) current dir, then in (2) $GMTHOME/share */
		FILE *fp;
		char line[BUFSIZ], path[BUFSIZ];

		sprintf (path, "%s%cshare%c%s", GMTHOME, DIR_DELIM, DIR_DELIM, name);

		if (!strcmp ("Sphere", name)) {
			/* Special case where previous setting in .gmtdefaults4 is a custom ellipse which
			 * is now stored in the place of "Sphere".  Hence the entry for "Sphere" is no longer
			 * there and we must reinsert manually here */
			 i = N_ELLIPSOIDS - 1;
			 strcpy (gmtdefs.ref_ellipsoid[i].name, "Sphere");
			 gmtdefs.ref_ellipsoid[i].date = 1980;
			 gmtdefs.ref_ellipsoid[i].eq_radius = 6371008.7714;
			 gmtdefs.ref_ellipsoid[i].pol_radius = gmtdefs.ref_ellipsoid[i].eq_radius;
			 gmtdefs.ref_ellipsoid[i].flattening = 0.0;
		}
		else if ((fp = fopen (name, "r")) == NULL && (fp = fopen (path, "r")) == NULL) {
			i = -1;	/* Failed, give error */
		}
		else {	/* Found file, now get parameters */
			i = N_ELLIPSOIDS - 1;
			while (fgets (line, BUFSIZ, fp) && (line[0] == '#' || line[0] == '\n'));
			fclose (fp);
			n = sscanf (line, "%s %d %lf %lf %lf", gmtdefs.ref_ellipsoid[i].name,
				&gmtdefs.ref_ellipsoid[i].date, &gmtdefs.ref_ellipsoid[i].eq_radius,
				&gmtdefs.ref_ellipsoid[i].pol_radius, &gmtdefs.ref_ellipsoid[i].flattening);
			if (n != 5) {
				fprintf (stderr, "GMT: Error decoding user ellipsoid parameters (%s)\n", line);
				exit (EXIT_FAILURE);
			}

			if (gmtdefs.ref_ellipsoid[i].pol_radius > 0.0 && gmtdefs.ref_ellipsoid[i].flattening < 0.0) {
				/* negative flattening means we must compute flattening from the polar and equatorial radii: */

				gmtdefs.ref_ellipsoid[i].flattening = 1.0 - (gmtdefs.ref_ellipsoid[i].pol_radius / gmtdefs.ref_ellipsoid[i].eq_radius);
				fprintf (stderr, "GMT: user-supplied ellipsoid has implicit flattening of %.8f\n", gmtdefs.ref_ellipsoid[i].flattening);
				if (gmtdefs.verbose) fprintf (stderr, "GMT: user-supplied ellipsoid has flattening of %s%.8f\n",
					gmtdefs.ref_ellipsoid[i].flattening != 0.0 ? "1/" : "", gmtdefs.ref_ellipsoid[i].flattening != 0.0 ? 1./gmtdefs.ref_ellipsoid[i].flattening : 0.0);
			}
			/* else check consistency: */
			else if (gmtdefs.ref_ellipsoid[i].pol_radius > 0.0 && fabs(gmtdefs.ref_ellipsoid[i].flattening - 1.0 + (gmtdefs.ref_ellipsoid[i].pol_radius/gmtdefs.ref_ellipsoid[i].eq_radius)) > 1.0e-11) {
				fprintf (stderr, "GMT Warning: Possible inconsistency in user ellipsoid parameters (%s)\n", line);
				/* exit (EXIT_FAILURE); */
			}
		}
	}

	return (i);
}

int GMT_get_datum (char *name)
{
	int i;

	for (i = 0; i < N_DATUMS && strcmp (name, gmtdefs.datum[i].name); i++);
	if (i == N_DATUMS) return (-1);	/* Error */
	return (i);
}

int GMT_key_lookup (char *name, char **list, int n)
{
	int i;

	for (i = 0; i < n && strcmp (name, list[i]); i++);
	return (i);
}

int GMT_font_lookup (char *name, struct GMT_FONT *list, int n)
{
	int i;

	for (i = 0; i < n && strcmp (name, list[i].name); i++);
	return (i);
}

int GMT_load_user_media (void) {	/* Load any user-specified media formats */
	int n, n_alloc, w, h;
	char line[BUFSIZ], media[80];
	FILE *fp;

	GMT_set_home ();

	sprintf (line, "%s%cshare%cgmtmedia.d", GMTHOME, DIR_DELIM, DIR_DELIM);
	if ((fp = fopen (line, "r")) == NULL) return (0);

	n_alloc = GMT_TINY_CHUNK;
	GMT_user_media = (struct GMT_MEDIA *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (struct GMT_MEDIA), GMT_program);
	GMT_user_media_name = (char **) GMT_memory (VNULL, (size_t)n_alloc, sizeof (char *), GMT_program);

	n = 0;
	while (fgets (line, BUFSIZ, fp)) {
		if (line[0] == '#' || line[0] == '\n') continue;

		sscanf (line, "%s %d %d", media, &w, &h);

		/* Convert string to lower case */

		GMT_str_tolower (media);

		GMT_user_media_name[n] = (char *) GMT_memory (VNULL, (size_t)(strlen(media)+1), sizeof (char), GMT_program);
		strcpy (GMT_user_media_name[n], media);
		GMT_user_media[n].width  = w;
		GMT_user_media[n].height = h;
		n++;
		if (n == n_alloc) {
			n_alloc += GMT_TINY_CHUNK;
			GMT_user_media = (struct GMT_MEDIA *) GMT_memory ((void *)GMT_user_media, (size_t)n_alloc, sizeof (struct GMT_MEDIA), GMT_program);
			GMT_user_media_name = (char **) GMT_memory ((void *)GMT_user_media_name, (size_t)n_alloc, sizeof (char *), GMT_program);
		}
	}
	fclose (fp);

	GMT_user_media = (struct GMT_MEDIA *) GMT_memory ((void *)GMT_user_media, (size_t)n, sizeof (struct GMT_MEDIA), GMT_program);
	GMT_user_media_name = (char **) GMT_memory ((void *)GMT_user_media_name, (size_t)n, sizeof (char *), GMT_program);

	return (n);
}

int GMT_get_time_system (char *name)
{
	int i;

	for (i = 0; i < GMT_N_SYSTEMS && strcmp (name, GMT_time_system[i].name); i++);
	return (i);
}

int GMT_get_char_encoding (char *name)
{
	int i;

	for (i = 0; i < 7 && strcmp (name, GMT_weekdays[i]); i++);
	return (i);
}

void GMT_get_time_language (char *name)
{
	FILE *fp;
	char file[BUFSIZ], line[BUFSIZ], full[16], abbrev[16], c[16], dwu;
	int i, nm = 0, nw = 0, nu = 0;

	sprintf (file, "%s%cshare%ctime%c%s.d", GMTHOME, DIR_DELIM, DIR_DELIM, DIR_DELIM, name);
	if ((fp = fopen (file, "r")) == NULL) {
		fprintf (stderr, "GMT Warning: Could not load %s - revert to us (English)!\n", name);
		sprintf (file, "%s%cshare%ctime%cus.d", GMTHOME, DIR_DELIM, DIR_DELIM, DIR_DELIM);
		if ((fp = fopen (file, "r")) == NULL) {
			fprintf (stderr, "GMT Error: Could not find %s!\n", file);
			exit (EXIT_FAILURE);
		}
		strcpy (gmtdefs.time_language, "us");
	}

	while (fgets (line, BUFSIZ, fp)) {
		if (line[0] == '#' || line[0] == '\n') continue;
		sscanf (line, "%c %d %s %s %s", &dwu, &i, full, abbrev, c);
		if (dwu == 'M') {	/* Month record */
			strncpy (GMT_time_language.month_name[0][i-1], full, 16);
			strncpy (GMT_time_language.month_name[1][i-1], abbrev, 16);
			strncpy (GMT_time_language.month_name[2][i-1], c, 16);
			nm += i;
		}
		else if (dwu == 'W') {	/* Weekday record */
			strncpy (GMT_time_language.day_name[0][i-1], full, 16);
			strncpy (GMT_time_language.day_name[1][i-1], abbrev, 16);
			strncpy (GMT_time_language.day_name[2][i-1], c, 16);
			nw += i;
		}
		else {			/* Week name record */
			strncpy (GMT_time_language.week_name[0], full, 16);
			strncpy (GMT_time_language.week_name[1], abbrev, 16);
			strncpy (GMT_time_language.week_name[2], c, 16);
			nu += i;
		}
	}
	fclose (fp);
	if (! (nm == 78 && nw == 28 && nu == 1)) {	/* Sums of 1-12, 1-7, and 1, respectively */
		fprintf (stderr, "GMT Error: Mismatch between expected and actual contents in %s!\n", file);
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < 12; i++) {	/* Get upper-case abbreviated month names for i/o */
		strcpy (month_names[i], GMT_time_language.month_name[1][i]);
		GMT_str_toupper (month_names[i]);
		months[i] = month_names[i];
	}
	GMT_hash_init (GMT_month_hashnode, months, 12, 12);
}

void GMT_setshorthand (void) {/* Read user's .gmt_io file and initialize shorthand notation */
	int n = 0, n_alloc;
	char file[BUFSIZ], line[BUFSIZ], *homedir, a[10], b[20], c[20], d[20], e[20];
        static char home [] = "HOME";
	FILE *fp;

	if ((homedir = getenv (home)) == NULL) {
		fprintf (stderr, "GMT Warning: Could not determine home directory!\n");
		return;
	}
	sprintf (file, "%s%c.gmt_io", homedir, DIR_DELIM);
	if ((fp = fopen (file, "r")) == NULL) return;

	n_alloc = GMT_SMALL_CHUNK;
	GMT_file_id = (int *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (int), GMT_program);
	GMT_file_scale = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), GMT_program);
	GMT_file_offset = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), GMT_program);
	GMT_file_nan = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), GMT_program);
	GMT_file_suffix = (char **) GMT_memory (VNULL, (size_t)n_alloc, sizeof (char *), GMT_program);

	while (fgets (line, BUFSIZ, fp)) {
		if (line[0] == '#' || line[0] == '\n') continue;
		sscanf (line, "%s %s %s %s %s", a, b, c, d, e);
		GMT_file_suffix[n] = (char *) GMT_memory (VNULL, (size_t)(strlen(a)+1), sizeof (char), GMT_program);
		strcpy (GMT_file_suffix[n], a);
		GMT_file_id[n] = GMT_grd_format_decoder (b);
		GMT_file_scale[n] = (strcmp (c, "-")) ? atof (c) : 1.0;
		GMT_file_offset[n] = (strcmp (d, "-")) ? atof (d) : 0.0;
		GMT_file_nan[n] = (strcmp (e, "-")) ? atof (e) : GMT_d_NaN;
		n++;
		if (n == n_alloc) {
			n_alloc += GMT_SMALL_CHUNK;
			GMT_file_id = (int *) GMT_memory ((void *)GMT_file_id, (size_t)n_alloc, sizeof (int), GMT_program);
			GMT_file_scale = (double *) GMT_memory ((void *)GMT_file_scale, (size_t)n_alloc, sizeof (double), GMT_program);
			GMT_file_offset = (double *) GMT_memory ((void *)GMT_file_offset, (size_t)n_alloc, sizeof (double), GMT_program);
			GMT_file_nan = (double *) GMT_memory ((void *)GMT_file_nan, (size_t)n_alloc, sizeof (double), GMT_program);
			GMT_file_suffix = (char **) GMT_memory ((void *)GMT_file_suffix, (size_t)n_alloc, sizeof (char *), GMT_program);
		}
	}
	fclose (fp);
	GMT_n_file_suffix = n;
	GMT_file_id = (int *) GMT_memory ((void *)GMT_file_id, (size_t)GMT_n_file_suffix, sizeof (int), GMT_program);
	GMT_file_scale = (double *) GMT_memory ((void *)GMT_file_scale, (size_t)GMT_n_file_suffix, sizeof (double), GMT_program);
	GMT_file_offset = (double *) GMT_memory ((void *)GMT_file_offset, (size_t)GMT_n_file_suffix, sizeof (double), GMT_program);
	GMT_file_nan = (double *) GMT_memory ((void *)GMT_file_nan, (size_t)GMT_n_file_suffix, sizeof (double), GMT_program);
	GMT_file_suffix = (char **) GMT_memory ((void *)GMT_file_suffix, (size_t)GMT_n_file_suffix, sizeof (char *), GMT_program);
}

int GMT_begin (int argc, char **argv)
{
	/* GMT_begin will merge the command line arguments with the arguments
	 * that were used the last time this program was called (if any).  This
	 * way one only has to give -R -J to repeat previous map projection
	 * instead of spelling out the -R and projection parameters every time.
	 * The information is stored in the first line of the file .gmtcommands4
	 * in the current directory [or optionally a provided filename] and will
	 * contain the last arguments to the common parameters like
	 * -B, -H, -J, -K, -O, -P, -R, -U, -V, -X, -Y, -c
	 * Since the meaning of -X/-Y depends on whether we have an overlay,
	 * we maintain -X -Y for absolute shifts and -x -y for relative shifts.
	 * If the argument +file is encountered then file is used in lieu of the
	 * usual .gmtdefaults4 file and this argument is chopped from argv
	 */

	int i, j, k, n;
	char *this;

#ifdef __FreeBSD__
	/* allow divide by zero -- Inf */
	fpsetmask (fpgetmask () & ~(FP_X_DZ | FP_X_INV));
#endif
	/* Initialize parameters that don'd depend on .gmtdefaults */

	GMT_stdin  = stdin;
	GMT_stdout = stdout;

	GMT_set_home ();

	GMT_init_fonts (&N_FONTS);
	this = CNULL;
	GMT_make_fnan (GMT_f_NaN);
	GMT_make_dnan (GMT_d_NaN);
	GMT_oldargc = 0;
	frame_info.plot = FALSE;
	project_info.projection = -1;
	project_info.gave_map_width = 0;
	project_info.region = TRUE;
	project_info.compute_scale[0] =  project_info.compute_scale[1] = project_info.compute_scale[2] = FALSE;
	project_info.x_off_supplied = project_info.y_off_supplied = FALSE;
	project_info.region_supplied = FALSE;
	for (j = 0; j < 10; j++) project_info.pars[j] = 0.0;
	project_info.xmin = project_info.ymin = 0.0;
	project_info.z_level = DBL_MAX;	/* Will be set in map_setup */
	project_info.xyz_pos[0] = project_info.xyz_pos[1] = TRUE;
	GMT_prepare_3D ();
	gmtdefs.dlon = (project_info.e - project_info.w) / gmtdefs.n_lon_nodes;
	gmtdefs.dlat = (project_info.n - project_info.s) / gmtdefs.n_lat_nodes;
	for (i = 0; i < 4; i++) project_info.edge[i] = TRUE;
	GMT_grdio_init ();
	for (i = 0; i < N_UNIQUE; i++) GMT_oldargv[i] = CNULL;
	for (i = strlen(argv[0]); i >= 0 && argv[0][i] != '/'; i--);
	GMT_program = &argv[0][i+1];
	frame_info.check_side = frame_info.horizontal = FALSE;
	project_info.f_horizon = 90.0;
	GMT_distance_func = (PFD) GMT_great_circle_dist_km;
	

	/* Set the gmtdefault parameters from the $HOME/.gmtdefaults4 (if any) */

        /* See if user specified +optional_defaults_file.  If so, assign filename to this and remove option from argv */

	for (i = j = 1; i < argc; i++) {
		argv[j] = argv[i];
		if (argv[j][0] == '+' && argv[i][1] && !access (&argv[i][1], R_OK))
			this = &argv[i][1];
		else
			j++;
	}
	argc = j;

	GMT_get_history (argc, argv);	/* Replace command shorthands */

	GMT_getdefaults (this);

	/* See if user specified -- defaults on the command line [Pr Dave Ball's suggestion].
	 * If found, apply option and remove from argv.  These options only apply to current process. */

	for (i = j = 1, n = 0; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] ) {
			if ((this = strchr (argv[i], '='))) {	/* Got --PAR=VALUE */
				this[0] = '\0';
				n += GMT_setparameter (&argv[i][2], &this[1]);
			}
			else				/* Got --PAR */
				n += GMT_setparameter (&argv[i][2], "TRUE");
		}
		else
			argv[j++] = argv[i];
	}
	argc = j;
	if (n) fprintf (stderr, "%s:  %d conversion errors from command-line default override settings!\n", GMT_program, n);

	GMT_prep_PS_bits ();	/* In case --DPARs were given */

	GMT_init_ellipsoid ();	/* Set parameters depending on the ellipsoid */

	GMT_init_time_system_structure ();

	if (GMT_got_frame_rgb) {	/* Must enforce change of frame, tick, and grid pen rgb */
		memcpy ((void *)gmtdefs.frame_pen.rgb, (void *)gmtdefs.basemap_frame_rgb, (size_t)(3 * sizeof (int)));
		memcpy ((void *)gmtdefs.tick_pen.rgb,  (void *)gmtdefs.basemap_frame_rgb, (size_t)(3 * sizeof (int)));
		memcpy ((void *)gmtdefs.grid_pen[0].rgb,   (void *)gmtdefs.basemap_frame_rgb, (size_t)(3 * sizeof (int)));
		memcpy ((void *)gmtdefs.grid_pen[1].rgb, (void *)gmtdefs.basemap_frame_rgb, (size_t)(3 * sizeof (int)));
	}

	GMT_io_init ();			/* Init the table i/o structure */

	GMT_get_time_language (gmtdefs.time_language);

	if (gmtdefs.gridfile_shorthand) GMT_setshorthand ();

	/* Copy various colors to GMT_BFN_COLOR structure */

	memset ((void *)(&GMT_bfn), 0, 3*sizeof (struct GMT_BFN_COLOR));
	memcpy ((void *)GMT_bfn[GMT_FGD].rgb, (void *)gmtdefs.foreground_rgb, 3 * sizeof (int));
	memcpy ((void *)GMT_bfn[GMT_BGD].rgb, (void *)gmtdefs.background_rgb, 3 * sizeof (int));
	memcpy ((void *)GMT_bfn[GMT_NAN].rgb, (void *)gmtdefs.nan_rgb, 3 * sizeof (int));
	for (k = 0; k < 3; k++) if (GMT_bfn[k].rgb[0] == -1) GMT_bfn[k].skip = TRUE;

	/* Make sure -b options are parsed first in case filenames are given
	 * before -b options on the command line.  This would only cause grief
	 * under WIN32. Also make -J come first and -R before -I, if present.
	 * We also run -f through in case -: is given.
	 * Finally, we look for -V so verbose is set prior to testing arguments */

	for (i = 1, j = k = n = 0; i < argc; i++) {
		if (!strncmp (argv[i], "-V", 2)) gmtdefs.verbose = TRUE;
		if (!strncmp (argv[i], "-b", 2)) GMT_io_selection (&argv[i][2]);
		if (!strncmp (argv[i], "-f", 2)) GMT_decode_coltype (&argv[i][2]);
		if (!strncmp (argv[i], "-J", 2)) j = i;
		if (!strncmp (argv[i], "-R", 2)) k = i;
		if (!strncmp (argv[i], "-I", 2)) n = i;
	}
	if (j > 1) {	/* rotate arguments to ensure that the -J option is processed before -B -R */
		char *p;
		p = argv[j];
		for (i = j; i > 1; i --) argv[i] = argv[i-1];
		argv[1] = p;
		if (k > 0 && k < j) k++;	/* Because this arg was shifted */
		if (n > 0 && n < j) n++;	/* Because this arg was shifted */
	}
	if (k > 0 && n > 0 && (n < k)) {	/* Both -R and -I, but -I came first.  Switch order */
		char *p;
		p = argv[k];
		argv[k] = argv[n];
		argv[n] = p;
	}
	if (gmtdefs.verbose) gmtdefs.page_orientation |= 2;
	return (argc);
}

void GMT_end (int argc, char **argv)
{
	/* GMT_end will clean up after us. */

	int i, j;
	struct GMT_HASH *p, *current;

	for (i = 0; i < N_UNIQUE; i++) if (GMT_oldargv[i]) GMT_free ((void *)GMT_oldargv[i]);
	if (GMT_lut) GMT_free ((void *)GMT_lut);
	GMT_free_plot_array ();
	/* Remove allocated hash structures */
	for (i = 0; i < 12; i++) {
		p = GMT_month_hashnode[i].next;
		while ((current = p)) {
			p = p->next;
			GMT_free ((void *)current);
		}
	}
	for (i = 0; i < GMT_N_COLOR_NAMES; i++) {
		p = GMT_rgb_hashnode[i].next;
		while ((current = p)) {
			p = p->next;
			GMT_free ((void *)current);
		}
	}
	for (i = 0; i < HASH_SIZE; i++) {
		p = hashnode[i].next;
		while ((current = p)) {
			p = p->next;
			GMT_free ((void *)current);
		}
	}

#ifdef __FreeBSD__
	fpresetsticky (FP_X_DZ | FP_X_INV);
	fpsetmask (FP_X_DZ | FP_X_INV);
#endif

	GMT_free ((void *)GMT_io.skip_if_NaN);
	GMT_free ((void *)GMT_io.in_col_type);
	GMT_free ((void *)GMT_io.out_col_type);
	
	for (i = 0; i < 3; i++) for (j = 0; j < 2; j++) if (GMT_plot_format[i][j]) GMT_free ((void *)GMT_plot_format[i][j]);

	fflush (GMT_stdout);	/* Make sure output buffer is flushed */

	exit (EXIT_SUCCESS);
}

void GMT_set_home (void)
{
	char *this;

	if (GMTHOME) return;	/* Already set elsewhere */

	if ((this = getenv ("GMTHOME")) == CNULL) {	/* Use default GMT path */
		GMTHOME = (char *) GMT_memory (VNULL, (size_t)(strlen (GMT_DEFAULT_PATH) + 1), (size_t)1, "GMT");
 		strcpy (GMTHOME, GMT_DEFAULT_PATH);
	}
	else {	/* Use user's default path */
		GMTHOME = (char *) GMT_memory (VNULL, (size_t)(strlen (this) + 1), (size_t)1, "GMT");
		strcpy (GMTHOME, this);
	}
	if ((this = getenv ("GMT_CPTDIR")) != CNULL) {	/* GMT_CPTDIR was set */
		GMT_CPTDIR = (char *) GMT_memory (VNULL, (size_t)(strlen (this) + 1), (size_t)1, "GMT");
		strcpy (GMT_CPTDIR, this);
	}
	if ((this = getenv ("GMT_DATADIR")) != CNULL) {	/* GMT_DATADIR was set */
		GMT_DATADIR = (char *) GMT_memory (VNULL, (size_t)(strlen (this) + 1), (size_t)1, "GMT");
		strcpy (GMT_DATADIR, this);
	}
	if ((this = getenv ("GMT_GRIDDIR")) != CNULL) {	/* GMT_GRIDDIR was set */
		GMT_GRIDDIR = (char *) GMT_memory (VNULL, (size_t)(strlen (this) + 1), (size_t)1, "GMT");
		strcpy (GMT_GRIDDIR, this);
	}
	if ((this = getenv ("GMT_IMGDIR")) != CNULL) {	/* GMT_IMGDIR was set */
		GMT_IMGDIR = (char *) GMT_memory (VNULL, (size_t)(strlen (this) + 1), (size_t)1, "GMT");
		strcpy (GMT_IMGDIR, this);
	}
}

void GMT_put_history (int argc, char **argv)
{
	/* GMT_put_history will update the .gmtcommands4
	 * file and write out the common parameters.
	 */

	int i, j, k, found_new, found_old, old_k = -1;
	BOOLEAN no_new_j = TRUE;
#ifndef NO_LOCK
	struct flock lock;
#endif

	/* First check that -X -Y was done correctly */

	if ((project_info.x_off_supplied && project_info.y_off_supplied) && GMT_x_abs != GMT_y_abs) {
		fprintf (stderr, "%s: GMT SYNTAX ERROR: -X -Y must both be absolute or relative\n", GMT_program);
		exit (EXIT_FAILURE);
	}
	if (GMT_x_abs && GMT_y_abs) gmtdefs.page_orientation |= 8;

	/* Ok, now update history, first rewind file pointer */

	if (!GMT_fp_history) return;	/* There was no such file after all */

	rewind (GMT_fp_history);

	fprintf (GMT_fp_history, "# GMT common arguments shelf\n");

	for (i = 0; i < N_UNIQUE; i++) {        /* Loop over GMT_unique_option parameters */

		GMT_processed_option[i] = FALSE;	/* Not processed this option yet */

		/* First see if an updated value exist for this common parameter */

		for (j = 1, found_new = FALSE; !found_new && j < argc; j++) {
			if (argv[j][0] != '-') continue;
			if (GMT_unique_option[i][0] == 'J') /* Range of -J? options */
				found_new = !strncmp (&argv[j][1], GMT_unique_option[i], 2);
			else
				found_new = (argv[j][1] == GMT_unique_option[i][0]);
		}

		if (found_new) { /* Need to store this updated value */
			fprintf (GMT_fp_history, "%s\n", argv[j-1]);
			if (GMT_unique_option[i][0] == 'J') {	/* Make this the last -J? of any kind, identified by lower-case -j */
				fprintf (GMT_fp_history, "-j%s\n", &argv[j-1][2]);
				no_new_j = FALSE;
			}
		}
		else  {	 	/* Need to find and store the old value if any */
			for (k = 0, found_old = FALSE; !found_old && k < GMT_oldargc; k++) {
				if (GMT_oldargv[k][0] != '-') continue;
				if (no_new_j && GMT_oldargv[k][1] == 'j')	/* Old -j option*/
					old_k = k;
				else if (GMT_unique_option[i][0] == 'J') /* Range of -J? options */
					found_old = !strncmp (&GMT_oldargv[k][1], GMT_unique_option[i], 2);
				else
					found_old = (GMT_oldargv[k][1] == GMT_unique_option[i][0]);
			}

			if (found_old)  /* Keep old value */
				fprintf (GMT_fp_history, "%s\n", GMT_oldargv[k-1]);
		}
	}
	if (no_new_j && old_k >= 0) fprintf (GMT_fp_history, "%s\n", GMT_oldargv[old_k]);	/* Write out old -j */
	fprintf (GMT_fp_history, "EOF\n");	/* Logical end of file marker (since old file may be longer) */
	fflush (GMT_fp_history);		/* To ensure all is written when lock is released */

#ifndef NO_LOCK
	lock.l_type = F_UNLCK;		/* Release lock and close file */
	lock.l_whence = SEEK_SET;	/* These three apply lock to entire file */
	lock.l_start = lock.l_len = 0;

	if (GMT_lock && fcntl (GMT_fd_history, F_SETLK, &lock)) {
		fprintf (stderr, "%s: Error returned by fcntl [F_UNLCK]\n", GMT_program);
		exit (EXIT_FAILURE);
	}
#endif

	fclose (GMT_fp_history);
}

void GMT_get_history (int argc, char ** argv)
{
	int i, j;
	BOOLEAN nothing_to_do = FALSE, need_xy = FALSE, overlay = FALSE, found, done;
	char line[BUFSIZ], hfile[BUFSIZ], cwd[BUFSIZ], *homedir;
        static char home [] = "HOME";
#ifndef NO_LOCK
	struct flock lock;
#endif

	/* Open .gmtcommands4 file and retrive old argv (if any)
	 * This is tricky since GMT programs are often hooked together
	 * with pipes so it actually has happened that the first program
	 * is updating the .gmtdefaults4 file while the second tries to
	 * read.  The result is that the second program often cannot expand
	 * the shorthand and fails with error message.  In GMT 3.1 we introduced
	 * Advisory File Locking and also update the .gmtcommands4 file as soon
	 * as possible so that commands down the pipe will see the new options
	 * already have taken effect.
	 */

	/* If current directory is writable, use it; else use the home directory */

	(void) getcwd (cwd, BUFSIZ);
	if (!access (cwd, W_OK)) {	/* Current directory is writable */
		sprintf (hfile, ".gmtcommands4");
	}
	else {	/* Try home directory instead */
		if ((homedir = getenv (home)) == NULL) {
			fprintf (stderr, "GMT Warning: Could not determine home directory nor write in current directory!\n");
			return;
		}
		sprintf (hfile, "%s%c.gmtcommands4", homedir, DIR_DELIM);
	}

	if (access (hfile, R_OK)) {    /* No .gmtcommands4 file in chosen directory, try to make one */
		if ((GMT_fp_history = fopen (hfile, "w")) == NULL) {
			fprintf (stderr, "GMT Warning: Could not create %s [permission problem?]\n", hfile);
			return;
		}
		nothing_to_do = TRUE;
	}
	else if ((GMT_fp_history = fopen (hfile, "r+")) == NULL) {
		fprintf (stderr, "GMT Warning: Could not update %s [permission problem?]\n", hfile);
		return;
	}

	/* When we get here the file exists */

#ifndef NO_LOCK
	/* now set exclusive lock */

	lock.l_type = F_WRLCK;		/* Lock for [exclusive] reading/writing */
	lock.l_whence = SEEK_SET;	/* These three apply lock to entire file */
	lock.l_start = lock.l_len = 0;

	GMT_fd_history = fileno (GMT_fp_history);	/* Get file descriptor */

	if (GMT_lock && fcntl (GMT_fd_history, F_SETLKW, &lock)) {	/* Will wait for file to be ready for reading */
		fprintf (stderr, "%s: Error returned by fcntl [F_WRLCK]\n", GMT_program);
		exit (EXIT_FAILURE);
	}

#endif

	if (nothing_to_do) return;	/* No processing, use original args */

	/* If we get here there is an old file to process */
	/* Get the common arguments and copy them to array GMT_oldargv */

	done = FALSE;
	while (!done && fgets (line, BUFSIZ, GMT_fp_history)) {

		if (line[0] == '#' || line[0] == '\n') continue;	/* Skip comments or blank lines */
		if (!strncmp (line, "EOF", 3)) {	/* Logical end of .gmtcommands4 file */
			done = TRUE;
			continue;
		}
		if (line[0] != '-') continue;	/* Possibly reading old .gmtcommands4 format or junk */
		line[strlen(line)-1] = 0;
		GMT_oldargv[GMT_oldargc] = (char *) GMT_memory (VNULL, (size_t)(strlen (line) + 1), 1, "GMT");
		strcpy (GMT_oldargv[GMT_oldargc], line);
		GMT_oldargc++;
		if (GMT_oldargc > N_UNIQUE) {
			fprintf (stderr, "GMT Fatal Error: Failed while decoding common arguments\n");
			exit (EXIT_FAILURE);
		}
	}

	/* See if (1) We need abs/rel shift and (2) if we have an overlay */

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') continue;
		if (argv[i][1] == 'X' || argv[i][1] == 'Y' || argv[i][1] == 'x' || argv[i][1] == 'y') need_xy = TRUE;
		if (argv[i][1] == 'O' || argv[i][1] == 'o') overlay = TRUE;
	}

	overlay = (need_xy && overlay); /* -O means overlay only if -X or -Y has been specified */

	/* Change x/y to upper case if no overlay and change X/Y to lower case if overlay */

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') continue;
		if (overlay) {
			if (argv[i][1] == 'X') argv[i][1] = 'x';
			if (argv[i][1] == 'Y') argv[i][1] = 'y';
		}
		else {
			if (argv[i][1] == 'x') argv[i][1] = 'X';
			if (argv[i][1] == 'y') argv[i][1] = 'Y';
		}
	}

	/* Loop over argv and merge argv and GMT_oldargv */

	for (i = 1; i < argc; i++) {

		/* Skip if filename or option has modifiers */

		if (argv[i][0] != '-') continue;			/* Not an option argument */
		if (argv[i][1] != 'J' && argv[i][2] != 0) continue;	/* 1-char option with modifier */
		if (argv[i][1] == 'J' && argv[i][2] != 0 && argv[i][3] != 0) continue;	/* 2-char option with modifier */

		for (j = 0, found = FALSE; !found && j < GMT_oldargc; j++) {
			if (argv[i][1] == 'J' && argv[i][2] == '\0')	/* Given -J only, match with -j */
				found = (GMT_oldargv[j][1] == 'j');
			else if (argv[i][1] == 'J')			/* Given -J? */
				found = (GMT_oldargv[j][1] == argv[i][1] && GMT_oldargv[j][2] == argv[i][2]);
			else						/* All other options */
				found = (GMT_oldargv[j][1] == argv[i][1]);
		}
		j--;

		/* Skip if not found or GMT_oldargv has no modifiers */

		if (!found) continue;

		if (argv[i][1] != 'J' && GMT_oldargv[j][2] == 0) continue;
		if (argv[i][1] == 'J' && GMT_oldargv[j][3] == 0) continue;

		/* Here, GMT_oldargv has modifiers and argv don't, set pointer */

		argv[i] = GMT_oldargv[j];
		if (argv[i][1] == 'j') argv[i][1] = 'J';	/* Reset to upper case */
	}
}

/* Here is the new -B parser with all its sub-functions */

void GMT_strip_colonitem (const char *in, const char *pattern, char *item, char *out) {
	/* Removes the searched-for item from in, returns it in item, with the rest in out.
	 * pattern is usually ":." for title, ":," for unit, and ":" for label.
	 * ASSUMPTION: Only pass ":" after first removing titles and units
	 */

	char *s;
	BOOLEAN error = FALSE;

	if ((s = strstr (in, pattern))) {		/* OK, found what we are looking for */
		int i, j, k;
		k = (int)(s - in);			/* Start index of item */
		strncpy (out, in, k);			/* Copy everything up to the pattern */
		i = k + strlen (pattern);		/* Now go to beginning of item */
		j = 0;
		while (in[i] && in[i] != ':') item[j++] = in[i++];	/* Copy the item... */
		item[j] = '\0';				/* ...and terminate the string */
		if (in[i] != ':') error = TRUE;		/* Error: Missing terminating colon */
		i++;					/* Skip the ending colon */
		while (in[i]) out[k++] = in[i++];	/* Copy rest to out... */
		out[k] = '\0';				/* .. and terminate */
	}
	else {	/* No item to update */
		strcpy (out, in);
	}

	if (error) {	/* Problems with decoding */
		fprintf (stderr, "%s: ERROR: Missing terminating colon in -B string %s\n", GMT_program, in);
		exit (EXIT_FAILURE);
	}
	if (strstr (out, pattern) && !strcmp (pattern, ":.")) {	/* Problems with decoding title */
		fprintf (stderr, "%s: ERROR: More than one title in  -B string %s\n", GMT_program, in);
		exit (EXIT_FAILURE);
	}
	if (strstr (out, pattern) && !strcmp (pattern, ":,")) {	/* Problems with decoding unit */
		fprintf (stderr, "%s: ERROR: More than one unit string in  -B component %s\n", GMT_program, in);
		exit (EXIT_FAILURE);
	}
	if (strstr (out, pattern) && !strcmp (pattern, ":=")) {	/* Problems with decoding prefix */
		fprintf (stderr, "%s: ERROR: More than one prefix string in  -B component %s\n", GMT_program, in);
		exit (EXIT_FAILURE);
	}
	if (strstr (out, pattern)) {	/* Problems with decoding label */
		fprintf (stderr, "%s: ERROR: More than one label string in  -B component %s\n", GMT_program, in);
		exit (EXIT_FAILURE);
	}
}

void GMT_strip_wesnz (const char *in, int t_side[], BOOLEAN *draw_box, char *out) {
	/* Removes the WESNZwesnz+ flags and sets the side/drawbox parameters
	 * and return the resulting stripped string
	 */

	BOOLEAN set_sides = FALSE, mute = FALSE;
	int i, k, set, side[5] = {0, 0, 0, 0, 0};

	for (i = k = 0; in[i]; i++) {
		if (in[i] == ':') mute = !mute;	/* Toggle so that mute is TRUE when we are within a :<stuff>: string */
		if (mute) {	/* Dont look for WEST inside a label */
			out[k++] = in[i];
			continue;
		}
		set = 0;
		switch (in[i]) {
			case 'W':	/* Draw AND Annotate */
				set++;
			case 'w':	/* Just Draw */
				side[3] = ++set;
				set_sides = TRUE;
				break;
			case 'E':	/* Draw AND Annotate */
				set++;
			case 'e':	/* Just Draw */
				if (i > 0 && (in[i-1] == '.' || isdigit (in[i-1])) && (in[i+1] && (isdigit (in[i+1]) || in[i+1] == '-' || in[i+1] == '+')))	/* Exponential notation */
					out[k++] = in[i];
				else {
					side[1] = ++set;
					set_sides = TRUE;
				}
				break;
			case 'S':	/* Draw AND Annotate */
				set++;
			case 's':	/* Just Draw */
				side[0] = ++set;
				set_sides = TRUE;
				break;
			case 'N':	/* Draw AND Annotate */
				set++;
			case 'n':	/* Just Draw */
				side[2] = ++set;
				set_sides = TRUE;
				break;
			case 'Z':	/* Draw AND Annotate */
				set++;
			case 'z':	/* Just Draw */
				side[4] = ++set;
				set_sides = TRUE;
				if (in[i+1] == '+') *draw_box = TRUE, i++;
				break;
			default:	/* Anything else is copy */
				out[k++] = in[i];
				break;
		}
	}
	out[k] = '\0';	/* Terminate string */

	if (set_sides) for (i = 0; i < 5; i++) t_side[i] = side[i];	/* Only changes these if WESN was provided */
}

void GMT_split_info (const char *in, char *info[]) {
	/* Take the -B string (minus the leading -B) and chop into 3 strings for x, y, and z */

	BOOLEAN mute = FALSE;

	int i, n_slash, s_pos[2];

	for (i = n_slash = 0; in[i] && n_slash < 3; i++) {
		if (in[i] == ':') mute = !mute;
		if (in[i] == '/' && !mute) {	/* Axis-separating slash, not a slash in a label */
			s_pos[n_slash++] = i;
		}
	}

	if (n_slash == 3) {
		fprintf (stderr, "%s: Error splitting -B string %s\n", GMT_program, in);
		exit (EXIT_FAILURE);
	}

	if (n_slash == 2) {	/* Got x/y/z */
		i = strlen (in);
		strncpy (info[0], in, s_pos[0]);				info[0][s_pos[0]] = '\0';
		strncpy (info[1], &in[s_pos[0]+1], s_pos[1] - s_pos[0] - 1);	info[1][s_pos[1] - s_pos[0] - 1] = '\0';
		strncpy (info[2], &in[s_pos[1]+1], i - s_pos[1] - 1);		info[2][i - s_pos[1] - 1] = '\0';
	}
	else if (n_slash == 1) {	/* Got x/y */
		i = strlen (in);
		strncpy (info[0], in, s_pos[0]);				info[0][s_pos[0]] = '\0';
		strncpy (info[1], &in[s_pos[0]+1], i - s_pos[0] - 1);		info[1][i - s_pos[0] - 1] = '\0';
		info[2][0] = '\0';			/* Zero out the z info */
	}
	else {	/* Got x with implicit copy to y */
		strcpy (info[0], in);
		strcpy (info[1], in);
		info[2][0] = '\0';			/* Zero out the z info */
	}
}

void GMT_decode_tinfo (char *in, struct PLOT_AXIS *A) {
	/* Decode the annot/tick segments of the clean -B string pieces */

	char *t, *s, flag, orig_flag = 0, unit;
	int error = 0;
	BOOLEAN time_interval_unit;
	double val, phase = 0.0;

	if (!in) return;	/* NULL pointer passed */

	t = in;
	while (t[0] && !error) {	/* As long as there are more segments to decode and no trouble so far */
		if (isdigit (t[0]) || t[0] == '-' || t[0] == '+' || t[0] == '.')	/* No segment type given, set to * which means a + f */
			flag = '*';
		else {
			flag = t[0];	/* Set flag */
			if (!strchr ("afg", flag)) {	/* Illegal flag given */
				error = 1;
				continue;
			}
			t++;		/* Skip to next */
			if (!t[0]) {
				error = 2;
				continue;
			}
		}

		/* Here, t must point to a valid number.  If t[0] is not [+,-,.] followed by a digit we have an error */

		if (!(isdigit (t[0]) || ((t[0] == '-' || t[0] == '+' || t[0] == '.') && strlen(t) > 1))) {
			error = 2;
			continue;
		}
		/* Decode interval, get pointer to next segment */
		if ((val = strtod (t, &s)) < 0.0) {			/* Interval must be >= 0 */
			error = 3;
			continue;
		}
		if (s[0] && (s[0] == '-' || s[0] == '+')) {	/* Phase shift information given */
			t = s;
			phase = strtod (t, &s);
		}
		if (s[0] && strchr ("YyOoUuKkJjDdHhMmCcrRlp", s[0])) {	/* Appended one of the allowed units, or l or p for log10/pow */
			unit = s[0];
			s++;
		}
		else if (A->type == TIME)				/* Default time system unit implied */
			unit = GMT_time_system[gmtdefs.time_system].unit;
		else
			unit = 0;	/* Not specified */

		/* else s is either 0 or points to the next segment */

		switch (unit) {	/* Determine if we have intervals or moments */
			case 'Y':
			case 'y':
			case 'O':
			case 'o':
			case 'K':
			case 'k':
			case 'J':
			case 'j':
			case 'D':
			case 'd':
			case 'R':
			case 'r':
			case 'U':
			case 'u':
				if (A->type == TIME && flag == 'a') flag = 'i';
				time_interval_unit = TRUE;
				break;
			default:
				time_interval_unit = FALSE;
				break;
		}
		orig_flag = flag;
		if (GMT_primary) {	/* Since this is primary axes items */
			if (flag == '*' && time_interval_unit) flag = '+';
		}
		else {			/* Since this is secondary axes items */
			if (flag == '*')
			 	flag = (time_interval_unit) ? '-' : '^';
			else
			 	flag = (char) toupper ((int)flag);
		}
		if (!error) GMT_set_titem (A, val, phase, flag, unit);				/* Store the findings for this segment */
		t = s;									/* Make t point to start of next segment, if any */
	}

	if (error) {
		switch (error) {
			case 1:
				fprintf (stderr, "%s: ERROR: Unrecognized axis item or unit %c in -B string component %s\n", GMT_program, orig_flag, in);
				break;
			case 2:
				fprintf (stderr, "%s: ERROR: Interval missing from -B string component %s\n", GMT_program, in);
				break;
			case 3:
				fprintf (stderr, "%s: ERROR: Negative intervaln -B string component %s\n", GMT_program, in);
				break;
			default:
				break;
		}
		exit (EXIT_FAILURE);
	}
}

void GMT_set_titem (struct PLOT_AXIS *A, double val, double phase, char flag, char unit) {
	/* Load the values into the appropriate PLOT_AXIS_ITEM structure */

	int i, n = 1;
	struct PLOT_AXIS_ITEM *I[2];
	char item_flag[8] = {'a', 'A', 'i', 'I', 'f', 'F', 'g', 'G'}, *format;

	if (A->type == TIME) {	/* Strict check on time intervals */
		if (GMT_verify_time_step (irint (val), unit)) exit (EXIT_FAILURE);
		if ((fmod (val, 1.0) > GMT_CONV_LIMIT)) {
			fprintf (stderr, "%s: ERROR: Time step interval (%g) must be an integer\n", GMT_program, val);
			exit (EXIT_FAILURE);
		}
	}

	switch (flag) {
		case 'a':	/* Upper tick annotation */
			I[0] = &A->item[0];
			break;
		case 'A':	/* Lower tick annotation */
			I[0] = &A->item[1];
			break;
		case 'i':	/* Upper interval annotation */
			I[0] = &A->item[2];
			break;
		case 'I':	/* Lower interval annotation */
			I[0] = &A->item[3];
			break;
		case 'f':	/* Upper Frame tick interval */
			I[0] = &A->item[4];
			break;
		case 'F':	/* Lower Frame tick interval */
			I[0] = &A->item[5];
			break;
		case 'g':	/* Upper Gridline interval */
			I[0] = &A->item[6];
			break;
		case 'G':	/* Lower gridline interval */
			I[0] = &A->item[7];
			break;
		case '*':	/* Both a and f */
			I[0] = &A->item[0];
			I[1] = &A->item[4];
			n = 2;
			break;
		case '+':	/* Both i and f */
			I[0] = &A->item[2];
			I[1] = &A->item[4];
			n = 2;
			break;
		case '^':	/* Both A and F */
			I[0] = &A->item[1];
			I[1] = &A->item[5];
			n = 2;
			break;
		case '-':	/* Both I and F */
			I[0] = &A->item[3];
			I[1] = &A->item[5];
			n = 2;
			break;
		default:	/* Bad flag should never get here */
			fprintf (stderr, "%s: Bad flag passed to GMT_set_titem\n", GMT_program);
			exit (EXIT_FAILURE);
			break;
	}

	switch (unit) {
		case 'l':	/* Log10 annotation flag */
			A->type = LOG10;
			unit = 0;
			break;
		case 'p':	/* pow annotation flag */
			A->type = POW;
			unit = 0;
			break;
		default:
			break;
	}

	if (phase != 0.0) A->phase = phase;	/* phase must apply to entire axis */
	for (i = 0; i < n; i++) {
		if (I[i]->active == 1) {
			fprintf (stderr, "%s: Warning: Axis sub-item %c set more than once (typo?)\n", GMT_program, item_flag[i]);
		}
		I[i]->interval = val;
		I[i]->unit = unit;
		I[i]->type = (flag == 'I' || flag == 'i') ? 'I' : 'A';
		I[i]->flavor = 0;
		I[i]->active = n;
		I[i]->upper_case = FALSE;
		format = (GMT_primary) ? gmtdefs.time_format[0] : gmtdefs.time_format[1];
		switch (format[0]) {	/* This parameter controls which version of month/day textstrings we use for plotting */
			case 'F':	/* Full name, upper case */
				I[i]->upper_case = TRUE;
			case 'f':	/* Full name, lower case */
				I[i]->flavor = 0;
				break;
			case 'A':	/* Abbreviated name, upper case */
				I[i]->upper_case = TRUE;
			case 'a':	/* Abbreviated name, lower case */
				I[i]->flavor = 1;
				break;
			case 'C':	/* 1-char name, upper case */
				I[i]->upper_case = TRUE;
			case 'c':	/* 1-char name, lower case */
				I[i]->flavor = 2;
				break;
			default:
				break;
		}
	}
}

int GMT_map_getframe (char *in) {
	/* GMT_map_getframe scans an argument string and extract parameters that
	 * set the interval for tickmarks and annotations on the boundary.
	 * The string must be continuous, i.e. no whitespace must be present
	 * The string may have 1, 2,  or 3 parts, separated by a slash '/'. All
	 * info after the first slash are assigned to the y-axis.  Info after
	 * the second slash are assigned to the z-axis.  If there is no
	 * slash, x-values are copied to y-values.
	 * A substring looks like [t][value][m|c]. The [t] and [m|c] are optional
	 * ([ and ] are NOT part of the string and are just used to clarify)
	 * [t] can be any of [a](annotation int), [f](frame int), or [g](gridline int).
	 * Default is a AND f. The [m], if present indicates value is in minutes.
	 * The [c], if present indicates value is in seConds (s already used for south...).
	 * Text between : and : are labels for the respective axes. If the first
	 * character of the text is a period, then the rest of the text is used
	 * as the plot title.  If it is a comma, then the rest is used as annotation unit.
	 * For LINEAR axes: If the first characters in args are one or more of w,e,s,n
	 * only those axes will be drawn. Upper case letters means the chosen axes
	 * also will be annotated. Default is all 4 axes drawn/annotated.
	 * For logscale plots:  l will cause log10(x) to be plotted
	 *			p will cause 10 ^ log10(x) to be plotted
	 *	annot/tick/grid interval can here be either:
	 *		1.0	-> Only powers of 10 are annotated
	 *		2.0	-> powers of 10 times (1, 2, 5) are annotated
	 *		3.0	-> powers of 10 times (1,2,3,..9) are annotated
	 *
	 * Up to two -B options may be given on the command line:
	 *	-B[p] the primary specifications
	 *	-Bs   the secondary specifications
	 *
	 *	-Bs must be in addition to -B[p].
	 */

	char out1[BUFSIZ], out2[BUFSIZ], out3[BUFSIZ], *info[3];
	char one[80], two[80], three[80];
	struct PLOT_AXIS *A;
	int i, j, k;

	if (in[0] == 's') {
		GMT_primary = FALSE;
		k = 1;
	}
	else if (in[0] == 'p') {
		GMT_primary = TRUE;
		k = 1;
	}
	else {
		GMT_primary = TRUE;
		k = 0;
	}
	/* frame_info.side[] may be set already when parsing .gmtdefaults4 flags */

	info[0] = one;	info[1] = two;	info[2] = three;
	if (!frame_info.plot) {	/* First time we initialize stuff */
		for (i = 0; i < 3; i++) {
			memset ((void *)&frame_info.axis[i], 0, sizeof (struct PLOT_AXIS));
			for (j = 0; j < 8; j++) {
				frame_info.axis[i].item[j].parent = i;
				frame_info.axis[i].item[j].id = j;
			}
			if (project_info.xyz_projection[i] == TIME) frame_info.axis[i].type = TIME;
		}
		frame_info.header[0] = '\0';
		frame_info.plot = TRUE;
		frame_info.draw_box = FALSE;
	}

	GMT_strip_colonitem (&in[k], ":.", frame_info.header, out1);			/* Extract header string, if any */

	GMT_strip_wesnz (out1, frame_info.side, &frame_info.draw_box, out2);		/* Decode WESNZwesnz+ flags, if any */

	GMT_split_info (out2, info);					/* Chop/copy the three axis strings */

	for (i = 0; i < 3; i++) {					/* Process each axis separately */

		if (!info[i][0]) continue;

		GMT_strip_colonitem (info[i], ":,", frame_info.axis[i].unit, out1);	/* Pull out annotation unit, if any */
		GMT_strip_colonitem (out1, ":=", frame_info.axis[i].prefix, out2);	/* Pull out annotation prefix, if any */
		GMT_strip_colonitem (out2, ":", frame_info.axis[i].label, out3);	/* Pull out axis label, if any */

		GMT_decode_tinfo (out3, &frame_info.axis[i]);				/* Decode the annotation intervals */

		/* Make sure we have ticks to match annotation stride */
		A = &frame_info.axis[i];
		if (A->item[GMT_ANNOT_UPPER].active && !A->item[GMT_TICK_UPPER].active)	/* Set frame ticks = annot stride */
			memcpy ((void *)&A->item[GMT_TICK_UPPER], (void *)&A->item[GMT_ANNOT_UPPER], sizeof (struct PLOT_AXIS_ITEM));
		else if (A->item[GMT_INTV_UPPER].active && !A->item[GMT_TICK_UPPER].active)	/* Set frame ticks = annot stride */
			memcpy ((void *)&A->item[GMT_TICK_UPPER], (void *)&A->item[GMT_INTV_UPPER], sizeof (struct PLOT_AXIS_ITEM));
		if (A->item[GMT_ANNOT_LOWER].active && !A->item[GMT_TICK_LOWER].active)	/* Set frame ticks = annot stride */
			memcpy ((void *)&A->item[GMT_TICK_LOWER], (void *)&A->item[GMT_ANNOT_LOWER], sizeof (struct PLOT_AXIS_ITEM));
		else if (A->item[GMT_INTV_LOWER].active && !A->item[GMT_TICK_LOWER].active)	/* Set frame ticks = annot stride */
			memcpy ((void *)&A->item[GMT_TICK_LOWER], (void *)&A->item[GMT_INTV_LOWER], sizeof (struct PLOT_AXIS_ITEM));
	}

	/* Check if we asked for linear projections of geographic coordinates and did not specify a unit - is so set degree symbol as unit */
	if (project_info.projection == LINEAR && gmtdefs.degree_symbol != gmt_none) {
		if (project_info.degree[0] && GMT_io.in_col_type[0] == GMT_IS_LON && frame_info.axis[0].unit[0] == 0) {
			frame_info.axis[0].unit[0] = '-';
			frame_info.axis[0].unit[1] = gmtdefs.encoding.code[gmtdefs.degree_symbol];
			frame_info.axis[0].unit[2] = '\0';
		}
		if (project_info.degree[1] && GMT_io.in_col_type[1] == GMT_IS_LAT && frame_info.axis[1].unit[0] == 0) {
			frame_info.axis[1].unit[0] = '-';
			frame_info.axis[1].unit[1] = gmtdefs.encoding.code[gmtdefs.degree_symbol];
			frame_info.axis[1].unit[2] = '\0';
		}
	}

	return (0);
}

int GMT_map_getproject (char *args)
{
	/* GMT_map_getproject scans the arguments given and extracts the parameters needed
	 * for the specified map projection. These parameters are passed through the
	 * project_info structure.  The function returns TRUE if an error is encountered.
	 */

	int i, j, k = 9, n, slash, l_pos[2], p_pos[2], t_pos[2], d_pos[2], id, project = -1, n_slashes = 0, width_given;
	BOOLEAN error = FALSE, skip = FALSE;
	double o_x, o_y, b_x, b_y, c, az;
	double GMT_units[3] = {0.01, 0.0254, 1.0};      /* No of meters in a cm, inch, m */
	char type, args_cp[BUFSIZ], txt_a[GMT_LONG_TEXT], txt_b[GMT_LONG_TEXT], txt_c[GMT_LONG_TEXT], txt_d[GMT_LONG_TEXT], txt_e[GMT_LONG_TEXT];

	l_pos[0] = l_pos[1] = p_pos[0] = p_pos[1] = t_pos[0] = t_pos[1] = d_pos[0] = d_pos[1] = 0;
	type = args[0];
	i = strlen (args) - 1;	/* Position of last character in this string */
	switch (args[i]) {	/* Check for what kind of width is given (only used if upper case is given below */
		case 'h':	/* Want map HEIGHT instead */
			width_given = 2;
			break;
		case '+':	/* Want this to be the MAX dimension of map */
			width_given = 3;
			break;
		case '-':	/* Want this to be the MIN dimension of map */
			width_given = 4;
			break;
		default:	/* Default is map WIDTH */
			width_given = 1;
			break;
	}

	GMT_io.in_col_type[0] = GMT_IS_LON;	GMT_io.in_col_type[1] = GMT_IS_LAT;	/* This may be overridden in -Jx, -Jp */
	GMT_io.out_col_type[0] = GMT_io.out_col_type[1] = GMT_IS_FLOAT;		/* This may be overridden by mapproject -I */
	project_info.degree[0] = project_info.degree[1] = TRUE;			/* May be overridden if not geographic projection */
	if (strchr ("AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz", (int)type) == NULL) return (TRUE);	/* NO valid projection specified */
	args++;

	for (j = 0; args[j]; j++) if (args[j] == '/') n_slashes++;

	if (!(type == 'z' || type == 'Z')) {
		/* Check to see if scale is specified in 1:xxxx */
		for (j = n = strlen (args), k = -1; j > 0 && k < 0 && args[j] != '/'; j--) if (args[j] == ':') k = j + 1;
		project_info.units_pr_degree = (k == -1) ? TRUE : FALSE;
	}

	project_info.unit = GMT_units[GMT_INCH];	/* No of meters in an inch */

	 switch (type) {
	 	case 'X':
	 		project_info.compute_scale[0] = project_info.compute_scale[1] = TRUE;
	 		if (args[0] == 'v') {
	 			project_info.pars[0] = gmtdefs.y_axis_length;
	 			project_info.pars[1] = gmtdefs.x_axis_length;
	 			skip = TRUE;
	 		}
	 		else if (args[0] == 'h') {
	 			project_info.pars[0] = gmtdefs.x_axis_length;
	 			project_info.pars[1] = gmtdefs.y_axis_length;
	 			skip = TRUE;
	 		}

	 	case 'x':		/* Linear x/y scaling */
			/* Default is not involving geographical coordinates */
			GMT_io.in_col_type[0] = GMT_io.in_col_type[1] = GMT_IS_FLOAT;

			error = (n_slashes > 1);
			if (!strncmp (args, "1:", 2)) k = 1;	/* Special check for linear proj with 1:xxx scale */

	 		/* Find occurrences of /, l, p, t, or d */
	 		for (j = 0, slash = 0; args[j] && slash == 0; j++) if (args[j] == '/') slash = j;
	 		for (j = id = 0; args[j]; j++) {
	 			if (args[j] == '/') id = 1;
	 			if (args[j] == 'L' || args[j] == 'l') l_pos[id] = j;
	 			if (args[j] == 'P' || args[j] == 'p') p_pos[id] = j;
	 			if (args[j] == 'T' || args[j] == 't') t_pos[id] = j;
	 			if (args[j] == 'D' || args[j] == 'd') d_pos[id] = j;
	 		}

			if (n_slashes && k >= 0) error = TRUE;	/* Cannot have 1:xxx separately for x/y */

			/* Distinguish between p for points and p<power> for scaling */

			n = strlen (args);
			for (j = 0; j < 2; j++) {
				if (!p_pos[j]) continue;
				i = p_pos[j] + 1;
				if (i == n || (args[i] == '/' || args[i] == 'd'))	/* This p is for points since no power is following */
					p_pos[j] = 0;
				else if (args[i] == 'p')	/* The 2nd p is the p for power */
					p_pos[j]++;
			}

	 		/* Get x-arguments */

			strcpy (args_cp, args);	/* Since GMT_convert_units modifies the string */
			if (slash) args_cp[slash] = 0;	/* Chop off y part */
			if ((i = MAX (l_pos[0], p_pos[0])) > 0)
				args_cp[i] = 0;	/* Chop off log or power part */
			else if (t_pos[0] > 0)
				args_cp[t_pos[0]] = 0;	/* Chop off time part */
			else if (d_pos[0] > 0)	/* Chop of trailing 'd' */
				args_cp[d_pos[0]] = 0;
	 		if (!skip) {
	 			if (k >= 0)	/* Scale entered as 1:mmmmm */
					project_info.pars[0] = 1.0 / GMT_convert_units (&args_cp[2], GMT_INCH);
				else
					project_info.pars[0] = GMT_convert_units (args_cp, GMT_INCH);	/* x-scale */
	 		}
	 		if (l_pos[0] > 0)
	 			project_info.xyz_projection[0] = LOG10;
	 		else if (p_pos[0] > 0) {
	 			project_info.xyz_projection[0] = POW;
	 			project_info.pars[2] = atof (&args[p_pos[0]+1]);	/* pow to raise x */
	 		}
	 		else if (t_pos[0] > 0) {	/* Add option to append time_systems or epoch/unit later */
	 			project_info.xyz_projection[0] = TIME;
				GMT_io.in_col_type[0] = (args[t_pos[0]] == 'T') ?  GMT_IS_ABSTIME : GMT_IS_RELTIME;
	 		}

			if (d_pos[0] > 0)
				GMT_io.in_col_type[0] = GMT_IS_LON;
			else
				project_info.degree[0] = FALSE;


	 		if (slash) {	/* Separate y-scaling desired */
				strcpy (args_cp, &args[slash+1]);	/* Since GMT_convert_units modifies the string */
				if ((i = MAX (l_pos[1], p_pos[1])) > 0)
					args_cp[i-slash-1] = 0;	/* Chop off log or power part */
				else if (t_pos[1] > 0)
					args_cp[t_pos[1]-slash-1] = 0;	/* Chop off log or power part */
				else if (d_pos[1] > 0)
					args_cp[d_pos[1]-slash-1] = 0;	/* Chop of trailing 'd' part */
	 			if (!skip) project_info.pars[1] = GMT_convert_units (args_cp, GMT_INCH);	/* y-scale */

	 			if (l_pos[1] > 0)
	 				project_info.xyz_projection[1] = LOG10;
	 			else if (p_pos[1] > 0) {
	 				project_info.xyz_projection[1] = POW;
	 				project_info.pars[3] = atof (&args[p_pos[1]+1]);	/* pow to raise y */
	 			}
	 			else if (t_pos[1] > 0) {	/* Add option to append time_systems or epoch/unit later */
	 				project_info.xyz_projection[1] = TIME;
					GMT_io.in_col_type[1] = (args[t_pos[0]] == 'T') ?  GMT_IS_ABSTIME : GMT_IS_RELTIME;
	 			}
				if (d_pos[1] > 0)
					GMT_io.in_col_type[1] = GMT_IS_LAT;
				else
					project_info.degree[1] = FALSE;
	 		}
	 		else {	/* Just copy x parameters */
	 			project_info.xyz_projection[1] = project_info.xyz_projection[0];
	 			if (!skip) project_info.pars[1] = project_info.pars[0];
	 			project_info.pars[3] = project_info.pars[2];
				if (project_info.degree[0])
					GMT_io.in_col_type[1] = GMT_IS_LAT;
				else {
					GMT_io.in_col_type[1] = GMT_io.in_col_type[0];
					project_info.degree[1] = FALSE;
				}
	 		}
	 		project = LINEAR;
	 		if (project_info.pars[0] == 0.0 || project_info.pars[1] == 0.0) error = TRUE;
	 		break;
	 	case 'Z':
			project_info.compute_scale[2] = TRUE;
	 	case 'z':

			error = (n_slashes > 0);
			GMT_io.in_col_type[2] = GMT_IS_FLOAT;

	 		/* Find occurrences of l, p, or t */
	 		for (j = 0; args[j]; j++) if (args[j] == 'L' || args[j] == 'l') l_pos[0] = j;
	 		for (j = 0; args[j]; j++) if (args[j] == 'P' || args[j] == 'p') p_pos[0] = j;
	 		for (j = 0; args[j]; j++) if (args[j] == 'T' || args[j] == 't') t_pos[0] = j;

			/* Distinguish between p for points and p<power> for scaling */

			n = strlen (args);
			if (p_pos[0]) {
				i = p_pos[0] + 1;
				if (i == n || (args[i] == 'd'))	/* This p is for points since no power is following */
					p_pos[0] = 0;
				else if (args[i] == 'p')	/* The 2nd p is the p for power */
					p_pos[0]++;
			}
	 		/* Get arguments */

			strcpy (args_cp, args);	/* Since GMT_convert_units modifies the string */
			if ((i = MAX (l_pos[0], p_pos[0])) > 0)
				args_cp[i] = 0;
			else if (t_pos[0] > 0)
				args_cp[t_pos[0]] = 0;
	 		project_info.z_pars[0] = GMT_convert_units (args_cp, GMT_INCH);	/* z-scale */

	 		if (l_pos[0] > 0)
	 			project_info.xyz_projection[2] = LOG10;
	 		else if (p_pos[0] > 0) {
	 			project_info.xyz_projection[2] = POW;
	 			project_info.z_pars[1] = atof (&args[p_pos[0]+1]);	/* pow to raise z */
	 		}
	 		else if (t_pos[0] > 0) {
	 			project_info.xyz_projection[2] = TIME;
				GMT_io.in_col_type[2] = (args[t_pos[0]] == 'T') ? GMT_IS_ABSTIME : GMT_IS_RELTIME;
	 		}
	 		if (project_info.z_pars[0] == 0.0) error = TRUE;
	 		break;
	 	case 'P':		/* Polar (theta,r) */
	 		project_info.gave_map_width = width_given;
	 	case 'p':
			GMT_io.in_col_type[0] = GMT_IS_LON;	GMT_io.in_col_type[1] = GMT_IS_FLOAT;
			if (args[0] == 'a' || args[0] == 'A') {
				project_info.got_azimuths = TRUE;	/* using azimuths instead of directions */
				i = 1;
			}
			else {
				project_info.got_azimuths = FALSE;
				i = 0;
			}
			j = strlen (args) - 1;
			if (args[j] == 'r') {	/* Gave optional r for reverse (elevations, presumably) */
				project_info.got_elevations = TRUE;
				args[j] = '\0';	/* Temporarily chop off the r */
			}
			else if (args[j] == 'z') {	/* Gave optional z for annotating depths rather than radius */
				project_info.z_down = TRUE;
				args[j] = '\0';	/* Temporarily chop off the z */
			}
			else
				project_info.got_elevations = project_info.z_down = FALSE;
	 		if (n_slashes == 1) {	/* Gave optional zero-base angle [0] */
	 		 	n = sscanf (args, "%[^/]/%lf", txt_a, &project_info.pars[1]);
				if (n == 2) project_info.pars[0] = GMT_convert_units (&txt_a[i], GMT_INCH);
				error = (project_info.pars[0] <= 0.0 || n != 2);
			}
			else if (n_slashes == 0) {
	 			project_info.pars[0] = GMT_convert_units (&args[i], GMT_INCH);
	 			n = (args) ? 1 : 0;
				error = (project_info.pars[0] <= 0.0 || n != 1);
	 		}
	 		else
	 			error = TRUE;
			if (project_info.got_elevations) args[j] = 'r';	/* Put the r back in the argument */
			if (project_info.z_down) args[j] = 'z';	/* Put the z back in the argument */
			if (project_info.got_azimuths) project_info.pars[1] = -project_info.pars[1];	/* Because azimuths go clockwise */
			project_info.degree[0] = project_info.degree[1] = FALSE;
	 		project = POLAR;
	 		break;

	 	/* Map projections */

	 	case 'A':	/* Lambert Azimuthal Equal-Area */
	 		project_info.gave_map_width = width_given;
	 	case 'a':
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			n = sscanf (args, "%[^/]/%[^/]/1:%lf", txt_a, txt_b, &project_info.pars[2]);
	 			if (project_info.pars[2] != 0.0) project_info.pars[2] = 1.0 / (project_info.pars[2] * project_info.unit);
				error = (!(n_slashes == 2 && n == 3));
	 		}
	 		else if (project_info.gave_map_width) {
	 			n = sscanf (args, "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
				if (n == 3) project_info.pars[2]= GMT_convert_units (txt_c, GMT_INCH);
				error = (!(n_slashes == 2 && n == 3));
	 		}
	 		else {	/* Scale entered as radius/lat */
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d);
				if (n == 4) {
					error += GMT_verify_expectations (GMT_IS_FLOAT, GMT_scanf (txt_c, GMT_IS_FLOAT, &project_info.pars[2]), txt_c);
					error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_d, GMT_IS_LAT, &project_info.pars[3]), txt_d);
				}
				error += (!(n_slashes == 3 && n == 4));
	 		}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += (project_info.pars[2] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = LAMB_AZ_EQ;
	 		break;
	 	case 'B':		/* Albers Equal-area Conic */
	 		project_info.gave_map_width = width_given;
	 	case 'b':
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%[^/]/1:%lf", txt_a, txt_b, txt_c, txt_d, &project_info.pars[4]);
	 			if (project_info.pars[4] != 0.0) project_info.pars[4] = 1.0 / (project_info.pars[4] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d, txt_e);
				project_info.pars[4]= GMT_convert_units (txt_e, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_c, GMT_IS_LAT, &project_info.pars[2]), txt_c);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_d, GMT_IS_LAT, &project_info.pars[3]), txt_d);
			error += !(n_slashes == 4 && n == 5);
			error += (project_info.pars[4] <= 0.0 || project_info.pars[2] == project_info.pars[3]);
			error += (k >= 0 && project_info.gave_map_width);
	 		project = ALBERS;
	 		break;

	 	case 'C':	/* Cassini */
	 		project_info.gave_map_width = width_given;
	 	case 'c':
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			n = sscanf (args, "%[^/]/%[^/]/1:%lf", txt_a, txt_b, &project_info.pars[2]);
	 			if (project_info.pars[2] != 0.0) project_info.pars[2] = 1.0 / (project_info.pars[2] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
				project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += !(n_slashes == 2 && n == 3);
			error += (project_info.pars[2] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = CASSINI;
	 		break;

	 	case 'D':		/* Equidistant Conic */
	 		project_info.gave_map_width = width_given;
	 	case 'd':
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%[^/]/1:%lf", txt_a, txt_b, txt_c, txt_d, &project_info.pars[4]);
	 			if (project_info.pars[4] != 0.0) project_info.pars[4] = 1.0 / (project_info.pars[4] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d, txt_e);
				project_info.pars[4]= GMT_convert_units (txt_e, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_c, GMT_IS_LAT, &project_info.pars[2]), txt_c);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_d, GMT_IS_LAT, &project_info.pars[3]), txt_d);
			error += !(n_slashes == 4 && n == 5);
			error += (project_info.pars[4] <= 0.0 || project_info.pars[2] == project_info.pars[3]);
			error += (k >= 0 && project_info.gave_map_width);
	 		project = ECONIC;
	 		break;

	 	case 'E':		/* Azimuthal equal-distant */
	 		project_info.gave_map_width = width_given;
	 	case 'e':
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			n = sscanf (args, "%[^/]/%[^/]/1:%lf", txt_a, txt_b, &project_info.pars[2]);
	 			if (project_info.pars[2] != 0.0) project_info.pars[2] = 1.0 / (project_info.pars[2] * project_info.unit);
				error = (!(n_slashes == 2 && n == 3));
	 		}
	 		else if (project_info.gave_map_width) {
		 		n = sscanf (args, "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
				project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
				error += (!(n_slashes == 2 && n == 3));
	 		}
	 		else {	/* Scale entered as radius/lat */
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d);
				if (n == 4) {
					project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
					error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_d, GMT_IS_LAT, &project_info.pars[3]), txt_d);
				}
				error += (!(n_slashes == 3 && n == 4));
	 		}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += (project_info.pars[2] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = AZ_EQDIST;
	 		break;

	 	case 'F':		/* Gnomonic */
	 		project_info.gave_map_width = width_given;
	 	case 'f':		/* Gnomonic */
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/1:%lf", txt_a, txt_b, txt_c, &project_info.pars[3]);
	 			if (project_info.pars[3] != 0.0) project_info.pars[3] = 1.0 / (project_info.pars[3] * project_info.unit);
				error = (!(n_slashes == 3 && n == 4));
	 		}
	 		else if (project_info.gave_map_width) {
		 		n = sscanf (args, "%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d);
				project_info.pars[3] = GMT_convert_units (txt_d, GMT_INCH);
				error = (!(n_slashes == 3 && n == 4));
			}
	 		else {	/* Scale entered as radius/lat */
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d, txt_e);
				if (n == 5) {
					project_info.pars[3] = GMT_convert_units (txt_d, GMT_INCH);
					error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_e, GMT_IS_LAT, &project_info.pars[4]), txt_e);
				}
				error += (!(n_slashes == 4 && n == 5));
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_c, GMT_IS_LAT, &project_info.pars[2]), txt_c);
			error += (project_info.pars[3] <= 0.0 || (k >= 0 && project_info.gave_map_width) || (project_info.pars[2] >= 90.0));
	 		project = GNOMONIC;
	 		break;

	 	case 'G':		/* Orthographic */
	 		project_info.gave_map_width = width_given;
	 	case 'g':		/* Orthographic */
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			n = sscanf (args, "%[^/]/%[^/]/1:%lf", txt_a, txt_b, &project_info.pars[2]);
	 			if (project_info.pars[2] != 0.0) project_info.pars[2] = 1.0 / (project_info.pars[2] * project_info.unit);
				error = (!(n_slashes == 2 && n == 3));
	 		}
	 		else if (project_info.gave_map_width) {
		 		n = sscanf (args, "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
				project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
				error = (!(n_slashes == 2 && n == 3));
			}
	 		else {	/* Scale entered as radius/lat */
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d);
				if (n == 4) {
					project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
					error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_d, GMT_IS_LAT, &project_info.pars[3]), txt_d);
				}
				error += (!(n_slashes == 3 && n == 4));
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += (project_info.pars[2] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = ORTHO;
	 		break;

	 	case 'H':	/* Hammer-Aitoff Equal-Area */
	 		project_info.gave_map_width = width_given;
	 	case 'h':
	 		if (k >= 0) {
	 			n = sscanf (args, "%[^/]/1:%lf", txt_a, &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%s", txt_a, txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = HAMMER;
	 		break;

	 	case 'I':	/* Sinusoidal Equal-Area */
	 		project_info.gave_map_width = width_given;
	 	case 'i':
	 		if (k >= 0) {
	 			n = sscanf (args, "%[^/]/1:%lf", txt_a, &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%s", txt_a, txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = SINUSOIDAL;
	 		break;

	 	case 'J':	/* Miller cylindrical */
	 		project_info.gave_map_width = width_given;
	 	case 'j':
	 		if (k >= 0) {
	 			n = sscanf (args, "%[^/]/1:%lf", txt_a, &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%s", txt_a, txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = MILLER;
	 		break;

	 	case 'K':	/* Eckert IV or VI projection */
	 		project_info.gave_map_width = width_given;
	 	case 'k':
			if (args[0] == 'f' || args[0] == 'F') {
				project = ECKERT4;
				j = 1;
			}
			else if (args[0] == 's' || args[0] == 'S') {
				project = ECKERT6;
				j = 1;
			}
			else {	/* Default is Eckert VI */
				project = ECKERT6;
				j = 0;
			}

	 		if (k >= 0) {
	 			n = sscanf (&args[j], "%[^/]/1:%lf", txt_a, &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (&args[j], "%[^/]/%s", txt_a, txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		break;

	 	case 'L':		/* Lambert Conformal Conic */
	 		project_info.gave_map_width = width_given;
	 	case 'l':
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%[^/]/1:%lf", txt_a, txt_b, txt_c, txt_d, &project_info.pars[4]);
	 			if (project_info.pars[4] != 0.0) project_info.pars[4] = 1.0 / (project_info.pars[4] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d, txt_e);
				project_info.pars[4] = GMT_convert_units (txt_e, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_c, GMT_IS_LAT, &project_info.pars[2]), txt_c);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_d, GMT_IS_LAT, &project_info.pars[3]), txt_d);
			error += !(n_slashes == 4 && n == 5);
			error += (project_info.pars[4] <= 0.0 || project_info.pars[2] == project_info.pars[3]);
			error += (k >= 0 && project_info.gave_map_width);
	 		project = LAMBERT;
	 		break;
	 	case 'M':		/* Mercator */
	 		project_info.gave_map_width = width_given;
	 	case 'm':
			if (n_slashes == 2) {	/* -JM|m<lon0/lat0/width|scale>, store w/s in [2] */
				project_info.m_got_parallel = TRUE;
	 			if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 				n = sscanf (args, "%[^/]/%[^/]/1:%lf", txt_a, txt_b, &project_info.pars[2]);
	 				if (project_info.pars[2] != 0.0) project_info.pars[2] = 1.0 / (project_info.pars[2] * project_info.unit);
	 			}
	 			else {
	 				n = sscanf (args, "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
					project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
				}
				error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
				error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
				if (n != 3 || project_info.pars[2] <= 0.0 || fabs (project_info.pars[1]) >= 90.0) error++;
			}
			else if (n_slashes == 0) {
				project_info.m_got_parallel = FALSE;
	 			if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 				n = sscanf (args, "1:%lf", &project_info.pars[0]);
	 				if (project_info.pars[0] != 0.0) project_info.pars[0] = 1.0 / (project_info.pars[0] * project_info.unit);
	 			}
	 			else {
	 				n = sscanf (args, "%s", txt_a);
					project_info.pars[0] = GMT_convert_units (txt_a, GMT_INCH);
				}
				if (n != 1 || project_info.pars[0] <= 0.0) error = TRUE;
			}
			else
				error = TRUE;
	 		project = MERCATOR;
	 		break;

	 	case 'N':	/* Robinson Projection */
	 		project_info.gave_map_width = width_given;
	 	case 'n':
	 		if (k >= 0) {
	 			n = sscanf (args, "%[^/]/1:%lf", txt_a, &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%s", txt_a, txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = ROBINSON;
	 		break;

	 	case 'O':		/* Oblique Mercator */
	 		project_info.gave_map_width = width_given;
	 	case 'o':
	 		if (args[0] == 'a') {	/* Origin and azimuth specified */
	 			if (k >= 0) {
					n = sscanf (&args[1], "%[^/]/%[^/]/%lf/1:%lf", txt_a, txt_b, &az, &project_info.pars[4]);
	 				if (project_info.pars[4] != 0.0) project_info.pars[4] = 1.0 / (project_info.pars[4] * project_info.unit);
	 			}
	 			else {
					n = sscanf (&args[1], "%[^/]/%[^/]/%lf/%s", txt_a, txt_b, &az, txt_c);
					project_info.pars[4] = GMT_convert_units (txt_c, GMT_INCH);
				}

				error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &o_x), txt_a);
				error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &o_y), txt_b);
	 			c = 10.0;	/* compute point 10 degrees from origin along azimuth */
	 			b_x = o_x + R2D * atan (sind (c) * sind (az) / (cosd (o_y) * cosd (c) - sind (o_y) * sind (c) * cosd (az)));
	 			b_y = R2D * d_asin (sind (o_y) * cosd (c) + cosd (o_y) * sind (c) * cosd (az));
	 			project_info.pars[6] = 0.0;
				error += !(n_slashes == 3 && n == 4);
	 		}
	 		else if (args[0] == 'b') {	/* Origin and second point specified */
	 			if (k >= 0) {
	 				n = sscanf (&args[1], "%[^/]/%[^/]/%[^/]/%[^/]/1:%lf", txt_a, txt_b, txt_c, txt_d, &project_info.pars[4]);
	 				if (project_info.pars[4] != 0.0) project_info.pars[4] = 1.0 / (project_info.pars[4] * project_info.unit);
	 			}
	 			else {
	 				n = sscanf (&args[1], "%[^/]/%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d, txt_e);
					project_info.pars[4] = GMT_convert_units (txt_e, GMT_INCH);
				}
				error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &o_x), txt_a);
				error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &o_y), txt_b);
				error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_c, GMT_IS_LON, &b_x), txt_c);
				error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_d, GMT_IS_LAT, &b_y), txt_d);
	 			project_info.pars[6] = 0.0;
				error += !(n_slashes == 4 && n == 5);
	 		}
	 		else if (args[0] == 'c') {	/* Origin and Pole specified */
	 			if (k >= 0) {
	 				n = sscanf (&args[1], "%[^/]/%[^/]/%[^/]/%[^/]/1:%lf", txt_a, txt_b, txt_c, txt_d, &project_info.pars[4]);
	 				if (project_info.pars[4] != 0.0) project_info.pars[4] = 1.0 / (project_info.pars[4] * project_info.unit);
	 			}
	 			else {
	 				n = sscanf (&args[1], "%[^/]/%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d, txt_e);
					project_info.pars[4] = GMT_convert_units (txt_e, GMT_INCH);
				}
				error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &o_x), txt_a);
				error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &o_y), txt_b);
				error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_c, GMT_IS_LON, &b_x), txt_c);
				error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_d, GMT_IS_LAT, &b_y), txt_d);
	 			if (b_y < 0.0) {	/* Flip from S hemisphere to N */
	 				b_y = -b_y;
	 				b_x += 180.0;
	 				if (b_x >= 360.0) b_x -= 360.0;
	 			}
	 			project_info.pars[6] = 1.0;
				error += !(n_slashes == 4 && n == 5);
	 		}
	 		else
	 			project = -1;
			error += (project_info.pars[4] <= 0.0);
			error += (k >= 0 && project_info.gave_map_width);
	 		project_info.pars[0] = o_x;	project_info.pars[1] = o_y;
	 		project_info.pars[2] = b_x;	project_info.pars[3] = b_y;

	 		/* see if wesn is in oblique degrees or just diagonal corners */

	 		project = OBLIQUE_MERC;
	 		break;

	 	case 'Q':	/* Equidistant Cylindrical (Plate Carree) */
	 		project_info.gave_map_width = width_given;
	 	case 'q':
	 		if (k >= 0) {
	 			n = sscanf (args, "%[^/]/1:%lf", txt_a, &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%s", txt_a, txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = CYL_EQDIST;
	 		break;

	 	case 'R':	/* Winkel Tripel Modified azimuthal */
	 		project_info.gave_map_width = width_given;
	 	case 'r':
	 		if (k >= 0) {
	 			n = sscanf (args, "%[^/]/1:%lf", txt_a, &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%s", txt_a, txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = WINKEL;
	 		break;

	 	case 'S':		/* Stereographic */
	 		project_info.gave_map_width = width_given;
	 	case 's':
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			if (n_slashes == 3) {	/* with true scale at specified latitude */
	 				n = sscanf (args, "%[^/]/%[^/]/%[^/]/1:%lf", txt_a, txt_b, txt_c, &project_info.pars[2]);
					error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_c, GMT_IS_LAT, &project_info.pars[3]), txt_c);
					project_info.pars[4] = 1.0;	/* flag for true scale case */
					error += (n != 4);
				}
	 			else if (n_slashes == 2) {
	 				n = sscanf (args, "%[^/]/%[^/]/1:%lf", txt_a, txt_b, &project_info.pars[2]);
					error = (n != 3);
	 			}
	 			else
	 				error = TRUE;
	 			if (project_info.pars[2] != 0.0) project_info.pars[2] = 1.0 / (project_info.pars[2] * project_info.unit);
	 		}
	 		else if (project_info.gave_map_width) {
		 		n = sscanf (args, "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
				project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
				error = (!(n_slashes == 2 && n == 3));
	 		}
	 		else {	/* Scale entered as radius/lat */
	 			n = sscanf (args, "%[^/]/%[^/]/%[^/]/%s", txt_a, txt_b, txt_c, txt_d);
				if (n == 4) {
					project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
					error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_d, GMT_IS_LAT, &project_info.pars[3]), txt_d);
				}
				error += (!(n_slashes == 3 && n == 4));
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += (project_info.pars[2] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = STEREO;
	 		break;

	 	case 'T':	/* Transverse Mercator */
	 		project_info.gave_map_width = width_given;
	 	case 't':
			if (n_slashes == 1) {	/* -JT<lon>/<width> */
	 			if (k >= 0) {
	 				n = sscanf (args, "%[^/]/1:%lf", txt_a, &project_info.pars[2]);
	 				if (project_info.pars[2] != 0.0) project_info.pars[2] = 1.0 / (project_info.pars[2] * project_info.unit);
	 			}
	 			else {
	 				n = sscanf (args, "%[^/]/%s", txt_a, txt_b);
					project_info.pars[2] = GMT_convert_units (txt_b, GMT_INCH);
				}
				error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
				project_info.pars[1] = 0.0;	/* Default latitude of origin */
				error += !(n_slashes == 1 && n == 2);
			}
			else {	/* -JT<lon>/<lat>/<width> */
	 			if (k >= 0) {
	 				n = sscanf (args, "%[^/]/%[^/]/1:%lf", txt_a, txt_b, &project_info.pars[2]);
	 				if (project_info.pars[2] != 0.0) project_info.pars[2] = 1.0 / (project_info.pars[2] * project_info.unit);
	 			}
	 			else {
	 				n = sscanf (args, "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
					project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
				}
				error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
				error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
				error += !(n_slashes == 2 && n == 3);
			}
			error += (project_info.pars[2] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = TM;
	 		break;

	 	case 'U':	/* Universal Transverse Mercator */
	 		project_info.gave_map_width = width_given;
	 	case 'u':
	 		if (k >= 0) {
	 			n = sscanf (args, "%lf/1:%lf", &project_info.pars[0], &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%lf/%s", &project_info.pars[0], txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			switch (args[0]) {
				case '-':	/* Enforce Southern hemisphere convention for y */
	 				project_info.utm_hemisphere = -1;
					break;
				case '+':	/* Enforce Norther hemisphere convention for y */
	 				project_info.utm_hemisphere = +1;
					break;
				default:	/* Decide in gmt_map_setup based on -R */
	 				project_info.utm_hemisphere = 0;
					break;
			}
	 		project_info.pars[0] = fabs (project_info.pars[0]);
			error = !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
			error += (project_info.pars[0] < 1 || project_info.pars[0] > 60);	/* Zones must be 1-60 */
	 		project = UTM;
	 		break;
	 	case 'V':	/* Van der Grinten */
	 		project_info.gave_map_width = width_given;
	 	case 'v':
	 		if (k >= 0) {
	 			n = sscanf (args, "%[^/]/1:%lf", txt_a, &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%s", txt_a, txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = GRINTEN;
	 		break;

	 	case 'W':	/* Mollweide Equal-Area */
	 		project_info.gave_map_width = width_given;
	 	case 'w':
	 		if (k >= 0) {
	 			n = sscanf (args, "%[^/]/1:%lf", txt_a, &project_info.pars[1]);
	 			if (project_info.pars[1] != 0.0) project_info.pars[1] = 1.0 / (project_info.pars[1] * project_info.unit);
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%s", txt_a, txt_b);
				project_info.pars[1] = GMT_convert_units (txt_b, GMT_INCH);
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += !(n_slashes == 1 && n == 2);
			error += (project_info.pars[1] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = MOLLWEIDE;
	 		break;

	 	case 'Y':		/* Cylindrical Equal Area */
	 		project_info.gave_map_width = width_given;
	 	case 'y':
	 		if (k >= 0) {	/* Scale entered as 1:mmmmm */
	 			n = sscanf (args, "%[^/]/%[^/]/1:%lf", txt_a, txt_b, &project_info.pars[2]);
	 			if (project_info.pars[2] != 0.0) project_info.pars[2] = 1.0 / (project_info.pars[2] * project_info.unit);
				error = (!(n_slashes == 2 && n == 3));
	 		}
	 		else {
	 			n = sscanf (args, "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
				project_info.pars[2] = GMT_convert_units (txt_c, GMT_INCH);
				error = (!(n_slashes == 2 && n == 3));
			}
			error += GMT_verify_expectations (GMT_IS_LON, GMT_scanf (txt_a, GMT_IS_LON, &project_info.pars[0]), txt_a);
			error += GMT_verify_expectations (GMT_IS_LAT, GMT_scanf (txt_b, GMT_IS_LAT, &project_info.pars[1]), txt_b);
			error += (fabs(project_info.pars[1]) >= 90.0);
			error += (project_info.pars[2] <= 0.0 || (k >= 0 && project_info.gave_map_width));
	 		project = CYL_EQ;
	 		break;

	 	default:
	 		error = TRUE;
	 		project = -1;
	 		break;
	 }

	 if (!project_info.units_pr_degree && project_info.gave_map_width) {
	 	fprintf (stderr, "%s: GMT SYNTAX ERROR -J%c option: Cannot specify map width with 1:xxxx format\n", GMT_program, type);
	 	error++;
	 }

	 if (!(type == 'z' || type == 'Z')) project_info.projection = project;
	 return (error);
}

void GMT_free_plot_array (void) {
	if (GMT_n_alloc) {
		GMT_free ((void *)GMT_x_plot);
		GMT_free ((void *)GMT_y_plot);
		GMT_free ((void *)GMT_pen);
	}
}

void GMT_prepare_3D (void) {	/* Initialize 3-D parameters */
	project_info.z_pars[0] = project_info.z_pars[1] = 0.0;
	project_info.xyz_pos[2] = TRUE;
	project_info.zmin = project_info.zmax = 0.0;
	GMT_z_forward = (PFI) NULL;
	GMT_z_inverse = (PFI) NULL;
	z_project.view_azimuth = 180.0;
	z_project.view_elevation = 90.0;
	project_info.z_bottom = project_info.z_top = 0.0;
}

int GMT_decode_symbol_option (char *text, struct GMT_SYMBOL *p, int mode, BOOLEAN cmd)
{
	/* mode = 0 for 2-D (psxy) and = 1 for 3-D (psxyz) */
	int decode_error = 0, bset = 0, j, n, k, len, slash = 0, one, colon;
	BOOLEAN check, old_style;
	char symbol_type, txt_a[GMT_LONG_TEXT], txt_b[GMT_LONG_TEXT], txt_c[GMT_LONG_TEXT], text_cp[GMT_LONG_TEXT], *c;
	static char *allowed_symbols[2] = {"-aAbCcDdeEfGgHhIiNnpqrSsTtVvwWxy", "-aAbCcDdeEfGgHhIiNnoOpqrSsTtuUVvwWxy"};
	static char *bar_symbols[2] = {"b", "-boOuU"};

	p->n_required = p->convert_angles = 0;
	p->user_unit = p->shrink = p->read_vector = FALSE;

	if (!text[0]) {	/* No symbol or size given */
		p->size_x = p->size_y = 0.0;
		symbol_type = '+';
	}
	else if (isdigit ((int)text[0]) || text[0] == '.') {	/* Size, but no symbol given */
		n = sscanf (text, "%[^/]/%s", txt_a, txt_b);
		p->size_x = p->given_size_x = GMT_convert_units (txt_a, GMT_INCH);
		if (n == 2)
			p->size_y = p->given_size_y = GMT_convert_units (txt_b, GMT_INCH);
		else if (n == 1)
			p->size_y = p->given_size_y = p->size_x;
		else
			decode_error = TRUE;
		symbol_type = '+';
	}
	else if (text[0] == 'l') {	/* Letter symbol is special case */
		strcpy (text_cp, text);
		if ((c = strchr (text_cp, '%'))) {	/* Gave font name or number, too */
			*c = ' ';	/* Make the % a space */
			c++;		/* Go to next character */
			if (c[0] >= '0' && c[0] <= '9')	/* Gave a font # */
				p->font_no = atoi (c);
			else
				p->font_no = GMT_font_lookup (c, GMT_font, N_FONTS);
			if (p->font_no >= N_FONTS) {
				fprintf (stderr, "%s: -Sl contains bad font (set to %s (0))\n", GMT_program, GMT_font[gmtdefs.annot_font[0]].name);
				p->font_no = gmtdefs.annot_font[0];
			}
		}
		if (text[1] == '/') {	/* No size given */
			symbol_type = 'l';
			if (p->size_x == 0.0) p->size_x = p->given_size_x;
			if (p->size_y == 0.0) p->size_y = p->given_size_y;
		}
		else {
			n = sscanf (text_cp, "%c%[^/]/%s", &symbol_type, txt_a, p->string);
			p->size_x = p->given_size_x = GMT_convert_units (txt_a, GMT_INCH);
			decode_error = (n != 3);
		}
	}
	else if (text[0] == 'k') {	/* Custom symbol spec */
		for (j = strlen (text); j > 0 && text[j] != '/'; j--);;
		if (j == 0) {	/* No slash, i.e., no symbol size given */
			if (p->size_x == 0.0) p->size_x = p->given_size_x;
			n = sscanf (text, "%c%s", &symbol_type, text_cp);
		}
		else {
			text[j] = ' ';
			n = sscanf (text, "%c%s %s", &symbol_type, text_cp, txt_a);
			text[j] = '/';
			p->given_size_x = p->size_x = GMT_convert_units (txt_a, GMT_INCH);
		}
	}
	else if (strchr (allowed_symbols[mode], (int) text[0]) && strchr ("CcIiMmPp", (int) text[1])) {	/* Symbol, but no size given (size assumed given on command line), only unit information */
		n = sscanf (text, "%c", &symbol_type);
		if (p->size_x == 0.0) p->size_x = p->given_size_x;
		if (p->size_y == 0.0) p->size_y = p->given_size_y;
		if (text[1] && (p->u = GMT_get_unit (text[1])) < 0) decode_error = TRUE;
		p->equal_area = FALSE;
	}
	else if (strchr (allowed_symbols[mode], (int) text[0]) && (text[1] == '\n' || text[1] == '\0')) {	/* Symbol, but no size given (size assumed given on command line) */
		n = sscanf (text, "%c", &symbol_type);
		if (p->size_x == 0.0) p->size_x = p->given_size_x;
		if (p->size_y == 0.0) p->size_y = p->given_size_y;
		p->equal_area = FALSE;
	}
	else if (strchr (bar_symbols[mode], (int) text[0])) {	/* Bar, column, cube with size */

		/* Bar:		-Sb<size_x>[c|i|m|p|u][b<base>]				*/
		/* Column:	-So|O<size_x>[c|i|m|p][/<ysize>[c|i|m|p]][u][b<base>]	*/
		/* Cube:	-Su|U<size_x>[c|i|m|p|u]	*/

		for (j = 0; text[j]; j++) {
			if (text[j] == '/') slash = j;
			if (text[j] == 'b') bset = j;
		}
		strcpy (text_cp, text);
		if (bset) text_cp[bset] = 0;	/* Chop off the b<base> from copy */
		if ((bset && text_cp[bset-1] == 'u') || (j && text[j-1] == 'u')) p->user_unit = TRUE;
		if (slash) {	/* Separate x/y sizes */
			n = sscanf (text_cp, "%c%[^/]/%s", &symbol_type, txt_a, txt_b);
			decode_error = (n != 3);
			if (p->user_unit) {
				p->size_x = p->given_size_x = atof (txt_a);
				p->size_y = p->given_size_y = atof (txt_b);
			}
			else {
				p->size_x = p->given_size_x = GMT_convert_units (txt_a, GMT_INCH);
				p->size_y = p->given_size_y = GMT_convert_units (txt_b, GMT_INCH);
			}
		}
		else {	/* Only a single x = y size */
			n = sscanf (text_cp, "%c%s", &symbol_type, txt_a);
			if (n == 2) {
				if (p->user_unit) {
					p->size_x = p->given_size_x = atof (txt_a);
					p->size_y = p->given_size_y = p->size_x;
				}
				else {
					p->size_x = p->given_size_x = GMT_convert_units (txt_a, GMT_INCH);
					p->size_y = p->given_size_y = p->size_x;
				}
			}
			else {
				if (p->size_x == 0.0) p->size_x = p->given_size_x;
				if (p->size_y == 0.0) p->size_y = p->given_size_y;
			}
		}
	}
	else {
		n = sscanf (text, "%c%[^/]/%s", &symbol_type, txt_a, txt_b);
		p->size_x = p->given_size_x = GMT_convert_units (txt_a, GMT_INCH);
		if (n == 3)
			p->size_y = p->given_size_y = GMT_convert_units (txt_b, GMT_INCH);
		else if (n == 2)
			p->size_y = p->given_size_y = p->size_x;
		else
			decode_error = TRUE;
		p->equal_area = FALSE;
	}

	check = TRUE;
	switch (symbol_type) {
		case '+':
			p->symbol = GMT_SYMBOL_NOT_SET;
			break;
		case '-':
			p->symbol = GMT_SYMBOL_XDASH;
			p->size_x *= 0.5;		/* We will use +- to get full width */
			break;
		case 'A':
			p->equal_area = TRUE;	/* To equal area of circle with same size */
		case 'a':
			p->symbol = GMT_SYMBOL_STAR;
			break;
		case 'b':
			p->symbol = GMT_SYMBOL_BAR;
			p->size_x *= 0.5;		/* We will use +- to get full width */
			if (bset) p->base = atof (&text[bset+1]);
			break;
		case 'C':
		case 'c':
			p->symbol = GMT_SYMBOL_CIRCLE;
			break;
		case 'D':
			p->equal_area = TRUE;	/* To equal area of circle with same size */
		case 'd':
			p->symbol = GMT_SYMBOL_DIAMOND;
			break;
		case 'E':	/* Expect axis in km to be scaled based on -J */
			p->convert_angles = 1;
		case 'e':
			p->symbol = GMT_SYMBOL_ELLIPSE;
			p->n_required = 3;
			check = FALSE;
			break;

		case 'f':	/* Fronts:   -Sf<spacing>/<size>[dir][type][:<offset>]	*/
			p->symbol = GMT_SYMBOL_FRONT;
			p->f.f_off = 0.0;
			strcpy (text_cp, text);
			if ((c = strchr (text_cp, ':'))) {	/* Gave :<offset>, set it and strip it off */
				c++;	/* Skip over the colon */
				p->f.f_off = GMT_convert_units (c, GMT_INCH);
				c--;	/* Go back to colon */
				*c = 0;	/* Effectively chops off the offset modifier */
			}
			len = strlen (text_cp) - 1;
	
			old_style = FALSE;
			switch (text_cp[len]) {
				case 'f':	/* Fault front */
					p->f.f_symbol = GMT_FRONT_FAULT;
					len--;
					break;
				case 't':	/* Triangle front */
					p->f.f_symbol = GMT_FRONT_TRIANGLE;
					len--;
					break;
				case 's':	/* Strike-slip front */
					p->f.f_symbol = GMT_FRONT_SLIP;
					len--;
					break;
				case 'c':	/* [half-]circle front */
					p->f.f_symbol = GMT_FRONT_CIRCLE;
					len--;
					break;
				case 'b':	/* [half-]square front */
					p->f.f_symbol = GMT_FRONT_BOX;
					len--;
					break;
			
				/* Old style (backward compatibility) */
		
				case 'L':	/* Left triangle */
					p->f.f_symbol = GMT_FRONT_TRIANGLE;
				case 'l':	/* Left ticked fault */
					p->f.f_sense = GMT_FRONT_LEFT;
					old_style = TRUE;
					break;
				case 'R':	/* Right triangle */
					p->f.f_symbol = GMT_FRONT_TRIANGLE;
				case 'r':	/* Right ticked fault */
					p->f.f_sense = GMT_FRONT_RIGHT;
					old_style = TRUE;
					break;
				default:
					p->f.f_sense = GMT_FRONT_CENTERED;
					break;
			}

			if (!old_style) {
				switch (text_cp[len]) {	/* Get sense - default is centered */
					case 'l':
						p->f.f_sense = GMT_FRONT_LEFT;
						break;
					case 'r':
						p->f.f_sense = GMT_FRONT_RIGHT;
						break;
					default:
						len++;
						p->f.f_sense = GMT_FRONT_CENTERED;
						if (p->f.f_symbol == GMT_FRONT_SLIP) {
							fprintf (stderr, "%s: Error in Option -Sf: Must specify (GMTMANSECTION)eft-lateral or (r)ight-lateral slip\n", GMT_program);
							exit (EXIT_FAILURE);
						}
						break;
				}
			}

			text_cp[len] = 0;	/* Gets rid of the [dir][type] flags, if present */
	
			/* Pull out and get spacing and size */
	
			sscanf (&text_cp[1], "%[^/]/%s", txt_a, txt_b);
			p->f.f_gap = GMT_convert_units (txt_a, GMT_INCH);
			p->f.f_len = GMT_convert_units (txt_b, GMT_INCH);
			break;
		case 'G':
			p->equal_area = TRUE;	/* To equal area of circle with same size */
		case 'g':
			p->symbol = GMT_SYMBOL_OCTAGON;
			break;
		case 'H':
			p->equal_area = TRUE;	/* To equal area of circle with same size */
		case 'h':
			p->symbol = GMT_SYMBOL_HEXAGON;
			break;
		case 'I':
			p->equal_area = TRUE;	/* To equal area of circle with same size */
		case 'i':
			p->symbol = GMT_SYMBOL_ITRIANGLE;
			break;
		case 'l':
			p->symbol = GMT_SYMBOL_TEXT;
			/* Look for a slash that separates size and string: */
			for (j = 1, slash = 0; text_cp[j] && !slash; j++) if (text_cp[j] == '/') slash = j;
			/* Set j to the first char in the string: */
			j = slash + 1;
			/* Copy string characters */
			k = 0;
			while (text_cp[j] && text_cp[j] != ' ' && k < 63) p->string[k++] = text_cp[j++];
			if (!k) {
				fprintf (stderr, "%s: GMT SYNTAX ERROR -Sl option:  No string given\n", GMT_program);
				decode_error++;
			}
			p->string[k] = 0;
			break;
		case 'N':
			p->equal_area = TRUE;	/* To equal area of circle with same size */
		case 'n':
			p->symbol = GMT_SYMBOL_PENTAGON;
			break;
		case 'o':	/*3-D symbol */
			p->shade3D = TRUE;
		case 'O':	/* Same but disable shading */
			p->symbol = GMT_SYMBOL_COLUMN;
			if (bset) p->base = atof (&text[bset+1]);
			if (mode == 0) {
				decode_error = TRUE;
				fprintf (stderr, "%s: GMT SYNTAX ERROR -S option:  Symbol type %c is 3-D only\n", GMT_program, symbol_type);
			}
			break;
		case 'p':
			p->symbol = GMT_SYMBOL_POINT;
			if (p->size_x == 0.0) p->size_x = GMT_POINT_SIZE;
			check = FALSE;
			break;
		case 'q':	/* Quoted lines: -Sq[d|n|l|x]<info>[:<labelinfo>] */
			p->symbol = GMT_SYMBOL_QUOTED_LINE;
			for (j = 1, colon = 0; text[j]; j++) if (text[j] == ':') colon = j;
			if (colon) {	/* Gave :<labelinfo> */
				text[colon] = 0;
				decode_error += GMT_contlabel_info ('S', &text[1], &p->G);
				decode_error += GMT_contlabel_specs (&text[colon+1], &p->G);
			}
			else
				decode_error += GMT_contlabel_info ('S', &text[1], &p->G);
			break;
			check = FALSE;
		case 'S':
			p->equal_area = TRUE;	/* To equal area of circle with same size */
		case 's':
			p->symbol = GMT_SYMBOL_SQUARE;
			break;
		case 'T':
			p->equal_area = TRUE;	/* To equal area of circle with same size */
		case 't':
			p->symbol = GMT_SYMBOL_TRIANGLE;
			break;
		case 'u':	/*3-D symbol */
			p->shade3D = TRUE;
		case 'U':	/* Same but disable shading */
			p->symbol = GMT_SYMBOL_CUBE;
			if (mode == 0) {
				decode_error = TRUE;
				fprintf (stderr, "%s: GMT SYNTAX ERROR -S option:  Symbol type %c is 3-D only\n", GMT_program, symbol_type);
			}
			break;
		case 'V':
			p->convert_angles = 1;
		case 'v':
			p->symbol = GMT_SYMBOL_VECTOR;
			switch (text[1]) {	/* Check if s(egment), h(ead), b(alance center), or t(ail) have been specified */
				case 'S':	/* Input (x,y) refers to vector head (the tip), double heads */
					p->v_double_heads = TRUE;
				case 's':	/* Input (x,y) refers to vector head (the tip), single head  */
					p->v_just = 3;
					one = 2;
					break;
				case 'H':	/* Input (x,y) refers to vector head (the tip), double heads */
					p->v_double_heads = TRUE;
				case 'h':	/* Input (x,y) refers to vector head (the tip), single head */
					p->v_just = 2;
					one = 2;
					break;
				case 'B':	/* Input (x,y) refers to balance point of vector, double heads */
					p->v_double_heads = TRUE;
				case 'b':	/* Input (x,y) refers to balance point of vector, single head */
					p->v_just = 1;
					one = 2;
					break;
				case 'T':	/* Input (x,y) refers to tail of vector, double heads */
					p->v_double_heads = TRUE;
				case 't':	/* Input (x,y) refers to tail of vector [Default], single head */
					p->v_just = 0;
					one = 2;
					break;
				default:	/* No modifier given, default to tail, single head */
					p->v_just = 0;
					one = 1;
					break;
			}
			for (j = one; text[j] && text[j] != 'n'; j++);
			len = strlen(text) - 1;
			if (text[j] == 'n') {	/* Normalize option used */
				k = GMT_get_unit (text[len]);
				if (k >= 0) p->u = k;
				p->v_norm = atof (&text[j+1]);
				if (p->v_norm > 0.0) {
					p->v_shrink = 1.0 / p->v_norm;
					p->symbol = GMT_SYMBOL_VECTOR2;
				}
				text[j] = 0;	/* Chop off the shrink part */
			}
			if (text[one]) {
				/* It is possible that the user have appended a unit modifier after
				 * the <size> argument (which here are vector attributes).  We use that
				 * to set the unit, but only if the vector attributes themselves have
				 * units. (If not we would override MEASURE_UNIT without cause).
				 * So, -SV0.1i/0.2i/0.3ic will expect 4th column to have length in cm
				 * while SV0.1i/0.2i/0.3i expects data units in MEASURE_UNIT
				 */

				if (isalpha ((int)text[len]) && isalpha ((int)text[len-1])) {
					p->u = GMT_get_unit (text[len]);
					text[len] = 0;
				}
				sscanf (&text[one], "%[^/]/%[^/]/%s", txt_a, txt_b, txt_c);
				p->v_width  = GMT_convert_units (txt_a, GMT_INCH);
				p->h_length = GMT_convert_units (txt_b, GMT_INCH);
				p->h_width  = GMT_convert_units (txt_c, GMT_INCH);
			}
			if (p->symbol == GMT_SYMBOL_VECTOR2) text[j] = 'n';	/* Put back the n<shrink> part */
			p->read_vector = TRUE;
			p->n_required = 2;
			check = FALSE;
			break;
		case 'W':
			p->convert_angles = 1;
		case 'w':
			p->symbol = GMT_SYMBOL_PIE;
			p->n_required = 2;
			break;
		case 'r':
			p->symbol = GMT_SYMBOL_RECT;
			p->n_required = 2;
			check = FALSE;
			break;
		case 'x':
			p->symbol = GMT_SYMBOL_CROSS;
			break;
		case 'y':
			p->symbol = GMT_SYMBOL_YDASH;
			p->size_x *= 0.5;		/* We will use +- to get full width */
			break;
		case 'z':
			p->symbol = GMT_SYMBOL_ZDASH;
			p->size_x *= 0.5;		/* We will use +- to get full width */
			break;
		case 'k':
			p->symbol = GMT_SYMBOL_CUSTOM;
			p->custom = GMT_get_custom_symbol (text_cp);
			break;
		default:
			decode_error = TRUE;
			fprintf (stderr, "%s: GMT SYNTAX ERROR -S option:  Unrecognized symbol type %c\n", GMT_program, symbol_type);
			break;
	}
	if (p->given_size_x == 0.0 && check) {
		p->read_size = TRUE;
		p->n_required++;
	}
	else
		p->read_size = FALSE;
	if (!cmd && p->symbol == GMT_SYMBOL_COLUMN) {
		if (!bset) p->base = (project_info.xyz_projection[2] == LOG10) ? 1.0 : 0.0;
	}
	if (!cmd && p->symbol == GMT_SYMBOL_BAR) {
		if (!bset) p->base = (project_info.xyz_projection[1] == LOG10) ? 1.0 : 0.0;
	}

	return (decode_error);
}

void GMT_extract_label (char *line, char *label)
{
	int i = 0, j, j0;
	char *p;

	if ((p = strstr (line, " -L")) || (p = strstr (line, "	-L"))) 	/* Get label specified wih -L option */
		i = p + 3 - line;
	else {								/* Bypass whitespace and pick first word */
		while (line[i] && (line[i] == ' ' || line[i] == '\t')) i++;

	}
	if ((p = strchr (&line[i], '\"'))) {	/* Gave several words as label */
		for (j0 = i, j = i + 1; line[j] != '\"'; j++);
		if (line[j] == '\"')	/* Found the matching quote */
			strncpy (label, &line[j0], j-j0);
		else {			/* Missing the matching quote */
			sscanf (&line[i], "%s", label);
			fprintf (stderr, "%s: Warning: Label (%s) not terminated by matching quote\n", GMT_program, label);
		}
	}
	else
		sscanf (&line[i], "%s", label);
}

char *GMT_putpen (struct GMT_PEN *pen)
{
	/* GMT_putpen creates a GMT textstring equivalent of the specified pen */

	static char text[BUFSIZ];
	int i;

	if (pen->texture[0]) {

		if (pen->rgb[0] == 0 && pen->rgb[0] == pen->rgb[1] && pen->rgb[1] == pen->rgb[2]) /* Default black pen */
			sprintf (text, "%.5gp,,%s:%.5g", pen->width, pen->texture, pen->offset);
		else
			sprintf (text, "%.5gp,%d/%d/%d,%s:%.5g", pen->width, pen->rgb[0], pen->rgb[1], pen->rgb[2], pen->texture, pen->offset);
		for (i = 0; text[i]; i++) if (text[i] == ' ') text[i] = '_';
	}
	else {
		if (pen->rgb[0] == 0 && pen->rgb[0] == pen->rgb[1] && pen->rgb[1] == pen->rgb[2]) /* Default black pen */
			sprintf (text, "%.5gp", pen->width);
		else
			sprintf (text, "%.5gp,%d/%d/%d", pen->width, pen->rgb[0], pen->rgb[1], pen->rgb[2]);
	}

	return (text);
}

int GMT_check_region (double w, double e, double s, double n)
{	/* If region is given then we must have w < e and s < n */
	return ((w >= e || s >= n) && project_info.region);
}

int GMT_get_unit (char c)
{
	/* Converts cC, iI, mM, and pP into 0-3 */

	int i;
	switch ((int)c) {
		case 'C':	/* cm */
		case 'c':
			i = 0;
			break;
		case 'I':	/* inch */
		case 'i':
			i = 1;
			break;
		case 'M':	/* meter */
		case 'm':
			i = 2;
			break;
		case 'P':	/* point */
		case 'p':
			i = 3;
			break;
		default:	/* error */
			i = -1;
			break;
	}
	return (i);
}

void GMT_init_scales (int unit, double *fwd_scale, double *inv_scale, double *inch_to_unit, double *unit_to_inch, char *unit_name) {
	/* unit is 0-6 and stands for m, km, miles, nautical miles, inch, cm, or point */
	/* fwd_scale is used to convert user distance units to meter */
	/* inv_scale is used to convert meters to user distance units */
	/* inch_to_unit is used to convert internal inches to users units (c, i, m, p) */
	/* unit_to_inch is used to convert users units (c, i, m, p) to internal inches */
	/* unit_name is the name of the users measure unit (cm/inch/m/point) */

	double scales[7];

	/* These scales are used if 1 to 1 scaling is chosen */

	scales[0] = 1.0;		/* m in m */
	scales[1] = 1000.0;		/* m in km */
	scales[2] = 1609.334;		/* m in miles */
	scales[3] = 1852.0;		/* m in nautical miles */
	scales[4] = 0.0254;		/* m in inch */
	scales[5] = 0.01;		/* m in cm */
	scales[6] = 0.0254 / 72.0;	/* m in points */

	/* These scales are used when 1:1 is not set to ensure that the
	 * output (or input with -I) is given (taken) in the units set
	 * by MEASURE_UNIT */

	switch (gmtdefs.measure_unit) {
		case GMT_CM:
			*inch_to_unit = 2.54;
			strcpy (unit_name, "cm");
			break;
		case GMT_INCH:
			*inch_to_unit = 1.0;
			strcpy (unit_name, "inch");
			break;
		case GMT_M:
			*inch_to_unit = 0.0254;
			strcpy (unit_name, "m");
			break;
		case GMT_PT:
			*inch_to_unit = 72.0;
			strcpy (unit_name, "point");
			break;
	}
	*unit_to_inch = 1.0 / (*inch_to_unit);
	*fwd_scale = 1.0 / scales[unit];
	*inv_scale = scales[unit];
}

int GMT_check_scalingopt (char *args, char *unit_name) {
	int unit;

	switch (args[2]) {
		case '\0':
			unit = 0;
			strcpy (unit_name, "m");
			break;
		case 'k':
		case 'K':
			unit = 1;
			strcpy (unit_name, "km");
			break;
		case 'm':
		case 'M':
			unit = 2;
			strcpy (unit_name, "miles");
			break;
		case 'n':
		case 'N':
			unit = 3;
			strcpy (unit_name, "nautical miles");
			break;
		case 'I':
		case 'i':
			unit = 4;
			strcpy (unit_name, "inch");
			break;
		case 'c':
		case 'C':
			unit = 5;
			strcpy (unit_name, "cm");
			break;
		case 'p':
		case 'P':
			unit = 6;
			strcpy (unit_name, "point");
			break;
		default:
			fprintf (stderr, "%s: GMT ERROR Option -%c: Only append one of cimpkn\n", GMT_program, args[1]);
			exit (EXIT_FAILURE);
	}

	return (unit);
}

void GMT_set_measure_unit (char *args) {
	/* Option to override the GMT measure unit default */

	switch (args[2]) {
		case 'm':
		case 'M':
			gmtdefs.measure_unit = GMT_M;
			break;
		case 'I':
		case 'i':
			gmtdefs.measure_unit = GMT_INCH;
			break;
		case 'c':
		case 'C':
			gmtdefs.measure_unit = GMT_CM;
			break;
		case 'p':
		case 'P':
			gmtdefs.measure_unit = GMT_PT;
			break;
		default:
			fprintf (stderr, "%s: GMT ERROR Option -%c: Only append one of cimp\n", GMT_program, args[1]);
			exit (EXIT_FAILURE);
	}
}

void	GMT_init_time_system_structure () {

	/* The last time system is user-defined and set up here.
		All others are known and already complete.  */
	if (gmtdefs.time_system < (GMT_N_SYSTEMS - 1) ) return;

	/* Check the unit sanity:  */
	switch (GMT_time_system[gmtdefs.time_system].unit) {
		case 'y':
			/* This is a kludge:  we assume all years
			are the same length, thinking that a user
			with decimal years doesn't care about
			precise time.  To do this right would
			take an entirely different scheme, not
			a simple unit conversion. */
			GMT_time_system[gmtdefs.time_system].scale = (365.2425 * 86400);
			break;
		case 'o':
			/* This is also a kludge:  we assume all months
			are the same length, thinking that a user
			with decimal years doesn't care about
			precise time.  To do this right would
			take an entirely different scheme, not
			a simple unit conversion. */
			GMT_time_system[gmtdefs.time_system].scale = (365.2425 * 86400 / 12.0);
			break;
		case 'd':
			GMT_time_system[gmtdefs.time_system].scale = 86400.0;
			break;
		case 'h':
			GMT_time_system[gmtdefs.time_system].scale = 3600.0;
			break;
		case 'm':
			GMT_time_system[gmtdefs.time_system].scale = 60.0;
			break;
		case 's':
			GMT_time_system[gmtdefs.time_system].scale = 1.0;
			break;
		default:
			fprintf (stderr, "GMT_FATAL_ERROR:  gmtdefault TIME_UNIT is invalid.\n");
			fprintf (stderr, "    Choose one only from y o d h m s\n");
			fprintf (stderr, "    Corresponding to year month day hour minute second\n");
			fprintf (stderr, "    Note year and month are simply defined (365.2425 days and 1/12 of a year)\n");
			exit (EXIT_FAILURE);
			break;
	}
	/* Set inverse scale and store it to avoid divisions later */
	GMT_time_system[gmtdefs.time_system].i_scale = 1.0 / GMT_time_system[gmtdefs.time_system].scale;

#ifndef OLDCAL
	if ( GMT_scanf_epoch (GMT_time_system[gmtdefs.time_system].epoch,
		&GMT_time_system[gmtdefs.time_system].rata_die, &GMT_time_system[gmtdefs.time_system].epoch_t0) ) {
#else
	if ( GMT_scanf_epoch (GMT_time_system[gmtdefs.time_system].epoch,
		&GMT_time_system[gmtdefs.time_system].epoch_t0) ) {
#endif

		fprintf (stderr, "GMT_FATAL_ERROR:  gmtdefault TIME_EPOCH format is invalid.\n");
		fprintf (stderr, "   A correct format has the form [-]yyyy-mm-ddThh:mm:ss[.xxx]\n");
		fprintf (stderr, "   or (using ISO weekly calendar)   yyyy-Www-dThh:mm:ss[.xxx]\n");
		fprintf (stderr, "   An example of a correct format is:  %s\n", GMT_time_system[0].epoch);
		exit (EXIT_FAILURE);
	}
}

#ifndef OLDCAL
int	GMT_scanf_epoch (char *s, int *rata_die, double *t0) {
#else
int	GMT_scanf_epoch (char *s, double *t0) {
#endif

	/* Read a string which must be in one of these forms:
		[-]yyyy-mm-ddThh:mm:ss[.xxx]
		[-]yyyy-Www-dThh:mm:ss[.xxx]
	*/

	double	ss = 0.0;
	int i, vals[3], hh = 0, mm = 0;
	GMT_cal_rd rd;

	i = 0;
	while (s[i] && s[i] == ' ') i++;
	if (!(s[i])) return (-1);
	if (strchr (&s[i], 'W') ) {	/* ISO calendar string */
		if (strchr (&s[i], ':') ) {	/* ISO calendar string with date and clock */
			if ( (sscanf (&s[i], "%5d-W%2d-%1dT%2d:%2d:%lf", &vals[0], &vals[1], &vals[2], &hh, &mm, &ss) ) != 6) return (-1);
		}
		else {				/* ISO calendar date only */
			if ( (sscanf (&s[i], "%5d-W%2d-%1dT", &vals[0], &vals[1], &vals[2]) ) != 3) return (-1);
		}
		if (GMT_iso_ywd_is_bad (vals[0], vals[1], vals[2]) ) return (-1);
		rd = GMT_rd_from_iywd (vals[0], vals[1], vals[2]);
	}
	else {				/* Gregorian calendar string */
		if (strchr (&s[i], ':') ) {	/* Gregorian calendar string with date and clock */
			if ( (sscanf (&s[i], "%5d-%2d-%2dT%2d:%2d:%lf", &vals[0], &vals[1], &vals[2], &hh, &mm, &ss) ) != 6) return (-1);
		}
		else {				/* Gregorian calendar string with date only  */
			if ( (sscanf (&s[i], "%5d-%2d-%2dT", &vals[0], &vals[1], &vals[2]) ) != 3) return (-1);
		}
		if (GMT_g_ymd_is_bad (vals[0], vals[1], vals[2]) ) return (-1);
		rd = GMT_rd_from_gymd (vals[0], vals[1], vals[2]);
	}
	if (GMT_hms_is_bad (hh, mm, ss)) return (-1);

#ifndef OLDCAL
	*rata_die = rd;								/* Rata day number of epoch */
	*t0 =  (GMT_HR2SEC_F * hh + GMT_MIN2SEC_F * mm + ss) * GMT_SEC2DAY;	/* Fractional day (0<= t0 < 1) since rata_die of epoch */
#else
	*t0 = GMT_rdc2dt (rd, 60.0*(60.0*hh + mm) + ss);
#endif
	return (0);
}


/* Load a PostScript encoding from a file, given the filename.
 * Use Brute Force and Ignorance.
 */
static void load_encoding (struct gmt_encoding *enc)
{
	char line[GMT_LONG_TEXT], symbol[GMT_LONG_TEXT];
	int code = 0, pos;
	FILE *in;

	sprintf (line, "%s%cshare%cpslib%c%s.ps", GMTHOME, DIR_DELIM, DIR_DELIM, DIR_DELIM, enc->name);
	in = GMT_fopen (line, "r");

	if (!in)
	{
		perror (line);
		exit (EXIT_FAILURE);
	}

	while (fgets (line, sizeof line, in))
	{
		pos = 0;
		while ((GMT_strtok (line, " /\t\n", &pos, symbol)))
		{
			if (strcmp (symbol, "[") == 0)	/* We have found the start of the encoding array. */
			{
				code = 0;
				continue;
			}
			if (strcmp (symbol, "degree") == 0)
				enc->code[gmt_degree] = code;
			else if (strcmp (symbol, "ring") == 0)
				enc->code[gmt_ring] = code;
			else if (strcmp (symbol, "quotedbl") == 0)
				enc->code[gmt_dquote] = code;
			else if (strcmp (symbol, "quotesingle") == 0)
				enc->code[gmt_squote] = code;
			else if (strcmp (symbol, "colon") == 0)
				enc->code[gmt_colon] = code;
			code++;
		}
	}

	GMT_fclose (in);
}

void GMT_verify_encodings () {
	/* Check that special map-related codes are present - if not give warning */

	/* First check for degree symbol */

	if (gmtdefs.encoding.code[gmt_ring] == 32 && gmtdefs.encoding.code[gmt_degree] == 32) {	/* Neither /ring or /degree encoded */
		fprintf (stderr, "GMT Warning: Selected character encoding does not have suitable degree symbol - will use space instead\n");
	}
	else if (gmtdefs.degree_symbol == 0 && gmtdefs.encoding.code[gmt_ring] == 32) {		/* want /ring but only /degree is encoded */
		fprintf (stderr, "GMT Warning: Selected character encoding does not have ring symbol - will use degree symbol instead\n");
		gmtdefs.degree_symbol = 1;
	}
	else if (gmtdefs.degree_symbol == 1 && gmtdefs.encoding.code[gmt_degree] == 32) {	/* want /degree but only /ring is encoded */
		fprintf (stderr, "GMT Warning: Selected character encoding does not have degree symbol - will use ring symbol instead\n");
		gmtdefs.degree_symbol = 0;
	}

	/* Then single quote for minute symbol... */

	if (gmtdefs.degree_symbol < 2 && gmtdefs.encoding.code[gmt_squote] == 32) {
		fprintf (stderr, "GMT Warning: Selected character encoding does not have minute symbol (single quote) - will use space instead\n");
	}

	/* ... and double quote for second symbol */

	if (gmtdefs.degree_symbol < 2 && gmtdefs.encoding.code[gmt_dquote] == 32) {
		fprintf (stderr, "GMT Warning: Selected character encoding does not have second symbol (double quote) - will use space instead\n");
	}
}

void GMT_init_fonts (int *n_fonts)
{
	FILE *in;
	int i = 0, n_GMT_fonts, n_alloc = 50;
	char buf[BUFSIZ];
	char fullname[BUFSIZ];

	/* Loads the available fonts for this installation */

	/* First the standard 35 PostScript fonts from Adobe */

	sprintf (fullname, "%s%cshare%cpslib%cPS_font_info.d", GMTHOME, DIR_DELIM, DIR_DELIM, DIR_DELIM);

	if ((in = fopen (fullname, "r")) == NULL)
	{
		fprintf (stderr, "GMT Fatal Error: ");
		perror (fullname);
		exit (EXIT_FAILURE);
	}

	GMT_font = (struct GMT_FONT *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (struct GMT_FONT), GMT_program);

	while (fgets (buf, BUFSIZ, in)) {
		if (buf[0] == '#' || buf[0] == '\n' || buf[0] == '\r') continue;
		if (sscanf (buf, "%s %lf %*d", fullname, &GMT_font[i].height) != 2) {
			fprintf (stderr, "GMT Fatal Error: Trouble decoding font info for font %d\n", i);
			exit (EXIT_FAILURE);
		}
		GMT_font[i].name = (char *)GMT_memory (VNULL, (size_t)(strlen (fullname)+1), sizeof (char), GMT_program);
		strcpy (GMT_font[i].name, fullname);
		i++;
		if (i == n_alloc) {
			n_alloc += 50;
			GMT_font = (struct GMT_FONT *) GMT_memory ((void *)GMT_font, (size_t)n_alloc, sizeof (struct GMT_FONT), GMT_program);
		}
	}
	fclose (in);
	*n_fonts = n_GMT_fonts = i;

 	/* Then any custom fonts */

	sprintf (fullname, "%s%cshare%cpslib%cCUSTOM_font_info.d", GMTHOME, DIR_DELIM, DIR_DELIM, DIR_DELIM);

	if (!access (fullname, R_OK)) {	/* Decode Custom font file */

		if ((in = fopen (fullname, "r")) == NULL)
		{
			fprintf (stderr, "GMT Fatal Error: ");
			perror (fullname);
			exit (EXIT_FAILURE);
		}

		while (fgets (buf, BUFSIZ, in)) {
			if (buf[0] == '#' || buf[0] == '\n' || buf[0] == '\r') continue;
			GMT_font[i].name = (char *)GMT_memory (VNULL, strlen (buf), sizeof (char), GMT_program);
			if (sscanf (buf, "%s %lf %*d", GMT_font[i].name, &GMT_font[i].height) != 2) {
				fprintf (stderr, "GMT Fatal Error: Trouble decoding custom font info for font %d\n", i - n_GMT_fonts);
				exit (EXIT_FAILURE);
			}
			i++;
			if (i == n_alloc) {
				n_alloc += 50;
				GMT_font = (struct GMT_FONT *) GMT_memory ((void *)GMT_font, (size_t)n_alloc, sizeof (struct GMT_FONT), GMT_program);
			}
		}
		fclose (in);
		*n_fonts = i;
	}
	GMT_font = (struct GMT_FONT *) GMT_memory ((void *)GMT_font, (size_t)(*n_fonts), sizeof (struct GMT_FONT), GMT_program);
}

void GMT_set_processed_option (int key, BOOLEAN state)
{
	GMT_processed_option[key] = state;
}

#ifdef WIN32

/* Make dummy functions so GMT will link under WIN32 */

struct passwd *getpwuid (const int uid)
{
	return ((struct passwd *)NULL);
}

int getuid (void) {
	return (0);
}

#endif

#ifdef SET_IO_MODE

/* Under non-Unix operating systems running on the PC, such as
 * Windows and OS/2, files are opened in either TEXT or BINARY
 * mode.  This difference does not exist under UNIX, but is important
 * on the PC.  Specifically, it causes a problem when a program
 * that writes/reads standard i/o wants to use binary data.
 * In those situations we must change the default (TEXT) mode of
 * the file handle to BINARY via a call to "setmode".
 *
 * Under OS/2, using the EMX compilation system, one can change
 * the mode of input/output from TEXT to BINARY using _fsetmode.
 * Suggested by Alan Cogbill, Los Alamos National Laboratory
 *
 * This can also be done under Win32 with the Microsoft VC++
 * compiler which supports ANSI-C (P. Wessel).  This may be true
 * of other Win32 compilers as well.  Until we know if _setmode
 * would work in the same way under OS/2 we choose to take two
 * different routes, hence the #ifdefs below
 */

void GMT_setmode (int i_or_o)
{
	/* Changes the stream to deal with BINARY rather than TEXT data */

	FILE *fp;

	if (GMT_io.binary[i_or_o]) {	/* User wants binary */

		fp = (i_or_o == 0) ? GMT_stdin : GMT_stdout;
		fflush (fp);	/* Should be untouched but anyway... */
#ifdef _WIN32
		setmode (fileno (fp), _O_BINARY);
#else
		_fsetmode (fp, "b");
#endif
	}
}

#endif	/* SET_IO_MODE */
