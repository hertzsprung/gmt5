/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2012 by P. Wessel, W. H. F. Smith, R. Scharroo, and J. Luis
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
 * This include file contains prototypes of functions that are not part
 * of POSIX but are often distributed anyway as part of ANSI C.  The
 * gmt_notposix.h include file have been generated by configure and tells
 * us which of the required functions are present or not and sets a macro or
 * prototype accordingly.  The purpose is to take advantage of the built-in
 * functions if they exist and provide an alternative definition otherwise.
 *
 * If you must do a manual edit, you should do so in gmt_notposix.h.
 * For non-UNIX platforms you should also look in gmt_notunix.h
 *
 * Version:	5 API
 */

#ifndef _GMT_MATH_H
#define _GMT_MATH_H

#if defined(copysign)
/* Macro already takes care of copysign - probably from BSD */
#elif defined(HAVE_COPYSIGN)
extern double copysign(double x, double y);
#else
#define copysign(x,y) ((y) < 0.0 ? -fabs(x) : fabs(x))
#endif

#if defined(log2)
/* Macro already takes care of log2 - probably from BSD */
#elif defined(HAVE_LOG2)
extern double log2(double x);
#else
#define log2(x) (log10(x)/0.30102999566398114250631579125183634459972381591796875)
#endif

#ifdef HAVE_ACOSH
extern double acosh(double x);
#else
#define acosh(x) log((x) + (d_sqrt((x) + 1.0)) * (d_sqrt((x) - 1.0)))
#endif

#ifdef HAVE_ASINH
extern double asinh(double x);
#else
#define asinh(x) log((x) + (hypot((x), 1.0)))
#endif

#ifdef HAVE_RINT
extern double rint(double x);
#else
EXTERN_MSC double rint(double x);
#endif

#ifdef HAVE_IRINT
extern int irint(double x);
#else
#define irint(x) ((int)rint(x))
#endif

/* Misc. ANSI-C math functions used by grdmath and gmtmath.
 * These functions are available on many platforms and we
 * seek to use them.  If not available then we compile in
 * replacements from gmt_notposix.c */

#if defined(j0)
/* Macro already takes care of j0 - probably from BSD */
#elif defined(HAVE_J0)
extern double j0(double x);
#else
EXTERN_MSC double j0(double x);
#endif

#if defined(j1)
/* Macro already takes care of j1 - probably from BSD */
#elif defined(HAVE_J1)
extern double j1(double x);
#else
EXTERN_MSC double j1(double x);
#endif

#if defined(jn)
/* Macro already takes care of jn - probably from BSD */
#elif defined(HAVE_JN)
extern double jn(int n, double x);
#else
EXTERN_MSC double jn(int n, double x);
#endif

#if defined(y0)
/* Macro already takes care of y0 - probably from BSD */
#elif defined(HAVE_Y0)
extern double y0(double x);
#else
EXTERN_MSC double y0(double x);
#endif

#if defined(y1)
/* Macro already takes care of y1 - probably from BSD */
#elif defined(HAVE_Y1)
extern double y1(double x);
#else
EXTERN_MSC double y1(double x);
#endif

#if defined(yn)
/* Macro already takes care of yn - probably from BSD */
#elif defined(HAVE_YN)
extern double yn(int n, double x);
#else
EXTERN_MSC double yn(int n, double x);
#endif

#if defined(erf)
/* Macro already takes care of erf - probably from BSD */
#elif defined(HAVE_ERF)
extern double erf(double x);
#else
EXTERN_MSC double erf(double x);
#endif

#if defined(erfc)
/* Macro already takes care of erfc - probably from BSD */
#elif defined(HAVE_ERFC)
extern double erfc(double x);
#else
EXTERN_MSC double erfc(double x);
#endif

#ifdef HAVE_ATANH
extern double atanh(double x);
#else
EXTERN_MSC double atanh(double x);
#endif

#if defined(log1p)
/* Macro already takes care of log1p - probably from BSD */
#elif defined(HAVE_LOG1P)
extern double log1p(double x);
#else
EXTERN_MSC double log1p(double x);
#endif

#if defined(hypot)
/* Macro already takes care of hypot - probably from BSD */
#elif defined(HAVE_HYPOT)
extern double hypot(double x, double y);
#else
EXTERN_MSC double hypot(double x, double y);
#endif

#if defined(strdup)
/* Macro already takes care of strdup - probably from BSD */
#elif defined(HAVE_STRDUP)
extern char *strdup(const char *s);
#else
EXTERN_MSC char *strdup(const char *s);
#endif

#ifndef HAVE_STRTOD
	EXTERN_MSC double strtod(const char *nptr, char **endptr);
#endif

/* On Dec Alpha OSF1 there is a sincos with different syntax.
 * Assembly wrapper provided by Lloyd Parkes <lloyd@must-have-coffee.gen.nz>
 * can be used instead.
 * See alpha-sincos.s
 */
 
#if defined(sincos)
/* Macro already takes care of strtod - probably from BSD */
#elif defined(HAVE_SINCOS)
extern void sincos (double x, double *s, double *c);
#elif defined(HAVE_ALPHASINCOS)
#define sincos(x,s,c) alpha_sincos (x, s, c)
extern void alpha_sincos (double x, double *s, double *c);
#else
EXTERN_MSC void sincos (double x, double *s, double *c);
#endif

/* Must replace the system qsort with ours which is 64-bit compliant
 * See gmt_qsort.c.
 */

#ifdef GMT_QSORT
EXTERN_MSC void GMT_qsort (void *a, size_t n, size_t es, int (*cmp) (const void *, const void *));
#define qsort GMT_qsort
#endif

#endif /* _GMT_MATH_H */