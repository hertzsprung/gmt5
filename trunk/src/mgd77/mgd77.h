/*-------------------------------------------------------------------------
 *	$Id: mgd77.h,v 1.85 2008-03-22 11:55:36 guru Exp $
 * 
 *    Copyright (c) 2005-2008 by P. Wessel
 *    See README file for copying and redistribution conditions.
 *
 *  File:	mgd77.h
 *
 *  Include file for programs that plan to read/write MGD77[+] files
 *
 *  Authors:    Paul Wessel, Primary Investigator, SOEST, U. of Hawaii
 *		Michael Chandler, Master's Candidate, SOEST, U. of Hawaii
 *		
 *  Version:	1.3
 *  Revised:	15-MAR-2006
 * 
 *-------------------------------------------------------------------------*/

#ifndef _MGD77_H
#define _MGD77_H

#include "gmt.h"

#define MGD77_VERSION		"1.3"		/* Current version of MGD77 supplement */
#define MGD77_CDF_VERSION	"2006.04.15"	/* Current version of MGD77+ files created */
#define MGD77_RECORD_LENGTH	120		/* Length of MGD77 ASCII data records */
#define MGD77_HEADER_LENGTH	80		/* Length of MGD77 ASCII header records */
#define MGD77_N_HEADER_RECORDS	24		/* Number of MGD77 header records */
#define MGD77_METERS_PER_NM     1852		/* meters per nautical mile */
#define MGD77_METERS_PER_M      1609.344	/* meters per statute mile */
#define MGD77_OLDEST_YY		39		/* Y2K says YY < 39 is 20YY */
#define MGD77_N_DATA_FIELDS	27		/* Original 27 data columns in MGD77 */
#define MGD77_N_DATA_EXTENDED	28		/* The 27 plus time */
#define MGD77_N_NUMBER_FIELDS	24		/* Original 24 numerical data columns in MGD77 */
#define MGD77_N_STRING_FIELDS	3		/* Original 3 text data columns in MGD77 */
#define MGD77_N_HEADER_ITEMS	66		/* Number of individual header items in the MGD77 header */
/* The 28 MGD77 standard types (27 original + 1 conglomerate (time)) */
#define MGD77_RECTYPE		0
#define MGD77_TZ		1
#define MGD77_YEAR		2
#define MGD77_MONTH		3
#define MGD77_DAY		4
#define MGD77_HOUR		5
#define MGD77_MIN		6
#define MGD77_LATITUDE		7
#define MGD77_LONGITUDE		8
#define MGD77_PTC		9
#define MGD77_TWT		10
#define MGD77_DEPTH		11
#define MGD77_BCC		12
#define MGD77_BTC		13
#define MGD77_MTF1		14
#define MGD77_MTF2		15
#define MGD77_MAG		16
#define MGD77_MSENS		17
#define MGD77_DIUR		18
#define MGD77_MSD		19
#define MGD77_GOBS		20
#define MGD77_EOT		21
#define MGD77_FAA		22
#define MGD77_NQC		23
#define MGD77_ID		24
#define MGD77_SLN		25
#define MGD77_SSPN		26
#define MGD77_TIME		27

#define ALL_NINES               "9999999999"	/* Typical text value meaning no-data */
#define ALL_BLANKS "                      "	/* 32 blanks */
#define MGD77_NOT_SET		(-1)

#define MGD77_RESET_CONSTRAINT	1
#define MGD77_RESET_EXACT	2
#define MGD77_SET_ALLEXACT	4

#define MGD77_N_FORMATS		3
#define MGD77_FORMAT_M77	0
#define MGD77_FORMAT_CDF	1
#define MGD77_FORMAT_TBL	2
#define MGD77_FORMAT_ANY	3

#define MGD77_READ_MODE		0
#define MGD77_WRITE_MODE	1

#define MGD77_N_SETS		2
#define MGD77_M77_SET		0
#define MGD77_CDF_SET		1
#define MGD77_SET_COLS		32
#define MGD77_MAX_COLS		64

#define MGD77_FROM_HEADER	1
#define MGD77_TO_HEADER		2

#define MGD77_IGRF_F		0
#define MGD77_IGRF_H		1
#define MGD77_IGRF_X		2
#define MGD77_IGRF_Y		3
#define MGD77_IGRF_Z		4
#define MGD77_IGRF_D		5
#define MGD77_IGRF_I		6

#define MGD77_IGF_HEISKANEN	1
#define MGD77_IGF_1930		2
#define MGD77_IGF_1967		3
#define MGD77_IGF_1980		4

#define TWT_PDR_WRAP	10.0						/* The 10 second PDR wrap-around we see in SIO cruises */
#define TWT_PDR_WRAP_TRIGGER	0.5 * TWT_PDR_WRAP	/* Any jump in TWT that exceeds this triggers a wrap */

#define GMT_IMG_MINLAT -72.0059773539
#define GMT_IMG_MAXLAT +72.0059773539

#define MGD77_BAD_HEADER_RECNO		-1
#define MGD77_BAD_HEADER_ITEM		-2

/* Return error numbers */

#define MGD77_NO_ERROR			0
#define MGD77_FILE_NOT_FOUND		1
#define MGD77_ERROR_OPEN_FILE		2
#define MGD77_NO_HEADER_REC		3
#define MGD77_ERROR_READ_HEADER_ASC	4
#define MGD77_ERROR_WRITE_HEADER_ASC	5
#define MGD77_ERROR_READ_ASC_DATA	6
#define MGD77_ERROR_WRITE_ASC_DATA	7
#define MGD77_WRONG_HEADER_REC		8
#define MGD77_NO_DATA_REC		9
#define MGD77_WRONG_DATA_REC_LEN	10
#define MGD77_ERROR_CONV_DATA_REC	11
#define MGD77_ERROR_READ_HEADER_BIN	12
#define MGD77_ERROR_WRITE_HEADER_BIN	13
#define MGD77_ERROR_READ_BIN_DATA	14
#define MGD77_ERROR_WRITE_BIN_DATA	15
#define MGD77_ERROR_NOT_MGD77PLUS	16
#define MGD77_UNKNOWN_FORMAT		17
#define MGD77_UNKNOWN_MODE		18
#define MGD77_ERROR_NOSUCHCOLUMN	19
#define MGD77_BAD_ARG			20
#define MGD77_BAD_IGRFDATE		21

/* We will use bit flags to keep track of which data column we are referring to.
 * field 0 is rightmost bit (1), field 1 is the next bit (2), field 2 is 4 and
 * so on for powers of 2.  We add these bit powers together to get the bit patterns
 * for a selection of columns.  E.g., field 0, 3, 5 has pattern 1 + 8 + 32 = 41.
 * Some combinations of columns have precalculated bit patterns given below.
 * Note: all this only applies to the MGD77_DATA_RECORD structure & number positions.
 */

#define MGD77_TWT_BIT		(1 << 10)
#define MGD77_DEPTH_BIT		(1 << 11)
#define MGD77_MTF1_BIT		(1 << 14)
#define MGD77_MTF2_BIT		(1 << 15)
#define MGD77_MAG_BIT		(1 << 16)
#define MGD77_GOBS_BIT		(1 << 20)
#define MGD77_FAA_BIT		(1 << 22)
#define MGD77_GEOPHYSICAL_BITS	5360640	/* 0101 0001 1100 1100 0000 0000 */
#define MGD77_CORRECTION_BITS	2883584	/* 0010 1100 0000 0000 0000 0000 */
#define MGD77_TIME_BITS		    124	    /* 0000 0000 0000 0000 0111 1100 */
#define MGD77_FLOAT_BITS	(MGD77_GEOPHYSICAL_BITS + MGD77_CORRECTION_BITS + MGD77_TIME_BITS)
#define MGD77_STRING_BITS	(16777216+33554432+67108864)

/* Codes for the logical tests */

#define MGD77_EQ		1
#define MGD77_LT		2
#define MGD77_LE		3
#define MGD77_GT		4
#define MGD77_GE		5
#define MGD77_BIT		6
#define MGD77_NEQ		8

#define N_AUX	15		/* Number of auxilliary columns in mgd77list */

typedef char byte;	/* Used to indicate 1-byte long integer */
typedef char* Text;	/* Used to indicate character strings */
 
/* The MGD77 File format contains a header section consisting a set of 24 records
 * of length 80 characters each.  This information can be read and stored internally
 * in the structure MGD77_HEADER.  The two i/o functions MGD77_read_header and
 * MGD77_write_header will do exactly what they say.
 */

#define MGD77_COL_ABBREV_LEN	16
#define MGD77_COL_NAME_LEN	64
#define MGD77_COL_UNIT_LEN	64
#define MGD77_COL_COMMENT_LEN	128

struct MGD77_COLINFO {
	char *abbrev;		/* Short name that identifies this column */
	char *name;		/* Longer, descriptive name for column */
	char *units;		/* Units of the data type in this column */
	char *comment;		/* Comment regarding this data column */
	double factor;		/* factor to multiply data immediately after reading from file */
	double offset;		/* offset to add after reading and multiplying by scale */
	double corr_factor;	/* Extra correction factor/offset to follow scale/offset; */
	double corr_offset;	/* this is used to correct wrong units, etc. */
	double limit[2];	/* Lower and upper limits on this data column */
	GMT_LONG pos;		/* Position in output record [0 - n_columns-1]*/
	nc_type type;		/* Type of representation of this data in the netCDF file (NC_SHORT, NC_INT, NC_BYTE, etc) */
	char text;		/* length if this is a text string, else 0 */
	int var_id;		/* netCDF variable ID */
	BOOLEAN constant;	/* TRUE if column is constant and only 1 row is/should be stored */
	BOOLEAN present;	/* TRUE if column is present in the file (NaN or otherwise) */
};

struct MGD77_DATA_INFO {
	short n_col;					/* Number of active columns in this MGD77+ file */
	struct MGD77_COLINFO col[MGD77_SET_COLS];	/* List of info per extra column */
	unsigned int bit_pattern;			/* Up to 32 bit flags, one for each parameter desired */
};

struct MGD77_META {	/* Information about a cruise as derived from navigation data */
	BOOLEAN verified;	/* TRUE once MGD77_Verify_Prep has been called */
	GMT_LONG n_ten_box;		/* Number of 10x10 degree boxes visited by this cruise */
	GMT_LONG w, e, s, n;		/* Whole degree left/right/bottom/top coordinates */
	GMT_LONG Departure[3];	/* yyyy, mm, dd of departure */
	GMT_LONG Arrival[3];		/* yyyy, mm, dd of arrival */
	signed char ten_box[20][38];	/* Set to 1 for each box visited */
};

struct MGD77_HEADER {	
	struct MGD77_HEADER_PARAMS *mgd77;		/* See MGD-77 Documentation from NGDC for details */
	struct MGD77_META meta;				/* Holds some meta-data derived directly from data records */
	char *author;					/* Name of author of last creation/modification */
	char *history;					/* History of creation/modifications */
	char *E77;					/* Statement of E77 information encoded */
	GMT_LONG n_records;					/* Number of MGD77 data records found */
	GMT_LONG n_fields;					/* Number of columns returned */
	GMT_LONG errors[3];					/* Number of total errors, (warnings, errors) found when reading this header */
	BOOLEAN no_time;				/* TRUE for those few cruises that have no time values */
	struct MGD77_DATA_INFO info[MGD77_N_SETS];	/* Info regarding [0] standard MGD77 columns and [1] any extra columns (max 32 each) */
};


/* The data records in the MGD77 file consist of records that are 120 characters.
 * This information can be read and stored internally in the structure MGD77_DATA_RECORD.
 * The two i/o functions MGD77_read_record and MGD77_write_record will do exactly what they say.
 */

struct MGD77_DATA_RECORD {	/* See MGD77 Documentation from NGDC for details */
	/* This is the classic MGD77 portion of the data record */
	double number[MGD77_N_NUMBER_FIELDS];	/* 24 fields that express numerical values */
	double time;				/* Time using current GMT absolute time conventions (J2000 UTC) */
	char word[MGD77_N_STRING_FIELDS][10];	/* The 3 text strings in MGD77 records */
	unsigned int bit_pattern;		/* Bit pattern indicating which of the 27 fields are present in current record */
};

struct MGD77_DATASET {	/* Info for an entire MGD77+ data set */
	GMT_LONG n_fields;				/* Number of active colums in the values table */
	GMT_LONG errors;				/* Number of errors encountered when writing this data */
	struct MGD77_HEADER H;			/* The file's header information */
	void *values[MGD77_MAX_COLS];		/* 2-D table of necessary number of columns and rows (mix of double and char pointers) */
	unsigned int *flags[MGD77_N_SETS];	/* Optional arrays of custom error bit flags for each set */
};

struct MGD77_RECORD_DEFAULTS {
	char *fieldID;     /* variable names for the different MGD77 data fields */
	char *abbrev ;     /* acronyms for the 27 MGD77 data fields */
	GMT_LONG start;         /* beginning character number for each data field */
	GMT_LONG length;	   /* number of characters for each data field */
	char *fortranCode; /* data type specified in NGDC's MGD-77 Documentation */
	double factor;	   /* implied decimal factor specified by NGDC */
	char *readMGD77;   /* sscanf conversions for MGD-77 input */
	GMT_LONG order;	   /* MGD-77 specified data record order */
	char *printMGD77;  /* printf conversions for MGD-77 output  */
	char *printVALS;   /* printf conversions for printing converted values */
	char *not_given;   /* MGD77 representation of "no value given" */
};

struct MGD77_CONSTRAINT {
	char name[MGD77_COL_ABBREV_LEN];	/* Name of data col that is constrained */
	GMT_LONG col;				/* Number of data col that is constrained */
	GMT_LONG code;				/* Which test this is */
	BOOLEAN exact;				/* If TRUE we MUST pass this test */
	double d_constraint;			/* Value for testing */
	char c_constraint[GMT_TEXT_LEN];	/* String value for testing */
	PFB double_test;			/* Pointer to function performing the chosen limit test on a double */
	PFB string_test;			/* Pointer to function performing the chosen limit test on a string */
};

struct MGD77_PAIR {
	char name[MGD77_COL_ABBREV_LEN];	/* Name of data col that is to match exactly */
	GMT_LONG col;				/* Number of data col that is constrained */
	GMT_LONG match;				/* 1 if we want the bit to be 1 or 0 if we want it to be 0 to indicate a match*/
	GMT_LONG set, item;				/* Entries into corresponding info structure column */
};

struct MGD77_ORDER {	/* Info on a single desired output column */
	GMT_LONG set;	/* 0 for standard MGD77 data set, 1 for extra CDF columns */
	GMT_LONG item;	/* Position in the H.info[set] column array */
};

struct MGD77_CONTROL {
	/* Programs that need to write out MGD77 data columns in a certain order will need
	 * to declare this structure and use the MGD77_Init function to get going
	 */
	
	/* File path information */
	char *MGD77_HOME;				/* Directory where paths are stored [$GMT_SHAREDIR/mgd77] */
	char **MGD77_datadir;				/* Directories where MGD77 data may live */
	GMT_LONG n_MGD77_paths;				/* Number of such directories */
	char user[MGD77_COL_ABBREV_LEN];		/* Current user id */
	char NGDC_id[MGD77_COL_ABBREV_LEN];		/* Current NGDC file tag id */
	char path[BUFSIZ];				/* Full path to current file */
	FILE *fp;					/* File pointer to current open file (not used by MGD77+) */
	GMT_LONG verbose_level;				/* 0 = none, 1 = warnings, 2 = errors (combined 3 for both) */
	GMT_LONG verbose_dest;				/* 1 = to stdout, 2 = to stderr */
	int nc_id;					/* netCDF ID for current open file (MGD77+ only) */
	int nc_recid;					/* netCDF ID for dimension of records (time) */
	GMT_LONG rec_no;					/* Current record to read/write for record-based i/o */
	GMT_LONG format;					/* 0 if any file format, 1 if MGD77, and 2 if netCDF, 3 if ascii table */
	/* Format-related issues */
	GMT_LONG time_format;				/* Either GMT_IS_ABSTIME or GMT_IS_RELTIME */
	/* Data use information */
	BOOLEAN original;				/* TRUE means we want original not revised header attributes */
	BOOLEAN Want_Header_Item[MGD77_N_HEADER_ITEMS];	/* TRUE means print this header item if dump is selected */
	BOOLEAN use_flags[MGD77_N_SETS];		/* TRUE means programs will use error bitflags (if present) when returning data */
	BOOLEAN use_corrections[MGD77_N_SETS];		/* TRUE means we will apply correction factors (if present) when reading data */
	struct MGD77_ORDER order[MGD77_MAX_COLS];	/* Gives the output order (set, item) of each desired column */
	unsigned int bit_pattern[2];			/* 64 bit flags, one for each parameter desired */
	GMT_LONG n_constraints;				/* Number of constraints specified */
	GMT_LONG n_exact;					/* Number of exact columns to match */
	GMT_LONG n_bit_tests;				/* Number of bit tests to match */
	GMT_LONG no_checking;				/* TRUE if there are no constraints, extact-tests, or bit-tests to pass */
	struct MGD77_CONSTRAINT Constraint[MGD77_MAX_COLS];		/* List of constraints, if any */
	char desired_column[MGD77_MAX_COLS][MGD77_COL_ABBREV_LEN];	/* List of desired column names in final output order */
	struct MGD77_PAIR Exact[MGD77_MAX_COLS];	/* List of column names whose values must be !NaN to be output, if any */
	struct MGD77_PAIR Bit_test[MGD77_MAX_COLS];	/* List of bit-tests, if any */
	GMT_LONG n_out_columns;				/* Number of output columns requested */
};

#define N_CARTER_BINS 64800             /* Number of 1x1 degree bins */
#define N_CARTER_ZONES 85               /* Number of Carter zones */
#define N_CARTER_OFFSETS 86             /* Number of Carter offsets */
#define N_CARTER_CORRECTIONS 5812       /* Number of Carter corrections */

struct MGD77_CARTER {
	GMT_LONG initialized;
	short int carter_zone[N_CARTER_BINS];
	short int carter_offset[N_CARTER_OFFSETS];
	short int carter_correction[N_CARTER_CORRECTIONS];
};

/* Structures for emphemeral corrections */

struct MGD77_CORRECTION {	/* Holds parameters for one term of a correction for one kind of observation */
	GMT_LONG id;			/* The id - entry to give us the data column to use*/
	double factor;		/* Amplitude to multiply the basis function [1] */
	double origin;		/* Local origin to subtract from argument [0] */
	double scale;		/* Scale to apply to (value - origin) */
	double power;		/* Power we should raise the argument to [1] */
	PFD modifier;		/* Pointer to function that will modify argument */
	struct MGD77_CORRECTION *next;
};

struct MGD77_CORRTABLE {
	struct MGD77_CORRECTION *term;
};

/* Primary user functions */

extern void MGD77_Init (struct MGD77_CONTROL *F);						/* Initialize the MGD77 machinery */
extern void MGD77_Reset (struct MGD77_CONTROL *F);									/* Reset after finishing a file */
extern GMT_LONG MGD77_Path_Expand (struct MGD77_CONTROL *F, char **argv, int argc, char ***list);				/* Returns the full list of IDs */
extern void MGD77_Cruise_Explain (void);										/* Explains how to specify IDs */
extern GMT_LONG MGD77_Get_Path (char *track_path, char *track, struct MGD77_CONTROL *F);					/* Returns full path to cruise */
extern GMT_LONG MGD77_Open_File (char *leg, struct MGD77_CONTROL *F, GMT_LONG rw);						/* Opens a MGD77[+] file */
extern GMT_LONG MGD77_Close_File (struct MGD77_CONTROL *F);									/* Closes a MGD77[+] file */
extern GMT_LONG MGD77_Read_File (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);				/* Allocate & Read entire file (selected columns only) */
extern GMT_LONG MGD77_Write_File (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);				/* Write entire file (all columns) */
extern GMT_LONG MGD77_Read_Header_Record (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H);			/* Read the header record */
extern GMT_LONG MGD77_Write_Header_Record (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H);			/* Write the header record */
extern GMT_LONG MGD77_Read_Data (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);				/* Allocate & Read all data (selected columns only); Header already read */
extern GMT_LONG MGD77_Write_Data (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);				/* Write all data (all columns); Header already written */
extern GMT_LONG MGD77_Read_Data_Record (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, double dvals[], char *tvals[]);	/* Read a single data record (selected columns only) */
extern GMT_LONG MGD77_Write_Data_Record (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, double dvals[], char *tvals[]);	/* Write a single data record (selected columns only) */
extern void MGD77_Free (struct MGD77_DATASET *S);									/* Free memory allocated by MGD77_Read_File/MGD77_Read_Data */
extern void MGD77_Select_Columns (char *string, struct MGD77_CONTROL *F, GMT_LONG option);					/* Decode the -F option specifying the desired columns */
extern GMT_LONG MGD77_Get_Column (char *word, struct MGD77_CONTROL *F);							/* Get column number from column name (or -1 if not present) */
extern GMT_LONG MGD77_Info_from_Abbrev (char *name, struct MGD77_HEADER *H, GMT_LONG *set, GMT_LONG *item);
extern void MGD77_List_Header_Items (struct MGD77_CONTROL *F);
extern GMT_LONG MGD77_Select_Header_Item (struct MGD77_CONTROL *F, char *item);
extern GMT_LONG MGD77_Get_Set (char *abbrev);										/* Returns 0 if abbrev is in the MGD77 set, else 1 */
extern void MGD77_Fatal_Error (GMT_LONG error);										/* Print message for this error and exit */
extern BOOLEAN MGD77_Pass_Record (struct MGD77_CONTROL *F, struct MGD77_DATASET *S, GMT_LONG rec);				/* Tests if a record passes all specified logical & exact tests */
extern void MGD77_Apply_Bitflags (struct MGD77_CONTROL *F, struct MGD77_DATASET *S, GMT_LONG rec, BOOLEAN apply_bits[]);	/* Replaces values whose flags are ON with NaNs */
extern void MGD77_Set_Unit (char *dist, double *scale, GMT_LONG way);							/* Convert appended distance unit to a numerical scale to give meters */
extern void MGD77_nc_status (GMT_LONG status);										/* Checks for netCDF errors and aborts with error message */
extern void MGD77_Process_Ignore (char code, char *format);								/* Process the ignre-format option */
extern void MGD77_Ignore_Format (GMT_LONG format);										/* Dissallow some formats for consideration */
extern struct MGD77_DATASET *MGD77_Create_Dataset ();									/* Create an empty data set structure */
extern void MGD77_Prep_Header_cdf (struct MGD77_CONTROL *F, struct MGD77_DATASET *S);					/* Prepare header before we write */
extern void MGD77_Dump_Header_Params (struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS *P);				/* Dump of header items, one per line */
extern void MGD77_Verify_Header (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, FILE *ufp);				/* Verify content of header per MGD77 docs */
extern void MGD77_Verify_Prep (struct MGD77_CONTROL *F, struct MGD77_DATASET *D);
extern void MGD77_Verify_Prep_m77 (struct MGD77_CONTROL *F, struct MGD77_META *C, struct MGD77_DATA_RECORD *D, GMT_LONG nrec);
extern GMT_LONG MGD77_Remove_E77 (struct MGD77_CONTROL *F);

/* Secondary user functions */

extern GMT_LONG MGD77_Read_Header_Record_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H);			/* Hardwired read of ascii/MGD77 header */
extern GMT_LONG MGD77_Read_Data_Record_m77 (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record);			/* Hardwired read of ascii/MGD77 data record */
extern GMT_LONG MGD77_Write_Header_Record_m77 (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H);			/* Hardwired write of ascii/MGD77 header */
extern GMT_LONG MGD77_Write_Data_Record_m77 (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record);		/* Hardwired write of ascii/MGD77 data record */

/* These are only for developers */

extern BOOLEAN MGD77_dbl_are_constant (double x[], GMT_LONG n, double limits[]);
extern BOOLEAN MGD77_txt_are_constant (char *txt, GMT_LONG n, GMT_LONG width);
extern GMT_LONG MGD77_do_scale_offset_before_write (double new[], const double x[], GMT_LONG n, double scale, double offset, GMT_LONG type);
extern void MGD77_select_high_resolution ();

/* User functions for direct use of Carter corrections */

extern GMT_LONG MGD77_carter_depth_from_twt (GMT_LONG zone, double twt_in_msec, struct MGD77_CARTER *C, double *depth_in_corr_m);
extern GMT_LONG MGD77_carter_twt_from_depth (GMT_LONG zone, double depth_in_corr_m, struct MGD77_CARTER *C, double *twt_in_msec);
extern GMT_LONG MGD77_carter_depth_from_xytwt (double lon, double lat, double twt_in_msec, struct MGD77_CARTER *C, double *depth_in_corr_m);
extern GMT_LONG MGD77_carter_twt_from_xydepth (double lon, double lat, double depth_in_corr_m, struct MGD77_CARTER *C, double *twt_in_msec);
extern double MGD77_carter_correction (double lon, double lat, double twt_in_msec, struct MGD77_CARTER *C);

/* User functions for direct use of IGRF corrections, theoretical gravity */

extern GMT_LONG MGD77_igrf10syn (GMT_LONG isv, double date, GMT_LONG itype, double alt, double lon, double lat, double *out);
extern double MGD77_Theoretical_Gravity (double lon, double lat, GMT_LONG version);
extern void MGD77_IGF_text (FILE *fp, GMT_LONG version);

/* These are called indirectly but remain accessible for specialist programs */

extern GMT_LONG MGD77_carter_init (struct MGD77_CARTER *C);
extern GMT_LONG MGD77_carter_get_bin (double lon, double lat, GMT_LONG *bin);
extern GMT_LONG MGD77_carter_get_zone (GMT_LONG bin, struct MGD77_CARTER *C, GMT_LONG *zone);
extern double *MGD77_Distances (double x[], double y[], GMT_LONG n, GMT_LONG dist_flag);

/* Global variables used by MGD77 programs */

extern struct MGD77_RECORD_DEFAULTS mgd77defs[MGD77_N_DATA_FIELDS];
extern double MGD77_NaN_val[7], MGD77_Low_val[7], MGD77_High_val[7];
extern char *MGD77_suffix[MGD77_N_FORMATS];
extern BOOLEAN MGD77_format_allowed[MGD77_N_FORMATS];	/* By default we allow opening of files in any format.  See MGD77_Ignore_Format() */
extern double MGD77_Epoch_zero;
extern GMT_LONG MGD77_pos[MGD77_N_DATA_EXTENDED];

void MGD77_Parse_Corrtable (struct MGD77_CONTROL *F, char *tablefile, char **cruises, GMT_LONG n_cruises, struct MGD77_CORRTABLE ***CORR);
void MGD77_Init_Correction (struct MGD77_CORRTABLE *CORR, double **value);
double MGD77_Correction (struct MGD77_CORRECTION *C, double **value, double *aux, GMT_LONG rec);

#include "mgd77_functions.h"	/* These were created by mgd77netcdfhelper.sh */

#endif	/* _MGD77_H */
