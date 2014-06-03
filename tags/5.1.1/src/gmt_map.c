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
 *			G M T _ M A P . C
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * GMT_map.c contains code related to generic coordinate transformation.
 * For the actual projection functions, see gmt_proj.c
 *
 *	Map Transformation Setup Routines
 *	These routines initializes the selected map transformation
 *	The names and main function are listed below
 *	NB! Note that the transformation function does not check that they are
 *	passed valid lon,lat numbers. I.e asking for log10 scaling using values
 *	<= 0 results in problems.
 *
 * The ellipsoid used is selectable by editing the gmt.conf in your
 * home directory.  If no such file, create one by running gmtdefaults.
 *
 * Usage: Initialize system by calling GMT_map_setup (separate module), and
 * then just use GMT_geo_to_xy() and GMT_xy_to_geo() functions.
 *
 * Author:	Paul Wessel
 * Date:	1-JAN-2010
 * Version:	5
 *
 *
 * PUBLIC GMT Functions include:
 *
 *	GMT_azim_to_angle :	Converts azimuth to angle on the map
 *	GMT_clip_to_map :	Force polygon points to be inside map
 *	GMT_compact_line :	Remove redundant pen movements
 *	GMT_geo_to_xy :		Generic lon/lat to x/y
 *	GMT_geo_to_xy_line :	Same for polygons
 *	GMT_geoz_to_xy :	Generic 3-D lon/lat/z to x/y
 *	GMT_grd_project :	Generalized grid projection with interpolation
 *	GMT_great_circle_dist :	Returns great circle distance in degrees
 *	GMT_img_project :	Generalized image projection with interpolation
 *	GMT_map_outside :	Generic function determines if we're outside map boundary
 *	GMT_map_path :		Return GMT_latpath or GMT_lonpath
 *	GMT_map_setup :		Initialize map projection
 *	GMT_project_init :	Initialize parameters for grid/image transformations
 *	GMT_xy_to_geo :		Generic inverse x/y to lon/lat projection
 *	GMT_xyz_to_xy :		Generic xyz to xy projection
 *	GMT_xyz_to_xy_n :	Same for an array
 *
 * Internal GMT Functions include:
 *
 *	gmt_get_origin :		Find origin of projection based on pole and 2nd point
 *	gmt_get_rotate_pole :		Find rotation pole based on two points on great circle
 *	gmt_ilinearxy :			Inverse linear projection
 *	gmt_init_three_D :		Initializes parameters needed for 3-D plots
 *	gmt_map_crossing :		Generic function finds crossings between line and map boundary
 *	GMT_latpath :			Return path between 2 points of equal latitide
 *	GMT_lonpath :			Return path between 2 points of equal longitude
 *	gmt_radial_crossing :		Determine map crossing in the Lambert azimuthal equal area projection
 *	GMT_left_boundary :		Return left boundary in x-inches
 *	gmt_linearxy :			Linear xy projection
 *	gmt_lon_inside :		Accounts for wrap-around in longitudes and checks for inside
 *	gmt_ellipse_crossing :		Find map crossings in the Mollweide projection
 *	gmt_move_to_rect :		Move an outside point straight in to nearest edge
 *	gmt_polar_outside :		Determines if a point is outside polar projection region
 *	gmt_pole_rotate_forward :	Compute positions from oblique coordinates
 *	gmt_radial_clip :		Clip path outside radial region
 *	gmt_radial_outside :		Determine if point is outside radial region
 *	gmt_radial_overlap :		Determine overlap, always true for his projection
 *	gmt_rect_clip :			Clip to rectangular region
 *	gmt_rect_crossing :		Find crossing between line and rect region
 *	gmt_rect_outside :		Determine if point is outside rect region
 *	gmt_rect_outside2 :		Determine if point is outside rect region (azimuthal proj only)
 *	gmt_rect_overlap :		Determine overlap between rect regions
 *	GMT_right_boundary :		Return x value of right map boundary
 *	gmt_xy_search :			Find xy map boundary
 *	GMT_wesn_clip:			Clip polygon to wesn boundaries
 *	gmt_wesn_crossing :		Find crossing between line and lon/lat rectangle
 *	gmt_wesn_outside :		Determine if a point is outside a lon/lat rectangle
 *	gmt_wesn_overlap :		Determine overlap between lon/lat rectangles
 *	gmt_wesn_search :		Search for extreme coordinates
 *	GMT_wrap_around_check_{x,tm} :	Check if line wraps around due to Greenwich
 *	GMT_x_to_xx :			Generic linear x projection
 *	GMT_xx_to_x :			Generic inverse linear x projection
 *	GMT_y_to_yy :			Generic linear y projection
 *	GMT_yy_to_y :			Generic inverse linear y projection
 *	GMT_z_to_zz :			Generic linear z projection
 *	GMT_zz_to_z :			Generic inverse linear z projection
 */

#include "gmt_dev.h"
#include "gmt_internals.h"

/* Basic error reporting when things go badly wrong. This Return macro can be
 * used in stead of regular return(code) to print out what the code is before
 * it returns.  We assume the GMT pointer is available in the function!
 */
#define VAR_TO_STR(arg)      #arg
#define Return(err) { GMT_Report(GMT->parent,GMT_MSG_NORMAL,"Internal Error = %s\n",VAR_TO_STR(err)); return (err);}


/* Note by P. Wessel, 18-Oct-2012:
 * In the olden days, GMT only did great circle distances.  In GMT 4 we implemented geodesic
 * distances by Rudoe's formula as given in Bomford [1971].  However, that geodesic is not
 * exactly what we wanted as it is a normal section and do not strictly follow the geodesic.
 * Other candidates are Vincenty [1975], which is widely used and Karney [2012], which is super-
 * accurate.  At this point their differences are in the micro-meter level.  For GMT 5 we have
 * now switched to the Vincenty algorithm as provided by Gerald Evenden, USGS [author of proj4],
 * which is a modified translation of the NGS algorithm and not exactly what is in proj4's geod
 * program (which Evenden thinks is inferior.)  I ran a comparison between many algorithms that
 * either were available via codes or had online calculators.  I sought the geodesic distance
 * from (0,0) to (10,10) on WGS-84; the results were (in meters):
 *
 *	GMT4 (Rudoe):		1565109.099232116
 *	proj4:			1565109.095557918
 *	vdist(0,0,10,10) [0]	1565109.09921775
 *	Karney [1]: 		1565109.09921789
 *	Vincenty [2]:		1565109.099218036
 *	NGS [3]			1565109.0992
 *
 * [0] via Joaquim Luis, supposedly Vincenty [2012]
 * [1] via online calculator at max precision http://geographiclib.sourceforge.net/cgi-bin/Geod
 * [2] downloading, compiling and running http://article.gmane.org/gmane.comp.gis.proj-4.devel/3478.
 *     This is not identical to Vincenty in proj4 but written by Evenden (proj.4 author)
 * [3] via online calculator http://www.ngs.noaa.gov/cgi-bin/Inv_Fwd/inverse2.prl. Their notes says
 *     this is Vincenty; unfortunately I cannot control the output precision.
 *
 * Based on these comparisons we decided to implement the Vincenty [2] code as given.  The older Rudoe
 * code remains in this file for reference.  The define of USE_VINCENTY below selects the new Vincenty code.
 * The choice was based on the readily available C code versus having to reimplement Karney in C.
 */

#define USE_VINCENTY 1	/* New GMT 5 behavior */

double gmt_get_angle (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2);

/* Private functions internal to gmt_map.c */

void gmt_set_oblique_pole_and_origin (struct GMT_CTRL *GMT, double plon, double plat, double olon, double olat)
{	/* The set quantities are used in GMT_obl and GMT_iobl */
	/* Get forward pole and origin vectors FP, FC */
	double P[3];
	GMT_geo_to_cart (GMT, plat, plon, GMT->current.proj.o_FP, true);	/* Set forward Cartesian pole o_FP */
	GMT_geo_to_cart (GMT, olat, olon, P, true);			/* P points to the origin  */
	GMT_cross3v (GMT, GMT->current.proj.o_FP, P, GMT->current.proj.o_FC);	/* Set forward Cartesian center o_FC */
	GMT_normalize3v (GMT, GMT->current.proj.o_FC);

	/* Get inverse pole and origin vectors FP, FC */
	GMT_obl (GMT, 0.0, M_PI_2, &plon, &plat);
	GMT_geo_to_cart (GMT, plat, plon, GMT->current.proj.o_IP, false);	/* Set inverse Cartesian pole o_IP */
	GMT_obl (GMT, 0.0, 0.0, &olon, &olat);
	GMT_geo_to_cart (GMT, olat, olon, P, false);			/* P points to origin  */
	GMT_cross3v (GMT, GMT->current.proj.o_IP, P, GMT->current.proj.o_IC);	/* Set inverse Cartesian center o_FC */
	GMT_normalize3v (GMT, GMT->current.proj.o_IC);
}

bool gmt_quickconic (struct GMT_CTRL *GMT)
{	/* Returns true if area/scale are large/small enough
	 * so that we can use spherical equations with authalic
	 * or conformal latitudes instead of the full ellipsoidal
	 * equations.
	 */

	double s, dlon, width;

	if (GMT->current.proj.gave_map_width) {	/* Gave width */
		dlon = GMT->common.R.wesn[XHI] - GMT->common.R.wesn[XLO];
		width = GMT->current.proj.pars[4] * GMT->session.u2u[GMT->current.setting.proj_length_unit][GMT_M];	/* Convert to meters */
		s = (dlon * GMT->current.proj.M_PR_DEG) / width;
	}
	else if (GMT->current.proj.units_pr_degree) {	/* Gave scale */
		/* Convert to meters */
		s = GMT->current.proj.M_PR_DEG / (GMT->current.proj.pars[4] * GMT->session.u2u[GMT->current.setting.proj_length_unit][GMT_M]);
	}
	else {	/* Got 1:xxx that was changed */
		s = (1.0 / GMT->current.proj.pars[4]) / GMT->current.proj.unit;
	}

	if (s > 1.0e7) {	/* if s in 1:s exceeds 1e7 we do the quick thing */
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Warning: Using spherical projection with conformal latitudes\n");
		return (true);
	}
	else /* Use full ellipsoidal terms */
		return (false);
}

bool gmt_quicktm (struct GMT_CTRL *GMT, double lon0, double limit)
{	/* Returns true if the region chosen is too large for the
	 * ellipsoidal series to be valid; hence use spherical equations
	 * with authalic latitudes instead.
	 * We let +-limit degrees from central meridian be the cutoff.
	 */

	double d_left, d_right;

	d_left  = lon0 - GMT->common.R.wesn[XLO] - 360.0;
	d_right = lon0 - GMT->common.R.wesn[XHI] - 360.0;
	while (d_left  < -180.0) d_left  += 360.0;
	while (d_right < -180.0) d_right += 360.0;
	if (fabs (d_left) > limit || fabs (d_right) > limit) {
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Warning: Using spherical projection with authalic latitudes\n");
		return (true);
	}
	else /* Use full ellipsoidal terms */
		return (false);
}

void gmt_set_polar (struct GMT_CTRL *GMT)
{
	/* Determines if the projection pole is N or S pole */

	if (doubleAlmostEqual (fabs (GMT->current.proj.pars[1]), 90.0)) {
		GMT->current.proj.polar = true;
		GMT->current.proj.north_pole = (GMT->current.proj.pars[1] > 0.0);
	}
}

void gmt_cyl_validate_clon (struct GMT_CTRL *GMT, unsigned int mode) {
	/* Make sure that for global (360-range) cylindrical projections, the central meridian is neither west nor east.
	 * If so then we reset it to the middle value or we change -R:
	 * mode == 0: <clon> should be reset based on w/e mid-point
	 * mode == 1: -J<clon> is firm so w/e is centered on <c.lon>
	 */
	if (GMT_is_dnan (GMT->current.proj.pars[0]))
		GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);	/* Not set at all, set to middle lon */
	else if (GMT->current.map.is_world && (GMT->current.proj.pars[0] == GMT->common.R.wesn[XLO] || GMT->current.proj.pars[0] == GMT->common.R.wesn[XHI])) {
		/* Reset central meridian since cannot be 360 away from one of the boundaries since that gives xmin == xmax below */
		if (mode == 1) {	/* Change -R to fit central meridian */
			double w = GMT->current.proj.pars[0] - 180.0, e = GMT->current.proj.pars[0] + 180.0;
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Warning: Region for global cylindrical projection had to be reset from %g/%g to %g/%g\n",
				GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], w, e);
			GMT->common.R.wesn[XLO] = w;	GMT->common.R.wesn[XHI] = e;
		}
		else {	/* Change central meridian to fit -R */
			double new_lon = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Warning: Central meridian for global cylindrical projection had to be reset from %g to %g\n", GMT->current.proj.pars[0], new_lon);
			GMT->current.proj.pars[0] = new_lon;
		}
	}
}

void gmt_lat_swap_init (struct GMT_CTRL *GMT)
{
	/* Initialize values in GMT->current.proj.GMT_lat_swap_vals based on GMT->current.proj.

	First compute GMT->current.proj.GMT_lat_swap_vals.ra (and rm), the radii to use in
	spherical formulae for area (respectively, N-S distance) when
	using the authalic (respectively, meridional) latitude.

	Then for each type of swap:
	First load GMT->current.proj.GMT_lat_swap_vals.c[itype][k], k=0,1,2,3 with the
	coefficient for sin(2 (k +1) phi), based on series from Adams.
	Next reshuffle these coefficients so they form a nested
	polynomial using equations (3-34) and (3-35) on page 19 of
	Snyder.

	References:  J. P. Snyder, "Map projections - a working manual",
	U. S. Geological Survey Professional Paper #1395, 1987.
	O. S. Adams, "Latitude Developments Connected With Geodesy and
	Cartography", U. S. Coast and Geodetic Survey Special Publication
	number 67, 1949.
	P. D. Thomas, "Conformal Projections in Geodesy and Cartography",
	US CGS Special Pub #251, 1952.
	See also other US CGS Special Pubs (#53, 57, 68, 193, and 251).

	Latitudes are named as follows (this only partly conforms to
	names in the literature, which are varied):

	Geodetic   = G, angle between ellipsoid normal and equator
	geocentric = O, angle between radius from Origin and equator
	Parametric = P, angle such that x=a*cos(phi), y=b*sin(phi) is on ellipse
	Authalic   = A, angle to use in equal area   development of ellipsoid
	Conformal  = GMT, angle to use in conformal    development of ellipsoid
	Meridional = M, angle to use in N-S distance calculation of ellipsoid

	(The parametric latitude is the one used in orthogonal curvilinear
	coordinates and ellipsoidal harmonics.  The term "authalic" was coined
	by A. Tissot in "Memoire sur la Representations des Surfaces et les
	projections des cartes geographiques", Gauthier Villars, Paris, 1881;
	it comes from the Greek meaning equal area.)

	The idea of latitude swaps is this:  Conformal, equal-area, and other
	developments of spherical surfaces usually lead to analytic formulae
	for the forward and inverse projections which are stable over a wide
	range of the values.  It is handy to use the same formulae when
	developing the surface of the ellipsoid.  The authalic (respectively,
	conformal) lat is such that when plugged into a spherical development
	formula, it results in an authalic (meaning equal area) (respectively,
	conformal) development of the ellipsoid.  The meridional lat does the
	same thing for measurement of N-S distances along a meridian.

	Adams gives coefficients for series in sin(2 (k +1) phi), for k
	up to 2 or 3.  I have extended these to k=3 in all cases except
	the authalic.  I have sometimes multiplied his coefficients by
	(-1) so that the sense here is always to give a correction to be
	added to the input lat to get the output lat in GMT_lat_swap().

	I have tested this code by checking that
		fabs (geocentric) < fabs (parametric) < fabs (geodetic)
	and also, for each pair of possible conversions, that the
	forward followed by the inverse returns the original lat to
	within a small tolerance.  This tolerance is as follows:

	geodetic <-> authalic:      max error (degrees) = 1.253344e-08
	geodetic <-> conformal:     max error (degrees) = 2.321796e-07  should be better after 13nov07 fix
	geodetic <-> meridional:    max error (degrees) = 4.490630e-12
	geodetic <-> geocentric:    max error (degrees) = 1.350031e-13
	geodetic <-> parametric:    max error (degrees) = 1.421085e-14
	geocentric <-> parametric:  max error (degrees) = 1.421085e-14

	Currently, (GMT v5) the only ones we anticipate using are
	geodetic, authalic, and conformal.  I have put others in here
	for possible future convenience.

	Also, I made this depend on GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid]
	rather than on GMT->current.proj, so that it will be possible to
	call GMT_lat_swap() without having to pass -R and -J to
	GMT_map_setup(), so that in the future we will be able to use
	lat conversions without plotting maps.

	W H F Smith, 10--13 May 1999.   */
	
	/* PW notes: Projections only convert latitudes if GMT->current.proj.GMT_convert_latitudes is true.
	 *       This is set by GMT_mapsetup if the ellipsoid is not a sphere.  Calling gmt_lat_swap_init by itself
	 *	 does not affect the mapping machinery.  Since various situations call for the use
	 *	 of auxilliary latitudes we initialize gmt_lat_swap_init in GMT_begin.  This means
	 *	 programs can use functions like GMT_lat_swap whenever needed.
	 */

	unsigned int i;
	double x, xx[4], a, f, e2, e4, e6, e8;

	f = GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].flattening;
	a = GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].eq_radius;

	if (GMT_IS_ZERO (f)) {
		GMT_memset (GMT->current.proj.GMT_lat_swap_vals.c, GMT_LATSWAP_N * 4, double);
		GMT->current.proj.GMT_lat_swap_vals.ra = GMT->current.proj.GMT_lat_swap_vals.rm = a;
		GMT->current.proj.GMT_lat_swap_vals.spherical = true;
		return;
	}
	GMT->current.proj.GMT_lat_swap_vals.spherical = false;

	/* Below are two sums for x to get the two radii.  I have nested the
	parentheses to add the terms in the order that would minimize roundoff
	error.  However, in double precision there may be no need to do this.
	I have carried these to 4 terms (eccentricity to the 8th power) because
	this is as far as Adams goes with anything, but it is not clear what
	the truncation error is, since every term in the sum has the same sign.  */

	e2 = f * (2.0 - f);
	e4 = e2 * e2;
	e6 = e4 * e2;
	e8 = e4 * e4;

	/* This expression for the Authalic radius comes from Adams [1949]  */
	xx[0] = 2.0 / 3.0;
	xx[1] = 3.0 / 5.0;
	xx[2] = 4.0 / 7.0;
	xx[3] = 5.0 / 9.0;
	x = xx[0] * e2 + ( xx[1] * e4 + ( xx[2] * e6 + xx[3] * e8));
	GMT->current.proj.GMT_lat_swap_vals.ra = a * sqrt( (1.0 + x) * (1.0 - e2));

	/* This expression for the Meridional radius comes from Gradshteyn and Ryzhik, 8.114.1,
	because Adams only gets the first two terms.  This can be worked out by expressing the
	meridian arc length in terms of an integral in parametric latitude, which reduces to
	equatorial radius times Elliptic Integral of the Second Kind.  Expanding this using
	binomial theorem leads to Gradshteyn and Ryzhik's expression:  */
	xx[0] = 1.0 / 4.0;
	xx[1] = xx[0] * 3.0 / 16.0;
	xx[2] = xx[1] * 3.0 * 5.0 / 36.0;
	xx[3] = xx[2] * 5.0 * 7.0 / 64.0;
	x = xx[0] * e2 + ( xx[1] * e4 + ( xx[2] * e6 + xx[3] * e8));
	GMT->current.proj.GMT_lat_swap_vals.rm = a * (1.0 - x);


	/* Geodetic to authalic:  */
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2A][0] = -(e2 / 3.0 + (31.0 * e4 / 180.0 + 59.0 * e6 / 560.0));
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2A][1] = 17.0 * e4 / 360.0 + 61.0 * e6 / 1260;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2A][2] = -383.0 * e6 / 45360.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2A][3] = 0.0;

	/* Authalic to geodetic:  */
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_A2G][0] = e2 / 3.0 + (31.0 * e4 / 180.0 + 517.0 * e6 / 5040.0);
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_A2G][1] = 23.0 * e4 / 360.0 + 251.0 * e6 / 3780;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_A2G][2] = 761.0 * e6 / 45360.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_A2G][3] = 0.0;

	/* Geodetic to conformal:  */
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2C][0] = -(e2 / 2.0 + (5.0 * e4 / 24.0 + (3.0 * e6 / 32.0 + 281.0 * e8 / 5760.0)));
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2C][1] = 5.0 * e4 / 48.0 + (7.0 * e6 / 80.0 + 697.0 * e8 / 11520.0);
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2C][2] = -(13.0 * e6 / 480.0 + 461.0 * e8 / 13440.0);
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2C][3] = 1237.0 * e8 / 161280.0;

	/* Conformal to geodetic:  */
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_C2G][0] = e2 / 2.0 + (5.0 * e4 / 24.0 + (e6 / 12.0 + 13.0 * e8 / 360.0)) ;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_C2G][1] = 7.0 * e4 / 48.0 + (29.0 * e6 / 240.0 + 811.0 * e8 / 11520.0);
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_C2G][2] = 7.0 * e6 / 120.0 + 81.0 * e8 / 1120.0;  /* Bug fixed 13nov07 whfs */
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_C2G][3] = 4279.0 * e8 / 161280.0;


	/* The meridional and parametric developments use this parameter:  */
	x = f/(2.0 - f);		/* Adams calls this n.  It is f/(2-f), or -betaJK in my notes.  */
	xx[0] = x;			/* n  */
	xx[1] = x * x;			/* n-squared  */
	xx[2] = xx[1] * x;		/* n-cubed  */
	xx[3] = xx[2] * x;		/* n to the 4th  */

	/* Geodetic to meridional:  */
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2M][0] = -(3.0 * xx[0] / 2.0 - 9.0 * xx[2] / 16.0);
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2M][1] = 15.0 * xx[1] / 16.0 - 15.0 * xx[3] / 32.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2M][2] = -35.0 * xx[2] / 48.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2M][3] = 315.0 * xx[3] / 512.0;

	/* Meridional to geodetic:  */
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_M2G][0] = 3.0 * xx[0] / 2.0 - 27.0 * xx[2] / 32.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_M2G][1] = 21.0 * xx[1] / 16.0 - 55.0 * xx[3] / 32.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_M2G][2] = 151.0 * xx[2] / 96.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_M2G][3] = 1097.0 * xx[3] / 512.0;

	/* Geodetic to parametric equals parametric to geocentric:  */
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2P][0] = GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_P2O][0] = -xx[0];
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2P][1] = GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_P2O][1] = xx[1] / 2.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2P][2] = GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_P2O][2] = -xx[2] / 3.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2P][3] = GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_P2O][3] = xx[3] / 4.0;

	/* Parametric to geodetic equals geocentric to parametric:  */
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_P2G][0] = GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_O2P][0] = xx[0];
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_P2G][1] = GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_O2P][1] = xx[1] / 2.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_P2G][2] = GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_O2P][2] = xx[2] / 3.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_P2G][3] = GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_O2P][3] = xx[3] / 4.0;


	/* The geodetic <->geocentric use this parameter:  */
	x = 1.0 - e2;
	x = (1.0 - x)/(1.0 + x);	/* Adams calls this m.  It is e2/(2-e2), or -betaJK in my notes.  */
	xx[0] = x;			/* m  */
	xx[1] = x * x;			/* m-squared  */
	xx[2] = xx[1] * x;		/* m-cubed  */
	xx[3] = xx[2] * x;		/* m to the 4th  */

	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2O][0] = -xx[0];
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2O][1] = xx[1] / 2.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2O][2] = -xx[2] / 3.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_G2O][3] = xx[3] / 4.0;

	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_O2G][0] = xx[0];
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_O2G][1] = xx[1] / 2.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_O2G][2] = xx[2] / 3.0;
	GMT->current.proj.GMT_lat_swap_vals.c[GMT_LATSWAP_O2G][3] = xx[3] / 4.0;


	/* Now do the Snyder Shuffle:  */
	for (i = 0; i < GMT_LATSWAP_N; i++) {
		GMT->current.proj.GMT_lat_swap_vals.c[i][0] = GMT->current.proj.GMT_lat_swap_vals.c[i][0] - GMT->current.proj.GMT_lat_swap_vals.c[i][2];
		GMT->current.proj.GMT_lat_swap_vals.c[i][1] = 2.0 * GMT->current.proj.GMT_lat_swap_vals.c[i][1] - 4.0 * GMT->current.proj.GMT_lat_swap_vals.c[i][3];
		GMT->current.proj.GMT_lat_swap_vals.c[i][2] *= 4.0;
		GMT->current.proj.GMT_lat_swap_vals.c[i][3] *= 8.0;
	}
}

/* The *_outside routines return the status of the current point.
 * Status is the sum of x_status and y_status.
 *	x_status may be
 *	0	w < lon < e
 *	-1	lon == w
 *	1	lon == e
 *	-2	lon < w
 *	2	lon > e
 *	y_status may be
 *	0	s < lat < n
 *	-1	lat == s
 *	1	lat == n
 *	-2	lat < s
 *	2	lat > n
 */

bool gmt_wesn_outside (struct GMT_CTRL *GMT, double lon, double lat)
{
	/* Determine if a point (lon,lat) is outside or on the rectangular lon/lat boundaries
	 * The check GMT->current.map.lon_wrap is include since we need to consider the 360
	 * degree periodicity of the longitude coordinate.
	 * When we are making basemaps and may want to ensure that a point is
	 * slightly outside the border without having it automatically flip by
	 * 360 degrees. In that case GMT->current.map.lon_wrap will be temporarily set to false.
	 */

	if (GMT->current.map.lon_wrap) {
		while (lon < GMT->common.R.wesn[XLO] && lon + 360.0 <= GMT->common.R.wesn[XHI]) lon += 360.0;
		while (lon > GMT->common.R.wesn[XHI] && lon - 360.0 >= GMT->common.R.wesn[XLO]) lon -= 360.0;
	}

	if (GMT->current.map.on_border_is_outside && fabs (lon - GMT->common.R.wesn[XLO]) < GMT_SMALL)
		GMT->current.map.this_x_status = -1;
	else if (GMT->current.map.on_border_is_outside && fabs (lon - GMT->common.R.wesn[XHI]) < GMT_SMALL)
		GMT->current.map.this_x_status = 1;
	else if (lon < GMT->common.R.wesn[XLO])
		GMT->current.map.this_x_status = -2;
	else if (lon > GMT->common.R.wesn[XHI])
		GMT->current.map.this_x_status = 2;
	else
		GMT->current.map.this_x_status = 0;

	if (GMT->current.map.on_border_is_outside && fabs (lat - GMT->common.R.wesn[YLO]) < GMT_SMALL)
		GMT->current.map.this_y_status = -1;
	else if (GMT->current.map.on_border_is_outside && fabs (lat - GMT->common.R.wesn[YHI]) < GMT_SMALL)
		GMT->current.map.this_y_status = 1;
	else if (lat < GMT->common.R.wesn[YLO])
		GMT->current.map.this_y_status = -2;
	else if (lat > GMT->common.R.wesn[YHI])
		GMT->current.map.this_y_status = 2;
	else
		GMT->current.map.this_y_status = 0;

	return (GMT->current.map.this_x_status != 0 || GMT->current.map.this_y_status != 0);

}

bool gmt_polar_outside (struct GMT_CTRL *GMT, double lon, double lat)
{
	gmt_wesn_outside (GMT, lon, lat);

	if (!GMT->current.proj.edge[1]) GMT->current.map.this_x_status = 0;	/* 360 degrees, no edge */
	if (GMT->current.map.this_y_status < 0 && !GMT->current.proj.edge[0]) GMT->current.map.this_y_status = 0;	/* South pole enclosed */
	if (GMT->current.map.this_y_status > 0 && !GMT->current.proj.edge[2]) GMT->current.map.this_y_status = 0;	/* North pole enclosed */

	return (GMT->current.map.this_x_status != 0 || GMT->current.map.this_y_status != 0);
}

bool gmt_eqdist_outside (struct GMT_CTRL *GMT, double lon, double lat)
{
	double cc, s, c;

	lon -= GMT->current.proj.central_meridian;
	while (lon < -180.0) lon += 360.0;
	while (lon > 180.0) lon -= 360.0;
	sincosd (lat, &s, &c);
	cc = GMT->current.proj.sinp * s + GMT->current.proj.cosp * c * cosd (lon);
	if (cc < -1.0) {
		GMT->current.map.this_y_status = -1;
		GMT->current.map.this_x_status = 0;
	}
	else
		GMT->current.map.this_x_status = GMT->current.map.this_y_status = 0;
	return (GMT->current.map.this_y_status != 0);
}

bool gmt_radial_outside (struct GMT_CTRL *GMT, double lon, double lat)
{
	double dist;

	/* Test if point is more than horizon spherical degrees from origin.  For global maps, let all borders be "south" */

	GMT->current.map.this_x_status = 0;
	dist = GMT_great_circle_dist_degree (GMT, lon, lat, GMT->current.proj.central_meridian, GMT->current.proj.pole);
	if (GMT->current.map.on_border_is_outside && fabs (dist - GMT->current.proj.f_horizon) < GMT_SMALL)
		GMT->current.map.this_y_status = -1;
	else if (dist > GMT->current.proj.f_horizon)
		GMT->current.map.this_y_status = -2;
	else
		GMT->current.map.this_y_status = 0;
	return (GMT->current.map.this_y_status != 0);
}

bool GMT_cart_outside (struct GMT_CTRL *GMT, double x, double y)
{	/* Expects x,y to be projected and comparing with rectangular projected domain */
	if (GMT->current.map.on_border_is_outside && fabs (x - GMT->current.proj.rect[XLO]) < GMT_SMALL)
		GMT->current.map.this_x_status = -1;
	else if (GMT->current.map.on_border_is_outside && fabs (x - GMT->current.proj.rect[XHI]) < GMT_SMALL)
		GMT->current.map.this_x_status = 1;
	else if (x < GMT->current.proj.rect[XLO] - GMT_CONV_LIMIT)
		GMT->current.map.this_x_status = -2;
	else if (x > GMT->current.proj.rect[XHI] + GMT_CONV_LIMIT)
		GMT->current.map.this_x_status = 2;
	else
		GMT->current.map.this_x_status = 0;

	if (GMT->current.map.on_border_is_outside && fabs (y -GMT->current.proj.rect[YLO]) < GMT_SMALL)
		GMT->current.map.this_y_status = -1;
	else if (GMT->current.map.on_border_is_outside && fabs (y - GMT->current.proj.rect[YHI]) < GMT_SMALL)
		GMT->current.map.this_y_status = 1;
	else if (y < GMT->current.proj.rect[YLO] - GMT_CONV_LIMIT)
		GMT->current.map.this_y_status = -2;
	else if (y > GMT->current.proj.rect[YHI] + GMT_CONV_LIMIT)
		GMT->current.map.this_y_status = 2;
	else
		GMT->current.map.this_y_status = 0;

	return ((GMT->current.map.this_x_status != 0 || GMT->current.map.this_y_status != 0) ? true : false);
}

bool gmt_rect_outside (struct GMT_CTRL *GMT, double lon, double lat)
{
	double x, y;

	GMT_geo_to_xy (GMT, lon, lat, &x, &y);

	return (GMT_cart_outside (GMT, x, y));
}

bool gmt_rect_outside2 (struct GMT_CTRL *GMT, double lon, double lat)
{	/* For Azimuthal proj with rect borders since gmt_rect_outside may fail for antipodal points */
	if (gmt_radial_outside (GMT, lon, lat)) return (true);	/* Point > 90 degrees away */
	return (gmt_rect_outside (GMT, lon, lat));	/* Must check if inside box */
}

void gmt_x_wesn_corner (struct GMT_CTRL *GMT, double *x)
{
/*	if (fabs (fmod (fabs (*x - GMT->common.R.wesn[XLO]), 360.0)) <= GMT_SMALL)
		*x = GMT->common.R.wesn[XLO];
	else if (fabs (fmod (fabs (*x - GMT->common.R.wesn[XHI]), 360.0)) <= GMT_SMALL)
		*x = GMT->common.R.wesn[XHI]; */

	if (fabs (*x - GMT->common.R.wesn[XLO]) <= GMT_SMALL)
		*x = GMT->common.R.wesn[XLO];
	else if (fabs (*x - GMT->common.R.wesn[XHI]) <= GMT_SMALL)
		*x = GMT->common.R.wesn[XHI];
}

void gmt_y_wesn_corner (struct GMT_CTRL *GMT, double *y)
{
	if (fabs (*y - GMT->common.R.wesn[YLO]) <= GMT_SMALL)
		*y = GMT->common.R.wesn[YLO];
	else if (fabs (*y - GMT->common.R.wesn[YHI]) <= GMT_SMALL)
		*y = GMT->common.R.wesn[YHI];
}

bool gmt_is_wesn_corner (struct GMT_CTRL *GMT, double x, double y)
{	/* Checks if point is a corner */
	GMT->current.map.corner = 0;

	if (doubleAlmostEqualZero (fmod(fabs(x), 360.0), fmod(fabs(GMT->common.R.wesn[XLO]), 360.0))) {
		if (doubleAlmostEqualZero (y, GMT->common.R.wesn[YLO]))
			GMT->current.map.corner = 1;
		else if (doubleAlmostEqualZero (y, GMT->common.R.wesn[YHI]))
			GMT->current.map.corner = 4;
	}
	else if (doubleAlmostEqualZero (fmod(fabs(x), 360.0), fmod(fabs(GMT->common.R.wesn[XHI]), 360.0))) {
		if (doubleAlmostEqualZero (y, GMT->common.R.wesn[YLO]))
			GMT->current.map.corner = 2;
		else if (doubleAlmostEqualZero (y, GMT->common.R.wesn[YHI]))
			GMT->current.map.corner = 3;
	}
	return (GMT->current.map.corner > 0);
}

bool gmt_lon_inside (struct GMT_CTRL *GMT, double lon, double w, double e)
{
	while (lon < GMT->common.R.wesn[XLO]) lon += 360.0;
	while (lon > GMT->common.R.wesn[XHI]) lon -= 360.0;

	if (lon < w) return (false);
	if (lon > e) return (false);
	return (true);
}

unsigned int gmt_wesn_crossing (struct GMT_CTRL *GMT, double lon0, double lat0, double lon1, double lat1, double *clon, double *clat, double *xx, double *yy, unsigned int *sides)
{
	/* Compute all crossover points of a line segment with the rectangular lat/lon boundaries
	 * Since it may not be obvious which side the line may cross, and since in some cases the two points may be
	 * entirely outside the region but still cut through it, we first find all possible candidates and then decide
	 * which ones are valid crossings.  We may find 0, 1, or 2 intersections */

	unsigned int n = 0, i;
	double d, x0, y0;

	/* If wrapping is allowed: first bring both points between W and E boundaries,
	 * then move the western-most point east if it is further than 180 degrees away.
	 * This may cause the points to span the eastern boundary */

	if (GMT->current.map.lon_wrap) {
		while (lon0 < GMT->common.R.wesn[XLO]) lon0 += 360.0;
		while (lon0 > GMT->common.R.wesn[XHI]) lon0 -= 360.0;
		while (lon1 < GMT->common.R.wesn[XLO]) lon1 += 360.0;
		while (lon1 > GMT->common.R.wesn[XHI]) lon1 -= 360.0;
		if (fabs (lon0 - lon1) <= 180.0) { /* Nothing */ }
		else if (lon0 < lon1)
			lon0 += 360.0;
		else
			lon1 += 360.0;
	}

	/* Then set 'almost'-corners to corners */
	gmt_x_wesn_corner (GMT, &lon0);
	gmt_x_wesn_corner (GMT, &lon1);
	gmt_y_wesn_corner (GMT, &lat0);
	gmt_y_wesn_corner (GMT, &lat1);

	/* Crossing South */
	if ((lat0 >= GMT->common.R.wesn[YLO] && lat1 <= GMT->common.R.wesn[YLO]) || (lat1 >= GMT->common.R.wesn[YLO] && lat0 <= GMT->common.R.wesn[YLO])) {
		sides[n] = 0;
		clat[n] = GMT->common.R.wesn[YLO];
		d = lat0 - lat1;
		clon[n] = (doubleAlmostEqualZero (lat0, lat1)) ? lon1 : lon1 + (lon0 - lon1) * (clat[n] - lat1) / d;
		gmt_x_wesn_corner (GMT, &clon[n]);
		if (fabs (d) > 0.0 && gmt_lon_inside (GMT, clon[n], GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])) n++;
	}
	/* Crossing East */
	if ((lon0 >= GMT->common.R.wesn[XHI] && lon1 <= GMT->common.R.wesn[XHI]) || (lon1 >= GMT->common.R.wesn[XHI] && lon0 <= GMT->common.R.wesn[XHI])) {
		sides[n] = 1;
		clon[n] = GMT->common.R.wesn[XHI];
		d = lon0 - lon1;
		clat[n] = (doubleAlmostEqualZero (lon0, lon1)) ? lat1 : lat1 + (lat0 - lat1) * (clon[n] - lon1) / d;
		gmt_y_wesn_corner (GMT, &clat[n]);
		if (fabs (d) > 0.0 && clat[n] >= GMT->common.R.wesn[YLO] && clat[n] <= GMT->common.R.wesn[YHI]) n++;
	}

	/* Now adjust the longitudes so that they might span the western boundary */
	if (GMT->current.map.lon_wrap && MAX(lon0, lon1) > GMT->common.R.wesn[XHI]) {
		lon0 -= 360.0; lon1 -= 360.0;
	}

	/* Crossing North */
	if ((lat0 >= GMT->common.R.wesn[YHI] && lat1 <= GMT->common.R.wesn[YHI]) || (lat1 >= GMT->common.R.wesn[YHI] && lat0 <= GMT->common.R.wesn[YHI])) {
		sides[n] = 2;
		clat[n] = GMT->common.R.wesn[YHI];
		d = lat0 - lat1;
		clon[n] = (doubleAlmostEqualZero (lat0, lat1)) ? lon1 : lon1 + (lon0 - lon1) * (clat[n] - lat1) / d;
		gmt_x_wesn_corner (GMT, &clon[n]);
		if (fabs (d) > 0.0 && gmt_lon_inside (GMT, clon[n], GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])) n++;
	}
	/* Crossing West */
	if ((lon0 <= GMT->common.R.wesn[XLO] && lon1 >= GMT->common.R.wesn[XLO]) || (lon1 <= GMT->common.R.wesn[XLO] && lon0 >= GMT->common.R.wesn[XLO])) {
		sides[n] = 3;
		clon[n] = GMT->common.R.wesn[XLO];
		d = lon0 - lon1;
		clat[n] = (doubleAlmostEqualZero (lon0, lon1)) ? lat1 : lat1 + (lat0 - lat1) * (clon[n] - lon1) / d;
		gmt_y_wesn_corner (GMT, &clat[n]);
		if (fabs (d) > 0.0 && clat[n] >= GMT->common.R.wesn[YLO] && clat[n] <= GMT->common.R.wesn[YHI]) n++;
	}

	if (n == 0) return (0);

	for (i = 0; i < n; i++) {
		GMT_geo_to_xy (GMT, clon[i], clat[i], &xx[i], &yy[i]);
		if (GMT->current.proj.projection == GMT_POLAR && sides[i]%2) sides[i] = 4 - sides[i];	/*  toggle 1 <-> 3 */
	}

	if (n == 1) return (1);

	/* Check for corner xover if n == 2 */

	if (gmt_is_wesn_corner (GMT, clon[0], clat[0])) return (1);

	if (gmt_is_wesn_corner (GMT, clon[1], clat[1])) {
		clon[0] = clon[1];
		clat[0] = clat[1];
		xx[0] = xx[1];
		yy[0] = yy[1];
		sides[0] = sides[1];
		return (1);
	}

	/* Sort the two intermediate points into the right order based on projected distances from the first point */

	GMT_geo_to_xy (GMT, lon0, lat0, &x0, &y0);

	if (hypot (x0 - xx[1], y0 - yy[1]) < hypot (x0 - xx[0], y0 - yy[0])) {
		double_swap (clon[0], clon[1]);
		double_swap (clat[0], clat[1]);
		double_swap (xx[0], xx[1]);
		double_swap (yy[0], yy[1]);
		uint_swap (sides[0], sides[1]);
	}

	return (2);
}

void gmt_x_rect_corner (struct GMT_CTRL *GMT, double *x)
{
	if (fabs (*x) <= GMT_SMALL)
		*x = 0.0;
	else if (fabs (*x - GMT->current.proj.rect[XHI]) <= GMT_SMALL)
		*x = GMT->current.proj.rect[XHI];
}

void gmt_y_rect_corner (struct GMT_CTRL *GMT, double *y)
{
	if (fabs (*y) <= GMT_SMALL)
		*y = 0.0;
	else if (fabs (*y - GMT->current.proj.rect[YHI]) <= GMT_SMALL)
		*y = GMT->current.proj.rect[YHI];
}

bool gmt_is_rect_corner (struct GMT_CTRL *GMT, double x, double y)
{	/* Checks if point is a corner */
	GMT->current.map.corner = -1;
	if (doubleAlmostEqualZero (x, GMT->current.proj.rect[XLO])) {
		if (doubleAlmostEqualZero (y, GMT->current.proj.rect[YLO]))
			GMT->current.map.corner = 1;
		else if (doubleAlmostEqualZero (y, GMT->current.proj.rect[YHI]))
			GMT->current.map.corner = 4;
	}
	else if (doubleAlmostEqualZero (x, GMT->current.proj.rect[XHI])) {
		if (doubleAlmostEqualZero (y, GMT->current.proj.rect[YLO]))
			GMT->current.map.corner = 2;
		else if (doubleAlmostEqualZero (y, GMT->current.proj.rect[YHI]))
			GMT->current.map.corner = 3;
	}
	return (GMT->current.map.corner > 0);
}

unsigned int gmt_rect_crossing (struct GMT_CTRL *GMT, double lon0, double lat0, double lon1, double lat1, double *clon, double *clat, double *xx, double *yy, unsigned int *sides)
{
	/* Compute all crossover points of a line segment with the boundaries in a rectangular projection */

	unsigned int i, j, n = 0;
	double x0, x1, y0, y1, d;

	/* Since it may not be obvious which side the line may cross, and since in some cases the two points may be
	 * entirely outside the region but still cut through it, we first find all possible candidates and then decide
	 * which ones are valid crossings.  We may find 0, 1, or 2 intersections */

	GMT_geo_to_xy (GMT, lon0, lat0, &x0, &y0);
	GMT_geo_to_xy (GMT, lon1, lat1, &x1, &y1);

	/* First set 'almost'-corners to corners */

	gmt_x_rect_corner (GMT, &x0);
	gmt_x_rect_corner (GMT, &x1);
	gmt_y_rect_corner (GMT, &y0);
	gmt_y_rect_corner (GMT, &y1);

	if ((y0 >= GMT->current.proj.rect[YLO] && y1 <= GMT->current.proj.rect[YLO]) || (y1 >= GMT->current.proj.rect[YLO] && y0 <= GMT->current.proj.rect[YLO])) {
		sides[n] = 0;
		yy[n] = GMT->current.proj.rect[YLO];
		d = y0 - y1;
		xx[n] = (doubleAlmostEqualZero (y0, y1)) ? x0 : x1 + (x0 - x1) * (yy[n] - y1) / d;
		gmt_x_rect_corner (GMT, &xx[n]);
		if (fabs (d) > 0.0 && xx[n] >= GMT->current.proj.rect[XLO] && xx[n] <= GMT->current.proj.rect[XHI]) n++;
	}
	if ((x0 <= GMT->current.proj.rect[XHI] && x1 >= GMT->current.proj.rect[XHI]) || (x1 <= GMT->current.proj.rect[XHI] && x0 >= GMT->current.proj.rect[XHI])) {
		sides[n] = 1;
		xx[n] = GMT->current.proj.rect[XHI];
		d = x0 - x1;
		yy[n] = (doubleAlmostEqualZero (x0, x1)) ? y0 : y1 + (y0 - y1) * (xx[n] - x1) / d;
		gmt_y_rect_corner (GMT, &yy[n]);
		if (fabs (d) > 0.0 && yy[n] >= GMT->current.proj.rect[YLO] && yy[n] <= GMT->current.proj.rect[YHI]) n++;
	}
	if ((y0 <= GMT->current.proj.rect[YHI] && y1 >= GMT->current.proj.rect[YHI]) || (y1 <= GMT->current.proj.rect[YHI] && y0 >= GMT->current.proj.rect[YHI])) {
		sides[n] = 2;
		yy[n] = GMT->current.proj.rect[YHI];
		d = y0 - y1;
		xx[n] = (doubleAlmostEqualZero (y0, y1)) ? x0 : x1 + (x0 - x1) * (yy[n] - y1) / d;
		gmt_x_rect_corner (GMT, &xx[n]);
		if (fabs (d) > 0.0 && xx[n] >= GMT->current.proj.rect[XLO] && xx[n] <= GMT->current.proj.rect[XHI]) n++;
	}
	if ((x0 >= GMT->current.proj.rect[XLO] && x1 <= GMT->current.proj.rect[XLO]) || (x1 >= GMT->current.proj.rect[XLO] && x0 <= GMT->current.proj.rect[XLO])) {
		sides[n] = 3;
		xx[n] = GMT->current.proj.rect[XLO];
		d = x0 - x1;
		yy[n] = (doubleAlmostEqualZero (x0, x1)) ? y0 : y1 + (y0 - y1) * (xx[n] - x1) / d;
		gmt_y_rect_corner (GMT, &yy[n]);
		if (fabs (d) > 0.0 && yy[n] >= GMT->current.proj.rect[YLO] && yy[n] <= GMT->current.proj.rect[YHI]) n++;
	}
	
	if (n == 0) return (0);

	/* Eliminate duplicates */

	for (i = 0; i < n; ++i) {
		for (j = i + 1; j < n; ++j) {
			if (doubleAlmostEqualZero (xx[i], xx[j]) && doubleAlmostEqualZero (yy[i], yy[j]))	/* Duplicate */
				sides[j] = 99;	/* Mark as duplicate */
		}
	}
	for (i = 1; i < n; i++) {
		if (sides[i] == 99) {	/* This is a duplicate, overwrite */
			for (j = i + 1; j < n; j++) {
				xx[j-1] = xx[j];
				yy[j-1] = yy[j];
				sides[j-1] = sides[j];
			}
			n--;
			i--;	/* Must start at same point again */
		}
	}

	for (i = 0; i < n; i++)	GMT_xy_to_geo (GMT, &clon[i], &clat[i], xx[i], yy[i]);

	if (!GMT_is_geographic (GMT, GMT_IN)) return (n);

	if (n < 2) return (n);

	/* Check for corner xover if n == 2 */

	if (gmt_is_rect_corner (GMT, xx[0], yy[0])) return (1);

	if (gmt_is_rect_corner (GMT, xx[1], yy[1])) {
		clon[0] = clon[1];
		clat[0] = clat[1];
		xx[0] = xx[1];
		yy[0] = yy[1];
		sides[0] = sides[1];
		return (1);
	}

	/* Sort the two intermediate points into the right order based on projected distances from the first point */

	if (hypot (x0 - xx[1], y0 - yy[1]) < hypot (x0 - xx[0], y0 - yy[0])) {
		double_swap (clon[0], clon[1]);
		double_swap (clat[0], clat[1]);
		double_swap (xx[0], xx[1]);
		double_swap (yy[0], yy[1]);
		uint_swap (sides[0], sides[1]);
	}

	return (2);
}

unsigned int gmt_radial_crossing (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2, double *clon, double *clat, double *xx, double *yy, unsigned int *sides)
{
	/* Computes the lon/lat of a point that is f_horizon spherical degrees from
	 * the origin and lies on the great circle between points 1 and 2 */

	double dist1, dist2, delta, eps, dlon;

	dist1 = GMT_great_circle_dist_degree (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, lon1, lat1);
	dist2 = GMT_great_circle_dist_degree (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, lon2, lat2);
	delta = dist2 - dist1;
	eps = (doubleAlmostEqualZero (dist1, dist2)) ? 0.0 : (GMT->current.proj.f_horizon - dist1) / delta;
	GMT_set_delta_lon (lon1, lon2, dlon);
	clon[0] = lon1 + dlon * eps;
	clat[0] = lat1 + (lat2 - lat1) * eps;

	GMT_geo_to_xy (GMT, clon[0], clat[0], &xx[0], &yy[0]);

	sides[0] = 1;

	return (1);
}

int gmt_map_jump_x (struct GMT_CTRL *GMT, double x0, double y0, double x1, double y1)
{
	/* true if x-distance between points exceeds 1/2 map width at this y value */
	double dx, map_half_size;

	if (!(GMT_IS_CYLINDRICAL (GMT) || GMT_IS_PERSPECTIVE (GMT) || GMT_IS_MISC (GMT))) return (0);	/* Only projections with peroidic boundaries may apply */

	if (!GMT_is_geographic (GMT, GMT_IN) || fabs (GMT->common.R.wesn[XLO] - GMT->common.R.wesn[XHI]) < 90.0) return (0);

	map_half_size = MAX (GMT_half_map_width (GMT, y0), GMT_half_map_width (GMT, y1));
	if (fabs (map_half_size) < GMT_SMALL) return (0);

	dx = x1 - x0;
	if (dx > map_half_size)	return (-1);	/* Cross left/west boundary */
	if (dx < (-map_half_size)) return (1);	/* Cross right/east boundary */
	return (0);
}

#if 0
/* NOT USED ?? */
int gmt_ellipse_crossing (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2, double *clon, double *clat, double *xx, double *yy, int *sides)
{
	/* Compute the crossover point(s) on the map boundary for rectangular projections */
	int n = 0, i, jump;
	double x1, x2, y1, y2;

	/* Crossings here must be at the W or E borders. Lat points may only touch border */

	if (lat1 <= -90.0) {
		sides[n] = 0;
		clon[n] = lon1;
		clat[n] = lat1;
		n = 1;
	}
	else if (lat2 <= -90.0) {
		sides[n] = 0;
		clon[n] = lon2;
		clat[n] = lat2;
		n = 1;
	}
	else if (lat1 >= 90.0) {
		sides[n] = 2;
		clon[n] = lon1;
		clat[n] = lat1;
		n = 1;
	}
	else if (lat2 >= 90.0) {
		sides[n] = 2;
		clon[n] = lon2;
		clat[n] = lat2;
		n = 1;
	}
	else {	/* May cross somewhere else */
		GMT_geo_to_xy (GMT, lon1, lat1, &x1, &y1);
		GMT_geo_to_xy (GMT, lon2, lat2, &x2, &y2);
		if ((jump = gmt_map_jump_x (GMT, x2, y2, x1, y1))) {
			(*GMT->current.map.get_crossings) (GMT, xx, yy, x2, y2, x1, y1);
			if (jump == 1) {	/* Add right border point first */
				double_swap (xx[0], xx[1]);
				double_swap (yy[0], yy[1]);
			}
			GMT_xy_to_geo (GMT, &clon[0], &clat[0], xx[0], yy[0]);
			GMT_xy_to_geo (GMT, &clon[1], &clat[1], xx[1], yy[1]);
			n = 2;
		}
	}
	if (n == 1) for (i = 0; i < n; i++) GMT_geo_to_xy (GMT, clon[i], clat[i], &xx[i], &yy[i]);
	return (n);
}

/* NOT USED ?? */
int GMT_eqdist_crossing (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2, double *clon, double *clat, double *xx, double *yy, int *sides)
{
	double angle, x, y, s, c;

	/* Computes the x.y of the antipole point that lies on a radius from
	 * the origin through the inside point */

	if (gmt_eqdist_outside (GMT, lon1, lat1)) {	/* Point 1 is on perimeter */
		GMT_geo_to_xy (GMT, lon2, lat2, &x, &y);
		angle = d_atan2 (y - GMT->current.proj.origin[GMT_Y], x - GMT->current.proj.origin[GMT_X]);
		sincos (angle, &s, &c);
		xx[0] = GMT->current.proj.r * c + GMT->current.proj.origin[GMT_X];
		yy[0] = GMT->current.proj.r * s + GMT->current.proj.origin[GMT_Y];
		clon[0] = lon1;
		clat[0] = lat1;
	}
	else {	/* Point 2 is on perimeter */
		GMT_geo_to_xy (GMT, lon1, lat1, &x, &y);
		angle = d_atan2 (y - GMT->current.proj.origin[GMT_Y], x - GMT->current.proj.origin[GMT_X]);
		sincos (angle, &s, &c);
		xx[0] = GMT->current.proj.r * c + GMT->current.proj.origin[GMT_X];
		yy[0] = GMT->current.proj.r * s + GMT->current.proj.origin[GMT_Y];
		clon[0] = lon2;
		clat[0] = lat2;
	}
	sides[0] = 1;

	return (1);
}
#endif

/*  Routines to do with clipping */

unsigned int gmt_map_crossing (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2, double *xlon, double *xlat, double *xx, double *yy, unsigned int *sides)
{
	if (GMT->current.map.prev_x_status == GMT->current.map.this_x_status && GMT->current.map.prev_y_status == GMT->current.map.this_y_status) {
		/* This is naive. We could have two points outside with a line drawn between crossing the plotting area. */
		return (0);
	}
	else if ((GMT->current.map.prev_x_status == 0 && GMT->current.map.prev_y_status == 0) || (GMT->current.map.this_x_status == 0 && GMT->current.map.this_y_status == 0)) {
		/* This is a crossing */
	}
	else if (!(*GMT->current.map.overlap) (GMT, lon1, lat1, lon2, lat2))	/* Less clearcut case, check for overlap */
		return (0);

	/* Now compute the crossing */

	GMT->current.map.corner = -1;
	return ((*GMT->current.map.crossing) (GMT, lon1, lat1, lon2, lat2, xlon, xlat, xx, yy, sides));
}

uint64_t GMT_clip_to_map (struct GMT_CTRL *GMT, double *lon, double *lat, uint64_t np, double **x, double **y)
{
	/* This routine makes sure that all points are either inside or on the map boundary
	 * and returns the number of points to be used for plotting (in x,y units) */

	uint64_t i, out, n;
	uint64_t total_nx = 0;
	int64_t out_x, out_y, np2;
	bool polygon;
	double *xx = NULL, *yy = NULL;

	/* First check for trivial cases:  All points outside or all points inside */

	for (i = out = out_x = out_y = 0; i < np; i++)  {
		(void) GMT_map_outside (GMT, lon[i], lat[i]);
		out_x += GMT->current.map.this_x_status;	/* Completely left of west gives -2 * np, right of east gives + 2 * np */
		out_y += GMT->current.map.this_y_status;	/* Completely below south gives -2 * np, above north gives + 2 * np */
		out += (abs (GMT->current.map.this_x_status) == 2 || abs (GMT->current.map.this_y_status) == 2);
	}
	if (out == 0) {		/* All points are inside map boundary; no clipping required */
		GMT_malloc2 (GMT, xx, yy, np, NULL, double);
		for (i = 0; i < np; i++) GMT_geo_to_xy (GMT, lon[i], lat[i], &xx[i], &yy[i]);
		*x = xx;	*y = yy;	n = np;
	}
	else if (out == np) {	/* All points are outside map boundary */
		np2 = 2 * np;
		if (int64_abs (out_x) == np2 || int64_abs (out_y) == np2)	/* All points safely outside the region, no part of polygon survives */
			n = 0;
		else {	/* All points are outside, but they are not just to one side so lines _may_ intersect the region */
			n = (*GMT->current.map.clip) (GMT, lon, lat, np, x, y, &total_nx);
			polygon = !GMT_polygon_is_open (GMT, lon, lat, np);	/* The following can only be used on closed polygons */
			/* Polygons that completely contains the -R region will not generate crossings, just duplicate -R box */
			if (polygon && n > 0 && total_nx == 0) {	/* No crossings and all points outside means one of two things: */
				/* Either the polygon contains portions of the -R region including corners or it does not.  We pick the corners and check for insidedness: */
				bool ok = false;
				if (GMT_non_zero_winding (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], lon, lat, np)) ok = true;		/* true if inside */
				if (!ok && GMT_non_zero_winding (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], lon, lat, np)) ok = true;	/* true if inside */
				if (!ok && GMT_non_zero_winding (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], lon, lat, np)) ok = true;	/* true if inside */
				if (!ok && GMT_non_zero_winding (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YHI], lon, lat, np)) ok = true;	/* true if inside */
				if (!ok) {
					/* Polygon does NOT contain the region and we delete it */
					n = 0;
					if (*x) GMT_free (GMT, *x);
					if (*y) GMT_free (GMT, *y);
				}
				/* Otherwise the polygon completely contains -R and we pass it along */
			}
			else if (GMT->common.R.oblique && GMT->current.proj.projection == GMT_AZ_EQDIST && n <= 5 && !strncmp (GMT->init.module_name, "pscoast", 7U)) {
				/* Special check for -JE where a coastline block is completely outside yet fully surrounds the rectangular -R -JE...r region.
				   This results in a rectangular closed polygon after the clipping. */
				n = 0;
				if (*x) GMT_free (GMT, *x);
				if (*y) GMT_free (GMT, *y);
			}
		}
	}
	else	/* Mixed case so we must clip the polygon */
		n = (*GMT->current.map.clip) (GMT, lon, lat, np, x, y, &total_nx);

	return (n);
}

double gmt_x_to_corner (struct GMT_CTRL *GMT, double x) {
	return ( (fabs (x - GMT->current.proj.rect[XLO]) < fabs (x - GMT->current.proj.rect[XHI])) ? GMT->current.proj.rect[XLO] : GMT->current.proj.rect[XHI]);
}

double gmt_y_to_corner (struct GMT_CTRL *GMT, double y) {
	return ( (fabs (y - GMT->current.proj.rect[YLO]) < fabs (y - GMT->current.proj.rect[YHI])) ? GMT->current.proj.rect[YLO] : GMT->current.proj.rect[YHI]);
}

uint64_t gmt_move_to_rect (struct GMT_CTRL *GMT, double *x_edge, double *y_edge, uint64_t j, uint64_t nx)
{
	uint64_t n = 0;
	int key;
	double xtmp, ytmp;

	/* May add 0, 1, or 2 points to path */

	if (GMT->current.map.this_x_status == 0 && GMT->current.map.this_y_status == 0) return (1);	/* Completely Inside */

	if (!nx && j > 0 && GMT->current.map.this_x_status != GMT->current.map.prev_x_status && GMT->current.map.this_y_status != GMT->current.map.prev_y_status) {	/* Must include corner */
		xtmp = x_edge[j];	ytmp = y_edge[j];
		if ((GMT->current.map.this_x_status * GMT->current.map.prev_x_status) == -4 || (GMT->current.map.this_y_status * GMT->current.map.prev_y_status) == -4) {	/* the two points outside on opposite sides */
			x_edge[j] = (GMT->current.map.prev_x_status < 0) ? GMT->current.proj.rect[XLO] : ((GMT->current.map.prev_x_status > 0) ? GMT->current.proj.rect[XHI] : gmt_x_to_corner (GMT, x_edge[j-1]));
			y_edge[j] = (GMT->current.map.prev_y_status < 0) ? GMT->current.proj.rect[YLO] : ((GMT->current.map.prev_y_status > 0) ? GMT->current.proj.rect[YHI] : gmt_y_to_corner (GMT, y_edge[j-1]));
			j++;
			x_edge[j] = (GMT->current.map.this_x_status < 0) ? GMT->current.proj.rect[XLO] : ((GMT->current.map.this_x_status > 0) ? GMT->current.proj.rect[XHI] : gmt_x_to_corner (GMT, xtmp));
			y_edge[j] = (GMT->current.map.this_y_status < 0) ? GMT->current.proj.rect[YLO] : ((GMT->current.map.this_y_status > 0) ? GMT->current.proj.rect[YHI] : gmt_y_to_corner (GMT, ytmp));
			j++;
		}
		else {
			key = MIN (GMT->current.map.this_x_status, GMT->current.map.prev_x_status);
			x_edge[j] = (key < 0) ? GMT->current.proj.rect[XLO] : GMT->current.proj.rect[XHI];
			key = MIN (GMT->current.map.this_y_status, GMT->current.map.prev_y_status);
			y_edge[j] = (key < 0) ? GMT->current.proj.rect[YLO] : GMT->current.proj.rect[YHI];
			j++;
		}
		x_edge[j] = xtmp;	y_edge[j] = ytmp;
		n = 1;
	}

	if (GMT->current.map.outside == gmt_rect_outside2) {	/* Need special check because this outside2 test is screwed up... */
		if (x_edge[j] < GMT->current.proj.rect[XLO]) {
			x_edge[j] = GMT->current.proj.rect[XLO];
			GMT->current.map.this_x_status = -2;
		}
		else if (x_edge[j] > GMT->current.proj.rect[XHI]) {
			x_edge[j] = GMT->current.proj.rect[XHI];
			GMT->current.map.this_x_status = 2;
		}
		if (y_edge[j] < GMT->current.proj.rect[YLO]) {
			y_edge[j] = GMT->current.proj.rect[YLO];
			GMT->current.map.this_y_status = -2;
		}
		else if (y_edge[j] > GMT->current.proj.rect[YHI]) {
			y_edge[j] = GMT->current.proj.rect[YHI];
			GMT->current.map.this_y_status = 2;
		}
	}
	else {
		if (GMT->current.map.this_x_status != 0) x_edge[j] = (GMT->current.map.this_x_status < 0) ? GMT->current.proj.rect[XLO] : GMT->current.proj.rect[XHI];
		if (GMT->current.map.this_y_status != 0) y_edge[j] = (GMT->current.map.this_y_status < 0) ? GMT->current.proj.rect[YLO] : GMT->current.proj.rect[YHI];
	}

	return (n + 1);
}

uint64_t gmt_rect_clip_old (struct GMT_CTRL *GMT, double *lon, double *lat, uint64_t n, double **x, double **y, uint64_t *total_nx)
{
	uint64_t i, j = 0;
	unsigned int nx, k, sides[4];
	double xlon[4], xlat[4], xc[4], yc[4];

	*total_nx = 0;	/* Keep track of total of crossings */

	if (n == 0) return (0);

	GMT_prep_tmp_arrays (GMT, 1, 2);	/* Init or reallocate tmp vectors */
	(void) GMT_map_outside (GMT, lon[0], lat[0]);
	GMT_geo_to_xy (GMT, lon[0], lat[0], &GMT->hidden.mem_coord[GMT_X][0], &GMT->hidden.mem_coord[GMT_Y][0]);
	j += gmt_move_to_rect (GMT, GMT->hidden.mem_coord[GMT_X], GMT->hidden.mem_coord[GMT_Y], 0, 0);
	for (i = 1; i < n; i++) {
		(void) GMT_map_outside (GMT, lon[i], lat[i]);
		nx = gmt_map_crossing (GMT, lon[i-1], lat[i-1], lon[i], lat[i], xlon, xlat, xc, yc, sides);
		for (k = 0; k < nx; k++) {
			GMT_prep_tmp_arrays (GMT, j, 2);	/* Init or reallocate tmp vectors */
			GMT->hidden.mem_coord[GMT_X][j] = xc[k];
			GMT->hidden.mem_coord[GMT_Y][j++] = yc[k];
			(*total_nx) ++;
		}
		GMT_geo_to_xy (GMT, lon[i], lat[i], &GMT->hidden.mem_coord[GMT_X][j], &GMT->hidden.mem_coord[GMT_Y][j]);
		GMT_prep_tmp_arrays (GMT, j+2, 2);	/* Init or reallocate tmp vectors */
		j += gmt_move_to_rect (GMT, GMT->hidden.mem_coord[GMT_X], GMT->hidden.mem_coord[GMT_Y], j, nx);	/* May add 2 points, which explains the j+2 stuff */
	}

	*x = GMT_assign_vector (GMT, j, GMT_X);
	*y = GMT_assign_vector (GMT, j, GMT_Y);

	return (j);
}

/* Functions and macros used for new rectangular clipping using the Sutherland/Hodgman algorithm
 * in which we clip the polygon against each of the 4 sides.  To avoid lots of if/switch I have
 * two clip functions (for x and y line) and two in/out functions that tells us if a point is
 * inside the polygon relative to the line.  Then, pointers to these functions are passed to
 * make sure the right functions are used for each side in the loop over sides.  Unless I can
 * figure out a more clever recursive way I need to have 2 temporary arrays to shuffle the
 * intermediate results around.
 *
 * P.Wessel, March 2008
 */

/* This macro calculates the x-coordinates where the line segment crosses the border x = border.
 * By swapping x and y in the call we can use it for finding the y intersection. This macro is
 * never called when (y_prev - y_curr) = 0 so we don't divide by zero.
 */
#define INTERSECTION_COORD(x_curr,y_curr,x_prev,y_prev,border) x_curr + (x_prev - x_curr) * (border - y_curr) / (y_prev - y_curr)

unsigned int gmt_clip_sn (double x_prev, double y_prev, double x_curr, double y_curr, double x[], double y[], double border, bool (*inside) (double, double), bool (*outside) (double, double), int *cross)
{	/* Clip against the south or north boundary (i.e., a horizontal line with y = border) */
	*cross = 0;
	if (doubleAlmostEqualZero (x_prev, x_curr) && doubleAlmostEqualZero (y_prev, y_curr))
		return (0);	/* Do nothing for duplicates */
	if (outside (y_prev, border)) {	/* Previous point is outside... */
		if (outside (y_curr, border)) return 0;	/* ...as is the current point. Do nothing. */
		/* Here, the line segment intersects the border - return both intersection and inside point */
		y[0] = border;	x[0] = INTERSECTION_COORD (x_curr, y_curr, x_prev, y_prev, border);
		*cross = +1;	/* Crossing to the inside */
		x[1] = x_curr;	y[1] = y_curr;	return (2);
	}
	/* Here x_prev is inside */
	if (inside (y_curr, border)) {	/* Return current point only */
		x[0] = x_curr;	y[0] = y_curr;	return (1);
	}
	/* Segment intersects border - return intersection only */
	*cross = -1;	/* Crossing to the outside */
	y[0] = border;	x[0] = INTERSECTION_COORD (x_curr, y_curr, x_prev, y_prev, border);	return (1);
}

unsigned int gmt_clip_we (double x_prev, double y_prev, double x_curr, double y_curr, double x[], double y[], double border, bool (*inside) (double, double), bool (*outside) (double, double), int *cross)
{	/* Clip against the west or east boundary (i.e., a vertical line with x = border) */
	*cross = 0;
	if (doubleAlmostEqualZero (x_prev, x_curr) && doubleAlmostEqualZero (y_prev, y_curr))
		return (0);	/* Do nothing for duplicates */
	if (outside (x_prev, border)) {	/* Previous point is outside... */
		if (outside (x_curr, border)) return 0;	/* ...as is the current point. Do nothing. */
		/* Here, the line segment intersects the border - return both intersection and inside point */
		x[0] = border;	y[0] = INTERSECTION_COORD (y_curr, x_curr, y_prev, x_prev, border);
		*cross = +1;	/* Crossing to the inside */
		x[1] = x_curr;	y[1] = y_curr;	return (2);
	}
	/* Here x_prev is inside */
	if (inside (x_curr, border)) {	/* Return current point only */
		x[0] = x_curr;	y[0] = y_curr;	return (1);
	}
	/* Segment intersects border - return intersection only */
	*cross = -1;	/* Crossing to the outside */
	x[0] = border;	y[0] = INTERSECTION_COORD (y_curr, x_curr, y_prev, x_prev, border);	return (1);
}

/* Tiny functions to tell if a value is <, <=, >=, > than the limit */
bool gmt_inside_lower_boundary (double val, double min) {return (val >= min);}
bool gmt_inside_upper_boundary (double val, double max) {return (val <= max);}
bool gmt_outside_lower_boundary (double val, double min) {return (val < min);}
bool gmt_outside_upper_boundary (double val, double max) {return (val > max);}

/* gmt_rect_clip is an implementation of the Sutherland/Hodgman algorithm polygon clipping algorithm.
 * Basically, it compares the polygon to one boundary at the time, and clips the polygon to be inside
 * that boundary; this is then repeated for all boundaries.  Assumptions here are Cartesian coordinates
 * so all boundaries are straight lines in x or y. */

uint64_t gmt_rect_clip (struct GMT_CTRL *GMT, double *lon, double *lat, uint64_t n, double **x, double **y, uint64_t *total_nx)
{
	uint64_t i, n_get, m;
	size_t n_alloc = 0;
	unsigned int side, in = 1, out = 0, j, np;
	int cross = 0;
	bool polygon;
	double *xtmp[2] = {NULL, NULL}, *ytmp[2] = {NULL, NULL}, xx[2], yy[2], border[4];
	unsigned int (*clipper[4]) (double, double, double, double, double *, double *, double, bool (*inside) (double, double), bool (*outside) (double, double), int *);
	bool (*inside[4]) (double, double);
	bool (*outside[4]) (double, double);
	
#ifdef DEBUG
	FILE *fp = NULL;
	bool dump = false;
#endif

	if (n == 0) return (0);

	polygon = !GMT_polygon_is_open (GMT, lon, lat, n);	/* true if input segment is a closed polygon */

	*total_nx = 1;	/* So that calling program will not discard the clipped polygon */

	/* Set up function pointers.  This could be done once in GMT_begin at some point */

	clipper[0] = gmt_clip_sn;	clipper[1] = gmt_clip_we; clipper[2] = gmt_clip_sn;	clipper[3] = gmt_clip_we;
	inside[1] = inside[2] = gmt_inside_upper_boundary;	outside[1] = outside[2] = gmt_outside_upper_boundary;
	inside[0] = inside[3] = gmt_inside_lower_boundary;		outside[0] = outside[3] = gmt_outside_lower_boundary;
	border[0] = border[3] = 0.0;	border[1] = GMT->current.map.width;	border[2] = GMT->current.map.height;

	n_get = lrint (1.05*n+5);	/* Anticipate just a few crossings (5%)+5, allocate more later if needed */
	/* Create a pair of arrays for holding input and output */
	GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], n_get, &n_alloc, double);

	/* Get Cartesian map coordinates */

	for (m = 0; m < n; m++) GMT_geo_to_xy (GMT, lon[m], lat[m], &xtmp[0][m], &ytmp[0][m]);

#ifdef DEBUG
	if (dump) {
		fp = fopen ("input.d", "w");
		for (i = 0; i < n; i++) fprintf (fp, "%g\t%g\n", xtmp[0][i], ytmp[0][i]);
		fclose (fp);
	}
#endif
	for (side = 0; side < 4; side++) {	/* Must clip polygon against a single border, one border at a time */
		n = m;	/* Current size of polygon */
		m = 0;	/* Start with nuthin' */

		uint_swap (in, out);	/* Swap what is input and output for clipping against this border */
		/* Must ensure we copy the very first point if it is inside the clip rectangle */
		if (inside[side] ((side%2) ? xtmp[in][0] : ytmp[in][0], border[side])) {xtmp[out][0] = xtmp[in][0]; ytmp[out][0] = ytmp[in][0]; m = 1;}	/* First point is inside; add it */
		for (i = 1; i < n; i++) {	/* For each line segment */
			np = clipper[side] (xtmp[in][i-1], ytmp[in][i-1], xtmp[in][i], ytmp[in][i], xx, yy, border[side], inside[side], outside[side], &cross);	/* Returns 0, 1, or 2 points */
			for (j = 0; j < np; j++) {	/* Add the np returned points to the new clipped polygon path */
				if (m == n_alloc) GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], m, &n_alloc, double);
				xtmp[out][m] = xx[j]; ytmp[out][m] = yy[j]; m++;
			}
		}
		if (polygon && GMT_polygon_is_open (GMT, xtmp[out], ytmp[out], m)) {	/* Do we need to explicitly close this clipped polygon? */
			if (m == n_alloc) GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], m, &n_alloc, double);
			xtmp[out][m] = xtmp[out][0];	ytmp[out][m] = ytmp[out][0];	m++;	/* Yes. */
		}
	}

	GMT_free (GMT, xtmp[1]);	/* Free the pairs of arrays that holds the last input array */
	GMT_free (GMT, ytmp[1]);

	if (m) {	/* Reallocate and return the array with the final clipped polygon */
		n_alloc = m;
		GMT_malloc2 (GMT, xtmp[0], ytmp[0], 0U, &n_alloc, double);
		*x = xtmp[0];
		*y = ytmp[0];
#ifdef DEBUG
		if (dump) {
			fp = fopen ("output.d", "w");
			for (i = 0; i < m; i++) fprintf (fp, "%g\t%g\n", xtmp[0][i], ytmp[0][i]);
			fclose (fp);
		}
#endif
	}
	else {	/* Nothing survived the clipping - free the output arrays */
		GMT_free (GMT, xtmp[0]);
		GMT_free (GMT, ytmp[0]);
	}

	return (m);
}

/* GMT_dateline_clip simply clips a polygon agains the dateline and results in two polygons in L */

unsigned int GMT_split_poly_at_dateline (struct GMT_CTRL *GMT, struct GMT_DATASEGMENT *S, struct GMT_DATASEGMENT ***Lout)
{
	int side, j, np, cross = 0;
	uint64_t row, m;
	size_t n_alloc = 0;
	char label[GMT_BUFSIZ] = {""}, *part = "EW";
	double xx[2], yy[2];
	struct GMT_DATASEGMENT **L = NULL;
	bool (*inside[2]) (double, double);
	bool (*outside[2]) (double, double);
	

	inside[0] = gmt_inside_upper_boundary;	outside[0] = gmt_outside_upper_boundary;
	inside[1] = gmt_inside_lower_boundary;	outside[1] = gmt_outside_lower_boundary;
	L = GMT_memory (GMT, NULL, 2, struct GMT_DATASEGMENT *);	/* The two polygons */

	for (row = 0; row < S->n_rows; row++) GMT_lon_range_adjust (GMT_IS_0_TO_P360_RANGE, &S->coord[GMT_X][row]);	/* First enforce 0 <= lon < 360 so we dont have to check again */

	for (side = 0; side < 2; side++) {	/* Do it twice to get two truncated polygons */
		L[side] = GMT_memory (GMT, NULL, 1, struct GMT_DATASEGMENT);
		n_alloc = lrint (1.05*S->n_rows+5);	/* Anticipate just a few crossings (5%)+5, allocate more later if needed */
		GMT_alloc_segment (GMT, L[side], n_alloc, S->n_columns, true);	/* Temp segment with twice the number of points as we will add crossings*/
		m = 0;		/* Start with nuthin' */

		/* Must ensure we copy the very first point if it is left of the Dateline */
		if (S->coord[GMT_X][0] < 180.0) { L[side]->coord[GMT_X][0] = S->coord[GMT_X][0]; L[side]->coord[GMT_Y][0] = S->coord[GMT_Y][0]; }	/* First point is inside; add it */
		for (row = 1; row < S->n_rows; row++) {	/* For each line segment */
			np = gmt_clip_we (S->coord[GMT_X][row-1], S->coord[GMT_Y][row-1], S->coord[GMT_X][row], S->coord[GMT_Y][row], xx, yy, 180.0, inside[side], outside[side], &cross);	/* Returns 0, 1, or 2 points */
			for (j = 0; j < np; j++) {	/* Add the np returned points to the new clipped polygon path */
				if (m == n_alloc) GMT_alloc_segment (GMT, L[side], n_alloc << 2, S->n_columns, false);
				L[side]->coord[GMT_X][m] = xx[j]; L[side]->coord[GMT_Y][m] = yy[j]; m++;
			}
		}
		if (GMT_polygon_is_open (GMT, L[side]->coord[GMT_X], L[side]->coord[GMT_Y], m)) {	/* Do we need to explicitly close this clipped polygon? */
			if (m == n_alloc) GMT_alloc_segment (GMT, L[side], n_alloc << 2, S->n_columns, false);
			L[side]->coord[GMT_X][m] = L[side]->coord[GMT_X][0];	L[side]->coord[GMT_Y][m] = L[side]->coord[GMT_Y][0];	m++;	/* Yes. */
		}
		if (m != n_alloc) GMT_alloc_segment (GMT, L[side], m, S->n_columns, false);
		L[side]->n_rows = m;
		if (S->label) {
			sprintf (label, "%s part %c", S->label, part[side]);
			L[side]->label = strdup (label);
		}
		if (S->header) L[side]->header = strdup (S->header);
		if (S->ogr) GMT_duplicate_ogr_seg (GMT, L[side], S);
	}
	L[0]->range = 2;	L[1]->range = 3;	
	*Lout = L;
	return (2);
}

/* GMT_wesn_clip differs from gmt_rect_clip in that the boundaries of constant lon or lat may end up as
 * curved lines depending on the map projection.  Thus, if a line crosses the boundary and reenters at
 * another point on the boundary then the straight line between these crossing points should really
 * project to a curved boundary segment.  The H-S algorithm was originally rectangular so we got straight
 * lines.  Here, we check if (1) the particular boundary being tested is curved, and if true then we
 * keep track of the indices of the exit and entry points in the array, and once a boundary has been
 * processed we must add more points between the exit and entry pairs to properly handle the curved
 * segment.  The arrays x_index and x_type stores the index of the exit/entry points and the type
 * (+1 we enter, -1 we exit).  We then use GMT_map_path to compute the required segments to insert.
 * P. Wessel, 2--9-05-07
 */

double gmt_lon_to_corner (struct GMT_CTRL *GMT, double lon) {
	return ( (fabs (lon - GMT->common.R.wesn[XLO]) < fabs (lon - GMT->common.R.wesn[XHI])) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI]);
}

double gmt_lat_to_corner (struct GMT_CTRL *GMT, double lat) {
	return ( (fabs (lat - GMT->common.R.wesn[YLO]) < fabs (lat - GMT->common.R.wesn[YHI])) ? GMT->common.R.wesn[YLO] : GMT->common.R.wesn[YHI]);
}

int gmt_move_to_wesn (struct GMT_CTRL *GMT, double *x_edge, double *y_edge, double lon, double lat, double lon_old, double lat_old, uint64_t j, uint64_t nx)
{
	int n = 0, key;
	double xtmp, ytmp, lon_p, lat_p;

	/* May add 0, 1, or 2 points to path */

	if (!nx && j > 0 && GMT->current.map.this_x_status != GMT->current.map.prev_x_status && GMT->current.map.this_y_status != GMT->current.map.prev_y_status) {	/* Need corner */
		xtmp = x_edge[j];	ytmp = y_edge[j];
		if ((GMT->current.map.this_x_status * GMT->current.map.prev_x_status) == -4 || (GMT->current.map.this_y_status * GMT->current.map.prev_y_status) == -4) {	/* the two points outside on opposite sides */
			lon_p = (GMT->current.map.prev_x_status < 0) ? GMT->common.R.wesn[XLO] : ((GMT->current.map.prev_x_status > 0) ? GMT->common.R.wesn[XHI] : gmt_lon_to_corner (GMT, lon_old));
			lat_p = (GMT->current.map.prev_y_status < 0) ? GMT->common.R.wesn[YLO] : ((GMT->current.map.prev_y_status > 0) ? GMT->common.R.wesn[YHI] : gmt_lat_to_corner (GMT, lat_old));
			GMT_geo_to_xy (GMT, lon_p, lat_p, &x_edge[j], &y_edge[j]);
			j++;
			lon_p = (GMT->current.map.this_x_status < 0) ? GMT->common.R.wesn[XLO] : ((GMT->current.map.this_x_status > 0) ? GMT->common.R.wesn[XHI] : gmt_lon_to_corner (GMT, lon));
			lat_p = (GMT->current.map.this_y_status < 0) ? GMT->common.R.wesn[YLO] : ((GMT->current.map.this_y_status > 0) ? GMT->common.R.wesn[YHI] : gmt_lat_to_corner (GMT, lat));
			GMT_geo_to_xy (GMT, lon_p, lat_p, &x_edge[j], &y_edge[j]);
			j++;
		}
		else {
			key = MIN (GMT->current.map.this_x_status, GMT->current.map.prev_x_status);
			lon_p = (key < 0) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI];
			key = MIN (GMT->current.map.this_y_status, GMT->current.map.prev_y_status);
			lat_p = (key < 0) ? GMT->common.R.wesn[YLO] : GMT->common.R.wesn[YHI];
			GMT_geo_to_xy (GMT, lon_p, lat_p, &x_edge[j], &y_edge[j]);
			j++;
		}
		x_edge[j] = xtmp;	y_edge[j] = ytmp;
		n = 1;
	}
	if (GMT->current.map.this_x_status != 0) lon = (GMT->current.map.this_x_status < 0) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI];
	if (GMT->current.map.this_y_status != 0) lat = (GMT->current.map.this_y_status < 0) ? GMT->common.R.wesn[YLO] : GMT->common.R.wesn[YHI];
	GMT_geo_to_xy (GMT, lon, lat, &x_edge[j], &y_edge[j]);
	return (n + 1);
}

uint64_t gmt_wesn_clip_old (struct GMT_CTRL *GMT, double *lon, double *lat, uint64_t n, double **x, double **y, uint64_t *total_nx)
{
	uint64_t i, j = 0, nx, k;
	unsigned int sides[4];
	double xlon[4], xlat[4], xc[4], yc[4];

	*total_nx = 0;	/* Keep track of total of crossings */

	if (n == 0) return (0);

	GMT_prep_tmp_arrays (GMT, 1, 2);	/* Init or reallocate tmp vectors */

	(void) GMT_map_outside (GMT, lon[0], lat[0]);
	j = gmt_move_to_wesn (GMT, GMT->hidden.mem_coord[GMT_X], GMT->hidden.mem_coord[GMT_Y], lon[0], lat[0], 0.0, 0.0, 0, 0);	/* Add one point */

	for (i = 1; i < n; i++) {
		(void) GMT_map_outside (GMT, lon[i], lat[i]);
		nx = gmt_map_crossing (GMT, lon[i-1], lat[i-1], lon[i], lat[i], xlon, xlat, xc, yc, sides);
		for (k = 0; k < nx; k++) {
			GMT_prep_tmp_arrays (GMT, j, 2);	/* Init or reallocate tmp vectors */
			GMT->hidden.mem_coord[GMT_X][j]   = xc[k];
			GMT->hidden.mem_coord[GMT_Y][j++] = yc[k];
			(*total_nx) ++;
		}
		GMT_prep_tmp_arrays (GMT, j+2, 2);	/* Init or reallocate tmp vectors */
		j += gmt_move_to_wesn (GMT, GMT->hidden.mem_coord[GMT_X], GMT->hidden.mem_coord[GMT_Y], lon[i], lat[i], lon[i-1], lat[i-1], j, nx);	/* May add 2 points, which explains the j+2 stuff */
	}

	*x = GMT_assign_vector (GMT, j, GMT_X);
	*y = GMT_assign_vector (GMT, j, GMT_Y);

#ifdef CRAP
{
	FILE *fp = NULL;
	double out[2];
	fp = fopen ("crap.d", "a");
	fprintf (fp, "> N = %d\n", (int)j);
	for (i = 0; i < j; i++) {
		out[GMT_X] = *x[i];
		out[GMT_Y] = *y[i];
		GMT->current.io.output (GMT, fp, 2, out);
	}
	fclose (fp);
}
#endif
	return (j);
}

uint64_t GMT_wesn_clip (struct GMT_CTRL *GMT, double *lon, double *lat, uint64_t n_orig, double **x, double **y, uint64_t *total_nx)
{
	char *x_type = NULL;
	size_t n_alloc = 0, n_x_alloc = 0, n_t_alloc = 0;
	uint64_t new_n, i, n_get, n, m, n_cross = 0, *x_index = NULL;
	unsigned int j, np, side, in = 1, out = 0;
	int cross = 0;
	bool curved, jump = false, polygon, periodic = false;
	double *xtmp[2] = {NULL, NULL}, *ytmp[2] = {NULL, NULL}, xx[2], yy[2], border[4];
	double x1, x2, y1, y2;
	unsigned int (*clipper[4]) (double, double, double, double, double *, double *, double, bool (*inside) (double, double), bool (*outside) (double, double), int *);
	bool (*inside[4]) (double, double);
	bool (*outside[4]) (double, double);
	
#ifdef DEBUG
	FILE *fp = NULL;
	bool dump = false;
#endif

	if ((n = n_orig) == 0) return (0);

	/* If there are jumps etc call the old clipper, else we try the new clipper */

	GMT_geo_to_xy (GMT, lon[0], lat[0], &x1, &y1);
	for (i = 1; !jump && i < n; i++) {
		GMT_geo_to_xy (GMT, lon[i], lat[i], &x2, &y2);
		jump = gmt_map_jump_x (GMT, x2, y2, x1, y1);
		x1 = x2;	y1 = y2;
	}

	if (jump) return (gmt_wesn_clip_old (GMT, lon, lat, n, x, y, total_nx));	/* Must do the old way for now */
	periodic = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);	/* No point clipping against W and E if periodic map */

	/* Here we can try the Sutherland/Hodgman algorithm */

	polygon = !GMT_polygon_is_open (GMT, lon, lat, n);	/* true if input segment is a closed polygon */

	*total_nx = 1;	/* So that calling program will not discard the clipped polygon */

	/* Set up function pointers.  This could be done once in GMT_begin at some point */

	clipper[0] = gmt_clip_sn;	clipper[1] = gmt_clip_we; clipper[2] = gmt_clip_sn;	clipper[3] = gmt_clip_we;
	inside[1] = inside[2] = gmt_inside_upper_boundary;	outside[1] = outside[2] = gmt_outside_upper_boundary;
	inside[0] = inside[3] = gmt_inside_lower_boundary;	outside[0] = outside[3] = gmt_outside_lower_boundary;
	border[0] = GMT->common.R.wesn[YLO]; border[3] = GMT->common.R.wesn[XLO];	border[1] = GMT->common.R.wesn[XHI];	border[2] = GMT->common.R.wesn[YHI];
	/* Make data longitudes have no jumps */
	for (i = 0; i < n; i++) {
		if (lon[i] < border[3] && (lon[i] + 360.0) <= border[1])
			lon[i] += 360.0;
		else if (lon[i] > border[1] && (lon[i] - 360.0) >= border[3])
			lon[i] -= 360.0;
	}

	n_get = lrint (1.05*n+5);	/* Anticipate just a few crossings (5%)+5, allocate more later if needed */
	/* Create a pair of arrays for holding input and output */
	GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], n_get, &n_alloc, double);

	/* Make copy of lon/lat coordinates */

	GMT_memcpy (xtmp[0], lon, n, double);	GMT_memcpy (ytmp[0], lat, n, double);
	m = n;

	/* Preallocate space for crossing information */

	x_index = GMT_malloc (GMT, NULL, GMT_TINY_CHUNK, &n_x_alloc, uint64_t);
	x_type = GMT_malloc (GMT, NULL,  GMT_TINY_CHUNK, &n_t_alloc, char);

#ifdef DEBUG
	if (dump) {
		fp = fopen ("input.d", "w");
		for (i = 0; i < n; i++) fprintf (fp, "%g\t%g\n", xtmp[0][i], ytmp[0][i]);
		fclose (fp);
	}
#endif
	for (side = 0; side < 4; side++) {	/* Must clip polygon against a single border, one border at a time */
		n = m;		/* Current size of polygon */
		m = 0;		/* Start with nuthin' */
		n_cross = 0;	/* No crossings so far */

		curved = !((side%2) ? GMT->current.map.meridian_straight : GMT->current.map.parallel_straight);	/* Is this border straight or curved when projected */
		uint_swap (in, out);	/* Swap what is input and output for clipping against this border */
		if (side%2 && periodic) {	/* No clipping can take place on w or e border; just copy all and go to next side */
			m = n;
			if (m == n_alloc) GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], m, &n_alloc, double);
			GMT_memcpy (xtmp[out], xtmp[in], m, double);
			GMT_memcpy (ytmp[out], ytmp[in], m, double);
			continue;
		}
		/* Must ensure we copy the very first point if it is inside the clip rectangle */
		if (inside[side] ((side%2) ? xtmp[in][0] : ytmp[in][0], border[side])) {xtmp[out][0] = xtmp[in][0]; ytmp[out][0] = ytmp[in][0]; m = 1;}	/* First point is inside; add it */
		for (i = 1; i < n; i++) {	/* For each line segment */
			np = clipper[side] (xtmp[in][i-1], ytmp[in][i-1], xtmp[in][i], ytmp[in][i], xx, yy, border[side], inside[side], outside[side], &cross);	/* Returns 0, 1, or 2 points */
			if (polygon && cross && curved) {	/* When crossing in/out of a curved boundary we must eventually sample along the curve between crossings */
				x_index[n_cross] = m;		/* Index of intersection point (which will be copied from xx[0], yy[0] below) */
				x_type[n_cross] = cross;	/* -1 going out, +1 going in */
				if (++n_cross == n_x_alloc) {
					x_index = GMT_malloc (GMT, x_index, n_cross, &n_x_alloc, uint64_t);
					x_type = GMT_malloc (GMT, x_type,  n_cross, &n_t_alloc, char);
				}
			}
			for (j = 0; j < np; j++) {	/* Add the np returned points to the new clipped polygon path */
				if (m == n_alloc) GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], m, &n_alloc, double);
				xtmp[out][m] = xx[j]; ytmp[out][m] = yy[j]; m++;
			}
		}
		if (polygon && GMT_polygon_is_open (GMT, xtmp[out], ytmp[out], m)) {	/* Do we need to explicitly close this clipped polygon? */
			if (m == n_alloc) GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], m, &n_alloc, double);
			xtmp[out][m] = xtmp[out][0];	ytmp[out][m] = ytmp[out][0];	m++;	/* Yes. */
		}
		if (polygon && curved && n_cross) {	/* Must resample between crossing points */
			double *x_add = NULL, *y_add = NULL, *x_cpy = NULL, *y_cpy = NULL;
			size_t np = 0;
			uint64_t add, last_index = 0, p, p_next;

			if (n_cross%2 == 1) {	/* Should not happen with a polygon */
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error in GMT_wesn_clip: odd number of crossings?");
			}

			/* First copy the current polygon */

			GMT_malloc2 (GMT, x_cpy, y_cpy, m, &np, double);
			GMT_memcpy (x_cpy, xtmp[out], m, double);
			GMT_memcpy (y_cpy, ytmp[out], m, double);

			for (p = np = 0; p < n_cross; p++) {	/* Process each crossing point */
				if (last_index < x_index[p]) {	/* Copy over segment from were we left off to this crossing point */
					add = x_index[p] - last_index;
					if ((new_n = (np+add)) >= n_alloc) GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], new_n, &n_alloc, double);
					GMT_memcpy (&xtmp[out][np], &x_cpy[last_index], add, double);
					GMT_memcpy (&ytmp[out][np], &y_cpy[last_index], add, double);
					np += add;
					last_index = x_index[p];
				}
				if (x_type[p] == -1) {	/* Must add path from this exit to the next entry */
					double start_lon, stop_lon;
					p_next = (p == (n_cross-1)) ? 0 : p + 1;	/* index of the next crossing */
					start_lon = x_cpy[x_index[p]];	stop_lon = x_cpy[x_index[p_next]];
					if (side%2 == 0 && periodic) {	/* Make sure we select the shortest longitude arc */
						if ((x_cpy[x_index[p_next]] - x_cpy[x_index[p]]) < -180.0)
							stop_lon += 360.0;
						else if ((x_cpy[x_index[p_next]] - x_cpy[x_index[p]]) > +180.0)
							stop_lon -= 360.0;
					}
					add = GMT_map_path (GMT, start_lon, y_cpy[x_index[p]], stop_lon, y_cpy[x_index[p_next]], &x_add, &y_add);
					if ((new_n = (np+add)) >= n_alloc) GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], new_n, &n_alloc, double);
					GMT_memcpy (&xtmp[out][np], x_add, add, double);
					GMT_memcpy (&ytmp[out][np], y_add, add, double);
					if (add) { GMT_free (GMT, x_add);	GMT_free (GMT, y_add); }
					np += add;
					last_index = x_index[p_next];
				}
			}
			if (x_index[0] > 0) {	/* First point was clean inside, must add last connection */
				add = m - last_index;
				if ((new_n = (np+add)) >= n_alloc) GMT_malloc4 (GMT, xtmp[0], ytmp[0], xtmp[1], ytmp[1], new_n, &n_alloc, double);
				GMT_memcpy (&xtmp[out][np], &x_cpy[last_index], add, double);
				GMT_memcpy (&ytmp[out][np], &y_cpy[last_index], add, double);
				np += add;
			}
			m = np;	/* New total of points */
			GMT_free (GMT, x_cpy);	GMT_free (GMT, y_cpy);
		}
	}

	GMT_free (GMT, xtmp[1]);	/* Free the pairs of arrays that holds the last input array */
	GMT_free (GMT, ytmp[1]);
	GMT_free (GMT, x_index);	/* Free the pairs of arrays that holds the crossing info */
	GMT_free (GMT, x_type);

	if (m) {	/* Reallocate and return the array with the final clipped polygon */
		n_alloc = m;
		GMT_malloc2 (GMT, xtmp[0], ytmp[0], 0U, &n_alloc, double);
		/* Convert to map coordinates */
		for (i = 0; i < m; i++) GMT_geo_to_xy (GMT, xtmp[0][i], ytmp[0][i], &xtmp[0][i], &ytmp[0][i]);

		*x = xtmp[0];
		*y = ytmp[0];
#ifdef DEBUG
		if (dump) {
			fp = fopen ("output.d", "w");
			for (i = 0; i < m; i++) fprintf (fp, "%g\t%g\n", xtmp[0][i], ytmp[0][i]);
			fclose (fp);
		}
#endif
	}
	else {	/* Nothing survived the clipping - free the output arrays */
		GMT_free (GMT, xtmp[0]);
		GMT_free (GMT, ytmp[0]);
	}

	return (m);
}

uint64_t gmt_radial_boundary_arc (struct GMT_CTRL *GMT, int this_way, double end_x[], double end_y[], double **xarc, double **yarc) {
	uint64_t n_arc, k, pt;
	double az1, az2, d_az, da, xr, yr, da_try, *xx = NULL, *yy = NULL;

	/* When a polygon crosses out then in again into the circle we need to add a boundary arc
	 * to the polygon where it is clipped.  We simply sample the circle as finely as the arc
	 * length and the current line_step demands */

	da_try = (GMT->current.setting.map_line_step * 360.0) / (TWO_PI * GMT->current.proj.r);	/* Angular step in degrees */
	az1 = d_atan2d (end_y[0], end_x[0]);	/* azimuth from map center to 1st crossing */
	az2 = d_atan2d (end_y[1], end_x[1]);	/* azimuth from map center to 2nd crossing */
	GMT_set_delta_lon (az1, az2, d_az);	/* Insist we take the short arc for now */
	n_arc = lrint (ceil (fabs (d_az) / da_try));	/* Get number of integer increments of da_try degree... */
	if (n_arc < 2) n_arc = 2;	/* ...but minimum 2 */
	da = d_az / (n_arc - 1);	/* Reset da to get exact steps */
	if (n_arc <= 2) return (0);	/* Arc is too short to have intermediate points */
	n_arc -= 2;	/* We do not include the end points since these are the crossing points handled in the calling function */
	GMT_malloc2 (GMT, xx, yy, n_arc, NULL, double);
	for (k = 1; k <= n_arc; k++) {	/* Create points along arc from first to second crossing point (k-loop excludes the end points) */
		sincosd (az1 + k * da, &yr, &xr);
		pt = (this_way) ? n_arc - k : k - 1;	/* The order we add the arc depends if we exited or entered the inside area */
		xx[pt] = GMT->current.proj.r * (1.0 + xr);
		yy[pt] = GMT->current.proj.r * (1.0 + yr);
	}

	*xarc = xx;
	*yarc = yy;
	return (n_arc);
}

#ifdef DEBUG
/* If we need to dump out clipped polygon then set clip_dump = 1 during execution */
int clip_dump = 0, clip_id = 0;
void gmt_dumppol (uint64_t n, double *x, double *y, int *id)
{
	uint64_t i;
	FILE *fp = NULL;
	char line[64];
	sprintf (line, "dump_%d.d", *id);
	fp = fopen (line, "w");
	for (i = 0; i < n; i++) fprintf (fp, "%g\t%g\n", x[i], y[i]);
	fclose (fp);
	(*id)++;
}
#endif


uint64_t gmt_radial_clip (struct GMT_CTRL *GMT, double *lon, double *lat, uint64_t np, double **x, double **y, uint64_t *total_nx)
{
	size_t n_alloc = 0;
	uint64_t n = 0, n_arc;
	unsigned int i, nx;
	unsigned int sides[4];
	bool this_side = false, add_boundary = false;
	double xlon[4], xlat[4], xc[4], yc[4], end_x[3], end_y[3], xr, yr;
	double *xx = NULL, *yy = NULL, *xarc = NULL, *yarc = NULL;

	*total_nx = 0;	/* Keep track of total of crossings */

	if (np == 0) return (0);

	if (!GMT_map_outside (GMT, lon[0], lat[0])) {
		GMT_malloc2 (GMT, xx, yy, n, &n_alloc, double);
		GMT_geo_to_xy (GMT, lon[0], lat[0], &xx[0], &yy[0]);
		n++;
	}
	nx = 0;
	for (i = 1; i < np; i++) {
		this_side = GMT_map_outside (GMT, lon[i], lat[i]);
		if (gmt_map_crossing (GMT, lon[i-1], lat[i-1], lon[i], lat[i], xlon, xlat, xc, yc, sides)) {
			if (this_side) {	/* Crossing boundary and leaving circle: Add exit point to the path */
				if (n == n_alloc) GMT_malloc2 (GMT, xx, yy, n, &n_alloc, double);
				xx[n] = xc[0];	yy[n] = yc[0];	n++;
			}
			end_x[nx] = xc[0] - GMT->current.proj.r;	end_y[nx] = yc[0] - GMT->current.proj.r;
			nx++;
			(*total_nx) ++;
			if (nx >= 2) {	/* Got a pair of entry+exit points */
				add_boundary = !this_side;	/* We only add boundary arcs if we first exited and now entered the circle again */
			}
			if (add_boundary) {	/* Crossed twice.  Now add arc between the two crossing points */
				/* PW: Currently, we make the assumption that the shortest arc is the one we want.  However,
				 * extremely large polygons could cut the boundary so that it is the longest arc we want.
				 * The way to improve this algorithm in the future is to find the two opposite points on
				 * the circle boundary that lies on the bisector of az1,az2, and see which point lies
				 * inside the polygon.  This would require that GMT_inonout_sphpol be called.
				 */
				if ((n_arc = gmt_radial_boundary_arc (GMT, this_side, &end_x[nx-2], &end_y[nx-2], &xarc, &yarc)) > 0) {
					if ((n + n_arc) >= n_alloc) GMT_malloc2 (GMT, xx, yy, n + n_arc, &n_alloc, double);
					GMT_memcpy (&xx[n], xarc, n_arc, double);	/* Copy longitudes of arc */
					GMT_memcpy (&yy[n], yarc, n_arc, double);	/* Copy latitudes of arc */
					n += n_arc;	/* Number of arc points added (end points are done separately) */
					GMT_free (GMT, xarc);	GMT_free (GMT,  yarc);
				}
				add_boundary = false;
				nx -= 2;	/* Done with those two crossings */
			}
			if (!this_side) {	/* Crossing boundary and entering circle: Add entry point to the path */
				if (n == n_alloc) GMT_malloc2 (GMT, xx, yy, n, &n_alloc, double);
				xx[n] = xc[0];	yy[n] = yc[0];	n++;
			}
		}
		GMT_geo_to_xy (GMT, lon[i], lat[i], &xr, &yr);
		if (!this_side) {	/* Only add points actually inside the map to the path */
			if (n == n_alloc) GMT_malloc2 (GMT, xx, yy, n, &n_alloc, double);
			xx[n] = xr;	yy[n] = yr;	n++;
		}
	}

	if (nx == 2) {	/* Must close polygon by adding boundary arc */
		if ((n_arc = gmt_radial_boundary_arc (GMT, this_side, end_x, end_y, &xarc, &yarc)) > 0) {
			if ((n + n_arc) >= n_alloc) GMT_malloc2 (GMT, xx, yy, n + n_arc, &n_alloc, double);
			GMT_memcpy (&xx[n], xarc, n_arc, double);	/* Copy longitudes of arc */
			GMT_memcpy (&yy[n], yarc, n_arc, double);	/* Copy latitudes of arc */
			n += n_arc;	/* Number of arc points added (end points are done separately) */
			GMT_free (GMT, xarc);	GMT_free (GMT,  yarc);
		}
		if (n == n_alloc) GMT_malloc2 (GMT, xx, yy, n, &n_alloc, double);
		xx[n] = xx[0];	yy[n] = yy[0];	n++;	/* Close the polygon */
	}
	n_alloc = n;
	GMT_malloc2 (GMT, xx, yy, 0U, &n_alloc, double);
	*x = xx;
	*y = yy;
#ifdef DEBUG
	if (clip_dump) gmt_dumppol (n, xx, yy, &clip_id);
#endif

	return (n);
}

bool gmt_rect_overlap (struct GMT_CTRL *GMT, double lon0, double lat0, double lon1, double lat1)
{
	/* Return true if the projection of either (lon0,lat0) and (lon1,lat1) is inside (not on) the rectangular map boundary */
	double x0, y0, x1, y1;

	GMT_geo_to_xy (GMT, lon0, lat0, &x0, &y0);
	GMT_geo_to_xy (GMT, lon1, lat1, &x1, &y1);

	if (x0 > x1) double_swap (x0, x1);
	if (y0 > y1) double_swap (y0, y1);

	if (x1 - GMT->current.proj.rect[XLO] < -GMT_CONV_LIMIT || x0 - GMT->current.proj.rect[XHI] > GMT_CONV_LIMIT) return (false);
	if (y1 - GMT->current.proj.rect[YLO] < -GMT_CONV_LIMIT || y0 - GMT->current.proj.rect[YHI] > GMT_CONV_LIMIT) return (false);
	return (true);
}

bool gmt_wesn_overlap (struct GMT_CTRL *GMT, double lon0, double lat0, double lon1, double lat1)
{
	/* Return true if either of the points (lon0,lat0) and (lon1,lat1) is inside (not on) the rectangular lon/lat boundaries */
	if (lon0 > lon1) double_swap (lon0, lon1);
	if (lat0 > lat1) double_swap (lat0, lat1);
	if (lon1 - GMT->common.R.wesn[XLO] < -GMT_CONV_LIMIT) {
		lon0 += 360.0;
		lon1 += 360.0;
	}
	else if (lon0 - GMT->common.R.wesn[XHI] > GMT_CONV_LIMIT) {
		lon0 -= 360.0;
		lon1 -= 360.0;
	}

	if (lon1 - GMT->common.R.wesn[XLO] < -GMT_CONV_LIMIT || lon0 - GMT->common.R.wesn[XHI] > GMT_CONV_LIMIT) return (false);
	if (lat1 - GMT->common.R.wesn[YLO] < -GMT_CONV_LIMIT || lat0 - GMT->common.R.wesn[YHI] > GMT_CONV_LIMIT) return (false);
	return (true);
}

bool gmt_radial_overlap (struct GMT_CTRL *GMT, double lon0, double lat0, double lon1, double lat1)
{	/* Dummy routine */
	return (true);
}

bool gmt_genper_overlap (struct GMT_CTRL *GMT, double lon0, double lat0, double lon1, double lat1)
{
/* Dummy routine */
	if (GMT->current.proj.g_debug > 0) GMT_Report (GMT->parent, GMT_MSG_DEBUG, "genper_overlap: overlap called\n");
	return (true);
}

void gmt_xy_search (struct GMT_CTRL *GMT, double *x0, double *x1, double *y0, double *y1, double w0, double e0, double s0, double n0)
{
	unsigned int i, j;
	double xmin, xmax, ymin, ymax, w, s, x, y, dlon, dlat;

	/* Find min/max forward values */

	xmax = ymax = -DBL_MAX;
	xmin = ymin = DBL_MAX;
	dlon = fabs (e0 - w0) / 500;
	dlat = fabs (n0 - s0) / 500;

	for (i = 0; i <= 500; i++) {
		w = w0 + i * dlon;
		(*GMT->current.proj.fwd) (GMT, w, s0, &x, &y);
		if (x < xmin) xmin = x;
		if (y < ymin) ymin = y;
		if (x > xmax) xmax = x;
		if (y > ymax) ymax = y;
		(*GMT->current.proj.fwd) (GMT, w, n0, &x, &y);
		if (x < xmin) xmin = x;
		if (y < ymin) ymin = y;
		if (x > xmax) xmax = x;
		if (y > ymax) ymax = y;
	}
	for (j = 0; j <= 500; j++) {
		s = s0 + j * dlat;
		(*GMT->current.proj.fwd) (GMT, w0, s, &x, &y);
		if (x < xmin) xmin = x;
		if (y < ymin) ymin = y;
		if (x > xmax) xmax = x;
		if (y > ymax) ymax = y;
		(*GMT->current.proj.fwd) (GMT, e0, s, &x, &y);
		if (x < xmin) xmin = x;
		if (y < ymin) ymin = y;
		if (x > xmax) xmax = x;
		if (y > ymax) ymax = y;
	}

	*x0 = xmin;	*x1 = xmax;	*y0 = ymin;	*y1 = ymax;
}

void gmt_map_setxy (struct GMT_CTRL *GMT, double xmin, double xmax, double ymin, double ymax)
{	/* Set x/y parameters */

	GMT->current.proj.rect_m[XLO] = xmin;	GMT->current.proj.rect_m[XHI] = xmax;	/* This is in original meters */
	GMT->current.proj.rect_m[YLO] = ymin;	GMT->current.proj.rect_m[YHI] = ymax;
	GMT_Report (GMT->parent, GMT_MSG_DEBUG, "Projected values in meters: %g %g %g %g\n", xmin, xmax, ymin, ymax);
	GMT->current.proj.rect[XHI] = (xmax - xmin) * GMT->current.proj.scale[GMT_X];
	GMT->current.proj.rect[YHI] = (ymax - ymin) * GMT->current.proj.scale[GMT_Y];
	GMT->current.proj.origin[GMT_X] = -xmin * GMT->current.proj.scale[GMT_X];
	GMT->current.proj.origin[GMT_Y] = -ymin * GMT->current.proj.scale[GMT_Y];
}

void gmt_map_setinfo (struct GMT_CTRL *GMT, double xmin, double xmax, double ymin, double ymax, double scl)
{	/* Set [and rescale] parameters */
	double factor = 1.0, w, h;

	if (GMT->current.map.is_world && doubleAlmostEqualZero (xmax, xmin)) {	/* Safety valve for cases when w & e both project to the same side due to round-off */
		xmax = MAX (fabs (xmin), fabs (xmax));
		xmin =-xmax;
	}
	w = (xmax - xmin) * GMT->current.proj.scale[GMT_X];
	h = (ymax - ymin) * GMT->current.proj.scale[GMT_Y];

	if (GMT->current.proj.gave_map_width == 1)		/* Must rescale to given width */
		factor = scl / w;
	else if (GMT->current.proj.gave_map_width == 2)	/* Must rescale to given height */
		factor = scl / h;
	else if (GMT->current.proj.gave_map_width == 3)	/* Must rescale to max dimension */
		factor = scl / MAX (w, h);
	else if (GMT->current.proj.gave_map_width == 4)	/* Must rescale to min dimension */
		factor = scl / MIN (w, h);
	GMT->current.proj.scale[GMT_X] *= factor;
	GMT->current.proj.scale[GMT_Y] *= factor;
	GMT->current.proj.w_r *= factor;

	if (GMT->current.proj.g_debug > 1) {
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "xmin %7.3f xmax %7.3f ymin %7.4f ymax %7.3f scale %6.3f\n", xmin/1000, xmax/1000, ymin/1000, ymax/1000, scl);
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "gave_map_width %d w %9.4e h %9.4e factor %9.4e\n", GMT->current.proj.gave_map_width, w, h, factor);
	}

	gmt_map_setxy (GMT, xmin, xmax, ymin, ymax);
}

double gmt_mean_radius (struct GMT_CTRL *GMT, double a, double f)
{
	double r, b = a * (1 - f);
	
	if (f == 0.0) return a;	/* Not that hard */
	
	switch (GMT->current.setting.proj_mean_radius) {
		case GMT_RADIUS_MEAN:
			r = a * (1.0 - f / 3.0);
			break;
		case GMT_RADIUS_AUTHALIC:
			r = sqrt (0.5 * a * a + 0.5 * b * b * atanh (GMT->current.proj.ECC) / GMT->current.proj.ECC);
			break;
		case GMT_RADIUS_VOLUMETRIC:
			r = pow (a*a*b, 1.0/3.0);
			break;
		case GMT_RADIUS_MERIDIONAL:
			r = pow (0.5 * (pow (a, 1.5) + pow (b, 1.5)), 2.0/3.0);
			break;
		case GMT_RADIUS_QUADRATIC:
			r = 0.5 * sqrt (3.0 * a * a + b * b);
			break;
		default:	/* Cannot get here! Safety valve */
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Internal ERROR: GMT mean radius not specified\n");
			exit (EXIT_FAILURE);
			break;
	}
	
	return (r);
}

void GMT_set_spherical (struct GMT_CTRL *GMT, bool notify)
{
	/* Set up ellipsoid parameters using spherical approximation */

	GMT->current.setting.ref_ellipsoid[GMT_N_ELLIPSOIDS - 1].eq_radius =
		gmt_mean_radius (GMT, GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].eq_radius, GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].flattening);
	GMT->current.setting.proj_ellipsoid = GMT_N_ELLIPSOIDS - 1;	/* Custom ellipsoid */
	GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].flattening = 0.0;
	if (notify) GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Warning: spherical approximation used!\n");
	GMT->current.setting.proj_aux_latitude = GMT_LATSWAP_NONE;	/* No lat swapping for spherical */

	GMT_init_ellipsoid (GMT);
}

#if 0
void GMT_set_geocentric (struct GMT_CTRL *GMT, bool notify)
{	/* The idea is to call this from sample1d/grdtrack */
	/* Set up ellipsoid parameters for spherical approximation with geocentric parameters */

	GMT->current.setting.proj_aux_latitude = GMT_LATSWAP_G2O;	/* Geocentric/Geodetic conversion */
	GMT->current.setting.proj_mean_radius = GMT_RADIUS_MERIDIONAL;
	GMT_init_ellipsoid (GMT);
}
#endif

double GMT_left_boundary (struct GMT_CTRL *GMT, double y)
{
	return ((*GMT->current.map.left_edge) (GMT, y));
}

double GMT_right_boundary (struct GMT_CTRL *GMT, double y)
{
	return ((*GMT->current.map.right_edge) (GMT, y));
}

double gmt_left_conic (struct GMT_CTRL *GMT, double y)
{
	double x_ws, y_ws, x_wn, y_wn, dy;

	GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &x_ws, &y_ws);
	GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YHI], &x_wn, &y_wn);
	dy = y_wn - y_ws;
	if (doubleAlmostEqualZero (y_wn, y_ws))
		return (0.0);
	return (x_ws + ((x_wn - x_ws) * (y - y_ws) / dy));
}

double gmt_right_conic (struct GMT_CTRL *GMT, double y)
{
	double x_es, y_es, x_en, y_en, dy;

	GMT_geo_to_xy (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], &x_es, &y_es);
	GMT_geo_to_xy (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &x_en, &y_en);
	dy = y_en - y_es;
	if (doubleAlmostEqualZero (y_en, y_es))
		return (GMT->current.map.width);
	return (x_es - ((x_es - x_en) * (y - y_es) / dy));
}

double gmt_left_rect (struct GMT_CTRL *GMT, double y)
{
	return (0.0);
}

double gmt_right_rect (struct GMT_CTRL *GMT, double y)
{
	return (GMT->current.map.width);
}

double gmt_left_circle (struct GMT_CTRL *GMT, double y)
{
	y -= GMT->current.proj.origin[GMT_Y];
	return (GMT->current.map.half_width - d_sqrt (GMT->current.proj.r * GMT->current.proj.r - y * y));
}

double gmt_right_circle (struct GMT_CTRL *GMT, double y)
{
	/* y -= GMT->current.proj.r; */
	y -= GMT->current.proj.origin[GMT_Y];
	return (GMT->current.map.half_width + d_sqrt (GMT->current.proj.r * GMT->current.proj.r - y * y));
}

double gmt_left_ellipse (struct GMT_CTRL *GMT, double y)
{
	/* Applies to Hammer and Mollweide only, where major axis = 2 * minor axis */

	y = (y - GMT->current.proj.origin[GMT_Y]) / GMT->current.proj.w_r;	/* Fraction, relative to Equator */
	return (GMT->current.map.half_width - 2.0 * GMT->current.proj.w_r * d_sqrt (1.0 - y * y));
}

double gmt_right_ellipse (struct GMT_CTRL *GMT, double y)
{
	/* Applies to Hammer and Mollweide only, where major axis = 2 * minor axis */

	y = (y - GMT->current.proj.origin[GMT_Y]) / GMT->current.proj.w_r;	/* Fraction, relative to Equator */
	return (GMT->current.map.half_width + 2.0 * GMT->current.proj.w_r * d_sqrt (1.0 - y * y));
}

void GMT_get_point_from_r_az (struct GMT_CTRL *GMT, double lon0, double lat0, double r, double azim, double *lon1, double *lat1)
/* Given point (lon0, lat0), find coordinates of a point r degrees away in the azim direction */
{
	double sinr, cosr, sinaz, cosaz, siny, cosy;

	sincosd (azim, &sinaz, &cosaz);
	sincosd (r, &sinr, &cosr);
	sincosd (lat0, &siny, &cosy);

	*lon1 = lon0 + atan2d (sinr * sinaz, (cosy * cosr - siny * sinr * cosaz));
	*lat1 = d_asind (siny * cosr + cosy * sinr * cosaz);
}

double gmt_az_backaz_cartesian (struct GMT_CTRL *GMT, double lonE, double latE, double lonS, double latS, bool baz)
{
	/* Calculate azimuths or backazimuths.  Cartesian case.
	 * First point is considered "Event" and second "Station".
	 * Azimuth is direction from Station to Event.
	 * BackAzimuth is direction from Event to Station */

	double az, dx, dy;

	if (baz) {	/* exchange point one and two */
		double_swap (lonS, lonE);
		double_swap (latS, latE);
	}
	dx = lonE - lonS;
	dy = latE - latS;
	az = (dx == 0.0 && dy == 0.0) ? GMT->session.d_NaN : 90.0 - atan2d (dy, dx);
	if (az < 0.0) az += 360.0;
	return (az);
}

double gmt_az_backaz_cartesian_proj (struct GMT_CTRL *GMT, double lonE, double latE, double lonS, double latS, bool baz)
{
	/* Calculate azimuths or backazimuths.  Cartesian case.
	 * First point is considered "Event" and second "Station".
	 * Azimuth is direction from Station to Event.
	 * BackAzimuth is direction from Event to Station */

	double az, dx, dy, xE, yE, xS, yS;

	if (baz) {	/* exchange point one and two */
		double_swap (lonS, lonE);
		double_swap (latS, latE);
	}
	GMT_geo_to_xy (GMT, lonE, latE, &xE, &yE);
	GMT_geo_to_xy (GMT, lonS, latS, &xS, &yS);
	dx = xE - xS;
	dy = yE - yS;
	az = (dx == 0.0 && dy == 0.0) ? GMT->session.d_NaN : 90.0 - atan2d (dy, dx);
	if (az < 0.0) az += 360.0;
	return (az);
}

double gmt_az_backaz_flatearth (struct GMT_CTRL *GMT, double lonE, double latE, double lonS, double latS, bool baz)
{
	/* Calculate azimuths or backazimuths.  Flat earth code.
	 * First point is considered "Event" and second "Station".
	 * Azimuth is direction from Station to Event.
	 * BackAzimuth is direction from Event to Station */

	double az, dx, dy, dlon;

	if (baz) {	/* exchange point one and two */
		double_swap (lonS, lonE);
		double_swap (latS, latE);
	}
	GMT_set_delta_lon (lonS, lonE, dlon);
	dx = dlon * cosd (0.5 * (latE + latS));
	dy = latE - latS;
	az = (dx == 0.0 && dy == 0.0) ? GMT->session.d_NaN : 90.0 - atan2d (dy, dx);
	if (az < 0.0) az += 360.0;
	return (az);
}

double gmt_az_backaz_sphere (struct GMT_CTRL *GMT, double lonE, double latE, double lonS, double latS, bool baz)
{
	/* Calculate azimuths or backazimuths.  Spherical code.
	 * First point is considered "Event" and second "Station".
	 * Azimuth is direction from Station to Event.
	 * BackAzimuth is direction from Event to Station */

	double az, sin_yS, cos_yS, sin_yE, cos_yE, sin_dlon, cos_dlon;

	if (baz) {	/* exchange point one and two */
		double_swap (lonS, lonE);
		double_swap (latS, latE);
	}
	sincosd (latS, &sin_yS, &cos_yS);
	sincosd (latE, &sin_yE, &cos_yE);
	sincosd (lonS - lonE, &sin_dlon, &cos_dlon);
	az = atan2d (cos_yS * sin_dlon, cos_yE * sin_yS - sin_yE * cos_yS * cos_dlon);
	if (az < 0.0) az += 360.0;
	return (az);
}

#ifdef USE_VINCENTY
#define GEOD_TEXT "Vincenty"
#define VINCENTY_EPS		5e-14
#define VINCENTY_MAX_ITER	50
double gmt_az_backaz_geodesic (struct GMT_CTRL *GMT, double lonE, double latE, double lonS, double latS, bool back_az)
{
	/* Translation of NGS FORTRAN code for determination of true distance
	** and respective forward and back azimuths between two points on the
	** ellipsoid.  Good for any pair of points that are not antipodal.
	**
	**      INPUT
	**	latS, lonS -- latitude and longitude of first point in radians.
	**	latE, lonE -- latitude and longitude of second point in radians.
	**
	**	OUTPUT
	**  	faz -- azimuth from first point to second in radians clockwise from North.
	**	baz -- azimuth from second point back to first point.
	**	bool back_az controls which is returned 
	** Modified by P.W. from: http://article.gmane.org/gmane.comp.gis.proj-4.devel/3478
	*/
	int n_iter = 0;
	static double az, c, d, e, r, f, d_lon, dx, x, y, sa, cx, cy, cz, sx, sy, c2a, cu1, cu2, su1, tu1, tu2, ts, baz, faz;

	f = GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].flattening;
	r = 1.0 - f;
	tu1 = r * tand (latS);
	tu2 = r * tand (latE);
	cu1 = 1.0 / sqrt (tu1 * tu1 + 1.0);
	su1 = cu1 * tu1;
	cu2 = 1.0 / sqrt (tu2 * tu2 + 1.0);
	ts  = cu1 * cu2;
	baz = ts * tu2;
	faz = baz * tu1;
	GMT_set_delta_lon (lonS, lonE, d_lon);
	x = dx = D2R * d_lon;
	do {
		n_iter++;
		sincos (x, &sx, &cx);
		tu1 = cu2 * sx;
		tu2 = baz - su1 * cu2 * cx;
		sy = sqrt (tu1 * tu1 + tu2 * tu2);
		cy = ts * cx + faz;
		y = atan2 (sy, cy);
		sa = ts * sx / sy;
		c2a = -sa * sa + 1.0;
		cz = faz + faz;
		if (c2a > 0.0) cz = -cz / c2a + cy;
		e = cz * cz * 2.0 - 1.0;
		c = ((c2a * -3.0 + 4.0) * f + 4.0) * c2a * f / 16.0;
		d = x;
		x = ((e * cy * c + cz) * sy * c + y) * sa;
		x = (1.0 - c) * x * f + dx;
	} while (fabs (d - x) > VINCENTY_EPS && n_iter <= VINCENTY_MAX_ITER);
	if (n_iter > VINCENTY_MAX_ITER) {
		GMT->current.proj.n_geodesic_approx++;	/* Count inaccurate results */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Near- or actual antipodal points encountered. Precision may be reduced slightly.\n");
	}
	GMT->current.proj.n_geodesic_calls++;
	/* To give the same sense of results as all other codes, we must basically swap baz and faz; here done in the ? test */
	az = (back_az) ? atan2 (tu1, tu2) : atan2 (cu1 * sx, baz * cx - su1 * cu2) + M_PI;
	return (R2D * az);
}
#else
#define GEOD_TEXT "Rudoe"
double gmt_az_backaz_geodesic (struct GMT_CTRL *GMT, double lonE, double latE, double lonS, double latS, bool baz)
{
	/* Calculate azimuths or backazimuths for geodesics using geocentric latitudes.
	 * First point is considered "Event" and second "Station".
	 * Azimuth is direction from Station to Event.
	 * BackAzimuth is direction from Event to Station */

	double az, a, b, c, d, e, f, g, h, a1, b1, c1, d1, e1, f1, g1, h1, thg, ss, sc;

	/* Equations are unstable for latitudes of exactly 0 degrees. */
	if (latE == 0.0) latE = 1.0e-08;
	if (latS == 0.0) latS = 1.0e-08;

	/* Must convert from geographic to geocentric coordinates in order
	 * to use the spherical trig equations.  This requires a latitude
	 * correction given by: 1-ECC2=1-2*f + f*f = GMT->current.proj.one_m_ECC2
	 */

	thg = atan (GMT->current.proj.one_m_ECC2 * tand (latE));
	sincos (thg, &c, &f);		f = -f;
	sincosd (lonE, &d, &e);		e = -e;
	a = f * e;
	b = -f * d;
	g = -c * e;
	h = c * d;

	/* Calculating some trig constants. */

	thg = atan (GMT->current.proj.one_m_ECC2 * tand (latS));
	sincos (thg, &c1, &f1);		f1 = -f1;
	sincosd (lonS, &d1, &e1);	e1 = -e1;
	a1 = f1 * e1;
	b1 = -f1 * d1;
	g1 = -c1 * e1;
	h1 = c1 * d1;

	/* Spherical trig relationships used to compute angles. */

	if (baz) {	/* Get Backazimuth */
		ss = pow(a-d1,2.0) + pow(b-e1,2.0) + c * c - 2.0;
		sc = pow(a-g1,2.0) + pow(b-h1,2.0) + pow(c-f1,2.0) - 2.0;
	}
	else {		/* Get Azimuth */
		ss = pow(a1-d, 2.0) + pow(b1-e, 2.0) + c1 * c1 - 2.0;
		sc = pow(a1-g, 2.0) + pow(b1-h, 2.0) + pow(c1-f, 2.0) - 2.0;
	}
	az = atan2d (ss,sc);
	if (az < 0.0) az += 360.0;
	return (az);
}
#endif

double GMT_az_backaz (struct GMT_CTRL *GMT, double lonE, double latE, double lonS, double latS, bool baz)
{
	return (GMT->current.map.azimuth_func (GMT, lonE, latE, lonS, latS, baz));
}

void GMT_auto_frame_interval (struct GMT_CTRL *GMT, unsigned int axis, unsigned int item) {
	/* Determine the annotation and frame tick interval when they are not set (interval = 0) */
	int i = 0;
	bool set_a = false;
	double maj[7] = {2.0, 5.0, 10.0, 15.0, 30.0, 60.0, 90.0}, sub[7] = {1.0, 1.0, 2.0, 5.0, 10.0, 15.0, 30.0};
	double d, f, p;
	struct GMT_PLOT_AXIS *A = &GMT->current.map.frame.axis[axis];
	struct GMT_PLOT_AXIS_ITEM *T;

	if (A->type == GMT_LOG10 || A->type == GMT_POW) return;

	if (!(A->item[item].active && A->item[item].interval == 0.0) &&
		!(A->item[item+2].active && A->item[item+2].interval == 0.0) &&
		!(A->item[item+4].active && A->item[item+4].interval == 0.0)) return;

	/* f = frame width/height (inch); d = domain width/height (world coordinates) */
	if (axis == GMT_X) {
		f = fabs (GMT->current.proj.rect[XHI] - GMT->current.proj.rect[XLO]);
		d = fabs (GMT->common.R.wesn[XHI] - GMT->common.R.wesn[XLO]);
	}
	else if (axis == GMT_Y) {
		f = fabs (GMT->current.proj.rect[YHI] - GMT->current.proj.rect[YLO]);
		d = fabs (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]);
	}
	else {
		f = fabs (GMT->current.proj.zmax - GMT->current.proj.zmin);
		d = fabs (GMT->common.R.wesn[ZHI] - GMT->common.R.wesn[ZLO]);
	}
	f *= GMT->session.u2u[GMT_INCH][GMT_PT];	/* Change to points */

	/* First guess of interval */
	d *= MAX (0.05, MIN (5.0 * GMT->current.setting.font_annot[item].size / f, 0.20));

	/* Now determine 'round' major and minor tick intervals */
	if (GMT_axis_is_geo (GMT, axis))	/* Geographical coordinate */
		p = (d < GMT_MIN2DEG) ? GMT_SEC2DEG : (d < 1.0) ? GMT_MIN2DEG : 1.0; 
	else	/* General (linear) axis */
		p = pow (10.0, floor (log10 (d)));
	d /= p;	/* d is now in degrees, minutes or seconds, or in the range [1;10) */
	while (i < 6 && maj[i] < d) i++;
	d = maj[i] * p, f = sub[i] * p;

	/* Set annotation/major tick interval */
	T = &A->item[item];
	if (T->active && T->interval == 0.0) T->interval = d, set_a = true;

	/* Set minor ticks as well (if copied from annotation, set to major interval) */
	T = &A->item[item+2];
	if (T->active && T->interval == 0.0) T->interval = (T->type == 'f' || T->type == 'F') ? f : d;

	/* Finally set grid interval (if annotation set as well, use major, otherwise minor interval) */
	T = &A->item[item+4];
	if (T->active && T->interval == 0.0) T->interval = set_a ? d : f;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 *	S E C T I O N  1 :	M A P  - T R A N S F O R M A T I O N S
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * GMT_map_setup sets up the transformations for the chosen map projection.
 * The user must pass:
 *   w,e,s,n,parameters[0] - parameters[np-1] (np = number of parameters), and project:
 *   w,e,s,n defines the area in degrees.
 *   project == GMT_LINEAR, GMT_POLAR, GMT_MERCATOR, GMT_STEREO, GMT_LAMBERT, GMT_OBLIQUE_MERC, GMT_UTM,
 *	GMT_TM, GMT_ORTHO, GMT_AZ_EQDIST, GMT_LAMB_AZ_EQ, GMT_WINKEL, GMT_ROBINSON, GMT_CASSINI, GMT_ALBERS, GMT_ECONIC,
 *	GMT_ECKERT4, GMT_ECKERT6, GMT_CYL_EQ, GMT_CYL_EQDIST, GMT_CYL_STEREO, GMT_MILLER, GMT_VANGRINTEN
 *	For GMT_LINEAR, we may have GMT_LINEAR, GMT_LOG10, or GMT_POW
 *
 * parameters[0] through parameters[np-1] mean different things to the various
 * projections, as explained below. (np also varies, of course)
 *
 * LINEAR projection:
 *	parameters[0] is inch (or cm)/x_user_unit.
 *	parameters[1] is inch (or cm)/y_user_unit. If 0, then yscale = xscale.
 *	parameters[2] is pow for x^pow (if GMT_POW is on).
 *	parameters[3] is pow for y^pow (if GMT_POW is on).
 *
 * POLAR (r,theta) projection:
 *	parameters[0] is inch (or cm)/x_user_unit (radius)
 *
 * MERCATOR projection:
 *	parameters[0] is central meridian
 *	parameters[1] is standard parallel
 *	parameters[2] is in inch (or cm)/degree_longitude @ equator OR 1:xxxxx OR map-width
 *
 * STEREOGRAPHIC projection:
 *	parameters[0] is longitude of pole
 *	parameters[1] is latitude of pole
 *	parameters[2] is radius in inches (or cm) from pole to the latitude specified
 *	   by parameters[3] OR 1:xxxxx OR map-width.
 *
 * LAMBERT projection (Conic):
 *	parameters[0] is first standard parallel
 *	parameters[1] is second standard parallel
 *	parameters[2] is scale in inch (or cm)/degree along parallels OR 1:xxxxx OR map-width
 *
 * OBLIQUE MERCATOR projection:
 *	parameters[0] is longitude of origin
 *	parameters[1] is latitude of origin
 *	Definition a:
 *		parameters[2] is longitude of second point along oblique equator
 *		parameters[3] is latitude of second point along oblique equator
 *		parameters[4] is scale in inch (or cm)/degree along oblique equator OR 1:xxxxx OR map-width
 *	Definition b:
 *		parameters[2] is azimuth along oblique equator at origin
 *		parameters[3] is scale in inch (or cm)/degree along oblique equator OR 1:xxxxx OR map-width
 *	Definition c:
 *		parameters[2] is longitude of pole of projection
 *		parameters[3] is latitude of pole of projection
 *		parameters[4] is scale in inch (cm)/degree along oblique equator OR 1:xxxxx OR map-width
 *
 * TRANSVERSE MERCATOR (TM) projection
 *	parameters[0] is central meridian
 *	parameters[1] is central parallel
 *	parameters[2] is scale in inch (cm)/degree along this meridian OR 1:xxxxx OR map-width
 *
 * UNIVERSAL TRANSVERSE MERCATOR (UTM) projection
 *	parameters[0] is UTM zone (0-60, use negative for S hemisphere)
 *	parameters[1] is scale in inch (cm)/degree along this meridian OR 1:xxxxx OR map-width
 *
 * LAMBERT AZIMUTHAL EQUAL AREA projection:
 *	parameters[0] is longitude of origin
 *	parameters[1] is latitude of origin
 *	parameters[2] is radius in inches (or cm) from pole to the latitude specified
 *	   by parameters[3] OR 1:xxxxx OR map-width.
 *
 * ORTHOGRAPHIC AZIMUTHAL projection:
 *	parameters[0] is longitude of origin
 *	parameters[1] is latitude of origin
 *	parameters[2] is radius in inches (or cm) from pole to the latitude specified
 *	   by parameters[3] OR 1:xxxxx OR map-width.
 *
 * GENERAL PERSPECTIVE projection:
 *      parameters[0] is longitude of origin
 *      parameters[1] is latitude of origin
 *      parameters[2] is radius in inches (or cm) from pole to the latitude specified
 *         by parameters[3] OR 1:xxxxx OR map-width.
 *      parameters[4] is the altitude of the view point in kilometers
 *         if altitude is < 10 then it is the distance from the center of the Earth
 *              divided by the radius of the Earth
 *      parameters[5] is the azimuth east for North of the viewpoint
 *      parameters[6] is the tilt upward of the plane of projection
 *      parameters[7] is the width of the viewpoint in degrees
 *         if = 0, no viewpoint clipping
 *      parameters[8] is the height of the viewpoint in degrees
 *         if = 0, no viewpoint clipping
 *
 * AZIMUTHAL EQUIDISTANCE projection:
 *	parameters[0] is longitude of origin
 *	parameters[1] is latitude of origin
 *	parameters[2] is radius in inches (or cm) from pole to the latitude specified
 *	   by parameters[3] OR 1:xxxxx OR map-width.
 *
 * MOLLWEIDE EQUAL-AREA projection
 *	parameters[0] is longitude of origin
 *	parameters[1] is in inch (or cm)/degree_longitude @ equator OR 1:xxxxx OR map-width
 *
 * HAMMER-AITOFF EQUAL-AREA projection
 *	parameters[0] is longitude of origin
 *	parameters[1] is in inch (or cm)/degree_longitude @ equator OR 1:xxxxx OR map-width
 *
 * SINUSOIDAL EQUAL-AREA projection
 *	parameters[0] is longitude of origin
 *	parameters[1] is in inch (or cm)/degree_longitude @ equator OR 1:xxxxx OR map-width
 *
 * WINKEL TRIPEL MODIFIED AZIMUTHAL projection
 *	parameters[0] is longitude of origin
 *	parameters[1] is in inch (or cm)/degree_longitude @ equator OR 1:xxxxx OR map-width
 *
 * ROBINSON PSEUDOCYLINDRICAL projection
 *	parameters[0] is longitude of origin
 *	parameters[1] is in inch (or cm)/degree_longitude @ equator OR 1:xxxxx OR map-width
 *
 * VAN DER VANGRINTEN projection
 *	parameters[0] is longitude of origin
 *	parameters[1] is in inch (or cm)/degree_longitude @ equator OR 1:xxxxx OR map-width
 *
 * CASSINI projection
 *	parameters[0] is longitude of origin
 *	parameters[1] is latitude of origin
 *	parameters[2] is scale in inch (cm)/degree along this meridian OR 1:xxxxx OR map-width
 *
 * ALBERS projection (Conic):
 *	parameters[0] is first standard parallel
 *	parameters[1] is second standard parallel
 *	parameters[2] is scale in inch (or cm)/degree along parallels OR 1:xxxxx OR map-width
 *
 * CONIC EQUIDISTANT projection:
 *	parameters[0] is first standard parallel
 *	parameters[1] is second standard parallel
 *	parameters[2] is scale in inch (or cm)/degree along parallels OR 1:xxxxx OR map-width
 *
 * ECKERT6 IV projection:
 *	parameters[0] is longitude of origin
 *	parameters[1] is scale in inch (or cm)/degree along parallels OR 1:xxxxx OR map-width
 *
 * ECKERT6 IV projection:
 *	parameters[0] is longitude of origin
 *	parameters[1] is scale in inch (or cm)/degree along parallels OR 1:xxxxx OR map-width
 *
 * CYLINDRICAL EQUAL-AREA projections (Behrmann, Gall-Peters):
 *	parameters[0] is longitude of origin
 *	parameters[1] is the standard parallel
 *	parameters[2] is scale in inch (or cm)/degree along parallels OR 1:xxxxx OR map-width
 *
 * CYLINDRICAL STEREOGRAPHIC projections (Braun, Gall, B.S.A.M.):
 *	parameters[0] is longitude of origin
 *	parameters[1] is the standard parallel
 *	parameters[2] is scale in inch (or cm)/degree along parallels OR 1:xxxxx OR map-width
 *
 * MILLER CYLINDRICAL projection:
 *	parameters[0] is longitude of origin
 *	parameters[1] is scale in inch (or cm)/degree along parallels OR 1:xxxxx OR map-width
 *
 * Pointers to the correct map transformation functions will be set up so that
 * there are no if tests to determine which routine to call. These pointers
 * are forward and inverse, and are called from GMT_geo_to_xy and GMT_xy_to_geo.
 *
 */

/*
 *	GENERIC TRANSFORMATION ROUTINES FOR THE LINEAR PROJECTION
 */

double GMT_x_to_xx (struct GMT_CTRL *GMT, double x)
{	/* Converts x to xx using the current linear projection */
	double xx;
	(*GMT->current.proj.fwd_x) (GMT, x, &xx);
	return (xx * GMT->current.proj.scale[GMT_X] + GMT->current.proj.origin[GMT_X]);
}

double GMT_y_to_yy (struct GMT_CTRL *GMT, double y)
{	/* Converts y to yy using the current linear projection */
	double yy;
	(*GMT->current.proj.fwd_y) (GMT, y, &yy);
	return (yy * GMT->current.proj.scale[GMT_Y] + GMT->current.proj.origin[GMT_Y]);
}

double GMT_z_to_zz (struct GMT_CTRL *GMT, double z)
{	/* Converts z to zz using the current linear projection */
	double zz;
	(*GMT->current.proj.fwd_z) (GMT, z, &zz);
	return (zz * GMT->current.proj.scale[GMT_Z] + GMT->current.proj.origin[GMT_Z]);
}

double GMT_xx_to_x (struct GMT_CTRL *GMT, double xx)
{	/* Converts xx back to x using the current linear projection */
	double x;
	xx = (xx - GMT->current.proj.origin[GMT_X]) * GMT->current.proj.i_scale[GMT_X];
	(*GMT->current.proj.inv_x) (GMT, &x, xx);
	return (x);
}

double GMT_yy_to_y (struct GMT_CTRL *GMT, double yy)
{	/* Converts yy back to y using the current linear projection */
	double y;
	yy = (yy - GMT->current.proj.origin[GMT_Y]) * GMT->current.proj.i_scale[GMT_Y];
	(*GMT->current.proj.inv_y) (GMT, &y, yy);
	return (y);
}

double GMT_zz_to_z (struct GMT_CTRL *GMT, double zz)
{	/* Converts zz back to z using the current linear projection */
	double z;
	zz = (zz - GMT->current.proj.origin[GMT_Z]) * GMT->current.proj.i_scale[GMT_Z];
	(*GMT->current.proj.inv_z) (GMT, &z, zz);
	return (z);
}

bool GMT_geo_to_xy (struct GMT_CTRL *GMT, double lon, double lat, double *x, double *y)
{	/* Converts lon/lat to x/y using the current projection */
	if (GMT_is_dnan (lon) || GMT_is_dnan (lat)) {(*x) = (*y) = GMT->session.d_NaN; return true;}	/* Quick and safe way to ensure NaN-input results in NaNs */
	(*GMT->current.proj.fwd) (GMT, lon, lat, x, y);
	(*x) = (*x) * GMT->current.proj.scale[GMT_X] + GMT->current.proj.origin[GMT_X];
	(*y) = (*y) * GMT->current.proj.scale[GMT_Y] + GMT->current.proj.origin[GMT_Y];
	return false;
}

void GMT_xy_to_geo (struct GMT_CTRL *GMT, double *lon, double *lat, double x, double y)
{
	/* Converts x/y to lon/lat using the current projection */

	if (GMT_is_dnan (x) || GMT_is_dnan (y)) {(*lon) = (*lat) = GMT->session.d_NaN; return;}	/* Quick and safe way to ensure NaN-input results in NaNs */
	x = (x - GMT->current.proj.origin[GMT_X]) * GMT->current.proj.i_scale[GMT_X];
	y = (y - GMT->current.proj.origin[GMT_Y]) * GMT->current.proj.i_scale[GMT_Y];

	(*GMT->current.proj.inv) (GMT, lon, lat, x, y);
}

void GMT_geoz_to_xy (struct GMT_CTRL *GMT, double x, double y, double z, double *x_out, double *y_out)
{	/* Map-projects xy first, the projects xyz onto xy plane */
	double x0, y0;
	GMT_geo_to_xy (GMT, x, y, &x0, &y0);
	GMT_xyz_to_xy (GMT, x0, y0, GMT_z_to_zz (GMT, z), x_out, y_out);
}

void GMT_xyz_to_xy (struct GMT_CTRL *GMT, double x, double y, double z, double *x_out, double *y_out)
{	/* projects xyz (inches) onto perspective xy plane (inches) */
	*x_out = - x * GMT->current.proj.z_project.cos_az + y * GMT->current.proj.z_project.sin_az + GMT->current.proj.z_project.x_off;
	*y_out = - (x * GMT->current.proj.z_project.sin_az + y * GMT->current.proj.z_project.cos_az) * GMT->current.proj.z_project.sin_el + z * GMT->current.proj.z_project.cos_el + GMT->current.proj.z_project.y_off;
}

void GMT_xyz_to_xy_n (struct GMT_CTRL *GMT, double *x, double *y, double z, uint64_t n)
{	/* projects xyz (inches) onto perspective xy plane (inches) for multiple points */
	uint64_t i;
	for (i = 0; i < n; i++) GMT_xyz_to_xy (GMT, x[i], y[i], z, &x[i], &y[i]);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE LINEAR PROJECTION (GMT_LINEAR)
 */

void gmt_linearxy (struct GMT_CTRL *GMT, double x, double y, double *x_i, double *y_i)
{	/* Transform both x and y linearly */
	(*GMT->current.proj.fwd_x) (GMT, x, x_i);
	(*GMT->current.proj.fwd_y) (GMT, y, y_i);
}

void gmt_ilinearxy (struct GMT_CTRL *GMT, double *x, double *y, double x_i, double y_i)
{	/* Inversely transform both x and y linearly */
	(*GMT->current.proj.inv_x) (GMT, x, x_i);
	(*GMT->current.proj.inv_y) (GMT, y, y_i);
}

bool gmt_map_init_linear (struct GMT_CTRL *GMT) {
	bool positive;
	double xmin = 0.0, xmax = 0.0, ymin = 0.0, ymax = 0.0;

	GMT->current.map.left_edge  = &gmt_left_rect;
	GMT->current.map.right_edge = &gmt_right_rect;
	GMT->current.proj.fwd = &gmt_linearxy;
	GMT->current.proj.inv = &gmt_ilinearxy;
	if (GMT_x_is_lon (GMT, GMT_IN)) {	/* x is longitude */
		GMT->current.proj.central_meridian = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
		GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	}
	else
		GMT->current.map.lon_wrap = false;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.pars[0];
	GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[1];
	if (GMT->current.proj.scale[GMT_X] < 0.0) GMT->current.proj.xyz_pos[GMT_X] = false;	/* User wants x to increase left */
	if (GMT->current.proj.scale[GMT_Y] < 0.0) GMT->current.proj.xyz_pos[GMT_Y] = false;	/* User wants y to increase down */
	switch ( (GMT->current.proj.xyz_projection[GMT_X]%3)) {	/* Modulo 3 so that GMT_TIME (3) maps to GMT_LINEAR (0) */
		case GMT_LINEAR:	/* Regular scaling */
			if (GMT->current.io.col_type[GMT_IN][GMT_X] == GMT_IS_ABSTIME && GMT->current.proj.xyz_projection[GMT_X] != GMT_TIME)
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Warning -JX|x option: Your x-column contains absolute time but -JX|x...T was not specified!\n");
			GMT->current.proj.fwd_x = ((GMT_x_is_lon (GMT, GMT_IN)) ? &GMT_translind  : &GMT_translin);
			GMT->current.proj.inv_x = ((GMT_x_is_lon (GMT, GMT_IN)) ? &GMT_itranslind : &GMT_itranslin);
			if (GMT->current.proj.xyz_pos[GMT_X]) {
				(*GMT->current.proj.fwd_x) (GMT, GMT->common.R.wesn[XLO], &xmin);
				(*GMT->current.proj.fwd_x) (GMT, GMT->common.R.wesn[XHI], &xmax);
			}
			else {
				(*GMT->current.proj.fwd_x) (GMT, GMT->common.R.wesn[XHI], &xmin);
				(*GMT->current.proj.fwd_x) (GMT, GMT->common.R.wesn[XLO], &xmax);
			}
			break;
		case GMT_LOG10:	/* Log10 transformation */
			if (GMT->common.R.wesn[XLO] <= 0.0 || GMT->common.R.wesn[XHI] <= 0.0) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Syntax error -JX|x option:  Limits must be positive for log10 option\n");
				GMT_exit (GMT, EXIT_FAILURE); return false;
			}
			xmin = (GMT->current.proj.xyz_pos[GMT_X]) ? d_log10 (GMT, GMT->common.R.wesn[XLO]) : d_log10 (GMT, GMT->common.R.wesn[XHI]);
			xmax = (GMT->current.proj.xyz_pos[GMT_X]) ? d_log10 (GMT, GMT->common.R.wesn[XHI]) : d_log10 (GMT, GMT->common.R.wesn[XLO]);
			GMT->current.proj.fwd_x = &GMT_translog10;
			GMT->current.proj.inv_x = &GMT_itranslog10;
			break;
		case GMT_POW:	/* x^y transformation */
			GMT->current.proj.xyz_pow[GMT_X] = GMT->current.proj.pars[2];
			GMT->current.proj.xyz_ipow[GMT_X] = 1.0 / GMT->current.proj.pars[2];
			positive = !((GMT->current.proj.xyz_pos[GMT_X] + (GMT->current.proj.xyz_pow[GMT_X] > 0.0)) % 2);
			xmin = (positive) ? pow (GMT->common.R.wesn[XLO], GMT->current.proj.xyz_pow[GMT_X]) : pow (GMT->common.R.wesn[XHI], GMT->current.proj.xyz_pow[GMT_X]);
			xmax = (positive) ? pow (GMT->common.R.wesn[XHI], GMT->current.proj.xyz_pow[GMT_X]) : pow (GMT->common.R.wesn[XLO], GMT->current.proj.xyz_pow[GMT_X]);
			GMT->current.proj.fwd_x = &GMT_transpowx;
			GMT->current.proj.inv_x = &GMT_itranspowx;
			break;
	}
	switch (GMT->current.proj.xyz_projection[GMT_Y]%3) {	/* Modulo 3 so that GMT_TIME (3) maps to GMT_LINEAR (0) */
		case GMT_LINEAR:	/* Regular scaling */
			if (GMT->current.io.col_type[GMT_IN][GMT_Y] == GMT_IS_ABSTIME && GMT->current.proj.xyz_projection[GMT_Y] != GMT_TIME)
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Warning -JX|x option:  Your y-column contains absolute time but -JX|x...T was not specified!\n");
			ymin = (GMT->current.proj.xyz_pos[GMT_Y]) ? GMT->common.R.wesn[YLO] : GMT->common.R.wesn[YHI];
			ymax = (GMT->current.proj.xyz_pos[GMT_Y]) ? GMT->common.R.wesn[YHI] : GMT->common.R.wesn[YLO];
			GMT->current.proj.fwd_y = &GMT_translin;
			GMT->current.proj.inv_y = &GMT_itranslin;
			break;
		case GMT_LOG10:	/* Log10 transformation */
			if (GMT->common.R.wesn[YLO] <= 0.0 || GMT->common.R.wesn[YHI] <= 0.0) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Syntax error -JX|x option:  Limits must be positive for log10 option\n");
				GMT_exit (GMT, EXIT_FAILURE); return false;
			}
			ymin = (GMT->current.proj.xyz_pos[GMT_Y]) ? d_log10 (GMT, GMT->common.R.wesn[YLO]) : d_log10 (GMT, GMT->common.R.wesn[YHI]);
			ymax = (GMT->current.proj.xyz_pos[GMT_Y]) ? d_log10 (GMT, GMT->common.R.wesn[YHI]) : d_log10 (GMT, GMT->common.R.wesn[YLO]);
			GMT->current.proj.fwd_y = &GMT_translog10;
			GMT->current.proj.inv_y = &GMT_itranslog10;
			break;
		case GMT_POW:	/* x^y transformation */
			GMT->current.proj.xyz_pow[GMT_Y] = GMT->current.proj.pars[3];
			GMT->current.proj.xyz_ipow[GMT_Y] = 1.0 / GMT->current.proj.pars[3];
			positive = !((GMT->current.proj.xyz_pos[GMT_Y] + (GMT->current.proj.xyz_pow[GMT_Y] > 0.0)) % 2);
			ymin = (positive) ? pow (GMT->common.R.wesn[YLO], GMT->current.proj.xyz_pow[GMT_Y]) : pow (GMT->common.R.wesn[YHI], GMT->current.proj.xyz_pow[GMT_Y]);
			ymax = (positive) ? pow (GMT->common.R.wesn[YHI], GMT->current.proj.xyz_pow[GMT_Y]) : pow (GMT->common.R.wesn[YLO], GMT->current.proj.xyz_pow[GMT_Y]);
			GMT->current.proj.fwd_y = &GMT_transpowy;
			GMT->current.proj.inv_y = &GMT_itranspowy;
	}

	/* Was given axes length instead of scale? */

	if (GMT->current.proj.compute_scale[GMT_X]) GMT->current.proj.scale[GMT_X] /= fabs (xmin - xmax);
	if (GMT->current.proj.compute_scale[GMT_Y]) GMT->current.proj.scale[GMT_Y] /= fabs (ymin - ymax);

	/* If either is zero, adjust width or height to the other */

	if (GMT->current.proj.scale[GMT_X] == 0) {
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y];
		GMT->current.proj.pars[0] = GMT->current.proj.scale[GMT_X] * fabs (xmin - xmax);
	}
	if (GMT->current.proj.scale[GMT_Y] == 0) {
		GMT->current.proj.scale[GMT_Y] = GMT->current.proj.scale[GMT_X];
		GMT->current.proj.pars[1] = GMT->current.proj.scale[GMT_Y] * fabs (ymin - ymax);
	}

	/* This override ensures that when using -J[x|X]...d degrees work as meters */

	GMT->current.proj.M_PR_DEG = 1.0;
	GMT->current.proj.KM_PR_DEG = GMT->current.proj.M_PR_DEG / METERS_IN_A_KM;

	gmt_map_setxy (GMT, xmin, xmax, ymin, ymax);
	if (GMT->current.io.col_type[GMT_IN][GMT_X] == GMT_IS_LON) {	/* Using linear projection with longitudes */
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
	}
	else {
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
	}
	GMT->current.map.n_lat_nodes = 2;
	GMT->current.map.n_lon_nodes = 3;	/* > 2 to avoid map-jumps */
	GMT->current.map.frame.check_side = true;
	GMT->current.map.frame.horizontal = 1;
	GMT->current.map.meridian_straight = GMT->current.map.parallel_straight = 1;

	return (false);
}

/*
 *	TRANSFORMATION ROUTINES FOR POLAR (theta,r) PROJECTION (GMT_POLAR)
 */

bool gmt_map_init_polar (struct GMT_CTRL *GMT)
{
	double xmin, xmax, ymin, ymax;

	GMT_vpolar (GMT, GMT->current.proj.pars[1]);
	if (GMT->current.proj.got_elevations) {	/* Requires s >= 0 and n <= 90 */
		if (GMT->common.R.wesn[YLO] < 0.0 || GMT->common.R.wesn[YHI] > 90.0) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: -JP...r for elevation plots requires s >= 0 and n <= 90!\n");
			GMT_exit (GMT, EXIT_FAILURE); return false;
		}
		if (doubleAlmostEqual (GMT->common.R.wesn[YHI], 90.0))
			GMT->current.proj.edge[2] = false;
	}
	else {
		if (GMT_IS_ZERO (GMT->common.R.wesn[YLO]))
			GMT->current.proj.edge[0] = false;
	}
	if (GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])) GMT->current.proj.edge[1] = GMT->current.proj.edge[3] = false;
	GMT->current.map.left_edge = &gmt_left_circle;
	GMT->current.map.right_edge = &gmt_right_circle;
	GMT->current.proj.fwd = &GMT_polar;
	GMT->current.proj.inv = &GMT_ipolar;
	GMT->current.map.is_world = false;	/* There is no wrapping around here */
	gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[0];
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[0]);
	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);

	/* GMT->current.proj.r = 0.5 * GMT->current.proj.rect[XHI]; */
	GMT->current.proj.r = GMT->current.proj.scale[GMT_Y] * GMT->common.R.wesn[YHI];
	GMT->current.map.outside = &gmt_polar_outside;
	GMT->current.map.crossing = &gmt_wesn_crossing;
	GMT->current.map.overlap = &gmt_wesn_overlap;
	GMT->current.map.clip = &GMT_wesn_clip;
	GMT->current.map.frame.horizontal = 1;
	if (!GMT->current.proj.got_elevations) GMT->current.plot.r_theta_annot = true;	/* Special labeling case (see GMT_get_annot_label) */
	GMT->current.map.n_lat_nodes = 2;
	GMT->current.map.meridian_straight = 1;

	return (false);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE MERCATOR PROJECTION (GMT_MERCATOR)
 */

bool gmt_map_init_merc (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax, D = 1.0;

	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) {	/* Set fudge factor */
		GMT_scale_eqrad (GMT);
		D = GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].eq_radius / GMT->current.proj.GMT_lat_swap_vals.rm;
	}
	if (GMT->common.R.wesn[YLO] <= -90.0 || GMT->common.R.wesn[YHI] >= 90.0) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Syntax error -R option:  Cannot include south/north poles with Mercator projection!\n");
		GMT_exit (GMT, EXIT_FAILURE); return false;
	}
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	gmt_cyl_validate_clon (GMT, 0);	/* Make sure the central longitude is valid */
	GMT_vmerc (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	GMT->current.proj.j_x *= D;
	GMT->current.proj.j_ix /= D;
	GMT->current.proj.fwd = &GMT_merc_sph;
	GMT->current.proj.inv = &GMT_imerc_sph;
	(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
	(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[2] /= (D * GMT->current.proj.M_PR_DEG);
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[2];
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[2]);
	GMT->current.map.n_lat_nodes = 2;
	GMT->current.map.n_lon_nodes = 3;	/* > 2 to avoid map-jumps */
	GMT->current.map.outside = &gmt_wesn_outside;
	GMT->current.map.crossing = &gmt_wesn_crossing;
	GMT->current.map.overlap = &gmt_wesn_overlap;
	GMT->current.map.clip = &GMT_wesn_clip;
	GMT->current.map.left_edge = &gmt_left_rect;
	GMT->current.map.right_edge = &gmt_right_rect;
	GMT->current.map.frame.horizontal = 1;
	GMT->current.map.frame.check_side = true;
	GMT->current.map.meridian_straight = GMT->current.map.parallel_straight = 1;

	return (false);	/* No need to search for wesn */
}

/*
 *	TRANSFORMATION ROUTINES FOR CYLINDRICAL EQUAL-AREA PROJECTIONS (GMT_CYL_EQ)
 */

bool gmt_map_init_cyleq (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT->current.proj.Dx = GMT->current.proj.Dy = 0.0;
	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) {
		double D, k0, qp, slat, e, e2;
		GMT_scale_eqrad (GMT);
		slat = GMT->current.proj.pars[1];
		GMT->current.proj.pars[1] = GMT_latg_to_lata (GMT, GMT->current.proj.pars[1]);
		e = GMT->current.proj.ECC;
		e2 = GMT->current.proj.ECC2;
		qp = 1.0 - 0.5 * (1.0 - e2) * log ((1.0 - e) / (1.0 + e)) / e;
		k0 = cosd (slat) / d_sqrt (1.0 - e2 * sind (GMT->current.proj.pars[1]) * sind (GMT->current.proj.pars[1]));
		D = k0 / cosd (GMT->current.proj.pars[1]);
		GMT->current.proj.Dx = D;
		GMT->current.proj.Dy = 0.5 * qp / D;
	}
	GMT->current.proj.iDx = 1.0 / GMT->current.proj.Dx;
	GMT->current.proj.iDy = 1.0 / GMT->current.proj.Dy;
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	gmt_cyl_validate_clon (GMT, 1);	/* Make sure the central longitude is valid */
	GMT_vcyleq (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	GMT_cyleq (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
	GMT_cyleq (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[2] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[2];
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[2]);
	GMT->current.map.n_lat_nodes = 2;
	GMT->current.map.n_lon_nodes = 3;	/* > 2 to avoid map-jumps */
	GMT->current.proj.fwd = &GMT_cyleq;
	GMT->current.proj.inv = &GMT_icyleq;
	GMT->current.map.outside = &gmt_wesn_outside;
	GMT->current.map.crossing = &gmt_wesn_crossing;
	GMT->current.map.overlap = &gmt_wesn_overlap;
	GMT->current.map.clip = &GMT_wesn_clip;
	GMT->current.map.left_edge = &gmt_left_rect;
	GMT->current.map.right_edge = &gmt_right_rect;
	GMT->current.map.frame.horizontal = 1;
	GMT->current.map.frame.check_side = true;
	GMT->current.map.meridian_straight = GMT->current.map.parallel_straight = 1;

	return (false);	/* No need to search for wesn */
}

/*
 *	TRANSFORMATION ROUTINES FOR CYLINDRICAL EQUIDISTANT PROJECTION (GMT_CYL_EQDIST)
 */

bool gmt_map_init_cyleqdist (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT_set_spherical (GMT, true);	/* Force spherical for now */

	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	gmt_cyl_validate_clon (GMT, 1);	/* Make sure the central longitude is valid */
	GMT_vcyleqdist (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	GMT_cyleqdist (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
	GMT_cyleqdist (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[2] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[2];
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[2]);
	GMT->current.map.n_lat_nodes = 2;
	GMT->current.map.n_lon_nodes = 3;	/* > 2 to avoid map-jumps */
	GMT->current.proj.fwd = &GMT_cyleqdist;
	GMT->current.proj.inv = &GMT_icyleqdist;
	GMT->current.map.outside = &gmt_wesn_outside;
	GMT->current.map.crossing = &gmt_wesn_crossing;
	GMT->current.map.overlap = &gmt_wesn_overlap;
	GMT->current.map.clip = &GMT_wesn_clip;
	GMT->current.map.left_edge = &gmt_left_rect;
	GMT->current.map.right_edge = &gmt_right_rect;
	GMT->current.map.frame.horizontal = 1;
	GMT->current.map.frame.check_side = true;
	GMT->current.map.meridian_straight = GMT->current.map.parallel_straight = 1;

	return (false);	/* No need to search for wesn */
}

/*
 *	TRANSFORMATION ROUTINES FOR MILLER CYLINDRICAL PROJECTION (GMT_MILLER)
 */

bool gmt_map_init_miller (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT_set_spherical (GMT, true);	/* Force spherical for now */

	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	gmt_cyl_validate_clon (GMT, 1);	/* Make sure the central longitude is valid */
	GMT_vmiller (GMT, GMT->current.proj.pars[0]);
	GMT_miller (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
	GMT_miller (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[1];
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);
	GMT->current.map.n_lat_nodes = 2;
	GMT->current.map.n_lon_nodes = 3;	/* > 2 to avoid map-jumps */
	GMT->current.proj.fwd = &GMT_miller;
	GMT->current.proj.inv = &GMT_imiller;
	GMT->current.map.outside = &gmt_wesn_outside;
	GMT->current.map.crossing = &gmt_wesn_crossing;
	GMT->current.map.overlap = &gmt_wesn_overlap;
	GMT->current.map.clip = &GMT_wesn_clip;
	GMT->current.map.left_edge = &gmt_left_rect;
	GMT->current.map.right_edge = &gmt_right_rect;
	GMT->current.map.frame.horizontal = 1;
	GMT->current.map.frame.check_side = true;
	GMT->current.map.meridian_straight = GMT->current.map.parallel_straight = 1;

	return (false);	/* No need to search for wesn */
}

/*
 *	TRANSFORMATION ROUTINES FOR CYLINDRICAL STEREOGRAPHIC PROJECTIONS (GMT_CYL_STEREO)
 */

bool gmt_map_init_cylstereo (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT_set_spherical (GMT, true);	/* Force spherical for now */

	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	gmt_cyl_validate_clon (GMT, 1);	/* Make sure the central longitude is valid */
	GMT_vcylstereo (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	GMT_cylstereo (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
	GMT_cylstereo (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[2] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[2];
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[2]);
	GMT->current.map.n_lat_nodes = 2;
	GMT->current.map.n_lon_nodes = 3;	/* > 2 to avoid map-jumps */
	GMT->current.proj.fwd = &GMT_cylstereo;
	GMT->current.proj.inv = &GMT_icylstereo;
	GMT->current.map.outside = &gmt_wesn_outside;
	GMT->current.map.crossing = &gmt_wesn_crossing;
	GMT->current.map.overlap = &gmt_wesn_overlap;
	GMT->current.map.clip = &GMT_wesn_clip;
	GMT->current.map.left_edge = &gmt_left_rect;
	GMT->current.map.right_edge = &gmt_right_rect;
	GMT->current.map.frame.horizontal = 1;
	GMT->current.map.frame.check_side = true;
	GMT->current.map.meridian_straight = GMT->current.map.parallel_straight = 1;

	return (false);	/* No need to search for wesn */
}


/*
 *	TRANSFORMATION ROUTINES FOR THE POLAR STEREOGRAPHIC PROJECTION (GMT_STEREO)
 */

bool gmt_map_init_stereo (struct GMT_CTRL *GMT) {
	unsigned int i;
	double xmin, xmax, ymin, ymax, dummy, radius, latg, D = 1.0;

	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	latg = GMT->current.proj.pars[1];

	gmt_set_polar (GMT);

	if (GMT->current.setting.proj_scale_factor == -1.0) GMT->current.setting.proj_scale_factor = 0.9996;	/* Select default map scale for Stereographic */
	if (GMT->current.proj.polar && (lrint (GMT->current.proj.pars[5]) == 1)) GMT->current.setting.proj_scale_factor = 1.0;	/* Gave true scale at given parallel set below */
	/* Equatorial view has a problem with infinite loops.  Until I find a cure
	  we set projection center latitude to 0.001 so equatorial works for now */

	if (fabs (GMT->current.proj.pars[1]) < GMT_SMALL) GMT->current.proj.pars[1] = 0.001;

	GMT_vstereo (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2]);

	if (GMT->current.proj.GMT_convert_latitudes) {	/* Set fudge factors when conformal latitudes are used */
		double e1p, e1m, s, c;

		D = GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].eq_radius / GMT->current.proj.GMT_lat_swap_vals.rm;
		if (GMT->current.proj.polar) {
			e1p = 1.0 + GMT->current.proj.ECC;	e1m = 1.0 - GMT->current.proj.ECC;
			D /= d_sqrt (pow (e1p, e1p) * pow (e1m, e1m));
			if (lrint (GMT->current.proj.pars[5]) == 1) {	/* Gave true scale at given parallel */
				double k_p, m_c, t_c, es;

				sincosd (fabs (GMT->current.proj.pars[4]), &s, &c);
				es = GMT->current.proj.ECC * s;
				m_c = c / d_sqrt (1.0 - GMT->current.proj.ECC2 * s * s);
				t_c = d_sqrt (((1.0 - s) / (1.0 + s)) * pow ((1.0 + es) / (1.0 - es), GMT->current.proj.ECC));
				k_p = 0.5 * m_c * d_sqrt (pow (e1p, e1p) * pow (e1m, e1m)) / t_c;
				D *= k_p;
			}
		}
		else {
			sincosd (latg, &s, &c);	/* Need original geographic pole coordinates */
			D *= (c / (GMT->current.proj.cosp * d_sqrt (1.0 - GMT->current.proj.ECC2 * s * s)));
		}
	}
	GMT->current.proj.Dx = GMT->current.proj.Dy = D;

	GMT->current.proj.iDx = 1.0 / GMT->current.proj.Dx;
	GMT->current.proj.iDy = 1.0 / GMT->current.proj.Dy;

	if (GMT->current.proj.polar) {	/* Polar aspect */
		GMT->current.proj.fwd = &GMT_plrs_sph;
		GMT->current.proj.inv = &GMT_iplrs_sph;
		if (GMT->current.proj.units_pr_degree) {
			(*GMT->current.proj.fwd) (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[4], &dummy, &radius);
			GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = fabs (GMT->current.proj.pars[3] / radius);
		}
		else
			GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[3];
		GMT->current.map.meridian_straight = 1;
	}
	else {
		GMT->current.proj.fwd = (GMT_IS_ZERO (GMT->current.proj.pole)) ? &GMT_stereo2_sph : &GMT_stereo1_sph;
		GMT->current.proj.inv = &GMT_istereo_sph;
		if (GMT->current.proj.units_pr_degree) {
			GMT_vstereo (GMT, 0.0, 90.0, GMT->current.proj.pars[2]);
			(*GMT->current.proj.fwd) (GMT, 0.0, fabs (GMT->current.proj.pars[4]), &dummy, &radius);
			GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = fabs (GMT->current.proj.pars[3] / radius);
		}
		else
			GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[3];

		GMT_vstereo (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2]);
	}


	if (GMT->common.R.oblique) {	/* Rectangular box given */
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);

		GMT->current.map.outside = &gmt_rect_outside2;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = !(GMT->current.setting.map_annot_oblique & 1);
		GMT->current.map.frame.horizontal = (fabs (GMT->current.proj.pars[1]) < 30.0 && fabs (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]) < 30.0) ? 1 : 0;
	}
	else {
		if (GMT->current.proj.polar) {	/* Polar aspect */
			if (GMT->current.proj.north_pole) {
				if (GMT->common.R.wesn[YLO] <= -90.0) {
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: South boundary cannot be -90.0 for north polar stereographic projection\n");
					GMT_exit (GMT, EXIT_FAILURE); return false;
				}
				if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;
			}
			else {
				if (GMT->common.R.wesn[YHI] >= 90.0) {
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: North boundary cannot be +90.0 for south polar stereographic projection\n");
					GMT_exit (GMT, EXIT_FAILURE); return false;
				}
				if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
			}
			if (GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])
					|| doubleAlmostEqualZero (GMT->common.R.wesn[XHI], GMT->common.R.wesn[XLO]))
				GMT->current.proj.edge[1] = GMT->current.proj.edge[3] = false;
			GMT->current.map.outside = &gmt_polar_outside;
			GMT->current.map.crossing = &gmt_wesn_crossing;
			GMT->current.map.overlap = &gmt_wesn_overlap;
			GMT->current.map.clip = &GMT_wesn_clip;
			GMT->current.map.frame.horizontal = 1;
			GMT->current.map.n_lat_nodes = 2;
			gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		}
		else {	/* Global view only */
			/* No annotations or tickmarks in global mode */
			for (i = 0; i < GMT_GRID_UPPER; i++)
				GMT->current.map.frame.axis[GMT_X].item[i].active = GMT->current.map.frame.axis[GMT_Y].item[i].active = false,
				GMT->current.map.frame.axis[GMT_X].item[i].interval = GMT->current.map.frame.axis[GMT_Y].item[i].interval = 0.0;
			GMT->common.R.wesn[XLO] = 0.0;
			GMT->common.R.wesn[XHI] = 360.0;
			GMT->common.R.wesn[YLO] = -90.0;
			GMT->common.R.wesn[YHI] = 90.0;
			xmax = ymax = GMT->current.proj.rho_max;
			xmin = ymin = -xmax;
			GMT->current.map.outside = &gmt_radial_outside;
			GMT->current.map.crossing = &gmt_radial_crossing;
			GMT->current.map.overlap = &gmt_radial_overlap;
			GMT->current.map.clip = &gmt_radial_clip;
			if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
		}
		GMT->current.map.left_edge = &gmt_left_circle;
		GMT->current.map.right_edge = &gmt_right_circle;
	}

	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[3]);
	GMT->current.proj.r = 0.5 * GMT->current.proj.rect[XHI];
	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE LAMBERT CONFORMAL CONIC PROJECTION (GMT_LAMBERT)
 */

bool gmt_map_init_lambert (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT->current.proj.GMT_convert_latitudes = gmt_quickconic (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);
	GMT_vlamb (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2], GMT->current.proj.pars[3]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[4] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[4];
	if (GMT_IS_SPHERICAL (GMT) || GMT->current.proj.GMT_convert_latitudes) {	/* Spherical code w/wo conformal latitudes */
		GMT->current.proj.fwd = &GMT_lamb_sph;
		GMT->current.proj.inv = &GMT_ilamb_sph;
	}
	else {
		GMT->current.proj.fwd = &GMT_lamb;
		GMT->current.proj.inv = &GMT_ilamb;
	}

	if (GMT->common.R.oblique) {	/* Rectangular box given*/
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);

		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &gmt_left_conic;
		GMT->current.map.right_edge = &gmt_right_conic;
	}
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[4]);
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	GMT->current.map.n_lat_nodes = 2;
	GMT->current.map.frame.horizontal = 1;
	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);
	GMT->current.map.meridian_straight = 1;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE OBLIQUE MERCATOR PROJECTION (GMT_OBLIQUE_MERC)
 */

void gmt_pole_rotate_forward (struct GMT_CTRL *GMT, double lon, double lat, double *tlon, double *tlat)
{
	/* Given the pole position in GMT->current.proj, geographical coordinates
	 * are computed from oblique coordinates assuming a spherical earth.
	 * Latitutes and longitudes are in degrees.
	 */

	double sin_lat, cos_lat, cos_lon, sin_lon, cc;

	sincosd (lat, &sin_lat, &cos_lat);
	sincosd (lon - GMT->current.proj.o_pole_lon, &sin_lon, &cos_lon);
	cc = cos_lat * cos_lon;
	*tlat = d_asind (GMT->current.proj.o_sin_pole_lat * sin_lat + GMT->current.proj.o_cos_pole_lat * cc);
	*tlon = GMT->current.proj.o_beta + d_atan2d (cos_lat * sin_lon, GMT->current.proj.o_sin_pole_lat * cc - GMT->current.proj.o_cos_pole_lat * sin_lat);
}

#if 0
/* Not curently used in GMT */
void GMT_pole_rotate_inverse (struct GMT_CTRL *GMT, double *lon, double *lat, double tlon, double tlat)
{
	/* Given the pole position in GMT->current.proj, geographical coordinates
	 * are computed from oblique coordinates assuming a spherical earth.
	 * Latitutes and longitudes are in degrees.
	 */

	double sin_tlat, cos_tlat, cos_tlon, sin_tlon, cc;

	sincosd (tlat, &sin_tlat, &cos_tlat);
	sincosd (tlon - GMT->current.proj.o_beta, &sin_tlon, &cos_tlon);
	cc = cos_tlat * cos_tlon;
	*lat = d_asind (GMT->current.proj.o_sin_pole_lat * sin_tlat - GMT->current.proj.o_cos_pole_lat * cc);
	*lon = GMT->current.proj.o_pole_lon + d_atan2d (cos_tlat * sin_tlon, GMT->current.proj.o_sin_pole_lat * cc + GMT->current.proj.o_cos_pole_lat * sin_tlat);
}
#endif

void gmt_get_origin (struct GMT_CTRL *GMT, double lon1, double lat1, double lon_p, double lat_p, double *lon2, double *lat2)
{
	double beta, dummy, d, az, c;

	/* Now find origin that is 90 degrees from pole, let oblique lon=0 go through lon1/lat1 */
	c = cosd (lat_p) * cosd (lat1) * cosd (lon1-lon_p);
	c = d_acosd (sind (lat_p) * sind (lat1) + c);

	if (c != 90.0) {	/* Not true origin */
		d = fabs (90.0 - c);
		az = d_asind (sind (lon_p-lon1) * cosd (lat_p) / sind (c));
		if (c < 90.0) az += 180.0;
		*lat2 = d_asind (sind (lat1) * cosd (d) + cosd (lat1) * sind (d) * cosd (az));
		*lon2 = lon1 + d_atan2d (sind (d) * sind (az), cosd (lat1) * cosd (d) - sind (lat1) * sind (d) * cosd (az));
		GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Warning: Correct projection origin = %g/%g\n", *lon2, *lat2);
	}
	else {
		*lon2 = lon1;
		*lat2 = lat1;
	}

	gmt_pole_rotate_forward (GMT, *lon2, *lat2, &beta, &dummy);

	GMT->current.proj.o_beta = -beta;
}

void gmt_get_rotate_pole (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2)
{
	double plon, plat, beta, dummy, x, y;
	double sin_lon1, cos_lon1, sin_lon2, cos_lon2, sin_lat1, cos_lat1, sin_lat2, cos_lat2;

	sincosd (lon1, &sin_lon1, &cos_lon1);
	sincosd (lon2, &sin_lon2, &cos_lon2);
	sincosd (lat1, &sin_lat1, &cos_lat1);
	sincosd (lat2, &sin_lat2, &cos_lat2);

	y = cos_lat1 * sin_lat2 * cos_lon1 - sin_lat1 * cos_lat2 * cos_lon2;
	x = sin_lat1 * cos_lat2 * sin_lon2 - cos_lat1 * sin_lat2 * sin_lon1;
	plon = d_atan2d (y, x);
	plat = atand (-cosd (plon - lon1) / tand (lat1));
	if (plat < 0.0) {
		plat = -plat;
		plon += 180.0;
		if (plon >= 360.0) plon -= 360.0;
	}
	GMT->current.proj.o_pole_lon = plon;
	GMT->current.proj.o_pole_lat = plat;
	sincosd (plat, &GMT->current.proj.o_sin_pole_lat, &GMT->current.proj.o_cos_pole_lat);
	gmt_pole_rotate_forward (GMT, lon1, lat1, &beta, &dummy);
	GMT->current.proj.o_beta = -beta;
	GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Oblique Mercator pole is %.12g %.12g, with beta = %.12g\n", plon, plat, -beta);
}

bool gmt_map_init_oblique (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;
	double o_x, o_y, p_x, p_y, c, az, b_x, b_y, w, e, s, n;

	GMT_set_spherical (GMT, true);	/* PW: Force spherical for now */

	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[4] /= GMT->current.proj.M_PR_DEG;	/* To get plot-units / m */

	o_x = GMT->current.proj.pars[0];	o_y = GMT->current.proj.pars[1];

	if (lrint (GMT->current.proj.pars[6]) == 1) {	/* Must get correct origin, then get second point */
		p_x = GMT->current.proj.pars[2];	p_y = GMT->current.proj.pars[3];

		GMT->current.proj.o_pole_lon = p_x;
		GMT->current.proj.o_pole_lat = p_y;
		GMT->current.proj.o_sin_pole_lat = sind (p_y);
		GMT->current.proj.o_cos_pole_lat = cosd (p_y);

		/* Find azimuth to pole, add 90, and compute second point 10 degrees away */

		gmt_get_origin (GMT, o_x, o_y, p_x, p_y, &o_x, &o_y);
		az = atand (cosd (p_y) * sind (p_x - o_x) / (cosd (o_y) * sind (p_y) - sind (o_y) * cosd (p_y) * cosd (p_x - o_x))) + 90.0;
		c = 10.0;	/* compute point 10 degrees from origin along azimuth */
		b_x = o_x + atand (sind (c) * sind (az) / (cosd (o_y) * cosd (c) - sind (o_y) * sind (c) * cosd (az)));
		b_y = d_asind (sind (o_y) * cosd (c) + cosd (o_y) * sind (c) * cosd (az));
		GMT->current.proj.pars[0] = o_x;	GMT->current.proj.pars[1] = o_y;
		GMT->current.proj.pars[2] = b_x;	GMT->current.proj.pars[3] = b_y;
	}
	else {	/* Just find pole */
		b_x = GMT->current.proj.pars[2];	b_y = GMT->current.proj.pars[3];
		gmt_get_rotate_pole (GMT, o_x, o_y, b_x, b_y);
	}

	/* Here we have pole and origin */

	gmt_set_oblique_pole_and_origin (GMT, GMT->current.proj.o_pole_lon, GMT->current.proj.o_pole_lat, o_x, o_y);
	GMT_vmerc (GMT, 0.0, 0.0);

	if (GMT->common.R.oblique) {	/* wesn is lower left and upper right corners in normal lon/lats */
		GMT_oblmrc (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_oblmrc (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
	}
	else {	/* Gave oblique degrees */
		/* Convert oblique wesn in degrees to meters using regular Mercator */
		if (GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])) {
			GMT->common.R.wesn[XLO] = -180.0;
			GMT->common.R.wesn[XHI] = +180.0;
		}
		GMT_merc_sph (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_merc_sph (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->common.R.oblique = true;	/* Since wesn was oblique, not geographical wesn */
	}	

	GMT_imerc_sph (GMT, &w, &s, xmin, ymin);	/* Get oblique wesn boundaries */
	GMT_imerc_sph (GMT, &e, &n, xmax, ymax);
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[4];
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[4]);
	GMT->current.proj.fwd = &GMT_oblmrc;
	GMT->current.proj.inv = &GMT_ioblmrc;
	GMT->current.map.outside = &gmt_rect_outside;
	GMT->current.map.crossing = &gmt_rect_crossing;
	GMT->current.map.overlap = &gmt_rect_overlap;
	GMT->current.map.clip = &gmt_rect_clip;
	GMT->current.map.left_edge = &gmt_left_rect;
	GMT->current.map.right_edge = &gmt_right_rect;

	GMT->current.map.is_world = GMT_360_RANGE (w, e);
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
	GMT->current.map.frame.check_side = !(GMT->current.setting.map_annot_oblique & 1);
	return (true);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE TRANSVERSE MERCATOR PROJECTION (GMT_TM)
 */

/* For global TM maps */

unsigned int gmt_wrap_around_check_tm (struct GMT_CTRL *GMT, double *angle, double last_x, double last_y, double this_x, double this_y, double *xx, double *yy, unsigned int *sides)
{
	double dx, dy, width, jump;

	jump = this_y - last_y;
	width = 0.5 * GMT->current.map.height;

	if (fabs (jump) - width <= GMT_SMALL || fabs (jump) <= GMT_SMALL) return (0);
	dx = this_x - last_x;

	if (jump < -width) {	/* Crossed top boundary */
		dy = GMT->current.map.height + jump;
		xx[0] = xx[1] = last_x + (GMT->current.map.height - last_y) * dx / dy;
		if (xx[0] < 0.0 || xx[0] > GMT->current.proj.rect[XHI]) return (0);
		yy[0] = GMT->current.map.height;	yy[1] = 0.0;
		sides[0] = 2;
		angle[0] = d_atan2d (dy, dx);
	}
	else {	/* Crossed bottom boundary */
		dy = GMT->current.map.height - jump;
		xx[0] = xx[1] = last_x + last_y * dx / dy;
		if (xx[0] < 0.0 || xx[0] > GMT->current.proj.rect[XHI]) return (0);
		yy[0] = 0.0;	yy[1] = GMT->current.map.height;
		sides[0] = 0;
		angle[0] = d_atan2d (dy, -dx);
	}
	angle[1] = angle[0] + 180.0;
	sides[1] = 2 - sides[0];

	return (2);
}

bool gmt_this_point_wraps_tm (struct GMT_CTRL *GMT, double y0, double y1)
{
	/* Returns true if the 2 y-points implies a jump at this x-level of the TM map */

	double dy;

	return ((dy = fabs (y1 - y0)) > GMT->current.map.half_height);
}

bool gmt_will_it_wrap_tm (struct GMT_CTRL *GMT, double *x, double *y, uint64_t n, uint64_t *start)
{	/* Determines if a polygon will wrap at edges for TM global projection */
	uint64_t i;
	bool wrap;

	if (!GMT->current.map.is_world) return (false);

	for (i = 1, wrap = false; !wrap && i < n; i++) {
		wrap = gmt_this_point_wraps_tm (GMT, y[i-1], y[i]);
	}
	*start = i - 1;
	return (wrap);
}

void gmt_get_crossings_tm (struct GMT_CTRL *GMT, double *xc, double *yc, double x0, double y0, double x1, double y1)
{	/* Finds crossings for wrap-arounds for global TM maps */
	double xa, xb, ya, yb, dy, c;

	xa = x0;	xb = x1;
	ya = y0;	yb = y1;
	if (ya > yb) {	/* Make A the minimum y point */
		double_swap (xa, xb);
		double_swap (ya, yb);
	}

	yb -= GMT->current.map.height;

	dy = ya - yb;
	c = (doubleAlmostEqualZero (ya, yb)) ? 0.0 : (xa - xb) / dy;
	xc[0] = xc[1] = xb - yb * c;
	if (y0 > y1) {	/* First cut top */
		yc[0] = GMT->current.map.height;
		yc[1] = 0.0;
	}
	else {
		yc[0] = 0.0;
		yc[1] = GMT->current.map.height;
	}
}

int gmt_map_jump_tm (struct GMT_CTRL *GMT, double x0, double y0, double x1, double y1)
{
	/* true if y-distance between points exceeds 1/2 map height at this x value */
	/* Only used for TM world maps */

	double dy;

	dy = y1 - y0;
	if (dy > GMT->current.map.half_height) return (-1);	/* Cross bottom/south boundary */
	if (dy < (-GMT->current.map.half_height)) return (1);	/* Cross top/north boundary */
	return (0);
}

bool gmt_map_init_tm (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	/* Wrap and truncations are in y, not x for TM */

	GMT->current.map.wrap_around_check = &gmt_wrap_around_check_tm;
	GMT->current.map.jump = &gmt_map_jump_tm;
	GMT->current.map.will_it_wrap = &gmt_will_it_wrap_tm;
	// GMT->current.map.this_point_wraps = &gmt_this_point_wraps_tm;
	GMT->current.map.get_crossings = &gmt_get_crossings_tm;

	if (GMT->current.setting.proj_scale_factor == -1.0) GMT->current.setting.proj_scale_factor = 1.0;	/* Select default map scale for TM */
	GMT->current.proj.GMT_convert_latitudes = gmt_quicktm (GMT, GMT->current.proj.pars[0], 10.0);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);
	GMT_vtm (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[2] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[2];
	if (GMT_IS_SPHERICAL (GMT) || GMT->current.proj.GMT_convert_latitudes) {	/* Spherical code w/wo conformal latitudes */
		GMT->current.proj.fwd = &GMT_tm_sph;
		GMT->current.proj.inv = &GMT_itm_sph;
	}
	else {
		GMT->current.proj.fwd = &GMT_tm;
		GMT->current.proj.inv = &GMT_itm;
	}

	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->current.map.is_world) {	/* Gave oblique degrees */
		double w, e, dummy;
		w = GMT->current.proj.central_meridian + GMT->common.R.wesn[YLO];
		e = GMT->current.proj.central_meridian + GMT->common.R.wesn[YHI];
		GMT->common.R.wesn[YLO] = -90;
		GMT->common.R.wesn[YHI] = 90;
		GMT->common.R.wesn[XHI] = e;
		GMT->common.R.wesn[XLO] = w;
		GMT_vtm (GMT, GMT->current.proj.pars[0], 0.0);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], 0.0, &xmin, &dummy);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], 0.0, &xmax, &dummy);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &dummy, &ymin);
		ymax = ymin + (TWO_PI * GMT->current.proj.EQ_RAD * GMT->current.setting.proj_scale_factor);
		GMT_vtm (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
		GMT->current.map.is_world_tm = true;
		GMT->common.R.oblique = true;	/* Since wesn was oblique, not geographical wesn */
		GMT->common.R.wesn[XHI] = GMT->current.proj.central_meridian + 180.0;
		GMT->common.R.wesn[XLO] = GMT->current.proj.central_meridian - 180.0;
	}
	else if (!GMT->common.R.oblique) {
		gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.is_world_tm = doubleAlmostEqualZero (GMT->common.R.wesn[YHI], GMT->common.R.wesn[YLO]);
		GMT->current.map.is_world = false;
	}
	else { /* Find min values */
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
		GMT->current.map.is_world_tm = false;
		GMT->current.map.is_world = (fabs (GMT->common.R.wesn[YLO] - GMT->common.R.wesn[YHI]) < GMT_SMALL);
	}

	GMT->current.map.frame.horizontal = 1;
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[2]);

	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE UNIVERSAL TRANSVERSE MERCATOR PROJECTION (GMT_UTM)
 */

bool gmt_map_init_utm (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax, lon0;

	if (GMT->current.setting.proj_scale_factor == -1.0) GMT->current.setting.proj_scale_factor = 0.9996;	/* Select default map scale for UTM */
	lon0 = 180.0 + 6.0 * GMT->current.proj.pars[0] - 3.0;	/* Central meridian for this UTM zone */
	if (lon0 >= 360.0) lon0 -= 360.0;
	GMT->current.proj.GMT_convert_latitudes = gmt_quicktm (GMT, lon0, 10.0);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);
	GMT_vtm (GMT, lon0, 0.0);	/* Central meridian for this zone */
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[1];
	switch (GMT->current.proj.utm_hemisphere) {	/* Set hemisphere */
		case -1:
			GMT->current.proj.north_pole = false;
			break;
		case +1:
			GMT->current.proj.north_pole = true;
			break;
		default:
			GMT->current.proj.north_pole = (GMT->common.R.wesn[YLO] >= 0.0);
			break;
	}
	if (GMT_IS_SPHERICAL (GMT) || GMT->current.proj.GMT_convert_latitudes) {	/* Spherical code w/wo conformal latitudes */
		GMT->current.proj.fwd = &GMT_utm_sph;
		GMT->current.proj.inv = &GMT_iutm_sph;
	}
	else {
		GMT->current.proj.fwd = &GMT_utm;
		GMT->current.proj.inv = &GMT_iutm;
	}

	if (fabs (GMT->common.R.wesn[XLO] - GMT->common.R.wesn[XHI]) > 360.0) {	/* -R in UTM meters */
		(*GMT->current.proj.inv) (GMT, &GMT->common.R.wesn[XLO], &GMT->common.R.wesn[YLO], GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO]);
		(*GMT->current.proj.inv) (GMT, &GMT->common.R.wesn[XHI], &GMT->common.R.wesn[YHI], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI]);
		GMT->common.R.oblique = true;
	}
	if (GMT->common.R.oblique) {
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
	}

	GMT->current.map.frame.horizontal = 1;

	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);

	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;

	return (GMT->common.R.oblique);
}

#if 0
double GMT_UTMzone_to_clon (struct GMT_CTRL *GMT, unsigned int zone_x, char zone_y)
{ /* Return the central longitude of this UTM zone */
	double clon = 180.0 + 6.0 * zone_x - 3.0;	/* This is valid for most zones */

	if (zone_y == 0) return (clon);	/* Latitude zone is not specified so we are done */
	if ((zone_y < 'A' || zone_y > 'Z') || zone_y < 'I' || zone_y < 'O') return (GMT->session.d_NaN);	/* Bad latitude zone so return NaN*/
	if (zone_y <= 'B') return (-90.0 + 180.0 * (zone_y - 'A'));	/* Either -90 or +90 */
	if (zone_y == 'V' && zone_x == 31) return (1.5);	/* Center of 31V */
	if (zone_y == 'V' && zone_x == 32) return (7.5);	/* Center of 32V */
	if (zone_y == 'X') {
		if (zone_x == 31) return (4.5);		/* Center of 31X */
		if (zone_x == 33) return (15.0);	/* Center of 33X */
		if (zone_x == 35) return (27.0);	/* Center of 35X */
		if (zone_x == 37) return (37.5);	/* Center of 37X */
		if (zone_x == 32 || zone_x == 34 || zone_x == 36) return (GMT->session.d_NaN);	/* Bad latitude zone so return NaN*/
		return (clon);
	}
	/* Only Y or Z left */
	return (-90.0 + 180.0 * (zone_y - 'Y'));	/* Either -90 or +90 */
}
#endif

/* Setting w/e/s/n for a fully qualified UTM zone */

bool GMT_UTMzone_to_wesn (struct GMT_CTRL *GMT, unsigned int zone_x, char zone_y, int hemi, double wesn[])
{	/* Given the full UTM zone specification, return w/e/s/n */

	bool error = false;

	wesn[XHI] = -180.0 + 6.0 * zone_x;	wesn[XLO] = wesn[XHI] - 6.0;

	if (zone_y == 0) {	/* Latitude zone is not specified */
		if (hemi == -1) {
			wesn[YLO] = -80.0;	wesn[YHI] = 0.0;
		}
		else if (hemi == +1) {
			wesn[YLO] = 0.0;	wesn[YHI] = 84.0;
		}
		else
			error = true;
		return (error);
	}
	else if (zone_y < 'A' || zone_y > 'Z')
		error = true;
	else if (zone_y <= 'B') {
		wesn[YLO] = -90.0;	wesn[YHI] = -80.0;
		wesn[XHI] = 180.0 * (zone_y - 'A');
		wesn[XLO] = wesn[XHI] - 180.0;
	}
	else if (zone_y <= 'I') {	/* I will behave as J */
		wesn[YLO] = -80.0 + 8.0 * (zone_y - 'C');	wesn[YHI] = wesn[YLO] + 8.0;
	}
	else if (zone_y <= 'O') {	/* O will behave as P */
		wesn[YLO] = -80.0 + 8.0 * (zone_y - 'D');	wesn[YHI] = wesn[YLO] + 8.0;
	}
	else if (zone_y <= 'W') {
		wesn[YLO] = -80.0 + 8.0 * (zone_y - 'E');	wesn[YHI] = wesn[YLO] + 8.0;
		if (zone_y == 'V' && zone_x == 31) wesn[XHI] = 3.0;
		if (zone_y == 'V' && zone_x == 32) wesn[XLO] = 3.0;
	}
	else if (zone_y == 'X') {
		wesn[YLO] = 72.0;	wesn[YHI] = 84.0;
		if (zone_x == 31) wesn[XHI] = 9.0;
		if (zone_x == 33) {wesn[XLO] = 9.0; wesn[XHI] = 21.0;}
		if (zone_x == 35) {wesn[XLO] = 21.0; wesn[XHI] = 33.0;}
		if (zone_x == 37) wesn[XLO] = 33.0;
		if (zone_x == 32 || zone_x == 34 || zone_x == 36) error = true;
	}
	else {	/* Y or Z */
		wesn[YLO] = 84.0;	wesn[YHI] = 90.0;
		wesn[XHI] = 180.0 * (zone_y - 'Y');
		wesn[XLO] = wesn[XHI] - 180.0;
	}

	return (error);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE LAMBERT AZIMUTHAL EQUAL-AREA PROJECTION (GMT_LAMB_AZ_EQ)
 */

bool gmt_map_init_lambeq (struct GMT_CTRL *GMT) {
	unsigned int i;
	double xmin, xmax, ymin, ymax, dummy, radius;

	GMT->current.proj.Dx = GMT->current.proj.Dy = 1.0;

	gmt_set_polar (GMT);
	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);
	GMT_vlambeq (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2]);

	if (GMT->current.proj.GMT_convert_latitudes) {
		double D, s, c;
		sincosd (GMT->current.proj.pars[1], &s, &c);
		D = (GMT->current.proj.polar) ? 1.0 : (GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].eq_radius / GMT->current.proj.GMT_lat_swap_vals.ra) * c / (GMT->current.proj.cosp * d_sqrt (1.0 - GMT->current.proj.ECC2 * s * s));
		GMT->current.proj.Dx = D;
		GMT->current.proj.Dy = 1.0 / D;
	}
	GMT->current.proj.iDx = 1.0 / GMT->current.proj.Dx;
	GMT->current.proj.iDy = 1.0 / GMT->current.proj.Dy;

	GMT->current.proj.fwd = &GMT_lambeq;
	GMT->current.proj.inv = &GMT_ilambeq;
	if (GMT->current.proj.units_pr_degree) {
		GMT_vlambeq (GMT, 0.0, 90.0, GMT->current.proj.pars[2]);
		GMT_lambeq (GMT, 0.0, fabs (GMT->current.proj.pars[4]), &dummy, &radius);
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = fabs (GMT->current.proj.pars[3] / radius);
		GMT_vlambeq (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2]);
	}
	else
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[3];

	if (GMT->common.R.oblique) {	/* Rectangular box given */
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);

		GMT->current.map.outside = &gmt_rect_outside2;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = !(GMT->current.setting.map_annot_oblique & 1);
		GMT->current.map.frame.horizontal = (fabs (GMT->current.proj.pars[1]) < 30.0 && fabs (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]) < 30.0) ? 1 : 0;
	}
	else {
		if (GMT->current.proj.polar) {	/* Polar aspect */
			if (GMT->current.proj.north_pole) {
				if (GMT->common.R.wesn[YLO] <= -90.0){
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: South boundary cannot be -90.0 for north polar Lambert azimuthal projection\n");
					GMT_exit (GMT, EXIT_FAILURE); return false;
				}
				if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;
			}
			else {
				if (GMT->common.R.wesn[YHI] >= 90.0) {
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: North boundary cannot be +90.0 for south polar Lambert azimuthal projection\n");
					GMT_exit (GMT, EXIT_FAILURE); return false;
				}
				if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
			}
			if (GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])
					|| doubleAlmostEqualZero (GMT->common.R.wesn[XHI], GMT->common.R.wesn[XLO]))
				GMT->current.proj.edge[1] = GMT->current.proj.edge[3] = false;
			GMT->current.map.outside = &gmt_polar_outside;
			GMT->current.map.crossing = &gmt_wesn_crossing;
			GMT->current.map.overlap = &gmt_wesn_overlap;
			GMT->current.map.clip = &GMT_wesn_clip;
			GMT->current.map.frame.horizontal = 1;
			GMT->current.map.n_lat_nodes = 2;
			gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		}
		else {	/* Global view only */
			/* No annotations or tickmarks in global mode */
			for (i = 0; i < GMT_GRID_UPPER; i++)
				GMT->current.map.frame.axis[GMT_X].item[i].active = GMT->current.map.frame.axis[GMT_Y].item[i].active = false,
				GMT->current.map.frame.axis[GMT_X].item[i].interval = GMT->current.map.frame.axis[GMT_Y].item[i].interval = 0.0;
			GMT->common.R.wesn[XLO] = 0.0;
			GMT->common.R.wesn[XHI] = 360.0;
			GMT->common.R.wesn[YLO] = -90.0;
			GMT->common.R.wesn[YHI] = 90.0;
			xmax = ymax = GMT->current.proj.rho_max;
			xmin = ymin = -xmax;
			GMT->current.map.outside = &gmt_radial_outside;
			GMT->current.map.crossing = &gmt_radial_crossing;
			GMT->current.map.overlap = &gmt_radial_overlap;
			GMT->current.map.clip = &gmt_radial_clip;
			if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
		}
		GMT->current.map.left_edge = &gmt_left_circle;
		GMT->current.map.right_edge = &gmt_right_circle;
	}

	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[3]);
	GMT->current.proj.r = 0.5 * GMT->current.proj.rect[XHI];
	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);
	if (GMT->current.proj.polar) GMT->current.map.meridian_straight = 1;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE ORTHOGRAPHIC PROJECTION (GMT_ORTHO)
 */

bool gmt_map_init_ortho (struct GMT_CTRL *GMT) {
	unsigned int i;
	double xmin, xmax, ymin, ymax, dummy, radius;

	GMT_set_spherical (GMT, true);	/* PW: Force spherical for now */

	gmt_set_polar (GMT);

	if (GMT->current.proj.units_pr_degree) {
		GMT_vortho (GMT, 0.0, 90.0, GMT->current.proj.pars[2]);
		GMT_ortho (GMT, 0.0, fabs (GMT->current.proj.pars[4]), &dummy, &radius);
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = fabs (GMT->current.proj.pars[3] / radius);
	}
	else
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[3];

	GMT_vortho (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2]);
	GMT->current.proj.fwd = &GMT_ortho;
	GMT->current.proj.inv = &GMT_iortho;

	if (GMT->common.R.oblique) {	/* Rectangular box given */
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);

		GMT->current.map.outside = &gmt_rect_outside2;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = !(GMT->current.setting.map_annot_oblique & 1);
		GMT->current.map.frame.horizontal = (fabs (GMT->current.proj.pars[1]) < 30.0 && fabs (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]) < 30.0) ? 1 : 0;
	}
	else {
		if (GMT->current.proj.polar) {	/* Polar aspect */
			if (GMT->current.proj.north_pole) {
				if (GMT->common.R.wesn[YLO] < 0.0) {
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Warning: South boundary cannot be < 0 for north polar orthographic projection (reset to 0)\n");
					GMT->common.R.wesn[YLO] = 0.0;
				}
				if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;
			}
			else {
				if (GMT->common.R.wesn[YHI] > 0.0) {
					GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Warning: North boundary cannot be > 0 for south polar orthographic projection (reset to 0)\n");
					GMT->common.R.wesn[YHI] = 0.0;
				}
				if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
			}
			if (GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])
					|| doubleAlmostEqualZero (GMT->common.R.wesn[XHI], GMT->common.R.wesn[XLO]))
				GMT->current.proj.edge[1] = GMT->current.proj.edge[3] = false;
			GMT->current.map.outside = &gmt_polar_outside;
			GMT->current.map.crossing = &gmt_wesn_crossing;
			GMT->current.map.overlap = &gmt_wesn_overlap;
			GMT->current.map.clip = &GMT_wesn_clip;
			GMT->current.map.frame.horizontal = 1;
			GMT->current.map.n_lat_nodes = 2;
			gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		}
		else {	/* Global view only */
			/* No annotations or tickmarks in global mode */
			for (i = 0; i < GMT_GRID_UPPER; i++)
				GMT->current.map.frame.axis[GMT_X].item[i].active = GMT->current.map.frame.axis[GMT_Y].item[i].active = false,
				GMT->current.map.frame.axis[GMT_X].item[i].interval = GMT->current.map.frame.axis[GMT_Y].item[i].interval = 0.0;
			GMT->common.R.wesn[XLO] = 0.0;
			GMT->common.R.wesn[XHI] = 360.0;
			GMT->common.R.wesn[YLO] = -90.0;
			GMT->common.R.wesn[YHI] = 90.0;
			xmax = ymax = GMT->current.proj.rho_max * GMT->current.proj.EQ_RAD;
			xmin = ymin = -xmax;
			GMT->current.map.outside = &gmt_radial_outside;
			GMT->current.map.crossing = &gmt_radial_crossing;
			GMT->current.map.overlap = &gmt_radial_overlap;
			GMT->current.map.clip = &gmt_radial_clip;
			if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
		}
		GMT->current.map.left_edge = &gmt_left_circle;
		GMT->current.map.right_edge = &gmt_right_circle;
	}

	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[3]);
	GMT->current.proj.r = 0.5 * GMT->current.proj.rect[XHI];
	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);
	if (GMT->current.proj.polar) GMT->current.map.meridian_straight = 1;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE GENERAL PERSPECTIVE PROJECTION (GMT_GENPER)
 */

bool gmt_map_init_genper (struct GMT_CTRL *GMT) {
	bool search;
	unsigned int i;
	double xmin, xmax, ymin, ymax, dummy, radius = 0.0;
	double alt, azimuth, tilt, width, height;
	double twist, scale, units;

	units = GMT->current.proj.pars[2];
	scale = GMT->current.proj.pars[3];
	alt = GMT->current.proj.pars[4];
	azimuth = GMT->current.proj.pars[5];
	tilt = GMT->current.proj.pars[6];
	twist = GMT->current.proj.pars[7];
	width = GMT->current.proj.pars[8];
	height = GMT->current.proj.pars[9];

	if (GMT->current.proj.g_sphere) GMT_set_spherical (GMT, true); /* PW: Force spherical for now */

	gmt_set_polar (GMT);

	if (GMT->current.proj.units_pr_degree) {
		GMT_vgenper (GMT, 0.0, 90.0, alt, azimuth, tilt, twist, width, height);
		GMT_genper (GMT, 0.0, fabs (GMT->current.proj.pars[3]), &dummy, &radius);
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = fabs (GMT->current.proj.pars[2] / radius);
	}
	else
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[2];

	if (GMT->current.proj.g_debug > 1) {
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "genper: units_pr_degree %d\n", GMT->current.proj.units_pr_degree);
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "genper: radius %f\n", radius);
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "genper: scale %f units %f\n", scale, units);
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "genper: x scale %f y scale %f\n", GMT->current.proj.scale[GMT_X], GMT->current.proj.scale[GMT_Y]);
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "genper: gave_map_width %d \n",GMT->current.proj.gave_map_width);
	}

	GMT_vgenper (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], alt, azimuth, tilt, twist, width, height);
	GMT->current.proj.fwd = &GMT_genper;
	GMT->current.proj.inv = &GMT_igenper;

	GMT->common.R.wesn[XLO] = 0.0;
	GMT->common.R.wesn[XHI] = 360.0;
	GMT->common.R.wesn[YLO] = -90.0;
	GMT->common.R.wesn[YHI] = 90.0;

	xmin = ymin = -GMT->current.proj.g_rmax;
	xmax = ymax = -xmin;

	xmin = GMT->current.proj.g_xmin;
	xmax = GMT->current.proj.g_xmax;
	ymin = GMT->current.proj.g_ymin;
	ymax = GMT->current.proj.g_ymax;

	if (GMT->current.proj.g_width != 0.0) {
		GMT->common.R.oblique = false;
		if (GMT->current.proj.g_debug > 0) GMT_Report (GMT->parent, GMT_MSG_DEBUG, "using windowed region\n");
		GMT->current.map.outside = &gmt_rect_outside2;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip_old;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = !(GMT->current.setting.map_annot_oblique & 1);
		GMT->current.map.frame.horizontal = (fabs (GMT->current.proj.pars[1]) < 30.0 && fabs (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]) < 30.0) ? 1 : 0;
		search = true;
	}
	else {
		if (GMT->current.proj.g_debug > 0) GMT_Report (GMT->parent, GMT_MSG_DEBUG, "using global view\n");
		/* No annotations or tickmarks in global mode */
		for (i = 0; i < GMT_GRID_UPPER; i++)
			GMT->current.map.frame.axis[GMT_X].item[i].active = GMT->current.map.frame.axis[GMT_Y].item[i].active = false,
			GMT->current.map.frame.axis[GMT_X].item[i].interval = GMT->current.map.frame.axis[GMT_Y].item[i].interval = 0.0;
		GMT->current.map.overlap = &gmt_genper_overlap;
		GMT->current.map.crossing = &gmt_radial_crossing;
		GMT->current.map.clip = &gmt_radial_clip;
		GMT->current.map.outside = &gmt_radial_outside;
		GMT->current.map.left_edge = &gmt_left_circle;
		GMT->current.map.right_edge = &gmt_right_circle;

		if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;

		search = false;
  	}

	if (GMT->current.proj.polar) {
		if (GMT->current.proj.north_pole) {
			if (GMT->common.R.wesn[YLO] < (90.0 - GMT->current.proj.f_horizon)) GMT->common.R.wesn[YLO] = 90.0 - GMT->current.proj.f_horizon;
			if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;
		} else {
			if (GMT->common.R.wesn[YHI] > -(90.0 - GMT->current.proj.f_horizon)) GMT->common.R.wesn[YHI] = -(90.0 - GMT->current.proj.f_horizon);
			if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
		}
		if (GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])
				|| doubleAlmostEqualZero (GMT->common.R.wesn[XHI], GMT->common.R.wesn[XLO]))
			GMT->current.proj.edge[1] = GMT->current.proj.edge[3] = false;
	}

	if (GMT->current.proj.g_debug > 0) GMT_Report (GMT->parent, GMT_MSG_DEBUG, "xmin %f xmax %f ymin %f ymax %f\n", xmin/1000, xmax/1000, ymin/1000, ymax/1000);

	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[2]);

	GMT->current.proj.r = 0.5 * GMT->current.proj.rect[XHI];

	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);

	if (GMT->current.proj.g_debug > 0) {
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "x scale %e y scale %e\n", GMT->current.proj.scale[GMT_X], GMT->current.proj.scale[GMT_Y]);
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "x center %f y center %f\n", GMT->current.proj.c_x0, GMT->current.proj.c_y0);
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "x max %f y max %f\n", GMT->current.proj.rect[XHI], GMT->current.proj.rect[YHI]);
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "x0 %f y0 %f\n\n", GMT->current.proj.origin[GMT_X], GMT->current.proj.origin[GMT_Y]);
		fflush(NULL);
	}

	return (search);

}

/*
 *	TRANSFORMATION ROUTINES FOR THE GNOMONIC PROJECTION (GMT_GNOMONIC)
 */

bool gmt_map_init_gnomonic (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax, dummy, radius;

	GMT_set_spherical (GMT, true);	/* PW: Force spherical for now */	gmt_set_polar (GMT);

	if (GMT->current.proj.units_pr_degree) {
		GMT_vgnomonic (GMT, 0.0, 90.0, 60.0);
		GMT_gnomonic (GMT, 0.0, fabs (GMT->current.proj.pars[4]), &dummy, &radius);
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = fabs (GMT->current.proj.pars[3] / radius);
	}
	else
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[3];

	GMT_vgnomonic (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2]);
	GMT->current.proj.fwd = &GMT_gnomonic;
	GMT->current.proj.inv = &GMT_ignomonic;

	if (GMT->common.R.oblique) {	/* Rectangular box given */
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);

		GMT->current.map.outside = &gmt_rect_outside2;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = !(GMT->current.setting.map_annot_oblique & 1);
		GMT->current.map.frame.horizontal = (fabs (GMT->current.proj.pars[1]) < 30.0 && fabs (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]) < 30.0) ? 1 : 0;
	}
	else {
		if (GMT->current.proj.polar) {	/* Polar aspect */
			if (GMT->current.proj.north_pole) {
				if (GMT->common.R.wesn[YLO] < (90.0 - GMT->current.proj.f_horizon)) GMT->common.R.wesn[YLO] = 90.0 - GMT->current.proj.f_horizon;
				if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;
			}
			else {
				if (GMT->common.R.wesn[YHI] > -(90.0 - GMT->current.proj.f_horizon)) GMT->common.R.wesn[YHI] = -(90.0 - GMT->current.proj.f_horizon);
				if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
			}
			if (GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])
					|| doubleAlmostEqualZero (GMT->common.R.wesn[XHI], GMT->common.R.wesn[XLO]))
				GMT->current.proj.edge[1] = GMT->current.proj.edge[3] = false;
			GMT->current.map.outside = &gmt_polar_outside;
			GMT->current.map.crossing = &gmt_wesn_crossing;
			GMT->current.map.overlap = &gmt_wesn_overlap;
			GMT->current.map.clip = &GMT_wesn_clip;
			GMT->current.map.frame.horizontal = 1;
			GMT->current.map.n_lat_nodes = 2;
			gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		}
		else {	/* Global view only */
			GMT->common.R.wesn[XLO] = 0.0;
			GMT->common.R.wesn[XHI] = 360.0;
			GMT->common.R.wesn[YLO] = -90.0;
			GMT->common.R.wesn[YHI] = 90.0;
			xmax = ymax = GMT->current.proj.rho_max * GMT->current.proj.EQ_RAD;
			xmin = ymin = -xmax;
			GMT->current.map.outside = &gmt_radial_outside;
			GMT->current.map.crossing = &gmt_radial_crossing;
			GMT->current.map.overlap = &gmt_radial_overlap;
			GMT->current.map.clip = &gmt_radial_clip;
			if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
		}
		GMT->current.map.left_edge = &gmt_left_circle;
		GMT->current.map.right_edge = &gmt_right_circle;
	}

	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[3]);
	GMT->current.proj.r = 0.5 * GMT->current.proj.rect[XHI];
	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE AZIMUTHAL EQUIDISTANT PROJECTION (GMT_AZ_EQDIST)
 */

bool gmt_map_init_azeqdist (struct GMT_CTRL *GMT) {
	unsigned int i;
	double xmin, xmax, ymin, ymax, dummy, radius;

	GMT_set_spherical (GMT, true);	/* PW: Force spherical for now */

	gmt_set_polar (GMT);

	if (GMT->current.proj.units_pr_degree) {
		GMT_vazeqdist (GMT, 0.0, 90.0, GMT->current.proj.pars[2]);
		GMT_azeqdist (GMT, 0.0, fabs (GMT->current.proj.pars[4]), &dummy, &radius);
		if (GMT_IS_ZERO (radius)) radius = GMT->current.proj.rho_max * GMT->current.proj.EQ_RAD;
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = fabs (GMT->current.proj.pars[3] / radius);
	}
	else
		GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[3];

	GMT_vazeqdist (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2]);
	GMT->current.proj.fwd = &GMT_azeqdist;
	GMT->current.proj.inv = &GMT_iazeqdist;

	if (GMT->common.R.oblique) {	/* Rectangular box given */
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);

		GMT->current.map.outside = &gmt_rect_outside2;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = !(GMT->current.setting.map_annot_oblique & 1);
		GMT->current.map.frame.horizontal = (fabs (GMT->current.proj.pars[1]) < 60.0 && fabs (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]) < 30.0) ? 1 : 0;
	}
	else {
		if (GMT->current.proj.polar && (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]) < 180.0) {	/* Polar aspect */
			if (!GMT->current.proj.north_pole && GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
			if (GMT->current.proj.north_pole && GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;
			if (GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])
					|| doubleAlmostEqualZero (GMT->common.R.wesn[XHI], GMT->common.R.wesn[XLO]))
				GMT->current.proj.edge[1] = GMT->current.proj.edge[3] = false;
			GMT->current.map.outside = &gmt_polar_outside;
			GMT->current.map.crossing = &gmt_wesn_crossing;
			GMT->current.map.overlap = &gmt_wesn_overlap;
			GMT->current.map.clip = &GMT_wesn_clip;
			GMT->current.map.frame.horizontal = 1;
			GMT->current.map.n_lat_nodes = 2;
			gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		}
		else {	/* Global view only, force wesn = 0/360/-90/90  */
			/* No annotations or tickmarks in global mode */
			for (i = 0; i < GMT_GRID_UPPER; i++)
				GMT->current.map.frame.axis[GMT_X].item[i].active = GMT->current.map.frame.axis[GMT_Y].item[i].active = false,
				GMT->current.map.frame.axis[GMT_X].item[i].interval = GMT->current.map.frame.axis[GMT_Y].item[i].interval = 0.0;
			GMT->common.R.wesn[XLO] = 0.0;
			GMT->common.R.wesn[XHI] = 360.0;
			GMT->common.R.wesn[YLO] = -90.0;
			GMT->common.R.wesn[YHI] = 90.0;
			xmax = ymax = GMT->current.proj.rho_max * GMT->current.proj.EQ_RAD;
			xmin = ymin = -xmax;
			GMT->current.map.outside = &gmt_radial_outside;
			GMT->current.map.crossing = &gmt_radial_crossing;
			GMT->current.map.overlap = &gmt_radial_overlap;
			GMT->current.map.clip = &gmt_radial_clip;
			if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
		}
		GMT->current.map.left_edge = &gmt_left_circle;
		GMT->current.map.right_edge = &gmt_right_circle;
	}

	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[3]);
	GMT->current.proj.r = 0.5 * GMT->current.proj.rect[XHI];
	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);
	if (GMT->current.proj.polar) GMT->current.map.meridian_straight = 1;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE MOLLWEIDE EQUAL AREA PROJECTION (GMT_MOLLWEIDE)
 */

bool gmt_map_init_mollweide (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax, y, dummy;

	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.pars[0] < 0.0) GMT->current.proj.pars[0] += 360.0;
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = M_PI * GMT->current.proj.pars[1] / sqrt (8.0);
	GMT_vmollweide (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
	if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;

	if (GMT->common.R.oblique) {
		GMT_mollweide (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_mollweide (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		y = (GMT->common.R.wesn[YLO] * GMT->common.R.wesn[YHI] <= 0.0) ? 0.0 : MIN (fabs (GMT->common.R.wesn[YLO]), fabs (GMT->common.R.wesn[YHI]));
		GMT_mollweide (GMT, GMT->common.R.wesn[XLO], y, &xmin, &dummy);
		GMT_mollweide (GMT, GMT->common.R.wesn[XHI], y, &xmax, &dummy);
		GMT_mollweide (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YLO], &dummy, &ymin);
		GMT_mollweide (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YHI], &dummy, &ymax);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &gmt_left_ellipse;
		GMT->current.map.right_edge = &gmt_right_ellipse;
		GMT->current.map.frame.horizontal = 2;
		GMT->current.proj.polar = true;
	}
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);
	GMT->current.proj.fwd = &GMT_mollweide;
	GMT->current.proj.inv = &GMT_imollweide;
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
	GMT->current.map.parallel_straight = 1;

	return (GMT->common.R.oblique);
}


/*
 *	TRANSFORMATION ROUTINES FOR THE HAMMER-AITOFF EQUAL AREA PROJECTION (GMT_HAMMER)
 */

bool gmt_map_init_hammer (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.pars[0] < 0.0) GMT->current.proj.pars[0] += 360.0;
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = 0.5 * M_PI * GMT->current.proj.pars[1] / M_SQRT2;
	GMT_vhammer (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
	if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;

	if (GMT->common.R.oblique) {
		GMT_hammer (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_hammer (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		double x, y, dummy;
		y = (GMT->common.R.wesn[YLO] * GMT->common.R.wesn[YHI] <= 0.0) ? 0.0 : MIN (fabs (GMT->common.R.wesn[YLO]), fabs (GMT->common.R.wesn[YHI]));
		x = (fabs (GMT->common.R.wesn[XLO] - GMT->current.proj.central_meridian) > fabs (GMT->common.R.wesn[XHI] - GMT->current.proj.central_meridian)) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI];
		GMT_hammer (GMT, GMT->common.R.wesn[XLO], y, &xmin, &dummy);
		GMT_hammer (GMT, GMT->common.R.wesn[XHI], y, &xmax, &dummy);
		GMT_hammer (GMT, x, GMT->common.R.wesn[YLO], &dummy, &ymin);
		GMT_hammer (GMT, x, GMT->common.R.wesn[YHI], &dummy, &ymax);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &gmt_left_ellipse;
		GMT->current.map.right_edge = &gmt_right_ellipse;
		GMT->current.map.frame.horizontal = 2;
		GMT->current.proj.polar = true;
	}
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);
	GMT->current.proj.fwd = &GMT_hammer;
	GMT->current.proj.inv = &GMT_ihammer;
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE VAN DER GRINTEN PROJECTION (GMT_VANGRINTEN)
 */

bool gmt_map_init_grinten (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT_set_spherical (GMT, true);

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.pars[0] < 0.0) GMT->current.proj.pars[0] += 360.0;
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[1];
	GMT_vgrinten (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
	if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;

	if (GMT->common.R.oblique) {
		GMT_grinten (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_grinten (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		double x, y, dummy;
		y = (GMT->common.R.wesn[YLO] * GMT->common.R.wesn[YHI] <= 0.0) ? 0.0 : MIN (fabs (GMT->common.R.wesn[YLO]), fabs (GMT->common.R.wesn[YHI]));
		x = (fabs (GMT->common.R.wesn[XLO] - GMT->current.proj.central_meridian) > fabs (GMT->common.R.wesn[XHI] - GMT->current.proj.central_meridian)) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI];
		GMT_grinten (GMT, GMT->common.R.wesn[XLO], y, &xmin, &dummy);
		GMT_grinten (GMT, GMT->common.R.wesn[XHI], y, &xmax, &dummy);
		GMT_grinten (GMT, x, GMT->common.R.wesn[YLO], &dummy, &ymin);
		GMT_grinten (GMT, x, GMT->common.R.wesn[YHI], &dummy, &ymax);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &gmt_left_circle;
		GMT->current.map.right_edge = &gmt_right_circle;
		GMT->current.map.frame.horizontal = 2;
		GMT->current.proj.polar = true;
	}
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);
	GMT->current.proj.r = 0.5 * GMT->current.proj.rect[XHI];
	GMT->current.proj.fwd = &GMT_grinten;
	GMT->current.proj.inv = &GMT_igrinten;
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE WINKEL-TRIPEL MODIFIED AZIMUTHAL PROJECTION (GMT_WINKEL)
 */

bool gmt_map_init_winkel (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT_set_spherical (GMT, true);	/* PW: Force spherical for now */

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.pars[0] < 0.0) GMT->current.proj.pars[0] += 360.0;
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT_vwinkel (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = 2.0 * GMT->current.proj.pars[1] / (1.0 + GMT->current.proj.r_cosphi1);

	if (GMT->common.R.oblique) {
		GMT_winkel (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_winkel (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		double x, y, dummy;
		y = (GMT->common.R.wesn[YLO] * GMT->common.R.wesn[YHI] <= 0.0) ? 0.0 : MIN (fabs (GMT->common.R.wesn[YLO]), fabs (GMT->common.R.wesn[YHI]));
		x = (fabs (GMT->common.R.wesn[XLO] - GMT->current.proj.central_meridian) > fabs (GMT->common.R.wesn[XHI] - GMT->current.proj.central_meridian)) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI];
		GMT_winkel (GMT, GMT->common.R.wesn[XLO], y, &xmin, &dummy);
		GMT_winkel (GMT, GMT->common.R.wesn[XHI], y, &xmax, &dummy);
		GMT_winkel (GMT, x, GMT->common.R.wesn[YLO], &dummy, &ymin);
		GMT_winkel (GMT, x, GMT->common.R.wesn[YHI], &dummy, &ymax);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &GMT_left_winkel;
		GMT->current.map.right_edge = &GMT_right_winkel;
		GMT->current.map.frame.horizontal = 2;
	}
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);
	GMT->current.proj.fwd = &GMT_winkel;
	GMT->current.proj.inv = &GMT_iwinkel;
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE ECKERT IV PROJECTION (GMT_ECKERT4)
 */

bool gmt_map_init_eckert4 (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.pars[0] < 0.0) GMT->current.proj.pars[0] += 360.0;
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT_veckert4 (GMT, GMT->current.proj.pars[0]);
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[1];

	if (GMT->common.R.oblique) {
		GMT_eckert4 (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_eckert4 (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		double y, dummy;
		y = (GMT->common.R.wesn[YLO] * GMT->common.R.wesn[YHI] <= 0.0) ? 0.0 : MIN (fabs (GMT->common.R.wesn[YLO]), fabs (GMT->common.R.wesn[YHI]));
		GMT_eckert4 (GMT, GMT->common.R.wesn[XLO], y, &xmin, &dummy);
		GMT_eckert4 (GMT, GMT->common.R.wesn[XHI], y, &xmax, &dummy);
		GMT_eckert4 (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YLO], &dummy, &ymin);
		GMT_eckert4 (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YHI], &dummy, &ymax);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &GMT_left_eckert4;
		GMT->current.map.right_edge = &GMT_right_eckert4;
		GMT->current.map.frame.horizontal = 2;
	}
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);
	GMT->current.proj.fwd = &GMT_eckert4;
	GMT->current.proj.inv = &GMT_ieckert4;
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
	GMT->current.map.parallel_straight = 1;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE ECKERT VI PROJECTION (GMT_ECKERT6)
 */

bool gmt_map_init_eckert6 (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.pars[0] < 0.0) GMT->current.proj.pars[0] += 360.0;
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT_veckert6 (GMT, GMT->current.proj.pars[0]);
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = 0.5 * GMT->current.proj.pars[1] * sqrt (2.0 + M_PI);

	if (GMT->common.R.oblique) {
		GMT_eckert6 (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_eckert6 (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		double y, dummy;
		y = (GMT->common.R.wesn[YLO] * GMT->common.R.wesn[YHI] <= 0.0) ? 0.0 : MIN (fabs (GMT->common.R.wesn[YLO]), fabs (GMT->common.R.wesn[YHI]));
		GMT_eckert6 (GMT, GMT->common.R.wesn[XLO], y, &xmin, &dummy);
		GMT_eckert6 (GMT, GMT->common.R.wesn[XHI], y, &xmax, &dummy);
		GMT_eckert6 (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YLO], &dummy, &ymin);
		GMT_eckert6 (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YHI], &dummy, &ymax);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &GMT_left_eckert6;
		GMT->current.map.right_edge = &GMT_right_eckert6;
		GMT->current.map.frame.horizontal = 2;
	}
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);
	GMT->current.proj.fwd = &GMT_eckert6;
	GMT->current.proj.inv = &GMT_ieckert6;
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
	GMT->current.map.parallel_straight = 1;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE ROBINSON PSEUDOCYLINDRICAL PROJECTION (GMT_ROBINSON)
 */

bool gmt_map_init_robinson (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT_set_spherical (GMT, true);	/* PW: Force spherical for now */

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.pars[0] < 0.0) GMT->current.proj.pars[0] += 360.0;
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT_vrobinson (GMT, GMT->current.proj.pars[0]);
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[1] / 0.8487;

	if (GMT->common.R.oblique) {
		GMT_robinson (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_robinson (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		double y, dummy;
		y = (GMT->common.R.wesn[YLO] * GMT->common.R.wesn[YHI] <= 0.0) ? 0.0 : MIN (fabs (GMT->common.R.wesn[YLO]), fabs (GMT->common.R.wesn[YHI]));
		GMT_robinson (GMT, GMT->common.R.wesn[XLO], y, &xmin, &dummy);
		GMT_robinson (GMT, GMT->common.R.wesn[XHI], y, &xmax, &dummy);
		GMT_robinson (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YLO], &dummy, &ymin);
		GMT_robinson (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YHI], &dummy, &ymax);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &GMT_left_robinson;
		GMT->current.map.right_edge = &GMT_right_robinson;
		GMT->current.map.frame.horizontal = 2;
	}
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);
	GMT->current.proj.fwd = &GMT_robinson;
	GMT->current.proj.inv = &GMT_irobinson;
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;
	GMT->current.map.parallel_straight = 1;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE SINUSOIDAL EQUAL AREA PROJECTION (GMT_SINUSOIDAL)
 */

bool gmt_map_init_sinusoidal (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	if (GMT->current.proj.pars[0] < 0.0) GMT->current.proj.pars[0] += 360.0;
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
	if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;
	GMT_vsinusoidal (GMT, GMT->current.proj.pars[0]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[1] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[1];
	GMT->current.proj.fwd = &GMT_sinusoidal;
	GMT->current.proj.inv = &GMT_isinusoidal;
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;

	if (GMT->common.R.oblique) {
		GMT_sinusoidal (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		GMT_sinusoidal (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		double dummy, y;
		y = (GMT->common.R.wesn[YLO] * GMT->common.R.wesn[YHI] <= 0.0) ? 0.0 : MIN (fabs (GMT->common.R.wesn[YLO]), fabs (GMT->common.R.wesn[YHI]));
		GMT_sinusoidal (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YLO], &dummy, &ymin);
		GMT_sinusoidal (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YHI], &dummy, &ymax);
		GMT_sinusoidal (GMT, GMT->common.R.wesn[XLO], y, &xmin, &dummy);
		GMT_sinusoidal (GMT, GMT->common.R.wesn[XHI], y, &xmax, &dummy);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &GMT_left_sinusoidal;
		GMT->current.map.right_edge = &GMT_right_sinusoidal;
		GMT->current.map.frame.horizontal = 2;
		GMT->current.proj.polar = true;
	}

	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[1]);
	GMT->current.map.parallel_straight = 1;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE CASSINI PROJECTION (GMT_CASSINI)
 */

bool gmt_map_init_cassini (struct GMT_CTRL *GMT) {
	bool too_big;
	double xmin, xmax, ymin, ymax;

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	if ((GMT->current.proj.pars[0] - GMT->common.R.wesn[XLO]) > 90.0 || (GMT->common.R.wesn[XHI] - GMT->current.proj.pars[0]) > 90.0) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "ERROR: Max longitude extension away from central meridian is limited to +/- 90 degrees\n");
		GMT_exit (GMT, EXIT_FAILURE); return false;
	}
	too_big = gmt_quicktm (GMT, GMT->current.proj.pars[0], 4.0);
	if (too_big) GMT_set_spherical (GMT, true);	/* Cannot use ellipsoidal series for this area */
	GMT_vcassini (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	if (GMT_IS_SPHERICAL (GMT)) {
		GMT->current.proj.fwd = &GMT_cassini_sph;
		GMT->current.proj.inv = &GMT_icassini_sph;
	}
	else {
		GMT->current.proj.fwd = &GMT_cassini;
		GMT->current.proj.inv = &GMT_icassini;
	}
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[2] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[2];
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;

	if (GMT->common.R.oblique) {
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &gmt_left_conic;
		GMT->current.map.right_edge = &gmt_right_conic;
	}

	GMT->current.map.frame.horizontal = 1;
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[2]);

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE ALBERS PROJECTION (GMT_ALBERS)
 */

bool gmt_map_init_albers (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax, dy, az, x1, y1;

	GMT->current.proj.GMT_convert_latitudes = gmt_quickconic (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);
	if (GMT_IS_SPHERICAL (GMT) || GMT->current.proj.GMT_convert_latitudes) {	/* Spherical code w/wo authalic latitudes */
		GMT_valbers_sph (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2], GMT->current.proj.pars[3]);
		GMT->current.proj.fwd = &GMT_albers_sph;
		GMT->current.proj.inv = &GMT_ialbers_sph;
	}
	else {
		GMT_valbers (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2], GMT->current.proj.pars[3]);
		GMT->current.proj.fwd = &GMT_albers;
		GMT->current.proj.inv = &GMT_ialbers;
	}
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[4] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[4];

	if (GMT->common.R.oblique) {
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &gmt_left_conic;
		GMT->current.map.right_edge = &gmt_right_conic;
	}
	GMT->current.map.frame.horizontal = 1;
	GMT->current.map.n_lat_nodes = 2;
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[4]);

	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);
	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian + 90., GMT->current.proj.pole, &x1, &y1);
	dy = y1 - GMT->current.proj.c_y0;
	az = 2.0 * d_atan2 (dy, x1 - GMT->current.proj.c_x0);
	dy /= (1.0 - cos (az));
	GMT->current.proj.c_y0 += dy;
	GMT->current.map.meridian_straight = 1;

	return (GMT->common.R.oblique);
}


/*
 *	TRANSFORMATION ROUTINES FOR THE EQUIDISTANT CONIC PROJECTION (GMT_ECONIC)
 */


bool gmt_map_init_econic (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax, dy, az, x1, y1;

	GMT->current.proj.GMT_convert_latitudes = !GMT_IS_SPHERICAL (GMT);
	if (GMT->current.proj.GMT_convert_latitudes) GMT_scale_eqrad (GMT);
	GMT_veconic (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1], GMT->current.proj.pars[2], GMT->current.proj.pars[3]);
	GMT->current.proj.fwd = &GMT_econic;
	GMT->current.proj.inv = &GMT_ieconic;
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[4] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[4];

	if (GMT->common.R.oblique) {
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
	}
	else {
		gmt_xy_search (GMT, &xmin, &xmax, &ymin, &ymax, GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO], GMT->common.R.wesn[YHI]);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &gmt_left_conic;
		GMT->current.map.right_edge = &gmt_right_conic;
	}
	GMT->current.map.frame.horizontal = 1;
	GMT->current.map.n_lat_nodes = 2;
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[4]);

	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, &GMT->current.proj.c_x0, &GMT->current.proj.c_y0);
	GMT_geo_to_xy (GMT, GMT->current.proj.central_meridian + 90., GMT->current.proj.pole, &x1, &y1);
	dy = y1 - GMT->current.proj.c_y0;
	az = 2.0 * d_atan2 (dy, x1 - GMT->current.proj.c_x0);
	dy /= (1.0 - cos (az));
	GMT->current.proj.c_y0 += dy;
	GMT->current.map.meridian_straight = 1;

	return (GMT->common.R.oblique);
}

/*
 *	TRANSFORMATION ROUTINES FOR THE POLYCONIC PROJECTION (GMT_POLYCONIC)
 */

bool gmt_map_init_polyconic (struct GMT_CTRL *GMT) {
	double xmin, xmax, ymin, ymax;

	GMT_set_spherical (GMT, true);	/* PW: Force spherical for now */

	if (GMT_is_dnan (GMT->current.proj.pars[0])) GMT->current.proj.pars[0] = 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
	GMT->current.map.is_world = GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]);
	if (GMT->common.R.wesn[YLO] <= -90.0) GMT->current.proj.edge[0] = false;
	if (GMT->common.R.wesn[YHI] >= 90.0) GMT->current.proj.edge[2] = false;
	GMT_vpolyconic (GMT, GMT->current.proj.pars[0], GMT->current.proj.pars[1]);
	if (GMT->current.proj.units_pr_degree) GMT->current.proj.pars[2] /= GMT->current.proj.M_PR_DEG;
	GMT->current.proj.scale[GMT_X] = GMT->current.proj.scale[GMT_Y] = GMT->current.proj.pars[2];
	GMT->current.proj.fwd = &GMT_polyconic;
	GMT->current.proj.inv = &GMT_ipolyconic;
	if (GMT->current.setting.map_frame_type & GMT_IS_FANCY) GMT->current.setting.map_frame_type = GMT_IS_PLAIN;

	if (GMT->common.R.oblique) {
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &xmin, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YHI], &xmax, &ymax);
		GMT->current.map.outside = &gmt_rect_outside;
		GMT->current.map.crossing = &gmt_rect_crossing;
		GMT->current.map.overlap = &gmt_rect_overlap;
		GMT->current.map.clip = &gmt_rect_clip;
		GMT->current.map.left_edge = &gmt_left_rect;
		GMT->current.map.right_edge = &gmt_right_rect;
		GMT->current.map.frame.check_side = true;
	}
	else {
		double y, dummy;
		y = (GMT->common.R.wesn[YLO] * GMT->common.R.wesn[YHI] <= 0.0) ? 0.0 : MIN (fabs (GMT->common.R.wesn[YLO]), fabs (GMT->common.R.wesn[YHI]));
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XLO], y, &xmin, &dummy);
		(*GMT->current.proj.fwd) (GMT, GMT->common.R.wesn[XHI], y, &xmax, &dummy);
		(*GMT->current.proj.fwd) (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YLO], &dummy, &ymin);
		(*GMT->current.proj.fwd) (GMT, GMT->current.proj.central_meridian, GMT->common.R.wesn[YHI], &dummy, &ymax);
		GMT->current.map.outside = &gmt_wesn_outside;
		GMT->current.map.crossing = &gmt_wesn_crossing;
		GMT->current.map.overlap = &gmt_wesn_overlap;
		GMT->current.map.clip = &GMT_wesn_clip;
		GMT->current.map.left_edge = &GMT_left_polyconic;
		GMT->current.map.right_edge = &GMT_right_polyconic;
		GMT->current.proj.polar = true;
	}

	GMT->current.map.frame.horizontal = 1;
	gmt_map_setinfo (GMT, xmin, xmax, ymin, ymax, GMT->current.proj.pars[2]);

	return (GMT->common.R.oblique);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 *	S E C T I O N  1.1 :	S U P P O R T I N G   R O U T I N E S
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void gmt_wesn_search (struct GMT_CTRL *GMT, double xmin, double xmax, double ymin, double ymax, double *west, double *east, double *south, double *north) {
	double dx, dy, w, e, s, n, x, y, lat, *lon = NULL;
	unsigned int i, j, k;

	/* Search for extreme lon/lat coordinates by matching along the rectangular boundary */

	dx = (xmax - xmin) / GMT->current.map.n_lon_nodes;
	dy = (ymax - ymin) / GMT->current.map.n_lat_nodes;
	/* Need temp array to hold all the longitudes we compute */
	lon = GMT_memory (GMT, NULL, 2 * (GMT->current.map.n_lon_nodes + GMT->current.map.n_lat_nodes + 2), double);
	w = s = DBL_MAX;	e = n = -DBL_MAX;
	for (i = k = 0; i <= GMT->current.map.n_lon_nodes; i++) {
		x = (i == GMT->current.map.n_lon_nodes) ? xmax : xmin + i * dx;
		GMT_xy_to_geo (GMT, &lon[k++], &lat, x, ymin);
		if (lat < s) s = lat;	if (lat > n) n = lat;
		GMT_xy_to_geo (GMT, &lon[k++], &lat, x, ymax);
		if (lat < s) s = lat;	if (lat > n) n = lat;
	}
	for (j = 0; j <= GMT->current.map.n_lat_nodes; j++) {
		y = (j == GMT->current.map.n_lat_nodes) ? ymax : ymin + j * dy;
		GMT_xy_to_geo (GMT, &lon[k++], &lat, xmin, y);
		if (lat < s) s = lat;	if (lat > n) n = lat;
		GMT_xy_to_geo (GMT, &lon[k++], &lat, xmax, y);
		if (lat < s) s = lat;	if (lat > n) n = lat;
	}
	GMT_get_lon_minmax (GMT, lon, k, &w, &e);	/* Determine lon-range by robust quandrant check */
	GMT_free (GMT, lon);

	/* Then check if one or both poles are inside map; then the above wont be correct */

	if (!GMT_map_outside (GMT, GMT->current.proj.central_meridian, +90.0)) { n = +90.0; w = 0.0; e = 360.0; }
	if (!GMT_map_outside (GMT, GMT->current.proj.central_meridian, -90.0)) { s = -90.0; w = 0.0; e = 360.0; }

	s -= 0.1;	if (s < -90.0) s = -90.0;	/* Make sure point is not inside area, 0.1 is just a small arbitrary number */
	n += 0.1;	if (n > 90.0) n = 90.0;		/* But dont go crazy beyond the pole */
	w -= 0.1;	e += 0.1;	if (fabs (w - e) > 360.0) { w = 0.0; e = 360.0; }	/* Ensure max 360 range */
	*west = w;	*east = e;	*south = s;	*north = n;	/* Pass back our findings */
}

int gmt_horizon_search (struct GMT_CTRL *GMT, double w, double e, double s, double n, double xmin, double xmax, double ymin, double ymax) {
	double dx, dy, d, x, y, lon, lat;
	unsigned int i, j;
	bool beyond = false;

	/* Search for extreme original coordinates lon/lat and see if any fall beyond the horizon */

	dx = (xmax - xmin) / GMT->current.map.n_lon_nodes;
	dy = (ymax - ymin) / GMT->current.map.n_lat_nodes;
	if ((d = GMT_great_circle_dist_degree (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, w, s)) > GMT->current.proj.f_horizon) beyond = true;
	if ((d = GMT_great_circle_dist_degree (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, e, n)) > GMT->current.proj.f_horizon) beyond = true;
	for (i = 0; !beyond && i <= GMT->current.map.n_lon_nodes; i++) {
		x = (i == GMT->current.map.n_lon_nodes) ? xmax : xmin + i * dx;
		GMT_xy_to_geo (GMT, &lon, &lat, x, ymin);
		if ((d = GMT_great_circle_dist_degree (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, lon, lat)) > GMT->current.proj.f_horizon) beyond = true;
		GMT_xy_to_geo (GMT, &lon, &lat, x, ymax);
		if ((d = GMT_great_circle_dist_degree (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, lon, lat)) > GMT->current.proj.f_horizon) beyond = true;
	}
	for (j = 0; !beyond && j <= GMT->current.map.n_lat_nodes; j++) {
		y = (j == GMT->current.map.n_lat_nodes) ? ymax : ymin + j * dy;
		GMT_xy_to_geo (GMT, &lon, &lat, xmin, y);
		if ((d = GMT_great_circle_dist_degree (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, lon, lat)) > GMT->current.proj.f_horizon) beyond = true;
		GMT_xy_to_geo (GMT, &lon, &lat, xmax, y);
		if ((d = GMT_great_circle_dist_degree (GMT, GMT->current.proj.central_meridian, GMT->current.proj.pole, lon, lat)) > GMT->current.proj.f_horizon) beyond = true;
	}
	if (beyond) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "ERROR: Rectangular region for azimuthal projection extends beyond the horizon\n");
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "ERROR: Please select a region that is completely within the visible hemisphere\n");
		GMT_exit (GMT, EXIT_FAILURE); return EXIT_FAILURE;
	}
	return (GMT_NOERROR);
}

bool GMT_map_outside (struct GMT_CTRL *GMT, double lon, double lat)
{	/* Save current status in previous status and update current in/out status */
	GMT->current.map.prev_x_status = GMT->current.map.this_x_status;
	GMT->current.map.prev_y_status = GMT->current.map.this_y_status;
	return ((*GMT->current.map.outside) (GMT, lon, lat));
}

unsigned int gmt_wrap_around_check_x (struct GMT_CTRL *GMT, double *angle, double last_x, double last_y, double this_x, double this_y, double *xx, double *yy, unsigned int *sides)
{
	double dx, dy, width, jump, GMT_half_map_width (struct GMT_CTRL *GMT, double y);

	jump = this_x - last_x;
	width = MAX (GMT_half_map_width (GMT, this_y), GMT_half_map_width (GMT, last_y));

	if (fabs (jump) - width <= GMT_SMALL || fabs (jump) <= GMT_SMALL) return (0);
	dy = this_y - last_y;

	if (jump < -width) {	/* Crossed right boundary */
		dx = GMT->current.map.width + jump;
		yy[0] = yy[1] = last_y + (GMT->current.map.width - last_x) * dy / dx;
		if (yy[0] < 0.0 || yy[0] > GMT->current.proj.rect[YHI]) return (0);
		xx[0] = GMT_right_boundary (GMT, yy[0]);	xx[1] = GMT_left_boundary (GMT, yy[0]);
		sides[0] = 1;
		angle[0] = d_atan2d (dy, dx);
	}
	else {	/* Crossed left boundary */
		dx = GMT->current.map.width - jump;
		yy[0] = yy[1] = last_y + last_x * dy / dx;
		if (yy[0] < 0.0 || yy[0] > GMT->current.proj.rect[YHI]) return (0);
		xx[0] = GMT_left_boundary (GMT, yy[0]);	xx[1] = GMT_right_boundary (GMT, yy[0]);
		sides[0] = 3;
		angle[0] = d_atan2d (dy, -dx);
	}
	sides[1] = 4 - sides[0];
	angle[1] = angle[0] + 180.0;

	return (2);
}

void gmt_get_crossings_x (struct GMT_CTRL *GMT, double *xc, double *yc, double x0, double y0, double x1, double y1)
{	/* Finds crossings for wrap-arounds */
	double xa, xb, ya, yb, dxa, dxb, dyb, left_yb, c;

	xa = x0;	xb = x1;
	ya = y0;	yb = y1;
	if (xa > xb) {	/* Make A the minimum x point */
		double_swap (xa, xb);
		double_swap (ya, yb);
	}

	xb -= 2.0 * GMT_half_map_width (GMT, yb);

	dxa = xa - GMT_left_boundary (GMT, ya);
	left_yb = GMT_left_boundary (GMT, yb);
	dxb = left_yb - xb;
	c = (doubleAlmostEqualZero (left_yb, xb)) ? 0.0 : 1.0 + dxa/dxb;
	dyb = (GMT_IS_ZERO (c)) ? 0.0 : fabs (yb - ya) / c;
	yc[0] = yc[1] = (ya > yb) ? yb + dyb : yb - dyb;
	xc[0] = GMT_left_boundary (GMT, yc[0]);
	xc[1] = GMT_right_boundary (GMT, yc[0]);
}

double GMT_half_map_width (struct GMT_CTRL *GMT, double y)
{	/* Returns 1/2-width of map in inches given y value */
	double half_width;

	switch (GMT->current.proj.projection) {

		case GMT_STEREO:	/* Must compute width of circular map based on y value (ASSUMES FULL CIRCLE!!!) */
		case GMT_LAMB_AZ_EQ:
		case GMT_GNOMONIC:
		case GMT_AZ_EQDIST:
		case GMT_VANGRINTEN:
			if (!GMT->common.R.oblique && GMT->current.map.is_world) {
				y -= GMT->current.proj.r;
				half_width = d_sqrt (GMT->current.proj.r * GMT->current.proj.r - y * y);
			}
			else
				half_width = GMT->current.map.half_width;
			break;

		case GMT_ORTHO:
		case GMT_GENPER:
			if (!GMT->common.R.oblique && GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI])) {
				y -= GMT->current.proj.r;
				half_width = d_sqrt (GMT->current.proj.r * GMT->current.proj.r - y * y);
			}
			else
				half_width = GMT->current.map.half_width;
			break;
		case GMT_MOLLWEIDE:		/* Must compute width of Mollweide map based on y value */
		case GMT_HAMMER:
		case GMT_WINKEL:
		case GMT_SINUSOIDAL:
		case GMT_ROBINSON:
		case GMT_ECKERT4:
		case GMT_ECKERT6:
			if (!GMT->common.R.oblique && GMT->current.map.is_world)
				half_width = GMT_right_boundary (GMT, y) - GMT->current.map.half_width;
			else
				half_width = GMT->current.map.half_width;
			break;

		default:	/* Rectangular maps are easy */
			half_width = GMT->current.map.half_width;
			break;
	}
	return (half_width);
}

bool gmt_this_point_wraps_x (struct GMT_CTRL *GMT, double x0, double x1, double w_last, double w_this)
{
	/* Returns true if the 2 x-points implies a jump at this y-level of the map */

	double w_min, w_max, dx;

	if (w_this > w_last) {
		w_max = w_this;
		w_min = w_last;
	}
	else {
		w_max = w_last;
		w_min = w_this;
	}

	/* Second condition deals with points near bottom/top of maps
	   where map width may shrink to zero */

	return ((dx = fabs (x1 - x0)) > w_max && w_min > GMT_SMALL);
}

bool gmt_will_it_wrap_x (struct GMT_CTRL *GMT, double *x, double *y, uint64_t n, uint64_t *start)
{	/* Determines if a polygon will wrap at edges */
	uint64_t i;
	bool wrap;
	double w_last, w_this;

	if (!GMT->current.map.is_world) return (false);

	w_this = GMT_half_map_width (GMT, y[0]);
	for (i = 1, wrap = false; !wrap && i < n; i++) {
		w_last = w_this;
		w_this = GMT_half_map_width (GMT, y[i]);
		wrap = gmt_this_point_wraps_x (GMT, x[i-1], x[i], w_last, w_this);
	}
	*start = i - 1;
	return (wrap);
}

uint64_t gmt_map_truncate_x (struct GMT_CTRL *GMT, double *x, double *y, uint64_t n, uint64_t start, int l_or_r)
{	/* Truncates a wrapping polygon against left or right edge */

	uint64_t i, i1, j, k;
	double xc[4], yc[4], w_last, w_this;

	/* First initialize variables that differ for left and right truncation */

	if (l_or_r == -1)	/* Left truncation (-1) */
		/* Find first point that is left of map center */
		i = (x[start] < GMT->current.map.half_width) ? start : start - 1;
	else				/* Right truncation (+1) */
		/* Find first point that is right of map center */
		i = (x[start] > GMT->current.map.half_width) ? start : start - 1;

	if (!GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);

	GMT->current.plot.x[0] = x[i];	GMT->current.plot.y[0] = y[i];
	w_this = GMT_half_map_width (GMT, y[i]);
	k = j = 1;
	while (k <= n) {
		i1 = i;
		i = (i + 1)%n;	/* Next point */
		w_last = w_this;
		w_this = GMT_half_map_width (GMT, y[i]);
		if (gmt_this_point_wraps_x (GMT, x[i1], x[i], w_last, w_this)) {
			(*GMT->current.map.get_crossings) (GMT, xc, yc, x[i1], y[i1], x[i], y[i]);
			if (l_or_r == -1)
				GMT->current.plot.x[j] = GMT_left_boundary (GMT, yc[0]);
			else
				GMT->current.plot.x[j] = GMT_right_boundary (GMT, yc[0]);
			GMT->current.plot.y[j] = yc[0];
			j++;
			if (j >= GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);
		}
		if (l_or_r == -1) /* Left */
			GMT->current.plot.x[j] = (x[i] >= GMT->current.map.half_width) ? GMT_left_boundary (GMT, y[i]) : x[i];
		else	/* Right */
			GMT->current.plot.x[j] = (x[i] < GMT->current.map.half_width) ? GMT_right_boundary (GMT, y[i]) : x[i];
		GMT->current.plot.y[j] = y[i];
		j++, k++;
		if (j >= GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);
	}
	return (j);
}

uint64_t gmt_map_truncate_tm (struct GMT_CTRL *GMT, double *x, double *y, uint64_t n, uint64_t start, int b_or_t)
{	/* Truncates a wrapping polygon against bottom or top edge for global TM maps */

	uint64_t i, i1, j, k;
	double xc[4], yc[4], trunc_y;

	/* First initialize variables that differ for bottom and top truncation */

	if (b_or_t == -1) {	/* Bottom truncation (-1) */
		/* Find first point that is below map center */
		i = (y[start] < GMT->current.map.half_height) ? start : start - 1;
		trunc_y = 0.0;
	}
	else {				/* Top truncation (+1) */
		/* Find first point that is above map center */
		i = (y[start] > GMT->current.map.half_height) ? start : start - 1;
		trunc_y = GMT->current.map.height;
	}

	if (!GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);

	GMT->current.plot.x[0] = x[i];	GMT->current.plot.y[0] = y[i];
	k = j = 1;
	while (k <= n) {
		i1 = i;
		i = (i + 1)%n;	/* Next point */
		if (gmt_this_point_wraps_tm (GMT, y[i1], y[i])) {
			gmt_get_crossings_tm (GMT, xc, yc, x[i1], y[i1], x[i], y[i]);
			GMT->current.plot.x[j] = xc[0];
			GMT->current.plot.y[j] = trunc_y;
			j++;
			if (j >= GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);
		}
		if (b_or_t == -1) /* Bottom */
			GMT->current.plot.y[j] = (y[i] >= GMT->current.map.half_height) ? 0.0 : y[i];
		else	/* Top */
			GMT->current.plot.y[j] = (y[i] < GMT->current.map.half_height) ? GMT->current.map.height : y[i];
		GMT->current.plot.x[j] = x[i];
		j++, k++;
		if (j >= GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);
	}
	return (j);
}

uint64_t GMT_map_truncate (struct GMT_CTRL *GMT, double *x, double *y, uint64_t n, uint64_t start, int side)
{	/* Truncates a wrapping polygon against left or right edge.
	   (bottom or top edge when projection is TM)
	   x, y : arrays of plot coordinates
	   n    : length of the arrays
	   start: first point of array to consider
	   side : -1 = left (bottom); +1 = right (top)
	*/

	if (GMT->current.proj.projection == GMT_TM)
		return (gmt_map_truncate_tm (GMT, x, y, n, start, side));
	else
		return (gmt_map_truncate_x (GMT, x, y, n, start, side));
}

/* GMT generic distance calculations between a pair of points in 2-D */

double GMT_distance_type (struct GMT_CTRL *GMT, double lonS, double latS, double lonE, double latE, unsigned int id)
{	/* Generic function available to programs for contour/label distance calculations */
	return (GMT->current.map.dist[id].scale * GMT->current.map.dist[id].func (GMT, lonS, latS, lonE, latE));
}

double GMT_distance (struct GMT_CTRL *GMT, double lonS, double latS, double lonE, double latE)
{	/* Generic function available to programs */
	return (GMT_distance_type (GMT, lonS, latS, lonE, latE, 0));
}

bool GMT_near_a_point (struct GMT_CTRL *GMT, double lon, double lat, struct GMT_DATATABLE *T, double dist)
{	/* Compute distance to nearest point in T from (lon, lat) */
	return (GMT->current.map.near_point_func (GMT, lon, lat, T, dist));
}

bool GMT_near_lines (struct GMT_CTRL *GMT, double lon, double lat, struct GMT_DATATABLE *T, unsigned int return_mindist, double *dist_min, double *x_near, double *y_near)
{	/* Compute distance to nearest line in T from (lon,lat) */
	return (GMT->current.map.near_lines_func (GMT, lon, lat, T, return_mindist, dist_min, x_near, y_near));
}

bool GMT_near_a_line (struct GMT_CTRL *GMT, double lon, double lat, uint64_t seg, struct GMT_DATASEGMENT *S, unsigned int return_mindist, double *dist_min, double *x_near, double *y_near)
{	/* Compute distance to the line S from (lon,lat) */
	return (GMT->current.map.near_a_line_func (GMT, lon, lat, seg, S, return_mindist, dist_min, x_near, y_near));
}

/* Specific functions that are accessed via pointer only */

double GMT_cartesian_dist (struct GMT_CTRL *GMT, double x0, double y0, double x1, double y1)
{	/* Calculates the good-old straight line distance in users units */
	return (hypot ( (x1 - x0), (y1 - y0)));
}

double GMT_cartesian_dist2 (struct GMT_CTRL *GMT, double x0, double y0, double x1, double y1)
{	/* Calculates the good-old straight line distance squared in users units */
	x1 -= x0;	y1 -= y0;
	return (x1*x1 + y1*y1);
}

double GMT_cartesian_dist_proj (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2)
{	/* Calculates the good-old straight line distance after first projecting the data */
	double x0, y0, x1, y1;
	GMT_geo_to_xy (GMT, lon1, lat1, &x0, &y0);
	GMT_geo_to_xy (GMT, lon2, lat2, &x1, &y1);
	return (hypot ( (x1 - x0), (y1 - y0)));
}

double GMT_cartesian_dist_proj2 (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2)
{	/* Calculates the good-old straight line distance squared after first projecting the data */
	double x0, y0, x1, y1;
	GMT_geo_to_xy (GMT, lon1, lat1, &x0, &y0);
	GMT_geo_to_xy (GMT, lon2, lat2, &x1, &y1);
	x1 -= x0;	y1 -= y0;
	return (x1*x1 + y1*y1);
}

double gmt_flatearth_dist_degree (struct GMT_CTRL *GMT, double x0, double y0, double x1, double y1)
{
	/* Calculates the approximate flat earth distance in degrees.
	   If difference in longitudes exceeds 180 we pick the other
	   offset (360 - offset)
	 */
	double dlon;
	
	GMT_set_delta_lon (x0, x1, dlon);
	return (hypot ( dlon * cosd (0.5 * (y1 + y0)), (y1 - y0)));
}

double gmt_flatearth_dist_meter (struct GMT_CTRL *GMT, double x0, double y0, double x1, double y1)
{
	/* Calculates the approximate flat earth distance in km.
	   If difference in longitudes exceeds 180 we pick the other
	   offset (360 - offset)
	 */
	return (gmt_flatearth_dist_degree (GMT, x0, y0, x1, y1) * GMT->current.proj.DIST_M_PR_DEG);
}

double gmt_haversine (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2)
{
	/* Haversine formula for great circle distance.  Intermediate function that returns sin^2 (half_angle).
	 * This avoids problems with short distances where cos(c) is close to 1 and acos is inaccurate.
	 */

	double sx, sy, sin_half_squared;

	if (lat1 == lat2 && lon1 == lon2) return (0.0);

	if (GMT->current.setting.proj_aux_latitude != GMT_LATSWAP_NONE) {	/* Use selected auxiliary latitude */
		lat1 = GMT_lat_swap (GMT, lat1, GMT->current.setting.proj_aux_latitude);
		lat2 = GMT_lat_swap (GMT, lat2, GMT->current.setting.proj_aux_latitude);
	}

	sy = sind (0.5 * (lat2 - lat1));
	sx = sind (0.5 * (lon2 - lon1));	/* If there is a 360 wrap here then the sign of sx is wrong but we only use sx^2 */
	sin_half_squared = sy * sy + cosd (lat2) * cosd (lat1) * sx * sx;

	return (sin_half_squared);
}

double GMT_great_circle_dist_cos (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2)
{
	/* Return cosine of great circle distance */

	double sin_half_squared = gmt_haversine (GMT, lon1, lat1, lon2, lat2);
	return (1.0 - 2.0 * sin_half_squared);	/* Convert sin^2 (half-angle) to cos (angle) */
}

double GMT_great_circle_dist_degree (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2)
{	/* Great circle distance on a sphere in degrees */

	double sin_half_squared = gmt_haversine (GMT, lon1, lat1, lon2, lat2);
	return (2.0 * d_asind (d_sqrt (sin_half_squared)));
}

double GMT_great_circle_dist_meter (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2)
{	/* Calculates the great circle distance in meter */
	return (GMT_great_circle_dist_degree (GMT, lon1, lat1, lon2, lat2) * GMT->current.proj.DIST_M_PR_DEG);
}

double gmt_geodesic_dist_degree (struct GMT_CTRL *GMT, double lonS, double latS, double lonE, double latE)
{
	/* Compute the great circle arc length in degrees on an ellipsoidal
	 * Earth.  We do this by converting to geocentric coordinates.
	 */

	double a, b, c, d, e, f, a1, b1, c1, d1, e1, f1, thg, sc, sd, dist;

	/* Equations are unstable for latitudes of exactly 0 degrees. */

	if (latE == 0.0) latE = 1.0e-08;
	if (latS == 0.0) latS = 1.0e-08;

	/* Must convert from geographic to geocentric coordinates in order
	 * to use the spherical trig equations.  This requires a latitude
	 * correction given by: 1-ECC2=1-2*F + F*F = GMT->current.proj.one_m_ECC2
	 */

	thg = atan (GMT->current.proj.one_m_ECC2 * tand (latE));
	sincos (thg, &c, &f);		f = -f;
	sincosd (lonE, &d, &e);		e = -e;
	a = f * e;
	b = -f * d;

	/* Calculate some trig constants. */

	thg = atan (GMT->current.proj.one_m_ECC2 * tand (latS));
	sincos (thg, &c1, &f1);		f1 = -f1;
	sincosd (lonS, &d1, &e1);	e1 = -e1;
	a1 = f1 * e1;
	b1 = -f1 * d1;

	/* Spherical trig relationships used to compute angles. */

	sc = a * a1 + b * b1 + c * c1;
	sd = 0.5 * sqrt ((pow (a-a1,2.0) + pow (b-b1,2.0) + pow (c-c1,2.0)) * (pow (a+a1,2.0) + pow (b+b1, 2.0) + pow (c+c1, 2.0)));
	dist = atan2d (sd, sc);
	if (dist < 0.0) dist += 360.0;

	return (dist);
}

double GMT_geodesic_dist_cos (struct GMT_CTRL *GMT, double lonS, double latS, double lonE, double latE)
{	/* Convenience function to get cosine instead */
	return (cosd (gmt_geodesic_dist_degree (GMT, lonS, latS, lonE, latE)));
}

#if USE_VINCENTY
double gmt_geodesic_dist_meter (struct GMT_CTRL *GMT, double lonS, double latS, double lonE, double latE)
{
	/* Translation of NGS FORTRAN code for determination of true distance
	** and respective forward and back azimuths between two points on the
	** ellipsoid.  Good for any pair of points that are not antipodal.
	**
	**      INPUT
	**	latS, lonS -- latitude and longitude of first point in radians.
	**	latE, lonE -- latitude and longitude of second point in radians.
	**
	**	OUTPUT
	**	s -- distance between points in meters.
	** Modified by P.W. from: http://article.gmane.org/gmane.comp.gis.proj-4.devel/3478
	*/
	static double s, c, d, e, r, f, d_lon, dx, x, y, sa, cx, cy, cz, sx, sy, c2a, cu1, cu2, su1, tu1, tu2, ts, baz, faz;
	int n_iter = 0;

	f = GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].flattening;
	r = 1.0 - f;
	tu1 = r * tand (latS);
	tu2 = r * tand (latE);
	cu1 = 1.0 / sqrt (tu1 * tu1 + 1.0);
	su1 = cu1 * tu1;
	cu2 = 1.0 / sqrt (tu2 * tu2 + 1.0);
	ts  = cu1 * cu2;
	baz = ts * tu2;
	faz = baz * tu1;
	GMT_set_delta_lon (lonS, lonE, d_lon);
	x = dx = D2R * d_lon;
	do {
		n_iter++;
		sincos (x, &sx, &cx);
		tu1 = cu2 * sx;
		tu2 = baz - su1 * cu2 * cx;
		sy = sqrt (tu1 * tu1 + tu2 * tu2);
		cy = ts * cx + faz;
		y = atan2 (sy, cy);
		sa = ts * sx / sy;
		c2a = -sa * sa + 1.0;
		cz = faz + faz;
		if (c2a > 0.0) cz = -cz / c2a + cy;
		e = cz * cz * 2.0 - 1.0;
		c = ((c2a * -3.0 + 4.0) * f + 4.0) * c2a * f / 16.0;
		d = x;
		x = ((e * cy * c + cz) * sy * c + y) * sa;
		x = (1.0 - c) * x * f + dx;
	} while (fabs (d - x) > VINCENTY_EPS && n_iter <= 50);
	if (n_iter > VINCENTY_MAX_ITER) {
		GMT->current.proj.n_geodesic_approx++;	/* Count inaccurate results */
		GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Near- or actual antipodal points encountered. Precision may be reduced slightly.\n");
		s = M_PI;
	}
	else {
		x = sqrt ((1.0 / r / r - 1.0) * c2a + 1.0) + 1.0;
		x = (x - 2.0) / x;
		c = (x * x / 4.0 + 1.0) / (1.0 - x);
		d = (x * 0.375 * x - 1.0) * x;
		s = ((((sy * sy * 4.0 - 3.0) * (1.0 - e - e) * cz * d / 6.0 - e * cy) * d / 4.0 + cz) * sy * d + y) * c * r;
		if (s > M_PI) {
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Near- or actual antipodal points encountered. Precision may be reduced slightly.\n");
			s = M_PI;
		}
	}
	GMT->current.proj.n_geodesic_calls++;
	return (s * GMT->current.proj.EQ_RAD);
}
#else
double gmt_geodesic_dist_meter (struct GMT_CTRL *GMT, double lonS, double latS, double lonE, double latE)
{
	/* Compute length of geodesic between locations in meters
	 * We use Rudoe's equation from Bomford.
	 */

	double e1, el, sinthi, costhi, sinthk, costhk, tanthi, tanthk, sina12, cosa12, d_lon;
	double al, a12top, a12bot, a12, e2, e3, c0, c2, c4, v1, v2, z1, z2, x2, y2, dist;
	double e1p1, sqrte1p1, sin_dl, cos_dl, u1bot, u1, u2top, u2bot, u2, b0, du, pdist;


	/* Equations are unstable for latitudes of exactly 0 degrees. */

	if (latE == 0.0) latE = 1.0e-08;
	if (latS == 0.0) latS = 1.0e-08;

	/* Now compute the distance between the two points using Rudoe's
	 * formula given in Bomford's GEODESY, section 2.15(b).
	 * (Unclear if it is 1971 or 1980 edition)
	 * (There is some numerical problem with the following formulae.
	 * If the station is in the southern hemisphere and the event in
	 * in the northern, these equations give the longer, not the
	 * shorter distance between the two locations.  Since the equations
	 * are fairly messy, the simplist solution is to reverse the
	 * meanings of the two locations for this case.)
	 */

	if (latS < 0.0) {	/* Station in southern hemisphere, swap */
		double_swap (lonS, lonE);
		double_swap (latS, latE);
	}
	el = GMT->current.proj.ECC2 / GMT->current.proj.one_m_ECC2;
	e1 = 1.0 + el;
	sincosd (latE, &sinthi, &costhi);
	sincosd (latS, &sinthk, &costhk);
	GMT_set_delta_lon (lonS, lonE, d_lon);
	sincosd (d_lon, &sin_dl, &cos_dl);
	tanthi = sinthi / costhi;
	tanthk = sinthk / costhk;
	al = tanthi / (e1 * tanthk) + GMT->current.proj.ECC2 * sqrt ((e1 + tanthi * tanthi) / (e1 + tanthk * tanthk));
	a12top = sin_dl;
	a12bot = (al - cos_dl) * sinthk;
	a12 = atan2 (a12top,a12bot);
	sincos (a12, &sina12, &cosa12);
	e1 = el * (pow (costhk * cosa12, 2.0) + sinthk * sinthk);
	e2 = e1 * e1;
	e3 = e1 * e2;
	c0 = 1.0 + 0.25 * e1 - (3.0 / 64.0) * e2 + (5.0 / 256.0) * e3;
	c2 = -0.125 * e1 + (1.0 / 32) * e2 - (15.0 / 1024.0) * e3;
	c4 = -(1.0 / 256.0) * e2 + (3.0 / 1024.0) * e3;
	v1 = GMT->current.proj.EQ_RAD / sqrt (1.0 - GMT->current.proj.ECC2 * sinthk * sinthk);
	v2 = GMT->current.proj.EQ_RAD / sqrt (1.0 - GMT->current.proj.ECC2 * sinthi * sinthi);
	z1 = v1 * (1.0 - GMT->current.proj.ECC2) * sinthk;
	z2 = v2 * (1.0 - GMT->current.proj.ECC2) * sinthi;
	x2 = v2 * costhi * cos_dl;
	y2 = v2 * costhi * sin_dl;
	e1p1 = e1 + 1.0;
	sqrte1p1 = sqrt (e1p1);
	u1bot = sqrte1p1 * cosa12;
	u1 = atan2 (tanthk, u1bot);
	u2top = v1 * sinthk + e1p1 * (z2 - z1);
	u2bot = sqrte1p1 * (x2 * cosa12 - y2 * sinthk * sina12);
	u2 = atan2 (u2top, u2bot);
	b0 = v1 * sqrt (1.0 + el * pow (costhk * cosa12, 2.0)) / e1p1;
	du = u2  - u1;
	if (fabs (du) > M_PI) du = copysign (TWO_PI - fabs (du), du);
	pdist = b0 * (c2 * (sin (2.0 * u2) - sin(2.0 * u1)) + c4 * (sin (4.0 * u2) - sin (4.0 * u1)));
	dist = fabs (b0 * c0 * du + pdist);

	return (dist);
}
#endif

double gmt_loxodrome_dist_degree (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2)
{	/* Calculates the distance along the loxodrome, in meter */
	double dist, d_lon;
	GMT_set_delta_lon (lon1, lon2, d_lon);
	if (doubleAlmostEqualZero (lat1, lat2)) {	/* Along parallel */
		if (GMT->current.proj.GMT_convert_latitudes) lat1 = GMT_latg_to_latc (GMT, lat1);
		dist = fabs (d_lon) * cosd (lat1);
	}
	else { /* General case */
		double dx, dy, Az;
		if (GMT->current.proj.GMT_convert_latitudes) {
			lat1 = GMT_latg_to_latc (GMT, lat1);
			lat2 = GMT_latg_to_latc (GMT, lat2);
		}
		dx = D2R * d_lon;
		dy = d_log (GMT, tand (45.0 + 0.5 * lat2)) - d_log (GMT, tand (45.0 + 0.5 * lat1));
		Az = atan2 (dx, dy);
		dist = fabs (dx / cos (Az));
	}
	return (dist);
}

double gmt_loxodrome_dist_meter (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2)
{	/* Calculates the loxodrome distance in meter */
	return (gmt_loxodrome_dist_degree (GMT, lon1, lat1, lon2, lat2) * GMT->current.proj.DIST_M_PR_DEG);
}

double gmt_az_backaz_loxodrome (struct GMT_CTRL *GMT, double lonE, double latE, double lonS, double latS, bool baz)
{
	/* Calculate azimuths or backazimuths.  Loxodrome mode.
	 * First point is considered "Event" and second "Station".
	 * Azimuth is direction from Station to Event.
	 * BackAzimuth is direction from Event to Station */

	double az, d_lon;

	if (baz) {	/* exchange point one and two */
		double_swap (lonS, lonE);
		double_swap (latS, latE);
	}
	GMT_set_delta_lon (lonE, lonS, d_lon);
	if (doubleAlmostEqualZero (latS, latE))	/* Along parallel */
		az = (d_lon > 0.0) ? 90 : -90.0;
	else { /* General case */
		double dx, dy;
		if (GMT->current.proj.GMT_convert_latitudes) {
			latS = GMT_latg_to_latc (GMT, latS);
			latE = GMT_latg_to_latc (GMT, latE);
		}
		dx = D2R * d_lon;
		dy = d_log (GMT, tand (45.0 + 0.5 * latS)) - d_log (GMT, tand (45.0 + 0.5 * latE));
		az = atan2d (dx, dy);
		if (az < 0.0) az += 360.0;
	}
	return (az);
}

/* Functions dealing with distance between points */

double GMT_mindist_to_point (struct GMT_CTRL *GMT, double lon, double lat, struct GMT_DATATABLE *T, uint64_t *id)
{
	uint64_t row, seg;
	double d, d_min;

	d_min = DBL_MAX;
	for (seg = 0; seg < T->n_segments; seg++) {
		for (row = 0; row < T->segment[seg]->n_rows; row++) {
			d = GMT_distance (GMT, lon, lat, T->segment[seg]->coord[GMT_X][row], T->segment[seg]->coord[GMT_Y][row]);
			if (d < d_min) {	/* Update the shortest distance and the point responsible */
				d_min = d;	id[0] = seg;	id[1] = row;
			}
		}
	}
	return (d_min);
}

bool gmt_near_a_point_spherical (struct GMT_CTRL *GMT, double x, double y, struct GMT_DATATABLE *T, double dist)
{
	uint64_t row, seg;
	bool each_point_has_distance;
	double d;

	each_point_has_distance = (dist <= 0.0 && T->segment[0]->n_columns > 2);
	for (seg = 0; seg < T->n_segments; seg++) {
		for (row = 0; row < T->segment[seg]->n_rows; row++) {
			d = GMT_distance (GMT, x, y, T->segment[seg]->coord[GMT_X][row], T->segment[seg]->coord[GMT_Y][row]);
			if (each_point_has_distance) dist = T->segment[seg]->coord[GMT_Z][row];
			if (d <= dist) return (true);
		}
	}
	return (false);
}

bool gmt_near_a_point_cartesian (struct GMT_CTRL *GMT, double x, double y, struct GMT_DATATABLE *T, double dist)
{	/* Since Cartesian we use a GMT_distance set to return distances^2 (avoiding hypot) */
	bool each_point_has_distance;
	uint64_t row, seg;
	double d, x0, y0, xn, d0, d02 = 0.0, dn;

	each_point_has_distance = (dist <= 0.0 && T->segment[0]->n_columns > 2);

	/* Assumes the points have been sorted so xp[0] is xmin and xp[n-1] is xmax] !!! */

	/* See if we are safely outside the range */
	x0 = T->segment[0]->coord[GMT_X][0];
	d0 = (each_point_has_distance) ? T->segment[0]->coord[GMT_Z][0] : dist;
	xn = T->segment[T->n_segments-1]->coord[GMT_X][T->segment[T->n_segments-1]->n_rows-1];
	dn = (each_point_has_distance) ? T->segment[T->n_segments-1]->coord[GMT_Z][T->segment[T->n_segments-1]->n_rows-1] : dist;
	if ((x < (x0 - d0)) || (x > (xn) + dn)) return (false);

	/* No, must search the points */
	if (!each_point_has_distance) d02 = dist * dist;
	
	for (seg = 0; seg < T->n_segments; seg++) {
		for (row = 0; row < T->segment[seg]->n_rows; row++) {
			x0 = T->segment[seg]->coord[GMT_X][row];
			d0 = (each_point_has_distance) ? T->segment[seg]->coord[GMT_Z][row] : dist;
			if (fabs (x - x0) <= d0) {	/* Simple x-range test first */
				y0 = T->segment[seg]->coord[GMT_Y][row];
				if (fabs (y - y0) <= d0) {	/* Simple y-range test next */
					/* Here we must compute distance */
					if (each_point_has_distance) d02 = d0 * d0;
					d = GMT_distance (GMT, x, y, x0, y0);
					if (d <= d02) return (true);
				}
			}
		}
	}
	return (false);
}

/* Functions involving distance from arbitrary points to a line */

bool gmt_near_a_line_cartesian (struct GMT_CTRL *GMT, double lon, double lat, uint64_t seg, struct GMT_DATASEGMENT *S, unsigned int return_mindist, double *dist_min, double *x_near, double *y_near)
{
	bool perpendicular_only = false, interior, within;
	uint64_t row0, row1;
	double edge, dx, dy, xc, yc, s, s_inv, d, dist_AB, fraction;
	/* gmt_near_a_line_cartesian works in one of two modes, depending on return_mindist.
	   Since return_mindist is composed of two settings we must first set
	   perpendicular_only = (return_mindist >= 10);
	   return_mindist -= 10 * perpendicular_only;
	   That is, if 10 was added it means perpendicular_only is set and then the 10 is
	   removed.  We now consider what is left of return_mindist:
	   (1) return_mindist == 0:
	      We expect each segment to have its dist variable set to a minimum distance,
	      and if the point is within this distance from the line then we return true;
	      otherwise we return false.  If the segments have not set their distances then
	      it will have been initialized at 0 and only a point on the line will return true.
	      If perpendicular_only we ignore a point that is within the distance of the
	      linesegment endpoints but project onto the extension of the line (i.e., it is
	      "outside" the extent of the line).  We return false in that case.
	   (2) return_mindist != 0:
	      Return the minimum distance via dist_min. In addition, if > 1:
	      If == 2 we also return the coordinate of nearest point via x_near, y_near.
	      If == 3 we instead return segment number and point number (fractional) of that point via x_near, y_near.
	      The function will always return true, except if perpendicular_only is set: then we
	      return false if the point projects onto the extension of the line (i.e., it is "outside"
	      the extent of the line).  */

	if (return_mindist >= 10) {	/* Exclude circular region surrounding line endpoints */
		perpendicular_only = true;
		return_mindist -= 10;
	}

	if (S->n_rows <= 0) return (false);	/* empty; skip */

	if (return_mindist) S->dist = 0.0;	/* Explicitly set dist to zero so the shortest distance can be found */

	/* Find nearest point on this line */

	for (row0 = 0; row0 < S->n_rows; row0++) {	/* loop over nodes on current line */
		d = GMT_distance (GMT, lon, lat, S->coord[GMT_X][row0], S->coord[GMT_Y][row0]);	/* Distance between our point and j'th node on seg'th line */
		if (return_mindist && d < (*dist_min)) {	/* Update min distance */
			*dist_min = d;
			if (return_mindist == 2) { *x_near = S->coord[GMT_X][row0]; *y_near = S->coord[GMT_Y][row0]; }	/* Also update (x,y) of nearest point on the line */
			else if (return_mindist == 3) { *x_near = (double)seg; *y_near = (double)row0;}		/* Instead update (seg, pt) of nearest point on the line */
		}
		interior = (row0 > 0 && row0 < (S->n_rows - 1));	/* Only false if we are processing one of the end points */
		if (d <= S->dist && (interior || !perpendicular_only)) return (true);		/* Node inside the critical distance; we are done */
	}

	if (S->n_rows < 2) return (false);	/* 1-point "line" is a point; skip segment check */

	/* If we get here we must check for intermediate points along the straight lines between segment nodes.
	 * However, since we know all nodes are outside the circle, we first check if the pair of nodes making
	 * up the next line segment are outside of the circumscribing square before we need to solve for the
	 * intersection between the line segment and the normal from our point. */

	for (row0 = 0, row1 = 1, within = false; row1 < S->n_rows; row0++, row1++) {	/* loop over straight segments on current line */
		if (!return_mindist) {
			edge = lon - S->dist;
			if (S->coord[GMT_X][row0] < edge && S->coord[GMT_X][row1] < edge) continue;	/* Left of square */
			edge = lon + S->dist;
			if (S->coord[GMT_X][row0] > edge && S->coord[GMT_X][row1] > edge) continue;	/* Right of square */
			edge = lat - S->dist;
			if (S->coord[GMT_Y][row0] < edge && S->coord[GMT_Y][row1] < edge) continue;	/* Below square */
			edge = lat + S->dist;
			if (S->coord[GMT_Y][row0] > edge && S->coord[GMT_Y][row1] > edge) continue;	/* Above square */
		}

		/* Here there is potential for the line segment crossing inside the circle */

		dx = S->coord[GMT_X][row1] - S->coord[GMT_X][row0];
		dy = S->coord[GMT_Y][row1] - S->coord[GMT_Y][row0];
		if (dx == 0.0) {		/* Line segment is vertical, our normal is thus horizontal */
			if (dy == 0.0) continue;	/* Dummy segment with no length */
			xc = S->coord[GMT_X][row0];
			yc = lat;
			if (S->coord[GMT_Y][row0] < yc && S->coord[GMT_Y][row1] < yc ) continue;	/* Cross point is on extension */
			if (S->coord[GMT_Y][row0] > yc && S->coord[GMT_Y][row1] > yc ) continue;	/* Cross point is on extension */
		}
		else {	/* Line segment is not vertical */
			if (dy == 0.0) {	/* Line segment is horizontal, our normal is thus vertical */
				xc = lon;
				yc = S->coord[GMT_Y][row0];
			}
			else {	/* General case of oblique line */
				s = dy / dx;
				s_inv = -1.0 / s;
				xc = (lat - S->coord[GMT_Y][row0] + s * S->coord[GMT_X][row0] - s_inv * lon ) / (s - s_inv);
				yc = S->coord[GMT_Y][row0] + s * (xc - S->coord[GMT_X][row0]);

			}
			/* To be inside, (xc, yc) must (1) be on the line segment and not its extension and (2) be within dist of our point */

			if (S->coord[GMT_X][row0] < xc && S->coord[GMT_X][row1] < xc ) continue;	/* Cross point is on extension */
			if (S->coord[GMT_X][row0] > xc && S->coord[GMT_X][row1] > xc ) continue;	/* Cross point is on extension */
		}

		/* OK, here we must check how close the crossing point is */

		d = GMT_distance (GMT, lon, lat, xc, yc);			/* Distance between our point and intersection */
		if (return_mindist && d < (*dist_min)) {			/* Update min distance */
			*dist_min = d;
			if (return_mindist == 2) { *x_near = xc; *y_near = yc;}	/* Also update nearest point on the line */
			else if (return_mindist == 3) {	/* Instead update (seg, pt) of nearest point on the line */
				*x_near = (double)seg;
				dist_AB = GMT_distance (GMT, S->coord[GMT_X][row0], S->coord[GMT_Y][row0], S->coord[GMT_X][row1], S->coord[GMT_Y][row1]);
				fraction = (dist_AB > 0.0) ? GMT_distance (GMT, S->coord[GMT_X][row0], S->coord[GMT_Y][row0], xc, yc) / dist_AB : 0.0;
				*y_near = (double)row0 + fraction;
			}
			within = true;
		}
		if (d <= S->dist) return (true);		/* Node inside the critical distance; we are done */
	}

	return (within);	/* All tests failed, we are not close to the line(s), or we just return distance and interior (see comments above) */
}

bool gmt_near_lines_cartesian (struct GMT_CTRL *GMT, double lon, double lat, struct GMT_DATATABLE *T, unsigned int return_mindist, double *dist_min, double *x_near, double *y_near)
{
	uint64_t seg;
	int mode = return_mindist, status;
	bool OK = false;
	if (mode >= 10) mode -= 10;	/* Exclude (or flag) circular region surrounding line endpoints */
	if (mode) *dist_min = DBL_MAX;	/* Want to find the minimum distance so init to huge */

	for (seg = 0; seg < T->n_segments; seg++) {	/* Loop over each line segment */
		status = gmt_near_a_line_cartesian (GMT, lon, lat, seg, T->segment[seg], return_mindist, dist_min, x_near, y_near);
		if (status) {	/* Got a min distance or satisfied the min dist requirement */
			if (!return_mindist) return (true);	/* Done, we are within distance of one of the lines */
			OK = true;
		}
	}
	return (OK);	
}

bool gmt_near_a_line_spherical (struct GMT_CTRL *P, double lon, double lat, uint64_t seg, struct GMT_DATASEGMENT *S, unsigned int return_mindist, double *dist_min, double *x_near, double *y_near)
{
	bool perpendicular_only = false, interior, within;
	uint64_t row, prev_row;
	double d, A[3], B[3], GMT[3], X[3], xlon, xlat, cx_dist, cos_dist, dist_AB, fraction;

	/* gmt_near_a_line_spherical works in one of two modes, depending on return_mindist.
	   Since return_mindist is composed of two settings we must first set
	   perpendicular_only = (return_mindist >= 10);
	   return_mindist -= 10 * perpendicular_only;
	   That is, if 10 was added it means perpendicular_only is set and then the 10 is
	   removed.  We now consider what is left of return_mindist:
	   (1) return_mindist == 0:
	      We expect each segment to have its dist variable set to a minimum distance,
	      and if the point is within this distance from the line then we return true;
	      otherwise we return false.  If the segments have not set their distances then
	      it will have been initialized at 0 and only a point on the line will return true.
	      If perpendicular_only we ignore a point that is within the distance of the
	      linesegment endpoints but project onto the extension of the line (i.e., it is
	      "outside" the extent of the line).  We return false in that case.
	   (2) return_mindist != 0:
	      Return the minimum distance via dist_min. In addition, if > 1:
	      If == 2 we also return the coordinate of nearest point via x_near, y_near.
	      If == 3 we instead return segment number and point number (fractional) of that point via x_near, y_near.
	      The function will always return true, except if perpendicular_only is set: then we
	      return false if the point projects onto the extension of the line (i.e., it is "outside"
	      the extent of the line).  */

	if (return_mindist >= 10) {	/* Exclude (or flag) circular region surrounding line endpoints */
		perpendicular_only = true;
		return_mindist -= 10;
	}
	GMT_geo_to_cart (P, lat, lon, GMT, true);	/* Our point to test is now GMT */

	if (S->n_rows <= 0) return (false);	/* Empty ; skip */

	/* Find nearest point on this line */

	if (return_mindist) S->dist = 0.0;	/* Explicitly set dist to zero so the shortest distance can be found */

	for (row = 0; row < S->n_rows; row++) {	/* loop over nodes on current line */
		d = GMT_distance (P, lon, lat, S->coord[GMT_X][row], S->coord[GMT_Y][row]);	/* Distance between our point and row'th node on seg'th line */
		if (return_mindist && d < (*dist_min)) {	/* Update minimum distance */
			*dist_min = d;
			if (return_mindist == 2) *x_near = S->coord[GMT_X][row], *y_near = S->coord[GMT_Y][row];	/* Also update (x,y) of nearest point on the line */
			if (return_mindist == 3) *x_near = (double)seg, *y_near = (double)row;	/* Also update (seg, pt) of nearest point on the line */
		}
		interior = (row > 0 && row < (S->n_rows - 1));	/* Only false if we are processing one of the end points */
		if (d <= S->dist && (interior || !perpendicular_only)) return (true);			/* Node inside the critical distance; we are done */
	}

	if (S->n_rows < 2) return (false);	/* 1-point "line" is a point; skip segment check */

	/* If we get here we must check for intermediate points along the great circle lines between segment nodes.*/

	if (return_mindist)		/* Cosine of the great circle distance we are checking for. 2 ensures failure to be closer */
		cos_dist = 2.0;
	else if (P->current.map.dist[GMT_MAP_DIST].arc)	/* Used angular distance measure */
		cos_dist = cosd (S->dist / P->current.map.dist[GMT_MAP_DIST].scale);
	else	/* Used distance units (e.g., meter, km). Conv to meters, then to degrees */
		cos_dist = cosd ((S->dist / P->current.map.dist[GMT_MAP_DIST].scale) / P->current.proj.DIST_M_PR_DEG);
	GMT_geo_to_cart (P, S->coord[GMT_Y][0], S->coord[GMT_X][0], B, true);		/* 3-D vector of end of last segment */

	for (row = 1, within = false; row < S->n_rows; row++) {				/* loop over great circle segments on current line */
		GMT_memcpy (A, B, 3, double);	/* End of last segment is start of new segment */
		GMT_geo_to_cart (P, S->coord[GMT_Y][row], S->coord[GMT_X][row], B, true);	/* 3-D vector of end of this segment */
		if (GMT_great_circle_intersection (P, A, B, GMT, X, &cx_dist)) continue;	/* X not between A and B */
		if (return_mindist) {		/* Get lon, lat of X, calculate distance, and update min_dist if needed */
			GMT_cart_to_geo (P, &xlat, &xlon, X, true);
			d = GMT_distance (P, xlon, xlat, lon, lat);	/* Distance between our point and closest perpendicular point on seg'th line */
			if (d < (*dist_min)) {	/* Update minimum distance */
				*dist_min = d;
				if (return_mindist == 2) { *x_near = xlon; *y_near = xlat;}	/* Also update (x,y) of nearest point on the line */
				else if (return_mindist == 3) {	/* Also update (seg, pt) of nearest point on the line */
					*x_near = (double)seg;
					prev_row = row - 1;
					dist_AB = GMT_distance (P, S->coord[GMT_X][prev_row], S->coord[GMT_Y][prev_row], S->coord[GMT_X][row], S->coord[GMT_Y][row]);
					fraction = (dist_AB > 0.0) ? GMT_distance (P, S->coord[GMT_X][prev_row], S->coord[GMT_Y][prev_row], xlon, xlat) / dist_AB : 0.0;
					*y_near = (double)prev_row + fraction;
				}
				within = true;	/* Found at least one segment with a valid inside distance */
			}
		}
		if (cx_dist >= cos_dist) return (true);	/* X is on the A-B extension AND within specified distance */
	}

	return (within);	/* All tests failed, we are not close to the line(s), or we return a mindist (see comments above) */
}

bool gmt_near_lines_spherical (struct GMT_CTRL *P, double lon, double lat, struct GMT_DATATABLE *T, unsigned int return_mindist, double *dist_min, double *x_near, double *y_near)
{
	uint64_t seg;
	int mode = return_mindist, status;
	bool OK = false;
	if (mode >= 10) mode -= 10;	/* Exclude (or flag) circular region surrounding line endpoints */
	if (mode) *dist_min = DBL_MAX;	/* Want to find the minimum distance so init to huge */

	for (seg = 0; seg < T->n_segments; seg++) {	/* Loop over each line segment */
		status = gmt_near_a_line_spherical (P, lon, lat, seg, T->segment[seg], return_mindist, dist_min, x_near, y_near);
		if (status) {	/* Got a min distance or satisfied the min dist requirement */
			if (!return_mindist) return (true);	/* Done, we are within distance of one of the lines */
			OK = true;
		}
	}
	return (OK);
}

int GMT_great_circle_intersection (struct GMT_CTRL *GMT, double A[], double B[], double C[], double X[], double *CX_dist)
{
	/* A, B, C are 3-D Cartesian unit vectors, i.e., points on the sphere.
	 * Let points A and B define a great circle, and consider a
	 * third point C.  A second great cirle goes through C and
	 * is orthogonal to the first great circle.  Their intersection
	 * X is the point on (A,B) closest to C.  We must test if X is
	 * between A,B or outside.
	 */
	unsigned int i;
	double P[3], E[3], M[3], Xneg[3], cos_AB, cos_MX1, cos_MX2, cos_test;

	GMT_cross3v (GMT, A, B, P);			/* Get pole position of plane through A and B (and origin O) */
	GMT_normalize3v (GMT, P);			/* Make sure P has unit length */
	GMT_cross3v (GMT, C, P, E);			/* Get pole E to plane through C (and origin) but normal to A,B (hence going through P) */
	GMT_normalize3v (GMT, E);			/* Make sure E has unit length */
	GMT_cross3v (GMT, P, E, X);			/* Intersection between the two planes is oriented line*/
	GMT_normalize3v (GMT, X);			/* Make sure X has unit length */
	/* The X we want could be +x or -X; must determine which might be closest to A-B midpoint M */
	for (i = 0; i < 3; i++) {
		M[i] = A[i] + B[i];
		Xneg[i] = -X[i];
	}
	GMT_normalize3v (GMT, M);			/* Make sure M has unit length */
	/* Must first check if X is along the (A,B) segment and not on its extension */

	cos_MX1 = GMT_dot3v (GMT, M, X);		/* Cos of spherical distance between M and +X */
	cos_MX2 = GMT_dot3v (GMT, M, Xneg);	/* Cos of spherical distance between M and -X */
	if (cos_MX2 > cos_MX1) GMT_memcpy (X, Xneg, 3, double);		/* -X is closest to A-B midpoint */
	cos_AB = fabs (GMT_dot3v (GMT, A, B));	/* Cos of spherical distance between A,B */
	cos_test = fabs (GMT_dot3v (GMT, A, X));	/* Cos of spherical distance between A and X */
	if (cos_test < cos_AB) return 1;	/* X must be on the A-B extension if its distance to A exceeds the A-B length */
	cos_test = fabs (GMT_dot3v (GMT, B, X));	/* Cos of spherical distance between B and X */
	if (cos_test < cos_AB) return 1;	/* X must be on the A-B extension if its distance to B exceeds the A-B length */

	/* X is between A and B.  Now calculate distance between C and X */

	*CX_dist = GMT_dot3v (GMT, C, X);		/* Cos of spherical distance between C and X */
	return (0);				/* Return zero if intersection is between A and B */
}

uint64_t *GMT_split_line (struct GMT_CTRL *GMT, double **xx, double **yy, uint64_t *nn, bool add_crossings)
{	/* Accepts x/y array for a line in projected inches and looks for
	 * map jumps.  If found it will insert the boundary crossing points and
	 * build a split integer array with the nodes of the first point
	 * for each segment.  The number of segments is returned.  If
	 * no jumps are found then NULL is returned.  This function is needed
	 * so that the new PS contouring machinery only sees lines that do no
	 * jump across the map.
	 * add_crossings is true if we need to find the crossings; false means
	 * they are already part of the line. */

	uint64_t i, j, k, n, n_seg, *split = NULL, *pos = NULL;
	size_t n_alloc = 0;
 	int l_or_r;
 	char *way = NULL;
	double *x = NULL, *y = NULL, *xin = NULL, *yin = NULL, xc[2], yc[2];

	/* First quick scan to see how many jumps there are */

	xin = *xx;	yin = *yy;
	GMT_set_meminc (GMT, GMT_SMALL_CHUNK);
	for (n_seg = 0, i = 1; i < *nn; i++) {
		if ((l_or_r = gmt_map_jump_x (GMT, xin[i], yin[i], xin[i-1], yin[i-1]))) {
			if (n_seg == n_alloc) {
				pos = GMT_malloc (GMT, pos, n_seg, &n_alloc, uint64_t);
				n_alloc = n_seg;
				way = GMT_malloc (GMT, way, n_seg, &n_alloc, char);
			}
			pos[n_seg] = i;		/* 2nd of the two points that generate the jump */
			way[n_seg] = l_or_r;	/* Which way we jump : +1 is right to left, -1 is left to right */
			n_seg++;
		}
	}
	GMT_reset_meminc (GMT);

	if (n_seg == 0) return (NULL);	/* No jumps, just return NULL */

	/* Here we have one or more jumps so we need to split the line */

	n = *nn;				/* Original line count */
	if (add_crossings) n += 2 * n_seg;	/* Must add 2 crossing points per jump */
	n_alloc = 0;
	GMT_malloc2 (GMT, x, y, n, &n_alloc, double);
	split = GMT_memory (GMT, NULL, n_seg+2, uint64_t);
	split[0] = n_seg;

	x[0] = xin[0];	y[0] = yin[0];
	for (i = j = 1, k = 0; i < *nn; i++, j++) {
		if (k < n_seg && i == pos[k]) {	/* At jump point */
			if (add_crossings) {	/* Find and insert the crossings */
				gmt_get_crossings_x (GMT, xc, yc, xin[i], yin[i], xin[i-1], yin[i-1]);
				if (way[k] == 1) {	/* Add right border point first */
					double_swap (xc[0], xc[1]);
					double_swap (yc[0], yc[1]);
				}
				x[j] = xc[0];	y[j++] = yc[0];	/* End of one segment */
				x[j] = xc[1];	y[j++] = yc[1];	/* Start of another */
			}
			split[++k] = j;		/* Node of first point in new segment */
		}
		/* Then copy the regular points */
		x[j] = xin[i];	y[j] = yin[i];
	}
	split[++k] = j;		/* End of last segment */

	/* Time to return the pointers to new data */

	GMT_free (GMT, pos);
	GMT_free (GMT, way);
	GMT_free (GMT, xin);
	GMT_free (GMT, yin);
	*xx = x;
	*yy = y;
	*nn = j;

	return (split);
}

/*  Routines to add pieces of parallels or meridians */

uint64_t GMT_graticule_path (struct GMT_CTRL *GMT, double **x, double **y, int dir, double w, double e, double s, double n)
{	/* Returns the path of a graticule (box of meridians and parallels) */
	size_t n_alloc = 0;
	uint64_t np = 0;
	double *xx = NULL, *yy = NULL;
	double px0, px1, px2, px3;

	if (dir == 1) {	/* Forward sense */
		px0 = px3 = w;	px1 = px2 = e;
	}
	else {	/* Reverse sense */
		px0 = px3 = e;	px1 = px2 = w;
	}

	/* Close graticule from point 0 through point 4 */

	if (GMT_IS_RECT_GRATICULE(GMT)) {	/* Simple rectangle in this projection */
		GMT_malloc2 (GMT, xx, yy, 5U, NULL, double);
		xx[0] = xx[4] = px0;	xx[1] = px1;	xx[2] = px2;	xx[3] = px3;
		yy[0] = yy[1] = yy[4] = s;	yy[2] = yy[3] = n;
		np = 5U;
	}
	else {	/* Must assemble path from meridians and parallel pieces */
		double *xtmp = NULL, *ytmp = NULL;
		size_t add;
		uint64_t k;

		/* SOUTH BORDER */

		if (GMT_is_geographic (GMT, GMT_IN) && s == -90.0) {	/* No path, just a point */
			GMT_malloc2 (GMT, xx, yy, 1U, &n_alloc, double);
			xx[0] = px1;	yy[0] = -90.0;
		}
		else
			n_alloc = GMT_latpath (GMT, s, px0, px1, &xx, &yy);	/* South */
		np = n_alloc;

		/* EAST (OR WEST) BORDER */

		add = GMT_lonpath (GMT, px1, s, n, &xtmp, &ytmp);	/* east (or west if dir == -1) */
		k = n_alloc + add;
		GMT_malloc2 (GMT, xx, yy, k, &n_alloc, double);
		GMT_memcpy (&xx[np], xtmp, add, double);
		GMT_memcpy (&yy[np], ytmp, add, double);
		np += add;
		GMT_free (GMT, xtmp);	GMT_free (GMT, ytmp);

		/* NORTH BORDER */

		if (GMT_is_geographic (GMT, GMT_IN) && n == 90.0) {	/* No path, just a point */
			add = 0;
			GMT_malloc2 (GMT, xtmp, ytmp, 1U, &add, double);
			xtmp[0] = px3;	ytmp[0] = +90.0;
		}
		else
			add = GMT_latpath (GMT, n, px2, px3, &xtmp, &ytmp);	/* North */

		k = n_alloc + add;
		GMT_malloc2 (GMT, xx, yy, k, &n_alloc, double);
		GMT_memcpy (&xx[np], xtmp, add, double);
		GMT_memcpy (&yy[np], ytmp, add, double);
		np += add;
		GMT_free (GMT, xtmp);	GMT_free (GMT, ytmp);

		/* WEST (OR EAST) BORDER */

		add = GMT_lonpath (GMT, px3, n, s, &xtmp, &ytmp);	/* west */
		k = n_alloc + add;
		GMT_malloc2 (GMT, xx, yy, k, &n_alloc, double);
		GMT_memcpy (&xx[np], xtmp, add, double);
		GMT_memcpy (&yy[np], ytmp, add, double);
		np += add;
		n_alloc = np;
		GMT_free (GMT, xtmp);	GMT_free (GMT, ytmp);
		GMT_malloc2 (GMT, xx, yy, 0, &n_alloc, double);
	}

	if (GMT_x_is_lon (GMT, GMT_IN)) {
		bool straddle;
		uint64_t i;
		straddle = (GMT->common.R.wesn[XLO] < 0.0 && GMT->common.R.wesn[XHI] > 0.0);
		for (i = 0; straddle && i < np; i++) {
			while (xx[i] < 0.0) xx[i] += 360.0;
			if (straddle && xx[i] > 180.0) xx[i] -= 360.0;
		}
	}

	*x = xx;
	*y = yy;
	return (np);
}

uint64_t GMT_map_path (struct GMT_CTRL *GMT, double lon1, double lat1, double lon2, double lat2, double **x, double **y)
{
	if (doubleAlmostEqualZero (lat1, lat2))
		return (GMT_latpath (GMT, lat1, lon1, lon2, x, y));
	else
		return (GMT_lonpath (GMT, lon1, lat1, lat2, x, y));
}

uint64_t GMT_lonpath (struct GMT_CTRL *GMT, double lon, double lat1, double lat2, double **x, double **y)
{
	size_t n_alloc = 0;
	uint64_t n, k;
	int n_try, pos;
	bool keep_trying;
	double dlat, dlat0, *tlon = NULL, *tlat = NULL, x0, x1, y0, y1, d, min_gap;

	if (GMT->current.map.meridian_straight == 2) {	/* Special non-sampling for gmtselect/grdlandmask */
		GMT_malloc2 (GMT, tlon, tlat, 2U, NULL, double);
		tlon[0] = tlon[1] = lon;
		tlat[0] = lat1;	tlat[1] = lat2;
		*x = tlon;
		*y = tlat;
		return (2ULL);
	}

	if (GMT->current.map.meridian_straight) {	/* Easy, just a straight line connect via quarter-points */
		GMT_malloc2 (GMT, tlon, tlat, 5, &n_alloc, double);
		tlon[0] = tlon[1] = tlon[2] = tlon[3] = tlon[4] = lon;
		dlat = lat2 - lat1;
		tlat[0] = lat1;	tlat[1] = lat1 + 0.25 * dlat;	tlat[2] = lat1 + 0.5 * dlat;
		tlat[3] = lat1 + 0.75 * dlat;	tlat[4] = lat2;
		*x = tlon;
		*y = tlat;
		return (n = n_alloc);
	}

	/* Must do general case */
	n = 0;
	min_gap = 0.1 * GMT->current.setting.map_line_step;
	if ((n_alloc = lrint (ceil (fabs (lat2 - lat1) / GMT->current.map.dlat))) == 0) return (0);

	n_alloc++;	/* So n_alloc is at least 2 */
	dlat0 = (lat2 - lat1) / n_alloc;
	pos = (dlat0 > 0.0);

	k = n_alloc;	n_alloc = 0;
	GMT_malloc2 (GMT, tlon, tlat, k, &n_alloc, double);

	tlon[0] = lon;
	tlat[0] = lat1;
	GMT_geo_to_xy (GMT, tlon[0], tlat[0], &x0, &y0);
	while ((pos && (tlat[n] < lat2)) || (!pos && (tlat[n] > lat2))) {
		n++;
		if (n == n_alloc-1) {
			n_alloc += GMT_SMALL_CHUNK;
			tlon = GMT_memory (GMT, tlon, n_alloc, double);
			tlat = GMT_memory (GMT, tlat, n_alloc, double);
		}
		n_try = 0;
		keep_trying = true;
		dlat = dlat0;
		tlon[n] = lon;
		do {
			n_try++;
			tlat[n] = tlat[n-1] + dlat;
			if (GMT_is_geographic (GMT, GMT_IN) && fabs (tlat[n]) > 90.0) tlat[n] = copysign (90.0, tlat[n]);
			GMT_geo_to_xy (GMT, tlon[n], tlat[n], &x1, &y1);
			if ((*GMT->current.map.jump) (GMT, x0, y0, x1, y1) || (y0 < GMT->current.proj.rect[YLO] || y0 > GMT->current.proj.rect[YHI]))
				keep_trying = false;
			else {
				d = hypot (x1 - x0, y1 - y0);
				if (d > GMT->current.setting.map_line_step)
					dlat *= 0.5;
				else if (d < min_gap)
					dlat *= 2.0;
				else
					keep_trying = false;
			}
		} while (keep_trying && n_try < 10);
		x0 = x1;	y0 = y1;
	}
	tlon[n] = lon;
	tlat[n] = lat2;
	n++;

	if (n != n_alloc) {
		tlon = GMT_memory (GMT, tlon, n, double);
		tlat = GMT_memory (GMT, tlat, n, double);
	}

	*x = tlon;	*y = tlat;
	return (n);
}

uint64_t GMT_latpath (struct GMT_CTRL *GMT, double lat, double lon1, double lon2, double **x, double **y)
{
	size_t n_alloc = 0;
	uint64_t k, n;
	int n_try, pos;
	bool keep_trying;
	double dlon, dlon0, *tlon = NULL, *tlat = NULL, x0, x1, y0, y1, d, min_gap;

	if (GMT->current.map.parallel_straight == 2) {	/* Special non-sampling for gmtselect/grdlandmask */
		GMT_malloc2 (GMT, tlon, tlat, 2U, NULL, double);
		tlat[0] = tlat[1] = lat;
		tlon[0] = lon1;	tlon[1] = lon2;
		*x = tlon;	*y = tlat;
		return (2ULL);
	}
	if (GMT->current.map.parallel_straight) {	/* Easy, just a straight line connection via quarter points */
		GMT_malloc2 (GMT, tlon, tlat, 5U, NULL, double);
		tlat[0] = tlat[1] = tlat[2] = tlat[3] = tlat[4] = lat;
		dlon = lon2 - lon1;
		tlon[0] = lon1;	tlon[1] = lon1 + 0.25 * dlon;	tlon[2] = lon1 + 0.5 * dlon;
		tlon[3] = lon1 + 0.75 * dlon;	tlon[4] = lon2;
		*x = tlon;	*y = tlat;
		return (5ULL);
	}
	/* Here we try to walk along lat for small increment in longitude to make sure our steps are smaller than the line_step */
	min_gap = 0.1 * GMT->current.setting.map_line_step;
	if ((n_alloc = lrint (ceil (fabs (lon2 - lon1) / GMT->current.map.dlon))) == 0) return (0);	/* Initial guess to path length */

	n_alloc++;	/* n_alloc is at least 2 */
	dlon0 = (lon2 - lon1) / n_alloc;
	pos = (dlon0 > 0.0);

	k = n_alloc;	n_alloc = 0;
	GMT_malloc2 (GMT, tlon, tlat, k, &n_alloc, double);
	tlon[0] = lon1;	tlat[0] = lat;
	GMT_geo_to_xy (GMT, tlon[0], tlat[0], &x0, &y0);
	n = 0;
	while ((pos && (tlon[n] < lon2)) || (!pos && (tlon[n] > lon2))) {
		n++;
		if (n == n_alloc) GMT_malloc2 (GMT, tlon, tlat, n, &n_alloc, double);
		n_try = 0;
		keep_trying = true;
		dlon = dlon0;
		tlat[n] = lat;
		do {
			n_try++;
			tlon[n] = tlon[n-1] + dlon;
			GMT_geo_to_xy (GMT, tlon[n], tlat[n], &x1, &y1);
			if ((*GMT->current.map.jump) (GMT, x0, y0, x1, y1) || (y0 < GMT->current.proj.rect[YLO] || y0 > GMT->current.proj.rect[YHI]))
				keep_trying = false;
			else {
				d = hypot (x1 - x0, y1 - y0);
				if (d > GMT->current.setting.map_line_step)
					dlon *= 0.5;
				else if (d < min_gap)
					dlon *= 2.0;
				else
					keep_trying = false;
			}
		} while (keep_trying && n_try < 10);
		x0 = x1;	y0 = y1;
	}
	tlon[n] = lon2;
	tlat[n] = lat;
	n++;
	n_alloc = n;
	GMT_malloc2 (GMT, tlon, tlat, 0, &n_alloc, double);

	*x = tlon;	*y = tlat;
	return (n);
}

uint64_t GMT_geo_to_xy_line (struct GMT_CTRL *GMT, double *lon, double *lat, uint64_t n)
{
	/* Traces the lon/lat array and returns x,y plus appropriate pen moves
	 * Pen moves are caused by breakthroughs of the map boundary or when
	 * a point has lon = NaN or lat = NaN (this means "pick up pen") */
	uint64_t j, np;
 	bool inside;
	unsigned int sides[4];
	unsigned int nx;
	double xlon[4], xlat[4], xx[4], yy[4];
	double this_x, this_y, last_x, last_y, dummy[4];

	while (n > GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);

	np = 0;
	GMT_geo_to_xy (GMT, lon[0], lat[0], &last_x, &last_y);
	if (!GMT_map_outside (GMT, lon[0], lat[0])) {
		GMT->current.plot.x[0] = last_x;	GMT->current.plot.y[0] = last_y;
		GMT->current.plot.pen[np++] = PSL_MOVE;
	}
	for (j = 1; j < n; j++) {
		GMT_geo_to_xy (GMT, lon[j], lat[j], &this_x, &this_y);
		inside = !GMT_map_outside (GMT, lon[j], lat[j]);
		if (GMT_is_dnan (lon[j]) || GMT_is_dnan (lat[j])) continue;	/* Skip NaN point now */
		if (GMT_is_dnan (lon[j-1]) || GMT_is_dnan (lat[j-1])) {		/* Point after NaN needs a move */
			GMT->current.plot.x[np] = this_x;	GMT->current.plot.y[np] = this_y;
			GMT->current.plot.pen[np++] = PSL_MOVE;
			if (np == GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);
			last_x = this_x;	last_y = this_y;
			continue;
		}
		if ((nx = gmt_map_crossing (GMT, lon[j-1], lat[j-1], lon[j], lat[j], xlon, xlat, xx, yy, sides))) { /* Nothing */ }
		else if (GMT->current.map.is_world)
			nx = (*GMT->current.map.wrap_around_check) (GMT, dummy, last_x, last_y, this_x, this_y, xx, yy, sides);
		if (nx == 1) {	/* inside-outside or outside-inside */
			GMT->current.plot.x[np] = xx[0];	GMT->current.plot.y[np] = yy[0];
			GMT->current.plot.pen[np++] = (inside) ? PSL_MOVE : PSL_DRAW;
			if (np == GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);
		}
		else if (nx == 2) {	/* outside-inside-outside or (with wrapping) inside-outside-inside */
			GMT->current.plot.x[np] = xx[0];	GMT->current.plot.y[np] = yy[0];
			GMT->current.plot.pen[np++] = (inside) ? PSL_DRAW : PSL_MOVE;
			if (np == GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);
			GMT->current.plot.x[np] = xx[1];	GMT->current.plot.y[np] = yy[1];
			GMT->current.plot.pen[np++] = (inside) ? PSL_MOVE : PSL_DRAW;
			if (np == GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);
		}
		if (inside) {
			if ( np >= GMT->current.plot.n_alloc ) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "bad access: cannot access current.plot.x[%" PRIu64 "], np=%" PRIu64 ", GMT->current.plot.n=%" PRIu64 "\n", np, np, GMT->current.plot.n);
			}
			else {
				GMT->current.plot.x[np] = this_x;	GMT->current.plot.y[np] = this_y;
				GMT->current.plot.pen[np++] = PSL_DRAW;
			}
			if (np == GMT->current.plot.n_alloc) GMT_get_plot_array (GMT);
		}
		last_x = this_x;	last_y = this_y;
	}
	if (np) GMT->current.plot.pen[0] = PSL_MOVE;	/* Sanity override: Gotta start off with new start point */
	return (np);
}

uint64_t GMT_compact_line (struct GMT_CTRL *GMT, double *x, double *y, uint64_t n, int pen_flag, int *pen)
{	/* true if pen movements is present */
	/* GMT_compact_line will remove unnecessary points in paths, but does not reallocate to the shorter size */
	uint64_t i, j;
	double old_slope, new_slope, dx;
	char *flag = NULL;

	if (n < 3) return (n);	/* Nothing to do */
	flag = GMT_memory (GMT, NULL, n, char);

	dx = x[1] - x[0];
	old_slope = (doubleAlmostEqualZero (x[1], x[0])) ? copysign (HALF_DBL_MAX, y[1] - y[0]) : (y[1] - y[0]) / dx;

	for (i = 1; i < n-1; ++i) {
		dx = x[i+1] - x[i];
		new_slope = (doubleAlmostEqualZero (x[i+1], x[i])) ? copysign (HALF_DBL_MAX, y[i+1] - y[i]) : (y[i+1] - y[i]) / dx;
		if (doubleAlmostEqualZero (new_slope, old_slope) && !(pen_flag && (pen[i]+pen[i+1]) > 4))	/* 4 is 2+2 which is draw line; a 3 will produce > 4 */
			flag[i] = 1;
		else
			old_slope = new_slope;
	}

	for (i = j = 1; i < n; i++) {	/* i = 1 since first point must be included */
		if (flag[i] == 0) {
			x[j] = x[i];
			y[j] = y[i];
			if (pen_flag) pen[j] = pen[i];
			j++;
		}
	}
	GMT_free (GMT, flag);

	return (j);
}

/* Routines to transform grdfiles to/from map projections */

int GMT_project_init (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header, double *inc, unsigned int nx, unsigned int ny, unsigned int dpi, unsigned int offset)
{
	if (inc[GMT_X] > 0.0 && inc[GMT_Y] > 0.0) {
		if (GMT->current.io.inc_code[GMT_X] || GMT->current.io.inc_code[GMT_Y]) {	/* Must convert from distance units to degrees */
			GMT_memcpy (header->inc, inc, 2, double);	/* Set these temporarily as the grids incs */
			GMT_RI_prepare (GMT, header);			/* Converts the proper xinc/yinc units */
			GMT_memcpy (inc, header->inc, 2, double);	/* Restore the inc array for use below */
			GMT->current.io.inc_code[GMT_X] = GMT->current.io.inc_code[GMT_Y] = 0;
		}
		header->nx = GMT_get_n (GMT, header->wesn[XLO], header->wesn[XHI], inc[GMT_X], offset);
		header->ny = GMT_get_n (GMT, header->wesn[YLO], header->wesn[YHI], inc[GMT_Y], offset);
		header->inc[GMT_X] = GMT_get_inc (GMT, header->wesn[XLO], header->wesn[XHI], header->nx, offset);
		header->inc[GMT_Y] = GMT_get_inc (GMT, header->wesn[YLO], header->wesn[YHI], header->ny, offset);
	}
	else if (nx > 0 && ny > 0) {
		header->nx = nx;	header->ny = ny;
		header->inc[GMT_X] = GMT_get_inc (GMT, header->wesn[XLO], header->wesn[XHI], header->nx, offset);
		header->inc[GMT_Y] = GMT_get_inc (GMT, header->wesn[YLO], header->wesn[YHI], header->ny, offset);
	}
	else if (dpi > 0) {
		header->nx = urint ((header->wesn[XHI] - header->wesn[XLO]) * dpi) + 1 - offset;
		header->ny = urint ((header->wesn[YHI] - header->wesn[YLO]) * dpi) + 1 - offset;
		header->inc[GMT_X] = GMT_get_inc (GMT, header->wesn[XLO], header->wesn[XHI], header->nx, offset);
		header->inc[GMT_Y] = GMT_get_inc (GMT, header->wesn[YLO], header->wesn[YHI], header->ny, offset);
	}
	else {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "GMT_project_init: Necessary arguments not set\n");
		GMT_exit (GMT, EXIT_FAILURE); return EXIT_FAILURE;
	}
	header->registration = offset;

	GMT_RI_prepare (GMT, header);	/* Ensure -R -I consistency and set nx, ny */
	GMT_err_pass (GMT, GMT_grd_RI_verify (GMT, header, 1), "");
	GMT_grd_setpad (GMT, header, GMT->current.io.pad);			/* Assign default pad */
	GMT_set_grddim (GMT, header);	/* Set all dimensions before returning */

	GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Grid projection from size %dx%d to %dx%d\n", nx, ny, header->nx, header->ny);
	return (GMT_NOERROR);
}


int GMT_grd_project (struct GMT_CTRL *GMT, struct GMT_GRID *I, struct GMT_GRID *O, bool inverse)
{
	/* Generalized grid projection that deals with both interpolation and averaging effects.
	 * It requires the input grid to have 2 boundary rows/cols so that the bcr
	 * functions can be used.  The I struct represents the input grid which is either in original
	 * (i.e., lon/lat) coordinates or projected x/y (if inverse = true).
	 *
	 * I:	Grid and header with input grid on a padded grid with 2 extra rows/columns
	 * O:	Grid and header for output grid, no padding needed (but allowed)
	 * inverse:	true if input is x/y and we want to invert for a lon/lat grid
	 *
	 * In addition, these settings (via -n) control interpolation:
	 * antialias:	true if we need to do the antialiasing STEP 1 (below)
	 * interpolant:	0 = nearest neighbor, 1 = bilinear, 2 = B-spline, 3 = bicubic
	 * threshold:	minumum weight to be used. If weight < threshold interpolation yields NaN.
	 * We initialize the O->data array to NaN.
	 *
	 * Changed 10-Sep-07 to include the argument "antialias" and "threshold" and
	 * made "interpolant" an integer (was int bilinear).
	 */

	int col_in, row_in, col_out, row_out;
 	uint64_t ij_in, ij_out;
	short int *nz = NULL;
	double x_proj = 0.0, y_proj = 0.0, z_int, inv_nz;
	double *x_in = NULL, *x_out = NULL, *x_in_proj = NULL, *x_out_proj = NULL;
	double *y_in = NULL, *y_out = NULL, *y_in_proj = NULL, *y_out_proj = NULL;

	/* Only input grid MUST have at least 2 rows/cols padding */
	if (I->header->pad[XLO] < 2 || I->header->pad[XHI] < 2 || I->header->pad[YLO] < 2 || I->header->pad[YHI] < 2) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "GMT_grd_project: Input grid does not have sufficient (2) padding\n");
		GMT_exit (GMT, EXIT_FAILURE); return EXIT_FAILURE;
	}

	/* Precalculate grid coordinates */

	x_in  = GMT_grd_coord (GMT, I->header, GMT_X);
	y_in  = GMT_grd_coord (GMT, I->header, GMT_Y);
	x_out = GMT_grd_coord (GMT, O->header, GMT_X);
	y_out = GMT_grd_coord (GMT, O->header, GMT_Y);

	if (GMT_IS_RECT_GRATICULE (GMT)) {	/* Since lon/lat parallels x/y it pays to precalculate projected grid coordinates up front */
		x_in_proj  = GMT_memory (GMT, NULL, I->header->nx, double);
		y_in_proj  = GMT_memory (GMT, NULL, I->header->ny, double);
		x_out_proj = GMT_memory (GMT, NULL, O->header->nx, double);
		y_out_proj = GMT_memory (GMT, NULL, O->header->ny, double);
		if (inverse) {
			GMT_row_loop  (GMT, I, row_in)  GMT_xy_to_geo (GMT, &x_proj, &y_in_proj[row_in], I->header->wesn[XLO], y_in[row_in]);
			GMT_col_loop2 (GMT, I, col_in)  GMT_xy_to_geo (GMT, &x_in_proj[col_in], &y_proj, x_in[col_in], I->header->wesn[YLO]);
			GMT_row_loop  (GMT, O, row_out) GMT_geo_to_xy (GMT, I->header->wesn[YLO], y_out[row_out], &x_proj, &y_out_proj[row_out]);
			GMT_col_loop2 (GMT, O, col_out) GMT_geo_to_xy (GMT, x_out[col_out], I->header->wesn[YLO], &x_out_proj[col_out], &y_proj);
		}
		else {
			GMT_row_loop  (GMT, I, row_in) GMT_geo_to_xy (GMT, I->header->wesn[XLO], y_in[row_in], &x_proj, &y_in_proj[row_in]);
			GMT_col_loop2 (GMT, I, col_in) GMT_geo_to_xy (GMT, x_in[col_in], I->header->wesn[YLO], &x_in_proj[col_in], &y_proj);
			GMT_row_loop  (GMT, O, row_out) GMT_xy_to_geo (GMT, &x_proj, &y_out_proj[row_out], I->header->wesn[YLO], y_out[row_out]);
			GMT_col_loop2 (GMT, O, col_out) {	/* Here we must also align longitudes properly */
				GMT_xy_to_geo (GMT, &x_out_proj[col_out], &y_proj, x_out[col_out], I->header->wesn[YLO]);
				if (GMT_x_is_lon (GMT, GMT_IN) && !GMT_is_dnan (x_out_proj[col_out])) {
					while (x_out_proj[col_out] < I->header->wesn[XLO] - GMT_SMALL) x_out_proj[col_out] += 360.0;
					while (x_out_proj[col_out] > I->header->wesn[XHI] + GMT_SMALL) x_out_proj[col_out] -= 360.0;
				}
			}
		}
	}

	GMT_grd_loop (GMT, O, row_out, col_out, ij_out) O->data[ij_out] = GMT->session.f_NaN;	/* So that nodes outside will retain a NaN value */

	/* PART 1: Project input grid points and do a blockmean operation */

	O->header->z_min = FLT_MAX; O->header->z_max = -FLT_MAX;	/* Min/max for out */
	if (GMT->common.n.antialias) {	/* Blockaverage repeat pixels, at least the first ~32767 of them... */
		int nx = O->header->nx, ny = O->header->ny;
		nz = GMT_memory (GMT, NULL, O->header->size, short int);
		GMT_row_loop (GMT, I, row_in) {	/* Loop over the input grid row coordinates */
			if (GMT_IS_RECT_GRATICULE (GMT)) y_proj = y_in_proj[row_in];
			GMT_col_loop (GMT, I, row_in, col_in, ij_in) {	/* Loop over the input grid col coordinates */
				if (GMT_IS_RECT_GRATICULE (GMT))
					x_proj = x_in_proj[col_in];
				else if (inverse)
					GMT_xy_to_geo (GMT, &x_proj, &y_proj, x_in[col_in], y_in[row_in]);
				else {
					if (GMT->current.map.outside (GMT, x_in[col_in], y_in[row_in])) continue;	/* Quite possible we are beyond the horizon */
					GMT_geo_to_xy (GMT, x_in[col_in], y_in[row_in], &x_proj, &y_proj);
				}

				/* Here, (x_proj, y_proj) is the projected grid point.  Now find nearest node on the output grid */

				row_out = GMT_grd_y_to_row (GMT, y_proj, O->header);
				if (row_out < 0 || row_out >= ny) continue;	/* Outside our grid region */
				col_out = GMT_grd_x_to_col (GMT, x_proj, O->header);
				if (col_out < 0 || col_out >= nx) continue;	/* Outside our grid region */

				/* OK, this projected point falls inside the projected grid's rectangular domain */

				ij_out = GMT_IJP (O->header, row_out, col_out);	/* The output node */
				if (nz[ij_out] == 0) O->data[ij_out] = 0.0f;	/* First time, override the initial value */
				if (nz[ij_out] < SHRT_MAX) {			/* Avoid overflow */
					O->data[ij_out] += I->data[ij_in];	/* Add up the z-sum inside this rect... */
					nz[ij_out]++;				/* ..and how many points there were */
				}
				if (GMT_is_fnan (O->data[ij_out])) continue;
				if (O->data[ij_out] < O->header->z_min) O->header->z_min = O->data[ij_out];
				if (O->data[ij_out] > O->header->z_max) O->header->z_max = O->data[ij_out];
			}
		}
	}

	/* PART 2: Create weighted average of interpolated and observed points */

	GMT_row_loop (GMT, O, row_out) {	/* Loop over the output grid row coordinates */
		if (GMT_IS_RECT_GRATICULE (GMT)) y_proj = y_out_proj[row_out];
		GMT_col_loop (GMT, O, row_out, col_out, ij_out) {	/* Loop over the output grid col coordinates */
			if (GMT_IS_RECT_GRATICULE (GMT))
				x_proj = x_out_proj[col_out];
			else if (inverse)
				GMT_geo_to_xy (GMT, x_out[col_out], y_out[row_out], &x_proj, &y_proj);
			else {
				GMT_xy_to_geo (GMT, &x_proj, &y_proj, x_out[col_out], y_out[row_out]);
				if (GMT->current.proj.projection == GMT_GENPER && GMT->current.proj.g_outside) continue;	/* We are beyond the horizon */

				/* On 17-Sep-2007 the slack of GMT_SMALL was added to allow for round-off
				   errors in the grid limits. */
				if (GMT_x_is_lon (GMT, GMT_IN) && !GMT_is_dnan (x_proj)) {
					while (x_proj < I->header->wesn[XLO] - GMT_SMALL) x_proj += 360.0;
					while (x_proj > I->header->wesn[XHI] + GMT_SMALL) x_proj -= 360.0;
				}
			}

			/* Here, (x_proj, y_proj) is the inversely projected grid point.  Now find nearest node on the input grid */

			z_int = GMT_get_bcr_z (GMT, I, x_proj, y_proj);

			if (!GMT->common.n.antialias || nz[ij_out] < 2)	/* Just use the interpolated value */
				O->data[ij_out] = (float)z_int;
			else if (GMT_is_dnan (z_int))		/* Take the average of what we accumulated */
				O->data[ij_out] /= nz[ij_out];		/* Plain average */
			else {						/* Weighted average between blockmean'ed and interpolated values */
				inv_nz = 1.0 / nz[ij_out];
				O->data[ij_out] = (float) ((O->data[ij_out] + z_int * inv_nz) / (nz[ij_out] + inv_nz));
			}
			if (O->data[ij_out] < O->header->z_min) O->header->z_min = O->data[ij_out];
			if (O->data[ij_out] > O->header->z_max) O->header->z_max = O->data[ij_out];
		}
	}
	
	if (O->header->z_min < I->header->z_min || O->header->z_max > I->header->z_max) {	/* Truncate output to input extrama */
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "GMT_grd_project: Output grid extrema [%g/%g] exceed extrema of input grid [%g/%g]\n",
			O->header->z_min, O->header->z_max, I->header->z_min, I->header->z_max);
		if (GMT->common.n.truncate) {
			GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "GMT_grd_project: Output grid clipped to input grid extrema\n");
			GMT_grd_loop (GMT, O, row_out, col_out, ij_out) {
				if (O->data[ij_out] < I->header->z_min) O->data[ij_out] = (float)I->header->z_min;
				else if (O->data[ij_out] > I->header->z_max) O->data[ij_out] = (float)I->header->z_max;
			}
			O->header->z_min = I->header->z_min;	O->header->z_max = I->header->z_max;
		}
	}
	
	/* Time to clean up our mess */

	GMT_free (GMT, x_in);
	GMT_free (GMT, y_in);
	GMT_free (GMT, x_out);
	GMT_free (GMT, y_out);
	if (GMT_IS_RECT_GRATICULE(GMT)) {
		GMT_free (GMT, x_in_proj);
		GMT_free (GMT, y_in_proj);
		GMT_free (GMT, x_out_proj);
		GMT_free (GMT, y_out_proj);
	}
	if (GMT->common.n.antialias) GMT_free (GMT, nz);

	return (GMT_NOERROR);
}

int GMT_img_project (struct GMT_CTRL *GMT, struct GMT_IMAGE *I, struct GMT_IMAGE *O, bool inverse)
{
	/* Generalized image projection that deals with both interpolation and averaging effects.
	 * It requires the input image to have 2 boundary rows/cols so that the bcr
	 * functions can be used.  The I struct represents the input image which is either in original
	 * (i.e., lon/lat) coordinates or projected x/y (if inverse = true).
	 *
	 * I:	Image and header with input image on a padded image with 2 extra rows/columns
	 * O:	Image and header for output image, no padding needed (but allowed)
	 * edgeinfo:	Structure with information about boundary conditions on input image
	 * inverse:	true if input is x/y and we want to invert for a lon/lat image
	 *
	 * In addition, these settings (via -n) control interpolation:
	 * antialias:	true if we need to do the antialiasing STEP 1 (below)
	 * interpolant:	0 = nearest neighbor, 1 = bilinear, 2 = B-spline, 3 = bicubic
	 * threshold:	minumum weight to be used. If weight < threshold interpolation yields NaN.
	 *
	 * We initialize the O->data array to the NaN color.
	 *
	 * Changed 10-Sep-07 to include the argument "antialias" and "threshold" and
	 * made "interpolant" an integer (was int bilinear).
	 */

	int col_in, row_in, col_out, row_out, b, nb = I->header->n_bands;
 	uint64_t ij_in, ij_out;
	short int *nz = NULL;
	double x_proj = 0.0, y_proj = 0.0, inv_nz, rgb[4];
	double *x_in = NULL, *x_out = NULL, *x_in_proj = NULL, *x_out_proj = NULL;
	double *y_in = NULL, *y_out = NULL, *y_in_proj = NULL, *y_out_proj = NULL;
	unsigned char z_int[4], z_int_bg[4];

	/* Only input image MUST have at least 2 rows/cols padding */
	if (I->header->pad[XLO] < 2 || I->header->pad[XHI] < 2 || I->header->pad[YLO] < 2 || I->header->pad[YHI] < 2) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "GMT_img_project: Input image does not have sufficient (2) padding\n");
		GMT_exit (GMT, EXIT_FAILURE); return EXIT_FAILURE;
	}

	/* Precalculate grid coordinates */

	x_in  = GMT_grd_coord (GMT, I->header, GMT_X);
	y_in  = GMT_grd_coord (GMT, I->header, GMT_Y);
	x_out = GMT_grd_coord (GMT, O->header, GMT_X);
	y_out = GMT_grd_coord (GMT, O->header, GMT_Y);

	if (GMT_IS_RECT_GRATICULE (GMT)) {	/* Since lon/lat parallels x/y it pays to precalculate projected grid coordinates up front */
		x_in_proj  = GMT_memory (GMT, NULL, I->header->nx, double);
		y_in_proj  = GMT_memory (GMT, NULL, I->header->ny, double);
		x_out_proj = GMT_memory (GMT, NULL, O->header->nx, double);
		y_out_proj = GMT_memory (GMT, NULL, O->header->ny, double);
		if (inverse) {
			GMT_row_loop  (GMT, I, row_in)  GMT_xy_to_geo (GMT, &x_proj, &y_in_proj[row_in], I->header->wesn[XLO], y_in[row_in]);
			GMT_col_loop2 (GMT, I, col_in)  GMT_xy_to_geo (GMT, &x_in_proj[col_in], &y_proj, x_in[col_in], I->header->wesn[YLO]);
			GMT_row_loop  (GMT, O, row_out) GMT_geo_to_xy (GMT, I->header->wesn[YLO], y_out[row_out], &x_proj, &y_out_proj[row_out]);
			GMT_col_loop2 (GMT, O, col_out) GMT_geo_to_xy (GMT, x_out[col_out], I->header->wesn[YLO], &x_out_proj[col_out], &y_proj);
		}
		else {
			GMT_row_loop  (GMT, I, row_in) GMT_geo_to_xy (GMT, I->header->wesn[XLO], y_in[row_in], &x_proj, &y_in_proj[row_in]);
			GMT_col_loop2 (GMT, I, col_in) GMT_geo_to_xy (GMT, x_in[col_in], I->header->wesn[YLO], &x_in_proj[col_in], &y_proj);
			GMT_row_loop  (GMT, O, row_out) GMT_xy_to_geo (GMT, &x_proj, &y_out_proj[row_out], I->header->wesn[YLO], y_out[row_out]);
			GMT_col_loop2 (GMT, O, col_out) {	/* Here we must also align longitudes properly */
				GMT_xy_to_geo (GMT, &x_out_proj[col_out], &y_proj, x_out[col_out], I->header->wesn[YLO]);
				if (GMT_x_is_lon (GMT, GMT_IN) && !GMT_is_dnan (x_out_proj[col_out])) {
					while (x_out_proj[col_out] < I->header->wesn[XLO] - GMT_SMALL) x_out_proj[col_out] += 360.0;
					while (x_out_proj[col_out] > I->header->wesn[XHI] + GMT_SMALL) x_out_proj[col_out] -= 360.0;
				}
			}
		}
	}

	//GMT_grd_loop (GMT, O, row_out, col_out, ij_out) 		/* So that nodes outside will have the NaN color */
		//for (b = 0; b < nb; b++) O->data[nb*ij_out+b] = GMT_u255 (GMT->current.setting.color_patch[GMT_NAN][b]);
	for (b = 0; b < 4; b++) z_int_bg[b] = GMT_u255 (GMT->current.setting.color_patch[GMT_NAN][b]);

	/* PART 1: Project input image points and do a blockmean operation */

	if (GMT->common.n.antialias) {	/* Blockaverage repeat pixels, at least the first ~32767 of them... */
		int nx = O->header->nx, ny = O->header->ny;
		nz = GMT_memory (GMT, NULL, O->header->size, short int);
		GMT_row_loop (GMT, I, row_in) {	/* Loop over the input grid row coordinates */
			if (GMT_IS_RECT_GRATICULE (GMT)) y_proj = y_in_proj[row_in];
			GMT_col_loop (GMT, I, row_in, col_in, ij_in) {	/* Loop over the input grid col coordinates */
				if (GMT_IS_RECT_GRATICULE (GMT))
					x_proj = x_in_proj[col_in];
				else if (inverse)
					GMT_xy_to_geo (GMT, &x_proj, &y_proj, x_in[col_in], y_in[row_in]);
				else {
					if (GMT->current.map.outside (GMT, x_in[col_in], y_in[row_in])) continue;	/* Quite possible we are beyond the horizon */
					GMT_geo_to_xy (GMT, x_in[col_in], y_in[row_in], &x_proj, &y_proj);
				}

				/* Here, (x_proj, y_proj) is the projected grid point.  Now find nearest node on the output grid */

				row_out = (int)GMT_grd_y_to_row (GMT, y_proj, O->header);
				if (row_out < 0 || row_out >= ny) continue;	/* Outside our grid region */
				col_out = (int)GMT_grd_x_to_col (GMT, x_proj, O->header);
				if (col_out < 0 || col_out >= nx) continue;	/* Outside our grid region */

				/* OK, this projected point falls inside the projected grid's rectangular domain */

				ij_out = GMT_IJP (O->header, row_out, col_out);	/* The output node */
				if (nz[ij_out] == 0) for (b = 0; b < nb; b++) O->data[nb*ij_out+b] = 0;	/* First time, override the initial value */
				if (nz[ij_out] < SHRT_MAX) {	/* Avoid overflow */
					for (b = 0; b < nb; b++) {
						rgb[b] = ((double)nz[ij_out] * O->data[nb*ij_out+b] + I->data[nb*ij_in+b])/(nz[ij_out] + 1.0);	/* Update the mean pix values inside this rect... */
						O->data[nb*ij_out+b] = (unsigned char) lrint (GMT_0_255_truncate (rgb[b]));
					}
					nz[ij_out]++;		/* ..and how many points there were */
				}
			}
		}
	}

	/* PART 2: Create weighted average of interpolated and observed points */

	GMT_row_loop (GMT, O, row_out) {	/* Loop over the output grid row coordinates */
		if (GMT_IS_RECT_GRATICULE (GMT)) y_proj = y_out_proj[row_out];
		GMT_col_loop (GMT, O, row_out, col_out, ij_out) {	/* Loop over the output grid col coordinates */
			if (GMT_IS_RECT_GRATICULE (GMT))
				x_proj = x_out_proj[col_out];
			else if (inverse)
				GMT_geo_to_xy (GMT, x_out[col_out], y_out[row_out], &x_proj, &y_proj);
			else {
				GMT_xy_to_geo (GMT, &x_proj, &y_proj, x_out[col_out], y_out[row_out]);
				if (GMT->current.proj.projection == GMT_GENPER && GMT->current.proj.g_outside) continue;	/* We are beyond the horizon */

				/* On 17-Sep-2007 the slack of GMT_SMALL was added to allow for round-off
				   errors in the grid limits. */
				if (GMT_x_is_lon (GMT, GMT_IN) && !GMT_is_dnan (x_proj)) {
					while (x_proj < I->header->wesn[XLO] - GMT_SMALL) x_proj += 360.0;
					while (x_proj > I->header->wesn[XHI] + GMT_SMALL) x_proj -= 360.0;
				}
			}

			/* Here, (x_proj, y_proj) is the inversely projected grid point.  Now find nearest node on the input grid */

			if (GMT_get_bcr_img (GMT, I, x_proj, y_proj, z_int))		/* So that nodes outside will have the NaN color */
				for (b = 0; b < 4; b++) z_int[b] = z_int_bg[b];

			if (!GMT->common.n.antialias || nz[ij_out] < 2)	/* Just use the interpolated value */
				for (b = 0; b < nb; b++) O->data[nb*ij_out+b] = z_int[b];
			else {						/* Weighted average between blockmean'ed and interpolated values */
				inv_nz = 1.0 / nz[ij_out];
				for (b = 0; b < nb; b++) {
					rgb[b] = ((double)nz[ij_out] * O->data[nb*ij_out+b] + z_int[b] * inv_nz) / (nz[ij_out] + inv_nz);
					O->data[nb*ij_out+b] = (unsigned char) lrint (GMT_0_255_truncate (rgb[b]));
				}
			}
		}
	}

	/* Time to clean up our mess */

	GMT_free (GMT, x_in);
	GMT_free (GMT, y_in);
	GMT_free (GMT, x_out);
	GMT_free (GMT, y_out);
	if (GMT_IS_RECT_GRATICULE(GMT)) {
		GMT_free (GMT, x_in_proj);
		GMT_free (GMT, y_in_proj);
		GMT_free (GMT, x_out_proj);
		GMT_free (GMT, y_out_proj);
	}
	if (GMT->common.n.antialias) GMT_free (GMT, nz);

	return (GMT_NOERROR);
}

double GMT_azim_to_angle (struct GMT_CTRL *GMT, double lon, double lat, double c, double azim)
{	/* All variables in degrees */

	double lon1, lat1, x0, x1, y0, y1, dx, width, sinaz, cosaz, angle;

	if (GMT_IS_LINEAR (GMT)) {	/* Trivial case */
		angle = 90.0 - azim;
		if (GMT->current.proj.scale[GMT_X] != GMT->current.proj.scale[GMT_Y]) {	/* But allow for different x,y scaling */
			sincosd (angle, &sinaz, &cosaz);
			angle = d_atan2d (sinaz * GMT->current.proj.scale[GMT_Y], cosaz * GMT->current.proj.scale[GMT_X]);
		}
		return (angle);
	}

	/* Find second point c spherical degrees away in the azim direction */

	GMT_get_point_from_r_az (GMT, lon, lat, c, azim, &lon1, &lat1);

	/* Convert both points to x,y and get angle */

	GMT_geo_to_xy (GMT, lon, lat, &x0, &y0);
	GMT_geo_to_xy (GMT, lon1, lat1, &x1, &y1);

	/* Check for wrap-around */

	dx = x1 - x0;
	if (GMT_360_RANGE (GMT->common.R.wesn[XLO], GMT->common.R.wesn[XHI]) && fabs (dx) > (width = GMT_half_map_width (GMT, y0))) {
		width *= 2.0;
		dx = copysign (width - fabs (dx), -dx);
		if (x1 < width)
			x0 -= width;
		else
			x0 += width;
	}
	angle = d_atan2d (y1 - y0, x1 - x0);
	return (angle);
}

uint64_t GMT_map_clip_path (struct GMT_CTRL *GMT, double **x, double **y, bool *donut)
{
	/* This function returns a clip path corresponding to the
	 * extent of the map.
	 */

	uint64_t i, j, np;
	bool do_circle = false;
	double *work_x = NULL, *work_y = NULL, da, r0, s, c, lon, lat;

	*donut = false;

	if (GMT->common.R.oblique)	/* Rectangular map boundary */
		np = 4;
	else {
		switch (GMT->current.proj.projection) {
			case GMT_LINEAR:
			case GMT_MERCATOR:
			case GMT_CYL_EQ:
			case GMT_CYL_EQDIST:
			case GMT_CYL_STEREO:
			case GMT_MILLER:
			case GMT_OBLIQUE_MERC:
				np = 4;
				break;
			case GMT_POLAR:
				if (GMT->current.proj.got_elevations)
					*donut = (GMT->common.R.wesn[YHI] < 90.0 && GMT->current.map.is_world);
				else
					*donut = (GMT->common.R.wesn[YLO] > 0.0 && GMT->current.map.is_world);
				np = GMT->current.map.n_lon_nodes + 1;
				if (GMT->common.R.wesn[YLO] > 0.0)	/* Need inside circle segment */
					np *= 2;
				else if (!GMT->current.map.is_world)	/* Need to include origin */
					np++;
				break;
			case GMT_GENPER:
			case GMT_STEREO:
			case GMT_LAMBERT:
			case GMT_LAMB_AZ_EQ:
			case GMT_ORTHO:
			case GMT_GNOMONIC:
			case GMT_AZ_EQDIST:
			case GMT_ALBERS:
			case GMT_ECONIC:
			case GMT_VANGRINTEN:
				np = (GMT->current.proj.polar && (GMT->common.R.wesn[YLO] <= -90.0 || GMT->common.R.wesn[YHI] >= 90.0)) ? GMT->current.map.n_lon_nodes + 2: 2 * (GMT->current.map.n_lon_nodes + 1);
				break;
			case GMT_MOLLWEIDE:
			case GMT_SINUSOIDAL:
			case GMT_ROBINSON:
				np = 2 * GMT->current.map.n_lat_nodes + 2;
				break;
			case GMT_WINKEL:
			case GMT_HAMMER:
			case GMT_ECKERT4:
			case GMT_ECKERT6:
				np = 2 * GMT->current.map.n_lat_nodes + 2;
				if (GMT->common.R.wesn[YLO] != -90.0) np += GMT->current.map.n_lon_nodes - 1;
				if (GMT->common.R.wesn[YHI] != 90.0) np += GMT->current.map.n_lon_nodes - 1;
				break;
			case GMT_TM:
			case GMT_UTM:
			case GMT_CASSINI:
			case GMT_POLYCONIC:
				np = 2 * (GMT->current.map.n_lon_nodes + GMT->current.map.n_lat_nodes);
				break;
			default:
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Bad case in GMT_map_clip_path (%d)\n", GMT->current.proj.projection);
				np = 0;
				break;
		}
	}

	work_x = GMT_memory (GMT, NULL, np, double);
	work_y = GMT_memory (GMT, NULL, np, double);

	if (GMT->common.R.oblique) {
		work_x[0] = work_x[3] = GMT->current.proj.rect[XLO];	work_y[0] = work_y[1] = GMT->current.proj.rect[YLO];
		work_x[1] = work_x[2] = GMT->current.proj.rect[XHI];	work_y[2] = work_y[3] = GMT->current.proj.rect[YHI];
	}
	else {
		switch (GMT->current.proj.projection) {	/* Fill in clip path */
			case GMT_LINEAR:
			case GMT_MERCATOR:
			case GMT_CYL_EQ:
			case GMT_CYL_EQDIST:
			case GMT_CYL_STEREO:
			case GMT_MILLER:
			case GMT_OBLIQUE_MERC:
				work_x[0] = work_x[3] = GMT->current.proj.rect[XLO];	work_y[0] = work_y[1] = GMT->current.proj.rect[YLO];
				work_x[1] = work_x[2] = GMT->current.proj.rect[XHI];	work_y[2] = work_y[3] = GMT->current.proj.rect[YHI];
				break;
			case GMT_LAMBERT:
			case GMT_ALBERS:
			case GMT_ECONIC:
				for (i = j = 0; i <= GMT->current.map.n_lon_nodes; i++, j++) {
					lon = (i == GMT->current.map.n_lon_nodes) ? GMT->common.R.wesn[XHI] : GMT->common.R.wesn[XLO] + i * GMT->current.map.dlon;
					GMT_geo_to_xy (GMT, lon, GMT->common.R.wesn[YLO], &work_x[j], &work_y[j]);
				}
				for (i = 0; i <= GMT->current.map.n_lon_nodes; i++, j++) {
					lon = (i == GMT->current.map.n_lon_nodes) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI] - i * GMT->current.map.dlon;
					GMT_geo_to_xy (GMT, lon, GMT->common.R.wesn[YHI], &work_x[j], &work_y[j]);
				}
				break;
			case GMT_TM:
			case GMT_UTM:
			case GMT_CASSINI:
			case GMT_POLYCONIC:
				for (i = j = 0; i < GMT->current.map.n_lon_nodes; i++, j++)	/* South */
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO] + i * GMT->current.map.dlon, GMT->common.R.wesn[YLO], &work_x[j], &work_y[j]);
				for (i = 0; i < GMT->current.map.n_lat_nodes; i++, j++)	/* East */
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO] + i * GMT->current.map.dlat, &work_x[j], &work_y[j]);
				for (i = 0; i < GMT->current.map.n_lon_nodes; i++, j++)	/* North */
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XHI] - i * GMT->current.map.dlon, GMT->common.R.wesn[YHI], &work_x[j], &work_y[j]);
				for (i = 0; i < GMT->current.map.n_lat_nodes; i++, j++)	/* West */
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YHI] - i * GMT->current.map.dlat, &work_x[j], &work_y[j]);
				break;
			case GMT_POLAR:
				r0 = GMT->current.proj.r * GMT->common.R.wesn[YLO] / GMT->common.R.wesn[YHI];
				if (*donut) {
					np /= 2;
					da = TWO_PI / np;
					for (i = 0, j = 2 * np - 1; i < np; i++, j--) {	/* Draw outer clippath */
						sincos (i * da, &s, &c);
						work_x[i] = GMT->current.proj.r * (1.0 + c);
						work_y[i] = GMT->current.proj.r * (1.0 + s);
						/* Do inner clippath and put it at end of array */
						work_x[j] = GMT->current.proj.r + r0 * c;
						work_y[j] = GMT->current.proj.r + r0 * s;
					}
				}
				else {
					da = fabs (GMT->common.R.wesn[XHI] - GMT->common.R.wesn[XLO]) / GMT->current.map.n_lon_nodes;
					if (GMT->current.proj.got_elevations) {
						for (i = j = 0; i <= GMT->current.map.n_lon_nodes; i++, j++)	/* Draw outer clippath */
							GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO] + i * da, GMT->common.R.wesn[YLO], &work_x[j], &work_y[j]);
						for (i = GMT->current.map.n_lon_nodes + 1; GMT->common.R.wesn[YHI] < 90.0 && i > 0; i--, j++)	/* Draw inner clippath */
							GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO] + (i-1) * da, GMT->common.R.wesn[YHI], &work_x[j], &work_y[j]);
						if (doubleAlmostEqual (GMT->common.R.wesn[YHI], 90.0) && !GMT->current.map.is_world)	/* Add origin */
							GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YHI], &work_x[j], &work_y[j]);
					}
					else {
						for (i = j = 0; i <= GMT->current.map.n_lon_nodes; i++, j++)	/* Draw outer clippath */
							GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO] + i * da, GMT->common.R.wesn[YHI], &work_x[j], &work_y[j]);
						for (i = GMT->current.map.n_lon_nodes + 1; GMT->common.R.wesn[YLO] > 0.0 && i > 0; i--, j++)	/* Draw inner clippath */
							GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO] + (i-1) * da, GMT->common.R.wesn[YLO], &work_x[j], &work_y[j]);
						if (GMT_IS_ZERO (GMT->common.R.wesn[YLO]) && !GMT->current.map.is_world)	/* Add origin */
							GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO], &work_x[j], &work_y[j]);
					}
				}
				break;
			case GMT_GENPER:
				GMT_genper_map_clip_path (GMT, np, work_x, work_y);
				break;
			case GMT_VANGRINTEN:
				do_circle = GMT->current.map.is_world;
			case GMT_LAMB_AZ_EQ:
			case GMT_ORTHO:
			case GMT_GNOMONIC:
			case GMT_STEREO:
				if (GMT->current.proj.polar && !do_circle) {
					j = 0;
					if (GMT->common.R.wesn[YLO] > -90.0) {
						for (i = 0; i <= GMT->current.map.n_lon_nodes; i++, j++) {
							lon = (i == GMT->current.map.n_lon_nodes) ? GMT->common.R.wesn[XHI] : GMT->common.R.wesn[XLO] + i * GMT->current.map.dlon;
							GMT_geo_to_xy (GMT, lon, GMT->common.R.wesn[YLO], &work_x[j], &work_y[j]);
						}
					}
					else { /* Just add S pole */
						GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], -90.0, &work_x[j], &work_y[j]);
						j++;
					}
					if (GMT->common.R.wesn[YHI] < 90.0) {
						for (i = 0; i <= GMT->current.map.n_lon_nodes; i++, j++) {
							lon = (i == GMT->current.map.n_lon_nodes) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI] - i * GMT->current.map.dlon;
							GMT_geo_to_xy (GMT, lon, GMT->common.R.wesn[YHI], &work_x[j], &work_y[j]);
						}
					}
					else { /* Just add N pole */
						GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], 90.0, &work_x[j], &work_y[j]);
						j++;
					}
				}
				else {
					da = TWO_PI / np;
					for (i = 0; i < np; i++) {
						sincos (i * da, &s, &c);
						work_x[i] = GMT->current.proj.r * (1.0 + c);
						work_y[i] = GMT->current.proj.r * (1.0 + s);
					}
				}
				break;
			case GMT_AZ_EQDIST:
				da = TWO_PI / np;
				for (i = 0; i < np; i++) {
					sincos (i * da, &s, &c);
					work_x[i] = GMT->current.proj.r * (1.0 + c);
					work_y[i] = GMT->current.proj.r * (1.0 + s);
				}
				break;
			case GMT_MOLLWEIDE:
			case GMT_SINUSOIDAL:
			case GMT_ROBINSON:
				for (i = j = 0; i <= GMT->current.map.n_lat_nodes; i++, j++) {	/* Right */
					lat = (i == GMT->current.map.n_lat_nodes) ? GMT->common.R.wesn[YHI] : GMT->common.R.wesn[YLO] + i * GMT->current.map.dlat;
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XHI], lat, &work_x[j], &work_y[j]);
				}
				GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YHI], &work_x[j], &work_y[j]);	j++;
				for (i = GMT->current.map.n_lat_nodes; i > 0; j++, i--)	{	/* Left */
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO] + (i-1) * GMT->current.map.dlat, &work_x[j], &work_y[j]);
				}
				break;
			case GMT_HAMMER:
			case GMT_WINKEL:
			case GMT_ECKERT4:
			case GMT_ECKERT6:
				for (i = j = 0; i <= GMT->current.map.n_lat_nodes; i++, j++) {	/* Right */
					lat = (i == GMT->current.map.n_lat_nodes) ? GMT->common.R.wesn[YHI] : GMT->common.R.wesn[YLO] + i * GMT->current.map.dlat;
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XHI], lat, &work_x[j], &work_y[j]);
				}
				for (i = 1; GMT->common.R.wesn[YHI] != 90.0 && i < GMT->current.map.n_lon_nodes; i++, j++)
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XHI] - i * GMT->current.map.dlon, GMT->common.R.wesn[YHI], &work_x[j], &work_y[j]);
				GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YHI], &work_x[j], &work_y[j]);	j++;
				for (i = GMT->current.map.n_lat_nodes; i > 0; j++, i--)	{	/* Left */
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO] + (i-1)* GMT->current.map.dlat, &work_x[j], &work_y[j]);
				}
				for (i = 1; GMT->common.R.wesn[YLO] != -90.0 && i < GMT->current.map.n_lon_nodes; i++, j++)
					GMT_geo_to_xy (GMT, GMT->common.R.wesn[XLO] + i * GMT->current.map.dlon, GMT->common.R.wesn[YLO], &work_x[j], &work_y[j]);
				break;
		}
	}

	if (!(*donut)) np = GMT_compact_line (GMT, work_x, work_y, np, false, NULL);

	*x = work_x;
	*y = work_y;

	return (np);
}

double GMT_lat_swap_quick (struct GMT_CTRL *GMT, double lat, double c[])
{
	/* Return latitude, in degrees, given latitude, in degrees, based on coefficients c */

	double delta, cos2phi, sin2phi;

	/* First deal with trivial cases */

	if (lat >=  90.0) return ( 90.0);
	if (lat <= -90.0) return (-90.0);
	if (GMT_IS_ZERO (lat)) return (0.0);

	sincosd (2.0 * lat, &sin2phi, &cos2phi);

	delta = sin2phi * (c[0] + cos2phi * (c[1] + cos2phi * (c[2] + cos2phi * c[3])));

	return (lat + R2D * delta);
}

double GMT_lat_swap (struct GMT_CTRL *GMT, double lat, unsigned int itype)
{
	/* Return latitude, in degrees, given latitude, in degrees, based on itype */

	double delta, cos2phi, sin2phi;

	/* First deal with trivial cases */

	if (lat >=  90.0) return ( 90.0);
	if (lat <= -90.0) return (-90.0);
	if (GMT_IS_ZERO (lat)) return (0.0);

	if (GMT->current.proj.GMT_lat_swap_vals.spherical) return (lat);

	if (itype >= GMT_LATSWAP_N) {
		/* This should never happen -?- or do we want to allow the
			possibility of using itype = -1 to do nothing  */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "GMT_lat_swap(): Invalid choice, programming bug.\n");
		return(lat);
	}

	sincosd (2.0 * lat, &sin2phi, &cos2phi);

	delta = sin2phi * (GMT->current.proj.GMT_lat_swap_vals.c[itype][0]
		+ cos2phi * (GMT->current.proj.GMT_lat_swap_vals.c[itype][1]
		+ cos2phi * (GMT->current.proj.GMT_lat_swap_vals.c[itype][2]
		+ cos2phi * GMT->current.proj.GMT_lat_swap_vals.c[itype][3])));

	return (lat + R2D * delta);
}

void GMT_scale_eqrad (struct GMT_CTRL *GMT)
{
	/* Reinitialize GMT->current.proj.EQ_RAD to the appropriate value */

	switch (GMT->current.proj.projection) {

		/* Conformal projections */

		case GMT_MERCATOR:
		case GMT_TM:
		case GMT_UTM:
		case GMT_OBLIQUE_MERC:
		case GMT_LAMBERT:
		case GMT_STEREO:

			GMT->current.proj.EQ_RAD = GMT->current.proj.GMT_lat_swap_vals.rm;
			break;

		/* Equal Area projections */

		case GMT_LAMB_AZ_EQ:
		case GMT_ALBERS:
		case GMT_ECKERT4:
		case GMT_ECKERT6:
		case GMT_HAMMER:
		case GMT_MOLLWEIDE:
		case GMT_SINUSOIDAL:

			GMT->current.proj.EQ_RAD = GMT->current.proj.GMT_lat_swap_vals.ra;
			break;

		default:	/* Keep EQ_RAD as is */
			break;
	}

	/* Also reset dependencies of EQ_RAD */

	GMT->current.proj.i_EQ_RAD = 1.0 / GMT->current.proj.EQ_RAD;
	GMT->current.proj.M_PR_DEG = TWO_PI * GMT->current.proj.EQ_RAD / 360.0;
	GMT->current.proj.KM_PR_DEG = GMT->current.proj.M_PR_DEG / METERS_IN_A_KM;
}


void GMT_init_ellipsoid (struct GMT_CTRL *GMT)
{
	double f;

	/* Set up ellipsoid parameters for the selected ellipsoid since gmt.conf could have changed them */

	f = GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].flattening;
	GMT->current.proj.ECC2 = 2.0 * f - f * f;
	GMT->current.proj.ECC4 = GMT->current.proj.ECC2 * GMT->current.proj.ECC2;
	GMT->current.proj.ECC6 = GMT->current.proj.ECC2 * GMT->current.proj.ECC4;
	GMT->current.proj.one_m_ECC2 = 1.0 - GMT->current.proj.ECC2;
	GMT->current.proj.i_one_m_ECC2 = 1.0 / GMT->current.proj.one_m_ECC2;
	GMT->current.proj.ECC = d_sqrt (GMT->current.proj.ECC2);
	GMT->current.proj.half_ECC = 0.5 * GMT->current.proj.ECC;
	if (GMT->current.proj.ECC != 0) { /* avoid division by 0 */
		GMT->current.proj.i_half_ECC = 0.5 / GMT->current.proj.ECC;	/* Only used in inverse Alberts when e > 0 anyway */
	}
	GMT->current.proj.EQ_RAD = GMT->current.setting.ref_ellipsoid[GMT->current.setting.proj_ellipsoid].eq_radius;
	GMT->current.proj.i_EQ_RAD = 1.0 / GMT->current.proj.EQ_RAD;

	/* Spherical degrees to m or km */
	GMT->current.proj.mean_radius = gmt_mean_radius (GMT, GMT->current.proj.EQ_RAD, f);
	GMT->current.proj.M_PR_DEG = TWO_PI * GMT->current.proj.mean_radius / 360.0;
	GMT->current.proj.KM_PR_DEG = GMT->current.proj.M_PR_DEG / METERS_IN_A_KM;
	GMT->current.proj.DIST_M_PR_DEG = GMT->current.proj.M_PR_DEG;
	GMT->current.proj.DIST_KM_PR_DEG = GMT->current.proj.KM_PR_DEG;
	
	gmt_lat_swap_init (GMT);		/* Compute coefficients needed for auxilliary latitude conversions */
}

/* Datum conversion routines */

void GMT_datum_init (struct GMT_CTRL *GMT, struct GMT_DATUM *from, struct GMT_DATUM *to, bool heights)
{
	/* Initialize datum conv structures based on the parsed values*/

	unsigned int k;

	GMT->current.proj.datum.h_given = heights;

	GMT_memcpy (&GMT->current.proj.datum.from, from, 1, struct GMT_DATUM);
	GMT_memcpy (&GMT->current.proj.datum.to,   to,   1, struct GMT_DATUM);

	GMT->current.proj.datum.da = GMT->current.proj.datum.to.a - GMT->current.proj.datum.from.a;
	GMT->current.proj.datum.df = GMT->current.proj.datum.to.f - GMT->current.proj.datum.from.f;
	for (k = 0; k < 3; k++) GMT->current.proj.datum.dxyz[k] = -(GMT->current.proj.datum.to.xyz[k] - GMT->current.proj.datum.from.xyz[k]);	/* Since the X, Y, Z are Deltas relative to WGS-84 */
	GMT->current.proj.datum.one_minus_f = 1.0 - GMT->current.proj.datum.from.f;
}

void GMT_ECEF_init (struct GMT_CTRL *GMT, struct GMT_DATUM *D)
{
	/* Duplicate the parsed datum to the GMT from datum */

	GMT_memcpy (&GMT->current.proj.datum.from, D, 1, struct GMT_DATUM);
}

int GMT_set_datum (struct GMT_CTRL *GMT, char *text, struct GMT_DATUM *D)
{
	int i;
	double t;

	if (text[0] == '\0' || text[0] == '-') {	/* Shortcut for WGS-84 */
		GMT_memset (D->xyz, 3, double);
		D->a = GMT->current.setting.ref_ellipsoid[0].eq_radius;
		D->f = GMT->current.setting.ref_ellipsoid[0].flattening;
		D->ellipsoid_id = 0;
	}
	else if (strchr (text, ':')) {	/* Has colons, must get ellipsoid and dr separately */
		char ellipsoid[GMT_LEN256] = {""}, dr[GMT_LEN256] = {""};
		if (sscanf (text, "%[^:]:%s", ellipsoid, dr) != 2) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Malformed <ellipsoid>:<dr> argument!\n");
			return (-1);
		}
		if (sscanf (dr, "%lf,%lf,%lf", &D->xyz[GMT_X], &D->xyz[GMT_Y], &D->xyz[GMT_Z]) != 3) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Malformed <x>,<y>,<z> argument!\n");
			return (-1);
		}
		if ((i = GMT_get_ellipsoid (GMT, ellipsoid)) >= 0) {	/* This includes looking for format <a>,<1/f> */
			D->a = GMT->current.setting.ref_ellipsoid[i].eq_radius;
			D->f = GMT->current.setting.ref_ellipsoid[i].flattening;
			D->ellipsoid_id = i;
		}
		else {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Ellipsoid %s not recognized!\n", ellipsoid);
			return (-1);
		}
	}
	else {		/* Gave a Datum ID tag [ 0-(GMT_N_DATUMS-1)] */
		int k;
		if (sscanf (text, "%d", &i) != 1) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Malformed or unrecognized <datum> argument (%s)!\n", text);
			return (-1);
		}
		if (i < 0 || i >= GMT_N_DATUMS) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Datum ID (%d) outside valid range (0-%d)!\n", i, GMT_N_DATUMS-1);
			return (-1);
		}
		if ((k = GMT_get_ellipsoid (GMT, GMT->current.setting.proj_datum[i].ellipsoid)) < 0) {	/* This should not happen... */
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Ellipsoid %s not recognized!\n", GMT->current.setting.proj_datum[i].ellipsoid);
			return (-1);
		}
		D->a = GMT->current.setting.ref_ellipsoid[k].eq_radius;
		D->f = GMT->current.setting.ref_ellipsoid[k].flattening;
		D->ellipsoid_id = k;
		for (k = 0; k< 3; k++) D->xyz[k] = GMT->current.setting.proj_datum[i].xyz[k];
	}
	D->b = D->a * (1 - D->f);
	D->e_squared = 2 * D->f - D->f * D->f;
	t = D->a /D->b;
	D->ep_squared = t * t - 1.0;	/* (a^2 - b^2)/a^2 */
	return 0;
}

void GMT_conv_datum (struct GMT_CTRL *GMT, double in[], double out[])
{
	/* Evaluate J^-1 and B on from ellipsoid */
	/* Based on Standard Molodensky Datum Conversion, implemented from
	 * http://www.colorado.edu/geography/gcraft/notes/datum/gif/molodens.gif */

	double sin_lon, cos_lon, sin_lat, cos_lat, sin_lat2, M, N, h, tmp_1, tmp_2, tmp_3;
	double delta_lat, delta_lon, delta_h, sc_lat;

	h = (GMT->current.proj.datum.h_given) ? in[GMT_Z] : 0.0;
	sincosd (in[GMT_X], &sin_lon, &cos_lon);
	sincosd (in[GMT_Y], &sin_lat, &cos_lat);
	sin_lat2 = sin_lat * sin_lat;
	sc_lat = sin_lat * cos_lat;
	M = GMT->current.proj.datum.from.a * (1.0 - GMT->current.proj.datum.from.e_squared) / pow (1.0 - GMT->current.proj.datum.from.e_squared * sin_lat2, 1.5);
	N = GMT->current.proj.datum.from.a / sqrt (1.0 - GMT->current.proj.datum.from.e_squared * sin_lat2);

	tmp_1 = -GMT->current.proj.datum.dxyz[GMT_X] * sin_lat * cos_lon - GMT->current.proj.datum.dxyz[GMT_Y] * sin_lat * sin_lon + GMT->current.proj.datum.dxyz[GMT_Z] * cos_lat;
	tmp_2 = GMT->current.proj.datum.da * (N * GMT->current.proj.datum.from.e_squared * sc_lat) / GMT->current.proj.datum.from.a;
	tmp_3 = GMT->current.proj.datum.df * (M / GMT->current.proj.datum.one_minus_f + N * GMT->current.proj.datum.one_minus_f) * sc_lat;
	delta_lat = (tmp_1 + tmp_2 + tmp_3) / (M + h);

	delta_lon = (-GMT->current.proj.datum.dxyz[GMT_X] * sin_lon + GMT->current.proj.datum.dxyz[GMT_Y] * cos_lon) / ((N + h) * cos_lat);

	tmp_1 = GMT->current.proj.datum.dxyz[GMT_X] * cos_lat * cos_lon + GMT->current.proj.datum.dxyz[GMT_Y] * cos_lat * sin_lon + GMT->current.proj.datum.dxyz[GMT_Z] * sin_lat;
	tmp_2 = -GMT->current.proj.datum.da * GMT->current.proj.datum.from.a / N;
	tmp_3 = GMT->current.proj.datum.df * GMT->current.proj.datum.one_minus_f * N * sin_lat2;
	delta_h = tmp_1 + tmp_2 + tmp_3;

	out[GMT_X] = in[GMT_X] + delta_lon * R2D;
	out[GMT_Y] = in[GMT_Y] + delta_lat * R2D;
	if (GMT->current.proj.datum.h_given) out[GMT_Z] = in[GMT_Z] + delta_h;
}

void GMT_ECEF_forward (struct GMT_CTRL *GMT, double in[], double out[])
{
	/* Convert geodetic lon, lat, height to ECEF coordinates given the datum parameters.
	 * GMT->current.proj.datum.from is always the ellipsoid to use */

	double sin_lon, cos_lon, sin_lat, cos_lat, N, tmp;

	sincosd (in[GMT_X], &sin_lon, &cos_lon);
	sincosd (in[GMT_Y], &sin_lat, &cos_lat);

	N = GMT->current.proj.datum.from.a / d_sqrt (1.0 - GMT->current.proj.datum.from.e_squared * sin_lat * sin_lat);
	tmp = (N + in[GMT_Z]) * cos_lat;
	out[GMT_X] = tmp * cos_lon + GMT->current.proj.datum.from.xyz[GMT_X];
	out[GMT_Y] = tmp * sin_lon + GMT->current.proj.datum.from.xyz[GMT_Y];
	out[GMT_Z] = (N * (1 - GMT->current.proj.datum.from.e_squared) + in[GMT_Z]) * sin_lat + GMT->current.proj.datum.from.xyz[GMT_Z];
}

void GMT_ECEF_inverse (struct GMT_CTRL *GMT, double in[], double out[])
{
	/* Convert ECEF coordinates to geodetic lon, lat, height given the datum parameters.
	 * GMT->current.proj.datum.from is always the ellipsoid to use */

	unsigned int i;
	double in_p[3], sin_lat, cos_lat, N, p, theta, sin_theta, cos_theta;

	/* First remove the xyz shifts, us in_p to avoid changing in */

	for (i = 0; i < 3; i++) in_p[i] = in[i] - GMT->current.proj.datum.from.xyz[i];

	p = hypot (in_p[GMT_X], in_p[GMT_Y]);
	theta = atan (in_p[GMT_Z] * GMT->current.proj.datum.from.a / (p * GMT->current.proj.datum.from.b));
	sincos (theta, &sin_theta, &cos_theta);
	out[GMT_X] = d_atan2d (in_p[GMT_Y], in_p[GMT_X]);
	out[GMT_Y] = atand ((in_p[GMT_Z] + GMT->current.proj.datum.from.ep_squared * GMT->current.proj.datum.from.b * pow (sin_theta, 3.0)) / (p - GMT->current.proj.datum.from.e_squared * GMT->current.proj.datum.from.a * pow (cos_theta, 3.0)));
	sincosd (out[GMT_Y], &sin_lat, &cos_lat);
	N = GMT->current.proj.datum.from.a / sqrt (1.0 - GMT->current.proj.datum.from.e_squared * sin_lat * sin_lat);
	out[GMT_Z] = (p / cos_lat) - N;
}

double * GMT_dist_array (struct GMT_CTRL *GMT, double x[], double y[], uint64_t n, bool cumulative)
{	/* Returns distances in units set by GMT_distaz. It bypassed points where x and/or y are NaN.
	 * If cumulative is false we just return the increments; otherwise we add up distances */
	uint64_t this_p, prev;
	bool xy_not_NaN;
	double *d = NULL, cum_dist = 0.0, inc = 0.0;

	if (n == 0) return (NULL);
	d = GMT_memory (GMT, NULL, n, double);
	if (GMT_is_dnan (x[0]) || GMT_is_dnan (y[0])) d[0] = GMT->session.d_NaN;
	for (this_p = 1, prev = 0; this_p < n; this_p++) {
		xy_not_NaN = !(GMT_is_dnan (x[this_p]) || GMT_is_dnan (y[this_p]));
		if (xy_not_NaN) {	/* safe to calculate inc */
			inc = GMT_distance (GMT, x[this_p], y[this_p], x[prev], y[prev]);
			if (cumulative) {
				cum_dist += inc;
				d[this_p] = cum_dist;
			}
			else
				d[this_p] = inc;
		}
		else
			d[this_p] = GMT->session.d_NaN;

		if (xy_not_NaN) prev = this_p;	/* This was a record with OK x,y; make it the previous point for distance calculations */
	}
	return (d);
}

double * GMT_dist_array_2 (struct GMT_CTRL *GMT, double x[], double y[], uint64_t n, double scale, int dist_flag)
{	/* Returns distances in meter; use scale to get other units */
	uint64_t this_p, prev;
	bool cumulative = true, do_scale, xy_not_NaN;
	double *d = NULL, cum_dist = 0.0, inc = 0.0;

	if (dist_flag < 0) {	/* Want increments and not cumulative distances */
		dist_flag = abs (dist_flag);
		cumulative = false;
	}

	if (dist_flag < 0 || dist_flag > 3) return (NULL);

	do_scale = (scale != 1.0);
	d = GMT_memory (GMT, NULL, n, double);
	if (GMT_is_dnan (x[0]) || GMT_is_dnan (y[0])) d[0] = GMT->session.d_NaN;
	for (this_p = 1, prev = 0; this_p < n; this_p++) {
		xy_not_NaN = !(GMT_is_dnan (x[this_p]) || GMT_is_dnan (y[this_p]));
		if (xy_not_NaN) {	/* safe to calculate inc */
			switch (dist_flag) {

				case 0:	/* Cartesian distances */

					inc = hypot (x[this_p] - x[prev], y[this_p] - y[prev]);
					break;

				case 1:	/* Flat earth distances in meter */

					inc = gmt_flatearth_dist_meter (GMT, x[this_p], y[this_p], x[prev], y[prev]);
					break;

				case 2:	/* Great circle distances in meter */

					inc = GMT_great_circle_dist_meter (GMT, x[this_p], y[this_p], x[prev], y[prev]);
					break;

				case 3:	/* Geodesic distances in meter */

					inc = gmt_geodesic_dist_meter (GMT, x[this_p], y[this_p], x[prev], y[prev]);
					break;
			}

			if (do_scale) inc *= scale;
			if (cumulative) cum_dist += inc;
			d[this_p] = (cumulative) ? cum_dist : inc;
		}
		else
			d[this_p] = GMT->session.d_NaN;

		if (xy_not_NaN) prev = this_p;	/* This was a record with OK x,y; make it the previous point for distance calculations */
	}
	return (d);
}

unsigned int GMT_map_latcross (struct GMT_CTRL *GMT, double lat, double west, double east, struct GMT_XINGS **xings)
{
	bool go = false;
	unsigned int i, nx, nc = 0;
	size_t n_alloc = GMT_SMALL_CHUNK;
	double lon, lon_old, this_x, this_y, last_x, last_y, xlon[2], xlat[2], gap;
	struct GMT_XINGS *X = NULL;


	X = GMT_memory (GMT, NULL, n_alloc, struct GMT_XINGS);

	lon_old = west - 2.0 * GMT_SMALL;
	GMT_map_outside (GMT, lon_old, lat);
	GMT_geo_to_xy (GMT, lon_old, lat, &last_x, &last_y);
	for (i = 1; i <= GMT->current.map.n_lon_nodes; i++) {
		lon = (i == GMT->current.map.n_lon_nodes) ? east + 2.0 * GMT_SMALL : west + i * GMT->current.map.dlon;
		GMT_map_outside (GMT, lon, lat);
		GMT_geo_to_xy (GMT, lon, lat, &this_x, &this_y);
		if ((nx = gmt_map_crossing (GMT, lon_old, lat, lon, lat, xlon, xlat, X[nc].xx, X[nc].yy, X[nc].sides))) {
			if (nx == 1) X[nc].angle[0] = gmt_get_angle (GMT, lon_old, lat, lon, lat);
			if (nx == 2) X[nc].angle[1] = X[nc].angle[0] + 180.0;
			if (GMT->current.map.corner > 0) {
				X[nc].sides[0] = (GMT->current.map.corner%4 > 1) ? 1 : 3;
				if (GMT->current.proj.got_azimuths) X[nc].sides[0] = (X[nc].sides[0] + 2) % 4;
				GMT->current.map.corner = 0;
			}
		}
		else if (GMT->current.map.is_world)
			nx = (*GMT->current.map.wrap_around_check) (GMT, X[nc].angle, last_x, last_y, this_x, this_y, X[nc].xx, X[nc].yy, X[nc].sides);
		if (nx == 2 && (fabs (X[nc].xx[1] - X[nc].xx[0]) - GMT->current.map.width) < GMT_SMALL && !GMT->current.map.is_world)
			go = false;
		else if (nx == 2 && (gap = fabs (X[nc].yy[1] - X[nc].yy[0])) > GMT_SMALL && (gap - GMT->current.map.height) < GMT_SMALL && !GMT->current.map.is_world_tm)
			go = false;
		else if (nx > 0)
			go = true;
		if (go) {
			X[nc].nx = nx;
			nc++;
			if (nc == n_alloc) {
				n_alloc <<= 1;
				X = GMT_memory (GMT, X, n_alloc, struct GMT_XINGS);
			}
			go = false;
		}
		lon_old = lon;
		last_x = this_x;	last_y = this_y;
	}

	if (nc > 0) {
		X = GMT_memory (GMT, X, nc, struct GMT_XINGS);
		*xings = X;
	}
	else
		GMT_free (GMT, X);

	return (nc);
}

unsigned int GMT_map_loncross (struct GMT_CTRL *GMT, double lon, double south, double north, struct GMT_XINGS **xings)
{
	bool go = false;
	unsigned int j, nx, nc = 0;
	size_t n_alloc = GMT_SMALL_CHUNK;
	double lat, lat_old, this_x, this_y, last_x, last_y, xlon[2], xlat[2], gap;
	struct GMT_XINGS *X = NULL;

	X = GMT_memory (GMT, NULL, n_alloc, struct GMT_XINGS);

	lat_old = ((south - (2.0 * GMT_SMALL)) >= -90.0) ? south - 2.0 * GMT_SMALL : south;	/* Outside */
	if ((north + 2.0 * GMT_SMALL) <= 90.0) north += 2.0 * GMT_SMALL;
	GMT_map_outside (GMT, lon, lat_old);
	GMT_geo_to_xy (GMT, lon, lat_old, &last_x, &last_y);
	for (j = 1; j <= GMT->current.map.n_lat_nodes; j++) {
		lat = (j == GMT->current.map.n_lat_nodes) ? north: south + j * GMT->current.map.dlat;
		GMT_map_outside (GMT, lon, lat);
		GMT_geo_to_xy (GMT, lon, lat, &this_x, &this_y);
		if ((nx = gmt_map_crossing (GMT, lon, lat_old, lon, lat, xlon, xlat, X[nc].xx, X[nc].yy, X[nc].sides))) {
			if (nx == 1) X[nc].angle[0] = gmt_get_angle (GMT, lon, lat_old, lon, lat);
			if (nx == 2) X[nc].angle[1] = X[nc].angle[0] + 180.0;
			if (GMT->current.map.corner > 0) {
				X[nc].sides[0] = (GMT->current.map.corner < 3) ? 0 : 2;
				GMT->current.map.corner = 0;
			}
		}
		else if (GMT->current.map.is_world)
			nx = (*GMT->current.map.wrap_around_check) (GMT, X[nc].angle, last_x, last_y, this_x, this_y, X[nc].xx, X[nc].yy, X[nc].sides);
		if (nx == 2 && (fabs (X[nc].xx[1] - X[nc].xx[0]) - GMT->current.map.width) < GMT_SMALL && !GMT->current.map.is_world)
			go = false;
		else if (nx == 2 && (gap = fabs (X[nc].yy[1] - X[nc].yy[0])) > GMT_SMALL && (gap - GMT->current.map.height) < GMT_SMALL && !GMT->current.map.is_world_tm)
			go = false;
		else if (nx > 0)
			go = true;
		if (go) {
			X[nc].nx = nx;
			nc++;
			if (nc == n_alloc) {
				n_alloc <<= 1;
				X = GMT_memory (GMT, X, n_alloc, struct GMT_XINGS);
			}
			go = false;
		}
		lat_old = lat;
		last_x = this_x;	last_y = this_y;
	}

	if (nc > 0) {
		X = GMT_memory (GMT, X, nc, struct GMT_XINGS);
		*xings = X;
	}
	else
		GMT_free (GMT, X);

	return (nc);
}

int gmt_init_three_D (struct GMT_CTRL *GMT) {
	unsigned int i;
	bool easy, positive;
	double x, y, zmin = 0.0, zmax = 0.0, z_range;

	GMT->current.proj.three_D = (GMT->current.proj.z_project.view_azimuth != 180.0 || GMT->current.proj.z_project.view_elevation != 90.0);
	GMT->current.proj.scale[GMT_Z] = GMT->current.proj.z_pars[0];
	GMT->current.proj.xyz_pos[GMT_Z] = (GMT->current.proj.scale[GMT_Z] >= 0.0);	/* Increase z up or not */
	/* z_level == DBL_MAX is signaling that it was not set by the user. In that case we change it to the lower z level */
	if (GMT->current.proj.z_level == DBL_MAX) GMT->current.proj.z_level = (GMT->current.proj.xyz_pos[GMT_Z]) ?  GMT->common.R.wesn[ZLO] : GMT->common.R.wesn[ZHI];

	switch (GMT->current.proj.xyz_projection[GMT_Z]%3) {	/* Modulo 3 so that GMT_TIME (3) maps to GMT_LINEAR (0) */
		case GMT_LINEAR:	/* Regular scaling */
			zmin = (GMT->current.proj.xyz_pos[GMT_Z]) ? GMT->common.R.wesn[ZLO] : GMT->common.R.wesn[ZHI];
			zmax = (GMT->current.proj.xyz_pos[GMT_Z]) ? GMT->common.R.wesn[ZHI] : GMT->common.R.wesn[ZLO];
			GMT->current.proj.fwd_z = &GMT_translin;
			GMT->current.proj.inv_z = &GMT_itranslin;
			break;
		case GMT_LOG10:	/* Log10 transformation */
			if (GMT->common.R.wesn[ZLO] <= 0.0 || GMT->common.R.wesn[ZHI] <= 0.0) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Syntax error for -Jz -JZ option: limits must be positive for log10 projection\n");
				GMT_exit (GMT, EXIT_FAILURE); return EXIT_FAILURE;
			}
			zmin = (GMT->current.proj.xyz_pos[GMT_Z]) ? d_log10 (GMT, GMT->common.R.wesn[ZLO]) : d_log10 (GMT, GMT->common.R.wesn[ZHI]);
			zmax = (GMT->current.proj.xyz_pos[GMT_Z]) ? d_log10 (GMT, GMT->common.R.wesn[ZHI]) : d_log10 (GMT, GMT->common.R.wesn[ZLO]);
			GMT->current.proj.fwd_z = &GMT_translog10;
			GMT->current.proj.inv_z = &GMT_itranslog10;
			break;
		case GMT_POW:	/* x^y transformation */
			GMT->current.proj.xyz_pow[GMT_Z] = GMT->current.proj.z_pars[1];
			GMT->current.proj.xyz_ipow[GMT_Z] = 1.0 / GMT->current.proj.z_pars[1];
			positive = !((GMT->current.proj.xyz_pos[GMT_Z] + (GMT->current.proj.xyz_pow[GMT_Z] > 0.0)) % 2);
			zmin = (positive) ? pow (GMT->common.R.wesn[ZLO], GMT->current.proj.xyz_pow[GMT_Z]) : pow (GMT->common.R.wesn[ZHI], GMT->current.proj.xyz_pow[GMT_Z]);
			zmax = (positive) ? pow (GMT->common.R.wesn[ZHI], GMT->current.proj.xyz_pow[GMT_Z]) : pow (GMT->common.R.wesn[ZLO], GMT->current.proj.xyz_pow[GMT_Z]);
			GMT->current.proj.fwd_z = &GMT_transpowz;
			GMT->current.proj.inv_z = &GMT_itranspowz;
	}
	z_range = zmax - zmin;
	if (z_range == 0.0)
		GMT->current.proj.scale[GMT_Z] = 0.0;	/* No range given, just flat projected map */
	else if (GMT->current.proj.compute_scale[GMT_Z])
		GMT->current.proj.scale[GMT_Z] /= fabs (z_range);
	GMT->current.proj.zmax = z_range * GMT->current.proj.scale[GMT_Z];
	GMT->current.proj.origin[GMT_Z] = -zmin * GMT->current.proj.scale[GMT_Z];

	if (GMT->current.proj.z_project.view_azimuth >= 360.0) GMT->current.proj.z_project.view_azimuth -= 360.0;
	if (GMT->current.proj.z_project.view_azimuth < 0.0)    GMT->current.proj.z_project.view_azimuth += 360.0;
	GMT->current.proj.z_project.quadrant = urint (floor (GMT->current.proj.z_project.view_azimuth / 90.0)) + 1;
	sincosd (GMT->current.proj.z_project.view_azimuth, &GMT->current.proj.z_project.sin_az, &GMT->current.proj.z_project.cos_az);
	sincosd (GMT->current.proj.z_project.view_elevation, &GMT->current.proj.z_project.sin_el, &GMT->current.proj.z_project.cos_el);

	/* Determine min/max y of plot */

	/* easy = true means we can use 4 corner points to find min x and y, false
	   means we must search along wesn perimeter the hard way */

	switch (GMT->current.proj.projection) {
		case GMT_LINEAR:
		case GMT_MERCATOR:
		case GMT_OBLIQUE_MERC:
		case GMT_CYL_EQ:
		case GMT_CYL_EQDIST:
		case GMT_CYL_STEREO:
		case GMT_MILLER:
			easy = true;
			break;
		case GMT_POLAR:
		case GMT_LAMBERT:
		case GMT_TM:
		case GMT_UTM:
		case GMT_CASSINI:
		case GMT_STEREO:
		case GMT_ALBERS:
		case GMT_ECONIC:
		case GMT_POLYCONIC:
		case GMT_LAMB_AZ_EQ:
		case GMT_ORTHO:
		case GMT_GENPER:
		case GMT_GNOMONIC:
		case GMT_AZ_EQDIST:
		case GMT_SINUSOIDAL:
		case GMT_MOLLWEIDE:
		case GMT_HAMMER:
		case GMT_VANGRINTEN:
		case GMT_WINKEL:
		case GMT_ECKERT4:
		case GMT_ECKERT6:
		case GMT_ROBINSON:
			easy = GMT->common.R.oblique;
			break;
		default:
			easy = false;
			break;
	}

	if (!GMT->current.proj.three_D) easy = true;

	GMT->current.proj.z_project.xmin = GMT->current.proj.z_project.ymin = DBL_MAX;
	GMT->current.proj.z_project.xmax = GMT->current.proj.z_project.ymax = -DBL_MAX;

	if (easy) {
		double xx[4], yy[4];

		xx[0] = xx[3] = GMT->current.proj.rect[XLO]; xx[1] = xx[2] = GMT->current.proj.rect[XHI];
		yy[0] = yy[1] = GMT->current.proj.rect[YLO]; yy[2] = yy[3] = GMT->current.proj.rect[YHI];

		for (i = 0; i < 4; i++) {
			GMT_xy_to_geo (GMT, &GMT->current.proj.z_project.corner_x[i], &GMT->current.proj.z_project.corner_y[i], xx[i], yy[i]);
			GMT_xyz_to_xy (GMT, xx[i], yy[i], GMT_z_to_zz(GMT, GMT->common.R.wesn[ZLO]), &x, &y);
			GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
			GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
			GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
			GMT_xyz_to_xy (GMT, xx[i], yy[i], GMT_z_to_zz(GMT, GMT->common.R.wesn[ZHI]), &x, &y);
			GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
			GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
			GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
		}
	}
	else if (GMT->current.proj.r > 0.0) {	/* Do not think the next four lines mean anything in this case, just copied from the general case */
		GMT->current.proj.z_project.corner_x[0] = GMT->current.proj.z_project.corner_x[3] = (GMT->current.proj.xyz_pos[GMT_X]) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI];
		GMT->current.proj.z_project.corner_x[1] = GMT->current.proj.z_project.corner_x[2] = (GMT->current.proj.xyz_pos[GMT_X]) ? GMT->common.R.wesn[XHI] : GMT->common.R.wesn[XLO];
		GMT->current.proj.z_project.corner_y[0] = GMT->current.proj.z_project.corner_y[1] = (GMT->current.proj.xyz_pos[GMT_Y]) ? GMT->common.R.wesn[YLO] : GMT->common.R.wesn[YHI];
		GMT->current.proj.z_project.corner_y[2] = GMT->current.proj.z_project.corner_y[3] = (GMT->current.proj.xyz_pos[GMT_Y]) ? GMT->common.R.wesn[YHI] : GMT->common.R.wesn[YLO];
		for (i = 0; i < 360; i++) {	/* Go around the circle */
			sincosd (i * 1.0, &y, &x);
			GMT_xyz_to_xy (GMT, GMT->current.proj.r * (1.0 + x), GMT->current.proj.r * (1.0 + y), GMT_z_to_zz(GMT, GMT->common.R.wesn[ZLO]), &x, &y);
			GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
			GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
			GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
			GMT_xyz_to_xy (GMT, GMT->current.proj.r * (1.0 + x), GMT->current.proj.r * (1.0 + y), GMT_z_to_zz(GMT, GMT->common.R.wesn[ZHI]), &x, &y);
			GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
			GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
			GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
		}
	}
	else {
		GMT->current.proj.z_project.corner_x[0] = GMT->current.proj.z_project.corner_x[3] = (GMT->current.proj.xyz_pos[GMT_X]) ? GMT->common.R.wesn[XLO] : GMT->common.R.wesn[XHI];
		GMT->current.proj.z_project.corner_x[1] = GMT->current.proj.z_project.corner_x[2] = (GMT->current.proj.xyz_pos[GMT_X]) ? GMT->common.R.wesn[XHI] : GMT->common.R.wesn[XLO];
		GMT->current.proj.z_project.corner_y[0] = GMT->current.proj.z_project.corner_y[1] = (GMT->current.proj.xyz_pos[GMT_Y]) ? GMT->common.R.wesn[YLO] : GMT->common.R.wesn[YHI];
		GMT->current.proj.z_project.corner_y[2] = GMT->current.proj.z_project.corner_y[3] = (GMT->current.proj.xyz_pos[GMT_Y]) ? GMT->common.R.wesn[YHI] : GMT->common.R.wesn[YLO];
		for (i = 0; i < GMT->current.map.n_lon_nodes; i++) {	/* S and N */
			GMT_geoz_to_xy (GMT, GMT->common.R.wesn[XLO] + i * GMT->current.map.dlon, GMT->common.R.wesn[YLO], GMT->common.R.wesn[ZLO], &x, &y);
			GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
			GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
			GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
			GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			if (GMT->common.R.wesn[ZHI] != GMT->common.R.wesn[ZLO]) {
				GMT_geoz_to_xy (GMT, GMT->common.R.wesn[XLO] + i * GMT->current.map.dlon, GMT->common.R.wesn[YLO], GMT->common.R.wesn[ZHI], &x, &y);
				GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
				GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
				GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
				GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			}
			GMT_geoz_to_xy (GMT, GMT->common.R.wesn[XLO] + i * GMT->current.map.dlon, GMT->common.R.wesn[YHI], GMT->common.R.wesn[ZLO], &x, &y);
			GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
			GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
			GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
			GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			if (GMT->common.R.wesn[ZHI] != GMT->common.R.wesn[ZLO]) {
				GMT_geoz_to_xy (GMT, GMT->common.R.wesn[XLO] + i * GMT->current.map.dlon, GMT->common.R.wesn[YHI], GMT->common.R.wesn[ZHI], &x, &y);
				GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
				GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
				GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
				GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			}
		}
		for (i = 0; i < GMT->current.map.n_lat_nodes; i++) {	/* W and E */
			GMT_geoz_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO] + i * GMT->current.map.dlat, GMT->common.R.wesn[ZLO], &x, &y);
			GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
			GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
			GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
			GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			if (GMT->common.R.wesn[ZHI] != GMT->common.R.wesn[ZLO]) {
				GMT_geoz_to_xy (GMT, GMT->common.R.wesn[XLO], GMT->common.R.wesn[YLO] + i * GMT->current.map.dlat, GMT->common.R.wesn[ZHI], &x, &y);
				GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
				GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
				GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
				GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			}
			GMT_geoz_to_xy (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO] + i * GMT->current.map.dlat, GMT->common.R.wesn[ZLO], &x, &y);
			GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
			GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
			GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
			GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			if (GMT->common.R.wesn[ZHI] != GMT->common.R.wesn[ZLO]) {
				GMT_geoz_to_xy (GMT, GMT->common.R.wesn[XHI], GMT->common.R.wesn[YLO] + i * GMT->current.map.dlat, GMT->common.R.wesn[ZHI], &x, &y);
				GMT->current.proj.z_project.ymin = MIN (GMT->current.proj.z_project.ymin, y);
				GMT->current.proj.z_project.ymax = MAX (GMT->current.proj.z_project.ymax, y);
				GMT->current.proj.z_project.xmin = MIN (GMT->current.proj.z_project.xmin, x);
				GMT->current.proj.z_project.xmax = MAX (GMT->current.proj.z_project.xmax, x);
			}
		}
	}

	GMT->current.proj.z_project.face[0] = (GMT->current.proj.z_project.quadrant == 1 || GMT->current.proj.z_project.quadrant == 2) ? 0 : 1;
	GMT->current.proj.z_project.face[1] = (GMT->current.proj.z_project.quadrant == 1 || GMT->current.proj.z_project.quadrant == 4) ? 2 : 3;
	GMT->current.proj.z_project.face[2] = (GMT->current.proj.z_project.view_elevation >= 0.0) ? 4 : 5;
	GMT->current.proj.z_project.draw[0] = (GMT->current.proj.z_project.quadrant == 1 || GMT->current.proj.z_project.quadrant == 4) ? true : false;
	GMT->current.proj.z_project.draw[1] = (GMT->current.proj.z_project.quadrant == 3 || GMT->current.proj.z_project.quadrant == 4) ? true : false;
	GMT->current.proj.z_project.draw[2] = (GMT->current.proj.z_project.quadrant == 2 || GMT->current.proj.z_project.quadrant == 3) ? true : false;
	GMT->current.proj.z_project.draw[3] = (GMT->current.proj.z_project.quadrant == 1 || GMT->current.proj.z_project.quadrant == 2) ? true : false;
	GMT->current.proj.z_project.sign[0] = GMT->current.proj.z_project.sign[3] = -1.0;
	GMT->current.proj.z_project.sign[1] = GMT->current.proj.z_project.sign[2] = 1.0;
	GMT->current.proj.z_project.z_axis = (GMT->current.proj.z_project.quadrant%2) ? GMT->current.proj.z_project.quadrant : GMT->current.proj.z_project.quadrant - 2;

	if (GMT->current.proj.z_project.fixed) {
		if (!GMT->current.proj.z_project.world_given) {	/* Pick center point of region */
			GMT->current.proj.z_project.world_x = (GMT_is_geographic (GMT, GMT_IN)) ? GMT->current.proj.central_meridian : 0.5 * (GMT->common.R.wesn[XLO] + GMT->common.R.wesn[XHI]);
			GMT->current.proj.z_project.world_y = 0.5 * (GMT->common.R.wesn[YLO] + GMT->common.R.wesn[YHI]);
			GMT->current.proj.z_project.world_z = GMT->current.proj.z_level;
		}
		GMT_geoz_to_xy (GMT, GMT->current.proj.z_project.world_x, GMT->current.proj.z_project.world_y, GMT->current.proj.z_project.world_z, &x, &y);
		if (!GMT->current.proj.z_project.view_given) {	/* Pick center of current page */
			GMT->current.proj.z_project.view_x = 0.5 * GMT->current.setting.ps_page_size[0] * GMT->session.u2u[GMT_PT][GMT_INCH];
			GMT->current.proj.z_project.view_y = 0.5 * GMT->current.setting.ps_page_size[1] * GMT->session.u2u[GMT_PT][GMT_INCH];
		}
		GMT->current.proj.z_project.x_off = GMT->current.proj.z_project.view_x - x;
		GMT->current.proj.z_project.y_off = GMT->current.proj.z_project.view_y - y;
	}
	else {
		GMT->current.proj.z_project.x_off = -GMT->current.proj.z_project.xmin;
		GMT->current.proj.z_project.y_off = -GMT->current.proj.z_project.ymin;
	}

	/* Adjust the xmin/xmax and ymin/ymax because of xoff and yoff */
	GMT->current.proj.z_project.xmin += GMT->current.proj.z_project.x_off;
	GMT->current.proj.z_project.xmax += GMT->current.proj.z_project.x_off;
	GMT->current.proj.z_project.ymin += GMT->current.proj.z_project.y_off;
	GMT->current.proj.z_project.ymax += GMT->current.proj.z_project.y_off;

	return (GMT_NOERROR);
}

int GMT_map_setup (struct GMT_CTRL *GMT, double wesn[])
{
	unsigned int i;
	bool search, double_auto[6];

	if (!GMT->common.J.active) Return (GMT_MAP_NO_PROJECTION);
	if (wesn[XHI] == wesn[XLO] && wesn[YHI] == wesn[YLO]) Return (GMT_MAP_NO_REGION);	/* Since -R may not be involved if there are grids */

	GMT_init_ellipsoid (GMT);	/* Set parameters depending on the ellipsoid since the latter could have been set explicitly */

	if (GMT_x_is_lon (GMT, GMT_IN)) {
		/* Limit east-west range to 360 and make sure east > -180 and west < 360 */
		if (wesn[XHI] < wesn[XLO]) wesn[XHI] += 360.0;
		if ((fabs (wesn[XHI] - wesn[XLO]) - 360.0) > GMT_SMALL) Return (GMT_MAP_EXCEEDS_360);
		while (wesn[XHI] < -180.0) {
			wesn[XLO] += 360.0;
			wesn[XHI] += 360.0;
		}
		while (wesn[XLO] > 360.0) {
			wesn[XLO] -= 360.0;
			wesn[XHI] -= 360.0;
		}
	}
	if (GMT->current.proj.got_elevations) {
		if (wesn[YLO] < 0.0 || wesn[YLO] >= 90.0) Return (GMT_MAP_BAD_ELEVATION_MIN);
		if (wesn[YHI] <= 0.0 || wesn[YHI] > 90.0) Return (GMT_MAP_BAD_ELEVATION_MAX);
	}
	if (GMT_y_is_lat (GMT, GMT_IN)) {
		if (wesn[YLO] < -90.0 || wesn[YLO] > 90.0) Return (GMT_MAP_BAD_LAT_MIN);
		if (wesn[YHI] < -90.0 || wesn[YHI] > 90.0) Return (GMT_MAP_BAD_LAT_MAX);
	}

	if (GMT->common.R.wesn != wesn)		/* In many cases they are both copies of same pointer */
		GMT_memcpy (GMT->common.R.wesn, wesn, 4, double);
	GMT->current.proj.GMT_convert_latitudes = false;
	if (GMT->current.proj.gave_map_width) GMT->current.proj.units_pr_degree = false;

	GMT->current.map.n_lon_nodes = GMT->current.map.n_lat_nodes = 0;
	GMT->current.map.wrap_around_check = &gmt_wrap_around_check_x;
	GMT->current.map.jump = &gmt_map_jump_x;
	GMT->current.map.will_it_wrap = &gmt_will_it_wrap_x;
	//GMT->current.map.this_point_wraps = &gmt_this_point_wraps_x;
	GMT->current.map.get_crossings = &gmt_get_crossings_x;

	GMT->current.map.lon_wrap = true;

	switch (GMT->current.proj.projection) {

		case GMT_LINEAR:		/* Linear transformations */
			search = gmt_map_init_linear (GMT);
			break;

		case GMT_POLAR:		/* Both lon/lat are actually theta, radius */
			search = gmt_map_init_polar (GMT);
			break;

		case GMT_MERCATOR:		/* Standard Mercator projection */
			search = gmt_map_init_merc (GMT);
			break;

		case GMT_STEREO:		/* Stereographic projection */
			search = gmt_map_init_stereo (GMT);
			break;

		case GMT_LAMBERT:		/* Lambert Conformal Conic */
			search = gmt_map_init_lambert (GMT);
			break;

		case GMT_OBLIQUE_MERC:	/* Oblique Mercator */
			search = gmt_map_init_oblique (GMT);
			break;

		case GMT_TM:		/* Transverse Mercator */
			search = gmt_map_init_tm (GMT);
			break;

		case GMT_UTM:		/* Universal Transverse Mercator */
			search = gmt_map_init_utm (GMT);
			break;

		case GMT_LAMB_AZ_EQ:	/* Lambert Azimuthal Equal-Area */
			search = gmt_map_init_lambeq (GMT);
			break;

		case GMT_ORTHO:		/* Orthographic Projection */
			search = gmt_map_init_ortho (GMT);
			break;

		case GMT_GENPER:		/* General Perspective Projection */
			search = gmt_map_init_genper (GMT);
			break;

		case GMT_AZ_EQDIST:		/* Azimuthal Equal-Distance Projection */
			search = gmt_map_init_azeqdist (GMT);
			break;

		case GMT_GNOMONIC:		/* Azimuthal Gnomonic Projection */
			search = gmt_map_init_gnomonic (GMT);
			break;

		case GMT_MOLLWEIDE:		/* Mollweide Equal-Area */
			search = gmt_map_init_mollweide (GMT);
			break;

		case GMT_HAMMER:		/* Hammer-Aitoff Equal-Area */
			search = gmt_map_init_hammer (GMT);
			break;

		case GMT_VANGRINTEN:		/* Van der Grinten */
			search = gmt_map_init_grinten (GMT);
			break;

		case GMT_WINKEL:		/* Winkel Tripel */
			search = gmt_map_init_winkel (GMT);
			break;

		case GMT_ECKERT4:		/* Eckert IV */
			search = gmt_map_init_eckert4 (GMT);
			break;

		case GMT_ECKERT6:		/* Eckert VI */
			search = gmt_map_init_eckert6 (GMT);
			break;

		case GMT_CYL_EQ:		/* Cylindrical Equal-Area */
			search = gmt_map_init_cyleq (GMT);
			break;

		case GMT_CYL_STEREO:			/* Cylindrical Stereographic */
			search = gmt_map_init_cylstereo (GMT);
			break;

		case GMT_MILLER:		/* Miller Cylindrical */
			search = gmt_map_init_miller (GMT);
			break;

		case GMT_CYL_EQDIST:	/* Cylindrical Equidistant */
			search = gmt_map_init_cyleqdist (GMT);
			break;

		case GMT_ROBINSON:		/* Robinson */
			search = gmt_map_init_robinson (GMT);
			break;

		case GMT_SINUSOIDAL:	/* Sinusoidal Equal-Area */
			search = gmt_map_init_sinusoidal (GMT);
			break;

		case GMT_CASSINI:		/* Cassini cylindrical */
			search = gmt_map_init_cassini (GMT);
			break;

		case GMT_ALBERS:		/* Albers Equal-Area Conic */
			search = gmt_map_init_albers (GMT);
			break;

		case GMT_ECONIC:		/* Equidistant Conic */
			search = gmt_map_init_econic (GMT);
			break;

		case GMT_POLYCONIC:		/* Polyconic */
			search = gmt_map_init_polyconic (GMT);
			break;

		default:	/* No projection selected, return to a horrible death */
			Return (GMT_MAP_NO_PROJECTION);
	}

	/* If intervals are not set specifically, round them to some "nice" values
	 * Remember whether frame items in both directions were are automatically set */
	for (i = 0; i < 6; i++)
		double_auto[i] = GMT_is_geographic (GMT, GMT_IN) && !GMT->current.map.frame.slash &&
		GMT->current.map.frame.axis[GMT_X].item[i].active && GMT->current.map.frame.axis[GMT_X].item[i].interval == 0.0 &&
		GMT->current.map.frame.axis[GMT_Y].item[i].active && GMT->current.map.frame.axis[GMT_Y].item[i].interval == 0.0;
	
	GMT_auto_frame_interval (GMT, GMT_X, GMT_ANNOT_UPPER);
	GMT_auto_frame_interval (GMT, GMT_Y, GMT_ANNOT_UPPER);
	GMT_auto_frame_interval (GMT, GMT_Z, GMT_ANNOT_UPPER);
	GMT_auto_frame_interval (GMT, GMT_X, GMT_ANNOT_LOWER);
	GMT_auto_frame_interval (GMT, GMT_Y, GMT_ANNOT_LOWER);
	GMT_auto_frame_interval (GMT, GMT_Z, GMT_ANNOT_LOWER);

	/* Now set the pairs of automatically set intervals to be the same in both x- and y-direction */
	for (i = 0; i < 6; i++) {
		if (double_auto[i]) GMT->current.map.frame.axis[GMT_X].item[i].interval = GMT->current.map.frame.axis[GMT_Y].item[i].interval =
		MAX (GMT->current.map.frame.axis[GMT_X].item[i].interval, GMT->current.map.frame.axis[GMT_Y].item[i].interval);
	}

	GMT->current.proj.i_scale[GMT_X] = (GMT->current.proj.scale[GMT_X] != 0.0) ? 1.0 / GMT->current.proj.scale[GMT_X] : 1.0;
	GMT->current.proj.i_scale[GMT_Y] = (GMT->current.proj.scale[GMT_Y] != 0.0) ? 1.0 / GMT->current.proj.scale[GMT_Y] : 1.0;
	GMT->current.proj.i_scale[GMT_Z] = (GMT->current.proj.scale[GMT_Z] != 0.0) ? 1.0 / GMT->current.proj.scale[GMT_Z] : 1.0;

	GMT->current.map.width  = fabs (GMT->current.proj.rect[XHI] - GMT->current.proj.rect[XLO]);
	GMT->current.map.height = fabs (GMT->current.proj.rect[YHI] - GMT->current.proj.rect[YLO]);
	GMT->current.map.half_width  = 0.5 * GMT->current.map.width;
	GMT->current.map.half_height = 0.5 * GMT->current.map.height;

	if (!GMT->current.map.n_lon_nodes) GMT->current.map.n_lon_nodes = urint (GMT->current.map.width / GMT->current.setting.map_line_step);
	if (!GMT->current.map.n_lat_nodes) GMT->current.map.n_lat_nodes = urint (GMT->current.map.height / GMT->current.setting.map_line_step);

	GMT->current.map.dlon = (GMT->common.R.wesn[XHI] - GMT->common.R.wesn[XLO]) / GMT->current.map.n_lon_nodes;
	GMT->current.map.dlat = (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]) / GMT->current.map.n_lat_nodes;

	if (GMT->current.map.width > 400.0 && GMT_is_grdmapproject (GMT)) {	/* ***project calling with true scale, probably  */
		search = false;	/* Safe-guard that prevents region search below for (map|grd)project and others (400 inch = ~> 10 meters) */
		GMT_Report (GMT->parent, GMT_MSG_DEBUG, "Warning: GMT_map_setup perimeter search skipped when using true scale with grdproject or mapproject.\n");
	}

	if (search) {	/* Loop around rectangular perimeter and determine min/max lon/lat extent */
		gmt_wesn_search (GMT, GMT->current.proj.rect[XLO], GMT->current.proj.rect[XHI], GMT->current.proj.rect[YLO], GMT->current.proj.rect[YHI], &GMT->common.R.wesn[XLO], &GMT->common.R.wesn[XHI], &GMT->common.R.wesn[YLO], &GMT->common.R.wesn[YHI]);
		GMT->current.map.dlon = (GMT->common.R.wesn[XHI] - GMT->common.R.wesn[XLO]) / GMT->current.map.n_lon_nodes;
		GMT->current.map.dlat = (GMT->common.R.wesn[YHI] - GMT->common.R.wesn[YLO]) / GMT->current.map.n_lat_nodes;
		if (GMT_IS_AZIMUTHAL(GMT) && GMT->common.R.oblique) gmt_horizon_search (GMT, wesn[XLO], wesn[XHI], wesn[YLO], wesn[YHI], GMT->current.proj.rect[XLO], GMT->current.proj.rect[XHI], GMT->current.proj.rect[YLO], GMT->current.proj.rect[YHI]);
	}

	if (GMT->current.proj.central_meridian < GMT->common.R.wesn[XLO] && (GMT->current.proj.central_meridian + 360.0) <= GMT->common.R.wesn[XHI]) GMT->current.proj.central_meridian += 360.0;
	if (GMT->current.proj.central_meridian > GMT->common.R.wesn[XHI] && (GMT->current.proj.central_meridian - 360.0) >= GMT->common.R.wesn[XLO]) GMT->current.proj.central_meridian -= 360.0;

	/* Maximum step size (in degrees) used for interpolation of line segments along great circles (or meridians/parallels)  before they are plotted */
	GMT->current.map.path_step = GMT->current.setting.map_line_step / GMT->current.proj.scale[GMT_X] / GMT->current.proj.M_PR_DEG;

	gmt_init_three_D (GMT);

	return (GMT_NOERROR);
}

void gmt_set_distaz (struct GMT_CTRL *GMT, unsigned int mode, unsigned int type)
{	/* Assigns pointers to the chosen distance and azimuth functions */
	char *type_name[3] = {"Map", "Contour", "Contour annotation"};
	char *aux[6] = {"no", "authalic", "conformal", "meridional", "geocentric", "parametric"};
	char *rad[5] = {"mean (R_1)", "authalic (R_2)", "volumetric (R_3)", "meridional", "quadratic"};
	int choice = (GMT->current.setting.proj_aux_latitude == GMT_LATSWAP_NONE) ? 0 : 1 + GMT->current.setting.proj_aux_latitude/2;
	GMT->current.map.dist[type].scale = 1.0;	/* Default scale */

	switch (mode) {	/* Set pointers to distance functions */
		case GMT_CARTESIAN_DIST:	/* Cartesian 2-D x,y data */
			GMT->current.map.dist[type].func = &GMT_cartesian_dist;
			GMT->current.map.azimuth_func = &gmt_az_backaz_cartesian;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be Cartesian\n", type_name[type]);
			break;
		case GMT_CARTESIAN_DIST2:	/* Cartesian 2-D x,y data, use r^2 instead of hypot */
			GMT->current.map.dist[type].func = &GMT_cartesian_dist2;
			GMT->current.map.azimuth_func = &gmt_az_backaz_cartesian;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be Cartesian\n", type_name[type]);
			break;
		case GMT_CARTESIAN_DIST_PROJ:	/* Cartesian distance after projecting 2-D lon,lat data */
			GMT->current.map.dist[type].func = &GMT_cartesian_dist_proj;
			GMT->current.map.azimuth_func = &gmt_az_backaz_cartesian_proj;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be Cartesian after first projecting via -J\n", type_name[type]);
			break;
		case GMT_CARTESIAN_DIST_PROJ2:	/* Cartesian distance after projecting 2-D lon,lat data, use r^2 instead of hypot  */
			GMT->current.map.dist[type].func = &GMT_cartesian_dist_proj2;
			GMT->current.map.azimuth_func = &gmt_az_backaz_cartesian_proj;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be Cartesian after first projecting via -J\n", type_name[type]);
			break;
		case GMT_DIST_M+GMT_FLATEARTH:	/* 2-D lon, lat data, but scale to Cartesian flat earth in meter */
			GMT->current.map.dist[type].func = &gmt_flatearth_dist_meter;
			GMT->current.map.azimuth_func  = &gmt_az_backaz_flatearth;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be Flat Earth in meters\n", type_name[type]);
			break;
		case GMT_DIST_M+GMT_GREATCIRCLE:	/* 2-D lon, lat data, use spherical distances in meter */
			GMT->current.map.dist[type].func = &GMT_great_circle_dist_meter;
			GMT->current.map.azimuth_func = &gmt_az_backaz_sphere;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be using great circle approximation with %s auxiliary latitudes and %s radius = %.4f m.\n",
				type_name[type], aux[choice], rad[GMT->current.setting.proj_mean_radius], GMT->current.proj.mean_radius);
			break;
		case GMT_DIST_M+GMT_GEODESIC:	/* 2-D lon, lat data, use geodesic distances in meter */
			GMT->current.map.dist[type].func = &gmt_geodesic_dist_meter;
			GMT->current.map.azimuth_func = &gmt_az_backaz_geodesic;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be using %s geodesics in meters\n", type_name[type], GEOD_TEXT);
			break;
		case GMT_DIST_DEG+GMT_FLATEARTH:	/* 2-D lon, lat data, use Flat Earth distances in degrees */
			GMT->current.map.dist[type].func = gmt_flatearth_dist_degree;
			GMT->current.map.azimuth_func = &gmt_az_backaz_flatearth;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be Flat Earth in degrees\n", type_name[type]);
			break;
		case GMT_DIST_DEG+GMT_GREATCIRCLE:	/* 2-D lon, lat data, use spherical distances in degrees */
			GMT->current.map.dist[type].func = &GMT_great_circle_dist_degree;
			GMT->current.map.azimuth_func = &gmt_az_backaz_sphere;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be using great circle approximation with %s auxiliary latitudes and return lengths in degrees.\n",
				type_name[type], aux[choice]);
			break;
		case GMT_DIST_DEG+GMT_GEODESIC:	/* 2-D lon, lat data, use geodesic distances in degrees */
			GMT->current.map.dist[type].func = &gmt_geodesic_dist_degree;
			GMT->current.map.azimuth_func = &gmt_az_backaz_geodesic;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be using geodesics in degrees\n", type_name[type]);
			break;
		case GMT_DIST_COS+GMT_GREATCIRCLE:	/* 2-D lon, lat data, and Green's function needs cosine of spherical distance */
			GMT->current.map.dist[type].func = &GMT_great_circle_dist_cos;
			GMT->current.map.azimuth_func = &gmt_az_backaz_sphere;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be using great circle approximation with %s auxiliary latitudes and return cosine of spherical angles.\n",
				type_name[type], aux[choice]);
			break;
		case GMT_DIST_COS+GMT_GEODESIC:	/* 2-D lon, lat data, and Green's function needs cosine of geodesic distance */
			GMT->current.map.dist[type].func = &GMT_geodesic_dist_cos;
			GMT->current.map.azimuth_func = &gmt_az_backaz_geodesic;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be using cosine of geodesic angle\n", type_name[type]);
			break;
		case GMT_DIST_M+GMT_LOXODROME:	/* 2-D lon, lat data, but measure distance along rhumblines in meter */
			GMT->current.map.dist[type].func = &gmt_loxodrome_dist_meter;
			GMT->current.map.azimuth_func  = &gmt_az_backaz_loxodrome;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be along loxodromes in meters\n", type_name[type]);
			break;
		case GMT_DIST_DEG+GMT_LOXODROME:	/* 2-D lon, lat data, but measure distance along rhumblines in degrees */
			GMT->current.map.dist[type].func = &gmt_loxodrome_dist_degree;
			GMT->current.map.azimuth_func = &gmt_az_backaz_loxodrome;
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "%s distance calculation will be along loxodromes with %s auxiliary latitudes and return lengths in degrees.\n",
				type_name[type], aux[choice]);
			break;
		default:	/* Cannot happen unless we make a bug */
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Mode (=%d) for distance function is unknown. Must be bug.\n", mode);
			exit (EXIT_FAILURE);
			break;
	}
	if (type > 0) return;	/* Contour-related assignemnts end here */

	/* Mapping only */
	if (mode == GMT_CARTESIAN_DIST || mode == GMT_CARTESIAN_DIST2)	{	/* Cartesian data */
		GMT->current.map.near_lines_func   = &gmt_near_lines_cartesian;
		GMT->current.map.near_a_line_func  = &gmt_near_a_line_cartesian;
		GMT->current.map.near_point_func   = &gmt_near_a_point_cartesian;
	}
	else {	/* Geographic data */
		GMT->current.map.near_lines_func   = &gmt_near_lines_spherical;
		GMT->current.map.near_a_line_func  = &gmt_near_a_line_spherical;
		GMT->current.map.near_point_func   = &gmt_near_a_point_spherical;
	}
}

unsigned int GMT_init_distaz (struct GMT_CTRL *GMT, char unit, unsigned int mode, unsigned int type)
{
	/* Initializes distance calcuation given the selected values for:
	 * Distance unit: must be on of the following:
	 *  1) d|e|f|k|m|M|n|s
	 *  2) GMT (Cartesian distance after projecting with -J) | X (Cartesian)
	 *  3) S (cosine distance) | P (cosine after first inverse projecting with -J)
	 * distance-calculation modifier mode: 0 (Cartesian), 1 (flat Earth), 2 (great-circle, 3 (geodesic), 4 (loxodrome)
	 * type: 0 = map distances, 1 = contour distances, 2 = contour annotation distances
	 * We set distance and azimuth functions and scales for this type.
	 * At the moment there is only one azimuth function pointer for all.
	 *
	 * The input args for GMT_init_distaz normally comes from calling GMT_get_distance.
	 */

	unsigned int proj_type = GMT_GEOGRAPHIC;	/* Default is to just use the geographic coordinates as they are */

	if (strchr (GMT_LEN_UNITS, unit) && !GMT_is_geographic (GMT, GMT_IN)) {	/* Want geographic distance units but -fg (or -J) not set */
		GMT_parse_common_options (GMT, "f", 'f', "g");
		GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Your distance unit (%c) implies geographic data; -fg has been set.\n", unit);
	}

	switch (unit) {
			/* First the three arc angular distance units */
			
		case 'd':	/* Arc degrees on spherical body using desired metric mode */
			gmt_set_distaz (GMT, GMT_DIST_DEG + mode, type);
			GMT->current.map.dist[type].arc = true;	/* Angular measure */
			break;
		case 'm':	/* Arc minutes on spherical body using desired metric mode */
			gmt_set_distaz (GMT, GMT_DIST_DEG + mode, type);
			GMT->current.map.dist[type].scale = GMT_DEG2MIN_F;
			GMT->current.map.dist[type].arc = true;	/* Angular measure */
			break;
		case 's':	/* Arc seconds on spherical body using desired metric mode */
			gmt_set_distaz (GMT, GMT_DIST_DEG + mode, type);
			GMT->current.map.dist[type].scale = GMT_DEG2SEC_F;
			GMT->current.map.dist[type].arc = true;	/* Angular measure */
			break;
			
			/* Various distance units on the planetary body */
			
		case 'e':	/* Meters on spherical body using desired metric mode */
			gmt_set_distaz (GMT, GMT_DIST_M + mode, type);
			break;
		case 'f':	/* Feet on spherical body using desired metric mode */
			gmt_set_distaz (GMT, GMT_DIST_M + mode, type);
			GMT->current.map.dist[type].scale = 1.0 / METERS_IN_A_FOOT;
			break;
		case 'k':	/* Km on spherical body using desired metric mode */
			gmt_set_distaz (GMT, GMT_DIST_M + mode, type);
			GMT->current.map.dist[type].scale = 1.0 / METERS_IN_A_KM;
			break;
		case 'M':	/* Statute Miles on spherical body using desired metric mode  */
			gmt_set_distaz (GMT, GMT_DIST_M + mode, type);
			GMT->current.map.dist[type].scale = 1.0 / METERS_IN_A_MILE;
			break;
		case 'n':	/* Nautical miles on spherical body using desired metric mode */
			gmt_set_distaz (GMT, GMT_DIST_M + mode, type);
			GMT->current.map.dist[type].scale = 1.0 / METERS_IN_A_NAUTICAL_MILE;
			break;
		case 'u':	/* Survey feet on spherical body using desired metric mode */
			gmt_set_distaz (GMT, GMT_DIST_M + mode, type);
			GMT->current.map.dist[type].scale = 1.0 / METERS_IN_A_SURVEY_FOOT;
			break;
			
			/* Cartesian distances.  Note: The X|C|R|Z|S|P 'units' are only passed internally and are not available as user selections directly */
			
		case 'X':	/* Cartesian distances in user units */
			proj_type = GMT_CARTESIAN;
			gmt_set_distaz (GMT, GMT_CARTESIAN_DIST, type);
			break;
		case 'C':	/* Cartesian distances (in PROJ_LENGTH_UNIT) after first projecting input coordinates with -J */
			gmt_set_distaz (GMT, GMT_CARTESIAN_DIST_PROJ, type);
			proj_type = GMT_GEO2CART;
			break;
			
		case 'R':	/* Cartesian distances squared in user units */
			proj_type = GMT_CARTESIAN;
			gmt_set_distaz (GMT, GMT_CARTESIAN_DIST2, type);
			break;
		case 'Z':	/* Cartesian distances squared (in PROJ_LENGTH_UNIT^2) after first projecting input coordinates with -J */
			gmt_set_distaz (GMT, GMT_CARTESIAN_DIST_PROJ2, type);
			proj_type = GMT_GEO2CART;
			break;

			/* Specialized cosine distances used internally only (e.g., greenspline) */
			
		case 'S':	/* Spherical cosine distances (for various gridding functions) */
			gmt_set_distaz (GMT, GMT_DIST_COS + mode, type);
			break;
		case 'P':	/* Spherical distances after first inversily projecting Cartesian coordinates with -J */
			gmt_set_distaz (GMT, GMT_CARTESIAN_DIST_PROJ_INV, type);
			proj_type = GMT_CART2GEO;
			break;
			
		default:
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Syntax error: Distance units must be one of %s\n", GMT_LEN_UNITS_DISPLAY);
			GMT_exit (GMT, EXIT_FAILURE); return EXIT_FAILURE;
			break;
	}
	
	GMT->current.map.dist[type].init = true;	/* OK, we have now initialized the info for this type */
	return (proj_type);
}