/*--------------------------------------------------------------------
 *	$Id: libspotter.c,v 1.20 2004-01-13 02:04:36 pwessel Exp $
 *
 *   Copyright (c) 1999-2001 by P. Wessel
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Contact info: www.soest.hawaii.edu/wessel
 *--------------------------------------------------------------------*/
/*
 * SPOTTER: functions for moving points along small circles on a sphere.
 *
 * Paul Wessel, University of Hawaii
 * October 24, 2001
 * Version 1.1
 *
 * The user-callable functions in this library are:
 *
 * spotter_init			: Load stage poles from file
 * spotter_backtrack		: Trace track from seamount to hotspot
 * spotter_forthtrack		: Trace track from hotspot to seamount
 * spotter_finite_to_stages	: Convert finite rotations to stage poles
 * spotter_stages_to_finite	: Convert stage poles to finite rotations
 * spotter_add_rotations	: Add to plate motion models together.
 *
 * programs must first call spotter_init() which reads a file of
 * backward stage poles.  Given the right flag it can convert these
 * to forward stage poles.
 *
 * Then to draw a hotspot track the program can:
 *	1. Draw FROM the hotspot TO a seamount: Use spotter_forthtrack
 *	2. Draw FROM a seamount BACK TO a hotspot: Use spotter_backtrack
 *
 * To draw crustal flowlines (seamounts motion over mantle) do select
 * flowline = TRUE when calling spotter_init and then:
 *	1. Draw FROM a hotspot TO a seamount: Use spotter_backtrack
 *	2. Draw FROM a seamount TO a hotspot (and beyond): Use spotter_forthtrack
 */

#include "spotter.h"

/* Internal functions */

void matrix_to_pole (double T[3][3], double *plon, double *plat, double *w);
void matrix_transpose (double At[3][3], double A[3][3]);
void matrix_mult (double a[3][3], double b[3][3], double c[3][3]);
void make_rot_matrix (double lonp, double latp, double w, double R[3][3]);
void reverse_rotation_order (struct EULER *p, int n);
void xyw_to_struct_euler (struct EULER *p, double lon[], double lat[], double w[], int n, BOOLEAN stages, BOOLEAN convert);
void set_I_matrix (double R[3][3]);
BOOLEAN must_do_track (int sideA[], int sideB[]);
void set_inout_sides (double x, double y, double wesn[], int sideXY[2]);

void spotter_finite_to_fwstages (struct EULER p[], int n, BOOLEAN finite_rates, BOOLEAN stage_rates);

int spotter_init (char *file, struct EULER **p, int flowline, BOOLEAN finite_in, BOOLEAN finite_out, double *t_max, BOOLEAN verbose)
{
	/* file;	Name of file with backward stage poles */
	/* p;		Pointer to stage pole array */
	/* flowline;	TRUE if flowlines rather than hotspot-tracks are needed */
	/* finite_in;	TRUE for finite (total construction poles) files [alternative is stage poles] */
	/* finite_out;	TRUE if we want to return finite (total construction poles) [alternative is stage poles]*/
	/* t_max;	Extend earliest stage pole back to this age */
	FILE *fp;
	struct EULER *e;
	char  buffer[BUFSIZ];
	int n, nf, i = 0, n_alloc = GMT_SMALL_CHUNK;
	double last_t;

	e = (struct EULER *) GMT_memory (VNULL, n_alloc, sizeof (struct EULER), "libspotter");

	if ((fp = fopen (file, "r")) == NULL) {
		fprintf (stderr, "libspotter: ERROR: Cannot open stage pole file: %s\n", file);
		exit (EXIT_FAILURE);
	}

	if (flowline) finite_out = TRUE;	/* Override so we get finite poles for conversion to forward stage poles at the end */
	
	last_t = (finite_in) ? 0.0 : DBL_MAX;
	while (fgets (buffer, 512, fp) != NULL) { /* Expects lon lat t0 t1 ccw-angle */
		if (buffer[0] == '#' || buffer[0] == '\n') continue;

		nf = sscanf (buffer, "%lf %lf %lf %lf %lf", &e[i].lon, &e[i].lat, &e[i].t_start, &e[i].t_stop, &e[i].omega);

		if (finite_in && nf == 4) e[i].omega = e[i].t_stop, e[i].t_stop = 0.0;	/* Only got 4 columns */
		
		if (e[i].t_stop >= e[i].t_start) {
			fprintf (stderr, "libspotter: ERROR: Stage rotation %d has start time younger than stop time\n", i);
			exit (EXIT_FAILURE);
		}
		e[i].duration = e[i].t_start - e[i].t_stop;
		if (finite_in) {
			if (e[i].t_start < last_t) {
				fprintf (stderr, "libspotter: ERROR: Finite rotations must go from youngest to oldest\n");
				exit (EXIT_FAILURE);
			}
			last_t = e[i].t_start;
		}
		else {
			if (e[i].t_stop > last_t) {
				fprintf (stderr, "libspotter: ERROR: Stage rotations must go from oldest to youngest\n");
				exit (EXIT_FAILURE);
			}
			last_t = e[i].t_stop;
		}
		e[i].omega /= e[i].duration;	/* Convert to opening rate */
		
		e[i].omega_r = e[i].omega * D2R;
		sincos (e[i].lat * D2R, &e[i].sin_lat, &e[i].cos_lat);
		e[i].lon_r = e[i].lon * D2R;
		e[i].lat_r = e[i].lat * D2R;
		i++;
		if (i == n_alloc) {
			n_alloc += GMT_SMALL_CHUNK;
			e = (struct EULER *) GMT_memory ((void *)e, n_alloc, sizeof (struct EULER), "libspotter");
		}
	}
	fclose (fp);
	
	n = i;

	if (finite_in && !finite_out) spotter_finite_to_stages (e, n, TRUE, TRUE);	/* Convert finite poles to backward stage poles */
	if (!finite_in && finite_out) spotter_stages_to_finite (e, n, TRUE, TRUE);	/* Convert backward stage poles to finite poles */
	

	e = (struct EULER *) GMT_memory ((void *)e, n, sizeof (struct EULER), "libspotter");

	if (flowline) {	/* Get the forward stage poles from the total reconstruction poles */
		spotter_finite_to_fwstages (e, n, TRUE, TRUE);
	}

	/* Extend oldest stage pole back to t_max Ma */

	if ((*t_max) > 0.0 && e[0].t_start < (*t_max)) {
		if (verbose) fprintf (stderr, "libspotter: Extending oldest stage pole back to %g Ma\n", (*t_max));

		e[0].t_start = (*t_max);
		e[0].duration = e[0].t_start - e[0].t_stop;
	}
	else
		(*t_max) = e[0].t_start;
	*p = e;

	return (n);
}

/* spotter_backtrack: Given a seamount location and age, trace the
 *	hotspot-track between this seamount and a seamount of 
 *	age t_zero.  For t_zero = 0 this means the hotspot
 */

int spotter_backtrack (double xp[], double yp[], double tp[], int np, struct EULER p[], int ns, double d_km, double t_zero, BOOLEAN do_time, double wesn[], double **c)
/* xp, yp;	Points, in RADIANS */
/* tp;		Age of feature in m.y. */
/* np;		# of points */
/* p;		Stage poles */
/* ns;		# of stage poles */
/* d_km;	Create track point every d_km km.  If == -1.0, return bend points only */
/* t_zero;	Backtrack up to this age */
/* do_time;	TRUE if we want to interpolate and return time along track, 2 if we just want stage # */
/* wesn:	if do_time >= 10, only to track within the given box */
/* **c;		Pointer to return track vector */
{
	int i, j, k, kk = 0, start_k, nd = 1, nn, n_alloc = 2 * GMT_CHUNK, sideA[2], sideB[2];
	BOOLEAN path, bend, go, box_check;
	double t, tt, dt, d_lon, tlon, dd, i_km, xnew, xx, yy, *track, next_x, next_y;
	double s_lat, c_lat, s_lon, c_lon, cc, ss, cs, i_nd;

	bend = (d_km <= (SMALL - 1.0));
	path = (bend || d_km > SMALL);
	if (do_time >= 10) {	/* Restrict track sampling to given wesn box */
		do_time -= 10;
		box_check = TRUE;
	}
	else {
		box_check = FALSE;
		go = TRUE;
	}

	if (path) {
		track = (double *) GMT_memory (VNULL, n_alloc, sizeof (double), "libspotter");
		i_km = EQ_RAD / d_km;
	}

	if (p[ns-1].t_stop > t_zero) t_zero = p[ns-1].t_stop;	/* In case we don't go all the way to zero */
	
	for (i = 0; i < np; i++) {

		if (path) {
			start_k = kk++;
			if (kk == n_alloc) {
				n_alloc += BIG_CHUNK;
				track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
			}
		}
		nn = 0;
			
		t = tp[i];

		if (box_check) set_inout_sides (xp[i], yp[i], wesn, sideB);
		while (t > t_zero) {	/* As long as we're not back at zero age */
			if (box_check) sideA[0] = sideB[0], sideA[1] = sideB[1];

			j = 0;
			while (j < ns && t <= p[j].t_stop) j++;	/* Find first applicable stage pole */
			if (j == ns) {
				fprintf (stderr, "libspotter: (spotter_backtrack) Ran out of stage poles for t = %g\n", t);
				exit (EXIT_FAILURE);
			}
			dt = MIN (p[j].duration, t - MAX(p[j].t_stop, t_zero));
			d_lon = p[j].omega_r * dt;

			xnew = xp[i] - p[j].lon_r;
			sincos (yp[i], &s_lat, &c_lat);
			sincos (xnew, &s_lon, &c_lon);
			cc = c_lat * c_lon;
			tlon = d_atan2 (c_lat * s_lon, p[j].sin_lat * cc - p[j].cos_lat * s_lat);
			s_lat = p[j].sin_lat * s_lat + p[j].cos_lat * cc;
			c_lat = sqrt (1.0 - s_lat * s_lat);
			ss = p[j].sin_lat * s_lat;
			cs = p[j].cos_lat * s_lat;

			/* Get the next bend point first */

			xnew = tlon + d_lon;
			sincos (xnew, &s_lon, &c_lon);
			cc = c_lat * c_lon;
			next_y = d_asin (ss - p[j].cos_lat * cc);
			next_x = p[j].lon_r + d_atan2 (c_lat * s_lon, p[j].sin_lat * cc + cs);

			if (next_x < 0.0) next_x += TWO_PI;
			if (next_x >= TWO_PI) next_x -= TWO_PI;

			if (box_check) {	/* See if this segment _might_ involve the box in any way; if so do the track sampling */
				set_inout_sides (next_x, next_y, wesn, sideB);
				go = must_do_track (sideA, sideB);
			}
			if (path) {
				if (!bend) {
					nd = (int) ceil ((fabs (d_lon) * c_lat) * i_km);
					i_nd = 1.0 / nd;
					dd = d_lon * i_nd;
					tt = dt * i_nd;
				}
				track[kk++] = xp[i];
				if (kk == n_alloc) {
					n_alloc += BIG_CHUNK;
					track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
				}
				track[kk++] = yp[i];
				if (kk == n_alloc) {
					n_alloc += BIG_CHUNK;
					track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
				}
				if (do_time) {
					track[kk++] = (do_time == 2) ? (double)(ns - j) : t;
					if (kk == n_alloc) {
						n_alloc += BIG_CHUNK;
						track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
					}
				}
				if (!go) nd = 1;
				for (k = 1; go && k < nd; k++) {

					xnew = tlon + k * dd;
					sincos (xnew, &s_lon, &c_lon);
					cc = c_lat * c_lon;
					yy = d_asin (ss - p[j].cos_lat * cc);
					xx = p[j].lon_r + d_atan2 (c_lat * s_lon, p[j].sin_lat * cc + cs);

					if (xx < 0.0) xx += TWO_PI;
					if (xx >= TWO_PI) xx -= TWO_PI;
					track[kk++] = xx;
					if (kk == n_alloc) {
						n_alloc += BIG_CHUNK;
						track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
					}
					track[kk++] = yy;
					if (kk == n_alloc) {
						n_alloc += BIG_CHUNK;
						track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
					}
					if (do_time) {
						track[kk++] = (do_time == 2) ? (double)(ns - j) : t - k * tt;
						if (kk == n_alloc) {
							n_alloc += BIG_CHUNK;
							track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
						}
					}
				}
				nn += nd;
			}
			xp[i] = next_x;	yp[i] = next_y;
			t -= dt;
		}
		if (path) {
			track[kk++] = xp[i];
			if (kk == n_alloc) {
				n_alloc += BIG_CHUNK;
				track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
			}
			track[kk++] = yp[i];
			if (kk == n_alloc) {
				n_alloc += BIG_CHUNK;
				track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
			}
			if (do_time) {
				track[kk++] = (do_time == 2) ? (double)(ns - j) : t;
				if (kk == n_alloc) {
					n_alloc += BIG_CHUNK;
					track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
				}
			}
			track[start_k] = nn+1;
		}
	}
	if (path) {
		track = (double *) GMT_memory ((void *)track, (size_t)kk, sizeof (double), "libspotter");
		*c = track;
		return (kk);
	}

	return (np);
}

void set_inout_sides (double x, double y, double wesn[], int sideXY[2]) {
	/* Given the rectangular region in wesn, return -1, 0, +1 for
	 * x and y if the point is left/below (-1) in (0), or right/above (+1).
	 * 
	 */
	 
	if (y < wesn[2])
		sideXY[1] = -1;
	else if (y > wesn[3])
		sideXY[1] = +1;
	else
		sideXY[1] = 0;
	while ((x + TWO_PI) < wesn[1]) x += TWO_PI;
	while ((x - TWO_PI) > wesn[0]) x -= TWO_PI;
	if (x < wesn[0])
		sideXY[0] = -1;
	else if (x > wesn[1])
		sideXY[0] = +1;
	else
		sideXY[0] = 0;
}	

BOOLEAN must_do_track (int sideA[], int sideB[]) {
	int dx, dy;
	/* First check if any of the two points are inside the box */
	if (sideA[0] == 0 && sideA[1] == 0) return (TRUE);
	if (sideB[0] == 0 && sideB[1] == 0) return (TRUE);
	/* Now check if the two points may cut a corner */
	dx = abs (sideA[0] - sideB[0]);
	dy = abs (sideA[1] - sideB[1]);
	if (dx && dy) return (TRUE);
	if (dx == 2 || dy == 2) return (TRUE);	/* COuld cut across the box */
	return (FALSE);
}

/* spotter_forthtrack: Given a hotspot location and final age, trace the
 *	hotspot-track between the seamount created at t_zero and a
 *	seamount of age tp.  For t_zero = 0 this means from the hotspot.
 */

int spotter_forthtrack (double xp[], double yp[], double tp[], int np, struct EULER p[], int ns, double d_km, double t_zero, BOOLEAN do_time, double wesn[], double **c)
/* xp, yp;	Points, in RADIANS */
/* tp;		Age of feature in m.y. */
/* np;		# of points */
/* p;		Stage poles */
/* ns;		# of stage poles */
/* d_km;	Create track point every d_km km.  If == -1.0, return bend points only */
/* t_zero;	Foretrack from this age forward */
/* do_time;	TRUE if we want to interpolate and return time along track */
/* wesn:	if do_time >= 10, only to track within the given box */
/* c;		Pointer to return track vector */
{
	int i, j, k, kk = 0, start_k, nd = 1, nn, n_alloc = 2 * GMT_CHUNK, sideA[2], sideB[2];
	BOOLEAN path, bend, go, box_check;
	double t, tt, dt, d_lon, tlon, dd, i_km, xnew, xx, yy, *track;
	double s_lat, c_lat, s_lon, c_lon, cc, ss, cs, i_nd, next_x, next_y;

	bend = (d_km <= (SMALL - 1.0));
	path = (bend || d_km > SMALL);
	if (do_time >= 10) {	/* Restrict track sampling to given wesn box */
		do_time -= 10;
		box_check = TRUE;
	}
	else {
		box_check = FALSE;
		go = TRUE;
	}

	if (path) {
		track = (double *) GMT_memory (VNULL, n_alloc, sizeof (double), "libspotter");
		i_km = EQ_RAD / d_km;
	}

	if (p[ns-1].t_stop > t_zero) t_zero = p[ns-1].t_stop;	/* In case we don't go all the way to zero */

	for (i = 0; i < np; i++) {

		if (path) {
			start_k = kk++;
			if (kk == n_alloc) {
				n_alloc += BIG_CHUNK;
				track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
			}
		}
		nn = 0;

		t = t_zero;
		
		if (box_check) set_inout_sides (xp[i], yp[i], wesn, sideB);
		while (t < tp[i]) {	/* As long as we're not back at zero age */
			if (box_check) sideA[0] = sideB[0], sideA[1] = sideB[1];
			j = ns - 1;
			while (j && (t + GMT_CONV_LIMIT) > p[j].t_start) j--;
			/* while (j < ns && (t + GMT_CONV_LIMIT) < p[j].t_stop) j++; */	/* Find first applicable stage pole */
			if (j == ns) {
				fprintf (stderr, "libspotter: (spotter_forthtrack) Ran out of stage poles for t = %g\n", t);
				exit (EXIT_FAILURE);
			}
			dt = MIN (tp[i], p[j].t_start) - t;	/* Time interval to rotate */
			d_lon = p[j].omega_r * dt;		/* Rotation angle (radians) */

			xnew = xp[i] - p[j].lon_r;
			sincos (yp[i], &s_lat, &c_lat);
			sincos (xnew, &s_lon, &c_lon);
			cc = c_lat * c_lon;
			tlon = d_atan2 (c_lat * s_lon, p[j].sin_lat * cc - p[j].cos_lat * s_lat);
			s_lat = p[j].sin_lat * s_lat + p[j].cos_lat * cc;
			c_lat = sqrt (1.0 - s_lat * s_lat);
			ss = p[j].sin_lat * s_lat;
			cs = p[j].cos_lat * s_lat;

			/* Get the next bend point first */

			xnew = tlon - d_lon;
			sincos (xnew, &s_lon, &c_lon);
			cc = c_lat * c_lon;
			next_y = d_asin (ss - p[j].cos_lat * cc);
			next_x = p[j].lon_r + d_atan2 (c_lat * s_lon, p[j].sin_lat * cc + cs);

			if (next_x < 0.0) next_x += TWO_PI;
			if (next_x >= TWO_PI) next_x -= TWO_PI;

			if (box_check) {	/* See if this segment _might_ involve the box in any way; if so do the track sampling */
				set_inout_sides (next_x, next_y, wesn, sideB);
				go = must_do_track (sideA, sideB);
			}
			if (path) {
				if (!bend) {
					nd = (int) ceil ((fabs (d_lon) * c_lat) * i_km);
					i_nd = 1.0 / nd;
					dd = d_lon * i_nd;
					tt = dt * i_nd;
				}
				track[kk++] = xp[i];
				if (kk == n_alloc) {
					n_alloc += BIG_CHUNK;
					track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
				}
				track[kk++] = yp[i];
				if (kk == n_alloc) {
					n_alloc += BIG_CHUNK;
					track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
				}
				if (do_time) {
					track[kk++] = (do_time == 2) ? (double)(ns - j) : t;
					if (kk == n_alloc) {
						n_alloc += BIG_CHUNK;
						track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
					}
				}
				if (!go) nd = 1;
				for (k = 1; go && k < nd; k++) {
					xnew = tlon - k * dd;
					sincos (xnew, &s_lon, &c_lon);
					cc = c_lat * c_lon;
					yy = d_asin (ss - p[j].cos_lat * cc);
					xx = p[j].lon_r + d_atan2 (c_lat * s_lon, p[j].sin_lat * cc + cs);

					if (xx < 0.0) xx += TWO_PI;
					if (xx >= TWO_PI) xx -= TWO_PI;
					track[kk++] = xx;
					if (kk == n_alloc) {
						n_alloc += BIG_CHUNK;
						track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
					}
					track[kk++] = yy;
					if (kk == n_alloc) {
						n_alloc += BIG_CHUNK;
						track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
					}
					if (do_time) {
						track[kk++] = (do_time == 2) ? (double)(ns - j) : t + k * tt;
						if (kk == n_alloc) {
							n_alloc += BIG_CHUNK;
							track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
						}
					}
				}
				nn += nd;
			}

			xp[i] = next_x;	yp[i] = next_y;
			t += dt;
		}
		if (path) {
			track[kk++] = xp[i];
			if (kk == n_alloc) {
				n_alloc += BIG_CHUNK;
				track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
			}
			track[kk++] = yp[i];
			if (kk == n_alloc) {
				n_alloc += BIG_CHUNK;
				track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
			}
			if (do_time) {
				track[kk++] = (do_time == 2) ? (double)(ns - j) : t;
				if (kk == n_alloc) {
					n_alloc += BIG_CHUNK;
					track = (double *) GMT_memory ((void *)track, (size_t)n_alloc, sizeof (double), "libspotter");
				}
			}
			track[start_k] = nn+1;
		}
	}
	if (path) {
		track = (double *) GMT_memory ((void *)track, (size_t)kk, sizeof (double), "libspotter");
		*c = track;
		return (kk);
	}

	return (np);
}

/* Converts a set of total reconstruction poles to forward stage poles for flowlines
 *
 * Based partly on Cox and Hart, 1986
 */

void spotter_finite_to_fwstages (struct EULER p[], int n, BOOLEAN finite_rates, BOOLEAN stage_rates)
{
	/* Convert finite rotations to forward stage rotations for flowlines */
	/* p[]		: Array of structure elements with rotation parameters
	 * n		: Number of rotations
	 * finite_rates	: TRUE if finite rotations given in degree/my [else we have opening angle]
	 * stage_rates	: TRUE if stage rotations should be returned in degree/my [else we return opening angle]
	 */
	 
	int i;
	double *elon, *elat, *ew, t_old;
	double R_young[3][3], R_old[3][3], R_stage[3][3];

	/* Expects total reconstruction models to have youngest poles first */
	
	elon = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "libspotter");
	elat = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "libspotter");
	ew   = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "libspotter");
	
	set_I_matrix (R_young);		/* The first time, R_young is simply I */
	
	/* First forward stage pole is the youngest total reconstruction pole */

	t_old = 0.0;
	for (i = 0; i < n; i++) {
		if (finite_rates) p[i].omega *= p[i].duration;			/* Convert opening rate to opening angle */
		make_rot_matrix (p[i].lon, p[i].lat, -p[i].omega, R_old);	/* Make rotation matrix from rotation parameters, take transpose by passing -omega */
		matrix_mult (R_young, R_old, R_stage);				/* This is R_stage = R_young * R_old^t */
		matrix_to_pole (R_stage, &elon[i], &elat[i], &ew[i]);		/* Get rotation parameters from matrix */
		if (elon[i] > 180.0) elon[i] -= 360.0;				/* Adjust lon */
		matrix_transpose (R_young, R_old);				/* Set R_young = (R_old^t)^t = R_old */
		p[i].t_stop = t_old;
		t_old = p[i].t_start;
	}
	
	/* Repopulate the EULER structure given the rotation parameters */
	
	xyw_to_struct_euler (p, elon, elat, ew, n, TRUE, stage_rates);

	GMT_free ((void *)elon);
	GMT_free ((void *)elat);
	GMT_free ((void *)ew);
	
	/* Flip order since stages go from oldest to youngest */
	
	reverse_rotation_order (p, n);	/* Flip order since stages go from oldest to youngest */
}

void spotter_finite_to_stages (struct EULER p[], int n, BOOLEAN finite_rates, BOOLEAN stage_rates)
{
	/* Convert finite rotations to backwards stage rotations for backtracking */
	/* p[]		: Array of structure elements with rotation parameters
	 * n		: Number of rotations
	 * finite_rates	: TRUE if finite rotations given in degree/my [else we have opening angle]
	 * stage_rates	: TRUE if stage rotations should be returned in degree/my [else we return opening angle]
	 */
	 
	int i;
	double *elon, *elat, *ew, t_old;
	double R_young[3][3], R_old[3][3], R_stage[3][3];

	/* Expects total reconstruction models to have youngest poles first */
	
	elon = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "libspotter");
	elat = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "libspotter");
	ew   = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "libspotter");
	
	set_I_matrix (R_young);		/* The first time, R_young is simply I */
	
	t_old = 0.0;
	for (i = 0; i < n; i++) {
		if (finite_rates) p[i].omega *= p[i].duration;			/* Convert opening rate to opening angle */
		make_rot_matrix (p[i].lon, p[i].lat, p[i].omega, R_old);	/* Get rotation matrix from pole and angle */
		matrix_mult (R_young, R_old, R_stage);				/* This is R_stage = R_young^t * R_old */
		matrix_to_pole (R_stage, &elon[i], &elat[i], &ew[i]);		/* Get rotation parameters from matrix */
		if (elon[i] > 180.0) elon[i] -= 360.0;				/* Adjust lon */
		matrix_transpose (R_young, R_old);				/* Sets R_young = transpose (R_old) for next round */
		p[i].t_stop = t_old;
		t_old = p[i].t_start;
	}
	
	/* Repopulate the EULER structure given the rotation parameters */
	
	xyw_to_struct_euler (p, elon, elat, ew, n, TRUE, stage_rates);

	GMT_free ((void *)elon);
	GMT_free ((void *)elat);
	GMT_free ((void *)ew);
	
	reverse_rotation_order (p, n);	/* Flip order since stages go from oldest to youngest */
}

void spotter_stages_to_finite (struct EULER p[], int n, BOOLEAN finite_rates, BOOLEAN stage_rates)
{
	/* Convert stage rotations to finite rotations */
	/* p[]		: Array of structure elements with rotation parameters
	 * n		: Number of rotations
	 * finite_rates	: TRUE if finite rotations should be returned in degree/my [else we return opening angle]
	 * stage_rates	: TRUE if stage rotations given in degree/my [else we have opening angle]
	 */

	int i;
	double *elon, *elat, *ew;
	double R_young[3][3], R_old[3][3], R_stage[3][3];

	/* Expects stage pole models to have oldest poles first, so we must flip order */
	
	reverse_rotation_order (p, n);	/* Expects stage pole models to have oldest poles first, so we must flip order */
	
	elon = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "libspotter");
	elat = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "libspotter");
	ew   = (double *) GMT_memory (VNULL, (size_t)n, sizeof (double), "libspotter");
	
	set_I_matrix (R_old);		/* The first time, R_old is simply I */
	
	for (i = 0; i < n; i++) {
		if (stage_rates) p[i].omega *= p[i].duration;				/* Convert opening rate to opening angle */
		make_rot_matrix (p[i].lon, p[i].lat, p[i].omega, R_stage);		/* Make matrix from rotation parameters */
		matrix_mult (R_old, R_stage, R_young);					/* Set R_young = R_old * R_stage */
		memcpy ((void *)R_old, (void *)R_young, (size_t)(9 * sizeof (double)));	/* Set R_old = R_young for next time around */
		matrix_to_pole (R_young, &elon[i], &elat[i], &ew[i]);			/* Get rotation parameters from matrix */
		if (elon[i] > 180.0) elon[i] -= 360.0;					/* Adjust lon */
	}
	
	/* Repopulate the EULER structure given the rotation parameters */
	
	xyw_to_struct_euler (p, elon, elat, ew, n, FALSE, finite_rates);

	GMT_free ((void *)elon);
	GMT_free ((void *)elat);
	GMT_free ((void *)ew);
}

void spotter_add_rotations (struct EULER a[], int n_a, struct EULER b[], int n_b, struct EULER *c[], int *n_c)
{
	/* Takes two finite rotation models and adds them together.
	 * We do this by first converting both to stage poles.  We then
	 * determine all the time knots needed, and then resample both
	 * stage rotation models onto the same list of knots.  This is
	 * easy to do with stage poles since we end up using partial
	 * stage rotations.  TO do this with finite poles would involve
	 * computing intermediate stages anyway.  When we have the resampled
	 * stage rotations we convert back to finite rotations and then
	 * simply add each pair of rotations using matrix multiplication.
	 * The final finite rotation model is returned in c. */
	
	struct EULER *a2, *b2, *c2;
	double *t, t_min, t_max, Ra[3][3], Rb[3][3], Rab[3][3], lon, lat, w, sign_a, sign_b;
	int i, j, k, n_k = 0;
	BOOLEAN a_ok = TRUE, b_ok = TRUE;
	
	sign_a = (n_a > 0) ? +1.0 : -1.0;
	sign_b = (n_b > 0) ? +1.0 : -1.0;
	n_a = abs (n_a);
	n_b = abs (n_b);
	
	/* Allocate more than we need, must likely */
	
	t = (double *) GMT_memory (VNULL, (size_t)(n_a + n_b), sizeof (double), GMT_program);
	
	/* First convert the two models to stage poles */
	
	spotter_finite_to_stages (a, n_a, TRUE, TRUE);		/* Return stage poles */
	spotter_finite_to_stages (b, n_b, TRUE, TRUE);		/* Return stage poles */
	
	/* Find all the time knots used by the two models */
	
	t_max = MIN (a[0].t_start, b[0].t_start);
	t_min = MAX (a[n_a-1].t_stop, b[n_b-1].t_stop);
	t[n_k++] = t_max;
	i = j = 0;
	while (i < n_a && a[i].t_stop > t[0]) i++;
	if (i == (n_a - 1)) a_ok = FALSE;
	while (j < n_b && b[j].t_stop > t[0]) j++;
	if (j == (n_b - 1)) b_ok = FALSE;
	while (a_ok || b_ok) {
		if (a_ok && !b_ok) {		/* Only a left */
			t[n_k] = a[i++].t_stop;
			if (i == (n_a - 1)) a_ok = FALSE;
		}
		else if (b_ok && !a_ok) {	/* Only b left */
			t[n_k] = b[j++].t_stop;
			if (j == (n_b - 1)) b_ok = FALSE;
		}
		else if (a_ok && a[i].t_stop > b[j].t_stop) {
			t[n_k] = a[i++].t_stop;
			if (i == (n_a - 1)) a_ok = FALSE;
		}
		else if (b_ok && b[j].t_stop > a[i].t_stop) {
			t[n_k] = b[j++].t_stop;
			if (j == (n_b - 1)) b_ok = FALSE;
		}
		else {	/* Same time for both */
			t[n_k] = b[j++].t_stop;
			i++;
			if (i == (n_a - 1)) a_ok = FALSE;
			if (j == (n_b - 1)) b_ok = FALSE;
		}
		n_k++;
	}
	t[n_k++] = t_min;
	n_k--;	/* Number of structure elements is one less than number of knots */
	
	b2 = (struct EULER *) GMT_memory (VNULL, (size_t)n_k, sizeof (struct EULER), GMT_program);
	a2 = (struct EULER *) GMT_memory (VNULL, (size_t)n_k, sizeof (struct EULER), GMT_program);
	c2 = (struct EULER *) GMT_memory (VNULL, (size_t)n_k, sizeof (struct EULER), GMT_program);
	
	for (k = i = j = 0; k < n_k; k++) {	/* Resample the two stage pole models onto the same knots */
		/* First resample p onto p2 */
		while (a[i].t_stop >= t[k]) i++;				/* Wind up */
		a2[k] = a[i];							/* First copy everything */
		if (a2[k].t_start > t[k]) a2[k].t_start = t[k];			/* Adjust start time */
		if (a2[k].t_stop < t[k+1]) a2[k].t_stop = t[k+1];		/* Adjust stop time */
		a2[k].duration = a2[k].t_start - a2[k].t_stop;			/* Set the duration */
		
		/* Then resample a onto a2 */
		while (b[j].t_stop >= t[k]) j++;				/* Wind up */
		b2[k] = b[j];							/* First copy everything */
		if (b2[k].t_start > t[k]) b2[k].t_start = t[k];			/* Adjust start time */
		if (b2[k].t_stop < t[k+1]) b2[k].t_stop = t[k+1];		/* Adjust stop time */
		b2[k].duration = b2[k].t_start - b2[k].t_stop;			/* Set the duration */
	}
	
	GMT_free ((void *)t);
	
	/* Now switch to finite rotations again to do the additions */
	
	spotter_stages_to_finite (a2, n_k, FALSE, TRUE);	/* Return opening angles, not rates this time */
	spotter_stages_to_finite (b2, n_k, FALSE, TRUE);
	
	for (i = 0; i < n_k; i++) {	/* Add each pair of rotations */
		make_rot_matrix (a2[i].lon, a2[i].lat, sign_a * a2[i].omega, Ra);
		make_rot_matrix (b2[i].lon, b2[i].lat, sign_b * b2[i].omega, Rb);
		matrix_mult (Rb, Ra, Rab);	/* Rot a + Rot b = RB * Ra ! */
		matrix_to_pole (Rab, &lon, &lat, &w);
		c2[i].lon = lon;	
		c2[i].lat = lat;
		c2[i].t_start = a2[i].t_start;
		c2[i].t_stop  = 0.0;
		c2[i].duration = c2[i].t_start;
		c2[i].omega = w / c2[i].duration;	/* Return rates again */
	}
	GMT_free ((void *)a2);
	GMT_free ((void *)b2);
	
	*n_c = n_k;
	*c = c2;
}

void make_rot_matrix (double lonp, double latp, double w, double R[3][3])
{
/*	lonp, latp	Euler pole in degrees
 *	w		angular rotation in degrees
 *
 *	R		the rotation matrix
 */

	double E[3], sin_w, cos_w, c, E_x, E_y, E_z, E_12c, E_13c, E_23c;
	
	sincos (w * D2R, &sin_w, &cos_w);
        GMT_geo_to_cart (&latp, &lonp, E, TRUE);
	c = 1 - cos_w;

	E_x = E[0] * sin_w;
	E_y = E[1] * sin_w;
	E_z = E[2] * sin_w;
	E_12c = E[0] * E[1] * c;
	E_13c = E[0] * E[2] * c;
	E_23c = E[1] * E[2] * c;

	R[0][0] = E[0] * E[0] * c + cos_w;
	R[0][1] = E_12c - E_z;
	R[0][2] = E_13c + E_y;

	R[1][0] = E_12c + E_z;
	R[1][1] = E[1] * E[1] * c + cos_w;
	R[1][2] = E_23c - E_x;

	R[2][0] = E_13c - E_y;
	R[2][1] = E_23c + E_x;
	R[2][2] = E[2] * E[2] * c + cos_w;
}

void make_rot0_matrix (double lonp, double latp, double R[3][3], double E[])
{	/* This starts setting up the matrix without knowing the angle of rotation
	 * Call set_rot_angle wiht R, E, and omega to complete the matrix
	 * lonp, latp	Euler pole in degrees
	 *
	 *	R		the rotation matrix without terms depending on omega
	 */

        GMT_geo_to_cart (&latp, &lonp, E, TRUE);

	R[0][0] = E[0] * E[0];
	R[0][1] = E[0] * E[1];
	R[0][2] = E[0] * E[2];

	R[1][0] = E[0] * E[1];
	R[1][1] = E[1] * E[1];
	R[1][2] = E[1] * E[2];

	R[2][0] = E[0] * E[2];
	R[2][1] = E[1] * E[2];
	R[2][2] = E[2] * E[2];
}

void set_rot_angle (double w, double R[3][3], double E[])
{	/* Sets R using R(no_omega) and the given rotation angle w in radians */
	double sin_w, cos_w, c, E_x, E_y, E_z;
	
	sincos (w, &sin_w, &cos_w);
	c = 1 - cos_w;

	E_x = E[0] * sin_w;
	E_y = E[1] * sin_w;
	E_z = E[2] * sin_w;

	R[0][0] = R[0][0] * c + cos_w;
	R[0][1] = R[0][1] * c - E_z;
	R[0][2] = R[0][2] * c + E_y;

	R[1][0] = R[1][0] * c + E_z;
	R[1][1] = R[1][1] * c + cos_w;
	R[1][2] = R[1][2] * c - E_x;

	R[2][0] = R[2][0] * c - E_y;
	R[2][1] = R[2][1] * c + E_x;
	R[2][2] = R[2][2] * c + cos_w;
}

void matrix_mult (double a[3][3], double b[3][3], double c[3][3])
{	/* C = A * B */
	int i, j, k;
	
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			c[i][j] = 0.0;
			for (k = 0; k < 3; k++) c[i][j] += a[i][k] * b[k][j];
		}
	}
}

void matrix_transpose (double At[3][3], double A[3][3])
{
	/* Computes the matrix transpose */

	int i, j;
	for (j = 0; j < 3; j++) {
		for (i = 0; i < 3; i++) {
			At[i][j] = A[j][i];
		}
	}
}

void matrix_to_pole (double T[3][3], double *plon, double *plat, double *w)
{
	double T13_m_T31, T32_m_T23, T21_m_T12, L, H, tr;
	
	T13_m_T31 = T[0][2] - T[2][0];
	T32_m_T23 = T[2][1] - T[1][2];
	T21_m_T12 = T[1][0] - T[0][1];
	H = T32_m_T23 * T32_m_T23 + T13_m_T31 * T13_m_T31;
	L = sqrt (H + T21_m_T12 * T21_m_T12);
	H = sqrt (H);
	tr = T[0][0] + T[1][1] + T[2][2];

	*plon = atan2 (T13_m_T31, T32_m_T23) * R2D;
	if (*plon < 0.0) (*plon) += 360.0;
	*plat = atan2 (T21_m_T12, H) * R2D;
	*w = atan2 (L, (tr - 1.0)) * R2D;
	if (*plat < 0.0) {	/* Make N hemisphere pole */
		*plat = -(*plat);
		*(plon) += 180.0;
		if (*plon > 360.0) *plon -=-360.0;
		*w = -(*w);
	}
}

void reverse_rotation_order (struct EULER *p, int n)
{	/* Simply shuffles the array from 1:n to n:1 */
	int i, j;
	struct EULER p_tmp;
	
	for (i = 0; i < n/2; i++) {
		j = n - i - 1;
		if (i != j) {
			p_tmp = p[i];
			p[i] = p[j];
			p[j] = p_tmp;
		}
	}
}

void xyw_to_struct_euler (struct EULER *p, double lon[], double lat[], double w[], int n, BOOLEAN stages, BOOLEAN convert)
{	/* Reload the EULER structure from the lon, lat, w arrays.
	 * stages is TRUE if we are loading stage rotations (FALSE is finite poles).
	 * convert is TRUE if we must change angles to rates or vice versa */
	int i;
	
	for (i = 0; i < n; i++) {
		p[i].lon = lon[i];	
		p[i].lat = lat[i];
		p[i].duration = (stages) ? p[i].t_start - p[i].t_stop : p[i].t_start;
		p[i].omega = w[i];
		if (convert) p[i].omega /= p[i].duration;	/* Convert opening angle to opening rate */
		p[i].omega_r = p[i].omega * D2R;
		p[i].sin_lat = sin (p[i].lat * D2R);
		p[i].cos_lat = cos (p[i].lat * D2R);
		p[i].lon_r = p[i].lon * D2R;	
		p[i].lat_r = p[i].lat * D2R;
	}
}

void set_I_matrix (double R[3][3])
{	/* Simply sets R to I, the identity matrix */

	memset ((void *)R, 0, (size_t)(9 * sizeof (double)));
	R[0][0] = R[1][1] = R[2][2] = 1.0;
}
