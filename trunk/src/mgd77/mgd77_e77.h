/* -------------------------------------------------------------------
 *	$Id: mgd77_e77.h,v 1.1 2007-06-09 01:09:05 mtchandl Exp $	
 *      See COPYING file for copying and redistribution conditions.
 *
 *    Copyright (c) 2004-2007 by P. Wessel and M. T. Chandler
 *	File:	mgd77_e77.h
 *
 *	Include file for mgd77 programs
 *
 *	Authors:
 *		Michael Chandler and Paul Wessel
 *		School of Ocean and Earth Science and Technology
 *		University of Hawaii
 * 
 *	Date:	8-June-2007
 * 
 * ------------------------------------------------------------------*/
 
 /* E77 Error classes */
#define E77_NAV                0
#define E77_VALUE              1
#define E77_SLOPE              2
#define N_ERROR_CLASSES        3
#define N_DEFAULT_TYPES      MGD77_N_NUMBER_FIELDS

/* E77 Nav Error Types */
#define NAV_TIME_OOR         1          /* A */
#define NAV_TIME_DECR        2          /* B */
#define NAV_HISPD            4          /* C */
#define NAV_ON_LAND          8          /* D */
#define NAV_LAT_UNDEF       16          /* E */
#define NAV_LON_UNDEF       32          /* F */
#define N_NAV_TYPES          6

/* E77 Header Errata Codes */
#define E77_HDR_SCALE        1
#define E77_HDR_DCSHIFT      2
#define E77_HDR_ANOM_FAA     3
#define E77_HDR_ANOM_MAG     4
#define E77_HDR_GRID_OFFSET  5
#define E77_HDR_FLAGRANGE    6
#define E77_HDR_BCC          7
#define E77_HDR_PRECISION    8
