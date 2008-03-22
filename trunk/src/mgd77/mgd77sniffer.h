/* -------------------------------------------------------------------
 *	$Id: mgd77sniffer.h,v 1.29 2008-03-22 11:55:36 guru Exp $	
 *      See COPYING file for copying and redistribution conditions.
 *
 *    Copyright (c) 2004-2008 by P. Wessel and M. T. Chandler
 *	File:	mgd77sniffer.h
 *
 *	Include file for mgd77sniffer
 *
 *	Authors:
 *		Michael Chandler and Paul Wessel
 *		School of Ocean and Earth Science and Technology
 *		University of Hawaii
 * 
 *	Date:	23-Feb-2004
 * 
 * ------------------------------------------------------------------*/
 
 #include "gmt.h"
 #include "mgd77.h"
 #include "mgd77_e77.h"
 #include "mgd77_rls_coeffs.h"
 
/* Constants */
#define MGD77_N_DATA_FIELDS         27
#define MGD77_NM_PER_DEGREE         60
#define MGD77_METERS_PER_NM         1852
#define MGD77_DEG_TO_KM             (6371.0087714 * D2R)
#define MGD77_NAV_PRECISION_KM		0.00111               /* km */
#define MGD77_MAX_SPEED             10                    /* m/s */
#define MGD77_MIN_SPEED             1                     /* m/s */
#define MGD77_MEDIAN_SPEED          4.7                   /* m/s */
#define MGD77_MAX_DT                900                   /* sec */
#define MGD77_MIN_DS                0.1                   /* km */
#define MGD77_MAX_DS                5                     /* km */
#define MGD77_MAX_DUPLICATES        250
#define MGD77_METERS_PER_FATHOM     (6 * 12 * 2.54 * 0.01)
#define MGD77_FATHOMS_PER_METER     (1.0 / MGD77_METERS_PER_FATHOM)
#define MGD77_MIN_RLS_BINS          20
#define MGD77_MIN_RLS_PTS           100
#define MGD77_N_MAG_RF              13
#define MGD77_MAX_SEARCH            50
#define MGD77_POS_BIT               (1 << 0)
#define MGD77_ZERO_BIT              (1 << 1)
#define MGD77_NEG_BIT               (1 << 2)

/* RLS statistic array cell names */
#define MGD77_RLS_SLOPE             0
#define MGD77_RLS_ICEPT             1
#define MGD77_RLS_STD               2
#define MGD77_RLS_SXX               3
#define MGD77_RLS_CORR              4
#define MGD77_RLS_SIG               5
#define MGD77_RLS_RMS               6
#define MGD77_RLS_SUMX2             7
#define MGD77_N_STATS               8

/* LMS Limits - Obtained from 4616 bathy and 1657 gravity cruises */
#define MGD77_MIN_DEPTH_SLOPE       0.900404              /* Q2.5 */
#define MGD77_MAX_DEPTH_SLOPE       1.1149               /* Q97.5 */
#define MGD77_MAX_DEPTH_INTERCEPT   0
#define MGD77_MAX_DEPTH_MISFIT      140.676                /* Q95 */
#define MGD77_MIN_FAA_SLOPE         0.868674     /*Q25(Q2.5=-0.17)*/
#define MGD77_MAX_FAA_SLOPE         1.06862              /* Q97.5 */
#define MGD77_MAX_FAA_INTERCEPT     19.1557                /* Q95 */
#define MGD77_MAX_FAA_MISFIT        7.98608                /* Q95 */

/* Verbose Error Display Codes */
#define TYPE_WARN            0
#define TIME_WARN            1
#define DISTANCE_WARN        2
#define SPEED_WARN           3
#define VALUE_WARN           4
#define SLOPE_WARN           5
#define GRID_WARN            6
#define SUMMARY_WARN         7
#define MGD77_N_WARN_TYPES   8

/* MGD77 date constants */
#define AUX_YR	0
#define AUX_MO	1
#define AUX_DY	2
#define AUX_HR	3
#define AUX_MI	4
#define AUX_SC	5

struct BAD_SECTION {	/* To flag a range of records as bad for given field */
	char abbrev[8];	/* Field name */
	GMT_LONG col;	/* Column number */
	GMT_LONG start;	/* First record to flag */
	GMT_LONG stop;	/* Last record to flag */
};
#define MAX_BAD_SECTIONS	100

struct MGD77_GRID_INFO {
	struct GRD_HEADER grdhdr;
	struct GMT_EDGEINFO edgeinfo;
	struct GMT_BCR bcr;
	GMT_LONG one_or_zero, nx, ny, col, sign, g_pts, format, mode, mx, interpolate;
	double scale, max_lat;
	char abbrev[8];
	char fname[32];
};

struct MGD77_SNIFFER_DEFAULTS {
	char abbrev[8];      /* MGD77 field name abbreviations  */
	double minValue;     /* minimum accepted field value    */
	double maxValue;     /* maximum accepted field value    */
	double delta;        /* RLS decimation binsize interval */
	double maxTimeGrad;  /* maximum +/- time derivative     */
	double maxSpaceGrad; /* maximum +/- space derivative    */
	double maxDiff;      /* maximum +/- value change        */
	double maxArea;      /* maximum grid offset area        */
};

struct MGD77_ERROR {
	unsigned int flags[6];
};

struct MGD77_MAG_RF {
	char *model;        /* Reference field model name */
	GMT_LONG code;           /* Reference field code       */
	GMT_LONG start;          /* Model start year           */
	GMT_LONG end;            /* Model end year             */
};

/* Local functions */
void read_grid (struct MGD77_GRID_INFO *info, float **grid, double w, double e, double s, double n, BOOLEAN bilinear, double threshold);
GMT_LONG sample_grid (struct MGD77_GRID_INFO *info, struct MGD77_DATA_RECORD *D, double **g, float *grid, GMT_LONG n_grid, GMT_LONG n);
void regress_ls (double *x, double *y, GMT_LONG n, double *stat, GMT_LONG col);
void regress_rls (double *x, double *y, GMT_LONG nvalues, double *stat, GMT_LONG col);
void regress_lms (double *x, double *y, GMT_LONG nvalues, double *stat, GMT_LONG gridField);
void regresslms_sub (double *x, double *y, double angle0, double angle1, GMT_LONG nvalues, GMT_LONG n_angle, double *stat, GMT_LONG gridField);
GMT_LONG decimate (double *x, double *y, GMT_LONG nclean, double min, double max, double delta, double **dec_new, double **dec_orig, char *fieldTest);
double lms (double *x, GMT_LONG n);
double median (double *x, GMT_LONG n);
