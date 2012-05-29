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

#ifndef _GMT_PLOT_H
#define _GMT_PLOT_H

/* Identifier for GMT_plane_perspective. The others come from GMT_io.h */

#define GMT_ZW	3

/* GMT symbol identifiers. Mostly the same as PSL_<symbol> but with
   extensions for custom symbols, psxy and psxyz */

#define GMT_SYMBOL_STAR		((GMT_LONG)'a')
#define GMT_SYMBOL_BARX		((GMT_LONG)'B')
#define GMT_SYMBOL_BARY		((GMT_LONG)'b')
#define GMT_SYMBOL_CIRCLE	((GMT_LONG)'c')
#define GMT_SYMBOL_DIAMOND	((GMT_LONG)'d')
#define GMT_SYMBOL_ELLIPSE	((GMT_LONG)'e')
#define GMT_SYMBOL_FRONT	((GMT_LONG)'f')
#define GMT_SYMBOL_OCTAGON	((GMT_LONG)'g')
#define GMT_SYMBOL_HEXAGON	((GMT_LONG)'h')
#define GMT_SYMBOL_INVTRIANGLE	((GMT_LONG)'i')
#define GMT_SYMBOL_ROTRECT	((GMT_LONG)'j')
#define GMT_SYMBOL_CUSTOM	((GMT_LONG)'k')
#define GMT_SYMBOL_TEXT		((GMT_LONG)'l')
#define GMT_SYMBOL_MARC		((GMT_LONG)'m')
#define GMT_SYMBOL_PENTAGON	((GMT_LONG)'n')
#define GMT_SYMBOL_COLUMN	((GMT_LONG)'o')
#define GMT_SYMBOL_DOT		((GMT_LONG)'p')
#define GMT_SYMBOL_QUOTED_LINE	((GMT_LONG)'q')
#define GMT_SYMBOL_RECT		((GMT_LONG)'r')
#define GMT_SYMBOL_RNDRECT	((GMT_LONG)'R')
#define GMT_SYMBOL_SQUARE	((GMT_LONG)'s')
#define GMT_SYMBOL_TRIANGLE	((GMT_LONG)'t')
#define GMT_SYMBOL_CUBE		((GMT_LONG)'u')
#define GMT_SYMBOL_VECTOR	((GMT_LONG)'v')
#define GMT_SYMBOL_WEDGE	((GMT_LONG)'w')
#define GMT_SYMBOL_CROSS	((GMT_LONG)'x')
#define GMT_SYMBOL_YDASH	((GMT_LONG)'y')
#define GMT_SYMBOL_ZDASH	((GMT_LONG)'z')
#define GMT_SYMBOL_PLUS		((GMT_LONG)'+')
#define GMT_SYMBOL_XDASH	((GMT_LONG)'-')

#define GMT_SYMBOL_MOVE		((GMT_LONG)'M')
#define GMT_SYMBOL_DRAW		((GMT_LONG)'D')
#define GMT_SYMBOL_STROKE	((GMT_LONG)'S')
#define GMT_SYMBOL_ARC		((GMT_LONG)'A')
#define GMT_SYMBOL_ROTATE	((GMT_LONG)'R')
#define GMT_SYMBOL_VARROTATE	((GMT_LONG)'V')
#define GMT_SYMBOL_TEXTURE	((GMT_LONG)'T')
#define GMT_SYMBOL_GEOVECTOR	((GMT_LONG)'=')

#define GMT_SYMBOL_LINE		0
#define GMT_SYMBOL_NONE		((GMT_LONG)' ')
#define GMT_SYMBOL_NOT_SET	((GMT_LONG)'*')

#define GMT_DOT_SIZE 0.005	/* Size of a "dot" on a GMT PS map [in inches] */

/* FRONT symbols */

enum GMT_enum_front {GMT_FRONT_FAULT = 0,
	GMT_FRONT_TRIANGLE,
	GMT_FRONT_SLIP,
	GMT_FRONT_CIRCLE,
	GMT_FRONT_BOX};

/* Direction of FRONT symbols: */

enum GMT_enum_frontdir {GMT_FRONT_RIGHT = -1,
	GMT_FRONT_CENTERED,
	GMT_FRONT_LEFT};

struct GMT_FRONTLINE {		/* A sub-symbol for symbols along a front */
	double f_gap;		/* Gap between front symbols in inches */
	double f_len;		/* Length of front symbols in inches */
	double f_off;		/* Offset of first symbol from start of front in inches */
	GMT_LONG f_sense;	/* Draw symbols to left (+1), centered (0), or right (-1) of line */
	GMT_LONG f_symbol;	/* Which symbol to draw along the front line */
};

enum GMT_enum_vecattr {GMT_VEC_LEFT = 1,	/* Only draw left half of vector head */
	GMT_VEC_RIGHT		= 2,		/* Only draw right half of vector head */
	GMT_VEC_BEGIN		= 4,		/* Place vector head at beginning of vector */
	GMT_VEC_END		= 8,		/* Place vector head at end of vector */
	GMT_VEC_JUST_B		= 0,		/* Align vector beginning at (x,y) */
	GMT_VEC_JUST_C		= 16,		/* Align vector center at (x,y) */
	GMT_VEC_JUST_E		= 32,		/* Align vector end at (x,y) */
	GMT_VEC_JUST_S		= 64,		/* Align vector center at (x,y) */
	GMT_VEC_OUTLINE		= 128,		/* Draw vector head outline using default pen */
	GMT_VEC_OUTLINE2	= 256,		/* Draw vector head outline using supplied v_pen */
	GMT_VEC_FILL		= 512,		/* Fill vector head using default fill */
	GMT_VEC_FILL2		= 1024,		/* Fill vector head using supplied v_fill) */
	GMT_VEC_MARC90		= 2048};	/* Matharc only: if angles subtend 90, draw straight angle symbol */

#define GMT_vec_justify(status) ((status>>4)&3)			/* Return justification as 0-3 */
#define GMT_vec_head(status) ((status>>2)&3)			/* Return head selection as 0-3 */
#define GMT_vec_side(status) ((status&3) ? 2*(status&3)-3 : 0)	/* Return side selection as 0,-1,+1 */

struct GMT_VECT_ATTR {
	/* Container for common attributes for plot attributes of vectors */
	COUNTER_MEDIUM status;	/* Bit flags for vector information (see GMT_enum_vecattr above) */
	float v_angle;		/* Head angle */
	float v_norm;		/* shrink when lengths are smaller than this */
	float v_width;		/* Width of vector stem in inches */
	float h_length;		/* Length of vector head in inches */
	float h_width;		/* Width of vector head in inches */
	struct GMT_PEN pen;	/* Pen for outline of head [NOT USED YET] */
	struct GMT_FILL fill;	/* Fill for head [USED IN PSROSE] */
};

struct GMT_SYMBOL {
	/* Voodoo: If next line is not the first member in this struct, psxy -Sl<size>/Text will have corrupt 'Text'
		   in non-debug binaries compiled with VS2010 */
	char string[GMT_TEXT_LEN64];	/* Character code to plot (could be octal) */

	GMT_LONG symbol;	/* Symbol id */
	COUNTER_MEDIUM n_required;	/* Number of additional columns necessary to decode chosen symbol */
	COUNTER_MEDIUM u;		/* Measure unit id (0 = cm, 1 = inch, 2 = m, 3 = point */
	GMT_BOOLEAN u_set;		/* TRUE if u was set */
	double size_x;		/* Current symbol size in x */
	double size_y;		/* Current symbol size in y */
	double given_size_x;	/* Symbol size read from file or command line */
	double given_size_y;	/* Symbol size read from file or command line */
	GMT_BOOLEAN read_size;	/* TRUE when we must read symbol size from file */
	GMT_BOOLEAN shade3D;	/* TRUE when we should simulate shading of 3D symbols cube and column */
	struct GMT_FONT font;	/* Font to use for the -Sl symbol */
	COUNTER_MEDIUM convert_angles;	/* If 2, convert azimuth to angle on map, 1 special case for -JX, 0 plain case */
	COUNTER_MEDIUM n_nondim;	/* Number of columns that has angles or km (and not dimensions with units) */
	COUNTER_MEDIUM nondim_col[6];	/* Which columns has angles or km for this symbol */
	COUNTER_MEDIUM convert_size;	/* 1 if we must convert given "size" to actual size, 2 if log10 is to be applied first */
	double scale, origin;	/* Used to convert size = (given_size - origin) * scale */

	/* These apply to bar symbols */

	double base;		/* From what level to draw the bar */
	GMT_BOOLEAN user_unit;	/* if TRUE */
	GMT_BOOLEAN base_set;	/* TRUE if user provided a custom base [otherwise default to bottom axis */

	/* These apply to vectors */

	struct GMT_VECT_ATTR v;	/* All attributes for vector shapes etc. [see struct above] */

	struct GMT_FRONTLINE f;	/* parameters needed for a front */
	struct GMT_CUSTOM_SYMBOL *custom;	/* pointer to a custom symbol */

	struct GMT_CONTOUR G;	/* For labelled lines */
};

#endif /* _GMT_PLOT_H */
