/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis and F. Wobbe
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
 * gmt_constants.h contains definitions of constants used throught GMT.
 *
 * Author:	Paul Wessel
 * Date:	01-OCT-2009
 * Version:	5 API
 */

#ifndef _GMT_CONSTANTS_H
#define _GMT_CONSTANTS_H

/*=====================================================================================
 *	GMT API CONSTANTS DEFINITIONS
 *===================================================================================*/

#include "gmtapi_error_codes.h"			/* All API error codes are defined here */
#include "gmtapi_define.h"			/* All constant values are defined here */

/*--------------------------------------------------------------------
 *			GMT CONSTANTS MACRO DEFINITIONS
 *--------------------------------------------------------------------*/

#ifndef TWO_PI
#define TWO_PI        6.28318530717958647692
#endif
#ifndef M_PI
#define M_PI          3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2          1.57079632679489661923
#endif
#ifndef M_PI_4
#define M_PI_4          0.78539816339744830962
#endif
#ifndef M_E
#define	M_E		2.7182818284590452354
#endif
#ifndef M_SQRT2
#define	M_SQRT2		1.41421356237309504880
#endif
#ifndef M_LN2_INV
#define	M_LN2_INV	(1.0 / 0.69314718055994530942)
#endif
#ifndef M_EULER
#define M_EULER		0.577215664901532860606512	/* Euler's constant (gamma) */
#endif

#define GMT_CONV_LIMIT	1.0e-8		/* Fairly tight convergence limit or "close to zero" limit */
#define GMT_SMALL	1.0e-4		/* Needed when results aren't exactly zero but close */

/* Various allocation-length parameters */
enum GMT_enum_length {
	GMT_TINY_CHUNK  = 8U,
	GMT_SMALL_CHUNK = 64U,
	GMT_CHUNK       = 2048U,
	GMT_TEXT_LEN64  = 64U,          /* Intermediate length of texts */
	GMT_TEXT_LEN256 = 256U,         /* Max size of some text items */
	GMT_MAX_COLUMNS = 4096U,        /* Limit on number of columns in data tables (not grids) */
	GMT_BUFSIZ      = 4096U,        /* Size of char record for i/o */
	GMT_MIN_MEMINC  = 2048U,        /* E.g., 16 kb of 8-byte doubles */
	GMT_MAX_MEMINC  = 67108864U};   /* E.g., 512 Mb of 8-byte doubles */

/* The four plot length units [m just used internally] */
enum GMT_enum_unit {
	GMT_CM = 0,
	GMT_INCH,
	GMT_M,
	GMT_PT};

/* Handling of swap/no swap in i/o */
enum GMT_swap_direction {
	k_swap_none = 0,
	k_swap_in,
	k_swap_out};

#define GMT_DIM_UNITS	"cip"		/* Plot dimensions in cm, inch, or point */
#define GMT_LEN_UNITS2	"efkMn"		/* Distances in meter, feet, km, Miles, nautical miles */
#define GMT_LEN_UNITS	"dmsefkMn"	/* Distances in arc-{degree,minute,second} or meter, feet, km, Miles, nautical miles */
#define GMT_DIM_UNITS_DISPLAY	"c|i|p"			/* Same, used to display as options */
#define GMT_LEN_UNITS_DISPLAY	"d|m|s|e|f|k|M|n"	/* Same, used to display as options */
#define GMT_LEN_UNITS2_DISPLAY	"e|f|k|M|n"		/* Same, used to display as options */
#define GMT_DEG2SEC_F	3600.0
#define GMT_DEG2SEC_I	3600
#define GMT_SEC2DEG	(1.0 / GMT_DEG2SEC_F)
#define GMT_DEG2MIN_F	60.0
#define GMT_DEG2MIN_I	60
#define GMT_MIN2DEG	(1.0 / GMT_DEG2MIN_F)
#define GMT_MIN2SEC_F	60.0
#define GMT_MIN2SEC_I	60
#define GMT_SEC2MIN	(1.0 / GMT_MIN2SEC_F)
#define GMT_DAY2HR_F	24.0
#define GMT_DAY2HR_I	24
#define GMT_HR2DAY	(1.0 / GMT_DAY2HR_F)
#define GMT_DAY2MIN_F	1440.0
#define GMT_DAY2MIN_I	1440
#define GMT_MIN2DAY	(1.0 / GMT_DAY2MIN_F)
#define GMT_DAY2SEC_F	86400.0
#define GMT_DAY2SEC_I	86400
#define GMT_SEC2DAY	(1.0 / GMT_DAY2SEC_F)
#define GMT_HR2SEC_F	3600.0
#define GMT_HR2SEC_I	3600
#define GMT_SEC2HR	(1.0 / GMT_HR2SEC_F)
#define GMT_HR2MIN_F	60.0
#define GMT_HR2MIN_I	60
#define GMT_MIN2HR	(1.0 / GMT_HR2MIN_F)

#define GMT_YR2SEC_F	(365.2425 * 86400.0)
#define GMT_MON2SEC_F	(365.2425 * 86400.0 / 12.0)

#define GMT_DEC_SIZE	0.54	/* Size of a decimal number compared to point size */
#define GMT_PER_SIZE	0.30	/* Size of a decimal point compared to point size */

#define GMT_PEN_LEN	128
#define GMT_PENWIDTH	0.25	/* Default pen width in points */

/* Various options for FFT calculations [Default is 0] */
enum FFT_implementations {
	k_fft_auto = 0,    /* Automatically select best FFT algorithm */
	k_fft_accelerate,  /* Select Accelerate Framework vDSP FFT [OS X only] */
	k_fft_fftw3,       /* Select FFTW-3 */
	k_fft_kiss,        /* Select Kiss FFT */
	k_fft_brenner,     /* Select Normal Brenners old-school FFT */
	k_n_fft_algorithms /* Number of FFT implementations available in GMT */
};

/* Various directions and modes to call the FFT */
enum FFT_modes {
	k_fft_fwd     = 0, /* forward Fourier transform */
	k_fft_inv     = 1, /* inverse Fourier transform */
	k_fft_real    = 0, /* real-input FT (currently unsupported) */
	k_fft_complex = 1  /* complex-input Fourier transform */
};

/* Various algorithms for triangulations */
enum GMT_enum_tri {
	GMT_TRIANGLE_WATSON = 0, /* Select Watson's algorithm */
	GMT_TRIANGLE_SHEWCHUK};  /* Select Shewchuk's algorithm */

/* Various grid/image interpolation modes */
enum GMT_enum_bcr {
	BCR_NEARNEIGHBOR = 0, /* Nearest neighbor algorithm */
	BCR_BILINEAR,         /* Bilinear spline */
	BCR_BSPLINE,          /* B-spline */
	BCR_BICUBIC};         /* Bicubic spline */

/* Various grid/image boundary conditions */
enum GMT_enum_bc {
	GMT_BC_IS_NOTSET = 0, /* BC not yet set */
	GMT_BC_IS_NATURAL,    /* Use natural BC */
	GMT_BC_IS_PERIODIC,   /* Use periodic BC */
	GMT_BC_IS_POLE,       /* N or S pole BC condition */
	GMT_BC_IS_DATA};      /* Fill in BC with actual data */

enum GMT_enum_alloc {
	GMT_ALLOCATED = 0, /* Item was allocated so GMT_* modules should free when GMT_Destroy_Data is called */
	GMT_REFERENCE,     /* Item was not allocated so GMT_* modules should NOT free when GMT_Destroy_Data is called, but may realloc if needed */
	GMT_READONLY,      /* Item was not allocated so GMT_* modules should NOT free when GMT_Destroy_Data is called . Consider read-only data */
	GMT_CLOBBER};      /* Free item no matter what its allocation status */

/* Help us with big and little endianness */
#ifdef WORDS_BIGENDIAN
#define GMT_BIGENDIAN	true
#define GMT_ENDIAN		'B'
#else
#define GMT_BIGENDIAN	false
#define GMT_ENDIAN		'L'
#endif

#endif  /* _GMT_CONSTANTS_H */
