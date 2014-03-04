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
 * gmt_dev.h is the main include file for the main development of gmt.
 * It includes the API (gmt.h) and contains lower-level definitions
 * for several of the structures and parameters used by all libraries.
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
 * counters of data records, which will be declared as uint64_t, and
 * variables that holds allocated number of bytes and similar, which
 * will be declared as size_t.  Occasionally, arrays of integer values
 * will be stored in smaller memory containers such as short int of
 * unsigned/signed char when the program logic places limits on their
 * possible ranges (e.g., true/false variables).
 */

#pragma once
#ifndef _GMT_DEV_H
#define _GMT_DEV_H

#ifdef __cplusplus	/* Basic C++ support */
extern "C" {
#endif

/* Declaration modifiers for DLL support (MSC et al) */
#include "declspec.h"

/* Declaration for PSL */
#include "pslib.h"

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

#include "gmt.h"             /* All GMT high-level API */
#include "gmt_private.h"     /* API declaration needed by libraries */

struct GMT_CTRL; /* forward declaration of GMT_CTRL */

#include "gmt_constants.h"      /* All basic constant definitions */
#include "gmt_dimensions.h"     /* Constant definitions created by configure */
#include "gmt_enums.h"          /* All basic enums definitions */
#include "gmt_macros.h"         /* All basic macros definitions */
#include "gmt_structs.h"        /* Declarations of basic GMT structures */
#include "gmt_texture.h"        /* Declarations of structures for dealing with pen, fill, etc. */
#include "gmt_defaults.h"       /* Declarations of structure for GMT default settings */
#include "gmt_ps.h"             /* Declarations of structure for GMT PostScript settings */
#include "gmt_hash.h"           /* Declarations of structure for GMT hashing */

#ifdef HAVE_GDAL
#	include "gmt_gdalread.h"      /* GDAL support */
#endif

#include "gmt_common.h"         /* For holding the GMT common option settings */
#include "gmt_fft.h"            /* Structures and enums used by programs needing FFTs */
#include "gmt_nan.h"            /* Machine-dependent macros for making and testing NaNs */
#include "gmt_error.h"          /* Only contains error codes */
#include "gmt_core_module.h" 	/* Core module modes and properties */
#include "gmt_supplements_module.h" 	/* Suppl module modes and properties */
#include "gmt_project.h"        /* Define GMT->current.proj and GMT->current.map.frame structures */
#include "gmt_grd.h"            /* Define grd file header structure */
#include "gmt_grdio.h"          /* Defines function pointers for grd i/o operations */
#include "gmt_io.h"             /* Defines structures and macros for table i/o */
#include "gmt_colors.h"         /* Defines color/shading global structure */
#include "gmt_shore.h"          /* Defines structures used when reading shore database */
#include "gmt_dcw.h"            /* Defines structure and functions used when using DCW polygons */
#include "gmt_calclock.h"       /* Calendar/time functions */
#include "gmt_symbol.h"         /* Custom symbol functions */
#include "gmt_contour.h"        /* Contour label structure and functions */
#include "gmt_map.h"            /* extern functions defined in gmt_map.c */
#include "gmt_plot.h"           /* extern functions defined in gmt_plot.c */
#include "gmt_memory.h"         /* extern functions defined in gmt_memory.c */
#include "gmt_support.h"        /* extern functions defined in gmt_support.c */
#include "gmt_crossing.h"       /* Declarations of structure for GMT map crossings */
#include "gmt_types.h"          /* GMT type declarations */

#ifdef _OPENMP                  /* Using open MP parallelization */
#include "omp.h"
#endif

#include "gmt_prototypes.h"     /* All GMT low-level API */
#include "gmt_stat.h"           /* extern functions defined in gmt_stat.c */

/* Size prefixes for printf/scanf for size_t and ptrdiff_t */
#ifdef _MSC_VER
#	define PRIxS "Ix"  /* printf size_t */
#	define PRIuS "Iu"  /* printf size_t */
#	define PRIdS "Id"  /* printf ptrdiff_t */
#	define SCNuS "u"   /* scanf  size_t (__int32 even on 64-bit platforms) */
#	define SCNdS "d"   /* scanf  ptrdiff_t (__int32 even on 64-bit platforms) */
#else
#	define PRIxS "zx"  /* printf size_t */
#	define PRIuS "zu"  /* printf size_t */
#	define PRIdS "zd"  /* printf ptrdiff_t */
#	define SCNuS PRIuS /* scanf  size_t */
#	define SCNdS PRIdS /* scanf  ptrdiff_t */
#endif

#ifdef __cplusplus
}
#endif

#endif  /* !_GMT_DEV_H */