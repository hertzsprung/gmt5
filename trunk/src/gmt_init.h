/*--------------------------------------------------------------------
 *	$Id: gmt_init.h,v 1.71 2008-02-19 23:42:24 guru Exp $
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
 * Include file for gmt_init.c
 *
 * Author:	Paul Wessel
 * Date:	21-AUG-1995
 * Revised:	22-MAR-2006
 * Version:	4.1
 */

#ifndef GMT_INIT_H
#define GMT_INIT_H
EXTERN_MSC double GMT_convert_units (char *from, int new_format);
EXTERN_MSC BOOLEAN GMT_is_invalid_number (char *t);
EXTERN_MSC int GMT_begin (int argc, char **argv);
EXTERN_MSC int GMT_check_region (double w, double e, double s, double n);
EXTERN_MSC int GMT_check_scalingopt (char option, char unit, char *unit_name);
EXTERN_MSC int GMT_font_lookup (char *name, struct GMT_FONT *list, int n);
EXTERN_MSC int GMT_sort_options (int argc, char **argv, char *order);
EXTERN_MSC int GMT_parse_common_options (char *item, double *w, double *e, double *s, double *n);
EXTERN_MSC int GMT_get_common_args (char *item, double *w, double *e, double *s, double *n);
EXTERN_MSC int GMT_get_ellipsoid (char *name);
EXTERN_MSC int GMT_get_time_system (char *name);
EXTERN_MSC int GMT_get_unit (char c);
EXTERN_MSC int GMT_hash (char *v, int n_hash);
EXTERN_MSC int GMT_hash_lookup (char *key, struct GMT_HASH *hashnode, int n, int n_hash);
EXTERN_MSC int GMT_parse_J_option (char *args);
EXTERN_MSC int GMT_parse_R_option (char *item, double *w, double *e, double *s, double *n);
EXTERN_MSC int GMT_unit_lookup (int c);
EXTERN_MSC void GMT_cont_syntax (int indent, int kind);
EXTERN_MSC void GMT_default_error (char option);
EXTERN_MSC void GMT_end (int argc, char **argv);
EXTERN_MSC void GMT_explain_option (char option);
EXTERN_MSC void GMT_fill_syntax (char option, char *string);
EXTERN_MSC void GMT_getdefaults (char *this_file);
EXTERN_MSC void GMT_putdefaults (char *this_file);
EXTERN_MSC void GMT_hash_init (struct GMT_HASH *hashnode , char **keys, int n_hash, int n_keys);
EXTERN_MSC void GMT_free_hash (struct GMT_HASH *hashnode, int n_items);
EXTERN_MSC void GMT_inc_syntax (char option, int error);
EXTERN_MSC int GMT_init_fonts (int *n_fonts);
EXTERN_MSC void GMT_init_scales (int unit, double *fwd_scale, double *inv_scale, double *inch_to_unit, double *unit_to_inch, char *unit_name);
EXTERN_MSC void GMT_label_syntax (int indent, int kind);
EXTERN_MSC void GMT_pen_syntax (char option, char *string);
EXTERN_MSC void GMT_rgb_syntax (char option, char *string);
EXTERN_MSC void GMT_set_home (void);
EXTERN_MSC int GMT_set_measure_unit (char unit);
EXTERN_MSC void GMT_setdefaults (int argc, char **argv);
EXTERN_MSC void GMT_syntax (char option);
EXTERN_MSC int GMT_getdefpath (int get, char **path);
EXTERN_MSC int GMT_parse_symbol_option (char *text, struct GMT_SYMBOL *p, int mode, BOOLEAN cmd);
EXTERN_MSC void GMT_extract_label (char *line, char *label);
EXTERN_MSC int GMT_setparameter(char *keyword, char *value);

#endif /* GMT_INIT_H */
