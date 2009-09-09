/*--------------------------------------------------------------------
 *	$Id: pslib.h,v 1.57 2009-09-09 23:27:03 guru Exp $
 *
 *	Copyright (c) 1991-2009 by P. Wessel and W. H. F. Smith
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
 *
 * This include file must be included by all programs using pslib.a
 *
 * Author:	Paul Wessel
 * Version:	4.3 [64-bit enabled edition]
 * Date:	20-MAR-2008
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _PSLIB_INC_H	/* I.e., included by a user application */
#ifdef _WIN64
typedef __int64 PS_LONG;		/* A signed 8-byte integer */
#else
typedef long PS_LONG;			/* A signed 4 (or 8-byte for 64-bit) integer */
#endif

/* Declaration modifiers for DLL support (MSC et al) */

#if defined(DLL_PSL)		/* define when library is a DLL */
#if defined(DLL_EXPORT)		/* define when building the library */
#define MSC_EXTRA_PSL __declspec(dllexport)
#else
#define MSC_EXTRA_PSL __declspec(dllimport)
#endif
#else
#define MSC_EXTRA_PSL
#endif				/* defined(DLL_PSL) */

#ifndef EXTERN_MSC
#define EXTERN_MSC extern MSC_EXTRA_PSL
#endif

/* So unless DLL_PSL is defined, EXTERN_MSC is simply extern */

#endif	/* _PSLIB_INC_H */

/* Macro for exit since this should be returned when called from Matlab */
#ifdef DO_NOT_EXIT
#define PS_exit(code) return(code)
#else
#define PS_exit(code) exit(code)
#endif

#define PSL_PEN_MOVE		3
#define PSL_PEN_DRAW		+2
#define PSL_PEN_DRAW_AND_STROKE	-2
#define PSL_ARC_BEGIN		1
#define PSL_ARC_END		2
#define PSL_ARC_DRAW		3

#define PSL_MAX_EPS_FONTS	6

struct EPS {    /* Holds info for eps files */
/* For Encapsulated PostScript Headers:

	You will need to supply a pointer to an EPS structure in order to
	get correct information in the EPS header.  If you pass a NULL pointer
	instead you will get default values for the BoundingBox plus no
	info is provided about the users name, document title, and fonts used.
	To fill in the structure you must:

	- Determine the extreme dimensions of your plot in points (1/72 inch).
	- Supply the user's name (or NULL)
	- Supply the document's title (or NULL)
	- Set the font values to the ids of the fonts used  First unused font must be set
 	  to -1.  E.g., if 4 fonts are used, font[0], font[1], font[2], and
	  font[3] must contain the integer ID of these fonts; font[4] = -1
*/
	double x0, x1, y0, y1;		/* Bounding box values in points */
	int portrait;			/* TRUE if start of plot was portrait */
	int clip_level;			/* Add/sub 1 as we clip/unclip - should end at 0 */
	int fontno[PSL_MAX_EPS_FONTS];	/* Array with font ids used (skip if -1). 6 is max fonts used in GMT anot/labels */
	char *name;			/* User name */
	char *title;			/* Plot title */
};

struct imageinfo {
	int magic;		/* magic number */
	int width;		/* width (pixels) of image */
	int height;		/* height (pixels) of image */
	int depth;		/* depth (1, 8, or 24 bits) of pixel; 0 for EPS */
	int length;		/* length (bytes) of image */
	int type;		/* type of file; see RT_* below */
	int maptype;	/* type of colormap; see RMT_* below */
	int maplength;	/* length (bytes) of following map */
	int xorigin;	/* x-coordinate of origin (EPS only) */
	int yorigin;	/* y-coordinate of origin (EPS only) */
	/* color map follows for maplength bytes, followed by image */
};

#define	RAS_MAGIC	0x59a66a95	/* Magic number for Sun rasterfile */
#define EPS_MAGIC	0x25215053	/* Magic number for EPS file */
#define RT_OLD		0		/* Old-style, unencoded Sun rasterfile */
#define RT_STANDARD	1		/* Standard, unencoded Sun rasterfile */
#define RT_BYTE_ENCODED	2		/* Run-length-encoded Sun rasterfile */
#define RT_FORMAT_RGB	3		/* [X]RGB instead of [X]BGR Sun rasterfile */
#define RMT_NONE	0		/* maplength is expected to be 0 */
#define RMT_EQUAL_RGB	1		/* red[maplength/3], green[], blue[] follow */

/* Public functions */

EXTERN_MSC void ps_arc (double x, double y, double radius, double az1, double az2, PS_LONG status);
EXTERN_MSC void ps_axis (double x, double y, double length, double val0, double val1, double annotation_int, char *label, double annotpointsize, PS_LONG side);
EXTERN_MSC void ps_bitimage (double x, double y, double xsize, double ysize, unsigned char *buffer, PS_LONG nx, PS_LONG ny, PS_LONG invert, int f_rgb[], int b_rgb[]);
EXTERN_MSC void ps_circle (double x, double y, double size, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_clipoff (void);
EXTERN_MSC void ps_clipon (double *x, double *y, PS_LONG n, int rgb[], PS_LONG flag);
EXTERN_MSC void ps_colorimage (double x, double y, double xsize, double ysize, unsigned char *buffer, PS_LONG nx, PS_LONG ny, PS_LONG nbits);
EXTERN_MSC void ps_colortiles (double x0, double y0, double xsize, double ysize, unsigned char *image, PS_LONG nx, PS_LONG ny);
EXTERN_MSC void ps_command (char *text);
EXTERN_MSC void ps_comment (char *text);
EXTERN_MSC void ps_cross (double x, double y, double size);
EXTERN_MSC void ps_encode_font (PS_LONG font_no);
EXTERN_MSC void ps_dash (double x0, double y0, double x1, double y1);
EXTERN_MSC void ps_diamond (double x, double y, double side, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_ellipse (double x, double y, double angle, double major, double minor, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_epsimage (double x, double y, double xsize, double ysize, unsigned char *buffer, PS_LONG size, PS_LONG nx, PS_LONG ny, PS_LONG ox, PS_LONG oy); 
EXTERN_MSC void ps_flush (void);
EXTERN_MSC void *ps_memory (void *prev_addr, PS_LONG nelem, size_t size);
EXTERN_MSC void ps_free (void *addr);
EXTERN_MSC void ps_hexagon (double x, double y, double side, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_image (double x, double y, double xsize, double ysize, unsigned char *buffer, PS_LONG nx, PS_LONG ny, PS_LONG nbits);
EXTERN_MSC PS_LONG ps_line (double *x, double *y, PS_LONG n, PS_LONG type, PS_LONG close);
EXTERN_MSC void ps_itriangle (double x, double y, double side, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_octagon (double x, double y, double side, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_pentagon (double x, double y, double side, int rgb[], PS_LONG outline);
EXTERN_MSC PS_LONG ps_pattern (PS_LONG image_no, char *imagefile, PS_LONG invert, PS_LONG image_dpi, PS_LONG outline, int f_rgb[], int b_rgb[]);
EXTERN_MSC void ps_pie (double x, double y, double radius, double az1, double az2, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_plot (double x, double y, int pen);
EXTERN_MSC PS_LONG ps_plotinit (char *plotfile, PS_LONG overlay, PS_LONG mode, double xoff, double yoff, double xscl, double yscl, PS_LONG ncopies, PS_LONG dpi, PS_LONG unit, PS_LONG *page_size, int *rgb, const char *encoding, struct EPS *eps);
EXTERN_MSC PS_LONG ps_plotinit_hires (char *plotfile, PS_LONG overlay, PS_LONG mode, double xoff, double yoff, double xscl, double yscl, PS_LONG ncopies, PS_LONG dpi, PS_LONG unit, double *page_size, int *rgb, const char *encoding, struct EPS *eps);
EXTERN_MSC void ps_plotend (PS_LONG lastpage);
EXTERN_MSC void ps_plotr (double x, double y, int pen);
EXTERN_MSC void ps_plus (double x, double y, double size);
EXTERN_MSC void ps_point (double x, double y, double diameter);
EXTERN_MSC void ps_polygon (double *x, double *y, PS_LONG n, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_rect (double x1, double y1, double x2, double y2, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_rotaterect (double x, double y, double angle, double x_len, double y_len, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_patch (double *x, double *y, PS_LONG np, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_rotatetrans (double x, double y, double angle);
EXTERN_MSC void ps_segment (double x0, double y0, double x1, double y1);
EXTERN_MSC void ps_setdash (char *pattern, PS_LONG offset);
EXTERN_MSC void ps_setfill (int rgb[], PS_LONG outline);
EXTERN_MSC void ps_setfont (PS_LONG font_no);
EXTERN_MSC void ps_setformat (PS_LONG n_decimals);
EXTERN_MSC void ps_setline (PS_LONG linewidth);
EXTERN_MSC void ps_setlinecap (PS_LONG cap);
EXTERN_MSC void ps_setlinejoin (PS_LONG join);
EXTERN_MSC void ps_setmiterlimit (PS_LONG limit);
EXTERN_MSC void ps_setpaint (int rgb[]);
EXTERN_MSC void ps_square (double x, double y, double side, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_star (double x, double y, double side, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_text (double x, double y, double pointsize, char *text, double angle, PS_LONG justify, PS_LONG form);
EXTERN_MSC void ps_textbox (double x, double y, double pointsize, char *text, double angle, PS_LONG justify, PS_LONG outline, double dx, double dy, int rgb[]);
EXTERN_MSC void ps_textpath (double x[], double y[], PS_LONG n, PS_LONG node[], double angle[], char *label[], PS_LONG m, double pointsize, double offset[], PS_LONG justify, PS_LONG form);
EXTERN_MSC void ps_textclip (double x[], double y[], PS_LONG m, double angle[], char *label[], double pointsize, double offset[], PS_LONG justify, PS_LONG key);
EXTERN_MSC void ps_transrotate (double x, double y, double angle);
EXTERN_MSC void ps_triangle (double x, double y, double side, int rgb[], PS_LONG outline);
EXTERN_MSC void ps_vector (double xtail, double ytail, double xtip, double ytip, double tailwidth, double headlength, double headwidth, double headshape, int rgb[], PS_LONG outline);
EXTERN_MSC unsigned char *ps_load_image (char *file, struct imageinfo *header);
EXTERN_MSC void ps_words (double x, double y, char **text, PS_LONG n_words, double line_space, double par_width, PS_LONG par_just, PS_LONG font, double font_size, double angle, int rgb[3], PS_LONG justify, PS_LONG draw_box, double x_off, double y_off, double x_gap, double y_gap, PS_LONG boxpen_width, char *boxpen_texture, PS_LONG boxpen_offset, int boxpen_rgb[], PS_LONG vecpen_width, char *vecpen_texture, PS_LONG vecpen_offset, int vecpen_rgb[], int boxfill_rgb[3]);
EXTERN_MSC void ps_setline (PS_LONG linewidth);
EXTERN_MSC void ps_textdim (char *dim, double pointsize, PS_LONG font, char *text);
EXTERN_MSC void ps_set_length (char *param, double value);
EXTERN_MSC void ps_set_height (char *param, double fontsize);
EXTERN_MSC void ps_define_rgb (char *param, int rgb[]);
EXTERN_MSC void ps_define_pen (char *param, PS_LONG width, char *texture, PS_LONG offset, int rgb[]);
EXTERN_MSC void ps_rgb_to_mono (unsigned char *buffer, struct imageinfo *h);
EXTERN_MSC PS_LONG ps_read_rasheader  (FILE *fp, struct imageinfo *h, PS_LONG i0, PS_LONG i1);
EXTERN_MSC PS_LONG ps_write_rasheader (FILE *fp, struct imageinfo *h, PS_LONG i0, PS_LONG i1);

#ifdef __cplusplus
}
#endif
