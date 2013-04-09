/*	$Id$
 *    Copyright (c) 1996-2012 by G. Patau
 *    Distributed under the Lesser GNU Public Licence
 *    See README file for copying and redistribution conditions.
 */

void get_trans (struct GMT_CTRL *GMT, double slon, double slat, double *t11, double *t12, double *t21, double *t22);
double ps_mechanism (struct GMT_CTRL *GMT, struct PSL_CTRL *PSL, double x0, double y0, st_me meca, double size, struct GMT_FILL *F, struct GMT_FILL *E, int outline);
double ps_plan (struct GMT_CTRL *GMT, struct PSL_CTRL *PSL, double x0, double y0, st_me meca, double size, int num_of_plane);
double computed_mw(struct MOMENT moment, double ms);
double computed_dip2(double str1, double dip1, double str2);
double computed_rake2(double str1, double dip1, double str2, double dip2, double fault);
void define_second_plane(struct nodal_plane NP, struct nodal_plane *NP2);
double ps_tensor (struct GMT_CTRL *GMT, struct PSL_CTRL *PSL, double x0, double y0, double size, struct AXIS T, struct AXIS N, struct AXIS P, struct GMT_FILL *C, struct GMT_FILL *E, int outline, int plot_zerotrace);
void axe2dc(struct AXIS T, struct AXIS P, struct nodal_plane *NP1, struct nodal_plane *NP2);
void dc2axe (st_me meca, struct AXIS *T, struct AXIS *N, struct AXIS *P);
void ps_pt_axis(double x0, double y0, st_me meca, double size, double *pp, double *dp, double *pt, double *dt, double *xp, double *yp, double *xt, double *yt);
void moment2axe(struct GMT_CTRL *GMT, struct M_TENSOR mt, struct AXIS *T, struct AXIS *N, struct AXIS *P);
void axis2xy(double x0, double y0, double size, double pp, double dp, double pt, double dt, double *xp, double *yp, double *xt, double *yt);
void transform_local (double x0, double y0, double dxp, double dyp, double scale, double t11, double t12, double t21, double t22, double *x1, double *y1);
void trace_arrow (struct GMT_CTRL *GMT, double slon, double slat, double dxp, double dyp, double scale, double *x1, double *y1, double *x2, double *y2);
void ellipse_convert (double sigx, double sigy, double rho, double conrad, double *eigen1, double *eigen2, double *ang) ;
void paint_ellipse (struct GMT_CTRL *GMT, double x0, double y0, double angle, double major, double minor, double scale, double t11, double t12, double t21, double t22, int polygon, struct GMT_FILL *fill, int outline);
int trace_cross (struct GMT_CTRL *GMT, double slon, double slat, double eps1, double eps2, double theta, double sscale, double v_width, double h_length, double h_width, double vector_shape, int outline, struct GMT_PEN pen);
void paint_wedge (struct PSL_CTRL *PSL, double x0, double y0, double spin, double spinsig, double sscale, double wedge_amp, double t11, double t12, double t21, double t22, int polygon, double *rgb, int epolygon, double *ergb, int outline);
double zero_360 (double str);
