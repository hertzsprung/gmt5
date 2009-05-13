/*	$Id: utilstrain.h,v 1.8 2009-05-13 21:06:43 guru Exp $
 *    Copyright (c) 1996-2009 by G. Patau
 *    Distributed under the GNU Public Licence
 *    See README file for copying and redistribution conditions.
 */

GMT_LONG trace_cross (double slon,double slat,double eps1,double eps2,double theta,double sscale,double v_width,double h_length,double h_width,double vector_shape,BOOLEAN outline,struct GMT_PEN pen);
void paint_wedge (double x0, double y0, double spin, double spinsig, double sscale, double wedge_amp, double t11,double t12,double t21,double t22, GMT_LONG polygon, int rgb[3], GMT_LONG epolygon, int ergb[3], GMT_LONG outline);
