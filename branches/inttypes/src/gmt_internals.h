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
/* gmt_internals.h  --  All lower-level functions needed within library.

   Authors:	P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
   Date:	1-OCT-2009
   Version:	5 API

*/

#ifndef _GMT_INTERNALS_H
#define _GMT_INTERNALS_H

enum GMT_enum_cplx {GMT_RE = 0, GMT_IM = 1};	/* Real and imaginary indices */

EXTERN_MSC COUNTER_MEDIUM GMT_unit_lookup (struct GMT_CTRL *C, GMT_LONG c, COUNTER_MEDIUM unit);
EXTERN_MSC void GMT_get_annot_label (struct GMT_CTRL *C, double val, char *label, BOOLEAN do_minutes, BOOLEAN do_seconds, BOOLEAN lonlat, BOOLEAN worldmap);
EXTERN_MSC COUNTER_MEDIUM GMT_coordinate_array (struct GMT_CTRL *C, double min, double max, struct GMT_PLOT_AXIS_ITEM *T, double **array, char ***labels);
EXTERN_MSC COUNTER_MEDIUM GMT_linear_array (struct GMT_CTRL *C, double min, double max, double delta, double phase, double **array);
EXTERN_MSC COUNTER_MEDIUM GMT_pow_array (struct GMT_CTRL *C, double min, double max, double delta, COUNTER_MEDIUM x_or_y, double **array);
EXTERN_MSC GMT_LONG GMT_prepare_label (struct GMT_CTRL *C, double angle, COUNTER_MEDIUM side, double x, double y, COUNTER_MEDIUM type, double *line_angle, double *text_angle, COUNTER_MEDIUM *justify);
EXTERN_MSC COUNTER_MEDIUM GMT_time_array (struct GMT_CTRL *C, double min, double max, struct GMT_PLOT_AXIS_ITEM *T, double **array);
EXTERN_MSC void GMT_get_lon_minmax (struct GMT_CTRL *C, double *lon, COUNTER_LARGE n, double *min, double *max);
EXTERN_MSC struct GMT_OGR * GMT_duplicate_ogr (struct GMT_CTRL *C, struct GMT_OGR *G);
EXTERN_MSC void GMT_free_ogr (struct GMT_CTRL *C, struct GMT_OGR **G, COUNTER_MEDIUM mode);
EXTERN_MSC GMT_LONG gmt_ogr_get_geometry (char *item);
EXTERN_MSC GMT_LONG gmt_ogr_get_type (char *item);
EXTERN_MSC void gmt_plot_C_format (struct GMT_CTRL *C);
EXTERN_MSC void gmt_clock_C_format (struct GMT_CTRL *C, char *form, struct GMT_CLOCK_IO *S, COUNTER_MEDIUM mode);
EXTERN_MSC void gmt_date_C_format (struct GMT_CTRL *C, char *form, struct GMT_DATE_IO *S, COUNTER_MEDIUM mode);
EXTERN_MSC char * GMT_ascii_textinput (struct GMT_CTRL *C, FILE *fp, COUNTER_MEDIUM *ncol, GMT_LONG *status);
EXTERN_MSC double GMT_get_map_interval (struct GMT_CTRL *C, struct GMT_PLOT_AXIS_ITEM *T);
EXTERN_MSC COUNTER_MEDIUM GMT_log_array (struct GMT_CTRL *C, double min, double max, double delta, double **array);
EXTERN_MSC GMT_LONG GMT_nc_get_att_text (struct GMT_CTRL *C, int ncid, int varid, char *name, char *text, size_t textlen);
EXTERN_MSC GMT_LONG GMT_akima (struct GMT_CTRL *C, double *x, double *y, COUNTER_LARGE nx, double *c);
EXTERN_MSC GMT_LONG GMT_cspline (struct GMT_CTRL *C, double *x, double *y, COUNTER_LARGE n, double *c);
EXTERN_MSC BOOLEAN GMT_annot_pos (struct GMT_CTRL *C, double min, double max, struct GMT_PLOT_AXIS_ITEM *T, double coord[], double *pos);
EXTERN_MSC int GMT_comp_int_asc (const void *p_1, const void *p_2);
EXTERN_MSC float GMT_decode (struct GMT_CTRL *C, void *vptr, COUNTER_LARGE k, COUNTER_MEDIUM type);
EXTERN_MSC void GMT_encode (struct GMT_CTRL *C, void *vptr, COUNTER_LARGE k, float z, COUNTER_MEDIUM type);
EXTERN_MSC GMT_LONG GMT_flip_justify (struct GMT_CTRL *C, COUNTER_MEDIUM justify);
EXTERN_MSC struct GMT_CUSTOM_SYMBOL * GMT_get_custom_symbol (struct GMT_CTRL *C, char *name);
EXTERN_MSC void GMT_free_custom_symbols (struct GMT_CTRL *C);
EXTERN_MSC BOOLEAN GMT_geo_to_dms (double val, GMT_LONG n_items, double fact, GMT_LONG *d, GMT_LONG *m,  GMT_LONG *s,  GMT_LONG *ix);
EXTERN_MSC double GMT_get_annot_offset (struct GMT_CTRL *C, BOOLEAN *flip, COUNTER_MEDIUM level);
EXTERN_MSC GMT_LONG GMT_get_coordinate_label (struct GMT_CTRL *C, char *string, struct GMT_PLOT_CALCLOCK *P, char *format, struct GMT_PLOT_AXIS_ITEM *T, double coord);
EXTERN_MSC void GMT_get_time_label (struct GMT_CTRL *C, char *string, struct GMT_PLOT_CALCLOCK *P, struct GMT_PLOT_AXIS_ITEM *T, double t);
EXTERN_MSC GMT_LONG GMT_getrgb_index (struct GMT_CTRL *C, double *rgb);
EXTERN_MSC char * GMT_getuserpath (struct GMT_CTRL *C, const char *stem, char *path);	/* Look for user file */
EXTERN_MSC size_t GMT_grd_data_size (struct GMT_CTRL *C, COUNTER_MEDIUM format, double *nan_value);
EXTERN_MSC void GMT_init_ellipsoid (struct GMT_CTRL *C);
EXTERN_MSC void GMT_io_init (struct GMT_CTRL *C);			/* Initialize pointers */
EXTERN_MSC COUNTER_LARGE GMT_latpath (struct GMT_CTRL *C, double lat, double lon1, double lon2, double **x, double **y);
EXTERN_MSC COUNTER_LARGE GMT_lonpath (struct GMT_CTRL *C, double lon, double lat1, double lat2, double **x, double **y);
EXTERN_MSC COUNTER_LARGE GMT_map_path (struct GMT_CTRL *C, double lon1, double lat1, double lon2, double lat2, double **x, double **y);
EXTERN_MSC double GMT_left_boundary (struct GMT_CTRL *C, double y);
EXTERN_MSC double GMT_right_boundary (struct GMT_CTRL *C, double y);
EXTERN_MSC COUNTER_MEDIUM GMT_map_latcross (struct GMT_CTRL *C, double lat, double west, double east, struct GMT_XINGS **xings);
EXTERN_MSC COUNTER_MEDIUM GMT_map_loncross (struct GMT_CTRL *C, double lon, double south, double north, struct GMT_XINGS **xings);
EXTERN_MSC void GMT_rotate2D (struct GMT_CTRL *C, double x[], double y[], COUNTER_LARGE n, double x0, double y0, double angle, double xp[], double yp[]);
EXTERN_MSC void GMT_set_bin_input (struct GMT_CTRL *C);
EXTERN_MSC COUNTER_LARGE * GMT_split_line (struct GMT_CTRL *C, double **xx, double **yy, COUNTER_LARGE *nn, BOOLEAN add_crossings);
EXTERN_MSC GMT_LONG GMT_verify_time_step (struct GMT_CTRL *C, GMT_LONG step, char unit);	/* Check that time step and unit for time axis are OK  */
EXTERN_MSC double GMT_xx_to_x (struct GMT_CTRL *C, double xx);
EXTERN_MSC double GMT_yy_to_y (struct GMT_CTRL *C, double yy);
EXTERN_MSC double GMT_zz_to_z (struct GMT_CTRL *C, double zz);
EXTERN_MSC GMT_LONG GMT_y2_to_y4_yearfix (struct GMT_CTRL *C, COUNTER_MEDIUM y2);	/* Convert a 2-digit year to a 4-digit year */
EXTERN_MSC GMT_LONG GMT_g_ymd_is_bad (GMT_LONG y, GMT_LONG m, GMT_LONG d);	/* Check range of month and day for Gregorian YMD calendar values  */
EXTERN_MSC GMT_LONG GMT_iso_ywd_is_bad (GMT_LONG y, GMT_LONG w, GMT_LONG d);	/* Check range of week and day for ISO W calendar.  */
EXTERN_MSC GMT_LONG GMT_genper_map_clip_path (struct GMT_CTRL *C, COUNTER_LARGE np, double *work_x, double *work_y);
EXTERN_MSC double GMT_half_map_width (struct GMT_CTRL *C, double y);
EXTERN_MSC void GMT_moment_interval (struct GMT_CTRL *C, struct GMT_MOMENT_INTERVAL *p, double dt_in, BOOLEAN init); /* step a time axis by time units */
EXTERN_MSC int64_t GMT_rd_from_iywd (struct GMT_CTRL *C, GMT_LONG iy, GMT_LONG iw, GMT_LONG id);
EXTERN_MSC GMT_LONG GMT_grd_format_decoder (struct GMT_CTRL *C, const char *code);
EXTERN_MSC GMT_LONG GMT_grd_prep_io (struct GMT_CTRL *C, struct GRD_HEADER *header, double wesn[], COUNTER_MEDIUM *width, COUNTER_MEDIUM *height, COUNTER_MEDIUM *first_col, COUNTER_MEDIUM *last_col, COUNTER_MEDIUM *first_row, COUNTER_MEDIUM *last_row, COUNTER_MEDIUM **index);
EXTERN_MSC void GMT_scale_eqrad (struct GMT_CTRL *C);
EXTERN_MSC void GMT_enforce_rgb_triplets (struct GMT_CTRL *C, char *text, COUNTER_MEDIUM size);
GMT_LONG GMT_get_fill_from_z (struct GMT_CTRL *C, struct GMT_PALETTE *P, double value, struct GMT_FILL *fill);
GMT_LONG GMT_update_grd_info (struct GMT_CTRL *C, char *file, struct GRD_HEADER *header);
EXTERN_MSC struct GMT_TEXTSET * GMT_create_textset (struct GMT_CTRL *C, COUNTER_MEDIUM n_tables, int64_t n_segments, COUNTER_LARGE n_rows);
EXTERN_MSC struct GMT_PALETTE * GMT_create_palette (struct GMT_CTRL *C, COUNTER_MEDIUM n_colors);
EXTERN_MSC struct GMT_TEXT_TABLE * GMT_read_texttable (struct GMT_CTRL *C, void *source, COUNTER_MEDIUM source_type);
EXTERN_MSC GMT_LONG GMT_write_textset (struct GMT_CTRL *C, void *dest, COUNTER_MEDIUM dest_type, struct GMT_TEXTSET *D, GMT_LONG table);
EXTERN_MSC struct GMT_TEXTSET * GMT_alloc_textset (struct GMT_CTRL *C, struct GMT_TEXTSET *Din, COUNTER_MEDIUM mode);
EXTERN_MSC GMT_LONG GMT_init_complex (GMT_LONG complex, COUNTER_MEDIUM *inc, COUNTER_MEDIUM *off);
EXTERN_MSC struct GMT_MATRIX * GMT_duplicate_matrix (struct GMT_CTRL *C, struct GMT_MATRIX *M_in, BOOLEAN duplicate_data);
EXTERN_MSC struct GMT_VECTOR * GMT_duplicate_vector (struct GMT_CTRL *C, struct GMT_VECTOR *V_in, BOOLEAN duplicate_data);
EXTERN_MSC void gmt_init_rot_matrix (double R[3][3], double E[]);
EXTERN_MSC void gmt_load_rot_matrix (double w, double R[3][3], double E[]);
EXTERN_MSC void gmt_matrix_vect_mult (double a[3][3], double b[3], double c[3]);
EXTERN_MSC void gmt_geo_polygon (struct GMT_CTRL *C, double *lon, double *lat, COUNTER_LARGE n);

EXTERN_MSC GMT_LONG GMT_gmonth_length (GMT_LONG year, GMT_LONG month);
EXTERN_MSC void GMT_gcal_from_dt (struct GMT_CTRL *C, double t, struct GMT_gcal *cal);	/* Break internal time into calendar and clock struct info  */
EXTERN_MSC GMT_LONG GMT_great_circle_intersection (struct GMT_CTRL *T, double A[], double B[], double C[], double X[], double *CX_dist);
EXTERN_MSC double GMT_great_circle_dist_degree (struct GMT_CTRL *C, double lon1, double lat1, double lon2, double lat2);
EXTERN_MSC void GMT_get_point_from_r_az (struct GMT_CTRL *C, double lon0, double lat0, double r, double azim, double *lon1, double *lat1);
EXTERN_MSC int gmt_parse_b_option (struct GMT_CTRL *C, char *text);
EXTERN_MSC COUNTER_LARGE GMT_fix_up_path_cartesian (struct GMT_CTRL *C, double **a_x, double **a_y, COUNTER_LARGE n, double step, COUNTER_MEDIUM mode);
EXTERN_MSC BOOLEAN GMT_check_url_name (char *fname);
EXTERN_MSC BOOLEAN GMT_is_a_blank_line (char *line);	/* Checks if line is a blank line or comment */
EXTERN_MSC int64_t GMT_splitinteger (double value, GMT_LONG epsilon, double *doublepart);
EXTERN_MSC BOOLEAN GMT_is_gleap (GMT_LONG gyear);
EXTERN_MSC void GMT_str_tolower (char *string);
EXTERN_MSC void GMT_str_toupper (char *string);

/* Functions declared in gmt_proj.c */

EXTERN_MSC void GMT_vpolar (struct GMT_CTRL *C, double lon0);
EXTERN_MSC void GMT_vmerc (struct GMT_CTRL *C, double lon0, double slat);
EXTERN_MSC void GMT_vcyleq (struct GMT_CTRL *C, double lon0, double slat);
EXTERN_MSC void GMT_vcyleqdist (struct GMT_CTRL *C, double lon0, double slat);
EXTERN_MSC void GMT_vcylstereo (struct GMT_CTRL *C, double lon0, double slat);
EXTERN_MSC void GMT_vmiller (struct GMT_CTRL *C, double lon0);
EXTERN_MSC void GMT_vstereo (struct GMT_CTRL *C, double lon0, double lat0, double horizon);
EXTERN_MSC void GMT_vlamb (struct GMT_CTRL *C, double lon0, double lat0, double pha, double phb);
EXTERN_MSC void GMT_vtm (struct GMT_CTRL *C, double lon0, double lat0);
EXTERN_MSC void GMT_vlambeq (struct GMT_CTRL *C, double lon0, double lat0, double horizon);
EXTERN_MSC void GMT_vortho (struct GMT_CTRL *C, double lon0, double lat0, double horizon);
EXTERN_MSC void GMT_vgenper (struct GMT_CTRL *C, double lon0, double lat0, double altitude, double azimuth, double tilt, double rotation, double width, double height);
EXTERN_MSC void GMT_vgnomonic (struct GMT_CTRL *C, double lon0, double lat0, double horizon);
EXTERN_MSC void GMT_vazeqdist (struct GMT_CTRL *C, double lon0, double lat0, double horizon);
EXTERN_MSC void GMT_vmollweide (struct GMT_CTRL *C, double lon0, double scale);
EXTERN_MSC void GMT_vhammer (struct GMT_CTRL *C, double lon0, double scale);
EXTERN_MSC void GMT_vwinkel (struct GMT_CTRL *C, double lon0, double scale);
EXTERN_MSC void GMT_veckert4 (struct GMT_CTRL *C, double lon0);
EXTERN_MSC void GMT_veckert6 (struct GMT_CTRL *C, double lon0);
EXTERN_MSC void GMT_vrobinson (struct GMT_CTRL *C, double lon0);
EXTERN_MSC void GMT_vsinusoidal (struct GMT_CTRL *C, double lon0);
EXTERN_MSC void GMT_vcassini (struct GMT_CTRL *C, double lon0, double lat0);
EXTERN_MSC void GMT_valbers (struct GMT_CTRL *C, double lon0, double lat0, double ph1, double ph2);
EXTERN_MSC void GMT_valbers_sph (struct GMT_CTRL *C, double lon0, double lat0, double ph1, double ph2);
EXTERN_MSC void GMT_veconic (struct GMT_CTRL *C, double lon0, double lat0, double ph1, double ph2);
EXTERN_MSC void GMT_vpolyconic (struct GMT_CTRL *C, double lon0, double lat0);
EXTERN_MSC void GMT_vgrinten (struct GMT_CTRL *C, double lon0, double scale);
EXTERN_MSC void GMT_polar (struct GMT_CTRL *C, double x, double y, double *x_i, double *y_i);		/* Convert x/y (being theta,r) to x,y	*/
EXTERN_MSC void GMT_ipolar (struct GMT_CTRL *C, double *x, double *y, double x_i, double y_i);		/* Convert (theta,r) to x,y	*/
EXTERN_MSC void GMT_translin (struct GMT_CTRL *C, double forw, double *inv);				/* Forward linear	*/
EXTERN_MSC void GMT_translind (struct GMT_CTRL *C, double forw, double *inv);				/* Forward linear, but using 0-360 degrees	*/
EXTERN_MSC void GMT_itranslin (struct GMT_CTRL *C, double *forw, double inv);				/* Inverse linear	*/
EXTERN_MSC void GMT_itranslind (struct GMT_CTRL *C, double *forw, double inv);				/* Inverse linear, but using 0-360 degrees	*/
EXTERN_MSC void GMT_translog10 (struct GMT_CTRL *C, double forw, double *inv);				/* Forward log10	*/
EXTERN_MSC void GMT_itranslog10 (struct GMT_CTRL *C, double *forw, double inv);				/* Inverse log10	*/
EXTERN_MSC void GMT_transpowx (struct GMT_CTRL *C, double x, double *x_in);				/* Forward pow x	*/
EXTERN_MSC void GMT_itranspowx (struct GMT_CTRL *C, double *x, double x_in);				/* Inverse pow x	*/
EXTERN_MSC void GMT_transpowy (struct GMT_CTRL *C, double y, double *y_in);				/* Forward pow y 	*/
EXTERN_MSC void GMT_itranspowy (struct GMT_CTRL *C, double *y, double y_in);				/* Inverse pow y 	*/
EXTERN_MSC void GMT_transpowz (struct GMT_CTRL *C, double z, double *z_in);				/* Forward pow z 	*/
EXTERN_MSC void GMT_itranspowz (struct GMT_CTRL *C, double *z, double z_in);				/* Inverse pow z 	*/
EXTERN_MSC void GMT_albers (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Albers)	*/
EXTERN_MSC void GMT_ialbers (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Albers) to lon/lat	*/
EXTERN_MSC void GMT_econic (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Equidistant Conic)	*/
EXTERN_MSC void GMT_ieconic (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Equidistant Conic) to lon/lat	*/
EXTERN_MSC void GMT_polyconic (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Polyconic)	*/
EXTERN_MSC void GMT_ipolyconic (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Polyconic) to lon/lat	*/
EXTERN_MSC void GMT_albers_sph (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Albers Spherical)	*/
EXTERN_MSC void GMT_ialbers_sph (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Albers Spherical) to lon/lat	*/
EXTERN_MSC void GMT_azeqdist (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Azimuthal equal-distance)*/
EXTERN_MSC void GMT_iazeqdist (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Azimuthal equal-distance) to lon/lat*/
EXTERN_MSC void GMT_cassini (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Cassini)	*/
EXTERN_MSC void GMT_icassini (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Cassini) to lon/lat	*/
EXTERN_MSC void GMT_cassini_sph (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Cassini Spherical)	*/
EXTERN_MSC void GMT_icassini_sph (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Cassini Spherical) to lon/lat	*/
EXTERN_MSC void GMT_hammer (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Hammer-Aitoff)	*/
EXTERN_MSC void GMT_ihammer (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Hammer-Aitoff) to lon/lat	*/
EXTERN_MSC void GMT_grinten (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (van der Grinten)	*/
EXTERN_MSC void GMT_igrinten (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (van der Grinten) to lon/lat	*/
EXTERN_MSC void GMT_merc_sph (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Mercator Spherical)	*/
EXTERN_MSC void GMT_imerc_sph (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Mercator Spherical) to lon/lat	*/
EXTERN_MSC void GMT_plrs (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Polar)		*/
EXTERN_MSC void GMT_iplrs (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Polar) to lon/lat		*/
EXTERN_MSC void GMT_plrs_sph (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Polar Spherical)	*/
EXTERN_MSC void GMT_iplrs_sph (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Polar Spherical) to lon/lat	*/
EXTERN_MSC void GMT_lamb (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Lambert)	*/
EXTERN_MSC void GMT_ilamb (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Lambert) to lon/lat 	*/
EXTERN_MSC void GMT_lamb_sph (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Lambert Spherical)	*/
EXTERN_MSC void GMT_ilamb_sph (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Lambert Spherical) to lon/lat 	*/
EXTERN_MSC void GMT_oblmrc (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Oblique Mercator)	*/
EXTERN_MSC void GMT_ioblmrc (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Oblique Mercator) to lon/lat 	*/
EXTERN_MSC void GMT_genper (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (ORTHO)  */
EXTERN_MSC void GMT_igenper (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (ORTHO) to lon/lat  */
EXTERN_MSC void GMT_ortho (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (GMT_ORTHO)	*/
EXTERN_MSC void GMT_iortho (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (GMT_ORTHO) to lon/lat 	*/
EXTERN_MSC void GMT_gnomonic (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (GMT_GNOMONIC)	*/
EXTERN_MSC void GMT_ignomonic (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (GMT_GNOMONIC) to lon/lat 	*/
EXTERN_MSC void GMT_sinusoidal (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (GMT_SINUSOIDAL)	*/
EXTERN_MSC void GMT_isinusoidal (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (GMT_SINUSOIDAL) to lon/lat 	*/
EXTERN_MSC void GMT_tm (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (TM)	*/
EXTERN_MSC void GMT_itm (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (TM) to lon/lat 	*/
EXTERN_MSC void GMT_tm_sph (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (GMT_TM Spherical)	*/
EXTERN_MSC void GMT_itm_sph (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (GMT_TM Spherical) to lon/lat 	*/
EXTERN_MSC void GMT_utm (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (UTM)	*/
EXTERN_MSC void GMT_iutm (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (UTM) to lon/lat 	*/
EXTERN_MSC void GMT_utm_sph (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (UTM Spherical)	*/
EXTERN_MSC void GMT_iutm_sph (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (UTM Spherical) to lon/lat 	*/
EXTERN_MSC void GMT_winkel (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Winkel)	*/
EXTERN_MSC void GMT_iwinkel (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Winkel) to lon/lat	*/
EXTERN_MSC void GMT_eckert4 (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Eckert IV)	*/
EXTERN_MSC void GMT_ieckert4 (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Eckert IV) to lon/lat	*/
EXTERN_MSC void GMT_eckert6 (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Eckert VI)	*/
EXTERN_MSC void GMT_ieckert6 (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Eckert VI) to lon/lat	*/
EXTERN_MSC void GMT_robinson (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Robinson)	*/
EXTERN_MSC void GMT_irobinson (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Robinson) to lon/lat	*/
EXTERN_MSC void GMT_stereo1 (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Stereographic)	*/
EXTERN_MSC void GMT_stereo2 (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Stereographic, equatorial view)*/
EXTERN_MSC void GMT_istereo (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Stereographic) to lon/lat 	*/
EXTERN_MSC void GMT_stereo1_sph (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Stereographic Spherical)*/
EXTERN_MSC void GMT_stereo2_sph (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Stereographic Spherical, equatorial view)	*/
EXTERN_MSC void GMT_istereo_sph (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Stereographic Spherical) to lon/lat 	*/
EXTERN_MSC void GMT_lambeq (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Lambert Azimuthal Equal-Area)*/
EXTERN_MSC void GMT_ilambeq (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Lambert Azimuthal Equal-Area) to lon/lat*/
EXTERN_MSC void GMT_mollweide (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Mollweide Equal-Area)	*/
EXTERN_MSC void GMT_imollweide (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Mollweide Equal-Area) to lon/lat 	*/
EXTERN_MSC void GMT_cyleq (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Cylindrical Equal-Area)	*/
EXTERN_MSC void GMT_icyleq (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Cylindrical Equal-Area) to lon/lat 	*/
EXTERN_MSC void GMT_cyleqdist (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Cylindrical Equidistant)	*/
EXTERN_MSC void GMT_icyleqdist (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Cylindrical Equidistant) to lon/lat 	*/
EXTERN_MSC void GMT_miller (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);		/* Convert lon/lat to x/y (Miller Cylindrical)	*/
EXTERN_MSC void GMT_imiller (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);		/* Convert x/y (Miller Cylindrical) to lon/lat 	*/
EXTERN_MSC void GMT_cylstereo (struct GMT_CTRL *C, double lon, double lat, double *x, double *y);	/* Convert lon/lat to x/y (Cylindrical Stereographic)	*/
EXTERN_MSC void GMT_icylstereo (struct GMT_CTRL *C, double *lon, double *lat, double x, double y);	/* Convert x/y (Cylindrical Stereographic) to lon/lat 	*/
EXTERN_MSC void GMT_obl (struct GMT_CTRL *C, double lon, double lat, double *olon, double *olat);	/* Convert lon/loat to oblique lon/lat		*/
EXTERN_MSC void GMT_iobl (struct GMT_CTRL *C, double *lon, double *lat, double olon, double olat);	/* Convert oblique lon/lat to regular lon/lat	*/
EXTERN_MSC double GMT_left_winkel (struct GMT_CTRL *C, double y);	/* For Winkel maps	*/
EXTERN_MSC double GMT_right_winkel (struct GMT_CTRL *C, double y);	/* For Winkel maps	*/
EXTERN_MSC double GMT_left_eckert4 (struct GMT_CTRL *C, double y);	/* For Eckert IV maps	*/
EXTERN_MSC double GMT_right_eckert4 (struct GMT_CTRL *C, double y);	/* For Eckert IV maps	*/
EXTERN_MSC double GMT_left_eckert6 (struct GMT_CTRL *C, double y);	/* For Eckert VI maps	*/
EXTERN_MSC double GMT_right_eckert6 (struct GMT_CTRL *C, double y);	/* For Eckert VI maps	*/
EXTERN_MSC double GMT_left_robinson (struct GMT_CTRL *C, double y);	/* For Robinson maps	*/
EXTERN_MSC double GMT_right_robinson (struct GMT_CTRL *C, double y);	/* For Robinson maps	*/
EXTERN_MSC double GMT_left_sinusoidal (struct GMT_CTRL *C, double y);	/* For sinusoidal maps	*/
EXTERN_MSC double GMT_right_sinusoidal (struct GMT_CTRL *C, double y);	/* For sinusoidal maps	*/
EXTERN_MSC double GMT_left_polyconic (struct GMT_CTRL *C, double y);	/* For polyconic maps	*/
EXTERN_MSC double GMT_right_polyconic (struct GMT_CTRL *C, double y);	/* For polyconic maps	*/

/* Complex math from gmt_stat.c */
EXTERN_MSC void gmt_Cmul (double A[], double B[], double C[]);
EXTERN_MSC void gmt_Cdiv (double A[], double B[], double C[]);
EXTERN_MSC void gmt_Ccot (double Z[], double cotZ[]);
EXTERN_MSC double Cabs (double A[]);

/* From gmt_fft.c */
EXTERN_MSC void GMT_fft_initialization (struct GMT_CTRL *C);

/* From gmtapi_util.c */
/* Sub function needed by GMT_end to free memory used in modules and at end of session */

EXTERN_MSC void GMT_Garbage_Collection (struct GMTAPI_CTRL *C, GMT_LONG level);

/* For supplements */
#ifdef GMT_COMPAT
	EXTERN_MSC GMT_LONG backwards_SQ_parsing (struct GMT_CTRL *C, char option, char *item);
#endif
EXTERN_MSC int gmt_comp_double_asc (const void *p_1, const void *p_2);

EXTERN_MSC void gmt_set_double_ptr (double **ptr, double *array);
EXTERN_MSC void gmt_set_char_ptr (char **ptr, char *array);
EXTERN_MSC void GMT_free_dataset_ptr (struct GMT_CTRL *C, struct GMT_DATASET *data);
EXTERN_MSC void GMT_free_textset_ptr (struct GMT_CTRL *C, struct GMT_TEXTSET *data);
EXTERN_MSC GMT_LONG GMT_free_cpt_ptr (struct GMT_CTRL *C, struct GMT_PALETTE *P);
EXTERN_MSC void GMT_free_grid_ptr (struct GMT_CTRL *C, struct GMT_GRID *G, BOOLEAN free_grid);
EXTERN_MSC void GMT_free_matrix_ptr (struct GMT_CTRL *C, struct GMT_MATRIX *M, BOOLEAN free_matrix);
EXTERN_MSC void GMT_free_vector_ptr (struct GMT_CTRL *C, struct GMT_VECTOR *V, BOOLEAN free_vector);
#ifdef USE_GDAL
EXTERN_MSC void GMT_free_image_ptr (struct GMT_CTRL *C, struct GMT_IMAGE *I, BOOLEAN free_image);
#endif
#endif /* _GMT_INTERNALS_H */
