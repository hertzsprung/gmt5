	20.0,			/* ANNOT_MIN_ANGLE */
	0.0,			/* ANNOT_MIN_SPACING */
	{0, 0},			/* ANNOT_FONT_PRIMARY, ANNOT_FONT_SECONDARY*/
	{14, 16},		/* ANNOT_FONT_SIZE_PRIMARY, ANNOT_FONT_SIZE_SECONDARY */
	{0.075,	0.075},		/* ANNOT_OFFSET_PRIMARY, ANNOT_OFFSET_SECONDARY */
	"WESN",			/* BASEMAP_AXES */
	{0, 0, 0},		/* BASEMAP_FRAME_RGB */
	0,			/* BASEMAP_TYPE */
	{0, 0, 0},		/* COLOR_BACKGROUND */
	{255, 255, 255},	/* COLOR_FOREGROUND */
	{128, 128, 128},	/* COLOR_NAN */
	0,			/* COLOR_IMAGE */
	3,			/* COLOR_MODEL */
	"%lg",			/* D_FORMAT */
	0,			/* DEGREE_FORMAT */
	300,			/* DOTS_PR_INCH */
	0,			/* ELLIPSOID */
	{1.25, 0.0, {0, 0, 0}, ""},	/* FRAME_PEN */
	0.075,			/* FRAME_WIDTH */
	1.0,			/* GLOBAL_X_SCALE */
	1.0,			/* GLOBAL_Y_SCALE */
	{0.0, 0.0},		/* GRID_CROSS_SIZE P/S */
	"nf",			/* GRID_FORMAT */
	{
	  {0.25, 0.0, {0, 0, 0}, ""},	/* GRID_PEN_PRIMARY */
	  {0.5,  0.0, {0, 0, 0}, ""}	/* GRID_PEN_SECONDARY */
	},
	FALSE,			/* GRIDFILE_SHORTHAND */
	0,			/* HEADER_FONT */
	36,			/* HEADER_FONT_SIZE */
	0.1875,			/* HEADER_OFFSET */
	1.0,			/* HSV_MIN_SATURATION */
	0.1,			/* HSV_MAX_SATURATION */
	0.3,			/* HSV_MIN_VALUE */
	1.0,			/* HSV_MAX_VALUE */
	1,			/* INTERPOLANT */
	{ FALSE, FALSE },	/* IO_HEADER [0|1] */
	1,			/* N_HEADER_RECS (if -H is set) */
	0,			/* LABEL_FONT */
	24,			/* LABEL_FONT_SIZE */
	0.1125,			/* LABEL_OFFSET */
	TRUE,			/* LAST_PAGE */
	0.01,
	-1.0,			/* MAP_SCALE_FACTOR */
	0.075,			/* MAP_SCALE_HEIGHT */
	1,			/* MEASURE_UNIT set to inch */
	25,			/* MEDIA set to Letter */
	1,			/* N_COPIES */
	50,			/* N_LAT_NODES */
	50,			/* N_LON_NODES */
	0.0,			/* --SET AT RUNTIME */
	0.0,			/* --SET AT RUNTIME */
	1,			/* OBLIQUE_ANNOTATION */
	FALSE,			/* OVERLAY */
	{255, 255, 255},	/* PAGE_COLOR */
	0,			/* PAGE_ORIENTATION */
	{612, 792},		/* PAPER_MEDIA (US Letter) */
	{85.0, 90.0},		/* POLAR_CAP (controls gridlines near poles) */
	0,			/* PS_COLOR (2 = HSV, 1 = CMYK, 0 = RGB) */
	0,			/* PS_IMAGE_COMPRESS (0 = NONE, 1 = RLE, 2 = LZW) */
	TRUE,			/* PS_IMAGE_FORMAT (TRUE = HEX, FALSE = BIN) */
	0,			/* PS_LINE_CAP (0 = butt, 1 = round, 2 = square) */
	0,			/* PS_LINE_JOIN (0 = miter, 1 = arc, 2 = bevel) */
	0,			/* PS_MITER_LIMIT (0 = Default, or 1-180) */
	FALSE,			/* PS_VERBOSE (TRUE = write comments, FALSE = no comments) */
	0.075,			/* TICK_LENGTH */
	{0.5, 0.0, {0, 0, 0}, ""},	/* TICK_PEN */
	FALSE,			/* UNIX_TIME */
	"",			/* --SET AT RUNTIME */
	{-0.75, -0.75},		/* UNIX_TIME_POS */
	0.0,			/* VECTOR_SHAPE */
	FALSE,			/* VERBOSE */
	FALSE,			/* WANT_EURO_FONT */
	9.0,			/* X_AXIS_LENGTH */
	6.0,			/* Y_AXIS_LENGTH */
	1.0,			/* X_ORIGIN */
	1.0,			/* Y_ORIGIN */
	{FALSE,	FALSE},		/* XY_TOGGLE */
	0,			/* Y_AXIS_TYPE */
	{	/* Ellipsoid structure and its members */
#include "gmt_ellipsoids.h"	/* This is created by Makefile.guru - do not edit it manually */
	},
	{	/* Datum structure and its members */
#include "gmt_datums.h"		/* This is created by Makefile.guru - do not edit it manually */
	},
	"hh:mm:ss",		/* INPUT_CLOCK_FORMAT */
	"yyyy-mm-dd",		/* INPUT_DATE_FORMAT */
	"hh:mm:ss",		/* OUTPUT_CLOCK_FORMAT */
	"yyyy-mm-dd",		/* OUTPUT_DATE_FORMAT */
	"+D",			/* OUTPUT_DEGREE_FORMAT */
	"hh:mm:ss",		/* PLOT_CLOCK_FORMAT */
	"yyyy-mm-dd",		/* PLOT_DATE_FORMAT */
	"+ddd:mm:ss",		/* PLOT_DEGREE_FORMAT */
	{ "full", "full" },	/* TIME_FORMAT_PRIMARY, TIME_FORMAT_SECONDARY */
	FALSE,			/* TIME_IS_INTERVAL */
	0.5,			/* TIME_INTERVAL_FRACTION */
	FALSE,			/* WANT_LEAP_SECONDS */
	"2000-01-01T00:00:00",	/* TIME_EPOCH */
	'd',			/* TIME_UNIT */
	0,			/* TIME_SYSTEM */
	0,			/* TIME_WEEK_START */
	"us",			/* TIME_LANGUAGE */
	1950,			/* Y2K_OFFSET_YEAR */
	"\t",			/* FIELD_DELIMITER */
	gmt_ring,		/* DEGREE_SYMBOL */
	{ "Standard", 		/* CHAR_ENCODING */
	  { 32, 32, 32, 32, 32 } } /* PostScript codes for degree, ring, colon, singlequote, and doublequote [Initialized to space] */
