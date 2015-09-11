/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2014 by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis and F. Wobbe
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
/*
 * Authors:     Paul Wessel and Seung-Sep Kim
 * Date:        10-JUN-2015
 *
 *
 * Calculates gravity due to 2-D crossectional polygonal shapes
 * of an infinite horizontal body [or bodies]
 * It expects all distances to be in meters (you can override
 * with the -M option) and densities to be in kg/m^3.
 *
 * Based on methods by:
 *
 * Chapman, M. E. (1979), Techniques for interpretation of geoid
 *    anomalies, J. Geophys. Res., 84(B8), 3793–3801.
 * Rasmussen, R., and L. B. Pedersen (1979), End corrections in
 *   potential field modeling, Geophys. Prospect., 27, 749–760.
 * Talwani, M., J. L. Worzel, and M. Landisman (1959), Rapid gravity
 *    computations for two-dimensional bodies with application to
 *    the Mendocino submarine fracture zone, J. Geophys. Res., 64, 49–59.
 * Kim, S.-S., and P. Wessel, 2015, in prep.
 *
 * Accelerated with OpenMP; see -x.
 */

#define THIS_MODULE_NAME	"talwani2d"
#define THIS_MODULE_LIB		"potential"
#define THIS_MODULE_PURPOSE	"Compute free-air, geoid or vertical gravity gradients anomalies over 2-D bodies"
#define THIS_MODULE_KEYS	"<DI,NDi,>DO"

#include "gmt_dev.h"

#define GMT_PROG_OPTIONS "-Vhio" GMT_ADD_x_OPT

#define TOL		1.0e-7
#define GAMMA 		6.673e-11	/* Gravitational constant */
#define G               9.81            /* Normal gravity */
#define SI_TO_MGAL	1.0e5		/* Convert m/s^2 to mGal */
#define SI_TO_EOTVOS	1.0e9		/* Convert (m/s^2)/m to Eotvos */

struct TALWANI2D_CTRL {
	struct Out {	/* -> */
		bool active;
		char *file;
	} Out;
	struct A {	/* -A positive up  */
		bool active;
	} A;
	struct D {	/* -D<rho> fixed density to override model files */
		bool active;
		double rho;
	} D;
	struct F {	/* -F[f|n|v] */
		bool active;
		unsigned int mode;
	} F;
	struct M {	/* -Mh|z  */
		bool active[2];	/* True if km, else m */
	} M;
	struct N {	/* Output points with optional obs-z values*/
		bool active;
		char *file;
	} N;
	struct T {	/* -T[<tmin/tmax/t_inc>[+]] | -T<file> */
		bool active;
		bool notime;
		double min, max, inc;
		char *file;
	} T;
	struct Z {	/* Observation level constant */
		bool active;
		double level;
		double ymin, ymax;
		unsigned int mode;
	} Z;
};

struct BODY2D {
	int n;
	double rho;
	double *x, *z;
};

enum Talwani2d_fields {
	TALWANI2D_FAA	= 0,
	TALWANI2D_VGG	= 1,
	TALWANI2D_GEOID	= 2,
	TALWANI2D_FAA2	= 3,
	TALWANI2D_HOR=0,
	TALWANI2D_VER=1
};

void *New_talwani2d_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct TALWANI2D_CTRL *C = NULL;
	
	C = GMT_memory (GMT, NULL, 1, struct TALWANI2D_CTRL);
	
	/* Initialize values whose defaults are not 0/false/NULL */

	return (C);
}

void Free_talwani2d_Ctrl (struct GMT_CTRL *GMT, struct TALWANI2D_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	if (C->Out.file) free (C->Out.file);
	if (C->N.file) free (C->N.file);	
	GMT_free (GMT, C);	
}

int GMT_talwani2d_parse (struct GMT_CTRL *GMT, struct TALWANI2D_CTRL *Ctrl, struct GMT_OPTION *options)
{
	unsigned int k, n_errors = 0, n_files = 0;
	int n;
	struct GMT_OPTION *opt = NULL;

	for (opt = options; opt; opt = opt->next) {		/* Process all the options given */
		switch (opt->option) {

			case '<':	/* Input file(s) */
				if (!GMT_check_filearg (GMT, '<', opt->arg, GMT_IN, GMT_IS_DATASET)) n_errors++;
				break;
			case '>':	/* Got named output file */
				if (n_files++ == 0 && GMT_check_filearg (GMT, '>', opt->arg, GMT_OUT, GMT_IS_DATASET))
					Ctrl->Out.file = strdup (opt->arg);
				else
					n_errors++;
				break;

			case 'A':	/* Specify z-axis is positive up [Default is down] */
				Ctrl->A.active = TRUE;
				break;
			case 'D':
				Ctrl->D.active = TRUE;
				Ctrl->D.rho = atof (opt->arg);
				break;
			case 'F':
				Ctrl->F.active = true;
				switch (opt->arg[0]) {
					case 'v': Ctrl->F.mode = TALWANI2D_VGG;		break;
					case 'n': Ctrl->F.mode = TALWANI2D_GEOID;	break;
					default:  Ctrl->F.mode = TALWANI2D_FAA; 	break;
				}
				break;
			case 'M':	/* Length units */
				k = 0;
				while (opt->arg[k]) {
					switch (opt->arg[k]) {
						case 'h': Ctrl->M.active[TALWANI2D_HOR] = true; break;
						case 'z': Ctrl->M.active[TALWANI2D_VER] = true; break;
						default:
							n_errors++;
							GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Syntax error -M: Unrecognized modifier %c\n", opt->arg[k]);
							break;
					}
					k++;
				}
				break;
			case 'N':
				Ctrl->N.active = true;
				Ctrl->N.file = strdup (opt->arg);
				break;
			case 'T':	/* Either get a file with time coordinate or a min/max/dt setting */
				Ctrl->T.active = true;
				/* Presumably gave tmin/tmax/tinc */
				if (sscanf (opt->arg, "%lf/%lf/%lf", &Ctrl->T.min, &Ctrl->T.max, &Ctrl->T.inc) != 3) {
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Syntax error: Unable to decode arguments for -T\n");
					n_errors++;
				}
				if (opt->arg[strlen(opt->arg)-1] == '+') {	/* Gave number of points instead; calculate inc */
					Ctrl->T.inc = (Ctrl->T.max - Ctrl->T.min) / (Ctrl->T.inc - 1.0);
				}
				break;
			case 'Z':
				Ctrl->Z.active = true;
				k = (opt->arg[0] == '/') ? 1 : 0;	/* In case someone gives -Z/ymin/ymax */
				n = sscanf (&opt->arg[k], "%lf/%lf/%lf", &Ctrl->Z.level, &Ctrl->Z.ymin, &Ctrl->Z.ymax);
				if (n == 3) Ctrl->Z.mode = 3;
				else if (n == 2) {Ctrl->Z.mode = 2; Ctrl->Z.ymax = Ctrl->Z.ymin; Ctrl->Z.ymin = Ctrl->Z.level;}
				else if (n == 1) Ctrl->Z.mode = 1;
				break;
			default:
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}
	n_errors += GMT_check_condition (GMT, Ctrl->T.active && Ctrl->N.active,
	                                 "Syntax error -N option: Cannot also specify -T\n");
	n_errors += GMT_check_condition (GMT, (Ctrl->Z.mode & 2) && Ctrl->Z.ymin >= Ctrl->Z.ymax,
				         "Syntax error -Z option: The ymin >= ymax for 2.5-D body\n");
	n_errors += GMT_check_condition (GMT, (Ctrl->Z.mode & 2) && Ctrl->F.mode != TALWANI2D_FAA,
				         "Syntax error -Z option: 2.5-D solution only available for FAA\n");
	n_errors += GMT_check_condition (GMT, n_files > 1, "Syntax error: Only one output destination can be specified\n");
	if ((Ctrl->Z.mode & 2) && Ctrl->F.mode == TALWANI2D_FAA) Ctrl->F.mode = TALWANI2D_FAA2;
	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

int GMT_talwani2d_usage (struct GMTAPI_CTRL *API, int level) {
	GMT_show_name_and_purpose (API, THIS_MODULE_LIB, THIS_MODULE_NAME, THIS_MODULE_PURPOSE);
	if (level == GMT_MODULE_PURPOSE) return (GMT_NOERROR);
	GMT_Message (API, GMT_TIME_NONE, "usage: talwani2d <modelfile> [-A] [-D<rho>] [-Ff|n|v]\n");
	GMT_Message (API, GMT_TIME_NONE, "\t[-M[hz]] [-T[<xmin>/<xmax>/<xinc>[+]]] [%s] [-Z[<level>][/<ymin/<ymax>]]\n", GMT_V_OPT);
	GMT_Message (API, GMT_TIME_NONE,"\t[%s]\n\t[%s] [%s]%s\n\n", GMT_h_OPT, GMT_i_OPT, GMT_o_OPT, GMT_x_OPT);

	if (level == GMT_SYNOPSIS) return (EXIT_FAILURE);

	GMT_Message (API, GMT_TIME_NONE, "\t<modelfile> is a multiple-segment ASCII file.\n");
	GMT_Message (API, GMT_TIME_NONE, "\n\tOPTIONS:\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-A The z-axis is positive upwards [Default is positive down].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-D Sets fixed density contrast that overrides settings in model file (in kg/m^3).\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-F Specify desired geopotential field component:\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   f = FAA Free-air anomalies (mGal) [Default].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   n = Geoid anomalies (meter).\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   v = VGG Vertical Gravity Gradient anomalies (Eotvos = 0.1 mGal/km).\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-M sets units used, as follows:\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   -Mh indicates all x-distances are given in km [meters]\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   -Mz indicates all z-distances are given in km [meters]\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-N Gives file with output locations x[,z].  If there are\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   z-coordinates then these are used as observation levels.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   You can use -Z to override these by setting a constant level.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-T Set domain from <xmin> to <xmax> in steps of <xinc>.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   Append + to xinc to indicate the number of points instead.\n");
	GMT_Option (API, "V");
	GMT_Message (API, GMT_TIME_NONE, "\t-Z Set observation level for output locations [0].\n");
	GMT_Message (API, GMT_TIME_NONE, "\t   For FAA only: Optionally append <ymin/ymax> to get a 2.5-D solution.\n");
	GMT_Option (API, "h,i,o,x,.");
	return (EXIT_FAILURE);
}

/*
 * grav_2_5D returns the gravitational attraction from a 2 1/2 - D
 * body, i.e. a 2D body with finite length in the strike direction.
 * The routine is based on the paper
 * Rasmussen & Pedersen, 1979, End corrections in potential field
 * modelling, Geophys. Prosp.
 */
 
double integralI1 (double xa, double xb, double za, double zb, double y)
{
	/* This function performs the integral I1 (i,Y) from
	 * Rasmussen & Pedersen's paper
	 */

	double yy, xdiff, zdiff, side, cosfi, sinfi, ui, uii, wi, ri, rii, rri, rrii;
	double part1, part2, part3, fact;

	yy = fabs (y);
	if (yy == 0.0) return (0.0);
	xdiff = xb - xa;
	zdiff = zb - za;
	side = hypot (xdiff, zdiff);
	cosfi = xdiff / side;
	sinfi = zdiff / side;
	ui  = cosfi * xa + sinfi * za;
	uii = cosfi * xb + sinfi * zb;
	wi = -sinfi * xa + cosfi * za;
	if (wi == 0.0) wi = 1.0e-30;
	ri = hypot (ui, wi);
	rii = hypot (uii, wi);
	rri = hypot (ri, yy);
	rrii = hypot (rii, yy);
	part1 = cosfi * yy * log ( (uii + rrii) / (ui + rri));
	fact = (xa * zb - za * xb) / (side * side);
	part2 = fact * zdiff * log ( (rii * (yy + rri)) / (ri * (yy + rrii)));
	part3 = fact * xdiff * (atan ( (uii * yy) / (wi * rrii)) - atan ( (ui * yy) / (wi * rri)));
	return (part1 + part2 + part3);
}

double grav_2_5D (struct GMT_CTRL *GMT, double x[], double z[], unsigned int n, double x0, double z0,
                  double rho, double ymin, double ymax) {
/*  x0;		X-coordinate of observation point */
/*  z0;		Z-coordinate of observation point */
/*  x[];	Array of xpositions */
/*  z[];	Array of zpositions */
/*  n;		Number of corners */
/*  rho;	Density contrast */
/*  ymin;	Extent of body in y-direction */
/*  ymax; */

	double xx0, zz0, xx1, zz1, part_1, part_2, sum;
	int i, i1;
	
	n--;	/* Since last point is repeated */
	xx0 = x[0] - x0;
	zz0 = z[0] - z0;
	sum = 0.0;
	if (hypot (xx0, zz0) == 0) {      
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Observation point coincides with a body vertex!\n");
		return GMT->session.d_NaN;
	}
	for (i = 0; i < (int)n; i++) {
		i1 = i + 1;	/* next point is simple since the last is repeated as first */
		xx1 = x[i1] - x0;
		zz1 = z[i1] - z0;
		if (hypot (xx1, zz1) == 0) {      
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Observation point coincides with a body vertex!\n");
			return GMT->session.d_NaN;
		}
		part_1 = integralI1 (xx0, xx1, zz0, zz1, ymin);
		if (ymin > 0.0) part_1 = -part_1;
		part_2 = integralI1 (xx0, xx1, zz0, zz1, ymax);
		if (ymax < 0.0) part_2 = -part_2;
		sum += (part_1 + part_2);
		xx0 = xx1;
		zz0 = zz1;
	}
	sum *= GAMMA * rho * SI_TO_MGAL;	/* To get mGal */
	return (sum);
}

double get_grav2d (struct GMT_CTRL *GMT, double x[], double z[], unsigned int n, double x0, double z0, double rho)
{
	/*  x0;		X-coordinate of observation point */
	/*  z0;		Z-coordinate of observation point */
	/*  x[];	Array of xpositions */
	/*  z[];	Array of zpositions */
	/*  n;		Number of corners */
	/*  rho;	Density contrast */
	int i, i1;
	double xi, xi1, zi, zi1, ri, ri1, phi_i, phi_i1, sum;

	n--;	/* Since last point is repeated */
	xi = x[0] - x0;
	zi = z[0] - z0;
	phi_i = atan2 (zi, xi);
	ri = hypot (xi, zi);
	if (ri == 0) {      
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Observation point coincides with a body vertex!\n");
		return GMT->session.d_NaN;
	}
	for (i = 0, sum = 0.0; i < (int)n; i++) {
		i1 = i + 1;	/* next point is simple since the last is repeated as first */
		xi1 = x[i1] - x0;
		zi1 = z[i1] - z0;
		phi_i1 = atan2 (zi1, xi1);
		ri1 = hypot (xi1, zi1);
		if (ri1 == 0) {      
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Observation point coincides with a body vertex!\n");
			return GMT->session.d_NaN;
		}
		sum += (xi * zi1 - zi * xi1) * ((xi1 - xi) * (phi_i - phi_i1) + (zi1 - zi) * log (ri1/ri)) /
			(pow (xi1 - xi, 2.0) + pow (zi1 - zi, 2.0));
		xi = xi1;
		zi = zi1;
		ri = ri1;
		phi_i = phi_i1;
	}
	sum *= 2.0 * GAMMA * rho * SI_TO_MGAL;	/* To get mGal */
	return (sum);
}

double get_vgg2d (struct GMT_CTRL *GMT, double *x, double *z, unsigned int n, double x0, double z0, double rho)
{	/* From Kim & Wessel, 2015 */
	int i1, i2;
	double sum = 0.0, x1, z1, x2, z2, r1sq, r2sq;
	double dx, dz, drsq, two_theta2, two_theta1, sin_2th2, sin_2th1;

	n--;	/* Since first and last point are duplicated */
	for (i1 = 0; i1 < (int)n; i1++) {
	        i2 = i1 + 1;
		x1 = x[i1] - x0;
		z1 = z[i1] - z0;
		x2 = x[i2] - x0;
		z2 = z[i2] - z0;
		r1sq = x1 * x1 + z1 * z1;
		if (r1sq == 0) {      
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Observation point coincides with a body vertex!\n");
			return GMT->session.d_NaN;
		}
		r2sq = x2 * x2 + z2 * z2;
		if (r2sq == 0) {      
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Observation point coincides with a body vertex!\n");
			return GMT->session.d_NaN;
		}
		dx = x2 - x1;	dz = z2 - z1;
		two_theta2 = 2.0 * atan2 (z2, x2);	two_theta1 = 2.0 * atan2 (z1, x1);
		sin_2th2 = sin (two_theta2);		sin_2th1 = sin (two_theta1);
		
		if (dz == 0)	/* z1 == z2 so any z will do.  Both log and delta_angle terms vanish */
			sum += log (z2) * (sin_2th2 - sin_2th1);
		else if (dx == 0)	/* log term vanish */
			sum += sin_2th2 * log (z2) - sin_2th1 * log (z1) - (two_theta2 - two_theta1);
		else {	/* Need all terms and must compute drsq */
			drsq = dx * dx + dz * dz;
			sum += (dz * (dx * log (r1sq/r2sq) - dz * (two_theta2 - two_theta1))) / drsq + sin_2th2 * log (z2) - sin_2th1 * log (z1);
		}
	}

	sum *= (-GAMMA * rho * SI_TO_EOTVOS);        /* To get Eotvos */
	return (sum);
}

double get_geoid2d (struct GMT_CTRL *GMT, double y[], double z[], unsigned int n, double y0, double z0, double rho)
{	/* Based on Chaptman, 1979, Techniques for interpretation of geoid anomalies, JGR, 84 (B8), 3793-3801.
	 *  y0;		Y-coordinate of observation point, in m
	 *  z0;		Z-coordinate of observation point, in m
	 *  y[];	Y-coordinates of vertices, in m
	 *  z[];	Z-coordinates of vertices, in m
	 *  n;	Number of vertices, with 1st == last point
	 *  rho;	Density contrast, in kg/m3
	 */
	int i1, i2;
	double dy1, dy2, dz1, dz2, hyp1, hyp2, mi, mi2, a2, bi, ci, ci2, di, di2;
	double part_a_1, part_b_1, part_c_1, part_d_1, part_e_1, part_f_1;
	double part_a_2, part_b_2, part_c_2, part_d_2, part_e_2, part_f_2;
	double N = 0.0, ni;

	n--;	/* Since last point is repeated */
	for (i1 = 0; i1 < (int)n; i1++) {
		i2 = i1 + 1;	/* next point is simple since the last is repeated as first */
		if (z[i1] == z[i2]) continue;		/* Slope mi is zero, so ni == 0 */
		dy1 = y[i1] - y0;	dy2 = y[i2] - y0;
		dz1 = z[i1] - z0;	dz2 = z[i2] - z0;
		hyp1 = dy1 * dy1 + dz1 * dz1;	hyp2 = dy2 * dy2 + dz2 * dz2;
		if (hyp1 == 0) {      
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Observation point coincides with a body vertex!\n");
			return GMT->session.d_NaN;
		}
		if (hyp2 == 0) {      
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Observation point coincides with a body vertex!\n");
			return GMT->session.d_NaN;
		}
		if (y[i1] == y[i2]) {			/* Slope mi is infinite */
			part_a_2 = dy2 * (dz2 * log (hyp2) - 2.0 * ( dz2 - fabs (dy2) * atan (dz2/dy2)) + dy2 * z[i2]);
			part_b_2 = hyp2 * atan (dy2/dz2) + dy2 * dz2;
			part_a_1 = dy1 * (dz1 * log (hyp1) - 2.0 * ( dz1 - fabs (dy1) * atan (dz1/dy1)) + dy1 * z[i1]);
			part_b_1 = hyp1 * atan (dy1/dz1) + dy1 * dz1;
			ni = (part_a_2 + part_b_2 - part_a_1 - part_b_1);
		}
		else {
			mi = (z[i2] - z[i1]) / (y[i2] - y[i1]);
			bi = z[i2] - mi * y[i2];
			mi2 = mi * mi;
			if ((bi/mi) == -y0) {
				part_a_2 = 0.5 * z[i2] * z[i2] * log ((1 + 1.0/mi2)*z[i2]*z[i2])/mi;
				part_b_2 = -1.5 * z[i2] * z[i2] / mi;
				part_c_2 = z[i2] * z[i2] * atan (1.0/mi);
				part_a_1 = 0.5 * z[i1] * z[i1] * log ((1 + 1.0/mi2)*z[i1]*z[i1])/mi;
				part_b_1 = -1.5 * z[i1] * z[i1] / mi;
				part_c_1 = z[i1] * z[i1] * atan (1.0/mi);
				ni = (part_a_2 + part_b_2 + part_c_2) - (part_a_1 + part_b_1 + part_c_1);
			}
			else {
				ci = 1.0 / mi;
				a2 = mi * y0 + bi - z0;
				di = -bi/mi - y0;
				ci2 = ci * ci;
				di2 = di * di;

				part_a_2 = 0.5 * mi * dy2 * dy2 * (log (hyp2) - 1.0) + mi2 * a2 * dy2 / (1.0 + mi2);
				part_b_2 = -(0.5 * mi * (mi2 - 1.0) * a2 * a2 / pow (mi2 + 1.0, 2.0)) * log (hyp2);
				part_c_2 = -(2.0 * mi2 * a2 * a2 / pow (1.0 + mi2, 2.0))
					 * atan (((1.0 + mi2) * dy2 + mi * a2) / a2);
				part_d_2 = -mi * dy2 * dy2 + z[i2] * z[i2] * atan (dy2 / z[i2]);
				part_e_2 = -(ci * di2 / pow (1.0 + ci2, 2.0))
					 * log (((1.0 + ci2) * z[i2] * z[i2] + 2.0 * ci * di * z[i2] + di2)/di2);
				part_f_2 = di * z[i2] / (1.0 + ci2)
					 + ((1.0 - ci2) * di2 / pow (1.0 + ci2, 2.0)) * atan (dy2 / z[i2]);

				part_a_1 = 0.5 * mi * dy1 * dy1 * (log (hyp1) - 1.0) + mi2 * a2 * dy1 / (1.0 + mi2);
				part_b_1 = -(0.5 * mi * (mi2 - 1.0) * a2 * a2 / pow (mi2 + 1.0, 2.0)) * log (hyp1);
				part_c_1 = -(2.0 * mi2 * a2 * a2 / pow (1.0 + mi2, 2.0))
					 * atan (((1.0 + mi2) * dy1 + mi * a2) / a2);
				part_d_1 = -mi * dy1 * dy1 + z[i1] * z[i1] * atan (dy1 / z[i1]);
				part_e_1 = -(ci * di2 / pow (1.0 + ci2, 2.0))
					 * log (((1.0 + ci2) * z[i1] * z[i1] + 2.0 * ci * di * z[i1] + di2)/di2);
				part_f_1 = di * z[i1] / (1.0 + ci2)
					 + ((1.0 - ci2) * di2 / pow (1.0 + ci2, 2.0)) * atan (dy1 / z[i1]);

				ni = (part_a_2 + part_b_2 + part_c_2 + part_d_2 + part_e_2 + part_f_2)
					- (part_a_1 + part_b_1 + part_c_1 + part_d_1 + part_e_1 + part_f_1);
			}
		}
		N = N + ni;
	}
	N *= (-GAMMA * rho / G);
	return (N);
}

double get_one_output2D (struct GMT_CTRL *GMT, double x_obs, double z_obs, struct BODY2D *body, unsigned int n_bodies, unsigned int mode, double ymin, double ymax)
{	/* Evaluate output at a single observation point (x,z) */
	/* Work array vtry must have at least of length ndepths */
	unsigned int k;
	double area, v_sum = 0.0, v;

	for (k = 0; k < n_bodies; k++) {	/* Loop over slices */
		area = GMT_pol_area (body[k].x, body[k].z, body[k].n);
		v = 0.0;
		if (mode == TALWANI2D_FAA) /* FAA */
			v += get_grav2d (GMT, body[k].x, body[k].z, body[k].n, x_obs, z_obs, body[k].rho);
		else if (mode == TALWANI2D_FAA2) /* FAA 2.5D */
			v += grav_2_5D (GMT, body[k].x, body[k].z, body[k].n, x_obs, z_obs, body[k].rho, ymin, ymax);
		else if (mode == TALWANI2D_VGG) /* VGG */
			v += get_vgg2d (GMT, body[k].x, body[k].z, body[k].n, x_obs, z_obs, body[k].rho);
		else /* GEOID*/
			v += get_geoid2d (GMT, body[k].x, body[k].z, body[k].n, x_obs, z_obs, body[k].rho);
		if (area < 0.0) v = -v;	/* Polygon went counter-clockwise */
		v_sum += v;
	}
	return (v_sum);
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_talwani2d_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_talwani2d (void *V_API, int mode, void *args)
{
	int srow, error = 0, ns;
	unsigned int k, tbl, seg, n = 0, geometry, n_bodies, dup_node, n_duplicate = 0;
	size_t n_alloc = 0, n_alloc1 = 0;
	uint64_t dim[4] = {1, 1, 0, 2}, row;
	double scl, rho, z_level, answer, min_answer = DBL_MAX, max_answer = -DBL_MAX;
	bool first = true;
	
	char *uname[2] = {"meter", "km"}, *kind[4] = {"FAA", "VGG", "GEOID", "FAA(2.5-D)"};
	double *x = NULL, *z = NULL, *in = NULL;
					
	struct BODY2D *body = NULL;
	struct TALWANI2D_CTRL *Ctrl = NULL;
	struct GMT_DATASET *Out = NULL;
	struct GMT_DATASEGMENT *S = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_NOT_A_SESSION);
	if (mode == GMT_MODULE_PURPOSE) return (GMT_talwani2d_usage (API, GMT_MODULE_PURPOSE));	/* Return the purpose of program */
	options = GMT_Create_Options (API, mode, args);
	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMT_OPT_USAGE)
		bailout (GMT_talwani2d_usage (API, GMT_USAGE));	/* Return the usage message */
	if (options->option == GMT_OPT_SYNOPSIS)
		bailout (GMT_talwani2d_usage (API, GMT_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments */

	GMT = GMT_begin_module (API, THIS_MODULE_LIB, THIS_MODULE_NAME, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, GMT_PROG_OPTIONS, options)) Return (API->error);
	Ctrl = New_talwani2d_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_talwani2d_parse (GMT, Ctrl, options))) Return (error);

	/*---------------------------- This is the talwani2d main code ----------------------------*/
	
	GMT_enable_threads (GMT);	/* Set number of active threads, if supported */
	scl = (Ctrl->M.active[TALWANI2D_HOR]) ? METERS_IN_A_KM : 1.0;	/* Perhaps convert to m */
	
	if (Ctrl->T.active) {
		/* Make sure the min/man/inc values harmonize */
		switch (GMT_minmaxinc_verify (GMT, Ctrl->T.min, Ctrl->T.max, Ctrl->T.inc, GMT_CONV4_LIMIT)) {
			case 1:
				GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -T option: (max - min) is not a whole multiple of inc\n");
				Return (EXIT_FAILURE);
				break;
			case 2:
				if (Ctrl->T.inc != 1.0) {	/* Allow for somebody explicitly saying -T0/0/1 */
					GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -T option: (max - min) is <= 0\n");
					Return (EXIT_FAILURE);
				}
				break;
			case 3:
				GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -T option: inc is <= 0\n");
				Return (EXIT_FAILURE);
				break;
			default:	/* OK */
				break;
		}
		geometry = GMT_IS_LINE;
		dim[GMT_ROW] = lrint ((Ctrl->T.max - Ctrl->T.min) / Ctrl->T.inc) + 1;
		if ((Out = GMT_Create_Data (API, GMT_IS_DATASET, GMT_IS_LINE, 0, dim, NULL, NULL, 0, 0, NULL)) == NULL) Return (GMT_MEMORY_ERROR);
		S = Out->table[0]->segment[0];	/* Only one segment when -T is used */
		for (row = 0; row < dim[GMT_ROW]; row++) S->coord[GMT_X][row] = (row == (S->n_rows-1)) ? Ctrl->T.max: Ctrl->T.min + row * Ctrl->T.inc;
	}
	else {	/* Got a dataset with output locations */
		geometry = GMT_IS_PLP;	/* We dont really know */
		if ((Out = GMT_Read_Data (API, GMT_IS_DATASET, GMT_IS_FILE, GMT_IS_PLP, GMT_READ_NORMAL, NULL, Ctrl->N.file, NULL)) == NULL) {
			Return (API->error);
		}
	}

	/* Specify input expected columns to be at least 2 */
	if ((error = GMT_set_cols (GMT, GMT_IN, 2)) != GMT_OK) {
		Return (error);
	}
	/* Register likely model files unless the caller has already done so */
	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POLYGON, GMT_IN, GMT_ADD_DEFAULT, 0, options) != GMT_OK) {	/* Registers default input sources, unless already set */
		Return (API->error);
	}
	/* Initialize the i/o for doing record-by-record reading */
	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_IN, GMT_HEADER_ON) != GMT_OK) {	/* Enables data input and sets access mode */
		Return (API->error);
	}

	/* Set up cake slice array and pointers */
	
	n_alloc1 = GMT_CHUNK;
	body = GMT_memory (GMT, NULL, n_alloc1, struct BODY2D);
	n_bodies = 0;
	/* Read polygon information from multiple segment file */
	GMT_Report (API, GMT_MSG_VERBOSE, "All x-values are assumed to be given in %s\n", uname[Ctrl->M.active[TALWANI2D_HOR]]);
	GMT_Report (API, GMT_MSG_VERBOSE, "All z-values are assumed to be given in %s\n", uname[Ctrl->M.active[TALWANI2D_VER]]);
	
	/* Read the sliced model */
	do {	/* Keep returning records until we reach EOF */
		if ((in = GMT_Get_Record (API, GMT_READ_DOUBLE, NULL)) == NULL) {	/* Read next record, get NULL if special case */
			if (GMT_REC_IS_ERROR (GMT)) 		/* Bail if there are any read errors */
				Return (GMT_RUNTIME_ERROR);
			if (GMT_REC_IS_TABLE_HEADER (GMT)) 	/* Skip all table headers */
				continue;
			if (GMT_REC_IS_SEGMENT_HEADER (GMT) || GMT_REC_IS_EOF (GMT)) {	/* Process segment headers or end-of-file */
				if (!first) {	/* First close previous body */
					if (!(x[n-1] == x[0] && z[n-1] == z[0])) {	/* Copy first point to last */
						if (n_duplicate == 1 && dup_node == n) n_duplicate = 0;	/* So it was the last == first duplicate; reset count */
						x[n] = x[0];	z[n] = z[0];
						n++;
					}
					x = GMT_memory (GMT, x, n, double);
					z = GMT_memory (GMT, z, n, double);
					body[n_bodies].rho = rho;
					body[n_bodies].x = x;
					body[n_bodies].z = z;
					body[n_bodies].n = n;
					n_bodies++;
				}
				first = false;
				if (GMT_REC_IS_EOF (GMT)) 		/* Reached end of file */
					break;
				/* Process the next segment header */
				ns = sscanf (GMT->current.io.segment_header, "%lf",  &rho);
				if (ns == 0 && !Ctrl->D.active) {
					GMT_Report (API, GMT_MSG_VERBOSE, "Neither segment header nor -D specified density - must quit\n");
					Return (API->error);
				}
				if (Ctrl->D.active) rho = Ctrl->D.rho;
				/* Allocate array for this body */
				n_alloc = GMT_CHUNK;
				x = GMT_memory (GMT, NULL, n_alloc, double);
				z = GMT_memory (GMT, NULL, n_alloc, double);
				n = 0;
				if (n_bodies == n_alloc1) {
					n_alloc1 <<= 1;
					body = GMT_memory (GMT, body, n_alloc1, struct BODY2D);
				}
				continue;
			}
		}
		/* Clean data record to process.  Add point unless duplicate */
		if (Ctrl->A.active) in[GMT_Y] = -in[GMT_Y];
		if (n && (x[n-1] == x[n] && z[n-1] == z[n])) {	/* Maybe a duplicate point - or it could be the repeated last = first */
			n_duplicate++;
			dup_node = n;
		}
		else {
			x[n] = in[GMT_X];	z[n] = in[GMT_Y];
			if (Ctrl->M.active[TALWANI2D_HOR]) x[n] *= METERS_IN_A_KM;	/* Change distances to m */
			if (Ctrl->M.active[TALWANI2D_VER]) z[n] *= METERS_IN_A_KM;	/* Change distances to m */
			n++;
			if (n == n_alloc) {
				n_alloc += GMT_CHUNK;
				x = GMT_memory (GMT, x, n_alloc, double);
				z = GMT_memory (GMT, z, n_alloc, double);
			}
		}
	} while (true);
	
	if (GMT_End_IO (API, GMT_IN, 0) != GMT_OK) {	/* Disables further data input */
		Return (API->error);
	}
	
	/* Finish allocation */
	
	body = GMT_memory (GMT, body, n_bodies, struct BODY2D);
	
	if (n_duplicate) GMT_Report (API, GMT_MSG_VERBOSE, "Ignored %u duplicate vertices\n", n_duplicate);
	
	if (Ctrl->A.active) Ctrl->Z.level = -Ctrl->Z.level;
	
	/* Now we can write to the screen the user's polygon model characteristics. */
	
	for (k = 0, rho = 0.0; k < n_bodies; k++) {
		rho += body[k].rho;
		GMT_Report (API, GMT_MSG_LONG_VERBOSE, "%lg Rho: %lg N-vertx: %4d\n", body[k].rho, body[k].n);
	}
	GMT_Report (API, GMT_MSG_VERBOSE, "Start calculating %s\n", kind[Ctrl->F.mode]);
	
	if (Out->n_segments > 1) GMT_set_segmentheader (GMT, GMT_OUT, true);	
	for (tbl = 0; tbl < Out->n_tables; tbl++) {
		for (seg = 0; seg < Out->table[tbl]->n_segments; seg++) {
			S = Out->table[tbl]->segment[seg];	/* Current segment */
#ifdef _OPENMP
			/* Spread calculation over selected cores */
#pragma omp parallel for private(srow,z_level,answer) shared(GMT,Ctrl,S,scl,body,n_bodies)
#endif
			for (srow = 0; srow < (int)S->n_rows; srow++) {	/* Calculate attraction at all output locations for this segment. OpenMP requires sign int srow */
				z_level = (S->n_columns == 2 && !(Ctrl->Z.mode & 1)) ? S->coord[GMT_Y][srow] : Ctrl->Z.level;
				answer = get_one_output2D (GMT, S->coord[GMT_X][srow] * scl, z_level, body, n_bodies, Ctrl->F.mode, Ctrl->Z.ymin, Ctrl->Z.ymax);
				S->coord[GMT_Y][srow] = answer;
				if (answer < min_answer) min_answer = answer;
				if (answer > max_answer) max_answer = answer;
			}
		}
	}
	if (Ctrl->F.mode == TALWANI2D_GEOID) {	/* Adjust zero level */
		double off = (rho > 0.0) ? min_answer : max_answer;
		for (tbl = 0; tbl < Out->n_tables; tbl++) {
			for (seg = 0; seg < Out->table[tbl]->n_segments; seg++) {
				S = Out->table[tbl]->segment[seg];	/* Current segment */
				for (row = 0; row < S->n_rows; row++) {	/* Calculate attraction at all output locations for this segment */
					S->coord[GMT_Y][row] -= off;
				}
			}
		}
	}
	if (GMT_Write_Data (API, GMT_IS_DATASET, GMT_IS_FILE, geometry, 0, NULL, Ctrl->Out.file, Out) != GMT_OK) {
		Return (API->error);
	}
		
	/* Clean up memory */
	
 	for (k = 0; k < n_bodies; k++) {
		GMT_free (GMT, body[k].x);
		GMT_free (GMT, body[k].z);
	}
	GMT_free (GMT, body);

	Return (EXIT_SUCCESS);
}