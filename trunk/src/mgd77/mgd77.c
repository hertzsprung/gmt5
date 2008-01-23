/*---------------------------------------------------------------------------
 *	$Id: mgd77.c,v 1.164 2008-01-23 03:22:49 guru Exp $
 *
 *    Copyright (c) 2005-2008 by P. Wessel
 *    See README file for copying and redistribution conditions.
 *
 *  File:	mgd77.c
 *
 *  Function library for programs that plan to read/write MGD77[+] files
 *
 *  Authors:    Paul Wessel, Primary Investigator, SOEST, U. of Hawaii
 *		Michael Chandler, Master's Candidate, SOEST, U. of Hawaii
 *		
 *  Version:	1.2
 *  Revised:	1-MAR-2006
 * 
 *-------------------------------------------------------------------------*/



#include "mgd77.h"
#include "mgd77_IGF_coeffs.h"
#include "mgd77_init.h"
#ifndef WIN32
#include <dirent.h>
#endif

#define MGD77_CDF_CONVENTION	"CF-1.0"	/* MGD77+ files are CF-1.0 and hence COARDS-compliant */

#define MGD77_COL_ORDER "#rec\tTZ\tyear\tmonth\tday\thour\tmin\tlat\t\tlon\t\tptc\ttwt\tdepth\tbcc\tbtc\tmtf1\tmtf2\tmag\tmsens\tdiur\tmsd\tgobs\teot\tfaa\tnqc\tid\tsln\tsspn\n"

struct MGD77_MAG_RF {
	char *model;        /* Reference field model name */
	int code;           /* Reference field code       */
	int start;          /* Model start year           */
	int end;            /* Model end year             */
};

#define MGD77_N_MAG_RF 13
struct MGD77_MAG_RF mgd77rf[MGD77_N_MAG_RF] = {
#include "mgd77magref.h"
};

/* PRIVATE FUNCTIONS TO MGD77.C */

void MGD77_Set_Home (struct MGD77_CONTROL *F);
void MGD77_Init_Columns (struct MGD77_CONTROL *F, struct MGD77_HEADER *H);
void MGD77_Path_Init (struct MGD77_CONTROL *F);
BOOLEAN MGD77_lt_test (double value, double limit);
BOOLEAN MGD77_le_test (double value, double limit);
BOOLEAN MGD77_eq_test (double value, double limit);
BOOLEAN MGD77_bit_test (double value, double limit);
BOOLEAN MGD77_neq_test (double value, double limit);
BOOLEAN MGD77_gt_test (double value, double limit);
BOOLEAN MGD77_ge_test (double value, double limit);
BOOLEAN MGD77_clt_test (char *value, char *match, int len);
BOOLEAN MGD77_cle_test (char *value, char *match, int len);
BOOLEAN MGD77_ceq_test (char *value, char *match, int len);
BOOLEAN MGD77_cneq_test (char *value, char *match, int len);
BOOLEAN MGD77_cgt_test (char *value, char *match, int len);
BOOLEAN MGD77_cge_test (char *value, char *match, int len);
int MGD77_Read_Header_Record_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H);
int MGD77_Write_Header_Record_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H);
int MGD77_Read_Header_Record_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H);
int MGD77_Read_Data_Record_m77 (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *H);
int MGD77_Write_Data_Record_m77 (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *H);
int MGD77_Read_Data_Record_cdf (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, double dvals[], char *tvals[]);
int MGD77_Write_Data_Record_cdf (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, double dvals[], char *tvals[]);
int MGD77_Read_Data_Record_tbl (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record);	 /* Will read a single tabular MGD77 record */
int MGD77_Write_Data_Record_tbl (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record);	 /* Will read a single tabular MGD77 record */
int MGD77_Write_Header_Record_m77 (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H);
BOOLEAN MGD77_txt_are_constant (char *txt, int n, int width);
BOOLEAN MGD77_dbl_are_constant (double x[], int n, double limits[2]);
char *MGD77_cp_txt (char *txt);
char *MGD77_alloc_txt (int len);
void MGD77_do_scale_offset_after_read (double x[], int n, double scale, double offset, double nan_val);
int MGD77_do_scale_offset_before_write (double new[], const double x[], int n, double scale, double offset, int type);
void MGD77_set_plain_mgd77 (struct MGD77_HEADER *H);
void MGD77_Select_All_Columns (struct MGD77_CONTROL *F, struct MGD77_HEADER *H);
int MGD77_Read_Data_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);
int MGD77_Read_Data_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);
int MGD77_Write_Data_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);
int MGD77_Write_Data_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);
int MGD77_Order_Columns (struct MGD77_CONTROL *F, struct MGD77_HEADER *H);
int MGD77_Convert_To_New_Format (char *line);
int MGD77_Decode_Header (struct MGD77_HEADER_PARAMS *P, char *record[], int dir);
void MGD77_Place_Text (int dir, char *struct_member, char *header_record, int start_pos, int n_char);
BOOLEAN MGD77_entry_in_MGD77record (char *name, int *entry);
int MGD77_Find_Cruise_ID (char *name, char **cruises, int n_cruises);
double MGD77_Sind (double z);
double MGD77_Cosd (double z);
double MGD77_Copy (double z);
int wrong_filler (char *field, int length);

struct MGD77_DATA_RECORD *MGD77Record;
 
double MGD77_NaN_val[7], MGD77_Low_val[7], MGD77_High_val[7];
int MGD77_pos[MGD77_N_DATA_EXTENDED];	/* Used to translate the positions 0-27 into MGD77_TIME, MGD77_LONGITUDE, etc */
struct MGD77_LIMITS {
	double limit[2];	/* Upper and lower range */
} mgd77_range[MGD77_N_DATA_EXTENDED];

struct MGD77_RECORD_DEFAULTS mgd77defs[MGD77_N_DATA_FIELDS] = {
#include "mgd77defaults.h"
};

struct MGD77_cdf {
	int type;		/* netCDF variable type */
	int len;		/* # of characters (if text), 1 otherwise */
	double factor;		/* scale to multiply stored data to get correct magnitude */
	double offset;		/* offset to add after multiplication */
	char *units;		/* Units of this data */
	char *comment;		/* Comments regarding this data */
};

struct MGD77_cdf mgd77cdf[MGD77_N_DATA_EXTENDED] = {
/* DRT */	{ NC_BYTE,	1,	1.0,	0.0, "", "Normally 5" },
/* TZ */	{ NC_BYTE,	1,	1.0,	0.0, "hours", "-13 to +12 inclusive" },
/* YEAR */	{ NC_BYTE,	1,	1.0,	0.0, "year", "Year of the survey" },
/* MONTH */	{ NC_BYTE,	1,	1.0,	0.0, "month", "1 to 12 inclusive" },
/* DAY */	{ NC_BYTE,	1,	1.0,	0.0, "day", "1 to 31 inclusive" },
/* HOUR */	{ NC_BYTE,	1,	1.0,	0.0, "hour", "0 to 23 inclusive" },
/* MIN */	{ NC_BYTE,	1,	1.0,	0.0, "min", "Decimal minutes with 0.001 precision, 0 to 59.999" },
/* LAT */	{ NC_INT,	1,	1.0e-7,	0.0, "degrees_north", "Negative south of Equator" },	/* 1e-7 gives < 1 cm precision in position */
/* LON */	{ NC_INT,	1,	2.0e-7,	0.0, "degrees_east", "Negative west of Greenwich" },	/* 2e-7 gives <=2.2 cm precision in position */
/* PTC */	{ NC_BYTE,	1,	1.0,	0.0, "", "Observed (1), Interpolated (3), or Unspecified (9)" },
/* TWT */	{ NC_INT,	1,	1.0e-8,	0.0, "second", "Corrected for transducer depth, etc." },	/* 1e-8 s precision implies < 10 ns twt precision ~ 7.5 um */
/* DEPTH */	{ NC_INT,	1,	1.0e-5,	0.0, "meter", "Corrected for sound velocity variations (if known)" },	/* 1e-5m is 0.01 mm precision */
/* BCC */	{ NC_BYTE,	1,	1.0,	0.0, "", "01-55 (= Matthew's zone), 59 (Matthew's zone unknown), 60 (Kuwahara), 61 (Wilson), 62 (Del Grosso) 63 (Carter), 88 (Other; see header), 98 (Unknown), or 99 (Unspecified)" },
/* BTC */	{ NC_BYTE,	1,	1.0,	0.0, "", "Observed (1), Interpolated (3), or Unspecified (9)" },
/* MTF1 */	{ NC_INT,	1,	1.0e-4,	0.0, "gamma", "Leading sensor" },	/* 1e-4 nTesla is 100 fTesla precision */
/* MTF2 */	{ NC_INT,	1,	1.0e-4,	0.0, "gamma", "Trailing sensor" },
/* MAG */	{ NC_SHORT,	1,	1.0e-1,	0.0, "gamma", "Corrected for reference field (see header)" },	/* 0.1 nTesla precision */
/* MSENS */	{ NC_BYTE,	1,	1.0,	0.0, "", "Magnetic sensor used: 1, 2, or Unspecified (9)" },
/* DIUR */	{ NC_SHORT,	1,	1.0e-1,	0.0, "gamma", "Already applied to data" },	/* 0.1 nTesla precision */
/* MSD */	{ NC_SHORT,	1,	1.0,	0.0, "meter", "Positive below sealevel" },	/* 1 m precision */
/* GOBS */	{ NC_INT,	1,	1.0e-5,	980000.0, "mGal", "Corrected for Eotvos, drift, and tares" },	/* 1e-5 is 10 nGal precision */
/* EOT */	{ NC_SHORT,	1,	1.0e-1,	0.0, "mGal", "7.5 V cos (lat) sin (azim) + 0.0042 V*V" },	/* 1e-1 is 0.1 mGal precision */
/* FAA */	{ NC_SHORT,	1,	1.0e-1,	0.0, "mGal", "Observed - theoretical" },
/* NQC */	{ NC_BYTE,	1,	1.0,	0.0, "", "Suspected by (5) source agency, (6) NGDC, or no problems found (9)" },
/* ID */	{ NC_BYTE,	8,	1.0,	0.0, "", "Identical to ID in header" },
/* SLN */	{ NC_BYTE,	5,	1.0,	0.0, "", "For cross-referencing with seismic data" },
/* SSPN */	{ NC_BYTE,	6,	1.0,	0.0, "", "For cross-referencing with seismic data" },
/* GMT_TIME */	{ NC_DOUBLE,	1,	1.0,	0.0, "seconds since 1970-01-01 00:00:00 0", "UTC time, subtract TZ to get ship local time" }
};

char *aux_names[N_AUX] = {
	"year",
	"month",
	"day",
	"hour",
	"min",
	"sec",
	"dist",
	"azim",
	"vel",
	"weight",
	"drt",
	"igrf",
	"carter",
	"ngrav",
	"ngdcid",
};

PFB MGD77_column_test_double[9];
PFB MGD77_column_test_string[9];
unsigned int MGD77_this_bit[MGD77_SET_COLS];

int MGD77_Write_File_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);
int MGD77_Write_File_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);
int MGD77_Read_File_Binary (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);
int MGD77_Read_File_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);
int MGD77_Read_File_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S);
int MGD77_Read_Header_Sequence (FILE *fp, char *record, int seq);
int MGD77_Read_Data_Sequence (FILE *fp, char *record);
int MGD77_Write_Header_Record_New (FILE *fp, struct MGD77_HEADER *H, int format);
void MGD77_Write_Sequence (FILE *fp, int seq);
int MGD77_Info_from_Abbrev (char *name, struct MGD77_HEADER *H, int *set, int *item);
int get_quadrant (int x, int y);

#include "mgd77_functions.c"	/* Get netCDF MGD77 header attribute i/o functions */

int MGD77_Param_Key (int record, int item) {
	int i, status = MGD77_BAD_HEADER_RECNO;
	/* Given record and item, return the structure array key that matches these two values.
	 * If not found return BAD_HEADER if record is outside range, or BAD_ITEM if no such item */
	 
	if (record < 0 || record > 24) return (MGD77_BAD_HEADER_RECNO);	/* Outside range */
	if (record >= 18) record = 18;	/* Special processing for 18-24 */
	
	for (i = 0; status < 0 && i < MGD77_N_HEADER_PARAMS; i++) {
		if (MGD77_Header_Lookup[i].record != record) continue;
		status = MGD77_BAD_HEADER_ITEM;
		if (MGD77_Header_Lookup[i].item != item) continue;
		status = i;
	}
	return (status);
}

void MGD77_select_high_resolution ()
{
	/* If it becomes necessary to store mag, diur, faa, and eot using 4-byte integers we modify
	 * these entries in the mgd77cdf structure array.
	 */
	 
	mgd77cdf[16].type = mgd77cdf[18].type = NC_INT;		/* MAG & DIUR:  4-byte integer with 100 fTesla precision */
	mgd77cdf[16].factor = mgd77cdf[18].factor = 1.0e-4;
	mgd77cdf[21].type = mgd77cdf[22].type = NC_INT;		/* EOT & FAA :  4-byte integer with 10 nGal precision */
	mgd77cdf[21].factor = mgd77cdf[22].factor = 1.0e-5;
	mgd77cdf[19].type = NC_INT;				/* MSD : 	4-byte integer with 0.01 mm precision */
	mgd77cdf[19].factor = 1.0e-5;
}

int MGD77_Write_File (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)
{
	int err = 0;
	
	switch (F->format) {
		case MGD77_FORMAT_M77:	/* Plain MGD77 file */
			err = MGD77_Write_File_asc (file, F, S);
			break;
		case MGD77_FORMAT_CDF:	/* netCDF MGD77 file */
			err = MGD77_Write_File_cdf (file, F, S);
			break;
		case MGD77_FORMAT_TBL:	/* Plain ascii table */
			err = MGD77_Write_File_asc (file, F, S);
			break;
		default:
			fprintf (stderr, "%s: Bad format (%d)!\n", GMT_program, F->format);
			GMT_exit (EXIT_FAILURE);
	}
	return (err);
}

int MGD77_Read_File (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)
{
	int err = 0;
	
	switch (F->format) {
		case MGD77_FORMAT_M77:	/* Plain MGD77 file */
		case MGD77_FORMAT_TBL:	/* Plain ascii table */
			err = MGD77_Read_File_asc (file, F, S);
			break;
		case MGD77_FORMAT_CDF:	/* netCDF MGD77 file */
			err = MGD77_Read_File_cdf (file, F, S);
			break;
		default:
			fprintf (stderr, "%s: Bad format (%d)!\n", GMT_program, F->format);
			err = MGD77_UNKNOWN_FORMAT;
	}
	return (err);
}

int MGD77_Write_Data (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)
{
	int err = 0;
	
	switch (F->format) {
		case MGD77_FORMAT_M77:	/* Plain MGD77 file */
		case MGD77_FORMAT_TBL:	/* Plain ascii table */
			err = MGD77_Write_Data_asc (file, F, S);
			break;
		case MGD77_FORMAT_CDF:	/* netCDF MGD77 file */
			err = MGD77_Write_Data_cdf (file, F, S);
			break;
		default:
			fprintf (stderr, "%s: Bad format (%d)!\n", GMT_program, F->format);
			err = MGD77_UNKNOWN_FORMAT;
	}
	return (err);
}

int MGD77_Read_Data (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)
{
	int err = 0;
	
	switch (F->format) {
		case MGD77_FORMAT_M77:	/* Plain MGD77 file */
		case MGD77_FORMAT_TBL:	/* Plain ascii table */
			err = MGD77_Read_Data_asc (file, F, S);
			break;
		case MGD77_FORMAT_CDF:	/* netCDF MGD77 file */
			err = MGD77_Read_Data_cdf (file, F, S);
			break;
		default:
			fprintf (stderr, "%s: Bad format (%d)!\n", GMT_program, F->format);
			err = MGD77_UNKNOWN_FORMAT;
	}
	return (err);
}

int MGD77_Open_File (char *leg, struct MGD77_CONTROL *F, int rw)  /* Opens a MGD77[+] file */
{	/* leg:		Prefix, Prefix.Suffix, or Path/Prefix.Suffix for a MGD77[+] file
	 * F		Pointer to MGD77 Control structure
	 * rw		0  for read or 1 for write.
	 */
	
	int start, stop;
	char mode[2];
	
	mode[1] = '\0';	/* Thus mode will be a 1-char string */
	
	if (rw == MGD77_READ_MODE) {	/* Reading a file */
		mode[0] = 'r';
		if (MGD77_Get_Path (F->path, leg, F)) {
   			fprintf (stderr, "%s : Cannot find leg %s\n", GMT_program, leg);
     			return (MGD77_FILE_NOT_FOUND);
  		}
	}
	else if (rw == MGD77_WRITE_MODE) {		/* Writing to a new file; leg is assumed to be complete name */
		int k, has_suffix = MGD77_NOT_SET;
		if (F->format == MGD77_FORMAT_ANY || F->format == MGD77_NOT_SET) {
			fprintf (stderr, "%s: Format type not set for output file %s\n", GMT_program, leg);
			return (MGD77_ERROR_OPEN_FILE);
		}
		mode[0] = 'w';
		for (k = 0; k < MGD77_FORMAT_ANY; k++) {	/* Determine if given leg name contains one of the 3 possible extensions */
			if ((strlen(leg)-strlen(MGD77_suffix[k])) > 0 && !strncmp (&leg[strlen(leg)-strlen(MGD77_suffix[k])], MGD77_suffix[k], strlen(MGD77_suffix[k]))) has_suffix = k;
		}
		if (has_suffix == MGD77_NOT_SET)	/* file name given without extension */
			sprintf (F->path, "%s.%s", leg, MGD77_suffix[F->format]);
		else
			strcpy (F->path, leg);
	}
	else
		return (MGD77_UNKNOWN_MODE);
	
	/* For netCDF format we do not open file - this is done differently later */
	
	if (F->format != MGD77_FORMAT_CDF && (F->fp = GMT_fopen (F->path, mode)) == NULL) {
		fprintf (stderr, "%s: Could not open %s\n", GMT_program, F->path);
		return (MGD77_ERROR_OPEN_FILE);
	}

	/* Strip out Prefix and store in control structure */
	
	start = stop = MGD77_NOT_SET;
	for (start = strlen (F->path) - 1; stop == MGD77_NOT_SET && start > 0; start--) if (F->path[start] == '.') stop = start;
	while (start >= 0 && F->path[start] != '/') start--;
	start++;
	strncpy (F->NGDC_id, &F->path[start], stop - start);
	F->NGDC_id[stop - start] = '\0';
	
	return (MGD77_NO_ERROR);
}

int MGD77_Close_File (struct MGD77_CONTROL *F)  /* Closes a MGD77[+] file */
{
	int error;

	switch (F->format) {
		case MGD77_FORMAT_M77:	/* These are accessed by file pointer */
		case MGD77_FORMAT_TBL:
			if (!F->fp) return (MGD77_NO_ERROR);	/* No file open */
			error = GMT_fclose (F->fp);
			break;
		case MGD77_FORMAT_CDF:	/* netCDF file is accessed by ID*/
			MGD77_nc_status (nc_close (F->nc_id));
			error = 0;
			break;
		default:
			error = MGD77_UNKNOWN_FORMAT;
			break;
	}
	
	return (error);
}

int MGD77_Read_Header_Record (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H)
{	/* Reads the header structgure form a MGD77[+] file */
	int error;
	
	switch (F->format) {
		case MGD77_FORMAT_M77:	/* Will read MGD77 headers from MGD77 files or ascii tables */
		case MGD77_FORMAT_TBL:
			error = MGD77_Read_Header_Record_asc (file, F, H);
			break;
		case MGD77_FORMAT_CDF:	/* Will read MGD77 headers from a netCDF file */
			error = MGD77_Read_Header_Record_cdf (file, F, H);
			break;
		default:
			error = MGD77_UNKNOWN_FORMAT;
			break;
	}
	
	MGD77_Init_Ptr (MGD77_Header_Lookup, H->mgd77);	/* set pointers */
	
	return (error);
}

int MGD77_Write_Header_Record (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H)
{	/* Writes the header structgure to a MGD77[+] file */
	int error;
	
	switch (F->format) {
		case MGD77_FORMAT_M77:	/* Will write MGD77 headers from MGD77 files or ascii tables */
			error = MGD77_Write_Header_Record_m77 (file, F, H);
			break;
		case MGD77_FORMAT_TBL:
			error = MGD77_Write_Header_Record_m77 (file, F, H);
			fprintf (F->fp, MGD77_COL_ORDER);
			break;
		case MGD77_FORMAT_CDF:	/* Will read MGD77 headers from a netCDF file */
			error = MGD77_Write_Header_Record_cdf (file, F, H);
			break;
		default:
			error = MGD77_UNKNOWN_FORMAT;
			break;
	}
	
	return (error);
}

int MGD77_Read_Data_Record (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, double dvals[], char *tvals[])
{	/* Reads a single data record into floating point and char string arrays */
	int i, k, error;
	struct MGD77_DATA_RECORD MGD77Record;

	switch (F->format) {
		case MGD77_FORMAT_M77:		/* Will read a single MGD77 record */
			error = MGD77_Read_Data_Record_m77 (F, &MGD77Record);
			dvals[0] = MGD77Record.time;
			for (i = 1; i < MGD77_N_NUMBER_FIELDS; i++) dvals[i] = MGD77Record.number[MGD77_pos[i]];
			for (i = MGD77_N_NUMBER_FIELDS, k = 0; i < MGD77_N_DATA_FIELDS; i++, k++) strcpy (tvals[k], MGD77Record.word[k]);
			break;
		case MGD77_FORMAT_CDF:		/* Will read a single MGD77+ netCDF record */
			error = MGD77_Read_Data_Record_cdf (F, H, dvals, tvals);
			break;
		case MGD77_FORMAT_TBL:		/* Will read a single ascii table record */
			error = MGD77_Read_Data_Record_tbl (F, &MGD77Record);
			dvals[0] = MGD77Record.time;
			for (i = 1; i < MGD77_N_NUMBER_FIELDS; i++) dvals[i] = MGD77Record.number[MGD77_pos[i]];
			dvals[MGD77_TIME] = MGD77Record.time;
			for (i = MGD77_N_NUMBER_FIELDS, k = 0; i < MGD77_N_DATA_FIELDS; i++, k++) strcpy (tvals[k], MGD77Record.word[k]);
			break;
		default:
			error = MGD77_UNKNOWN_FORMAT;
			break;
	}

	return (error);
}

int MGD77_Write_Data_Record (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, double dvals[], char *tvals[])
{	/* writes a single data record based on floating point and char string arrays */
	int i, k, error;
	struct MGD77_DATA_RECORD MGD77Record;

	switch (F->format) {
		case MGD77_FORMAT_M77:		/* Will write a single MGD77 record; first fill out MGD77_RECORD structure */
			MGD77Record.time = dvals[0];
			for (i = 1; i < MGD77_N_NUMBER_FIELDS; i++) MGD77Record.number[MGD77_pos[i]] = dvals[i];
			for (i = MGD77_N_NUMBER_FIELDS, k = 0; i < MGD77_N_DATA_FIELDS; i++, k++) strcpy (MGD77Record.word[k], tvals[k]);
			error = MGD77_Write_Data_Record_m77 (F, &MGD77Record);
			break;
		case MGD77_FORMAT_CDF:		/* Will write a single MGD77+ netCDF record */
			error = MGD77_Write_Data_Record_cdf (F, H, dvals, tvals);
			break;
		case MGD77_FORMAT_TBL:		/* Will write a single ascii table record; first fill out MGD77_RECORD structure */
			MGD77Record.time = dvals[0];
			for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) MGD77Record.number[MGD77_pos[i]] = dvals[i];
			for (i = MGD77_N_NUMBER_FIELDS, k = 0; i < MGD77_N_DATA_FIELDS; i++, k++) strcpy (MGD77Record.word[k], tvals[k]);
			error = MGD77_Write_Data_Record_tbl (F, &MGD77Record);
			break;
		default:
			error = MGD77_UNKNOWN_FORMAT;
			break;
	}

	return (error);
}

int MGD77_Read_Header_Record_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H)
{	/* Applies to MGD77 files */
	char *MGD77_header[MGD77_N_HEADER_RECORDS], line[BUFSIZ];
	int sequence, err;
	struct STAT buf;
	
	/* argument file is generally ignored since file is already open */
	
	memset ((void *)H, '\0', sizeof (struct MGD77_HEADER));	/* Completely wipe existing header */
	if (F->format == MGD77_FORMAT_M77) {			/* Can compute # records from file size because format is fixed */
		if (STAT (F->path, &buf)) {	/* Inquiry about file failed somehow */
			fprintf (stderr, "%s: Unable to stat file %s\n", GMT_program, F->path);
			GMT_exit (EXIT_FAILURE);
		}
		/* Not tested under Windoze: Do we use +2 because of \r\n ? */
		H->n_records = irint ((double)(buf.st_size - (MGD77_N_HEADER_RECORDS * (MGD77_HEADER_LENGTH + 1))) / (double)(MGD77_RECORD_LENGTH + 1));
	}
	else {
		/* Since we do not know the number of records, we must quickly count lines */
		while (fgets (line, BUFSIZ, F->fp)) if (line[0] != '#') H->n_records++;	/* Count every line except comments  */
		rewind (F->fp);						/* Go back to beginning of file */
		H->n_records -= MGD77_N_HEADER_RECORDS;			/* Adjust for the 24 records in the header block */
	}
	
	/* Read Sequences No 01-24: */

	for (sequence = 0; sequence < MGD77_N_HEADER_RECORDS; sequence++) {
		MGD77_header[sequence] = (char *)GMT_memory (VNULL, MGD77_HEADER_LENGTH + 1, sizeof (char), GMT_program);
		if ((err = MGD77_Read_Header_Sequence (F->fp, MGD77_header[sequence], sequence+1))) return (err);
	}
	if (F->format == MGD77_FORMAT_TBL) fgets (line, BUFSIZ, F->fp);			/* Skip the column header for tables */
	
	H->mgd77 = (struct MGD77_HEADER_PARAMS *) GMT_memory (VNULL, 1, sizeof (struct MGD77_HEADER_PARAMS), GMT_program);	/* Allocate parameter header */
	
	if ((err = MGD77_Decode_Header (H->mgd77, MGD77_header, MGD77_FROM_HEADER))) return (err);	/* Decode individual items in the text headers */
	for (sequence = 0; sequence < MGD77_N_HEADER_RECORDS; sequence++) GMT_free ((void *)MGD77_header[sequence]);

	/* Fill in info in F */
	
	MGD77_set_plain_mgd77 (H);				/* Set the info for the standard 27 data fields in MGD-77 files */
	if ((err = MGD77_Order_Columns (F, H))) return (err);	/* Make sure requested columns are OK; if not given set defaults */
	
	return (MGD77_NO_ERROR);	/* Success, it seems */
}

int MGD77_Decode_Header (struct MGD77_HEADER_PARAMS *P, char *record[], int dir)
{
	/* Copies information between the header structure and the header records */
	int i, k;

	if (dir == MGD77_TO_HEADER) {	/* Set all records to space-filled records */
		for (k = 0; k < MGD77_N_HEADER_RECORDS; k++) {
			memset ((void *)record[k], ' ', MGD77_HEADER_LENGTH);
			sprintf (&record[k][78], "%2.2d", k + 1);	/* Place sequence number */
		}
		P->Record_Type = '4';	/* Set record type */
	}
	
	/* Process Sequence No 01: */

	k = 0;
	if (dir == MGD77_FROM_HEADER && record[k][0] != '4') return (MGD77_NO_HEADER_REC);
	
	MGD77_Place_Text (dir, &P->Record_Type, record[k], 1, 1);
	MGD77_Place_Text (dir, P->Survey_Identifier, record[k], 2, 8);
	MGD77_Place_Text (dir, P->Format_Acronym, record[k], 10, 5);
	MGD77_Place_Text (dir, P->Data_Center_File_Number, record[k], 15, 8);
	MGD77_Place_Text (dir, P->Parameters_Surveyed_Code, record[k], 27, 5);
	MGD77_Place_Text (dir, P->File_Creation_Year, record[k], 32, 4);
	MGD77_Place_Text (dir, P->File_Creation_Month, record[k], 36, 2);
	MGD77_Place_Text (dir, P->File_Creation_Day, record[k], 38, 2);
	MGD77_Place_Text (dir, P->Source_Institution, record[k], 40, 39);

	/* Process Sequence No 02: */

	k = 1;
	MGD77_Place_Text (dir, P->Country, record[k], 1, 18);
	MGD77_Place_Text (dir, P->Platform_Name, record[k], 19, 21);
	MGD77_Place_Text (dir, &P->Platform_Type_Code, record[k], 40, 1);
	MGD77_Place_Text (dir, P->Platform_Type, record[k], 41, 6);
	MGD77_Place_Text (dir, P->Chief_Scientist, record[k], 47, 32);

	/* Process Sequence No 03: */

	k = 2;
	MGD77_Place_Text (dir, P->Project_Cruise_Leg, record[k], 1, 58);
	MGD77_Place_Text (dir, P->Funding, record[k], 59, 20);

	/* Process Sequence No 04: */

	k = 3;
	MGD77_Place_Text (dir, P->Survey_Departure_Year, record[k], 1, 4);
	MGD77_Place_Text (dir, P->Survey_Departure_Month, record[k], 5, 2);
	MGD77_Place_Text (dir, P->Survey_Departure_Day, record[k], 7, 2);
	MGD77_Place_Text (dir, P->Port_of_Departure, record[k], 9, 32);
	MGD77_Place_Text (dir, P->Survey_Arrival_Year, record[k], 41, 4);
	MGD77_Place_Text (dir, P->Survey_Arrival_Month, record[k], 45, 2);
	MGD77_Place_Text (dir, P->Survey_Arrival_Day, record[k], 47, 2);
	MGD77_Place_Text (dir, P->Port_of_Arrival, record[k], 49, 30);

	/* Process Sequence No 05: */

	k = 4;
	MGD77_Place_Text (dir, P->Navigation_Instrumentation, record[k], 1, 40);
	MGD77_Place_Text (dir, P->Geodetic_Datum_Position_Determination_Method, record[k], 41, 38);

	/* Process Sequence No 06: */

	k = 5;
	MGD77_Place_Text (dir, P->Bathymetry_Instrumentation, record[k], 1, 40);
	MGD77_Place_Text (dir, P->Bathymetry_Add_Forms_of_Data, record[k], 41, 38);

	/* Process Sequence No 07: */

	k = 6;
	MGD77_Place_Text (dir, P->Magnetics_Instrumentation, record[k], 1, 40);
	MGD77_Place_Text (dir, P->Magnetics_Add_Forms_of_Data, record[k], 41, 38);

	/* Process Sequence No 08: */

	k = 7;
	MGD77_Place_Text (dir, P->Gravity_Instrumentation, record[k], 1, 40);
	MGD77_Place_Text (dir, P->Gravity_Add_Forms_of_Data, record[k], 41, 38);

	/* Process Sequence No 09: */

	k = 8;
	MGD77_Place_Text (dir, P->Seismic_Instrumentation, record[k], 1, 40);
	MGD77_Place_Text (dir, P->Seismic_Add_Forms_of_Data, record[k], 41, 38);

	/* Process Sequence No 10: */

	k = 9;
	MGD77_Place_Text (dir, &P->Format_Type, record[k], 1, 1);
	MGD77_Place_Text (dir | 32, P->Format_Description, record[k], 2, 75);	/* The 32 prevents removal of trailing spaces just yet */

	/* Process Sequence No 11: */

	k = 10;
	MGD77_Place_Text (dir, &P->Format_Description[75], record[k], 1, 19);	/* Now we can remove spaces */
	MGD77_Place_Text (dir, P->Topmost_Latitude, record[k], 41, 3);
	MGD77_Place_Text (dir, P->Bottommost_Latitude, record[k], 44, 3);
	MGD77_Place_Text (dir, P->Leftmost_Longitude, record[k], 47, 4);
	MGD77_Place_Text (dir, P->Rightmost_Longitude, record[k], 51, 4);

	/* Process Sequence No 12: */

	k = 11;
	MGD77_Place_Text (dir, P->Bathymetry_Digitizing_Rate, record[k], 1, 3);
	MGD77_Place_Text (dir, P->Bathymetry_Sampling_Rate, record[k], 4, 12);
	MGD77_Place_Text (dir, P->Bathymetry_Assumed_Sound_Velocity, record[k], 16, 5);
	MGD77_Place_Text (dir, P->Bathymetry_Datum_Code, record[k], 21, 2);
	MGD77_Place_Text (dir, P->Bathymetry_Interpolation_Scheme, record[k], 23, 56);

	/* Process Sequence No 13: */

	k = 12;
	MGD77_Place_Text (dir, P->Magnetics_Digitizing_Rate, record[k], 1, 3);
	MGD77_Place_Text (dir, P->Magnetics_Sampling_Rate, record[k], 4, 2);
	MGD77_Place_Text (dir, P->Magnetics_Sensor_Tow_Distance, record[k], 6, 4);
	MGD77_Place_Text (dir, P->Magnetics_Sensor_Depth, record[k], 10, 5);
	MGD77_Place_Text (dir, P->Magnetics_Sensor_Separation, record[k], 15, 3);
	MGD77_Place_Text (dir, P->Magnetics_Ref_Field_Code, record[k], 18, 2);
	MGD77_Place_Text (dir, P->Magnetics_Ref_Field, record[k], 20, 12);
	MGD77_Place_Text (dir, P->Magnetics_Method_Applying_Res_Field, record[k], 32, 47);

	/* Process Sequence No 14: */

	k = 13;
	MGD77_Place_Text (dir, P->Gravity_Digitizing_Rate, record[k], 1, 3);
	MGD77_Place_Text (dir, P->Gravity_Sampling_Rate, record[k], 4, 2);
	MGD77_Place_Text (dir, &P->Gravity_Theoretical_Formula_Code, record[k], 6, 1);
	MGD77_Place_Text (dir, P->Gravity_Theoretical_Formula, record[k], 7, 17);
	MGD77_Place_Text (dir, &P->Gravity_Reference_System_Code, record[k], 24, 1);
	MGD77_Place_Text (dir, P->Gravity_Reference_System, record[k], 25, 16);
	MGD77_Place_Text (dir, P->Gravity_Corrections_Applied, record[k], 41, 38);

	/* Process Sequence No 15: */

	k = 14;
	MGD77_Place_Text (dir, P->Gravity_Departure_Base_Station, record[k], 1, 7);
	MGD77_Place_Text (dir, P->Gravity_Departure_Base_Station_Name, record[k], 8, 33);
	MGD77_Place_Text (dir, P->Gravity_Arrival_Base_Station, record[k], 41, 7);
	MGD77_Place_Text (dir, P->Gravity_Arrival_Base_Station_Name, record[k], 48, 31);

	/* Process Sequence No 16: */

	k = 15;
	MGD77_Place_Text (dir, P->Number_of_Ten_Degree_Identifiers, record[k], 1, 2);
	MGD77_Place_Text (dir | 32, P->Ten_Degree_Identifier, record[k], 4, 75);	/* The 32 prevents removal of trailing spaces just yet */

	/* Process Sequence No 17: */

	k = 16;
	MGD77_Place_Text (dir, &P->Ten_Degree_Identifier[75], record[k], 1, 75);	/* Now we can remove spaces */

	/* Process Sequence No 18-24: */

	for (i = 0, k = 17; i < 7; i++, k++) MGD77_Place_Text (dir, P->Additional_Documentation[i], record[k], 1, 78);

	return (NC_NOERR);
}

/* To test parsing of the messages produced below you must compile with -DPARSE_TEST.  Then, all the messages
 * will be output regardless of there being any errors involved.  Used for code testing only!
 */
 
#ifdef PARSE_TEST
#define OR_TRUE || 1
#define AND_FALSE && 0
#else
#define OR_TRUE
#define AND_FALSE
#endif

#define ERR   2
#define WARN  1
#define TOTAL 0

void MGD77_Verify_Header (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, FILE *ufp)
{
	int i, k, pos, ix, iy, w, e, s, n, n_block, kind = 0, ref_field_code, y, yr1, rfStart, yr2, rfEnd;
	char copy[151], p[GMT_TEXT_LEN], text[GMT_TEXT_LEN];
	char *pscode[5] = {"Bathy", "Magnetics", "Gravity", "3.5 kHz", "Seismics"};
	time_t now;
	struct tm *T;
	FILE *fp_err;
	struct MGD77_HEADER_PARAMS *P;
	
	if (!F->verbose_level) return;	/* No verbosity desired */
	
	if (ufp) {	/* User provided alternative output pipe */
		fp_err = ufp;
	}
	else {
		fp_err = (F->verbose_dest == 1) ? GMT_stdout : stderr;
	}
	
	H->errors[TOTAL] = H->errors[WARN] = H->errors[ERR] = 0;
	
	P = H->mgd77;
	
	if (!H->meta.verified) {
		fprintf (stderr, "%s: ERROR: MGD77_Verify_Header called before MGD77_Verify_Prep\n", GMT_program);
		GMT_exit (EXIT_FAILURE);
	}

	(void) time (&now);
	
	T = gmtime (&now);
	
	/* Verify Sequence No 01: */
	
	if ((!(P->Record_Type == '1' || P->Record_Type == '4')) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "Y-E-%s-H01-01: Invalid Record Type: (%c) [4]\n", F->NGDC_id, P->Record_Type);
		H->errors[ERR]++;
	}
	if (!P->Survey_Identifier[0] OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H01-02: Survey Identifier missing: () [        ]\n", F->NGDC_id);
		H->errors[ERR]++;
	}
	if (strcmp (P->Format_Acronym, "MGD77") OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "Y-E-%s-H01-03: Invalid Format Acronym: (%s) [MGD77]\n", F->NGDC_id, P->Format_Acronym);
		H->errors[ERR]++;
	}
	if (strcmp (P->Data_Center_File_Number, F->NGDC_id) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H01-04: Invalid Data Center File Number: (%s) [%s]\n", F->NGDC_id, P->Data_Center_File_Number, F->NGDC_id);
		H->errors[ERR]++;
	}
	for (i = 0; i < 5; i++) {
		if (P->Parameters_Surveyed_Code[i] == '\0' AND_FALSE) continue;	/* A string might get terminated if there are trailing blanks */
		if (P->Parameters_Surveyed_Code[i] == ' '  AND_FALSE) continue;	/* Skip the OK codes */
		if (P->Parameters_Surveyed_Code[i] == '0'  AND_FALSE) continue;
		if (P->Parameters_Surveyed_Code[i] == '1'  AND_FALSE) continue;
		if (P->Parameters_Surveyed_Code[i] == '3'  AND_FALSE) continue;
		if (P->Parameters_Surveyed_Code[i] == '5'  AND_FALSE) continue;
		if (F->verbose_level) fprintf (fp_err, "?-E-%s-H01-%2.2d: Invalid Parameter Survey Code (%s): (%c) [ ]\n", F->NGDC_id, 5 + i, pscode[i], P->Parameters_Surveyed_Code[i]);
		H->errors[ERR]++;
	}
	if ((P->File_Creation_Year[0] && ((i = atoi (P->File_Creation_Year)) < (1900 + MGD77_OLDEST_YY) || i > (1900 + T->tm_year))) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H01-10: Invalid File Creation Year: (%s) [    ]\n", F->NGDC_id, P->File_Creation_Year);
		H->errors[ERR]++;
	}
	if ((P->File_Creation_Month[0] && ((i = atoi (P->File_Creation_Month)) < 1 || i > 12)) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H01-11: Invalid File Creation Month: (%s) [  ]\n", F->NGDC_id, P->File_Creation_Month);
		H->errors[ERR]++;
	}
	if ((P->File_Creation_Day[0] && ((i = atoi (P->File_Creation_Day)) < 1 || i > 31)) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H01-12: Invalid File Creation Day: (%s) [  ]\n", F->NGDC_id, P->File_Creation_Day);
		H->errors[ERR]++;
	}

	/* Verify Sequence No 02: */

	if ((P->Platform_Type_Code < '0' || P->Platform_Type_Code > '9') OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H02-03: Invalid Platform Type Code: (%c) [0]\n", F->NGDC_id, P->Platform_Type_Code);
		H->errors[ERR]++;
	}

	/* Verify Sequence No 04: */

	if ((P->Survey_Departure_Year[0] && ((i = atoi (P->Survey_Departure_Year)) < (1900 + MGD77_OLDEST_YY) || i > (1900 + T->tm_year) || (H->meta.Departure[0] && i != H->meta.Departure[0]))) OR_TRUE) {
		if (H->meta.Departure[0])
			sprintf (text, "%4.4d", H->meta.Departure[0]);
		else
			strcpy (text, "    ");
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H04-01: Invalid Survey Departure Year: (%s) [%s]\n", F->NGDC_id, P->Survey_Departure_Year, text);
		H->errors[ERR]++;
	}
	if ((P->Survey_Departure_Month[0] && ((i = atoi (P->Survey_Departure_Month)) < 1 || i > 12 || (H->meta.Departure[1] && i != H->meta.Departure[1]))) OR_TRUE) {
		if (H->meta.Departure[1])
			sprintf (text, "%2.2d", H->meta.Departure[1]);
		else
			strcpy (text, "  ");
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H04-02: Invalid Survey Departure Month: (%s) [%s]\n", F->NGDC_id, P->Survey_Departure_Month, text);
		H->errors[ERR]++;
	}
	if ((P->Survey_Departure_Day[0] && ((i = atoi (P->Survey_Departure_Day)) < 1 || i > 31 || (H->meta.Departure[2] && i != H->meta.Departure[2]))) OR_TRUE) {
		if (H->meta.Departure[2])
			sprintf (text, "%2.2d", H->meta.Departure[2]);
		else
			strcpy (text, "  ");
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H04-03: Invalid Survey Departure Day: (%s) [%s]\n", F->NGDC_id, P->Survey_Departure_Day, text);
		H->errors[ERR]++;
	}
	if ((P->Survey_Arrival_Year[0] && ((i = atoi (P->Survey_Arrival_Year)) < (1900 + MGD77_OLDEST_YY) || i > (1900 + T->tm_year) || (H->meta.Arrival[0] && i != H->meta.Arrival[0]))) OR_TRUE) {
		if (H->meta.Arrival[0])
			sprintf (text, "%4.4d", H->meta.Arrival[0]);
		else
			strcpy (text, "    ");
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H04-04: Invalid Survey Arrival Year: (%s) [%s]\n", F->NGDC_id, P->Survey_Arrival_Year, text);
		H->errors[ERR]++;
	}
	if ((P->Survey_Arrival_Month[0] && ((i = atoi (P->Survey_Arrival_Month)) < 1 || i > 12 || (H->meta.Arrival[1] && i != H->meta.Arrival[1]))) OR_TRUE) {
		if (H->meta.Arrival[1])
			sprintf (text, "%2.2d", H->meta.Arrival[1]);
		else
			strcpy (text, "  ");
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H04-05: Invalid Survey Arrival Month: (%s) [%s]\n", F->NGDC_id, P->Survey_Arrival_Month, text);
		H->errors[ERR]++;
	}
	if ((P->Survey_Arrival_Day[0] && ((i = atoi (P->Survey_Arrival_Day)) < 1 || i > 31 || (H->meta.Arrival[2] && i != H->meta.Arrival[2]))) OR_TRUE) {
		if (H->meta.Arrival[2])
			sprintf (text, "%2.2d", H->meta.Arrival[2]);
		else
			strcpy (text, "  ");
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H04-06: Invalid Survey Arrival Day: (%s) [%s]\n", F->NGDC_id, P->Survey_Arrival_Day, text);
		H->errors[ERR]++;
	}
	/* Verify Sequence No 10: */

	if (P->Format_Type != 'A' OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "Y-E-%s-H10-01: Invalid Format Type: (%c) [A]\n", F->NGDC_id, P->Format_Type);
		H->errors[ERR]++;
	}
	strcpy (copy, P->Format_Description);
	GMT_str_toupper (copy);
	if (strcmp (copy, "(I1,A8,I3,I4,3I2,F5.3,F8.5,F9.5,I1,F6.4,F6.1,I2,I1,3F6.1,I1,F5.1,F6.0,F7.1,F6.1,F5.1,A5,A6,I1)") OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "Y-E-%s-H10-02: Invalid Format Description: (%s) [(I1,A8,I3,I4,3I2,F5.3,F8.5,F9.5,I1,F6.4,F6.1,I2,I1,3F6.1,I1,F5.1,F6.0,F7.1,F6.1,F5.1,A5,A6,I1)]\n", F->NGDC_id, P->Format_Description);
		H->errors[ERR]++;
	}

	/* Process Sequence No 11: */

	w = e = s = n = 9999;
	if ((P->Topmost_Latitude[0] && (((n = atoi (P->Topmost_Latitude)) < -90 || n > +90) || n != H->meta.n)) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H11-02: Invalid Topmost Latitude : (%s) [%+2.2d]\n", F->NGDC_id, P->Topmost_Latitude, H->meta.n);
		H->errors[ERR]++;
	}
	if ((P->Bottommost_Latitude[0] && (((s = atoi (P->Bottommost_Latitude)) < -90 || s > +90) || s != H->meta.s)) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H11-03: Invalid Bottommost Latitude: (%s) [%+2.2d]\n", F->NGDC_id, P->Bottommost_Latitude, H->meta.s);
		H->errors[ERR]++;
	}
	if ((!(s == 9999 || n == 9999) && s > n) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H11-04: Bottommost Latitude %d exceeds Topmost Latitude %d\n", F->NGDC_id, s, n);
		H->errors[ERR]++;
	}
	if ((P->Leftmost_Longitude[0] && (((w = atoi (P->Leftmost_Longitude)) < -180 || w > +180) || w != H->meta.w)) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H11-05: Invalid Leftmost Longitude: (%s) [%+3.3d]\n", F->NGDC_id, P->Leftmost_Longitude, H->meta.w);
		H->errors[ERR]++;
	}
	if ((P->Rightmost_Longitude[0] && (((e = atoi (P->Rightmost_Longitude)) < -180 || e > +180) || e != H->meta.e)) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H11-06: Invalid Rightmost Longitude: (%s) [%+3.3d]\n", F->NGDC_id, P->Rightmost_Longitude, H->meta.e);
		H->errors[ERR]++;
	}

	/* Process Sequence No 12: */

	if ((P->Bathymetry_Digitizing_Rate[0] && ((i = atoi (P->Bathymetry_Digitizing_Rate)) <= 0 || i >= 300)) OR_TRUE) {	/* 30 min */
		kind = (wrong_filler (P->Bathymetry_Digitizing_Rate, 3)) ? ERR : WARN;
		if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H12-01: Invalid Bathymetry Digitizing Rate: (%s) [   ]\n", F->NGDC_id, P->Bathymetry_Digitizing_Rate);
			else
				fprintf (fp_err, "?-E-%s-H12-01: Invalid Bathymetry Digitizing Rate: (%s) [%3s]\n", F->NGDC_id, P->Bathymetry_Digitizing_Rate, P->Bathymetry_Digitizing_Rate);
		}
		H->errors[kind]++;
	}
	if ((P->Bathymetry_Assumed_Sound_Velocity[0] && !((i = atoi (P->Bathymetry_Assumed_Sound_Velocity)) < 140000 || i > 15500)) OR_TRUE) {
		kind = (wrong_filler (P->Bathymetry_Assumed_Sound_Velocity, 5)) ? ERR : WARN;
		if (i > 1400 && i < 1550) {
			if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H12-03: Invalid Bathymetry Assumed Sound Velocity: (%s) [%d0]\n", F->NGDC_id, P->Bathymetry_Assumed_Sound_Velocity, i);
		}
		else if (i == 8000 OR_TRUE) {
			if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H12-03: Invalid Bathymetry Assumed Sound Velocity: (%s) [14630]\n", F->NGDC_id, P->Bathymetry_Assumed_Sound_Velocity);
		}
		else if (kind == ERR OR_TRUE) {
			if (F->verbose_level | 1) fprintf (fp_err, "Y-E-%s-H12-03: Invalid Bathymetry Assumed Sound Velocity: (%s) [     ]\n", F->NGDC_id, P->Bathymetry_Assumed_Sound_Velocity);
		}
		else if (F->verbose_level & kind)
			fprintf (fp_err, "?-E-%s-H12-03: Invalid Bathymetry Assumed Sound Velocity: (%s) [%5s]\n", F->NGDC_id, P->Bathymetry_Assumed_Sound_Velocity, P->Bathymetry_Assumed_Sound_Velocity);
		H->errors[kind]++;
	}
	if (P->Bathymetry_Datum_Code[0] OR_TRUE) {
		i = atoi (P->Bathymetry_Datum_Code);
		if (!((i >= 0 && i <= 11) || i == 88)) {
			kind = (i == 99) ? ERR : WARN;
			if (i == 99) {
				if (F->verbose_level & kind) fprintf (fp_err, "Y-E-%s-H12-04: Invalid Bathymetry Datum Code: (%s) [  ]\n", F->NGDC_id, P->Bathymetry_Datum_Code);
			}
			else {
				if (F->verbose_level & kind) fprintf (fp_err, "?-E-%s-H12-04: Invalid Bathymetry Datum Code: (%s) [%2s]\n", F->NGDC_id, P->Bathymetry_Datum_Code, P->Bathymetry_Datum_Code);
			}
			H->errors[kind]++;
		}
	}

	/* Process Sequence No 13: */

	if ((P->Magnetics_Digitizing_Rate[0] && ((i = atoi (P->Magnetics_Digitizing_Rate)) < 0 || i >= 300)) OR_TRUE) {	/* 30 m */
		kind = (wrong_filler (P->Magnetics_Digitizing_Rate, 3)) ? ERR : WARN;
		if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H13-01: Invalid Magnetics Digitizing Rate: (%s) [   ]\n", F->NGDC_id, P->Magnetics_Digitizing_Rate);
			else
				fprintf (fp_err, "?-E-%s-H13-01: Invalid Magnetics Digitizing Rate: (%s) [%3s]\n", F->NGDC_id, P->Magnetics_Digitizing_Rate, P->Magnetics_Digitizing_Rate);
		}
		H->errors[kind]++;
	}
	if ((P->Magnetics_Sampling_Rate[0] && ((i = atoi (P->Magnetics_Sampling_Rate)) < 0 || i > 60)) OR_TRUE) {
		kind = (wrong_filler (P->Magnetics_Sampling_Rate, 2)) ? ERR : WARN;
		if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H13-02: Invalid Magnetics Sampling Rate: (%s) [  ]\n", F->NGDC_id, P->Magnetics_Sampling_Rate);
			else
				fprintf (fp_err, "?-E-%s-H13-02: Invalid Magnetics Sampling Rate: (%s) [%2s]\n", F->NGDC_id, P->Magnetics_Sampling_Rate, P->Magnetics_Sampling_Rate);
		}
		H->errors[kind]++;
	}
	if ((P->Magnetics_Sensor_Tow_Distance[0] && ((i = atoi (P->Magnetics_Sensor_Tow_Distance)) < 0)) OR_TRUE) {
		kind = (wrong_filler (P->Magnetics_Sensor_Tow_Distance, 4)) ? ERR : WARN;
		if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H13-03: Invalid Magnetics Sensor Tow Distance: (%s) [    ]\n", F->NGDC_id, P->Magnetics_Sensor_Tow_Distance);
			else
				fprintf (fp_err, "?-E-%s-H13-03: Invalid Magnetics Sensor Tow Distance: (%s) [%4s]\n", F->NGDC_id, P->Magnetics_Sensor_Tow_Distance, P->Magnetics_Sensor_Tow_Distance);
		}
		H->errors[kind]++;
	}
	if ((P->Magnetics_Sensor_Depth[0] && ((i = atoi (P->Magnetics_Sensor_Depth)) < 0)) OR_TRUE) {
		kind = (wrong_filler (P->Magnetics_Sensor_Depth, 5)) ? ERR : WARN;
		if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H13-04: Invalid Magnetics Sensor Depth: (%s) [     ]\n", F->NGDC_id, P->Magnetics_Sensor_Depth);
			else
				fprintf (fp_err, "?-E-%s-H13-04: Invalid Magnetics Sensor Depth: (%s) [%5s]\n", F->NGDC_id, P->Magnetics_Sensor_Depth, P->Magnetics_Sensor_Depth);
		}
		H->errors[kind]++;
	}
	if ((P->Magnetics_Sensor_Separation[0] && ((i = atoi (P->Magnetics_Sensor_Separation)) < 0)) OR_TRUE) {
		kind = (wrong_filler (P->Magnetics_Sensor_Separation, 3)) ? ERR : WARN;
		if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H13-05: Invalid Magnetics Sensor Separation: (%s) [   ]\n", F->NGDC_id, P->Magnetics_Sensor_Separation);
			else
				fprintf (fp_err, "?-E-%s-H13-05: Invalid Magnetics Sensor Separation: (%s) [%3s]\n", F->NGDC_id, P->Magnetics_Sensor_Separation, P->Magnetics_Sensor_Separation);
		}
		H->errors[kind]++;
	}
	i = -1;
	if (P->Magnetics_Ref_Field_Code[0] OR_TRUE) {
		i = atoi (P->Magnetics_Ref_Field_Code);
		if ((!((i >= 0 && i <= 13) || i == 88)) OR_TRUE) {
			kind = (i == 99) ? ERR : WARN;
			if (F->verbose_level & kind) {
				if (i == 99)
					fprintf (fp_err, "Y-E-%s-H13-06: Invalid Magnetics Reference Field Code: (%s) [00]\n", F->NGDC_id, P->Magnetics_Ref_Field_Code);
				else {
					fprintf (fp_err, "?-E-%s-H13-06: Invalid Magnetics Reference Field Code: (%s) [%2s]\n", F->NGDC_id, P->Magnetics_Ref_Field_Code, P->Magnetics_Ref_Field_Code);
					i = 99;	/* To skip the test on time range below */
				}
			}
			H->errors[kind]++;
		}
	}
	if ((!P->Magnetics_Ref_Field[0] && i == 88) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H13-07: Invalid Magnetics Ref Code == 88 but no Ref Field specified [00]\n", F->NGDC_id);
		H->errors[ERR]++;
	}
	ref_field_code = i;
	if (!strcmp (P->Magnetics_Ref_Field, "IGRF") OR_TRUE) {	/* Check IGRF  number if 88 was given */
		i = 0;
		while (P->Magnetics_Ref_Field[i] && P->Magnetics_Ref_Field[i] != 'F') i++;
		if (P->Magnetics_Ref_Field[i] == '-') i++;
		n = atoi (&P->Magnetics_Ref_Field[i]);
		if (((n == 3 || n == 4 || (n >= 11 && n < 88)) && ref_field_code != n) OR_TRUE) {
			if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H13-08: %s Ref Field code found (%d) do not match given ref field (%d) [%d]\n", F->NGDC_id, P->Magnetics_Ref_Field, n, ref_field_code, n);
			H->errors[ERR]++;
		}
	}

	/* If cruise has magnetics check for correct IGRF */
	
	yr1 = (H->meta.Departure[0]) ? H->meta.Departure[0] : atoi (P->Survey_Departure_Year);
	yr2 = (H->meta.Arrival[0]) ? H->meta.Arrival[0] : atoi (P->Survey_Arrival_Year);
			
	if (yr1 && yr2 && ref_field_code != -1 && ref_field_code != 99) {
		if (ref_field_code == 88) {
			if (!strncmp(P->Magnetics_Ref_Field,"IGRF",4)) {
				for (k = 0; P->Magnetics_Ref_Field[k] != 'F'; k++);
				if (P->Magnetics_Ref_Field[k] == '-' || P->Magnetics_Ref_Field[k] == ' ') k++;
				y = atoi (&P->Magnetics_Ref_Field[k]);
				rfEnd = 1900 + y;
				rfStart = rfEnd - 5;
			}
			else {
				rfStart = 0;
				rfEnd = INT_MAX;
				if (F->verbose_level | 2) fprintf (fp_err, "Y-W-%s-H13-09: Unknown IGRF specified (%s)\n", F->NGDC_id, P->Magnetics_Ref_Field);
			}
		}
		else {
			rfStart = mgd77rf[ref_field_code-1].start;
			rfEnd = mgd77rf[ref_field_code-1].end;
		}
		(yr1 == yr2) ? sprintf (text, "%d", yr1) : sprintf (text, "%d-%d", yr1, yr2);
		if (yr1 < rfStart || yr2 > rfEnd) {
			if (F->verbose_level | 1) fprintf (fp_err, "Y-W-%s-H13-10: Survey year (%s) outside magnetic reference field %s time range (%d-%d)\n", F->NGDC_id, text, P->Magnetics_Ref_Field, rfStart, rfEnd);
			H->errors[WARN]++;
		}
	}

	/* Process Sequence No 14: */

	if ((P->Gravity_Digitizing_Rate[0] && ((i = atoi (P->Gravity_Digitizing_Rate)) < 0 || i > 300)) OR_TRUE) {	/* 30 m */
		kind = (wrong_filler (P->Gravity_Digitizing_Rate, 3)) ? ERR : WARN;
		if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H14-01: Invalid Gravity Digitizing Rate: (%s) [   ]\n", F->NGDC_id, P->Gravity_Digitizing_Rate);
			else
				fprintf (fp_err, "?-E-%s-H14-01: Invalid Gravity Digitizing Rate: (%s) [%3s]\n", F->NGDC_id, P->Gravity_Digitizing_Rate, P->Gravity_Digitizing_Rate);
		}
		H->errors[kind]++;
	}
	if ((P->Gravity_Sampling_Rate[0] && ((i = atoi (P->Gravity_Sampling_Rate)) < 0 || i > 98)) OR_TRUE) {
		kind = (wrong_filler (P->Gravity_Sampling_Rate, 2)) ? ERR : WARN;
		if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H14-02: Invalid Gravity Sampling Rate: (%s) [  ]\n", F->NGDC_id, P->Gravity_Sampling_Rate);
			else
				fprintf (fp_err, "?-E-%s-H14-02: Invalid Gravity Sampling Rate: (%s) [%2s]\n", F->NGDC_id, P->Gravity_Sampling_Rate, P->Gravity_Sampling_Rate);
		}
		H->errors[kind]++;
	}
	i = P->Gravity_Theoretical_Formula_Code - '0';
	if ((P->Gravity_Theoretical_Formula_Code && !((i >= 1 && i <= 4) || i == 8)) OR_TRUE) {
		if (F->verbose_level & kind) {
			if (i == 9)
				fprintf (fp_err, "Y-E-%s-H14-03: Invalid Gravity Theoretical Formula Code: (%c) [ ]\n", F->NGDC_id, P->Gravity_Theoretical_Formula_Code);
			else
				fprintf (fp_err, "?-E-%s-H14-03: Invalid Gravity Theoretical Formula Code: (%c) [%c]\n", F->NGDC_id, P->Gravity_Theoretical_Formula_Code, P->Gravity_Theoretical_Formula_Code);
		}
		H->errors[ERR]++;
	}
	i = P->Gravity_Reference_System_Code - '0';
	if ((P->Gravity_Reference_System_Code && !((i >= 1 && i <= 3) || i == 9)) OR_TRUE) {
		if (F->verbose_level | 2) {
			if (i == 9)
				fprintf (fp_err, "Y-E-%s-H14-05: Invalid Gravity Reference System Code: (%c) [ ]\n", F->NGDC_id, P->Gravity_Reference_System_Code);
			else
				fprintf (fp_err, "?-E-%s-H14-05: Invalid Gravity Reference System Code: (%c) [%c]\n", F->NGDC_id, P->Gravity_Reference_System_Code, P->Gravity_Reference_System_Code);
		}
		H->errors[ERR]++;
	}

	/* Process Sequence No 15: */

	if ((P->Gravity_Departure_Base_Station[0] && ((i = atoi (P->Gravity_Departure_Base_Station)) < 9750000 || i > 9850000)) OR_TRUE) {
		kind = (wrong_filler (P->Gravity_Departure_Base_Station, 7)) ? ERR : WARN;
		if ((i > 975000 && i < 985000) OR_TRUE) {	/* Off by factor of 10? */
			if (F->verbose_level & kind) fprintf (fp_err, "?-E-%s-H15-01: Invalid Gravity Departure Base Station Value: (%s) [%d0]\n", F->NGDC_id, P->Gravity_Departure_Base_Station, i);
		}
		else if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H15-01: Invalid Gravity Departure Base Station Value: (%s) [       ]\n", F->NGDC_id, P->Gravity_Departure_Base_Station);
			else
				fprintf (fp_err, "?-E-%s-H15-01: Invalid Gravity Departure Base Station Value: (%s) [%7s]\n", F->NGDC_id, P->Gravity_Departure_Base_Station, P->Gravity_Departure_Base_Station);
		}
		H->errors[kind]++;
	}
	if ((P->Gravity_Arrival_Base_Station[0] && ((i = atoi (P->Gravity_Arrival_Base_Station)) < 9750000 || i > 9850000))) {
		kind = (wrong_filler (P->Gravity_Departure_Base_Station, 7)) ? ERR : WARN;
		if (i > 975000 && i < 985000) {	/* Off by factor of 10? */
			if (F->verbose_level & kind) fprintf (fp_err, "?-E-%s-H15-03: Invalid Gravity Arrival Base Station Value: (%s) [%d0]\n", F->NGDC_id, P->Gravity_Arrival_Base_Station, i);
		}
		else if (F->verbose_level & kind) {
			if (kind == ERR)
				fprintf (fp_err, "Y-E-%s-H15-03: Invalid Gravity Arrival Base Station Value: (%s) [       ]\n", F->NGDC_id, P->Gravity_Arrival_Base_Station);
			else
				fprintf (fp_err, "?-E-%s-H15-03: Invalid Gravity Arrival Base Station Value: (%s) [%7s]\n", F->NGDC_id, P->Gravity_Arrival_Base_Station, P->Gravity_Arrival_Base_Station);
		}
		H->errors[kind]++;
	}

	/* Process Sequence No 16: */

	n = 0;
	if ((P->Number_of_Ten_Degree_Identifiers[0] && (((n = atoi (P->Number_of_Ten_Degree_Identifiers)) < 1 || n > 30) || n != H->meta.n_ten_box)) OR_TRUE) {
		if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H16-01: Invalid Number of Ten Degree Identifiers: (%s) [%d]\n", F->NGDC_id, P->Number_of_Ten_Degree_Identifiers, H->meta.n_ten_box);
		H->errors[ERR]++;
	}
	pos = n_block = 0;
	strcpy (copy, P->Ten_Degree_Identifier);
	while (GMT_strtok (copy,",", &pos, p)) {
		if (!strcmp (p, "9999")) {
			if ((n && n_block != n) OR_TRUE) {
				if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H16-02: Invalid Number of Ten Degree Identifiers: (%d) [%d]\n", F->NGDC_id, n_block, n);
				n = 0;
			}
			continue;
		}
		if (!strcmp (p, "   0")) continue;
		if (!strcmp (p, "    ")) continue;
		k = 0;
		if ((!(p[0] == '1' || p[0] == '3' || p[0] == '5' || p[0] == '7')) OR_TRUE) {
			if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H16-03-%2.2d: Invalid Ten Degree Identifier quadrant: (%s)\n", F->NGDC_id, n_block+1, p);
			k++;
		}
		if ((!(p[1] >= '0' && p[1] <= '9')) OR_TRUE) {
			if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H16-04-%2.2d: Invalid Ten Degree Identifier latitude: (%s)\n", F->NGDC_id, n_block+1, p);
			k++;
		}
		if (((ix = atoi (&p[2])) < 0 || ix > 18) OR_TRUE) {
			if (F->verbose_level | 2) fprintf (fp_err, "?-E-%s-H16-05-%2.2d: Invalid Ten Degree Identifier longitude: (%s)\n", F->NGDC_id, n_block+1, p);
			k++;
		}
		if (k && (F->verbose_level | 2)) fprintf (fp_err, "?-E-%s-H16-06-%2.2d: Invalid Ten Degree Identifier: (%s)\n", F->NGDC_id, n_block+1, p);
		H->errors[ERR] += k;
		n_block++;
		if (p[0] == '1' || p[0] == '3') ix += 19;
		iy = (p[1] - '0');
		if (p[0] == '1' || p[0] == '7') iy += 10;
		H->meta.ten_box[iy][ix] -= 1;	/* So if there is perfect match we should have 0s */
	}
	for (iy = 0; iy < 20; iy++) {
		for (ix = 0; ix < 38; ix++) {
			if (!H->meta.ten_box[iy][ix]) continue;
			i = get_quadrant (ix, iy);
			if (H->meta.ten_box[iy][ix] == 1) {
				if (F->verbose_level | 2) fprintf (fp_err, "Y-W-%s-H16-06: Ten Degree Identifier %d not marked in header but block was crossed\n", F->NGDC_id, i);
			}
			else if (H->meta.ten_box[iy][ix] == -1) {
				if (F->verbose_level | 2) fprintf (fp_err, "Y-W-%s-H16-06: Ten Degree Identifier %d marked in header but was not crossed\n", F->NGDC_id, i);
			}
		}
	}

	H->errors[TOTAL] = H->errors[WARN] + H->errors[ERR];	/* Sum of warnings and errors */
}

int wrong_filler (char *field, int length) {
	/* Returns TRUE if the field is completely 00000.., 9999., or ?????. */
	int i, nines, zeros, qmarks;
	
	for (i = nines = zeros = qmarks = 0; field[i] && i < length; i++) {
		if (field[i] == '0')
			zeros++;
		else if (field[i] == '9')
			nines++;
		else if (field[i] == '?')
			qmarks++;
	}
	return (zeros == length || nines == length || qmarks == length);
}

int get_quadrant (int x, int y)
{	/* Assign MGD77 quadrant 10x10 flag */
	int value;
	if (y <= 9) {	/* Southern hemisphere */
		if (x <= 18)	/* Western hemisphere */
			value = 5;
		else		/* Eastern hemisphere */
			value = 3;
	}
	else {		/* Northern hemisphere */
		y -= 10;
		if (x <= 18)	/* Western hemisphere */
			value = 7;
		else		/* Eastern hemisphere */
			value = 1;
	}
	if (x > 18) x -= 19;
	value *= 1000;
	value += abs(y) * 100 + abs(x);
	return (value);
}
	
void MGD77_Verify_Prep_m77 (struct MGD77_CONTROL *F, struct MGD77_META *C, struct MGD77_DATA_RECORD *D, int nrec)
{
	int i, ix, iy;
	double lon, lat, xpmin, xpmax, xnmin, xnmax, ymin, ymax;
	
	xpmin = xnmin = ymin = +DBL_MAX;
	xpmax = xnmax = ymax = -DBL_MAX;
	memset ((void *) C, 0, sizeof (struct MGD77_META));
	
	C->verified = TRUE;
	for (i = 0; i < nrec; i++) {
		lon = D[i].number[MGD77_LONGITUDE];
		lat = D[i].number[MGD77_LATITUDE];
		if (lon >= 180.0) lon -= 360.0;
		ix = (int)floor (fabs(lon) / 10.0);	/* Gives 0-18 for 19 possible values */
		iy = (int)floor (fabs(lat) / 10.0);	/* Gives 0-9 for 10 possible values */
		if (lon >= 0.0) ix += 19;
		if (lat >= 0.0) iy += 10;
		C->ten_box[iy][ix] = 1;
		if (lat < ymin) ymin = lat;
		if (lat > ymax) ymax = lat;
		if (lon >= 0.0 && lon < xpmin) xpmin = lon;
		if (lon >= 0.0 && lon > xpmax) xpmax = lon;
		if (lon < 0.0 && lon < xnmin) xnmin = lon;
		if (lon < 0.0 && lon > xnmax) xnmax = lon;
	}
	xpmin = floor (xpmin);	xnmin = floor (xnmin);	ymin = floor (ymin);
	xpmax = ceil (xpmax);	xnmax = ceil (xnmax);	ymax = ceil (ymax);
	if (xpmin == DBL_MAX) {	/* Only negative longitudes found */
		C->w = irint (xnmin);
		C->e = irint (xnmax);
	}
	else if (xnmin == DBL_MAX) {	/* Only positive longitudes found */
		C->w = irint (xpmin);
		C->e = irint (xpmax);
	}
	else if ((xpmin - xnmax) < 90.0) {	/* Crossed Greenwhich */
		C->w = irint (xnmin);
		C->e = irint (xpmax);
	}
	else {					/* Crossed Dateline */
		C->w = irint (xpmin);
		C->e = irint (xnmax);
	}
	C->s = irint (ymin);
	C->n = irint (ymax);
	
	/* Get the cruise time period for later checking against IGRF used, etc. */
			
	if (!GMT_is_fnan (D[0].time)) {	/* We have  time - obtain yyyy/mm/dd of departure and arrival days */
		C->Departure[0] = irint (D[0].number[MGD77_YEAR]);
		C->Departure[1] = irint (D[0].number[MGD77_MONTH]);
		C->Departure[2] = irint (D[0].number[MGD77_DAY]);
		C->Arrival[0] = irint (D[nrec-1].number[MGD77_YEAR]);
		C->Arrival[1] = irint (D[nrec-1].number[MGD77_MONTH]);
		C->Arrival[2] = irint (D[nrec-1].number[MGD77_DAY]);
	}
	
	for (iy = 0; iy < 20; iy++) {
		for (ix = 0; ix < 38; ix++) {
			if (!C->ten_box[iy][ix]) continue;
			C->n_ten_box++;
		}
	}
}

void MGD77_Verify_Prep (struct MGD77_CONTROL *F, struct MGD77_DATASET *D)
{
	int i, ix, iy;
	double *values[3], lon, lat, xpmin, xpmax, xnmin, xnmax, ymin, ymax;
	struct MGD77_META *C;
	
	values[0] = (double*)D->values[0];	/* time */
	values[1] = (double*)D->values[3];	/* lat */
	values[2] = (double*)D->values[4];	/* lon */
	xpmin = xnmin = ymin = +DBL_MAX;
	xpmax = xnmax = ymax = -DBL_MAX;
	C = &(D->H.meta);
	memset ((void *) C, 0, sizeof (struct MGD77_META));
	C->verified = TRUE;
	
	for (i = 0; i < D->H.n_records; i++ ){
		lat = values[1][i];
		lon = values[2][i];
		if (lon > 180.0) lon -= 360.0;
		ix = (int)floor (fabs(lon) / 10.0);	/* Gives 0-18 for 19 possible values */
		iy = (int)floor (fabs(lat) / 10.0);	/* Gives 0-9 for 10 possible values */
		if (lon >= 0.0) ix += 19;
		if (lat >= 0.0) iy += 10;
		C->ten_box[iy][ix] = 1;
		if (lat < ymin) ymin = lat;
		if (lat > ymax) ymax = lat;
		if (lon >= 0.0 && lon < xpmin) xpmin = lon;
		if (lon >= 0.0 && lon > xpmax) xpmax = lon;
		if (lon < 0.0 && lon < xnmin) xnmin = lon;
		if (lon < 0.0 && lon > xnmax) xnmax = lon;
	}
	xpmin = floor (xpmin);	xnmin = floor (xnmin);	ymin = floor (ymin);
	xpmax = ceil (xpmax);	xnmax = ceil (xnmax);	ymax = ceil (ymax);
	if (xpmin == DBL_MAX) {	/* Only negative longitudes found */
		C->w = irint (xnmin);
		C->e = irint (xnmax);
	}
	else if (xnmin == DBL_MAX) {	/* Only positive longitudes found */
		C->w = irint (xpmin);
		C->e = irint (xpmax);
	}
	else if ((xpmin - xnmax) < 90.0) {	/* Crossed Greenwhich */
		C->w = irint (xnmin);
		C->e = irint (xpmax);
	}
	else {					/* Crossed Dateline */
		C->w = irint (xpmin);
		C->e = irint (xnmax);
	}
	C->s = irint (ymin);
	C->n = irint (ymax);
	
	if (!GMT_is_fnan (values[0][0])) {	/* We have time - obtain yyyy/mm/dd of departure and arrival days */
		GMT_cal_rd rd;
		double s;
		struct GMT_gcal CAL;
		GMT_dt2rdc (values[0][0], &rd, &s);
		GMT_gcal_from_rd (rd, &CAL);
		C->Departure[0] = CAL.year;
		C->Departure[1] = CAL.month;
		C->Departure[2] = CAL.day_m;
		GMT_dt2rdc (values[0][D->H.n_records-1], &rd, &s);
		GMT_gcal_from_rd (rd, &CAL);
		C->Arrival[0] = CAL.year;
		C->Arrival[1] = CAL.month;
		C->Arrival[2] = CAL.day_m;
	}
	for (iy = 0; iy < 20; iy++) {
		for (ix = 0; ix < 38; ix++) {
			if (!C->ten_box[iy][ix]) continue;
			C->n_ten_box++;
		}
	}
}

void MGD77_Place_Text (int dir, char *struct_member, char *header_record, int start_pos, int n_char)
{	/* Pos refers to position in the Fortran punch card, ranging from 1-80.
	 * We either copy from header to structure member or the other way. */
	int i;
	BOOLEAN strip_trailing_spaces;
	
	strip_trailing_spaces = !(dir & 32);
	dir &= 31;	/* Knock off 32 flag if present */
	start_pos--;	/* C starts at 0, not 1 */
	if (dir == MGD77_FROM_HEADER) {
		for (i = 0; i < n_char; i++) struct_member[i] = header_record[start_pos+i];
		if (strip_trailing_spaces) {	/* start at end and go to beginning while space */
			i = n_char - 1;
			while (i >= 0 && struct_member[i] == ' ') i--;
			struct_member[++i] = '\0';
		}
	}
	else if (dir == MGD77_TO_HEADER) {	/* Copy up to end of string */
		for (i = 0; struct_member[i] && i < n_char; i++) header_record[start_pos+i] = struct_member[i];
	}
	else
		MGD77_Fatal_Error (MGD77_BAD_ARG);
}

char *MGD77_cp_txt (char *txt)
{
	int len;
	char *t;
	len = strlen (txt);
	t = (char *)GMT_memory (VNULL, (size_t)(strlen (txt)+1), sizeof (char), GMT_program);
	strcpy (t, txt);
	return (t);
}

char *MGD77_alloc_txt (int len)
{
	char *t;
	t = (char *)GMT_memory (VNULL, (size_t)len, sizeof (char), GMT_program);
	return (t);
}

void MGD77_set_plain_mgd77 (struct MGD77_HEADER *H)
{
	int i, k;
	
	/* When reading a plain ASCII MGD77 file we must set the information structure manually here.
	 * We will fill in information for all columns in the MGD77 ASCII file except for drt and
	 * we will use time & tz instead of year, month, day, hour, min. */
	
	for (i = 0; i < MGD77_SET_COLS; i++) H->info[MGD77_M77_SET].col[i].present = H->info[MGD77_CDF_SET].col[i].present = FALSE;

	/* Start with the time field */
	
	k = 0;
	H->info[MGD77_M77_SET].col[k].abbrev = MGD77_cp_txt ("time");
	H->info[MGD77_M77_SET].col[k].name = MGD77_cp_txt ("Time");
	H->info[MGD77_M77_SET].col[k].units = MGD77_cp_txt (mgd77cdf[MGD77_TIME].units);
	H->info[MGD77_M77_SET].col[k].comment = MGD77_cp_txt (mgd77cdf[MGD77_TIME].comment);
	H->info[MGD77_M77_SET].col[k].factor = mgd77cdf[MGD77_TIME].factor;
	H->info[MGD77_M77_SET].col[k].offset = mgd77cdf[MGD77_TIME].offset;
	H->info[MGD77_M77_SET].col[k].corr_factor = 1.0;
	H->info[MGD77_M77_SET].col[k].corr_offset = 0.0;
	H->info[MGD77_M77_SET].col[k].type = (nc_type) mgd77cdf[MGD77_TIME].type;
	H->info[MGD77_M77_SET].col[k].text = 0;
	H->info[MGD77_M77_SET].col[k].pos = MGD77_TIME;
	H->info[MGD77_M77_SET].col[k].present = TRUE;
	k++;
	
	for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) {	/* Do all the numerical fields */
		if (i >= MGD77_YEAR && i <= MGD77_MIN) continue;	/* Skip these as time + tz represent the same information */
		H->info[MGD77_M77_SET].col[k].abbrev = MGD77_cp_txt (mgd77defs[i].abbrev);
		H->info[MGD77_M77_SET].col[k].name = MGD77_cp_txt (mgd77defs[i].fieldID);
		H->info[MGD77_M77_SET].col[k].units = MGD77_cp_txt (mgd77cdf[i].units);
		H->info[MGD77_M77_SET].col[k].comment = MGD77_cp_txt (mgd77cdf[i].comment);
		H->info[MGD77_M77_SET].col[k].factor = mgd77cdf[i].factor;
		H->info[MGD77_M77_SET].col[k].offset = mgd77cdf[i].offset;
		H->info[MGD77_M77_SET].col[k].corr_factor = 1.0;
		H->info[MGD77_M77_SET].col[k].corr_offset = 0.0;
		H->info[MGD77_M77_SET].col[k].type = (nc_type) mgd77cdf[i].type;
		H->info[MGD77_M77_SET].col[k].text = 0;
		H->info[MGD77_M77_SET].col[k].pos = i;
		H->info[MGD77_M77_SET].col[k].present = TRUE;
		k++;
	}
	for (i = MGD77_N_NUMBER_FIELDS; i < MGD77_N_DATA_FIELDS; i++) {	/* Do the three text fields */
		H->info[MGD77_M77_SET].col[k].abbrev = MGD77_cp_txt (mgd77defs[i].abbrev);
		H->info[MGD77_M77_SET].col[k].name = MGD77_cp_txt (mgd77defs[i].fieldID);
		H->info[MGD77_M77_SET].col[k].units = MGD77_cp_txt (mgd77cdf[i].units);
		H->info[MGD77_M77_SET].col[k].comment = MGD77_cp_txt (mgd77cdf[i].comment);
		H->info[MGD77_M77_SET].col[k].factor = 1.0;
		H->info[MGD77_M77_SET].col[k].offset = 0.0;
		H->info[MGD77_M77_SET].col[k].corr_factor = 1.0;
		H->info[MGD77_M77_SET].col[k].corr_offset = 0.0;
		H->info[MGD77_M77_SET].col[k].type = (nc_type) mgd77cdf[i].type;
		H->info[MGD77_M77_SET].col[k].text = mgd77cdf[i].len;
		H->info[MGD77_M77_SET].col[k].pos = i;
		H->info[MGD77_M77_SET].col[k].present = TRUE;
		k++;
	}

	H->n_fields = H->info[MGD77_M77_SET].n_col = k;
}

int MGD77_Read_Header_Record_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H)  /* Will read the entire 24-section header structure */
{
	int id, c, c_id[2], n_dims, err, n_vars, dims[2];
	size_t count[2] = {0, 0}, length;
	char name[32];
	
	if (MGD77_Open_File (file, F, MGD77_READ_MODE)) return (-1);			/* Basically sets the path */
	
	MGD77_nc_status (nc_open (F->path, NC_NOWRITE, &F->nc_id));	/* Open the file */
	
	memset ((void *)H, 0, sizeof (struct MGD77_HEADER));	/* Initialize header */
	
	/* GET AUTHOR, HISTORY INFORMATION */
	
	MGD77_nc_status (nc_inq_attlen (F->nc_id, NC_GLOBAL, "Author", count));					/* Get length of author */
	H->author = (char *) GMT_memory (VNULL, count[0] + 1, sizeof (char), "MGD77_Read_Header_Record_cdf");	/* Get memory for author */
	MGD77_nc_status (nc_get_att_text (F->nc_id, NC_GLOBAL, "Author",  H->author));
	MGD77_nc_status (nc_inq_attlen (F->nc_id, NC_GLOBAL, "history", count));				/* Get length of history */
	H->history = (char *) GMT_memory (VNULL, count[0] + 1, sizeof (char), "MGD77_Read_Header_Record_cdf");	/* Get memory for history */
	MGD77_nc_status (nc_get_att_text (F->nc_id, NC_GLOBAL, "history", H->history));
	H->history[count[0]] = '\0';
	
	/* GET E77 INFORMATION (IF PRESENT) */
	
	if (nc_inq_attlen (F->nc_id, NC_GLOBAL, "E77", count) == NC_NOERR) {	/* Get length of E77 if present */
		H->E77 = (char *) GMT_memory (VNULL, count[0] + 1, sizeof (char), "MGD77_Read_Header_Record_cdf");	/* Get memory for E77 */
		MGD77_nc_status (nc_get_att_text (F->nc_id, NC_GLOBAL, "E77",  H->E77));
		H->E77[count[0]] = '\0';
	}

	/* GET MGD77 HEADER INFORMATION */
		
	H->mgd77 = (struct MGD77_HEADER_PARAMS *) GMT_memory (VNULL, 1, sizeof (struct MGD77_HEADER_PARAMS), GMT_program);	/* Allocate parameter header */
	MGD77_Read_Header_Params (F, H->mgd77);	/* Get all the MGD77 header attributes */

	/* DETERMINE DIMENSION OF GMT_TIME-SERIES */
	
	MGD77_nc_status (nc_inq_unlimdim (F->nc_id, &F->nc_recid));		/* Get id of unlimited dimension */
	if (F->nc_recid == -1) {	/* We are in deep trouble */
		fprintf (stderr, "%s: No record dimension in file %s - cannot read contents\n", GMT_program, file);
		return (MGD77_ERROR_NOT_MGD77PLUS);
	}
	MGD77_nc_status (nc_inq_dimname (F->nc_id, F->nc_recid, name));	/* Get dimension name */
	H->no_time = (strcmp (name, "time"));				/* True if data set has no time column */
	MGD77_nc_status (nc_inq_dimlen (F->nc_id, F->nc_recid, count));	/* Get number of records */
	H->n_records = count[0];

	/* GET INFORMATION OF ALL COLUMNS AND STORE IN HEADER STRUCTURE */
	
	nc_inq_nvars (F->nc_id, &n_vars);			/* Total number of variables in this file */
	c_id[MGD77_M77_SET] = c_id[MGD77_CDF_SET] = 0;		/* Start with zero columns for both sets */
	
	if (H->no_time) {	/* Create an artificial NaN entry anyway */
		H->info[MGD77_M77_SET].col[0].abbrev = MGD77_cp_txt ("time");
		H->info[MGD77_M77_SET].col[0].name = MGD77_cp_txt ("Time");
		H->info[MGD77_M77_SET].col[0].units = MGD77_cp_txt (mgd77cdf[MGD77_TIME].units);
		H->info[MGD77_M77_SET].col[0].comment = MGD77_cp_txt (mgd77cdf[MGD77_TIME].comment);
		H->info[MGD77_M77_SET].col[0].factor = mgd77cdf[MGD77_TIME].factor;
		H->info[MGD77_M77_SET].col[0].offset = mgd77cdf[MGD77_TIME].offset;
		H->info[MGD77_M77_SET].col[0].corr_factor = 1.0;
		H->info[MGD77_M77_SET].col[0].corr_offset = 0.0;
		H->info[MGD77_M77_SET].col[0].type = (nc_type) mgd77cdf[MGD77_TIME].type;
		H->info[MGD77_M77_SET].col[0].text = 0;
		H->info[MGD77_M77_SET].col[0].pos = MGD77_TIME;
		H->info[MGD77_M77_SET].col[0].present = TRUE;
		c_id[MGD77_M77_SET]++;	/* Move to next position in the set */
	}
	
	for (id = 0; id < n_vars && c_id[MGD77_M77_SET] < MGD77_SET_COLS && c_id[MGD77_CDF_SET] < MGD77_SET_COLS; id++) {	/* Keep checking for extra columns until all are found */
		
		MGD77_nc_status (nc_inq_varname    (F->nc_id, id, name));	/* Get column abbreviation */
		if (!strcmp (name, "MGD77_flags") || !strcmp (name, "CDF_flags")) continue;	/* Flags are dealt with separately later */
		c = MGD77_Get_Set (name);					/* Determine which set this column belongs to */
		H->info[c].col[c_id[c]].abbrev = MGD77_cp_txt (name);
		MGD77_nc_status (nc_inq_vartype    (F->nc_id, id, &H->info[c].col[c_id[c]].type));	/* Get data type */
		/* Look for optional attributes */
		if (nc_inq_attlen   (F->nc_id, id, "long_name", &length) != NC_ENOTATT) {		/* Get long name */
			H->info[c].col[c_id[c]].name = MGD77_alloc_txt (length);
			MGD77_nc_status (nc_get_att_text   (F->nc_id, id, "long_name", H->info[c].col[c_id[c]].name));
		}
		if (nc_inq_attlen   (F->nc_id, id, "units", &length) != NC_ENOTATT) {	/* Get units */
			H->info[c].col[c_id[c]].units = MGD77_alloc_txt (length);
			MGD77_nc_status (nc_get_att_text   (F->nc_id, id, "units", H->info[c].col[c_id[c]].units));
		}
		if (nc_inq_attlen   (F->nc_id, id, "comment", &length) != NC_ENOTATT) {	/* get comments */
			H->info[c].col[c_id[c]].comment = MGD77_alloc_txt (length);
			MGD77_nc_status (nc_get_att_text   (F->nc_id, id, "comment", H->info[c].col[c_id[c]].comment));
		}
		if (nc_get_att_double (F->nc_id, id, "scale_factor", &H->info[c].col[c_id[c]].factor) == NC_ENOTATT) H->info[c].col[c_id[c]].factor = 1.0;	/* Get scale for reading */
		if (nc_get_att_double (F->nc_id, id, "add_offset",   &H->info[c].col[c_id[c]].offset) == NC_ENOTATT) H->info[c].col[c_id[c]].offset = 0.0;	/* Get offset for reading */

		/* In addition to scale_factor/offset, which are used to temporarily scale data to fit in the given nc_type format,
		 * it may have been discovered that the stored data are in the wrong unit (e.g., fathoms instead of meters, mGal instead of 0.1 mGal).
		 * Two optional terms, corr_factor and corr_offset, if present, are used to correct such mistakes (since original data are not to be changed).
		 */
		if (nc_get_att_double (F->nc_id, id, "corr_factor",   &H->info[c].col[c_id[c]].corr_factor) == NC_ENOTATT) H->info[c].col[c_id[c]].corr_factor = 1.0;
		if (nc_get_att_double (F->nc_id, id, "corr_offset",   &H->info[c].col[c_id[c]].corr_offset) == NC_ENOTATT) H->info[c].col[c_id[c]].corr_offset = 0.0;
		H->info[c].col[c_id[c]].var_id = id;				/* Save the netCDF variable ID */
		MGD77_nc_status (nc_inq_varndims (F->nc_id, id, &n_dims));	/* Get number of dimensions */
		MGD77_nc_status (nc_inq_vardimid (F->nc_id, id, dims));		/* Get dimension id(s) of this variable */
		if (n_dims == 2) {	/* Variable is a 2-D text array */
			MGD77_nc_status (nc_inq_dimlen (F->nc_id, dims[1], &count[1]));	/* Get length of each string */
			H->info[c].col[c_id[c]].text = count[1];
		}
		else {	/* Variable is a 1-d array or a single text string */
			if (n_dims == 0 || dims[0] == F->nc_recid)	/* Scalar number or array of numbers */
				H->info[c].col[c_id[c]].text = 0;
			else {	/* Single text string, get its length */
				MGD77_nc_status (nc_inq_dimlen (F->nc_id, dims[0], count));	/* Get dimension length of this dimension */
				H->info[c].col[c_id[c]].text = count[0];
			}
		} 
		H->info[c].col[c_id[c]].constant = (n_dims == 0 || (n_dims == 1 && H->info[c].col[c_id[c]].text));	/* Field is constant (or NaN) for all records */
		H->info[c].col[c_id[c]].present = TRUE;		/* Field is present in this file */
		
		c_id[c]++;	/* Move to next position in the set */
	}
	
	for (c = 0; c < MGD77_N_SETS; c++) H->info[c].n_col = c_id[c];			/* Set the number of columns per set */
	H->n_fields = H->info[MGD77_M77_SET].n_col + H->info[MGD77_CDF_SET].n_col;	/* Set total number of columns */

	if ((err = MGD77_Order_Columns (F, H))) return (err);	/* Make sure requested columns are OK; if not give set defaults */
	
	return (MGD77_NO_ERROR); /* Success, unless failure! */
}

int MGD77_Write_Header_Record_m77 (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H)  /* Will write the entire 24-section header structure */
{
	int i, err;
	char *MGD77_header[MGD77_N_HEADER_RECORDS];
	
	for (i = 0; i < MGD77_N_HEADER_RECORDS; i++) MGD77_header[i] = (char *)GMT_memory (VNULL, MGD77_HEADER_LENGTH + 1, sizeof (char), GMT_program);
	if ((err = MGD77_Decode_Header (H->mgd77, MGD77_header, MGD77_TO_HEADER))) return (err);	/* Encode individual header attributes in the text headers */

	for (i = 0; i < MGD77_N_HEADER_RECORDS; i++) {
		fprintf (F->fp, "%s\n", MGD77_header[i]);
		GMT_free ((void *)MGD77_header[i]);
	}

	return (MGD77_NO_ERROR);
}

void MGD77_Select_All_Columns (struct MGD77_CONTROL *F, struct MGD77_HEADER *H)
{
	/* If MGD77_Select_Column has not been called, we want to return all the columns
	 * present in the current file.  Here, we implement this default "-Fall" choice
	 */
	int id, k, set;
	
	if (F->n_out_columns) return;	/* Already made selection via MGD77_Select_Columns */
	
	/* Here, no selection is made, we return everything available in the file */
	
	for (set = k = 0; set < MGD77_N_SETS; set++) {
		for (id = 0; id < MGD77_SET_COLS; id++) {
			if (!H->info[set].col[id].present) continue;	/* This column is not available */
			F->order[k].set = set;
			F->order[k].item = id;
			H->info[set].col[id].pos = k;
			strcpy (F->desired_column[k], H->info[set].col[id].abbrev);
			k++;
		}
	}
	F->n_out_columns = k;
}

void MGD77_List_Header_Items (struct MGD77_CONTROL *F)
{
	int i;
	
	for (i = 0; i < MGD77_N_HEADER_ITEMS; i++) fprintf (stderr, "\t\t%2d. %s\n", i+1, MGD77_Header_Lookup[i].name);
}
	
int MGD77_Select_Header_Item (struct MGD77_CONTROL *F, char *item)
{
	int i, id, match, length, pick[MGD77_N_HEADER_ITEMS];
	
	memset ((void *)F->Want_Header_Item, 0, MGD77_N_HEADER_ITEMS * sizeof (BOOLEAN));
	
	if (item && item[0] == '-') return 1;	/* Just wants a listing */
	
	if (!item || item[0] == '\0' || !strcmp (item, "all")) {	/* No item (or all) selected, select all */
		for (i = 0; i < MGD77_N_HEADER_ITEMS; i++) F->Want_Header_Item[i] = TRUE;
		return 0;
	}
	
	length = (int)strlen (item);
	
	/* Check if an item number was given */
	
	for (i = match = id = 0; i < length; i++) if (isdigit ((int)item[i])) match++;
	if (match == length && ((id = atoi (item)) >= 1 && id <= MGD77_N_HEADER_ITEMS)) {
		F->Want_Header_Item[id] = TRUE;
		return 0;
	}
	
	/* Now search for matching text strings.  We only look for the first n characters where n is length of item */
	
	for (i = match = 0; i < MGD77_N_HEADER_ITEMS; i++) {
		if (!strncmp (MGD77_Header_Lookup[i].name, item, length)) {
			pick[match] = id = i;
			match++;
		}
	}
	
	if (match == 0) {
		fprintf (stderr, "%s: ERROR: No header item matched your string %s\n", GMT_program, item);
		return -1;
	}
	if (match > 1) {	/* More than one.  See if any of the multiple matches is a full name */
		int n_exact;
		for (i = n_exact = 0; i < match; i++) {
			if (strlen (MGD77_Header_Lookup[pick[i]].name) == (size_t)length) {
				id = pick[i];
				n_exact++;
			}
		}
		if (n_exact == 1) {	/* Found one that matches exactly */
			F->Want_Header_Item[id] = TRUE;
			return 0;
		}
		else {
			fprintf (stderr, "%s: ERROR: More than one item matched your string %s:\n", GMT_program, item);
			for (i = 0; i < match; i++) fprintf (stderr, "	-> %s\n", MGD77_Header_Lookup[pick[i]].name);
			return -2;
		}
	}
	
	/* Here we have a unique match */
	
	F->Want_Header_Item[id] = TRUE;
	return 0;
}

int MGD77_Read_File_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)	  /* Will read all MGD77 records in current file */
{
	int err;
	
	err = MGD77_Open_File (file, F, MGD77_READ_MODE);
	if (err) return (err);
	err = MGD77_Read_Header_Record_asc (file, F, &S->H);  /* Will read the entire 24-section header structure */
	if (err) return (err);
	
	MGD77_Select_All_Columns (F, &S->H);	/* We know we only deal with items from set 0 here */
	
	err = MGD77_Read_Data_asc (file, F, S);	  /* Will read all MGD77 records in current file */
	if (err) return (err);
	
	MGD77_Close_File (F);

	return (MGD77_NO_ERROR);
}

int MGD77_Read_Data_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)	  /* Will read all MGD77 records in current file */
{
	int rec, k, col, n_txt, n_val, id, err, n_nan_times, entry, mgd77_col[MGD77_SET_COLS], Clength[3] = {8, 5, 6};
	struct MGD77_DATA_RECORD MGD77Record;
	double *values[MGD77_N_NUMBER_FIELDS+1];
	char *text[3];
	
	for (k = n_txt = 0; k < F->n_out_columns; k++) if (S->H.info[MGD77_M77_SET].col[F->order[k].item].text) n_txt++;
	if (n_txt > 3) return (MGD77_ERROR_READ_ASC_DATA);
	
	for (k = 0; k < F->n_out_columns - n_txt; k++) values[k] = (double *)GMT_memory (VNULL, S->H.n_records, sizeof (double), "MGD77_Read_File_asc");
	for (k = 0; k < n_txt; k++) text[k] = (char *)GMT_memory (VNULL, S->H.n_records*Clength[k], sizeof (char), "MGD77_Read_File_asc");
	S->H.info[MGD77_M77_SET].bit_pattern = S->H.info[MGD77_CDF_SET].bit_pattern = 0;
	
	for (col = 0; col < F->n_out_columns; col++) {
		if (!MGD77_entry_in_MGD77record (F->desired_column[col], &entry)) continue;
		mgd77_col[col] = entry;
	}

	for (rec = n_nan_times = 0; rec < S->H.n_records; rec++) {
		err = (F->format == MGD77_FORMAT_TBL) ? MGD77_Read_Data_Record_tbl (F, &MGD77Record) : MGD77_Read_Data_Record_m77 (F, &MGD77Record);
		if (err) return (err);
		for (col = n_txt = n_val = 0; col < F->n_out_columns; col++) {
			id = mgd77_col[col];
			if (id >= MGD77_ID && id <= MGD77_SSPN) {
				k = id - MGD77_N_NUMBER_FIELDS;
				strncpy (&text[n_txt++][rec*Clength[k]], MGD77Record.word[k], Clength[k]);
				
			}
			else
				values[n_val++][rec] = (id == MGD77_TIME) ? MGD77Record.time : MGD77Record.number[id];
		}
		S->H.info[MGD77_M77_SET].bit_pattern |= MGD77Record.bit_pattern;
		if (GMT_is_dnan (MGD77Record.time)) n_nan_times++;
	}
	S->H.no_time = (n_nan_times == S->H.n_records);
	for (col = n_txt = n_val = 0; col < F->n_out_columns; col++) S->values[col] = ((S->H.info[MGD77_M77_SET].col[F->order[col].item].text) ? (void *)text[n_txt++] : (void *)values[n_val++]);
	S->n_fields = F->n_out_columns;
	
	return (MGD77_NO_ERROR);
}

int MGD77_Write_File_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)	  /* Will write all MGD77 records in current file */
{
	int err;
	
	if (MGD77_Open_File (file, F, MGD77_WRITE_MODE)) return (-1);
	err = MGD77_Write_Header_Record_m77 (file, F, &S->H);  /* Will write the entire 24-section header structure */
	if (err) return (err);
	if (F->format == MGD77_FORMAT_TBL) fprintf (F->fp, MGD77_COL_ORDER);

	err = MGD77_Write_Data_asc (file, F, S);	  /* Will write all MGD77 records in current file */
	if (err) return (err);

	err = MGD77_Close_File (F);
	if (err) return (err);

	return (MGD77_NO_ERROR);
}

int MGD77_Write_Data_asc (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)	  /* Will write all MGD77 records in current file */
{
	int rec, k, err, col[MGD77_N_DATA_FIELDS+1], id, Clength[3] = {8, 5, 6};
	BOOLEAN make_ymdhm;
	struct MGD77_DATA_RECORD MGD77Record;
	double tz, *values[MGD77_N_DATA_FIELDS+1];
	char *text[MGD77_N_DATA_FIELDS+1];
	struct GMT_gcal cal;
	
	for (k = 0; k < F->n_out_columns; k++) {
		text[k] = (char *)S->values[k];
		values[k] = (double *)S->values[k];
	}
	
	for (id = 0; id < MGD77_N_DATA_FIELDS; id++) {	/* See which columns correspond to our standard MGD77 columns */
		for (k = 0, col[id] = MGD77_NOT_SET; k < F->n_out_columns; k++) if (S->H.info[MGD77_M77_SET].col[k].abbrev && !strcmp (S->H.info[MGD77_M77_SET].col[k].abbrev, mgd77defs[id].abbrev)) col[id] = k;
	}
	for (k = 0, col[MGD77_TIME] = MGD77_NOT_SET; k < F->n_out_columns; k++) if (S->H.info[MGD77_M77_SET].col[k].abbrev && !strcmp (S->H.info[MGD77_M77_SET].col[k].abbrev, "time")) col[MGD77_TIME] = k;
	make_ymdhm = (col[MGD77_TIME] >= 0 && (col[MGD77_YEAR] == MGD77_NOT_SET && col[MGD77_MONTH] == MGD77_NOT_SET && col[MGD77_DAY] == MGD77_NOT_SET && col[MGD77_HOUR] == MGD77_NOT_SET && col[MGD77_MIN] == MGD77_NOT_SET));
	
	memset ((void *)&MGD77Record, 0, sizeof (struct MGD77_DATA_RECORD));
	for (rec = 0; rec < S->H.n_records; rec++) {
		MGD77Record.number[MGD77_RECTYPE] = (col[MGD77_RECTYPE] == MGD77_NOT_SET || GMT_is_dnan (values[col[MGD77_RECTYPE]][rec])) ?  5.0 : values[col[MGD77_RECTYPE]][rec];
		for (id = 1; id < MGD77_N_NUMBER_FIELDS; id++) {
			MGD77Record.number[id] = (col[id] >= 0) ? (double)values[col[id]][rec] : GMT_d_NaN;
		}
		if (make_ymdhm) {	/* Split time into yyyy, mm, dd, hh, mm.xxx */
			MGD77Record.time = values[col[MGD77_TIME]][rec];
			tz = (GMT_is_dnan (MGD77Record.number[MGD77_TZ])) ? 0.0 : MGD77Record.number[MGD77_TZ];
			GMT_gcal_from_dt (MGD77Record.time - tz * 3600.0, &cal);	/* Adjust for TZ to get local calendar */
			MGD77Record.number[MGD77_YEAR]  = cal.year;
			MGD77Record.number[MGD77_MONTH] = cal.month;
			MGD77Record.number[MGD77_DAY]   = cal.day_m;
			MGD77Record.number[MGD77_HOUR]  = cal.hour;
			MGD77Record.number[MGD77_MIN]   = cal.min + cal.sec / 60.0;
		}
		for (id = MGD77_N_NUMBER_FIELDS; id < MGD77_N_DATA_FIELDS; id++) {
			k = id - MGD77_N_NUMBER_FIELDS;
			if (col[id] >= 0)	/* Have this string column */
				strncpy (MGD77Record.word[k], (char *)&text[col[id]][rec*Clength[k]], Clength[k]);
			else
				strncpy (MGD77Record.word[k], ALL_NINES, Clength[k]);
		}
		err = (F->format == MGD77_FORMAT_TBL) ? MGD77_Write_Data_Record_tbl (F, &MGD77Record) : MGD77_Write_Data_Record_m77 (F, &MGD77Record);
		if (err) return (err);
	}

	return (MGD77_NO_ERROR);
}

/* MGD77_Read_Record_m77 decodes the MGD77 data record, storing values in a structure of type
 * MGD77_DATA_RECORD (see MGD77.h for structure definition).
 */
int MGD77_Read_Data_Record_m77 (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record)	  /* Will read a single MGD77 record */
{
	int len, i, k, nwords, value, rata_die, yyyy, mm, dd, nconv;
	char line[BUFSIZ], currentField[10];
	BOOLEAN may_convert;
	double secs, tz;

	if (!(fgets (line, BUFSIZ, F->fp))) return (MGD77_ERROR_READ_ASC_DATA);			/* Try to read one line from the file */

	if (!(line[0] == '3' || line[0] == '5')) return (MGD77_NO_DATA_REC);			/* Only process data records */

	GMT_chop (line);	/* Get rid of CR or LF */
	
	if ((len = (int)strlen(line)) != MGD77_RECORD_LENGTH) {
		fprintf (stderr, "Incorrect record length (%d), skipped\n",len);
		return (MGD77_WRONG_DATA_REC_LEN);
	}
	
	/* Convert old format to new if necessary */
	if (line[0] == '3') MGD77_Convert_To_New_Format (line);

	MGD77Record->bit_pattern = 0;

	/* DECODE the 27 data fields (24 numerical and 3 strings) and store in MGD77_DATA_RECORD */
	
	for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) {	/* Do the numerical fields first */
	
		strncpy (currentField, &line[mgd77defs[i].start-1], mgd77defs[i].length);
		currentField[mgd77defs[i].length] = '\0';
		
		may_convert = !(MGD77_this_bit[i] & MGD77_FLOAT_BITS) || strcmp (currentField, mgd77defs[i].not_given);
		if (may_convert) {	/* OK, we need to decode the value and scale it according to factor */
			MGD77Record->bit_pattern |= MGD77_this_bit[i];	/* Turn on this bit */
			if ((nconv = sscanf (currentField, mgd77defs[i].readMGD77, &value)) != 1) return (MGD77_ERROR_CONV_DATA_REC);
			MGD77Record->number[i] = ((double) value) / mgd77defs[i].factor;
		}
		else 	/* Geophysical observation absent, assign NaN (assign NaN to unspecified time values??) */
			MGD77Record->number[i] = GMT_d_NaN;
	}

	for (i = MGD77_N_NUMBER_FIELDS, nwords = 0; i < MGD77_N_DATA_FIELDS; i++, nwords++) {	/* Do the last 3 string fields */
	
		strncpy (currentField,&line[mgd77defs[i].start-1],mgd77defs[i].length);
		currentField[mgd77defs[i].length] = '\0';

		may_convert = (strncmp(currentField, ALL_NINES, mgd77defs[i].length));
		if (may_convert) {		/* Turn on this data bit */
			MGD77Record->bit_pattern |= MGD77_this_bit[i];
		}
		/* Remove trailing blanks - may lead to empty string */
		k = strlen (currentField) - 1;
		while (k >= 0 && currentField[k] == ' ') k--;
		currentField[++k] = '\0';	/* No longer any trailing blanks */
		strcpy (MGD77Record->word[nwords], currentField);	/* Just copy text without changing it at all */
	}

	/* Get absolute time, if all the pieces are there */
	
	if ((MGD77Record->bit_pattern & MGD77_TIME_BITS) == MGD77_TIME_BITS) {	/* Got all the time items */
		yyyy = irint (MGD77Record->number[MGD77_YEAR]);
		mm = irint (MGD77Record->number[MGD77_MONTH]);
		dd = irint (MGD77Record->number[MGD77_DAY]);
		rata_die = GMT_rd_from_gymd (yyyy, mm, dd);
		tz = (GMT_is_dnan (MGD77Record->number[MGD77_TZ])) ? 0.0 : MGD77Record->number[MGD77_TZ];
		secs = GMT_HR2SEC_I * (MGD77Record->number[MGD77_HOUR] + tz) + GMT_MIN2SEC_I * MGD77Record->number[MGD77_MIN];
		MGD77Record->time = GMT_rdc2dt (rata_die, secs);	/* This gives GMT Unix time in seconds */
		MGD77Record->bit_pattern |= MGD77_this_bit[MGD77_TIME];	/* Turn on this bit */
	}
	else	/* Not present or incomplete, assign NaN */
		MGD77Record->time = GMT_d_NaN;
	
	return (MGD77_NO_ERROR);
}

int MGD77_Read_Data_Record_tbl (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record)	  /* Will read a single tabular MGD77 record */
{
	int i, j, n9, nwords, k, pos, yyyy, mm, dd, rata_die;
	char line[BUFSIZ], p[BUFSIZ];
	double tz, secs;

	if (!fgets (line, BUFSIZ, F->fp)) return (MGD77_ERROR_READ_ASC_DATA);		/* End of file? */
	GMT_chop (line);	/* Get rid of CR or LF */

	MGD77Record->bit_pattern = 0;
	for (i = pos = k = nwords = 0; i < MGD77_N_DATA_FIELDS; i++) {
		if (!GMT_strtok (line, "\t", &pos, p)) return (MGD77_ERROR_READ_ASC_DATA);	/* Premature record end */
		if (i >= MGD77_ID && i <= MGD77_SSPN) {
			strcpy (MGD77Record->word[nwords++], p);		/* Just copy text without changing it at all */
			for (j = n9 = 0; p[j]; j++) if (p[j] == '9') n9++;
			if (n9 < j) MGD77Record->bit_pattern |= MGD77_this_bit[i];
		}
		else {
			MGD77Record->number[k] = (p[0] == 'N') ? GMT_d_NaN : atof (p);
			if (i == 0 && !(p[0] == '5' || p[0] == '3')) return (MGD77_NO_DATA_REC);
			if (!GMT_is_dnan (MGD77Record->number[k])) MGD77Record->bit_pattern |= MGD77_this_bit[i];
			k++;
		}
	}		
	/* Get absolute time, if all the pieces are there */
	
	if ((MGD77Record->bit_pattern & MGD77_TIME_BITS) == MGD77_TIME_BITS) {	/* Got all the time items */
		yyyy = irint (MGD77Record->number[MGD77_YEAR]);
		mm = irint (MGD77Record->number[MGD77_MONTH]);
		dd = irint (MGD77Record->number[MGD77_DAY]);
		rata_die = GMT_rd_from_gymd (yyyy, mm, dd);
		tz = (GMT_is_dnan (MGD77Record->number[MGD77_TZ])) ? 0.0 : MGD77Record->number[MGD77_TZ];
		secs = GMT_HR2SEC_I * (MGD77Record->number[MGD77_HOUR] + tz) + GMT_MIN2SEC_I * MGD77Record->number[MGD77_MIN];
		MGD77Record->time = GMT_rdc2dt (rata_die, secs);	/* This gives GMT Unix time in seconds */
		MGD77Record->bit_pattern |= MGD77_this_bit[MGD77_TIME];	/* Turn on this bit */
	}
	else	/* Not present or incomplete, assign NaN */
		MGD77Record->time = GMT_d_NaN;
	return (MGD77_NO_ERROR);
}

int MGD77_Write_Data_Record_tbl (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record)	  /* Will read a single tabular MGD77 record */
{
	int i, nwords, k;

	for (i = nwords = k = 0; i < MGD77_N_DATA_FIELDS; i++) {
		if (i >= MGD77_ID && i <= MGD77_SSPN) {
			fprintf (F->fp, "%s", MGD77Record->word[nwords++]);
		}
		else
			GMT_ascii_output_one (F->fp, MGD77Record->number[k++], 2);
		if (i < (MGD77_N_DATA_FIELDS-1)) fprintf (F->fp, "%s", gmtdefs.field_delimiter);
	}
	fprintf (F->fp, "\n");
	return (MGD77_NO_ERROR);
}

/* MGD77_Write_Data_Record writes the MGD77_DATA_RECORD structure, printing stored values in original MGD77 format.
 */
int MGD77_Write_Data_Record_m77 (struct MGD77_CONTROL *F, struct MGD77_DATA_RECORD *MGD77Record)	/* Will write a single ASCII MGD77 record */
{
	int nwords = 0, nvalues = 0, i;

	for (i = 0; i < MGD77_N_DATA_FIELDS; i++) {
		if (i == 1) fprintf (F->fp, mgd77defs[24].printMGD77, MGD77Record->word[nwords++]);
		else if (i == 24 || i == 25) fprintf (F->fp, mgd77defs[i+1].printMGD77, MGD77Record->word[nwords++]);
		else {
			if (GMT_is_dnan (MGD77Record->number[nvalues]))	fprintf (F->fp, "%s", mgd77defs[nvalues].not_given);
			else fprintf (F->fp, mgd77defs[nvalues].printMGD77, irint (MGD77Record->number[nvalues]*mgd77defs[nvalues].factor));
			nvalues++;
		}
	}
	fprintf (F->fp, "\n");
	return (MGD77_NO_ERROR);
}

int MGD77_View_Line (FILE *fp, char *MGD77line)	/* View a single MGD77 string */
{
/*	char line[MGD77_RECORD_LENGTH];
	strcpy (MGD77line,line); */
	if (!(fgets (MGD77line, BUFSIZ, fp))) return FALSE;	/* Read one line from the file */
	if (!(fputs (MGD77line, fp))) return FALSE;		/* Put the line back on the stream */
	return TRUE;
}

int MGD77_Convert_To_New_Format (char *line)
{
	int yy, nconv;

	if (line[0] != '3') return FALSE;

	/* Fix DRT and Time Zone Corrector */
	line[0] = '5';
	line[10] = line[12]; 
	line[11] = line[13];

	/* Fix year - Y2K Kludge Fix */
	if ((nconv = sscanf (&line[14], "%2d", &yy)) != 1)	return FALSE;
	if (yy == 99 && !strncmp(&line[16],"99999999999",11)) {
		line[12] = '9';
		line[13] = '9';
	} else {
		if (yy < MGD77_OLDEST_YY) {
			line[12] = '2';
			line[13] = '0';
		} else {
			line[12] = '1';
			line[13] = '9';
		}
	}
	return TRUE;
}

int MGD77_Convert_To_Old_Format (char *newFormatLine, char *oldFormatLine)
{
	int tz; char legid[9], s_tz[6], s_year[5];
	
	if (newFormatLine[0] != '5') return FALSE;
	strncpy (legid, &oldFormatLine[mgd77defs[1].start-1], mgd77defs[1].length);
	tz = atoi (strncpy(s_tz, &newFormatLine[mgd77defs[2].start-1], mgd77defs[2].length));
	strncpy(s_year, &newFormatLine[mgd77defs[3].start-1], mgd77defs[3].length);
	if (tz == 99) tz = 9999;  /* Handle the empty case */
	else tz *= 100;
	sprintf (oldFormatLine,"3%s%+05d%2d%s", legid, tz, *(s_year + 2), (newFormatLine + mgd77defs[4].start-1));
	return TRUE;
}

int MGD77_Read_Header_Sequence (FILE *fp, char *record, int seq)
{
	int got;
	
	if (seq == 1) {	/* Check for MGD77 file header */
		got = fgetc (fp);		/* Read the first character from the file stream */
		ungetc (got, fp);		/* Put the character back on the stream */
		if (got != '4') {
			fprintf (stderr, "MGD77_Read_Header: No header record present\n");
			return (MGD77_NO_HEADER_REC);
		}
	}
	if (fgets (record, MGD77_RECORD_LENGTH, fp) == NULL) {
		fprintf (stderr, "MGD77_Read_Header: Failure to read header sequence %2.2d\n", seq);
		return (MGD77_ERROR_READ_HEADER_ASC);
	}
	GMT_chop (record);

	got = atoi (&record[78]);
	if (got != seq) {
		fprintf (stderr, "MGD77_Read_Header: Expected header sequence %2.2d says it is %2.2d\n", seq, got);
		return (MGD77_WRONG_HEADER_REC);
	}
	return (MGD77_NO_ERROR);
}

int MGD77_Read_Data_Sequence (FILE *fp, char *record)
{
	if (fgets (record, MGD77_RECORD_LENGTH, fp)) return (1);
	return (MGD77_NO_ERROR);
}

void MGD77_Write_Sequence (FILE *fp, int seq)
{
	if (seq > 0) fprintf (fp, "%2.2d", seq);
	fprintf (fp, "\n");
}

void MGD77_Ignore_Format (int format)
{
	/* Allow user to turn on/off acceptance of certain formats.
	 * Use MGD77_FORMAT_ANY to reset back to defaults (all OK) */
	 
	 if (format == MGD77_FORMAT_ANY) {
	 	MGD77_format_allowed[MGD77_FORMAT_M77] = TRUE;
	 	MGD77_format_allowed[MGD77_FORMAT_CDF] = TRUE;
	 	MGD77_format_allowed[MGD77_FORMAT_TBL] = TRUE;
	}
	else if (format >= MGD77_FORMAT_M77 && format <= MGD77_FORMAT_TBL)
		MGD77_format_allowed[format] = FALSE;
}

void MGD77_Process_Ignore (char code, char *format)
{
	int i;

	for (i = 0; i < (int)strlen(format); i++) {
		switch (format[i]) {									
			case 'a':		/* Ignore any files in Standard ASCII MGD-77 format */
			case 'A':
				MGD77_Ignore_Format (MGD77_FORMAT_M77);
				break;
			case 'c':		/* Ignore any files in Enhanced MGD77+ netCDF format */
			case 'C':
				MGD77_Ignore_Format (MGD77_FORMAT_CDF);
				break;
			case 't':		/* Ignore any files in Plain ASCII dat table format */
			case 'T':
				MGD77_Ignore_Format (MGD77_FORMAT_TBL);
				break;
			default:
				fprintf (stderr, "%s: Option -%c Bad format (%c)!\n", GMT_program, code, format[i]);
				GMT_exit (EXIT_FAILURE);
				break;
		}
	}
}

void MGD77_Init (struct MGD77_CONTROL *F)
{
	/* Initialize MGD77 control system */
	int i, k;
	struct passwd *pw;

	memset ((void *)F, 0, sizeof (struct MGD77_CONTROL));		/* Initialize structure */
	MGD77_Path_Init (F);
	MGD77_Init_Columns (F, NULL);
	F->use_flags[MGD77_M77_SET] = F->use_flags[MGD77_CDF_SET] = TRUE;		/* TRUE means programs will use error bitflags (if present) when returning data */
	F->use_corrections[MGD77_M77_SET] = F->use_corrections[MGD77_CDF_SET] = TRUE;	/* TRUE means we will apply correction factors (if present) when reading data */
	GMT_get_time_system ("unix");							/* MGD77+ uses GMT's Unix time epoch */
	memset ((void *)mgd77_range, 0, (size_t)(MGD77_N_DATA_EXTENDED * sizeof (struct MGD77_LIMITS)));
	for (i = 0; i < MGD77_SET_COLS; i++) MGD77_this_bit[i] = 1 << i;
	gmtdefs.time_system = 4;	/* Use UNIX time as rtime */
	if ((pw = getpwuid (getuid ())) != NULL) {
		strcpy (F->user, pw->pw_name);
	}
	F->verbose_level = 0;
	F->verbose_dest = 2;
	F->format = MGD77_FORMAT_ANY;
	F->original = FALSE;	/* Default is to get the latest value for any attribute */
	MGD77_NaN_val[NC_BYTE] = MGD77_NaN_val[NC_CHAR] = CHAR_MIN;
	MGD77_NaN_val[NC_SHORT] = SHRT_MIN;
	MGD77_NaN_val[NC_INT] = INT_MIN;
	MGD77_NaN_val[NC_FLOAT] = MGD77_NaN_val[NC_DOUBLE] = GMT_d_NaN;
	MGD77_Low_val[NC_BYTE] = MGD77_Low_val[NC_CHAR] = CHAR_MIN;
	MGD77_Low_val[NC_SHORT] = SHRT_MIN;
	MGD77_Low_val[NC_INT] = INT_MIN;
	MGD77_Low_val[NC_FLOAT] = -FLT_MAX;
	MGD77_Low_val[NC_DOUBLE] = -DBL_MAX;
	MGD77_High_val[NC_BYTE] = MGD77_High_val[NC_CHAR] = CHAR_MAX;
	MGD77_High_val[NC_SHORT] = SHRT_MAX;
	MGD77_High_val[NC_INT] = INT_MAX;
	MGD77_High_val[NC_FLOAT] = FLT_MAX;
	MGD77_High_val[NC_DOUBLE] = DBL_MAX;
	MGD77_pos[0] = MGD77_TIME;
	for (i = 0, k = 1; i < MGD77_N_NUMBER_FIELDS; i++) {	/* Do all the numerical fields */
		if (i >= MGD77_YEAR && i <= MGD77_MIN) continue;	/* Skip these as time + tz represent the same information */
		MGD77_pos[k] = i;
		k++;
	}
	for (i = MGD77_N_NUMBER_FIELDS; i < MGD77_N_DATA_FIELDS; i++, k++) {	/* Do the three text fields */
		MGD77_pos[k] = i;
	}

}

void MGD77_Init_Columns (struct MGD77_CONTROL *F, struct MGD77_HEADER *H)
{
	/* Initializes the output columns to equal all the input columns
	 * and using the original order.  To change this the program must
	 * call MGD77_Select_Columns.
	 */
	
	F->time_format = GMT_IS_ABSTIME;	/* Default time format is calendar time */

	/* Initialize pointers to limit tests */
	
	MGD77_column_test_double[MGD77_EQ]   = MGD77_eq_test;
	MGD77_column_test_double[MGD77_NEQ]  = MGD77_neq_test;
	MGD77_column_test_double[MGD77_LT]   = MGD77_lt_test;
	MGD77_column_test_double[MGD77_LE]   = MGD77_le_test;
	MGD77_column_test_double[MGD77_GE]   = MGD77_ge_test;
	MGD77_column_test_double[MGD77_GT]   = MGD77_gt_test;
	MGD77_column_test_double[MGD77_BIT]  = MGD77_bit_test;
	MGD77_column_test_string[MGD77_EQ]   = MGD77_ceq_test;
	MGD77_column_test_string[MGD77_NEQ]  = MGD77_cneq_test;
	MGD77_column_test_string[MGD77_LT]   = MGD77_clt_test;
	MGD77_column_test_string[MGD77_LE]   = MGD77_cle_test;
	MGD77_column_test_string[MGD77_GE]   = MGD77_cge_test;
	MGD77_column_test_string[MGD77_GT]   = MGD77_cgt_test;
}

void MGD77_Reset (struct MGD77_CONTROL *F)
{
	/* Reset the entire MGD77 control system except system paths, etc */

	F->use_flags[MGD77_M77_SET] = F->use_flags[MGD77_CDF_SET] = TRUE;		/* TRUE means programs will use error bitflags (if present) when returning data */
	F->use_corrections[MGD77_M77_SET] = F->use_corrections[MGD77_CDF_SET] = TRUE;	/* TRUE means we will apply correction factors (if present) when reading data */
	F->rec_no = F->n_out_columns = F->bit_pattern[0] = F->bit_pattern[1] = F->n_constraints = F->n_exact = F->n_bit_tests = 0;
	F->no_checking = FALSE;
	memset ((void *)F->NGDC_id, 0, (size_t)(MGD77_COL_ABBREV_LEN * sizeof (char)));
	memset ((void *)F->path, 0, (size_t)(BUFSIZ * sizeof (char)));
	F->fp = NULL;
	F->nc_id = F->nc_recid = MGD77_NOT_SET;
	F->format = MGD77_FORMAT_ANY;
	memset ((void *)F->order, 0, (size_t)(MGD77_MAX_COLS * sizeof (struct MGD77_ORDER)));
	memset ((void *)F->Constraint, 0, (size_t)(MGD77_MAX_COLS * sizeof (struct MGD77_CONSTRAINT)));
	memset ((void *)F->desired_column, 0, (size_t)(MGD77_MAX_COLS * MGD77_COL_ABBREV_LEN));
	memset ((void *)F->Exact, 0, (size_t)(MGD77_MAX_COLS * sizeof (struct MGD77_PAIR)));
	memset ((void *)F->Bit_test, 0, (size_t)(MGD77_MAX_COLS * sizeof (struct MGD77_PAIR)));
}

int MGD77_Order_Columns (struct MGD77_CONTROL *F, struct MGD77_HEADER *H)
{	/* Having processed -F and read the file's header, we can organize which
	 * columns must be read and in what order.  If -F was never set we call
	 * MGD77_Select_All_Columns to select every column for output. */
	int i, id, set, item;
	
	MGD77_Select_All_Columns (F, H);	/* Make sure n_out_columns is set */

	for (i = 0; i < F->n_out_columns; i++) {	/* This is not really needed if MGD77_Select_All_Columns did things, but just in case */
		if (MGD77_Info_from_Abbrev (F->desired_column[i], H, &set, &item) == MGD77_NOT_SET) {
			fprintf (stderr, "%s: Requested column %s not in data set!\n", GMT_program, F->desired_column[i]);
			return (MGD77_ERROR_NOSUCHCOLUMN);
		}
		F->order[i].item = item;
		F->order[i].set  = set;
		H->info[set].col[item].pos = i;
	}
	
	for (i = 0; i < F->n_exact; i++) {	/* Determine column and info numbers from column name */
		F->Exact[i].col = MGD77_Get_Column (F->Exact[i].name, F);
	}
	
	/* F->Exact[] now holds F->n_exact values that refer to the output column order */
	
	for (i = 0; i < F->n_constraints; i++) {	/* Determine column and info numbers from column name */
		F->Constraint[i].col = MGD77_Get_Column (F->Constraint[i].name, F);
		set = F->order[F->Constraint[i].col].set;
		id = F->order[F->Constraint[i].col].item;
		if (H->info[set].col[id].text) {
			F->Constraint[i].string_test = MGD77_column_test_string[F->Constraint[i].code];
		}
		else {
			F->Constraint[i].d_constraint = (!strcmp (F->Constraint[i].c_constraint, "NaN")) ? GMT_d_NaN : atof (F->Constraint[i].c_constraint);
			F->Constraint[i].double_test = MGD77_column_test_double[F->Constraint[i].code];
		}
	}
	
	for (i = 0; i < F->n_bit_tests; i++) {	/* Determine column and info numbers from column name */
		F->Bit_test[i].col = MGD77_Get_Column (F->Bit_test[i].name, F);
		F->Bit_test[i].set  = F->Bit_test[i].col / MGD77_SET_COLS;
		F->Bit_test[i].item = F->Bit_test[i].col % MGD77_SET_COLS;
	}
	return (MGD77_NO_ERROR);
}

int MGD77_Info_from_Abbrev (char *name, struct MGD77_HEADER *H, int *set, int *item)
{
	int id, c;
	
	/* Returns the number in the output list AND passes set,item as the entry in H */
	
	for (c = 0; c < MGD77_N_SETS; c++) {
		for (id = 0; id < H->info[c].n_col; id++) {
			if (!strcmp (name, H->info[c].col[id].abbrev)) {
				*item = id;
				*set = c;
				return (H->info[c].col[id].pos);
			}
		}
	}
	*set = *item = MGD77_NOT_SET;
	return (MGD77_NOT_SET);
}

BOOLEAN MGD77_entry_in_MGD77record (char *name, int *entry)
{
	int i;
	
	/* Returns the number in the MGD77 Datarecord number[x] and text[y] arrays */

	*entry = MGD77_NOT_SET;
	
	if (MGD77_Get_Set (name) == MGD77_CDF_SET) return (FALSE);	/* Wrong set entirely */
	
	/* Try time */
	if (!strcmp (name, "time")) {
		*entry = MGD77_TIME;
		return (TRUE);
	}
	
	/* Try theother fields */
	for (i = MGD77_RECTYPE; i <= MGD77_SSPN; i++) if (!strcmp (name, mgd77defs[i].abbrev)) {
		*entry = i;
		return (TRUE);
	}
	
	return (FALSE);
}

void MGD77_Select_Columns (char *arg, struct MGD77_CONTROL *F, int option)
{
	/* Scan the -Fstring and select which columns to use and which order
	 * they should appear on output.  columns given in upper case must
	 * be non-NaN on records to be output.  Use the argument all_exact to set
	 * all columns to upper case status.
	 *
	 * arg :== [<col1>,<col2>,col3>,...][<cola>OP<val>,<colb>OP<val>,...][:+|-<colx>,+|-<coly>,+|-...]
	 *
	 * First [set] are columns to be output. Upper case columns MUST be non-NaN to pass
	 * Second [set] are logical tests on columns.  One or more tests must be passed. ALL
	 *	UPPER CASE test MUST be passed.
	 * Third [set] are list of comumns whose bitflag must be either be 1 (+) or 0 (-).
	 * The presence of the : also turns the automatic use of ALL flags off.
	 */

	char p[BUFSIZ], cstring[BUFSIZ], bstring[BUFSIZ], word[GMT_LONG_TEXT], value[GMT_LONG_TEXT];
	int i, j, k, constraint, n, pos;
	BOOLEAN exact, all_exact;

	/* Special test for keywords mgd77 and all */
	
	if (!arg || !arg[0]) return;	/* Return when nothing is passed to us */
	
	memset ((void *)F->order, 0, (size_t)(MGD77_MAX_COLS * sizeof (int)));		/* Initialize array */
	F->bit_pattern[MGD77_M77_SET] = F->bit_pattern[MGD77_CDF_SET] = 0;
	
	if (strchr (arg, ':')) {	/* Have specific bit-flag conditions */
		i = j = 0;
		while (arg[i] != ':') cstring[i] = arg[i], i++;
		cstring[i] = '\0';
		i++;
		while (arg[i]) bstring[j++] = arg[i++];
		bstring[j] = '\0';
		if (!bstring[0]) F->use_flags[MGD77_M77_SET] = F->use_flags[MGD77_CDF_SET] = FALSE;	/* Turn use of flag bits OFF */
	}
	else {	/* No bit-flag conditions */
		strcpy (cstring, arg);
		bstring[0] = '\0';
	}

	if (option & MGD77_RESET_CONSTRAINT) F->n_constraints = 0;
	if (option & MGD77_RESET_EXACT) F->n_exact = 0;
	all_exact = (option & MGD77_SET_ALLEXACT);

	i = pos = 0;		/* Start at the first ouput column */
	while ((GMT_strtok (cstring, ",", &pos, p))) {	/* Until we run out of abbreviations */
		/* Must check if we need to break this word into flag[=|<=|>=|<|>value] */
		for (k = constraint = 0; p[k] && constraint == 0; k++) {
			if (p[k] == '>') {
				constraint = MGD77_GT;
				if (p[k+1] == '=') constraint |= MGD77_EQ;
			}
			else if (p[k] == '<') {
				constraint = MGD77_LT;
				if (p[k+1] == '=') constraint |= MGD77_EQ;
			}
			else if (p[k] == '=') {
				constraint = MGD77_EQ;
			}
			else if (p[k] == '|') {
				constraint = MGD77_BIT;
			}
			else if (p[k] == '!' && p[k+1] == '=') {
				constraint = MGD77_NEQ;
			}
		}
		if (constraint) {	/* Got a constraint, split the p string into word and value */
			strncpy (word, p, k-1);
			word[k-1] = '\0';
			while (p[k] && strchr ("><=!", p[k])) k++;
			strcpy (value, &p[k]);
		}
		else			/* Just copy the word */
			strcpy (word, p);
			
		/* Turn word into lower case if upper case */
		
		n = strlen (word);
		for (j = k = 0; j < n; j++) if (isupper ((int)word[j])) {
			word[j] = tolower ((int)word[j]);
			k++;
		}
		exact = (all_exact || k == n);			/* TRUE if this constraint must match exactly */
		
		if (!strcmp (word, "atime")) {		/* Same as time */
			strcpy (word, "time");
			F->time_format = GMT_IS_ABSTIME;
		}
		else if (!strcmp (word, "rtime")) {	/* Time relative to EPOCH */
			strcpy (word, "time");
			F->time_format = GMT_IS_RELTIME;	/* Alternate time format is time relative to EPOCH */
		}

		/* OK, here we are ready to update the structures */
		
		if (constraint) {	/* Got a column constraint, just key it by name for now */
			strcpy (F->Constraint[F->n_constraints].name, word);
			strcpy (F->Constraint[F->n_constraints].c_constraint, value);
			F->Constraint[F->n_constraints].code = constraint;
			F->Constraint[F->n_constraints].exact = exact;
			F->n_constraints++;
		}
		else {	/* Desired output column */
			for (j = 0, k = MGD77_NOT_SET; k == MGD77_NOT_SET && j < i; j++) if (!strcmp (word, F->desired_column[j])) k = j;
			if (k != MGD77_NOT_SET) {	/* Mentioned before */
				fprintf (stderr, "%s: Warning: Column \"%s\" given more than once.\n", GMT_program, word);
			}
			strcpy (F->desired_column[i], word);
			if (exact) {		/* This geophysical column must be != NaN for us to output record */
				strcpy (F->Exact[F->n_exact].name, word);
				F->n_exact++;
			}
			i++;					/* Move to the next output column */
		}
	}

	F->n_out_columns = i;
	
	i = pos = 0;		/* Start at the first ouput column */
	while ((GMT_strtok (bstring, ",", &pos, p))) {	/* Until we run out of abbreviations */
		if (p[0] == '+')
			F->Bit_test[i].match = 1;
		else if (p[0] == '-')
			F->Bit_test[i].match = 0;
		else {
			fprintf (stderr, "%s: Error: Bit-test flag (%s) is not in +<col> or -<col> format.\n", GMT_program, p);
			GMT_exit (EXIT_FAILURE);
		}
		strcpy (F->Bit_test[i].name, &p[1]);
		i++;
	}
	F->n_bit_tests = i;
	
	F->no_checking = (F->n_constraints == 0 && F->n_exact == 0 && F->n_bit_tests == 0);	/* Easy street */
}


int MGD77_Get_Column (char *word, struct MGD77_CONTROL *F)
{
	int j, k;
	
	for (j = 0, k = MGD77_NOT_SET; k == MGD77_NOT_SET && j < F->n_out_columns; j++) if (!strcmp (word, F->desired_column[j])) k = j;
	return (k);
}

int MGD77_Get_Set (char *word)
{	/* If word is one of the standard 27 MGD77 columns or time, return 0, else return 1 */
	int j, k;
	
	for (j = 0, k = MGD77_NOT_SET; k == MGD77_NOT_SET && j <= MGD77_SSPN; j++) if (!strcmp (word, mgd77defs[j].abbrev)) k = j;
	if (k == MGD77_NOT_SET && !strcmp (word, "time")) k = j;
	return ((k == MGD77_NOT_SET) ? MGD77_CDF_SET : MGD77_M77_SET);
}

void MGD77_Set_Home (struct MGD77_CONTROL *F)
{
	char *this;

	if (F->MGD77_HOME) return;	/* Already set elsewhere */

	if ((this = getenv ("MGD77_HOME")) != CNULL) {	/* MGD77_HOME was set */
		F->MGD77_HOME = (char *) GMT_memory (VNULL, (size_t)(strlen (this) + 1), 1, "MGD77_Set_Home");
		strcpy (F->MGD77_HOME, this);
	}
	else {	/* Set default path */
		fprintf (stderr, "mgd77: Warning: MGD77_HOME not defined, set to %s/mgd77\n", GMT_SHAREDIR);
		F->MGD77_HOME = (char *) GMT_memory (VNULL, (size_t)(strlen (GMT_SHAREDIR) + 7), 1, "MGD77_Set_Home");
		sprintf (F->MGD77_HOME, "%s%cmgd77", GMT_SHAREDIR, DIR_DELIM);
	}
}

void MGD77_Path_Init (struct MGD77_CONTROL *F)
{
	int i;
	size_t n_alloc = GMT_SMALL_CHUNK;
	char file[BUFSIZ], line[BUFSIZ];
	FILE *fp;
	
	MGD77_Set_Home (F);

	sprintf (file, "%s%cmgd77_paths.txt", F->MGD77_HOME, DIR_DELIM);
	
	F->n_MGD77_paths = 0;

	if ((fp = GMT_fopen (file, "r")) == NULL) {
		fprintf (stderr, "%s: Warning: path file %s for MGD77 files not found\n", GMT_program, file);
		fprintf (stderr, "%s: (Will only look in current directory for such files)\n", GMT_program);
		return;
	}
	
	F->MGD77_datadir = (char **) GMT_memory (VNULL, n_alloc, sizeof (char *), "MGD77_path_init");
	while (fgets (line, BUFSIZ, fp)) {
		if (line[0] == '#') continue;	/* Comments */
		if (line[0] == ' ' || line[0] == '\0') continue;	/* Blank line, \n included in count */
		GMT_chop (line);
		F->MGD77_datadir[F->n_MGD77_paths] = GMT_memory (VNULL, (size_t)1, (size_t)(strlen (line)+1), "MGD77_path_init");
#if _WIN32
		for (i = 0; line[i]; i++) if (line[i] == '/') line[i] = DIR_DELIM;
#else
		for (i = 0; line[i]; i++) if (line[i] == '\\') line[i] = DIR_DELIM;
#endif
		strcpy (F->MGD77_datadir[F->n_MGD77_paths], line);
		F->n_MGD77_paths++;
		if (F->n_MGD77_paths == (int)n_alloc) {
			n_alloc <<= 1;
			F->MGD77_datadir = (char **) GMT_memory ((void *)F->MGD77_datadir, n_alloc, sizeof (char *), "MGD77_path_init");
		}
	}
	GMT_fclose (fp);
	F->MGD77_datadir = (char **) GMT_memory ((void *)F->MGD77_datadir, (size_t)F->n_MGD77_paths, sizeof (char *), "MGD77_path_init");
}

void MGD77_Cruise_Explain (void)
{
	fprintf (stderr, "\t<cruises> can be one of five kinds of specifiers:\n");
	fprintf (stderr, "\t1) 8-character NGDC IDs, e.g., 01010083, JA010010, etc., etc.\n");
	fprintf (stderr, "\t2) 2-character <agency> codes which will return all cruises from each agency.\n");
	fprintf (stderr, "\t3) 4-character <agency><vessel> codes, which will return all cruises from those vessels.\n");
	fprintf (stderr, "\t4) A single =<list>, where <list> is a table with NGDC IDs, one per line.\n");
	fprintf (stderr, "\t5) If nothing is specified we return all cruises in the data base.\n");
	fprintf (stderr, "\t   [See the documentation for agency and vessel codes].\n");
}

int MGD77_Path_Expand (struct MGD77_CONTROL *F, char **argv, int argc, char ***list)
{
	/* Traverse the MGD77 directories in search of files matching the given arguments (or get all if none) */
	
	int i, j, k, n = 0, flist = 0, length, compare_L (const void *p1, const void *p2);
	BOOLEAN all;
	size_t n_alloc = 0;
	char **L = NULL, *d_name, line[BUFSIZ];
#ifdef WIN32
	FILE *fp;
#else
	DIR *dir;
	struct dirent *entry;
#endif
	
	for (j = 1; j < argc; j++) {	/* First count the number of cruise arguments, if any */
		if (argv[j][0] == '-') continue;	/* Skip command line options */
		if (argv[j][0] == '=') {		/* Specified a file list of files */
			flist = j;
			continue;
		}
		n++;
	}
	
	all = (flist == 0 && n == 0);	/* If nothing is specified we select everything */
	n = 0;
	
	if (flist) {	/* Just read and return the list of files in the given file list; skip leading = in filename */
		FILE *fp;
		if ((fp = GMT_fopen (&argv[flist][1], "r")) == NULL) {
			fprintf (stderr, "%s: WARNING: Unable to open file list %s\n", GMT_program, &argv[flist][1]);
			GMT_exit (EXIT_FAILURE);
		}
		while (fgets (line, BUFSIZ, fp)) {
			GMT_chop (line);	/* Get rid of CR/LF issues */
			if (line[0] == '#' || line[0] == '>' || (length = strlen (line)) == 0) continue;	/* Skip comments and blank lines */
			if (n == (int)n_alloc) L = (char **)GMT_memory ((void *)L, n_alloc += GMT_CHUNK, sizeof (char *), "MGD77_Path_Expand");
			L[n] = (char *)GMT_memory (VNULL, length+1, sizeof (char), "MGD77_Path_Expand");
			strcpy (L[n++], line);
		}
		GMT_fclose (fp);
	}

	for (j = 1; j < argc; j++) {
		if (!all && argv[j][0] == '-') continue;	/* Skip command line options, except first time if all */
		length = (all) ? 0 : strlen (argv[j]);		/* length == 0 means get all */
		if (length == 8) {	/* Full NGDC ID length, append to list */
			if (n == (int)n_alloc) L = (char **)GMT_memory ((void *)L, n_alloc += GMT_CHUNK, sizeof (char *), "MGD77_Path_Expand");
			L[n] = (char *)GMT_memory (VNULL, 9, sizeof (char), "MGD77_Path_Expand");
			strcpy (L[n++], argv[j]);
			continue;
		}
		/* Here we have either <agency> or <agency><vessel> code or blank for all */	
		for (i = 0; i < F->n_MGD77_paths; i++) {	/* Examine all directories */
#ifdef WIN32
			/* We simulate Unix opendir/readdir/closedir by listing the directory to a temp file */
			sprintf (line, "dir /b %s > .tmpdir", F->MGD77_datadir[i]);
			system (line);
			fp = fopen (".tmpdir", "r");
			while (fgets (line, BUFSIZ, fp)) {
				GMT_chop (line);	/* Get rid of CR/LF issues */
				d_name = line;
#else
			/* The directory search is only supported on Unix-like systems for now */
			/* Here we have either <agency> or <agency><vessel> code or blank for all */	
			if ((dir = opendir (F->MGD77_datadir[i])) == NULL) {
				fprintf (stderr, "%s: WARNING: Unable to open directory %s\n", GMT_program, F->MGD77_datadir[i]);
				continue;
			}
			while ((entry = readdir (dir)) != NULL) {
				d_name = entry->d_name;
#endif
				if (length && strncmp (d_name, argv[j], length)) continue;
				k = strlen (d_name) - 1;
				while (k && d_name[k] != '.') k--;	/* Strip off file extension */
				if (k < 8) continue;	/* Not a NGDC 8-char ID */
				if (n == (int)n_alloc) L = (char **)GMT_memory ((void *)L, n_alloc += GMT_CHUNK, sizeof (char *), "MGD77_Path_Expand");
				L[n] = (char *)GMT_memory (VNULL, k + 1, sizeof (char), "MGD77_Path_Expand");
				strncpy (L[n], d_name, k);
				L[n++][k] = '\0';
			}
#ifdef WIN32
			fclose (fp);
			remove (".tmpdir");
#else
			closedir (dir);
#endif
		}
		all = FALSE;	/* all is only TRUE once (or never) inside this loop */
	}
	
	if (n) {	/* Avoid duplicates by sorting and removing them */
		qsort ((void *)L, (size_t)n, sizeof (char *), compare_L);
		for (i = j = 1; j < n; j++) {
			if (i != j) L[i] = L[j];
			if (strcmp (L[i], L[i-1])) i++;
		}
		n = i;
	}
	
	if (n != (int)n_alloc) L = (char **)GMT_memory ((void *)L, n, sizeof (char *), "MGD77_Path_Expand");
	*list = L;
	return (n);
}

int compare_L (const void *p1, const void *p2)
{	/* Only used in MGD77_Path_Expand */
	char **a, **b;
	a = (char **)p1;
	b = (char **)p2;
	return (strcmp (*a, *b));
}

/* MGD77_Get_Path takes a track name as argument and returns the full path
 * to where this data file can be found.  MGD77_path_init must be called first.
 * Return 1 if there is a problem (not found)
 */
 
int MGD77_Get_Path (char *track_path, char *track, struct MGD77_CONTROL *F)
{	/* Assemple proper path to READ a mgd77 file.
	 * track may be:
	 *  a) a complete hardpath, which is copied verbatim to track_path
	 *  b) a local file with extension, which is copied to track_path
	 *  c) a leg name (no extension), in which we try
	 *	- append .mgd77+ and see if we can find it in listed directories
	 *      - append .mgd77 and see if we can find it in listed directories
	 */
	int id, fmt, f_start, f_stop, k, has_suffix = MGD77_NOT_SET;
	BOOLEAN append = FALSE;
	char geo_path[BUFSIZ];
	
	for (k = 0; k < MGD77_FORMAT_ANY; k++) {	/* Determine if given track name contains one of the 3 possible extensions */
		if ((strlen(track)-strlen(MGD77_suffix[k])) > 0 && !strncmp (&track[strlen(track)-strlen(MGD77_suffix[k])], MGD77_suffix[k], strlen(MGD77_suffix[k]))) has_suffix = k;
	}

	if (has_suffix != MGD77_NOT_SET && !MGD77_format_allowed[has_suffix]) {	/* Filename clashes with allowed extensions */
		fprintf (stderr, "%s: Error: File has suffix (%s) that is set to be ignored!\n", GMT_program, MGD77_suffix[has_suffix]);
		return (MGD77_FILE_NOT_FOUND);
	}
	if (has_suffix == MGD77_NOT_SET && (track[0] == '/' || track[1] == ':')) {	/* Hard path given without extension */
		fprintf (stderr, "%s: Error: Hard path (%s) has no recognized extension!\n", GMT_program, track);
		return (MGD77_FILE_NOT_FOUND);
	}
	
	if (track[0] == '/' || track[1] == ':') {	/* Hard path given (assumes X: is beginning of DOS path for arbitrary drive letter X) */
		if (!access (track, R_OK)) {	/* OK, found it */
			F->format = has_suffix;	/* Set this format */
			strcpy (track_path, track);
			return (MGD77_NO_ERROR);
		}
		else
			return (MGD77_FILE_NOT_FOUND);	/* Hard path did not work */
	}
	
	switch (((has_suffix == MGD77_NOT_SET) ? MGD77_FORMAT_ANY : has_suffix)) {
		case MGD77_FORMAT_M77:		/* Look for MGD77 ASCII files only */
			f_start = f_stop = MGD77_FORMAT_M77;
			break;
		case MGD77_FORMAT_CDF:		/* Look for MGD77+ netCDF files only */
			f_start = f_stop = MGD77_FORMAT_CDF;
			break;
		case MGD77_FORMAT_TBL:		/* Look for ASCII DAT files only */
			f_start = f_stop = MGD77_FORMAT_TBL;
			break;
		case MGD77_FORMAT_ANY:		/* Not set, try all */
			f_start = MGD77_FORMAT_M77;
			f_stop  = MGD77_FORMAT_TBL;
			break;
		default:	/* Bad */
			fprintf (stderr, "%s: Bad file format specified given (%d)\n", GMT_program, F->format);
			GMT_exit (EXIT_FAILURE);
			break;
	}
	
	append = (has_suffix == MGD77_NOT_SET);		/* No extension, must append extension */
	
	/* First look in current directory using all allowed suffices */
	
	for (fmt = f_start; fmt <= f_stop; fmt++) {	/* Try either one or any of three formats... */
		if (!MGD77_format_allowed[fmt]) continue;		/* ...but not this one, apparently */
		if (append)	/* No extension, must append extension */
			sprintf (geo_path, "%s.%s", track, MGD77_suffix[fmt]);
		else 
			strcpy (geo_path, track);	/* Extension already there */
	
		/* Here we have a relative path.  First look in current directory */
	
		if (!access (geo_path, R_OK)) {	/* OK, found it */
			strcpy (track_path, geo_path);
			F->format = fmt;
			return (MGD77_NO_ERROR);
		}
	}
	
	/* Not in current directory.  Now look in the MGD77 list of directories */
	
	for (fmt = f_start; fmt <= f_stop; fmt++) {	/* Try either one or any of three formats... */
		if (!MGD77_format_allowed[fmt]) continue;		/* ...but not this one, apparently */
		for (id = 0; id < F->n_MGD77_paths; id++) {	/* try each directory */
			if (append)
				sprintf (geo_path, "%s%c%s.%s", F->MGD77_datadir[id], DIR_DELIM, track, MGD77_suffix[fmt]);
			else
				sprintf (geo_path, "%s%c%s", F->MGD77_datadir[id], DIR_DELIM, track);
			if (!access (geo_path, R_OK)) {
				strcpy (track_path, geo_path);
				F->format = fmt;
				return (MGD77_NO_ERROR);
			}
		}
	}
	
	return (MGD77_FILE_NOT_FOUND);	/* No luck */
}

void MGD77_Apply_Bitflags (struct MGD77_CONTROL *F, struct MGD77_DATASET *S, int rec, BOOLEAN apply_bits[])
{
	int set, i;
	double *value;
	
	/* We get here when we need to take action on the bitflags */
	
	for (i = 0; i < F->n_out_columns; i++) {
		set = F->order[i].set;
		if (apply_bits[set] && (S->flags[set][rec] & (1 << F->order[i].item))) {
			value = (double *)S->values[i];
			value[rec] = GMT_d_NaN;
		}
	}
}

BOOLEAN MGD77_Pass_Record (struct MGD77_CONTROL *F, struct MGD77_DATASET *S, int rec)
{
	int i, col, c, id, match, n_passed;
	BOOLEAN pass;
	double *value;
	char *text;
	
	if (F->no_checking) return (TRUE);	/* Nothing to check for - get outa here */
	
	if (F->n_exact) {	/* Must make sure that none of these key geophysical columnss are NaN */
		for (i = 0; i < F->n_exact; i++) {
			value = (double *)S->values[F->Exact[i].col];
			if (GMT_is_dnan (value[rec])) return (FALSE);	/* Sorry, one NaN and you're history */
		}
	}
	
	if (F->n_constraints) {	/* Must pass all constraints to be successful */
		for (i = n_passed = 0; i < F->n_constraints; i++) {
			col = F->Constraint[i].col;
			c  = F->order[col].set;
			id = F->order[col].item;
			if (S->H.info[c].col[id].text) {
				text = (char *)S->values[col];
				pass = F->Constraint[i].string_test (&text[rec*S->H.info[c].col[id].text], F->Constraint[i].c_constraint, S->H.info[c].col[id].text);
			}
			else {
				value = (double *)S->values[col];
				pass = F->Constraint[i].double_test (value[rec], F->Constraint[i].d_constraint);
			}
			if (pass)	/* OK, we survived for now, tally up victories and goto next battle */
				n_passed++;
			else if (F->Constraint[i].exact)	/* Oops, we failed a must-pass test... */
				return (FALSE);
		}
		return (n_passed > 0);	/* Pass if we passed at least one test, since failing any exact test would have returned by now */
	}
		
	if (F->n_bit_tests) {	/* Must pass ALL bit tests */
		for (i = 0; i < F->n_bit_tests; i++) {
			match = (S->flags[F->Bit_test[i].set][rec] & MGD77_this_bit[F->Bit_test[i].item]);	/* TRUE if flags bit #item is set */
			if (match != F->Bit_test[i].match) return (FALSE);				/* Sorry, one missed test and you're history */
		}
	}
	
	return (TRUE);	/* We live to fight another day (i.e., record) */
}

BOOLEAN MGD77_lt_test (double value, double limit)
{
	/* Test that checks for value < limit */
	
	if (GMT_is_dnan (value)) return (FALSE);	/* Cannot pass a test with a NaN */
	return (value < limit);
}

BOOLEAN MGD77_le_test (double value, double limit)
{
	/* Test that checks for value <= limit */
	
	if (GMT_is_dnan (value)) return (FALSE);	/* Cannot pass a test with a NaN */
	return (value <= limit);
}

BOOLEAN MGD77_eq_test (double value, double limit)
{
	/* Test that checks for value == limit */
	
	if (GMT_is_dnan (value) && GMT_is_dnan (limit)) return (TRUE);	/* Matching two NaNs is OK... */
	if (GMT_is_dnan (value) || GMT_is_dnan (limit)) return (FALSE);	/* ...but if only one of them is NaN we fail */
	return (value == limit);
}

BOOLEAN MGD77_bit_test (double value, double limit)
{
	unsigned int ivalue, ilimit;
	
	/* Test that checks for (value & limit) > 0 */
	/* We except both value and limit to be integers encoded as doubles, but we first check for NaNs anyway */
	
	if (GMT_is_dnan (value)) return (FALSE);	/* Cannot pass a test with a NaN */
	if (GMT_is_dnan (limit)) return (FALSE);	/* Cannot pass a test with a NaN */
	ivalue = (unsigned int) irint (value);
	ilimit = (unsigned int) irint (limit);
	return (ivalue & ilimit);			/* TRUE if any of the bits in limit line up with value */
}

BOOLEAN MGD77_neq_test (double value, double limit)
{
	/* Test that checks for value != limit */
	
	if (GMT_is_dnan (value) && GMT_is_dnan (limit)) return (FALSE);	/* Both NaNs so we fail */
	if (GMT_is_dnan (value) || GMT_is_dnan (limit)) return (TRUE);	/* ...but if only one of them is NaN it is OK */
	return (value != limit);
}

BOOLEAN MGD77_ge_test (double value, double limit)
{
	/* Test that checks for value >= limit */
	
	if (GMT_is_dnan (value)) return (FALSE);	/* Cannot pass a test with a NaN */
	return (value >= limit);
}

BOOLEAN MGD77_gt_test (double value, double limit)
{
	/* Test that checks for value > limit */
	
	if (GMT_is_dnan (value)) return (FALSE);	/* Cannot pass a test with a NaN */
	return (value > limit);
}

BOOLEAN MGD77_clt_test (char *value, char *match, int len)
{
	/* Test that checks for value < match for strings */
	
	return (strncmp (value, match, len) < 0);
}

BOOLEAN MGD77_cle_test (char *value, char *match, int len)
{
	/* Test that checks for value <= match for strings */
	
	return (strncmp (value, match, len) <= 0);
}

BOOLEAN MGD77_ceq_test (char *value, char *match, int len)
{
	/* Test that checks for value == match for strings */
	
	return (strncmp (value, match, len) == 0);
}

BOOLEAN MGD77_cneq_test (char *value, char *match, int len)
{
	/* Test that checks for value != match for strings */
	
	return (strncmp (value, match, len) != 0);
}

BOOLEAN MGD77_cge_test (char *value, char *match, int len)
{
	/* Test that checks for value >= match for strings */
	
	return (strncmp (value, match, len) >= 0);
}

BOOLEAN MGD77_cgt_test (char *value, char *match, int len)
{
	/* Test that checks for value > match for strings */
	
	return (strncmp (value, match, len) > 0);
}

void MGD77_Set_Unit (char *dist, double *scale, int way)
{	/* Return scale needed to convert a unit distance in the given unit to meter.
	 * If way is -1 we return the inverse (convert meters to given unit) */
	
	switch (dist[strlen(dist)-1]) {
		case 'k':	/* km */
			*scale = 1000.0;
			break;
		case 'm':	/* miles */
			*scale = MGD77_METERS_PER_M;
			break;
		case 'n':	/* nautical miles */
			*scale = MGD77_METERS_PER_NM;
			break;
		default:
			*scale = 1.0;
			break;
	}
	if (way == -1) *scale = 1.0 / *scale;
}

void MGD77_Fatal_Error (int error)
{
	fprintf (stderr, "%s: Error [%d]: ", GMT_program, error);
	switch (error) {
		case MGD77_NO_HEADER_REC:
			fprintf (stderr, "Header record not found");
			break;
		case MGD77_ERROR_READ_HEADER_ASC:
			fprintf (stderr, "Error reading ASCII header record");
			break;
		case MGD77_ERROR_READ_HEADER_BIN:
			fprintf (stderr, "Error reading binary header record");
			break;
		case MGD77_ERROR_WRITE_HEADER_ASC:
			fprintf (stderr, "Error writing ASCII header record");
			break;
		case MGD77_ERROR_WRITE_HEADER_BIN:
			fprintf (stderr, "Error writing binary header record");
			break;
		case MGD77_WRONG_HEADER_REC:
			fprintf (stderr, "Wrong header record was read");
			break;
		case MGD77_NO_DATA_REC:
			fprintf (stderr, "Data record not found");
			break;
		case MGD77_ERROR_READ_ASC_DATA:
			fprintf (stderr, "Error reading ASCII data record");
			break;
		case MGD77_ERROR_READ_BIN_DATA:
			fprintf (stderr, "Error reading binary data record");
			break;
		case MGD77_ERROR_WRITE_ASC_DATA:
			fprintf (stderr, "Error writing ASCII data record");
			break;
		case MGD77_ERROR_WRITE_BIN_DATA:
			fprintf (stderr, "Error writing binary data record");
			break;
		case MGD77_WRONG_DATA_REC_LEN:
			fprintf (stderr, "Data record has incorrect length");
			break;
		case MGD77_ERROR_CONV_DATA_REC:
			fprintf (stderr, "Error converting a field in current data record");
			break;
		case MGD77_ERROR_NOT_MGD77PLUS:
			fprintf (stderr, "File is not in MGD77+ format");
			break;
		case MGD77_UNKNOWN_FORMAT:
			fprintf (stderr, "Unknown file format specifier");
			break;
		case MGD77_UNKNOWN_MODE:
			fprintf (stderr, "Unknown file open/create mode");
			break;
		case MGD77_ERROR_NOSUCHCOLUMN:
			fprintf (stderr, "Column not in present file");
			break;
		case MGD77_BAD_ARG:
			fprintf (stderr, "Bad arument given to MGD77_Place_Text");
			break;
		default:
			fprintf (stderr, "Unrecognized error");
			break;
	}
		
	GMT_exit (EXIT_FAILURE);
}

/* MGD77+ functions will be added down here */

void MGD77_Prep_Header_cdf (struct MGD77_CONTROL *F, struct MGD77_DATASET *S)
{	/* Must determine which columns are present and if time is available, etc.
	 * MUST BE CALLED BEFORE MGD77_Write_Header_Record_cdf!
	 * 
	 */
	 
	int i, id, t_id, set, t_set = MGD77_NOT_SET, entry;
	BOOLEAN crossed_dateline = FALSE, crossed_greenwich = FALSE;
	char *text;
	double *values, dx;
	
	entry = MGD77_Info_from_Abbrev ("time", &S->H, &t_set, &t_id);
	if (entry != MGD77_NOT_SET) {	/* Supposedly has time, but we we'll check again */
		values = (double *)S->values[entry];
		if (MGD77_dbl_are_constant (values, S->H.n_records, S->H.info[t_set].col[t_id].limit)) {	/* If constant time it means NaNs */
			S->H.no_time = TRUE;
			S->H.info[t_set].col[t_id].present = FALSE;
			for (id = entry; id < S->H.n_fields; id++) S->values[id] = S->values[id+1];	/* Shuffle fields one up */
			S->H.n_fields--;
		}
		else
			S->H.no_time = FALSE;
	}
	else
		S->H.no_time = TRUE;	/* Some cruises do not have time */
	
	entry = MGD77_Info_from_Abbrev ("lon", &S->H, &t_set, &t_id);
	if (entry == MGD77_NOT_SET) {	/* Not good */
		fprintf (stderr, "%s: Longitude not present!\n", GMT_program);
		GMT_exit (EXIT_FAILURE);
	}
	
	/* Determine if there is a longitude jump and if so shift longitudes to avoid it.
	   This is done to be in compliance with COARDS which says there should be no jump in longitude.
	 */

	values = (double *)S->values[entry];
	for (i = 1; i < S->H.n_records; i++) {	/* Look at pairs of longitudes for jumps */
		dx = values[i] - values[i-1];
		if (fabs (dx) > 180.0) {	/* Crossed Greenwich or Dateline, depending on range */
			if (MIN (values[i], values[i-1]) < 0.0) /* Crossed Dateline with lons in -180 and +180 format */
				crossed_dateline = TRUE;
			else
				crossed_greenwich = TRUE;
		}
	}
	if (crossed_dateline && crossed_greenwich)
		fprintf (stderr, "%s: Warning: Longitude crossing both Dateline and Greenwich; not adjusted!\n", GMT_program);
	else if (crossed_dateline) {	/* Cruise is crossing Dateline; switch to 0-360 format for COARDS compliancy */
		for (i = 0; i < S->H.n_records; i++) if (values[i] < 0.0) values[i] += 360.0;
	}
	else if (crossed_greenwich) {	/* Cruise is crossing Greenwich; switch to -180/+180 format for COARDS compliancy */
		for (i = 0; i < S->H.n_records; i++) if (values[i] > 180.0) values[i] -= 360.0;
	}

	for (set = entry = 0; set < MGD77_N_SETS; set++) {	/* For both sets */
		for (id = 0; id < MGD77_SET_COLS; id++) {
			if (!S->H.info[set].col[id].present) continue;	/* No such field, move on */
			if (S->H.info[set].col[id].text) {		/* This variable is a text string */
				text = (char *)S->values[entry];
				S->H.info[set].col[id].constant = (MGD77_txt_are_constant (text, S->H.n_records, S->H.info[set].col[id].text));	/* Do we need to store 1 or n strings? */
			}
			else {					/* This variable is a numerical field */
				values = (double *)S->values[entry];
				S->H.info[set].col[id].constant = (MGD77_dbl_are_constant (values, S->H.n_records, S->H.info[set].col[id].limit));
			}
			entry++;
		}
	}
}

int MGD77_Write_Header_Record_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_HEADER *H)
{	/* This function will create a netCDF version of a standard MGD77 file.  No additional
	 * columns are considered.  Such columns may be added/deleted by mgd77manage.  We assume
	 * that the dataset was read by MGD77_Read_File_asc which will return the entire set
	 * of columns so that we can assume S->values[MGD77_TWT] etc is in the right column.
	 * Only columns that are all non-NaN are written, and columns with constant values are
	 * written as scalars.  The read routine will replicate these to columns.
	 * This function simply defines the file and header attributes and is called by
	 * MGD77_Write_File_cdf which also writes the data.  Note that no optional factors
	 * such as 2ndary correction scale and offset are defined since they do not exist
	 * for MGD77 standard files.  Such terms can be added by mgd77manage.
	 * 
	 */
	 
	int id, j, k, set, entry, time_id, dims[2] = {0, 0}, var_id;
	time_t now;
	char string[128];
	FILE *fp_err;
	
	if (MGD77_Open_File (file, F, MGD77_WRITE_MODE)) return (-1);	/* Basically creates the full path */
	
	fp_err = (F->verbose_dest == 1) ? GMT_stdout : stderr;

	MGD77_nc_status (nc_create (F->path, NC_NOCLOBBER, &F->nc_id));	/* Create the file */
	
	/* Put attributes header, author, title and history */
	
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "Conventions", strlen (MGD77_CDF_CONVENTION) + 1, (const char *)MGD77_CDF_CONVENTION));
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "Version",     strlen(MGD77_CDF_VERSION), (const char *)MGD77_CDF_VERSION));
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "Author",      strlen (H->author), H->author));
	sprintf (string, "Cruise %s (NGDC ID %s)", H->mgd77->Survey_Identifier, F->NGDC_id);
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "title", strlen (string), string));
	if (!H->history) {	/* Blank history, set initial message */
		(void) time (&now);
		sprintf (string, "%s [%s] Conversion from MGD77 ASCII to MGD77+ netCDF format", ctime(&now), H->author);
		k = strlen (string);
		for (j = 0; j < k; j++) if (string[j] == '\n') string[j] = ' ';	/* Remove the \n returned by ctime() */
		string[k++] = '\n';	string[k] = '\0';	/* Add LF at end of line */
		H->history = (char *)GMT_memory (VNULL, k, sizeof (char), GMT_program);
		strcpy (H->history, string);
	}
	/* else, history already filled out, use as is */
	MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "history", strlen (H->history), H->history));
	if (H->E77 && strlen(H->E77) > 0) MGD77_nc_status (nc_put_att_text (F->nc_id, NC_GLOBAL, "E77", strlen (H->E77), H->E77));
	MGD77_Write_Header_Params (F, H->mgd77);	/* Write all the MGD77 header attributes */
	
	/* It is assumed that MGD77_Prep_Header_cdf has been called */
	
	if (H->no_time) {
		if (gmtdefs.verbose) fprintf (stderr, "%s: Data set %s has no time values\n", GMT_program, file);
		MGD77_nc_status (nc_def_dim (F->nc_id, "record_no", NC_UNLIMITED, &F->nc_recid));	/* Define unlimited record dimension */
		time_id = MGD77_NOT_SET;
	}
	else {
		MGD77_nc_status (nc_def_dim (F->nc_id, "time", NC_UNLIMITED, &F->nc_recid));		/* Define unlimited time dimension */
		entry = MGD77_Info_from_Abbrev ("time", H, &set, &time_id);
	}

	dims[0] = F->nc_recid;	/* Number of points in all arrays */
	for (set = entry = 0; set < MGD77_N_SETS; set++) {	/* For both sets */
		for (id = 0; id < MGD77_SET_COLS; id++) {
			if (!H->info[set].col[id].present) continue;	/* No such field, move on */
			if (H->info[set].col[id].text) {			/* This variable is a text string */
				sprintf (string, "%s_dim", H->info[set].col[id].abbrev);
				MGD77_nc_status (nc_def_dim (F->nc_id, string, H->info[set].col[id].text, &dims[1]));	/* Define character length dimension */
				if (H->info[set].col[id].constant) {	/* Simply store one value */
					MGD77_nc_status (nc_def_var (F->nc_id, H->info[set].col[id].abbrev, H->info[set].col[id].type, 1, &dims[1], &var_id));	/* Define a 1-text variable */
				}
				else {	/* Must store array */
					MGD77_nc_status (nc_def_var (F->nc_id, H->info[set].col[id].abbrev, H->info[set].col[id].type, 2, dims, &var_id));	/* Define a n-text variable */
				}
			}
			else {					/* This variable is a numerical field */
				if (H->info[set].col[id].constant) {	/* Simply store one value */
					MGD77_nc_status (nc_def_var (F->nc_id, H->info[set].col[id].abbrev, H->info[set].col[id].type, 0, NULL, &var_id));	/* Define a scalar variable */
				}
				else {	/* Must store array */
					MGD77_nc_status (nc_def_var (F->nc_id, H->info[set].col[id].abbrev, H->info[set].col[id].type, 1, dims, &var_id));	/* Define an array variable */
				}
			}
			if (H->info[set].col[id].name && strcmp (H->info[set].col[id].name, H->info[set].col[id].abbrev)) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "long_name", strlen (H->info[set].col[id].name), H->info[set].col[id].name));
			if (H->info[set].col[id].units) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "units", strlen (H->info[set].col[id].units), H->info[set].col[id].units));
			if (!H->info[set].col[id].constant) MGD77_nc_status (nc_put_att_double   (F->nc_id, var_id, "actual_range", NC_DOUBLE, 2, H->info[set].col[id].limit));
			if (H->info[set].col[id].comment) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "comment", strlen (H->info[set].col[id].comment), H->info[set].col[id].comment));
			if (set == MGD77_M77_SET && (!strcmp (H->info[set].col[id].abbrev, "depth") || !strcmp (H->info[set].col[id].abbrev, "msd"))) MGD77_nc_status (nc_put_att_text   (F->nc_id, var_id, "positive", 4, "down"));
			if (!(set == MGD77_M77_SET && id == time_id)) {	/* Time coordinate value cannot have missing values */
				MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "_FillValue", H->info[set].col[id].type, 1, &MGD77_NaN_val[H->info[set].col[id].type]));
				MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "missing_value", H->info[set].col[id].type, 1, &MGD77_NaN_val[H->info[set].col[id].type]));
			}
			if (H->info[set].col[id].factor != 1.0) MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "scale_factor", NC_DOUBLE, 1, &H->info[set].col[id].factor));
			if (H->info[set].col[id].offset != 0.0) MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "add_offset", NC_DOUBLE, 1, &H->info[set].col[id].offset));
			if (H->info[set].col[id].corr_factor  != 1.0) MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "corr_factor", NC_DOUBLE, 1, &H->info[set].col[id].corr_factor));
			if (H->info[set].col[id].corr_offset != 0.0) MGD77_nc_status (nc_put_att_double (F->nc_id, var_id, "corr_offset", NC_DOUBLE, 1, &H->info[set].col[id].corr_offset));
			H->info[set].col[id].var_id = var_id;
			entry++;
		}
	}
	
	MGD77_nc_status (nc_enddef (F->nc_id));
	
	return (MGD77_NO_ERROR);
}

int MGD77_Write_File_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)
{	/* This function will create a netCDF version of a standard MGD77 file.  No additional
	 * columns are considered.  Such columns may be added/deleted by mgd77manage.  We assume
	 * that the dataset was read by MGD77_Read_File_asc which will return the entire set
	 * of columns so that we can assume S->values[MGD77_TWT] etc is in the right column.
	 * All MGD77 columns are written, but those with constant values (or all NaN) are
	 * written as scalars.  The read routine will replicate these to columns.
	 */
	 
	int err;
	
	MGD77_Prep_Header_cdf (F, S);

	err = MGD77_Write_Header_Record_cdf (file, F, &S->H);	/* Upon successful return the netcdf file is in open mode */
	if (err) return (err);
	
	err = MGD77_Write_Data_cdf (file, F, S);
	if (err) return (err);

	MGD77_nc_status (nc_close (F->nc_id));

	return (MGD77_NO_ERROR);
}

int MGD77_Write_Data_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)
{	/* This function will create a netCDF version of a standard MGD77 file.  No additional
	 * columns are considered.  Such columns may be added/deleted by mgd77manage.  We assume
	 * that the dataset was read by MGD77_Read_File_asc which will return the entire set
	 * of columns so that we can assume S->values[MGD77_TWT] etc is in the right column.
	 * All columns are written, but columns with constant values (or all NaNs) are
	 * written as scalars.  The read routine will replicate these to columns.
	 */
	 
	int id, set, entry, n_bad = 0;
	size_t start[2] = {0, 0}, count[2] = {0, 0};
	double *values, *x, *xtmp = NULL, single_val, scale, offset;
	char *text;
	BOOLEAN transform, not_allocated = TRUE;
	FILE *fp_err;
	
	count[0] = S->H.n_records;
	fp_err = (F->verbose_dest == 1) ? GMT_stdout : stderr;
	
	for (set = entry = 0; set < MGD77_N_SETS; set++) {	/* For both sets */
		for (id = 0; id < MGD77_SET_COLS; id++) {
			if (!S->H.info[set].col[id].present) continue;	/* No such field, move on */
			if (S->H.info[set].col[id].text) {		/* This variable is a text string */
				count[1] = S->H.info[set].col[id].text;	/* Set text dimension */
				text = (char *)S->values[entry];
				if (S->H.info[set].col[id].constant)	/* Only need to store one text string */
					MGD77_nc_status (nc_put_vara_schar (F->nc_id, S->H.info[set].col[id].var_id, start, &count[1], (signed char *)text));
				else
					MGD77_nc_status (nc_put_vara_schar (F->nc_id, S->H.info[set].col[id].var_id, start, count, (signed char *)text));
			}
			else {						/* Numerical data */
				scale = S->H.info[set].col[id].factor;
				offset = S->H.info[set].col[id].offset;
				if (F->use_corrections[set]) {	/* TRUE by default, but this can be turned off by changing this parameter in F */
					/* Combine effect of main and 2nd scale factors and 2nd scale and 2nd offset into one offset */
					scale *= S->H.info[set].col[id].corr_factor;
					offset = S->H.info[set].col[id].offset * S->H.info[set].col[id].corr_factor + S->H.info[set].col[id].corr_offset;
				}
				transform = (! (scale == 1.0 && offset == 0.0));	/* TRUE if we must transform before writing */
				values = (double *)S->values[entry];			/* Pointer to current double array */
				if (S->H.info[set].col[id].constant) {	/* Only write a single value (after possibly transforming) */
					n_bad = MGD77_do_scale_offset_before_write (&single_val, values, 1, scale, offset, S->H.info[set].col[id].type);
					MGD77_nc_status (nc_put_var1_double (F->nc_id, S->H.info[set].col[id].var_id, start, &single_val));
				}
				else {	/* Must write the entire array */
					if (transform) {	/* Must use temprary storage for scalings so that original values in S->values remain unchanged */
						if (not_allocated) xtmp = (double *) GMT_memory (VNULL, count[0], sizeof (double), "MGD77_Write_Data_cdf");	/* Get mem the first time */
						not_allocated = FALSE;	/* No longer the first time */
						n_bad = MGD77_do_scale_offset_before_write (xtmp, values, S->H.n_records, scale, offset, S->H.info[set].col[id].type);	/* mod copy */
						x = xtmp;	/* Points to modified copy */
					}
					else {	/* Save as is */
						x = values;	/* Points to original values */
						n_bad = 0;	/* No chance to find bad ones */
					}
					MGD77_nc_status (nc_put_vara_double (F->nc_id, S->H.info[set].col[id].var_id, start, count, x));
				}
				if (n_bad) {	/* Report what we found */
					if (F->verbose_level | 1) fprintf (stderr, "%s: %s [%s] had %d values outside valid range <%g,%g> for the chosen type (set to NaN = %g)\n",
						GMT_program, F->NGDC_id, S->H.info[set].col[id].abbrev, n_bad, MGD77_Low_val[S->H.info[set].col[id].type],
						MGD77_High_val[S->H.info[set].col[id].type], MGD77_NaN_val[S->H.info[set].col[id].type]);
				}
			}
			entry++;
			S->errors += n_bad;
		}
	}
	
	if (xtmp) GMT_free ((void *)xtmp);
	
	return (MGD77_NO_ERROR);
}

int MGD77_Read_File_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)
{
	int err;
	
	err = MGD77_Read_Header_Record_cdf (file, F, &S->H);  /* Read all meta information from header */
	if (err) return (err);
	
	MGD77_Select_All_Columns (F, &S->H);

	err = MGD77_Read_Data_cdf (file, F, S);
	if (err) return (err);

	MGD77_nc_status (nc_close (F->nc_id));

	return (MGD77_NO_ERROR);
}

int MGD77_Read_Data_cdf (char *file, struct MGD77_CONTROL *F, struct MGD77_DATASET *S)
{
	int i, k, c, id;
	size_t start[2] = {0, 0}, count[2] = {0, 0};
	unsigned int *flags;
	char *text, *flagname[MGD77_N_SETS] = {"MGD77_flags", "CDF_flags"};
	double scale, offset, *values;
	
	if (MGD77_Open_File (file, F, MGD77_READ_MODE)) return (-1);	/* Basically sets the path */
	
	count[0] = S->H.n_records;
	for (i = 0; i < F->n_out_columns; i++) {	/* Only loop over columns that are desired */
		c  = F->order[i].set;	/* Determine set and item */
		id = F->order[i].item;
		/* Use the attribute scale & offset to adjust the values */
		scale = S->H.info[c].col[id].factor;
		offset = S->H.info[c].col[id].offset;
		if (F->use_corrections[c]) {	/* TRUE by default, but this can be turned off by changing this parameter in F */
			/* Combine effect of main and 2nd scale factors and 2nd scale and 2nd offset into one offset */
			scale *= S->H.info[c].col[id].corr_factor;
			offset = S->H.info[c].col[id].offset * S->H.info[c].col[id].corr_factor + S->H.info[c].col[id].corr_offset;
		}
		if (S->H.info[c].col[id].text) {	/* Text variable */
			count[1] = S->H.info[c].col[id].text;	/* Get length of each string */
			text = (char *) GMT_memory (VNULL, count[0] * count[1], sizeof (char), "MGD77_Read_File_cdf");
			if (S->H.info[c].col[id].constant) {	/* Scalar, must read one and then replicate */
				MGD77_nc_status (nc_get_vara_schar (F->nc_id, S->H.info[c].col[id].var_id, start, &count[1], (signed char *)text));
				for (k = 1; k < (int)count[0]; k++) strncpy (&text[k*count[1]], text, count[1]);	/* Replicate one string */
			}
			else	/* Get all individual strings */
				MGD77_nc_status (nc_get_vara_schar (F->nc_id, S->H.info[c].col[id].var_id, start, count, (signed char *)text));
			S->values[i] = (void *)text;
			S->H.info[c].bit_pattern |= MGD77_this_bit[id];		/* We return this data field */
		}
		else if (S->H.no_time && !strcmp (S->H.info[c].col[id].abbrev, "time")) {	/* Fake NaN time and bit_pattern not set */
			values = (double *) GMT_memory (VNULL, count[0], sizeof (double), "MGD77_Read_File_cdf");
			for (k = 0; k < (int)count[0]; k++) values[k] = GMT_d_NaN;
			S->values[i] = (void *)values;
		}
		else {
			values = (double *) GMT_memory (VNULL, count[0], sizeof (double), "MGD77_Read_File_cdf");
			if (S->H.info[c].col[id].constant) {	/* Scalar, must read one and then replicate */
				MGD77_nc_status (nc_get_var1_double (F->nc_id, S->H.info[c].col[id].var_id, start, values));
				MGD77_do_scale_offset_after_read (values, 1, scale, offset, MGD77_NaN_val[S->H.info[c].col[id].type]);	/* Just modify one point */
				for (k = 1; k < (int)count[0]; k++) values[k] = values[0];
			}
			else {	/* Read entire array */
				MGD77_nc_status (nc_get_vara_double (F->nc_id, S->H.info[c].col[id].var_id, start, count, values));
				MGD77_do_scale_offset_after_read (values, count[0], scale, offset, MGD77_NaN_val[S->H.info[c].col[id].type]);
			}
			S->values[i] = (void *)values;
			S->H.info[c].bit_pattern |= MGD77_this_bit[id];		/* We return this data field */
		}
	}

	/* Look for optional bit flags to read */
	
	for (k = 0; k < MGD77_N_SETS; k++) {
		if (F->use_flags[k] && nc_inq_varid (F->nc_id, flagname[k], &id) == NC_NOERR) {	/* There are bitflags for this set and we want them */
			flags = (unsigned int *) GMT_memory (VNULL, count[0], sizeof (unsigned int), "MGD77_Read_File_cdf");
			MGD77_nc_status (nc_get_vara_int (F->nc_id, id, start, count, (int *)flags));
			S->flags[k] = flags;
		}
	}
	S->n_fields = F->n_out_columns;

	return (MGD77_NO_ERROR);
}

int MGD77_Read_Data_Record_cdf (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, double dvals[], char *tvals[])
{	/* Returns a single record from a MGD77+ netCDF file.  Two important conditions:
	 * 1. You must specify record number via F->rec_no before calling this function
	 * 2. You must have preallocated enough space for the dvals and tvals arrays.
	 */
	 
	int i, c, id, n_val, n_txt;
	size_t start;

	for (i = n_val = n_txt = 0; i < F->n_out_columns; i++) {
		c  = F->order[i].set;
		id = F->order[i].item;
		H->info[c].bit_pattern |= MGD77_this_bit[id];			/* We return this data field */
		start = (H->info[c].col[id].constant) ? 0 : F->rec_no;	/* Scalar, must read first and then copy */
		if (H->info[c].col[id].text) {	/* Text variable */
			MGD77_nc_status (nc_get_vara_schar (F->nc_id, H->info[c].col[id].var_id, &start, (size_t *)&H->info[c].col[id].text, (signed char *)tvals[n_txt++]));
		}
		else {
			MGD77_nc_status (nc_get_var1_double (F->nc_id, H->info[c].col[id].var_id, &start, &dvals[n_val]));
			MGD77_do_scale_offset_after_read (&dvals[n_val], 1, H->info[c].col[id].factor, H->info[c].col[id].offset, MGD77_NaN_val[H->info[c].col[id].type]);
			n_val++;
		}
	}
	return (MGD77_NO_ERROR);
}

int MGD77_Write_Data_Record_cdf (struct MGD77_CONTROL *F, struct MGD77_HEADER *H, double dvals[], char *tvals[])
{	/* Writes a single record to a MGD77+ netCDF file.  One important conditions:
	 * 1. You must specify record number via F->rec_no before calling this function
	 */
	 
	int i, c, id, n_val, n_txt;
	double single_val;
	size_t start;

	for (i = n_val = n_txt = 0; i < F->n_out_columns; i++) {
		c  = F->order[i].set;
		id = F->order[i].item;
		H->info[c].bit_pattern |= MGD77_this_bit[id];			/* We return this data field */
		start = (H->info[c].col[id].constant) ? 0 : F->rec_no;	/* Scalar, must write first to rec */
		if (H->info[c].col[id].text) {	/* Text variable */
			MGD77_nc_status (nc_put_vara_schar (F->nc_id, H->info[c].col[id].var_id, &start, (size_t *)&H->info[c].col[id].text, (signed char *)tvals[n_txt++]));
		}
		else {
			single_val = dvals[n_val++];
			MGD77_do_scale_offset_before_write (&single_val, &single_val, 1, H->info[c].col[id].factor, H->info[c].col[id].offset, H->info[c].col[id].type);
			MGD77_nc_status (nc_put_var1_double (F->nc_id, H->info[c].col[id].var_id, &start, &single_val));
		}
	}
	return (MGD77_NO_ERROR);
}

int MGD77_Remove_E77 (struct MGD77_CONTROL *F)
{
	/* Will remove all traces of E77 attributes in this file (in redef mode) */
	
	int var_id, n_vars;
	
	MGD77_Reset_Header_Params (F);				/* Remove any previously revised header parameters */

	MGD77_nc_status (nc_inq_nvars (F->nc_id, &n_vars));
	for (var_id = 0; var_id < n_vars; var_id++) {		/* For all variables, try to remove factor & offset attributes */
		nc_del_att (F->nc_id, var_id, "corr_factor");
		nc_del_att (F->nc_id, var_id, "corr_offset");
	}
	
	return (nc_inq_varid (F->nc_id, "MGD77_flags", &var_id) == NC_NOERR);	/* TRUE if there are old E77 bitflags */
}

void MGD77_Free (struct MGD77_DATASET *S)
{
	int i;
	
	for (i = 0; i < S->n_fields; i++) GMT_free ((void *)S->values[i]);
	for (i = 0; i < MGD77_N_SETS; i++) if (S->flags[i]) GMT_free ((void *)S->flags[i]);
	GMT_free ((void *)S->H.mgd77);
}

struct MGD77_DATASET *MGD77_Create_Dataset ()
{
	struct MGD77_DATASET *S;
	
	S = (struct MGD77_DATASET *) GMT_memory (VNULL, 1, sizeof (struct MGD77_DATASET), GMT_program);
	return (S);
}

BOOLEAN MGD77_dbl_are_constant (double x[], int n, double limits[2])
{	/* Determine if the values in x[] are all the same, and sets actual range limits */
	int i;
	BOOLEAN constant = TRUE;
	double last;
	
	limits[0] = limits[1] = x[0];
	if (n == 1) return (constant);
	
	i = 0;
	while (i < n && GMT_is_dnan (x[i])) i++;	/* i is now at first non-NaN value (if any) */
	if (i == n) return (constant);			/* All are NaN */
	last = limits[0] = limits[1] = x[i];
	for (i++; i < n; i++) {
		if (GMT_is_dnan (x[i])) continue;
		if (x[i] != last) constant = FALSE;
		if (x[i] < limits[0]) limits[0] = x[i];	/* New lower value */
		if (x[i] > limits[1]) limits[1] = x[i];	/* New upper value */
		last = x[i];
	}
	return (constant);
}

BOOLEAN MGD77_txt_are_constant (char *txt, int n, int width)
{
	int i = 0;
	
	if (n == 1) return (TRUE);
	
	for (i = 2; i < n; i++) if (strncmp (&txt[i*width], &txt[(i-1)*width], width)) return (FALSE);
	return (TRUE);
}

void MGD77_do_scale_offset_after_read (double x[], int n, double scale, double offset, double nan_val)
{
	int k;
	BOOLEAN check_nan;
	
	check_nan = !GMT_is_dnan (nan_val);
	if (! (scale == 1.0 && offset == 0.0)) {
		if (offset == 0.0) {	/*  Just do scaling */
			for (k = 0; k < n; k++) x[k] = (check_nan && x[k] == nan_val) ? GMT_d_NaN : x[k] * scale;
		}
		else if (scale == 1.0) {	/* Just do offset */
			for (k = 0; k < n; k++) x[k] = (check_nan && x[k] == nan_val) ? GMT_d_NaN : x[k] + offset;
		}
		else {					/* Scaling and offset */
			for (k = 0; k < n; k++) x[k] = (check_nan && x[k] == nan_val) ? GMT_d_NaN : (x[k] * scale) + offset;
		}
	}
	else
		for (k = 0; k < n; k++) if (check_nan && x[k] == nan_val) x[k] = GMT_d_NaN;
	
}

int MGD77_do_scale_offset_before_write (double new[], const double x[], int n, double scale, double offset, int type)
{	/* Here we apply the various scale/offsets to fit the data in a smaller data type.
	 * We also replace NaNs with special values that represent NaNs for the saved data
	 * type, and finally replace transformed values that fall outside the valid range
	 * with NaN, and report the number of such problems.
	 */
	int k, n_crap = 0;
	double nan_val, lo_val, hi_val, i_scale;
	
	nan_val = MGD77_NaN_val[type];
	lo_val = MGD77_Low_val[type];
	hi_val = MGD77_High_val[type];
	
	if (! (scale == 1.0 && offset == 0.0)) {		/* Must do our own data scaling to ensure healthy rounding */
		if (offset == 0.0) {	/*  Just do scaling */
			i_scale = 1.0 / scale;
			for (k = 0; k < n; k++) {
				if (GMT_is_dnan (x[k]))
					new[k] = nan_val;
				else {
					new[k] = (type < NC_FLOAT) ? rint (x[k] * i_scale) : x[k] * i_scale;
					if (new[k] < lo_val || new[k] > hi_val) {
						new[k] = nan_val;
						n_crap++;
					}
				}
			}
		}
		else if (scale == 1.0) {	/* Just do offset */
			for (k = 0; k < n; k++) {
				if (GMT_is_dnan (x[k]))
					new[k] = nan_val;
				else {
					new[k] = (type < NC_FLOAT) ? rint (x[k] - offset) : x[k] - offset;
					if (new[k] < lo_val || new[k] > hi_val) {
						new[k] = nan_val;
						n_crap++;
					}
				}
			}
		}
		else {					/* Scaling and offset */
			i_scale = 1.0 / scale;
			for (k = 0; k < n; k++) {
				if (GMT_is_dnan (x[k]))
					new[k] = nan_val;
				else {
					new[k] = (type < NC_FLOAT) ? rint ((x[k] - offset) * i_scale) : (x[k] - offset) * i_scale;
					if (new[k] < lo_val || new[k] > hi_val) {
						new[k] = nan_val;
						n_crap++;
					}
				}
			}
		}
	}
	else {	/* Just replace NaNs and check range */
		for (k = 0; k < n; k++) {
			if (GMT_is_dnan (x[k]))
				new[k] = nan_val;
			else {
				new[k] = (type < NC_FLOAT) ? rint (x[k]) : x[k];
				if (new[k] < lo_val || new[k] > hi_val) {
					new[k] = nan_val;
					n_crap++;
				}
			}
		}
	}
	return (n_crap);
}

void MGD77_nc_status (int status)
{	/* This function checks the return status of a netcdf function and takes
	 * appropriate action if the status != NC_NOERR
	 */
	if (status != NC_NOERR) {
		fprintf (stderr, "%s: %s\n", GMT_program, nc_strerror (status));
		GMT_exit (EXIT_FAILURE);
	}
}


/* CARTER TABLE ROUTINES */

int MGD77_carter_init (struct MGD77_CARTER *C)
{
	/* This routine must be called once before using carter table stuff.
	It reads the carter.d file and loads the appropriate arrays.
	It sets carter_not_initialized = FALSE upon successful completion
	and returns 0.  If failure occurs, it returns -1.  */

	FILE *fp = NULL;
	char buffer [BUFSIZ];
	int  i;

	memset ((void *)C, 0, sizeof (struct MGD77_CARTER));
	
	/* Read the correction table:  */

	sprintf (buffer, "%s%cmgg%ccarter.d", GMT_SHAREDIR, DIR_DELIM, DIR_DELIM);
	if ( (fp = fopen (buffer, "r")) == NULL) {
                fprintf (stderr,"MGD77_carter_init:  Cannot open r %s\n", buffer);
                return (-1);
        }

	for (i = 0; i < 4; i++) fgets (buffer, BUFSIZ, fp);	/* Skip 4 headers */
	fgets (buffer, BUFSIZ, fp);

	if ((i = atoi (buffer)) != N_CARTER_CORRECTIONS) {
		fprintf (stderr, "MGD77_carter_init:  Incorrect correction key (%d), should be %d\n", i, N_CARTER_CORRECTIONS);
                return(-1);
	}

        for (i = 0; i < N_CARTER_CORRECTIONS; i++) {
                if (!fgets (buffer, BUFSIZ, fp)) {
			fprintf (stderr, "MGD77_carter_init:  Could not read correction # %d\n", i);
			return (-1);
		}
                C->carter_correction[i] = atoi (buffer);
        }

	/* Read the offset table:  */

	fgets (buffer, BUFSIZ, fp);	/* Skip header */
	fgets (buffer, BUFSIZ, fp);

	if ((i = atoi (buffer)) != N_CARTER_OFFSETS) {
		fprintf (stderr, "MGD77_carter_init:  Incorrect offset key (%d), should be %d\n", i, N_CARTER_OFFSETS);
                return (-1);
	}

        for (i = 0; i < N_CARTER_OFFSETS; i++) {
                 if (!fgets (buffer, BUFSIZ, fp)) {
			fprintf (stderr, "MGD77_carter_init:  Could not read offset # %d\n", i);
			return (-1);
		}
                C->carter_offset[i] = atoi (buffer);
        }

	/* Read the zone table:  */

	fgets (buffer, BUFSIZ, fp);	/* Skip header */
	fgets (buffer, BUFSIZ, fp);

	if ((i = atoi (buffer)) != N_CARTER_BINS) {
		fprintf (stderr, "MGD77_carter_init:  Incorrect zone key (%d), should be %d\n", i, N_CARTER_BINS);
                return (-1);
	}

        for (i = 0; i < N_CARTER_BINS; i++) {
                 if (!fgets (buffer, BUFSIZ, fp)) {
			fprintf (stderr, "MGD77_carter_init:  Could not read offset # %d\n", i);
			return (-1);
		}
                C->carter_zone[i] = atoi (buffer);
        }
        fclose (fp);

	/* Get here when all is well.  */

	C->initialized = TRUE;
	
	return (MGD77_NO_ERROR);
}

int MGD77_carter_get_bin (double lon, double lat, int *bin)
{
	/* Calculate Carter bin #.  Returns 0 if OK, -1 if error.  */

	int latdeg, londeg;

	if (lat < -90.0 || lat > 90.0) {
		fprintf (stderr, "MGD77 ERROR: in MGD77_carter_get_bin:  Latitude domain error (%g)\n", lat);
		return (-1);
	}
	while (lon >= 360.0) lon -= 360.0;
	while (lon < 0.0) lon += 360.0;
	latdeg = (int)floor (lat + 90.0);
	if (latdeg == 180) latdeg = 179;	/* Map north pole to previous row  */

	londeg = (int)floor (lon);
	*bin = 360 * latdeg + londeg;

	return (MGD77_NO_ERROR);
}

int MGD77_carter_get_zone (int bin, struct MGD77_CARTER *C, int *zone)
{
	/* Sets value pointed to by zone to the Carter zone corresponding to
		the bin "bin".  Returns 0 if successful, -1 if bin out of
		range.  */

	if (!C->initialized && MGD77_carter_init(C) ) {
		fprintf (stderr, "MGD77 ERROR: in MGD77_carter_get_zone:  Initialization failure.\n");
		return (-1);
	}

	if (bin < 0 || bin >= N_CARTER_BINS) {
		fprintf (stderr, "MGD77 ERROR: in MGD77_carter_get_zone:  Input bin out of range [0-%d]: %d.\n", N_CARTER_BINS, bin);
		return (-1);
	}
	*zone = C->carter_zone[bin];
	return (MGD77_NO_ERROR);
}

int MGD77_carter_depth_from_xytwt (double lon, double lat, double twt_in_msec, struct MGD77_CARTER *C, double *depth_in_corr_m)
{
	int bin, zone, ierr;
	
	if ((ierr = MGD77_carter_get_bin (lon, lat, &bin))) return (ierr);
	if ((ierr = MGD77_carter_get_zone (bin, C, &zone))) return (ierr);
	if ((ierr = MGD77_carter_depth_from_twt (zone, twt_in_msec, C, depth_in_corr_m))) return (ierr);
	return (MGD77_NO_ERROR);
}

int MGD77_carter_twt_from_xydepth (double lon, double lat, double twt_in_msec, struct MGD77_CARTER *C, double *depth_in_corr_m)
{
	int bin, zone, ierr;
	
	if ((ierr = MGD77_carter_get_bin (lon, lat, &bin))) return (ierr);
	if ((ierr = MGD77_carter_get_zone (bin, C, &zone))) return (ierr);
	if ((ierr = MGD77_carter_depth_from_twt (zone, twt_in_msec, C, depth_in_corr_m))) return (ierr);
	return (MGD77_NO_ERROR);
}

int MGD77_carter_depth_from_twt (int zone, double twt_in_msec, struct MGD77_CARTER *C, double *depth_in_corr_m)
{
	/* Given two-way travel time of echosounder in milliseconds, and
		Carter Zone number, finds depth in Carter corrected meters.
		Returns (0) if OK, -1 if error condition.  */

	int	i, nominal_z1500, low_hundred, part_in_100;

	if (GMT_is_dnan (twt_in_msec)) {
		*depth_in_corr_m = GMT_d_NaN;
		return (0);
	}
	if (!C->initialized && MGD77_carter_init(C) ) {
		fprintf (stderr,"MGD77 ERROR: in MGD77_carter_depth_from_twt:  Initialization failure.\n");
		return (-1);
	}
	if (zone < 1 || zone > N_CARTER_ZONES) {
		fprintf (stderr,"MGD77 ERROR: in MGD77_carter_depth_from_twt:  Zone out of range [1-%d]: %d\n", N_CARTER_ZONES, zone);
		return (-1);
	}
	if (twt_in_msec < 0.0) {
		fprintf (stderr,"MGD77 ERROR: in MGD77_carter_depth_from_twt:  Negative twt: %g msec\n", twt_in_msec);
		return (-1);
	}

	nominal_z1500 = irint (0.75 * twt_in_msec);

	if (nominal_z1500 <= 100.0) {	/* There is no correction in water this shallow.  */
		*depth_in_corr_m = nominal_z1500;
		return (MGD77_NO_ERROR);
	}

	low_hundred = (int) floor (nominal_z1500 / 100.0);
	i = C->carter_offset[zone-1] + low_hundred - 1;	/* -1 'cause .f indices */
	
	if (i >= (C->carter_offset[zone] - 1) ) {
		fprintf (stderr, "MGD77 ERROR: in MGD77_carter_depth_from_twt:  twt too big: %g msec\n", twt_in_msec);
		return (-1);
	}

	part_in_100 = irint (fmod (nominal_z1500, 100.0));

	if (part_in_100 > 0.0) {	/* We have to interpolate the table  */

		if ( i == (C->carter_offset[zone] - 2) ) {
			fprintf (stderr, "GMT ERROR: in MGD77_carter_depth_from_twt:  twt too big: %g msec\n", twt_in_msec);
			return (-1);
		}

		*depth_in_corr_m = (double)C->carter_correction[i] + 0.01 * part_in_100 * (C->carter_correction[i+1] - C->carter_correction[i]);
		return (MGD77_NO_ERROR);
	}
	else {
		*depth_in_corr_m = (double)C->carter_correction[i];
		return (MGD77_NO_ERROR);
	}
}


int MGD77_carter_twt_from_depth (int zone, double depth_in_corr_m, struct MGD77_CARTER *C, double *twt_in_msec)
{
	/*  Given Carter zone and depth in Carter corrected meters,
	finds the two-way travel time of the echosounder in milliseconds.
	Returns -1 upon error, 0 upon success.  */

	int	min, max, guess;
	double	fraction;

	if (GMT_is_dnan (depth_in_corr_m)) {
		*twt_in_msec = GMT_d_NaN;
		return (0);
	}
	if (!C->initialized && MGD77_carter_init(C) ) {
		fprintf(stderr,"MGD77 ERROR: in MGD77_carter_twt_from_depth:  Initialization failure.\n");
		return (-1);
	}
	if (zone < 1 || zone > N_CARTER_ZONES) {
		fprintf (stderr,"MGD77 ERROR: in MGD77_carter_twt_from_depth:  Zone out of range [1-%d]: %d\n", N_CARTER_ZONES, zone);
		return (-1);
	}
	if (depth_in_corr_m < 0.0) {
		fprintf(stderr,"MGD77 ERROR: in MGD77_carter_twt_from_depth:  Negative depth: %g m\n", depth_in_corr_m);
		return(-1);
	}

	if (depth_in_corr_m <= 100.0) {	/* No correction applies.  */
		*twt_in_msec = 1.33333 * depth_in_corr_m;
		return (MGD77_NO_ERROR);
	}

	max = C->carter_offset[zone] - 2;
	min = C->carter_offset[zone-1] - 1;

	if (depth_in_corr_m > C->carter_correction[max]) {
		fprintf (stderr, "MGD77 ERROR: in MGD77_carter_twt_from_depth:  Depth too big: %g m.\n", depth_in_corr_m);
		return (-1);
	}

	if (depth_in_corr_m == C->carter_correction[max]) {	/* Hit last entry in table exactly  */
		*twt_in_msec = 133.333 * (max - min);
		return (MGD77_NO_ERROR);
	}

	guess = irint ((depth_in_corr_m / 100.0)) + min;
	if (guess > max) guess = max;
	while (guess < max && C->carter_correction[guess] < depth_in_corr_m) guess++;
	while (guess > min && C->carter_correction[guess] > depth_in_corr_m) guess--;

	if (depth_in_corr_m == C->carter_correction[guess]) {	/* Hit a table value exactly  */
		*twt_in_msec = 133.333 * (guess - min);
		return (MGD77_NO_ERROR);
	}
	fraction = ((double)(depth_in_corr_m - C->carter_correction[guess]) / (double)(C->carter_correction[guess+1] - C->carter_correction[guess]));
	*twt_in_msec = 133.333 * (guess - min + fraction);
	return (MGD77_NO_ERROR);
}

double MGD77_carter_correction (double lon, double lat, double twt_in_msec, struct MGD77_CARTER *C)
{	/* Returns the correction term to be subtracted from uncorrected depth to give corrected depth */

	double depth_in_corr_m;
	
	MGD77_carter_depth_from_xytwt (lon, lat, twt_in_msec, C, &depth_in_corr_m);
	return (twt_in_msec * 0.75 - depth_in_corr_m);
}

/* IGRF function from Susan Macmillian's FORTRAN via J. Luis f2c translation */

/*--------------------------------------------------------------------*
  *
  * C version of Susan Macmillian original fortran code
  * Computes the magnetic field components from the IGRF 1900-2010 model
  * Author:	Joaquim Luis
  * Date: 	18 Aug  2004
  * Revised: 	5  Oct 2006
  * Extracted & Modified for MGD77 by P. Wessel.
  *
*--------------------------------------------------------------------*/

int MGD77_igrf10syn (int isv, double date, int itype, double alt, double elong, double lat, double *out) {
 /*     This is a synthesis routine for the 10th generation IGRF as agreed
  *     in December 2004 by IAGA Working Group V-MOD. It is valid 1900.0 to
  *     2010.0 inclusive. Values for dates from 1945.0 to 2000.0 inclusive are
  *     definitve, otherwise they are non-definitive.
  *   INPUT
  *     isv   = 0 if main-field values are required
  *     isv   = 1 if secular variation values are required
  *     date  = year A.D. Must be greater than or equal to 1900.0 and
  *             less than or equal to 2015.0. Warning message is given
  *             for dates greater than 2010.0. Must be double precision.
  *     itype = 1 if geodetic (spheroid)
  *     itype = 2 if geocentric (sphere)
  *     alt   = height in km above sea level if itype = 1
  *           = distance from centre of Earth in km if itype = 2 (>3485 km)
  *     lat   = latitude (90-90)
  *     elong = east-longitude (0-360) -- it works also in [-180;+180]
  *   OUTPUT
  *     out[0] F  = total intensity (nT) if isv = 0, rubbish if isv = 1
  *     out[1] H  = horizontal intensity (nT)
  *     out[2] X  = north component (nT) if isv = 0, nT/year if isv = 1
  *     out[3] Y  = east component (nT) if isv = 0, nT/year if isv = 1
  *     out[4] Z  = vertical component (nT) if isv = 0, nT/year if isv = 1
  *     out[5] D  = declination
  *     out[6] I  = inclination
  *
  *     To get the other geomagnetic elements (D, I, H and secular
  *     variations dD, dH, dI and dF) use routines ptoc and ptocsv.
  *
  *     Adapted from 8th generation version to include new maximum degree for
  *     main-field models for 2000.0 and onwards and use WGS84 spheroid instead
  *     of International Astronomical Union 1966 spheroid as recommended by IAGA
  *     in July 2003. Reference radius remains as 6371.2 km - it is NOT the mean
  *     radius (= 6371.0 km) but 6371.2 km is what is used in determining the
  *     coefficients. Adaptation by Susan Macmillan, August 2003 (for
  *     9th generation) and December 2004.
  *
  *	Joaquim Luis 1-MARS-2005
  *	Converted to C (with help of f2c, which explains the ugliness)
  *     1995.0 coefficients as published in igrf9coeffs.xls and igrf10coeffs.xls 
  *     used - (Kimmo Korhonen spotted 1 nT difference in 11 coefficients) 
  *     Susan Macmillan July 2005 (PW update Oct 2006) 
  */

     struct IGRF {
	double e_1[3060];
     };
     /* Initialized data */
     static struct IGRF equiv_22 = {
       {-31543.,-2298., 5922., -677., 2905.,-1061.,  924., 1121., /* g0 (1900) */
         1022.,-1469., -330., 1256.,    3.,  572.,  523.,  876.,
          628.,  195.,  660.,  -69., -361., -210.,  134.,  -75.,
         -184.,  328., -210.,  264.,   53.,    5.,  -33.,  -86.,
         -124.,  -16.,    3.,   63.,   61.,   -9.,  -11.,   83.,
         -217.,    2.,  -58.,  -35.,   59.,   36.,  -90.,  -69.,
           70.,  -55.,  -45.,    0.,  -13.,   34.,  -10.,  -41.,
           -1.,  -21.,   28.,   18.,  -12.,    6.,  -22.,   11.,
            8.,    8.,   -4.,  -14.,   -9.,    7.,    1.,  -13.,
            2.,    5.,   -9.,   16.,    5.,   -5.,    8.,  -18.,
            8.,   10.,  -20.,    1.,   14.,  -11.,    5.,   12.,
           -3.,    1.,   -2.,   -2.,    8.,    2.,   10.,   -1.,
           -2.,   -1.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    6.,   -4.,    4.,    0.,
            0.,   -2.,    2.,    4.,    2.,    0.,    0.,   -6.,
       -31464.,-2298., 5909., -728., 2928.,-1086., 1041., 1065., /* g1 (1905) */
         1037.,-1494., -357., 1239.,   34.,  635.,  480.,  880.,
          643.,  203.,  653.,  -77., -380., -201.,  146.,  -65.,
         -192.,  328., -193.,  259.,   56.,   -1.,  -32.,  -93.,
         -125.,  -26.,   11.,   62.,   60.,   -7.,  -11.,   86.,
         -221.,    4.,  -57.,  -32.,   57.,   32.,  -92.,  -67.,
           70.,  -54.,  -46.,    0.,  -14.,   33.,  -11.,  -41.,
            0.,  -20.,   28.,   18.,  -12.,    6.,  -22.,   11.,
            8.,    8.,   -4.,  -15.,   -9.,    7.,    1.,  -13.,
            2.,    5.,   -8.,   16.,    5.,   -5.,    8.,  -18.,
            8.,   10.,  -20.,    1.,   14.,  -11.,    5.,   12.,
           -3.,    1.,   -2.,   -2.,    8.,    2.,   10.,    0.,
           -2.,   -1.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    6.,   -4.,    4.,    0.,
            0.,   -2.,    2.,    4.,    2.,    0.,    0.,   -6.,
       -31354.,-2297., 5898., -769., 2948.,-1128., 1176., 1000., /* g2 (1910) */
         1058.,-1524., -389., 1223.,   62.,  705.,  425.,  884.,
          660.,  211.,  644.,  -90., -400., -189.,  160.,  -55.,
         -201.,  327., -172.,  253.,   57.,   -9.,  -33., -102.,
         -126.,  -38.,   21.,   62.,   58.,   -5.,  -11.,   89.,
         -224.,    5.,  -54.,  -29.,   54.,   28.,  -95.,  -65.,
           71.,  -54.,  -47.,    1.,  -14.,   32.,  -12.,  -40.,
            1.,  -19.,   28.,   18.,  -13.,    6.,  -22.,   11.,
            8.,    8.,   -4.,  -15.,   -9.,    6.,    1.,  -13.,
            2.,    5.,   -8.,   16.,    5.,   -5.,    8.,  -18.,
            8.,   10.,  -20.,    1.,   14.,  -11.,    5.,   12.,
           -3.,    1.,   -2.,   -2.,    8.,    2.,   10.,    0.,
           -2.,   -1.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    6.,   -4.,    4.,    0.,
            0.,   -2.,    2.,    4.,    2.,    0.,    0.,   -6.,
       -31212.,-2306., 5875., -802., 2956.,-1191., 1309.,  917., /* g3 (1915) */
         1084.,-1559., -421., 1212.,   84.,  778.,  360.,  887.,
          678.,  218.,  631., -109., -416., -173.,  178.,  -51.,
         -211.,  327., -148.,  245.,   58.,  -16.,  -34., -111.,
         -126.,  -51.,   32.,   61.,   57.,   -2.,  -10.,   93.,
         -228.,    8.,  -51.,  -26.,   49.,   23.,  -98.,  -62.,
           72.,  -54.,  -48.,    2.,  -14.,   31.,  -12.,  -38.,
            2.,  -18.,   28.,   19.,  -15.,    6.,  -22.,   11.,
            8.,    8.,   -4.,  -15.,   -9.,    6.,    2.,  -13.,
            3.,    5.,   -8.,   16.,    6.,   -5.,    8.,  -18.,
            8.,   10.,  -20.,    1.,   14.,  -11.,    5.,   12.,
           -3.,    1.,   -2.,   -2.,    8.,    2.,   10.,    0.,
           -2.,   -1.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    6.,   -4.,    4.,    0.,
            0.,   -2.,    1.,    4.,    2.,    0.,    0.,   -6.,
       -31060.,-2317., 5845., -839., 2959.,-1259., 1407.,  823., /* g4 (1920) */
         1111.,-1600., -445., 1205.,  103.,  839.,  293.,  889.,
          695.,  220.,  616., -134., -424., -153.,  199.,  -57.,
         -221.,  326., -122.,  236.,   58.,  -23.,  -38., -119.,
         -125.,  -62.,   43.,   61.,   55.,    0.,  -10.,   96.,
         -233.,   11.,  -46.,  -22.,   44.,   18., -101.,  -57.,
           73.,  -54.,  -49.,    2.,  -14.,   29.,  -13.,  -37.,
            4.,  -16.,   28.,   19.,  -16.,    6.,  -22.,   11.,
            7.,    8.,   -3.,  -15.,   -9.,    6.,    2.,  -14.,
            4.,    5.,   -7.,   17.,    6.,   -5.,    8.,  -19.,
            8.,   10.,  -20.,    1.,   14.,  -11.,    5.,   12.,
           -3.,    1.,   -2.,   -2.,    9.,    2.,   10.,    0.,
           -2.,   -1.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    6.,   -4.,    4.,    0.,
            0.,   -2.,    1.,    4.,    3.,    0.,    0.,   -6.,
       -30926.,-2318., 5817., -893., 2969.,-1334., 1471.,  728., /* g5 (1925) */
         1140.,-1645., -462., 1202.,  119.,  881.,  229.,  891.,
          711.,  216.,  601., -163., -426., -130.,  217.,  -70.,
         -230.,  326.,  -96.,  226.,   58.,  -28.,  -44., -125.,
         -122.,  -69.,   51.,   61.,   54.,    3.,   -9.,   99.,
         -238.,   14.,  -40.,  -18.,   39.,   13., -103.,  -52.,
           73.,  -54.,  -50.,    3.,  -14.,   27.,  -14.,  -35.,
            5.,  -14.,   29.,   19.,  -17.,    6.,  -21.,   11.,
            7.,    8.,   -3.,  -15.,   -9.,    6.,    2.,  -14.,
            4.,    5.,   -7.,   17.,    7.,   -5.,    8.,  -19.,
            8.,   10.,  -20.,    1.,   14.,  -11.,    5.,   12.,
           -3.,    1.,   -2.,   -2.,    9.,    2.,   10.,    0.,
           -2.,   -1.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    6.,   -4.,    4.,    0.,
            0.,   -2.,    1.,    4.,    3.,    0.,    0.,   -6.,
       -30805.,-2316., 5808., -951., 2980.,-1424., 1517.,  644., /* g6 (1930) */
         1172.,-1692., -480., 1205.,  133.,  907.,  166.,  896.,
          727.,  205.,  584., -195., -422., -109.,  234.,  -90.,
         -237.,  327.,  -72.,  218.,   60.,  -32.,  -53., -131.,
         -118.,  -74.,   58.,   60.,   53.,    4.,   -9.,  102.,
         -242.,   19.,  -32.,  -16.,   32.,    8., -104.,  -46.,
           74.,  -54.,  -51.,    4.,  -15.,   25.,  -14.,  -34.,
            6.,  -12.,   29.,   18.,  -18.,    6.,  -20.,   11.,
            7.,    8.,   -3.,  -15.,   -9.,    5.,    2.,  -14.,
            5.,    5.,   -6.,   18.,    8.,   -5.,    8.,  -19.,
            8.,   10.,  -20.,    1.,   14.,  -12.,    5.,   12.,
           -3.,    1.,   -2.,   -2.,    9.,    3.,   10.,    0.,
           -2.,   -2.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    6.,   -4.,    4.,    0.,
            0.,   -2.,    1.,    4.,    3.,    0.,    0.,   -6.,
       -30715.,-2306., 5812.,-1018., 2984.,-1520., 1550.,  586., /* g7 (1935) */
         1206.,-1740., -494., 1215.,  146.,  918.,  101.,  903.,
          744.,  188.,  565., -226., -415.,  -90.,  249., -114.,
         -241.,  329.,  -51.,  211.,   64.,  -33.,  -64., -136.,
         -115.,  -76.,   64.,   59.,   53.,    4.,   -8.,  104.,
         -246.,   25.,  -25.,  -15.,   25.,    4., -106.,  -40.,
           74.,  -53.,  -52.,    4.,  -17.,   23.,  -14.,  -33.,
            7.,  -11.,   29.,   18.,  -19.,    6.,  -19.,   11.,
            7.,    8.,   -3.,  -15.,   -9.,    5.,    1.,  -15.,
            6.,    5.,   -6.,   18.,    8.,   -5.,    7.,  -19.,
            8.,   10.,  -20.,    1.,   15.,  -12.,    5.,   11.,
           -3.,    1.,   -3.,   -2.,    9.,    3.,   11.,    0.,
           -2.,   -2.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    6.,   -4.,    4.,    0.,
            0.,   -1.,    2.,    4.,    3.,    0.,    0.,   -6.,
       -30654.,-2292., 5821.,-1106., 2981.,-1614., 1566.,  528., /* g8 (1940) */
         1240.,-1790., -499., 1232.,  163.,  916.,   43.,  914.,
          762.,  169.,  550., -252., -405.,  -72.,  265., -141.,
         -241.,  334.,  -33.,  208.,   71.,  -33.,  -75., -141.,
         -113.,  -76.,   69.,   57.,   54.,    4.,   -7.,  105.,
         -249.,   33.,  -18.,  -15.,   18.,    0., -107.,  -33.,
           74.,  -53.,  -52.,    4.,  -18.,   20.,  -14.,  -31.,
            7.,   -9.,   29.,   17.,  -20.,    5.,  -19.,   11.,
            7.,    8.,   -3.,  -14.,  -10.,    5.,    1.,  -15.,
            6.,    5.,   -5.,   19.,    9.,   -5.,    7.,  -19.,
            8.,   10.,  -21.,    1.,   15.,  -12.,    5.,   11.,
           -3.,    1.,   -3.,   -2.,    9.,    3.,   11.,    1.,
           -2.,   -2.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    6.,   -4.,    4.,    0.,
            0.,   -1.,    2.,    4.,    3.,    0.,    0.,   -6.,
       -30594.,-2285., 5810.,-1244., 2990.,-1702., 1578.,  477., /* g9 (1945) */
         1282.,-1834., -499., 1255.,  186.,  913.,  -11.,  944.,
          776.,  144.,  544., -276., -421.,  -55.,  304., -178.,
         -253.,  346.,  -12.,  194.,   95.,  -20.,  -67., -142.,
         -119.,  -82.,   82.,   59.,   57.,    6.,    6.,  100.,
         -246.,   16.,  -25.,   -9.,   21.,  -16., -104.,  -39.,
           70.,  -40.,  -45.,    0.,  -18.,    0.,    2.,  -29.,
            6.,  -10.,   28.,   15.,  -17.,   29.,  -22.,   13.,
            7.,   12.,   -8.,  -21.,   -5.,  -12.,    9.,   -7.,
            7.,    2.,  -10.,   18.,    7.,    3.,    2.,  -11.,
            5.,  -21.,  -27.,    1.,   17.,  -11.,   29.,    3.,
           -9.,   16.,    4.,   -3.,    9.,   -4.,    6.,   -3.,
            1.,   -4.,    8.,   -3.,   11.,    5.,    1.,    1.,
            2.,  -20.,   -5.,   -1.,   -1.,   -6.,    8.,    6.,
           -1.,   -4.,   -3.,   -2.,    5.,    0.,   -2.,   -2.,
       -30554.,-2250., 5815.,-1341., 2998.,-1810., 1576.,  381., /* ga (1950) */
         1297.,-1889., -476., 1274.,  206.,  896.,  -46.,  954.,
          792.,  136.,  528., -278., -408.,  -37.,  303., -210.,
         -240.,  349.,    3.,  211.,  103.,  -20.,  -87., -147.,
         -122.,  -76.,   80.,   54.,   57.,   -1.,    4.,   99.,
         -247.,   33.,  -16.,  -12.,   12.,  -12., -105.,  -30.,
           65.,  -55.,  -35.,    2.,  -17.,    1.,    0.,  -40.,
           10.,   -7.,   36.,    5.,  -18.,   19.,  -16.,   22.,
           15.,    5.,   -4.,  -22.,   -1.,    0.,   11.,  -21.,
           15.,   -8.,  -13.,   17.,    5.,   -4.,   -1.,  -17.,
            3.,   -7.,  -24.,   -1.,   19.,  -25.,   12.,   10.,
            2.,    5.,    2.,   -5.,    8.,   -2.,    8.,    3.,
          -11.,    8.,   -7.,   -8.,    4.,   13.,   -1.,   -2.,
           13.,  -10.,   -4.,    2.,    4.,   -3.,   12.,    6.,
            3.,   -3.,    2.,    6.,   10.,   11.,    3.,    8.,
       -30500.,-2215., 5820.,-1440., 3003.,-1898., 1581.,  291., /* gb (1955) */
         1302.,-1944., -462., 1288.,  216.,  882.,  -83.,  958.,
          796.,  133.,  510., -274., -397.,  -23.,  290., -230.,
         -229.,  360.,   15.,  230.,  110.,  -23.,  -98., -152.,
         -121.,  -69.,   78.,   47.,   57.,   -9.,    3.,   96.,
         -247.,   48.,   -8.,  -16.,    7.,  -12., -107.,  -24.,
           65.,  -56.,  -50.,    2.,  -24.,   10.,   -4.,  -32.,
            8.,  -11.,   28.,    9.,  -20.,   18.,  -18.,   11.,
            9.,   10.,   -6.,  -15.,  -14.,    5.,    6.,  -23.,
           10.,    3.,   -7.,   23.,    6.,   -4.,    9.,  -13.,
            4.,    9.,  -11.,   -4.,   12.,   -5.,    7.,    2.,
            6.,    4.,   -2.,    1.,   10.,    2.,    7.,    2.,
           -6.,    5.,    5.,   -3.,   -5.,   -4.,   -1.,    0.,
            2.,   -8.,   -3.,   -2.,    7.,   -4.,    4.,    1.,
           -2.,   -3.,    6.,    7.,   -2.,   -1.,    0.,   -3.,
       -30421.,-2169., 5791.,-1555., 3002.,-1967., 1590.,  206., /* gc (1960) */
         1302.,-1992., -414., 1289.,  224.,  878., -130.,  957.,
          800.,  135.,  504., -278., -394.,    3.,  269., -255.,
         -222.,  362.,   16.,  242.,  125.,  -26., -117., -156.,
         -114.,  -63.,   81.,   46.,   58.,  -10.,    1.,   99.,
         -237.,   60.,   -1.,  -20.,   -2.,  -11., -113.,  -17.,
           67.,  -56.,  -55.,    5.,  -28.,   15.,   -6.,  -32.,
            7.,   -7.,   23.,   17.,  -18.,    8.,  -17.,   15.,
            6.,   11.,   -4.,  -14.,  -11.,    7.,    2.,  -18.,
           10.,    4.,   -5.,   23.,   10.,    1.,    8.,  -20.,
            4.,    6.,  -18.,    0.,   12.,   -9.,    2.,    1.,
            0.,    4.,   -3.,   -1.,    9.,   -2.,    8.,    3.,
            0.,   -1.,    5.,    1.,   -3.,    4.,    4.,    1.,
            0.,    0.,   -1.,    2.,    4.,   -5.,    6.,    1.,
            1.,   -1.,   -1.,    6.,    2.,    0.,    0.,   -7.,
       -30334.,-2119., 5776.,-1662., 2997.,-2016., 1594.,  114., /* gd (1965) */
         1297.,-2038., -404., 1292.,  240.,  856., -165.,  957.,
          804.,  148.,  479., -269., -390.,   13.,  252., -269.,
         -219.,  358.,   19.,  254.,  128.,  -31., -126., -157.,
          -97.,  -62.,   81.,   45.,   61.,  -11.,    8.,  100.,
         -228.,   68.,    4.,  -32.,    1.,   -8., -111.,   -7.,
           75.,  -57.,  -61.,    4.,  -27.,   13.,   -2.,  -26.,
            6.,   -6.,   26.,   13.,  -23.,    1.,  -12.,   13.,
            5.,    7.,   -4.,  -12.,  -14.,    9.,    0.,  -16.,
            8.,    4.,   -1.,   24.,   11.,   -3.,    4.,  -17.,
            8.,   10.,  -22.,    2.,   15.,  -13.,    7.,   10.,
           -4.,   -1.,   -5.,   -1.,   10.,    5.,   10.,    1.,
           -4.,   -2.,    1.,   -2.,   -3.,    2.,    2.,    1.,
           -5.,    2.,   -2.,    6.,    4.,   -4.,    4.,    0.,
            0.,   -2.,    2.,    3.,    2.,    0.,    0.,   -6.,
       -30220.,-2068., 5737.,-1781., 3000.,-2047., 1611.,   25., /* ge (1970) */
         1287.,-2091., -366., 1278.,  251.,  838., -196.,  952.,
          800.,  167.,  461., -266., -395.,   26.,  234., -279.,
         -216.,  359.,   26.,  262.,  139.,  -42., -139., -160.,
          -91.,  -56.,   83.,   43.,   64.,  -12.,   15.,  100.,
         -212.,   72.,    2.,  -37.,    3.,   -6., -112.,    1.,
           72.,  -57.,  -70.,    1.,  -27.,   14.,   -4.,  -22.,
            8.,   -2.,   23.,   13.,  -23.,   -2.,  -11.,   14.,
            6.,    7.,   -2.,  -15.,  -13.,    6.,   -3.,  -17.,
            5.,    6.,    0.,   21.,   11.,   -6.,    3.,  -16.,
            8.,   10.,  -21.,    2.,   16.,  -12.,    6.,   10.,
           -4.,   -1.,   -5.,    0.,   10.,    3.,   11.,    1.,
           -2.,   -1.,    1.,   -3.,   -3.,    1.,    2.,    1.,
           -5.,    3.,   -1.,    4.,    6.,   -4.,    4.,    0.,
            1.,   -1.,    0.,    3.,    3.,    1.,   -1.,   -4.,
       -30100.,-2013., 5675.,-1902., 3010.,-2067., 1632.,  -68., /* gf (1975) */
         1276.,-2144., -333., 1260.,  262.,  830., -223.,  946.,
          791.,  191.,  438., -265., -405.,   39.,  216., -288.,
         -218.,  356.,   31.,  264.,  148.,  -59., -152., -159.,
          -83.,  -49.,   88.,   45.,   66.,  -13.,   28.,   99.,
         -198.,   75.,    1.,  -41.,    6.,   -4., -111.,   11.,
           71.,  -56.,  -77.,    1.,  -26.,   16.,   -5.,  -14.,
           10.,    0.,   22.,   12.,  -23.,   -5.,  -12.,   14.,
            6.,    6.,   -1.,  -16.,  -12.,    4.,   -8.,  -19.,
            4.,    6.,    0.,   18.,   10.,  -10.,    1.,  -17.,
            7.,   10.,  -21.,    2.,   16.,  -12.,    7.,   10.,
           -4.,   -1.,   -5.,   -1.,   10.,    4.,   11.,    1.,
           -3.,   -2.,    1.,   -3.,   -3.,    1.,    2.,    1.,
           -5.,    3.,   -2.,    4.,    5.,   -4.,    4.,   -1.,
            1.,   -1.,    0.,    3.,    3.,    1.,   -1.,   -5.,
       -29992.,-1956., 5604.,-1997., 3027.,-2129., 1663., -200., /* gg (1980) */
         1281.,-2180., -336., 1251.,  271.,  833., -252.,  938.,
          782.,  212.,  398., -257., -419.,   53.,  199., -297.,
         -218.,  357.,   46.,  261.,  150.,  -74., -151., -162.,
          -78.,  -48.,   92.,   48.,   66.,  -15.,   42.,   93.,
         -192.,   71.,    4.,  -43.,   14.,   -2., -108.,   17.,
           72.,  -59.,  -82.,    2.,  -27.,   21.,   -5.,  -12.,
           16.,    1.,   18.,   11.,  -23.,   -2.,  -10.,   18.,
            6.,    7.,    0.,  -18.,  -11.,    4.,   -7.,  -22.,
            4.,    9.,    3.,   16.,    6.,  -13.,   -1.,  -15.,
            5.,   10.,  -21.,    1.,   16.,  -12.,    9.,    9.,
           -5.,   -3.,   -6.,   -1.,    9.,    7.,   10.,    2.,
           -6.,   -5.,    2.,   -4.,   -4.,    1.,    2.,    0.,
           -5.,    3.,   -2.,    6.,    5.,   -4.,    3.,    0.,
            1.,   -1.,    2.,    4.,    3.,    0.,    0.,   -6.,
       -29873.,-1905., 5500.,-2072., 3044.,-2197., 1687., -306., /* gi (1985) */
         1296.,-2208., -310., 1247.,  284.,  829., -297.,  936.,
          780.,  232.,  361., -249., -424.,   69.,  170., -297.,
         -214.,  355.,   47.,  253.,  150.,  -93., -154., -164.,
          -75.,  -46.,   95.,   53.,   65.,  -16.,   51.,   88.,
         -185.,   69.,    4.,  -48.,   16.,   -1., -102.,   21.,
           74.,  -62.,  -83.,    3.,  -27.,   24.,   -2.,   -6.,
           20.,    4.,   17.,   10.,  -23.,    0.,   -7.,   21.,
            6.,    8.,    0.,  -19.,  -11.,    5.,   -9.,  -23.,
            4.,   11.,    4.,   14.,    4.,  -15.,   -4.,  -11.,
            5.,   10.,  -21.,    1.,   15.,  -12.,    9.,    9.,
           -6.,   -3.,   -6.,   -1.,    9.,    7.,    9.,    1.,
           -7.,   -5.,    2.,   -4.,   -4.,    1.,    3.,    0.,
           -5.,    3.,   -2.,    6.,    5.,   -4.,    3.,    0.,
            1.,   -1.,    2.,    4.,    3.,    0.,    0.,   -6.,
       -29775.,-1848., 5406.,-2131., 3059.,-2279., 1686., -373., /* gj (1990) */
         1314.,-2239., -284., 1248.,  293.,  802., -352.,  939.,
          780.,  247.,  325., -240., -423.,   84.,  141., -299.,
         -214.,  353.,   46.,  245.,  154., -109., -153., -165.,
          -69.,  -36.,   97.,   61.,   65.,  -16.,   59.,   82.,
         -178.,   69.,    3.,  -52.,   18.,    1.,  -96.,   24.,
           77.,  -64.,  -80.,    2.,  -26.,   26.,    0.,   -1.,
           21.,    5.,   17.,    9.,  -23.,    0.,   -4.,   23.,
            5.,   10.,   -1.,  -19.,  -10.,    6.,  -12.,  -22.,
            3.,   12.,    4.,   12.,    2.,  -16.,   -6.,  -10.,
            4.,    9.,  -20.,    1.,   15.,  -12.,   11.,    9.,
           -7.,   -4.,   -7.,   -2.,    9.,    7.,    8.,    1.,
           -7.,   -6.,    2.,   -3.,   -4.,    2.,    2.,    1.,
           -5.,    3.,   -2.,    6.,    4.,   -4.,    3.,    0.,
            1.,   -2.,    3.,    3.,    3.,   -1.,    0.,   -6.,
       -29692.,-1784., 5306.,-2200., 3070.,-2366., 1681., -413., /* gk (1995) */
         1335.,-2267., -262., 1249.,  302.,  759., -427.,  940.,
          780.,  262.,  290., -236., -418.,   97.,  122., -306.,
         -214.,  352.,   46.,  235.,  165., -118., -143., -166.,
          -55.,  -17.,  107.,   68.,   67.,  -17.,   68.,   72.,
         -170.,   67.,   -1.,  -58.,   19.,    1.,  -93.,   36.,
           77.,  -72.,  -69.,    1.,  -25.,   28.,    4.,    5.,
           24.,    4.,   17.,    8.,  -24.,   -2.,   -6.,   25.,
            6.,   11.,   -6.,  -21.,   -9.,    8.,  -14.,  -23.,
            9.,   15.,    6.,   11.,   -5.,  -16.,   -7.,   -4.,
            4.,    9.,  -20.,    3.,   15.,  -10.,   12.,    8.,
           -6.,   -8.,   -8.,   -1.,    8.,   10.,    5.,   -2.,
           -8.,   -8.,    3.,   -3.,   -6.,    1.,    2.,    0.,
           -4.,    4.,   -1.,    5.,    4.,   -5.,    2.,   -1.,
            2.,   -2.,    5.,    1.,    1.,   -2.,    0.,   -7.,
            0.,    0.,    0.,    0.,    0.,    0.,    0.,    0.,
            0.,    0.,    0.,    0.,    0.,    0.,    0.,    0.,
            0.,    0.,    0.,    0.,    0.,    0.,    0.,    0.,
            0.,    0.,    0.,    0.,    0.,    0.,    0.,    0.,
            0.,    0.,    0.,    0.,    0.,    0.,    0.,    0.,
            0.,    0.,    0.,    0.,    0.,    0.,    0.,    0.,
            0.,    0.,    0.,    0.,    0.,    0.,    0.,    0.,
            0.,    0.,    0.,    0.,    0.,    0.,    0.,    0.,
            0.,    0.,    0.,    0.,    0.,    0.,    0.,    0.,
            0.,    0.,    0.,
       -29619.4,-1728.2, 5186.1,-2267.7, 3068.4,-2481.6, 1670.9, /* gl (2000) */
         -458.0, 1339.6,-2288.0, -227.6, 1252.1,  293.4,  714.5,
         -491.1,  932.3,  786.8,  272.6,  250.0, -231.9, -403.0,
          119.8,  111.3, -303.8, -218.8,  351.4,   43.8,  222.3,
          171.9, -130.4, -133.1, -168.6,  -39.3,  -12.9,  106.3,
           72.3,   68.2,  -17.4,   74.2,   63.7, -160.9,   65.1,
           -5.9,  -61.2,   16.9,    0.7,  -90.4,   43.8,   79.0,
          -74.0,  -64.6,    0.0,  -24.2,   33.3,    6.2,    9.1,
           24.0,    6.9,   14.8,    7.3,  -25.4,   -1.2,   -5.8,
           24.4,    6.6,   11.9,   -9.2,  -21.5,   -7.9,    8.5,
          -16.6,  -21.5,    9.1,   15.5,    7.0,    8.9,   -7.9,
          -14.9,   -7.0,   -2.1,    5.0,    9.4,  -19.7,    3.0,
           13.4,   -8.4,   12.5,    6.3,   -6.2,   -8.9,   -8.4,
           -1.5,    8.4,    9.3,    3.8,   -4.3,   -8.2,   -8.2,
            4.8,   -2.6,   -6.0,    1.7,    1.7,    0.0,   -3.1,
            4.0,   -0.5,    4.9,    3.7,   -5.9,    1.0,   -1.2,
            2.0,   -2.9,    4.2,    0.2,    0.3,   -2.2,   -1.1,
           -7.4,    2.7,   -1.7,    0.1,   -1.9,    1.3,    1.5,
           -0.9,   -0.1,   -2.6,    0.1,    0.9,   -0.7,   -0.7,
            0.7,   -2.8,    1.7,   -0.9,    0.1,   -1.2,    1.2,
           -1.9,    4.0,   -0.9,   -2.2,   -0.3,   -0.4,    0.2,
            0.3,    0.9,    2.5,   -0.2,   -2.6,    0.9,    0.7,
           -0.5,    0.3,    0.3,    0.0,   -0.3,    0.0,   -0.4,
            0.3,   -0.1,   -0.9,   -0.2,   -0.4,   -0.4,    0.8,
           -0.2,   -0.9,   -0.9,    0.3,    0.2,    0.1,    1.8,
           -0.4,   -0.4,    1.3,   -1.0,   -0.4,   -0.1,    0.7,
            0.7,   -0.4,    0.3,    0.3,    0.6,   -0.1,    0.3,
            0.4,   -0.2,    0.0,   -0.5,    0.1,   -0.9,
       -29556.8,-1671.8, 5080.0,-2340.5, 3047.0,-2594.9, 1656.9, /* gm (2005) */
         -516.7, 1335.7,-2305.3, -200.4, 1246.8,  269.3,  674.4,
         -524.5,  919.8,  798.2,  281.4,  211.5, -225.8, -379.5,
          145.7,  100.2, -304.7, -227.6,  354.4,   42.7,  208.8,
          179.8, -136.6, -123.0, -168.3,  -19.5,  -14.1,  103.6,
           72.9,   69.6,  -20.2,   76.6,   54.7, -151.1,   63.7,
          -15.0,  -63.4,   14.7,    0.0,  -86.4,   50.3,   79.8,
          -74.4,  -61.4,   -1.4,  -22.5,   38.6,    6.9,   12.3,
           25.4,    9.4,   10.9,    5.5,  -26.4,    2.0,   -4.8,
           24.8,    7.7,   11.2,  -11.4,  -21.0,   -6.8,    9.7,
          -18.0,  -19.8,   10.0,   16.1,    9.4,    7.7,  -11.4,
          -12.8,   -5.0,   -0.1,    5.6,    9.8,  -20.1,    3.6,
           12.9,   -7.0,   12.7,    5.0,   -6.7,  -10.8,   -8.1,
           -1.3,    8.1,    8.7,    2.9,   -6.7,   -7.9,   -9.2,
            5.9,   -2.2,   -6.3,    2.4,    1.6,    0.2,   -2.5,
            4.4,   -0.1,    4.7,    3.0,   -6.5,    0.3,   -1.0,
            2.1,   -3.4,    3.9,   -0.9,   -0.1,   -2.3,   -2.2,
           -8.0,    2.9,   -1.6,    0.3,   -1.7,    1.4,    1.5,
           -0.7,   -0.2,   -2.4,    0.2,    0.9,   -0.7,   -0.6,
            0.5,   -2.7,    1.8,   -1.0,    0.1,   -1.5,    1.0,
           -2.0,    4.1,   -1.4,   -2.2,   -0.3,   -0.5,    0.3,
            0.3,    0.9,    2.3,   -0.4,   -2.7,    1.0,    0.6,
           -0.4,    0.4,    0.5,    0.0,   -0.3,    0.0,   -0.4,
            0.3,    0.0,   -0.8,   -0.4,   -0.4,    0.0,    1.0,
           -0.2,   -0.9,   -0.7,    0.3,    0.3,    0.3,    1.7,
           -0.4,   -0.5,    1.2,   -1.0,   -0.4,    0.0,    0.7,
            0.7,   -0.3,    0.2,    0.4,    0.6,   -0.1,    0.4,
            0.4,   -0.2,   -0.1,   -0.5,   -0.3,   -1.0,
           8.8,  10.8, -21.3, -15.0,  -6.9, -23.3,  -1.0, -14.0, /* gp (2007) */
          -0.3,  -3.1,   5.4,  -0.9,  -6.5,  -6.8,  -2.0,  -2.5,
           2.8,   2.0,  -7.1,   1.8,   5.9,   5.6,  -3.2,   0.0,
          -2.6,   0.4,   0.1,  -3.0,   1.8,  -1.2,   2.0,   0.2,
           4.5,  -0.6,  -1.0,  -0.8,   0.2,  -0.4,  -0.2,  -1.9,
           2.1,  -0.4,  -2.1,  -0.4,  -0.4,  -0.2,   1.3,   0.9,
          -0.4,   0.0,   0.8,  -0.2,   0.4,   1.1,   0.1,   0.6,
           0.2,   0.4,  -0.9,  -0.5,  -0.3,   0.9,   0.3,  -0.2,
           0.2,  -0.2,  -0.2,   0.2,   0.2,   0.2,  -0.2,   0.4,
           0.2,   0.2,   0.5,  -0.3,  -0.7,   0.5,   0.5,   0.4,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,
           0.0,   0.0,   0.0}
	 };
#define gh ((double *)&equiv_22)

	int i, j, k, l, m, n, ll, lm, kmx, nmx, nc;
	double cd, cl[13], tc, ct, sd, fn = 0.0, gn = 0.0, fm, sl[13];
	double rr, st, one, gmm, rho, two, three, ratio;
	double p[105], q[105], r, t, a2, b2, colat;
	double H, F, X = 0, Y = 0, Z = 0, dec, dip;
	
	if (date < 1900.0 || date > 2015.0) {
		fprintf (stderr, "%s: Your date (%g) is outside valid extrapolated range for IGRF (1900-2015)\n", GMT_program, date);
		return (MGD77_BAD_IGRFDATE);
	}
	
	if (date < 2005.) {
		t = 0.2 * (date - 1900.);
		ll = (int) t;
		one = (double) ll;
		t -= one;
		if (date < 1995.) {
			nmx = 10;
			nc = nmx * (nmx + 2);
			ll = nc * ll;
			kmx = (nmx + 1) * (nmx + 2) / 2;
		} else {
			nmx = 13;
			nc = nmx * (nmx + 2);
			ll = (int) ((date - 1995.) * .2);
			ll = nc * ll + 2280;
			kmx = (nmx + 1) * (nmx + 2) / 2;
		}
		tc = 1. - t;
		if (isv == 1) {
			tc = -.2;
			t = .2;
		}
	}
	else {
		t = date - 2005.;
		tc = 1.;
		if (isv == 1) {
			t = 1.;
			tc = 0.;
		}
		ll = 2670;
		nmx = 13;
		nc = nmx * (nmx + 2);
		kmx = (nmx + 1) * (nmx + 2) / 2;
	}
	r = alt;
	colat = 90. - lat;
	one = colat * D2R;
	ct = cos(one);
	st = sin(one);
	one = elong * D2R;
	cl[0] = cos(one);
	sl[0] = sin(one);
	cd = 1.;
	sd = 0.;
	l = 1;
	m = 1;
	n = 0;
	if (itype == 1) { /* conversion from geodetic to geocentric coordinates (using the WGS84 spheroid) */
		a2 = 40680631.6;
		b2 = 40408296.0;
		one = a2 * st * st;
		two = b2 * ct * ct;
		three = one + two;
		rho = sqrt(three);
		r = sqrt(alt * (alt + rho * 2.) + (a2 * one + b2 * two) / three);
		cd = (alt + rho) / r;
		sd = (a2 - b2) / rho * ct * st / r;
		one = ct;
		ct = ct * cd - st * sd;
		st = st * cd + one * sd;
		ratio = 6371.2 / r;
	}
	else
		ratio = 1.0;
	rr = ratio * ratio;

	/* computation of Schmidt quasi-normal coefficients p and x(=q) */

	p[0] = 1.;
	p[2] = st;
	q[0] = 0.;
	q[2] = ct;
	for (k = 2; k <= kmx; ++k) {
		if (n < m) {
			m = 0;
			n++;
			rr *= ratio;
			fn = (double) n;
			gn = (double) (n - 1);
		}
		fm = (double) m;
		if (k != 3) {
			if (m == n) {
				one = sqrt(1. - .5 / fm);
				j = k - n - 1;
				p[k-1] = one * st * p[j-1];
				q[k-1] = one * (st * q[j-1] + ct * p[j-1]);
				cl[m-1] = cl[m-2] * cl[0] - sl[m-2] * sl[0];
				sl[m-1] = sl[m-2] * cl[0] + cl[m-2] * sl[0];
			}
			else {
				gmm = (double) (m * m);
				one = sqrt(fn * fn - gmm);
				two = sqrt(gn * gn - gmm) / one;
				three = (fn + gn) / one;
				i = k - n;
				j = i - n + 1;
				p[k-1] = three * ct * p[i-1] - two * p[j-1];
				q[k-1] = three * (ct * q[i-1] - st * p[i-1]) - two * q[j-1];
			}
		}

		/* synthesis of x, y and z in geocentric coordinates */

		lm = ll + l;
		one = (tc * gh[lm-1] + t * gh[lm+nc-1]) * rr;
		if (m == 0) {
			X += one * q[k-1];
			Z -= (fn + 1.) * one * p[k-1];
			l++;
		}
		else {
			two = (tc * gh[lm] + t * gh[lm+nc]) * rr;
			three = one * cl[m-1] + two * sl[m - 1];
			X += three * q[k-1];
			Z -= (fn + 1.) * three * p[k-1];
			if (st != 0.)
				Y += (one * sl[m-1] - two * cl[m-1]) * fm * p[k-1] / st;
			else
				Y += (one * sl[m-1] - two * cl[m-1]) * q[k-1] * ct;
			l += 2;
		}
		m++;
	}

	/* conversion to coordinate system specified by itype */
	one = X;
	X = X * cd + Z * sd;
	Z = Z * cd - one * sd;
	H = sqrt(X*X + Y*Y);
	F = sqrt(H*H + Z*Z);
	dec = atan2(Y,X)*R2D;   dip = atan2(Z,H)*R2D;
	out[0] = F;		out[1] = H;
	out[2] = X;		out[3] = Y;
	out[4] = Z;
	out[5] = dec;	out[6] = dip;
	
	return (MGD77_NO_ERROR);
}

void MGD77_IGF_text (FILE *fp, int version)
{	
	switch (version) {
		case 1:	/* Heiskanen 1924 model */
			fprintf (fp, "g = %.12g * [1 + %.6f * sin^2(lat) - %.7f * sin^2(2*lat) + %.6f * cos^2(lat) * cos^2(lon-18)]\n", 
				MGD77_IGF24_G0, MGD77_IGF24_G1, MGD77_IGF24_G2, MGD77_IGF24_G3);
			break;
		case 2:	/* International 1930 model */
			fprintf (fp, "g = %.12g * [1 + %.7f * sin^2(lat) - %.7f * sin^2(2*lat)]\n", MGD77_IGF30_G0, MGD77_IGF30_G1, MGD77_IGF30_G2 );
			break;
		case 3:	/* IAG 1967 model */
			fprintf (fp, "g = %.12g * [1 + %.7f * sin^2(lat) - %.7f * sin^2(2*lat)]\n", MGD77_IGF67_G0, MGD77_IGF67_G1, MGD77_IGF67_G2);
			break;
		case 4:	/* IAG 1980 model */
			fprintf (fp, "g = %.12g * [(1 + %.14g * sin^2(lat)) / sqrt (1 - %.14g * sin^2(lat))]\n", MGD77_IGF80_G0, MGD77_IGF80_G1, MGD77_IGF80_G2);
			break;
		default:	/* Unrecognized */
			fprintf (fp, "Unrecognized theoretical gravity formula code (%d)\n", version);
			break;
	}
}

double MGD77_Theoretical_Gravity (double lon, double lat, int version)
{
	/* Calculates theoretical gravity given latitude and which formulae to use.
	 * Version is as per MGD-77 Docs:
	 *
	 * 1 : Heiskanen, 1924: 978052       (1 + 0.005285  sin2 (lat) - 0.0000070 sin2 (2*lat) + 0.000027 cos2 (lat) cos2 (lon - 18))
	 * 2 : IGF 1930 :       978049       (1 + 0.0052884 sin2 (lat) - 0.0000059 sin2 (2*lat)
	 * 3 : IAG 1967 :       978031.846   (1 + 0.0053024 sin2 (lat) - 0.0000058 sin2 (2*lat)
	 * 4 : IAG 1980 :       978032.67714 ((1 + 0.00193185138639 sin2 (lat)) / sqrt (1 - 0.00669437999013 sin2 (lat)))
	 */

	double slat2, clat2, s2lat, clon2, g;
	
	lat *= D2R;		/* Convert to radians */
	slat2 = sin (lat);
	slat2 *= slat2;		/* Squared sin (latitude) */
	
	switch (version) {
		case 1:	/* Heiskanen 1924 model */
			clon2 = cosd (lon - 18.0);
			clon2 *= clon2;			/* Squared cos (longitude - 18) */
			s2lat = sin (2.0 * lat);	/* sin of 2*lat */
			s2lat *= s2lat		;	/* Squared sin of 2*lat */
			clat2 = 1.0 - slat2;		/* Squared cos (latitude) */
			g = MGD77_IGF24_G0 * (1.0 + MGD77_IGF24_G1 * slat2 - MGD77_IGF24_G2 * s2lat + MGD77_IGF24_G3 * clat2 * clon2);
			break;
		case 2:	/* International 1930 model */
			s2lat = sin (2.0 * lat);	/* sin of 2*lat */
			s2lat *= s2lat		;	/* Squared sin of 2*lat */
			g = MGD77_IGF30_G0 * (1.0 + MGD77_IGF30_G1 * slat2 - MGD77_IGF30_G2 * s2lat);
			break;
		case 3:	/* IAG 1967 model */
			s2lat = sin (2.0 * lat);	/* sin of 2*lat */
			s2lat *= s2lat		;	/* Squared sin of 2*lat */
			g = MGD77_IGF67_G0 * (1.0 + MGD77_IGF67_G1 * slat2 - MGD77_IGF67_G2 * s2lat);
			break;
		case 4:	/* IAG 1980 model */
			g = MGD77_IGF80_G0 * ((1.0 + MGD77_IGF80_G1 * slat2) / sqrt (1.0 - MGD77_IGF80_G2 * slat2));
			break;
		default:	/* Unrecognized */
			g = GMT_d_NaN;
			fprintf (stderr, "%s: Unrecognized theoretical gravity formula code (%d)\n", GMT_program, version);
			break;
	}
	
	return (g);
}

/* Here lies the core functions used to parse the correction table
 * and apply the corrections to data before output in mgd77list
 */

void MGD77_Parse_Corrtable (struct MGD77_CONTROL *F, char *tablefile, char **cruises, int n_cruises, struct MGD77_CORRTABLE ***CORR)
{
	/* We seek to make the correction system very flexible, in particular
	 * since it is difficult to anticipate exactly what systematic trends
	 * will be detected via crossover analysis etc.  Thus, we build a modular
	 * system in which various basis functions (1, time, cos(lat), etc) can
	 * be specified and multiplied with correction constants and added together
	 * Thus, corrections are coded in the form:
	 *
	 * del_z = Factor[0] * pow (conv (scale * (Basis[0] - Origin[0])), Order[0]) +  ...
	 *
	 * where conv converts the argument (either none, cos, or sin).
	 * The number of terms depends on the number of factors given in the table,
	 * thus we implement this as a chain of structures.
	 *
	 * This function is called after we have secured the list of cruises to use,
	 * thus we pass the list in as an argument so we can determine the id of the
	 * current cruise.
	 *
	 * Each record looks like this:
	 * cruise abbrev term_1 term_2 ... term_n
	 */
	
	int cruise_id, id, i, pos, rec = 0;
	BOOLEAN skip;
	char line[BUFSIZ], name[GMT_TEXT_LEN], factor[GMT_TEXT_LEN], origin[GMT_TEXT_LEN], basis[BUFSIZ];
	char arguments[BUFSIZ], cruise[GMT_TEXT_LEN], word[BUFSIZ], *p, *f;
	struct MGD77_CORRTABLE **C_table;
	struct MGD77_CORRECTION *c, **previous;
	FILE *fp;
	
	if (!tablefile) {	/* Try default correction table */
		sprintf (line, "%s%cmgd77_corrections.d" , F->MGD77_HOME, DIR_DELIM);
		if ((fp = GMT_fopen (line, "r")) == NULL) {
			fprintf (stderr, "%s: No default MGD77 Correction table (%s) found!\n", GMT_program, line);
			GMT_exit (EXIT_FAILURE);
		}
	}
	else if ((fp = GMT_fopen (tablefile, "r")) == NULL) {
		fprintf (stderr, "%s: Correction table %s not found!\n", GMT_program, tablefile);
		GMT_exit (EXIT_FAILURE);
	}
	
	/* Allocate empty correction table */
	
	C_table = (struct MGD77_CORRTABLE **)GMT_memory (VNULL, n_cruises, sizeof (struct MGD77_CORRTABLE *), "MGD77_parse_corrtable");
	for (cruise_id = 0; cruise_id < n_cruises; cruise_id++) C_table[cruise_id] = (struct MGD77_CORRTABLE *)GMT_memory (VNULL, MGD77_SET_COLS, sizeof (struct MGD77_CORRTABLE), "MGD77_parse_corrtable");

	while (GMT_fgets (line, BUFSIZ, fp)) {
		rec++;
		if (line[0] == '#' || line[0] == '\0') continue;
		if (line[0] == '>') {	/* Cruise specified, get ID */
			GMT_chop (line);	/* Deal with CR/LF issues */
			sscanf (&line[1], "%s", name);
			cruise_id = MGD77_Find_Cruise_ID (name, cruises, n_cruises);
			skip = (cruise_id == -1); /* Not a cruise we are interested in at the moment */
			continue;
		}
		sscanf (line, "%s %s %[^\n]", cruise, name, arguments);
		if ((cruise_id = MGD77_Find_Cruise_ID (cruise, cruises, n_cruises)) == -1) continue; /* Not a cruise we are interested in at the moment */
		if ((id = MGD77_Get_Column (name, F)) == MGD77_NOT_SET) {
			fprintf (stderr, "%s: Column %s not found - requested by the correction table %s!\n", GMT_program, name, tablefile);
			GMT_exit (EXIT_FAILURE);
		}
		pos = 0;
		previous = &C_table[cruise_id][id].term;
		while (GMT_strtok (arguments, " ,\t", &pos, word)) {
			c = (struct MGD77_CORRECTION *)GMT_memory (VNULL, 1, sizeof (struct MGD77_CORRECTION), "MGD77_parse_corrtable");
			/* Each word p will be of the form factor*[cos|sin]([<scale>](<name>[-<origin>]))[^<power>] */
			if ((f = strchr (word, '*')) == NULL) {	/* No basis function, just a constant, the intercept term */
				c->factor = atof (word);
				c->modifier = (PFD) MGD77_Copy;
				c->origin = 0.0;
				c->power = c->scale = 1.0;
				c->id = -1;	/* Means it is jus a constant factor - no fancy calcuations needed */
			}
			else {	/* factor*basis */
				sscanf (word, "%[^*]*%s", factor, basis);
				p = basis;
				c->factor = atof (factor);
				if (p[0] == 'C' || p[0] == 'c') {	/* Need cosine transformation */
					c->modifier = (PFD) MGD77_Cosd;
					p += 3;
				}
				else if (p[0] == 'S' || p[0] == 's') {	/* Need sine transformation */
					c->modifier = (PFD) MGD77_Sind;
					p += 3;
				}
				else if (p[0] == 'E' || p[0] == 'e') {	/* Need exponential transformation */
					c->modifier = (PFD) exp;
					p += 3;
				}
				else					/* Nothing, just copy value */
					c->modifier = (PFD) MGD77_Copy;
				if (p[0] != '(') {
					fprintf (stderr, "%s: Correction table format error line %d, term = %s: Expected 1st opening parenthesis!\n", GMT_program, rec, arguments);
					GMT_exit (EXIT_FAILURE);
				}
				p++;
				c->scale = (p[0] == '(') ? 1.0 : atof (p);
				while (p && *p != '(') p++;	/* Skip the opening parentheses */
				if (p[0] != '(') {
					fprintf (stderr, "%s: Correction table format error line %d, term = %s: Expected 2nd opening parenthesis!\n", GMT_program, rec, arguments);
					GMT_exit (EXIT_FAILURE);
				}
				p++;
				if (strchr (p, '-')) {	/* Have (value-origin) */
					sscanf (p, "%[^-]-%[^)])", name, origin);
					c->origin = (origin[0] == 'T') ? GMT_d_NaN : atof (origin);	/* NaN means first use value in 1st record of the cruise */
				}
				else {			/* Just (value), origin == 0.0 */
					sscanf (p, "%[^)])", name);
					c->origin = 0.0;
				}
				if ((c->id = MGD77_Get_Column (name, F)) == MGD77_NOT_SET) {;	/* Not a regular column, check auxilliaries */
					for (i = 0; i < N_AUX; i++) if (!strcmp (name, aux_names[i])) c->id = i;
					if (c->id == MGD77_NOT_SET) { /* Not an auxilliary column either */
						fprintf (stderr, "%s: Column %s not found - requested by the correction table %s!\n", GMT_program, name, tablefile);
						GMT_exit (EXIT_FAILURE);
					}
					c->id += MGD77_MAX_COLS;	/* To flag this is an aux column */
				}
				c->power = ((f = strchr (p, '^'))) ? atof ((f+1)) : 1.0;	/* Get specified power or 1 */
			}
			*previous = c;			/* Hook to linked list of terms */
			previous = &((*previous)->next);	/* Get to end of list */
		}
	}
	GMT_fclose (fp);
	
	*CORR = C_table;		
}

void MGD77_Init_Correction (struct MGD77_CORRTABLE *CORR, double **value)
{	/* Call this once for each cruise to initialize parameter origin */
	int col;
	struct MGD77_CORRECTION *current;
	
	for (col = 0; col < MGD77_SET_COLS; col++) {
		for (current = CORR[col].term; current; current = current->next) {
			if (GMT_is_dnan (current->origin)) current->origin = value[current->id][0];
			if (GMT_is_dnan (current->origin)) {
				fprintf (stderr, "%s: Correction origin = T has NaN in 1st record, reset to 0!\n", GMT_program);
				current->origin = 0.0;
			}
		}
	}
}

double MGD77_Correction (struct MGD77_CORRECTION *C, double **value, double *aux, int rec)
{	/* Calculates the correction term for a single observation */
	double dz = 0.0, z;
	struct MGD77_CORRECTION *current;
	
	for (current = C; current; current = current->next) {
		if (current->id == -1) {	/* Just a constant */
			dz = current->factor;
		}
		else {
			z = (current->id >= MGD77_MAX_COLS) ? aux[current->id-MGD77_MAX_COLS] : value[current->id][rec];
			dz += current->factor * pow ((current->modifier) (current->scale * (z - current->origin)), current->power);
		}
	}
	return (dz);
}

double MGD77_Copy (double z) {
	/* Just returns its argument - used when no transformation is selected */
	return (z);
}

double MGD77_Cosd (double z) {
	/* cosine of degrees */
	return (cos (D2R * z));
}

double MGD77_Sind (double z) {
	/* sine of degrees */
	return (sin (D2R * z));
}

int MGD77_Find_Cruise_ID (char *name, char **cruises, int n_cruises)
{
	int low, high, mid, last = -1, way;
	
	low = 0;
	high = n_cruises;
	while (low < high) {
		mid = (low + high) / 2;
		if (mid == last) return (-1);	/* No such cruise */
		way = strcmp (name, cruises[mid]);
		if (way > 0)
			low = mid;
		else if (way < 0)
			high = mid;
		else 
			return (mid);
		last = mid;
	}
	return (low);
}
