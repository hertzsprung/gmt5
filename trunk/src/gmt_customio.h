/*--------------------------------------------------------------------
 *	$Id: gmt_customio.h,v 1.25 2008-01-23 03:22:48 guru Exp $
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
 
/*
 * Include file for gmt_customio functions.
 *
 * Author:	Paul Wessel
 * Date:	06-DEC-2001
 * Version:	4
 *
 */

#ifndef GMT_CUSTOMIO_H
#define GMT_CUSTOMIO_H

/* List groups of 5 integer functions for each custom i/o grd format */

/* Format # 0 (default) and # 7-11 */
EXTERN_MSC int GMT_cdf_read_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_cdf_update_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_cdf_write_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_cdf_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);
EXTERN_MSC int GMT_cdf_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);

/* Format # 1 */
EXTERN_MSC int GMT_native_read_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_native_update_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_native_write_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_native_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);
EXTERN_MSC int GMT_native_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);

/* Format # 3 */
EXTERN_MSC int GMT_ras_read_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_ras_update_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_ras_write_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_ras_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);
EXTERN_MSC int GMT_ras_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);

/* Format # 5 */
EXTERN_MSC int GMT_bit_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);
EXTERN_MSC int GMT_bit_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);

/* Format # 6+20 */
EXTERN_MSC int GMT_srf_read_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_srf_update_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_srf_write_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_srf_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);
EXTERN_MSC int GMT_srf_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);

/* Format # 12 */
#include "gmt_mgg_header2.h"

/* Format # 15-19 */
EXTERN_MSC int GMT_nc_read_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_nc_update_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_nc_write_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_nc_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);
EXTERN_MSC int GMT_nc_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);

/* Format # 21 */
EXTERN_MSC int GMT_agc_read_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_agc_update_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_agc_write_grd_info (struct GRD_HEADER *header);
EXTERN_MSC int GMT_agc_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);
EXTERN_MSC int GMT_agc_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, int *pad, BOOLEAN complex);

#define GRD_HEADER_SIZE	892

EXTERN_MSC float GMT_decode (void *vptr, int k, int type);
EXTERN_MSC void GMT_encode (void *vptr, int k, float z, int type);

/* Definition for Sun rasterfiles */
struct rasterfile {
        int magic;		/* magic number */
        int width;		/* width (pixels) of image */
        int height;		/* height (pixels) of image */
        int depth;		/* depth (1, 8, or 24 bits) of pixel */
        int length;		/* length (bytes) of image */
        int type;		/* type of file; see RT_* below */
        int maptype;		/* type of colormap; see RMT_* below */
        int maplength;		/* length (bytes) of following map */
        /* color map follows for maplength bytes, followed by image */
};

#define	RAS_MAGIC	0x59a66a95	/* Magic number for Sun rasterfile */
#define EPS_MAGIC	0x25215053	/* Magic number for EPS file */
#define RT_OLD		0	/* Old-style, unencoded Sun rasterfile */
#define RT_STANDARD	1	/* Standard, unencoded Sun rasterfile */
#define RT_BYTE_ENCODED	2	/* Run-length-encoded Sun rasterfile */
#define RT_FORMAT_RGB	3	/* [X]RGB instead of [X]BGR Sun rasterfile */
#define RMT_NONE	0	/* maplength is expected to be 0 */
#define RMT_EQUAL_RGB	1	/* red[maplength/3], green[], blue[] follow */

#endif /* GMT_CUSTOMIO_H */
