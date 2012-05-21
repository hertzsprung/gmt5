/*-----------------------------------------------------------------
 *	$Id$
 *
 *      Copyright (c) 1999-2012 by P. Wessel
 *      See LICENSE.TXT file for copying and redistribution conditions.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU Lesser General Public License as published by
 *      the Free Software Foundation; version 3 or any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU Lesser General Public License for more details.
 *
 *      Contact info: www.soest.hawaii.edu/pwessel
 *--------------------------------------------------------------------*/
/* x2sys.h contains the declaration for the X2SYS and XOVER structures
 * used in the XSYSTEM programs
 *
 * Author:	Paul Wessel
 * Date:	18-OCT-2005
 * Version:	1.1, based on the spirit of the old xsystem code
 *		1.2 Includes support for MGD77+
 *		1.3 GMT 5 style
 *
 */

/* X2SYS IS POSIX.1 COMPLIANT AND REQUIRES GMT AND ITS MGG supplement */

/*-----------------------------------------------------------------

 The data files that can be understood by X2SYS can either be ASCII
 or binary files.  In either case, the file formats must obey some
 simple and reasonable rules:

  ASCII data records:
  -------------------

    -Type 1:  columns separated by tabs, spaces, or commas:
	Each field type must be set to 'a' for ascii string
    -Type 2:  FORTRAN card format, all glued together:
	Each field type must be set to 'A' for ascii string
	and the start-stop column must be appended at the end of
	the info line.

  BINARY data records:
  --------------------

  Each field of the input data file must be identified as one of the
  following types:

	c - signed 1-byte character
	u - unsigned 1-byte character
	h - signed 2-byte integer
	i - signed 4-byte integer
	l - signed 4- or 8-byte integer (long)
	f - 4-byte floating point single precision
	d - 8-byte floating point double precision

	E.g., the GMT MGG format is iiihhh

  This information must be conveyed via the *.def files (-D)
------------------------------------------------------------------*/

#include "mgd77/mgd77.h"
#include "gmt_x2sys.h"

#ifdef GMT_COMPAT
/* Here are legacy functions for old GMT MGG supplement needed in x2sys */

struct GMTMGG_REC {	/* Format of old *.gmt file records */
	int time, lat, lon;
	short int gmt[3];
};

#define GMTMGG_NODATA (-32000)		/* .gmt file NaN proxy */
#define MDEG2DEG	0.000001	/* Convert millidegrees to degrees */
#endif

#define S_RDONLY 0000444

#define X2SYS_VERSION "1.2"

/* Make sure structure sizes are multiples of 8 */

struct X2SYS_TRK_INFO {			/* Structure with info about one track */
	char name[32];			/* Name of track */
	GMT_LONG year;			/* Year the track was collected */
	GMT_LONG nx_int;		/* Total number of internal track cross-over points */
	GMT_LONG nx_ext;		/* Total number of EXTERN_MSCal track cross-over points */
	GMT_LONG flag;			/* Processing flags */
};

struct X2SYS_SET {		/* Structure with info for a data type along the track */
	char id_set[16];		/* Name of this data type or set */
	GMT_LONG nx_int;		/* Number of internal data crossovers */
	GMT_LONG nx_ext;		/* Number of EXTERN_MSCal data crossovers */
	double mean_int;		/* Mean data internal xover value */
	double mean_ext;		/* Mean data EXTERN_MSCal xover value */
	double stdev_int;		/* St. Dev. of the internal data crossovers */
	double stdev_ext;		/* Same for EXTERN_MSCal xovers */
	double dc_shift;		/* Best fitting d.c.-shift for data set */
	double drift_rate;		/* Best fitting drift rate for data set [units/sec] */
};

struct X2SYS_TRK {			/* Structure for each track */
	struct X2SYS_TRK_INFO info;	/* Track-level information */
	struct X2SYS_SET *set;		/* Array of structures; one for each data set */
	struct X2SYS_TRK *next_track;	/* Pointer to next track */
};

struct X2SYS_XOVER_SET {		/* Structure with info on one data type cross-over values */
	double x_val;			/* Data cross-over mismatch */
	double ave;			/* Average data value at crossover */
	double xhead[2];		/* Azimuth at cross-over along track 1 and 2 */
};

struct X2SYS_CORR {		/* Structure with the corrections for each leg */
	char id_name[16];		/* Name of track */
	GMT_LONG year;			/* Year the track was collected */
	double dc_shift;		/* Best fitting d.c.-shift for data set */
	double drift_rate;		/* Best fitting drift rate for data set [units/sec] */
};

struct X2SYS_INFO {
	/* Information of this datasets particular organization */

	char *TAG;			/* The system TAG */
	COUNTER_MEDIUM n_fields;	/* Number of input columns */
	COUNTER_MEDIUM n_out_columns;	/* Number of output columns */
	COUNTER_MEDIUM n_data_cols;	/* Number of data columns (other than x,y,t) */
	size_t rec_size;		/* Number of bytes for a potential x2sys_dbase_*.b file */
	GMT_LONG x_col, y_col, t_col;	/* Column numbers for x, y, and t [== -1 if not set] */
	COUNTER_MEDIUM skip;		/* Number of header records to skip */
	COUNTER_MEDIUM flags;		/* Various processing flags for internal use */
	COUNTER_MEDIUM *out_order;	/* Array with column number in the order for output */
	GMT_BOOLEAN *use_column;		/* Array of T/F for which columns to use */
	COUNTER_MEDIUM geodetic;		/* How longitudes should be stored: 0: (0-360), 1: (-360,0), 2 (-180/+180) */
	COUNTER_MEDIUM dist_flag;		/* How distances are calulated: (0 = Cartesian, 1 = Flat earth, 2 = great circle, 3 = geodesic) */
	p_func_l read_file;		/* Pointer to function that reads this file */
	COUNTER_MEDIUM file_type;		/* 0 = ASCII, 1 = native binary, 2 = netCDF */
	GMT_BOOLEAN ascii_out;		/* TRUE if output should be in ascii */
	GMT_BOOLEAN multi_segment;		/* TRUE if there are multiple segments in this file */
	GMT_BOOLEAN geographic;		/* TRUE if x/y data are lon/lat */
	GMT_BOOLEAN ms_next;		/* TRUE if we just read 1st record in a new segments in this file */
	char unit[2][2];		/* Units for distance (c = Cartesian, e = meter, k = km, m = miles, n = nautical miles)
	 				   and speed (c = Cartesian, e = m/s, k = km/hr, m = miles/hr, n = knots) */
	char ms_flag;			/* Multi-segment header flag */
	char suffix[16];		/* Suffix for these data files */
	char fflags[GMT_BUFSIZ];	/* Text copy of selected columns */
	char path[GMT_BUFSIZ];		/* Full path to current data file */
	struct X2SYS_DATA_INFO *info;	/* Array of info for each data field */
};

struct X2SYS_DATA_INFO {
	double nan_proxy;	/* Value that signifies lack of data (NaN) */
	double scale;		/* Input value should be multiplied by this value */
	double offset;		/* And then add this value */
	COUNTER_MEDIUM start_col;	/* For cardformat: starting column */
	COUNTER_MEDIUM stop_col;	/* For cardformat: last column */
	COUNTER_MEDIUM n_cols;	/* For cardformat: number of columns */
	GMT_BOOLEAN has_nan_proxy;	/* TRUE if there is a special value that indicates NaN */
	GMT_BOOLEAN has_nans;	/* TRUE if there are NaNs in this field */
	GMT_BOOLEAN do_scale;	/* TRUE if scale != 1 or offset != 0 */
	char name[32];		/* Name of this data type */
	char format[32];	/* Output print format for ascii conversion */
	char intype;		/* Input data type (cuhilfdaA) */
};

struct X2SYS_FILE_INFO {
	/* Information for a particular data file */
	GMT_LONG year;		/* Starting year for this leg */
	COUNTER_LARGE n_rows;	/* Number of rows */
	COUNTER_LARGE n_segments;	/* Number of segments in this file */
	COUNTER_LARGE *ms_rec;	/* Pointer to array with start record for each segment */
	char name[32];		/* Name of cruise or agency */
};

struct X2SYS_BIX {
	/* Information for the track binindex setup */
	double wesn[4];		/* Left/Right/Bottom/Top edge of region */
	double inc[2];		/* Spacing between x and y bins */
	double i_bin_x;		/* 1/dx */
	double i_bin_y;		/* 1/dy */
	double time_gap;	/* We have a data-gap if two records differ by this amount in time */
	double dist_gap;	/* We have a data-gap if two records differ by this amount in distance [if there is no time column] */
	GMT_LONG nx_bin;	/* Number of x bins */
	GMT_LONG ny_bin;	/* Number of y bins */
	COUNTER_LARGE nm_bin;	/* Total number of bins */
	GMT_BOOLEAN periodic;	/* 1 if x is periodic */
	unsigned int *binflag;	/* The bin array */
	struct X2SYS_BIX_DATABASE *base;
	struct X2SYS_BIX_TRACK_INFO *head;
};

struct X2SYS_BIX_DATABASE {
	uint32_t bix;
	uint32_t n_tracks;
	struct X2SYS_BIX_TRACK *first_track, *last_track;
};

struct X2SYS_BIX_TRACK {
	uint32_t track_id;
	uint32_t track_flag;
	struct X2SYS_BIX_TRACK *next_track;
};
	
struct X2SYS_BIX_TRACK_INFO {
	char *trackname;
	uint32_t track_id;
	uint32_t flag;
	struct X2SYS_BIX_TRACK_INFO *next_info;
};

#define COE_X	0	/* Position of crossover x in data array */
#define COE_Y	1	/* Position of crossover y in data array */
#define COE_T	2	/* Time at crossover in data array */
#define COE_Z	3	/* Observed z value at crossover in data array */
#define COE_D	4	/* Distance at crossover in data array */
#define COE_H	5	/* Azimuth at crossover in data array */
#define COE_V	6	/* Velocity at crossover in data array */

struct X2SYS_COE {	/* Holds the information for a single crossover */
	double data[2][7];
};

struct X2SYS_COE_PAIR {	/* Holds the information for COE between a pair of tracks */
	char trk[2][GMT_TEXT_LEN64];	/* Track names */
	GMT_LONG year[2];		/* Start year for each track */
	COUNTER_MEDIUM id[2];		/* Internal ID track numbers */
	COUNTER_MEDIUM nx;		/* Number of crossovers */
	double start[2];		/* Time of first point for each track */
	double stop[2];			/* Time of last point for each track */
	double dist[2];			/* Length of each track */
	struct X2SYS_COE *COE;		/* Array of nx COE structures */
};

/* Global variables used by x2sys functions */

EXTERN_MSC char *X2SYS_program;
EXTERN_MSC char *X2SYS_HOME;
EXTERN_MSC double *x2sys_Y;
EXTERN_MSC struct MGD77_CONTROL M;

/* Function prototypes.  These can be accessed in user programs */

EXTERN_MSC FILE *x2sys_fopen (struct GMT_CTRL *C, char *fname, char *mode);
EXTERN_MSC GMT_LONG x2sys_access (struct GMT_CTRL *C, char *fname, int mode);
EXTERN_MSC void x2sys_path (struct GMT_CTRL *C, char *fname, char *path);

EXTERN_MSC GMT_LONG x2sys_read_record (struct GMT_CTRL *C, FILE *fp, double *data, struct X2SYS_INFO *s, struct GMT_IO *G);
EXTERN_MSC GMT_LONG x2sys_read_file        (struct GMT_CTRL *C, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *G, COUNTER_LARGE *n_rec);
EXTERN_MSC GMT_LONG x2sys_read_gmtfile     (struct GMT_CTRL *C, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *G, COUNTER_LARGE *n_rec);
EXTERN_MSC GMT_LONG x2sys_read_mgd77file   (struct GMT_CTRL *C, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *G, COUNTER_LARGE *n_rec);
EXTERN_MSC GMT_LONG x2sys_read_mgd77ncfile (struct GMT_CTRL *C, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *G, COUNTER_LARGE *n_rec);
EXTERN_MSC GMT_LONG x2sys_read_ncfile      (struct GMT_CTRL *C, char *fname, double ***data, struct X2SYS_INFO *s, struct X2SYS_FILE_INFO *p, struct GMT_IO *G, COUNTER_LARGE *n_rec);
EXTERN_MSC COUNTER_MEDIUM x2sys_n_data_cols (struct GMT_CTRL *C, struct X2SYS_INFO *s);
EXTERN_MSC GMT_LONG x2sys_read_list (struct GMT_CTRL *C, char *file, char ***list, COUNTER_MEDIUM *n);
EXTERN_MSC GMT_LONG x2sys_read_weights (struct GMT_CTRL *C, char *file, char ***list, double **weights, COUNTER_MEDIUM *nf);
EXTERN_MSC void x2sys_free_list (struct GMT_CTRL *C, char **list, COUNTER_LARGE n);
EXTERN_MSC GMT_LONG x2sys_find_track (struct GMT_CTRL *C, char *name, char **list, COUNTER_MEDIUM n);
GMT_LONG x2sys_get_tracknames (struct GMT_CTRL *C, struct GMT_OPTION *options, char ***tracklist, GMT_BOOLEAN *cmdline);

EXTERN_MSC double *x2sys_dummytimes (struct GMT_CTRL *C, COUNTER_LARGE n);

EXTERN_MSC void x2sys_skip_header (struct GMT_CTRL *C, FILE *fp, struct X2SYS_INFO *s);
EXTERN_MSC GMT_LONG x2sys_fclose (struct GMT_CTRL *C, char *fname, FILE *fp);
EXTERN_MSC void x2sys_free_info (struct GMT_CTRL *C, struct X2SYS_INFO *s);
EXTERN_MSC void x2sys_free_data (struct GMT_CTRL *C, double **data, COUNTER_MEDIUM n, struct X2SYS_FILE_INFO *p);
EXTERN_MSC GMT_LONG x2sys_pick_fields (struct GMT_CTRL *C, char *string, struct X2SYS_INFO *s);

EXTERN_MSC GMT_LONG x2sys_initialize (struct GMT_CTRL *C, char *TAG, char *fname, struct GMT_IO *G, struct X2SYS_INFO **I);
EXTERN_MSC void x2sys_end (struct GMT_CTRL *C, struct X2SYS_INFO *X);

EXTERN_MSC GMT_LONG x2sys_set_system (struct GMT_CTRL *C, char *TAG, struct X2SYS_INFO **s, struct X2SYS_BIX *B, struct GMT_IO *G);
EXTERN_MSC void x2sys_bix_init (struct GMT_CTRL *C, struct X2SYS_BIX *B, GMT_BOOLEAN alloc);
EXTERN_MSC struct X2SYS_BIX_TRACK_INFO *x2sys_bix_make_entry (struct GMT_CTRL *C, char *name, uint32_t id_no, uint32_t flag);
EXTERN_MSC struct X2SYS_BIX_TRACK *x2sys_bix_make_track (struct GMT_CTRL *C, uint32_t id, uint32_t flag);
EXTERN_MSC GMT_LONG x2sys_bix_read_tracks (struct GMT_CTRL *C, struct X2SYS_INFO *s, struct X2SYS_BIX *B, GMT_LONG mode, uint32_t *ID);
EXTERN_MSC GMT_LONG x2sys_bix_read_index (struct GMT_CTRL *C, struct X2SYS_INFO *s, struct X2SYS_BIX *B, GMT_BOOLEAN swap);
EXTERN_MSC GMT_LONG x2sys_bix_get_index (struct GMT_CTRL *C, double x, double y, GMT_LONG *i, GMT_LONG *j, struct X2SYS_BIX *B, COUNTER_LARGE *ID);

EXTERN_MSC void x2sys_path_init (struct GMT_CTRL *C, struct X2SYS_INFO *s);
EXTERN_MSC GMT_LONG x2sys_get_data_path (struct GMT_CTRL *C, char *track_path, char *track, char *suffix);
EXTERN_MSC GMT_LONG x2sys_err_pass (struct GMT_CTRL *C, GMT_LONG err, char *file);
EXTERN_MSC void x2sys_err_fail (struct GMT_CTRL *C, GMT_LONG err, char *file);
EXTERN_MSC const char * x2sys_strerror (struct GMT_CTRL *C, GMT_LONG err);

EXTERN_MSC COUNTER_LARGE x2sys_read_coe_dbase (struct GMT_CTRL *C, struct X2SYS_INFO *s, char *dbase, char *ignorefile, double *wesn, char *fflag, GMT_LONG coe_kind, char *one_trk, struct X2SYS_COE_PAIR **xpairs, COUNTER_LARGE *nx, COUNTER_LARGE *ntracks);
EXTERN_MSC void x2sys_free_coe_dbase (struct GMT_CTRL *C, struct X2SYS_COE_PAIR *P, COUNTER_LARGE np);
EXTERN_MSC void x2sys_get_corrtable (struct GMT_CTRL *C, struct X2SYS_INFO *s, char *table, COUNTER_LARGE ntracks, char **trk_name, char *column, struct MGD77_AUX_INFO *aux, struct MGD77_AUXLIST *auxlist, struct MGD77_CORRTABLE ***CORR);

#define X2SYS_ASCII		0
#define X2SYS_BINARY		1
#define X2SYS_NETCDF		2

#define X2SYS_NOERROR		0
#define X2SYS_FCLOSE_ERR	-1
#define X2SYS_BAD_DEF		-2
#define X2SYS_BAD_COL		-3
#define X2SYS_TAG_NOT_SET	-4
#define X2SYS_BAD_ARG		-5
#define X2SYS_CONFLICTING_ARGS	-6
#define X2SYS_BIX_BAD_ROW	-7
#define X2SYS_BIX_BAD_COL	-8
#define X2SYS_BIX_BAD_INDEX	-9

#define X2SYS_DIST_SELECTION	0
#define X2SYS_SPEED_SELECTION	1

#define X2SYS_bit(bit) (1U << (bit))	/* Set the specified bit to 1.  1,2,4,8 etc for 0,1,2,3... */
