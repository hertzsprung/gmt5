/*--------------------------------------------------------------------
 *	$Id: gmt_map.h,v 1.28 2009-03-24 02:26:43 guru Exp $
 *
 *	Copyright (c) 1991-2009 by P. Wessel and W. H. F. Smith
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

#ifndef _GMT_MAP_H
#define _GMT_MAP_H

EXTERN_MSC double GMT_az_backaz_cartesian (double lonE, double latE, double lonS, double latS, BOOLEAN baz);
EXTERN_MSC double GMT_az_backaz_flatearth (double lonE, double latE, double lonS, double latS, BOOLEAN baz);
EXTERN_MSC double GMT_az_backaz_geodesic (double lonE, double latE, double lonS, double latS, BOOLEAN baz);
EXTERN_MSC double GMT_az_backaz_sphere (double lonE, double latE, double lonS, double latS, BOOLEAN baz);
EXTERN_MSC double GMT_geodesic_dist_degree (double lonS, double latS, double lonE, double latE);
EXTERN_MSC double GMT_geodesic_dist_km (double lonS, double latS, double lonE, double latE);
EXTERN_MSC double GMT_geodesic_dist_meter (double lonS, double latS, double lonE, double latE);
EXTERN_MSC double GMT_geodesic_dist_cos (double lonS, double latS, double lonE, double latE);
EXTERN_MSC double GMT_great_circle_dist (double lon1, double lat1, double lon2, double lat2);
EXTERN_MSC double GMT_great_circle_dist_cos (double lon1, double lat1, double lon2, double lat2);
EXTERN_MSC double GMT_half_map_width (double y);
EXTERN_MSC double GMT_left_boundary (double y);
EXTERN_MSC double GMT_right_boundary (double y);
EXTERN_MSC int GMT_set_datum (char *text, struct GMT_DATUM *D);
EXTERN_MSC void GMT_ECEF_init (struct GMT_DATUM *D);
EXTERN_MSC GMT_LONG GMT_clip_to_map (double *lon, double *lat, GMT_LONG np, double **x, double **y);
EXTERN_MSC GMT_LONG GMT_compact_line (double *x, double *y, GMT_LONG n, BOOLEAN pen_flag, int *pen);
EXTERN_MSC void GMT_datum_init (struct GMT_DATUM *from, struct GMT_DATUM *to, BOOLEAN heights);
EXTERN_MSC int GMT_geo_to_xy_line (double *lon, double *lat, GMT_LONG n);
EXTERN_MSC int GMT_graticule_path (double **x, double **y, int dir, double w, double e, double s, double n);
EXTERN_MSC int GMT_grd_project (float *z_in, struct GRD_HEADER *I, float *z_out, struct GRD_HEADER *O, struct GMT_EDGEINFO *edgeinfo, BOOLEAN antialias, int interpolant, double threshold, BOOLEAN inverse);
EXTERN_MSC int GMT_great_circle_intersection (double A[], double B[], double C[], double X[], double *CX_dist);
EXTERN_MSC int GMT_latpath (double lat, double lon1, double lon2, double **x, double **y);
EXTERN_MSC int GMT_lonpath (double lon, double lat1, double lat2, double **x, double **y);
EXTERN_MSC int GMT_map_clip_path (double **x, double **y, BOOLEAN *donut);
EXTERN_MSC int GMT_genper_map_clip_path (GMT_LONG np, double *work_x, double *work_y);
EXTERN_MSC int GMT_map_outside (double lon, double lat);
EXTERN_MSC int GMT_map_path (double lon1, double lat1, double lon2, double lat2, double **x, double **y);
EXTERN_MSC int GMT_wesn_outside_np (double lon, double lat);
EXTERN_MSC void GMT_2D_to_3D (double *x, double *y, double z, GMT_LONG n);
EXTERN_MSC void GMT_2Dz_to_3D (double *x, double *y, double z, GMT_LONG n);
EXTERN_MSC void GMT_ECEF_forward (double in[], double out[]);
EXTERN_MSC void GMT_ECEF_inverse (double in[], double out[]);
EXTERN_MSC void GMT_azim_to_angle (double lon, double lat, double c, double azim, double *angle);
EXTERN_MSC void GMT_get_point_from_r_az (double lon0, double lat0, double r, double azim, double *lon1, double *lat1);
EXTERN_MSC void GMT_conv_datum (double in[], double out[]);
EXTERN_MSC BOOLEAN GMT_geo_to_xy (double lon, double lat, double *x, double *y);
EXTERN_MSC void GMT_geoz_to_xy (double x, double y, double z, double *x_out, double *y_out);
EXTERN_MSC int GMT_grdproject_init (struct GRD_HEADER *head, double x_inc, double y_inc, int nx, int ny, int dpi, int offset);
EXTERN_MSC void GMT_init_ellipsoid (void);
EXTERN_MSC int GMT_map_setup (double west, double east, double south, double north);
EXTERN_MSC void GMT_project3D (double x, double y, double z, double *x_out, double *y_out, double *z_out);
EXTERN_MSC void GMT_x_to_xx (double x, double *xx);
EXTERN_MSC void GMT_xx_to_x (double *x, double xx);
EXTERN_MSC void GMT_xy_do_z_to_xy (double x, double y, double z, double *x_out, double *y_out);
EXTERN_MSC void GMT_xy_to_geo (double *lon, double *lat, double x, double y);
EXTERN_MSC void GMT_xyz_to_xy (double x, double y, double z, double *x_out, double *y_out);
EXTERN_MSC void GMT_y_to_yy (double y, double *yy);
EXTERN_MSC void GMT_yy_to_y (double *y, double yy);
EXTERN_MSC void GMT_z_to_zz (double z, double *zz);
EXTERN_MSC void GMT_zz_to_z (double *z, double zz);
EXTERN_MSC int GMT_distances (double x[], double y[], GMT_LONG n, double scale, int dist_flag, double **dist);
EXTERN_MSC int GMT_map_loncross (double lon, double south, double north, struct GMT_XINGS **xings);
EXTERN_MSC int GMT_map_latcross (double lat, double west, double east, struct GMT_XINGS **xings);
EXTERN_MSC BOOLEAN GMT_set_greenwich (int mode);
EXTERN_MSC int GMT_UTMzone_to_wesn (int zone_x, int zone_y, int hemi, double *w, double *e, double *s, double *n);

#endif /* _GMT_MAP_H */
