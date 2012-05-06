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
 * gmt_types.h contains definitions of special types used by GMT.
 *
 * Author:	Paul Wessel
 * Date:	01-OCT-2009
 * Version:	5 API
 */

#ifndef _GMT_TYPES_H
#define _GMT_TYPES_H
#include <stdbool.h>
#include <stdint.h>
/* Two types for counting, depending on expected range of integers we need */
#define COUNTER_LARGE	uint64_t
#define COUNTER_MEDIUM	unsigned
/* Type for TRUE [!0] or FALSE [0] only */
#define BOOLEAN	bool

/*--------------------------------------------------------------------
 *			GMT TYPEDEF DEFINITIONS
 *--------------------------------------------------------------------*/

/* Note: Under Windows 64-bit a 64-bit integer is __int64 and when used
 * with scanf the format must be %lld.  This is not exactly what we call
 * POSIX-clean where %ld is expected.  Thus, in places where such 64-bit
 * variables are processed we let the compiler build the actual format
 * using the GMT_LL string which is either "l" or "ll"
 */

typedef int GMT_LONG;			/* plain integer */
typedef unsigned int GMT_ULONG;	/* plain unsigned */
#define GMT_LL ""

typedef void (*PFV) ();         /* PFV declares a pointer to a function returning void */
typedef void* (*PFP) ();        /* PFP declares a pointer to a function returning void* */
typedef GMT_LONG (*PFL) ();     /* PFL declares a pointer to a function returning an GMT_LONG */
typedef size_t (*PFS) ();    	/* PFS declares a pointer to a function returning an size_t */
typedef uint64_t (*PFU8) ();    /* PFU8 declares a pointer to a function returning an COUNTER_LARGE */
typedef unsigned (*PFU4) ();    /* PFU4 declares a pointer to a function returning an unsigned int */
typedef int (*PFI) ();          /* PFI declares a pointer to a function returning an int */
typedef double (*PFD) ();       /* PFD declares a pointer to a function returning a double */
typedef BOOLEAN (*PFB) ();	/* PFB declares a pointer to a function returning a BOOLEAN */

typedef struct {float x[2];} fpair;	/* Can be used to hold pairs of data, e.g. real, imag or x, weight */
typedef struct {double x[2];} dpair;	/* Same, at double precision */

#endif  /* _GMT_TYPES_H */
