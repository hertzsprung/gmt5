/*--------------------------------------------------------------------
 *	$Id: gmt_types.h,v 1.2 2011-03-15 02:06:36 guru Exp $
 *
 *	Copyright (c) 1991-2011 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
 *	See LICENSE.TXT file for copying and redistribution conditions.
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
 * gmt_constants.h contains definitions of special types used by GMT.
 *
 * Author:	Paul Wessel
 * Date:	01-OCT-2009
 * Version:	5 API
 */

#ifndef _GMT_TYPES_H
#define _GMT_TYPES_H

/*--------------------------------------------------------------------
 *			GMT TYPEDEF DEFINITIONS
 *--------------------------------------------------------------------*/

/* Note: Under Windows 64-bit a 64-bit integer is __int64 and when used
 * with scanf the format must be %lld.  This is not exactly what we call
 * POSIX-clean where %ld is expected.  Thus, in places where such 64-bit
 * variables are processed we let the compiler build the actual format
 * using the GMT_LL string which is either "l" or "ll"
 */
#ifdef _WIN64
typedef __int64 GMT_LONG;	/* A signed 8-byte integer under 64-bit Windows */
#define GMT_LL "ll"
#else
typedef long GMT_LONG;		/* A signed 4 (or 8-byte for 64-bit) integer */
#define GMT_LL "l"
#endif
typedef void (*PFV) ();		/* PFV declares a pointer to a function returning void */
typedef GMT_LONG (*PFL) ();	/* PFI declares a pointer to a function returning an GMT_LONG */
typedef int (*PFI) ();		/* PFI declares a pointer to a function returning an int */
typedef GMT_LONG (*PFB) ();	/* PFB declares a pointer to a function returning a GMT_LONG */
typedef double (*PFD) ();	/* PFD declares a pointer to a function returning a double */

#endif  /* _GMT_TYPES_H */
