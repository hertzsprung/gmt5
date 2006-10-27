/*--------------------------------------------------------------------
 *	$Id: gmt_bcr.h,v 1.10 2006-10-27 23:45:22 pwessel Exp $
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
 *--------------------------------------------------------------------*/
/* bcr.h  --  stuff for implementing bicubic rectangle grid interpolation.
   Has bilinear case as a subset.
   The bilinear interpolation will give continuity of the function z only.
   The bicubic interpolation will give continuity of z, dz/dx, dz/dy, and
   d2z/dxdy.
   Designed to operate with at least one spare row/column around each edge,
   to allow derivative computations.
   Follows the outline in Lancaster and Salkauskas, section 9.3
   Meant to replace Taylor routines in GMT in version 3.0 of GMT.

   Author:	Walter H F Smith
   Date:	23 September, 1993
   Version:	4.1.x

   This include file defines structures and functions used.
*/

#ifndef _GMT_BCR_H
#define _GMT_BCR_H

struct GMT_BCR {	/* Used mostly in gmt_support.c */
	double	nodal_value[4][4];	/* z, dz/dx, dz/dy, d2z/dxdy at 4 corners  */
	double	bcr_basis[4][4];	/* multiply on nodal vals, yields z at point */
	double	bl_basis[4];		/* bilinear basis functions  */
	double	rx_inc;			/* 1.0 / grd.x_inc  */
	double	ry_inc;			/* 1.0 / grd.y_inc  */
	double	offset;			/* 0 or 0.5 for grid or pixel registration  */
	double	threshold;		/* sum of cardinals must >= threshold in bilinear; else NaN */
/* If we later want to estimate of dz/dx or dz/dy, we will need [4][4] basis for these  */
	int	ij_move[4];		/* add to ij of zero vertex to get other vertex ij  */
	int	i;			/* Location of current nodal_values  */
	int	j;			/* Ditto.   */
	int	bilinear;		/* T/F use bilinear instead of bicubic  */
	int	nan_condition;		/* T/F we cannot evaluate; return z = NaN  */
	int	ioff;			/* Padding on west side of array  */
	int	joff;			/* Padding on north side of array  */
	int	mx;			/* Padded array dimension  */
	int	my;			/* Ditto  */
};

EXTERN_MSC void GMT_bcr_init (struct GRD_HEADER *grd, int *pad, int bilinear, double threshold, struct GMT_BCR *bcr);
EXTERN_MSC double GMT_get_bcr_z (struct GRD_HEADER *grd, double xx, double yy, float *data,  struct GMT_EDGEINFO *edgeinfo, struct GMT_BCR *bcr);		/* Compute z(x,y) from bcr structure  */

/*----------------------------------------------------------------
		Here are some more remarks:

	Define x,y on the bcr so that they are in the range [0,1).  For pixel grids
with points close to edges, x,y will have to be in [-0.5,1) or [0, 1.5].  Now the
rectangle has 4 vertices, which we number thus:
	vertex 0:	x=0, y=0;
	vertex 1:	x=1, y=0;
	vertex 2:	x=0, y=1;
	vertex 3:	x=1, y=1.

i,j in struct BCR refers to the location of vertex 0.  The i,j will be referred to
the struct GRD header values, not to the location in the padded array, so that i=0,
j=0 is the point h.x_min, h.y_max.

The nodal values are specified at each vertex as nodal_value[vertex][value], where
value stands for the following:
	value 0:	z;
	value 1:	dz/dx;
	value 2:	dzdy;
	value 3:	d2zdxdy.

If we want a bilinear estimate of z(x,y), only nodal_value[vertex][0] is needed
and the estimate is obtained thus:
	z = 0.0;
	for (vertex = 0; vertex < 4; vertex++)
		z += bl_basis[vertex] * nodal_value[vertex][0];

This means that bl_basis[i] is the function that gives 1 at vertex i and 0 at
the other three vertices:
	bl_basis[0] = (1.0 - x)*(1.0 - y);
	bl_basis[1] = x*(1.0 - y);
	bl_basis[2] = y*(1.0 - x);
	bl_basis[3] = x*y;


	If we want a bicubic surface, then there are sixteen multiply-add operations,
looping over vertex and over value.  There are 16 basis functions, which are more
complicated.  See the table in Lancaster and Salkauskas or the source code for
GMT_get_bcr_cardinals().

	Given a point xx,yy in user's units, we need to be able to find i,j for
that point and x,y.  This will depend on struct GRD_HEADER information.

	If i,j for x,y is not equal to the last i,j we need to recompute the
nodal_values.  If the current i,j is within +/- 1 of the last i,j we can save
some old nodal_values.  If we discover a NaN during the computation of these,
we cannot continue.

	If nan_condition is false, we can evaluate the cardinals and do the
multiply-adds.  If x=0 or 1 or y = 0 or 1, there may be a trivial solution.

*/
#endif /* _GMT_BCR_H */
