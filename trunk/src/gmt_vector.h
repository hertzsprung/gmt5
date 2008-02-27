/*--------------------------------------------------------------------
 *	$Id: gmt_vector.h,v 1.6 2008-02-27 03:16:54 guru Exp $
 *
 *	Copyright (c) 1991-2008 by P. Wessel and W. H. F. Smith
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

#ifndef _GMT_VECTOR_H
#define _GMT_VECTOR_H

EXTERN_MSC double GMT_dot3v (double *a, double *b);
EXTERN_MSC double GMT_mag3v (double *a);
EXTERN_MSC int GMT_chol_dcmp (double *a, double *d, double *cond, int nr, int n);
EXTERN_MSC int GMT_fix_up_path (double **a_lon, double **a_lat, int n, double step, int mode);
EXTERN_MSC int GMT_jacobi (double *a, int *n, int *m, double *d, double *v, double *b, double *z, int *nrots);
EXTERN_MSC void GMT_cart_to_geo (double *alat, double *alon, double *a, int rads);
EXTERN_MSC void GMT_chol_recover (double *a, double *d, int nr, int n, int nerr, int donly);
EXTERN_MSC void GMT_chol_solv (double *a, double *x, double *y, int nr, int n);
EXTERN_MSC void GMT_cross3v (double *a, double *b, double *c);
EXTERN_MSC void GMT_geo_to_cart (double *alat, double *alon, double *a, int rads);
EXTERN_MSC void GMT_normalize3v (double *a);

#endif /* _GMT_VECTOR_H */
