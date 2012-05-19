/*--------------------------------------------------------------------
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
 * gmt.h is the main include file for GMT.  It contains definitions
 * for several of the structures and parameters used by all programs.
 * It also includes all of the other include files that are needed.
 *
 * Author:	Paul Wessel
 * Date:	01-AUG-2011
 * Version:	5 API
 */

/* Note on data type:  GMT will generally use double precision for
 * all floating point values except for grids which are held in single
 * precision floats.  All integer values are standard int (presumably
 * 32-bit) except for quantities that may be very large, such as
 * counters of data records, which will be declared as COUNTER_LARGE, and
 * variables that holds allocated number of bytes and similar, which
 * will be declared as size_t.  Occasionally, arrays of integer values
 * will be stored in smaller memory containers such as short int of
 * unsigned/signed char when the program logic places limits on their
 * possible ranges (e.g., true/false variables).
 */

#pragma once
#ifndef _GMT_H
#define _GMT_H

#ifdef __cplusplus	/* Basic C++ support */
extern "C" {
#endif

/* CMake definitions: This must be first! */
#include "gmt_config.h"

/* GMT 4 compatibility */
#ifdef DEBUG
#define GMT_MSG_COMPAT 1	/* Set high rank warning level for compatibility for debuggers */
#else
#define GMT_MSG_COMPAT 1	/* Set lower rank warning level otherwise */
#endif

/* Declaration modifiers for DLL support (MSC et al) */
#include "declspec.h"


/*--------------------------------------------------------------------
 *      SYSTEM HEADER FILES
 *--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <float.h>
#include <math.h>
#include <limits.h>

#include <time.h>

#include "gmt_types.h"          /* All basic typedef declarations */
#include "gmt_notposix.h"       /* Non-POSIX extensions */

#include "gmt_constants.h"      /* All basic constant definitions */
#include "gmt_macros.h"         /* All basic macros definitions */
#include "gmt_dimensions.h"     /* Constant definitions created by configure */
#include "gmt_time.h"           /* Declarations of structures for dealing with time */
#include "gmt_texture.h"        /* Declarations of structures for dealing with pen, fill, etc. */
#include "gmt_defaults.h"       /* Declarations of structure for GMT default settings */
#include "gmt_ps.h"             /* Declarations of structure for GMT PostScript settings */
#include "gmt_hash.h"           /* Declarations of structure for GMT hashing */
#include "gmt_crossing.h"       /* Declarations of structure for GMT map crossings */

/* Experimental GDAL support */
#ifdef USE_GDAL
#include "gmt_gdalread.h"
#endif
#include "gmt_common.h"         /* For holding the GMT common option settings */
#include "gmt_nan.h"            /* Machine-dependent macros for making and testing NaNs */
#include "gmt_error.h"          /* Only contains error codes */
#include "gmt_synopsis.h"       /* Only contains macros for synopsis lines */
#include "gmt_version.h"        /* Only contains the current GMT version number */
#include "gmt_project.h"        /* Define GMT->current.proj and GMT->current.map.frame structures */
#include "gmt_grd.h"            /* Define grd file header structure */
#include "gmt_grdio.h"          /* Defines function pointers for grd i/o operations */
#include "gmt_io.h"             /* Defines structures and macros for table i/o */
#include "gmt_colors.h"         /* Defines color/shading global structure */
#include "gmt_shore.h"          /* Defines structures used when reading shore database */
#include "gmt_calclock.h"       /* Calendar/time functions */
#include "gmt_symbol.h"         /* Custom symbol functions */
#include "gmt_contour.h"        /* Contour label structure and functions */
#include "gmt_map.h"            /* extern functions defined in gmt_map.c */
#include "gmt_plot.h"           /* extern functions defined in gmt_plot.c */
#include "gmt_init.h"           /* extern functions defined in gmt_init.c */
#include "gmt_support.h"        /* extern functions defined in gmt_support.c */
#include "gmt_vector.h"         /* extern functions defined in gmt_vector.c */

#ifdef _OPENMP                  /* Using open MP parallelization */
#include "omp.h"
#endif

struct GMT_MAP {		/* Holds all map-related parameters */
	struct GMT_PLOT_FRAME frame;		/* Everything about the frame parameters */
	GMT_LONG this_x_status;			/* Tells us what quadrant old and new points are in (-4/4) */
	GMT_LONG this_y_status;
	GMT_LONG prev_x_status;
	GMT_LONG prev_y_status;
	GMT_LONG corner;
	GMT_BOOLEAN on_border_is_outside;		/* TRUE if a point exactly on the map border shoud be considered outside the map */
	GMT_BOOLEAN is_world;			/* TRUE if map has 360 degrees of longitude range */
	GMT_BOOLEAN is_world_tm;			/* TRUE if GMT_TM map is global? */
	GMT_BOOLEAN lon_wrap;			/* TRUE when longitude wrapping over 360 degrees is allowed */
	GMT_BOOLEAN meridian_straight;		/* TRUE if meridians plot as straight lines */
	GMT_BOOLEAN parallel_straight;		/* TRUE if parallels plot as straight lines */
	GMT_BOOLEAN z_periodic;			/* TRUE if grid values are 0-360 degrees (phases etc) */
	COUNTER_MEDIUM n_lon_nodes;		/* Somewhat arbitrary # of nodes for lines in longitude (may be reset in gmt_map.c) */
	COUNTER_MEDIUM n_lat_nodes;		/* Somewhat arbitrary # of nodes for lines in latitude (may be reset in gmt_map.c) */
	COUNTER_MEDIUM path_mode;		/* 0 if we should call GMT_fix_up_path to resample across gaps > path_step, 1 to leave alone */
	double width;				/* Full width in inches of this world map */
	double height;				/* Full height in inches of this world map */
	double half_width;			/* Half width in inches of this world map */
	double half_height;			/* Half height of this world map */
	double dlon;				/* Steps taken in longitude along gridlines (gets reset in gmt_init.c) */
	double dlat;				/* Steps taken in latitude along gridlines (gets reset in gmt_init.c) */
	double path_step;			/* Sampling interval if resampling of paths should be done */
	p_func_b outside;			/* Pointer to function checking if a lon/lat point is outside map */
	p_func_b overlap;			/* Pointer to function checking for overlap between 2 regions */
	p_func_b will_it_wrap;			/* TRUE if consecutive points indicate wrap */
	p_func_l jump;				/* TRUE if we jump in x or y */
	p_func_l crossing;			/* Pointer to functions returning crossover point at boundary */
	p_func_l clip;				/* Pointer to functions that clip a polygon to fit inside map */
	p_func_d left_edge, right_edge;		/* Pointers to functions that return left/right edge of map */
	struct GMT_DIST dist[3];		/* Pointers to functions/scales returning distance between two points points */
	p_func_d azimuth_func;			/* Pointer to function returning azimuth between two points points */
	p_func_l near_lines_func;		/* Pointer to function returning distance to nearest line among a set of lines */
	p_func_l near_a_line_func;		/* Pointer to function returning distance to line */
	p_func_l near_point_func;		/* Pointer to function returning distance to nearest point */
	p_func_l wrap_around_check;		/* Does x or y wrap checks */
	p_func_b this_point_wraps;		/* Used in above */
	p_func_v get_crossings;			/* Returns map crossings in x or y */
	p_func_l truncate;			/* Truncate polygons agains boundaries */
};

struct GMT_TIME_CONV {		/* Holds all time-related parameters */
	struct GMT_TRUNCATE_TIME truncate;
	struct GMT_Y2K_FIX Y2K_fix;		/* Used to convert 2-digit years to 4-digit years */
	struct GMT_TIME_LANGUAGE language;	/* For time axis */
};

struct GMT_INIT {		/* Holds misc run-time parameters */
	COUNTER_MEDIUM n_custom_symbols;
	/* The rest of the struct contains pointers that may point to memory not included by this struct */
	char *progname;					/* Name of current GMT program */
	char *runtime_bindir;				/* Directory that contains the main exe at run-time */
	char *module_name;				/* Name of current GMT module */
	char *history[GMT_N_UNIQUE];			/* The internal .gmtcommands information */
	struct GMT_CUSTOM_SYMBOL **custom_symbol;	/* For custom symbol plotting in psxy[z]. */

};

struct GMT_PLOT {		/* Holds all plotting-related parameters */
	COUNTER_LARGE n;			/* Number of such points */
	size_t n_alloc;			/* Size of allocated plot arrays */
	GMT_BOOLEAN r_theta_annot;		/* TRUE for special r-theta map annotation (see GMT_get_annot_label) */
	COUNTER_MEDIUM mode_3D;		/* Determines if we draw fore and/or back 3-D box lines [Default is both] */
	GMT_LONG *pen;			/* Pen (PSL_MOVE = up, PSL_DRAW = down) for these points */
	struct GMT_PLOT_CALCLOCK calclock;
	/* The rest of the struct contains pointers that may point to memory not included by this struct */
	double *x;			/* Holds the x/y (inches) of a line to be plotted */
	double *y;
	char format[3][2][GMT_TEXT_LEN256];	/* Keeps the 6 formats for dd:mm:ss plot output */
};

struct GMT_CURRENT {
	/* These are internal parameters that need to be passed around between
	 * many GMT functions.  These values may change by user interaction. */
	struct GMT_DEFAULTS setting;	/* Holds all GMT defaults parameters */
	struct GMT_IO io;		/* Holds all i/o-related parameters */
	struct GMT_PROJ proj;		/* Holds all projection-related parameters */
	struct GMT_MAP map;		/* Holds all projection-related parameters */
	struct GMT_PLOT plot;		/* Holds all plotting-related parameters */
	struct GMT_TIME_CONV time;	/* Holds all time-related parameters */
	struct GMT_PS ps;		/* Hold parameters related to PS setup */
};

struct GMT_INTERNAL {
	/* These are internal parameters that need to be passed around between
	 * many GMT functions.  These may change during execution but are not
	 * modified directly by user interaction. */
	COUNTER_MEDIUM func_level;	/* Keeps track of what level in a nested GMT_func calling GMT_func etc we are.  0 is top function */
};

struct GMT_SHORTHAND {	/* Holds information for each grid extension shorthand read from the user's .gmtio file */
	double scale, offset, nan;
	COUNTER_MEDIUM id;
	char *suffix;
};
	
struct GMT_SESSION {
	/* These are parameters that is set once at the start of a GMT session and
	 * are essentially read-only constants for the duration of the session */
	FILE *std[3];			/* Pointers for standard input, output, and error */
	p_func_vp input_ascii;		/* Pointer to function reading ascii tables only */
	p_func_l output_ascii;		/* Pointer to function writing ascii tables only */
	COUNTER_MEDIUM n_fonts;		/* Total number of fonts returned by GMT_init_fonts */
	COUNTER_MEDIUM n_user_media;	/* Total number of user media returned by gmt_load_user_media */
	size_t min_meminc;		/* DEBUG, sets min/max memory increments */
	size_t max_meminc;
	float f_NaN;			/* Holds the IEEE NaN for floats */
	double d_NaN;			/* Holds the IEEE NaN for doubles */
	double no_rgb[4];		/* To hold {-1, -1, -1, 0} when needed */
	double u2u[4][4];		/* u2u is the 4x4 conversion matrix for cm, inch, m, pt */
	char unit_name[4][8];		/* Full name of the 4 units cm, inch, m, pt */
	struct GMT_HASH rgb_hashnode[GMT_N_COLOR_NAMES];/* Used to translate colornames to r/g/b */
	COUNTER_MEDIUM n_shorthands;			/* Length of arrray with shorthand information */
	COUNTER_MEDIUM grdcode[GMT_N_GRD_FORMATS];	/* Old (obsolete) grid ID code */
	char *grdformat[GMT_N_GRD_FORMATS];	/* Type and description of grid format */
	p_func_l readinfo[GMT_N_GRD_FORMATS];	/* Pointers to grid read header functions */
	p_func_l updateinfo[GMT_N_GRD_FORMATS];	/* Pointers to grid update header functions */
	p_func_l writeinfo[GMT_N_GRD_FORMATS];	/* Pointers to grid write header functions */
	p_func_l readgrd[GMT_N_GRD_FORMATS];		/* Pointers to grid read functions */
	p_func_l writegrd[GMT_N_GRD_FORMATS];	/* Pointers to grid read functions */
	p_func_l fft1d[N_GMT_FFT];			/* Pointers to available 1-D FFT functions (or NULL if not configured) */
	p_func_l fft2d[N_GMT_FFT];			/* Pointers to available 2-D FFT functions (or NULL if not configured) */
	/* This part contains pointers that may point to additional memory outside this struct */
	char *GSHHGDIR;			/* Path to the GSHHG directory */
	char *SHAREDIR;			/* Path to the GMT share directory */
	char *HOMEDIR;			/* Path to the user's home directory */
	char *USERDIR;			/* Path to the user's GMT settings directory */
	char *DATADIR;			/* Path to one or more directories with data sets */
	char *TMPDIR;			/* Path to the directory directory for isolation mode */
	char **user_media_name;		/* Length of array with custom media dimensions */
	struct GMT_FONTSPEC *font;		/* Array with font names and height specification */
	struct GMT_MEDIA *user_media;		/* Array with custom media dimensions */
	struct GMT_SHORTHAND *shorthand;	/* Array with info about shorthand file extension magic */
};
	
struct GMT_CTRL {
	/* Master structure for a GMT invokation.  All internal settings for GMT is accessed here */
	struct GMT_SESSION session;	/* Structure with all values that do not change throughout a session */
	struct GMT_INIT init;		/* Structure with all values that do not change in a GMT_func call */
	struct GMT_COMMON common;	/* Structure with all the common GMT command settings (-R -J ..) */
	struct GMT_CURRENT current;	/* Structure with all the GMT items that can change during execution, such as defaults settings (pens, colors, fonts.. ) */
	struct GMT_INTERNAL hidden;	/* Internal global variables that are not to be changed directly by users */
	struct PSL_CTRL *PSL;		/* Pointer to the PSL structure [or NULL] */
	struct GMTAPI_CTRL *parent;	/* Owner of this structure [or NULL]; gives access to the API from functions being passed *GMT only */
};

#include "gmtapi.h"             /* All GMT high-level API */
#include "gmt_prototypes.h"     /* All GMT low-level API */
#include "gmt_stat.h"           /* extern functions defined in gmt_stat.c */
#include "common_math.h"        /* Shared math functions */
#include "common_string.h"      /* All code shared between GMT and PSL */

#include "gmt_modules.h"        /* All GMT_* modules */

#ifdef DEBUG
/* Items needed if -DDEBUG is in effect */
EXTERN_MSC void GMT_memtrack_init (struct GMT_CTRL *C, struct MEMORY_TRACKER **M);
EXTERN_MSC void GMT_memtrack_report (struct GMT_CTRL *C, struct MEMORY_TRACKER *M);
EXTERN_MSC void GMT_memtrack_on (struct GMT_CTRL *C, struct MEMORY_TRACKER *M);
EXTERN_MSC void GMT_memtrack_off (struct GMT_CTRL *C, struct MEMORY_TRACKER *M);

/* This single pointer will be a global to avoid having to pass it just do to debugging */
EXTERN_MSC struct MEMORY_TRACKER *GMT_mem_keeper;
#endif

#ifdef __cplusplus
}
#endif

#endif  /* !_GMT_H */
