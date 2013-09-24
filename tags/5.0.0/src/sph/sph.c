/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 2008-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 or any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 * Spherical triangulation - Delaunay or Voronoi options.
 * Relies on STRIPACK Fortran F77 library (Renka, 1997). Reference:
 * Renka, R, J,, 1997, Algorithm 772: STRIPACK: Delaunay Triangulation
 *    and Voronoi Diagram on the Surface of a Sphere, AMC Trans. Math.
 *    Software, 23 (3), 416-434.
 * Spherical interpolation - tension or smoothing.
 * Relies on SSRFPACK Fortran F77 library (Renka, 1997). Reference:
 * Renka, R, J,, 1997, Algorithm 773: SSRFPACK: Interpolation of
 *    Scattered Data on the Surface of a Sphere with a Surface under Tension,
 *    AMC Trans. Math. Software, 23 (3), 435-442.
 * We translated both to C using f2c and link with -lf2c
 *
 * Author:      Paul Wessel
 * Date:        1-AUG-2011
 * Version:	API 5 64-bit
 *
 */
 
#include "gmt.h"
#include "sph.h"

typedef double doublereal;
typedef GMT_LONG integer;
typedef GMT_LONG logical;

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif
#define FALSE_ 0
#define TRUE_ 1

/* define SPH_DEBUG to get more original verbose output from s*pack.c */

#include "stripack.c"
#include "ssrfpack.c"

void stripack_lists (struct GMT_CTRL *C, GMT_LONG n, double *x, double *y, double *z, struct STRIPACK *T)
{
 	/* n, the number of points.
	 * x, y, z, the arrays with coordinates of points 
	 *
	 * xc, yc, zc: the coordinates of the Voronoi polygon vertices.
	 * lend, points to the "first" vertex in the Voronoi polygon around a particular node.
	 * lptr: given a vertex, returns the next vertex in the Voronoi polygon.
	 *
	 * NOTE: All indeces returned are C (0->) adjusted from FORTRAN (1->).
	 */

	GMT_LONG k, nrow = TRI_NROW, lnew, ierror, n_alloc;
	GMT_LONG *iwk = NULL, *list = NULL, *lptr = NULL, *lend = NULL;
	double *ds = NULL;
	
	ds = GMT_memory (C, NULL, n, double);
	lend = GMT_memory (C, NULL, n, GMT_LONG);
 	iwk = GMT_memory (C, NULL, 2*n, GMT_LONG);
	n_alloc = 6 * (n - 2);
	lptr = GMT_memory (C, NULL, n_alloc, GMT_LONG);
	list = GMT_memory (C, NULL, n_alloc, GMT_LONG);

	/* Create the triangulation. Main output is (list, lptr, lend) */

	GMT_report (C, GMT_MSG_VERBOSE, "Call STRIPACK TRMESH subroutine...");
	trmesh_ (&n, x, y, z, list, lptr, lend, &lnew, iwk, &iwk[n], ds, &ierror);
	GMT_free (C, ds);
	GMT_free (C, iwk);
	GMT_report (C, GMT_MSG_VERBOSE, "OK\n");

	if (ierror == -2) {
		GMT_report (C, GMT_MSG_FATAL, "STRIPACK: Error in TRMESH. The first 3 nodes are collinear.\n");
		GMT_exit (EXIT_FAILURE);
	}

	if (ierror > 0) {
		GMT_report (C, GMT_MSG_FATAL, "STRIPACK: Error in TRMESH.  Duplicate nodes encountered.\n");
		GMT_exit (EXIT_FAILURE);
	}

	if (T->mode == INTERPOLATE) {	/* Pass back the three lists from trmesh_ */
		T->I.list = list;	/* Save these for output */
		T->I.lptr = lptr;
		T->I.lend = lend;
		return;
	}
	
	/* Create a triangle list which returns the number of triangles and their node list tri */

	n_alloc = 2 * (n - 2);
	T->D.tri = GMT_memory (C, NULL, (TRI_NROW*n_alloc), GMT_LONG);
	GMT_report (C, GMT_MSG_VERBOSE, "Call STRIPACK TRLIST subroutine...");
	trlist_ (&n, list, lptr, lend, &nrow, &T->D.n, T->D.tri, &ierror);
	GMT_report (C, GMT_MSG_VERBOSE, "OK\n");

	if (ierror) {
		GMT_report (C, GMT_MSG_FATAL, "STRIPACK: Error in TRLIST.\n");
		GMT_exit (EXIT_FAILURE);
	}
	
	if (T->mode == VORONOI) {	/* Construct the Voronoi diagram */
		GMT_LONG *lbtri = NULL;
		double *rc = NULL;
		double *xc = NULL, *yc = NULL, *zc = NULL;	/* Voronoi polygon vertices */
	
		/* Note that the triangulation data structure is altered if NB > 0 */

		n_alloc = 2 * (n - 2);
		xc = GMT_memory (C, NULL, n_alloc, double);
		yc = GMT_memory (C, NULL, n_alloc, double);
		zc = GMT_memory (C, NULL, n_alloc, double);
		rc = GMT_memory (C, NULL, n_alloc, double);
		n_alloc = 6 * (n - 2);
		T->V.listc = GMT_memory (C, NULL, n_alloc, GMT_LONG);
		lbtri = GMT_memory (C, NULL, 6*n, GMT_LONG);

		GMT_report (C, GMT_MSG_VERBOSE, "Call STRIPACK CRLIST subroutine...");
		crlist_ (&n, &n, x, y, z, list, lend, lptr, &lnew, lbtri, T->V.listc, &T->V.n, xc, yc, zc, rc, &ierror);
		GMT_report (C, GMT_MSG_VERBOSE, "OK\n");
		GMT_free (C, lbtri);
		GMT_free (C, rc);
		T->V.lend = lend;	/* Save these for output */
		T->V.lptr = lptr;
		/* Convert polygon vertices vectors to lon, lat */
		n_alloc = 2 * (n - 2);
		T->V.lon = GMT_memory (C, NULL, n_alloc, double);
		T->V.lat = GMT_memory (C, NULL, n_alloc, double);
		cart_to_geo (C, n_alloc, xc, yc, zc, T->V.lon, T->V.lat);
		GMT_free (C, xc);
		GMT_free (C, yc);
		GMT_free (C, zc);

		if (0 < ierror) {
			GMT_report (C, GMT_MSG_FATAL, "STRIPACK: Error in CRLIST.  IERROR = %ld.\n", ierror);
			GMT_exit (EXIT_FAILURE);
		}
		
		/* Adjust Fortran to C indeces */
		n_alloc = 6 * (n - 2);
		for (k = 0; k < n_alloc; k++) T->V.listc[k]--;
		for (k = 0; k < n_alloc; k++) T->V.lptr[k]--;
		for (k = 0; k < n; k++) T->V.lend[k]--;
	}
	else {	/* Free things not needed */
		GMT_free (C, lend);
		GMT_free (C, lptr);
	}
	
	/* Adjust Fortran to C indeces */
	for (k = 0; k < TRI_NROW*T->D.n; k++) T->D.tri[k]--;
	
	GMT_free (C, list);
}

double stripack_areas (double *V1, double *V2, double *V3)
{	/* Wrapper for STRIPACK areas_ */
	return (areas_ (V1, V2, V3));
}

void cart_to_geo (struct GMT_CTRL *C, GMT_LONG n, double *x, double *y, double *z, double *lon, double *lat)
{	/* Convert Cartesian vectors back to lon, lat vectors */
	GMT_LONG k;
	double V[3];
	for (k = 0; k < n; k++) {
		V[0] = x[k];	V[1] = y[k];	V[2] = z[k];
		GMT_cart_to_geo (C, &lat[k], &lon[k], V, TRUE);
	}
}

/* Must be int due to qsort requirement */
int compare_arc (const void *p1, const void *p2)
{
	struct STRPACK_ARC *a = (struct STRPACK_ARC *)p1;
	struct STRPACK_ARC *b = (struct STRPACK_ARC *)p2;
	if (a->begin < b->begin) return (-1);
	if (a->begin > b->begin) return (1);
	if (a->end < b->end) return (-1);
	if (a->end > b->end) return (1);
	return (0);
}

/* Functions for spherical surface interpolation */

void ssrfpack_grid (struct GMT_CTRL *C, double *x, double *y, double *z, double *w, GMT_LONG n, GMT_LONG mode, double *par, GMT_LONG vartens, struct GRD_HEADER *h, double *f)
{
	GMT_LONG ierror, nm, k, i, j, n_sig, nxp, ist, ij, iflgs, iter, itgs, plus = 1, minus = -1, nx = (GMT_LONG)h->nx, ny = (GMT_LONG)h->ny;
	double *sigma = NULL, *grad = NULL, *plon = NULL, *plat = NULL, tol = 0.01, dsm, dgmx;
	struct STRIPACK P;
	
	n_sig = ((vartens) ? 6 * (n - 2) : 1);

	/* Create the triangulation. Main output is (P.I->(list, lptr, lend) */

	GMT_memset (&P, 1, struct STRIPACK);
	P.mode = INTERPOLATE;
	stripack_lists (C, n, x, y, z, &P);
	
	/* Set out output nodes */
	
	plon = GMT_memory (C, NULL, h->nx, double);
	plat = GMT_memory (C, NULL, h->ny, double);
	for (i = 0; i < h->nx; i++) plon[i] = D2R * GMT_grd_col_to_x (C, i, h);
	for (j = 0; j < h->ny; j++) plat[j] = D2R * GMT_grd_row_to_y (C, j, h);
	nm = h->nx * h->ny;
	
	/* Time to work on the interpolation */

	sigma = GMT_memory (C, NULL, n_sig, double);
	if (mode) grad = GMT_memory (C, NULL, 3*n, double);
	
	if (mode == 0) {	 /* C-0 interpolation (INTRC0). */
		nxp = 0;
		ist = 1;
		for (j = 0; j < h->ny; j++) {
			for (i = 0; i < h->nx; i++) {
				ij = i * h->ny + (h->ny - j -1); /* Use Fortran indexing since calling program will transpose to GMT order */
				intrc0_ (&n, &plat[j], &plon[i], x, y, z, w, P.I.list, P.I.lptr, P.I.lend, &ist, &f[ij], &ierror);
				if (ierror > 0) nxp++;
	            		if (ierror < 0) {
					GMT_report (C, GMT_MSG_FATAL, "Error in INTRC0: I = %ld, J = %ld, IER = %ld\n", j, i, ierror);
					GMT_exit (EXIT_FAILURE);
	            		}
			}
		}
		GMT_report (C, GMT_MSG_VERBOSE, "INTRC0: Number of evaluations = %ld, number of extrapolations = %ld\n", nm, nxp);
	}
	else if (mode == 1) {	/* C-1 interpolation (INTRC1) with local gradients GRADL. */
	   	/* Accumulate the sum of the numbers of nodes used in the least squares fits in sum. */
		GMT_LONG k1;
		double sum = 0.0;
		for (k = 0; k < n; k++) {
			k1 = k + 1;	/* Since gradl expects Fortran indexing */
			gradl_ (&n, &k1, x, y, z, w, P.I.list, P.I.lptr, P.I.lend, &grad[3*k], &ierror);
			if (ierror < 0) {
				GMT_report (C, GMT_MSG_FATAL, "Error in GRADL: K = %ld IER = %ld\n", k1, ierror);
				GMT_exit (EXIT_FAILURE);
            		}
			sum += (double)ierror;
		}
		sum /= n;
		GMT_report (C, GMT_MSG_VERBOSE, "GRADL: Average number of nodes used in the least squares fits = %g\n", sum);
	        if (vartens) {	/* compute tension factors sigma (getsig). */
			getsig_ (&n, x, y, z, w, P.I.list, P.I.lptr, P.I.lend, grad, &tol, sigma, &dsm, &ierror);
			if (ierror < 0) {
				GMT_report (C, GMT_MSG_FATAL, "Error in GETSIG: IER = %ld\n", ierror);
				GMT_exit (EXIT_FAILURE);
			}
			GMT_report (C, GMT_MSG_VERBOSE, "GETSIG: %ld tension factors altered;  Max change = %g\n", ierror, dsm);
	        }
	
		/* compute interpolated values on the uniform grid (unif). */

		iflgs = 0;
		if (vartens) iflgs = 1;
		unif_ (&n, x, y, z, w, P.I.list, P.I.lptr, P.I.lend, &iflgs, sigma, &ny, &ny, &nx, plat, plon, &plus, grad, f, &ierror);
		if (ierror < 0) {
			GMT_report (C, GMT_MSG_FATAL, "Error in UNIF: IER = %ld\n", ierror);
			GMT_exit (EXIT_FAILURE);
		}
		GMT_report (C, GMT_MSG_VERBOSE, "UNIF: Number of evaluation points = %ld, number of extrapolation points = %ld\n", nm, ierror);
	}
	else if (mode == 2) {	/* c-1 interpolation (intrc1) with global gradients gradg. */
		GMT_LONG maxit, nitg;
		double dgmax;
		/* initialize gradients grad to zeros. */
		GMT_memset (grad, 3*n, double);
		itgs  = (par[0] == 0.0) ? 3    : irint (par[0]);
		maxit = (par[1] == 0.0) ? 10   : irint (par[1]);
		dgmax = (par[2] == 0.0) ? 0.01 : par[2];
		if (!vartens) itgs = 1;

		/* loop on gradg/getsig iterations. */

		for (iter = iflgs = 0; iter < itgs; iter++) {
			nitg = maxit;
			dgmx = dgmax;
			gradg_ (&n, x, y, z, w, P.I.list, P.I.lptr, P.I.lend, &iflgs, sigma, &nitg, &dgmx, grad, &ierror);
			if (ierror < 0) {
				GMT_report (C, GMT_MSG_FATAL, "Error in GRADG (iteration %ld): IER = %ld\n", iter, ierror);
				GMT_exit (EXIT_FAILURE);
			}
			GMT_report (C, GMT_MSG_VERBOSE, "GRADG (iteration %ld): tolerance = %g max change = %g  maxit = %ld no. iterations = %ld ier = %ld\n",
				iter, dgmax, dgmx, maxit, nitg, ierror);
			if (vartens) {
				/* compute tension factors sigma (getsig).  iflgs > 0 if vartens = true */
				iflgs = 1;
				getsig_ (&n, x, y, z, w, P.I.list, P.I.lptr, P.I.lend, grad, &tol, sigma, &dsm, &ierror);
				if (ierror < 0) {
					GMT_report (C, GMT_MSG_FATAL, "Error in GETSIG (iteration %ld): ier = %ld\n", iter, ierror);
					GMT_exit (EXIT_FAILURE);
				}
				GMT_report (C, GMT_MSG_VERBOSE, "GETSIG (iteration %ld): %ld tension factors altered;  Max change = %g\n", iter, ierror, dsm);
			}
		}
		/* compute interpolated values on the uniform grid (unif). */

		unif_ (&n, x, y, z, w, P.I.list, P.I.lptr, P.I.lend, &iflgs, sigma, &ny, &ny, &nx, plat, plon, &plus, grad, f, &ierror);
		if (ierror < 0) {
			GMT_report (C, GMT_MSG_FATAL, "Error in UNIF: IER = %ld\n", ierror);
			GMT_exit (EXIT_FAILURE);
		}
		GMT_report (C, GMT_MSG_VERBOSE, "UNIF: Number of evaluations = %ld, number of extrapolations = %ld\n", nm, ierror);
	}
	else if (mode == 3) {	/* c-1 smoothing method smsurf. */
		double wtk, smtol, gstol, e, sm, *wt = GMT_memory (C, NULL, n, double);
		e    = (par[0] == 0.0) ? 0.01 : par[0];
		sm   = (par[1] <= 0.0) ? (double)n : par[1];
		itgs = (par[2] == 0.0) ? 3 : irint (par[2]);
		if (!vartens) itgs = 1;
		wtk = 1.0 / e;
		for (k = 0; k < n; k++) wt[k] = wtk;	/* store the weights wt. */
		/* compute and print smsurf parameters. */
		smtol = sqrt (2.0 / sm);
		gstol = 0.05 * e;
		GMT_report (C, GMT_MSG_VERBOSE, "SMSURF parameters:\n\texpected squared error = %g\n\tsmoothing parameter sm = %g\n", e, sm);
		GMT_report (C, GMT_MSG_VERBOSE, "\tgauss-seidel tolerance = %g\n\tsmoothing tolerance = %g\n\tweights = %g\n", gstol, smtol, wtk);

		/* loop on smsurf/getsig iterations. */
		for (iter = iflgs = 0; iter < itgs; iter++) {
			smsurf_ (&n, x, y, z, w, P.I.list, P.I.lptr, P.I.lend, &iflgs, sigma, wt, &sm, &smtol, &gstol, &minus, f, grad, &ierror);
			if (ierror < 0) {
				GMT_report (C, GMT_MSG_FATAL, "Error in SMSURF (iteration %ld): IER = %ld\n", iter, ierror);
				GMT_exit (EXIT_FAILURE);
			}
			if (ierror == 1) GMT_report (C, GMT_MSG_VERBOSE, "Error in SMSURF: inactive constraint in SMSURF (iteration %ld).  f is a constant function\n", iter);
			if (vartens) {	/* compute tension factors sigma (getsig).  iflgs > 0 if vt = true. */
				iflgs = 1;
				getsig_ (&n, x, y, z, f, P.I.list, P.I.lptr, P.I.lend, grad, &tol, sigma, &dsm, &ierror);
				if (ierror < 0) {
					GMT_report (C, GMT_MSG_FATAL, "Error in GETSIG (iteration %ld): IER = %ld\n", iter, ierror);
					GMT_exit (EXIT_FAILURE);
				}
				GMT_report (C, GMT_MSG_VERBOSE, "GETSIG (iteration %ld): %ld tension factors altered;  Max change = %g\n", iter, ierror, dsm);
			}
		}
		/* compute interpolated values on the uniform grid (unif). */
		unif_ (&n, x, y, z, w, P.I.list, P.I.lptr, P.I.lend, &iflgs, sigma, &ny, &ny, &nx, plat, plon, &plus, grad, f, &ierror);
		GMT_free (C, wt);
		if (ierror < 0) {
			GMT_report (C, GMT_MSG_FATAL, "Error in UNIF: ier = %ld\n", ierror);
			GMT_exit (EXIT_FAILURE);
		}
		GMT_report (C, GMT_MSG_VERBOSE, "UNIF: Number of evaluations = %ld, number of extrapolations = %ld\n", nm, ierror);
	}
	
	GMT_free (C, plon);
	GMT_free (C, plat);
	GMT_free (C, P.I.list);
	GMT_free (C, P.I.lptr);
	GMT_free (C, P.I.lend);
	if (sigma) GMT_free (C, sigma);
	if (grad) GMT_free (C, grad);
}