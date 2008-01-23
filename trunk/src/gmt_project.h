/*--------------------------------------------------------------------
 *	$Id: gmt_project.h,v 1.61 2008-01-23 03:22:48 guru Exp $
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
 * Include file for programs that use the map-projections functions.  Note
 * that most programs will include this by including gmt.h
 *
 * Author:	Paul Wessel
 * Date:	26-FEB-1990
 * Revised:	24-MAR-2000
 * Revised:	18-DEC-2007
 * Version:	4.2.2
 *
 */
#ifndef _GMT_PROJECT_H
#define _GMT_PROJECT_H

#define GMT_N_PROJECTIONS	29	/* Total number of projections in GMT */

/* These numbers should remain flexible. Do not use them in any programming. Use only their symbolic names.
   However, all the first items in each section (i.e. GMT_LINEAR, GMT_MERCATOR,...) should remain the first.
*/
#define GMT_NO_PROJ		-1	/* Projection not specified (initial value) */

/* Linear projections tagged 0-99 */
#define GMT_IS_LINEAR (project_info.projection / 100 == 0)
#define GMT_LINEAR		0
#define GMT_LOG10		1	/* These numbers are only used for project_info.xyz_projection[3], */
#define GMT_POW			2	/* while project_info.projection = 0 */
#define GMT_TIME		3
#define GMT_ZAXIS		50

/* Cylindrical projections tagged 100-199 */
#define GMT_IS_CYLINDRICAL (project_info.projection / 100 == 1)
#define GMT_MERCATOR		100
#define	GMT_CYL_EQ		101
#define	GMT_CYL_EQDIST		102
#define GMT_CYL_STEREO		103
#define GMT_MILLER		104
#define GMT_TM			106
#define GMT_UTM			107
#define GMT_CASSINI		108
#define GMT_OBLIQUE_MERC	150
#define GMT_OBLIQUE_MERC_POLE	151

/* Conic projections tagged 200-299 */
#define GMT_IS_CONICAL (project_info.projection / 100 == 2)
#define GMT_ALBERS		200
#define GMT_ECONIC		201
#define GMT_LAMBERT		250

/* Azimuthal projections tagged 300-399 */
#define GMT_IS_AZIMUTHAL (project_info.projection / 100 == 3)
#define GMT_STEREO		300
#define GMT_LAMB_AZ_EQ		301
#define GMT_ORTHO		302
#define GMT_AZ_EQDIST		303
#define GMT_GNOMONIC		304
#define GMT_GENPER              305
#define GMT_POLAR		350

/* Misc projections tagged 400-499 */
#define GMT_IS_MISC (project_info.projection / 100 == 4)
#define GMT_MOLLWEIDE		400
#define GMT_HAMMER		401
#define GMT_SINUSOIDAL		402
#define GMT_VANGRINTEN		403
#define GMT_ROBINSON		404
#define GMT_ECKERT4		405
#define GMT_ECKERT6		406
#define GMT_WINKEL		407

#define GMT_IS_RECT_GRATICULE (project_info.projection <= GMT_MILLER)
#define GMT_GRID_CLIP_OK (GMT_IS_LINEAR || (GMT_IS_CYLINDRICAL && project_info.projection < GMT_OBLIQUE_MERC) || GMT_IS_CONICAL)
#define GMT_POLE_IS_POINT (project_info.projection >= GMT_LAMBERT && project_info.projection <= GMT_VANGRINTEN)

#define GMT_IS_MAPPING (project_info.degree[0] && project_info.degree[1])	/* TRUE when map projections are used */
#define GMT_IS_SPHERICAL (gmtdefs.ref_ellipsoid[gmtdefs.ellipsoid].flattening < 1.0e-10)

#define GMT_360_RANGE(w,e) (fabs (fabs((e) - (w)) - 360.0) < GMT_CONV_LIMIT)
#define GMT_180_RANGE(s,n) (fabs (fabs((n) - (s)) - 180.0) < GMT_CONV_LIMIT)
#define GMT_IS_ZERO(x) (fabs (x) < GMT_CONV_LIMIT)

#define D2R (M_PI / 180.0)
#define R2D (180.0 / M_PI)

/* UTM offsets */

#define GMT_FALSE_EASTING    500000.0
#define GMT_FALSE_NORTHING 10000000.0

/* Number of nodes in Robinson interpolation */

#define GMT_N_ROBINSON	19

#define GMT_LATSWAP_N	12	/* number of defined swaps  */

struct GMT_LATSWAP_CONSTS {
	double  c[GMT_LATSWAP_N][4];	/* Coefficients in 4-term series  */
	double	ra;			/* Authalic   radius (sphere for equal-area)  */
	double	rm;			/* Meridional radius (sphere for N-S distance)  */
	BOOLEAN spherical;		/* True if no conversions need to be done.  */
};

struct GMT_MAP_PROJECTIONS {

	double pars[10];		/* Raw unprocessed map-projection parameters as passed on command line */
	double z_pars[2];		/* Raw unprocessed z-projection parameters as passed on command line */

	/* Common projection parameters */

	int projection;			/* Gives the id number for the projection used */

	BOOLEAN x_off_supplied;		/* Used to set xorigin/yorigin for overlay/final plots */
	BOOLEAN y_off_supplied;
	BOOLEAN region_supplied;	/* TRUE when -R option has been given */
	BOOLEAN units_pr_degree;	/* TRUE if scale is given as inch (or cm)/degree.  FALSE for 1:xxxxx */
	BOOLEAN region;			/* TRUE if -R gives lon/lat boundaries, FALSE if it gives lower/left upper right corners */
	BOOLEAN north_pole;		/* TRUE if projection is on northern hemisphere, FALSE on southern */
	BOOLEAN edge[4];		/* TRUE if the edge is a map boundary */
	BOOLEAN three_D;		/* Parameters for 3-D projections */
	BOOLEAN GMT_convert_latitudes;	/* TRUE if using spherical code with authalic/conformal latitudes */
	struct GMT_LATSWAP_CONSTS GMT_lat_swap_vals;

	double x0, y0, z0;		/* Projected values of the logical origin for the projection */
	double xmin, xmax, ymin, ymax, zmin, zmax;	/* Extreme projected values */
	double w, e, s, n;		/* Bounding geographical region, if applicable */
	double z_bottom, z_top;		/* Bounds in z direction */
	double x_scale, y_scale;	/* Scaling for meters to map-distance (typically inch) conversion */
	double z_scale;			/* Scale for z projection (user-units to map-units (typically inch) */
	double i_x_scale, i_y_scale;	/* Inverse Scaling for meters to map-distance (typically inch) conversion */
	double i_z_scale;		/* Inverse Scale for z projection (user-units to map-units (typically inch) */
	double z_level;			/* Level at which to draw basemap [0] */
	double unit;			/* Gives meters pr plot unit (0.01 or 0.0254) */
	double central_meridian;	/* Central meridian for projection */
	double pole;			/* +90 pr -90, depending on which pole */
	double EQ_RAD, i_EQ_RAD;	/* Current ellipsoid parameters */
	double ECC, ECC2, ECC4, ECC6;	/* Powers of eccentricity */
	double M_PR_DEG, KM_PR_DEG;	/* Current spherical approximations to convert degrees to dist */
	double half_ECC, i_half_ECC;	/* 0.5 * ECC and 0.5 / ECC */
	double one_m_ECC2, i_one_m_ECC2; /* 1.0 - ECC2 and inverse */
	int gave_map_width;		/* nonzero if map width (1), height (2), max dim (3) or min dim (4) is given instead of scale.  0 for 1:xxxxx */

	double f_horizon, rho_max;	/* Azimuthal horizon (deg) and in plot coordinates */

	/* Linear plot parameters */

	int xyz_projection[3];		/* For linear projection, 0 = linear, 1 = log10, 2 = pow */
	BOOLEAN xyz_pos[3];		/* TRUE if x,y,z-axis increases in normal positive direction */
	BOOLEAN compute_scale[3];	/* TRUE if axes lengths were set rather than scales */
	BOOLEAN degree[3];		/* TRUE if we have linear projection with geographical data */
	double xyz_pow[3];		/* For GMT_POW projection */
	double xyz_ipow[3];

	/* Center of radii for all conic projections */

	double c_x0, c_y0;

	/* Lambert conformal conic parameters. */

	double l_N, l_i_N, l_Nr, l_i_Nr;
	double l_F, l_rF, l_i_rF;
	double l_rho0;

	/* Oblique Mercator Projection (Spherical version )*/

	double o_sin_pole_lat, o_cos_pole_lat;	/* Pole of rotation */
	double o_pole_lon;	/* In Radians */
	double o_pole_lat;	/* In Radians */
	double o_beta;		/* lon' = beta for central_meridian (In Radians) */
	double o_FP[3], o_FC[3], o_IP[3], o_IC[3];

	/* TM and UTM Projections */

	double t_lat0;
	double t_e2, t_M0;
	double t_c1, t_c2, t_c3, t_c4;
	double t_i1, t_i2, t_i3, t_i4, t_i5;
	double t_r, t_ir;		/* Short for project_info.EQ_RAD * gmtdefs.map_scale_factor and its inverse */
	int utm_hemisphere;	/* -1 for S, +1 for N, 0 if to be set by -R */
	int utm_zonex;		/* The longitude component 1-60 */
	int utm_zoney;		/* The latitude component A-Z */

	/* Lambert Azimuthal Equal-Area Projection */

	double sinp;
	double cosp;
	double Dx, Dy, iDx, iDy;	/* Fudge factors for projections w/authalic lats */

	/* Stereographic Projection */

	double s_c, s_ic;
	double r;		/* Radius of projected sphere in plot units (inch or cm) */
	BOOLEAN polar;		/* True if projection pole coincides with S or N pole */

	/* Mollweide, Hammer-Aitoff and Winkel Projection */

	double w_x, w_y, w_iy, w_r;

	/* Winkel Tripel Projection */

	double r_cosphi1;	/* = cos (50.467) */

	/* Robinson Projection */

	double n_cx, n_cy;	/* = = 0.8487R, 1.3523R */
	double n_i_cy;
	double n_phi[GMT_N_ROBINSON], n_X[GMT_N_ROBINSON], n_Y[GMT_N_ROBINSON];
	double *n_x_coeff, *n_y_coeff, *n_iy_coeff;

	/* Eckert IV Projection */

	double k4_x, k4_y, k4_ix, k4_iy;

	/* Eckert VI Projection */

	double k6_r, k6_ir;

	/* Cassini Projection */

	double c_M0, c_c1, c_c2, c_c3, c_c4;
	double c_i1, c_i2, c_i3, c_i4, c_i5, c_p;

	/* All Cylindrical Projections */

	double j_x, j_y, j_ix, j_iy;

	/* Albers Equal-area conic parameters. */

	double a_n, a_i_n;
	double a_C, a_n2ir2, a_test, a_Cin;
	double a_rho0;

	/* Equidistant conic parameters. */

	double d_n, d_i_n;
	double d_G, d_rho0;

	/* Van der Grinten parameters. */

	double v_r, v_ir;

        /* General Perspective parameters */
        double g_H, g_R;
        double g_P, g_P_inverse;
        double g_lon0;
        double g_sphi1, g_cphi1;
        double g_phig, g_sphig, g_cphig;
        double g_sdphi, g_cdphi;
        double g_B, g_D, g_L, g_G, g_J;
        double g_BLH, g_DG, g_BJ, g_DHJ, g_LH2, g_HJ;
        double g_sin_tilt, g_cos_tilt;
        double g_azimuth, g_sin_azimuth, g_cos_azimuth;
        double g_sin_twist, g_cos_twist;
        double g_width;
        double g_yoffset;
        double g_rmax;
        double g_max_yt;
        double g_xmin, g_xmax;
        double g_ymin, g_ymax;

        int g_debug;
        BOOLEAN g_box, g_outside, g_longlat_set, g_sphere, g_radius, g_auto_twist;

	/* Polar (cylindrical) projection */

	double p_base_angle;
	BOOLEAN got_azimuths, got_elevations, z_down;

};

#define GMT_IS_PLAIN	0	/* Plain baseframe */
#define GMT_IS_FANCY	1	/* Fancy baseframe */
#define GMT_IS_ROUNDED	2	/* Rounded, fancy baseframe */

/* Define the 6 axis items that each axis can have (some are mutually exclusive: only one ANNOT/INTV for upper and lower) */

#define GMT_ANNOT_UPPER		0	/* Tick annotations closest to the axis (the only kind in GMT3.4 or earlier) */
#define GMT_ANNOT_LOWER		1	/* Tick annotations farthest from the axis*/
#define GMT_INTV_UPPER		2	/* Interval annotations closest to the axis */
#define GMT_INTV_LOWER		3	/* Interval annotations farthest from the axis */
#define GMT_TICK_UPPER		4	/* Frame tick marks closest to the axis (the only kind in GMT3.4 or earlier) */
#define GMT_TICK_LOWER		5	/* Frame tick marks closest to the axis (the only kind in GMT3.4 or earlier) */
#define GMT_GRID_UPPER		6	/* Gridline spacing */
#define GMT_GRID_LOWER		7	/* Gridline spacing */

/* Some convenient macros for axis routines */

#define GMT_interval_axis_item(k) (((k) == GMT_INTV_UPPER || (k) == GMT_INTV_LOWER) ? TRUE : FALSE)	/* TRUE for interval annotations */
#define GMT_lower_axis_item(k) (((k) == GMT_ANNOT_LOWER || (k) == GMT_INTV_LOWER) ? 1 : 0)		/* 1 if this is a lower axis annotation */
#define GMT_upper_and_lower_items(j) (((frame_info.axis[j].item[GMT_ANNOT_UPPER].active || frame_info.axis[j].item[GMT_INTV_UPPER].active) && \
	(frame_info.axis[j].item[GMT_ANNOT_LOWER].active || frame_info.axis[j].item[GMT_INTV_LOWER].active)) ? TRUE : FALSE)	/* TRUE if we have two levels of annotations (tick or interval) */
#define GMT_two_annot_items(j) ((frame_info.axis[j].item[GMT_ANNOT_UPPER].active && frame_info.axis[j].item[GMT_ANNOT_LOWER].active) ? TRUE : FALSE)	/* TRUE if we have two levels of tick annotations */
#define GMT_uneven_interval(unit) ((unit == 'o' || unit == 'O' || unit == 'k' || unit == 'K' || unit == 'R' || unit == 'r' || unit == 'D' || unit == 'd') ? TRUE : FALSE)	/* TRUE for uneven units */

struct GMT_PLOT_AXIS_ITEM {		/* Information for one type of tick/annotation */
	int parent;			/* Id of axis this item belongs to (0,1,2) */
	int id;				/* Id of this item (0-7) */
	BOOLEAN active;			/* TRUE if we want to use this item */
	double interval;		/* Distance between ticks in user units */
	int flavor;			/* Index into month/day name abbreviation array (0-2) */
	BOOLEAN upper_case;		/* TRUE if we want upper case text (used with flavor) */
	char type;			/* One of a, i, A, I, f, F, g, G */
	char unit;			/* User's interval unit (y, M, u, d, h, m, c) */
};

struct GMT_PLOT_AXIS {		/* Information for one time axis */
	struct GMT_PLOT_AXIS_ITEM item[8];	/* see above defines for which is which */
	int type;			/* GMT_LINEAR, GMT_LOG10, GMT_POW, or GMT_TIME */
	double phase;			/* Phase offset for strides: (knot-phase)%interval = 0  */
	char label[GMT_LONG_TEXT];	/* Label of the axis */
	char unit[GMT_TEXT_LEN];	/* Axis unit appended to annotations */
	char prefix[GMT_TEXT_LEN];	/* Axis prefix starting all annotations */
};

struct GMT_PLOT_FRAME {		/* Various parameters for plotting of time axis boundaries */
	struct GMT_PLOT_AXIS axis[3];	/* One each for x, y, and z */
	char header[GMT_LONG_TEXT];	/* Plot title */
	BOOLEAN plotted_header;		/* TRUE if header has been plotted */
	BOOLEAN plot;			/* TRUE if -B was used */
	BOOLEAN draw_box;		/* TRUE is a 3-D Z-box is desired */
	BOOLEAN check_side;		/* TRUE if lon and lat annotations should be on x and y axis only */
	BOOLEAN horizontal;		/* TRUE is S/N annotations should be parallel to axes */
	int side[5];			/* Which sides to plot. 2 is annot/draw, 1 is draw, 0 is not */
};


struct GMT_THREE_D {
	double view_azimuth, view_elevation;
	double cos_az, sin_az, cos_el, sin_el;
	double corner_x[4], corner_y[4];
	double xmin, xmax, ymin, ymax;
	double phi[3];		/* Angle each axis makes with horizontal */
	double xshrink[3];	/* Shrinkage in x-dir due to projection */
	double yshrink[3];	/* Same for y-dir */
	double tilt[3];		/* Slant of characters due to projection */
	double sign[4];		/* Used to determine direction of tickmarks etc */
	int quadrant;		/* quadrant we're looking from */
	int z_axis;		/* Which z-axis to draw. */
	int k;			/* For drawing-axis. 0 = plot in x dir, 1 in y */
	int face[3];		/* Tells if this facet has normal in pos direction */
	int draw[4];		/* axes to draw */
};

struct GMT_DATUM {	/* Main parameter for a particular datum */
	double a, b, f, e_squared, ep_squared;
	double xyz[3];
	int ellipsoid_id;	/* Ellipsoid GMT ID number */
};

struct GMT_DATUM_CONV {
	BOOLEAN h_given;	/* TRUE if we have incoming height data [h = 0] */
	double da;		/* Major semi-axis in meters */
	double df;		/* Flattening */
	double e_squared;	/* Eccentricity squared (e^2 = 2*f - f*f) */
	double one_minus_f;	/* 1 - f */
	double dxyz[3];		/* Ellipsoids offset in meter from Earth's center of mass for x,y, and z */
	struct GMT_DATUM from, to;	/* The old and new datums */
};

#endif /* _GMT_PROJECT_H */
